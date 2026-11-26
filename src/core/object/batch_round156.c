/* The commonest move step: when the animation has ended the move is over and the fighter starts again
   from standing (0x0886469C), else the animation ticks. 21 copies, one per move; with four relatives
   that use the part animation or a timer. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_088D3EC0(Cpu *c);

static void framed_call(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back; fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): standing again once the animation has ended (+75 < 0), else ticked. */
static void end_round_or_tick(Cpu *c, uint32_t back_end, uint32_t back_tick) {
    if (S8(ARG(0) + 75) < 0) framed_call(c, f_0886469C, back_end);
    else framed_call(c, f_08861848, back_tick);
}
SFA_FUNC(088F08FC, move_end_round_or_tick_08fc) { end_round_or_tick(c, 0x088F0918u, 0x088F092Cu); }
SFA_FUNC(088F1108, move_end_round_or_tick_1108) { end_round_or_tick(c, 0x088F1124u, 0x088F1138u); }
SFA_FUNC(088FA2C4, move_end_round_or_tick_a2c4) { end_round_or_tick(c, 0x088FA2E0u, 0x088FA2F4u); }
SFA_FUNC(088FE1B4, move_end_round_or_tick_e1b4) { end_round_or_tick(c, 0x088FE1D0u, 0x088FE1E4u); }
SFA_FUNC(0890C800, move_end_round_or_tick_c800) { end_round_or_tick(c, 0x0890C830u, 0x0890C81Cu); }
SFA_FUNC(089221A0, move_end_round_or_tick_21a0) { end_round_or_tick(c, 0x089221D0u, 0x089221BCu); }
SFA_FUNC(0892BCC8, move_end_round_or_tick_bcc8) { end_round_or_tick(c, 0x0892BCE4u, 0x0892BCF8u); }
SFA_FUNC(0892C538, move_end_round_or_tick_c538) { end_round_or_tick(c, 0x0892C554u, 0x0892C568u); }
SFA_FUNC(08931468, move_end_round_or_tick_1468) { end_round_or_tick(c, 0x08931484u, 0x08931498u); }
SFA_FUNC(0893675C, move_end_round_or_tick_675c) { end_round_or_tick(c, 0x08936778u, 0x0893678Cu); }
SFA_FUNC(0893E9D0, move_end_round_or_tick_e9d0) { end_round_or_tick(c, 0x0893E9ECu, 0x0893EA00u); }
SFA_FUNC(0895EB90, move_end_round_or_tick_eb90) { end_round_or_tick(c, 0x0895EBACu, 0x0895EBC0u); }
SFA_FUNC(08965358, move_end_round_or_tick_5358) { end_round_or_tick(c, 0x08965374u, 0x08965388u); }
SFA_FUNC(08969218, move_end_round_or_tick_9218) { end_round_or_tick(c, 0x08969234u, 0x08969248u); }
SFA_FUNC(08969370, move_end_round_or_tick_9370) { end_round_or_tick(c, 0x0896938Cu, 0x089693A0u); }
SFA_FUNC(089698BC, move_end_round_or_tick_98bc) { end_round_or_tick(c, 0x089698D8u, 0x089698ECu); }
SFA_FUNC(0899C54C, move_end_round_or_tick_c54c) { end_round_or_tick(c, 0x0899C57Cu, 0x0899C568u); }
SFA_FUNC(089A4734, move_end_round_or_tick_4734) { end_round_or_tick(c, 0x089A4750u, 0x089A4764u); }
SFA_FUNC(089A4A80, move_end_round_or_tick_4a80) { end_round_or_tick(c, 0x089A4A9Cu, 0x089A4AB0u); }
SFA_FUNC(089A70BC, move_end_round_or_tick_70bc) { end_round_or_tick(c, 0x089A70D8u, 0x089A70ECu); }
SFA_FUNC(089B18F8, move_end_round_or_tick_18f8) { end_round_or_tick(c, 0x089B1914u, 0x089B1928u); }

/* (character): the part animation's version. */
SFA_FUNC(088D3A80, part_end_round_or_tick) {
    if (S8(ARG(0) + 75) < 0) framed_call(c, f_0886469C, 0x088D3A9Cu);
    else framed_call(c, f_088D3EC0, 0x088D3AB0u);
}

/* (character): the part ticked, then standing again once it has ended. */
SFA_FUNC(089C9010, part_tick_then_round) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x089C9024u; f_088D3EC0(c);
    if (S8(o + 75) < 0) { ARG(0) = o; c->r[31] = 0x089C9038u; f_0886469C(c); }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): as the common step, with the mark's +4 cleared when it ends. */
SFA_FUNC(0891B018, move_end_round_or_tick_mark) {
    if (S8(ARG(0) + 75) < 0) { *B(ARG(0) + 0x334 + 4) = 0; framed_call(c, f_0886469C, 0x0891B034u); }
    else framed_call(c, f_08861848, 0x0891B048u);
}

/* (fighter): the +82 wait runs down; past 0 standing again. */
SFA_FUNC(08968638, move_wait_then_round) {
    gaddr o = ARG(0);
    int8_t t = (int8_t)(S8(o + 0x52) - 1);
    *B(o + 0x52) = (uint8_t)t;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (t < 0) { c->r[31] = 0x08968668u; f_0886469C(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}