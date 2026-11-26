/* Small functions the scenarios reach, 40 - 46 instructions: level effects' starts, fighters' move
   starts on the ground or in the air, an effect on its fighter by level. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089CCDE8(Cpu *c); void f_089B7B14(Cpu *c); void f_0886BF84(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DBE8(Cpu *c); void f_08981C50(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): on (+4, +78 cleared), its level step, then its owner's (+88) place, +11 - +13, side and
   +164. */
static gaddr level_start(Cpu *c, void (*step)(Cpu *), uint32_t back) {
    gaddr o = ARG(0);
    *W(c->r[29] + 16) = c->r[16];
    c->r[16] = o;
    *W(c->r[29] + 20) = c->r[17];
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 78) = 0;
    call_on(c, o, back, step);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 11) = *B(p + 11);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    return o;
}

/* ... with +170 / +171 = 1. */
SFA_FUNC(089CCA20, effect_level_start_3) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = ra;
    gaddr o = level_start(c, f_089CCDE8, 0x089CCA50u);
    *B(o + 170) = 1; *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ... with +170 = 1 for a level (+78) under 7, +171 = 1. */
SFA_FUNC(089B7980, effect_level_start_1) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = ra;
    gaddr o = level_start(c, f_089B7B14, 0x089B79B0u);
    if (S8(o + 78) < 7) *B(o + 170) = 1;
    *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The common tail: step 1/0/7, +235 = 1, 0x0886DBE8. */
static void move_go(Cpu *c, gaddr f, uint32_t back) {
    *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
    *B(f + 235) = 1;
    call_on(c, f, back, f_0886DBE8);
}

/* (fighter): on the ground once 0x0886BF84 allows it, in the air rising (+100 not negative) once
   0x0886C2B4 does: +236 = 3, +11 = +697 and the move; 1 then, else 0. */
SFA_FUNC(08994314, fighter_move_start_rising) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    int ok = 0;
    if (S8(f + 73) == 0) { call_on(c, f, 0x08994338u, f_0886BF84); ok = c->r[2] != 0; }
    else if ((int32_t)*W(f + 100) >= 0) { call_on(c, f, 0x089943A4u, f_0886C2B4); ok = c->r[2] != 0; }
    uint32_t r = 0;
    if (ok) {
        *B(f + 236) = 3;
        c->r[17] = 1;
        *B(f + 11) = (uint8_t)S8(f + 697);
        move_go(c, f, 0x08994378u);
        r = 1;
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in the air once 0x0886C2B4 allows it (+236 = 3), on the ground once 0x0886BF84 does (+236 =
   2, +11 = +697): the move; 1 then, else 0. */
static void move_start_32(Cpu *c, uint32_t b_air, uint32_t b_ground, uint32_t b_go) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    int ok;
    if (S8(f + 73) != 0) {
        call_on(c, f, b_air, f_0886C2B4);
        ok = c->r[2] != 0;
        if (ok) *B(f + 236) = 3;
    } else {
        call_on(c, f, b_ground, f_0886BF84);
        ok = c->r[2] != 0;
        if (ok) { *B(f + 236) = 2; *B(f + 11) = (uint8_t)S8(f + 697); }
    }
    uint32_t r = 0;
    if (ok) { c->r[17] = 1; move_go(c, f, b_go); r = 1; }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088EF288, fighter_move_start_32a) { move_start_32(c, 0x088EF2ACu, 0x088EF300u, 0x088EF2E0u); }
SFA_FUNC(089631C8, fighter_move_start_32b) { move_start_32(c, 0x089631ECu, 0x08963240u, 0x08963220u); }

/* (fighter): in the air (+386 not negative) once 0x0886C2B4 allows it (+236 = 0xD), on the ground once
   0x0886BF84 does (+236 = 3, +11 = +697): the move, 0x0886DBE8's answer; else 0. */
SFA_FUNC(0892AD84, fighter_move_start_d3) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = f;
    int ok = 0;
    if (S8(f + 73) != 0) {
        if (S8(f + 386) >= 0) {
            call_on(c, f, 0x0892ADB0u, f_0886C2B4);
            ok = c->r[2] != 0;
            if (ok) *B(f + 236) = 0xD;
        }
    } else {
        call_on(c, f, 0x0892ADFCu, f_0886BF84);
        ok = c->r[2] != 0;
        if (ok) { *B(f + 11) = (uint8_t)S8(f + 697); *B(f + 236) = 3; }
    }
    if (ok) move_go(c, f, 0x0892ADE4u);
    else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in the air over 0x28 above its ground once 0x0886C2B4 allows it (+236 = 0xB), on the ground
   once 0x0886BF84 does (+236 = 5, +11 = +697): the move; 1 then, else 0. */
SFA_FUNC(0893014C, fighter_move_start_b5) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if (S8(f + 73) != 0) {
        if (S16(f + 132) + 0x28 < S16(f + 22)) {
            call_on(c, f, 0x0893018Cu, f_0886C2B4);
            if (c->r[2] != 0) { c->r[17] = 1; *B(f + 236) = 0xB; }
        }
    } else {
        call_on(c, f, 0x089301A8u, f_0886BF84);
        if (c->r[2] != 0) { *B(f + 236) = 5; c->r[17] = 1; *B(f + 11) = (uint8_t)S8(f + 697); }
    }
    if (c->r[17] != 0) move_go(c, f, 0x089301ECu);
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, fighter): off (+1) and gone (+4 on) once the fighter is no longer its character (+338 =
   +3); its level (+296's +10): none clears +127, 160 or more (or under that with +0x338's bit 0x80)
   runs 0x08981C50, else +127 cleared. */
SFA_FUNC(08981B98, effect_on_fighter_level_gate) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = ra;
    if ((uint32_t)S8(p + 338) != *B(o + 3)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t lv = *B(*W(p + 296) + 10);
        if (lv == 0) *B(o + 127) = 0;
        else if (!((int32_t)lv < 160)) call2(c, o, p, 0x08981C44u, f_08981C50);
        else if (S8(p + 0x338) & 0x80) call2(c, o, p, 0x08981C1Cu, f_08981C50);
        else *B(o + 127) = 0;
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
