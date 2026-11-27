/* More character move starts and steps, most of them not played by any test scenario and written
   from the original code alone; also the two play clocks. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088616EC(Cpu *c); void f_08822EAC(Cpu *c); void f_0886163C(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886C508(Cpu *c); void f_08987440(Cpu *c); void f_08861848(Cpu *c); void f_08864888(Cpu *c);
void f_0886B468(Cpu *c); void f_0886CA2C(Cpu *c); void f_08864740(Cpu *c); void f_089633BC(Cpu *c);
void f_08828C04(Cpu *c); void f_0886DBE8(Cpu *c); void f_0886DB94(Cpu *c);

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
static void state(gaddr o, uint32_t a, uint32_t b, uint32_t d, uint32_t e) {
    *B(o + 4) = (uint8_t)a; *B(o + 5) = (uint8_t)b; *B(o + 6) = (uint8_t)d; *B(o + 7) = (uint8_t)e;
}

/* (fighter): move 11's counter start facing the opponent: state 1 / 0 / 7 / 0, +235 and +777 set,
   +696 / +127 cleared, the opponent's +127 = 0x1A, +573 = 0x1E, the animation 0x19: 1. s0 holds 1
   across the call. */
SFA_FUNC(0897C42C, move_11_counter_start) {
    gaddr o = ARG(0), other = *W(o + 92);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    state(o, 1, 0, 7, 0);
    *B(o + 236) = 0xB; *B(o + 696) = 0;
    *W(c->r[29] + 16) = s0; c->r[16] = 1;
    int8_t facing = S8(o + 697);
    *B(o + 235) = 1; *B(o + 127) = 0; *B(o + 11) = (uint8_t)facing;
    *B(other + 127) = 0x1A;
    *B(o + 573) = 0x1E; *B(o + 777) = 1;
    *W(c->r[29] + 20) = ra;
    ARG(1) = 0x19; c->r[31] = 0x0897C490u; f_088616EC(c);
    c->r[2] = c->r[16];
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 10, hidden, with +82 = 0x80: two new objects of kind 0x3E (0 and 1) owned by the
   fighter, the second only when the first found room, and the cue 0x13. */
SFA_FUNC(08989878, step_10_spawn_3e_pair) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 0xA; *B(o + 1) = 0; *H(o + 82) = 0x80;
    *W(c->r[29] + 20) = sv[1];
    call2(c, 0x3E, 0, f_08822EAC, 0x089898A8u);
    if (c->r[2] != 0) {
        *W(c->r[2] + 88) = o;
        call2(c, 0x3E, 1, f_08822EAC, 0x089898C4u);
        if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    }
    call2(c, o, 0x13, f_0886163C, 0x089898E0u);
    leave(c, sv);
}

/* (fighter): with +600 set, move 7 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent, +235
   set: 1; else 0. */
SFA_FUNC(08994448, move_7_try_600) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t have = *W(o + 600);
    enter(c, o, sv);
    uint32_t done = 0;
    if (have != 0) {
        call1(c, o, f_0886BF84, 0x08994468u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            *B(o + 236) = 7;
            int8_t facing = S8(o + 697);
            *B(o + 235) = 1; *B(o + 11) = (uint8_t)facing;
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* A play clock, one frame on: frames (60 to a second), seconds (60 to a minute) and minutes, which
   stop at 99 with the rest at 0. */
static void clock_tick(gaddr minutes, gaddr seconds, gaddr frames) {
    int32_t m = S8(minutes);
    if (m == 99) return;
    *B(frames) = (uint8_t)(S8(frames) + 1);
    if (S8(frames) < 60) return;
    int32_t s = S8(seconds);
    *B(frames) = 0;
    *B(seconds) = (uint8_t)(s + 1);
    if (S8(seconds) < 60) return;
    *B(minutes) = (uint8_t)(m + 1);
    *B(seconds) = 0;
    if (S8(minutes) < 100) return;
    *B(minutes) = 99; *B(seconds) = 0; *B(frames) = 0;
}

/* (): the clock at 0x09517A20. No frame. */
SFA_FUNC(08825D08, clock_tick_17a20) { (void)c; clock_tick(0x09517A20u, 0x09517A21u, 0x09517A22u); }

/* (): the clock at 0x095179EC. No frame. */
SFA_FUNC(08825C88, clock_tick_179ec) { (void)c; clock_tick(0x095179ECu, 0x095179EDu, 0x095179EEu); }

/* (fighter): with a choice of 0 or more and +568 clear, move 6 behind 0x0886C508: state 1 / 0 / 7 /
   0, +235 cleared: 1; else 0. */
SFA_FUNC(0893D1B0, move_6_try_choice) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t choice = S8(o + 386);
    enter(c, o, sv);
    uint32_t done = 0;
    if (!(choice < 0) && S8(o + 568) == 0) {
        call1(c, o, f_0886C508, 0x0893D1DCu);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            *B(o + 236) = 6; *B(o + 235) = 0;
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* (fighter): a fall after 0x08987440: ticked while rising, or still more than 32 above the ground
   (+132); else set down (+22, +73 and +20 cleared) and onto the ground (0x08864888). */
SFA_FUNC(08984EA8, fall_then_ground) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x08984EBCu; f_08987440(c);
    if (!((int32_t)*W(o + 100) < 0)) call1(c, o, f_08861848, 0x08984ED0u);
    else {
        int32_t ground = S16(o + 132);
        if (ground - 32 < S16(o + 22)) call1(c, o, f_08861848, 0x08984EF4u);
        else {
            *H(o + 22) = (uint16_t)ground;
            *B(o + 73) = 0;
            *H(o + 20) = 0;
            call1(c, o, f_08864888, 0x08984F14u);
        }
    }
    leave(c, sv);
}

/* (fighter): the paid escape: the opponent's (0x0886B468) +356 = 10, +553 set, +554 down by one,
   +266 cleared, the meter pays 72, and the character's own start for an opponent on the ground
   (+416's +104) or in the air (+108). */
SFA_FUNC(08863940, escape_paid_start) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x08863954u; f_0886B468(c);
    *B(c->r[2] + 356) = 10;
    int32_t left = S8(o + 554);
    *B(o + 553) = 1;
    *B(o + 554) = (uint8_t)(left - 1);
    *B(o + 266) = 0;
    call2(c, o, 0xFFFFFFB8u, f_0886CA2C, 0x08863980u);
    int air = S8(*W(o + 92) + 73) != 0;
    gaddr start = *W(*W(o + 416) + (air ? 108u : 104u));
    c->r[5] = start; ARG(0) = o; c->r[31] = air ? 0x088639B0u : 0x0886399Cu;
    rt_call_indirect(c, start);
    leave(c, sv);
}

/* (fighter): while the fight is held (BATTLE + 7 / 13) +262 is cleared and it crouches (0x08864740);
   else the next step with +0x3C0 cleared and +262 set, the meter by 6 and the animation 0x20. */
SFA_FUNC(08938A5C, next_step_or_crouch) {
    gaddr o = ARG(0); uint32_t sv[2];
    int held = S8(BATTLE + 7) | S8(BATTLE + 13);
    enter(c, o, sv);
    if (held != 0) {
        *B(o + 262) = 0;
        call1(c, o, f_08864740, 0x08938ACCu);
    } else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 0x3A0 + 32) = 0;
        *B(o + 262) = 1;
        call2(c, o, 6, f_0886CA2C, 0x08938AB0u);
        call2(c, o, 0x20, f_088616EC, 0x08938ABCu);
    }
    leave(c, sv);
}

/* (fighter): move 12 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent, +78 the other way,
   and 0x089633BC: 1; else 0. */
SFA_FUNC(08963490, move_12_try_633bc) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x089634A4u; f_0886BF84(c);
    if (c->r[2] != 0) {
        int8_t facing = S8(o + 697);
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7;
        *B(o + 78) = (uint8_t)facing;
        *B(o + 7) = 0;
        int8_t now = S8(o + 78);
        *B(o + 236) = 0xC; *B(o + 11) = (uint8_t)facing;
        *B(o + 78) = (uint8_t)(now ^ 1);
        call1(c, o, f_089633BC, 0x089634E8u);
        c->r[2] = 1;
    } else c->r[2] = 0;
    leave(c, sv);
}

/* (fighter): unless the choice is -1, move 9 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the
   opponent, +235 cleared, and 0x08828C04: 1; else 0. */
SFA_FUNC(088EF0D8, move_9_try_choice) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t choice = S8(o + 386);
    enter(c, o, sv);
    uint32_t done = 0;
    if (choice != -1) {
        call1(c, o, f_0886BF84, 0x088EF0FCu);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 9; *B(o + 11) = (uint8_t)facing; *B(o + 235) = 0;
            call1(c, o, f_08828C04, 0x088EF134u);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* (fighter): with +251 or +254, move 11 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent
   and 0x08828C04: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(0898454C, move_11_try_251) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t a = S8(o + 251);
    enter2(c, o, sv);
    c->r[17] = 0;
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, 0x08984580u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 0xB;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)facing;
            call1(c, o, f_08828C04, 0x089845B8u);
        }
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* A move behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent, +235 set, and the strength by
   the buttons (`pick`): 1; else 0. A frame keeping s0, s1. */
static void move_try_pick(Cpu *c, uint32_t move, void (*pick)(Cpu *), uint32_t back, uint32_t back_pick) {
    gaddr o = ARG(0); uint32_t sv[3]; enter2(c, o, sv);
    c->r[31] = back; f_0886BF84(c);
    if (c->r[2] != 0) {
        state(o, 1, 0, 7, 0);
        *B(o + 236) = (uint8_t)move;
        c->r[17] = 1;
        int8_t facing = S8(o + 697);
        *B(o + 235) = 1; *B(o + 11) = (uint8_t)facing;
        call1(c, o, pick, back_pick);
        c->r[2] = c->r[17];
    } else c->r[2] = 0;
    leave2(c, sv);
}
SFA_FUNC(0898A568, move_12_try_kicks)   { move_try_pick(c, 0xC, f_0886DBE8, 0x0898A580u, 0x0898A5BCu); }
SFA_FUNC(089945AC, move_11_try_punches) { move_try_pick(c, 0xB, f_0886DB94, 0x089945C4u, 0x08994600u); }
