/* Move steps of 15 - 22 instructions: step dispatchers, jumps and knock-backs with their speeds, landings
   with a cue, and moves started when an animation ends. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_089145C8(Cpu *c);
void f_089145F8(Cpu *c); void f_089B01BC(Cpu *c); void f_089B0244(Cpu *c); void f_0886E294(Cpu *c);
void f_0899CF0C(Cpu *c); void f_088D3D94(Cpu *c); void f_0887C614(Cpu *c); void f_088EF7F8(Cpu *c);
void f_088EF6D4(Cpu *c); void f_08837098(Cpu *c); void f_08939BF4(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void tick_after(Cpu *c, void (*pre)(gaddr), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    pre(ARG(0));
    c->r[31] = back; f_08861848(c);
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
static void tick_then(Cpu *c, void (*post)(gaddr), uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, back);
    post(o);
    leave(c, sv);
}

/* (fighter): step 0 starts the move, later steps run it. */
static void dispatch(Cpu *c, void (*start)(Cpu *), uint32_t back_start, void (*run)(Cpu *), uint32_t back_run) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(ARG(0) + 7) == 0) { c->r[31] = back_start; start(c); }
    else { c->r[31] = back_run; run(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0891458C, move_dispatch_a) { dispatch(c, f_089145C8, 0x089145A8u, f_089145F8, 0x089145BCu); }
SFA_FUNC(089B0208, move_dispatch_b) { dispatch(c, f_089B0244, 0x089B0224u, f_089B01BC, 0x089B0238u); }

static void pre_77_jump2(gaddr o) {
    if (S8(o + 77) == 0) {
        bump(o + 7);
        *W(o + 96) = 0x30000; *W(o + 104) = 0; *W(o + 100) = 0x50000; *W(o + 108) = 0xFFFF8000u;
    }
}
static void pre_77_bounce(gaddr o) {
    if (S8(o + 77) != 0) {
        bump(o + 7);
        *W(o + 96) = 0xFFFD8000u; *W(o + 104) = 0; *W(o + 100) = 0x58000; *W(o + 108) = 0xFFFFB000u;
    }
}
static void pre_slide(gaddr o) {
    uint32_t v = *W(o + 96);
    if (*B(o + 11) == 0) *W(o + 16) = *W(o + 16) - v;
    else *W(o + 16) = *W(o + 16) + v;
    v += *W(o + 104);
    *W(o + 96) = v;
    if ((int32_t)v < 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
}
static void pre_123_flag(gaddr o) {
    if (S8(o + 123) != 0) { *B(o + 123) = 0; *B(*W(o + 92) + 127) = 4; }
    if (S8(o + 77) & 0xF) { *B(o + 77) = 0; bump(o + 7); }
}
SFA_FUNC(0899BF98, move_tick_77_jump2) { tick_after(c, pre_77_jump2, 0x0899BFDCu); }
SFA_FUNC(0890F978, move_tick_77_bounce) { tick_after(c, pre_77_bounce, 0x0890F9C4u); }
SFA_FUNC(088F0C48, move_tick_slide) { tick_after(c, pre_slide, 0x088F0C94u); }
SFA_FUNC(0891B2F8, move_tick_123_flag) { tick_after(c, pre_123_flag, 0x0891B344u); }

static void post_77_drop(gaddr o) {
    if (S8(o + 77) == 0) { bump(o + 7); *W(o + 100) = 0xFFFE8000u; *W(o + 108) = 0xFFFFB400u; }
}
static void post_76_573(gaddr o) {
    if (S8(o + 76) == 0) { bump(o + 7); *B(o + 237) = 0; *B(o + 573) = (uint8_t)((S8(o + 194) << 1) + 4); }
}
static void post_77_shift(gaddr o) {
    if (S8(o + 77) != 0) {
        bump(o + 7);
        *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) == 0 ? -0x2A : 0x2A));
    }
}
SFA_FUNC(089A6924, move_tick_then_drop) { tick_then(c, post_77_drop, 0x089A6938u); }
SFA_FUNC(089A7E10, move_tick_then_drop_b) { tick_then(c, post_77_drop, 0x089A7E24u); }
SFA_FUNC(089A7B98, move_tick_then_573) { tick_then(c, post_76_573, 0x089A7BACu); }
SFA_FUNC(08908A7C, move_tick_then_shift) { tick_then(c, post_77_shift, 0x08908A90u); }

/* (fighter): +82 cleared (30 when on the right side in the same state as the opponent), +7 steps on,
   move 0x22. */
SFA_FUNC(089C60EC, move_wait_by_opponent) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr opp = *W(o + 92);
    *H(o + 82) = 0;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 338) == S8(opp + 338) && S8(o + 337) != 0) *H(o + 82) = 0x1E;
    bump(o + 7);
    ARG(1) = 0x22; c->r[31] = 0x089C6130u; f_0886163C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): without a frame flag step 3, +237 cleared, 0x0886E294 and (without +81) +573 = 12; ticked. */
SFA_FUNC(088F4DB0, move_flag_or_recover) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        *B(o + 7) = 3; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x088F4DDCu);
        if (S8(o + 81) == 0) *B(o + 573) = 0xC;
    }
    call1(c, o, f_08861848, 0x088F4DF4u);
    leave(c, sv);
}

/* (fighter): +7 steps on, +73 = 1, jump speeds (x 4.0, y 9.0 / -0.4375), move 0x13 + 2 * its +194. */
SFA_FUNC(089078C0, move_jump_13) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    bump(o + 7);
    *B(o + 73) = 1;
    *W(o + 96) = 0x40000; *W(o + 104) = 0; *W(o + 100) = 0x90000;
    int32_t k = S8(o + 194);
    *W(o + 108) = 0xFFFF9000u;
    *W(c->r[29] + 16) = ra;
    ARG(1) = (uint32_t)(k << 1) + 0x13;
    c->r[31] = 0x08907908u; f_088616EC(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): when the animation ends +7 steps on, +73 = 1, +123 cleared and move 0xB; else ticked. */
static void end_then_b(Cpu *c, uint32_t back_pick, uint32_t back_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) < 0) {
        bump(o + 7); *B(o + 73) = 1; *B(o + 123) = 0;
        ARG(1) = 0xB; c->r[31] = back_pick; f_088616EC(c);
    } else { c->r[31] = back_tick; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089213CC, move_end_then_b) { end_then_b(c, 0x08921400u, 0x08921414u); }
SFA_FUNC(0892288C, move_end_then_b2) { end_then_b(c, 0x089228C0u, 0x089228D4u); }

/* (fighter): 0x0899CF0C; when the animation ends +7 steps on with move 0x1A, else ticked. */
SFA_FUNC(0899BF44, move_helper_end_1a) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0899CF0C, 0x0899BF58u);
    if (S8(o + 75) < 0) { bump(o + 7); call2(c, o, 0x1A, f_088616EC, 0x0899BF88u); }
    else call1(c, o, f_08861848, 0x0899BF6Cu);
    leave(c, sv);
}

/* (effect): +5 steps on, its fighter's (+88) count +536 down and its link (+228) to it cut; without
   +141 its animation 4 of 0x08AB2FDC set. */
SFA_FUNC(089CBD40, effect_release) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr f = *W(o + 88);
    bump(o + 5);
    *B(f + 536) = (uint8_t)(*B(f + 536) - 1);
    *W(c->r[29] + 16) = ra;
    if (*W(f + 228) == o) *W(f + 228) = 0;
    if (S8(o + 141) == 0) { ARG(1) = 0x08AB2FDCu; ARG(2) = 4; c->r[31] = 0x089CBD88u; f_088D3D94(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): step 5, facing +11 from +238 (0 for 1, else 1), cue 0x22 / 0x23, move 0x30. */
SFA_FUNC(08864204, move_turn_30) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    *B(o + 7) = 5;
    int32_t t = S8(o + 238);
    enter(c, o, sv);
    *B(o + 11) = t == 1 ? 0 : 1;
    call1(c, o, f_0887C614, 0x08864240u);
    call2(c, o, 0x30, f_0886163C, 0x0886424Cu);
    leave(c, sv);
}

/* (fighter): 0x088EF7F8; on the floor (+22 not below +132) it lands (0x088EF6D4), else ticked until a
   frame flag. */
SFA_FUNC(088EF67C, move_fall_or_land) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088EF7F8, 0x088EF690u);
    if (S16(o + 132) < S16(o + 22)) { if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x088EF6C4u); }
    else call1(c, o, f_088EF6D4, 0x088EF6ACu);
    leave(c, sv);
}

/* (fighter): landed: step 5, height +22 = +132, +262 / +73 / +20 cleared, cue 0x22 / 0x23, move 0xB. */
SFA_FUNC(089070C4, move_land_b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    int32_t y = S16(o + 132);
    *B(o + 7) = 5; *H(o + 22) = (uint16_t)y;
    *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
    *W(c->r[29] + 20) = ra;
    call1(c, o, f_0887C614, 0x08907100u);
    call2(c, o, 0xB, f_088616EC, 0x0890710Cu);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): step 2 (3 with +373), +104 = -0.5, +82 = 8, move 0xD (0x33 when +386 is negative). */
SFA_FUNC(08911058, move_knock_d) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    *B(o + 7) = S8(o + 373) != 0 ? 3 : 2;
    *W(o + 104) = 0xFFFF8000u;
    int32_t k = S8(o + 386);
    *H(o + 82) = 8;
    ARG(1) = (k & 0x80) ? 0x33 : 0xD;
    c->r[31] = 0x089110A4u; f_088616EC(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 steps on, +0x3C0 cleared, +262 = 1, 0x08837098, 0x08939BF4, move 0x20 (list 1). */
SFA_FUNC(0893878C, move_step_20) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    bump(o + 7);
    *B(o + 0x3A0 + 32) = 0;
    *B(o + 262) = 1;
    *W(c->r[29] + 20) = ra;
    call1(c, o, f_08837098, 0x089387C0u);
    call1(c, o, f_08939BF4, 0x089387C8u);
    call2(c, o, 0x20, f_088616EC, 0x089387D4u);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
