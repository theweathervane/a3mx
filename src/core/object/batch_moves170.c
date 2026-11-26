/* Move steps of 34 instructions: falls that land into a move, a dash that marks where it started
   (0x08916878), a held opponent spawned as a puppet (0x0893E414), the timed recovery step, and per-charge
   move picks. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861674(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_08864740(Cpu *c); void f_0886DB2C(Cpu *c); void f_0890F5EC(Cpu *c); void f_08912FBC(Cpu *c);
void f_08912FEC(Cpu *c); void f_08822D2C(Cpu *c); void f_089426E4(Cpu *c); void f_0886E294(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): it falls (+20 by +100 by +108); above the floor it also slides by +96 toward its facing
   (+96 by +104; once that turns, step 3); ticked. */
SFA_FUNC(088F8158, move_fall_slide) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *W(o + 100) = vy + *W(o + 108);
    frame_in(c, &ra);
    if (floor < y) {
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + toward(o, v);
        uint32_t v2 = v + *W(o + 104);
        *W(o + 96) = v2;
        if ((int32_t)v2 < 0) *B(o + 7) = 3;
    }
    call1(c, o, f_08861848, 0x088F81D4u);
    frame_out(c, ra);
}

/* (fighter): locked (+373) or not choice 1 (+386): 0x0890F5EC; else move 2k + 6 (with +290 bit 0) or
   2k + 7. */
SFA_FUNC(0890F564, move_pick_lock_b) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 373) != 0) call1(c, o, f_0890F5EC, 0x0890F580u);
    else if (S8(o + 386) != 1) call1(c, o, f_0890F5EC, 0x0890F5A0u);
    else {
        uint32_t n = (uint32_t)S8(o + 194) << 1;
        if (*H(o + 290) & 1) call2(c, o, (n + 6) & 0xFFFF, f_08861674, 0x0890F5E0u);
        else call2(c, o, (n + 7) & 0xFFFF, f_08861674, 0x0890F5C8u);
    }
    frame_out(c, ra);
}

/* (fighter): ended: crouching; else ticked, but on a frame flag 0x08912FBC first, and moving back it
   ticks twice with +74 = 1. */
SFA_FUNC(0890F8F0, move_crouch_or_flag_tick) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e & 0x80) call1(c, o, f_08864740, 0x0890F918u);
    else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0890F930u);
    else {
        call1(c, o, f_08912FBC, 0x0890F940u);
        if (*W(o + 96) & 0x80000000u) { *B(o + 74) = 1; call1(c, o, f_08861848, 0x0890F960u); }
        call1(c, o, f_08861848, 0x0890F968u);
    }
    leave(c, sv);
}

/* (fighter): 0x08912FEC; falling below the floor it lands (step 1, at the floor, +73 cleared, move 0x20
   with its cancel data); else ticked. */
SFA_FUNC(0890F9D0, move_fall_land_20) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08912FEC, 0x0890F9E4u);
    if (*W(o + 100) & 0x80000000u) {
        int32_t floor = S16(o + 132);
        if (S16(o + 22) < floor) {
            *B(o + 7) = 1; *W(o + 20) = (uint32_t)floor << 16; *B(o + 73) = 0;
            call2(c, o, 0x20, f_08861674, 0x0890FA28u);
        } else call1(c, o, f_08861848, 0x0890FA38u);
    } else call1(c, o, f_08861848, 0x0890FA48u);
    leave(c, sv);
}

/* (fighter): 0x08912FEC; falling, a frame flag steps +7 on with x stopped and move 0x14 + strength;
   else ticked. */
SFA_FUNC(089116A0, move_fall_flag_14) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08912FEC, 0x089116B4u);
    if ((*W(o + 100) & 0x80000000u) && S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 96) = 0; *W(o + 104) = 0;
        call2(c, o, (uint32_t)(k + 0x14) & 0xFFFF, f_088616EC, 0x08911708u);
    } else if (*W(o + 100) & 0x80000000u) call1(c, o, f_08861848, 0x089116DCu);
    else call1(c, o, f_08861848, 0x08911718u);
    leave(c, sv);
}

/* (fighter): ended: +7 steps on, the mark (+0x334) set 0x160 ahead of x and a dash (8.0 / -0.75)
   toward the facing; ticked. */
SFA_FUNC(08916878, move_dash_mark) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) & 0x80) {
        uint32_t st = *B(o + 7);
        int32_t x = S16(o + 18);
        *B(o + 7) = (uint8_t)(st + 1);
        gaddr m = o + 0x334;
        *H(m) = (uint16_t)x;
        if (*B(o + 11) == 0) {
            *H(m) = (uint16_t)(S16(m) - 0x160);
            *W(o + 96) = 0xFFF80000u; *W(o + 104) = 0xC000;
        } else {
            *H(m) = (uint16_t)(S16(m) + 0x160);
            *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF4000u;
        }
    }
    call1(c, o, f_08861848, 0x089168F4u);
    frame_out(c, ra);
}

/* (fighter): ended: standing. Else the +82 timer (when set) runs down: while it lasts only ticked; out
   (+262 cleared) or not set, 0x0886DB2C and ticked. */
SFA_FUNC(0892B3B0, move_recover_timer_d) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x0892B3D0u);
    else {
        int32_t t = S16(o + 82);
        int wait = 0;
        if (t != 0) {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) != 0) wait = 1; else *B(o + 262) = 0;
        }
        if (wait) call1(c, o, f_08861848, 0x0892B408u);
        else { call1(c, o, f_0886DB2C, 0x0892B420u); call1(c, o, f_08861848, 0x0892B428u); }
    }
    leave(c, sv);
}

/* (fighter): the +82 wait runs down while it moves by +96; out, +7 steps on, it faces the opponent
   (+697), +653 = -1 when its guard (+78) bit is in +238, and move 8. */
SFA_FUNC(0893DEB0, move_dash_then_8) {
    gaddr o = ARG(0); uint32_t ra;
    int8_t t = (int8_t)(S8(o + 0x52) - 1);
    *B(o + 0x52) = (uint8_t)t;
    frame_in(c, &ra);
    if (t != 0) *W(o + 16) = *W(o + 16) + *W(o + 96);
    else {
        uint32_t st = *B(o + 7);
        int32_t face = S8(o + 697), g = S8(o + 78);
        *B(o + 7) = (uint8_t)(st + 1);
        int32_t bits = S8(o + 238);
        *B(o + 11) = (uint8_t)face;
        if (bits & (1 << (g & 31))) *B(o + 653) = 0xFF;
        call2(c, o, 8, f_088616EC, 0x0893DF14u);
    }
    frame_out(c, ra);
}

/* (fighter): ticked; a frame flag steps +7 on and spawns the held opponent's puppet (0x08822D2C 0x19,
   set up by 0x089426E4): +255 = 5, +347 / +112 cleared, kept at +228 and counted (+536). */
SFA_FUNC(0893E414, move_flag_puppet) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x0893E42Cu);
    if (S8(o + 77) != 0) {
        bump(o + 7);
        ARG(0) = o; ARG(1) = 0x19; ARG(2) = 0; c->r[31] = 0x0893E454u; f_08822D2C(c);
        gaddr e = c->r[2];
        c->r[17] = e;
        if (e != 0) {
            ARG(0) = e; ARG(1) = o; c->r[31] = 0x0893E468u; f_089426E4(c);
            *B(e + 255) = 5; *B(e + 347) = 0; *H(e + 112) = 0;
            uint32_t n = *B(o + 536);
            *W(o + 228) = e;
            *B(o + 536) = (uint8_t)(n + 1);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): the +83 wait runs down; past 0 +237 cleared, +7 steps on and +573 from 0x08A74528 by
   strength (with +81: by 0x0886E294); ticked. */
SFA_FUNC(0893F02C, move_wait_573) {
    gaddr o = ARG(0); uint32_t sv[2];
    gaddr w = o + 0x53;
    int8_t t = (int8_t)(S8(w) - 1);
    enter(c, o, sv);
    *B(w) = (uint8_t)t;
    if (t < 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x0893F078u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08A74528u + i);
    }
    call1(c, o, f_08861848, 0x0893F0A4u);
    leave(c, sv);
}
