/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 26 - 49
   instructions: the select screen's ISM boxes (by the fighter's ISM, +386), an ENTRY panel's set-up, the
   intro's name plates, a texture freed, the ticker's restart, hit marks by the opponent's state. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_088CE89C(Cpu *c); void f_08871A44(Cpu *c); void f_08871808(Cpu *c); void f_08871828(Cpu *c);
void f_088718B4(Cpu *c); void f_08871A84(Cpu *c); void f_08888EE4(Cpu *c); void f_088969BC(Cpu *c);
void f_08858D04(Cpu *c); void f_08858C5C(Cpu *c); void f_089E0540(Cpu *c); void f_08823220(Cpu *c);
void f_088B18A4(Cpu *c); void f_088B19F0(Cpu *c); void f_088B1AD0(Cpu *c); void f_088B1B00(Cpu *c);
void f_088B1D10(Cpu *c);

static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WRAP20(back, fn) wrap(c, 0x20, 16, back, fn)
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
#define WITH_S01(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; c->r[16] = (o)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- select screen */
/* (box): the ISM box: when its fighter's (+92) ISM (+386) changed, +80 = it and y (+22) from 0x08A2A9DC;
   its blink (+0x52 frame 0 - 3, +0x53 count 6) steps on; when the frame or ISM changed, palette
   0x138 + 4 x (ISM + 1) + frame into slot 0x14 + the fighter's side (0x088969BC). */
SFA_FUNC(0889AD44, ism_box) {
    gaddr o = ARG(0), f = *W(o + 92), p = o + 0x52;
    int8_t ism = S8(f + 386);
    if (S8(o + 80) != ism) {
        *B(o + 80) = (uint8_t)ism;
        *H(o + 22) = (uint16_t)S16(0x08A2A9DCu + (uint32_t)(ism + 1) * 2);
    }
    int8_t v = S8(o + 80);
    uint8_t n = (uint8_t)(*B(p + 1) - 1);
    *B(p + 1) = n;
    if (n == 0) {
        *B(p) = (uint8_t)(*B(p) + 1);
        uint8_t fr = *B(p);
        *B(p + 1) = 6;
        *B(p) = fr & 3;
    } else if (S8(o + 81) == v) { ARG(0) = (uint32_t)(int32_t)S8(o + 81); ARG(1) = p; return; }
    *B(o + 81) = (uint8_t)v;
    ARG(0) = ((uint32_t)(v + 1) << 2) + *B(p) + 0x138;
    ARG(1) = (uint32_t)(S8(f + 337) + 0x14);
    ARG(2) = 1;
    WRAP20(0x0889ADF0u, f_088969BC);
}
/* (panel): an ENTRY panel's set-up (+4 on, +1 = 1, +148 = 3, +86 = 0x12, +24 = 6, +12 = 1, +13 = entry
   + 7, others cleared), then 0x088CE89C (its record). */
SFA_FUNC(088CE834, entry_panel_setup) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *W(o + 148) = 3;
    *H(o + 86) = 0x12;
    *W(o + 164) = 0;
    *H(o + 24) = 6;
    *B(o + 9) = 0;
    int8_t k = S8(o + 78);
    *B(o + 14) = 0;
    *B(o + 11) = 0;
    *B(o + 12) = 1;
    ARG(1) = (uint32_t)(k + 7);
    *B(o + 13) = (uint8_t)(k + 7);
    WRAP20(0x088CE890u, f_088CE89C);
}
/* (object): with the record at 0x0950FF40's +10 clear, 0x08888EE4; else while +129 is 0, +14 grows by
   10 to 160 with +26 = +14 x 5 / 10, then +129 = 1 and the record's +10 = 2; once +129 is set, +93 =
   whether the record's +10 is not 3. */
SFA_FUNC_WRAPPED(088888E8, grow_14_to_160, pausefilter_panel) {   /* the pause menu's panel (extensions/pausefilter.c) */
    gaddr o = ARG(0), r = 0x0950FF40u;
    uint8_t state = *B(r + 10);
    ARG(1) = r;
    if (state == 0) { WRAP20(0x0888890Cu, f_08888EE4); return; }
    uint8_t k = *B(o + 129);
    if (k != 0) { *B(o + 93) = state != 3; return; }
    int16_t v = S16(o + 14);
    if (!(v < 160)) { *B(o + 129) = (uint8_t)(k + 1); *B(r + 10) = 2; return; }
    *H(o + 14) = (uint16_t)(v + 10);
    int32_t n = S16(o + 14);
    DIV(c, (uint32_t)(n * 5), 10);
    *H(o + 26) = (uint16_t)c->lo;
}
/* (object): the intro's name plate: +4 on, at x 0x180 (side 1) or -0xA0, y 0xA8; picture +78 x 16 + +3
   (to 0x1F, or 0x20 - 0x3F with +3 bit 5) through 0x08858D04; +156 = 0x08A177B0; then 0x08858C5C. */
SFA_FUNC(08858BA0, intro_name_plate) {
    gaddr o = ARG(0); WITH_S0(o);
    *B(o + 12) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 11) = 0;
    *B(o + 14) = 0;
    *H(o + 24) = 7;
    *H(o + 82) = 9;
    int32_t side = S8(o + 78);
    *H(o + 18) = (uint16_t)(side != 0 ? 0x180 : -0xA0);
    uint8_t b3 = *B(o + 3);
    uint32_t pic = ((uint32_t)side << 4) + b3;
    pic &= 0x1F;
    if (b3 & 0x20) pic = (uint32_t)(int32_t)(int16_t)(pic + 0x20);
    *H(o + 22) = 0xA8;
    *H(o + 86) = 5;
    *W(o + 148) = 5;
    *B(o + 12) = 0; *B(o + 13) = 0;
    ARG(1) = pic; ARG(2) = 5;
    call_on(c, o, 0x08858C38u, f_08858D04);
    *W(o + 156) = 0x08A177B0u;
    call_on(c, o, 0x08858C4Cu, f_08858C5C);
    END_S0();
}
/* (object): the ticker: from GAME +16's low byte 20 on, 0x08823220; otherwise +82's high byte counts
   down, and at 0 (+5 on) 0x088B18A4, 0x088B19F0, 0x088B1AD0, 0x088B1B00 and 0x088B1D10 run. */
SFA_FUNC(088B174C, ticker_step) {
    gaddr o = ARG(0); WITH_S01(o);
    if (!(((uint32_t)(int32_t)S16(GAME + 16) & 0xFF) < 20)) call_on(c, o, 0x088B1784u, f_08823220);
    else {
        int16_t n = (int16_t)((int16_t)(S16(o + 82) >> 8) - 1);
        c->r[17] = (uint32_t)(int32_t)n;
        if (n == 0) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            call_on(c, o, 0x088B17C0u, f_088B18A4);
            call_on(c, o, 0x088B17C8u, f_088B19F0);
            call_on(c, o, 0x088B17D0u, f_088B1AD0);
            call_on(c, o, 0x088B17D8u, f_088B1B00);
            call_on(c, o, 0x088B17E0u, f_088B1D10);
        }
        *H(o + 82) = (uint16_t)((uint32_t)(int32_t)S16(o + 82) & 0xFF);
        *H(o + 82) = (uint16_t)((uint32_t)(int32_t)S16(o + 82) | ((uint32_t)(int32_t)n << 8));
    }
    END_S01();
}

/* ---------------------------------------------------------------- textures and hits */
/* (n): texture record n (24 bytes at 0x08B0C334): a held texture (+0, unless +8 bit 0) or a palette-less
   one (+4, with +8 bit 0) freed (0x089E0540), the record cleared, the count at 0x08ACCD84 down. */
SFA_FUNC(089E06BC, texture_free) {
    gaddr t = 0x08B0C334u + ARG(0) * 24;
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = t;
    for (int k = 0; k < 2; k++) {
        uint32_t p = *W(t + (k ? 4 : 0));
        int flag = (*W(t + 8) & 1) != 0;
        if (p == 0 || flag != k) continue;
        ARG(0) = p;
        c->r[31] = k ? 0x089E0748u : 0x089E0704u; f_089E0540(c);
        *W(t + 4) = 0; *W(t) = 0; *W(t + 8) = 0; *W(t + 16) = 0;
        *W(0x08ACCD84u) -= 1;
        break;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (fighter, other): the hit's mark by the other's state (0x08871A44) and guard (+696): guarding
   0x08871808 / 0x08871828 (or 0x088718B4 with +588 set), else +117 = 4 / 5 (7 / 8) and 0x08871A84. */
static void mark_by_state(Cpu *c, uint8_t m1, uint8_t m2, const uint32_t backs[6]) {
    gaddr f = ARG(0), x = ARG(1);
    WITH_S01(x);
    c->r[17] = f;
    call2(c, f, x, backs[0], f_08871A44);
    if (c->r[2] != 0) {
        if (S8(f + 696) != 0) call2(c, f, x, backs[1], f_08871808);
        else { *B(f + 117) = m1; call2(c, f, x, backs[2], f_08871A84); }
    } else if (S8(f + 588) != 0) call2(c, f, x, backs[3], f_088718B4);
    else if (S8(f + 696) != 0) call2(c, f, x, backs[4], f_08871828);
    else { *B(f + 117) = m2; call2(c, f, x, backs[5], f_08871A84); }
    END_S01();
}
SFA_FUNC(088715C0, mark_4_5) {
    static const uint32_t b[6] = { 0x088715DCu, 0x088715FCu, 0x08871614u, 0x0887162Cu, 0x0887164Cu, 0x08871664u };
    mark_by_state(c, 4, 5, b);
}
SFA_FUNC(08871730, mark_7_8) {
    static const uint32_t b[6] = { 0x0887174Cu, 0x0887176Cu, 0x08871784u, 0x0887179Cu, 0x088717BCu, 0x088717D4u };
    mark_by_state(c, 7, 8, b);
}
