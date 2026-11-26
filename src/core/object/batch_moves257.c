/* The meter gain at strength 2 picked by distance (0x0886D5D0) and two special starts that use it with
   +81, or else 0x0886D34C by the choice. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886CA2C(Cpu *c); void f_0886D5D0(Cpu *c); void f_0886DC3C(Cpu *c); void f_0886D34C(Cpu *c);

/* (fighter): strength 2 (+194 / +552); the meter gain from 0x08A1BDF8 at it when the distance (+366) is
   past its entry, else the strength picked by distance (0 under 96, 1 under 144, else 2) and its gain. */
SFA_FUNC(0886D5D0, meter_by_distance_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    const gaddr tab = 0x08A1BDF8u;
    *B(o + 552) = 2;
    int32_t i = S8(o + 552);
    *B(o + 194) = 2;
    int32_t r7 = (int16_t)(-S16(tab + (uint32_t)(i + i)));
    int32_t d = S16(o + 366);
    *W(c->r[29] + 16) = ra;
    if (r7 < d) {
        *B(o + 552) = (uint8_t)S8(o + 194);
        ARG(0) = o; ARG(1) = 0u - (uint32_t)r7; c->r[31] = 0x0886D628u; f_0886CA2C(c);
    } else {
        int32_t k = d >= 144 ? 2 : d >= 96 ? 1 : 0;
        *B(o + 194) = (uint8_t)k; *B(o + 552) = (uint8_t)k;
        ARG(0) = o; ARG(1) = (uint32_t)S16(tab + (uint32_t)(k + k)); c->r[31] = 0x0886D66Cu; f_0886CA2C(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with +81 the gain at strength 2; else with no choice made strength 2 and 0x0886DC3C, with a
   choice 0x0886D34C (`which`). */
static void start_81(Cpu *c, uint32_t which, const uint32_t b[3]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t k = S8(o + 81);
    *W(c->r[29] + 16) = ra;
    if (k != 0) { ARG(1) = (uint32_t)k; c->r[31] = b[0]; f_0886D5D0(c); }
    else if (S8(o + 386) == -1) {
        *B(o + 194) = 2; *B(o + 552) = 2;
        ARG(1) = 2; ARG(2) = 0xFFFFFFFFu; c->r[31] = b[1]; f_0886DC3C(c);
    } else { ARG(1) = which; ARG(2) = 0xFFFFFFFFu; c->r[31] = b[2]; f_0886D34C(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(0886D2EC, start_81_0) { static const uint32_t b[3] = { 0x0886D308u, 0x0886D32Cu, 0x0886D340u }; start_81(c, 0, b); }
SFA_FUNC(0886D3E0, start_81_1) { static const uint32_t b[3] = { 0x0886D3FCu, 0x0886D420u, 0x0886D434u }; start_81(c, 1, b); }
