/* Move steps of 17 - 18 instructions: the animation ticked with the step moved on by the frame's flags,
   speeds or a helper, and moves picked as steps begin. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0885D134(Cpu *c); void f_0887C408(Cpu *c); void f_088616EC(Cpu *c);
void f_0886163C(Cpu *c); void f_0892B188(Cpu *c); void f_08944300(Cpu *c); void f_08862004(Cpu *c);
void f_089B298C(Cpu *c); void f_08837098(Cpu *c);

#define BATTLE 0x092E1228u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

/* The animation ticked after `pre` has looked at the fighter (a 0x20 frame keeping the return). */
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

/* The animation ticked, then `post` looks at the fighter (s0 kept). */
static void tick_then(Cpu *c, void (*post)(gaddr), uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = back; f_08861848(c);
    post(o);
    leave(c, sv);
}

static void pre_77_take_move16(gaddr o) {
    if (S8(o + 77) != 0) { *B(o + 77) = 0; bump(o + 7); }
    *W(o + 16) = *W(o + 16) + *W(o + 96);
}
static void pre_77_set3_speeds(gaddr o) {
    if (S8(o + 77) == 0) {
        *B(o + 7) = 3; *B(o + 73) = 1;
        *W(o + 96) = *W(o + 100); *W(o + 104) = *W(o + 108);
    }
}
static void pre_77_next_mark(gaddr o) { if (S8(o + 77) != 0) { bump(o + 7); *B(o + 73) = 1; *B(o + 0x334 + 8) = 0; } }
static void pre_77_next_wait(gaddr o) { if (S8(o + 77) == 0) { bump(o + 7); *B(o + 73) = 1; *H(o + 82) = 0x10; } }
static void pre_fall(gaddr o) {
    *W(o + 20) = *W(o + 20) + *W(o + 100);
    uint32_t v = *W(o + 100) + *W(o + 108);
    *W(o + 100) = v;
    if ((int32_t)v < 0) *B(o + 7) = 6;
}
static void pre_battle_next6(gaddr o) { if (S8(BATTLE + 36) == 0 && S8(BATTLE + 28) == 0) bump(o + 6); }

static void post_77_set_next(gaddr o) { if (S8(o + 77) != 0) { bump(o + 7); *B(o + 73) = 1; } }
static void post_77_clear_next(gaddr o) { if (S8(o + 77) == 0) { bump(o + 7); *B(o + 73) = 1; } }
static void post_75_end_235(gaddr o) { if (S8(o + 75) < 0) { bump(o + 7); *B(o + 235) = 1; } }

SFA_FUNC(088FE170, move_tick_drift) { tick_after(c, pre_77_take_move16, 0x088FE1A8u); }
SFA_FUNC(08902B98, move_tick_77_speeds) { tick_after(c, pre_77_set3_speeds, 0x08902BD0u); }
SFA_FUNC(0890A460, move_tick_77_mark) { tick_after(c, pre_77_next_mark, 0x0890A498u); }
SFA_FUNC(08910690, move_tick_77_wait) { tick_after(c, pre_77_next_wait, 0x089106C8u); }
SFA_FUNC(089656C0, move_tick_fall) { tick_after(c, pre_fall, 0x089656F8u); }
SFA_FUNC(088ED82C, move_tick_battle) { tick_after(c, pre_battle_next6, 0x088ED868u); }

SFA_FUNC(088FF45C, move_tick_then_77_set) { tick_then(c, post_77_set_next, 0x088FF470u); }
SFA_FUNC(089069D4, move_tick_then_77_set_b) { tick_then(c, post_77_set_next, 0x089069E8u); }
SFA_FUNC(08906BF0, move_tick_then_77_clear) { tick_then(c, post_77_clear_next, 0x08906C04u); }
SFA_FUNC(089873C0, move_tick_then_75) { tick_then(c, post_75_end_235, 0x089873D4u); }

/* (fighter): while alive (+112 >= 0) its cue 0x203 + its +414; else the one-off cue set (0x0885D134). */
SFA_FUNC(0885D0F0, fighter_voice_or_ko) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S16(o + 112) >= 0) {
        ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = (uint32_t)(S8(o + 414) + 0x203);
        c->r[31] = 0x0885D114u; f_0887C408(c);
    } else { c->r[31] = 0x0885D128u; f_0885D134(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 steps on, +11 toggled, move 0x28 (0x2A with +194), list 1. */
SFA_FUNC(089161F8, move_toggle_28) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    bump(o + 7);
    *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
    *W(c->r[29] + 16) = ra;
    ARG(1) = S8(o + 194) == 0 ? 0x28 : 0x2A;
    c->r[31] = 0x08916230u; f_088616EC(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 = 1, 0x0892B188, +11 = +697 and move 0x18 (list 1). */
SFA_FUNC(0892E4F8, move_start_18) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1;
    ARG(0) = o; c->r[31] = 0x0892E518u; f_0892B188(c);
    *B(o + 11) = *B(o + 697);
    ARG(0) = o; ARG(1) = 0x18; c->r[31] = 0x0892E52Cu; f_088616EC(c);
    leave(c, sv);
}

/* (fighter): ticked; at frame flag 4 (+77) step 3, +262 cleared and 0x08944300. */
SFA_FUNC(08944350, move_tick_then_4) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x08944364u; f_08861848(c);
    if (S8(o + 77) == 4) {
        *B(o + 7) = 3; *B(o + 262) = 0;
        ARG(0) = o; c->r[31] = 0x08944384u; f_08944300(c);
    }
    leave(c, sv);
}

/* (fighter): 0x08862004; once falling (+100 < 0) step 6 with +108 = +820; then ticked. */
SFA_FUNC(089460C0, move_rise_then_fall) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x089460D4u; f_08862004(c);
    if ((int32_t)*W(o + 100) < 0) { *B(o + 7) = 6; *W(o + 108) = *W(o + 820); }
    ARG(0) = o; c->r[31] = 0x089460F4u; f_08861848(c);
    leave(c, sv);
}

/* (fighter): 0x089B298C; +7 steps on once +96 is negative; then ticked. */
SFA_FUNC(089B1408, move_check96_then_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x089B141Cu; f_089B298C(c);
    if ((int32_t)*W(o + 96) < 0) bump(o + 7);
    ARG(0) = o; c->r[31] = 0x089B143Cu; f_08861848(c);
    leave(c, sv);
}

/* (fighter): step 1 / 0 / 3 / 0, +266 / +696 cleared, +543 = 5, move 0x10. */
SFA_FUNC(089BD024, move_reset_10) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3; *B(o + 7) = 0;
    *B(o + 266) = 0; *B(o + 696) = 0; *B(o + 543) = 5;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 0x10; c->r[31] = 0x089BD05Cu; f_0886163C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 steps on, move 0x1D (list 1), 0x08837098 and +262 = 1. */
SFA_FUNC(089C1C68, move_step_1d) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    bump(o + 7);
    ARG(1) = 0x1D; c->r[31] = 0x089C1C8Cu; f_088616EC(c);
    ARG(0) = o; c->r[31] = 0x089C1C94u; f_08837098(c);
    *B(o + 262) = 1;
    leave(c, sv);
}
