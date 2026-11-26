/* Network functions the link scenarios reach, 51 - 78 instructions: the link session torn down, the
   link's outgoing queue sent. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DC2C4(Cpu *c); void f_088DDB68(Cpu *c); void f_088DE8F0(Cpu *c); void f_088DF6D4(Cpu *c);
void f_088E0B04(Cpu *c); void f_089F3470(Cpu *c); void f_088DD124(Cpu *c); void f_088DF910(Cpu *c);
void f_088E0698(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* The link session (0x09523820) torn down: each peer node (+232 list) has its queue (+0xAC) reset, its
   socket (+0x98) closed and its buffer (+148) let go before the node is freed; then the session's own,
   the session cleared (0xE8 bytes) and 0x088DE8F0 on its +4. */
SFA_FUNC(088DCD40, link_session_teardown) {
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    const gaddr ses = 0x09523820u;
    c->r[19] = ses;
    *W(ses + 236) = 0;
    *W(sp + 24) = sv[2];
    gaddr node = *W(ses + 232);
    c->r[18] = node;
    *B(ses + 240) = 0;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 32) = ra;
    c->r[16] = ses + 4;
    while (node != 0) {
        gaddr cur = node;
        node = *W(cur);
        c->r[17] = cur; c->r[18] = node;
        call_on(c, cur + 0xAC, 0x088DCD84u, f_088E0B04);
        call_on(c, cur + 0x98, 0x088DCD8Cu, f_088DF6D4);
        if (cur + 4 != 0) {
            call_on(c, *W(cur + 148), 0x088DCDA0u, f_088DDB68);
            *W(cur + 148) = 0;
        }
        call_on(c, cur, 0x088DCDACu, f_088DC2C4);
        *W(ses + 232) = node;
    }
    call_on(c, ses + 0xAC, 0x088DCDBCu, f_088E0B04);
    call_on(c, ses + 0x98, 0x088DCDC4u, f_088DF6D4);
    if (ses + 4 != 0) {
        call_on(c, *W(ses + 148), 0x088DCDD4u, f_088DDB68);
        *W(ses + 148) = 0;
    }
    ARG(0) = ses; ARG(1) = 0; ARG(2) = 0xE8;
    c->r[31] = 0x088DCDE8u; f_089F3470(c);
    call_on(c, ses + 4, 0x088DCDF0u, f_088DE8F0);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* While the link runs (0x095236C0 +14, no error at +0), the outgoing queue (0x095236E0, count +8) is sent
   packet by packet (0x088DF910 from our id, length by the packet's +9 kind from 0x08A3EC50 + 4, to its +4
   peer); a fully sent packet is taken off (0x088E0698 under the semaphore 0x08A3EB88) and freed
   (0x088DC2C4); a partial send is tried again, nothing sent ends it, an error is kept at +0. */
SFA_FUNC(088E1518, link_send_queue) {
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    const gaddr st = 0x095236C0u;
    c->r[19] = st;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    *W(sp + 40) = ra;
    if (S8(st + 14) != 0 && *W(st) == 0) {
        const gaddr q = 0x095236E0u;
        c->r[18] = q; c->r[17] = 0x08A3EC50u; c->r[16] = 0x08A40000u;
        while (*W(q + 8) != 0 && *W(st) == 0) {
            gaddr pk = *W(q);
            if (pk == 0) { *W(q + 8) = 0; break; }
            c->r[21] = pk;
            uint32_t len = *W(0x08A3EC50u + ((*B(pk + 9) & 0xF) << 2)) + 4;
            c->r[20] = len;
            c->r[31] = 0x088E15A8u; f_088DD124(c);
            ARG(0) = c->r[2]; ARG(1) = pk + 8; ARG(2) = len; ARG(3) = *W(pk + 4);
            c->r[31] = 0x088E15BCu; f_088DF910(c);
            int32_t r = (int32_t)c->r[2];
            if (r <= 0) {
                if (r < 0) *W(st) = (uint32_t)r;
                break;
            }
            if ((uint32_t)r != len) continue;
            if (*W(0x08A3EB88u) != 0) {
                ARG(0) = *W(0x08A3EB88u); ARG(1) = 1; ARG(2) = 0;
                c->r[31] = 0x088E15E4u; hle_sceKernelWaitSema(c);
            }
            call_on(c, q, 0x088E15ECu, f_088E0698);
            gaddr got = c->r[2];
            c->r[20] = got;
            if (*W(0x08A3EB88u) != 0) {
                ARG(0) = *W(0x08A3EB88u); ARG(1) = 1;
                c->r[31] = 0x088E1600u; hle_sceKernelSignalSema(c);
            }
            if (got != 0) call_on(c, got, 0x088E1610u, f_088DC2C4);
        }
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
