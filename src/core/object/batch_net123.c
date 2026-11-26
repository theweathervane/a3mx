/* The link's per-peer sender (177 instructions, called every frame for each peer): the peer's queued
   packets merged into one send, sent until the socket would block, and packets meant for several frames
   split back onto the resend queue. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DC18C(Cpu *c); void f_088DC2C4(Cpu *c); void f_088DD124(Cpu *c); void f_088DD314(Cpu *c);
void f_088DF910(Cpu *c); void f_088DFC10(Cpu *c); void f_088E066C(Cpu *c); void f_088E0698(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

#define SEMA 0x08A3EB88u

static void sema_wait(Cpu *c, uint32_t back) {
    ARG(1) = 1;
    if (*W(SEMA) != 0) { ARG(0) = *W(SEMA); ARG(2) = 0; c->r[31] = back; hle_sceKernelWaitSema(c); }
}
static void sema_signal(Cpu *c, uint32_t back) {
    if (*W(SEMA) != 0) { ARG(0) = *W(SEMA); ARG(1) = 1; c->r[31] = back; hle_sceKernelSignalSema(c); }
}
static void free_block(Cpu *c, gaddr b, uint32_t back) { ARG(0) = b; c->r[31] = back; f_088DC2C4(c); }
static void requeue(Cpu *c, gaddr q, gaddr e, uint32_t back) {
    ARG(0) = q; ARG(1) = e; c->r[31] = back; f_088E066C(c);
}
static void copy16(gaddr to, gaddr from) {
    uint32_t a = *W(from), b = *W(from + 4), d = *W(from + 8);
    *W(to) = a; uint32_t e = *W(from + 12);
    *W(to + 4) = b; *W(to + 8) = d; *W(to + 12) = e;
}

/* The next packet gathered from the peer's queue (+0x10) into +8 with its length at +0: a lone entry taken
   as it is (4 bytes), several merged into one block (header of the first, then each entry's word at
   +0xC + 4 i while their peer (+4) and kind (+8) match; the others put back). Returns the block or 0. */
static gaddr gather(Cpu *c, gaddr p) {
    gaddr q = p + 0x10;
    *H(p) = 0;
    sema_wait(c, 0x088E1FACu);
    uint32_t n = *W(q + 8);
    if ((int32_t)n < 2) {
        ARG(0) = q; c->r[31] = 0x088E20A8u; f_088E0698(c);
        *W(p + 8) = c->r[2];
        if (c->r[2] != 0) *H(p) = 4;
    } else {
        ARG(0) = (n << 2) + 0x10;
        c->r[31] = 0x088E1FC4u; f_088DC18C(c);
        gaddr buf = c->r[2];
        *W(p + 8) = buf;
        if (buf != 0) {
            int32_t cnt = (int32_t)*W(q + 8);
            c->r[18] = (uint32_t)cnt;
            uint32_t off = 0;
            for (int32_t i = 0; i < cnt; i++, off += 4) {
                c->r[16] = (uint32_t)i; c->r[17] = off;
                ARG(0) = q; c->r[31] = 0x088E1FE8u; f_088E0698(c);
                gaddr e = c->r[2];
                if (e == 0) break;
                buf = *W(p + 8);
                if (i == 0) {
                    copy16(buf, e);
                    free_block(c, e, 0x088E207Cu);
                    *H(p) = (uint16_t)(S16(p) + 4);
                } else if (*W(buf + 4) != *W(e + 4) || *B(buf + 8) != *B(e + 8)) {
                    requeue(c, q, e, 0x088E2050u);
                } else {
                    *W(buf + off + 0xC) = *W(e + 0xC);
                    *H(p) = (uint16_t)(S16(p) + 4);
                    free_block(c, e, 0x088E203Cu);
                }
            }
        }
    }
    sema_signal(c, 0x088E20CCu);
    return *W(p + 8);
}

/* (peer id): unless it is us (0x088DD124), the peer (0x088DD314; 0x81001003 kept at the link 0x095236C0
   without one) sends: while the link has no error, a packet is gathered when none is pending (its length
   +0 kept as +2) and its unsent part sent (0x088DF910); when it is all out, a one-off packet (+8 kind 0) is
   freed, and a packet repeated over several frames is split into one 0x14 block per frame on the resend
   queue 0x095236D4 (fewer than two: the packet itself). Nothing to send or a would-block stops; an error
   is kept at the link. A peer marked (+6) that was idle is flushed (0x088DFC10) and unmarked. */
SFA_FUNC(088E1EF8, link_peer_send) {
    uint32_t id = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 8; i++) *W(sp + 24 + 4 * i) = sv[i];
    *W(sp + 56) = sv[8]; *W(sp + 60) = ra;
    c->r[16] = id;
    c->r[31] = 0x088E1F2Cu; f_088DD124(c);
    const gaddr L = 0x095236C0u, resend = 0x095236D4u;
    c->r[30] = 0x09520000u;
    if (id != c->r[2]) {
        *W(sp + 16) = id;
        c->r[30] = L;
        ARG(0) = id; c->r[31] = 0x088E1F48u; f_088DD314(c);
        gaddr p = c->r[2];
        c->r[19] = p;
        if (p == 0) *W(L) = 0x81001003u;
        else {
            int8_t mark = S8(p + 6);
            *B(sp + 20) = (uint8_t)mark;
            c->r[22] = p + 0x10;
            int go = 1;
            if ((*W(p + 8) | (uint32_t)(int32_t)mark) == 0 && (int32_t)*W(p + 0x10 + 8) < (int32_t)S8(L + 15))
                go = 0;
            if (go) {
                c->r[23] = resend; c->r[21] = 0x08A40000u;
                for (;;) {
                    gaddr pk = *W(p + 8);
                    if (pk == 0) {
                        pk = gather(c, p);
                        if (pk == 0) { *H(p) = 0; *H(p + 2) = 0; break; }
                        if (S16(p) == 0) {
                            free_block(c, pk, 0x088E20ECu);
                            *W(p + 8) = 0; *H(p + 2) = 0;
                            break;
                        }
                        *H(p + 2) = *H(p);
                    }
                    int32_t left = S16(p);
                    ARG(0) = *W(sp + 16); ARG(1) = pk + 0xC + (uint32_t)(S16(p + 2) - left);
                    ARG(2) = (uint32_t)left; ARG(3) = *W(pk + 4);
                    c->r[31] = 0x088E2128u; f_088DF910(c);
                    int32_t r = (int32_t)c->r[2];
                    if (r <= 0) {
                        if (r < 0) *W(L) = (uint32_t)r;
                        break;
                    }
                    *H(p) = (uint16_t)(S16(p) - r);
                    if (S16(p) == 0) {
                        pk = *W(p + 8);
                        if (*B(pk + 8) == 0) {
                            free_block(c, pk, 0x088E228Cu);
                            *W(p + 8) = 0;
                        } else {
                            uint32_t n = (uint32_t)(int32_t)S16(p + 2) >> 2;
                            c->r[20] = n; c->r[18] = 0;
                            if ((int32_t)n < 2) {
                                sema_wait(c, 0x088E218Cu);
                                requeue(c, resend, *W(p + 8), 0x088E219Cu);
                                *W(p + 8) = 0;
                                sema_signal(c, 0x088E21B4u);
                            } else {
                                uint32_t off = 0;
                                for (int32_t i = 0; i < (int32_t)n; i++, off += 4) {
                                    c->r[17] = off;
                                    ARG(0) = 0x14; c->r[31] = 0x088E21D4u; f_088DC18C(c);
                                    gaddr nb = c->r[2];
                                    c->r[16] = nb;
                                    if (nb != 0) {
                                        gaddr src = *W(p + 8);
                                        copy16(nb, src);
                                        *W(nb + 0xC) = *W(*W(p + 8) + off + 0xC);
                                        sema_wait(c, 0x088E2230u);
                                        requeue(c, resend, nb, 0x088E223Cu);
                                        c->r[16] = *W(SEMA);
                                        sema_signal(c, 0x088E2250u);
                                    }
                                    c->r[18] = (uint32_t)(i + 1);
                                }
                                if (*W(p + 8) != 0) {
                                    free_block(c, *W(p + 8), 0x088E2274u);
                                    *W(p + 8) = 0;
                                }
                            }
                        }
                    }
                    if (*W(L) != 0) break;
                }
            }
            if (S8(sp + 20) != 0) {
                ARG(0) = *W(sp + 16);
                c->r[31] = 0x088E22C4u; f_088DFC10(c);
                *B(p + 6) = 0;
            }
        }
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
