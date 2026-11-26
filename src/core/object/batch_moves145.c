/* Move steps of 9 - 19 instructions: the common "back to standing when the animation ends, else tick"
   step, landings, and moves started on frame flags or timers. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0885C374(Cpu *c);
void f_089A04D8(Cpu *c); void f_0886731C(Cpu *c); void f_0891CDFC(Cpu *c); void f_08837098(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

static void framed(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back; fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
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

/* (fighter): when the animation has ended (+75 < 0) back to standing (step 1 / 0 / 0 / 0, move 0);
   else ticked. */
static void end_or_tick(Cpu *c, uint32_t back_stand, uint32_t back_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        ARG(1) = 0; c->r[31] = back_stand; f_0886163C(c);
    } else { c->r[31] = back_tick; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088EDD28, move_end_or_tick) { end_or_tick(c, 0x088EDD54u, 0x088EDD68u); }
SFA_FUNC(089002D4, move_end_or_tick_b) { end_or_tick(c, 0x08900314u, 0x089002F0u); }
SFA_FUNC(08905788, move_end_or_tick_c) { end_or_tick(c, 0x089057B4u, 0x089057C8u); }
SFA_FUNC(0891FB6C, move_end_or_tick_d) { end_or_tick(c, 0x0891FB98u, 0x0891FBACu); }
SFA_FUNC(08924BDC, move_end_or_tick_e) { end_or_tick(c, 0x08924C1Cu, 0x08924BF8u); }
SFA_FUNC(0892F764, move_end_or_tick_f) { end_or_tick(c, 0x0892F790u, 0x0892F7A4u); }
SFA_FUNC(08933A54, move_end_or_tick_g) { end_or_tick(c, 0x08933A94u, 0x08933A70u); }

/* (fighter): landed with kind `k` (+116), +631 cleared (0x0885C374). */
static void land(Cpu *c, uint8_t k, uint32_t back) {
    *B(ARG(0) + 116) = k; *B(ARG(0) + 631) = 0;
    framed(c, f_0885C374, back);
}
SFA_FUNC(0885C350, move_land_3) { land(c, 3, 0x0885C368u); }
SFA_FUNC(0885C7A8, move_land_5) { land(c, 5, 0x0885C7C0u); }
/* As above with kind 4, +270 = 1, +656 = 3, +127 = 0x30. */
SFA_FUNC(0885C81C, move_land_4) {
    gaddr o = ARG(0);
    *B(o + 116) = 4; *B(o + 631) = 0; *B(o + 270) = 1; *B(o + 656) = 3; *B(o + 127) = 0x30;
    framed(c, f_0885C374, 0x0885C84Cu);
}

static void pre_82_fast(gaddr o) {
    *H(o + 82) = (uint16_t)(S16(o + 82) - 0x100);
    if (S16(o + 82) < 0) { bump(o + 7); *B(o + 74) = 1; }
}
static void pre_82_skip2(gaddr o) {
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) { *B(o + 7) = (uint8_t)(*B(o + 7) + 2); *B(o + 237) = 0; *H(o + 82) = 4; }
}
static void pre_77_jump(gaddr o) {
    if (S8(o + 77) != 0) { bump(o + 7); *B(o + 73) = 1; *W(o + 96) = 0xE0000; *W(o + 104) = 0xFFFFA000u; }
}
SFA_FUNC(0899C504, move_tick_82_fast) { tick_after(c, pre_82_fast, 0x0899C540u); }
SFA_FUNC(088F9FCC, move_tick_82_skip2) { tick_after(c, pre_82_skip2, 0x088FA00Cu); }
SFA_FUNC(089114AC, move_tick_77_jump) { tick_after(c, pre_77_jump, 0x089114ECu); }

/* (fighter): step 4, the mark's +5 cleared, 0x089A04D8, ticked, 0x089A04D8. */
SFA_FUNC(089A0308, move_mark_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 7) = 4;
    *B(o + 0x334 + 5) = 0;
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; c->r[31] = 0x089A0330u; f_089A04D8(c);
    ARG(0) = o; c->r[31] = 0x089A0338u; f_08861848(c);
    ARG(0) = o; c->r[31] = 0x089A0340u; f_089A04D8(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): step 1 / 0 / 1 / 0, +696 cleared, 0x0886731C, move 7. */
SFA_FUNC(088623E4, move_reset_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 1; *B(o + 7) = 0; *B(o + 696) = 0;
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; c->r[31] = 0x08862414u; f_0886731C(c);
    ARG(0) = o; ARG(1) = 7; c->r[31] = 0x08862420u; f_0886163C(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): when the animation ends step 2 with move 0x19 + its +194 (list 1); ticked either way. */
SFA_FUNC(088F9C60, move_end_then_19) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e < 0) {
        int32_t k = S8(o + 194);
        *B(o + 7) = 2;
        ARG(0) = o; ARG(1) = (uint32_t)(k + 0x19) & 0xFFFF;
        c->r[31] = 0x088F9C94u; f_088616EC(c);
    }
    ARG(0) = o; c->r[31] = 0x088F9C9Cu; f_08861848(c);
    leave(c, sv);
}

/* (fighter): ticked until frame flag bit 0; then +7 steps on, +73 = 1, and ticked. */
SFA_FUNC(08907388, move_tick_flag1) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if ((S8(o + 77) & 1) == 0) { c->r[31] = 0x089073A8u; f_08861848(c); }
    else { bump(o + 7); *B(o + 73) = 1; c->r[31] = 0x089073C8u; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): frame flag bits 0 - 3: +7 steps on and 0x0891CDFC; then ticked. */
SFA_FUNC(0891B350, move_flag_helper_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k & 0xF) { bump(o + 7); ARG(0) = o; c->r[31] = 0x0891B384u; f_0891CDFC(c); }
    ARG(0) = o; c->r[31] = 0x0891B38Cu; f_08861848(c);
    leave(c, sv);
}

/* (fighter): +656 cleared, +7 steps on, +0x3AA cleared, 0x08837098, move 8 (list 1). */
SFA_FUNC(08921BF0, move_step_8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    *B(o + 656) = 0;
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 0x3A0 + 10) = 0;
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; c->r[31] = 0x08921C20u; f_08837098(c);
    ARG(0) = o; ARG(1) = 8; c->r[31] = 0x08921C2Cu; f_088616EC(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (part): +7 steps on; move 0x32, or 0x22 when its fighter (+92) is in a state (+338). */
SFA_FUNC(08924B90, part_step_by_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr f = *W(o + 92);
    bump(o + 7);
    int32_t s = S8(f + 338);
    *W(c->r[29] + 16) = ra;
    if (s == 0) { ARG(1) = 0x32; c->r[31] = 0x08924BBCu; }
    else { ARG(1) = 0x22; c->r[31] = 0x08924BD0u; }
    f_0886163C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
