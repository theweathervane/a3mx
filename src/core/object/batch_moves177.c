/* Move steps of 38 instructions: landings into a move with a cue, a hop on a frame flag (with a creep
   before it), a landing that turns around or cues by the mark (0x08931D14), a wait into the fall
   stance (0x0893DFA8), and a pair of timed rises with their speeds by strength. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_0887C614(Cpu *c); void f_089291C0(Cpu *c); void f_0896A34C(Cpu *c); void f_0898F9DC(Cpu *c);
void f_08993090(Cpu *c); void f_08995134(Cpu *c); void f_0899939C(Cpu *c);

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

/* (fighter): moves by +96, 0x089291C0; at or below the floor +7 steps on, at the floor, +73 / +20 / +262
   cleared, cue 0x22 / 0x23 and move 0x11 (0x12 with strength); else ticked. */
SFA_FUNC(08926828, move_slide_land_11) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    call1(c, o, f_089291C0, 0x0892684Cu);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 0; *H(o + 20) = 0; *B(o + 262) = 0;
        call1(c, o, f_0887C614, 0x08926884u);
        call2(c, o, S8(o + 194) == 0 ? 0x11 : 0x12, f_088616EC, 0x089268A0u);
    } else call1(c, o, f_08861848, 0x089268B0u);
    leave(c, sv);
}

/* (fighter): ticked; a frame flag (cleared) starts a hop: +7 on, +73 = 1, x 1.5, fall -0.375, y by
   strength (0x08A6DB44); a negative flag moves it by +96. */
SFA_FUNC(08930EA0, move_tick_flag_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08930EB4u);
    int32_t k = S8(o + 77);
    if (k > 0) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 73) = 1;
        *W(o + 104) = 0;
        int32_t s = S8(o + 194);
        *W(o + 108) = 0xFFFFA000u; *W(o + 96) = 0x18000;
        *W(o + 100) = *W(0x08A6DB44u + ((uint32_t)s << 2));
    } else if (k < 0) *W(o + 16) = *W(o + 16) + *W(o + 96);
    leave(c, sv);
}

/* (fighter): lands (at the floor, +73 / +20 cleared); without the mark (+0x334) it turns around with
   step 6, x -2.0 / +0.125 and move 0x28; with it step 3, cue 0x22 / 0x23 and move 0x27. */
SFA_FUNC(08931D14, move_land_turn_or_cue) {
    gaddr o = ARG(0); uint32_t sv[2];
    *H(o + 22) = (uint16_t)S16(o + 132);
    *B(o + 73) = 0; *H(o + 20) = 0;
    int32_t m = S8(o + 0x334);
    enter(c, o, sv);
    if (m == 0) {
        *B(o + 7) = 6;
        *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
        *W(o + 96) = 0xFFFE0000u; *W(o + 104) = 0x2000;
        call2(c, o, 0x28, f_088616EC, 0x08931D78u);
    } else {
        *B(o + 7) = 3;
        call1(c, o, f_0887C614, 0x08931D90u);
        call2(c, o, 0x27, f_088616EC, 0x08931D9Cu);
    }
    leave(c, sv);
}

/* (fighter): it falls (+20 by +100 by +108); at or below the floor +6 steps on, +73 / y speeds / +20
   cleared, at the floor, cue 0x22 / 0x23 and move 0x11; else ticked. */
SFA_FUNC(089339BC, move_fall_land_6_11) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132);
    *W(o + 100) = vy + *W(o + 108);
    if ((floor & 0xFFFF) < (S16(o + 22) & 0xFFFF)) call1(c, o, f_08861848, 0x08933A0Cu);
    else {
        uint32_t n = *B(o + 6);
        *B(o + 73) = 0; *B(o + 6) = (uint8_t)(n + 1);
        *W(o + 108) = 0; *W(o + 100) = 0; *H(o + 20) = 0; *H(o + 22) = (uint16_t)floor;
        call1(c, o, f_0887C614, 0x08933A38u);
        call2(c, o, 0x11, f_0886163C, 0x08933A44u);
    }
    leave(c, sv);
}

/* (fighter): ticked; the +82 wait runs down, and out: in the air the fall stance (1 / 0 / 3 / 1, +262 and
   speeds cleared, fall -0.1875, move 0x21), on the ground standing. */
SFA_FUNC(0893DFA8, move_wait_fall_or_stand) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0893DFBCu);
    gaddr w = o + 0x52;
    int8_t t = (int8_t)(S8(w) - 1);
    *B(w) = (uint8_t)t;
    if (t == 0) {
        if (S8(o + 73) != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3; *B(o + 7) = 1; *B(o + 262) = 0;
            *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 96) = 0; *W(o + 108) = 0xFFFFD000u;
            call2(c, o, 0x21, f_0886163C, 0x0893E020u);
        } else call1(c, o, f_0886469C, 0x0893E030u);
    }
    leave(c, sv);
}

/* (fighter): 0x0896A34C; at or below the floor +7 steps on, at the floor, speeds / +20 / +73 cleared, 24
   back toward its facing and move 0x1C; else ticked. */
SFA_FUNC(0896831C, move_land_back_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896A34C, 0x08968330u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x0896834Cu);
    else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 96) = 0; *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 108) = 0; *H(o + 20) = 0;
        uint32_t f = *B(o + 11);
        *B(o + 73) = 0;
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(f != 0 ? x + 0x18 : x - 0x18);
        call2(c, o, 0x1C, f_088616EC, 0x089683A4u);
    }
    leave(c, sv);
}

/* (fighter): with +123 `alt`; else `check`, and the +82 timer runs down: out, +7 steps on with the y
   speeds by strength (`tab`: y, fall); ticked. */
static void timed_rise(Cpu *c, void (*alt)(Cpu *), void (*check)(Cpu *), gaddr tab, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    enter(c, o, sv);
    if (f != 0) call1(c, o, alt, b[0]);
    else {
        call1(c, o, check, b[1]);
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            gaddr e = tab + ((uint32_t)s << 3);
            *W(o + 100) = *W(e); *W(o + 108) = *W(e + 4);
        }
        call1(c, o, f_08861848, b[2]);
    }
    leave(c, sv);
}
SFA_FUNC(0898F77C, move_timed_rise) {
    static const uint32_t b[3] = { 0x0898F79Cu, 0x0898F7ACu, 0x0898F804u };
    timed_rise(c, f_0898F9DC, f_08993090, 0x08A9663Cu, b);
}
SFA_FUNC(08994ED4, move_timed_rise_b) {
    static const uint32_t b[3] = { 0x08994EF4u, 0x08994F04u, 0x08994F5Cu };
    timed_rise(c, f_08995134, f_0899939C, 0x08A98738u, b);
}
