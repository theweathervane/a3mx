/* A stage's start (258 instructions): the stage handlers' table filled, its colours copied, its
   state cleared, its data entries and lists resolved, its music and layers set going. Offsets stay
   numbers until something shows what they are.
 */
#include "../object/step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define PALS 0x092D9E00u

void f_088199CC(Cpu *c); void f_08821F9C(Cpu *c); void f_0882E0D0(Cpu *c); void f_089DE2C0(Cpu *c);
void f_089DE9F0(Cpu *c); void f_089E11C8(Cpu *c);

/* Resolves a list of halfword indices (0xFFFF ends it, at most 0x1000) into pointers base + index *
   size. */
static void resolve(gaddr dst, gaddr idx, gaddr base, uint32_t size) {
    for (uint32_t i = 0; i < 0x1000; i++) {
        uint32_t h = *H(idx + 2 * i);
        if (h == 0xFFFF) break;
        *W(dst + 4 * i) = base + h * size;
    }
}

/* (a, b, stage): the stage handlers (0x08A0FDC8 +288 - +320, 0x08A25AD4, 0x08A259F8) set, the stage's
   0x60 colours (0x08AB76F0) copied into line 0x20 and marked changed, 0x08821F9C; b's +6 = 1, a's +2
   counted; the stage's state cleared (+2 - +6, +24 - +33, +36 - +58, +98 - +109, +126 - +134 (0xFF; +134 =
   0xA), +158 / +160, +170 / +188 / +206, +224 - +264, +312); its entry (0x08AB74B0 by +0: +268 / +288 -
   +300 and 0x088199CC on its sixth word); +60 / +284 / +304 / +276 from the kept buffers; its two lists
   resolved (10-byte records into 0x08B04334 as +272, 8-byte into 0x08B08334 as +280); +316 = GAME +53 / 2;
   +50 / +56 from 0x08AB7582; the layer flags 0x0951F950 (+28 / +180 / +332 = 4 / 2 / 6); 0x089DE9F0 for
   stages without 0x08AB7558; its +292 colours into line 0x3F; 0x0882E0D0 and 0x089DE2C0 (a, b, stage). */
SFA_FUNC(089DF21C, stage_start) {
    gaddr a = ARG(0), b = ARG(1), st = ARG(2);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1];
    c->r[17] = b; c->r[16] = a;
    const gaddr T = 0x08A0FDC8u;
    static const uint32_t handlers[9] = { 0x089CD01Cu, 0x089CD248u, 0x089CF124u, 0x089CF574u, 0x089D948Cu,
        0x089D9594u, 0x089CF384u, 0x089D9994u, 0x089DC1A8u };
    for (int i = 0; i < 9; i++) *W(T + 288 + 4 * (uint32_t)i) = handlers[i];
    *W(0x08A25AA0u + 52) = 0x089DD9F0u;
    *W(sp + 28) = sv[3];
    *W(sp + 24) = sv[2];
    *W(0x08A259F8u) = 0x089DDE90u;
    c->r[18] = st;
    c->r[19] = 0x092DF980u;
    *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5]; *W(sp + 40) = ra;
    ARG(0) = 0x092DF980u + 0x400; ARG(1) = 0x08AB76F0u; ARG(2) = 0x60;
    c->r[31] = 0x089DF2FCu; f_089E11C8(c);
    c->r[20] = 1; c->r[21] = 0x092E0000u;
    *W(0x092E0980u) = 1;
    c->r[31] = 0x089DF30Cu; f_08821F9C(c);
    *B(b + 6) = 1;
    *H(a + 2) = (uint16_t)(*H(a + 2) + 1);
    *B(a + 89) = 0;
    *H(st + 6) = 0; *H(st + 4) = 0; *H(st + 2) = 0;
    *B(a + 94) = 0;
    *H(a + 18) = 0; *H(a + 6) = 0; *H(a + 4) = 0;
    *B(a + 138) = 0;
    *B(b + 15) = 0; *B(b + 37) = 0;
    *B(st + 49) = 0;
    for (uint32_t o = 98; o <= 104; o++) *B(st + o) = 0;
    *B(st + 58) = 0; *B(st + 52) = 0;
    *H(st + 46) = 0; *H(st + 160) = 0; *H(st + 158) = 0;
    *B(st + 40) = 0; *B(st + 36) = 0; *B(st + 32) = 0;
    *H(st + 254) = 0; *H(st + 244) = 0; *H(st + 234) = 0; *H(st + 224) = 0;
    *H(st + 264) = 0;
    *H(st + 132) = 0xFF; *H(st + 130) = 0xFF; *H(st + 128) = 0xFF; *H(st + 126) = 0xFF;
    *H(st + 30) = 0xFF; *H(st + 28) = 0xFF; *H(st + 26) = 0xFF; *H(st + 24) = 0xFF;
    *B(st + 206) = 0; *B(st + 188) = 0; *B(st + 170) = 0;
    *B(st + 109) = 0; *B(st + 106) = 0; *B(st + 105) = 0; *B(st + 107) = 0;
    *B(st + 41) = 0; *B(st + 37) = 0; *B(st + 33) = 0;
    *W(st + 312) = 0;
    gaddr e = *W(0x08AB74B0u + ((uint32_t)S16(st) << 2));
    *W(st + 268) = *W(e);
    *W(st + 288) = *W(e + 4);
    *W(st + 292) = *W(e + 8);
    *W(st + 296) = *W(e + 12);
    *W(st + 300) = *W(e + 16);
    ARG(0) = *W(e + 20);
    c->r[31] = 0x089DF448u; f_088199CC(c);
    const gaddr q = 0x08A0584Cu;
    *W(st + 60) = *W(q);
    *W(st + 284) = *W(PALS + 100);
    *W(st + 304) = *W(0x08A05834u + 12);
    *W(st + 276) = *W(q + 20);
    resolve(0x08B04334u, *W(q + 12), *W(q + 4), 10);
    *W(st + 272) = 0x08B04334u;
    resolve(0x08B08334u, *W(q + 16), *W(q + 8), 8);
    int32_t k = S16(st);
    *W(st + 280) = 0x08B08334u;
    *W(st + 316) = (uint32_t)((int32_t)S8(GAME + 53) >> 1);
    gaddr e2 = 0x08AB7582u + 6 * (uint32_t)k;
    *H(st + 54) = 0;
    *H(st + 50) = (uint16_t)S16(e2);
    *B(st + 53) = 0;
    *H(st + 56) = (uint16_t)S16(e2 + 4);
    *B(st + 52) = 0;
    const gaddr lf = 0x0951F950u;
    *B(lf) = 0; *B(lf + 152) = 0; *B(lf + 304) = 0;
    *B(lf + 28) = 4; *B(lf + 180) = 2; *B(lf + 332) = 6;
    *H(st + 134) = 0xA;
    if (S8(0x08AB7558u + (uint32_t)S16(st)) == 0) {
        *W(st + 308) = 0;
        ARG(0) = st; c->r[31] = 0x089DF5BCu; f_089DE9F0(c);
    }
    gaddr p = *W(st + 292);
    if (p != 0 && *H(p) != 0) {
        ARG(0) = 0x092DF980u + 0x7E0; ARG(1) = *W(p); ARG(2) = 0x10;
        c->r[31] = 0x089DF5E4u; f_089E11C8(c);
        *W(0x092E0980u) = 1;
    }
    c->r[31] = 0x089DF5F0u; f_0882E0D0(c);
    ARG(0) = a; ARG(1) = b; ARG(2) = st;
    c->r[31] = 0x089DF600u; f_089DE2C0(c);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
