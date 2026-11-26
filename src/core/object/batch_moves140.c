/* Small move steps, 7 - 11 instructions: the animation ticked or a move picked as a character's move
   moves on. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861BE8(Cpu *c); void f_08861674(Cpu *c); void f_08861A38(Cpu *c);
void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c); void f_089A04D8(Cpu *c);

/* A call made through a 0x20 frame keeping only the return address. */
static void framed(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back; fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): the step (+7) moves on and move n of list `fn` is picked. */
static void step_pick(Cpu *c, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    *B(ARG(0) + 7) = (uint8_t)(*B(ARG(0) + 7) + 1);
    ARG(1) = n;
    framed(c, fn, back);
}

SFA_FUNC(0885F0F8, move_tick) { framed(c, f_08861848, 0x0885F108u); }
SFA_FUNC(088D3A64, move_pick_c) { framed(c, f_08861674, 0x088D3A74u); }

/* (task): its fighter's (+92) frame timer ticked / move picked. */
SFA_FUNC(08861BCC, owner_anim_tick) { ARG(0) = *W(ARG(0) + 92); framed(c, f_08861BE8, 0x08861BDCu); }
SFA_FUNC(08861A14, owner_anim_from_list) {
    ARG(0) = *W(ARG(0) + 92); ARG(1) &= 0xFFFF; ARG(2) &= 0xFFFF;
    framed(c, f_08861A38, 0x08861A2Cu);
}

/* (fighter): ticked only with +305 / unless +77 is 2. */
static void tick_if(Cpu *c, int go, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (go) { c->r[31] = back; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088619EC, move_tick_if_305) { tick_if(c, *B(ARG(0) + 305) != 0, 0x08861A08u); }
SFA_FUNC(08908B7C, move_tick_unless_77_2) { tick_if(c, S8(ARG(0) + 77) != 2, 0x08908B9Cu); }

SFA_FUNC(088F0FD8, move_step_b02) { step_pick(c, 0x02, f_088616EC, 0x088F0FF4u); }
SFA_FUNC(0890C6F0, move_step_b2f) { step_pick(c, 0x2F, f_088616EC, 0x0890C70Cu); }
SFA_FUNC(0896E5A8, move_step_b24) { step_pick(c, 0x24, f_088616EC, 0x0896E5C4u); }
SFA_FUNC(08998278, move_step_b2d) { step_pick(c, 0x2D, f_088616EC, 0x08998294u); }
SFA_FUNC(089B4458, move_step_a22) { step_pick(c, 0x22, f_0886163C, 0x089B4474u); }
SFA_FUNC(089B84BC, move_step_a22_b) { step_pick(c, 0x22, f_0886163C, 0x089B84D8u); }

/* (fighter): step 4, +266 cleared, move 0x1C. */
SFA_FUNC(0886425C, move_set_1c) {
    *B(ARG(0) + 7) = 4; *B(ARG(0) + 266) = 0;
    ARG(1) = 0x1C; framed(c, f_0886163C, 0x08864278u);
}
/* (fighter): step 2, +1 = 1, move 0x22. */
SFA_FUNC(0890575C, move_set_22) {
    *B(ARG(0) + 7) = 2; *B(ARG(0) + 1) = 1;
    ARG(1) = 0x22; framed(c, f_0886163C, 0x0890577Cu);
}
/* (fighter): step 5, +82 = 120, move 0x35. */
SFA_FUNC(0892F7B0, move_set_35) {
    *B(ARG(0) + 7) = 5; *H(ARG(0) + 82) = 0x78;
    ARG(1) = 0x35; framed(c, f_0886163C, 0x0892F7D0u);
}

/* (fighter): the move of list 2 for its +117 (table 0x08A18F3C). */
SFA_FUNC(0885CE18, move_by_117) {
    ARG(1) = *B(0x08A18F3Cu + *B(ARG(0) + 117));
    framed(c, f_08861724, 0x0885CE38u);
}

/* (fighter): ticked, then 0x089A04D8. */
SFA_FUNC(089A0194, move_tick_then) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x089A01A8u; f_08861848(c);
    ARG(0) = o; c->r[31] = 0x089A01B0u; f_089A04D8(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
