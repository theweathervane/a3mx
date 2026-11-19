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
 * WIDE NATIVE: a cell is skipped when it starts left of minus its width, which in the 16:9 picture is
 * still inside the 48 pixels added on the left. That cut a fighter's back at the old 4:3 edge
 * (Blanka's shoulder against the wall). The 16:9 patch (Rickow, sfa3-PSP-widescreen, MIT) moved that
 * limit to -96 in 0x08811BCC only; here all four have it with rt_widescreen. The walls and everything
 * else stay as they are: only what is drawn changes (docs/WIDESCREEN.md).
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA) and round
 * differently from the PSP.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */

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
#define WIDE_LEFT  (-96)         /* WIDE NATIVE: the left limit, as the patch has it in 0x08811BCC */

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

/* The left limit: minus the cell's width, or WIDE_LEFT in WIDE NATIVE */
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

/* WIDE NATIVE: set by a sprite drawer (sprite_drawers.c) for a pass that draws a picture scaled by
   sprite_grid_scale about the point (sprite_grid_ax, sprite_grid_ay) on the screen, in the 16:9 margins
   only (each cell cut at the 4:3 picture's sides, its texels with it): the map screen's beam, a wedge
   from its point, carried on along its own edges past the end of its picture. Only the drawing changes. */
float sprite_grid_scale, sprite_grid_ax, sprite_grid_ay;
/* WIDE NATIVE: with sprite_grid_scale, the whole scaled cell is drawn instead of only its parts in the
   margins (the super finish's backdrops: the 4:3 picture over-sized to the 16:9 screen) */
int sprite_grid_scale_all;
static int32_t round_to(float f) { return (int32_t)(f < 0.0f ? f - 0.5f : f + 0.5f); }

/* the quad (corners top left, bottom left, top right, bottom right) drawn, or in the scaled pass its
   scaled copy's parts in the margins */
/* WIDE NATIVE: set by a sprite drawer for a picture drawn widened instead: the left half of its columns
   moved 48 left, the right half 48 right, and the 96 pixels between them filled with the middle cell's
   first texel column stretched (the profile screen's header and text box, whose middles are plain). A
   picture narrower than the 4:3 picture in the same drawing (the name plate) is moved whole with the
   half its middle is in.
   Only the drawing changes. */
int sprite_grid_half;
#define PICTURE_W 384

int sprite_grid_margins_only;   /* WIDE NATIVE: a pass that draws only what goes into the margins */
static void emit_cell(Cpu *c, gaddr ds, gaddr v, int32_t cols, uint32_t flags, uint32_t blend, uint32_t effect) {
    if (sprite_grid_margins_only) return;
    if (sprite_grid_half) {
        int32_t col = cols - (int32_t)*W(ds + DS_COLS_LEFT), split = cols / 2;
        int16_t keep[4][2];
        for (int i = 0; i < 4; i++) { keep[i][0] = S16(v + 16 * i); keep[i][1] = S16(v + 16 * i + 6); }
        int32_t w = keep[2][0] - keep[0][0];
        if (w < 0) w = -w;
        if (cols * w < PICTURE_W) {   /* a narrower picture (the name plate): moved whole with its side */
            int32_t mid = S16(ds + DS_ROW_X) + cols * w / 2;
            split = mid < PICTURE_W / 2 ? cols : 0;
        }
        if (col == split && split > 0 && split < cols) {   /* the gap: the cell's first texel column (as shown) from x - 48 to x + 48 */
            int32_t x0 = keep[0][0] < keep[2][0] ? keep[0][0] : keep[2][0];
            int32_t u = keep[0][0] <= keep[2][0] ? keep[0][1] : keep[2][1];
            u += keep[0][0] <= keep[2][0] ? (keep[2][1] > keep[0][1] ? 1 : -1) : (keep[0][1] > keep[2][1] ? 1 : -1);
            SET16(v + 0, x0 - 48); SET16(v + 16, x0 - 48); SET16(v + 32, x0 + 48); SET16(v + 48, x0 + 48);
            for (int i = 0; i < 4; i++) SET16(v + 16 * i + 6, u);
            CALL5(f_089E1950, v, 4, flags, blend, effect);
            for (int i = 0; i < 4; i++) SET16(v + 16 * i + 6, keep[i][1]);
        }
        int32_t shift = col < split ? -48 : 48;
        for (int i = 0; i < 4; i++) SET16(v + 16 * i, keep[i][0] + shift);
        CALL5(f_089E1950, v, 4, flags, blend, effect);
        for (int i = 0; i < 4; i++) SET16(v + 16 * i, keep[i][0]);
        return;
    }
    if (sprite_grid_scale == 0.0f) { CALL5(f_089E1950, v, 4, flags, blend, effect); return; }
    int16_t keep[4][3];
    for (int i = 0; i < 4; i++) {
        keep[i][0] = S16(v + 16 * i + 0); keep[i][1] = S16(v + 16 * i + 2); keep[i][2] = S16(v + 16 * i + 6);
        SET16(v + 16 * i + 0, round_to(sprite_grid_ax + ((float)keep[i][0] - sprite_grid_ax) * sprite_grid_scale));
        SET16(v + 16 * i + 2, round_to(sprite_grid_ay + ((float)keep[i][1] - sprite_grid_ay) * sprite_grid_scale));
    }
    if (sprite_grid_scale_all) {
        CALL5(f_089E1950, v, 4, flags, blend, effect);
        for (int i = 0; i < 4; i++) { SET16(v + 16 * i + 0, keep[i][0]); SET16(v + 16 * i + 2, keep[i][1]); }
        return;
    }
    int32_t x0 = S16(v + 0), x1 = S16(v + 32), u0 = keep[0][2], u1 = keep[2][2];
    for (int side = 0; side < 2 && x1 > x0; side++) {
        int32_t a = side ? (x0 > 384 ? x0 : 384) : x0, b = side ? x1 : (x1 < 0 ? x1 : 0);
        if (a >= b) continue;
        int32_t ua = u0 + (u1 - u0) * (a - x0) / (x1 - x0), ub = u0 + (u1 - u0) * (b - x0) / (x1 - x0);
        SET16(v + 0, a); SET16(v + 16, a); SET16(v + 32, b); SET16(v + 48, b);
        SET16(v + 6, ua); SET16(v + 22, ua); SET16(v + 38, ub); SET16(v + 54, ub);
        CALL5(f_089E1950, v, 4, flags, blend, effect);
    }
    for (int i = 0; i < 4; i++) { SET16(v + 16 * i + 0, keep[i][0]); SET16(v + 16 * i + 2, keep[i][1]); SET16(v + 16 * i + 6, keep[i][2]); }
}

/* WIDE NATIVE: set by a sprite drawer (sprite_drawers.c) around a picture that is a band across the 4:3
   picture (at least 384 wide, or for 3 starting at its left edge; others are left alone): 1 for
   FIGHT's translucent box and the GAME OVER band, 2 for the ROUND banner's band, 4 for the super finish's background (each row's
   end cells mirrored out, then as they are), 3 for the continue screen's bar (its right end
   breaks up, so only the left margin, from a texel column a quarter into the row's first cell drawn:
   the bar's first cells blink empty, and the margin stays steady). Each row's last cell carries
   on to the right margin (to x 432) as one strip of a single texel column from its far part (so a
   letter's edge in the cell is not repeated), and with 1 the same strip goes from the left margin
   (x -48) to the row's start. With 2 the left margin instead gets the dimmer step of the band's glow
   after the D (picture x 300 - 304, before the hole for the 1's bar, drawn 16 wide as that step is)
   next to the row's start, then the plain strip: the band's own left end before the R is the glow's
   brighter step, so the glow fades bright, dimmer, line to the left as it does after the D on the right.
   Only for a sprite drawn the right way round. Only the drawing changes. */
int sprite_grid_wide_edges;
int sprite_grid_wide_strip;   /* mode 4: 0 the side cells mirrored, then as they are; 1 the edge's colour stretched; 2 the side cells repeated along a slope */
int sprite_grid_wide_dy;      /* mode 4, 2: the lines' rise in screen pixels over a cell to the right */
int32_t sprite_grid_left_row;   /* mode 3: the row whose strip is drawn (the drawer resets it for each picture) */
#define PICTURE_W 384
#define MARGIN_W  48
#define TAPER_FROM 300           /* ROUND (mode 2): the glow's dimmer step after the D (the D ends at 284; */
#define TAPER_TO   304           /* past 304 the band has the hole for the 1's bar) */
#define TAPER_W    16            /* drawn this wide: the step is 16 wide after the D */

/* A quad from x0 to x1 on the screen, the cell's texels from u0 (at x0) to u1 (at x1) across */
static void edge_quad(Cpu *c, gaddr ds, gaddr v, int32_t x0, int32_t x1, int16_t y, int16_t h, uint8_t last,
                      int32_t texels, int32_t u0, int32_t u1, uint32_t flags, uint32_t blend, uint32_t effect) {
    if (x1 <= x0) return;
    cell_quad(ds, v, (int16_t)x0, y, (int16_t)(x1 - x0), h, last, S16(ds + DS_U), S16(ds + DS_V), texels);
    SET16(v + 16 * 0 + 6, u0); SET16(v + 16 * 1 + 6, u0); SET16(v + 16 * 2 + 6, u1); SET16(v + 16 * 3 + 6, u1);
    CALL5(f_089E1950, v, 4, flags, blend, effect);
}

/* the texel across the cell for a point `off` pixels in from its left as it is shown */
static int32_t texel_at(gaddr ds, uint8_t last, int32_t texels, int32_t w, int32_t off) {
    int flipped = (((uint32_t)last >> CELL_FLIP_X) & 1) ^ *W(ds + DS_FLIP_X);
    int32_t t = off * texels / w;
    return S16(ds + DS_U) + (flipped ? texels - t : t);
}

static void wide_edge_cells(Cpu *c, gaddr ds, gaddr v, int16_t cx, int16_t y, int16_t w, int16_t h,
                            uint8_t last, int32_t texels, uint32_t flags, uint32_t blend, uint32_t effect,
                            int32_t cols) {
    if (w <= 0 || S16(ds + DS_STEP_X) <= 0) return;
    int32_t col = cols - (int32_t)*W(ds + DS_COLS_LEFT), row_x = S16(ds + DS_ROW_X);
    int mode = sprite_grid_wide_edges;
    /* only a band as wide as the picture (not for mode 4); for mode 3 one that starts at its left edge
       (the continue bar's frames are 23 or 24 cells) */
    if (mode != 4 && (mode == 3 ? row_x > 0 : cols * w < PICTURE_W)) return;
    if (mode == 4) {   /* cells at the 4:3 picture's sides mirrored out, then as they are: a pattern
                          carried on seamlessly (the backdrop is two pictures side by side) */
        int32_t step = cx <= 0 && cx + w > 0 ? -w : cx < PICTURE_W && cx + w >= PICTURE_W ? w : 0;
        if (!step) return;
        if (sprite_grid_wide_strip == 1) {   /* a texel column at the edge stretched: the edge's colour on */
            int32_t u = texel_at(ds, last, texels, w, step < 0 ? 2 : w - 3);   /* two in: the very edge samples its neighbour */
            if (step < 0) edge_quad(c, ds, v, -MARGIN_W, cx, y, h, last, texels, u, u, flags, blend, effect);
            else edge_quad(c, ds, v, cx + w, PICTURE_W + MARGIN_W, y, h, last, texels, u, u, flags, blend, effect);
            return;
        }
        if (sprite_grid_wide_strip == 2) {
            /* diagonal lines (the backdrop's slanted speed lines): the side cell drawn again a cell out and
               two, moved up or down by sprite_grid_wide_dy a cell so the lines go straight on (the band
               that leaves open at the top or bottom is filled by a pass of 1 before, margins only) */
            int32_t sh = sprite_grid_wide_dy * step / w;
            for (int k = 1; k <= 2; k++) {
                cell_quad(ds, v, (int16_t)(cx + k * step), (int16_t)(y + k * sh), w, h, last, S16(ds + DS_U), S16(ds + DS_V), texels);
                CALL5(f_089E1950, v, 4, flags, blend, effect);
            }
            return;
        }
        uint8_t mirrored = (uint8_t)(last ^ (1u << CELL_FLIP_X));
        cell_quad(ds, v, (int16_t)(cx + step), y, w, h, mirrored, S16(ds + DS_U), S16(ds + DS_V), texels);
        CALL5(f_089E1950, v, 4, flags, blend, effect);
        cell_quad(ds, v, (int16_t)(cx + 2 * step), y, w, h, last, S16(ds + DS_U), S16(ds + DS_V), texels);
        CALL5(f_089E1950, v, 4, flags, blend, effect);
        return;
    }
    if (mode == 3) {   /* the left margin only, from the row's first cell drawn */
        if (y != sprite_grid_left_row) {
            sprite_grid_left_row = y;
            int32_t u = texel_at(ds, last, texels, w, w / 4);
            edge_quad(c, ds, v, -MARGIN_W, row_x < 0 ? row_x : 0, y, h, last, texels, u, u, flags, blend, effect);
        }
        return;
    }
    if (col == cols - 1) {
        int32_t u = texel_at(ds, last, texels, w, w * 3 / 4);   /* one texel column, three quarters in */
        edge_quad(c, ds, v, cx + w, PICTURE_W + MARGIN_W, y, h, last, texels, u, u, flags, blend, effect);
        int32_t end = mode == 2 ? row_x - TAPER_W : row_x;
        edge_quad(c, ds, v, -MARGIN_W, end, y, h, last, texels, u, u, flags, blend, effect);
    }
    if (mode == 2) {   /* the part of the glow in this cell, mirrored about the row's start */
        int32_t xc = cx - row_x, a = xc > TAPER_FROM ? xc : TAPER_FROM, b = xc + w < TAPER_TO ? xc + w : TAPER_TO;
        if (a < b)
            edge_quad(c, ds, v, row_x - (b - TAPER_FROM) * TAPER_W / (TAPER_TO - TAPER_FROM),
                      row_x - (a - TAPER_FROM) * TAPER_W / (TAPER_TO - TAPER_FROM), y, h, last, texels,
                      texel_at(ds, last, texels, w, b - xc), texel_at(ds, last, texels, w, a - xc), flags, blend, effect);
    }
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
                    emit_cell(c, ds, v, *B(pic + PIC_COLS), *W(page + 8), blend, effect);
                    if (sprite_grid_wide_edges)
                        wide_edge_cells(c, ds, v, cx, row_y, S16(ds + k->w_at), S16(ds + k->h_at), flags_byte,
                                        k->size, *W(page + 8), blend, effect, *B(pic + PIC_COLS));
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
                    emit_cell(c, ds, v, *B(pic + PIC_COLS), flags, blend, *B(ds + DS_EFFECT));
                    if (sprite_grid_wide_edges)
                        wide_edge_cells(c, ds, v, cx, row_y, S16(ds + DS_W16), S16(ds + DS_H16), flags_byte,
                                        16, flags, blend, *B(ds + DS_EFFECT), *B(pic + PIC_COLS));
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
                    emit_cell(c, ds, v, *B(pic + PIC_COLS), flags, blend, *B(ds + DS_EFFECT));
                    if (sprite_grid_wide_edges)
                        wide_edge_cells(c, ds, v, cx, S16(ds + DS_ROW_Y), S16(ds + DS_W32), S16(ds + DS_H32),
                                        flags_byte, 32, flags, blend, *B(ds + DS_EFFECT), *B(pic + PIC_COLS));
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
