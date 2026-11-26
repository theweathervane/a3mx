/* Small network functions the link scenarios reach, 9 - 17 instructions: the link's worker stopped by
   clearing its busy flags, every socket closed, the worker started once. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E1D88(Cpu *c); void f_088DD114(Cpu *c); void f_088DF754(Cpu *c); void f_088E0C6C(Cpu *c);
void f_088E1BC8(Cpu *c);

static void stop_with(Cpu *c, uint32_t flag, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0x095236C0u;
    *B(0x095236C0u + flag) = 0;
    c->r[31] = back; f_088E1D88(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The link (0x095236C0) +13 cleared and its worker stopped (0x088E1D88). */
SFA_FUNC(088E0C48, link_stop_13) { stop_with(c, 13, 0x088E0C60u); }

/* The link (0x095236C0) +14 cleared and its worker stopped (0x088E1D88). */
SFA_FUNC(088E1444, link_stop_14) { stop_with(c, 14, 0x088E145Cu); }

/* Every socket id from the last (0x088DD114) down to 0 closed (0x088DF754). */
SFA_FUNC(088DF778, link_sockets_close_all) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[31] = 0x088DF78Cu; f_088DD114(c);
    for (int32_t i = (int32_t)c->r[2]; i >= 0; i--) {
        c->r[16] = (uint32_t)i;
        ARG(0) = (uint32_t)i;
        c->r[31] = 0x088DF7A0u; f_088DF754(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* Unless the link (0x095236C0) +13 is set: set it, 0x088E0C6C (0) and its worker started (0x088E1BC8). */
SFA_FUNC(088E0AC0, link_start_once) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    const gaddr l = 0x095236C0u;
    c->r[16] = l;
    *W(c->r[29] + 20) = ra;
    int was = S8(l + 13);
    ARG(0) = 1;
    if (!was) {
        *B(l + 13) = 1;
        ARG(0) = 0; c->r[31] = 0x088E0AECu; f_088E0C6C(c);
        ARG(0) = l; c->r[31] = 0x088E0AF4u; f_088E1BC8(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
