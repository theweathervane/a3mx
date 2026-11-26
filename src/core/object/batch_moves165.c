/* Move steps of 32 - 33 instructions: landings into the next move, launches that place the fighter by
   its side or facing, the round's crouching start (0x08864740) and part characters' flag steps.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_0887C614(Cpu *c); void f_0894CAF4(Cpu *c); void f_08961448(Cpu *c); void f_0886E294(Cpu *c);
void f_08961428(Cpu *c); void f_089930D8(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D3140(Cpu *c);
void f_088D35C0(Cpu *c); void f_08862004(Cpu *c); void f_088D35D0(Cpu *c); void f_088D2DBC(Cpu *c);
void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c); void f_08864494(Cpu *c); void f_0886731C(Cpu *c);
void f_0886D29C(Cpu *c);

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

/* (fighter): 0x0894CAF4; at or below the floor +7 steps on, at the floor, +73 / +262 cleared, cue 0x22 /
   0x23 and move 0x2B + strength; else ticked. */
SFA_FUNC(0894A288, move_land_2b_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0894CAF4, 0x0894A29Cu);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0; *B(o + 262) = 0;
        call1(c, o, f_0887C614, 0x0894A2D4u);
        call2(c, o, str_move(o, 0x2B), f_088616EC, 0x0894A2E8u);
    } else call1(c, o, f_08861848, 0x0894A2F8u);
    leave(c, sv);
}

/* (fighter): 0x08961448; in the air ticked; on the floor +7 steps on, +20 / +73 / +235 / +262 cleared,
   move 0xC and a pause +74 = 2 x strength + 1. */
SFA_FUNC(0895D2A4, move_land_c_pause) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08961448, 0x0895D2B8u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x0895D2D4u);
    else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 235) = 0; *B(o + 262) = 0;
        call2(c, o, 0xC, f_088616EC, 0x0895D304u);
        *B(o + 74) = (uint8_t)((S8(o + 194) << 1) + 1);
    }
    leave(c, sv);
}

/* (fighter): ticked; unless the frame flag is negative +7 steps on, +237 cleared and +573 from 0x08A85BD4
   by its +194 (with +81: by 0x0886E294). */
SFA_FUNC(0895ED6C, move_tick_573_r) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x0895ED84u);
    if (S8(o + 77) >= 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 81);
        *B(o + 7) = (uint8_t)(st + 1);
        const gaddr tab = 0x08A85BD4u;
        c->r[17] = tab;
        *B(o + 237) = 0;
        if (k != 0) { call1(c, o, f_0886E294, 0x0895EDB8u); *B(o + 573) = *B(c->r[2] + tab); }
        else *B(o + 573) = *B(tab + (uint32_t)S8(o + 194));
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 0x08961428; at or below the floor +7 steps on at the floor, +20 / +73 / +235 cleared, cue
   0x22 / 0x23 and move 0x63 + strength; else ticked. */
SFA_FUNC(0895EF7C, move_land_63) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08961428, 0x0895EF90u);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 235) = 0;
        call1(c, o, f_0887C614, 0x0895EFC8u);
        call2(c, o, str_move(o, 0x63), f_088616EC, 0x0895EFDCu);
    } else call1(c, o, f_08861848, 0x0895EFECu);
    leave(c, sv);
}

/* (fighter): thrown off: step 12, +73 = 1, 120 back from its side, 168 up, x 10.5, y -10.5 with nothing
   slowing it, +1 = 1, move 0x40. */
SFA_FUNC(0896B4E8, move_thrown_off) {
    gaddr o = ARG(0);
    uint32_t ra;
    *B(o + 7) = 0xC;
    int32_t side = S8(o + 337);
    *B(o + 73) = 1;
    frame_in(c, &ra);
    *H(o + 18) = (uint16_t)(S16(o + 18) + (side != 0 ? 0x78 : -0x78));
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0xA8);
    *W(o + 96) = 0xA8000; *W(o + 100) = 0xFFF58000u; *W(o + 108) = 0; *W(o + 104) = 0;
    *B(o + 1) = 1;
    call2(c, o, 0x40, f_0886163C, 0x0896B55Cu);
    frame_out(c, ra);
}

/* (fighter): a jump back: +7 steps on, +262 / +20 cleared at the floor, facing away from the opponent,
   x -3.0, y 8.0 / -2.0, +73 = 1, move 0x18. */
SFA_FUNC(08984DF4, move_jump_back_18) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t st = *B(o + 7);
    int32_t y = S16(o + 132);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 262) = 0; *H(o + 22) = (uint16_t)y;
    gaddr opp = *W(o + 92);
    *H(o + 20) = 0; *B(o + 11) = 0;
    int behind = S16(opp + 18) < S16(o + 18);
    frame_in(c, &ra);
    if (!behind) *B(o + 11) = 1;
    *W(o + 96) = 0xFFFD0000u; *W(o + 100) = 0x80000; *W(o + 104) = 0; *W(o + 108) = 0xFFFE0000u;
    *B(o + 73) = 1;
    call2(c, o, 0x18, f_088616EC, 0x08984E68u);
    frame_out(c, ra);
}

/* (fighter): 0x089930D8; when it says so +7 steps on, fall -0.34 and move 0x37 + 2 x strength; else +82
   bit 0 toggles and it ticks on the odd frames. */
SFA_FUNC(089915F4, move_check_37) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089930D8, 0x08991608u);
    if (c->r[2] != 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        *W(o + 108) = 0xFFFFA800u;
        call2(c, o, (uint32_t)((k << 1) + 0x37) & 0xFFFF, f_088616EC, 0x0899163Cu);
    } else {
        uint32_t h = (uint32_t)S16(o + 82) ^ 1;
        *H(o + 82) = (uint16_t)h;
        if ((int16_t)h != 0) call1(c, o, f_08861848, 0x08991664u);
    }
    leave(c, sv);
}

/* (fighter): +82 and the x speeds by strength (0x08A98908 / 0x08A9890C) toward its facing, move 0x58. */
SFA_FUNC(089967C8, move_dash_58) {
    gaddr o = ARG(0);
    uint32_t ra;
    int32_t k = S8(o + 194);
    *H(o + 82) = (uint16_t)S8(0x08A98908u + (uint32_t)k);
    gaddr e = 0x08A9890Cu + ((uint32_t)k << 3);
    uint32_t f = *B(o + 11);
    frame_in(c, &ra);
    if (f != 0) { *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); }
    else { *W(o + 96) = 0u - *W(e); *W(o + 104) = 0u - *W(e + 4); }
    call2(c, o, 0x58, f_088616EC, 0x0899683Cu);
    frame_out(c, ra);
}

/* (character): the part ticked; ended: standing; else a frame flag runs 0x088D3140, the first time also
   0x088D35C0 (facing), then +7 steps on and +82 = 9. */
SFA_FUNC(089B9EF4, part_flag_step_9) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089B9F08u);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089B9F1Cu);
    else if (S8(o + 77) != 0) {
        call1(c, o, f_088D3140, 0x089B9F34u);
        int32_t h = S16(o + 82);
        if (h == 0) {
            ARG(1) = *B(o + 11);
            *H(o + 82) = (uint16_t)(h + 1);
            ARG(0) = o; c->r[31] = 0x089B9F50u; f_088D35C0(c);
        }
        bump(o + 7);
        *H(o + 82) = 9;
    }
    leave(c, sv);
}

/* (character): the part ticked, 0x08862004; when 0x088D35D0 gives 1, +7 steps on, +73 cleared, move 0xA,
   cue 0x22, 0x088D36D0, 0x088D3960 (0, 0) and +262 cleared. */
SFA_FUNC(089BB95C, part_air_teleport_c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BB970u);
    call1(c, o, f_08862004, 0x089BB978u);
    call1(c, o, f_088D35D0, 0x089BB980u);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, 0xA, f_088616EC, 0x089BB9A8u);
        call1(c, o, f_088D2DBC, 0x089BB9B0u);
        call1(c, o, f_088D36D0, 0x089BB9B8u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089BB9C8u; f_088D3960(c);
        *B(o + 262) = 0;
    }
    leave(c, sv);
}

/* (fighter): 0x08864494, then crouching (step 1 / 0 / 0 / 2, +696 = 1), 0x0886731C, 0x0886D29C, the hit
   state cleared and move 1. */
SFA_FUNC(08864740, move_round_crouch) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08864494, 0x08864754u);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 2; *B(o + 696) = 1;
    call1(c, o, f_0886731C, 0x0886477Cu);
    call1(c, o, f_0886D29C, 0x08864784u);
    *B(o + 538) = 0; *B(o + 125) = 0; *H(o + 778) = 0; *B(o + 126) = 0; *W(o + 244) = 0;
    call2(c, o, 1, f_0886163C, 0x088647B4u);
    leave(c, sv);
}
