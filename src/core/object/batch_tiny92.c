/* A select screen's background object start (176 instructions): its place and palette by the page
   (GAME +22's high byte), and the list-0x23 pieces each page shows. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08822F54(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static uint32_t page(void) { return (uint32_t)((int32_t)((uint32_t)S16(GAME + 22) & 0xFF00) >> 8); }

/* One list-0x23 piece (0x08822F54 (0x23, kind)), on and pointing back (+88), kept at its +40 / +44. */
static void piece(Cpu *c, gaddr o, uint32_t kind, uint32_t keep, uint32_t back) {
    call2(c, 0x23, kind, back, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) { *B(n) = 1; *W(n + 88) = o; *W(o + keep) = n; }
}

/* (background): its place (0x08A2E150 / 0x08A2E170 by 0x09525DE3, 4 bytes a page) and palette (0x08A2DB30 /
   0x08A2DB50, 5 lines into 0); for the second table page 0 also its +0xA0 colours into 0x60, and +171 = 1
   on pages 0 / 1 / 2 / 7 (else 0); then its pieces: first table page 3 kind 0 (+40), page 6 kinds 1 / 2
   (+40 / +44); second table pages 3 / 4 / 0 kinds 3 / 4 / 5 (+44). */
SFA_FUNC(088AE2E8, select_background_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 36) = sv[1];
    c->r[17] = GAME;
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    uint32_t h = page();
    *W(sp + 40) = sv[2];
    c->r[18] = 0x09520000u;
    int sel = S8(0x09525DE3u) != 0;
    *W(sp + 44) = sv[3];
    c->r[19] = 1;
    *W(sp + 48) = sv[4]; *W(sp + 52) = ra;
    c->r[20] = 3;
    gaddr e = (sel ? 0x08A2E150u : 0x08A2E170u) + 4 * h;
    *H(o + 18) = (uint16_t)S16(e);
    *H(o + 22) = (uint16_t)S16(e + 2);
    uint32_t pal = *W((sel ? 0x08A2DB30u : 0x08A2DB50u) + (page() << 2));
    ARG(0) = 1; ARG(1) = pal; ARG(2) = 0; ARG(3) = 0; c->r[8] = 5;
    c->r[31] = 0x088AE3DCu; f_0880AEA8(c);
    if (S8(0x09525DE3u) != 0) *B(o + 171) = 0;
    else {
        if (page() == 0) {
            ARG(0) = 1; ARG(1) = *W(0x08A2DB50u) + 0xA0; ARG(2) = 0; ARG(3) = 0x60; c->r[8] = 1;
            c->r[31] = 0x088AE418u; f_0880AEA8(c);
        }
        if (S8(0x09525DE3u) != 0) *B(o + 171) = 0;
        else {
            uint32_t p = page();
            *B(o + 171) = (p == 0 || p == 1 || p == 2 || p == 7) ? 1 : 0;
        }
    }
    uint32_t k = page();
    c->r[17] = k;
    if (S8(0x09525DE3u) != 0) {
        if (k == 3) piece(c, o, 0, 40, 0x088AE494u);
        else if (k == 6) {
            c->r[17] = 1;
            piece(c, o, 1, 40, 0x088AE4C4u);
            piece(c, o, 2, 44, 0x088AE4E8u);
        }
    } else {
        if (k == 3) piece(c, o, 3, 44, 0x088AE518u);
        else if (k == 4) piece(c, o, 4, 44, 0x088AE544u);
        else if (k == 0) piece(c, o, 5, 44, 0x088AE570u);
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
