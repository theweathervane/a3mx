/* A three-item menu (0x088E9AE0): the cursor (+22) moved by the pad's up / down bits, picked on the
   button, a countdown (+20) that picks for you, and the three item labels drawn by 0x0880A6CC, the
   cursor's one in another colour. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D5E7C(Cpu *c); void f_088E348C(Cpu *c); void f_088D55F8(Cpu *c); void f_088E376C(Cpu *c);
void f_088D561C(Cpu *c); void f_0880A6CC(Cpu *c);

/* (menu): 0x088D5E7C says whether it is idle. Idle: +20 counts down and at 0 the menu is picked
   (0x088E348C (1), +20 cleared, cursor 2, +4 on). Otherwise +20 counts down too (at 0 cursor 2 and
   picked); before then the pad (0x08E75BA0 +12) moves the cursor up (bit 3) or down (bit 2) with the
   sound 0x088D55F8, or 0x088E376C and the button (bit 7, sound 0x088D561C) picks; the cursor wraps over
   0..2; a pick sets +4 on, +20 cleared and 0x088E348C (1). Then each label from 0x08A3F340 down is drawn
   at y 0x90 (colour by +114 mode 6: 0xC4 / 0xB7 / 0xAA, else 0xBD / 0xAD / 0x9D; palette 3 under the
   cursor, else 9). In mode 6 the title 0x08A014E0 is drawn too. A 0x40 frame keeping s0..s5. */
SFA_FUNC(088E9AE0, menu3_cursor) {
    gaddr o = ARG(0);
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 6; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 56) = ra;
    c->r[16] = o;
    c->r[31] = 0x088E9B08u; f_088D5E7C(c);
    uint32_t t = (uint32_t)S16(o + 20) - 1;
    c->r[17] = t; c->r[19] = 6;
    if (c->r[2] != 0) {
        *H(o + 20) = (uint16_t)t;
        if ((int16_t)t == 0) {
            ARG(0) = o; ARG(1) = 1; c->r[31] = 0x088E9B34u; f_088E348C(c);
            *H(o + 20) = 0; *H(o + 22) = 2;
            *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        }
    } else {
        *H(o + 20) = (uint16_t)t;
        c->r[17] = 0x08A3F338u; c->r[20] = 0;
        int picked = 0;
        if ((int16_t)t == 0) { *H(o + 22) = 2; picked = 1; }
        else {
            const gaddr pad = 0x08E75BA0u;
            c->r[18] = pad;
            uint32_t w = *W(pad + 12);
            ARG(0) = w & 4; ARG(1) = w & 8;
            if (w & 8) {
                c->r[31] = 0x088E9B9Cu; f_088D55F8(c);
                *H(o + 22) = (uint16_t)(S16(o + 22) - 1);
            } else if (w & 4) {
                c->r[31] = 0x088E9BBCu; f_088D55F8(c);
                *H(o + 22) = (uint16_t)(S16(o + 22) + 1);
            } else {
                c->r[31] = 0x088E9BD4u; f_088E376C(c);
                if (c->r[2] != 0) {
                    uint32_t b = *W(pad + 12) & 0x80;
                    ARG(0) = b;
                    if (b) { c->r[31] = 0x088E9BF8u; f_088D561C(c); picked = 1; }
                }
            }
        }
        c->r[20] = (uint32_t)picked;
        if (S16(o + 22) >= 3) *H(o + 22) = 0;
        if (S16(o + 22) < 0) *H(o + 22) = 2;
        c->r[18] = 0x08A3F338u;
        if (picked) {
            *H(o + 20) = 0;
            *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
            ARG(0) = o; ARG(1) = 1; c->r[31] = 0x088E9C40u; f_088E348C(c);
        }
        int32_t i = 2; gaddr lab = 0x08A3F340u; uint32_t on = 0xC4, off = 0xBD;
        for (;;) {
            c->r[17] = (uint32_t)i; c->r[18] = lab; c->r[20] = on; c->r[21] = off;
            uint32_t col = S8(o + 114) == 6 ? on : off;
            uint32_t pal = S16(o + 22) == i ? 3 : 9;
            ARG(0) = *W(lab); ARG(1) = 3; ARG(2) = 0x90; ARG(3) = col; ARG(4) = pal;
            c->r[31] = 0x088E9C8Cu; f_0880A6CC(c);
            i--; lab -= 4; on -= 13; off -= 16;
            c->r[17] = (uint32_t)i; c->r[18] = lab; c->r[20] = on; c->r[21] = off;
            if (i < 0) break;
        }
    }
    if (S8(o + 114) == 6) {
        ARG(0) = 0x08A014E0u; ARG(1) = 2; ARG(2) = 0x58; ARG(3) = 5; ARG(4) = 0xB;
        c->r[31] = 0x088E9CC4u; f_0880A6CC(c);
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra; c->r[29] += 0x40;
}
