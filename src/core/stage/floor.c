/* The stage's floor band: the strip of floor along the bottom of the screen that most stages draw on
 * their own, outside the tile layers and the stage's objects.
 *
 * A stage's own routine (table 0x08A0893C by stage, run by stage_routine in object/runners.c) is
 * 0x0881C5A8 on 22 stages: it draws the floor as four textured quads side by side, each 64 texels of
 * the floor's texture wide and 256 pixels on screen, from the layer's scroll (+18 / +98) and the
 * screen shake (0x092E5F78), so the floor slides under the fighters with its own perspective.
 * CHARLIE (0x088196DC) and the BISON stages (0x0881C82C) have their own; the rest have none.
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA) and round
 * differently from the PSP (as in tiles.c).
 */
#include "../decomp.h"

void f_0880A3C4(Cpu *c);   /* draw_depth (depth.c): a0 the table's row, a1 its column */
void f_0881C5A8(Cpu *c);   /* stage_floor, below */
void f_089E1950(Cpu *c);   /* draws a quad list: a0 its vertices, a1 their count, a2 the texture, a3 / t0 0 */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline void      SET16(gaddr a, int32_t v) { *H(a) = (uint16_t)v; }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

#define SHAKE      0x092E5F78u   /* +28 the shake's strength, +130 / +132 its two heights */
#define SCALE_X    0x08E759E8u   /* float: the screen's scale */
#define SCALE_Y    0x08E759ECu
#define OFFSET_X   0x08E75A04u   /* float: the screen's offset */
#define OFFSET_Y   0x08E75A0Cu
#define FLOOR_QUAD 0x08B0151Cu   /* four vertices of 16 bytes: +0 x, +2 y, +4 depth, +6 / +8 u / v */
#define FLOOR_TEXTURES 0x08B0155Cu   /* u32: the floor's first texture; the layer's frame (+131) picks */

/* 0x089E0EF8: a float made whole, towards zero. */
static float whole(float f) { return (float)CVT_TRUNC(NULL, f); }

static void draw_floor(Cpu *c, gaddr layer) {
    int32_t up = (int16_t)(-S16(layer + 98));                       /* the layer's vertical scroll */
    int32_t across = (int16_t)(0x80 - S16(layer + 18));             /* and horizontal, from 128 */
    int32_t shake = (int32_t)*W(SHAKE + 28);
    /* the shake leans the band: its near edge by strength x (height 2 - height 1), its far edge by
       strength x 40, both in 1/65536ths */
    int32_t lean = (int16_t)(((uint32_t)shake * (uint32_t)(S16(SHAKE + 132) - S16(SHAKE + 130))) >> 16);
    int32_t lean_far = (int16_t)((int32_t)((uint32_t)shake * 40u) >> 16);

    float sy = F(SCALE_Y), ox_half, oy_half, f;
    oy_half = F(OFFSET_Y) + 0.5f;
    ox_half = F(OFFSET_X) + 0.5f;

    /* the band's top and bottom rows on screen */
    f = (float)(up + 0x100);
    f = f * sy;
    f = f + oy_half;
    int32_t top = CVT_TRUNC(NULL, whole(f));
    SET16(FLOOR_QUAD + 50, top);
    SET16(FLOOR_QUAD + 18, top);
    f = (float)(up + S16(SHAKE + 130));
    f = f * F(SCALE_Y);
    f = f + oy_half;
    int32_t bottom = CVT_TRUNC(NULL, whole(f));

    /* the left edge of the first quad, near (bottom) and far (top) */
    float sx = F(SCALE_X);
    float near_x = (float)(across + lean), far_x = (float)(across - lean_far);
    near_x = near_x * sx;
    far_x = far_x * sx;
    f = ox_half + near_x;
    float g = ox_half + far_x;
    SET16(FLOOR_QUAD + 34, bottom);
    SET16(FLOOR_QUAD + 2, bottom);
    SET16(FLOOR_QUAD + 22, 0);
    SET16(FLOOR_QUAD + 6, 0);
    SET16(FLOOR_QUAD + 54, 0x100);
    SET16(FLOOR_QUAD + 32, CVT_TRUNC(NULL, f));
    SET16(FLOOR_QUAD + 38, 0x100);
    SET16(FLOOR_QUAD + 48, CVT_TRUNC(NULL, g));
    int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, *B(layer + 122));
    SET16(FLOOR_QUAD + 52, depth);
    SET16(FLOOR_QUAD + 36, depth);
    SET16(FLOOR_QUAD + 20, depth);
    SET16(FLOOR_QUAD + 4, depth);

    /* four quads, each 256 pixels (scaled) to the right of the last, 64 texels of the texture on */
    for (int32_t i = 0; i < 4; i++) {
        float step = F(SCALE_X);
        step = step * 256.0f;
        SET16(FLOOR_QUAD + 0, S16(FLOOR_QUAD + 32));               /* this quad's left = last one's right */
        near_x = near_x + step;
        far_x = far_x + step;
        SET16(FLOOR_QUAD + 16, S16(FLOOR_QUAD + 48));
        int32_t u = (i << 6) + 0x40;
        int32_t v = u + S16(SHAKE + 130) - 0x100;
        float r = ox_half + near_x;
        float s = ox_half + far_x;
        SET16(FLOOR_QUAD + 32, CVT_TRUNC(NULL, r));
        SET16(FLOOR_QUAD + 48, CVT_TRUNC(NULL, s));
        SET16(FLOOR_QUAD + 40, v);
        SET16(FLOOR_QUAD + 8, v);
        SET16(FLOOR_QUAD + 24, u);
        SET16(FLOOR_QUAD + 56, u);
        c->r[4] = FLOOR_QUAD; c->r[5] = 4;
        c->r[6] = *W(FLOOR_TEXTURES) + (uint32_t)(int32_t)(int8_t)*B(layer + 131);
        c->r[7] = 0; c->r[8] = 0;
        c->r[31] = 0x0881C7E0;
        f_089E1950(c);
    }
}

/* (layer): the floor band, for the 22 stages whose own routine it is. */
SFA_FUNC(0881C5A8, stage_floor) {
    gaddr layer = ARG(0);
    c->r[29] -= 0x60;
    draw_floor(c, layer);
    c->r[29] += 0x60;
}

/* ---- CHARLIE: the floor, and before it one more picture (0x088196DC) ----
   A quad 192 x 48 (scaled) from texels 0 - 0xC0 x 0x30 - 0x60 at layer x 0x270, y 0x50 that slides with the
   shake: its left edge moves by 7/0xA0 of how far +104 is from the layer's +8 (in 1/65536ths), its
   corners by the shake's +114. Drawn with palette 1, then the floor as everywhere. */
void f_089E0EA4(Cpu *c);   /* selects a palette (a0) */
void f_089E1618(Cpu *c);   /* draws a quad list: a0 its descriptor, a1 vertex count, a2 flags, f12 depth */
#define CHARLIE_QUAD  0x08B014DCu   /* the quad list's descriptor; its corners at +16 (0x08B014EC) */
#define CHARLIE_UV    0x08B0150Cu   /* u / v by corner */

static void set_float_arg(Cpu *c, float f) { memcpy(&c->f[12], &f, 4); }

SFA_FUNC(088196DC, stage_floor_charlie) {
    gaddr layer = ARG(0);
    c->r[29] -= 0x30;
    CALL1(f_089E0EA4, 1);
    int32_t gap = (int32_t)(*W(SHAKE + 104) - *W(layer + 8));
    int32_t slide = gap / 0xA0;
    float sx = F(SCALE_X), sy = F(SCALE_Y), f, g;
    float x = (float)(S16(layer + 18) - 0x270);
    float y = (float)(S16(layer + 98) - 0x50);
    float ox_half = F(OFFSET_X) + 0.5f, oy_half = F(OFFSET_Y) + 0.5f;
    x = x * sx; x = -x;
    y = y * sy; y = -y;
    float left = ox_half + x;                                   /* the quad's left edge on screen */
    float top = oy_half + y;
    float wide = (float)S16(SHAKE + 114);
    float moved = (float)((int32_t)((uint32_t)slide * 7u) >> 16);
    wide = wide * sx;                                           /* the shake's spread */
    moved = moved * sx;
    float ninety_six = sx * 96.0f;
    f = left - moved;
    f = f + ninety_six;
    int32_t x0 = CVT_TRUNC(NULL, whole(f));
    f = F(SCALE_X); f = f * 192.0f;
    f = left + f;                                               /* 192 to the right */
    g = left + wide;
    float x2 = f - moved;
    SET16(CHARLIE_QUAD + 16, x0);
    f = f + wide;
    SET16(CHARLIE_QUAD + 16 + 12, CVT_TRUNC(NULL, g));
    int32_t y0 = CVT_TRUNC(NULL, top);
    SET16(CHARLIE_QUAD + 16 + 6, y0);
    SET16(CHARLIE_QUAD + 16 + 2, y0);
    SET16(CHARLIE_QUAD + 16 + 4, CVT_TRUNC(NULL, x2));
    SET16(CHARLIE_QUAD + 16 + 8, CVT_TRUNC(NULL, f));
    float h = F(SCALE_Y); h = h * 48.0f;
    f = top + h;
    int32_t y1 = CVT_TRUNC(NULL, whole(f));
    SET16(CHARLIE_UV + 0, 0x60);
    SET16(CHARLIE_QUAD + 16 + 14, y1);
    SET16(CHARLIE_QUAD + 16 + 10, y1);
    SET16(CHARLIE_UV + 8, 0xC0); SET16(CHARLIE_UV + 4, 0xC0);
    SET16(CHARLIE_UV + 12, 0); SET16(CHARLIE_UV + 6, 0); SET16(CHARLIE_UV + 2, 0);
    SET16(CHARLIE_UV + 14, 0x30); SET16(CHARLIE_UV + 10, 0x30);
    int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, *B(layer + 122));
    set_float_arg(c, (float)depth);
    c->r[4] = CHARLIE_QUAD; c->r[5] = 4; c->r[6] = 0x80000060u;
    c->r[31] = 0x088198C8;
    f_089E1618(c);
    c->r[4] = layer; c->r[31] = 0x088198D0;
    f_0881C5A8(c);
    c->r[29] += 0x30;
}

/* ---- BISON 1 / 2: two more bands above the floor, then the floor (0x0881C82C) ----
   Rows 0xA0 - 0xC0 of the layer: four quads 256 wide (scaled), 32 texels high, with palette 4 + the
   layer's frame / 2 and the frame's low bit picking the texture's half; rows 0xC0 - 0xD0: four more, 16
   texels high, with palette = the frame. Both start at x 128 less the fighters' layer's travel (+16
   less the shake's +112, in pixels). */
#define BISON_QUAD 0x08B014DCu   /* the quad list's descriptor; its corners at +16 */
#define BISON_UV   0x08B0150Cu

static int32_t band_row(int32_t row, gaddr layer, float oy_half) {
    float f = (float)(row - S16(layer + 98));
    f = f * F(SCALE_Y);
    f = f + oy_half;
    return CVT_TRUNC(NULL, whole(f));
}

SFA_FUNC(0881C82C, stage_floor_bison) {
    gaddr layer = ARG(0);
    const gaddr v = BISON_QUAD + 16;
    c->r[29] -= 0x50;
    float oy_half = F(OFFSET_Y) + 0.5f, ox_half = F(OFFSET_X) + 0.5f, f;
    int32_t y = band_row(0xA0, layer, oy_half);
    SET16(v + 6, y); SET16(v + 2, y);
    y = band_row(0xC0, layer, oy_half);
    SET16(v + 14, y); SET16(v + 10, y);
    float travel = (float)((int32_t)(*W(layer + 16) - *W(SHAKE + 112)) >> 16);
    float x = 128.0f - travel;
    x = x * F(SCALE_X);
    f = ox_half + x;
    *W(c->r[29] + 16) = SHAKE;                                   /* as the original keeps it */
    SET16(BISON_UV + 12, 0); SET16(BISON_UV + 0, 0);
    SET16(BISON_UV + 8, 0x100); SET16(BISON_UV + 4, 0x100);
    SET16(v + 4, CVT_TRUNC(NULL, f));
    for (int32_t i = 0, u = 0; i < 4; i++, u += 0x20) {
        int32_t frame = (int8_t)*B(layer + 131);
        CALL1(f_089E0EA4, (uint32_t)(frame / 2 + 4));
        float step = F(SCALE_X); step = step * 256.0f;
        int32_t left = S16(v + 4);
        SET16(v + 12, left);
        x = x + step;
        SET16(v + 0, left);
        f = ox_half + x;
        int32_t right = CVT_TRUNC(NULL, f);
        SET16(v + 8, right); SET16(v + 4, right);
        int32_t top = u + ((*B(layer + 131) & 1) << 7);
        SET16(BISON_UV + 2, top);
        SET16(BISON_UV + 6, top);
        SET16(BISON_UV + 14, S16(BISON_UV + 2) + 0x20); SET16(BISON_UV + 10, S16(BISON_UV + 2) + 0x20);
        int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, *B(layer + 122));
        set_float_arg(c, (float)depth);
        c->r[4] = BISON_QUAD; c->r[5] = 4; c->r[6] = 0x80000060u; c->r[31] = 0x0881CA20;
        f_089E1618(c);
    }
    const float nudge = 0x1.a36e2ep-14f;                          /* 0x38D1B717, about 0.0001 */
    f = (float)band_row(0xC0, layer, oy_half); f = f + nudge;
    y = CVT_TRUNC(NULL, f);
    SET16(v + 6, y); SET16(v + 2, y);
    f = (float)band_row(0xD0, layer, oy_half); f = f + nudge;
    y = CVT_TRUNC(NULL, f);
    SET16(v + 14, y); SET16(v + 10, y);
    travel = (float)((int32_t)(*W(layer + 16) - *W(*W(c->r[29] + 16) + 112)) >> 16);
    SET16(BISON_UV + 12, 0);
    x = 128.0f - travel;
    SET16(BISON_UV + 0, 0);
    SET16(BISON_UV + 8, 0x100); SET16(BISON_UV + 4, 0x100);
    x = x * F(SCALE_X);
    f = ox_half + x;
    SET16(v + 4, CVT_TRUNC(NULL, f));
    for (int32_t i = 0, u = 0; i < 4; i++, u += 0x40) {
        CALL1(f_089E0EA4, (uint32_t)(int32_t)(int8_t)*B(layer + 131));
        float step = F(SCALE_X); step = step * 256.0f;
        int32_t left = S16(v + 4);
        SET16(BISON_UV + 2, u);
        SET16(v + 12, left);
        x = x + step;
        SET16(v + 0, left);
        int32_t bottom = S16(BISON_UV + 2) + 0x10;
        SET16(BISON_UV + 6, u);
        f = ox_half + x;
        SET16(BISON_UV + 14, bottom); SET16(BISON_UV + 10, bottom);
        int32_t right = CVT_TRUNC(NULL, f);
        SET16(v + 8, right); SET16(v + 4, right);
        int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, *B(layer + 122));
        set_float_arg(c, (float)depth);
        c->r[4] = BISON_QUAD; c->r[5] = 4; c->r[6] = 0x80000020u; c->r[31] = 0x0881CB80;
        f_089E1618(c);
    }
    c->r[4] = layer; c->r[31] = 0x0881CB98;
    f_0881C5A8(c);
    c->r[29] += 0x50;
}
