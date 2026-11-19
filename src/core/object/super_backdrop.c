/* The backdrop of a super combo's finish (0x0880C208, the text pass in frame.c): while the fight's
 * BATTLE + 36 is set, and its kind (BATTLE + 43) is not 0, 11 or 12, a picture of 32-pixel cells over the
 * whole 4:3 screen (the green speed lines of a KO by a super combo), drawn through 0x08811BCC.
 *
 * The picture: number 0x08A04B2C[kind] (u16) in the picture list 0x08E75AD8 + 68, its texture page
 * 0x08E75A18 + 68; it and the pictures it goes on into (+10 nonzero) are drawn at x 0, y 0xE0, depth
 * index 2, colour white, not flipped.
 *
 * WIDE NATIVE: the vertical lines (3, 4) and the speed lines (5) carry each row's end cells on into the
 * margins, mirrored, then as they are (sprite_grid.c, mode 4); every other kind (the slanted lines 1, 2,
 * the splash 6, the burst 0, 7, 9, 10, the swirl 8) is drawn whole, 1.25 times over-sized about the
 * screen's middle, so its pattern fills the 16:9 screen unbroken. Only the drawing changes.
 */
#include "../decomp.h"

extern int rt_widescreen;          /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */
extern int sprite_grid_wide_edges, sprite_grid_wide_strip, sprite_grid_scale_all;   /* sprite_grid.c */
extern float sprite_grid_scale, sprite_grid_ax, sprite_grid_ay;

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_089E0CF4(Cpu *c);   /* selects a texture page (a0 its descriptor) */
void f_08811BCC(Cpu *c);   /* a picture's 32-pixel cells (a0 draw state, a1 picture, a2 its list) */

#define BATTLE      0x092E1228u
#define DRAW_STATE  0x08AD9320u
#define PICTURES    0x08E75AD8u   /* +68: the list */
#define TEXTURES    0x08E75A18u   /* +68: its page */
#define BY_KIND     0x08A04B2Cu   /* u16[]: the picture by kind */
enum { DS_X = 0, DS_Y = 2, DS_DEPTH = 44, DS_COLOUR = 48, DS_FLIP_X = 56, DS_FLIP_Y = 60, DS_BLEND = 85 };

/* the picture and the ones it goes on into */
static void draw_pictures(Cpu *c, gaddr ds, gaddr pic, gaddr list) {
    for (;;) {
        int more = (int16_t)*H(pic + 10) != 0;
        c->r[4] = ds; c->r[5] = pic; c->r[6] = list;
        f_08811BCC(c);
        if (!more) break;
        pic += 12;
    }
}

static void draw_backdrop(Cpu *c) {
    if ((int8_t)*B(BATTLE + 36) == 0) return;
    int32_t kind = (int8_t)*B(BATTLE + 43);
    if (kind == 11 || kind == 12) return;
    gaddr ds = DRAW_STATE;
    *H(ds + DS_X) = 0;
    *H(ds + DS_Y) = 0xE0;
    gaddr list = *W(PICTURES + 68);
    gaddr pic = list + (uint32_t)*H(BY_KIND + (uint32_t)(kind * 2)) * 12u;
    CALL1(f_089E0CF4, *W(TEXTURES + 68));
    *W(ds + DS_FLIP_Y) = 0;
    *W(ds + DS_FLIP_X) = 0;
    *W(ds + DS_COLOUR) = 0xFFFFFFFFu;
    *B(ds + DS_BLEND) = 0;
    *W(ds + DS_DEPTH) = 2;
    if (rt_widescreen && kind != 3 && kind != 4 && kind != 5) {
        /* the slanted lines (1, 2), the splash (6), the burst (0, 7, 9, 10) and the swirl (8): the whole
           picture over-sized 1.25 times about the 4:3 screen's middle (192, 112), so its 384 fill the 480
           and its lines and rings go on unbroken; 28 rows are lost above and below. 32-pixel cells
           become 40, on whole pixels (x' = 1.25 x - 48, y' = 1.25 y - 28) */
        sprite_grid_scale = 1.25f; sprite_grid_ax = 192.0f; sprite_grid_ay = 112.0f; sprite_grid_scale_all = 1;
        draw_pictures(c, ds, pic, list);
        sprite_grid_scale = 0.0f; sprite_grid_scale_all = 0;
        return;
    }
    /* the vertical lines (3, 4: the same picture, 4 the other way) and the speed lines (5): each row's
       end cells carried on into the margins, mirrored, then as they are (sprite_grid.c, mode 4) */
    sprite_grid_wide_edges = rt_widescreen ? 4 : 0;
    sprite_grid_wide_strip = 0;
    draw_pictures(c, ds, pic, list);
    sprite_grid_wide_edges = sprite_grid_wide_strip = 0;
}

SFA_FUNC(0880C208, super_finish_backdrop) { c->r[29] -= 0x20; draw_backdrop(c); c->r[29] += 0x20; }
