/* A six-item menu (0x088304FC): the cursor at 0x08B01F34 moved by a pad's left / right with auto-repeat
   (0x08B01F35), wrapping over 0..5, and picked on the button. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C4CC(Cpu *c); void f_0880A6CC(Cpu *c); void f_08887AFC(Cpu *c); void f_0880B6CC(Cpu *c);
void f_088D2B5C(Cpu *c);

static void cue(Cpu *c, uint32_t n, uint32_t back) {
    ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c);
}

/* (menu, choice): the pad by +53 bit 0 (0x08E75BA0, else 0x08E75BCC). A press of bit 2 / 3 moves the
   cursor on / back with the cue 0x38 and the repeat at 20 frames; holding them repeats every 6. The cursor
   goes to the choice's +159. On a pick (+12 bits 7 / 15) the choice's +56 is set (3 for items 3 and 4;
   item 5 by +53: 1 with +154 cleared or 2 with +154 = 1), +155 = 1 for item 2, +4 on and the cue 0x39;
   on cancel (bit 8) +159 cleared, +4 on and the cue 0x2F. Then the title 0x089FE160, 0x08887AFC,
   0x0880B6CC and 0x088D2B5C. A 0x40 frame keeping s0..s6. */
SFA_FUNC(088304FC, menu6_cursor) {
    gaddr o = ARG(0), ch = ARG(1);
    uint32_t sv[7] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 7; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 60) = ra;
    const gaddr cur = 0x08B01F34u, rep = 0x08B01F35u;
    gaddr pad = (S8(o + 53) & 1) ? 0x08E75BA0u : 0x08E75BCCu;
    c->r[16] = o; c->r[17] = ch; c->r[18] = pad; c->r[19] = 0x08B00000u;
    c->r[20] = 6; c->r[21] = 5; c->r[22] = 2;
    int32_t v = S8(cur);
    uint32_t w = *W(pad + 12);
    int32_t n;
    if (w & 4) { *B(rep) = 0x14; *B(cur) = (uint8_t)(v + 1); cue(c, 0x38, 0x08830594u); n = S8(cur) + 6; }
    else if (w & 8) { *B(rep) = 0x14; *B(cur) = (uint8_t)(v - 1); cue(c, 0x38, 0x088305C4u); n = S8(cur) + 6; }
    else {
        uint32_t h = *W(pad + 4);
        n = v + 6;
        if (h & 0xC) {
            int32_t t = S8(rep) - 1;
            *B(rep) = (uint8_t)t;
            if ((int8_t)t < 0) {
                *B(rep) = 6;
                if (h & 4) { *B(cur) = (uint8_t)(v + 1); cue(c, 0x38, 0x08830618u); }
                else { *B(cur) = (uint8_t)(v - 1); cue(c, 0x38, 0x08830660u); }
                n = S8(cur) + 6;
            }
        }
    }
    *B(cur) = (uint8_t)n;
    DIV(c, (uint32_t)S8(cur), 6);
    *B(cur) = (uint8_t)c->hi;
    *B(ch + 159) = (uint8_t)S8(cur);
    uint32_t k = *W(pad + 12);
    c->r[18] = k;
    if (k & 0x8080) {
        int32_t it = S8(ch + 159);
        if (it == 3 || it == 4) *H(ch + 56) = 3;
        if (it == 5) {
            int32_t m = S8(o + 53);
            if (m & 1) { *H(ch + 56) = 1; *B(ch + 154) = 0; }
            else if (m & 2) { *H(ch + 56) = 2; *B(ch + 154) = 1; }
        }
        if (it == 2) *B(ch + 155) = 1;
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        cue(c, 0x39, 0x08830710u);
    } else if (k & 0x100) {
        *B(ch + 159) = 0;
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        cue(c, 0x2F, 0x08830744u);
    }
    ARG(0) = 0x089FE160u; ARG(1) = 2; ARG(2) = 0x40; ARG(3) = 5; ARG(4) = 0xB;
    c->r[31] = 0x08830760u; f_0880A6CC(c);
    c->r[31] = 0x08830768u; f_08887AFC(c);
    c->r[31] = 0x08830770u; f_0880B6CC(c);
    c->r[31] = 0x08830778u; f_088D2B5C(c);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[20] = sv[4]; c->r[21] = sv[5]; c->r[22] = sv[6];
    c->r[31] = ra; c->r[29] += 0x40;
}
