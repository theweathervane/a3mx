/* The network's disconnect, a step each frame (104 instructions): the game leaves the link, the
   worker and sockets stop, the adhoc layer is left and waited on, then the link state is torn down.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0880A6CC(Cpu *c); void f_088DC0E0(Cpu *c); void f_088DC42C(Cpu *c); void f_088DCD40(Cpu *c);
void f_088DECF4(Cpu *c); void f_088DEDE4(Cpu *c); void f_088DF778(Cpu *c); void f_088E0C48(Cpu *c);
void f_088E1444(Cpu *c); void f_088E35B8(Cpu *c); void f_088E3ED8(Cpu *c);

static void next_step(gaddr o) { *H(o) = (uint16_t)(*H(o) + 1); }

/* (net state): by step (+0): 0 the game leaves the link (0x088E35B8) and 0x088E3ED8 (6); 1 the worker and
   every socket stopped; 2 once 0x088DECF4 succeeds a 300-frame timer (+4) starts; 3 the timer counts down
   while 0x088DEDE4 still reports something; 4 waits for the timer to drop below 121; 5 the session and
   peers torn down, +2 / +47 cleared and the generation (+24) counted; past that everything is reset
   (+20 = 0x08A3EB14). A block at +8 is released each time (0x0880A6CC 0x08A0104C, 2, 0x98, 4, 9). */
SFA_FUNC(088D7614, net_disconnect_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    uint32_t k = *H(o);
    *W(c->r[29] + 36) = ra;
    ARG(0) = k;
    switch (k) {
    case 0:
        c->r[31] = 0x088D7668u; f_088E35B8(c);
        ARG(0) = 6; c->r[31] = 0x088D7670u; f_088E3ED8(c);
        next_step(o);
        break;
    case 1:
        c->r[31] = 0x088D768Cu; f_088E1444(c);
        c->r[31] = 0x088D7694u; f_088E0C48(c);
        c->r[31] = 0x088D769Cu; f_088DF778(c);
        next_step(o);
        break;
    case 2:
        c->r[31] = 0x088D76B8u; f_088DECF4(c);
        if (c->r[2] == 0) { *H(o + 4) = 0x12C; next_step(o); }
        break;
    case 3: {
        int32_t t = S16(o + 4);
        ARG(0) = (uint32_t)(t - 1);
        if (t > 0) *H(o + 4) = (uint16_t)(t - 1);
        c->r[31] = 0x088D76F0u; f_088DEDE4(c);
        if ((int32_t)c->r[2] <= 0) next_step(o);
        break;
    }
    case 4: {
        int32_t t = S16(o + 4);
        if (t < 121) next_step(o);
        else *H(o + 4) = (uint16_t)(t - 1);
        break;
    }
    case 5: {
        c->r[31] = 0x088D7738u; f_088DCD40(c);
        c->r[31] = 0x088D7740u; f_088DC42C(c);
        c->r[31] = 0x088D7748u; f_088DC0E0(c);
        uint32_t g = *H(o + 24);
        *H(o + 2) = 0;
        *H(o + 24) = (uint16_t)(g + 1);
        *H(o) = 0;
        *B(o + 47) = 0;
        break;
    }
    default:
        *W(o + 20) = 0x08A3EB14u;
        *H(o + 24) = 0; *H(o + 2) = 0; *H(o) = 0;
        *B(o + 47) = 0;
        break;
    }
    int has = *W(o + 8) != 0;
    ARG(0) = 0x08A00000u;
    if (has) {
        ARG(0) = 0x08A0104Cu; ARG(1) = 2; ARG(2) = 0x98; ARG(3) = 4; c->r[8] = 9;
        c->r[31] = 0x088D77A4u; f_0880A6CC(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
