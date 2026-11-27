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


void f_088DD124(Cpu *c); void f_088DD314(Cpu *c); void f_088DC18C(Cpu *c); void f_089F3470(Cpu *c);
void f_088E066C(Cpu *c); void f_088DC2C4(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

#define LINK_ERR 0x095236C0u
#define LINK_SEMA 0x08A3EB88u

static void sema_wait(Cpu *c, uint32_t back) {
    uint32_t s = *W(LINK_SEMA);
    c->r[5] = 1;
    if (s != 0) { ARG(0) = s; ARG(1) = 1; ARG(2) = 0; c->r[31] = back; hle_sceKernelWaitSema(c); }
}
static int sema_signal(Cpu *c, uint32_t back) {
    uint32_t s = *W(LINK_SEMA);
    if (s == 0) return 0;
    ARG(0) = s; ARG(1) = 1; c->r[31] = back; hle_sceKernelSignalSema(c);
    return 1;
}

/* (link): a stream connection's receive loop: unless the link is the current one (0x088DD124), its
   queue (0x088DD314; none sets the error 0x81001003): each 16-byte packet is filled by receives
   (0x088DFA48, +4 counting the bytes still wanted), and a full one is handed to the queue at +0x1C
   (0x088E066C) under the link semaphore; it stops on an error (kept at 0x095236C0) or when no data is
   left. A 0x30 frame keeping s0..s5. */
SFA_FUNC(088E0728, net_stream_recv) {
    gaddr link = ARG(0);
    uint32_t sv[6], ra = c->r[31];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 6; i++) *W(sp + 16 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 40) = ra;
    c->r[16] = link;
    c->r[31] = 0x088E0750u; f_088DD124(c);
    c->r[21] = LINK_ERR;
    if (link != c->r[2]) {
        ARG(0) = link; c->r[31] = 0x088E0764u; f_088DD314(c);
        gaddr q = c->r[2]; c->r[20] = q;
        if (q == 0) *W(LINK_ERR) = 0x81001003u;
        else {
            c->r[19] = q + 0x1C; c->r[18] = 4; c->r[17] = 0x08A40000u;
            for (;;) {
                gaddr buf = *W(q + 12);
                if (buf == 0) {
                    ARG(0) = 0x10; c->r[31] = 0x088E0790u; f_088DC18C(c);
                    buf = c->r[2];
                    *W(q + 12) = buf;
                    if (buf == 0) break;
                    ARG(0) = buf; ARG(1) = 0; ARG(2) = 0x10; c->r[31] = 0x088E07A8u; f_089F3470(c);
                    *H(q + 4) = 4;
                    buf = *W(q + 12);
                }
                int32_t rem = S16(q + 4);
                ARG(0) = link; ARG(1) = buf + 0x10 - (uint32_t)rem; ARG(2) = (uint32_t)rem; ARG(3) = buf + 4;
                c->r[31] = 0x088E07CCu; f_088DFA48(c);
                int32_t n = (int32_t)c->r[2];
                if (n <= 0) { if (n < 0) *W(LINK_ERR) = (uint32_t)n; break; }
                *H(q + 4) = (uint16_t)(S16(q + 4) - n);
                if (S16(q + 4) == 0) {
                    sema_wait(c, 0x088E0804u);
                    ARG(0) = q + 0x1C; ARG(1) = *W(q + 12); c->r[31] = 0x088E0810u; f_088E066C(c);
                    *W(q + 12) = 0; *H(q + 4) = 0;
                    sema_signal(c, 0x088E082Cu);
                }
                if (*W(LINK_ERR) != 0) break;
            }
        }
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (): the datagram receive loop: while the link is open (+14) and no error is kept, each 0x10C-byte
   packet (0x088DC18C, cleared) receives up to 0x104 bytes from the current link; one whose size matches
   its kind (+9's low bits, sizes at 0x08A3EC50) is queued at 0x095236EC under the semaphore, others are
   freed (0x088DC2C4); stops when nothing arrives (an error is kept). A 0x30 frame keeping s0..s4. */
SFA_FUNC(088E1650, net_datagram_recv) {
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4]; c->r[20] = LINK_ERR;
    int32_t open = S8(LINK_ERR + 14);
    for (int i = 0; i < 4; i++) *W(sp + 16 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 36) = ra;
    c->r[4] = (uint32_t)open;
    if (open != 0 && *W(LINK_ERR) == 0) {
        c->r[19] = 0x095236ECu; c->r[17] = 0x08A3EC50u; c->r[16] = 0x08A40000u;
        for (;;) {
            ARG(0) = 0x10C; c->r[31] = 0x088E16A8u; f_088DC18C(c);
            gaddr b = c->r[2]; c->r[18] = b;
            if (b == 0) break;
            ARG(0) = b; ARG(1) = 0; ARG(2) = 0x10C; c->r[31] = 0x088E16C0u; f_089F3470(c);
            c->r[31] = 0x088E16C8u; f_088DD124(c);
            ARG(0) = c->r[2]; ARG(1) = b + 8; ARG(2) = 0x104; ARG(3) = b + 4;
            c->r[31] = 0x088E16DCu; f_088DFA48(c);
            int32_t n = (int32_t)c->r[2];
            if (n <= 0) {
                if (n < 0) *W(LINK_ERR) = (uint32_t)n;
                ARG(0) = b; c->r[31] = 0x088E1780u; f_088DC2C4(c);
                break;
            }
            if ((int32_t)*W(b + 4) < 0) { ARG(0) = b; c->r[31] = 0x088E1768u; f_088DC2C4(c); }
            else if (*W(0x08A3EC50u + ((uint32_t)(*B(b + 9) & 0xF) << 2)) + 4 != (uint32_t)n) {
                ARG(0) = b; c->r[31] = 0x088E1758u; f_088DC2C4(c);
            } else {
                sema_wait(c, 0x088E1728u);
                ARG(0) = 0x095236ECu; ARG(1) = b; c->r[31] = 0x088E1734u; f_088E066C(c);
                sema_signal(c, 0x088E1748u);
            }
            if (*W(LINK_ERR) != 0) break;
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}


void f_088DD3DC(Cpu *c); void f_088DD418(Cpu *c);

/* (): while the stream link is open (+13) without an error, each peer (an iterator from 0x088DD3DC,
   stepped by 0x088DD418) has its stream received (0x088E0728 with its +228). A frame keeping s0, s1. */
SFA_FUNC(088E08F4, net_stream_recv_all) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s1; c->r[17] = LINK_ERR;
    int32_t open = S8(LINK_ERR + 13);
    *W(sp + 20) = s0; *W(sp + 28) = ra;
    c->r[4] = (uint32_t)open;
    if (open != 0 && *W(LINK_ERR) == 0) {
        c->r[31] = 0x088E092Cu; f_088DD3DC(c);
        *W(sp + 16) = c->r[2];
        c->r[16] = sp + 0x10;
        for (;;) {
            ARG(0) = sp + 0x10; c->r[31] = 0x088E093Cu; f_088DD418(c);
            gaddr p = c->r[2];
            c->r[4] = p;
            if (p == 0 || S8(LINK_ERR + 13) == 0 || *W(LINK_ERR) != 0) break;
            ARG(0) = *W(p + 228); c->r[31] = 0x088E0968u; f_088E0728(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_088E087C(Cpu *c); void f_088E1518(Cpu *c); void f_088E1938(Cpu *c);
void hle_sceKernelSleepThread(Cpu *c); void hle_sceKernelCancelWakeupThread(Cpu *c);
void hle_sceKernelGetThreadId(Cpu *c); void hle_sceNetFreeThreadinfo(Cpu *c); void hle_sceKernelExitThread(Cpu *c);

/* The link thread: +16 = 1; each round +15 = +12, the link steps (0x088E087C, 0x088E1518, 0x088E1938,
   the datagram and stream receives), then sleep until woken; once +16 is cleared it frees its thread
   info and exits with the kept error. A frame keeping s0. */
SFA_FUNC(088E1B3C, net_link_thread) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = LINK_ERR;
    *B(LINK_ERR + 16) = 1;
    *W(c->r[29] + 20) = ra;
    do {
        *B(LINK_ERR + 15) = (uint8_t)S8(LINK_ERR + 12);
        c->r[31] = 0x088E1B64u; f_088E087C(c);
        c->r[31] = 0x088E1B6Cu; f_088E1518(c);
        c->r[31] = 0x088E1B74u; f_088E1938(c);
        c->r[31] = 0x088E1B7Cu; f_088E1650(c);
        c->r[31] = 0x088E1B84u; f_088E08F4(c);
        c->r[31] = 0x088E1B8Cu; hle_sceKernelSleepThread(c);
        ARG(0) = 0; c->r[31] = 0x088E1B94u; hle_sceKernelCancelWakeupThread(c);
    } while (S8(LINK_ERR + 16) != 0);
    c->r[31] = 0x088E1BA8u; hle_sceKernelGetThreadId(c);
    ARG(0) = c->r[2]; c->r[31] = 0x088E1BB0u; hle_sceNetFreeThreadinfo(c);
    ARG(0) = *W(LINK_ERR); c->r[31] = 0x088E1BB8u; hle_sceKernelExitThread(c);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
