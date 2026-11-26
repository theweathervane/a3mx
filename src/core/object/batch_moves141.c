/* Small move steps, 12 - 14 instructions: a move picked as a step begins, or the animation ticked with
   the step moved on by its frame's +77 / +75. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3DD8(Cpu *c);

static void framed(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back; fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

/* (fighter): +6 steps on, +11 = +697, move 0. */
SFA_FUNC(088ED7FC, move_start_697) {
    gaddr o = ARG(0);
    bump(o + 6); *B(o + 11) = *B(o + 697);
    ARG(1) = 0; framed(c, f_0886163C, 0x088ED820u);
}
/* (fighter): +7 steps on, +1 = 1, move 0x22 (list 0) / 0x32 (list 1). */
SFA_FUNC(089145C8, move_step1_a22) {
    bump(ARG(0) + 7); *B(ARG(0) + 1) = 1;
    ARG(1) = 0x22; framed(c, f_0886163C, 0x089145ECu);
}
SFA_FUNC(089B0244, move_step1_b32) {
    bump(ARG(0) + 7); *B(ARG(0) + 1) = 1;
    ARG(1) = 0x32; framed(c, f_088616EC, 0x089B0268u);
}
/* (fighter): +7 = 3, +656 / +262 cleared, move 0x12 + its +194 (list 1). */
SFA_FUNC(0896D594, move_set_b12) {
    gaddr o = ARG(0);
    *B(o + 7) = 3;
    int32_t k = S8(o + 194);
    *B(o + 656) = 0; *B(o + 262) = 0;
    ARG(1) = (uint32_t)(k + 0x12) & 0xFFFF; framed(c, f_088616EC, 0x0896D5BCu);
}
/* (fighter): step 1 / 0 / 1 / 2 (+4 - +7), +696 cleared, move 4. */
SFA_FUNC(0886244C, move_reset_4) {
    gaddr o = ARG(0);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 1; *B(o + 7) = 2; *B(o + 696) = 0;
    ARG(1) = 4; framed(c, f_0886163C, 0x08862478u);
}
/* (fighter): +7 steps on, +262 = 1, +644 = 2, move 0x26 (list 1). */
SFA_FUNC(088F08C4, move_step_b26) {
    gaddr o = ARG(0);
    bump(o + 7); *B(o + 262) = 1; *B(o + 644) = 2;
    ARG(1) = 0x26; framed(c, f_088616EC, 0x088F08F0u);
}
/* (fighter): +7 steps on, +82 = 60, +11 = +697, move 0. */
SFA_FUNC(08908834, move_step_wait_697) {
    gaddr o = ARG(0);
    bump(o + 7); *H(o + 82) = 0x3C; *B(o + 11) = *B(o + 697);
    ARG(1) = 0; framed(c, f_0886163C, 0x08908860u);
}
/* (fighter): move (table 0x08A790DE by its +194) + its +199, list 1. */
SFA_FUNC(0894A35C, move_by_194) {
    gaddr o = ARG(0);
    ARG(1) = ((uint32_t)*B(0x08A790DEu + (uint32_t)S8(o + 194)) + (uint32_t)S8(o + 199)) & 0xFFFF;
    framed(c, f_088616EC, 0x0894A388u);
}

/* The animation ticked after `pre` has looked at the fighter. */
static void tick_after(Cpu *c, void (*pre)(gaddr), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    pre(ARG(0));
    c->r[31] = back; f_08861848(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
static void pre_77_set_2(gaddr o) { if (S8(o + 77) == 0) { *B(o + 7) = 2; *B(o + 73) = 1; } }
static void pre_77_set_1(gaddr o) { if (S8(o + 77) == 0) { *B(o + 7) = 1; *B(o + 73) = 1; } }
static void pre_77_nonneg(gaddr o) { if (S8(o + 77) >= 0) bump(o + 7); }
static void pre_77_zero(gaddr o) { if (S8(o + 77) == 0) bump(o + 7); }
static void pre_77_zero_237(gaddr o) { if (S8(o + 77) == 0) { *B(o + 237) = 0; bump(o + 7); } }
static void pre_77_one(gaddr o) { if (S8(o + 77) == 1) bump(o + 7); }
static void pre_77_take(gaddr o) { if (S8(o + 77) != 0) { *B(o + 77) = 0; bump(o + 7); } }
static void pre_75_end(gaddr o) { if (S8(o + 75) < 0) { *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; } }

SFA_FUNC(088F3514, move_tick_77_2) { tick_after(c, pre_77_set_2, 0x088F353Cu); }
SFA_FUNC(088F8124, move_tick_77_2_b) { tick_after(c, pre_77_set_2, 0x088F814Cu); }
SFA_FUNC(08976CC4, move_tick_77_1) { tick_after(c, pre_77_set_1, 0x08976CECu); }
SFA_FUNC(08968BE4, move_tick_77_nonneg) { tick_after(c, pre_77_nonneg, 0x08968C0Cu); }
SFA_FUNC(08984E74, move_tick_77_zero) { tick_after(c, pre_77_zero, 0x08984E9Cu); }
SFA_FUNC(08912224, move_tick_77_zero_237) { tick_after(c, pre_77_zero_237, 0x08912250u); }
SFA_FUNC(08926C6C, move_tick_77_one) { tick_after(c, pre_77_one, 0x08926C98u); }
SFA_FUNC(08926DA4, move_tick_77_one_b) { tick_after(c, pre_77_one, 0x08926DD0u); }
SFA_FUNC(0899FA18, move_tick_77_take) { tick_after(c, pre_77_take, 0x0899FA44u); }
SFA_FUNC(08947770, move_tick_75_end) { tick_after(c, pre_75_end, 0x0894779Cu); }

/* (part): with its animation ended (+75 < 0) +4 steps on and +5 cleared; then the part's timer. */
SFA_FUNC(089CBD94, part_tick_75_next) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) < 0) { *B(o + 5) = 0; bump(o + 4); }
    c->r[31] = 0x089CBDC0u; f_088D3DD8(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked, then +656 = its +76 / 2. */
SFA_FUNC(08917840, move_tick_656) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08917854u; f_08861848(c);
    *B(o + 656) = (uint8_t)(S8(o + 76) >> 1);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked, then +7 = 4 when its +77 is set. */
SFA_FUNC(0892C388, move_tick_then_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x0892C39Cu; f_08861848(c);
    if (S8(o + 77) != 0) *B(o + 7) = 4;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
