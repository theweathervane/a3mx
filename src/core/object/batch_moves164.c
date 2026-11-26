/* Move steps of 26 - 32 instructions: slides and falls that end in the next move, a part character's
   teleport landing and stance, a bounce that lifts the opponent (0x0890AA58), and the timed recovery
   step that clears +262 after its pause. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c); void f_0886DB2C(Cpu *c);
void f_088D3EC0(Cpu *c); void f_089612D0(Cpu *c); void f_08862004(Cpu *c); void f_088D35D0(Cpu *c);
void f_088D2DBC(Cpu *c); void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c); void f_089BD024(Cpu *c);
void f_089BD068(Cpu *c); void f_088D31AC(Cpu *c); void f_08912FEC(Cpu *c); void f_0887C614(Cpu *c);
void f_08917840(Cpu *c); void f_089291C0(Cpu *c);

#define BATTLE 0x092E1228u

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
static uint32_t str_move(gaddr o, int32_t base) { return (uint32_t)(S8(o + 194) + base) & 0xFFFF; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): the +82 wait runs down; past 0 +7 steps on and its side's bit set in the battle's +11; then
   the frame-flag projectile step (0x089612D0). */
SFA_FUNC(08961268, move_wait_side_projectile) {
    gaddr o = ARG(0);
    uint32_t ra;
    int8_t t = (int8_t)(S8(o + 0x52) - 1);
    *B(o + 0x52) = (uint8_t)t;
    frame_in(c, &ra);
    if (t < 0) {
        bump(o + 7);
        *B(BATTLE + 11) = (uint8_t)((uint32_t)S8(BATTLE + 11) | (1u << (S8(o + 337) & 31)));
    }
    call1(c, o, f_089612D0, 0x089612C4u);
    frame_out(c, ra);
}

/* (character): the part ticked, 0x08862004; when 0x088D35D0 gives 1, +7 steps on, +73 cleared, move
   0x23, cue 0x22, 0x088D36D0 and 0x088D3960 (0, 0). */
SFA_FUNC(089BC684, part_air_teleport_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BC698u);
    call1(c, o, f_08862004, 0x089BC6A0u);
    call1(c, o, f_088D35D0, 0x089BC6A8u);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, 0x23, f_088616EC, 0x089BC6D0u);
        call1(c, o, f_088D2DBC, 0x089BC6D8u);
        call1(c, o, f_088D36D0, 0x089BC6E0u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089BC6F0u; f_088D3960(c);
    }
    leave(c, sv);
}

/* (character): once ended back to its stance (+832 / +73 / +78 = 1, 0x089BD024, 0x089BD068, +492 / +493 /
   +262 cleared); else a frame flag runs 0x088D31AC and the part ticks. */
SFA_FUNC(089BD154, part_end_stance) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 75) < 0) {
        c->r[17] = 1;
        *W(o + 832) = 1; *B(o + 73) = 1;
        call1(c, o, f_089BD024, 0x089BD184u);
        *B(o + 78) = 1;
        call1(c, o, f_089BD068, 0x089BD190u);
        *B(o + 493) = 0; *B(o + 492) = 0; *B(o + 262) = 0;
    } else {
        if (S8(o + 77) != 0) call1(c, o, f_088D31AC, 0x089BD1B4u);
        call1(c, o, f_088D3EC0, 0x089BD1BCu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (character): ended: standing. Else the +82 timer (when set) runs down; 0x0886DB2C when it is out (then
   +262 cleared) or was not set; the part ticks. */
SFA_FUNC(089C7A34, part_recover_timer_c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089C7A54u);
    else {
        int32_t t = S16(o + 82);
        if (t == 0) call1(c, o, f_0886DB2C, 0x089C7A70u);
        else {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) { call1(c, o, f_0886DB2C, 0x089C7A94u); *B(o + 262) = 0; }
        }
        call1(c, o, f_088D3EC0, 0x089C7AA0u);
    }
    leave(c, sv);
}

/* (fighter): sinks (+20 down 1/8) and ticks; once ended the +820 count runs down and at 0 +7 steps on with
   move 0x20 + strength. */
SFA_FUNC(088F043C, move_sink_count_20) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        uint32_t v = *W(o + 820) - 1;
        *W(o + 820) = v;
        if (v == 0) {
            bump(o + 7);
            call2(c, o, str_move(o, 0x20), f_088616EC, 0x088F047Cu);
            frame_out(c, ra);
            return;
        }
        *W(o + 20) = *W(o + 20) - 0x2000;
        call1(c, o, f_08861848, 0x088F0498u);
    } else {
        *W(o + 20) = *W(o + 20) - 0x2000;
        call1(c, o, f_08861848, 0x088F04B0u);
    }
    frame_out(c, ra);
}

/* (fighter): flies (+16 by +96, +20 by +100 by +108); once falling step 2, the mark's +0 = strength + 1
   and move 0xA + strength; else ticked. */
SFA_FUNC(088F3AA0, move_fly_then_a) {
    gaddr o = ARG(0);
    uint32_t ra;
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    uint32_t v = vy + *W(o + 108);
    frame_in(c, &ra);
    *W(o + 100) = v;
    if ((int32_t)v < 0) {
        *B(o + 7) = 2;
        *B(o + 0x334) = (uint8_t)(S8(o + 194) + 1);
        call2(c, o, str_move(o, 0xA), f_088616EC, 0x088F3B00u);
    } else call1(c, o, f_08861848, 0x088F3B14u);
    frame_out(c, ra);
}

/* (fighter): the +82 wait runs down; past 0 step 3, +237 cleared, +573 from 0x08A4976C by strength (entry 3
   with +81) and move 0x46 + strength; else ticked. */
SFA_FUNC(088F8F68, move_wait_then_46) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    frame_in(c, &ra);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) {
        *B(o + 7) = 3; *B(o + 237) = 0;
        int32_t k = S8(o + 194);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)k : 3;
        int8_t v = S8(0x08A4976Cu + i);
        *B(o + 573) = (uint8_t)v;
        call2(c, o, (uint32_t)(k + 0x46) & 0xFFFF, f_088616EC, 0x088F8FC8u);
    } else call1(c, o, f_08861848, 0x088F8FDCu);
    frame_out(c, ra);
}

/* (fighter): the first time it starts falling (+82 clear) +82 = 1, +127 = 10, y speed cleared with a
   fall -0.375, and an opponent in state 1 / 1 (+4 / +5) is lifted 3.0; ticked unless it is still rising
   on that first check. */
SFA_FUNC(0890AA58, move_bounce_lift) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    int tick = 1;
    if (S16(o + 82) == 0) {
        if ((int32_t)*W(o + 100) >= 0) tick = 0;
        else {
            *H(o + 82) = 1; *B(o + 127) = 0xA; *W(o + 100) = 0;
            gaddr opp = *W(o + 92);
            *W(o + 108) = 0xFFFFA000u;
            if ((((uint32_t)*B(opp + 4) << 8) | *B(opp + 5)) == 0x101) *W(opp + 100) = *W(opp + 100) + 0x30000;
        }
    }
    if (tick) call1(c, o, f_08861848, 0x0890AACCu);
    frame_out(c, ra);
}

/* (fighter): ticked; unless the frame flag is negative +7 steps on, +73 = 1 and the four speeds by
   strength (0x08A5AE28: x, x brake, y, y fall). */
SFA_FUNC(08911620, move_tick_flag_speeds_c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08911634u);
    if ((S8(o + 77) & 0x80) == 0) {
        uint32_t st = *B(o + 7);
        gaddr e = 0x08A5AE28u + ((uint32_t)S8(o + 194) << 4);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1;
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    leave(c, sv);
}

/* (fighter): 0x08912FEC; below the floor it lands (+7 on, at the floor, +73 / +262 cleared, cue 0x22 /
   0x23, move 0x17 + strength); else ticked. */
SFA_FUNC(08911728, move_land_17_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08912FEC, 0x0891173Cu);
    int32_t floor = S16(o + 132);
    if (S16(o + 22) < floor) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0; *B(o + 262) = 0;
        call1(c, o, f_0887C614, 0x08911774u);
        call2(c, o, str_move(o, 0x17), f_088616EC, 0x08911788u);
    } else call1(c, o, f_08861848, 0x08911798u);
    leave(c, sv);
}

/* (fighter): the mark's +8 counts down (+262 cleared as it passes 0); it slides by +96 toward its facing,
   +96 by +104, stepping +7 on once that turns; then 0x08917840. */
SFA_FUNC(089156A8, move_slide_mark8) {
    gaddr o = ARG(0);
    uint32_t ra;
    gaddr m = o + 0x334 + 8;
    int32_t t = S8(m);
    *B(m) = (uint8_t)(t - 1);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    frame_in(c, &ra);
    if (t == 0) *B(o + 262) = 0;
    *W(o + 16) = x + toward(o, v);
    uint32_t v2 = v + *W(o + 104);
    *W(o + 96) = v2;
    if ((int32_t)v2 < 0) bump(o + 7);
    call1(c, o, f_08917840, 0x0891571Cu);
    frame_out(c, ra);
}

/* (fighter): once +96 turns +7 steps on with the x speeds cleared; else 0x089291C0 and it slides by +96
   toward its facing (+96 by +104); ticked. */
SFA_FUNC(089267A8, move_slide_until_stop) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t v0 = (int32_t)*W(o + 96);
    enter(c, o, sv);
    if (v0 < 0) {
        uint32_t st = *B(o + 7);
        *W(o + 104) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 96) = 0;
    } else {
        call1(c, o, f_089291C0, 0x089267E0u);
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + toward(o, v);
        *W(o + 96) = v + *W(o + 104);
    }
    call1(c, o, f_08861848, 0x08926818u);
    leave(c, sv);
}
