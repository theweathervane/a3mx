/* Characters' attack steps that are the shared one (0x0886D678) under their own address, and three with
   a step of their own first. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886D678(Cpu *c); void f_089B0554(Cpu *c); void f_08983758(Cpu *c);

/* a 0x20 frame keeping ra around one call */
static void via(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back; fn(c);
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(088EE4F8, attack_step_e4f8) { via(c, f_0886D678, 0x088EE508u); }
SFA_FUNC(088EE758, attack_step_e758) { via(c, f_0886D678, 0x088EE768u); }
SFA_FUNC(088EE90C, attack_step_e90c) { via(c, f_0886D678, 0x088EE91Cu); }
SFA_FUNC(0890F64C, attack_step_f64c) { via(c, f_0886D678, 0x0890F65Cu); }
SFA_FUNC(0890F754, attack_step_f754) { via(c, f_0886D678, 0x0890F764u); }
SFA_FUNC(0890F8D4, attack_step_f8d4) { via(c, f_0886D678, 0x0890F8E4u); }
SFA_FUNC(089147D8, attack_step_47d8) { via(c, f_0886D678, 0x089147E8u); }
SFA_FUNC(08925954, attack_step_5954) { via(c, f_0886D678, 0x08925964u); }
SFA_FUNC(0896709C, attack_step_709c) { via(c, f_0886D678, 0x089670ACu); }
SFA_FUNC(0897B1A4, attack_step_b1a4) { via(c, f_0886D678, 0x0897B1B4u); }
SFA_FUNC(0899E544, attack_step_e544) { via(c, f_0886D678, 0x0899E554u); }
SFA_FUNC(089A2C7C, attack_step_2c7c) { via(c, f_0886D678, 0x089A2C8Cu); }
SFA_FUNC(089B88D4, attack_step_88d4) { via(c, f_0886D678, 0x089B88E4u); }

/* (fighter): +696 cleared; step 0 is 0x089B0554, later the attack step. */
SFA_FUNC(089B0518, attack_or_0554) {
    gaddr o = ARG(0);
    uint32_t st = *B(o + 7);
    *B(o + 696) = 0;
    if (st == 0) via(c, f_089B0554, 0x089B0534u); else via(c, f_0886D678, 0x089B0548u);
}

/* (fighter): step 0 is 0x08983758, step 1 the attack step. */
SFA_FUNC(08983714, attack_or_3758) {
    uint32_t st = *B(ARG(0) + 7);
    if (st == 0) via(c, f_08983758, 0x08983730u);
    else {
        uint32_t ra = c->r[31];
        c->r[29] -= 0x20;
        *W(c->r[29] + 16) = ra;
        if (st == 1) { c->r[31] = 0x0898374Cu; f_0886D678(c); }
        c->r[31] = ra; c->r[29] += 0x20;
    }
}

/* (fighter): on frame flag bit 0 (+76) 3 toward its facing, then the attack step. */
SFA_FUNC(08942AD8, attack_nudge) {
    gaddr o = ARG(0);
    if (S8(o + 76) & 1) *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) != 0 ? 3 : -3));
    via(c, f_0886D678, 0x08942B18u);
}