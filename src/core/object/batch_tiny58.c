/* Small functions the scenarios reach, 53 - 54 instructions: effects following their owners or
   their owners' step and level, fighters' picture tables with their helpers. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088215C4(Cpu *c); void f_088219B0(Cpu *c); void f_08924734(Cpu *c); void f_08924784(Cpu *c);
void f_08821384(Cpu *c); void f_08821424(Cpu *c); void f_088768F0(Cpu *c); void f_089410B0(Cpu *c);
void f_0894113C(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c); void f_0882196C(Cpu *c);
void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): with +3 set it runs to +75 going negative (then on, +4); else it keeps its owner's
   (+88) x and goes on (+4) unless the owner is in step 7 (not 7/4) with +236 clear; 0x088219B0,
   and drawn (0x088215C4) unless +82 = 1. */
SFA_FUNC(08917BE0, effect_owner_x) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    gaddr p = *W(o + 88);
    if (*B(o + 3) != 0) {
        if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        call_on(c, o, 0x08917C9Cu, f_088219B0);
        call_on(c, o, 0x08917CA4u, f_088215C4);
    } else {
        if (!(S8(p + 236) == 0 && *B(p + 6) == 7 && *B(p + 7) != 4)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        int32_t x = S16(p + 18);
        c->r[5] = (uint32_t)x;
        *H(o + 18) = (uint16_t)x;
        call_on(c, o, 0x08917C5Cu, f_088219B0);
        if (S16(o + 82) != 1) call_on(c, o, 0x08917C74u, f_088215C4);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): off (+1) and gone (+4 on) once its owner (+88) is no longer its character (+338 = +3);
   with a level (+296's +10, above 0) at the owner's place and side and on: a new one (+78) is set
   up (0x08924734), else 0x08924784, or with +77 its +13 and 0x088219B0. */
SFA_FUNC(08924624, effect_owner_level_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = ra;
    if ((uint32_t)S8(p + 338) != *B(o + 3)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t lv = *B(*W(p + 296) + 10);
        if ((int8_t)lv <= 0) *B(o + 78) = 0;
        else {
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 11) = *B(p + 11);
            uint32_t cur = (uint32_t)S8(o + 78) & 0xFF;
            c->r[7] = cur; c->r[8] = 1;
            *B(o + 1) = 1;
            ARG(0) = o; ARG(1) = p; ARG(2) = lv;
            if (lv != cur) {
                *B(o + 78) = (uint8_t)lv;
                c->r[31] = 0x089246C8u; f_08924734(c);
            } else if (S8(o + 77) == 0) {
                c->r[6] = (uint32_t)S8(o + 77);
                c->r[31] = 0x089246B4u; f_08924784(c);
            } else {
                c->r[6] = (uint32_t)S8(o + 77);
                ARG(1) = *B(p + 13);
                *B(o + 13) = (uint8_t)ARG(1);
                c->r[31] = 0x089246A0u; f_088219B0(c);
            }
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): unless halted, while its owner (+88) is in step 1/0/7 it runs (0x088768F0, 0x0894113C)
   and moves (0x08821384), drawn, until +75 goes negative; then (or with the owner gone on) off
   (+4 = 2) via 0x089410B0. Halted it is only drawn. */
SFA_FUNC(08940FDC, effect_during_owner_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) != 0) call_on(c, o, 0x089410A0u, f_08821424);
    else {
        gaddr p = *W(o + 88);
        int off = 1;
        if (*B(p + 4) == 1 && *B(p + 5) == 0 && *B(p + 6) == 7) {
            call_on(c, o, 0x08941040u, f_088768F0);
            call_on(c, o, 0x08941048u, f_0894113C);
            if (S8(o + 75) >= 0) {
                off = 0;
                call_on(c, o, 0x08941080u, f_08821384);
                call_on(c, o, 0x08941088u, f_08821424);
            }
        }
        if (off) {
            *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            call_on(c, o, 0x08941070u, f_089410B0);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A9ACCC, 0x0886EB48
   (0x20, 0x08A9AD68), +549 cleared, +86 = its side, +164 = 1 and a list-0x16 helper (0x08822F54)
   linked both ways (its +88, the fighter's +40) with its side, +164, +12 and +14. */
SFA_FUNC(0899A86C, fighter_table_copy_20) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A9ACCCu + 4 * n);
    ARG(0) = f; ARG(1) = 0x20; ARG(2) = 0x08A9AD68u;
    c->r[31] = 0x0899A8CCu; f_0886EB48(c);
    *B(f + 549) = 0;
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    call2(c, 0x16, 0, 0x0899A8F4u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
        *W(f + 40) = n;
        *B(n + 12) = (uint8_t)S8(f + 12);
        *B(n + 14) = *B(f + 14);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): off (+1) and gone (+4 on) once the owner is no longer its character (+338 =
   +3); with a level (+296's +10) at the owner's place and side and on: a new one (+78) takes its
   animation from 0x08A861B0, else 0x088219B0. No level clears +78. */
SFA_FUNC(089660FC, effect_owner_level_3) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 3) != (uint32_t)S8(p + 338)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t lv = *B(*W(p + 296) + 10);
        if (lv == 0) *B(o + 78) = 0;
        else {
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 1) = 1;
            *B(o + 11) = *B(p + 11);
            lv = *B(*W(p + 296) + 10);
            if ((uint32_t)S8(o + 78) != lv) {
                *B(o + 78) = (uint8_t)lv;
                c->r[6] = *W(0x08A861B0u + 4 * *B(*W(p + 296) + 10));
                call2(c, o, c->r[6], 0x089661B4u, f_0882196C);
            } else call_on(c, o, 0x089661C8u, f_088219B0);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, +164 = 1, its 39-word picture table (+416) from 0x08A8FE6C,
   0x0886EB48 (0x1A, 0x08A8FF08) and a list-0x16 helper (0x08822EAC) of its character pointing back
   at it (+88), with its +12 - +15 / +24 / +38, +72 = 1, +73 cleared. */
SFA_FUNC(08971FE4, fighter_table_copy_1a) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(f + 164) = 1;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A8FE6Cu + 4 * n);
    ARG(0) = f; ARG(1) = 0x1A; ARG(2) = 0x08A8FF08u;
    c->r[31] = 0x0897204Cu; f_0886EB48(c);
    call2(c, 0x16, 0, 0x08972058u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 73) = 0;
        *B(n + 3) = (uint8_t)S8(f + 338);
        *W(n + 88) = f;
        *B(n + 12) = (uint8_t)S8(f + 12);
        *B(n + 13) = *B(f + 13);
        *B(n + 14) = *B(f + 14);
        *B(n + 72) = 1;
        *H(n + 24) = *H(f + 24);
        *H(n + 38) = (uint16_t)S16(f + 38);
        *B(n + 15) = *B(f + 15);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
