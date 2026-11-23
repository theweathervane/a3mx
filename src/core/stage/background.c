/* The stage's background: each frame, the three camera layers and the floor, each by its own drawer.
 *
 * 0x08819CFC (from the fight's frame) sets up the shared quad list (0x08B014DC) and its colour, then for
 * each layer, if the layer's draw flag in layer 1's +123 is set: layer 1 (bit 2), the stage's own
 * routine with layer 1 (bit 8: the floor, floor.c), layer 0 (bit 1), layer 2 (bit 4). A layer's kind
 * (+3) picks its drawer: the 32-pixel tile drawer (tiles.c), a 16-pixel one through a record set up
 * by 0x0881A8E0 / 0x0881BD28 / 0x0881C354, or a picture of a few quads (0x08819498 / 0x0881C0C8,
 * BISON's sky and mountains). The first time a layer is seen (+7 = 0) its kind is set from the stage
 * number instead and it isn't drawn.
 *
 * Every float multiply and add is its own statement (no FMA), as in tiles.c.
 */
#include "../decomp.h"

void f_089E0CF4(Cpu *c);   /* sets the drawing state from a list (a0) */
void f_088192C0(Cpu *c);   /* stage_routine (object/runners.c): the stage's own routine, a0 the layer */
void f_08819498(Cpu *c);   /* picture layer, one quad (below) */
void f_0881C0C8(Cpu *c);   /* picture layer, two quads (below) */
void f_0881A8E0(Cpu *c);   /* 16-pixel layer from the layer (below): a0 the layer, a1 its map */
void f_0881BD28(Cpu *c);   /* 16-pixel layer, wide (below) */
void f_0881C354(Cpu *c);   /* 16-pixel layer, strips (below) */
void f_0881AB28(Cpu *c);   /* stage_draw_layer_32 (tiles.c): a0 the layer, a1 its map */
extern int tiles_ending_mode;   /* tiles.c: WIDE NATIVE, an ending's layers cut or scaled */
extern int tiles_ending_shift;  /* tiles.c: ... or moved (E. Honda's dojo) */
int tiles_ending_quad(void);

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline void      SET16(gaddr a, int32_t v) { *H(a) = (uint16_t)v; }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

#define BATTLE    0x092E1228u   /* +0 the stage, +23 this frame skipped */
#define LAYER_0   0x092E8800u
#define LAYER_1   0x092E88A0u   /* +123: which layers are drawn (bit 1 layer 0, 2 layer 1, 4 layer 2, 8 the floor) */
#define LAYER_2   0x092E8940u
#define SHAKE     0x092E5F78u   /* +7: set after the first frame */
#define MAPS      0x092E8A00u   /* the layers' tile maps */
#define QUADS     0x08B014DCu   /* the quad list's descriptor: corners, colours, u / v, count */
#define SCALE_X   0x08E759E8u   /* float: the screen's scale */
#define SCALE_Y   0x08E759ECu
#define OFFSET_X  0x08E75A04u   /* float: the screen's offset */
#define OFFSET_Y  0x08E75A0Cu
#define QUAD_XY   0x08B014ECu   /* s16 x, y for the quad's 4 corners: top left, top right, bottom right, bottom left */
#define QUAD_UV   0x08B0150Cu   /* s16 u, v, the same order */
#define PAUSED    0x08E75969u   /* s8: nonzero stops background animation (with BATTLE +15 / +37) */
#define FADE      0x08E75C44u   /* s16: the stage's brightness, 0 - 0xFF */

void f_0880A3C4(Cpu *c);   /* draw_depth (depth.c): a0 the table's row, a1 its column */
void f_089E0EA4(Cpu *c);   /* selects a palette (a0) */
void f_089E1618(Cpu *c);   /* draws a quad list: a0 its descriptor, a1 vertex count, a2 flags, f12 depth */
void f_0881B268(Cpu *c);   /* stage_draw_layer_16 (tiles.c): a0 its record */

/* GRID record fields (as tiles.c names them): the 16-pixel layer drawer 0x0881B268 reads its record from
   the caller's stack frame, so the three builders below lay it out there as the original did. */
enum {
    G_Y = 12, G_Y0 = 14, G_LINE = 20, G_STEP_X = 24, G_STEP_Y = 26, G_OFS_X = 28, G_OFS_Y = 30,
    G_X0 = 32, G_ROW_WRAP = 36, G_BOTTOM = 48, G_MAP_COL = 54, G_SCROLL = 56, G_DEPTH = 60,
    G_ROWS = 62, G_LAYER_2 = 64, G_STRIPS = 72, G_BASE = 80, G_COLS_TO_WRAP = 88,
    G_ROWS_PER_WRAP = 92, G_KIND = 94,
};

/* 0x089E0EF8: a float made whole, towards zero. */
static float whole(float f) { return (float)CVT_TRUNC(NULL, f); }

static void set_float_arg(Cpu *c, float f) { memcpy(&c->f[12], &f, 4); }

/* The fighters' layer's travel from the camera's left edge, in pixels. */
static int32_t travel(gaddr layer) {
    return (int32_t)(*W(layer + 16) - *W(SHAKE + 112)) >> 16;
}

/* A record's start shared by the builders: the layer's +2 / +3 and a 16-pixel tile's size on screen. */
static void grid_start(gaddr st, gaddr layer) {
    SET16(st + G_LAYER_2, *B(layer + 2));
    SET16(st + G_KIND, *B(layer + 3));
    float f = F(SCALE_X);
    f = f * 16.0f;
    SET16(st + G_STEP_X, CVT_TRUNC(NULL, f));
    f = F(SCALE_Y);
    f = f * 16.0f;
    SET16(st + G_STEP_Y, CVT_TRUNC(NULL, f));
}

/* The part of a tile scrolled past, on screen: rows from the vertical scroll s, columns from the
   horizontal h. */
static void grid_parts(gaddr st, int32_t s, int32_t h) {
    float part = (float)(s & 0xF);
    part = part * F(SCALE_Y);
    part = -part;
    SET16(st + G_Y, CVT_TRUNC(NULL, part));
    part = (float)(h & 0xF);
    part = part * F(SCALE_X);
    part = -part;
    SET16(st + G_X0, CVT_TRUNC(NULL, part));
}

/* The screen offset, rounded, and where the first row starts and ends. */
static void grid_screen(gaddr st) {
    float f = F(OFFSET_X) + 0.5f;
    SET16(st + G_OFS_X, CVT_TRUNC(NULL, f));
    SET16(st + G_Y0, S16(st + G_Y));
    f = F(OFFSET_Y) + 0.5f;
    SET16(st + G_OFS_Y, CVT_TRUNC(NULL, f));
    SET16(st + G_BOTTOM, S16(st + G_Y0) + S16(st + G_OFS_Y));
}

static void draw_record(Cpu *c, gaddr st, gaddr map, uint32_t back) {
    c->r[4] = st; c->r[5] = map; c->r[31] = back;
    f_0881B268(c);
}

/* ---- 0x0881A8E0: a 16-pixel layer, 64 tiles wide ----
   a0 the layer (+2 1: it scrolls with the fighters' layer, else by its own +18; +3 kind 0x85: animated,
   16 rows of 144 words, else 32 rows of 64), a1 its map. */
static void draw_layer_16_wide(Cpu *c, gaddr layer, gaddr map) {
    c->r[29] -= 0x80;
    gaddr st = c->r[29] + 16;
    grid_start(st, layer);
    SET16(st + G_SCROLL, *H(layer + 98));
    int animated = *B(layer + 3) == 0x85;
    SET16(st + G_SCROLL, S16(st + G_SCROLL) & (animated ? 0xFF : 0x1FF));
    int32_t s = S16(st + G_SCROLL);
    SET16(st + G_MAP_COL, *B(layer + 2) == 1 ? travel(layer) : S16(layer + 18));
    SET16(st + G_MAP_COL, S16(st + G_MAP_COL) & 0x3FF);
    int32_t h = S16(st + G_MAP_COL);
    grid_parts(st, s, h);
    *W(st + G_LINE) = map;
    *W(st + G_BASE) = map;
    int32_t rows_in = s & 0xFFF0, row0 = s >> 4;
    gaddr line;
    if (animated) {
        line = map + (uint32_t)(rows_in * 36);                  /* 0x90 words a row */
        SET16(st + G_ROW_WRAP, 0x10 - row0);
        SET16(st + G_ROWS_PER_WRAP, 0x10);
    } else {
        line = map + (uint32_t)(rows_in << 4);                  /* 0x40 words a row */
        SET16(st + G_ROW_WRAP, 0x20 - row0);
        SET16(st + G_ROWS_PER_WRAP, 0x20);
    }
    SET16(st + G_MAP_COL, h >> 4);
    int32_t col0 = S16(st + G_MAP_COL);
    *W(st + G_LINE) = line + (uint32_t)(col0 << 2);
    SET16(st + G_COLS_TO_WRAP, 0x40 - col0);
    grid_screen(st);
    SET16(st + G_ROWS, 0x10);
    SET16(st + G_DEPTH, *B(layer + 122));
    draw_record(c, st, map, 0x0881AB1C);
    c->r[29] += 0x80;
}

SFA_FUNC(0881A8E0, stage_layer_16_wide) {
    gaddr layer = ARG(0), map = ARG(1);
    draw_layer_16_wide(c, layer, map);
}

/* ---- 0x08819300: a 16-pixel layer, 64 tiles by 64 rows ----
   The endings' layer 1 (core/task/ending.c, ending kinds 1 and 2): as 0x0881A8E0, scrolled by the layer's
   own +18 and +98 only, from a map of 64 rows of 64 words that wraps both ways every 64 tiles.
   Wrapped for WIDE NATIVE's scaled endings (extensions/scaled_endings_stages.c): a scene cut from a
   stage gets that stage's own art drawn on past its sides. */
static void draw_layer_16_tall(Cpu *c, gaddr layer, gaddr map) {
    c->r[29] -= 0x80;
    gaddr st = c->r[29] + 16;
    grid_start(st, layer);
    SET16(st + G_SCROLL, *H(layer + 98));
    SET16(st + G_SCROLL, S16(st + G_SCROLL) & 0x3FF);
    SET16(st + G_MAP_COL, *H(layer + 18));
    int32_t s = S16(st + G_SCROLL);
    SET16(st + G_MAP_COL, S16(st + G_MAP_COL) & 0x3FF);
    int32_t h = S16(st + G_MAP_COL);
    grid_parts(st, s, h);
    *W(st + G_LINE) = map;
    *W(st + G_BASE) = map;
    gaddr line = map + (uint32_t)((s & 0xFFF0) << 4);           /* 0x40 words a row */
    *W(st + G_LINE) = line;
    SET16(st + G_MAP_COL, h >> 4);
    int32_t col0 = S16(st + G_MAP_COL);
    SET16(st + G_ROW_WRAP, 0x40 - (s >> 4));
    SET16(st + G_ROWS_PER_WRAP, 0x40);
    *W(st + G_LINE) = line + (uint32_t)(col0 << 2);
    SET16(st + G_COLS_TO_WRAP, 0x40 - col0);
    grid_screen(st);
    SET16(st + G_ROWS, 0x10);
    SET16(st + G_DEPTH, *B(layer + 122));
    draw_record(c, st, map, 0x0881948C);
    c->r[29] += 0x80;
}

SFA_FUNC_WRAPPED(08819300, stage_layer_16_tall, scaled_endings_stage_layer) {
    gaddr layer = ARG(0), map = ARG(1);
    draw_layer_16_tall(c, layer, map);
}

/* ---- 0x0881BD28: a 16-pixel layer whose top rows can lag behind ----
   The default for layer 1. Kinds 0xA0 and 0xA4 split it: the rows above a line (0x8F / 0xEF less the
   scroll and 0x40 / 0x70) move across with the screen shake's strength (+28) -- eight times it, or times
   the shake's height difference + 2 -- and the rows below with the fighters' layer; other kinds move
   whole with the fighters' layer. a0 the layer, a1 its map (32 rows of 64 words). */
static void draw_layer_16_split(Cpu *c, gaddr layer, gaddr map) {
    c->r[29] -= 0xA0;
    gaddr st = c->r[29] + 16;
    grid_start(st, layer);
    int32_t kind = S16(st + G_KIND);
    SET16(st + G_SCROLL, *H(layer + 98));
    int32_t s = S16(st + G_SCROLL);
    int32_t col;
    if (kind == 0xA0) {
        SET16(st + G_ROWS, s + 0x40);
        SET16(st + G_ROWS, 0x8F - S16(st + G_ROWS));
        SET16(st + G_MAP_COL, (int32_t)(*W(layer + 16) - (*W(SHAKE + 28) << 3)) >> 16);
    } else if (kind == 0xA4) {
        SET16(st + G_ROWS, s + 0x70);
        SET16(st + G_ROWS, 0xEF - S16(st + G_ROWS));
        uint32_t lag = *W(SHAKE + 28) * (uint32_t)(S16(SHAKE + 132) - S16(SHAKE + 130) + 2);
        SET16(st + G_MAP_COL, (int32_t)(*W(layer + 16) - lag) >> 16);
    } else {
        SET16(st + G_ROWS, 0);
        SET16(st + G_MAP_COL, travel(layer));
    }
    col = S16(st + G_MAP_COL) & 0x3FF;
    int32_t split = S16(st + G_ROWS);

    SET16(st + G_SCROLL, s & 0x1FF);
    s = S16(st + G_SCROLL);
    SET16(st + G_MAP_COL, col);
    int32_t h = S16(st + G_MAP_COL);
    grid_parts(st, s, h);
    *W(st + G_LINE) = map;
    SET16(st + G_MAP_COL, h >> 4);
    *W(st + G_BASE) = map;
    gaddr line = map + (uint32_t)((s & 0xFFF0) << 4);
    int32_t col0 = S16(st + G_MAP_COL);
    *W(st + G_LINE) = line + (uint32_t)(col0 << 2);
    SET16(st + G_COLS_TO_WRAP, 0x40 - col0);
    SET16(st + G_ROW_WRAP, 0x20 - (s >> 4));
    SET16(st + G_ROWS_PER_WRAP, 0x20);
    grid_screen(st);
    SET16(st + G_DEPTH, *B(layer + 122));
    if (split == 0) {
        SET16(st + G_ROWS, 0x10);
        draw_record(c, st, map, 0x0881BFAC);
        c->r[29] += 0xA0;
        return;
    }
    /* the top rows */
    SET16(st + G_SCROLL, s + split);
    SET16(st + G_SCROLL, S16(st + G_SCROLL) & 0x1FF);
    SET16(st + G_ROWS, split >> 4);
    draw_record(c, st, map, 0x0881BFDC);
    /* the rest, with the fighters' layer, from where the top rows ended */
    SET16(st + G_MAP_COL, travel(layer));
    SET16(st + G_MAP_COL, S16(st + G_MAP_COL) & 0x3FF);
    h = S16(st + G_MAP_COL);
    float part = (float)(h & 0xF);
    int32_t s2 = S16(st + G_SCROLL);
    *W(st + G_LINE) = map;
    part = part * F(SCALE_X);
    SET16(st + G_MAP_COL, h >> 4);
    *W(st + G_BASE) = map;
    line = map + (uint32_t)((s2 & 0xFFF0) << 4);
    col0 = S16(st + G_MAP_COL);
    part = -part;
    SET16(st + G_X0, CVT_TRUNC(NULL, part));
    *W(st + G_LINE) = line + (uint32_t)(col0 << 2);
    SET16(st + G_Y0, S16(st + G_Y));
    SET16(st + G_COLS_TO_WRAP, 0x40 - col0);
    SET16(st + G_ROW_WRAP, 0x20 - (s2 >> 4));
    SET16(st + G_BOTTOM, S16(st + G_Y0) + S16(st + G_OFS_Y));
    SET16(st + G_ROWS, 0x10 - S16(st + G_ROWS));
    draw_record(c, st, map, 0x0881C09C);
    c->r[29] += 0xA0;
}

SFA_FUNC(0881BD28, stage_layer_16_split) {
    gaddr layer = ARG(0), map = ARG(1);
    draw_layer_16_split(c, layer, map);
}

/* ---- 0x0881C354: a 16-pixel layer of strips (GUILE) ----
   The map is strips 16 words wide and 0xD0 words a row; a table (0x08A08BDD, 128 bytes) gives which
   strip shows in each 256-pixel stretch of the layer's horizontal scroll (+18), and 0x0881B268's kind
   0x90 draws three of them side by side. a0 the layer, a1 its map. */
#define STRIP_TABLE 0x08A08BDDu
extern gaddr stage_strip_before;   /* tiles.c */

static void draw_layer_16_strips(Cpu *c, gaddr layer, gaddr map) {
    c->r[29] -= 0x80;
    gaddr st = c->r[29] + 16;
    grid_start(st, layer);
    SET16(st + G_SCROLL, *H(layer + 98));
    SET16(st + G_SCROLL, S16(st + G_SCROLL) & 0xFF);
    SET16(st + G_MAP_COL, *H(layer + 18));
    int32_t h = S16(st + G_MAP_COL);
    int32_t n0 = (h >> 8) & 0x7F;
    int32_t n1 = (int16_t)(n0 + 1) & 0x7F;
    int32_t n2 = (int16_t)(n1 + 1) & 0x7F;
    uint32_t strip0 = *B(STRIP_TABLE + (uint32_t)n0);
    uint32_t strip1 = *B(STRIP_TABLE + (uint32_t)n1);
    uint32_t strip2 = *B(STRIP_TABLE + (uint32_t)n2);
    int32_t s = S16(st + G_SCROLL);
    SET16(st + G_MAP_COL, h & 0xFF);
    h = S16(st + G_MAP_COL);
    grid_parts(st, s, h);
    *W(st + G_LINE) = map;
    *W(st + G_STRIPS) = map + strip1 * 64;
    *W(st + G_STRIPS + 4) = map + strip2 * 64;
    gaddr first = map + strip0 * 64;
    /* WIDE NATIVE: the strip before the first (tiles.c draws the columns left of the first strip's start
       from its end) */
    stage_strip_before = map + (uint32_t)*B(STRIP_TABLE + (uint32_t)((n0 - 1) & 0x7F)) * 64u;
    *W(st + G_LINE) = first;
    *W(st + G_BASE) = first;
    SET16(st + G_MAP_COL, h >> 4);
    int32_t col0 = S16(st + G_MAP_COL);
    gaddr line = first + (uint32_t)((s & 0xFFF0) * 52);           /* 0xD0 words a row */
    *W(st + G_LINE) = line + (uint32_t)(col0 << 2);
    SET16(st + G_COLS_TO_WRAP, 0x10 - col0);
    SET16(st + G_ROW_WRAP, 0x10 - (s >> 4));
    SET16(st + G_ROWS_PER_WRAP, 0x10);
    grid_screen(st);
    SET16(st + G_ROWS, 0x10);
    SET16(st + G_DEPTH, *B(layer + 122));
    draw_record(c, st, map, 0x0881C59C);
    stage_strip_before = 0;
    c->r[29] += 0x80;
}

SFA_FUNC(0881C354, stage_layer_16_strips) {
    gaddr layer = ARG(0), map = ARG(1);
    draw_layer_16_strips(c, layer, map);
}

/* ---- 0x08819498: a picture layer of one quad (BISON's mountains) ----
   Layer rows 0x60 - 0xB0 and columns 0x200 - 0x300 less the fighters' layer's travel, palette 6, the
   texture's two halves (v 0 or 0x50) swapping every 8 frames unless the background is stopped. */
static int anim_stopped(void) {
    return ((int8_t)*B(PAUSED) | (int8_t)*B(BATTLE + 15)) != 0 || (int8_t)*B(BATTLE + 37) != 0;
}

static void draw_picture(Cpu *c, gaddr layer) {
    c->r[29] -= 0x30;
    float oy_half = F(OFFSET_Y) + 0.5f, ox_half = F(OFFSET_X) + 0.5f, f;
    f = (float)(0x60 - S16(layer + 98));
    f = f * F(SCALE_Y);
    f = f + oy_half;
    int32_t top = CVT_TRUNC(NULL, whole(f));
    SET16(QUAD_XY + 6, top); SET16(QUAD_XY + 2, top);
    f = (float)(0xB0 - S16(layer + 98));
    f = f * F(SCALE_Y);
    f = f + oy_half;
    int32_t bottom = CVT_TRUNC(NULL, whole(f));
    SET16(QUAD_XY + 14, bottom); SET16(QUAD_XY + 10, bottom);
    int32_t moved = (int16_t)travel(layer);
    f = (float)(0x200 - moved);
    f = f * F(SCALE_X);
    f = f + ox_half;
    int32_t left = CVT_TRUNC(NULL, whole(f));
    SET16(QUAD_XY + 12, left); SET16(QUAD_XY + 0, left);
    f = (float)(0x300 - moved);
    f = f * F(SCALE_X);
    f = f + ox_half;
    int32_t right = CVT_TRUNC(NULL, whole(f));
    SET16(QUAD_UV + 12, 0); SET16(QUAD_UV + 0, 0);
    SET16(QUAD_XY + 8, right); SET16(QUAD_XY + 4, right);
    SET16(QUAD_UV + 8, 0x100); SET16(QUAD_UV + 4, 0x100);
    c->r[31] = 0x088195FC;
    CALL1(f_089E0EA4, 6);

    int32_t frame = (int8_t)*B(layer + 131);
    if (!anim_stopped()) {
        uint32_t n = *W(layer + 60) - 1;
        *W(layer + 60) = n;
        if (n == 0) {
            *W(layer + 60) = 8;
            *B(layer + 131) = (uint8_t)(frame ^ 1);
            frame = (int8_t)*B(layer + 131);
        }
    }
    int32_t v = frame != 0 ? 0x50 : 0;
    SET16(QUAD_UV + 2, v); SET16(QUAD_UV + 6, v);
    v = S16(QUAD_UV + 2) + 0x50;
    SET16(QUAD_UV + 14, v); SET16(QUAD_UV + 10, v);
    if ((tiles_ending_mode || tiles_ending_shift) && !tiles_ending_quad()) { c->r[29] += 0x30; return; }   /* WIDE NATIVE, an ending */
    int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, *B(layer + 122));
    set_float_arg(c, (float)depth);
    c->r[4] = QUADS; c->r[5] = 4; c->r[6] = 0x80000060u; c->r[31] = 0x088196B4;
    f_089E1618(c);
    c->r[29] += 0x30;
}

/* void draw_picture(layer *a0): wrapped so WIDE NATIVE's stage strips (src/extensions/stagestrips.c) join it */
SFA_FUNC_WRAPPED(08819498, stage_picture, stagestrips_picture) {
    gaddr layer = ARG(0);
    draw_picture(c, layer);
}

/* ---- 0x0881C0C8: a rippling picture layer (BISON's sky) ----
   Three 256-pixel columns side by side from (layer +8 less +16) / 2 - 0xC0, each 160 rows drawn as a
   1-pixel slice whose x is nudged by a wave table (0x08A08ADC, s8, 0x7F ends it) from the layer's +62.
   The texture's row counts on across the slices, palette 7 for the first 256 and 8 after. */
#define WAVE_TABLE 0x08A08ADCu

static void draw_sky(Cpu *c, gaddr layer) {
    c->r[29] -= 0x50;
    CALL1(f_089E0EA4, 7);
    int32_t x0 = (int16_t)(((int32_t)(*W(layer + 8) - *W(layer + 16)) >> 17) - 0xC0);
    int32_t up = (int16_t)(-S16(layer + 98));
    SET16(QUAD_UV + 12, 0); SET16(QUAD_UV + 0, 0);
    SET16(QUAD_UV + 8, 0x100); SET16(QUAD_UV + 4, 0x100);
    SET16(QUAD_UV + 10, 0);
    float ox_half = F(OFFSET_X) + 0.5f;
    float fup = (float)up;
    float oy_half = F(OFFSET_Y) + 0.5f;
    int32_t texrow = 1;
    for (int col = 0; col < 3; col++) {
        float f = F(SCALE_Y);
        f = fup * f;
        f = f + oy_half;
        SET16(QUAD_XY + 10, CVT_TRUNC(NULL, whole(f)));
        int32_t row = up + 1;
        gaddr wave = WAVE_TABLE + (uint32_t)S16(layer + 62);
        for (int i = 0; i < 160; i++, row++, wave++) {
            if ((int8_t)*B(wave) == 0x7F) wave = WAVE_TABLE;
            int32_t x = x0 + (int8_t)*B(wave);
            float sx = F(SCALE_X);
            float l = (float)x, r = (float)(x + 0x100);
            int32_t prev = S16(QUAD_XY + 10);              /* the slice's top: the last one's bottom */
            l = l * sx;
            r = r * sx;
            l = ox_half + l;
            r = ox_half + r;
            SET16(QUAD_XY + 6, prev);
            int32_t left = CVT_TRUNC(NULL, l), right = CVT_TRUNC(NULL, r);
            SET16(QUAD_XY + 12, left); SET16(QUAD_XY + 0, left);
            SET16(QUAD_XY + 8, right); SET16(QUAD_XY + 4, right);
            SET16(QUAD_XY + 2, prev);
            f = (float)row;
            f = f * F(SCALE_Y);
            f = f + oy_half;
            int32_t bottom = CVT_TRUNC(NULL, whole(f));
            int32_t was = texrow;
            texrow = (int16_t)(texrow + 1);
            SET16(QUAD_XY + 14, bottom); SET16(QUAD_XY + 10, bottom);
            if (was == 0x100) {                            /* the first texture used up: the second */
                c->r[31] = 0x0881C2A0;
                CALL1(f_089E0EA4, 8);
                SET16(QUAD_UV + 10, 0);
                texrow = 1;
            }
            int32_t v = S16(QUAD_UV + 10);
            SET16(QUAD_UV + 14, texrow);
            SET16(QUAD_UV + 6, v); SET16(QUAD_UV + 2, v);
            SET16(QUAD_UV + 10, texrow);
            uint16_t keep[16];   /* WIDE NATIVE, an ending: the slice cut or scaled, then put back (the next
                                    slice starts from this one's bottom) */
            if (tiles_ending_mode) { memcpy(keep, guest_ptr(QUAD_XY), 16); memcpy(keep + 8, guest_ptr(QUAD_UV), 16); }
            if (!(tiles_ending_mode || tiles_ending_shift) || tiles_ending_quad()) {
                int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, *B(layer + 122));
                set_float_arg(c, (float)depth);
                c->r[4] = QUADS; c->r[5] = 4; c->r[6] = 0x80000020u; c->r[31] = 0x0881C2E4;
                f_089E1618(c);
            }
            if (tiles_ending_mode) { memcpy(guest_ptr(QUAD_XY), keep, 16); memcpy(guest_ptr(QUAD_UV), keep + 8, 16); }
        }
        x0 = (int16_t)(x0 + 0x100);
    }
    c->r[29] += 0x50;
}

/* void draw_sky(layer *a0): wrapped as stage_picture is */
SFA_FUNC_WRAPPED(0881C0C8, stage_sky, stagestrips_picture) {
    gaddr layer = ARG(0);
    draw_sky(c, layer);
}

static void call2(Cpu *c, GuestFunc f, gaddr a0, gaddr a1, uint32_t back) {
    c->r[4] = a0; c->r[5] = a1; c->r[31] = back;
    f(c);
}

static void draw_background(Cpu *c) {
    if ((int8_t)*B(BATTLE + 23) != 0) return;                   /* a skipped frame draws nothing */
    *W(QUADS + 0) = 0x08B014ECu;                                 /* corners */
    *W(QUADS + 4) = 0x08B014FCu;                                 /* colours */
    *W(QUADS + 8) = 0x08B0150Cu;                                 /* u / v */
    *W(QUADS + 12) = 4;
    uint32_t v = (uint32_t)S16(FADE);
    uint32_t colour = v + (v << 8) + (v << 16) - 0x1000000u;     /* grey of that brightness, opaque */
    const gaddr floor_quad = 0x08B0151Cu;
    *W(floor_quad + 60) = colour; *W(floor_quad + 44) = colour;
    *W(floor_quad + 28) = colour; *W(floor_quad + 12) = colour;
    *W(0x08B014FCu + 12) = colour; *W(0x08B014FCu + 8) = colour; *W(0x08B014FCu + 4) = colour;
    *W(0x08B014FCu) = colour;
    CALL1(f_089E0CF4, 0x08A053CCu);
    *W(0x08B0155Cu) = 0xA0;                                      /* the floor's first texture */

    int32_t stage = S16(BATTLE);
    /* layer 1 */
    if (*B(LAYER_1 + 7) == 0) {
        *B(LAYER_1 + 3) = (uint8_t)((stage & 0xFF) + 0xA0);
        *B(LAYER_1 + 7) = 1;
    } else if (*B(LAYER_1 + 123) & 2) {
        uint8_t kind = *B(LAYER_1 + 3);
        if (kind == 0xAA || kind == 0xBE) call2(c, f_0881C0C8, LAYER_1, 0xBE, 0x08819E14);
        else if (kind == 0xB5) call2(c, f_0881A8E0, LAYER_1, MAPS + 0x2400, 0x08819E38);
        else if (kind == 0xC0) call2(c, f_0881A8E0, LAYER_1, MAPS + 0x3400, 0x08819E58);
        else call2(c, f_0881BD28, LAYER_1, MAPS + 0x2000, 0x08819E6C);
    }
    /* the floor (the stage's own routine), from the second frame */
    if (*B(SHAKE + 7) == 0) *B(SHAKE + 7) = 1;
    else if (*B(LAYER_1 + 123) & 8) { c->r[4] = LAYER_1; c->r[31] = 0x08819EB0; f_088192C0(c); }
    /* layer 0 */
    if (*B(LAYER_0 + 7) == 0) {
        *B(LAYER_0 + 3) = (uint8_t)((S16(BATTLE) & 0xFF) + 0x70);
        *B(LAYER_0 + 7) = 1;
    } else if (*B(LAYER_1 + 123) & 1) {
        uint8_t kind = *B(LAYER_0 + 3);
        if (kind == 0x7A || kind == 0x8E) call2(c, f_08819498, LAYER_0, 0x8E, 0x08819EFC);
        else if (kind == 0x90) call2(c, f_0881C354, LAYER_0, MAPS, 0x08819F24);
        else if ((int8_t)*B(LAYER_0 + 155) != 0) call2(c, f_0881A8E0, LAYER_0, MAPS, 0x08819F44);
        else call2(c, f_0881AB28, LAYER_0, MAPS, 0x08819F58);
    }
    /* layer 2 */
    if (*B(LAYER_2 + 7) == 0) {
        *B(LAYER_2 + 3) = (uint8_t)((S16(BATTLE) & 0xFF) + 0xD0);
        *B(LAYER_2 + 7) = 1;
    } else if (*B(LAYER_1 + 123) & 4) {
        uint8_t kind = *B(LAYER_2 + 3);
        if (kind == 0xE5) call2(c, f_0881AB28, LAYER_2, MAPS + 0x4400, 0x08819FB4);
        else if (kind == 0xF0) call2(c, f_0881AB28, LAYER_2, MAPS + 0x5400, 0x08819FD0);
        else if ((int8_t)*B(LAYER_2 + 155) != 0) call2(c, f_0881A8E0, LAYER_2, MAPS + 0x4000, 0x08819FF0);
        else call2(c, f_0881AB28, LAYER_2, MAPS + 0x4000, 0x0881A004);
    }
}

/* (): the stage's background, once a frame. */
SFA_FUNC(08819CFC, stage_background) {
    c->r[29] -= 0x30;
    draw_background(c);
    c->r[29] += 0x30;
}

/* ---- the select / VS / map screens' backdrops: the stage's camera layers shown behind a menu ---- */

/* The quad list and its colour as the background sets them (0x08819CFC's start), the texture list a0 and
   the floor's first texture. */
static void backdrop_start(Cpu *c, uint32_t textures, uint32_t floor_texture, uint32_t back) {
    *W(QUADS + 0) = 0x08B014ECu; *W(QUADS + 4) = 0x08B014FCu; *W(QUADS + 8) = 0x08B0150Cu; *W(QUADS + 12) = 4;
    uint32_t v = (uint32_t)S16(FADE);
    uint32_t colour = v + (v << 8) + (v << 16) - 0x1000000u;
    const gaddr floor_quad = 0x08B0151Cu;
    *W(floor_quad + 60) = colour; *W(floor_quad + 44) = colour;
    *W(floor_quad + 28) = colour; *W(floor_quad + 12) = colour;
    *W(0x08B014FCu + 12) = colour; *W(0x08B014FCu + 8) = colour; *W(0x08B014FCu + 4) = colour;
    *W(0x08B014FCu) = colour;
    c->r[4] = textures; c->r[31] = back; f_089E0CF4(c);
    *W(0x08B0155Cu) = floor_texture;
}

/* A layer's first frame sets it up (+7); later frames draw it when layer 1's +123 has its bit, with
   the 16-pixel drawer if its +155 says so. */
static void backdrop_layer(Cpu *c, gaddr layer, uint32_t bit, gaddr map, uint32_t back) {
    if (*B(layer + 7) == 0) { *B(layer + 7) = 1; return; }
    if (!(*B(LAYER_1 + 123) & bit)) return;
    if ((int8_t)*B(layer + 155) != 0) call2(c, f_0881A8E0, layer, map, back);
    else call2(c, f_0881AB28, layer, map, back + 0x14);
}

/* ---- 0x0881A040: the select screens' backdrop: kinds 3 × the screen's backdrop number (0x08E75C52)
   + 0 / 1 / 2, textures 0x08A054D0[it] ---- */
SFA_FUNC(0881A040, screen_backdrop_draw) {
    c->r[29] -= 0x20;
    if ((int8_t)*B(BATTLE + 23) == 0) {
        uint32_t n = *H(0x08E75C52u);
        backdrop_start(c, *W(0x08A054D0u + n * 4u), 0xD0, 0x0881A100);
        n = *H(0x08E75C52u);
        uint32_t k = n + n * 2;
        int32_t ks = (int8_t)k;
        *B(LAYER_0 + 3) = (uint8_t)k;
        *B(LAYER_1 + 3) = (uint8_t)(ks + 1);
        *B(LAYER_2 + 3) = (uint8_t)(ks + 2);
        *W(LAYER_0 + 96) = 0u - *W(LAYER_0 + 20) - ((uint32_t)S16(LAYER_0 + 90) << 16);
        *W(LAYER_1 + 96) = 0u - *W(LAYER_1 + 20) - ((uint32_t)S16(LAYER_1 + 90) << 16);
        *W(LAYER_2 + 96) = 0u - *W(LAYER_2 + 20) - ((uint32_t)S16(LAYER_2 + 90) << 16);
        *W(SHAKE + 112) = 0;
        backdrop_layer(c, LAYER_1, 2, MAPS + 0x2000, 0x0881A1D8);
        backdrop_layer(c, LAYER_0, 1, MAPS, 0x0881A234);
        backdrop_layer(c, LAYER_2, 4, MAPS + 0x4000, 0x0881A290);
    }
    c->r[29] += 0x20;
}

/* ---- 0x08819144: the VS screen's backdrop: layers 1 and 2 (kind 0xFF) from the maps 0x08A0793C /
   0x08A0813C ---- */
SFA_FUNC(08819144, screen_vs_backdrop_draw) {
    c->r[29] -= 0x20;
    if ((int8_t)*B(BATTLE + 23) == 0) {
        backdrop_start(c, *W(0x08A0552Cu), 0xE0, 0x088191F0);
        *B(LAYER_1 + 3) = 0xFF; *B(LAYER_2 + 3) = 0xFF;
        *W(LAYER_1 + 96) = 0u - *W(LAYER_1 + 20);
        *W(LAYER_2 + 96) = 0u - *W(LAYER_2 + 20);
        *W(SHAKE + 112) = 0;
        if (*B(LAYER_1 + 7) == 0) *B(LAYER_1 + 7) = 1;
        else if (*B(LAYER_1 + 123) & 2) call2(c, f_0881AB28, LAYER_1, 0x08A0793Cu, 0x08819264);
        if (*B(LAYER_2 + 7) == 0) *B(LAYER_2 + 7) = 1;
        else if (*B(LAYER_1 + 123) & 4) call2(c, f_0881AB28, LAYER_2, 0x08A0813Cu, 0x088192A0);
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881CFD0: (x, y): a 160 × 32 picture (texels 0 - 0xA0 × 0xC0 - 0xE0 of the list 0x08A05078,
   palette 0) at depth column 7, from a quad list on its own stack ---- */
SFA_FUNC(0881CFD0, screen_backdrop_picture) {
    int32_t x = (int16_t)ARG(0), y = (int16_t)ARG(1);
    c->r[29] -= 0x60;
    gaddr sp = c->r[29], d = sp + 16, xy = sp + 0x20, col = sp + 0x30, uv = sp + 0x40;
    *W(d) = xy; *W(d + 4) = col; *W(d + 8) = uv; *W(d + 12) = 4;
    for (int i = 0; i < 4; i++) *W(col + 4u * (uint32_t)i) = 0xFFFFFFFFu;
    static const uint16_t UV[8] = { 0, 0xC0, 0xA0, 0xC0, 0xA0, 0xE0, 0, 0xE0 };
    for (int i = 0; i < 8; i++) *H(uv + 2u * (uint32_t)i) = UV[i];
    c->r[4] = 0x08A05078u; c->r[31] = 0x0881D05C; f_089E0CF4(c);
    c->r[4] = 0; c->r[31] = 0x0881D064; f_089E0EA4(c);
    float sx = F(SCALE_X), sy = F(SCALE_Y), ox = F(OFFSET_X), oy = F(OFFSET_Y), f;
    f = (float)x; f = sx * f; int32_t x0 = (int16_t)CVT_TRUNC(NULL, f);
    f = (float)(x + 0xA0); f = sx * f; int32_t x1 = (int16_t)CVT_TRUNC(NULL, f);
    f = (float)y; f = sy * f; int32_t y0 = (int16_t)CVT_TRUNC(NULL, f);
    f = (float)(y + 0x20); f = sy * f; int32_t y1 = (int16_t)CVT_TRUNC(NULL, f);
    f = (float)x0; f = f + ox; int32_t l = CVT_TRUNC(NULL, f);
    f = (float)x1; f = f + ox; int32_t r = CVT_TRUNC(NULL, f);
    f = (float)y0; f = f + oy; int32_t t = CVT_TRUNC(NULL, f);
    f = (float)y1; f = f + oy; int32_t b = CVT_TRUNC(NULL, f);
    SET16(xy + 12, l); SET16(xy + 0, l); SET16(xy + 8, r); SET16(xy + 4, r);
    SET16(xy + 6, t); SET16(xy + 2, t); SET16(xy + 14, b); SET16(xy + 10, b);
    int32_t depth = (int32_t)CALL2(f_0880A3C4, 1, 7);
    set_float_arg(c, (float)depth);
    c->r[4] = d; c->r[5] = 4; c->r[6] = 0x80000060u; c->r[31] = 0x0881D194;
    f_089E1618(c);
    c->r[29] += 0x60;
}
