/* Move steps of 12 - 18 instructions: the animation ticked around frame flags, falls and timers, and
   moves picked as steps begin. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0885F0F8(Cpu *c); void f_0885CE18(Cpu *c); void f_088F501C(Cpu *c);
void f_08903954(Cpu *c); void f_08908C04(Cpu *c); void f_08908BA8(Cpu *c); void f_0887C614(Cpu *c);
void f_08861674(Cpu *c); void f_088616EC(Cpu *c); void f_0886163C(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

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
    c->r[31] = back; f_08861848(c);
    post(o);
    leave(c, sv);
}

static void pre_mark_timer(gaddr o) {
    int8_t t = (int8_t)(S8(o + 0x336) - 1);
    *B(o + 0x336) = (uint8_t)t;
    if (t < 0) bump(o + 7);
}
static void pre_fall_next(gaddr o) {
    *W(o + 20) = *W(o + 20) + *W(o + 100);
    uint32_t v = *W(o + 100) + *W(o + 108);
    *W(o + 100) = v;
    if ((int32_t)v < 0) bump(o + 7);
}
static void pre_77_hop(gaddr o) {
    if (S8(o + 77) == 0) { bump(o + 6); *B(o + 73) = 1; *W(o + 100) = 0; *W(o + 108) = 0xFFFFE000u; }
}
static void pre_77_198(gaddr o) {
    if (S8(o + 77) == 0) { bump(o + 7); *B(o + 73) = 1; *B(o + 198) = (uint8_t)(S8(o + 194) + 5); }
}
static void post_77_rise(gaddr o) { if (S8(o + 77) != 0) { bump(o + 7); *H(o + 22) = (uint16_t)(S16(o + 22) + 0x28); } }
static void post_77_take(gaddr o) { if (S8(o + 77) != 0) { bump(o + 7); *B(o + 73) = 1; *B(o + 77) = 0; } }

SFA_FUNC(0890A660, move_tick_mark_timer) { tick_after(c, pre_mark_timer, 0x0890A69Cu); }
SFA_FUNC(0893175C, move_tick_fall_next) { tick_after(c, pre_fall_next, 0x08931798u); }
SFA_FUNC(08933974, move_tick_77_hop) { tick_after(c, pre_77_hop, 0x089339B0u); }
SFA_FUNC(08949FCC, move_tick_77_198) { tick_after(c, pre_77_198, 0x0894A008u); }
SFA_FUNC(0895DBE4, move_tick_then_rise) { tick_then(c, post_77_rise, 0x0895DBF8u); }
SFA_FUNC(08995714, move_tick_then_rise_b) { tick_then(c, post_77_rise, 0x08995728u); }
SFA_FUNC(0896DF5C, move_tick_then_take) { tick_then(c, post_77_take, 0x0896DF70u); }

/* (fighter): +11 = +697 unless +599; then 0x0885F0F8. */
SFA_FUNC(0885F0C8, move_dir_then_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 599) == 0) *B(o + 11) = *B(o + 697);
    c->r[31] = 0x0885F0ECu; f_0885F0F8(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +73 = -1, +125 counted, height +22 held down to +132; then 0x0885CE18. */
SFA_FUNC(0885C374, move_land_count) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t n = S8(o + 125);
    *B(o + 73) = 0xFF;
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *B(o + 125) = (uint8_t)(n + 1);
    *W(c->r[29] + 16) = ra;
    if (!(floor < y)) *H(o + 22) = (uint16_t)floor;
    c->r[31] = 0x0885C3ACu; f_0885CE18(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; a frame flag (+77) is taken (negative: step 4) and 0x088F501C. */
SFA_FUNC(088F4E04, move_tick_then_flag) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x088F4E18u; f_08861848(c);
    int32_t k = S8(o + 77);
    if (k != 0) {
        if (k < 0) *B(o + 7) = 4;
        *B(o + 77) = 0;
        ARG(0) = o; c->r[31] = 0x088F4E3Cu; f_088F501C(c);
    }
    leave(c, sv);
}

/* (fighter): without a frame flag step 2 and +73 = 1; 0x08903954 and ticked. */
SFA_FUNC(0890381C, move_flag_then_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) { *B(o + 7) = 2; *B(o + 73) = 1; }
    ARG(0) = o; c->r[31] = 0x0890384Cu; f_08903954(c);
    ARG(0) = o; c->r[31] = 0x08903854u; f_08861848(c);
    leave(c, sv);
}

/* (fighter): 0x08908C04; +7 steps on when 0x08908BA8 is negative; ticked. */
SFA_FUNC(08906C34, move_check2_then_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x08906C48u; f_08908C04(c);
    ARG(0) = o; c->r[31] = 0x08906C50u; f_08908BA8(c);
    if ((int32_t)c->r[2] < 0) bump(o + 7);
    ARG(0) = o; c->r[31] = 0x08906C6Cu; f_08861848(c);
    leave(c, sv);
}

/* (fighter): +7 steps on, +73 cleared, +543 = 5, cue 0x22 / 0x23 (0x0887C614), ticked. */
SFA_FUNC(0890A2F8, move_step_cue_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    bump(o + 7);
    *B(o + 73) = 0; *B(o + 543) = 5;
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; c->r[31] = 0x0890A328u; f_0887C614(c);
    ARG(0) = o; c->r[31] = 0x0890A330u; f_08861848(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked until a frame flag; then +7 steps on and the mark's +3 = 10. */
SFA_FUNC(0890B958, move_tick_until_flag) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 77) == 0) { c->r[31] = 0x0890B974u; f_08861848(c); }
    else { bump(o + 7); *B(o + 0x334 + 3) = 10; }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): move 0x12 (0x18 with +78; 3 more with +193) + its +194, picked with its cancel data. */
SFA_FUNC(0890FB68, move_pick_by_flags) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t n = S8(o + 78) != 0 ? 0x18 : 0x12;
    if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
    ARG(1) = (n + (uint32_t)S8(o + 194)) & 0xFFFF;
    c->r[31] = 0x0890FBA4u; f_08861674(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): step `st`, cue 0x22 / 0x23, +20 cleared, height +22 = +132, move 0x30 (list 1). */
static void land_30(Cpu *c, uint8_t st, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 7) = st;
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; c->r[31] = back1; f_0887C614(c);
    int32_t y = S16(o + 132);
    *H(o + 20) = 0; *H(o + 22) = (uint16_t)y;
    ARG(0) = o; ARG(1) = 0x30; c->r[31] = back2; f_088616EC(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08921940, move_land_30_7) { land_30(c, 7, 0x08921960u, 0x08921978u); }
SFA_FUNC(08922AC0, move_land_30_4) { land_30(c, 4, 0x08922AE0u, 0x08922AF8u); }

/* (fighter): when the animation ends +7 steps on with move 0x23; else ticked. */
SFA_FUNC(0896B6F0, move_end_then_23) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) < 0) { bump(o + 7); ARG(1) = 0x23; c->r[31] = 0x0896B718u; f_0886163C(c); }
    else { c->r[31] = 0x0896B72Cu; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
