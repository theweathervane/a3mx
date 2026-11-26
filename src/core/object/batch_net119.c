/* Network functions the link scenarios reach, 79 - 97 instructions: the link's worker thread stopped,
   the link's broadcast packet resent on a timer. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E0B9C(Cpu *c); void f_088E13AC(Cpu *c); void f_088DC2C4(Cpu *c); void f_088DD124(Cpu *c);
void f_088DF910(Cpu *c);
void hle_sceKernelGetThreadId(Cpu *c); void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);
void hle_sceNetFreeThreadinfo(Cpu *c); void hle_sceKernelWakeupThread(Cpu *c); void hle_sceKernelTerminateThread(Cpu *c);
void hle_sceKernelDeleteThread(Cpu *c); void hle_sceKernelWaitThreadEnd(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (session): with its worker (+4) running (+16) and not the caller, nor busy (+13 / +14), the worker is
   told to stop (+16 cleared under the semaphore 0x08A3EB88), freed and woken to finish (waited for up
   to 10 s; if it can't be woken it is terminated and deleted, +4 cleared); then the peers reset
   (0x088E0B9C), the queue 0x095236E0 dropped (0x088E13AC) and the caller's thread info freed. */
SFA_FUNC(088E1D88, link_worker_stop) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; *W(sp + 24) = s1; *W(sp + 28) = s2; *W(sp + 32) = ra;
    c->r[16] = o;
    if (S8(o + 16) != 0) {
        uint32_t w = *W(o + 4);
        c->r[17] = w;
        c->r[31] = 0x088E1DB0u; hle_sceKernelGetThreadId(c);
        if (w != c->r[2] && S8(o + 14) == 0 && S8(o + 13) == 0) {
            w = *W(o + 4);
            c->r[17] = w;
            int stop = 0;
            if ((int32_t)w >= 0) {
                c->r[31] = 0x088E1DE4u; hle_sceKernelGetThreadId(c);
                stop = w != c->r[2];
            }
            if (stop) {
                gaddr t = *W(o + 4);
                c->r[18] = t;
                *W(sp + 16) = 0x00989680u;
                c->r[17] = 0x08A40000u;
                if (*W(0x08A3EB88u) != 0) {
                    ARG(0) = *W(0x08A3EB88u); ARG(1) = 1; ARG(2) = 0;
                    c->r[31] = 0x088E1E10u; hle_sceKernelWaitSema(c);
                }
                if (S8(o + 16) == 0) {
                    if (*W(0x08A3EB88u) != 0) {
                        ARG(0) = *W(0x08A3EB88u); ARG(1) = 1;
                        c->r[31] = 0x088E1E84u; hle_sceKernelSignalSema(c);
                    }
                } else {
                    *B(o + 16) = 0;
                    if (*W(0x08A3EB88u) != 0) {
                        ARG(0) = *W(0x08A3EB88u); ARG(1) = 1;
                        c->r[31] = 0x088E1E34u; hle_sceKernelSignalSema(c);
                    }
                    call_on(c, t, 0x088E1E3Cu, hle_sceNetFreeThreadinfo);
                    call_on(c, t, 0x088E1E44u, hle_sceKernelWakeupThread);
                    if ((int32_t)c->r[2] >= 0) {
                        ARG(0) = t; ARG(1) = sp + 0x10;
                        c->r[31] = 0x088E1E6Cu; hle_sceKernelWaitThreadEnd(c);
                    } else {
                        call_on(c, t, 0x088E1E54u, hle_sceKernelTerminateThread);
                        call_on(c, t, 0x088E1E5Cu, hle_sceKernelDeleteThread);
                        *W(o + 4) = 0;
                    }
                }
            }
            call_on(c, o, 0x088E1E8Cu, f_088E0B9C);
            ARG(0) = o; ARG(1) = 0x095236E0u;
            c->r[31] = 0x088E1E9Cu; f_088E13AC(c);
            c->r[31] = 0x088E1EA4u; hle_sceKernelGetThreadId(c);
            call_on(c, c->r[2], 0x088E1EACu, hle_sceNetFreeThreadinfo);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* While the link runs (0x095236C0 +14, no error at +0): a newly queued broadcast (0x095236E0 +28) replaces
   the current one (+24, the old freed), its interval +34 = +38, +32 / +36 cleared (under the semaphore
   0x08A3EB88); the current one is resent to everyone (0x088DF910 to -1) whenever its countdown (+32)
   runs out, the next countdown 0x14 for the first three sends (+36) and then +34; errors kept at +0. */
SFA_FUNC(088E1938, link_broadcast_resend) {
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    const gaddr st = 0x095236C0u, q = 0x095236E0u;
    c->r[16] = st;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    if (S8(st + 14) != 0 && *W(st) == 0) {
        c->r[17] = q;
        if (*W(q + 28) != 0) {
            c->r[18] = 0x08A40000u;
            if (*W(0x08A3EB88u) != 0) {
                ARG(0) = *W(0x08A3EB88u); ARG(1) = 1; ARG(2) = 0;
                c->r[31] = 0x088E199Cu; hle_sceKernelWaitSema(c);
            }
            if (*W(q + 28) != 0) {
                if (*W(q + 24) != 0) {
                    call_on(c, *W(q + 24), 0x088E19BCu, f_088DC2C4);
                    *W(q + 24) = 0;
                }
                *W(q + 24) = *W(q + 28);
                *H(q + 34) = (uint16_t)S16(q + 38);
                *H(q + 36) = 0; *H(q + 32) = 0;
                *W(q + 28) = 0;
            }
            uint32_t sema = *W(0x08A3EB88u);
            c->r[18] = sema;
            if (sema != 0) {
                ARG(0) = sema; ARG(1) = 1;
                c->r[31] = 0x088E19F0u; hle_sceKernelSignalSema(c);
            }
        }
        gaddr pk = *W(q + 24);
        c->r[18] = pk;
        if (pk == 0) { *W(q + 24) = 0; *H(q + 34) = 0; }
        else if (S16(q + 32) != 0) *H(q + 32) = (uint16_t)(S16(q + 32) - 1);
        else {
            c->r[19] = pk;
            uint32_t len = *W(0x08A3EC50u + ((*B(pk + 9) & 0xF) << 2)) + 4;
            c->r[18] = len;
            c->r[31] = 0x088E1A38u; f_088DD124(c);
            ARG(0) = c->r[2]; ARG(1) = pk + 8; ARG(2) = len; ARG(3) = 0xFFFFFFFFu;
            c->r[31] = 0x088E1A4Cu; f_088DF910(c);
            int32_t r = (int32_t)c->r[2];
            if (r <= 0) { if (r < 0) *W(st) = (uint32_t)r; }
            else if ((uint32_t)r == len) {
                uint32_t k = *H(q + 36);
                *H(q + 32) = (int32_t)k < 3 ? 0x14 : (uint16_t)S16(q + 34);
                *H(q + 36) = (uint16_t)(k + 1);
            }
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
