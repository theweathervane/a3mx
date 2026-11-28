/* Three more step routines: the two around the marker's setup in batch_steps387.c, and the
   frame's fade request. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088EAF40(Cpu *c); void f_088EACC8(Cpu *c); void f_088EAD20(Cpu *c); void f_089FB928(Cpu *c);
void f_089FB6A8(Cpu *c); void f_089FBA04(Cpu *c); void f_089E0BA8(Cpu *c); void f_0880B74C(Cpu *c);

/* (o): the marker is set up by its kind, and its +93 set. A 0x20 frame keeping s0. */
SFA_FUNC(088EAEB4, kind_marker_begin) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x088EAEC8u; f_088EAF40(c);
    ARG(0) = 1;
    *B(o + 93) = 1;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (o): the marker's step: its count (+128) goes up, its two routines run, and +12, +14 and +6
   are each divided by the double at 0x08A3F4B8, as whole numbers. A 0x20 frame keeping s0, s2,
   s3. */
SFA_FUNC(088EADDC, kind_marker_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = o;
    ARG(1) = *B(o + 128) + 1u;
    *W(sp + 20) = s2; *W(sp + 24) = s3; *W(sp + 28) = ra;
    *B(o + 128) = (uint8_t)ARG(1);
    c->r[31] = 0x088EAE04u; f_088EACC8(c);
    ARG(0) = o; c->r[31] = 0x088EAE0Cu; f_088EAD20(c);
    static const uint8_t scaled[3] = { 12, 14, 6 };
    uint32_t back = 0x088EAE20u;
    for (int i = 0; i < 3; i++, back += 0x2C) {
        ARG(0) = (uint32_t)S16(o + scaled[i]);
        c->r[31] = i == 0 ? 0x088EAE14u : back; f_089FB928(c);
        if (i == 0) {
            ARG(0) = 0x08A40000u;
            c->r[19] = *W(0x08A3F4BCu);
            c->r[18] = *W(0x08A3F4B8u);
        }
        ARG(1) = c->r[3]; ARG(3) = c->r[19]; ARG(2) = c->r[18]; ARG(0) = c->r[2];
        c->r[31] = back + 0x14; f_089FB6A8(c);
        ARG(1) = c->r[3]; ARG(0) = c->r[2];
        c->r[31] = back + 0x20; f_089FBA04(c);
        *H(o + scaled[i]) = (uint16_t)c->r[2];
    }
    c->r[16] = s0; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x20;
}

/* The frame's fade request: when one is waiting (the word at 0x08E75B9A), it is taken and the
   fade set from the float at 0x08E75B9C; then the frame's next routine. A 0x20 frame. */
SFA_FUNC(08804E68, frame_fade_apply) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0x08E70000u;
    ARG(1) = (uint32_t)S16(0x08E75B9Au);
    if (ARG(1) != 0) {
        *H(0x08E75B9Au) = 0;
        c->f[12] = *W(0x08E75B9Cu);
        c->r[31] = 0x08804E90u; f_089E0BA8(c);
    }
    c->r[31] = 0x08804E98u; f_0880B74C(c);
    c->r[31] = ra; c->r[29] += 0x20;
}
