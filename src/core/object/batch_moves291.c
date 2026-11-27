/* More character move starts and steps no test scenario plays, written from the original code alone.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_08828C04(Cpu *c); void f_089DEA64(Cpu *c); void f_0886CA2C(Cpu *c);
void f_088616EC(Cpu *c); void f_0886DEC8(Cpu *c); void f_0886C508(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DD68(Cpu *c); void f_08863114(Cpu *c); void f_08861674(Cpu *c); void f_089238A0(Cpu *c);
void f_08821330(Cpu *c); void f_0886163C(Cpu *c);

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
/* The same with s1 as well (+20) and the return address at +24; s1 starts at 0. */
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o; c->r[17] = 0;
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

/* A move for a fighter with +251 or +254, behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent,
   an optional call first, and 0x08828C04. */
static void move_try_251(Cpu *c, uint32_t move, void (*first)(Cpu *), const uint32_t back[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t a = S8(o + 251);
    enter(c, o, sv);
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, back[0]);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = (uint8_t)move; *B(o + 11) = (uint8_t)facing;
            if (first) call1(c, o, first, back[1]);
            call1(c, o, f_08828C04, back[2]);
        }
    }
    leave(c, sv);
}
SFA_FUNC(089B09E0, move_6_try_251) {
    static const uint32_t b[3] = { 0x089B0A0Cu, 0, 0x089B0A40u };
    move_try_251(c, 6, 0, b);
}
SFA_FUNC(08920A38, move_8_try_251) {
    static const uint32_t b[3] = { 0x08920A64u, 0x08920A98u, 0x08920AA0u };
    move_try_251(c, 8, f_089238A0, b);
}

/* (object): 0x089DEA64 run with the object's +112..+116 standing in from +144..+148, and put back
   afterwards. A 0x20 frame keeping s0 at +24, the three kept at +16. */
SFA_FUNC(089DEB18, call_with_144) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s0; c->r[16] = o;
    int16_t a = S16(o + 112), b = S16(o + 114);
    *H(sp + 16) = (uint16_t)a;
    int16_t d = S16(o + 116);
    *H(sp + 18) = (uint16_t)b;
    int16_t x = S16(o + 144);
    *H(sp + 20) = (uint16_t)d;
    int16_t y = S16(o + 146);
    *H(o + 112) = (uint16_t)x;
    int16_t z = S16(o + 148);
    *H(o + 114) = (uint16_t)y; *H(o + 116) = (uint16_t)z;
    *W(sp + 28) = ra;
    call1(c, o, f_089DEA64, 0x089DEB60u);
    *H(o + 112) = *H(sp + 16); *H(o + 114) = *H(sp + 18); *H(o + 116) = *H(sp + 20);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 12: 5.5 up with gravity 0x1800 and 2.0 across by its facing, the meter by 8, the
   animation 0x3C and 0x0886DEC8. */
SFA_FUNC(088FAA74, step_12_float_anim_3c) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 0xC;
    *W(o + 100) = 0x58000; *W(o + 108) = 0xFFFFE800u; *W(o + 104) = 0;
    uint8_t facing = *B(o + 11);
    *W(c->r[29] + 20) = sv[1];
    *W(o + 96) = facing != 0 ? 0x20000u : 0xFFFE0000u;
    call2(c, o, 8, f_0886CA2C, 0x088FAAC4u);
    call2(c, o, 0x3C, f_088616EC, 0x088FAAD0u);
    call1(c, o, f_0886DEC8, 0x088FAAD8u);
    leave(c, sv);
}

/* (fighter): unless +568, move 6 behind 0x0886C508: state 1 / 0 / 7 / 0, +235 set, the middle
   strength: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(08909BDC, move_6_try_568) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t off = S8(o + 568);
    enter2(c, o, sv);
    if (off == 0) {
        call1(c, o, f_0886C508, 0x08909C04u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            *B(o + 236) = 6; *B(o + 235) = 1;
            c->r[17] = 1;
            *B(o + 194) = 1;
        }
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* (fighter): with +251 clear, move 10 behind 0x0886C2B4: state 1 / 0 / 8 / 0 and the timings by the
   buttons: 1; else 0. */
SFA_FUNC(089208A4, move_10_try_8) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t off = S8(o + 251);
    enter(c, o, sv);
    uint32_t done = 0;
    if (off == 0) {
        call1(c, o, f_0886C2B4, 0x089208C4u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            *B(o + 236) = 0xA;
            call1(c, o, f_0886DD68, 0x089208F0u);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* (fighter): move 10 from its step 3, behind 0x0886BF84: state 1 / 0 / 7 / 3 facing the opponent and
   0x08828C04: 1; else 0. */
SFA_FUNC(0892A748, move_10_try_step_3) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x0892A75Cu; f_0886BF84(c);
    if (c->r[2] != 0) {
        state(o, 1, 0, 7, 3);
        int8_t facing = S8(o + 697);
        *B(o + 236) = 0xA; *B(o + 11) = (uint8_t)facing;
        call1(c, o, f_08828C04, 0x0892A794u);
        c->r[2] = 1;
    } else c->r[2] = 0;
    leave(c, sv);
}

/* (fighter): move 9 at step 0 into the air: +82 down by 30 (not below 0), 6.0 up with gravity 0x6000
   and 1.0 across by its facing, with the animation 5. */
SFA_FUNC(089745AC, move_9_leap_less_30) {
    gaddr o = ARG(0);
    int32_t v = S16(o + 82);
    *B(o + 236) = 9;
    v -= 30;
    *B(o + 7) = 0;
    *H(o + 82) = (uint16_t)v;
    uint32_t ra = enter0(c);
    if ((int16_t)v < 0) *H(o + 82) = 0;
    *B(o + 73) = 1;
    *W(o + 100) = 0x60000; *W(o + 108) = 0xFFFFA000u; *W(o + 104) = 0;
    *W(o + 96) = *B(o + 11) != 0 ? 0x10000u : 0xFFFF0000u;
    ARG(1) = 5; c->r[31] = 0x08974614u; f_088616EC(c);
    leave0(c, ra);
}

/* A counter's start for move 2: state 1 / 0 / 7 / step facing the opponent, +235 and +777 set, +696,
   +127 and +193 cleared, optionally the heavy strength, the opponent's +127 marked, the wait +573 and
   the animation. */
static void counter_2_start(Cpu *c, uint32_t step, int heavy, uint32_t mark, uint32_t wait, uint32_t anim,
                            uint32_t back) {
    gaddr o = ARG(0), other = *W(o + 92);
    state(o, 1, 0, 7, step);
    *B(o + 236) = 2; *B(o + 696) = 0; *B(o + 235) = 1;
    int8_t facing = S8(o + 697);
    if (heavy) *B(o + 194) = 2;
    *B(o + 127) = 0; *B(o + 11) = (uint8_t)facing;
    *B(other + 127) = (uint8_t)mark;
    *B(o + 573) = (uint8_t)wait; *B(o + 193) = 0; *B(o + 777) = 1;
    uint32_t ra = enter0(c);
    ARG(1) = anim; c->r[31] = back; f_088616EC(c);
    leave0(c, ra);
}
SFA_FUNC(0899EDE0, counter_2_start_9ede0) { counter_2_start(c, 0, 1, 0x21, 0x25, 0x36, 0x0899EE48u); }
SFA_FUNC(089A3678, counter_2_start_a3678) { counter_2_start(c, 4, 0, 0x1C, 0x20, 0x1E, 0x089A36E0u); }

/* (object): step 4 into the air (+594 cleared): 4.0 across slowing by 0x1000, turned by its facing,
   7.5 up with gravity 0.5, and the animation 14. */
SFA_FUNC(089BAB0C, step_4_arc_anim_e) {
    gaddr o = ARG(0);
    *B(o + 7) = 4; *B(o + 594) = 0; *B(o + 73) = 1;
    *W(o + 96) = 0x40000; *W(o + 104) = 0xFFFFF000u; *W(o + 100) = 0x78000;
    uint8_t facing = *B(o + 11);
    uint32_t ra = enter0(c);
    *W(o + 108) = 0xFFFF8000u;
    if (facing == 0) {
        uint32_t vx = *W(o + 96), ax = *W(o + 104);
        *W(o + 96) = 0 - vx; *W(o + 104) = 0 - ax;
    }
    ARG(1) = 0xE; c->r[31] = 0x089BAB74u; f_088616EC(c);
    leave0(c, ra);
}

/* (fighter): with a choice of 0 or more, move 10 behind 0x0886BF84: facing the opponent, state 1 / 0
   / 7 / 0 and 0x08828C04: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(088F2D9C, move_10_try_choice) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t choice = S8(o + 386);
    enter2(c, o, sv);
    if (!(choice < 0)) {
        call1(c, o, f_0886BF84, 0x088F2DC4u);
        if (c->r[2] != 0) {
            int8_t facing = S8(o + 697);
            *B(o + 236) = 0xA; *B(o + 11) = (uint8_t)facing;
            c->r[17] = 1;
            state(o, 1, 0, 7, 0);
            call1(c, o, f_08828C04, 0x088F2DFCu);
        }
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* The next step facing the opponent with +235 set: 0x08863114, the meter by the strength, and the
   animation 12 + strength (15 + with +193). */
static void next_step_anim_c(Cpu *c, uint32_t back, uint32_t back2, uint32_t back3) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    int8_t facing = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)facing;
    *W(c->r[29] + 20) = sv[1];
    call2(c, o, 1, f_08863114, back);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, back2);
    uint32_t base = S8(o + 193) == 0 ? 0xC : 0xF;
    call2(c, o, (base + (uint32_t)(int32_t)S8(o + 194)) & 0xFFFF, f_08861674, back3);
    leave(c, sv);
}
SFA_FUNC(08914B60, next_step_anim_c_14b60) { next_step_anim_c(c, 0x08914B94u, 0x08914BA0u, 0x08914BC8u); }
SFA_FUNC(08925A84, next_step_anim_c_25a84) { next_step_anim_c(c, 0x08925AB8u, 0x08925AC4u, 0x08925AECu); }

/* (fighter): step 5, shown and in the air: placed from what 0x08821330 returns (its +18, 0x1C0 right
   or 0x40 left of it by the side +337), 0x40 higher, with the cue 0x22. */
SFA_FUNC(08929E18, step_5_place_cue_22) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 5; *B(o + 1) = 1; *B(o + 73) = 1;
    *W(c->r[29] + 20) = sv[1];
    call1(c, o, f_08821330, 0x08929E48u);
    int32_t d = S8(o + 337) != 0 ? 0x1C0 : -0x40;
    int32_t at = S16(c->r[2] + 18), y = S16(o + 22);
    *H(o + 18) = (uint16_t)(d + at);
    *H(o + 22) = (uint16_t)(y + 0x40);
    call2(c, o, 0x22, f_0886163C, 0x08929E80u);
    leave(c, sv);
}

/* (fighter): step 23 with +282 / +591 set and +82 = 35: in from its side of the screen (+337: at
   0x2A0 moving left, or 0x260 moving right, at 2.25), with the cue 2. */
SFA_FUNC(0896BA88, step_23_run_in) {
    gaddr o = ARG(0);
    *B(o + 7) = 0x17; *B(o + 282) = 1; *B(o + 591) = 1;
    int32_t side = S8(o + 337);
    uint32_t ra = enter0(c);
    *H(o + 82) = 0x23;
    if (side == 0) { *H(o + 18) = 0x2A0; *W(o + 96) = 0xFFFDC000u; *B(o + 11) = 0; }
    else { *H(o + 18) = 0x260; *W(o + 96) = 0x24000; *B(o + 11) = 1; }
    ARG(1) = 2; c->r[31] = 0x0896BAF4u; f_0886163C(c);
    leave0(c, ra);
}
