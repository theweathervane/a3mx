/* The link play's connection to one peer, a step at a time. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DD2A8(Cpu *c); void f_088DF6D4(Cpu *c); void f_088DD23C(Cpu *c); void f_088DD22C(Cpu *c);
void hle_sceNetAdhocPtpOpen(Cpu *c); void hle_sceNetAdhocPtpConnect(Cpu *c); void hle_sceNetAdhocPtpRecv(Cpu *c);

#define WOULD_BLOCK 0x80410709u
#define REFUSED     0x80410718u

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }

/* (peer): one step of connecting to a peer; its connection is what 0x088DD2A8 returns (none:
   0x80410707), with the socket at +0, the last error at +4, +8, a wait at +12, the tries left at
   +14 and the step at +16:
   0     closes what was there (0x088DF6D4), 30 tries, a wait of 0x4B0, and goes on as 1;
   1     opens a socket to the peer's address (0x088DD23C, port 0x401 kept at its +6) from our own
         (0x088DD22C): 0x1000 bytes, a 200 ms retry, 0x32 tries; and goes on as 2;
   2     connects without waiting: still on its way counts the wait down, and when it runs out a try
         (none left: 0x80410715); refused closes and waits 0x3C in step 50; done goes to 3;
   3, 4  reads four bytes, and with them all goes to 5;
   5     done: the step and error are cleared and it returns 1;
   50    the wait, then step 51, which starts over at step 1 (as does any failed call while tries
         are left).
   With no tries left the connection is closed, the error is kept at +4, the step is 0 and the error
   is returned; otherwise 0 while it is still at work. A 0x40 frame keeping s0..s3. */
SFA_FUNC(088E02C0, link_connect_step) {
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 4; i++) *W(sp + 40 + 4 * i) = sv[i];
    *W(sp + 56) = sv[4];
    c->r[16] = ARG(0); c->r[17] = 0;
    c->r[31] = 0x088E02E4u; f_088DD2A8(c);
    gaddr n = c->r[2];
    c->r[18] = n;
    uint32_t res = 0, state, sock;
    if (n == 0) { res = 0x80410707u; goto fail; }
    state = *B(n + 16);
    switch (state) {
    case 0:
        call1(c, n, f_088DF6D4, 0x088E0398u);
        *H(n + 14) = 0x1E;
        {
            uint8_t s = *B(n + 16);
            *H(n + 12) = 0x4B0;
            *W(n + 8) = 1;
            *W(n + 4) = 0;
            *B(n + 16) = (uint8_t)(s + 1);
        }
        /* fall through */
    case 1: {
        c->r[19] = 0x30D40; c->r[17] = 0x401;
        call1(c, c->r[16], f_088DD23C, 0x088E03E4u);
        gaddr to = c->r[2];
        c->r[16] = to;
        *H(to + 6) = 0x401;
        c->r[31] = 0x088E03F0u; f_088DD22C(c);
        ARG(0) = c->r[2]; ARG(1) = 0; ARG(2) = to; ARG(3) = *H(to + 6);
        c->r[8] = 0x1000; c->r[9] = 0x30D40; c->r[10] = 0x32; c->r[11] = 0;
        c->r[31] = 0x088E0414u; hle_sceNetAdhocPtpOpen(c);
        res = c->r[2]; c->r[17] = res;
        if ((int32_t)res < 0) goto retry;
        *W(n) = res;
        *B(n + 16) = (uint8_t)(*B(n + 16) + 1);
        sock = res;
        goto connect;
    }
    case 2:
        sock = *W(n);
    connect:
        c->r[17] = WOULD_BLOCK; c->r[16] = WOULD_BLOCK;
        ARG(0) = sock; ARG(1) = 0; ARG(2) = 1;
        c->r[31] = 0x088E045Cu; hle_sceNetAdhocPtpConnect(c);
        res = c->r[2]; c->r[17] = res;
        if (res == WOULD_BLOCK) {
            uint32_t wait = (uint32_t)*H(n + 12) - 1;
            *H(n + 12) = (uint16_t)wait;
            res = 0;
            if ((wait & 0xFFFF) == 0) {
                uint32_t tries = (uint32_t)*H(n + 14) - 1;
                *H(n + 14) = (uint16_t)tries;
                if ((tries & 0xFFFF) == 0) { res = 0x80410715u; goto fail; }
                *H(n + 12) = 0x4B0;
            }
        } else if (res == REFUSED) {
            call1(c, n, f_088DF6D4, 0x088E04CCu);
            *B(n + 16) = 0x32;
            *H(n + 12) = 0x3C;
            res = 0;
        } else if (res != 0) goto retry;
        else *B(n + 16) = (uint8_t)(*B(n + 16) + 1);
        break;
    case 3:
        *H(n + 12) = 0;
        *B(n + 16) = (uint8_t)(state + 1);
        /* fall through */
    case 4:
        c->r[16] = WOULD_BLOCK;
        *W(sp + 36) = 4;
        ARG(0) = *W(n); ARG(1) = sp + 0x20; ARG(2) = sp + 0x24; ARG(3) = 0; c->r[8] = 1;
        c->r[31] = 0x088E0548u; hle_sceNetAdhocPtpRecv(c);
        res = c->r[2]; c->r[17] = res;
        if (res == WOULD_BLOCK) res = 0;
        else if ((int32_t)res < 0) goto retry;
        else {
            *H(n + 12) = (uint16_t)(*H(n + 12) + *W(sp + 36));
            if (!(*H(n + 12) < 4u)) *B(n + 16) = (uint8_t)(*B(n + 16) + 1);
            res = 0;
        }
        break;
    case 5:
        *H(n + 12) = 0;
        *B(n + 16) = 0;
        *W(n + 4) = 0;
        res = 1;
        break;
    case 50: {
        uint32_t wait = (uint32_t)*H(n + 12) - 1;
        *H(n + 12) = (uint16_t)wait;
        if ((wait & 0xFFFF) != 0) break;
        uint32_t tries = (uint32_t)*H(n + 14) - 1;
        *H(n + 14) = (uint16_t)tries;
        if ((tries & 0xFFFF) != 0) { *B(n + 16) = (uint8_t)(state + 1); break; }
        *W(n + 4) = REFUSED;
        goto retry;
    }
    case 51:
        goto restart;
    default:
        break;
    }
    if (res != 0) *B(n + 16) = 0;
    c->r[2] = res;
    goto out;
retry: {
        uint32_t tries = (uint32_t)*H(n + 14) - 1;
        *H(n + 14) = (uint16_t)tries;
        if ((tries & 0xFFFF) == 0) goto fail;
    }
restart:
    call1(c, n, f_088DF6D4, 0x088E0634u);
    *H(n + 12) = 0x4B0;
    *W(n + 8) = 1;
    *W(n + 4) = 0;
    *B(n + 16) = 1;
    c->r[2] = 0;
    goto out;
fail:
    c->r[17] = res;
    call1(c, n, f_088DF6D4, 0x088E0300u);
    *W(n + 4) = res;
    *B(n + 16) = 0;
    c->r[2] = res;
out:
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}
