/* Small move steps, 15 - 16 instructions: the animation ticked with the step moved on by the frame's
   +77 / +76 / +75 or a timer, and moves picked with a height. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088D3DD8(Cpu *c); void f_089452F8(Cpu *c);
void f_08908BD8(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

static void tick_after(Cpu *c, void (*pre)(gaddr), void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    pre(ARG(0));
    c->r[31] = back; fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
static void pre_77_next(gaddr o) { if (S8(o + 77) == 0) { bump(o + 7); *B(o + 73) = 1; } }
static void pre_77_set2_clear(gaddr o) {
    if (S8(o + 77) == 0) { *B(o + 7) = 2; *B(o + 73) = 1; *B(o + 573) = 0; *B(o + 656) = 0; }
}
static void pre_77_take_next(gaddr o) { if (S8(o + 77) != 0) { bump(o + 7); *B(o + 77) = 0; *B(o + 73) = 1; } }
static void pre_76_next(gaddr o) { if (S8(o + 76) >= 0) { bump(o + 7); *B(o + 73) = 1; *B(o + 76) = 0; } }
static void pre_82_timer(gaddr o) {
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) bump(o + 7);
}
static void pre_75_part(gaddr o) { if (S8(o + 75) < 0) { bump(o + 5); *B(o + 1) = 0; *B(o + 141) = 1; } }

SFA_FUNC(088EF5B4, move_tick_77_next) { tick_after(c, pre_77_next, f_08861848, 0x088EF5E4u); }
SFA_FUNC(0892676C, move_tick_77_next_b) { tick_after(c, pre_77_next, f_08861848, 0x0892679Cu); }
SFA_FUNC(08963720, move_tick_77_next_c) { tick_after(c, pre_77_next, f_08861848, 0x08963750u); }
SFA_FUNC(08968B34, move_tick_77_next_d) { tick_after(c, pre_77_next, f_08861848, 0x08968B64u); }
SFA_FUNC(0892B000, move_tick_77_set2) { tick_after(c, pre_77_set2_clear, f_08861848, 0x0892B030u); }
SFA_FUNC(0899FA50, move_tick_77_take_next) { tick_after(c, pre_77_take_next, f_08861848, 0x0899FA84u); }
SFA_FUNC(089A4290, move_tick_76_next) { tick_after(c, pre_76_next, f_08861848, 0x089A42C4u); }
SFA_FUNC(089650DC, move_tick_timer) { tick_after(c, pre_82_timer, f_08861848, 0x08965110u); }
SFA_FUNC(089CC494, part_tick_75_next5) { tick_after(c, pre_75_part, f_088D3DD8, 0x089CC4C8u); }

/* (fighter): +6 steps on, height +22 = +132 + 240, +1 = 1, move 0x22. */
SFA_FUNC(08933938, move_drop_in) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t y = S16(o + 132);
    bump(o + 6);
    *H(o + 22) = (uint16_t)(y + 0xF0);
    *B(o + 1) = 1;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 0x22; c->r[31] = 0x08933968u; f_0886163C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): step 7, +1 = 1, height +22 = +132, +82 = 6, move 0x22. */
SFA_FUNC(089897BC, move_land) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 7) = 7;
    int32_t y = S16(o + 132);
    *B(o + 1) = 1;
    *H(o + 22) = (uint16_t)y;
    *B(o + 0x52) = 6;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 0x22; c->r[31] = 0x089897F0u; f_0886163C(c);
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

/* (fighter): 0x089452F8, +7 = 8 with +77, then ticked. */
SFA_FUNC(089452BC, move_then_tick_8) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x089452D0u; f_089452F8(c);
    if (S8(o + 77) != 0) *B(o + 7) = 8;
    ARG(0) = o; c->r[31] = 0x089452E8u; f_08861848(c);
    leave(c, sv);
}

/* (fighter): ticked, then +7 steps on unless +77. */
SFA_FUNC(089B20E0, move_tick_then_next) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x089B20F4u; f_08861848(c);
    if (S8(o + 77) == 0) bump(o + 7);
    leave(c, sv);
}

/* (fighter): +7 steps on when 0x08908BD8 is negative, then ticked. */
SFA_FUNC(08906A18, move_check_then_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    c->r[31] = 0x08906A2Cu; f_08908BD8(c);
    if ((int32_t)c->r[2] < 0) bump(o + 7);
    ARG(0) = o; c->r[31] = 0x08906A48u; f_08861848(c);
    leave(c, sv);
}
