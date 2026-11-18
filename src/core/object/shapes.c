/* Flat-colour shapes drawn over the whole screen: the wipes into and out of the VS screen and the
 * fight (the green shapes, 0x0880D5B4 draws three side by side, 0x0880DA78 fades them).
 *
 * 0x0880E434 (a0 the shape's place: +0 x, +2 y, +44 its depth index; a1 the shape; a2 the alpha):
 * the shape is a list of quads (0x08A042C8[a1], 4 corners of two u16 each, ending at 0xFFFF), each
 * drawn in one colour (0x08A04B88[a1 / 3], with the alpha byte 0x08A04B98[a2]) through the 2D library's
 * quad list at 0x08AD9378 (corners at 0x08AD9388, colours at 0x08AD9398), blended unless a2 is 7.
 *
 * WIDE FORCED: a corner on or past the 4:3 picture's edge (x 0 or 384 on the screen) goes 48 further
 * out, so a shape that reaches the edge covers the 16:9 margin too, as the wipe sweeps across.
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA).
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE FORCED: extensions/widescreen.c (extensions_off.c: 0) */

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

static int16_t wide_x(int16_t x) {
    if (!rt_widescreen) return x;
    if (x <= 0) return (int16_t)(x - 48);
    if (x >= 384) return (int16_t)(x + 48);
    return x;
}

static void draw_shape(Cpu *c, gaddr place, uint32_t shape, uint32_t alpha) {
    int32_t px = (int16_t)*H(place), py = (int16_t)*H(place + 2);
    uint32_t colour = (uint32_t)*B(ALPHAS + alpha) << 24 | *W(COLOURS + (uint32_t)((int32_t)shape / 3) * 4u);
    *W(QUAD_COL + 12) = colour; *W(QUAD_COL + 8) = colour; *W(QUAD_COL + 4) = colour; *W(QUAD_COL) = colour;
    gaddr p = *W(SHAPES + shape * 4u);
    float sx = F(SCALE_X), ox = F(OFFSET_X), sy = F(SCALE_Y), oy = F(OFFSET_Y);
    while (*H(p) != 0xFFFF) {
        for (int i = 0; i < 4; i++, p += 4) {
            *H(QUAD_XY + i * 4) = (uint16_t)wide_x(to_screen(px + (int32_t)*H(p), sx, ox));
            *H(QUAD_XY + i * 4 + 2) = (uint16_t)to_screen(py + (int32_t)*H(p + 2), sy, oy);
        }
        float z = (float)(int32_t)*H(DEPTHS + *W(place + 44) * 2u);
        memcpy(&c->f[12], &z, 4);
        c->r[4] = QUAD; c->r[5] = 4; c->r[6] = alpha != NO_BLEND ? 0x60 : 0x20;
        f_089E1618(c);
    }
}

SFA_FUNC(0880E434, screen_draw_shape) {
    gaddr place = ARG(0); uint32_t shape = ARG(1), alpha = ARG(2);
    c->r[29] -= 0x40; draw_shape(c, place, shape, alpha); c->r[29] += 0x40;
}
