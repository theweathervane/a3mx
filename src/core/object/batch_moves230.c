/* Special-move starts with their meter change: moves picked by distance or button, a counted fifth
   use (0x0897B0C4) and a two-step special (0x0898B3C0). Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_08861848(Cpu *c); void f_0886469C(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_08863114(Cpu *c); void f_0898B0C8(Cpu *c);

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

/* (fighter): +7 on, +262 = 1, +502 = 1 with +540 (else cleared and the meter by 6), 0x08837098, y still,
   x / brake by strength (0x08A644B8, negated facing right), +82 from 0x08A644B4 and move 0x10 +
   strength. */
SFA_FUNC(0891B728, special_dash_10) {
    gaddr o = ARG(0); uint32_t sv[2];
    step_on(o); *B(o + 262) = 1;
    int32_t k = S8(o + 540);
    *B(o + 502) = 1;
    enter(c, o, sv);
    if (k == 0) { *B(o + 502) = 0; call2(c, o, 6, f_0886CA2C, 0x0891B768u); }
    call1(c, o, f_08837098, 0x0891B770u);
    int32_t s = S8(o + 194);
    *W(o + 108) = 0; *W(o + 100) = 0;
    gaddr e = 0x08A644B8u + ((uint32_t)s << 3);
    uint32_t f = *B(o + 11);
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
    if (f != 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    *H(o + 82) = (uint16_t)S8(0x08A644B4u + (uint32_t)s);
    call2(c, o, (uint32_t)(s + 0x10) & 0xFFFF, f_088616EC, 0x0891B7ECu);
    leave(c, sv);
}

/* (fighter): +7 on, facing the opponent, +235 = 1, 0x08863114, the meter by strength and move 2 x
   strength (+1 beyond the distance from 0x08A92510; at strength 0 every fifth use (+668) 0x20; at
   strength 1 with +290 bit 1 0x1E) of its list. */
SFA_FUNC(0897B0C4, special_counted_fifth) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, 0x0897B0FCu);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0897B108u);
    int32_t s = S8(o + 194);
    uint32_t n = (uint32_t)(s << 1) & 0xFFFF;
    if (s == 0) {
        gaddr p = o + 0x29C;
        int32_t k = S8(p);
        *B(p) = (uint8_t)(k + 1);
        if (k < 5) { *B(o + 668) = 0; n = 0x20; }
    }
    if (S16(0x08A92510u + (uint32_t)(s + s)) < S16(o + 508)) n = (n + 1) & 0xFFFF;
    if (s == 1 && (*H(o + 290) & 2)) n = 0x1E;
    call2(c, o, n, f_08861674, 0x0897B190u);
    leave3(c, sv);
}

/* (fighter): +7 on, the meter by 9, 0x08837098, +262 = 1, y's low half from 0x08AA3C5C by strength, the
   fall's low half 0x40, x 8.0, +83 = -1, +696 cleared and the move by stance (0x08AA3C62, 0x08AA3C6A
   with no choice made) + strength. */
SFA_FUNC(089B1244, special_by_stance) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    call2(c, o, 9, f_0886CA2C, 0x089B1268u);
    call1(c, o, f_08837098, 0x089B1270u);
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    *H(o + 0x64) = (uint16_t)S16(0x08AA3C5Cu + (uint32_t)(s + s));
    *H(o + 0x6C) = 0x40;
    *W(o + 96) = 0x80000; *W(o + 104) = 0;
    *B(o + 0x53) = 0xFF;
    int32_t k = S8(o + 386);
    *B(o + 696) = 0;
    int32_t st = S8(o + 236);
    int32_t s2 = S8(o + 194);
    if (k < 0) call2(c, o, (*H(0x08AA3C6Au + (uint32_t)(st + st)) + (uint32_t)s2) & 0xFFFF, f_088616EC, 0x089B12F8u);
    else call2(c, o, (*H(0x08AA3C62u + (uint32_t)(st + st)) + (uint32_t)s2) & 0xFFFF, f_088616EC, 0x089B1320u);
    leave(c, sv);
}

/* (fighter): +7 on, facing the opponent, +235 = 1, +123 / +82 cleared, 0x08863114, the meter by strength
   and move 2 x strength (+1 unlocked with no choice made without +290 bit 0, else beyond the distance
   from 0x08A5FED4; move 4 sets step 2) of its list. */
SFA_FUNC(089146E8, special_by_distance_b) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1; *B(o + 123) = 0; *H(o + 82) = 0;
    call1(c, o, f_08863114, 0x08914730u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0891473Cu);
    int32_t s = S8(o + 194);
    uint32_t n = (uint32_t)(s << 1) & 0xFFFF;
    if (S8(o + 373) == 0 && S8(o + 386) == 1) { if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF; }
    else if (S16(0x08A5FED4u + (uint32_t)(s + s)) < S16(o + 508)) n = (n + 1) & 0xFFFF;
    if (n == 4) *B(o + 7) = 2;
    call2(c, o, n, f_08861674, 0x089147C4u);
    leave3(c, sv);
}

/* (fighter): step 0: 0x0898B0C8, +7 on, +262 = 1, the meter by 3 (1 with +836), 0x08837098, +840 cleared
   and move 0x18 + strength (0x48 with +836); later a frame flag clears +262, 0x0898B0C8 and standing once
   ended, else ticked. */
SFA_FUNC(0898B3C0, special_two_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t st = *B(o + 7);
    enter(c, o, sv);
    if (st == 0) {
        call1(c, o, f_0898B0C8, 0x0898B3E0u);
        step_on(o);
        uint32_t k = *W(o + 836);
        *B(o + 262) = 1;
        if (k == 0) call2(c, o, 3, f_0886CA2C, 0x0898B408u);
        else call2(c, o, 1, f_0886CA2C, 0x0898B41Cu);
        call1(c, o, f_08837098, 0x0898B424u);
        uint32_t k2 = *W(o + 836);
        *W(o + 840) = 0;
        int32_t s = S8(o + 194);
        if (k2 != 0) call2(c, o, (uint32_t)(s + 0x48) & 0xFFFF, f_088616EC, 0x0898B444u);
        else call2(c, o, (uint32_t)(s + 0x18) & 0xFFFF, f_088616EC, 0x0898B45Cu);
    } else {
        if (S8(o + 77) != 0) *B(o + 262) = 0;
        call1(c, o, f_0898B0C8, 0x0898B478u);
        if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x0898B490u);
        else call1(c, o, f_08861848, 0x0898B4A0u);
    }
    leave(c, sv);
}
