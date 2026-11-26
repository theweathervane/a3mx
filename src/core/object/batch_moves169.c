/* Move steps of 33 instructions: jumps and dashes started by frame flags (one with its x speed scaled by
   strength, 0x0892C3BC), landings with a cue or a voice, and part-character flag steps. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0887C614(Cpu *c); void f_089376E4(Cpu *c);
void f_0887C408(Cpu *c); void f_08993064(Cpu *c); void f_0899CF6C(Cpu *c); void f_08862004(Cpu *c);
void f_088D3EC0(Cpu *c); void f_088D2E04(Cpu *c); void f_089B7598(Cpu *c);

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

/* (fighter): ticked; a negative frame flag starts a jump (+7 on, y 9.0 / -0.375); otherwise it slides by
   +96 toward its facing, stopping once +96 (by +104) turns. */
SFA_FUNC(0892BB58, move_tick_jump_or_slide) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0892BB6Cu);
    if (S8(o + 77) < 0) {
        bump(o + 7);
        *W(o + 100) = 0x90000; *W(o + 108) = 0xFFFFA000u;
    } else {
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + toward(o, v);
        uint32_t v2 = v + *W(o + 104);
        *W(o + 96) = v2;
        if ((int32_t)v2 < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
    }
    leave(c, sv);
}

/* (fighter): ticked; frame flag 2 becomes 1; with none left, step 5, +73 = 1 and a jump: x 3.0 per
   strength level (+194 + 1), y 9.0 / -0.375, x brake -0.5. */
SFA_FUNC(0892C3BC, move_tick_jump_by_strength) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0892C3D0u);
    if (S8(o + 77) == 2) *B(o + 77) = 1;
    if (S8(o + 77) == 0) {
        int64_t m = (int64_t)(S8(o + 194) + 1) * 0x30000;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        *B(o + 7) = 5; *B(o + 73) = 1;
        *W(o + 96) = c->lo; *W(o + 100) = 0x90000; *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
    }
    leave(c, sv);
}

/* (fighter): it falls (+20 by +100 by +108); at or below the floor step 7, +235 / +73 cleared, height to
   the floor, cue 0x22 / 0x23 and ticked; in the air ticked until a frame flag. */
SFA_FUNC(0892C4B4, move_fall_land_7) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132);
    *W(o + 100) = vy + *W(o + 108);
    if (!(floor < S16(o + 22))) {
        *B(o + 7) = 7; *B(o + 235) = 0; *B(o + 73) = 0; *H(o + 22) = (uint16_t)floor;
        call1(c, o, f_0887C614, 0x0892C520u);
        call1(c, o, f_08861848, 0x0892C528u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0892C528u);
    leave(c, sv);
}

/* (fighter): the +82 wait runs down; past 0 +237 cleared, +7 steps on, +573 from 0x08A6DB80 by strength
   (entry 3 with +81) and move 0x1D; else ticked. */
SFA_FUNC(08931328, move_wait_then_1d) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    frame_in(c, &ra);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : 3;
        *B(o + 573) = (uint8_t)S8(0x08A6DB80u + i);
        call2(c, o, 0x1D, f_088616EC, 0x0893138Cu);
    } else call1(c, o, f_08861848, 0x089313A0u);
    frame_out(c, ra);
}

/* (fighter): 0x089376E4; once falling +196 counts, +83 = 6, speeds cleared but a fall of -0.75, cue
   0x225 from its side; else ticked. */
SFA_FUNC(089373A0, move_fall_cue_225) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089376E4, 0x089373B4u);
    if ((int32_t)*W(o + 100) < 0) {
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1);
        *B(o + 0x53) = 6;
        *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 96) = 0;
        int32_t side = S8(o + 337);
        *W(o + 108) = 0xFFFF4000u;
        ARG(0) = o; ARG(1) = (uint32_t)side; ARG(2) = 0x225; c->r[31] = 0x08937404u; f_0887C408(c);
    } else call1(c, o, f_08861848, 0x08937414u);
    leave(c, sv);
}

/* (fighter): 0x08993064; when it says so and it is on the floor +7 steps on, at the floor, x 3.0 / -0.47,
   y cleared, move 0x24; else ticked. */
SFA_FUNC(0898FE18, move_check_land_24) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08993064, 0x0898FE2Cu);
    int32_t floor = S16(o + 132);
    if (c->r[2] == 0 || floor < S16(o + 22)) call1(c, o, f_08861848, 0x0898FE50u);
    else {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *W(o + 96) = 0x30000; *W(o + 104) = 0xFFFF8800u; *W(o + 100) = 0; *W(o + 108) = 0;
        call2(c, o, 0x24, f_088616EC, 0x0898FE8Cu);
    }
    leave(c, sv);
}

/* (fighter): 0x0899CF6C; when negative and on the floor +7 steps on, at the floor, +20 / +73 cleared and
   move 0x1B + strength; else ticked. */
SFA_FUNC(0899BFE8, move_check_land_1b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0899CF6C, 0x0899BFFCu);
    if ((int32_t)c->r[2] >= 0) call1(c, o, f_08861848, 0x0899C00Cu);
    else {
        int32_t floor = S16(o + 132);
        if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x0899C02Cu);
        else {
            uint32_t st = *B(o + 7);
            int32_t k = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0;
            call2(c, o, (uint32_t)(k + 0x1B) & 0xFFFF, f_088616EC, 0x0899C05Cu);
        }
    }
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing +7 steps on, the bounce count (+78) down (out: step 5), +73 / +76 /
   +77 cleared at the floor; ticked without frame byte +76. */
SFA_FUNC(089A4634, move_rise_bounce_count) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A4648u);
    if (c->r[2] != 0) {
        uint32_t st = *B(o + 7);
        int32_t m = S8(o + 78) - 1;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 78) = (uint8_t)m;
        if ((int8_t)m < 0) *B(o + 7) = 5;
        int32_t y = S16(o + 132);
        *B(o + 73) = 0;
        *W(o + 20) = (uint32_t)y << 16;
        *B(o + 76) = 0; *B(o + 77) = 0;
    }
    if (S8(o + 76) == 0) call1(c, o, f_08861848, 0x089A46A8u);
    leave(c, sv);
}

/* (character): the part ticked; frame flag 2 plays bank voice 0x15; frame flag 3 steps +7 on (with +820
   = 1 also voice 0xC and +820 cleared) and clears +262. */
SFA_FUNC(089B60A0, part_flag_voices_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089B60B4u);
    if (S8(o + 77) == 2) { *B(o + 77) = 0; call2(c, o, 0x15, f_088D2E04, 0x089B60D4u); }
    if (S8(o + 77) == 3) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0;
        uint32_t n = *W(o + 820);
        *B(o + 7) = (uint8_t)(st + 1);
        if (n == 1) { call2(c, o, 0xC, f_088D2E04, 0x089B610Cu); *W(o + 820) = 0; }
        *B(o + 262) = 0;
    }
    leave(c, sv);
}

/* (character): the part ticked, 0x089B7598; frame flag n steps it n toward its facing; ended: +7 steps on
   with move 0x26 + strength. */
SFA_FUNC(089B698C, part_step_end_26) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089B69A0u);
    call1(c, o, f_089B7598, 0x089B69A8u);
    int32_t d = S8(o + 77);
    if (d != 0) {
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + d : x - d);
        *B(o + 77) = 0;
    }
    if (S8(o + 75) < 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, (uint32_t)(k + 0x26), f_088616EC, 0x089B6A00u);
    }
    leave(c, sv);
}
