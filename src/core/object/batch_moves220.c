/* Special-move starts with their meter change, and the meter gain scaled by a per-character rate when
   no choice is made (0x0886D560). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_088D34F4(Cpu *c); void f_0887C408(Cpu *c); void f_089967C8(Cpu *c); void f_08863114(Cpu *c);
void f_0886E3CC(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static uint32_t by_str(gaddr o, uint32_t base) { return (uint32_t)(S8(o + 194) + (int32_t)base) & 0xFFFF; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* (fighter): +696 cleared, +7 on, +123 cleared, +262 = 1, the meter by strength + 6, 0x08837098 and
   move 3 + strength. */
static void special_3_m6(Cpu *c, uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 696) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 123) = 0;
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 6), f_0886CA2C, b0);
    call1(c, o, f_08837098, b1);
    call2(c, o, by_str(o, 3), f_088616EC, b2);
    leave(c, sv);
}
SFA_FUNC(0898F68C, special_3_m6_a) { special_3_m6(c, 0x0898F6C8u, 0x0898F6D0u, 0x0898F6E4u); }
SFA_FUNC(08994DE4, special_3_m6_b) { special_3_m6(c, 0x08994E20u, 0x08994E28u, 0x08994E3Cu); }

/* (fighter): +7 on, +262 = 1, +696 cleared, voice 0x212, the meter by 6, 0x08837098 and 0x089967C8. */
SFA_FUNC(08996760, special_voice_212) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t side = S8(o + 337);
    *B(o + 262) = 1; *B(o + 696) = 0;
    ARG(0) = o; ARG(1) = (uint32_t)side; ARG(2) = 0x212; c->r[31] = 0x0899679Cu; f_0887C408(c);
    call2(c, o, 6, f_0886CA2C, 0x089967A8u);
    call1(c, o, f_08837098, 0x089967B0u);
    call1(c, o, f_089967C8, 0x089967B8u);
    leave(c, sv);
}

/* (fighter): +7 on, the meter by 15 and the four speeds by strength (0x08A58328: x, y, x brake, fall). */
SFA_FUNC(0890A3F4, special_speeds_m15) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    call2(c, o, 0xF, f_0886CA2C, 0x0890A418u);
    gaddr e = 0x08A58328u + ((uint32_t)S8(o + 194) << 4);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4); *W(o + 104) = *W(e + 8); *W(o + 108) = *W(e + 12);
    leave(c, sv);
}

/* (fighter): +7 on, +235 = 1, facing the opponent, the mark (+0x334) cleared, 0x08863114, the meter by
   strength and move 0xC + strength of its list. */
SFA_FUNC(0890F6E8, special_face_c_mark) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t face = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)face; *B(o + 0x334) = 0;
    call1(c, o, f_08863114, 0x0890F724u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0890F730u);
    call2(c, o, by_str(o, 0xC), f_08861674, 0x0890F744u);
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, the mark (+0x334) cleared, the meter by 7, 0x08837098, +0x339 = 3 and
   move 0x24 + strength. */
SFA_FUNC(08931B80, special_start_24) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *B(o + 262) = 1;
    *W(c->r[29] + 20) = s1;
    *B(o + 7) = 1;
    gaddr m = o + 0x334;
    c->r[17] = m;
    *B(m) = 0;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 7, f_0886CA2C, 0x08931BB4u);
    call1(c, o, f_08837098, 0x08931BBCu);
    *B(m + 5) = 3;
    call2(c, o, by_str(o, 0x24), f_088616EC, 0x08931BD8u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, amount): with no choice made and a rate (+804), the amount scaled by its entry in
   0x08A1BDFE / 10; then the meter gain. */
SFA_FUNC(0886D560, meter_gain_rated) {
    gaddr o = ARG(0);
    int32_t n = (int32_t)ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 386) == -1) {
        uint32_t k = *H(o + 804);
        if (k != 0) {
            int64_t m = (int64_t)n * (int32_t)*B(0x08A1BDFEu + k);
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            DIV(c, (uint32_t)m, 10);
            n = (int32_t)c->lo;
        }
    }
    c->r[6] = (uint32_t)n;
    ARG(0) = o; ARG(1) = (uint32_t)n; c->r[31] = 0x0886D5C4u; f_0886CA2C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 on, +262 = 1, the meter by 5, 0x08837098, a dash (8.0 / -0.75) and move 0x1C +
   strength. */
SFA_FUNC(08915AF8, special_dash_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x08915B28u);
    call1(c, o, f_08837098, 0x08915B30u);
    *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF4000u;
    call2(c, o, by_str(o, 0x1C), f_088616EC, 0x08915B58u);
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, the meter by 4, 0x08837098, +0x337 / +0x338 cleared, strength doubled
   when locked, and move 0xE. */
SFA_FUNC(08930620, special_start_e) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, 4, f_0886CA2C, 0x08930648u);
    call1(c, o, f_08837098, 0x08930650u);
    *B(o + 0x338) = 0; *B(o + 0x337) = 0;
    if (S8(o + 373) != 0) *B(o + 194) = (uint8_t)(S8(o + 194) << 1);
    call2(c, o, 0xE, f_088616EC, 0x08930680u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 7, 0x08837098, x 6.0 toward the facing and move 0xA +
   strength. */
SFA_FUNC(08930E30, special_dash_a) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 7, f_0886CA2C, 0x08930E60u);
    call1(c, o, f_08837098, 0x08930E68u);
    uint32_t v = *B(o + 11) != 0 ? 0x60000u : 0xFFFA0000u;
    int32_t s = S8(o + 194);
    *W(o + 96) = v;
    call2(c, o, (uint32_t)(s + 0xA) & 0xFFFF, f_088616EC, 0x08930E90u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 6, 0x08837098, hit cue (8, 8), +551 cleared and move 0x10 +
   strength. */
SFA_FUNC(08968CFC, special_start_10_cue) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x08968D2Cu);
    call1(c, o, f_08837098, 0x08968D34u);
    ARG(0) = o; ARG(1) = 8; ARG(2) = 8; c->r[31] = 0x08968D44u; f_0886E3CC(c);
    int32_t s = S8(o + 194);
    *B(o + 551) = 0;
    call2(c, o, (uint32_t)(s + 0x10) & 0xFFFF, f_088616EC, 0x08968D5Cu);
    leave(c, sv);
}

/* (character): +7 on, +262 = 1, +828 = 16, +832 = 60, the meter by 4, 0x08837098, +820 / +824 cleared and
   move 0x12 + strength. */
SFA_FUNC(089B6204, part_special_12_hold) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    *W(o + 828) = 0x10; *W(o + 832) = 0x3C;
    call2(c, o, 4, f_088D34F4, 0x089B6244u);
    call1(c, o, f_08837098, 0x089B624Cu);
    int32_t s = S8(o + 194);
    *W(o + 820) = 0; *W(o + 824) = 0;
    call2(c, o, (uint32_t)(s + 0x12), f_088616EC, 0x089B6264u);
    leave(c, sv);
}
