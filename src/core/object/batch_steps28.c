/* Steps (0.157.0): the next ready function of 201 - 400 instructions: the fight HUD's stun gauges for
   fighters 1 and 2 (layout 0): their shades, lengths, flashing marks and the palette words they flash.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define PAL_DIRTY 0x092E12D8u   /* u32: set when the palette words below changed */

/* One side's stun gauge (cells at the HUD object's offsets; palette words `pal` from `src`). */
struct gauge { uint32_t full, full_add, bar, bar_add, bar_min, mark, timer; gaddr pal, src; uint16_t glyph[3]; };

static const struct gauge GAUGES[2] = {
    { 0x6B4, 0x59, 0x634, 0x110, 273, 0x734, 26, 0x092E14FCu, 0x092DFB9Cu, { 0xB9, 0xB8, 0xB7 } },
    { 0xCF4, 0,    0xC74, 0xB2,  179, 0xD74, 46, 0x092E153Cu, 0x092DFBDCu, { 0x5B, 0x5A, 0x59 } },
};

/* A side's gauge: its full mark (+556) and bar (+556 - +557, hidden below its start); with the health
   below 0 the palette back to normal and the mark hidden; else its hit flash (timer counting down:
   white 0xFBC8 at 0x20 and 0x2C, else normal; a new one 0x30 on +698 with +599) and, when the bar is
   short (+556 - +557 < 17), the blinking mark (by HUD +9) and the palette flashing with HUD +6. */
static void gauge(gaddr o, gaddr f, const struct gauge *g) {
    *H(o + g->full + 60) = (uint16_t)(S8(f + 556) + g->full_add);
    gaddr bar = o + g->bar;
    *H(bar + 60) = (uint16_t)(S8(f + 556) - S8(f + 557) + g->bar_add);
    if ((int32_t)*H(bar + 60) < (int32_t)g->bar_min) *H(bar + 60) = 0x8000;
    gaddr mark = o + g->mark, pal = g->pal, src = g->src;
    if (S16(f + 112) < 0) {
        *H(pal) = *H(src);
        *H(pal + 2) = *H(src + 2);
        *H(mark + 60) = 0x8000;
        return;
    }
    int8_t t = S8(o + g->timer);
    if (t != 0) {
        *B(o + g->timer) = (uint8_t)(t - 1);
        int8_t t2 = S8(o + g->timer);
        if (t2 == 0x20 || t2 == 0x2C) { *H(pal) = 0xFBC8; *H(pal + 2) = 0xFBC8; }
        else { *H(pal) = *H(src); *H(pal + 2) = *H(src + 2); }
        *W(PAL_DIRTY) = 1;
    } else if (S8(f + 698) != 0 && S8(f + 599) != 0) {
        *B(o + g->timer) = 0x30;
    }
    if (!(S8(f + 556) - S8(f + 557) < 17)) { *H(mark + 60) = 0x8000; return; }
    uint32_t blink = *B(o + 9);
    *H(mark + 60) = (blink & 1) ? g->glyph[0] : (blink & 3) ? g->glyph[1] : g->glyph[2];
    if (*H(o + 6) & 2) { *H(pal) = 0xFFFF; *H(pal + 2) = 0xFFFF; }
    else { *H(pal) = *H(src); *H(pal + 2) = *H(src + 2); }
    *W(PAL_DIRTY) = 1;
}

/* (hud): in layout 0 only: the gauges' shades (+0xF4 +36 - +51: 0 / 0 / 8, or 0x10 unpaused and
   unhalted); unless paused (GAME +89 / +145), each of fighters 1 and 2 not out (+313) nor in states
   0x820010 (+792) has its gauge drawn (fighter 2's only when fighter 1 isn't skipped by +313 / +792:
   both are checked in turn). */
SFA_FUNC(0888FF38, stun_gauges) {
    gaddr o = ARG(0);
    if (S16(BATTLE + 56) != 0) return;
    gaddr s = o + 0xF4;
    uint8_t v = (S8(BATTLE + 37) != 0 || S8(GAME + 89) != 0 || S8(GAME + 145) != 0) ? 8 : 0x10;
    *B(s + 48) = v; *B(s + 42) = 0; *B(s + 36) = 0;
    for (int i = 1; i <= 3; i++) { *B(s + 36 + i) = 0; *B(s + 42 + i) = 0; *B(s + 48 + i) = v; }
    if (S8(GAME + 89) != 0 || S8(GAME + 145) != 0) return;
    static const gaddr F[2] = { FIGHTER0, FIGHTER1 };
    for (int k = 0; k < 2; k++) {
        gaddr f = F[k];
        if (S8(f + 313) != 0 || (*W(f + 792) & 0x820010u)) { if (k == 0) continue; return; }
        gauge(o, f, &GAUGES[k]);
    }
}
