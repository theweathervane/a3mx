/* Flat-colour shapes drawn over the whole screen: the wipes into and out of the VS screen and the
 * fight (the green shapes, 0x0880D5B4 draws three side by side, 0x0880DA78 fades them).
 *
 * 0x0880E434 (a0 the shape's place: +0 x, +2 y, +44 its depth index; a1 the shape; a2 the alpha):
 * the shape is a list of quads (0x08A042C8[a1], 4 corners of two u16 each, ending at 0xFFFF), each
 * drawn in one colour (0x08A04B88[a1 / 3], with the alpha byte 0x08A04B98[a2]) through the 2D library's
 * quad list at 0x08AD9378 (corners at 0x08AD9388, colours at 0x08AD9398), blended unless a2 is 7.
 *
 * WIDE NATIVE: a quad with an edge on or past the 4:3 picture's side (x 0 or 384 on the screen) gets
 * a rectangle in the colour beside it, from that edge out to the 16:9 screen's side, as tall as the
 * edge. The quad itself keeps its corners, so the shapes' slopes (the A, X and V of the wipes into the
 * fight) stay as on the 4:3 screen. (0.99.8 moved such corners 48 out instead, which widened them.)
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA).
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

void f_089E1618(Cpu *c);   /* draws a quad list: a0 its descriptor, a1 vertex count, a2 flags, f12 depth */

#define SHAPES     0x08A042C8u   /* u32[]: each shape's quad list */
#define COLOURS    0x08A04B88u   /* u32[]: the colour of shapes 3n - 3n + 2 */
#define ALPHAS     0x08A04B98u   /* u8[]: alpha by a2 */
#define QUAD       0x08AD9378u   /* the quad list's descriptor */
#define QUAD_XY    0x08AD9388u   /* s16 x, y for its 4 corners */
#define QUAD_COL   0x08AD9398u   /* u32 colour for its 4 corners */
#define DEPTHS     0x08AE03B8u   /* u16[]: depth by index */
#define SCALE_X    0x08E759E8u
#define SCALE_Y    0x08E759ECu
#define OFFSET_X   0x08E75A04u
#define OFFSET_Y   0x08E75A0Cu
#define NO_BLEND   7

static int16_t to_screen(int32_t v, float scale, float offset) {
    float f = (float)v;
    f = f * scale;
    f = f + offset;
    return (int16_t)CVT_TRUNC(NULL, f);
}

/* WIDE NATIVE: the quad's corners on or past one side (x <= 0, or >= 384): if they make an edge (two
   heights), a rectangle from the outermost of them to the screen's side (-48 / 432), using the same
   quad list (corners top left, top right, bottom right, bottom left) */
static void wide_side(Cpu *c, const int16_t x[4], const int16_t y[4], int right, uint32_t flags, float z) {
    int n = 0; int16_t ex = 0, y0 = 0, y1 = 0;
    for (int i = 0; i < 4; i++) {
        if (right ? x[i] < 384 : x[i] > 0) continue;
        if (!n++) { ex = x[i]; y0 = y1 = y[i]; continue; }
        if (right ? x[i] > ex : x[i] < ex) ex = x[i];
        if (y[i] < y0) y0 = y[i];
        if (y[i] > y1) y1 = y[i];
    }
    if (n < 2 || y0 == y1 || (right ? ex >= 432 : ex <= -48)) return;
    int16_t l = right ? ex : -48, r = right ? 432 : ex;
    const int16_t rx[4] = { l, r, r, l }, ry[4] = { y0, y0, y1, y1 };
    for (int i = 0; i < 4; i++) { *H(QUAD_XY + i * 4) = (uint16_t)rx[i]; *H(QUAD_XY + i * 4 + 2) = (uint16_t)ry[i]; }
    memcpy(&c->f[12], &z, 4);
    c->r[4] = QUAD; c->r[5] = 4; c->r[6] = flags;
    f_089E1618(c);
}

/* (place, quads: 4 corners of u16 x, y each, ending at 0xFFFF, the colour (0xRRGGBB; the alpha byte
   is ORed in), the alpha's index): the quads in one colour. Also extensions/i_ism.c's I shapes. */
void shapes_draw(Cpu *c, gaddr place, const uint16_t *q, uint32_t rgb, uint32_t alpha) {
    int32_t px = (int16_t)*H(place), py = (int16_t)*H(place + 2);
    uint32_t colour = (uint32_t)*B(ALPHAS + alpha) << 24 | rgb;
    *W(QUAD_COL + 12) = colour; *W(QUAD_COL + 8) = colour; *W(QUAD_COL + 4) = colour; *W(QUAD_COL) = colour;
    float sx = F(SCALE_X), ox = F(OFFSET_X), sy = F(SCALE_Y), oy = F(OFFSET_Y);
    while (*q != 0xFFFF) {
        int16_t qx[4], qy[4];
        for (int i = 0; i < 4; i++, q += 2) {
            qx[i] = to_screen(px + (int32_t)q[0], sx, ox);
            qy[i] = to_screen(py + (int32_t)q[1], sy, oy);
            *H(QUAD_XY + i * 4) = (uint16_t)qx[i];
            *H(QUAD_XY + i * 4 + 2) = (uint16_t)qy[i];
        }
        float z = (float)(int32_t)*H(DEPTHS + *W(place + 44) * 2u);
        uint32_t flags = alpha != NO_BLEND ? 0x60 : 0x20;
        memcpy(&c->f[12], &z, 4);
        c->r[4] = QUAD; c->r[5] = 4; c->r[6] = flags;
        f_089E1618(c);
        if (rt_widescreen) { wide_side(c, qx, qy, 0, flags, z); wide_side(c, qx, qy, 1, flags, z); }
    }
}

static void draw_shape(Cpu *c, gaddr place, uint32_t shape, uint32_t alpha) {
    gaddr p = *W(SHAPES + shape * 4u);
    shapes_draw(c, place, H(p), *W(COLOURS + (uint32_t)((int32_t)shape / 3) * 4u), alpha);
}

SFA_FUNC_WRAPPED(0880E434, screen_draw_shape, i_ism_shape) {
    gaddr place = ARG(0); uint32_t shape = ARG(1), alpha = ARG(2);
    c->r[29] -= 0x40; draw_shape(c, place, shape, alpha); c->r[29] += 0x40;
}
