/* Move steps of 47 - 49 instructions: jumps with their speeds by strength, a guard dash stop, a counted
   flight with a cue, a hop that leaps at the opponent (0x08984AB0), landings into a move, and the
   jump-start pick by guard direction and state (0x0885AA00, 458 calls). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_089376A8(Cpu *c);
void f_0887C408(Cpu *c); void f_08936DD8(Cpu *c); void f_0886E3CC(Cpu *c); void f_08862004(Cpu *c);
void f_0887C614(Cpu *c);

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

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1, +82 from 0x08A9876C and the four speeds
   by strength (0x08A98770: x, y, x brake, fall; x ones toward the facing). */
SFA_FUNC(08995264, move_tick_jump_timer) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08995278u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        int32_t s = S8(o + 194);
        *B(o + 73) = 1;
        *H(o + 82) = (uint16_t)S8(0x08A9876Cu + (uint32_t)s);
        gaddr e = 0x08A98770u + ((uint32_t)s << 4);
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(e); *W(o + 104) = 0u - *W(e + 8); }
        else { *W(o + 96) = *W(e); *W(o + 104) = *W(e + 8); }
        *W(o + 100) = *W(e + 4); *W(o + 108) = *W(e + 12);
    }
    leave(c, sv);
}

/* (fighter): moves by +96 by its guard (+78: forward, else back; +96 by +104), faces the opponent
   (+697); when its guard bit is in +238 (+653 = -1) or the speed is down to 2.25 (3.0 with +193), step
   3, +82 = 10, +262 cleared and move 0; else ticked. */
SFA_FUNC(088F965C, move_guard_dash_stop_b) {
    gaddr o = ARG(0); uint32_t ra;
    int32_t g = S8(o + 78);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    frame_in(c, &ra);
    *W(o + 16) = g != 0 ? x + v : x - v;
    int32_t face = S8(o + 697);
    *W(o + 96) = v + *W(o + 104);
    int32_t bits = S8(o + 238);
    *B(o + 11) = (uint8_t)face;
    int stop = 1;
    if (bits & (1 << (g & 31))) *B(o + 653) = 0xFF;
    else {
        int32_t lim = S8(o + 193) != 0 ? 0x30000 : 0x24000;
        if (lim < (int32_t)*W(o + 96)) stop = 0;
    }
    if (stop) {
        *B(o + 7) = 3; *H(o + 82) = 0xA; *B(o + 262) = 0;
        call2(c, o, 0, f_0886163C, 0x088F9710u);
    } else call1(c, o, f_08861848, 0x088F96E8u);
    frame_out(c, ra);
}

/* (fighter): 0x089376A8; on frame flag 1 (cue 0x207 when +82 is 2) the +83 count runs down: past 0 +7
   steps on into move 0x31; else the flag is cleared, 0x08936DD8 and ticked; other frames ticked. */
SFA_FUNC(08936D18, move_count_flight_31) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_089376A8, 0x08936D30u);
    gaddr w = o + 0x52;
    c->r[17] = w;
    int stepped = 0;
    if (S8(o + 77) == 1) {
        if (S8(w) == 2) {
            ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x207; c->r[31] = 0x08936D68u; f_0887C408(c);
        }
        int8_t t = (int8_t)(S8(w + 1) - 1);
        *B(w + 1) = (uint8_t)t;
        if (t < 0) { stepped = 1; bump(o + 7); call2(c, o, 0x31, f_088616EC, 0x08936DC4u); }
        else { *B(o + 77) = 0; call1(c, o, f_08936DD8, 0x08936D94u); }
    }
    if (!stepped) call1(c, o, f_08861848, 0x08936D9Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 0x0886E3CC (8, 8); a frame flag steps +7 on, clears +824 - +832 and the y speeds, sets the
   waits (+82 = 4 (locked: from 0x08A8B940 by +193), +83 = 2) and x by strength (0x08A8B928); else
   ticked. */
SFA_FUNC(08968D6C, move_flag_wait_dash) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    ARG(0) = o; ARG(1) = 8; ARG(2) = 8; c->r[31] = 0x08968D88u; f_0886E3CC(c);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08968D9Cu);
    else {
        uint32_t st = *B(o + 7);
        *W(o + 824) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 828) = 0; *W(o + 832) = 0; *W(o + 100) = 0; *W(o + 108) = 0;
        gaddr w = o + 0x52;
        *B(w) = 4; *B(w + 1) = 2;
        gaddr e = 0x08A8B928u + ((uint32_t)S8(o + 194) << 3);
        *W(o + 96) = *W(e);
        int32_t lock = S8(o + 373);
        *W(o + 104) = *W(e + 4);
        if (lock != 0) *B(w) = *B(0x08A8B940u + (uint32_t)(int32_t)S8(o + 193));
    }
    leave(c, sv);
}

/* (fighter): ticked on a frame flag; else in the air (+73 = 1) until 0x08862004 lands it: +73 cleared,
   +7 on, cue 0x22 / 0x23, x / y by strength (0x08A90070, x toward the facing) and move 0x10 +
   strength. */
SFA_FUNC(08973E74, move_air_land_10) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08861848, 0x08973E94u);
    else {
        *B(o + 73) = 1;
        call1(c, o, f_08862004, 0x08973EACu);
        if (c->r[2] == 0) call1(c, o, f_08861848, 0x08973EBCu);
        else {
            uint32_t st = *B(o + 7);
            *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
            call1(c, o, f_0887C614, 0x08973ED8u);
            int32_t s = S8(o + 194);
            gaddr e = 0x08A90070u + ((uint32_t)s << 3);
            *W(o + 96) = *W(e);
            uint32_t f = *B(o + 11);
            *W(o + 100) = *W(e + 4);
            if (f == 0) *W(o + 96) = 0u - *W(o + 96);
            call2(c, o, (uint32_t)(s + 0x10) & 0xFFFF, f_088616EC, 0x08973F24u);
        }
    }
    leave(c, sv);
}

/* (fighter): a leap at the opponent: +7 on, +262 = 1, speeds 6/256 of the distance (y plus 8.75, fall
   -0.4375), 19 up, facing and 57 toward the opponent, and move 0x12. */
SFA_FUNC(08984AB0, move_leap_at_opponent) {
    gaddr o = ARG(0); uint32_t ra;
    bump(o + 7);
    gaddr opp = *W(o + 92);
    *B(o + 262) = 1;
    uint32_t dx = *W(opp + 16) - *W(o + 16);
    int32_t vx = (int32_t)(dx * 6u) >> 8;
    *W(o + 96) = (uint32_t)vx;
    uint32_t dy = *W(opp + 20) - *W(o + 20);
    uint32_t vy = (uint32_t)((int32_t)(dy * 6u) >> 8);
    *W(o + 100) = vy;
    *W(o + 100) = vy + 0x8C000u;
    int32_t y = S16(o + 22);
    *W(o + 104) = 0; *W(o + 108) = 0xFFFF9000u;
    *B(o + 11) = 0;
    *H(o + 22) = (uint16_t)(y + 0x13);
    frame_in(c, &ra);
    int32_t d = -0x39;
    if (vx >= 0) { *B(o + 11) = 1; d = 0x39; }
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
    call2(c, o, 0x12, f_088616EC, 0x08984B64u);
    frame_out(c, ra);
}

/* (fighter): a negative +123 starts a hop (below the floor put on it; step 3, x -0.5, y 8.0 / -0.5) into
   move 9 + strength; else when 0x08862004 lands it step 4, at the floor, +73 / +262 cleared and move
   0x2C; otherwise ticked. */
SFA_FUNC(08996AE0, move_hop_or_land_2c) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    enter(c, o, sv);
    if (f < 0) {
        int32_t floor = S16(o + 132);
        if (floor < S16(o + 22)) *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 7) = 3; *W(o + 96) = 0xFFFF8000u;
        int32_t s = S8(o + 194);
        *W(o + 104) = 0; *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFF8000u;
        call2(c, o, (uint32_t)(s + 9) & 0xFFFF, f_088616EC, 0x08996B44u);
    } else {
        call1(c, o, f_08862004, 0x08996B54u);
        if (c->r[2] == 0) call1(c, o, f_08861848, 0x08996B64u);
        else {
            *B(o + 7) = 4; *W(o + 20) = (uint32_t)S16(o + 132) << 16; *B(o + 73) = 0; *B(o + 262) = 0;
            call2(c, o, 0x2C, f_088616EC, 0x08996B90u);
        }
    }
    leave(c, sv);
}

/* (fighter): ended: +7 on, +696 / +543 cleared, +73 = 1 and the jump by guard direction (+78): forward
   0x13 (no choice made: 0x46 in state 0xE, 0x4E in 0x19), back 0x14 (0x47 in state 0xE with no choice
   made), neutral 0x12; else ticked. */
SFA_FUNC(0885AA00, fighter_jump_start_pick) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 696) = 0; *B(o + 73) = 1;
        int32_t g = S8(o + 78);
        *B(o + 543) = 0;
        uint32_t n;
        if (g == 1) {
            n = 0x13;
            if (S8(o + 386) == -1) {
                int32_t s = S8(o + 338);
                if (s == 0xE) n = 0x46;
                else if (s == 0x19) n = 0x4E;
            }
        } else if (g == -1) {
            n = 0x14;
            if (S8(o + 338) == 0xE && S8(o + 386) == -1) n = 0x47;
        } else n = 0x12;
        call2(c, o, n, f_0886163C, 0x0885AAA4u);
    } else call1(c, o, f_08861848, 0x0885AAB8u);
    frame_out(c, ra);
}
