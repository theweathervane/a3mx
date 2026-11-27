/* More character move starts no test scenario plays (written from the original code alone), the
   other direction of the stage pick, a throw check, and objects two moves bring out. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0887C4CC(Cpu *c); void f_0890EFE0(Cpu *c); void f_08861674(Cpu *c);
void f_08822EAC(Cpu *c); void f_088616EC(Cpu *c); void f_0886DBE8(Cpu *c); void f_08828C04(Cpu *c);
void f_0886AFB0(Cpu *c); void f_0886C6EC(Cpu *c); void f_0886D3E0(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* A 0x20 frame with s0 at +16 and the return address at +20; s0 holds the fighter. */
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
/* The same with s1 as well (+20) and the return address at +24. */
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
/* A 0x20 frame with only the return address, at +16. */
static uint32_t enter0(Cpu *c) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    return ra;
}
static void leave0(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void state(gaddr o, uint32_t a, uint32_t b, uint32_t d, uint32_t e) {
    *B(o + 4) = (uint8_t)a; *B(o + 5) = (uint8_t)b; *B(o + 6) = (uint8_t)d; *B(o + 7) = (uint8_t)e;
}

/* (fighter, strength): the strength kept in +0x3E0; move 5 behind 0x0886BF84: state 1 / 0 / 7 / 0
   with that strength, facing the opponent, +235 set: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(089B0EA4, move_5_try_strength) {
    gaddr o = ARG(0); uint32_t sv[3]; enter2(c, o, sv);
    c->r[17] = o + 0x3D8;
    *B(o + 0x3D8 + 8) = (uint8_t)ARG(1);
    c->r[31] = 0x089B0EC4u; f_0886BF84(c);
    if (c->r[2] != 0) {
        state(o, 1, 0, 7, 0);
        *B(o + 236) = 5;
        int8_t strength = S8(o + 0x3D8 + 8), facing = S8(o + 697);
        *B(o + 194) = (uint8_t)strength;
        *B(o + 11) = (uint8_t)facing; *B(o + 235) = 1;
        c->r[2] = 1;
    } else c->r[2] = 0;
    leave2(c, sv);
}

/* (battle): the battle's +79 from the list 0x08A0E778, stepping the place 0x08B01F34 on by 4; from
   8 or more it goes back 8 instead, and back 8 too when the entry is the list's end (255). Then
   0x0887C4CC (0, 0x38, 0xC0). The way back is 0x0882C4D0. */
SFA_FUNC(0882C384, pick_79_from_list_on) {
    gaddr b = ARG(0);
    const gaddr at = 0x08B01F34u, list = 0x08A0E778u;
    int32_t k = S8(at);
    uint32_t ra = enter0(c);
    if (k + 4 < 12) {
        *B(at) = (uint8_t)(k + 4);
        int32_t k2 = S8(at);
        uint32_t v = *B(list + (uint32_t)k2);
        if (v == 0xFF) {
            *B(at) = (uint8_t)(k2 - 8);
            v = *B(list + (uint32_t)(int32_t)S8(at));
        }
        *B(b + 79) = (uint8_t)v;
    } else {
        *B(at) = (uint8_t)(k - 8);
        *B(b + 79) = *B(list + (uint32_t)(int32_t)S8(at));
    }
    c->r[6] = 0xC0;
    call2(c, 0, 0x38, f_0887C4CC, 0x0882C404u);
    leave0(c, ra);
}

/* (fighter): +0x334 = 1; a player with the choice 1 takes the animation twice the strength (plus 1
   unless back is held, +290 bit 0) by 0x08861674; anyone else 0x0890EFE0. */
SFA_FUNC(0890EED0, anim_by_strength_choice_1) {
    gaddr o = ARG(0);
    c->r[5] = 1;
    *B(o + 0x334) = 1;
    int32_t cpu = S8(o + 373);
    uint32_t ra = enter0(c);
    if (cpu != 0) { c->r[31] = 0x0890EEF8u; f_0890EFE0(c); }
    else if (S8(o + 386) != 1) { c->r[31] = 0x0890EF14u; f_0890EFE0(c); }
    else {
        uint32_t stick = *H(o + 290);
        uint32_t anim = (uint32_t)(int32_t)S8(o + 194) << 1;
        if (stick & 1) { ARG(1) = anim & 0xFFFF; c->r[31] = 0x0890EF50u; f_08861674(c); }
        else { ARG(1) = (anim + 1) & 0xFFFF; c->r[31] = 0x0890EF3Cu; f_08861674(c); }
    }
    leave0(c, ra);
}

/* (fighter): a new object of kind 0x29 / 11 owned by the fighter, at its place and facing with its
   +12..+14, +24, +38, +86 and +164. */
SFA_FUNC(0891F038, spawn_29_like_owner) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call2(c, 0x29, 0xB, f_08822EAC, 0x0891F054u);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 14) = *B(o + 14);
        *B(n + 12) = (uint8_t)S8(o + 12);
        *B(n + 13) = *B(o + 13);
        *H(n + 38) = (uint16_t)S16(o + 38);
        uint16_t h24 = *H(o + 24);
        *W(n + 88) = o;
        *H(n + 24) = h24;
        *H(n + 18) = (uint16_t)S16(o + 18);
        *H(n + 22) = (uint16_t)S16(o + 22);
        *B(n + 11) = *B(o + 11);
        *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
    }
    leave(c, sv);
}

/* (fighter): by the opponent: in the air step 5 with the animation 0x19; crouching, or lower than
   the fighter by more than +820, step 4 with 0x18; else step 5 with 0x19. */
SFA_FUNC(0895DD9C, step_by_opponent_height) {
    gaddr o = ARG(0), other = *W(o + 92);
    int32_t air = S8(other + 73);
    uint32_t ra = enter0(c);
    if (air != 0) { *B(o + 7) = 5; ARG(1) = 0x19; c->r[31] = 0x0895DDC4u; f_088616EC(c); }
    else if (S8(other + 696) != 0 || (int32_t)*W(o + 820) < S16(o + 22) - S16(other + 22)) {
        *B(o + 7) = 4; ARG(1) = 0x18; c->r[31] = 0x0895DE04u; f_088616EC(c);
    } else { *B(o + 7) = 5; ARG(1) = 0x19; c->r[31] = 0x0895DE1Cu; f_088616EC(c); }
    leave0(c, ra);
}

/* (fighter): move 5 from its step 1, behind 0x0886BF84: state 1 / 0 / 7 / 1, +836 cleared, +193 and
   +235 set, facing the opponent, the strength by the kicks (0x0886DBE8): 1; else 0. A frame keeping
   s0, s1. */
SFA_FUNC(0898A284, move_5_try_step_1) {
    gaddr o = ARG(0); uint32_t sv[3]; enter2(c, o, sv);
    c->r[31] = 0x0898A29Cu; f_0886BF84(c);
    if (c->r[2] != 0) {
        state(o, 1, 0, 7, 1);
        *B(o + 236) = 5;
        *W(o + 836) = 0;
        c->r[17] = 1;
        int8_t facing = S8(o + 697);
        *B(o + 193) = 1; *B(o + 235) = 1; *B(o + 11) = (uint8_t)facing;
        call1(c, o, f_0886DBE8, 0x0898A2E0u);
        c->r[2] = c->r[17];
    } else c->r[2] = 0;
    leave2(c, sv);
}

/* (fighter): with +251 or +254, move 21 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent,
   +235 cleared, and 0x08828C04: 1; else 0. Two copies. */
static void move_21_try_251(Cpu *c, uint32_t back, uint32_t back_start) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t a = S8(o + 251);
    enter(c, o, sv);
    uint32_t done = 0;
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, back);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 0x15; *B(o + 235) = 0; *B(o + 11) = (uint8_t)facing;
            call1(c, o, f_08828C04, back_start);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}
SFA_FUNC(0898E978, move_21_try_251_8e978) { move_21_try_251(c, 0x0898E9A4u, 0x0898E9DCu); }
SFA_FUNC(08994144, move_21_try_251_94144) { move_21_try_251(c, 0x08994170u, 0x089941A8u); }

/* (fighter, kind): a throw check against an opponent who can be thrown (+573 and +702 clear) and has
   any of the three bytes its +256 points at set: 0x0886AFB0 (fighter, opponent, kind); else 0. */
SFA_FUNC(0886AC80, throw_check_256) {
    gaddr o = ARG(0); uint32_t kind = ARG(1);
    gaddr other = *W(o + 92);
    int held = S8(other + 573) | S8(other + 702);
    uint32_t ra = enter0(c);
    c->r[2] = 0;
    if (held == 0) {
        gaddr p = *W(other + 256);
        if (*B(p) != 0 || *B(p + 1) != 0 || *B(p + 2) != 0) {
            c->r[6] = kind; c->r[7] = kind;
            call2(c, o, other, f_0886AFB0, 0x0886AD04u);
        }
    }
    leave0(c, ra);
}

/* (fighter): with the choice 0 and 48 of meter, move 11 behind 0x0886C6EC: state 1 / 0 / 8 / 0
   facing the opponent, +235 cleared, and 0x08828C04: 1; else 0. */
SFA_FUNC(08925E40, move_11_try_meter) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t choice = S8(o + 386);
    enter(c, o, sv);
    uint32_t done = 0;
    if (choice == 0 && !(S16(o + 366) < 48)) {
        call1(c, o, f_0886C6EC, 0x08925E70u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 0xB; *B(o + 11) = (uint8_t)facing; *B(o + 235) = 0;
            call1(c, o, f_08828C04, 0x08925EA8u);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* (fighter): step 4: a new object of kind 0x3B / 1 owned by the fighter (+9 = 3, +78 = 0x4B), across
   from it by +0x3B8, on the ground (+132), on its side (+337 into +86) with its +164; the animation
   0x38. */
SFA_FUNC(089371B8, step_4_spawn_3b) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 4;
    *W(c->r[29] + 20) = sv[1];
    call2(c, 0x3B, 1, f_08822EAC, 0x089371DCu);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 9) = 3;
        *W(n + 16) = *W(o + 16) + *W(o + 0x3A0 + 24);
        int16_t ground = S16(o + 132);
        *B(n + 78) = 0x4B;
        *H(n + 22) = (uint16_t)ground;
        *W(n + 88) = o;
        *H(n + 86) = (uint16_t)(int16_t)S8(o + 337);
        *W(n + 164) = *W(o + 164);
    }
    call2(c, o, 0x38, f_088616EC, 0x08937238u);
    leave(c, sv);
}

/* (fighter): with the choice 0, move 5 behind 0x0886C6EC: state 1 / 0 / 8 / 0 facing the opponent,
   +235 set, and 0x0886D3E0: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(08963138, move_5_try_super) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t choice = S8(o + 386);
    enter2(c, o, sv);
    uint32_t done = 0;
    if (choice == 0) {
        call1(c, o, f_0886C6EC, 0x0896315Cu);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 5;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)facing; *B(o + 235) = 1;
            call1(c, o, f_0886D3E0, 0x08963198u);
            done = 1;
        }
    }
    c->r[2] = done;
    leave2(c, sv);
}
