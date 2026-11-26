/* Move steps of 9 - 36 instructions: the knock-out check's entry, flights that land into a move, a lunge
   that marks its reach (0x089164E8), a throw placed by side or cut short by the opponent's state
   (0x0892F5B4) and a stance-dependent voice (0x08987170). Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0885D21C(Cpu *c);
void f_0896E118(Cpu *c); void f_0890AA58(Cpu *c); void f_08917870(Cpu *c); void f_0887C614(Cpu *c);
void f_0892F7B0(Cpu *c); void f_08837098(Cpu *c); void f_08939BF4(Cpu *c); void f_0887C408(Cpu *c);

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

/* (fighter): +7 steps on, then the knock-out check (0x0885D21C). */
SFA_FUNC(0885D1F8, fighter_hit_ko_step) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    bump(o + 7);
    call1(c, o, f_0885D21C, 0x0885D210u);
    frame_out(c, ra);
}

/* (fighter): with +123 a hop back (step 5, x -2.0 / +1/64, y 4.5 / -0.3125, +16 / +20 low halves
   cleared); else 0x0896E118. */
SFA_FUNC(0896E0B4, move_hop_back_or) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 123) != 0) {
        *B(o + 7) = 5;
        *W(o + 96) = 0xFFFE0000u; *W(o + 104) = 0x400; *W(o + 100) = 0x48000; *W(o + 108) = 0xFFFFB000u;
        *H(o + 20) = 0; *H(o + 16) = 0;
    } else call1(c, o, f_0896E118, 0x0896E0D0u);
    frame_out(c, ra);
}

/* (fighter): flies (+16 by +96 by +104, +20 by +100 by +108); at or below the floor +7 steps on, +73
   cleared and move 0x2D; else 0x0890AA58. */
SFA_FUNC(0890A9C8, move_fly_land_2d) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t vx = *W(o + 96), vy = *W(o + 100);
    *W(o + 16) = *W(o + 16) + vx;
    *W(o + 20) = *W(o + 20) + vy;
    *W(o + 96) = vx + *W(o + 104);
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *W(o + 100) = vy + *W(o + 108);
    frame_in(c, &ra);
    if (!(floor < y)) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 0;
        call2(c, o, 0x2D, f_088616EC, 0x0890AA38u);
    } else call1(c, o, f_0890AA58, 0x0890AA4Cu);
    frame_out(c, ra);
}

/* (fighter): a frame flag starts a lunge: +7 on, +82 / +123 cleared, x 9.0 / -0.25 toward the facing, the
   mark (+0x334) 192 ahead, 0x08917870; ticked. */
SFA_FUNC(089164E8, move_flag_lunge_mark) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        uint32_t st = *B(o + 7);
        *H(o + 82) = 0; *B(o + 7) = (uint8_t)(st + 1);
        uint32_t f = *B(o + 11);
        *B(o + 123) = 0;
        int32_t x = S16(o + 18);
        gaddr m = o + 0x334;
        if (f == 0) { *W(o + 96) = 0xFFF70000u; *W(o + 104) = 0x4000; *H(m) = (uint16_t)(x - 0xC0); }
        else { *W(o + 96) = 0x90000; *W(o + 104) = 0xFFFFC000u; *H(m) = (uint16_t)(x + 0xC0); }
        call1(c, o, f_08917870, 0x08916560u);
    }
    call1(c, o, f_08861848, 0x08916568u);
    leave(c, sv);
}

/* (fighter): it falls (+20 by +100 by +108); at or below the floor +235 / +73 cleared, +7 steps on, height
   to the floor, cue 0x22 / 0x23 and ticked; in the air ticked until a frame flag. */
SFA_FUNC(0892BC38, move_fall_land_cue_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *W(o + 100) = vy + *W(o + 108);
    if (!(floor < y)) {
        uint32_t st = *B(o + 7);
        *B(o + 235) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 73) = 0; *H(o + 22) = (uint16_t)floor;
        call1(c, o, f_0887C614, 0x0892BCB0u);
        call1(c, o, f_08861848, 0x0892BCB8u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0892BC8Cu);
    leave(c, sv);
}

/* (fighter): +1 = 1; with the opponent in state 0xC / 0x1D / 0x22, step 4 and 0x0892F7B0; else placed
   at x 0x170 / 0x390 by its side, x 8.0 / -0.375, step 1 and move 0x22. */
SFA_FUNC(0892F5B4, move_throw_or_cut) {
    gaddr o = ARG(0); uint32_t ra;
    gaddr opp = *W(o + 92);
    *B(o + 1) = 1;
    int32_t s = S8(opp + 338);
    frame_in(c, &ra);
    if (s == 0xC || s == 0x1D || s == 0x22) { *B(o + 7) = 4; call1(c, o, f_0892F7B0, 0x0892F5F4u); }
    else {
        *H(o + 18) = (S8(o + 337) & 1) ? 0x170 : 0x390;
        *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFFA000u;
        *B(o + 7) = 1;
        call2(c, o, 0x22, f_0886163C, 0x0892F638u);
    }
    frame_out(c, ra);
}

/* (fighter): +7 steps on, +262 = 1, 0x08837098, 0x08939BF4, y speeds cleared, +83 = 8, x 5.0 away from
   the facing, move 0x19 + strength. */
SFA_FUNC(0893848C, move_step_19_back) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    bump(o + 7);
    *B(o + 262) = 1;
    call1(c, o, f_08837098, 0x089384B8u);
    call1(c, o, f_08939BF4, 0x089384C0u);
    *W(o + 108) = 0; *W(o + 100) = 0; *W(o + 104) = 0;
    *B(o + 0x53) = 8;
    *W(o + 96) = *B(o + 11) != 0 ? 0xFFFB0000u : 0x50000;
    call2(c, o, (uint32_t)(S8(o + 194) + 0x19) & 0xFFFF, f_088616EC, 0x0893850Cu);
    leave(c, sv);
}

/* (fighter): +543 = 2; once +96 turns, step 5; else (without a frame flag it also rises: +73 = 1, +20 by
   +100 by +108) it moves by +96 toward its facing, +96 by +104; ticked. */
SFA_FUNC(08965630, move_glide) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t v = *W(o + 96);
    frame_in(c, &ra);
    *B(o + 543) = 2;
    if ((int32_t)v < 0) *B(o + 7) = 5;
    else {
        uint32_t x = *W(o + 16);
        if (S8(o + 77) == 0) {
            uint32_t vy = *W(o + 100);
            *B(o + 73) = 1;
            *W(o + 20) = *W(o + 20) + vy;
            *W(o + 100) = vy + *W(o + 108);
        }
        *W(o + 16) = *B(o + 11) == 0 ? x - v : x + v;
        *W(o + 96) = v + *W(o + 104);
    }
    call1(c, o, f_08861848, 0x089656B4u);
    frame_out(c, ra);
}

/* (fighter): +7 steps on, a voice by stance (+236: 5 the second of 0x08A94768, else the first) from its
   side, then move 0x13 in stance 3, else 0x11. */
SFA_FUNC(08987170, move_stance_voice) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    int32_t stance = S8(o + 236);
    *W(c->r[29] + 20) = s1;
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[17] = 3;
    *W(c->r[29] + 24) = ra;
    uint32_t i = stance == 5 ? 1 : 0;
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = (uint32_t)(int32_t)S16(0x08A94768u + i + i);
    c->r[31] = 0x089871D4u; f_0887C408(c);
    call2(c, o, S8(o + 236) == 3 ? 0x13 : 0x11, f_088616EC, 0x089871ECu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
