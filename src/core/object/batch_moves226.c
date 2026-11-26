/* Special-move starts with their meter change and launch speeds, and the meter award for a landed hit
   (0x08872410, 994 calls): the attacker gains by the hit's value (halved on a block, scaled by its rate
   with no choice made) and the defender gains 3 unless it blocked. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_08863A50(Cpu *c); void f_0892B188(Cpu *c);

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
static void enter3(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* (fighter): +7 on, +262 = 1, the meter by strength + 5, 0x08837098, +20 cleared, a jump (y 4.0 /
   -0.375, +73 = 1, x by strength from 0x08A8A908 toward the facing) and move 7. */
SFA_FUNC(08963CD8, special_jump_7_c) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    step_on(o);
    int32_t s = S8(o + 194);
    c->r[17] = 1;
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 5), f_0886CA2C, 0x08963D10u);
    call1(c, o, f_08837098, 0x08963D18u);
    *H(o + 20) = 0;
    int32_t s2 = S8(o + 194);
    *W(o + 100) = 0x40000; *B(o + 73) = 1; *W(o + 108) = 0xFFFFA000u;
    uint32_t v = *W(0x08A8A908u + ((uint32_t)s2 << 2));
    *W(o + 96) = *B(o + 11) != 0 ? v : 0u - v;
    call2(c, o, 7, f_088616EC, 0x08963D6Cu);
    leave3(c, sv);
}

/* (fighter): step 3, +235 = 1, 0x08863A50, the meter by strength and move 0x12 (0x18 with a guard) + 3
   with +193, + strength (with no choice made and +290 bit 2, 0x1C becomes 0x20) of its list. */
SFA_FUNC(089670B8, rush_by_guard_d) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 3; *B(o + 235) = 1;
    call1(c, o, f_08863A50, 0x089670E0u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089670ECu);
    uint32_t n = S8(o + 78) == 0 ? 0x12 : 0x18;
    if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
    n = (n + (uint32_t)S8(o + 194)) & 0xFFFF;
    if (S8(o + 386) == -1 && n == 0x1C && (*H(o + 290) & 4)) n = 0x20;
    call2(c, o, n, f_08861674, 0x08967150u);
    leave(c, sv);
}

/* (fighter): step 1, +262 = 1, the meter by strength + 5, 0x08837098, the four speeds by strength
   (0x08A51508: x, x brake, y, fall), +73 = 1 and move 0x29 + 2 x strength. */
SFA_FUNC(08902A60, special_air_29_2k) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    *B(o + 7) = 1;
    int32_t s = S8(o + 194);
    c->r[17] = 1;
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 5), f_0886CA2C, 0x08902A94u);
    call1(c, o, f_08837098, 0x08902A9Cu);
    int32_t s2 = S8(o + 194);
    gaddr e = 0x08A51508u + ((uint32_t)s2 << 4);
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    *B(o + 73) = 1;
    call2(c, o, (uint32_t)((s2 << 1) + 0x29) & 0xFFFF, f_088616EC, 0x08902AF8u);
    leave3(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 7, 0x08837098, brake -0.5, fall -0.375, x / y by strength
   (0x08A6A40C) and move 0xE + strength (0x2C with +820, which is cleared). */
SFA_FUNC(089266C0, special_jump_e_or_2c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 7, f_0886CA2C, 0x089266F0u);
    call1(c, o, f_08837098, 0x089266F8u);
    int32_t s = S8(o + 194);
    *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
    gaddr e = 0x08A6A40Cu + ((uint32_t)s << 3);
    uint32_t k = *W(o + 820);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    uint32_t n;
    if (k != 0) { n = (uint32_t)(s + 0x2C) & 0xFFFF; *W(o + 820) = 0; }
    else n = (uint32_t)(s + 0xE) & 0xFFFF;
    call2(c, o, n, f_088616EC, 0x0892675Cu);
    leave(c, sv);
}

/* (fighter): step 1, 0x0892B188, +262 = 1, the meter by 9, 0x08837098, a pending +828 (cleared) sets
   +573 = -1 and +656 = 7, a jump by strength (x 1.5 + 2 per level, y 2.5 + 2 per level, brake -0.5,
   fall -0.375) and move 4 + strength. */
SFA_FUNC(0892AF54, special_jump_4) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1;
    call1(c, o, f_0892B188, 0x0892AF74u);
    *B(o + 262) = 1;
    call2(c, o, 9, f_0886CA2C, 0x0892AF88u);
    call1(c, o, f_08837098, 0x0892AF90u);
    if (*W(o + 828) != 0) { *W(o + 828) = 0; *B(o + 573) = 0xFF; *B(o + 656) = 7; }
    int32_t s = S8(o + 194);
    uint32_t k = (uint32_t)s << 17;
    *W(o + 96) = k + 0x30000; *W(o + 100) = k + 0x50000;
    *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
    call2(c, o, (uint32_t)(s + 4) & 0xFFFF, f_088616EC, 0x0892AFF0u);
    leave(c, sv);
}

/* (defender, hit data): the attacker (0x09358410 +24) gains the hit's value (+17; halved when blocked,
   +117 bit 7; with no choice made scaled by its rate from 0x08A21F48 / 32), and the defender (+28) gains
   3 unless it blocked. */
SFA_FUNC(08872410, hit_meter_award) {
    gaddr o = ARG(0), hit = ARG(1);
    uint32_t sv[3];
    enter3(c, o, sv);
    gaddr pair = 0x09358410u;
    c->r[17] = pair;
    int32_t v = *B(hit + 17);
    gaddr att = *W(pair + 24);
    if (*B(o + 117) & 0x80) v >>= 1;
    if (S8(att + 386) == 0) {
        uint32_t k = (uint32_t)S16(att + 806);
        if (k != 0) {
            int64_t m = (int64_t)v * (int32_t)*B(0x08A21F48u + k);
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            int32_t lo = (int32_t)(uint32_t)m;
            v = (lo + (int32_t)(((uint32_t)(lo >> 5)) >> 27)) >> 5;
        }
    }
    call2(c, att, (uint32_t)v, f_0886CA2C, 0x08872490u);
    if (!(*B(o + 117) & 0x80)) call2(c, *W(pair + 28), 3, f_0886CA2C, 0x088724ACu);
    leave3(c, sv);
}

/* (fighter): +262 = 1, step 1, the meter by 9, 0x08837098, brake -0.5, fall -0.375, x / y by strength
   (0x08A45484, or 0x08A4549C with no choice made) and move 5 + strength. */
SFA_FUNC(088F3464, special_jump_5) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, 9, f_0886CA2C, 0x088F348Cu);
    call1(c, o, f_08837098, 0x088F3494u);
    int32_t s = S8(o + 194);
    *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
    gaddr tab = S8(o + 386) < 0 ? 0x08A4549Cu : 0x08A45484u;
    gaddr e = tab + ((uint32_t)s << 3);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    call2(c, o, (uint32_t)(s + 5) & 0xFFFF, f_088616EC, 0x088F3504u);
    leave(c, sv);
}

/* (fighter): +656 cleared, +7 on, the meter by 5, 0x08837098, +73 / +262 = 1, y 2.0 / -0.1875 and x 5.0
   + 1 per level toward the facing (brake 1/8) into move 5 + strength. */
SFA_FUNC(08921104, special_jump_5_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 656) = 0; *B(o + 7) = (uint8_t)(st + 1);
    call2(c, o, 5, f_0886CA2C, 0x08921130u);
    call1(c, o, f_08837098, 0x08921138u);
    *B(o + 73) = 1; *B(o + 262) = 1;
    int32_t s = S8(o + 194);
    *W(o + 100) = 0x20000;
    uint32_t k = (uint32_t)s << 16;
    *W(o + 108) = 0xFFFFD000u;
    if (*B(o + 11) != 0) { *W(o + 96) = k + 0x50000; *W(o + 104) = 0xFFFFE000u; }
    else { *W(o + 96) = 0xFFFB0000u - k; *W(o + 104) = 0x2000; }
    call2(c, o, (uint32_t)(s + 5) & 0xFFFF, f_088616EC, 0x089211A4u);
    leave(c, sv);
}
