/* Network functions the link scenarios reach, 34 - 43 instructions: a link's handshake packet wait,
   the link's peer list emptied, the link's peers reset. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E0C6C(Cpu *c); void f_088E1074(Cpu *c); void f_088E11C0(Cpu *c); void f_088E12FC(Cpu *c);
void f_088DC2C4(Cpu *c); void f_088DDB68(Cpu *c); void f_088DD114(Cpu *c); void f_088DD1AC(Cpu *c);
void f_088DD304(Cpu *c); void f_088E06D0(Cpu *c); void f_088E0B04(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (session, link): 0x088E0C6C (0), then the link's received packets (+0xAC, 0x088E1074 into the stack)
   until one starts with the handshake word 0x1CE14355 (others dropped, 0x088E11C0): then it is dropped
   too, 0x088E12FC, the session's +30 / +33 and the link's +4 / +6 cleared, +2 counted, and the session's
   +36 returned; with none, the link's wait (+8) counts and at 241 its +2 clears; 0 returned. */
SFA_FUNC(088E2D78, link_handshake_wait) {
    gaddr o = ARG(0), l = ARG(1);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 40) = sv[4];
    c->r[20] = l;
    *W(sp + 24) = sv[0]; *W(sp + 44) = sv[5];
    c->r[16] = o;
    gaddr st = l + 0xD4;
    c->r[21] = st;
    *W(sp + 28) = sv[1]; *W(sp + 32) = sv[2]; *W(sp + 36) = sv[3]; *W(sp + 48) = ra;
    call_on(c, 0, 0x088E2DACu, f_088E0C6C);
    const uint32_t magic = 0x1CE14355u;
    gaddr q = l + 0xAC;
    c->r[18] = magic; c->r[20] = q; c->r[19] = sp + 0x10; c->r[17] = sp + 0x14;
    uint32_t r = 0;
    for (;;) {
        ARG(0) = q; ARG(1) = sp + 0x10;
        c->r[31] = 0x088E2DCCu; f_088E1074(c);
        if (c->r[2] == 0) {
            *H(st + 8) = (uint16_t)(S16(st + 8) + 1);
            if (!(S16(st + 8) < 241)) *H(st + 2) = 0;
            r = 0;
            break;
        }
        *W(sp + 20) = magic;
        if (*W(*W(sp + 16)) != magic) {
            ARG(0) = q; ARG(1) = sp + 0x14; ARG(2) = 0;
            c->r[31] = 0x088E2DF8u; f_088E11C0(c);
            continue;
        }
        ARG(0) = q; ARG(1) = sp + 0x14; ARG(2) = 0;
        c->r[31] = 0x088E2E10u; f_088E11C0(c);
        c->r[31] = 0x088E2E18u; f_088E12FC(c);
        *H(o + 30) = 0;
        *B(o + 33) = 0;
        *H(st + 6) = 0;
        *H(st + 2) = (uint16_t)(*H(st + 2) + 1);
        *H(st + 4) = 0;
        r = (uint32_t)S8(o + 36);
        break;
    }
    c->r[2] = r;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* Under the list's semaphore (0x09523720 +8), every node of the peer list (0x09523720) freed: its +148
   buffer let go (0x088DDB68) and the node (0x088DC2C4); then the list and its counts (+4 / +12 / +14 /
   +18) cleared. */
SFA_FUNC(088DC328, link_peers_empty) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    const gaddr list = 0x09523720u;
    c->r[16] = list;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    if (*W(list + 8) != 0) {
        ARG(0) = *W(list + 8); ARG(1) = 1; ARG(2) = 0;
        c->r[31] = 0x088DC358u; hle_sceKernelWaitSema(c);
    }
    gaddr node = *W(list);
    if (node == 0) *W(list + 4) = 0;
    else {
        while (node != 0) {
            gaddr item = node + 4;
            gaddr next = *W(node);
            c->r[17] = item; c->r[18] = next;
            if (item != 0) {
                call_on(c, *W(item + 144), 0x088DC378u, f_088DDB68);
                *W(item + 144) = 0;
                node = *W(list);
            }
            call_on(c, node, 0x088DC388u, f_088DC2C4);
            *W(list) = next;
            node = next;
        }
        *W(list + 4) = 0;
    }
    *H(list + 14) = 0; *H(list + 12) = 0;
    *H(list + 18) = 0;
    if (*W(list + 8) != 0) {
        ARG(0) = *W(list + 8); ARG(1) = 1;
        c->r[31] = 0x088DC3B4u; hle_sceKernelSignalSema(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (session): every peer (0x088DD114 of them, from the last; 0x088DD1AC) has its queue (+0xAC) reset
   (0x088E0B04); the session's +0x14 reset (0x088E06D0) under the semaphore 0x08A3EB88; the local queue
   (0x088DD304) reset too and the session's +0 cleared. */
SFA_FUNC(088E0B9C, link_peers_reset) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    c->r[31] = 0x088E0BB8u; f_088DD114(c);
    int32_t n = (int32_t)c->r[2];
    c->r[17] = 0x08A40000u;
    c->r[18] = (uint32_t)n;
    if (n > 0) {
        for (int32_t i = n - 1; ; i--) {
            c->r[18] = (uint32_t)i;
            call_on(c, (uint32_t)i, 0x088E0BD0u, f_088DD1AC);
            if (c->r[2] != 0) call_on(c, c->r[2] + 0xAC, 0x088E0BE4u, f_088E0B04);
            if (!(i > 0)) { c->r[18] = (uint32_t)(i - 1); break; }
        }
    }
    if (*W(0x08A3EB88u) != 0) {
        ARG(0) = *W(0x08A3EB88u); ARG(1) = 1; ARG(2) = 0;
        c->r[31] = 0x088E0C00u; hle_sceKernelWaitSema(c);
    }
    call_on(c, o + 0x14, 0x088E0C08u, f_088E06D0);
    if (*W(0x08A3EB88u) != 0) {
        ARG(0) = *W(0x08A3EB88u); ARG(1) = 1;
        c->r[31] = 0x088E0C1Cu; hle_sceKernelSignalSema(c);
    }
    c->r[31] = 0x088E0C24u; f_088DD304(c);
    call_on(c, c->r[2], 0x088E0C2Cu, f_088E0B04);
    *W(o) = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
