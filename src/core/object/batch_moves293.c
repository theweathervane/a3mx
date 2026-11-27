/* Character moves that need the guard bar broken or +254: ten of them share one shape. With them a
   stage pick from a list, a fall start by choice, and a move for choices of 0 or more. No test scenario
   plays these: written from the original code alone. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_08828C04(Cpu *c); void f_0887C4CC(Cpu *c); void f_088616EC(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);

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

/* (fighter): with +251 or +254, a move behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent
   and 0x08828C04: 1; else 0. */
static void move_try_251(Cpu *c, uint32_t move, uint32_t back, uint32_t back_start) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t a = S8(o + 251);
    enter(c, o, sv);
    uint32_t done = 0;
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, back);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            int8_t facing = S8(o + 697);
            *B(o + 236) = (uint8_t)move; *B(o + 11) = (uint8_t)facing;
            call1(c, o, f_08828C04, back_start);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}
SFA_FUNC(088FD978, move_6_try_251_fd978) { move_try_251(c, 6, 0x088FD9A4u, 0x088FD9D8u); }
SFA_FUNC(089062C8, move_7_try_251_062c8) { move_try_251(c, 7, 0x089062F4u, 0x08906328u); }
SFA_FUNC(0890FE90, move_7_try_251_0fe90) { move_try_251(c, 7, 0x0890FEBCu, 0x0890FEF0u); }
SFA_FUNC(089150E0, move_6_try_251_150e0) { move_try_251(c, 6, 0x0891510Cu, 0x08915140u); }
SFA_FUNC(089498F8, move_9_try_251_498f8) { move_try_251(c, 9, 0x08949924u, 0x08949958u); }
SFA_FUNC(08962CD8, move_6_try_251_62cd8) { move_try_251(c, 6, 0x08962D04u, 0x08962D38u); }
SFA_FUNC(08967848, move_7_try_251_67848) { move_try_251(c, 7, 0x08967874u, 0x089678A8u); }
SFA_FUNC(08973620, move_2_try_251_73620) { move_try_251(c, 2, 0x0897364Cu, 0x08973680u); }
SFA_FUNC(0897C264, move_8_try_251_7c264) { move_try_251(c, 8, 0x0897C290u, 0x0897C2C4u); }
SFA_FUNC(0898A7EC, move_6_try_251_8a7ec) { move_try_251(c, 6, 0x0898A818u, 0x0898A84Cu); }

/* (battle): the battle's +79 from the list 0x08A0E778, stepping the place 0x08B01F34 back by 4; from
   below 4 it goes on 8 first, and back 4 again when that entry is the list's end (255). Then
   0x0887C4CC (0, 0x38, 0xC0). */
SFA_FUNC(0882C4D0, pick_79_from_list) {
    gaddr b = ARG(0);
    const gaddr at = 0x08B01F34u, list = 0x08A0E778u;
    int32_t k = S8(at);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (k - 4 >= 0) {
        *B(at) = (uint8_t)(k - 4);
        *B(b + 79) = *B(list + (uint32_t)(int32_t)S8(at));
    } else {
        *B(at) = (uint8_t)(k + 8);
        int32_t k2 = S8(at);
        uint32_t v = *B(list + (uint32_t)k2);
        if (v == 0xFF) {
            *B(at) = (uint8_t)(k2 - 4);
            v = *B(list + (uint32_t)(int32_t)S8(at));
        }
        *B(b + 79) = (uint8_t)v;
    }
    c->r[6] = 0xC0;
    call2(c, 0, 0x38, f_0887C4CC, 0x0882C54Cu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, n): the next step with the animation n + 0x11 (n + 0x37 for a choice below 0), falling
   from -2.5 by 0x2000 a frame, the meter by 2 and 0x08837098. */
SFA_FUNC(089218B8, next_step_dive) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    uint8_t step = *B(o + 7);
    uint32_t n = ARG(1) & 0xFFFF;
    int32_t choice = S8(o + 386);
    *W(c->r[29] + 20) = sv[1];
    *B(o + 7) = (uint8_t)(step + 1);
    if (choice < 0) call2(c, o, (n + 0x37) & 0xFFFF, f_088616EC, 0x089218F0u);
    else call2(c, o, (n + 0x11) & 0xFFFF, f_088616EC, 0x08921908u);
    *W(o + 100) = 0xFFFD8000u; *W(o + 108) = 0xFFFFE000u;
    call2(c, o, 2, f_0886CA2C, 0x08921928u);
    call1(c, o, f_08837098, 0x08921930u);
    leave(c, sv);
}

/* (fighter): for a choice of 0 or more, move 14 behind 0x0886BF84: +235 cleared, state 1 / 0 / 7 / 0
   facing the opponent, +196 cleared, and 0x08828C04, whose result it returns; else 0. */
SFA_FUNC(0897BE54, move_14_try_choice) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    int32_t choice = S8(o + 386);
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *W(c->r[29] + 20) = sv[1];
    int started = 0;
    if (!(choice & 0x80)) {
        call1(c, o, f_0886BF84, 0x0897BE7Cu);
        if (c->r[2] != 0) {
            *B(o + 236) = 0xE; *B(o + 235) = 0;
            *B(o + 4) = 1; *B(o + 5) = 0;
            int8_t facing = S8(o + 697);
            *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 11) = (uint8_t)facing; *B(o + 196) = 0;
            call1(c, o, f_08828C04, 0x0897BEB8u);
            started = 1;
        }
    }
    if (!started) c->r[2] = 0;
    leave(c, sv);
}
