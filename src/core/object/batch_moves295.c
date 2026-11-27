/* More character moves behind their checks: the ones that need the guard bar broken or +254, and
   super starts for the choice 0. No test scenario plays these: written from the original code alone.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_08828C04(Cpu *c); void f_0886C6EC(Cpu *c); void f_0886D2EC(Cpu *c);
void f_089439DC(Cpu *c); void f_0886C5D0(Cpu *c);

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

/* (fighter): with +251 or +254, a move behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent,
   +235 set for some, and 0x08828C04: 1; else 0. A frame keeping s0, s1 (1 across the last call). */
static void move_try_251(Cpu *c, uint32_t move, int set235, uint32_t back, uint32_t back_start) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t a = S8(o + 251);
    enter2(c, o, sv);
    uint32_t done = 0;
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, back);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = (uint8_t)move;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)facing;
            if (set235) *B(o + 235) = 1;
            call1(c, o, f_08828C04, back_start);
            done = 1;
        }
    }
    c->r[2] = done;
    leave2(c, sv);
}
SFA_FUNC(0899B13C, move_1_try_251_9b13c) { move_try_251(c, 1, 0, 0x0899B16Cu, 0x0899B1A0u); }
SFA_FUNC(0899ECE8, move_1_try_251_9ece8) { move_try_251(c, 1, 0, 0x0899ED18u, 0x0899ED4Cu); }
SFA_FUNC(089A31A0, move_1_try_251_a31a0) { move_try_251(c, 1, 0, 0x089A31D0u, 0x089A3204u); }
SFA_FUNC(089A6374, move_1_try_251_a6374) { move_try_251(c, 1, 0, 0x089A63A4u, 0x089A63D8u); }
SFA_FUNC(0895C4FC, move_0_try_251_5c4fc) { move_try_251(c, 0, 1, 0x0895C52Cu, 0x0895C564u); }
SFA_FUNC(08925ED0, move_9_try_251_25ed0) { move_try_251(c, 9, 1, 0x08925F00u, 0x08925F3Cu); }
SFA_FUNC(089303F0, move_9_try_251_303f0) { move_try_251(c, 9, 1, 0x08930420u, 0x0893045Cu); }

/* (fighter): with +251 or +254, behind 0x0886BF84: state 1 / 0 / 7 / 0, the move set by 0x089439DC
   (fighter, 1), and 0x08828C04: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(08943948, move_by_439dc_try_251) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t a = S8(o + 251);
    enter2(c, o, sv);
    uint32_t done = 0;
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, 0x08943978u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            c->r[17] = 1;
            call2(c, o, 1, f_089439DC, 0x089439A4u);
            call1(c, o, f_08828C04, 0x089439ACu);
            done = 1;
        }
    }
    c->r[2] = done;
    leave2(c, sv);
}

/* (fighter): with the choice 0 and 48 of meter, move 7 behind 0x0886C6EC: state 1 / 0 / 8 / 0 facing
   the opponent (+78 too), +235 set, and 0x0886D2EC: 1; else 0. A frame keeping s0, s1. */
SFA_FUNC(088F79C0, move_7_try_super) {
    gaddr o = ARG(0); uint32_t sv[3];
    int32_t choice = S8(o + 386);
    enter2(c, o, sv);
    c->r[17] = 0;
    if (choice == 0 && !(S16(o + 366) < 48)) {
        call1(c, o, f_0886C6EC, 0x088F79F8u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 7; *B(o + 11) = (uint8_t)facing; *B(o + 78) = (uint8_t)facing; *B(o + 235) = 1;
            c->r[17] = 1;
            call1(c, o, f_0886D2EC, 0x088F7A3Cu);
        }
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* (fighter): with the choice 0 and 48 of meter, move 12 behind 0x0886C6EC: state 1 / 0 / 8 / 0 facing
   the opponent, +235 / +552 / +194 cleared, and 0x08828C04: 1; else 0. */
SFA_FUNC(0892A59C, move_12_try_super) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t choice = S8(o + 386);
    enter(c, o, sv);
    uint32_t done = 0;
    if (choice == 0 && !(S16(o + 366) < 48)) {
        call1(c, o, f_0886C6EC, 0x0892A5CCu);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            *B(o + 236) = 0xC; *B(o + 235) = 0;
            int8_t facing = S8(o + 697);
            *B(o + 552) = 0; *B(o + 194) = 0; *B(o + 11) = (uint8_t)facing;
            call1(c, o, f_08828C04, 0x0892A60Cu);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* (fighter): with the choice 0, +600 set and a full 144 of meter, move 14 behind 0x0886C5D0: state 1
   / 0 / 8 / 0 facing the opponent, +552 and the strength 2: 1; else 0. Two copies. */
static void move_14_try_full(Cpu *c, uint32_t back) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t choice = S8(o + 386);
    enter(c, o, sv);
    uint32_t done = 0;
    if (choice == 0 && *W(o + 600) != 0 && !(S16(o + 366) < 144)) {
        call1(c, o, f_0886C5D0, back);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 0xE; *B(o + 11) = (uint8_t)facing; *B(o + 552) = 2; *B(o + 194) = 2;
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}
SFA_FUNC(0898EDC0, move_14_try_full_8edc0) { move_14_try_full(c, 0x0898EDFCu); }
SFA_FUNC(089946D0, move_14_try_full_946d0) { move_14_try_full(c, 0x0899470Cu); }
