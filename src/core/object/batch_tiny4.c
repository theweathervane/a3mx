/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, made ready
   by batch_tiny3.c: wrappers that step a counter or take a value first, an owner's count of its
   objects (+536) going down as one goes, and objects moved on both axes by their speeds. Offsets stay
   numbers until something shows what they are.
 */
#include "../decomp.h"
#include "../guest_mem.h"

void f_0890DAC8(Cpu *c); void f_0891E914(Cpu *c); void f_0893B460(Cpu *c); void f_088D3B64(Cpu *c);
void f_089CB4E0(Cpu *c); void f_088230F4(Cpu *c);

/* One call inside a 0x20 frame with the return address kept at +16, as the originals do. */
static void wrap(Cpu *c, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
static uint8_t inc8(gaddr a) { uint8_t v = (uint8_t)(*B(a) + 1); *B(a) = v; return v; }

/* (object): animation +3 + 0x11 of the table at 0x08A53DF0 (0x0890DAC8). */
SFA_FUNC(0890DAA8, animation_3_plus_11) { ARG(1) = *B(ARG(0) + 3) + 0x11u; wrap(c, 0x0890DABCu, f_0890DAC8); }
/* (object): +9 = 1, then animation 0 of the table at 0x08A6129C (0x0891E914). */
SFA_FUNC(0891E0C0, set_9_animation_0) { *B(ARG(0) + 9) = 1; ARG(1) = 0; wrap(c, 0x0891E0D8u, f_0891E914); }
/* (object): +14 = its owner's (+88), then the animation by +78 (0x0893B460) with it in a1. */
SFA_FUNC(0893AAD0, take_14_animation) {
    uint8_t v = *B(*W(ARG(0) + 88) + 14);
    ARG(1) = v;
    *B(ARG(0) + 14) = v;
    wrap(c, 0x0893AAE8u, f_0893B460);
}
/* (object): +5 on, then the animation by +78 (0x0893B460) with the new +5 in a1. */
SFA_FUNC(0893ACFC, step_on_5_animation) { ARG(1) = inc8(ARG(0) + 5); wrap(c, 0x0893AD14u, f_0893B460); }
/* (object, n): animation n of the table at 0x08AAE47C (0x088D3B64). */
SFA_FUNC(089C47B0, animation_08AAE47C) { ARG(2) = ARG(1); ARG(1) = 0x08AAE47Cu; wrap(c, 0x089C47C8u, f_088D3B64); }
/* (object): +4 on, then unlinked from its owner's +44 and freed (0x089CB4E0), the new +4 in a1. */
SFA_FUNC(089CB4BC, step_on_4_unlink_44) { ARG(1) = inc8(ARG(0) + 4); wrap(c, 0x089CB4D4u, f_089CB4E0); }

/* (object): its owner's (+88) count +536 down by one, the owner's +228 cleared when it reaches 0,
   then 0x088230F4 (object, owner, the count). */
static void owner_536_down(Cpu *c, uint32_t back) {
    gaddr w = *W(ARG(0) + 88);
    uint8_t n = (uint8_t)(*B(w + 536) - 1);
    *B(w + 536) = n;
    if (n == 0) *W(w + 228) = 0;
    ARG(1) = w; ARG(2) = n;
    wrap(c, back, f_088230F4);
}
SFA_FUNC(088F5AE0, owner_536_down_a) { owner_536_down(c, 0x088F5B0Cu); }
SFA_FUNC(08904E70, owner_536_down_b) { owner_536_down(c, 0x08904E9Cu); }
SFA_FUNC(0891CCB4, owner_536_down_c) { owner_536_down(c, 0x0891CCE0u); }
SFA_FUNC(08923AB0, owner_536_down_d) { owner_536_down(c, 0x08923ADCu); }
SFA_FUNC(089289F8, owner_536_down_e) { owner_536_down(c, 0x08928A24u); }
SFA_FUNC(08933318, owner_536_down_f) { owner_536_down(c, 0x08933344u); }
SFA_FUNC(08940EA0, owner_536_down_g) { owner_536_down(c, 0x08940ECCu); }
SFA_FUNC(08965F20, owner_536_down_h) { owner_536_down(c, 0x08965F4Cu); }

/* (object): x (+16) and y (+20) moved by their speeds (+96 / +100), which change by +104 / +108. */
static void move_xy(gaddr o) {
    uint32_t vx = *W(o + 96), vy = *W(o + 100), ax = *W(o + 104), ay = *W(o + 108);
    *W(o + 16) += vx; *W(o + 20) += vy;
    *W(o + 96) = vx + ax; *W(o + 100) = vy + ay;
}
SFA_FUNC(089134A0, move_xy_a) { move_xy(ARG(0)); }
SFA_FUNC(0891B9E0, move_xy_b) { move_xy(ARG(0)); }
SFA_FUNC(0891E940, move_xy_c) { move_xy(ARG(0)); }
SFA_FUNC(08923864, move_xy_d) { move_xy(ARG(0)); }
SFA_FUNC(089376A8, move_xy_e) { move_xy(ARG(0)); }
SFA_FUNC(0893A724, move_xy_f) { move_xy(ARG(0)); }
SFA_FUNC(08971D6C, move_xy_g) { move_xy(ARG(0)); }
SFA_FUNC(08981AE4, move_xy_h) { move_xy(ARG(0)); }
