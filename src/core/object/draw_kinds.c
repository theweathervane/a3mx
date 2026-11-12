/* Drawing an object of the third list by its kind (draw_objects in frame.c): kind 0 a plain
 * rectangle, kind 1 a textured one, kind 2 a set of kind-1 parts. Kinds 3 (0x0880F820, a shape built
 * from many polygons) and 4 (0x08810128, a shaded rectangle) stay translated: no scenario draws one.
 *
 * Screen positions go through the game's float scale and offset (GAME + 0xD8 ...) and are truncated
 * to whole pixels. Every multiply and add is its own statement so the compiler cannot fuse them: the
 * PSP rounds each one.
 */
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

void f_089E0CF4(Cpu *c);   /* selects a texture page (a0 its descriptor) */
void f_089E0EA4(Cpu *c);   /* selects a palette (a0) */
void f_089E1438(Cpu *c);   /* returns the current texture's draw flags */
void f_089E1950(Cpu *c);   /* draws textured vertices: a0, a1 count, a2 flags, a3 blend, t0 effect */
void f_089E0C4C(Cpu *c);   /* GE drawing mode on (1) / off (0) */
void f_089E10D8(Cpu *c);   /* draws untextured vertices: a0, a1 count, a2 blend */
void f_0881032C(Cpu *c);   /* kind 1: a kind-2 object draws its parts through it */

#define DRAW_STATE   0x08AD9320u   /* +44: the depth index */
#define DEPTHS       0x08AE03B8u   /* u16[]: depth by index */
#define ANCHORS      0x08A04014u   /* u32[]: anchor records (+18 x, +22 y) */
#define TEXTURES     0x08E75A18u   /* u32[]: texture page descriptors */
#define SCALE_X      0x08E759E8u   /* float; the flipped pair is +8 / +12 */
#define SCALE_Y      0x08E759ECu
#define OFFSET_X     0x08E75A04u
#define OFFSET_Y     0x08E75A0Cu

/* An object of the third list (offsets used here; 0x94 bytes) */
enum {
    OB_ANCHOR = 3, OB_X = 6, OB_Y = 10, OB_LEFT = 12, OB_WIDTH = 14, OB_TOP = 24, OB_HEIGHT = 26,
    OB_RED = 36, OB_GREEN = 42, OB_BLUE = 48,   /* 4 corners each */
    OB_TEXTURE = 54, OB_PALETTE = 60, OB_UV = 64, OB_UV_ROT = 68, OB_UV2 = 76, OB_UV2_ROT = 80,
    OB_KIND = 88, OB_FLIP_Y = 89, OB_PARTS = 132, OB_ALPHA = 136, OB_ALPHA_2 = 137, OB_GE_MODE = 138,
    OB_EFFECT = 139,
};
#define KIND_ROTATED_UV 0x80

static uint32_t corner_colour(gaddr ob, int i) {
    return (uint32_t)*B(ob + OB_ALPHA) << 24 | (uint32_t)*B(ob + OB_RED + i) << 16 |
           (uint32_t)*B(ob + OB_GREEN + i) << 8 | *B(ob + OB_BLUE + i);
}

/* x' = trunc(x * scale + offset), rounded as the PSP does: a product, then a sum */
static int16_t to_screen(int16_t v, float scale, float offset) {
    float f = (float)v;
    f = f * scale;
    f = f + offset;
    return (int16_t)CVT_TRUNC(NULL, f);
}

/* ---- kinds 0 and 1: a rectangle placed by the object's anchor; flipped objects use the second
   scale pair and grow downwards ---- */

static void rect_on_screen(gaddr ob, int16_t *X0, int16_t *X1, int16_t *Y0, int16_t *Y1) {
    int flip = *B(ob + OB_FLIP_Y) != 0;
    int16_t x = S16(ob + OB_X), y = S16(ob + OB_Y);
    if (!flip) y = (int16_t)(0xF0 - y);
    uint8_t anchor = *B(ob + OB_ANCHOR);
    if (anchor != 0) {
        gaddr rec = *W(ANCHORS + anchor * 4u);
        x = (int16_t)(x - S16(rec + 18));
        y = (int16_t)(y + S16(rec + 22));
    }
    int16_t x0 = (int16_t)(x + S16(ob + OB_LEFT)), x1 = (int16_t)(x0 + S16(ob + OB_WIDTH));
    int16_t y0 = flip ? (int16_t)(y + S16(ob + OB_TOP)) : (int16_t)(y - S16(ob + OB_TOP));
    int16_t y1 = (int16_t)(y0 + S16(ob + OB_HEIGHT));
    float sx = F(flip ? SCALE_X + 8 : SCALE_X), sy = F(flip ? SCALE_Y + 8 : SCALE_Y);
    *X0 = to_screen(x0, sx, F(OFFSET_X)); *X1 = to_screen(x1, sx, F(OFFSET_X));
    *Y0 = to_screen(y0, sy, F(OFFSET_Y)); *Y1 = to_screen(y1, sy, F(OFFSET_Y));
}

/* corner colours: the object's corners 0 - 3 are top-left, top-right, bottom-right, bottom-left;
   vertices go top-left, bottom-left, top-right, bottom-right */
static const uint8_t CORNER_OF_VERTEX[4] = { 0, 3, 1, 2 };

/* Kind 0: a plain rectangle, colour per corner. Vertices at frame + 16, 12 bytes each: x, y, depth,
   colour (+8). */
static void draw_plain(Cpu *c, gaddr ob, gaddr frame) {
    int16_t X0, X1, Y0, Y1;
    rect_on_screen(ob, &X0, &X1, &Y0, &Y1);
    gaddr v = frame + 16;
    const int16_t xy[4][2] = { { X0, Y0 }, { X0, Y1 }, { X1, Y0 }, { X1, Y1 } };
    uint16_t z = *H(DEPTHS + *W(DRAW_STATE + 44) * 2u);
    for (int i = 0; i < 4; i++) {
        *H(v + i * 12 + 0) = (uint16_t)xy[i][0];
        *H(v + i * 12 + 2) = (uint16_t)xy[i][1];
        *H(v + i * 12 + 4) = z;
        *W(v + i * 12 + 8) = corner_colour(ob, CORNER_OF_VERTEX[i]);
    }
    CALL3(f_089E10D8, v, 4, *B(ob + OB_ALPHA) != 0xFF);
}

/* ---- kind 1: a textured rectangle ---- */

/* The vertices at frame + 32, 16 bytes each: x, y, depth, u, v, colour (+12). Corners in the order
   top-left, bottom-left, top-right, bottom-right. */
static void draw_textured(Cpu *c, gaddr ob, gaddr frame) {
    CALL1(f_089E0CF4, *W(TEXTURES + *B(ob + OB_TEXTURE) * 4u));
    CALL1(f_089E0EA4, *W(ob + OB_PALETTE));

    int16_t X0, X1, Y0, Y1;
    rect_on_screen(ob, &X0, &X1, &Y0, &Y1);

    gaddr v = frame + 32;
    const int16_t xy[4][2] = { { X0, Y0 }, { X0, Y1 }, { X1, Y0 }, { X1, Y1 } };
    uint16_t z = *H(DEPTHS + *W(DRAW_STATE + 44) * 2u);
    for (int i = 0; i < 4; i++) {
        *H(v + i * 16 + 0) = (uint16_t)xy[i][0];
        *H(v + i * 16 + 2) = (uint16_t)xy[i][1];
        *H(v + i * 16 + 4) = z;
    }
    /* texture coordinates: u from OB_UV (+0 left, +2 right), v from OB_UV2 (+0 top, +2 bottom);
       a rotated object gives each corner its own pair */
    if (*B(ob + OB_KIND) & KIND_ROTATED_UV) {
        static const uint8_t u_of[4] = { OB_UV, OB_UV + 2, OB_UV_ROT, OB_UV_ROT + 2 };
        static const uint8_t v_of[4] = { OB_UV2, OB_UV2 + 2, OB_UV2_ROT, OB_UV2_ROT + 2 };
        for (int i = 0; i < 4; i++) { *H(v + i * 16 + 6) = *H(ob + u_of[i]); *H(v + i * 16 + 8) = *H(ob + v_of[i]); }
    } else {
        for (int i = 0; i < 4; i++) {
            *H(v + i * 16 + 6) = *H(ob + OB_UV + (i >> 1) * 2);
            *H(v + i * 16 + 8) = *H(ob + OB_UV2 + (i & 1) * 2);
        }
    }
    for (int i = 0; i < 4; i++) *W(v + i * 16 + 12) = corner_colour(ob, CORNER_OF_VERTEX[i]);

    if ((int8_t)*B(ob + OB_GE_MODE) != 0) CALL1(f_089E0C4C, 1);
    uint32_t blend = *B(ob + OB_ALPHA_2) != 0 || *B(ob + OB_ALPHA) != 0xFF;
    f_089E1438(c);
    uint32_t flags = c->r[2];
    CALL5(f_089E1950, v, 4, flags, blend, (uint32_t)(int32_t)(int8_t)*B(ob + OB_EFFECT));
    if ((int8_t)*B(ob + OB_GE_MODE) != 0) CALL1(f_089E0C4C, 0);
}

/* ---- kind 2: parts ---- */

/* The object's parts (OB_PARTS: 12 bytes each, +0 x, +2 y, +4 image, +8 nonzero on the last), drawn
   last to first, each as a kind-1 object made on the stack from the object and the part's image
   (+0 width, +2 height, +4 palette, +8 / +10 / +12 / +14 texture coordinates). Only these fields of
   the stack object are written; the rest is whatever the stack held, as in the original. */
static void draw_parts(Cpu *c, gaddr ob, gaddr frame) {
    gaddr part = *W(ob + OB_PARTS);
    uint32_t count = 0;
    while (*H(part + 8) == 0) { part += 12; count++; }
    count++;

    gaddr tmp = frame + 16;
    static const uint8_t copied[] = {
        OB_TEXTURE, OB_KIND, OB_FLIP_Y, OB_ANCHOR, OB_ALPHA, OB_ALPHA_2, OB_GE_MODE,
        OB_RED, OB_RED + 1, OB_RED + 2, OB_RED + 3, OB_GREEN, OB_GREEN + 1, OB_GREEN + 2, OB_GREEN + 3,
        OB_BLUE, OB_BLUE + 1, OB_BLUE + 2, OB_BLUE + 3,
    };
    for (size_t i = 0; i < sizeof copied; i++) *B(tmp + copied[i]) = *B(ob + copied[i]);

    for (; count != 0; count--, part -= 12) {
        gaddr image = *W(part + 4);
        *H(tmp + OB_X) = (uint16_t)(S16(ob + OB_X) + S16(part + 0));
        *H(tmp + OB_Y) = (uint16_t)(S16(ob + OB_Y) + S16(part + 2));
        *H(tmp + OB_TOP) = 0;
        *H(tmp + OB_LEFT) = 0;
        *W(tmp + OB_PALETTE) = *W(image + 4);
        *H(tmp + OB_WIDTH) = *H(image + 0);
        *H(tmp + OB_HEIGHT) = *H(image + 2);
        *H(tmp + OB_UV) = *H(image + 8);
        *H(tmp + OB_UV2) = *H(image + 10);
        *H(tmp + OB_UV + 2) = *H(image + 12);
        *H(tmp + OB_UV2 + 2) = *H(image + 14);
        CALL1(f_0881032C, tmp);
    }
}

SFA_FUNC(0880F4FC, game_draw_plain)    { c->r[29] -= 0x50; draw_plain(c, ARG(0), c->r[29]); c->r[29] += 0x50; }
SFA_FUNC(0881032C, game_draw_textured) { c->r[29] -= 0x70; draw_textured(c, ARG(0), c->r[29]); c->r[29] += 0x70; }
SFA_FUNC(08810754, game_draw_parts)    { c->r[29] -= 0xC0; draw_parts(c, ARG(0), c->r[29]); c->r[29] += 0xC0; }
