/* The link's send (78 instructions, called every frame of a link match): a packet sent on the
   socket's kind, PTP or PDP to a peer's address. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DD23C(Cpu *c); void f_088DD2A8(Cpu *c); void f_088DF690(Cpu *c);
void hle_sceNetAdhocPtpSend(Cpu *c); void hle_sceNetAdhocPdpSend(Cpu *c);

/* (id, data, length, peer): the socket (0x088DD2A8 of id) when open (0x088DF690, else 0x80410707) sends by
   its kind (+8): 1 a PTP send (length by reference), 2 a PDP send to the peer's address (0x088DD23C of
   peer, 0x80410702 without one; a negative peer sends to 0x08A3EB80) with its port (+6); the length is
   returned when sent (or for other kinds), 0 when it would block (0x80410709), errors kept at the
   socket's +4 and returned. */
SFA_FUNC(088DF910, link_send) {
    uint32_t id = ARG(0), data = ARG(1), len = ARG(2), peer = ARG(3);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 36) = sv[0]; *W(sp + 40) = sv[1]; *W(sp + 48) = sv[3];
    c->r[16] = peer; c->r[17] = data; c->r[19] = 0;
    *W(sp + 44) = sv[2]; *W(sp + 52) = ra;
    *W(sp + 32) = len;
    ARG(0) = id;
    c->r[31] = 0x088DF93Cu; f_088DD2A8(c);
    gaddr s = c->r[2];
    c->r[18] = s;
    ARG(0) = s;
    c->r[31] = 0x088DF948u; f_088DF690(c);
    uint32_t r = 0;
    int err = 0;
    if (c->r[2] == 0) { r = 0x80410707u; err = 1; }
    else {
        int32_t k = (int32_t)*W(s + 8);
        if (k == 1) {
            ARG(0) = *W(s); ARG(1) = data; ARG(2) = sp + 0x20; ARG(3) = 0; c->r[8] = 1;
            c->r[31] = 0x088DF988u; hle_sceNetAdhocPtpSend(c);
            r = c->r[2];
        } else if (k == 2) {
            gaddr mac;
            if ((int32_t)peer < 0) mac = 0x08A3EB80u;
            else {
                ARG(0) = peer;
                c->r[31] = 0x088DF9ACu; f_088DD23C(c);
                mac = c->r[2];
            }
            if (mac == 0) { r = 0x80410702u; err = 1; }
            else {
                ARG(0) = *W(s); ARG(1) = mac; ARG(2) = *H(mac + 6); ARG(3) = data;
                c->r[8] = *W(sp + 32); c->r[9] = 0; c->r[10] = 1;
                c->r[31] = 0x088DF9E4u; hle_sceNetAdhocPdpSend(c);
                r = c->r[2];
            }
        }
        if (!err) {
            if (r == 0x80410709u) r = 0;
            else if ((int32_t)r >= 0) r = *W(sp + 32);
            else { *W(s + 4) = r; err = 1; }
        }
    }
    if (err) *W(s + 4) = r;
    c->r[2] = r;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
