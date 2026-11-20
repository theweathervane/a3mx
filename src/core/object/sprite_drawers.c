/* Five of the sprite drawers (0x08A04B48[sprite +148], called by draw_sprite in sprite.c with the
 * draw state set up): a sprite's picture, or a set of pictures, through 0x0880E150 (sprite_grid.c).
 *
 * A sprite's frame record is the u16 at (+28) + 4: bit 15 means nothing to draw, the rest its number.
 * Pictures come from the list 0x08E75AD8[+86] (12 bytes each); a picture with a nonzero +10 goes on
 * into the next one, so each is drawn until the one with +10 = 0, that one included.
 *
 *   drawer 0 (0x0880C4C8): picture number = the frame
 *   drawers 1 and 2 (0x0880C598): a set of pieces from the table 0x092E3E98[drawer - 1], the frame's
 *     entry onwards: 6 bytes each, +0 x, +2 y (added to the sprite's place), +4 the picture (bit 15:
 *     the last piece, bit 14: not drawn); drawn last first
 *   drawer 3 (0x0880C70C): as drawer 0, the picture number at the sprite's +84 (bit 15: none)
 *   drawer 6 (0x0880CAA8): as drawers 1 and 2, the pieces from the sprite's +156
 *   drawer 11 (0x0880CF60): the same with the set from the sprite's +156, every piece at the sprite's
 *     place; a piece with a nonzero +0 is drawn at half alpha (the after-images of ROUND and FIGHT)
 *
 * All of them copy the sprite's +170 / +171 to the draw state's +86 / +87 first.
 *
 * WIDE NATIVE (extensions/widescreen.c says for which sprites): bands whose cells carry on into the
 * margins (widescreen_edge_cells: the ROUND band, FIGHT's translucent box, the continue bar, the GAME
 * OVER band; sprite_grid.c); rows of one piece that get one more piece at an end that has reached the
 * 4:3 edge (widescreen_edge_row 1: FIGHT FIGHT, KNOCK OUT); the continue count's numbers carried on to
 * the left (widescreen_edge_row 2); the map screen's beam drawn again scaled behind itself, in the
 * margins (widescreen_beam); the profile screen's header and text box drawn widened, their halves moved
 * out (widescreen_widen).
 */
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

extern int rt_widescreen;          /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */
extern int sprite_grid_wide_edges; /* sprite_grid.c */
extern int32_t sprite_grid_left_row;
int widescreen_edge_cells(gaddr sp);   /* which pieces: */
enum { EDGES_SOLID = 1, EDGES_AFTER_IMAGE = 2, EDGES_MIRROR_LEFT = 4, EDGES_LEFT_ONLY = 8, EDGES_MIRROR = 16 };   /* and how */
int widescreen_edge_row(gaddr sp);
int widescreen_beam(gaddr sp);
int widescreen_widen(gaddr sp);
extern int sprite_grid_half;   /* sprite_grid.c */
extern float sprite_grid_scale, sprite_grid_ax, sprite_grid_ay;   /* sprite_grid.c */
extern int sprite_grid_scale_all;                                  /* sprite_grid.c */
int widescreen_oversize(gaddr sp);                                 /* extensions/widescreen.c */
int widescreen_column_copies(gaddr sp, int dx[2]);                 /* extensions/widescreen.c */
int widescreen_bar_ends(gaddr sp);                                 /* extensions/widescreen.c */
int widescreen_piece_shift(gaddr sp, int dx);                      /* extensions/widescreen.c */
extern int sprite_grid_bar_ends;                                   /* sprite_grid.c */
void f_0880E150(Cpu *c);   /* a sprite's picture drawn as a grid of cells (a0 draw state, a1 sprite, a2 picture, a3 list, t0 flag) */

#define PICTURE_LISTS 0x08E75AD8u   /* u32[]: picture lists by the sprite's +86 */
#define PIECE_TABLES  0x092E3E98u   /* u32[]: drawers 1 and 2's piece tables */
#define SCALE_X       0x08E759E8u   /* floats: screen = position * scale + offset */
#define SCALE_Y       0x08E759ECu
#define OFFSET_X      0x08E75A04u
#define OFFSET_Y      0x08E75A0Cu
enum { SP_FRAME = 28, SP_FRAME_84 = 84, SP_LIST = 86, SP_DRAWER = 148, SP_PIECES = 156, SP_EFFECT = 170, SP_SCALED = 171 };
enum { DS_X = 0, DS_Y = 2, DS_COLOUR = 48, DS_POS_X = 80, DS_POS_Y = 82, DS_BLEND = 85, DS_EFFECT = 86,
       DS_SCALED = 87 };
enum { PIECE_LAST = 0x8000, PIECE_SKIP = 0x4000 };

static uint32_t sprite_frame(gaddr sp) { return *H(*W(sp + SP_FRAME) + 4); }
static gaddr sprite_list(gaddr sp) { return *W(PICTURE_LISTS + (uint32_t)*H(sp + SP_LIST) * 4u); }

/* a picture and the ones it goes on into */
static void draw_picture(Cpu *c, gaddr ds, gaddr sp, gaddr pic, gaddr list, int edges) {
    int wide = rt_widescreen ? widescreen_edge_cells(sp) : 0;
    sprite_grid_left_row = INT32_MIN;
    sprite_grid_wide_edges = !(wide & edges) ? 0 : (wide & EDGES_MIRROR) ? 4 : (wide & EDGES_MIRROR_LEFT) ? 2
                           : (wide & EDGES_LEFT_ONLY) ? 3 : 1;   /* 4 with sprite_grid_wide_strip 0: the side cells mirrored out */
    for (;;) {
        int more = S16(pic + 10) != 0;
        c->r[4] = ds; c->r[5] = sp; c->r[6] = pic; c->r[7] = list; c->r[8] = 0;
        f_0880E150(c);
        if (!more) break;
        pic += 12;
    }
    sprite_grid_wide_edges = 0;
}

static void copy_effects(gaddr ds, gaddr sp) {
    *B(ds + DS_EFFECT) = *B(sp + SP_EFFECT);
    *B(ds + DS_SCALED) = *B(sp + SP_SCALED);
}

/* the last entry of a piece set */
static gaddr last_piece(gaddr e) {
    while (!(*H(e + 4) & PIECE_LAST)) e += 6;
    return e;
}

/* WIDE NATIVE: the map screen's beam (widescreen_beam) is a wedge from its picture's place (the map's
   point), cut where its picture ends: 16 or 32 pixels into the margin. Its far end is noise (the part near
   the point has the map's wings in it, lit); the margin past the picture's end gets that noise carried on
   as it is, not scaled: cells of the picture's far-end column (the inner ones, not the two rows nearest
   the wing side's straight edge) drawn again one column further out and one row further on the side it
   widens to, each cut to the wedge (sprite_grid.c). The wedges' slanted edge drops 0.513 screen pixels a
   pixel (0.4225 of the 2D library's rows, 224 to the 272-pixel screen): pictures 44 and 50 (right, left)
   lie between it and the level of the point, 46 and 52 (down-right, up-left) beyond it. */
struct SpriteGridWedge { int on, dir, vside, sector; float ax, ay, k; };
extern struct SpriteGridWedge sprite_grid_wedge;   /* sprite_grid.c */
#define BEAM_SLOPE 0.4225f
enum { PIC_CELLS = 0, PIC_COLS = 4, PIC_ROWS = 5, PIC_ORIGIN_X = 6, PIC_ORIGIN_Y = 8, PIC_MORE = 10, DS_FLIP_X_ = 56, DS_FLIP_Y_ = 60 };
static void wide_beam(Cpu *c, gaddr ds, gaddr sp, gaddr pic, gaddr list) {
    int32_t cols = *B(pic + PIC_COLS), rows = *B(pic + PIC_ROWS), ox = S16(pic + PIC_ORIGIN_X), oy = S16(pic + PIC_ORIGIN_Y);
    if (cols < 2 || rows < 3 || cols > 32 || rows > 32) return;
    int left = ox == cols * 16, up = oy == rows * 16;          /* the picture's far end, as drawn unflipped */
    if (!left && ox != 0) return;
    int fx = *W(ds + DS_FLIP_X_) != 0, fy = *W(ds + DS_FLIP_Y_) != 0;
    int dir = (left ? -1 : 1) * (fx ? -1 : 1), vside = (up ? -1 : 1) * (fy ? -1 : 1);
    int32_t x = S16(ds + DS_X), y = S16(ds + DS_Y);
    int32_t end = x + dir * cols * 16, room = dir < 0 ? end - (-48) : 432 - end;
    if (room <= 0 || (dir < 0 ? end >= 0 : end <= 384)) return;   /* only a picture already past the 4:3 side */
    int32_t extra = (room + 15) / 16;
    /* the far-end column's cells (unflipped column 0 for a picture to the left, the last otherwise) */
    gaddr cell = list + *W(pic + PIC_CELLS), far[32]; int nfar = 0;
    int sector = cols >= 2 * rows ? 0 : 1;                      /* 44 / 50 (14 x 6): at least twice as wide as tall */
    for (int32_t r = 0; r < rows; r++)
        for (int32_t q = 0; q < cols; q++) {
            if (*B(cell) == 0xFF) { cell += 1; continue; }
            int at_end = left ? q == 0 : q == cols - 1;
            int near_wing = sector == 0 && (up ? r >= rows - 2 : r < 2);
            if (at_end && !near_wing && !(*B(cell + 2) & 0x20) && nfar < 32) far[nfar++] = cell;
            cell += 3;
        }
    if (nfar == 0) return;
    /* the cells past the end: `extra` columns, rows + 1 (one more on the side it widens to), on the stack */
    int32_t ncols = extra, nrows = rows + 1;
    uint32_t need = 12u + (uint32_t)(ncols * nrows * 3) + 3u;
    c->r[29] -= (need + 15u) & ~15u;
    gaddr mine = c->r[29], cells = mine + 12u;
    for (int32_t r = 0; r < nrows; r++)
        for (int32_t q = 0; q < ncols; q++) {
            gaddr from = far[(r + q) % nfar], to = cells + (uint32_t)((r * ncols + q) * 3);
            *B(to) = *B(from); *B(to + 1) = *B(from + 1); *B(to + 2) = *B(from + 2);
        }
    *W(mine) = cells - list;
    *B(mine + PIC_COLS) = (uint8_t)ncols; *B(mine + PIC_ROWS) = (uint8_t)nrows;
    *H(mine + PIC_ORIGIN_X) = (uint16_t)(left ? ox + ncols * 16 : ox - cols * 16);
    *H(mine + PIC_ORIGIN_Y) = (uint16_t)(up ? oy + 16 : oy);
    *H(mine + PIC_MORE) = 0;
    float sx = F(SCALE_X), sy = F(SCALE_Y);
    float ax = (float)x, ay = (float)y;
    ax = ax * sx; ax = ax + F(OFFSET_X);
    ay = ay * sy; ay = ay + F(OFFSET_Y);
    sprite_grid_wedge = (struct SpriteGridWedge){ 1, dir, vside, sector, ax, ay, BEAM_SLOPE * sy / sx };
    draw_picture(c, ds, sp, mine, list, 0);
    sprite_grid_wedge.on = 0;
    *H(ds + DS_X) = (uint16_t)x; *H(ds + DS_Y) = (uint16_t)y;
    c->r[29] += (need + 15u) & ~15u;
}

static void draw_one(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    copy_effects(ds, sp);
    if (rt_widescreen && widescreen_beam(sp)) wide_beam(c, ds, sp, list + frame * 12u, list);
    if (rt_widescreen && widescreen_oversize(sp)) {
        /* WIDE NATIVE: a backdrop across the whole 4:3 picture drawn whole, 1.25 times over-sized about
           the 4:3 screen's middle (192, 112), as the super finish's swirl and burst are
           (super_backdrop.c): its 384 fill the 480 and its rings and rays go on unbroken */
        sprite_grid_scale = 1.25f; sprite_grid_ax = 192.0f; sprite_grid_ay = 112.0f; sprite_grid_scale_all = 1;
        draw_picture(c, ds, sp, list + frame * 12u, list, 0);
        sprite_grid_scale = 0.0f; sprite_grid_scale_all = 0;
        return;
    }
    draw_picture(c, ds, sp, list + frame * 12u, list, EDGES_SOLID);
}

/* WIDE NATIVE: a row of the same piece at even steps (the FIGHT FIGHT ... rows) gets one more piece at
   an end that has reached the 4:3 picture's edge, in the margin beyond it: the row then reaches the
   16:9 sides, and while it slides in nothing is added ahead of it */
static void wide_row_end(Cpu *c, gaddr ds, gaddr sp, gaddr list, gaddr first, gaddr last, int right) {
    if (last <= first || ((*H(first + 4) ^ *H(last + 4)) & 0x7FFF) || ((*H(first + 4) | *H(last + 4)) & PIECE_SKIP)) return;
    int32_t step = (S16(last) - S16(first)) / (int32_t)((last - first) / 6);
    if (step <= 0) return;
    int32_t x = S16(ds + DS_POS_X) + (right ? S16(last) + step : S16(first) - step);
    if (right ? x < 384 : x + step > 0) return;
    *H(ds + DS_X) = (uint16_t)x;
    *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(first + 2));
    draw_picture(c, ds, sp, list + (*H(first + 4) & 0x7FFF) * 12u, list, 0);
}

/* WIDE NATIVE: a row of pieces at even steps, each its own picture (the attract mode's coloured panels
   behind the name rows, widescreen_edge_row 3): carried on round, the last piece's picture again before
   the first and the first's after the last, where they reach into a margin */
static void wide_row_wrap(Cpu *c, gaddr ds, gaddr sp, gaddr list, gaddr first, gaddr last) {
    if (last <= first) return;
    int32_t step = (S16(last) - S16(first)) / (int32_t)((last - first) / 6);
    if (step <= 0) return;
    int32_t x0 = S16(ds + DS_POS_X) + S16(first), x1 = S16(ds + DS_POS_X) + S16(last) + step;
    for (int side = 0; side < 2; side++) {
        gaddr from = side ? first : last;
        int32_t x = side ? x1 : x0 - step;
        if (side ? x >= 432 : x + step <= -48) continue;
        if (*H(from + 4) & PIECE_SKIP) continue;
        *H(ds + DS_X) = (uint16_t)x;
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(from + 2));
        draw_picture(c, ds, sp, list + (*H(from + 4) & 0x7FFF) * 12u, list, 0);
    }
}

/* WIDE NATIVE: the continue screen's count (widescreen_edge_row 2): a row of two-digit numbers, one every
   16 pixels, each two pieces at the same place (the second digit, then the first with its point: pictures
   582 + d and 572 + d), counting up to the right. The numbers before the first are drawn on to the left
   until the left margin is covered (down to 0.0). */
#define COUNT_FIRST 572
#define COUNT_SECOND 582
static void wide_count_left(Cpu *c, gaddr ds, gaddr sp, gaddr list, gaddr first, gaddr last) {
    if (last < first + 18) return;
    int32_t hi = (int32_t)(*H(first + 4) & 0x3FFF) - COUNT_FIRST, lo = (int32_t)(*H(first + 6 + 4) & 0x3FFF) - COUNT_SECOND;
    int32_t step = S16(first + 12) - S16(first);
    if (hi < 0 || hi > 9 || lo < 0 || lo > 9 || S16(first + 6) != S16(first) || step <= 0) return;
    int32_t value = hi * 10 + lo;
    for (int32_t k = 1; value - k >= 0; k++) {
        int32_t x = S16(ds + DS_POS_X) + S16(first) - k * step;
        if (x + step <= -48) break;
        *H(ds + DS_X) = (uint16_t)x;
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(first + 2));
        draw_picture(c, ds, sp, list + (uint32_t)(COUNT_SECOND + (value - k) % 10) * 12u, list, 0);
        *H(ds + DS_X) = (uint16_t)x;
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(first + 2));
        draw_picture(c, ds, sp, list + (uint32_t)(COUNT_FIRST + (value - k) / 10) * 12u, list, 0);
    }
}

/* drawer 3: like drawer 0, the picture number from the sprite's +84; the effects copied first */
static void draw_one_84(Cpu *c, gaddr ds, gaddr sp) {
    copy_effects(ds, sp);
    uint32_t frame = *H(sp + SP_FRAME_84);
    if (frame & 0x8000) return;
    gaddr list = sprite_list(sp);
    sprite_grid_half = rt_widescreen && widescreen_widen(sp);   /* WIDE NATIVE: widened (sprite_grid.c) */
    sprite_grid_bar_ends = rt_widescreen ? widescreen_bar_ends(sp) : 0;   /* WIDE NATIVE: a longer bar (sprite_grid.c) */
    draw_picture(c, ds, sp, list + frame * 12u, list, EDGES_SOLID);
    sprite_grid_half = 0;
    sprite_grid_bar_ends = 0;
}

/* WIDE NATIVE: a column of pieces (the ALPHA tiles before the title, widescreen_column_copies) drawn
   again moved by dx, unless that is well off the screen */
static void wide_column_copy(Cpu *c, gaddr ds, gaddr sp, gaddr list, gaddr first, gaddr last, int32_t dx) {
    int32_t x = S16(ds + DS_POS_X) + S16(first) + dx;
    if (x < -160 || x >= 520) return;                     /* well off the screen (this screen's x is not the 4:3 picture's) */
    for (gaddr e = last; e >= first; e -= 6) {
        uint32_t p = *H(e + 4);
        if (p & PIECE_SKIP) continue;
        *H(ds + DS_X) = (uint16_t)(S16(ds + DS_POS_X) + S16(e) + dx);
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(e + 2));
        draw_picture(c, ds, sp, list + (p & 0x7FFF) * 12u, list, 0);
    }
}

static void draw_pieces_from(Cpu *c, gaddr ds, gaddr sp, gaddr table);
static void draw_pieces(Cpu *c, gaddr ds, gaddr sp) {
    draw_pieces_from(c, ds, sp, *W(PIECE_TABLES + *W(sp + SP_DRAWER) * 4u - 4u));
}
static void draw_pieces_from(Cpu *c, gaddr ds, gaddr sp, gaddr table) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    gaddr first = table + frame * 6u;
    gaddr e = last_piece(first), last = e;
    copy_effects(ds, sp);
    int row = rt_widescreen ? widescreen_edge_row(sp) : 0;
    if (row == 1) wide_row_end(c, ds, sp, list, first, last, 1);
    for (; e >= first; e -= 6) {
        uint32_t p = *H(e + 4);
        if (p & PIECE_SKIP) continue;
        *H(ds + DS_X) = (uint16_t)(S16(ds + DS_POS_X) + S16(e) + (rt_widescreen ? widescreen_piece_shift(sp, S16(e)) : 0));
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(e + 2));
        draw_picture(c, ds, sp, list + (p & 0x7FFF) * 12u, list, EDGES_SOLID);
    }
    if (row == 1) wide_row_end(c, ds, sp, list, first, last, 0);
    if (row == 2) wide_count_left(c, ds, sp, list, first, last);
    if (row == 3) wide_row_wrap(c, ds, sp, list, first, last);
    int dx[2], copies = rt_widescreen ? widescreen_column_copies(sp, dx) : 0;
    for (int k = 0; k < copies; k++) wide_column_copy(c, ds, sp, list, first, last, dx[k]);
}

static void draw_ghosted(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    gaddr first = *W(sp + SP_PIECES) + frame * 6u;
    gaddr e = last_piece(first);
    uint32_t colour = *W(ds + DS_COLOUR);
    uint32_t half = (colour & 0x00FFFFFFu) | 0x80000000u;
    *B(ds + DS_BLEND) = 0;
    copy_effects(ds, sp);
    for (; e >= first; e -= 6) {
        uint32_t p = *H(e + 4);
        if (p & PIECE_SKIP) continue;
        *H(ds + DS_X) = *H(ds + DS_POS_X);
        *H(ds + DS_Y) = *H(ds + DS_POS_Y);
        int ghost = S16(e) != 0;
        *W(ds + DS_COLOUR) = ghost ? half : colour;
        draw_picture(c, ds, sp, list + (p & 0x7FFF) * 12u, list, ghost ? EDGES_AFTER_IMAGE : EDGES_SOLID);
    }
}

SFA_FUNC(0880C4C8, sprite_drawer_one) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x40; draw_one(c, ds, sp); c->r[29] += 0x40; }
SFA_FUNC(0880C598, sprite_drawer_pieces) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x40; draw_pieces(c, ds, sp); c->r[29] += 0x40; }
SFA_FUNC(0880C70C, sprite_drawer_one_84) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x40; draw_one_84(c, ds, sp); c->r[29] += 0x40; }
SFA_FUNC(0880CAA8, sprite_drawer_pieces_156) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x40; draw_pieces_from(c, ds, sp, *W(sp + SP_PIECES)); c->r[29] += 0x40; }
SFA_FUNC(0880CF60, sprite_drawer_ghosted) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x50; draw_ghosted(c, ds, sp); c->r[29] += 0x50; }
