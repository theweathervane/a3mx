/* The stage backgrounds' tile drawers: a stage layer is a grid of square tiles (32 or 16 pixels) read
 * from a tile map that wraps, drawn one textured quad per tile, row by row, from the layer's scroll.
 *
 *   0x0881AB28  32-pixel tiles, 13 columns by 9 rows. Works out the grid from the layer (its scroll,
 *               kind and animation) into a record on its own stack frame, then draws it.
 *   0x0881B268  16-pixel tiles, 25 columns by a row count the record gives. The record is the caller's
 *               (it persists in game memory), already worked out, so every field it writes is kept.
 *
 * Both draw through the same record layout (GRID_*). A map word: bit 31 set = a tile to draw; bits
 * 28-29 flip it (1 across, 2 up and down); bit 30 picks the blend; bits 8-15 its palette, bits 0-7 its
 * texel cell (16 x 16 cells of 16 texels).
 *
 * WIDE FORCED: the 16:9 patch (Rickow, sfa3-PSP-widescreen, MIT) changes these drawers to start
 * further left and draw more columns: 0x0881AB28 two tiles earlier and 17 columns, 0x0881B268 three
 * tiles earlier and 31. That is done here as the patch does it, while rt_widescreen is set, so the
 * reference run (the patched translation) and this code agree with WIDE FORCED on or off. It is the
 * first part of the patch done in native code (docs/WIDESCREEN.md).
 *
 * 0x0881B7C8, the third drawer, stays translated: only 0x0881A2C8 calls it, and no scenario reaches it.
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA) and round
 * differently from the PSP.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* the widescreen patch's switch: extensions/widescreen.c (extensions_off.c: 0) */

void f_089E0EA4(Cpu *c);   /* selects a palette (a0) */
void f_0880A3C4(Cpu *c);   /* a draw depth from the depth table: a0 the table's row, a1 its column */
void f_089E1618(Cpu *c);   /* draws a quad list: a0 its descriptor, a1 vertex count, a2 flags, f12 depth */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline void      SET16(gaddr a, int32_t v) { *H(a) = (uint16_t)v; }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

#define SCALE_X      0x08E759E8u   /* float: screen scale */
#define SCALE_Y      0x08E759ECu
#define OFFSET_X     0x08E75A04u   /* float: screen offset */
#define OFFSET_Y     0x08E75A0Cu
#define QUAD         0x08B014DCu   /* the quad list's descriptor (its positions, colours and texels) */
#define QUAD_XY      0x08B014ECu   /* s16 x, y for its 4 corners: top left, top right, bottom right, bottom left */
#define QUAD_UV      0x08B0150Cu   /* s16 u, v, the same order */
#define PAUSED       0x08E75969u   /* s8: nonzero stops background animation (with the two below) */
#define FIGHT        0x092E1228u   /* +15, +37: s8 flags that stop it too */
#define ANIM_B268    0x092F8800u   /* 0x0881B268's animation for kind 0x85: +60 countdown, +131 frame */
#define ANIM_B268_ROWS 0x08A08AD8u /* s8[4]: the frame's offset in the map, in steps of 192 bytes */

/* The grid record (halfwords unless marked) */
enum {
    GRID_COL = 0,            /* word: column being drawn */
    GRID_ROW = 4,            /* word: row being drawn */
    GRID_X = 8,              /* the tile's left edge */
    GRID_Y = 12,             /* the row's top edge */
    GRID_Y0 = 14,            /* (0x0881AB28 only) the first row's top */
    GRID_TILE = 16,          /* word: map address of the tile */
    GRID_LINE = 20,          /* word: map address of the row's first tile */
    GRID_STEP_X = 24, GRID_STEP_Y = 26,     /* a tile's size on screen */
    GRID_OFS_X = 28, GRID_OFS_Y = 30,       /* the screen offset, rounded */
    GRID_X0 = 32,            /* a row's first left edge */
    GRID_COL_WRAP = 34,      /* the column at which the map wraps across */
    GRID_ROW_WRAP = 36,      /* the row at which it wraps down */
    GRID_U = 38, GRID_V = 40,
    GRID_LEFT = 42, GRID_RIGHT = 44, GRID_TOP = 46, GRID_BOTTOM = 48,
    GRID_MAP_COL = 54,       /* the map column the grid starts at */
    GRID_SCROLL = 56,        /* vertical scroll (0x0881B268's kind 0x90: its strips' offset) */
    GRID_WORD = 58,          /* the tile's low 16 bits */
    GRID_DEPTH = 60,         /* (0x0881B268) the depth table column */
    GRID_ROWS = 62,          /* (0x0881B268) rows to draw */
    GRID_STRIPS = 72,        /* (0x0881B268, kind 0x90) words: each strip's map address */
    GRID_BASE = 80,          /* (0x0881B268) word: map address of the first row */
    GRID_LINE_STEP = 84,     /* map words from one row to the next */
    GRID_COL_BACK = 86,      /* map words back when wrapping across */
    GRID_COLS_TO_WRAP = 88,  /* columns from the grid's first to the wrap */
    GRID_ROWS_PER_WRAP = 92, /* rows between wraps down */
    GRID_KIND = 94,
    GRID_NEW_WRAP = 96,      /* (0x0881B268) set when the map wrapped down */
    GRID_LAYER = 100,        /* (0x0881AB28) word: the layer */
    GRID_MAP = 104,          /* (0x0881AB28) word: map address of the first row */
};

/* What differs between the two drawers' loops */
typedef struct {
    int size;            /* tile size in texels */
    int cols, wide_cols; /* columns, and with WIDE FORCED */
    int wide_tiles;      /* tiles further left with WIDE FORCED */
    int b268;            /* 0x0881B268: its rows, wrap and strips; else 0x0881AB28's */
} Grid;

static const Grid GRID_AB28 = { 32, 13, 17, 2, 0 };
static const Grid GRID_B268 = { 16, 25, 31, 3, 1 };

static int columns(const Grid *g) { return rt_widescreen ? g->wide_cols : g->cols; }

/* One tile: its palette, the quad's corners (flipped as the map word says) and texels, then the draw. */
static void draw_tile(Cpu *c, gaddr st, const Grid *g) {
    SET16(st + GRID_WORD, *W(*W(st + GRID_TILE)));
    CALL1(f_089E0EA4, (uint32_t)(*H(st + GRID_WORD) >> 8));
    int32_t x = S16(st + GRID_X);
    SET16(st + GRID_LEFT, x + S16(st + GRID_OFS_X));
    SET16(st + GRID_RIGHT, x + S16(st + GRID_STEP_X) + S16(st + GRID_OFS_X));

    uint32_t flip = *W(*W(st + GRID_TILE)) & 0x30000000u;
    int32_t l = S16(st + GRID_LEFT), r = S16(st + GRID_RIGHT);
    int32_t t = S16(st + GRID_TOP), b = S16(st + GRID_BOTTOM);
    if (flip & 0x10000000u) { int32_t s = l; l = r; r = s; }
    if (flip & 0x20000000u) { int32_t s = t; t = b; b = s; }
    SET16(QUAD_XY + 0, l);  SET16(QUAD_XY + 12, l);
    SET16(QUAD_XY + 4, r);  SET16(QUAD_XY + 8, r);
    SET16(QUAD_XY + 2, t);  SET16(QUAD_XY + 6, t);
    SET16(QUAD_XY + 10, b); SET16(QUAD_XY + 14, b);

    uint16_t w = *H(st + GRID_WORD);
    SET16(st + GRID_U, (w & 0xF) << 4);
    SET16(st + GRID_V, w & 0xF0);
    int32_t u = S16(st + GRID_U), v = S16(st + GRID_V);
    SET16(QUAD_UV + 0, u);  SET16(QUAD_UV + 12, u);
    SET16(QUAD_UV + 4, u + g->size);  SET16(QUAD_UV + 8, u + g->size);
    SET16(QUAD_UV + 2, v);  SET16(QUAD_UV + 6, v);
    SET16(QUAD_UV + 10, v + g->size); SET16(QUAD_UV + 14, v + g->size);

    uint32_t column = g->b268 ? (uint32_t)S16(st + GRID_DEPTH) : *B(*W(st + GRID_LAYER) + 122);
    uint32_t blend = *W(*W(st + GRID_TILE)) & 0x40000000u;
    uint32_t depth = CALL2(f_0880A3C4, 1, column);
    float z = (float)(int32_t)depth;
    memcpy(&c->f[12], &z, 4);                                   /* the float argument */
    CALL3(f_089E1618, QUAD, 4, blend ? 0x80000060u : 0x80000020u);
}

/* 0x0881B268's kind 0x90 wraps across into the next of its strips, each a map of its own. */
static void next_strip(gaddr st, gaddr *strip) {
    uint32_t map = *W(*strip + GRID_STRIPS);
    *W(st + GRID_TILE) = map;
    int32_t row = (int32_t)*W(st + GRID_ROW);
    if (S16(st + GRID_NEW_WRAP) != 0) {
        SET16(st + GRID_SCROLL, -(row << 4));
        SET16(st + GRID_NEW_WRAP, 0);
    }
    uint32_t v = (uint32_t)S16(st + GRID_SCROLL) & 0xFFF0;
    int32_t ofs = (int16_t)(v * 13);
    int32_t line = S16(st + GRID_LINE_STEP) * row;
    *W(st + GRID_TILE) = map + (uint32_t)(ofs << 2) + (uint32_t)(line << 2);
    SET16(st + GRID_COL_WRAP, S16(st + GRID_COL_WRAP) + 0x10);
    *strip += 4;
}

/* The grid, row by row: the map wraps down at GRID_ROW_WRAP rows and across at GRID_COL_WRAP columns. */
static void draw_grid(Cpu *c, gaddr st, const Grid *g) {
    *W(st + GRID_ROW) = 0;
    int32_t row = 0;
    while (row < (g->b268 ? S16(st + GRID_ROWS) : 9)) {
        if (row == S16(st + GRID_ROW_WRAP)) {
            uint32_t first = g->b268 ? *W(st + GRID_BASE) : *W(st + GRID_MAP);
            *W(st + GRID_LINE) = first + (uint32_t)(S16(st + GRID_MAP_COL) << 2);
            SET16(st + GRID_ROW_WRAP, S16(st + GRID_ROW_WRAP) + S16(st + GRID_ROWS_PER_WRAP));
            if (g->b268) SET16(st + GRID_NEW_WRAP, 1);
        }
        int32_t y = S16(st + GRID_Y) + S16(st + GRID_STEP_Y);
        int32_t top = S16(st + GRID_BOTTOM);
        *W(st + GRID_TILE) = *W(st + GRID_LINE);
        if (rt_widescreen) {                                     /* WIDE FORCED: start further left */
            SET16(st + GRID_X, S16(st + GRID_X0) - g->wide_tiles * g->size);
            *W(st + GRID_TILE) -= (uint32_t)(g->wide_tiles * 4);
        } else {
            SET16(st + GRID_X, S16(st + GRID_X0));
        }
        SET16(st + GRID_Y, y);
        SET16(st + GRID_TOP, top);
        SET16(st + GRID_BOTTOM, S16(st + GRID_Y) + S16(st + GRID_OFS_Y));
        SET16(st + GRID_COL_WRAP, S16(st + GRID_COLS_TO_WRAP) + (rt_widescreen ? g->wide_tiles : 0));

        *W(st + GRID_COL) = 0;
        gaddr strip = st;
        int32_t col = 0;
        do {
            if (col == S16(st + GRID_COL_WRAP)) {
                if (g->b268 && S16(st + GRID_KIND) == 0x90) next_strip(st, &strip);
                else *W(st + GRID_TILE) -= (uint32_t)(S16(st + GRID_COL_BACK) << 2);
            }
            gaddr tile = *W(st + GRID_TILE);
            if ((int32_t)*W(tile) < 0) draw_tile(c, st, g);
            col++;
            SET16(st + GRID_X, S16(st + GRID_X) + S16(st + GRID_STEP_X));
            *W(st + GRID_COL) = (uint32_t)col;
            *W(st + GRID_TILE) = tile + 4;
        } while (col < columns(g));

        row++;
        *W(st + GRID_ROW) = (uint32_t)row;
        *W(st + GRID_LINE) += (uint32_t)(S16(st + GRID_LINE_STEP) << 2);
    }
}

/* Background animation is stopped while any of three flags is set (pause and two fight states); the
   callers then run a countdown and step a frame of their own. */
static int anim_stopped(void) {
    return ((int8_t)*B(PAUSED) | (int8_t)*B(FIGHT + 15)) != 0 || (int8_t)*B(FIGHT + 37) != 0;
}

/* ---- 0x0881AB28: a 32-pixel layer ----
   a0 the layer: +2, +3 its kind (5: a 16 x 104-tile map that scrolls up through 0xD00 pixels; 0xDA and
   0xEE: an animated one of 11 frames, each 96 bytes on in the map; others 32 tiles wide), +18 horizontal and +98 vertical
   scroll, +60 the animation's countdown, +122 its depth column, +131 its frame. a1 the tile map. */
static void draw_layer_32(Cpu *c, gaddr layer, gaddr map) {
    c->r[29] -= 0xB0;                       /* the original's frame: the record lives in it, and the */
    gaddr st = c->r[29] + 16;               /* callees' frames sit below it as in the original */

    SET16(st + 64, *B(layer + 2));
    SET16(st + GRID_KIND, *B(layer + 3));
    float sx = F(SCALE_X), sy = F(SCALE_Y), f;
    f = sx * 32.0f;
    SET16(st + GRID_STEP_X, CVT_TRUNC(NULL, f));
    f = sy * 32.0f;
    SET16(st + GRID_STEP_Y, CVT_TRUNC(NULL, f));
    int32_t kind = S16(st + GRID_KIND);
    SET16(st + GRID_SCROLL, *H(layer + 98));

    /* the vertical scroll, kept inside the map's height */
    int32_t s = S16(st + GRID_SCROLL);
    if (kind == 5) {
        if (s < 0) {
            do s = (int16_t)(s + 0xD00); while (s < 0);
            SET16(st + GRID_SCROLL, s);
        }
    } else {
        SET16(st + GRID_SCROLL, (kind == 0xDA || kind == 0xEE) ? (s & 0xFF) : (s & 0x1FF));
        s = S16(st + GRID_SCROLL);
    }
    SET16(st + GRID_MAP_COL, *H(layer + 18));
    SET16(st + GRID_MAP_COL, S16(st + GRID_MAP_COL) & (kind == 5 ? 0x1FF : 0x3FF));
    int32_t h = S16(st + GRID_MAP_COL);

    /* the part of a tile scrolled past, on screen, and the first tile: rows from the vertical scroll,
       columns from the horizontal one */
    float part = (float)(s & 0x1F);
    part = part * sy;
    part = -part;
    SET16(st + GRID_Y, CVT_TRUNC(NULL, part));
    part = (float)(h & 0x1F);
    part = part * sx;
    SET16(st + GRID_MAP_COL, h >> 5);
    int32_t col0 = S16(st + GRID_MAP_COL);
    *W(st + GRID_LINE) = map;
    part = -part;
    SET16(st + GRID_X0, CVT_TRUNC(NULL, part));

    int32_t row0 = s >> 5, rows_in = s & 0xFFE0;
    gaddr line;
    if (kind == 5) {
        SET16(st + GRID_ROW_WRAP, 0x68 - row0);
        SET16(st + GRID_ROWS_PER_WRAP, 0x68);
        line = map + (uint32_t)((rows_in >> 1) << 2);
        SET16(st + GRID_COLS_TO_WRAP, 0x10 - col0);
    } else if (kind == 0xDA || kind == 0xEE) {
        SET16(st + GRID_ROW_WRAP, 8 - row0);
        SET16(st + GRID_ROWS_PER_WRAP, 8);
        int32_t q = rows_in >> 2;
        line = map + (uint32_t)((q << 7) + (q << 3));
        SET16(st + GRID_COLS_TO_WRAP, 0x20 - col0);
    } else {
        SET16(st + GRID_ROW_WRAP, 0x10 - row0);
        SET16(st + GRID_ROWS_PER_WRAP, 0x10);
        line = map + (uint32_t)(rows_in << 2);
        SET16(st + GRID_COLS_TO_WRAP, 0x20 - col0);
    }
    *W(st + GRID_LINE) = line + (uint32_t)(col0 << 2);

    f = F(OFFSET_X) + 0.5f;
    SET16(st + GRID_OFS_X, CVT_TRUNC(NULL, f));
    SET16(st + GRID_Y0, S16(st + GRID_Y));
    f = F(OFFSET_Y) + 0.5f;
    SET16(st + GRID_OFS_Y, CVT_TRUNC(NULL, f));
    SET16(st + GRID_BOTTOM, S16(st + GRID_Y0) + S16(st + GRID_OFS_Y));

    gaddr first = map;
    if (kind == 5) {
        SET16(st + GRID_LINE_STEP, 0x10);
        SET16(st + GRID_COL_BACK, 0x10);
    } else if (kind == 0xDA || kind == 0xEE) {
        /* animated: 11 frames 96 bytes apart in the map, each shown for 6 calls. GRID_COL_BACK is not set: the original
           reads what its stack frame held (only a wrap across reads it). */
        SET16(st + GRID_LINE_STEP, 0x110);
        if (!anim_stopped()) {
            uint32_t n = *W(layer + 60) - 1;
            *W(layer + 60) = n;
            if (n == 0) {
                int32_t frame = (int8_t)*B(layer + 131) + 1;
                *W(layer + 60) = 6;
                *B(layer + 131) = (uint8_t)frame;
                if ((int8_t)frame == 11) *B(layer + 131) = 0;
            }
        }
        int32_t frame = (int8_t)*B(layer + 131);
        uint32_t ofs = (uint32_t)(frame * 96);
        first = map + ofs;
        *W(st + GRID_LINE) += ofs;
    } else {
        SET16(st + GRID_LINE_STEP, 0x20);
        SET16(st + GRID_COL_BACK, 0x20);
    }
    *W(st + GRID_LAYER) = layer;
    *W(st + GRID_MAP) = first;

    draw_grid(c, st, &GRID_AB28);
    c->r[29] += 0xB0;
}

/* void draw_layer_32(layer *a0, u32 *map a1) */
SFA_FUNC_WRAPPED(0881AB28, stage_draw_layer_32, stagestrips_layer_32) {
    gaddr layer = ARG(0), map = ARG(1);
    RETURN(layer);                          /* v0 as the original leaves it when it draws nothing */
    draw_layer_32(c, layer, map);
}

/* ---- 0x0881B268: a 16-pixel layer from its record ----
   a0 the record (GRID_*), worked out by the caller. Kind 0x85: animated, 4 frames of 8 each, the frame
   moving the map by a table (ANIM_B268_ROWS) of 192-byte steps; 0x90: strips (GRID_STRIPS) side by side;
   others: a 64-word-wide map. */
static void draw_layer_16(Cpu *c, gaddr st) {
    c->r[29] -= 0x40;                       /* the original's frame, so the callees' sit where they did */
    int32_t kind = S16(st + GRID_KIND);
    if (kind == 0x85) {
        SET16(st + GRID_LINE_STEP, 0x90);
        if (!anim_stopped()) {
            uint32_t n = *W(ANIM_B268 + 60) - 1;
            *W(ANIM_B268 + 60) = n;
            if (n == 0) {
                *B(ANIM_B268 + 131) = (uint8_t)((int8_t)*B(ANIM_B268 + 131) + 1);
                int32_t frame = (int8_t)*B(ANIM_B268 + 131);
                *W(ANIM_B268 + 60) = 8;
                *B(ANIM_B268 + 131) = (uint8_t)(frame & 3);
            }
        }
        int32_t frame = (int8_t)*B(ANIM_B268 + 131);
        int32_t step = (int8_t)*B(ANIM_B268_ROWS + (uint32_t)frame);
        uint32_t ofs = (uint32_t)(step * 192);
        *W(st + GRID_LINE) += ofs;
        *W(st + GRID_BASE) += ofs;
    } else if (kind == 0x90) {
        SET16(st + GRID_NEW_WRAP, 0);
        SET16(st + GRID_LINE_STEP, 0xD0);
        SET16(st + GRID_COL_BACK, 0xD0);
    } else {
        SET16(st + GRID_COL_BACK, 0x40);
        SET16(st + GRID_LINE_STEP, 0x40);
    }
    draw_grid(c, st, &GRID_B268);
    c->r[29] += 0x40;
}

/* void draw_layer_16(grid *a0) */
SFA_FUNC_WRAPPED(0881B268, stage_draw_layer_16, stagestrips_layer_16) {
    gaddr st = ARG(0);
    RETURN(c->r[2]);
    draw_layer_16(c, st);
}
