/* The HUD's super gauge colours each frame (50 instructions, 7,629 calls): the gauge level's flash
   row and the bar's tint from the palette colours, for each side. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define PALS 0x092D9E00u

void f_0880AEA8(Cpu *c);

/* Splits a 15-bit colour into the bar's three channel bytes (+36 / +42 / +48, each copied to the next
   three). */
static void bar_colour(gaddr q, uint32_t col) {
    *B(q + 48) = (uint8_t)((int32_t)(col & 0x7C00) >> 7);
    *B(q + 42) = (uint8_t)((int32_t)(col & 0x3E0) >> 2);
    *B(q + 36) = (uint8_t)((col & 0x1F) << 3);
}
static void bar_spread(gaddr q) {
    uint8_t r = *B(q + 36), g = *B(q + 42), b = *B(q + 48);
    *B(q + 39) = r; *B(q + 38) = r; *B(q + 37) = r;
    *B(q + 45) = g; *B(q + 44) = g; *B(q + 43) = g;
    *B(q + 51) = b; *B(q + 50) = b; *B(q + 49) = b;
}
static void bar_halve(gaddr q) {
    *B(q + 48) = (uint8_t)(*B(q + 48) >> 1);
    *B(q + 42) = (uint8_t)(*B(q + 42) >> 1);
    *B(q + 36) = (uint8_t)(*B(q + 36) >> 1);
    bar_spread(q);
}

/* (-, fighter, side): the gauge's flag (0x092E56F0 / 0x092E5D30: 0x8000 under a level, else 0x331 / 0x332)
   and the flash row (cycling 0xE - 0x16 by the frame byte 0x092E51C8, 0xE at rest) loaded from PALS +64
   into line 0xF / 0x11 (0x0880AEA8); the bar (0x092E55B4 / 0x092E5BF4: level at +14, colour from
   0x092DFB7C / 0x092DFBBC) lit white on blinking frames (0x092E51C9) of a fighter's +237, dimmed while
   the other's +237 or GAME +89 / +145; side 0's height +6 = 0xB1 - level, side 1's 0xCF. */
SFA_FUNC(08890440, hud_super_gauge_colours) {
    gaddr f = ARG(1);
    uint32_t side = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    const gaddr hud = 0x092E51C0u, pal = 0x092DF980u;
    c->r[7] = hud; c->r[8] = pal;
    *W(c->r[29] + 32) = ra;
    gaddr flag = side != 0 ? hud + 0xB34 : hud + 0x4F4;
    uint32_t row;
    if (S16(f + 366) < 144) {
        *H(flag + 60) = 0x8000;
        row = 0x1C0;
    } else {
        *H(flag + 60) = side != 0 ? 0x332 : 0x331;
        uint32_t t = *B(hud + 8) & 7;
        uint32_t idx = t < 4 ? ((t + 0xE) & 0xFFFF) : ((0x16 - t) & 0xFFFF);
        row = idx << 5;
    }
    int32_t level = S16(f + 366);
    gaddr q;
    if (side == 0) {
        q = hud + 0x3F4;
        *H(q + 14) = (uint16_t)level;
        bar_colour(q, *H(pal + 508));
        bar_spread(q);
        if (S8(f + 237) != 0) {
            *H(q + 14) = 0x90;
            if (*B(hud + 9) & 1) {
                *B(q + 48) = 0xFF; *B(q + 42) = 0xFF; *B(q + 36) = 0xFF;
                *H(q + 6) = (uint16_t)(0xB0 - S16(q + 14));
                bar_spread(q);
            }
        }
        gaddr o = *W(f + 92);
        if (S8(o + 237) != 0 || S8(GAME + 89) != 0 || S8(GAME + 145) != 0) bar_halve(q);
        *H(q + 6) = (uint16_t)(0xB1 - S16(q + 14));
    } else {
        q = hud + 0xA34;
        *H(q + 14) = (uint16_t)level;
        bar_colour(q, *H(pal + 572));
        bar_spread(q);
        if (S8(f + 237) != 0) {
            if (*B(hud + 9) & 1) {
                *B(q + 48) = 0xFF; *B(q + 42) = 0xFF; *B(q + 36) = 0xFF;
                *H(q + 14) = 0x90;
                bar_spread(q);
            } else *H(q + 14) = 0x90;
        }
        gaddr o = *W(f + 92);
        if (S8(o + 237) != 0 || S8(GAME + 89) != 0 || S8(GAME + 145) != 0) bar_halve(q);
        *H(q + 6) = 0xCF;
    }
    c->r[10] = *W(PALS + 64) + row;
    c->r[11] = row;
    ARG(0) = 0; ARG(1) = c->r[10]; ARG(2) = 0; ARG(3) = side != 0 ? 0x11 : 0xF; c->r[8] = 1;
    c->r[31] = side != 0 ? 0x08890860u : 0x08890840u;
    f_0880AEA8(c);
    c->r[31] = ra;
    c->r[29] += 0x30;
}
