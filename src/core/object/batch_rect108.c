/* A flat-coloured rectangle drawer (68 instructions): its corners through the screen scale / offset
   floats into the quad at 0x08AD9388, its colour into 0x08AD9398, sent with 0x089E1618 at a float
   depth. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E1618(Cpu *c);

/* (x0, y0, x1, y1, colour, depth): the rectangle (x0, y0) - (x1, y1) in game units, scaled by
   0x08E759E8 / 0x08E759EC and offset by 0x08E75A04 / 0x08E75A0C, as four vertices (0x08AD9388: x0 y0,
   x1 y0, x1 y1, x0 y1) of one colour (0x08AD9398), drawn by 0x089E1618 (0x08AD9378, 4, 0x60) with the
   depth as a float in f12. */
SFA_FUNC(0880E788, rect_draw) {
    int32_t x0 = (int16_t)ARG(0), y0 = (int16_t)ARG(1), x1 = (int16_t)ARG(2), y1 = (int16_t)ARG(3);
    uint32_t colour = c->r[8], depth = c->r[9];
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->f[12] = (uint32_t)x0; c->f[13] = (uint32_t)x1;
    FSET(c, 12, (float)(int32_t)c->f[12]);
    FSET(c, 13, (float)(int32_t)c->f[13]);
    c->f[14] = RW(0x08E759E8u);
    FSET(c, 12, FS(c, 12) * FS(c, 14));
    FSET(c, 13, FS(c, 13) * FS(c, 14));
    c->f[15] = (uint32_t)y0;
    c->f[16] = RW(0x08E75A04u);
    FSET(c, 15, (float)(int32_t)c->f[15]);
    c->f[17] = RW(0x08E759ECu);
    FSET(c, 12, FS(c, 12) + FS(c, 16));
    FSET(c, 13, FS(c, 13) + FS(c, 16));
    FSET(c, 14, FS(c, 15) * FS(c, 17));
    c->f[19] = (uint32_t)y1;
    c->f[18] = RW(0x08E75A0Cu);
    FSET(c, 16, (float)(int32_t)c->f[19]);
    c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 12));
    FSET(c, 14, FS(c, 14) + FS(c, 18));
    c->f[13] = (uint32_t)CVT_TRUNC(c, FS(c, 13));
    FSET(c, 15, FS(c, 16) * FS(c, 17));
    const gaddr v = 0x08AD9388u, col = 0x08AD9398u;
    uint16_t sx0 = (uint16_t)c->f[12], sx1 = (uint16_t)c->f[13];
    *H(v + 12) = sx0; *H(v) = sx0;
    FSET(c, 15, FS(c, 15) + FS(c, 18));
    *H(v + 8) = sx1; *H(v + 4) = sx1;
    c->f[14] = (uint32_t)CVT_TRUNC(c, FS(c, 14));
    uint16_t sy0 = (uint16_t)c->f[14];
    *H(v + 6) = sy0; *H(v + 2) = sy0;
    c->f[15] = (uint32_t)CVT_TRUNC(c, FS(c, 15));
    *W(col + 12) = colour; *W(col + 8) = colour; *W(col + 4) = colour; *W(col) = colour;
    uint16_t sy1 = (uint16_t)c->f[15];
    c->f[12] = depth;
    *H(v + 14) = sy1;
    FSET(c, 12, (float)(int32_t)c->f[12]);
    *H(v + 10) = sy1;
    c->r[7] = v; c->r[10] = col;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0x08AD9378u; ARG(1) = 4; ARG(2) = 0x60;
    c->r[31] = 0x0880E88Cu; f_089E1618(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
