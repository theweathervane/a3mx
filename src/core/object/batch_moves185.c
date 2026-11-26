/* Move steps of 18 - 47 instructions: a creep into the guard dash, a bounce that lands into a second
   hop (0x089073D4), creeps that step forward on landing, a held-button release (0x0897D088), a rise with
   a one-time move (0x0890A4A4), and the knocked-out start (0x08863208) that clears the hit records
   and, in a timed match, marks the fighter. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861724(Cpu *c); void f_088616EC(Cpu *c); void f_08964DFC(Cpu *c);
void f_08908BD8(Cpu *c); void f_08908B7C(Cpu *c); void f_0887C614(Cpu *c); void f_08961448(Cpu *c);
void f_0890CC44(Cpu *c); void f_0890CC04(Cpu *c); void f_0887C408(Cpu *c); void f_0899FC90(Cpu *c);
void f_089A25A0(Cpu *c); void f_089A26D8(Cpu *c); void f_0886D29C(Cpu *c); void f_08867060(Cpu *c);

#define MATCH 0x092E1228u

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

/* (fighter): ended: step 1 and the guard dash (0x08964DFC); else x by the frame flag and ticked. */
SFA_FUNC(08964DB4, move_creep_then_guard_dash) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) { *B(o + 7) = 1; call1(c, o, f_08964DFC, 0x08964DD0u); }
    else {
        *H(o + 18) = (uint16_t)(S16(o + 18) + S8(o + 77));
        call1(c, o, f_08861848, 0x08964DF0u);
    }
    frame_out(c, ra);
}

/* (fighter): 0x08908BD8 (negative: the fall grows by +0x338); above the floor 0x08908B7C; at or below
   it +7 steps on, at the floor, +73 / +20 cleared, a second hop (x 7.0, y 3.0 / -0.375, +0x338 =
   -0.1875) and cue 0x22 / 0x23. */
SFA_FUNC(089073D4, move_bounce_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08908BD8, 0x089073E8u);
    gaddr p = o + 0x334;
    if ((int32_t)c->r[2] < 0) *W(o + 108) = *W(o + 108) + *W(p + 4);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08908B7C, 0x0890741Cu);
    else {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0; *H(o + 20) = 0;
        *W(o + 96) = 0x70000; *W(o + 104) = 0; *W(o + 100) = 0x30000; *W(o + 108) = 0xFFFFA000u;
        *W(p + 4) = 0xFFFFD000u;
        call1(c, o, f_0887C614, 0x08907478u);
    }
    leave(c, sv);
}

/* (fighter): ticked until a frame flag; when 0x08961448 is negative at or below the floor +7 steps on,
   at the floor, +20 / +73 and speeds cleared, 24 toward its facing, cue 0x22 / 0x23, +74 = 1 and
   ticked. */
SFA_FUNC(0895D9A4, move_creep_land_cue) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) call1(c, o, f_08861848, 0x0895D9C4u);
    call1(c, o, f_08961448, 0x0895D9CCu);
    int32_t floor = S16(o + 132);
    if ((int32_t)c->r[2] < 0 && !(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0;
        *W(o + 96) = 0; *W(o + 100) = 0; *W(o + 104) = 0;
        uint32_t f = *B(o + 11);
        *W(o + 108) = 0;
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(f != 0 ? x + 0x18 : x - 0x18);
        call1(c, o, f_0887C614, 0x0895DA38u);
        *B(o + 74) = 1;
        call1(c, o, f_08861848, 0x0895DA48u);
    }
    leave(c, sv);
}

/* (fighter): ticked; each frame flag (cleared): released (+752 without the strength's bit from
   0x08A92630) or the +82 timer out, +7 steps on into move 0x2D + strength. */
SFA_FUNC(0897D088, move_hold_release_2d) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0897D09Cu);
    if (S8(o + 77) != 0) {
        int32_t s = S8(o + 194);
        *B(o + 77) = 0;
        uint32_t held = *H(o + 752) & *H(0x08A92630u + (uint32_t)(s + s));
        if (held == 0) { bump(o + 7); call2(c, o, (uint32_t)(s + 0x2D) & 0xFFFF, f_088616EC, 0x0897D0F0u); }
        else {
            uint32_t t = (uint32_t)S16(o + 82) - 1;
            *H(o + 82) = (uint16_t)t;
            if ((int16_t)t == 0) { bump(o + 7); call2(c, o, (uint32_t)(s + 0x2D) & 0xFFFF, f_088616EC, 0x0897D12Cu); }
        }
    }
    leave(c, sv);
}

/* (fighter): unless a frame flag is up it flies (+73 = 1; +16 by +96, +20 by +100 by +108); at or below
   the floor +7 steps on, at the floor, +73 / +262 cleared, cue 0x22 / 0x23 and move 0xE; else
   ticked. */
SFA_FUNC(088FDCF8, move_fly_land_e) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08861848, 0x088FDD18u);
    else {
        uint32_t vy = *W(o + 100);
        *B(o + 73) = 1;
        *W(o + 20) = *W(o + 20) + vy;
        *W(o + 16) = *W(o + 16) + *W(o + 96);
        *W(o + 100) = vy + *W(o + 108);
        int32_t floor = S16(o + 132);
        if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x088FDD70u);
        else {
            uint32_t st = *B(o + 7);
            *H(o + 22) = (uint16_t)floor;
            *B(o + 7) = (uint8_t)(st + 1);
            *B(o + 73) = 0; *B(o + 262) = 0;
            call1(c, o, f_0887C614, 0x088FDD94u);
            call2(c, o, 0xE, f_088616EC, 0x088FDDA0u);
        }
    }
    leave(c, sv);
}

/* (fighter): 0x0890CC44; below y speed 2.0 the first time (+0x33C) move 0xF; rising 0x0890CC04 and
   ticked; falling +7 steps on with x 1.5, y -8.0, cue 0x207 from its side and move 7. */
SFA_FUNC(0890A4A4, move_rise_once_f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0890CC44, 0x0890A4B8u);
    int32_t vy = (int32_t)*W(o + 100);
    gaddr p = o + 0x334;
    if (vy < 0x20000 && S8(p + 8) == 0) { *B(p + 8) = 1; call2(c, o, 0xF, f_088616EC, 0x0890A4ECu); }
    else if (vy >= 0) { call1(c, o, f_0890CC04, 0x0890A504u); call1(c, o, f_08861848, 0x0890A50Cu); }
    else {
        bump(o + 7);
        *W(o + 96) = 0x18000; *W(o + 100) = 0xFFF80000u;
        ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x207; c->r[31] = 0x0890A540u; f_0887C408(c);
        call2(c, o, 7, f_088616EC, 0x0890A54Cu);
    }
    leave(c, sv);
}

/* (fighter): a frame flag (cleared) with strength runs 0x089A25A0 and 0x089A26D8; 0x0899FC90; falling
   +7 steps on with the four speeds by strength (0x08A9BFB0: x, y, x brake, fall) into move 3 +
   strength; else ticked. */
SFA_FUNC(0899FA90, move_flag_fall_3) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        int32_t s = S8(o + 194);
        *B(o + 77) = 0;
        if (s != 0) { call1(c, o, f_089A25A0, 0x0899FABCu); call1(c, o, f_089A26D8, 0x0899FAC4u); }
    }
    call1(c, o, f_0899FC90, 0x0899FACCu);
    if ((int32_t)*W(o + 100) < 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        gaddr e = 0x08A9BFB0u + ((uint32_t)s << 4);
        *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4); *W(o + 104) = *W(e + 8); *W(o + 108) = *W(e + 12);
        call2(c, o, (uint32_t)(s + 3) & 0xFFFF, f_088616EC, 0x0899FB38u);
    } else call1(c, o, f_08861848, 0x0899FAE0u);
    leave(c, sv);
}

/* (fighter): knocked out: state 1 / 6 / 0 / 0, hit records (+560 - +562) cleared, +599 = 1; 0x0886D29C
   while the opponent's +248 is below 8; in a match not yet over (+7) +126 / +778 cleared and, on the
   clock (+56), +579 / +671 = 1 and +591 cleared; move 0x1B of list 2 and 0x08867060. */
SFA_FUNC(08863208, fighter_ko_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    gaddr opp = *W(o + 92);
    c->r[29] -= 0x20;
    *B(o + 4) = 1; *B(o + 5) = 6; *B(o + 6) = 0; *B(o + 7) = 0;
    *B(o + 561) = 0; *B(o + 560) = 0;
    *W(c->r[29] + 24) = s2;
    c->r[18] = 1;
    *B(o + 562) = 0; *B(o + 599) = 1;
    int32_t n = S8(opp + 248);
    *W(c->r[29] + 20) = s1;
    c->r[17] = MATCH;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    if (n < 8) call1(c, o, f_0886D29C, 0x0886326Cu);
    if (S8(MATCH + 7) == 0) {
        *B(o + 126) = 0; *H(o + 778) = 0;
        if (S16(MATCH + 56) != 0) { *B(o + 579) = 1; *B(o + 671) = 1; *B(o + 591) = 0; }
    }
    call2(c, o, 0x1B, f_08861724, 0x088632A4u);
    call1(c, o, f_08867060, 0x088632ACu);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
