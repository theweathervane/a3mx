/* Move steps of 28 - 29 instructions: jumps and dashes with their speeds by strength, landings, the
   move start that resets the hit state (0x088646CC), and the part-animation characters' counters.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EC0(Cpu *c);
void f_08993064(Cpu *c); void f_08999370(Cpu *c); void f_0886E294(Cpu *c); void f_088D31AC(Cpu *c);
void f_089CA064(Cpu *c); void f_089CB650(Cpu *c); void f_0886731C(Cpu *c); void f_0886D29C(Cpu *c);
void f_088FF65C(Cpu *c); void f_0887C614(Cpu *c); void f_0890CC04(Cpu *c); void f_0890CC44(Cpu *c);
void f_0890A2F8(Cpu *c); void f_0887E420(Cpu *c);

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
static gaddr by_strength(gaddr tab, gaddr o, uint32_t size) { return tab + (uint32_t)S8(o + 194) * size; }

/* (fighter): without frame byte +76 +7 steps on, +73 = 1 and a jump by strength (0x08A8DEB8: x, y; y
   fall -0.65625); ticked. */
SFA_FUNC(0896DA24, move_tick_76_jump) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 76) == 0) {
        gaddr e = by_strength(0x08A8DEB8u, o, 8);
        bump(o + 7); *B(o + 73) = 1;
        *W(o + 104) = 0; *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4); *W(o + 108) = 0xFFFF5800u;
    }
    call1(c, o, f_08861848, 0x0896DA88u);
    frame_out(c, ra);
}

/* (fighter): `check`; above the floor ticked; else +7 steps on at the floor, +73 / +235 / +262 cleared and
   move 0xC. */
static void land_c(Cpu *c, void (*check)(Cpu *), uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, check, b0);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, b1);
    else {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0; *B(o + 235) = 0; *B(o + 262) = 0;
        call2(c, o, 0xC, f_088616EC, b2);
    }
    leave(c, sv);
}
SFA_FUNC(0898F8A8, move_land_c) { land_c(c, f_08993064, 0x0898F8BCu, 0x0898F8D8u, 0x0898F908u); }
SFA_FUNC(08995000, move_land_c_b) { land_c(c, f_08999370, 0x08995014u, 0x08995030u, 0x08995060u); }

/* (fighter): ticked; frame byte +76 steps it that far forward; without a frame flag +7 steps on, +73 = 1. */
SFA_FUNC(08990968, move_tick_step_76) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0899097Cu);
    int32_t d = S8(o + 76);
    if (d != 0) {
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + d : x - d);
    }
    if (S8(o + 77) == 0) { bump(o + 7); *B(o + 73) = 1; }
    leave(c, sv);
}

/* (fighter): ticked; unless the frame flag is negative +237 cleared, +7 steps on and +573 from 0x08A967F8
   by its +194 (with +81: by 0x0886E294). */
SFA_FUNC(0899146C, move_tick_573_j) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08991480u);
    if ((S8(o + 77) & 0x80) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x089914A8u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = *B(0x08A967F8u + i);
    }
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, +73 = 1, x 1.5 and a jump by strength (0x08A9AE0C); ticked. */
SFA_FUNC(0899BB50, move_tick_77_jump4) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        bump(o + 7); *B(o + 73) = 1;
        *W(o + 96) = 0x18000;
        gaddr e = by_strength(0x08A9AE0Cu, o, 8);
        *W(o + 104) = 0;
        *W(o + 100) = *W(e); *W(o + 108) = *W(e + 4);
    }
    call1(c, o, f_08861848, 0x0899BBB4u);
    frame_out(c, ra);
}

/* (fighter): ticked; unless frame byte +76 is negative +237 cleared, +7 steps on, 0x0886E294 and +573
   from 0x08A9AE58 by its +194 (0 with +81). */
SFA_FUNC(0899BE64, move_tick_573_k) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0899BE78u);
    if (S8(o + 76) >= 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x0899BE9Cu);
        *B(o + 573) = S8(o + 81) != 0 ? 0 : *B(0x08A9AE58u + (uint32_t)S8(o + 194));
    }
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on (to step 6 unless strength 2), a jump (x 3.0, y 5.0 / -0.5),
   +73 = 1; ticked. */
SFA_FUNC(0899BED4, move_tick_77_jump5) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        if (k != 2) *B(o + 7) = 6;
        *W(o + 96) = 0x30000; *W(o + 104) = 0; *W(o + 100) = 0x50000; *W(o + 108) = 0xFFFF8000u;
        *B(o + 73) = 1;
    }
    call1(c, o, f_08861848, 0x0899BF38u);
    frame_out(c, ra);
}

/* (fighter): step 0 picks move 0x22 (step 1); later, back to standing when the animation ends, else ticked. */
static void start_22_or_end(Cpu *c, uint32_t b_start, uint32_t b_stand, uint32_t b_tick) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t st = *B(o + 7);
    frame_in(c, &ra);
    if (st == 0) { *B(o + 7) = 1; call2(c, o, 0x22, f_0886163C, b_start); }
    else if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        call2(c, o, 0, f_0886163C, b_stand);
    } else call1(c, o, f_08861848, b_tick);
    frame_out(c, ra);
}
SFA_FUNC(0899DE6C, move_start_22_or_end) { start_22_or_end(c, 0x0899DE8Cu, 0x0899DEBCu, 0x0899DED0u); }
SFA_FUNC(089A2A80, move_start_22_or_end_b) { start_22_or_end(c, 0x089A2AA0u, 0x089A2AD0u, 0x089A2AE4u); }

/* (fighter): when the animation ends +7 steps on, +73 = 1, +82 from 0x08A9D714 and move 0x23 by strength;
   else ticked. */
SFA_FUNC(089A4928, move_end_then_23_wait) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        int32_t k = S8(o + 194);
        bump(o + 7); *B(o + 73) = 1;
        *H(o + 82) = (uint16_t)S8(0x08A9D714u + (uint32_t)k);
        call2(c, o, (uint32_t)(k + 0x23) & 0xFFFF, f_088616EC, 0x089A498Cu);
    } else call1(c, o, f_08861848, 0x089A4944u);
    frame_out(c, ra);
}

/* (fighter): +543 = 2; ticked; unless the frame flag is negative +237 cleared, +7 steps on and +573 from
   0x08AA3D28 by its +194 (entry 3 with +81). */
SFA_FUNC(089B2300, move_tick_573_l) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 543) = 2;
    call1(c, o, f_08861848, 0x089B2320u);
    if (S8(o + 77) >= 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : 3;
        *B(o + 573) = (uint8_t)S8(0x08AA3D28u + i);
    }
    leave(c, sv);
}

/* (character): the part ticked; without a frame flag +7 steps on, a jump (4.0 each way, y fall -0.375)
   toward the facing, +73 = 1. */
SFA_FUNC(089BB8EC, part_tick_jump) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BB900u);
    if (S8(o + 77) == 0) {
        bump(o + 7);
        *W(o + 96) = 0x40000; *W(o + 104) = 0; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA000u;
        uint32_t f = *B(o + 11);
        *B(o + 73) = 1;
        if (f != 0) *W(o + 96) = 0u - *W(o + 96);
    }
    leave(c, sv);
}

/* (character): the part ticked, 0x088D31AC, 0x089CA064; each frame flag counts +828 down, at 0 +7 steps on
   with move 0x27 + its +194. */
SFA_FUNC(089C89A4, part_tick_count_27) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C89B8u);
    call1(c, o, f_088D31AC, 0x089C89C0u);
    call1(c, o, f_089CA064, 0x089C89C8u);
    if (S8(o + 77) != 0) {
        uint32_t v = *W(o + 828);
        *B(o + 77) = 0;
        *W(o + 828) = v - 1;
        if (v - 1 == 0) {
            int32_t k = S8(o + 194);
            bump(o + 7);
            call2(c, o, (uint32_t)(k + 0x27), f_088616EC, 0x089C8A04u);
        }
    }
    leave(c, sv);
}

/* (character): the part ticked; a frame flag steps +7 on, clears +82, counts 0x089CB650 (2)'s byte and
   +536, and +828 = 12. */
SFA_FUNC(089C8E34, part_tick_flag_b650) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C8E48u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        *H(o + 82) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 77) = 0;
        call2(c, o, 2, f_089CB650, 0x089C8E74u);
        gaddr r = c->r[2];
        *B(r) = (uint8_t)(*B(r) + 1);
        uint32_t n = *B(o + 536);
        *W(o + 828) = 0xC;
        *B(o + 536) = (uint8_t)(n + 1);
    }
    leave(c, sv);
}

/* (fighter, move): the move picked (list 0), standing again (step 1 / 0 / 0 / 0, +696 cleared),
   0x0886731C, 0x0886D29C, and the hit state cleared (+538, +125, +126, the combo damage +778, +244). */
SFA_FUNC(088646CC, move_start_reset) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call2(c, o, ARG(1) & 0xFFFF, f_0886163C, 0x088646E4u);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 696) = 0;
    call1(c, o, f_0886731C, 0x08864704u);
    call1(c, o, f_0886D29C, 0x0886470Cu);
    *B(o + 538) = 0; *B(o + 125) = 0; *H(o + 778) = 0; *B(o + 126) = 0; *W(o + 244) = 0;
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +237 cleared, +7 steps on, 0x0886E294 and +573 from 0x08A4BA8C
   by its +194 (0 with +81). */
SFA_FUNC(088FE090, move_tick_573_m) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088FE0A4u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 77) = 0;
        call1(c, o, f_0886E294, 0x088FE0CCu);
        *B(o + 573) = S8(o + 81) != 0 ? 0 : *B(0x08A4BA8Cu + (uint32_t)S8(o + 194));
    }
    leave(c, sv);
}

/* (fighter): 0x088FF65C; when it lands +7 steps on, at the floor, +262 / +73 / +20 cleared, cue 0x22 /
   0x23 and move 0x31; else ticked. */
SFA_FUNC(088FF51C, move_land_31) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088FF65C, 0x088FF530u);
    if ((int32_t)c->r[2] >= 0) {
        int32_t y = S16(o + 132);
        bump(o + 7);
        *H(o + 22) = (uint16_t)y; *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x088FF574u);
        call2(c, o, 0x31, f_088616EC, 0x088FF580u);
    } else call1(c, o, f_08861848, 0x088FF540u);
    leave(c, sv);
}

/* (fighter): 0x0890CC04, 0x0890CC44; when the mark's timer (+0x337) runs out, and again once at or below
   the floor, 0x0890A2F8. */
SFA_FUNC(0890A284, move_air_timer_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0890CC04, 0x0890A298u);
    call1(c, o, f_0890CC44, 0x0890A2A0u);
    gaddr m = o + 0x334;
    int32_t t = S8(m + 3);
    if (t != 0) {
        *B(m + 3) = (uint8_t)(t - 1);
        if ((int8_t)(t - 1) == 0) call1(c, o, f_0890A2F8, 0x0890A2CCu);
    }
    if (!(S16(o + 132) < S16(o + 22))) call1(c, o, f_0890A2F8, 0x0890A2E8u);
    leave(c, sv);
}

/* (fighter): +123 cleared, +7 steps on, 0x0887E420 (-27, 0), x speeds by strength (0x08A5842C), move 0x12. */
SFA_FUNC(0890BA54, move_dash_12) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 123) = 0; *B(o + 7) = (uint8_t)(st + 1);
    ARG(0) = o; ARG(1) = 0xFFFFFFE5u; ARG(2) = 0; c->r[31] = 0x0890BA84u; f_0887E420(c);
    gaddr e = by_strength(0x08A5842Cu, o, 8);
    *W(o + 96) = *W(e);
    *W(o + 104) = *W(e + 4);
    call2(c, o, 0x12, f_088616EC, 0x0890BAB8u);
    leave(c, sv);
}
