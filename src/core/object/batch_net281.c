/* The wireless (ad-hoc) link's receive: by the connection's kind (1 PTP, 2 PDP), with the sender's
   address turned into a peer. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DCE0C(Cpu *c); void f_088DD2A8(Cpu *c); void f_088DF690(Cpu *c);
void hle_sceNetAdhocPtpRecv(Cpu *c); void hle_sceNetAdhocPdpRecv(Cpu *c);

/* (address...): the peer for an address (0x088DCE0C): its +228, or 0x81001000 when there is none. */
SFA_FUNC(088DD134, net_peer_of) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088DD144u; f_088DCE0C(c);
    gaddr p = c->r[2];
    c->r[4] = p;
    c->r[2] = p != 0 ? *W(p + 228) : 0x81001000u;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (link, buffer, length, peer out): receive on the link's connection (0x088DD2A8, checked by
   0x088DF690, else 0x80410707): PTP into the buffer, or PDP with the sender's peer stored through the
   last argument. "No data" (0x80410709) gives 0, another error is kept in the connection's +4 and
   returned, success (or a kind with no receive) returns the length word. A 0x40 frame keeping s0..s3. */
SFA_FUNC(088DFA48, net_recv) {
    gaddr buf = ARG(1), out = ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 44) = s0; *W(sp + 48) = s1; *W(sp + 56) = s3;
    c->r[16] = out; c->r[17] = buf; c->r[19] = 0;
    *W(sp + 52) = s2; *W(sp + 60) = ra;
    *W(sp + 32) = ARG(2);
    c->r[31] = 0x088DFA74u; f_088DD2A8(c);
    gaddr h = c->r[2]; c->r[18] = h;
    ARG(0) = h; c->r[31] = 0x088DFA80u; f_088DF690(c);
    int32_t r;
    int tail;   /* 0: the error / no-receive tail, 1: the result checks */
    if (c->r[2] == 0) { r = (int32_t)0x80410707u; tail = 0; }
    else {
        int32_t kind = (int32_t)*W(h + 8);
        r = 0;
        if (kind == 1) {
            ARG(0) = *W(h); ARG(1) = buf; ARG(2) = sp + 32; ARG(3) = 0; c->r[8] = 1;
            c->r[31] = 0x088DFAC0u; hle_sceNetAdhocPtpRecv(c);
            r = (int32_t)c->r[2];
        } else if (kind == 2) {
            c->r[7] = buf;
            gaddr addr = sp + 0x24; c->r[17] = addr;
            ARG(0) = *W(h); ARG(1) = addr; ARG(2) = sp + 0x2A; ARG(3) = buf;
            c->r[8] = sp + 0x20; c->r[9] = 0; c->r[10] = 1;
            c->r[31] = 0x088DFAF4u; hle_sceNetAdhocPdpRecv(c);
            r = (int32_t)c->r[2];
            if (out != 0) {
                ARG(0) = addr; c->r[31] = 0x088DFB04u; f_088DD134(c);
                *W(out) = c->r[2];
            }
        }
        c->r[4] = 0x80410000u;
        tail = 1;
    }
    if (tail) {
        if ((uint32_t)r == 0x80410709u) r = 0;
        else if (r >= 0) r = (int32_t)*W(sp + 32);
        else { *W(h + 4) = (uint32_t)r; tail = 0; }
    }
    if (!tail) {
        *W(h + 4) = (uint32_t)r;
        if (r >= 0) r = (int32_t)0x81001000u;
    }
    c->r[19] = (uint32_t)r;
    c->r[2] = (uint32_t)r;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x40;
}
