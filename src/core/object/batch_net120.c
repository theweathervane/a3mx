/* Network functions the link scenarios reach, 112 - 243 instructions: the link's worker thread started,
   a listening (PTP server) socket stepped through listen / accept / handshake. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E0B9C(Cpu *c); void f_088E13AC(Cpu *c); void f_088DD2A8(Cpu *c); void f_088DF6D4(Cpu *c);
void f_088DD22C(Cpu *c); void f_088DCE0C(Cpu *c); void f_089F3470(Cpu *c);
void hle_sceKernelCreateSema(Cpu *c); void hle_sceKernelCreateThread(Cpu *c); void hle_sceKernelStartThread(Cpu *c);
void hle_sceKernelDelayThread(Cpu *c); void hle_sceNetFreeThreadinfo(Cpu *c); void hle_sceKernelDeleteThread(Cpu *c);
void hle_sceNetAdhocPtpListen(Cpu *c); void hle_sceNetAdhocPtpAccept(Cpu *c); void hle_sceNetAdhocPtpSend(Cpu *c);
void hle_sceNetAdhocPtpClose(Cpu *c); void hle_sceNetAdhocPtpFlush(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (session): unless already running (+16), the semaphore 0x08A3EB88 made if missing, the worker thread
   (+4, entry 0x088E1B3C, priority 0x6D) made if missing and started (+16 = 1); the last result returned.
   A failure waits 1 ms and tries again, 15 times; at the end the worker (if any) is freed and deleted,
   the peers reset (0x088E0B9C) and the queue 0x095236E0 dropped, the error kept at +0. */
SFA_FUNC(088E1BC8, link_worker_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[8];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    for (int i = 0; i < 8; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 64) = ra;
    c->r[16] = o;
    uint32_t res = 0;
    c->r[17] = 0;
    if (S8(o + 16) != 0) res = 0;
    else {
        const gaddr sema = 0x08A3EB88u;
        c->r[19] = 0x08A40000u; c->r[20] = 0x08A01380u; c->r[21] = 0x08A01390u;
        c->r[22] = 0x088E1B3Cu; c->r[23] = 0x095236E0u;
        int32_t tries = 0xF;
        c->r[18] = (uint32_t)tries;
        for (;;) {
            int32_t r = 0;
            int failed = 0;
            if (*W(sema) == 0) {
                ARG(0) = 0x08A01380u; ARG(1) = 0; ARG(2) = 1; ARG(3) = 1; c->r[8] = 0;
                c->r[31] = 0x088E1C7Cu; hle_sceKernelCreateSema(c);
                r = (int32_t)c->r[2];
                c->r[17] = (uint32_t)r;
                if (r < 0) failed = 1; else *W(sema) = (uint32_t)r;
            }
            if (!failed) {
                uint32_t t = *W(o + 4);
                if (t == 0) {
                    ARG(0) = 0x08A01390u; ARG(1) = 0x088E1B3Cu; ARG(2) = 0x6D; ARG(3) = 0x2000;
                    c->r[8] = 0; c->r[9] = 0;
                    c->r[31] = 0x088E1CB4u; hle_sceKernelCreateThread(c);
                    r = (int32_t)c->r[2];
                    c->r[17] = (uint32_t)r;
                    if (r < 0) failed = 1; else { *W(o + 4) = (uint32_t)r; t = (uint32_t)r; }
                }
                if (!failed) {
                    if (t != 0) {
                        ARG(0) = t; ARG(1) = 0; ARG(2) = 0;
                        c->r[31] = 0x088E1CD8u; hle_sceKernelStartThread(c);
                        r = (int32_t)c->r[2];
                        c->r[17] = (uint32_t)r;
                        if (r < 0) failed = 1; else *B(o + 16) = 1;
                    } else ARG(1) = 0;
                    if (!failed) { res = c->r[17]; break; }
                }
            }
            tries = (int8_t)(tries - 1);
            c->r[18] = (uint32_t)tries;
            if (tries > 0) {
                ARG(0) = 0x3E8;
                c->r[31] = 0x088E1D34u; hle_sceKernelDelayThread(c);
                c->r[17] = 0;
                continue;
            }
            uint32_t w = *W(o + 4);
            *W(o) = c->r[17];
            if ((int32_t)w > 0) {
                call_on(c, w, 0x088E1D50u, hle_sceNetFreeThreadinfo);
                call_on(c, *W(o + 4), 0x088E1D58u, hle_sceKernelDeleteThread);
                call_on(c, o, 0x088E1D60u, f_088E0B9C);
                ARG(0) = o; ARG(1) = 0x095236E0u;
                c->r[31] = 0x088E1D6Cu; f_088E13AC(c);
            }
            *W(o + 4) = 0;
            if (tries != 0) { res = c->r[17]; break; }
            tries = (int8_t)(tries - 1);
            c->r[18] = (uint32_t)tries;
        }
    }
    c->r[2] = res;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x50;
}

/* (id): its socket (0x088DD2A8; 0x80410707 without one) stepped by its state (+16): 0 reset (closed, tries
   +14 = 60, ticks +12 = 600, kind +8 = PTP); 1 listen on port 0x401 (0x088DD22C's address), the socket kept
   at +0; 2 accept (a would-block counts +12 down, then +14, giving up with 0x80410715) and keep the peer
   if it is the session's (0x088DCE0C +228), else close it; 3 / 4 send the handshake word 0x1AC3C501 until
   4 bytes are out; 5 flush; 6 done (1 returned, state back to 0). A hard error spends a try (+14): with
   tries left the socket is reset to state 1 and 0 returned, else the error is kept at +4 and returned. */
SFA_FUNC(088DFEF4, link_ptp_server_step) {
    uint32_t id = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0xE0;
    gaddr sp = c->r[29];
    *W(sp + 188) = sv[0];
    c->r[16] = id;
    *W(sp + 192) = sv[1]; *W(sp + 196) = sv[2]; *W(sp + 200) = sv[3]; *W(sp + 204) = sv[4]; *W(sp + 208) = ra;
    c->r[17] = 0;
    ARG(0) = id;
    c->r[31] = 0x088DFF1Cu; f_088DD2A8(c);
    gaddr s = c->r[2];
    c->r[18] = s;
    uint32_t r = 0;
    enum { DONE, FAIL, RETRY } how = DONE;   /* FAIL: +4 already set, state cleared, closed */
    if (s == 0) { r = 0x80410707u; *W(s + 4) = r; how = FAIL; }
    else {
        uint32_t st = *B(s + 16);
        switch (st) {
        case 0: case 1: {
            if (st == 0) {
                call_on(c, s, 0x088DFF90u, f_088DF6D4);
                *H(s + 14) = 0x3C;
                uint32_t b = *B(s + 16);
                *H(s + 12) = 0x258; *W(s + 8) = 1; *W(s + 4) = 0;
                *B(s + 16) = (uint8_t)(b + 1);
            }
            c->r[19] = 0x30D40u; c->r[17] = 0x401;
            c->r[31] = 0x088DFFDCu; f_088DD22C(c);
            gaddr p = c->r[2];
            *H(p + 6) = 0x401;
            ARG(0) = p; ARG(1) = *H(p + 6); ARG(2) = 0x1000; ARG(3) = 0x30D40u;
            c->r[8] = 0x32; c->r[9] = 1; c->r[10] = 0;
            c->r[31] = 0x088E0000u; hle_sceNetAdhocPtpListen(c);
            r = c->r[2];
            c->r[17] = r;
            if ((int32_t)r < 0) { how = RETRY; break; }
            *W(s) = r;
            *B(s + 16) = (uint8_t)(*B(s + 16) + 1);
        }   /* fall through */
        case 2: {
            gaddr addr = sp + 0x20;
            c->r[20] = 0x80410709u; c->r[19] = addr; c->r[17] = sp + 0x26;
            ARG(0) = addr; ARG(1) = 0; ARG(2) = 0x94;
            c->r[31] = 0x088E0050u; f_089F3470(c);
            ARG(0) = *W(s); ARG(1) = addr; ARG(2) = sp + 0x26; ARG(3) = 0; c->r[8] = 1;
            c->r[31] = 0x088E0068u; hle_sceNetAdhocPtpAccept(c);
            r = c->r[2];
            c->r[17] = r;
            if (r == 0x80410709u) {
                uint32_t k = (uint32_t)*H(s + 12) - 1;
                *H(s + 12) = (uint16_t)k;
                r = 0;
                if ((k & 0xFFFF) == 0) {
                    uint32_t n = (uint32_t)*H(s + 14) - 1;
                    *H(s + 14) = (uint16_t)n;
                    if ((n & 0xFFFF) != 0) *H(s + 12) = 0x258;
                    else { r = 0x80410715u; *W(s + 4) = r; how = FAIL; }
                }
            } else if ((int32_t)r < 0) how = RETRY;
            else {
                ARG(0) = addr;
                c->r[31] = 0x088E00D4u; f_088DCE0C(c);
                if (id != *W(c->r[2] + 228)) {
                    ARG(0) = r; ARG(1) = 0;
                    c->r[31] = 0x088E011Cu; hle_sceNetAdhocPtpClose(c);
                } else {
                    if (*W(s) != r) call_on(c, s, 0x088E00F4u, f_088DF6D4);
                    *W(s) = r;
                    uint32_t b = *B(s + 16);
                    *W(s + 8) = 1;
                    *B(s + 16) = (uint8_t)(b + 1);
                }
                r = 0;
            }
            break;
        }
        case 3: case 4: {
            if (st == 3) { *H(s + 12) = 0; *B(s + 16) = (uint8_t)(st + 1); }
            c->r[16] = 0x80410709u;
            *W(sp + 180) = 0x1AC3C501u;
            uint32_t n = 4 - (uint32_t)*H(s + 12);
            *W(sp + 184) = n;
            ARG(0) = *W(s); ARG(1) = sp + 0xB4; ARG(2) = sp + 0xB8; ARG(3) = 0; c->r[8] = 1;
            c->r[31] = 0x088E0194u; hle_sceNetAdhocPtpSend(c);
            r = c->r[2];
            c->r[17] = r;
            if (r == 0x80410709u) r = 0;
            else if ((int32_t)r < 0) how = RETRY;
            else {
                *H(s + 12) = (uint16_t)(*H(s + 12) + *W(sp + 184));
                if (*H(s + 12) >= 4) *B(s + 16) = (uint8_t)(*B(s + 16) + 1);
                r = 0;
            }
            break;
        }
        case 5:
            ARG(0) = *W(s); ARG(1) = 0; ARG(2) = 1;
            c->r[31] = 0x088E01F4u; hle_sceNetAdhocPtpFlush(c);
            r = c->r[2];
            c->r[17] = r;
            if (r == 0x80410709u) r = 0;
            else if ((int32_t)r < 0) how = RETRY;
            else { *B(s + 16) = (uint8_t)(*B(s + 16) + 1); r = 0; }
            break;
        case 6:
            *H(s + 12) = 0; *W(s + 4) = 0; *B(s + 16) = 0;
            r = 1;
            break;
        default:
            r = 0;
            break;
        }
    }
    if (how == RETRY) {
        uint32_t n = (uint32_t)*H(s + 14) - 1;
        *H(s + 14) = (uint16_t)n;
        if ((n & 0xFFFF) == 0) { *W(s + 4) = r; how = FAIL; }
        else {
            call_on(c, s, 0x088E0284u, f_088DF6D4);
            *H(s + 12) = 0x258; *W(s + 8) = 1; *W(s + 4) = 0; *B(s + 16) = 1;
            r = 0;
        }
    }
    if (how == FAIL) {
        *B(s + 16) = 0;
        call_on(c, s, 0x088DFF40u, f_088DF6D4);
    } else if (how == DONE && r != 0) *B(s + 16) = 0;
    c->r[2] = r;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[31] = ra;
    c->r[29] += 0xE0;
}
