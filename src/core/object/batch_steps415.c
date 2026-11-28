/* A screen's wait for the player, and its way on when the mode ends. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0882B4E4(Cpu *c); void f_0882B578(Cpu *c); void f_0882AE20(Cpu *c);

/* (a, b): a's step (+4) goes on with a wait of 0x3C frames (+16), and b's +81 becomes -1. */
SFA_FUNC(0882B558, step_on_with_wait) {
    gaddr a = ARG(0), b = ARG(1);
    c->r[6] = *H(a + 4) + 1u;
    c->r[7] = 0x3C;
    *H(a + 4) = (uint16_t)c->r[6];
    *H(a + 16) = 0x3C;
    ARG(0) = 0xFFFFFFFFu;
    *B(b + 81) = 0xFF;
}

/* (a, b): the screen's wait: what the routine at 0x0882B4E4 gives is kept at b's +92. With a's
   +69 below 0 the step goes on with its wait. Else, unless a's +42 is set: in the plain game
   (a's +114 at 0) with b's +85 clear, +20 at 9, +56 at 0 and +61 clear, b's +69 is set and the
   mode's end runs; otherwise the routine at 0x0882B578. A 0x20 frame keeping s0, s1. */
SFA_FUNC(0882B394, screen_wait_step) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = b;
    *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[17] = a;
    c->r[31] = 0x0882B3B0u; f_0882B4E4(c);
    *W(b + 92) = c->r[2];
    if (S8(a + 69) < 0) {
        ARG(0) = a; ARG(1) = b; c->r[31] = 0x0882B3C8u; f_0882B558(c);
    } else if (S8(a + 42) == 0) {
        if (S8(a + 114) == 0 && S8(b + 85) == 0 && S16(b + 20) == 9 && S16(b + 56) == 0 && S8(b + 61) == 0) {
            *B(b + 69) = 1;
            ARG(0) = a; ARG(1) = b; c->r[31] = 0x0882B44Cu; f_0882AE20(c);
        } else {
            ARG(0) = a; ARG(1) = b; c->r[31] = 0x0882B434u; f_0882B578(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
