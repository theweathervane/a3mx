/* The link's start-up handshake (224 instructions, called each frame until the link is up): three
   magic words sent and waited for in turn, then every peer counted in. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08804ECC(Cpu *c); void f_08804F50(Cpu *c); void f_088DD3DC(Cpu *c); void f_088E0C6C(Cpu *c);
void f_088E0CC8(Cpu *c); void f_088E0F98(Cpu *c); void f_088E11C0(Cpu *c); void f_088E12FC(Cpu *c);

/* The word sent on the link's queue (+0xAC, to its +228); 1 when sent. */
static int send_word(Cpu *c, gaddr l, gaddr at, uint32_t word, uint32_t back) {
    *W(at) = word;
    ARG(0) = l + 0xAC; ARG(1) = at; ARG(2) = 0; ARG(3) = *W(l + 228);
    c->r[31] = back; f_088E0CC8(c);
    return (int32_t)c->r[2] > 0;
}

/* Words read from the link's queue until none is left (0x088E11C0 < 0); the expected one moves the step
   to `next` (and counts +42 when asked). The scratch word after the slot is overwritten each time. */
static void wait_word(Cpu *c, gaddr o, gaddr l, gaddr st, gaddr at, uint32_t word, uint16_t next, int count,
                      uint32_t back) {
    for (;;) {
        ARG(0) = l + 0xAC; ARG(1) = at; ARG(2) = 0;
        c->r[31] = back; f_088E11C0(c);
        if ((int32_t)c->r[2] < 0) return;
        int hit = *W(at) == word;
        *W(at + 4) = word;
        if (hit) {
            *H(st + 4) = next;
            if (count) *B(o + 42) = (uint8_t)(*B(o + 42) + 1);
        }
    }
}

/* (link state, link): 0x088E0C6C (0), then by step (+0xD4 +4): 0 blank inputs fed to the three pads, +4 -
   +12 cleared, the +10 toggle flipped and the step moved on 1 or 2; 1 send 0xE198DE71 (to 4); 2 wait for it
   (to 3); 3 send 0x53201336 (to 6); 4 wait for it (to 5; 240 empty waits back to 1); 5 send 0x7F035D72 (to
   7); 6 wait for it (to 8, a peer counted at +42; empty waits counted at +8); 7 once 0x088E0F98 of +228 is
   clear, a peer counted (to 8); 8 once every peer (0x088DD3DC) is counted, on; past that everything reset
   (0x08804ECC, 0x088E12FC) for the next attempt. +44 = 1 and +36 returned. */
SFA_FUNC(088E3798, link_handshake_step) {
    gaddr o = ARG(0), l = ARG(1);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 56) = sv[1];
    c->r[17] = l;
    *W(sp + 52) = sv[0]; *W(sp + 60) = sv[2];
    c->r[16] = o;
    gaddr st = l + 0xD4;
    c->r[18] = st;
    *W(sp + 64) = sv[3]; *W(sp + 68) = sv[4]; *W(sp + 72) = sv[5]; *W(sp + 76) = ra;
    ARG(0) = 0; c->r[31] = 0x088E37CCu; f_088E0C6C(c);
    uint32_t k = *H(st + 4);
    ARG(0) = k << 2;
    switch (k) {
    case 0: {
        const gaddr rec = 0x08E75C28u, pads = 0x08E75BA0u;
        c->r[17] = rec; c->r[19] = 1; c->r[20] = pads;
        *H(rec) = 0; *H(rec + 2) = 1; *W(rec + 4) = 0;
        ARG(0) = pads; ARG(1) = rec; c->r[31] = 0x088E3820u; f_08804F50(c);
        *H(rec + 8) = 1; *H(rec + 10) = 1; *W(rec + 12) = 0;
        ARG(0) = pads + 0x2C; ARG(1) = rec + 8; c->r[31] = 0x088E3838u; f_08804F50(c);
        *H(rec + 16) = 2; *H(rec + 18) = 1; *W(rec + 20) = 0;
        ARG(0) = pads + 0x58; ARG(1) = rec + 0x10; c->r[31] = 0x088E3854u; f_08804F50(c);
        *W(o + 4) = 0; *W(o + 8) = 0; *W(o + 12) = 0;
        uint32_t t = *B(st + 10);
        *H(st + 8) = 0;
        *B(st + 10) = (uint8_t)(t + 1);
        *B(st + 10) = (uint8_t)(*B(st + 10) & 1);
        *H(st + 4) = (uint16_t)(*H(st + 4) + 1);
        if (*B(st + 10) != 0) *H(st + 4) = (uint16_t)(*H(st + 4) + 1);
        break;
    }
    case 1:
        if (send_word(c, l, sp + 0x10, 0xE198DE71u, 0x088E38C0u)) *H(st + 4) = 4;
        break;
    case 2:
        c->r[21] = l + 0xAC; c->r[17] = 0xE198DE71u; c->r[19] = sp + 0x14; c->r[20] = 3;
        wait_word(c, o, l, st, sp + 0x14, 0xE198DE71u, 3, 0, 0x088E38F8u);
        break;
    case 3:
        if (send_word(c, l, sp + 0x1C, 0x53201336u, 0x088E3934u)) { *H(o + 30) = 0; *H(st + 4) = 6; }
        break;
    case 4:
        c->r[21] = l + 0xAC; c->r[17] = 0x53201336u; c->r[19] = sp + 0x20; c->r[20] = 5;
        wait_word(c, o, l, st, sp + 0x20, 0x53201336u, 5, 0, 0x088E3970u);
        *H(st + 8) = (uint16_t)(S16(st + 8) + 1);
        if (S16(st + 8) >= 241) { *H(st + 4) = 1; *H(st + 8) = 0; }
        break;
    case 5:
        if (send_word(c, l, sp + 0x28, 0x7F035D72u, 0x088E39D4u)) { *H(o + 30) = 0; *H(st + 4) = 7; }
        break;
    case 6:
        c->r[17] = l + 0xAC; c->r[20] = sp + 0x2C; c->r[21] = 8; c->r[19] = 0x7F035D72u;
        wait_word(c, o, l, st, sp + 0x2C, 0x7F035D72u, 8, 1, 0x088E3A10u);
        *H(st + 8) = (uint16_t)(S16(st + 8) + 1);
        break;
    case 7:
        ARG(0) = *W(l + 228);
        c->r[31] = 0x088E3A4Cu; f_088E0F98(c);
        if (c->r[2] == 0) { *B(o + 42) = (uint8_t)(*B(o + 42) + 1); *H(st + 4) = 8; }
        break;
    case 8: {
        uint32_t n = *B(o + 42);
        c->r[17] = n;
        c->r[31] = 0x088E3A74u; f_088DD3DC(c);
        if (n == c->r[2]) *H(st + 4) = (uint16_t)(*H(st + 4) + 1);
        break;
    }
    default:
        c->r[31] = 0x088E3A94u; f_08804ECC(c);
        *W(o) = 0; *W(o + 12) = 0; *W(o + 8) = 0; *W(o + 4) = 0;
        *B(o + 46) = 0;
        c->r[31] = 0x088E3AACu; f_088E12FC(c);
        *B(o + 43) = 0;
        *B(o + 32) = (uint8_t)S8(0x08A3EB96u);
        *H(o + 30) = 0; *B(o + 42) = 0;
        uint32_t h = *H(st);
        *H(st + 2) = 0;
        *H(st) = (uint16_t)(h + 1);
        *H(st + 6) = 0; *H(st + 4) = 0;
        *H(o + 30) = 0; *B(o + 33) = 0; *B(o + 42) = 0;
        break;
    }
    c->r[2] = (uint32_t)S8(o + 36);
    *B(o + 44) = 1;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x50;
}
