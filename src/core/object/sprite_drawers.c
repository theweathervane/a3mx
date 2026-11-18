/* Three of the sprite drawers (0x08A04B48[sprite +148], called by draw_sprite in sprite.c with the
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
 *   drawer 11 (0x0880CF60): the same with the set from the sprite's +156, every piece at the sprite's
 *     place; a piece with a nonzero +0 is drawn at half alpha (the after-images of ROUND and FIGHT)
 *
 * All three copy the sprite's +170 / +171 to the draw state's +86 / +87 first.
 *
 * WIDE FORCED: for the sprites widescreen_edge_cells picks (extensions/widescreen.c: the ROUND banner),
 * the band's end cells repeat out into the margins (sprite_grid.c), its after-images (the half alpha
 * pieces) excepted; for those widescreen_edge_row picks (the FIGHT FIGHT ... rows), a row of pieces
 * gets one more piece at an end that has reached the 4:3 edge.
 */
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }

extern int rt_widescreen;          /* WIDE FORCED: extensions/widescreen.c (extensions_off.c: 0) */
extern int sprite_grid_wide_edges; /* sprite_grid.c */
int widescreen_edge_cells(gaddr sp);
int widescreen_edge_row(gaddr sp);
void f_0880E150(Cpu *c);   /* a sprite's picture drawn as a grid of cells (a0 draw state, a1 sprite, a2 picture, a3 list, t0 flag) */

#define PICTURE_LISTS 0x08E75AD8u   /* u32[]: picture lists by the sprite's +86 */
#define PIECE_TABLES  0x092E3E98u   /* u32[]: drawers 1 and 2's piece tables */
enum { SP_FRAME = 28, SP_LIST = 86, SP_DRAWER = 148, SP_PIECES = 156, SP_EFFECT = 170, SP_SCALED = 171 };
enum { DS_X = 0, DS_Y = 2, DS_COLOUR = 48, DS_POS_X = 80, DS_POS_Y = 82, DS_BLEND = 85, DS_EFFECT = 86,
       DS_SCALED = 87 };
enum { PIECE_LAST = 0x8000, PIECE_SKIP = 0x4000 };

static uint32_t sprite_frame(gaddr sp) { return *H(*W(sp + SP_FRAME) + 4); }
static gaddr sprite_list(gaddr sp) { return *W(PICTURE_LISTS + (uint32_t)*H(sp + SP_LIST) * 4u); }

/* a picture and the ones it goes on into */
static void draw_picture(Cpu *c, gaddr ds, gaddr sp, gaddr pic, gaddr list, int edges) {
    sprite_grid_wide_edges = edges && rt_widescreen && widescreen_edge_cells(sp);
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

static void draw_one(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    copy_effects(ds, sp);
    draw_picture(c, ds, sp, list + frame * 12u, list, 1);
}

/* WIDE FORCED: a row of the same piece at even steps (the FIGHT FIGHT ... rows) gets one more piece at
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

static void draw_pieces(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    gaddr first = *W(PIECE_TABLES + *W(sp + SP_DRAWER) * 4u - 4u) + frame * 6u;
    gaddr e = last_piece(first), last = e;
    copy_effects(ds, sp);
    int row = rt_widescreen && widescreen_edge_row(sp);
    if (row) wide_row_end(c, ds, sp, list, first, last, 1);
    for (; e >= first; e -= 6) {
        uint32_t p = *H(e + 4);
        if (p & PIECE_SKIP) continue;
        *H(ds + DS_X) = (uint16_t)(S16(ds + DS_POS_X) + S16(e));
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(e + 2));
        draw_picture(c, ds, sp, list + (p & 0x7FFF) * 12u, list, 1);
    }
    if (row) wide_row_end(c, ds, sp, list, first, last, 0);
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
        draw_picture(c, ds, sp, list + (p & 0x7FFF) * 12u, list, !ghost);
    }
}

SFA_FUNC(0880C4C8, sprite_drawer_one) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x40; draw_one(c, ds, sp); c->r[29] += 0x40; }
SFA_FUNC(0880C598, sprite_drawer_pieces) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x40; draw_pieces(c, ds, sp); c->r[29] += 0x40; }
SFA_FUNC(0880CF60, sprite_drawer_ghosted) { gaddr ds = ARG(0), sp = ARG(1); c->r[29] -= 0x50; draw_ghosted(c, ds, sp); c->r[29] += 0x50; }
