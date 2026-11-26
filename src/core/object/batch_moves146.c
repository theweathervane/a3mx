/* Move steps of 19 - 20 instructions: more "back to standing when the animation ends" steps, landings,
   jumps with their speeds, and moves started with a cue. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c);
void f_08861674(Cpu *c); void f_0886E3CC(Cpu *c); void f_0887C614(Cpu *c); void f_08931A94(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

static void framed(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back; fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter): back to standing (step 1 / 0 / 0 / 0, move 0, and +`extra` cleared when given) when the
   animation has ended (+75 < 0); else ticked. */
static void end_or_tick(Cpu *c, uint32_t extra, uint32_t back_stand, uint32_t back_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        if (extra) *B(o + extra) = 0;
        ARG(1) = 0; c->r[31] = back_stand; f_0886163C(c);
    } else { c->r[31] = back_tick; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0893BC3C, move_end_or_tick_h) { end_or_tick(c, 0, 0x0893BC68u, 0x0893BC7Cu); }
SFA_FUNC(08948810, move_end_or_tick_i) { end_or_tick(c, 0, 0x0894883Cu, 0x08948850u); }
SFA_FUNC(089B01BC, move_end_or_tick_j) { end_or_tick(c, 0, 0x089B01E8u, 0x089B01FCu); }
SFA_FUNC(0897AA00, move_end_or_tick_k) { end_or_tick(c, 0, 0x0897AA30u, 0x0897AA44u); }
SFA_FUNC(0898976C, move_end_or_tick_l) { end_or_tick(c, 0, 0x0898979Cu, 0x089897B0u); }
SFA_FUNC(089145F8, move_end_or_tick_372) { end_or_tick(c, 372, 0x08914628u, 0x0891463Cu); }
SFA_FUNC(0896B738, move_end_or_tick_656) { end_or_tick(c, 656, 0x0896B768u, 0x0896B77Cu); }

/* (fighter): when the animation ends +7 steps on with move 0x2F + its +194 (list 1); else ticked. */
SFA_FUNC(089683B4, move_end_then_2f) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) < 0) {
        bump(o + 7);
        ARG(1) = (uint32_t)(S8(o + 194) + 0x2F) & 0xFFFF;
        c->r[31] = 0x089683F4u; f_088616EC(c);
    } else { c->r[31] = 0x089683D0u; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): the +82 timer runs down; past 0 step 3 with move 0x36, else ticked. */
SFA_FUNC(0892F7DC, move_timer_then_36) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    *W(c->r[29] + 16) = ra;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) { *B(o + 7) = 3; ARG(1) = 0x36; c->r[31] = 0x0892F80Cu; f_0886163C(c); }
    else { c->r[31] = 0x0892F820u; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): landed with kind 4 (+116), +631 cleared, +73 = -1, +125 counted, height held down to +132,
   move 6 of list 2. */
SFA_FUNC(0885C858, move_land_6) {
    gaddr o = ARG(0);
    int32_t n = S8(o + 125);
    *B(o + 116) = 4; *B(o + 631) = 0; *B(o + 73) = 0xFF;
    int32_t y = S16(o + 22), floor = S16(o + 132);
    *B(o + 125) = (uint8_t)(n + 1);
    if (y < floor) *H(o + 22) = (uint16_t)floor;
    ARG(1) = 6; framed(c, f_08861724, 0x0885C89Cu);
}

/* (fighter): +7 steps on, the grab record (+0x3B8: on, -1 at +5, the opponent's (+92) +16 at +8),
   +262 = 1, move 7 of list 1. */
SFA_FUNC(0893DE0C, move_grab_7) {
    gaddr o = ARG(0);
    bump(o + 7);
    gaddr g = o + 0x3B8;
    *B(g) = 1;
    gaddr opp = *W(o + 92);
    *B(o + 262) = 1;
    *W(g + 8) = *W(opp + 16);
    *B(g + 5) = 0xFF;
    ARG(1) = 7; framed(c, f_088616EC, 0x0893DE4Cu);
}

/* (fighter): ticked; on a frame flag +7 steps on, +74 = 1 and ticked again. */
SFA_FUNC(0896D6AC, move_tick_flag_twice) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0896D6C0u);
    if (S8(o + 77) != 0) { bump(o + 7); *B(o + 74) = 1; call1(c, o, f_08861848, 0x0896D6E8u); }
    leave(c, sv);
}

/* (fighter): 0x0886E3CC (9, 0x12), then move 0x12 + 4 * its +194 + its +96 (list 1). */
SFA_FUNC(089A3ED8, move_strength_12) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    ARG(1) = 9; ARG(2) = 0x12; c->r[31] = 0x089A3EF4u; f_0886E3CC(c);
    uint32_t n = *W(o + 96) + ((uint32_t)S8(o + 194) << 2) + 0x12;
    call2(c, o, n & 0xFFFF, f_088616EC, 0x089A3F14u);
    leave(c, sv);
}

/* (fighter): +7 steps on, +73 / +235 / +262 cleared, height +22 = +132, cue 0x22 / 0x23, ticked. */
SFA_FUNC(088EF6D4, move_land_cue_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    int32_t y = S16(o + 132);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 73) = 0; *H(o + 22) = (uint16_t)y; *B(o + 235) = 0; *B(o + 262) = 0;
    *W(c->r[29] + 20) = ra;
    call1(c, o, f_0887C614, 0x088EF70Cu);
    call1(c, o, f_08861848, 0x088EF714u);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): step 5, +73 = 1, +20 cleared, speeds (x 6.0 / -0.3125, y 8.0 / -0.1875), move 0x31. */
SFA_FUNC(088F4E4C, move_jump_31) {
    gaddr o = ARG(0);
    *B(o + 7) = 5; *B(o + 73) = 1; *H(o + 20) = 0;
    *W(o + 96) = 0x60000; *W(o + 104) = 0xFFFFB000u;
    *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFFD000u;
    ARG(1) = 0x31; framed(c, f_088616EC, 0x088F4E90u);
}

/* (fighter): step 3, +73 = 1, move 0x1F picked, then the speeds cleared with a slight fall (+108). */
SFA_FUNC(0890F83C, move_hover_1f) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 7) = 3; *B(o + 73) = 1;
    *W(c->r[29] + 20) = ra;
    call2(c, o, 0x1F, f_08861674, 0x0890F868u);
    *W(o + 100) = 0; *W(o + 108) = 0xFFFFFFAAu; *W(o + 96) = 0; *W(o + 104) = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; a frame flag is taken: +7 steps on and +104 = -0.25; then 0x08931A94 either way. */
SFA_FUNC(089319F0, move_tick_then_brake) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931A04u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0;
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 104) = 0xFFFFC000u;
    }
    call1(c, o, f_08931A94, 0x08931A30u);
    leave(c, sv);
}

/* (fighter): landed: step 6, +73 / +20 / +262 cleared, height +22 = +132, cue 0x22 / 0x23, move `n`
   (list 1). */
static void land_move(Cpu *c, uint32_t n, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 7) = 6;
    int32_t y = S16(o + 132);
    *B(o + 73) = 0; *H(o + 20) = 0; *H(o + 22) = (uint16_t)y; *B(o + 262) = 0;
    *W(c->r[29] + 20) = ra;
    call1(c, o, f_0887C614, back1);
    call2(c, o, n, f_088616EC, back2);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08935E6C, move_land_12) { land_move(c, 0x12, 0x08935EA0u, 0x08935EACu); }
SFA_FUNC(08936178, move_land_2b) { land_move(c, 0x2B, 0x089361ACu, 0x089361B8u); }

/* (fighter): +7 steps on, 0x0886E3CC (0, 0), +262 cleared, move 0x16 + its +194 (list 1). */
SFA_FUNC(0896900C, move_step_16) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    bump(o + 7);
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x08969034u; f_0886E3CC(c);
    int32_t k = S8(o + 194);
    *B(o + 262) = 0;
    call2(c, o, (uint32_t)(k + 0x16) & 0xFFFF, f_088616EC, 0x0896904Cu);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
