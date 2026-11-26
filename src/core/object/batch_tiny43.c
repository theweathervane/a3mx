/* Small functions the scenarios reach, 23 - 39 instructions: fighter objects' starts, effects set up
   from their owners and tables, a blinking effect step, a fighter's picture table and helper.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08934FCC(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DBE8(Cpu *c); void f_088215C4(Cpu *c);
void f_0882196C(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_08940FA8(Cpu *c);
void f_0894113C(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): its start (0x08934FCC (0)) when it has no effects (+536), or under two while +251 is set. */
SFA_FUNC(08934F70, object_start_if_few_3) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t n = *B(o + 536);
    if (S8(o + 251) != 0) { if ((int32_t)n < 2) call2(c, o, 0, 0x08934F98u, f_08934FCC); }
    else if (n == 0) call2(c, o, 0, 0x08934FB4u, f_08934FCC);
    else ARG(1) = n;
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 not negative, +542 clear and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1,
   +236 = 9; +5 / +7 cleared, +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(088FD578, object_start_542) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) >= 0 && S8(o + 542) == 0) {
        c->r[31] = 0x088FD5A8u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 5) = 0; *B(o + 236) = 9;
            c->r[17] = 1;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, 0x088FD5E4u, f_0886DBE8);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4 = 1, +9 = 3; +78 / +82 cleared), its owner's (+88) +11 - +14 / +24 / +86 / +164
   taken, its animation by kind +3 (0x08A5B478's list) and drawn. */
SFA_FUNC(08917B44, effect_from_owner_b458) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = 1; *B(o + 9) = 3;
    *B(o + 14) = *B(p + 14);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 11) = *B(p + 11);
    *H(o + 82) = 0;
    *H(o + 24) = *H(p + 24);
    *B(o + 78) = 0;
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(c->r[29] + 20) = ra;
    call2(c, o, *W(0x08A5B458u + ((uint32_t)*B(o + 3) << 2) + 32), 0x08917BC8u, f_0882196C);
    call_on(c, o, 0x08917BD0u, f_088215C4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): every 6 frames (+0x6C) its owner's (+88) +12 / +13 again, on alternate times (+0x6D) one
   palette on with +12 = -1; then its step by +5 (0x08A64530). */
SFA_FUNC(0891C6F4, effect_blink_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr q = o + 0x6C, p = *W(o + 88);
    int8_t t = (int8_t)(S8(q) - 1);
    *B(q) = (uint8_t)t;
    *W(c->r[29] + 16) = ra;
    if (t < 0) {
        *B(q) = 5;
        *B(o + 12) = (uint8_t)S8(p + 12);
        *B(o + 13) = *B(p + 13);
        *B(q + 1) = (uint8_t)(*B(q + 1) ^ 1);
        if (*B(q + 1) & 1) {
            *B(o + 12) = 0xFF;
            *B(o + 13) = (uint8_t)(*B(o + 13) + 1);
        }
    }
    gaddr h = *W(0x08A64530u + ((uint32_t)*B(o + 5) << 2));
    ARG(1) = h;
    c->r[31] = 0x0891C784u; rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +9 cleared, its tables (+128 = 0x08A71128, +200 = 0x08A70F4C, +352 = 0x08A734E4,
   +356 = 0x08A73564), 0x08876998, +73 cleared, 0x0894113C, +344 = -1, 0x0887684C (+346) and
   0x08940FA8. */
SFA_FUNC(08940F0C, effect_tables_set_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 9) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 352) = 0x08A734E4u;
    *W(o + 356) = 0x08A73564u;
    *W(o + 128) = 0x08A71128u;
    *W(o + 200) = 0x08A70F4Cu;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x08940F64u, f_08876998);
    *B(o + 73) = 0;
    call_on(c, o, 0x08940F70u, f_0894113C);
    *B(o + 344) = 0xFF;
    call2(c, o, (uint32_t)(int32_t)S8(o + 346) & 0xFFFF, 0x08940F90u, f_0887684C);
    call_on(c, o, 0x08940F98u, f_08940FA8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4 = 1; +9 / +86 cleared, +344 = -1), +24 its owner's, its tables (+128 = 0x08A74A60,
   +200 = 0x08A74A10, +352 = 0x08A749CC, +356 = 0x08A749D8), 0x08876998, +73 / +112 cleared and
   0x0887684C (+346). */
SFA_FUNC(08947DB0, effect_tables_set_3) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = 1; *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 86) = 0;
    *H(o + 24) = *H(*W(o + 88) + 24);
    *W(o + 352) = 0x08A749CCu;
    *W(o + 356) = 0x08A749D8u;
    *W(o + 128) = 0x08A74A60u;
    *W(o + 200) = 0x08A74A10u;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x08947E1Cu, f_08876998);
    *B(o + 73) = 0;
    *H(o + 112) = 0;
    call2(c, o, (uint32_t)(int32_t)S8(o + 346) & 0xFFFF, 0x08947E3Cu, f_0887684C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A8A6F8, 0x0886EB48
   (0x17, 0x08A8A794), +164 = 1, +828 cleared and a list-0 helper (0x08822F54) linked both ways (its
   +88, the fighter's +40). */
SFA_FUNC(08961AA8, fighter_table_copy_17) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A8A6F8u + 4 * n);
    ARG(0) = f; ARG(1) = 0x17; ARG(2) = 0x08A8A794u;
    c->r[31] = 0x08961B08u; f_0886EB48(c);
    *W(f + 164) = 1;
    *W(f + 828) = 0;
    call2(c, 0, 0, 0x08961B20u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) { *W(n + 88) = f; *W(f + 40) = n; }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +9 = +72 = 1, its owner's (+88) character (+338, as +127), +11 - +14 / +24 / +38 /
   x / y taken, then its set-up by kind +3 (0x08A928B4). */
SFA_FUNC(08980CDC, effect_from_owner_28b4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 72) = 1; *B(o + 9) = 1;
    *B(o + 127) = (uint8_t)S8(p + 338);
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    uint32_t kind = *B(o + 3);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    gaddr h = *W(0x08A928B4u + (kind << 2));
    *W(c->r[29] + 16) = ra;
    ARG(1) = h; ARG(2) = kind << 2; ARG(3) = kind;
    c->r[31] = 0x08980D6Cu; rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
