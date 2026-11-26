/* Network functions the link scenarios reach, 13 - 72 instructions: the link shut down from its worker,
   the per-frame round of peer sends, the link session left and joined. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E0C48(Cpu *c); void f_088E1444(Cpu *c); void f_088DD3DC(Cpu *c); void f_088DD418(Cpu *c);
void f_088E1EF8(Cpu *c); void f_08804ECC(Cpu *c); void f_0882134C(Cpu *c); void f_088D5E44(Cpu *c);
void f_088D5ECC(Cpu *c); void f_088E0AC0(Cpu *c);
void hle_sceKernelGetThreadId(Cpu *c); void hle_sceNetFreeThreadinfo(Cpu *c);

/* Both busy flags cleared with the worker stopped (0x088E0C48, 0x088E1444), the caller's thread info freed. */
SFA_FUNC(088E1EC4, link_shutdown) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088E1ED4u; f_088E0C48(c);
    c->r[31] = 0x088E1EDCu; f_088E1444(c);
    c->r[31] = 0x088E1EE4u; hle_sceKernelGetThreadId(c);
    ARG(0) = c->r[2]; c->r[31] = 0x088E1EECu; hle_sceNetFreeThreadinfo(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* While the link (0x095236C0) runs (+13) without error: the peer cursor 0x08A3EB8C (started from the last
   peer, 0x088DD3DC) steps (0x088DD418); the peer it reaches sends (0x088E1EF8 of its +228), else it wraps
   to 0. */
SFA_FUNC(088E087C, link_send_round) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    const gaddr L = 0x095236C0u, cur = 0x08A3EB8Cu;
    ARG(0) = L;
    if (S8(L + 13) != 0) {
        ARG(0) = *W(L);
        c->r[16] = 0x08A40000u;
        if (ARG(0) == 0) {
            if (*W(cur) == 0) {
                ARG(0) = 0;
                c->r[31] = 0x088E08BCu; f_088DD3DC(c);
                *W(cur) = c->r[2];
            }
            ARG(0) = cur; c->r[31] = 0x088E08C8u; f_088DD418(c);
            gaddr p = c->r[2];
            if (p == 0) *W(cur) = 0;
            else { ARG(0) = *W(p + 228); c->r[31] = 0x088E08DCu; f_088E1EF8(c); }
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The link's inputs and pad records cleared (08804ECC twice), the shared link 0x09523920's words +0 -
   +12 and +46 cleared and 0x0882134C on its +24. */
static void clear_link_inputs(Cpu *c, gaddr fighter_side, uint32_t back1, gaddr L2, uint32_t back2) {
    *B(fighter_side + 32) = 0; *B(fighter_side + 33) = 0; *B(fighter_side + 59) = 0;
    c->r[31] = back1; f_08804ECC(c);
    *W(L2) = 0; *W(L2 + 12) = 0; *W(L2 + 8) = 0;
    ARG(0) = (uint32_t)(int32_t)S16(L2 + 24);
    *W(L2 + 4) = 0; *B(L2 + 46) = 0;
    c->r[31] = back2; f_0882134C(c);
}

/* (link, side): the link left: +48 / +16 / +26 / +28 / +42 cleared, its worker stopped (0x088E0C48), the
   inputs (+0 - +12, +46) cleared, then the side's and the shared link's (0x09523920), and 0x0882134C on
   the link's +24. */
SFA_FUNC(088E2F80, link_leave) {
    gaddr o = ARG(0), side = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 48) = 0; *W(o + 16) = 0; *H(o + 26) = 0;
    *W(c->r[29] + 20) = s1;
    *H(o + 28) = 0;
    c->r[17] = side;
    *W(c->r[29] + 24) = ra;
    *B(o + 42) = 0;
    c->r[31] = 0x088E2FB0u; f_088E0C48(c);
    c->r[31] = 0x088E2FB8u; f_08804ECC(c);
    *W(o) = 0; *W(o + 12) = 0; *W(o + 8) = 0; *W(o + 4) = 0;
    *B(o + 46) = 0;
    *B(side + 32) = 0; *B(side + 33) = 0; *B(side + 59) = 0;
    c->r[31] = 0x088E2FDCu; f_08804ECC(c);
    const gaddr L2 = 0x09523920u;
    *W(L2) = 0; *W(L2 + 12) = 0; *W(L2 + 8) = 0; *W(L2 + 4) = 0;
    *B(L2 + 46) = 0;
    ARG(0) = (uint32_t)(int32_t)S16(o + 24);
    c->r[31] = 0x088E3000u; f_0882134C(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (side, delay): in a link match (0x088D5E44) that is not over (0x088D5ECC), the shared link (0x09523920)
   joined: the first time (+48 clear) every peer's handshake counts (+0xD4 +0 - +6) cleared, the link's
   state reset (+37 = 1, delay +47) and its worker started (0x088E0AC0); then +48 / +49 set and the
   inputs cleared as on leaving. */
SFA_FUNC(088E336C, link_join) {
    gaddr side = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[0]; *W(sp + 40) = sv[5];
    int8_t delay = (int8_t)ARG(1);
    c->r[21] = (uint32_t)(int32_t)delay;
    c->r[16] = side;
    *W(sp + 24) = sv[1]; *W(sp + 28) = sv[2]; *W(sp + 32) = sv[3]; *W(sp + 36) = sv[4]; *W(sp + 44) = ra;
    c->r[31] = 0x088E339Cu; f_088D5E44(c);
    if (c->r[2] != 0) {
        c->r[31] = 0x088E33ACu; f_088D5ECC(c);
        c->r[20] = 0x09520000u;
        if (c->r[2] == 0) {
            const gaddr L2 = 0x09523920u;
            c->r[19] = L2; c->r[17] = 1;
            ARG(0) = (uint32_t)(int32_t)S8(L2 + 48);
            if (S8(L2 + 48) == 0) {
                c->r[31] = 0x088E33CCu; f_088DD3DC(c);
                *W(sp + 16) = c->r[2];
                c->r[18] = sp + 0x10;
                for (;;) {
                    ARG(0) = sp + 0x10; c->r[31] = 0x088E33DCu; f_088DD418(c);
                    gaddr p = c->r[2];
                    ARG(0) = p + 0xD4;
                    if (p == 0) break;
                    gaddr h = p + 0xD4;
                    *H(h + 2) = 0; *H(h) = 0; *H(h + 6) = 0; *H(h + 4) = 0;
                }
                *B(L2 + 36) = 0; *B(L2 + 37) = 1; *H(L2 + 30) = 0; *B(L2 + 43) = 0; *B(L2 + 34) = 0;
                *H(L2 + 26) = 0; *B(L2 + 35) = 0; *B(L2 + 32) = 0; *B(L2 + 33) = 0; *B(L2 + 42) = 0;
                *W(L2 + 20) = 0; *B(L2 + 46) = 0; *B(L2 + 47) = (uint8_t)delay;
                c->r[31] = 0x088E3434u; f_088E0AC0(c);
            }
            *B(L2 + 48) = 1; *B(L2 + 49) = 1;
            clear_link_inputs(c, side, 0x088E344Cu, L2, 0x088E3468u);
        }
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
