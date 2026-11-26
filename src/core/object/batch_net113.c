/* Small network functions the link scenarios reach, 32 - 42 instructions: a link socket closed,
   the link's peer state read under its semaphore, a socket flushed. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DF690(Cpu *c); void f_088DD124(Cpu *c);
void hle_sceNetAdhocPtpClose(Cpu *c); void hle_sceNetAdhocPdpDelete(Cpu *c); void hle_sceNetAdhocPtpFlush(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

/* (socket): when open (0x088DF690) closed by its kind (+8: 1 a PTP socket, 2 a PDP one); then +0 = -1 and
   +4 / +8 cleared. */
SFA_FUNC(088DF6D4, link_socket_close) {
    gaddr s = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = s;
    c->r[31] = 0x088DF6E8u; f_088DF690(c);
    if (c->r[2] != 0) {
        int32_t k = (int32_t)*W(s + 8);
        if (k == 1) { ARG(0) = *W(s); ARG(1) = 0; c->r[31] = 0x088DF714u; hle_sceNetAdhocPtpClose(c); }
        else if (k == 2) { ARG(0) = *W(s); ARG(1) = 0; c->r[31] = 0x088DF730u; hle_sceNetAdhocPdpDelete(c); }
    }
    if (s != 0) { *W(s) = 0xFFFFFFFFu; *W(s + 8) = 0; *W(s + 4) = 0; }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (id, peer): the link's state (0x095236C0) unless it is unset and 0x088DD124 isn't id: then the peer's
   +6 = 1 and, under the semaphore 0x08A3EB88 (wait / signal 1), the peer's +24 returned. */
SFA_FUNC(088E0EF4, link_peer_state) {
    uint32_t id = ARG(0);
    gaddr peer = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2;
    const gaddr state = 0x095236C0u;
    c->r[18] = state;
    *W(c->r[29] + 20) = s1;
    c->r[17] = id;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = peer;
    uint32_t r;
    if (*W(state) != 0) r = *W(state);
    else {
        c->r[31] = 0x088E0F28u; f_088DD124(c);
        if (c->r[2] == id) r = *W(state);
        else {
            *B(peer + 6) = 1;
            c->r[17] = 0x08A40000u;
            uint32_t sema = *W(0x08A3EB88u);
            if (sema != 0) {
                ARG(0) = sema; ARG(1) = 1; ARG(2) = 0;
                c->r[31] = 0x088E0F4Cu; hle_sceKernelWaitSema(c);
                sema = *W(0x08A3EB88u);
            }
            r = *W(peer + 24);
            c->r[16] = r;
            if (sema != 0) {
                ARG(0) = sema; ARG(1) = 1;
                c->r[31] = 0x088E0F60u; hle_sceKernelSignalSema(c);
            }
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (socket): for an open PTP socket (+8 = 1) a flush (sceNetAdhocPtpFlush (+0, 0, 1)): 1 when sent, 0 when it
   would block (0x80410709) or isn't a PTP socket; errors (and 0x80410707 for a closed socket) kept at +4
   and returned. */
SFA_FUNC(088DFB68, link_socket_flush) {
    gaddr s = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = s;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = 0x088DFB84u; f_088DF690(c);
    uint32_t r;
    if (c->r[2] == 0) { r = 0x80410707u; *W(s + 4) = r; }
    else if (*W(s + 8) != 1) r = 0;
    else {
        ARG(0) = *W(s); ARG(1) = 0; ARG(2) = 1;
        c->r[31] = 0x088DFBB4u; hle_sceNetAdhocPtpFlush(c);
        r = c->r[2];
        if (r == 0x80410709u) r = 0;
        else if ((int32_t)r >= 0) r = 1;
        else { *W(s + 4) = r; *W(s + 4) = r; }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
