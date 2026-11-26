/* A 2D text line drawer the menus reach (167 instructions): each character a textured 8 x 16 quad
   from the font sheet, placed through the screen scale / offset floats. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E1950(Cpu *c);

/* (text): its string (+12: first byte the start x offset, then characters; 0 = a new line whose next
   byte is that line's x offset) drawn from (+6, +10) with colour +16 (the colour table 0x08AE03B8),
   +17 characters, font cell = character + +18 - 1 (32 cells a row of 8 x 16 at v 0x50); each a quad
   of four 16-byte vertices on the stack, x / y through the scales 0x08E759E8 / 0x08E759EC and offsets
   0x08E75A04 / 0x08E75A0C, sent with 0x089E1950 (4, 0x8C, 1, 0). The pen lives at 0x08AD9320. */
SFA_FUNC(08812284, text_line_draw) {
    gaddr t = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x90;
    gaddr sp = c->r[29];
    for (int i = 0; i < 8; i++) *W(sp + 96 + 4 * i) = sv[i];
    *W(sp + 128) = sv[8]; *W(sp + 132) = ra;
    const gaddr pen = 0x08AD9320u;
    c->r[16] = pen;
    gaddr s = *W(t + 12);
    *H(pen + 2) = (uint16_t)S16(t + 10);
    *H(pen) = (uint16_t)(S16(t + 6) + S8(s));
    *W(pen + 44) = *B(t + 16);
    s++;
    c->r[19] = t;
    int32_t i = 0;
    int32_t count = *B(t + 17);
    c->r[20] = s; c->r[21] = 0;
    if (0 < count) {
        c->r[30] = 0x08AE03B8u; c->r[18] = 0xFFFFFFFFu; c->r[23] = 0x8C; c->r[22] = sp + 0x20;
        for (;;) {
            uint32_t ch = (uint32_t)(int32_t)S8(s) & 0xFFFF;
            s++;
            int32_t next;
            if (ch == 0) {
                *H(pen + 2) = (uint16_t)(S16(pen + 2) + 0x10);
                *H(pen) = (uint16_t)(S16(t + 6) + S8(s));
                s++;
                next = i;
            } else {
                c->f[12] = (uint32_t)(int32_t)S16(pen);
                FSET(c, 12, (float)(int32_t)c->f[12]);
                c->f[13] = RW(0x08E759E8u);
                c->f[14] = RW(0x08E75A04u);
                FSET(c, 12, FS(c, 12) * FS(c, 13));
                FSET(c, 12, FS(c, 12) + FS(c, 14));
                c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 12));
                uint32_t x0 = c->f[12];
                *H(sp + 48) = (uint16_t)x0; *H(sp + 32) = (uint16_t)x0;
                c->f[15] = (uint32_t)(S16(pen) + 8);
                FSET(c, 15, (float)(int32_t)c->f[15]);
                c->f[16] = RW(0x08E759ECu);
                FSET(c, 13, FS(c, 15) * FS(c, 13));
                FSET(c, 12, FS(c, 13) + FS(c, 14));
                c->f[15] = RW(0x08E75A0Cu);
                c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 12));
                uint32_t x1 = c->f[12];
                *H(sp + 80) = (uint16_t)x1; *H(sp + 64) = (uint16_t)x1;
                c->f[13] = (uint32_t)(int32_t)S16(pen + 2);
                FSET(c, 13, (float)(int32_t)c->f[13]);
                uint32_t cell = ch + *B(t + 18);
                FSET(c, 13, FS(c, 13) * FS(c, 16));
                FSET(c, 12, FS(c, 13) + FS(c, 15));
                c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 12));
                uint32_t y0 = c->f[12];
                *H(sp + 66) = (uint16_t)y0; *H(sp + 34) = (uint16_t)y0;
                c->f[13] = (uint32_t)(S16(pen + 2) + 0x10);
                FSET(c, 13, (float)(int32_t)c->f[13]);
                cell = ((cell & 0xFFFF) - 1) & 0xFFFF;
                FSET(c, 13, FS(c, 13) * FS(c, 16));
                FSET(c, 12, FS(c, 13) + FS(c, 15));
                uint32_t u = (cell & 0x1F) << 3;
                c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 12));
                uint32_t v = (((uint32_t)((int32_t)cell >> 5)) << 4) + 0x50;
                uint32_t y1 = c->f[12];
                *H(sp + 82) = (uint16_t)y1; *H(sp + 50) = (uint16_t)y1;
                uint16_t col = *H(0x08AE03B8u + 2 * *W(pen + 44));
                *H(sp + 84) = col; *H(sp + 68) = col; *H(sp + 52) = col; *H(sp + 36) = col;
                *H(pen + 8) = (uint16_t)u;
                *H(pen + 10) = (uint16_t)v;
                *H(sp + 54) = (uint16_t)S16(pen + 8); *H(sp + 38) = (uint16_t)S16(pen + 8);
                *H(sp + 86) = (uint16_t)(S16(pen + 8) + 8); *H(sp + 70) = (uint16_t)(S16(pen + 8) + 8);
                *H(sp + 72) = (uint16_t)S16(pen + 10); *H(sp + 40) = (uint16_t)S16(pen + 10);
                *W(sp + 92) = 0xFFFFFFFFu;
                *H(sp + 88) = (uint16_t)(S16(pen + 10) + 0x10); *H(sp + 56) = (uint16_t)(S16(pen + 10) + 0x10);
                *W(sp + 76) = 0xFFFFFFFFu; *W(sp + 60) = 0xFFFFFFFFu; *W(sp + 44) = 0xFFFFFFFFu;
                *W(pen + 76) = 0x8C;
                c->r[17] = s; c->r[20] = s; c->r[21] = (uint32_t)i;
                ARG(0) = sp + 0x20; ARG(1) = 4; ARG(2) = 0x8C; ARG(3) = 1; c->r[8] = 0;
                c->r[31] = 0x08812494u; f_089E1950(c);
                *H(pen) = (uint16_t)(S16(pen) + 8);
                next = i + 1;
            }
            count = *B(t + 17);
            i = next;
            c->r[17] = s; c->r[20] = s; c->r[21] = (uint32_t)i;
            if (!(i < count)) break;
        }
    }
    for (int k = 0; k < 8; k++) c->r[16 + k] = sv[k];
    c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x90;
}
