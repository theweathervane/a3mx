/* Small functions the scenarios reach, 30 - 31 instructions: effects landing, stepping and set up from
   their owners, a fighter's picture table copied, the guard / hit level marks. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0887684C(Cpu *c); void f_088768F0(Cpu *c); void f_0893A724(Cpu *c); void f_088215C4(Cpu *c);
void f_088828CC(Cpu *c); void f_08971DA8(Cpu *c); void f_0882196C(Cpu *c); void f_0886EB48(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): 0x0893A724; under y 40 it lands (y 40, +6 on, y and its steps cleared, 0x0887684C (5)),
   else 0x088768F0. */
SFA_FUNC(0893A580, effect_land_40) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call_on(c, o, 0x0893A594u, f_0893A724);
    if (S16(o + 22) < 40) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 22) = 0x28; *H(o + 20) = 0;
        *W(o + 108) = 0; *W(o + 100) = 0;
        call2(c, o, 5, 0x0893A5D0u, f_0887684C);
    } else call_on(c, o, 0x0893A5E8u, f_088768F0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): with BATTLE +42 set just on (+4); else its step by kind +3 (0x08A70BC8) unless halted
   (BATTLE +37), then drawn (0x088215C4). */
SFA_FUNC(0893AC34, effect_step_bc8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (S8(BATTLE + 42) != 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        if (S8(BATTLE + 37) == 0) {
            gaddr h = *W(0x08A70BC8u + ((uint32_t)*B(o + 3) << 2));
            ARG(0) = o; ARG(1) = h;
            c->r[31] = 0x0893AC84u; rt_call_indirect(c, h);
        }
        call_on(c, o, 0x0893AC8Cu, f_088215C4);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its step by +4 called with its owner (+88): from 0x08A8AAB4 while +2 is 0, from 0x08A8AAC4
   at +2 = 0x1F, none otherwise. */
SFA_FUNC(08965F9C, effect_step_by_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr p = *W(o + 88);
    uint32_t k = *B(o + 2);
    gaddr table = k == 0 ? 0x08A8AAB4u : k == 0x1F ? 0x08A8AAC4u : 0;
    if (table != 0) {
        gaddr h = *W(table + ((uint32_t)*B(o + 4) << 2));
        ARG(1) = p; c->r[7] = h;
        c->r[31] = k == 0 ? 0x08965FD0u : 0x08966008u;
        rt_call_indirect(c, h);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, 0x08971DA8, +12 / +13 cleared, +9 = 3, y on its owner's owner's floor (+132), +86 =
   5, shade +160 = 0xC0 (+161 - +163 cleared), then 0x088828CC (0x16). */
SFA_FUNC(089718C4, effect_on_floor) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[31] = 0x089718E4u; f_08971DA8(c);
    *B(o + 12) = 0; *B(o + 13) = 0;
    *B(o + 9) = 3;
    *H(o + 22) = (uint16_t)S16(*W(*W(o + 88) + 88) + 132);
    *H(o + 86) = 5;
    *B(o + 160) = 0xC0; *B(o + 163) = 0; *B(o + 162) = 0; *B(o + 161) = 0;
    call2(c, o, 0x16, 0x0897192Cu, f_088828CC);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): thrown up and back: x step -0x40000 slowing 0x500, y step 0x90000 falling 0x18000, +112
   = -10 (all x turned for +11 clear), +114 = 3; its animation from 0x08A90F90's word. */
SFA_FUNC(08980D9C, effect_thrown) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(o + 96) = 0xFFFC0000u;
    *W(o + 104) = 0x500;
    *W(o + 100) = 0x90000;
    *W(o + 108) = 0xFFFE8000u;
    *H(o + 112) = 0xFFF6;
    *W(c->r[29] + 16) = ra;
    *H(o + 114) = 3;
    if (*B(o + 11) == 0) {
        *W(o + 96) = 0x40000;
        *W(o + 104) = 0xFFFFFB00u;
        *H(o + 112) = 0xA;
    }
    call2(c, o, *W(0x08A90CF0u + 672), 0x08980E08u, f_0882196C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A954F4, 0x0886EB48
   (0x1D, 0x08A95590), +164 = 1. */
SFA_FUNC(0898920C, fighter_table_copy) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    *B(f + 12) = 0xFF;
    gaddr dst = *W(f + 416);
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A954F4u + 4 * n);
    ARG(0) = f; ARG(1) = 0x1D; ARG(2) = 0x08A95590u;
    c->r[31] = 0x0898926Cu; f_0886EB48(c);
    *W(f + 164) = 1;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +12 cleared, its owner's (+88) +11 / +14 / +24 / x / y / +164 and side (+337, as
   +86) taken; its animation the owner's +0x3DC list's +16. */
SFA_FUNC(089B3A04, effect_from_owner_3dc) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 14) = *B(p + 14);
    *B(o + 12) = 0;
    *H(o + 24) = *H(p + 24);
    *B(o + 11) = *B(p + 11);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    *W(c->r[29] + 16) = ra;
    ARG(2) = p + 0x3D8;
    call2(c, o, *W(*W(p + 0x3D8 + 4) + 16), 0x089B3A70u, f_0882196C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its guard / hit marks: with +308 set the partner's (+92) +127 and its +573 cleared; else
   by +194: 0 -> 0 / 4, 1 -> 4 / 0xA, 2 -> 6 / 0x12. */
SFA_FUNC(088D4094, fighter_level_marks) {
    gaddr f = ARG(0);
    gaddr g = *W(f + 92);
    if (S8(f + 308) != 0) { *B(g + 127) = 0; *B(f + 573) = 0; return; }
    switch (S8(f + 194)) {
        case 0: *B(g + 127) = 0; *B(f + 573) = 4; break;
        case 1: *B(g + 127) = 4; *B(f + 573) = 0xA; break;
        case 2: *B(g + 127) = 6; *B(f + 573) = 0x12; break;
        default: break;
    }
}
