/* The HUD's super gauge each frame (698 instructions, the most-called HUD routine): the stock count
   and its flags, the three 48-unit bar segments with their flash while a super is under way, the
   dimming, and the gauge's colour row. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define PALS 0x092D9E00u
#define HUD  0x092E51C0u

void f_0880AEA8(Cpu *c);

static void seg_colour(gaddr q, uint8_t r, uint8_t g, uint8_t b) {
    *B(q + 48) = b; *B(q + 42) = g; *B(q + 36) = r;
    uint8_t cr = *B(q + 36), cg = *B(q + 42), cb = *B(q + 48);
    *B(q + 39) = cr; *B(q + 38) = cr; *B(q + 37) = cr;
    *B(q + 45) = cg; *B(q + 44) = cg; *B(q + 43) = cg;
    *B(q + 51) = cb; *B(q + 50) = cb; *B(q + 49) = cb;
}
static void seg_halve(gaddr q) {
    uint8_t b = *B(q + 48), g = *B(q + 42), r = *B(q + 36);
    seg_colour(q, (uint8_t)(r >> 1), (uint8_t)(g >> 1), (uint8_t)(b >> 1));
}
/* The flash while a super runs: white on blinking frames (HUD +9), the segment shown full (0x30). */
static void seg_flash(gaddr q) {
    if (*B(HUD + 9) & 1) {
        *B(q + 48) = 0xFF; *B(q + 42) = 0xFF; *B(q + 36) = 0xFF;
        *H(q + 14) = 0x30;
        uint8_t cr = *B(q + 36), cg = *B(q + 42), cb = *B(q + 48);
        *B(q + 39) = cr; *B(q + 38) = cr; *B(q + 37) = cr;
        *B(q + 45) = cg; *B(q + 44) = cg; *B(q + 43) = cg;
        *B(q + 51) = cb; *B(q + 50) = cb; *B(q + 49) = cb;
    } else *H(q + 14) = 0x30;
}
static int dimmed(gaddr f) {
    return S8(*W(f + 92) + 237) != 0 || S8(GAME + 89) != 0 || S8(GAME + 145) != 0;
}

/* (-, fighter, side): the flash counter (0x08B02712 + side) takes the gauge level (at most 0x60) when the
   fighter's super (+237) starts and clears after; the stock flags (HUD +0x4B4 / +0xAF4 +60: 0x154 / 0x156
   blinking at three stocks by HUD +6, else 0x8000; HUD +0x4F4 / +0xB34 +60: 0x159 + stocks, or 0x8000 with
   none); the three segments (HUD +0x534 / +0x574 / +0x5B4, side 1 +0xB74 / +0xBB4 / +0xBF4) filled by the
   level (48 each), green, flashing while the super's level lasts, dimmed while the other fighter's +237 or
   GAME +89 / +145; side 0's segments placed by their height (+6 = 0xB3 / 0x83 / 0x53 - height); then the
   colour row (0x18, or 0x18 + stocks on blinking frames, 0x18 during the super) from PALS +64 into line
   0xF / 0x11. */
SFA_FUNC(0889086C, hud_super_gauge) {
    gaddr f = ARG(1);
    uint32_t side = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr fl = 0x08B02712u + side;
    c->r[8] = 0x08B00000u; c->r[7] = 0x08B02712u; c->r[9] = HUD;
    *W(c->r[29] + 32) = ra;
    if (S8(f + 237) == 0) {
        if (*B(fl) != 0) *B(fl) = 0;
    } else if (*B(fl) == 0) {
        *B(fl) = (uint8_t)S16(f + 366);
        if (!((int32_t)*B(fl) < 97)) *B(fl) = 0x60;
    }
    int32_t L = S16(f + 366);
    uint32_t n = (L >= 48) + (L >= 96) + (L >= 144);
    uint32_t row = 0x18;
    if (n != 0 && (*B(HUD + 9) & 1)) row = (n + 0x18) & 0xFFFF;
    if (S8(f + 237) != 0) row = 0x18;
    row = (row << 3) << 2;
    gaddr o1 = side != 0 ? HUD + 0xAF4 : HUD + 0x4B4;
    if (n != 3) *H(o1 + 60) = 0x8000;
    else {
        uint32_t v = *H(HUD + 6);
        if (v & 0x3F) *H(o1 + 60) = (v & 0x40) ? 0x154 : 0x156;
    }
    gaddr o2 = side != 0 ? HUD + 0xB34 : HUD + 0x4F4;
    *H(o2 + 60) = n == 0 ? 0x8000 : (uint16_t)(n + 0x159);

    /* Segment 0: levels 0 - 48. */
    L = S16(f + 366);
    gaddr q = side != 0 ? HUD + 0xB74 : HUD + 0x534;
    *H(q + 14) = (uint16_t)(L < 49 ? L : 0x30);
    seg_colour(q, 0, 0xC0, 0);
    if (S8(f + 237) != 0 && (int32_t)*B(fl) < 48) seg_flash(q);
    if (dimmed(f)) seg_halve(q);
    if (side == 0) *H(q + 6) = (uint16_t)(0xB3 - S16(q + 14));

    /* Segment 1: levels 48 - 96. */
    L = S16(f + 366);
    q = side != 0 ? HUD + 0xBB4 : HUD + 0x574;
    *H(q + 14) = (uint16_t)(L < 49 ? 0 : (L < 97 ? L - 0x30 : 0x30));
    seg_colour(q, 0, 0xC0, 0);
    if (S8(f + 237) != 0) {
        int32_t s = S8(f + 194);
        int32_t x = (int32_t)*B(fl) + 48 * s;
        if (!(x < 48)) {
            int32_t y = ((uint32_t)s < 1u ? 48 : 0) + (int32_t)*B(fl);
            if (y != 0x90) seg_flash(q);
        }
    }
    if (dimmed(f)) seg_halve(q);
    if (side == 0) {
        *H(q + 6) = (uint16_t)(0x80 - S16(q + 14));
        *H(q + 6) = (uint16_t)(S16(q + 6) + 3);
    }

    /* Segment 2: levels 96 - 144. */
    L = S16(f + 366);
    q = side != 0 ? HUD + 0xBF4 : HUD + 0x5B4;
    *H(q + 14) = (uint16_t)(L < 97 ? 0 : L - 0x60);
    seg_colour(q, 0, 0xC0, 0);
    if (S8(f + 237) != 0 && !((int32_t)*B(fl) + 48 * S8(f + 194) < 96)) seg_flash(q);
    if (dimmed(f)) seg_halve(q);
    if (side == 0) {
        *H(q + 6) = (uint16_t)(0x50 - S16(q + 14));
        *H(q + 6) = (uint16_t)(S16(q + 6) + 3);
    }

    c->r[10] = row; c->r[11] = q;
    ARG(0) = 0; ARG(1) = *W(PALS + 64) + row; ARG(2) = 0; ARG(3) = side != 0 ? 0x11 : 0xF; c->r[8] = 1;
    c->r[31] = side != 0 ? 0x08891348u : 0x08891324u;
    f_0880AEA8(c);
    c->r[31] = ra;
    c->r[29] += 0x30;
}
