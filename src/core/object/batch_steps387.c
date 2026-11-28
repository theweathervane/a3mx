/* One more step routine: an object set up by its kind, with its places scaled by the constant
   the game keeps for that. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_089FB928(Cpu *c); void f_089FB6A8(Cpu *c); void f_089FBA04(Cpu *c);

/* (o): by the object's kind (+92): its thirteen shades (+136 and the three sets of four from
   +36, +42 and +48), its two places (+14 and +26 from the table at 0x08A3F440, 16 bytes a kind;
   +6 and +10 from the one at 0x08A3F4A0, 4 bytes a kind) and its state (+56). The kind the
   battle has chosen (its +159) is full bright, 8 further out and 4 nearer, in state 1; any
   other is dimmer, in state 2. Then +12, +14 and +6 are each divided by the double at
   0x08A3F4B8, as whole numbers. A 0x10 frame keeping s0, s2, s3. */
SFA_FUNC(088EAF40, kind_marker_setup) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = s0;
    c->r[16] = o;
    uint32_t kind = *B(o + 92);
    int32_t chosen = S8(BATTLE + 159);
    gaddr outer = 0x08A3F440u + kind * 16;
    gaddr inner = 0x08A3F4A0u + kind * 4;
    *W(sp + 8) = s3;
    c->r[19] = *W(0x08A3F4BCu);
    *W(sp + 4) = s2;
    *W(sp + 12) = ra;
    c->r[18] = *W(0x08A3F4B8u);
    int is_chosen = (uint32_t)chosen == kind;
    uint8_t shade = is_chosen ? 0xFF : 0x80;
    static const uint8_t shades[12] = { 51, 50, 49, 48, 45, 44, 43, 42, 39, 38, 37, 36 };
    *B(o + 136) = is_chosen ? 0xFF : 0xC0;
    for (int i = 0; i < 12; i++) *B(o + shades[i]) = shade;
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    uint32_t out = is_chosen ? 8 : 0, in = is_chosen ? 0xFFFFFFFCu : 0;
    *H(o + 14) = (uint16_t)(*H(outer) + out);
    *H(o + 26) = (uint16_t)(*H(outer + 2) + out);
    *H(o + 6) = (uint16_t)(*H(inner) + in);
    *H(o + 10) = (uint16_t)(*H(inner + 2) + in);
    *B(o + 56) = is_chosen ? 1 : 2;
    uint32_t back = is_chosen ? 0x088EB018u : 0x088EB110u;
    static const uint8_t scaled[3] = { 12, 14, 6 };
    for (int i = 0; i < 3; i++, back += 0x2C) {
        ARG(0) = (uint32_t)S16(o + scaled[i]);
        c->r[31] = back; f_089FB928(c);
        ARG(1) = c->r[3]; ARG(3) = c->r[19]; ARG(0) = c->r[2]; ARG(2) = c->r[18];
        c->r[31] = back + 0x14; f_089FB6A8(c);
        ARG(1) = c->r[3]; ARG(0) = c->r[2];
        c->r[31] = back + 0x20; f_089FBA04(c);
        *H(o + scaled[i]) = (uint16_t)c->r[2];
    }
    c->r[16] = s0; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x10;
}
