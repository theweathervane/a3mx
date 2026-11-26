/* The link's input exchange each frame (214 instructions): the peers' input words gathered, checked
   for being on the same frame, and fed to the three pads (or blank inputs while the delay fills).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08804F50(Cpu *c); void f_088D5E7C(Cpu *c); void f_088DD124(Cpu *c); void f_088DD3DC(Cpu *c);
void f_088E1280(Cpu *c); void f_089F3470(Cpu *c);

static void get_input(Cpu *c, uint32_t who, gaddr to, uint32_t back) {
    ARG(0) = who; ARG(1) = to; ARG(2) = 0;
    c->r[31] = back; f_088E1280(c);
}
static void feed(Cpu *c, gaddr pad, gaddr rec, uint16_t k, uint32_t in, uint32_t back) {
    *H(rec) = k; *H(rec + 2) = 1; *W(rec + 4) = in;
    ARG(0) = pad; ARG(1) = rec;
    c->r[31] = back; f_08804F50(c);
}

/* (link): the three input words (stack, cleared first) taken from the peers (0x088E1280): in a two-player
   link (0x088D5E7C) ours and the other's (0x09523980 +12) in id order, else each peer 0x088DD3DC .. 0; with
   +47 set (and +43 clear) their frame bytes (top byte) must match, else 0 is returned with +46 cleared
   and +16 = 0x81001004 (once). While the start delay lasts (+32 above -0x08A3EB96) blank inputs are fed
   to the pads (0x08804F50 on 0x08E75BA0 / +0x2C / +0x58, records 0x08E75C28 / +8 / +0x10), +32 counted
   down and +4 - +12 / +46 cleared; else the inputs' low halves are fed and kept (+4 / +8 / +12) and +46
   counts up to 2. Then +26 / +35 counted, each pad's bits 16 - 17 into +39 - +41, and 1 returned. */
SFA_FUNC(088E233C, link_input_exchange) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    for (int i = 0; i < 8; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 64) = sv[8]; *W(sp + 68) = ra;
    c->r[16] = o;
    ARG(0) = o;
    c->r[31] = 0x088E2370u; f_088DD3DC(c);
    int32_t n = (int32_t)c->r[2];
    gaddr buf = sp + 0x10;
    c->r[17] = buf; c->r[18] = (uint32_t)n;
    ARG(0) = buf; ARG(1) = 0; ARG(2) = 0xC;
    c->r[31] = 0x088E2388u; f_089F3470(c);
    c->r[31] = 0x088E2390u; f_088D5E7C(c);
    if (c->r[2] != 0) {
        c->r[31] = 0x088E23A0u; f_088DD124(c);
        int32_t me = (int32_t)c->r[2];
        int32_t other = (int32_t)*W(0x09523980u + 12);
        c->r[18] = (uint32_t)me; c->r[19] = (uint32_t)other;
        if (me < other) {
            get_input(c, (uint32_t)other, sp + 0x14, 0x088E23CCu);
            get_input(c, (uint32_t)me, buf, 0x088E23DCu);
        } else {
            get_input(c, (uint32_t)me, sp + 0x14, 0x088E23F4u);
            get_input(c, (uint32_t)other, buf, 0x088E2404u);
        }
    } else if (n >= 0) {
        for (int32_t i = n; i >= 0; i--) {
            c->r[18] = (uint32_t)i; c->r[17] = buf + 4 * (uint32_t)i;
            get_input(c, (uint32_t)i, buf + 4 * (uint32_t)i, 0x088E242Cu);
        }
    }
    if (S8(o + 43) == 0 && S8(o + 47) != 0) {
        ARG(0) = 0x08A40000u;
        c->r[31] = 0x088E2458u; f_088DD3DC(c);
        int32_t m = (int32_t)c->r[2];
        if (m > 0) {
            uint32_t frame = *W(sp + 16) & 0xFF000000u;
            for (int32_t j = m; j > 0; j--) {
                if ((*W(sp + 16 + 4 * (uint32_t)j) & 0xFF000000u) != frame) {
                    *B(o + 46) = 0;
                    if (*W(o + 16) == 0) *W(o + 16) = 0x81001004u;
                    c->r[2] = 0;
                    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
                    c->r[30] = sv[8];
                    c->r[31] = ra;
                    c->r[29] += 0x50;
                    return;
                }
            }
        }
    }
    int32_t t = S8(o + 32);
    const gaddr rec = 0x08E75C28u, pads = 0x08E75BA0u;
    c->r[17] = 1; c->r[18] = rec + 8; c->r[19] = pads + 0x2C; c->r[20] = rec + 0x10; c->r[21] = 2; c->r[22] = pads + 0x58;
    uint32_t h26, b35;
    if (-(int32_t)S8(0x08A3EB96u) < t) {
        *B(o + 32) = (uint8_t)(t - 1);
        *B(o + 46) = 0;
        feed(c, pads, rec, 0, 0, 0x088E2544u);
        feed(c, pads + 0x2C, rec + 8, 1, 0, 0x088E255Cu);
        feed(c, pads + 0x58, rec + 0x10, 2, 0, 0x088E2574u);
        *W(o + 4) = 0; *W(o + 8) = 0; *W(o + 12) = 0;
        h26 = (uint32_t)*H(o + 26) + 1;
        b35 = (uint32_t)*B(o + 35) + 1;
    } else {
        *W(sp + 28) = 2;
        uint32_t a = *W(sp + 16) & 0xFFFF, b = *W(sp + 20) & 0xFFFF, d = *W(sp + 24) & 0xFFFF;
        c->r[23] = a; c->r[30] = b; c->r[21] = d;
        feed(c, pads, rec, 0, a, 0x088E25CCu);
        feed(c, pads + 0x2C, rec + 8, 1, b, 0x088E25E4u);
        feed(c, pads + 0x58, rec + 0x10, (uint16_t)*W(sp + 28), d, 0x088E2600u);
        *W(o + 4) = a;
        int32_t k = S8(o + 46);
        *W(o + 8) = b; *W(o + 12) = d;
        h26 = (uint32_t)*H(o + 26) + 1;
        b35 = (uint32_t)*B(o + 35) + 1;
        if (k < 2) *B(o + 46) = (uint8_t)(k + 1);
    }
    for (int32_t i = 2; i >= 0; i--)
        *B(o + 39 + (uint32_t)i) = (uint8_t)(((int32_t)*W(sp + 16 + 4 * (uint32_t)i) >> 16) & 3);
    *H(o + 26) = (uint16_t)h26;
    *B(o + 35) = (uint8_t)b35;
    c->r[2] = 1;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x50;
}
