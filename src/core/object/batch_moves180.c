/* Move steps of 39 - 41 instructions: jumps that shift on take-off with their speeds by strength, a slide
   that stops into a move, landings, an afterimage copied from the fighter (0x08969178), and the landing
   after a hit (0x0885DD48, 4,630 calls) that knocks down with a cue. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861724(Cpu *c); void f_088616EC(Cpu *c); void f_08864740(Cpu *c);
void f_0887C614(Cpu *c); void f_08903954(Cpu *c); void f_08904A20(Cpu *c); void f_08822EAC(Cpu *c);
void f_0896F62C(Cpu *c); void f_08987404(Cpu *c); void f_08862004(Cpu *c); void f_08863748(Cpu *c);
void f_0887C56C(Cpu *c); void f_0886E3CC(Cpu *c);

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
static void speeds4(gaddr o, gaddr e) {
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
}

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1, 24 toward its facing and the four speeds
   by strength (0x08A9E8B0: x, x brake, y, fall). */
SFA_FUNC(089A7CD8, move_tick_shift_jump) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089A7CECu);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        uint32_t f = *B(o + 11);
        *B(o + 73) = 1;
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(f != 0 ? x + 0x18 : x - 0x18);
        speeds4(o, 0x08A9E8B0u + ((uint32_t)S8(o + 194) << 4));
    }
    leave(c, sv);
}

/* (fighter): rises (+20 by +100 by +108) and slides by +96 toward its facing (+96 by +104); once that
   turns, x stopped, +7 steps on and move 0x25 + strength; else ticked. */
SFA_FUNC(089A7D74, move_arc_stop_25) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    *W(o + 100) = vy + *W(o + 108);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    *W(o + 16) = *B(o + 11) != 0 ? x + v : x - v;
    uint32_t v2 = v + *W(o + 104);
    *W(o + 96) = v2;
    if ((int32_t)v2 < 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 104) = 0; *W(o + 96) = 0;
        call2(c, o, (uint32_t)(s + 0x25) & 0xFFFF, f_088616EC, 0x089A7DF0u);
    } else call1(c, o, f_08861848, 0x089A7E04u);
    frame_out(c, ra);
}

/* (fighter): when 0x08904A20 is negative the fall is -0.28 and at or below the floor it lands (step 3, at
   the floor, +20 / +262 / +235 / +73 cleared, cue 0x22 / 0x23) into move 0x14 + 2 x strength; else
   0x08903954 and ticked. */
SFA_FUNC(08903864, move_fall_land_14_2k) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08904A20, 0x08903878u);
    int landed = 0;
    if ((int32_t)c->r[2] < 0) {
        int32_t floor = S16(o + 132);
        *W(o + 108) = 0xFFFFB800u;
        if (!(floor < S16(o + 22))) {
            landed = 1;
            *B(o + 7) = 3; *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0;
            *B(o + 262) = 0; *B(o + 235) = 0; *B(o + 73) = 0;
            call1(c, o, f_0887C614, 0x089038BCu);
            call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0x14) & 0xFFFF, f_088616EC, 0x089038D4u);
        }
    }
    if (!landed) {
        call1(c, o, f_08903954, 0x089038ECu);
        call1(c, o, f_08861848, 0x089038F4u);
    }
    leave(c, sv);
}

/* (fighter): a frame flag steps +7 on and spawns afterimage 0x3C / 0 with the fighter's facing, place,
   floor, picture (+24 / +86 / +148 / +164) and tied to it; ticked. */
SFA_FUNC(08969178, move_flag_afterimage) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        bump(o + 7);
        ARG(0) = 0x3C; ARG(1) = 0; c->r[31] = 0x089691A8u; f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 11) = *B(o + 11); *B(e + 12) = *B(o + 12);
            *H(e + 18) = *H(o + 18); *H(e + 22) = *H(o + 22); *H(e + 132) = *H(o + 132);
            *H(e + 24) = *H(o + 24); *H(e + 86) = *H(o + 86);
            *W(e + 164) = *W(o + 164);
            uint32_t p = *W(o + 148);
            *W(e + 88) = o; *W(e + 148) = p;
        }
    }
    call1(c, o, f_08861848, 0x08969208u);
    leave(c, sv);
}

/* (fighter): on a frame flag 0x0896F62C (x stopped when moving forward), and at or below the floor +7
   steps on, at the floor, +20 / +73 cleared and move 0x21 + strength; else ticked. */
SFA_FUNC(0896E4D4, move_flag_land_21) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        call1(c, o, f_0896F62C, 0x0896E4F4u);
        if ((int32_t)*W(o + 96) > 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
        int32_t floor = S16(o + 132);
        if (!((floor & 0xFFFF) < (S16(o + 22) & 0xFFFF))) {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0;
            call2(c, o, (uint32_t)(s + 0x21) & 0xFFFF, f_088616EC, 0x0896E54Cu);
            leave(c, sv);
            return;
        }
    }
    call1(c, o, f_08861848, 0x0896E564u);
    leave(c, sv);
}

/* (fighter): 0x08987404; falling at or below the floor it lands crouching (+73 / +20 cleared); else from
   64 up with +238 set +7 steps on into move 0x14, otherwise ticked. */
SFA_FUNC(08987320, move_fall_crouch_or_14) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08987404, 0x08987334u);
    int32_t y = S16(o + 22);
    if ((int32_t)*W(o + 100) < 0 && !(S16(o + 132) < y)) {
        *H(o + 22) = (uint16_t)S16(o + 132); *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_08864740, 0x0898736Cu);
    } else if (y >= 64 && S8(o + 238) != 0) {
        bump(o + 7);
        call2(c, o, 0x14, f_088616EC, 0x089873B0u);
    } else call1(c, o, f_08861848, 0x08987394u);
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +7 steps on, 32 toward its facing, +73 = 1 and the four speeds
   by 4 x strength (0x08A9E770: x, x brake, y, fall). */
SFA_FUNC(089A67F4, move_tick_shift_jump_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089A6808u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        int32_t x = S16(o + 18);
        *B(o + 7) = (uint8_t)(st + 1);
        int32_t d = *B(o + 11) != 0 ? 0x20 : -0x20;
        int32_t s = S8(o + 194);
        *H(o + 18) = (uint16_t)(x + d);
        int32_t i = (int16_t)(s << 2);
        *B(o + 73) = 1;
        speeds4(o, 0x08A9E770u + ((uint32_t)i << 2));
    }
    leave(c, sv);
}

/* (fighter): after a hit, when 0x08862004 lands it falling: +7 on, 0x08863748, +73 cleared, +283 by 8,
   at the floor, move 0x16 of list 2, the landing cue (0x0887C56C) and (without +702) 0x0886E3CC (9, 8);
   else ticked. */
SFA_FUNC(0885DD48, fighter_hit_land) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x0885DD5Cu);
    if (c->r[2] != 0 && (int32_t)*W(o + 100) < 0) {
        bump(o + 7);
        call1(c, o, f_08863748, 0x0885DD84u);
        int32_t k = S8(o + 283), floor = S16(o + 132);
        *B(o + 73) = 0;
        *B(o + 283) = (uint8_t)(k + 8);
        *W(o + 20) = (uint32_t)floor << 16;
        call2(c, o, 0x16, f_08861724, 0x0885DDACu);
        call1(c, o, f_0887C56C, 0x0885DDB4u);
        if (S8(o + 702) == 0) { ARG(0) = o; ARG(1) = 9; ARG(2) = 8; c->r[31] = 0x0885DDCCu; f_0886E3CC(c); }
    } else call1(c, o, f_08861848, 0x0885DDDCu);
    leave(c, sv);
}
