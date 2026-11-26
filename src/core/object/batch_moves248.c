/* A leap that hands over to 0x08921FD8 and a special that calls the effect spawner once per frame flag.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861848(Cpu *c); void f_08921FD8(Cpu *c); void f_0886D468(Cpu *c);
void f_088370D8(Cpu *c); void f_0891F508(Cpu *c); void f_0886469C(Cpu *c); void f_0887FD2C(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter): +656 cleared, ticked; with +77 below zero +7 on, in the air (+73 = 1), +123 cleared, x 9.0
   toward its facing with the rest still, and move strength + 0x29; else 0x08921FD8. */
SFA_FUNC(08921F40, leap_to_fd8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    *B(o + 656) = 0;
    call1(c, o, f_08861848, 0x08921F58u);
    if (S8(o + 77) < 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 73) = 1; *B(o + 123) = 0;
        *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 108) = 0;
        *W(o + 96) = *B(o + 11) != 0 ? 0x90000u : 0xFFF70000u;
        call2(c, o, (uint32_t)(S8(o + 194) + 0x29) & 0xFFFF, f_088616EC, 0x08921FB8u);
    } else call1(c, o, f_08921FD8, 0x08921FC8u);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 0: step 1, the gain by distance, 0x088370D8, 0x0891F508 and move 0x24; step 1: ticked;
   without a frame flag +237 cleared, +7 on and standing; on a flag other than 2 it becomes 2, +237
   toward +81 and the effect at (0xE, 0x50). */
SFA_FUNC(0891B39C, fx_once_e) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t st = *B(o + 7);
    c->r[17] = 1;
    if (st == 0) {
        *B(o + 7) = 1;
        call1(c, o, f_0886D468, 0x0891B3CCu);
        call1(c, o, f_088370D8, 0x0891B3D4u);
        call1(c, o, f_0891F508, 0x0891B3DCu);
        call2(c, o, 0x24, f_088616EC, 0x0891B3E8u);
    } else if (st == 1) {
        call1(c, o, f_08861848, 0x0891B400u);
        int32_t k = S8(o + 77);
        if (k == 0) {
            *B(o + 237) = 0;
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call1(c, o, f_0886469C, 0x0891B424u);
        } else if (k != 2) {
            *B(o + 77) = 2;
            *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF;
            ARG(0) = o; ARG(1) = 0xE; ARG(2) = 0x50; c->r[31] = 0x0891B464u; f_0887FD2C(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
