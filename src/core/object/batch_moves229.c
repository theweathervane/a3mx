/* Special-move starts with their meter change and launch speeds, a rush that becomes an air rush for
   a charge with no choice made (0x08905F14) and part characters' starts. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_08863114(Cpu *c); void f_08863A50(Cpu *c); void f_088D34F4(Cpu *c); void f_088D3180(Cpu *c);
void f_088D2DB4(Cpu *c); void f_089B7590(Cpu *c); void f_089A0454(Cpu *c); void f_089A04A4(Cpu *c);

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

/* (fighter): +7 on, facing the opponent, +235 = 1, 0x08863114, the meter by strength and move 6 + 2 x
   strength (7 + with no choice made and unlocked when +290 bit 0 is clear, else when the opponent is
   beyond the strength's distance from 0x08A5388C) of its list. */
SFA_FUNC(08905D54, special_face_6_or_7) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, 0x08905D8Cu);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08905D98u);
    int32_t s = S8(o + 194);
    uint32_t base = 6;
    if (S8(o + 373) == 0 && S8(o + 386) == 1) { if ((*H(o + 290) & 1) == 0) base = 7; }
    else if (S8(0x08A5388Cu + (uint32_t)s) < S16(o + 508)) base = 7;
    call2(c, o, (base + ((uint32_t)s << 1)) & 0xFFFF, f_08861674, 0x08905E00u);
    leave3(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 6, 0x08837098, the four speeds from 0x08A8B8E8 by the
   strength's entry in 0x08A8B8E4, and move 0x29 + strength. */
SFA_FUNC(08968A74, special_speeds_entry_29) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x08968AA4u);
    call1(c, o, f_08837098, 0x08968AACu);
    int32_t s = S8(o + 194);
    gaddr ent = 0x08A8B8E4u + (uint32_t)s, base = 0x08A8B8E8u;
    *W(o + 96) = *W(base + ((uint32_t)*B(ent) << 2));
    *W(o + 104) = *W(base + ((uint32_t)*B(ent) << 2) + 4);
    *W(o + 100) = *W(base + ((uint32_t)*B(ent) << 2) + 8);
    *W(o + 108) = *W(base + ((uint32_t)*B(ent) << 2) + 12);
    call2(c, o, (uint32_t)(s + 0x29) & 0xFFFF, f_088616EC, 0x08968B24u);
    leave(c, sv);
}

/* (character): +7 on, the meter by 4, 0x08837098, a hop (x 1.5, y 3.0 / 3.5 / 4.0 by strength, fall
   -0.125), 0x088D3180, move 0x35 + strength and +262 = 1. */
SFA_FUNC(089C935C, part_hop_35) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    call2(c, o, 4, f_088D34F4, 0x089C9380u);
    call1(c, o, f_08837098, 0x089C9388u);
    int32_t s = S8(o + 194);
    if (s == 0) *W(o + 100) = 0x30000;
    else if (s == 1) *W(o + 100) = 0x38000;
    else if (s == 2) *W(o + 100) = 0x40000;
    *W(o + 96) = 0x18000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFE000u;
    call1(c, o, f_088D3180, 0x089C93F4u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0x35), f_088616EC, 0x089C9404u);
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): step 3, +192 = 2, +235 = 1, 0x08863A50, the meter by strength, move 0x12 (0x18 with a
   guard) + 3 with +193, + strength, of its list; with no choice made and +193 then state 1 / 0 / 5 / 0
   and move 0x24 + strength. */
SFA_FUNC(08905F14, rush_then_air) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 3; *B(o + 192) = 2; *B(o + 235) = 1;
    call1(c, o, f_08863A50, 0x08905F44u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08905F50u);
    uint32_t n = S8(o + 78) != 0 ? 0x18 : 0x12;
    if (S8(o + 193) != 0) n = (uint32_t)(int32_t)(int8_t)(n + 3);
    call2(c, o, (n + (uint32_t)S8(o + 194)) & 0xFFFF, f_08861674, 0x08905F8Cu);
    if (S8(o + 386) < 0 && S8(o + 193) != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0;
        int32_t s = S8(o + 194);
        *B(o + 6) = 5; *B(o + 7) = 0;
        call2(c, o, (uint32_t)(s + 0x24), f_088616EC, 0x08905FC8u);
    }
    leave(c, sv);
}

/* (character): with a guard step 14 and a brake of 0.5 toward the facing into move 0x3E; else +7 on, the
   meter by 4, 0x08837098, +82 = 60, x 8.0 toward the facing, move 0xB and 0x088D2DB4; +262 = 1. */
SFA_FUNC(089BA034, part_guard_or_dash) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 78) != 0) {
        *B(o + 7) = 0xE;
        uint32_t f = *B(o + 11);
        *W(o + 104) = 0x8000;
        if (f == 0) *W(o + 104) = 0u - *W(o + 104);
        call2(c, o, 0x3E, f_088616EC, 0x089BA07Cu);
    } else {
        step_on(o);
        call2(c, o, 4, f_088D34F4, 0x089BA09Cu);
        call1(c, o, f_08837098, 0x089BA0A4u);
        *H(o + 82) = 0x3C; *W(o + 96) = 0x80000;
        uint32_t f = *B(o + 11);
        *W(o + 104) = 0;
        if (f == 0) *W(o + 96) = 0u - *W(o + 96);
        call2(c, o, 0xB, f_088616EC, 0x089BA0D8u);
        call1(c, o, f_088D2DB4, 0x089BA0E0u);
    }
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 9, 0x08837098, x by strength (0x08A5FF44), the mark (+0x334)
   ahead by 0x08A5FF50, brake -0.75, move 5 + strength and +656 = +76 / 2. */
SFA_FUNC(08915420, special_mark_5) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 9, f_0886CA2C, 0x08915450u);
    call1(c, o, f_08837098, 0x08915458u);
    int32_t s = S8(o + 194);
    *W(o + 96) = *W(0x08A5FF44u + ((uint32_t)s << 2));
    uint32_t f = *B(o + 11);
    int32_t x = S16(o + 18);
    int32_t d = S16(0x08A5FF50u + (uint32_t)(s + s));
    *H(o + 0x334) = (uint16_t)(f != 0 ? x + d : x - d);
    *W(o + 104) = 0xFFFF4000u;
    call2(c, o, (uint32_t)(S8(o + 194) + 5) & 0xFFFF, f_088616EC, 0x089154CCu);
    *B(o + 656) = (uint8_t)(S8(o + 76) >> 1);
    leave(c, sv);
}

/* (character): +7 on, +828 = strength + 1, a creep by strength (0 / 1.0 / 1.5, no brake), +310 = 1
   unless the opponent's +375 is below 18, the meter by 4, 0x08837098, move `strength`, 0x089B7590 and
   +262 = 1. */
SFA_FUNC(089B5938, part_creep) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    gaddr opp = *W(o + 92);
    uint32_t st = *B(o + 7);
    int32_t s = S8(o + 194);
    *B(o + 7) = (uint8_t)(st + 1);
    *W(o + 828) = (uint32_t)(s + 1);
    c->r[17] = 1;
    if (s == 0) *W(o + 96) = 0;
    else if (s == 1) *W(o + 96) = 0x10000;
    else if (s == 2) *W(o + 96) = 0x18000;
    *W(o + 104) = 0;
    if (!(S8(opp + 375) < 18)) *B(o + 310) = 1;
    call2(c, o, 4, f_088D34F4, 0x089B59D0u);
    call1(c, o, f_08837098, 0x089B59D8u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_088616EC, 0x089B59E4u);
    call1(c, o, f_089B7590, 0x089B59ECu);
    *B(o + 262) = 1;
    leave3(c, sv);
}

/* (fighter): +7 on, +262 = 1, the record (+0x334) cleared and +0x388 copied to +0x378, +895 = 1, +82 =
   1, the meter by 3, 0x08837098, 0x089A0454, 0x089A04A4 and move 0xF + strength. */
SFA_FUNC(0899FF98, special_record_f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    gaddr p = o + 0x334;
    *B(p + 5) = 0; *H(p) = 0; *B(p + 4) = 0;
    for (int i = 0; i < 8; i++) *B(o + 0x378 + i) = *B(o + 0x388 + i);
    *B(o + 895) = 1; *H(o + 82) = 1;
    call2(c, o, 3, f_0886CA2C, 0x089A002Cu);
    call1(c, o, f_08837098, 0x089A0034u);
    call1(c, o, f_089A0454, 0x089A003Cu);
    call1(c, o, f_089A04A4, 0x089A0044u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0xF) & 0xFFFF, f_088616EC, 0x089A0058u);
    leave(c, sv);
}
