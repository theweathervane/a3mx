/* Drawing a sprite's picture as a grid of square cells: three of the four drawers that 0x0880E150
 * chooses from (by the call's flag and the sprite's +164). Each walks the picture's cell bytes row by
 * row and draws a cell as one textured quad of four vertices (0x089E1950), unless the cell is empty
 * (0xFF) or lies off the screen.
 *
 *   0x08810D84  16-pixel cells from the picture cache: 16 cells to a cached page
 *   0x088112A8  32-pixel cells from the picture cache: 4 cells to a page
 *   0x088117D0  16-pixel cells with their own palette and texel cell (the fighters use it, with the
 *               other two, for their parts)
 *   0x08811BCC  32-pixel cells with their own palette and texel cell
 *
 * WIDE FORCED: a cell is skipped when it starts left of minus its width, which in the 16:9 picture is
 * still inside the 48 pixels added on the left. That cut a fighter's back at the old 4:3 edge
 * (Blanka's shoulder against the wall). The 16:9 patch (Rickow, sfa3-PSP-widescreen, MIT) moves that
 * limit to -96 in 0x08811BCC only; tools/data/widescreen_more.json does the same in the other three, so
 * the reference run (the translation) and this code agree with WIDE FORCED on or off. The walls and
 * everything else stay as they are: only what is drawn changes (docs/WIDESCREEN.md).
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA) and round
 * differently from the PSP.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* the widescreen patch's switch: extensions/widescreen.c (extensions_off.c: 0) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }
static inline void      SET16(gaddr a, int32_t v) { *H(a) = (uint16_t)v; }

void f_0880E388(Cpu *c);   /* colour_dimmed (pool.c): a0 the draw state, a1 a colour, a2 the owner or 0 */
void f_089E0EA4(Cpu *c);   /* selects a palette (a0) */
void f_089E1438(Cpu *c);   /* returns the current texture's draw flags */
void f_089E1950(Cpu *c);   /* draws textured vertices: a0, a1 count, a2 flags, a3 blend, t0 effect */

#define DEPTHS     0x08AE03B8u   /* u16[]: depth by index */
#define SCALE_X    0x08E759E8u   /* floats: screen = position * scale + offset */
#define SCALE_Y    0x08E759ECu
#define OFFSET_X   0x08E75A04u
#define OFFSET_Y   0x08E75A0Cu
#define WIDE_LEFT  (-96)         /* WIDE FORCED: the left limit, as the patch has it in 0x08811BCC */

/* The draw state (0x08AD9320; a0) */
enum {
    DS_X = 0, DS_Y = 2,           /* the sprite's position */
    DS_CELL_X = 4,                /* the cell being drawn: screen x, then y */
    DS_ROW_Y = 6,
    DS_U = 8, DS_V = 10,          /* its texel cell */
    DS_W16 = 12, DS_H16 = 14,     /* the cell size on screen: 16-pixel cells, then 32 */
    DS_W32 = 16, DS_H32 = 18,
    DS_RIGHT = 20,                /* cells starting at or right of this are not drawn */
    DS_ROW_X = 22,                /* each row's first cell */
    DS_STEP_X = 24, DS_STEP_Y = 26,   /* from cell to cell and row to row (negative when flipped) */
    DS_CELL_X_TEST = 28,
    DS_DEPTH = 44, DS_COLOUR = 48, DS_ALPHA = 52,   /* + 52: the colour's alpha byte */
    DS_FLIP_X = 56, DS_FLIP_Y = 60,
    DS_PAGE_CELL = 64,            /* the cell's place in the cached page */
    DS_ROWS_LEFT = 68, DS_COLS_LEFT = 72,
    DS_CELL_WORD = 76,            /* 0x08811BCC: the cell's first two bytes, big-endian */
    DS_EFFECT = 86, DS_SCALED = 87,
};
/* The picture (a1): +0 where its cells start (from a2), +4 columns, +5 rows, +6 / +8 its origin */
enum { PIC_CELLS = 0, PIC_COLS = 4, PIC_ROWS = 5, PIC_ORIGIN_X = 6, PIC_ORIGIN_Y = 8 };
/* A cell: 3 bytes, the first 0xFF for an empty cell. The last: bit 7 flips it up and down, bit 6
   across, bit 5 blends it. The first two: 0x088117D0's palette and texel cell (the cached drawers
   read them when the picture is cached, elsewhere). */
enum { CELL_EMPTY = 0xFF, CELL_FLIP_Y = 7, CELL_FLIP_X = 6, CELL_BLEND = 0x20 };
#define ALPHA_OPAQUE 0xFF

/* A cached page (a3): +0 the next page, +8 its draw flags */

/* The vertices, on the frame: 16 bytes each (+0 x, +2 y, +4 depth, +6 u, +8 v, +12 colour) in the
   order top-left, bottom-left, top-right, bottom-right */
#define FRAME_SIZE 0x80
#define VERTICES   32

/* x' = trunc(x * scale + offset), rounded as the PSP does: a product, then a sum */
static int32_t to_screen(int16_t v, float scale, float offset) {
    float f = (float)v;
    f = f * scale;
    f = f + offset;
    return CVT_TRUNC(NULL, f);
}

/* The left limit: minus the cell's width, or WIDE_LEFT in WIDE FORCED */
static int32_t left_limit(int16_t width) { return rt_widescreen ? WIDE_LEFT : -(int32_t)width; }

/* A row's first cell and the step along it; a sprite flipped across starts from its far end and
   steps left. `size` is the cell size in the picture, `w` where the cell width on screen is. */
static void place_columns(gaddr ds, gaddr pic, int32_t size, int32_t w_at, int32_t x) {
    int32_t cols = *B(pic + PIC_COLS), w;
    if (*W(ds + DS_FLIP_X) != 0) {
        SET16(ds + DS_ROW_X, x + S16(pic + PIC_ORIGIN_X) - (cols + 1) * size);
        w = S16(ds + w_at);
        SET16(ds + DS_ROW_X, to_screen(S16(ds + DS_ROW_X), F(SCALE_X), F(OFFSET_X)));
        SET16(ds + DS_STEP_X, -w);
        SET16(ds + DS_ROW_X, S16(ds + DS_ROW_X) + cols * w);
    } else {
        SET16(ds + DS_ROW_X, x - S16(pic + PIC_ORIGIN_X));
        w = S16(ds + w_at);
        SET16(ds + DS_ROW_X, to_screen(S16(ds + DS_ROW_X), F(SCALE_X), F(OFFSET_X)));
        SET16(ds + DS_STEP_X, w);
    }
}

/* The first row and the step between rows, the same way up and down */
static void place_rows(gaddr ds, gaddr pic, int32_t size, int16_t h, int32_t y) {
    int32_t rows = *B(pic + PIC_ROWS);
    if (*W(ds + DS_FLIP_Y) != 0) {
        SET16(ds + DS_ROW_Y, y + S16(pic + PIC_ORIGIN_Y) - (rows + 1) * size);
        SET16(ds + DS_ROW_Y, to_screen(S16(ds + DS_ROW_Y), F(SCALE_Y), F(OFFSET_Y)));
        SET16(ds + DS_STEP_Y, -h);
        SET16(ds + DS_ROW_Y, S16(ds + DS_ROW_Y) + rows * h);
    } else {
        SET16(ds + DS_ROW_Y, y - S16(pic + PIC_ORIGIN_Y));
        SET16(ds + DS_STEP_Y, h);
        SET16(ds + DS_ROW_Y, to_screen(S16(ds + DS_ROW_Y), F(SCALE_Y), F(OFFSET_Y)));
    }
}

/* The colour (dimmed by the fade) and depth, the same in all four vertices; the alpha byte kept */
static void prepare_vertices(Cpu *c, gaddr ds, gaddr v, gaddr owner) {
    uint32_t colour = CALL3(f_0880E388, ds, *W(ds + DS_COLOUR), owner);
    uint16_t z = *H(DEPTHS + *W(ds + DS_DEPTH) * 2u);
    for (int i = 0; i < 4; i++) { *W(v + i * 16 + 12) = colour; *H(v + i * 16 + 4) = z; }
    *W(ds + DS_ALPHA) = *W(ds + DS_COLOUR) >> 24;
}

/* One cell's quad: corners from (x, y), w by h on screen; texels from (u, v), `texels` square,
   flipped by the cell's last byte and the sprite's flips */
static void cell_quad(gaddr ds, gaddr v, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t last,
                      int16_t u, int16_t vv, int32_t texels) {
    SET16(v + 16 * 0 + 0, x); SET16(v + 16 * 1 + 0, x);
    SET16(v + 16 * 2 + 0, x + w); SET16(v + 16 * 3 + 0, x + w);
    SET16(v + 16 * 0 + 2, y); SET16(v + 16 * 2 + 2, y);
    SET16(v + 16 * 1 + 2, y + h); SET16(v + 16 * 3 + 2, y + h);
    int32_t u0 = u, u1 = u + texels, v0 = vv, v1 = vv + texels;
    if ((((uint32_t)last >> CELL_FLIP_X) & 1) ^ *W(ds + DS_FLIP_X)) { int32_t t = u0; u0 = u1; u1 = t; }
    if (((uint32_t)last >> CELL_FLIP_Y) ^ *W(ds + DS_FLIP_Y)) { int32_t t = v0; v0 = v1; v1 = t; }
    SET16(v + 16 * 0 + 6, u0); SET16(v + 16 * 1 + 6, u0); SET16(v + 16 * 2 + 6, u1); SET16(v + 16 * 3 + 6, u1);
    SET16(v + 16 * 0 + 8, v0); SET16(v + 16 * 2 + 8, v0); SET16(v + 16 * 1 + 8, v1); SET16(v + 16 * 3 + 8, v1);
}

/* ---- 0x08810D84 / 0x088112A8: cells from the picture cache ---- */

typedef struct {
    int32_t size;        /* cell size in the picture and in texels: 16 or 32 */
    int32_t w_at, h_at;  /* where the cell size on screen is in the draw state */
    int32_t scaled_w;    /* the cell width a scaled sprite (+87) gets: 1.2 times the size */
    int32_t per_page;    /* cells to a cached page */
    gaddr   texels;      /* u32 pairs (u, v): each cell's place in its page */
    int     big;         /* 0x088112A8: a sprite with an effect (+86) always blends, and the next
                            page is followed without checking it */
} CachedKind;

static const CachedKind SMALL = { 16, DS_W16, DS_H16, 0x13, 16, 0x08A04BE0u, 0 };
static const CachedKind BIG   = { 32, DS_W32, DS_H32, 0x26, 4,  0x08A04C60u, 1 };

static void draw_cached(Cpu *c, const CachedKind *k, gaddr ds, gaddr pic, gaddr base, gaddr page, gaddr frame) {
    gaddr cell = base + *W(pic + PIC_CELLS);
    int32_t x = S16(ds + DS_X), y = S16(ds + DS_Y);
    int16_t h = S16(ds + k->h_at);

    if (*B(ds + DS_SCALED) != 0) {
        /* scaled: wider cells, and the origin 1.2 times as far */
        SET16(ds + k->w_at, k->scaled_w);
        int32_t origin = S16(pic + PIC_ORIGIN_X) * 12 / 10;
        int32_t cols = *B(pic + PIC_COLS), w = S16(ds + k->w_at);
        if (*W(ds + DS_FLIP_X) != 0) {
            SET16(ds + DS_ROW_X, x + origin - (cols + 1) * w);
            SET16(ds + DS_ROW_X, to_screen(S16(ds + DS_ROW_X), F(SCALE_X), F(OFFSET_X)));
            SET16(ds + DS_STEP_X, -w);
            SET16(ds + DS_ROW_X, S16(ds + DS_ROW_X) + cols * w);
        } else {
            SET16(ds + DS_ROW_X, x - origin);
            SET16(ds + DS_ROW_X, to_screen(S16(ds + DS_ROW_X), F(SCALE_X), F(OFFSET_X)));
            SET16(ds + DS_STEP_X, w);
        }
    } else {
        place_columns(ds, pic, k->size, k->w_at, x);
    }
    place_rows(ds, pic, k->size, h, y);

    gaddr v = frame + VERTICES;
    prepare_vertices(c, ds, v, page);
    *W(ds + DS_PAGE_CELL) = 0;
    *W(ds + DS_ROWS_LEFT) = *B(pic + PIC_ROWS);
    do {
        int16_t row_y = S16(ds + DS_ROW_Y);
        SET16(ds + DS_CELL_X, S16(ds + DS_ROW_X));
        *W(ds + DS_COLS_LEFT) = *B(pic + PIC_COLS);
        do {
            uint8_t first = *B(cell);
            cell++;
            if (first != CELL_EMPTY) {
                gaddr last = cell + 1;
                cell += 2;
                SET16(ds + DS_CELL_X_TEST, S16(ds + DS_CELL_X));
                int16_t cx = S16(ds + DS_CELL_X_TEST);
                uint32_t n = *W(ds + DS_PAGE_CELL);
                if (cx < S16(ds + DS_RIGHT) && left_limit(S16(ds + k->w_at)) < cx) {
                    gaddr t = k->texels + n * 8u;
                    SET16(ds + DS_U, (int32_t)*W(t));
                    SET16(ds + DS_V, (int32_t)*W(t + 4));
                    uint8_t flags_byte = *B(last);
                    cell_quad(ds, v, cx, row_y, S16(ds + k->w_at), S16(ds + k->h_at), flags_byte,
                              S16(ds + DS_U), S16(ds + DS_V), k->size);
                    uint8_t effect = *B(ds + DS_EFFECT);
                    uint32_t blend = (k->big && effect != 0) || (flags_byte & CELL_BLEND) != 0 ||
                                     *W(ds + DS_ALPHA) != ALPHA_OPAQUE;
                    CALL5(f_089E1950, v, 4, *W(page + 8), blend, effect);
                    n = *W(ds + DS_PAGE_CELL);
                }
                /* culled or not, the cell had its place in the page */
                n++;
                *W(ds + DS_PAGE_CELL) = n;
                if ((int32_t)n >= k->per_page) {
                    if (k->big || page != 0) page = *W(page);
                    *W(ds + DS_PAGE_CELL) = 0;
                }
            }
            SET16(ds + DS_CELL_X, S16(ds + DS_CELL_X) + S16(ds + DS_STEP_X));
        } while (--*W(ds + DS_COLS_LEFT) != 0);
        SET16(ds + DS_ROW_Y, row_y + S16(ds + DS_STEP_Y));
    } while (--*W(ds + DS_ROWS_LEFT) != 0);
    SET16(ds + k->w_at, k->size);   /* the width a scaled sprite changed, back */
}

/* ---- 0x088117D0: cells with their own palette and texel cell ---- */

static void draw_paletted(Cpu *c, gaddr ds, gaddr pic, gaddr base, gaddr frame) {
    gaddr cell = base + *W(pic + PIC_CELLS);
    place_columns(ds, pic, 16, DS_W16, S16(ds + DS_X));
    place_rows(ds, pic, 16, S16(ds + DS_H16), S16(ds + DS_Y));

    gaddr v = frame + VERTICES;
    prepare_vertices(c, ds, v, 0);
    *W(ds + DS_ROWS_LEFT) = *B(pic + PIC_ROWS);
    do {
        SET16(ds + DS_CELL_X, S16(ds + DS_ROW_X));
        *W(ds + DS_COLS_LEFT) = *B(pic + PIC_COLS);
        do {
            uint8_t first = *B(cell);
            cell++;
            if (first != CELL_EMPTY) {
                CALL1(f_089E0EA4, first);                         /* the cell's palette */
                SET16(ds + DS_U, (*B(cell) & 0xF) << 4);          /* its texel cell: 16 by 16 */
                SET16(ds + DS_V, *B(cell) & 0xF0);
                SET16(ds + DS_CELL_X_TEST, S16(ds + DS_CELL_X));
                int16_t cx = S16(ds + DS_CELL_X_TEST);
                gaddr last = cell + 1;
                cell += 2;
                if (cx < S16(ds + DS_RIGHT) && left_limit(S16(ds + DS_W16)) < cx) {
                    int16_t row_y = S16(ds + DS_ROW_Y);
                    uint8_t flags_byte = *B(last);
                    cell_quad(ds, v, cx, row_y, S16(ds + DS_W16), S16(ds + DS_H16), flags_byte,
                              S16(ds + DS_U), S16(ds + DS_V), 16);
                    uint32_t blend = (flags_byte & CELL_BLEND) != 0 || *W(ds + DS_ALPHA) != ALPHA_OPAQUE;
                    f_089E1438(c);
                    uint32_t flags = c->r[2];
                    CALL5(f_089E1950, v, 4, flags, blend, *B(ds + DS_EFFECT));
                }
            }
            SET16(ds + DS_CELL_X, S16(ds + DS_CELL_X) + S16(ds + DS_STEP_X));
        } while (--*W(ds + DS_COLS_LEFT) != 0);
        SET16(ds + DS_ROW_Y, S16(ds + DS_ROW_Y) + S16(ds + DS_STEP_Y));
    } while (--*W(ds + DS_ROWS_LEFT) != 0);
}

/* ---- 0x08811BCC: 32-pixel cells with their own palette and texel cell ---- */

/* The cell's first two bytes as one big-endian word (+76): the palette above bit 6, and the texel
   cell in the low 6 bits (8 cells of 32 to a row of the texture). Unlike 0x088117D0 the palette and
   texel cell are set only for a cell that is drawn. */
static void draw_paletted32(Cpu *c, gaddr ds, gaddr pic, gaddr base, gaddr frame) {
    gaddr cell = base + *W(pic + PIC_CELLS);
    place_columns(ds, pic, 32, DS_W32, S16(ds + DS_X));
    place_rows(ds, pic, 32, S16(ds + DS_H32), S16(ds + DS_Y));

    gaddr v = frame + VERTICES;
    prepare_vertices(c, ds, v, 0);
    *W(ds + DS_ROWS_LEFT) = *B(pic + PIC_ROWS);
    do {
        SET16(ds + DS_CELL_X, S16(ds + DS_ROW_X));
        *W(ds + DS_COLS_LEFT) = *B(pic + PIC_COLS);
        do {
            uint8_t first = *B(cell);
            if (first == CELL_EMPTY) {
                cell++;
            } else {
                uint32_t word = (uint32_t)first << 8 | *B(cell + 1);
                *W(ds + DS_CELL_WORD) = word;
                SET16(ds + DS_CELL_X_TEST, S16(ds + DS_CELL_X));
                int16_t cx = S16(ds + DS_CELL_X_TEST);
                gaddr last = cell + 2;
                cell += 3;
                if (cx < S16(ds + DS_RIGHT) && left_limit(S16(ds + DS_W32)) < cx) {
                    CALL1(f_089E0EA4, (uint32_t)((int32_t)*W(ds + DS_CELL_WORD) >> 6));   /* the palette */
                    word = *W(ds + DS_CELL_WORD);
                    SET16(ds + DS_U, (word & 7) << 5);
                    SET16(ds + DS_V, (word << 2) & 0xE0);
                    uint8_t flags_byte = *B(last);
                    cell_quad(ds, v, cx, S16(ds + DS_ROW_Y), S16(ds + DS_W32), S16(ds + DS_H32), flags_byte,
                              S16(ds + DS_U), S16(ds + DS_V), 32);
                    uint32_t blend = (flags_byte & CELL_BLEND) != 0 || *W(ds + DS_ALPHA) != ALPHA_OPAQUE;
                    f_089E1438(c);
                    uint32_t flags = c->r[2];
                    CALL5(f_089E1950, v, 4, flags, blend, *B(ds + DS_EFFECT));
                }
            }
            SET16(ds + DS_CELL_X, S16(ds + DS_CELL_X) + S16(ds + DS_STEP_X));
        } while (--*W(ds + DS_COLS_LEFT) != 0);
        SET16(ds + DS_ROW_Y, S16(ds + DS_ROW_Y) + S16(ds + DS_STEP_Y));
    } while (--*W(ds + DS_ROWS_LEFT) != 0);
}

/* a0 the draw state, a1 the picture, a2 where its cells are counted from, a3 the first cached page
   (0x088117D0, 0x08811BCC: unused) */
SFA_FUNC(08810D84, game_draw_cells_cached16) {
    c->r[29] -= FRAME_SIZE; draw_cached(c, &SMALL, ARG(0), ARG(1), ARG(2), ARG(3), c->r[29]); c->r[29] += FRAME_SIZE;
}
SFA_FUNC(088112A8, game_draw_cells_cached32) {
    c->r[29] -= FRAME_SIZE; draw_cached(c, &BIG, ARG(0), ARG(1), ARG(2), ARG(3), c->r[29]); c->r[29] += FRAME_SIZE;
}
SFA_FUNC(088117D0, game_draw_cells_paletted16) {
    c->r[29] -= FRAME_SIZE; draw_paletted(c, ARG(0), ARG(1), ARG(2), c->r[29]); c->r[29] += FRAME_SIZE;
}
SFA_FUNC(08811BCC, game_draw_cells_paletted32) {
    c->r[29] -= FRAME_SIZE; draw_paletted32(c, ARG(0), ARG(1), ARG(2), c->r[29]); c->r[29] += FRAME_SIZE;
}
