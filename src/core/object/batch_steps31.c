/* Steps (0.160.0): the fight HUD's health bars: who each side shows (by BATTLE +56), the bars' lengths
   with the red "damage" part catching up, their markers, and their colours (dimmed while halted or
   paused). Offsets stay numbers until something shows what they are.
 */
#include "step.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER1  0x092E3EA0u
#define FIGHTER2  0x092E4338u
#define FIGHTER3  0x092E47D0u
#define MARKS     0x08A26D40u   /* a side's marker by the fighter's +337 */
#define POS_X     0x08A26D76u   /* x nudge by the fighter's +338 */
#define POS_Y     0x08A26D4Cu
#define COLOURS   0x08A26CA8u   /* a bar's colour (BGR555) by HUD +16 / +36 */
#define LOW       0x39E         /* the colour at under 43 health */
#define FULL      0x90          /* a bar's longest */

/* HUD fields: +20 / +40 the two sides' health shown, +22 / +42 their red part, +24 / +44 the red part's
   step counters, +12 / +32 the fighters shown; bars at +0x1B4 (side 1), +0x1F4 (its red part), +0x7F4
   (side 2), +0x834 (its red part), shades of the frames at +0x274 / +0x874. */

static int32_t set16(gaddr a, int32_t v) { *H(a) = (uint16_t)v; return S16(a); }

/* A fighter's partner (+92) or, without one, the given stand-in. */
static gaddr partner(gaddr f, gaddr none) { gaddr p = *W(f + 92); return p ? p : none; }

/* A bar's length (+14) from a value; 0 shown as 1 while the value is above 0 (it can't be here). */
static int32_t bar_len(gaddr bar, gaddr value_at, int32_t v) {
    int32_t n = set16(bar + 14, v);
    if (n == 0 && S16(value_at) > 0) n = set16(bar + 14, 1);
    return n;
}

static void quads(gaddr s, uint8_t b, uint8_t g, uint8_t r) {
    *B(s + 36) = b; *B(s + 42) = g; *B(s + 48) = r;
    for (int i = 1; i < 4; i++) { *B(s + 36 + i) = b; *B(s + 42 + i) = g; *B(s + 48 + i) = r; }
}

/* A bar's colour: corners 36 / 42 / 48 and 39 / 45 / 51 take c (left) or the fixed shade, and
   37 - 38 / 43 - 44 / 49 - 50 the other; `sh` 0 dimmed (>> 8 / >> 3 / << 2, shade 0x78 / 0x70 / 0),
   1 bright (>> 7 / >> 2 / << 3, shade 0xF0 / 0xE0 / 0). Side 2 is mirrored. */
static void bar_colour(gaddr bar, uint32_t c, int bright, int mirrored) {
    uint8_t r = (uint8_t)(bright ? (c & 0x7C00) >> 7 : (c & 0x7C00) >> 8);
    uint8_t g = (uint8_t)(bright ? (c & 0x3E0) >> 2 : (c & 0x3E0) >> 3);
    uint8_t b = (uint8_t)(bright ? (c & 0x1F) << 3 : (c & 0x1F) << 2);
    uint8_t fb = bright ? 0xF0 : 0x78, fg = bright ? 0xE0 : 0x70;
    uint8_t ob = mirrored ? fb : b, og = mirrored ? fg : g, orr = mirrored ? 0 : r;   /* outer corners */
    uint8_t ib = mirrored ? b : fb, ig = mirrored ? g : fg, ir = mirrored ? r : 0;    /* inner */
    *B(bar + 36) = ob; *B(bar + 39) = ob; *B(bar + 37) = ib; *B(bar + 38) = ib;
    *B(bar + 42) = og; *B(bar + 45) = og; *B(bar + 43) = ig; *B(bar + 44) = ig;
    *B(bar + 48) = orr; *B(bar + 51) = orr; *B(bar + 49) = ir; *B(bar + 50) = ir;
}

static uint32_t colour_of(gaddr f, gaddr hud_index) {
    if (S16(f + 112) < 43) return LOW;
    return *H(COLOURS + (uint32_t)(int32_t)S8(hud_index) * 2);
}

/* (hud): leaf; keeps s0 - s4 in its guest frame as the original does. */
SFA_FUNC(0888F0EC, hud_health_bars) {
    gaddr o = ARG(0), sp = c->r[29] - 0x20;
    *W(sp) = c->r[16]; *W(sp + 4) = c->r[17]; *W(sp + 8) = c->r[18]; *W(sp + 12) = c->r[19];
    *W(sp + 16) = c->r[20];
    int32_t step1 = S16(o + 24), step2 = S16(o + 44);
    int32_t order = S16(BATTLE + 56);
    gaddr f1, f2;   /* side 1 / side 2's fighter (an unknown order leaves the original's raw values) */

    if (order == 0) {
        f1 = FIGHTER1; f2 = FIGHTER2;
        *H(o + 20) = *H(f1 + 112);
        *H(o + 40) = *H(f2 + 112);
        *B(o + 694) = 0; *B(o + 2294) = 0x40;
    } else {
        if (order == 1) {
            f1 = FIGHTER1; f2 = partner(FIGHTER1, FIGHTER3);
            *B(o + 694) = 0;
            *H(o + 42) = *H(f2 + 112);
            *B(o + 2294) = (uint8_t)*W(MARKS + (uint32_t)(int32_t)S8(f2 + 337) * 4 - 4);
        } else if (order == 2) {
            f2 = FIGHTER2; f1 = partner(FIGHTER2, FIGHTER3);
            *H(o + 22) = *H(f1 + 112);
            *B(o + 2294) = 0;
            *B(o + 694) = (uint8_t)*W(MARKS + (uint32_t)(int32_t)S8(f1 + 337) * 4 - 4);
        } else if (order == 3) {
            gaddr lead = *W(BATTLE + 136);
            if (S8(lead + 337) == 2) {
                f1 = FIGHTER3; f2 = partner(FIGHTER3, FIGHTER1);
                *B(o + 694) = 0x48;
                *H(o + 42) = *H(f2 + 112);
                *B(o + 2294) = (uint8_t)*W(MARKS + (uint32_t)(int32_t)S8(f2 + 337) * 4);
            } else {
                f2 = FIGHTER3; f1 = partner(FIGHTER3, FIGHTER1);
                *H(o + 22) = *H(f1 + 112);
                *B(o + 2294) = 0x48;
                *B(o + 694) = (uint8_t)*W(MARKS + (uint32_t)(int32_t)S8(f1 + 337) * 4);
            }
        } else {
            f1 = 2; f2 = (gaddr)order;   /* as the original: no fighters */
        }
        *H(o + 20) = *H(f1 + 112);
        *H(o + 40) = *H(f2 + 112);
    }
    int32_t hp1 = S16(o + 20), hp2 = S16(o + 40), red2 = S16(o + 42), red1 = S16(o + 22);

    *W(o + 12) = f1; *W(o + 32) = f2;
    *H(o + 752)  = (uint16_t)(*B(POS_X + (uint32_t)(int32_t)S8(f1 + 338)) + 0x17D);
    *H(o + 816)  = (uint16_t)(*B(POS_Y + (uint32_t)(int32_t)S8(f1 + 338)) + 0xF6);
    *H(o + 2352) = (uint16_t)(*B(POS_X + (uint32_t)(int32_t)S8(f2 + 338)) + 0x1AA);
    *H(o + 2416) = (uint16_t)(*B(POS_Y + (uint32_t)(int32_t)S8(f2 + 338)) + 0x125);

    int halted = S8(GAME + 89) != 0 || S8(GAME + 145) != 0;
    if (!halted) {   /* the red parts move every fourth frame */
        set16(o + 24, step1 + 1); set16(o + 44, step2 + 1);
        step1 = set16(o + 24, S16(o + 24) & 3);
        step2 = set16(o + 44, S16(o + 44) & 3);
    }
    if (step1 == 0) red1 = set16(o + 22, red1 - (hp1 < red1));
    if (step2 == 0) red2 = set16(o + 42, red2 - (hp2 < red2));
    if (red1 < hp1) red1 = set16(o + 22, hp1);
    if (red2 < hp2) red2 = set16(o + 42, hp2);
    if (red1 < 0) red1 = set16(o + 22, 0);
    if (hp1 < 0)  hp1 = set16(o + 20, 0);
    if (red2 < 0) red2 = set16(o + 42, 0);
    if (hp2 < 0)  hp2 = set16(o + 40, 0);
    if (red1 > FULL) set16(o + 22, FULL);
    if (hp1 > FULL)  hp1 = set16(o + 20, FULL);
    if (red2 > FULL) set16(o + 42, FULL);
    if (hp2 > FULL)  set16(o + 40, FULL);

    /* Lengths (+14); side 1's run from the right (x +6 = 0xB0 - length). */
    *H(o + 0x1B4 + 6) = (uint16_t)(0xB0 - bar_len(o + 0x1B4, o + 20, hp1));
    *H(o + 0x1F4 + 6) = (uint16_t)(0xB0 - bar_len(o + 0x1F4, o + 22, S16(o + 22)));
    bar_len(o + 0x7F4, o + 40, S16(o + 40));
    bar_len(o + 0x834, o + 42, S16(o + 42));

    if (S8(BATTLE + 37) != 0 || halted) {
        /* Dimmed: frames dark, bars at half, red parts 0x50 (side 2's with the original's odd corners). */
        quads(o + 0x274, 0, 0, 0);
        quads(o + 0x874, 0, 0, 0);
        bar_colour(o + 0x1B4, colour_of(f1, o + 16), 0, 0);
        bar_colour(o + 0x7F4, colour_of(f2, o + 36), 0, 1);
        quads(o + 0x1F4, 0x50, 0, 0);
        quads(o + 0x834, 0x50, 0, 0);
        *B(o + 0x834 + 37) = 0; *B(o + 0x834 + 49) = 0x50;
    } else {
        uint32_t c1 = *H(COLOURS + (uint32_t)(int32_t)S8(o + 16) * 2);
        uint32_t c2 = *H(COLOURS + (uint32_t)(int32_t)S8(o + 36) * 2);
        quads(o + 0x274, (uint8_t)((c1 & 0x1F) << 2), (uint8_t)((c1 & 0x3E0) >> 3), (uint8_t)((c1 & 0x7C00) >> 8));
        quads(o + 0x874, (uint8_t)((c2 & 0x1F) << 2), (uint8_t)((c2 & 0x3E0) >> 3), (uint8_t)((c2 & 0x7C00) >> 8));
        bar_colour(o + 0x1B4, colour_of(f1, o + 16), 1, 0);
        bar_colour(o + 0x7F4, colour_of(f2, o + 36), 1, 1);
        quads(o + 0x1F4, 0xA0, 0, 0);
        quads(o + 0x834, 0xA0, 0, 0);
    }
}
