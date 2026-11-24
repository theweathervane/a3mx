/* Drawing one queued sprite (0x0880C308): a small tree of sprites, each set up in the shared draw state
 * and handed to a drawer chosen by its kind.
 */
#include "../decomp.h"
#include <string.h>

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_089E0C4C(Cpu *c);   /* switches a GE drawing mode on (1) or off (0) around a sprite that asks for it */

#include "../guest_mem.h"

#define DRAW_STATE     0x08AD9320u   /* where a sprite is set up for its drawer */
#define DRAWERS        0x08A04B48u   /* u32[]: drawer by sprite kind (+148) */
#define ANCHORS        0x08A04014u   /* u32[]: anchor records by anchor kind (+14); +18 x, +22 y */
#define DRAW_OFF       0x08E7602Cu   /* u32: nonzero = no sprites drawn */
#define MODE_SWITCH_OFF 0x08E759F8u  /* u16: nonzero = leave the GE mode alone for flagged sprites */
#define SCROLL         0x092E5F78u   /* +28: a scroll value (16.16) for anchor kind 2 */
#define BATTLE         0x092E1228u   /* +74: the screen shake */

/* A sprite (offsets used here) */
enum {
    SP_ACTIVE = 0, SP_SHOWN = 1, SP_GROUP = 8, SP_FLIP = 11, SP_ANCHOR = 14, SP_SHAKES = 15,
    SP_X = 18, SP_Y = 22, SP_ATTR = 24, SP_SCROLL_X = 38, SP_NEXT = 40, SP_AFTER = 44,
    SP_BEFORE_2 = 48, SP_BEFORE_1 = 52, SP_STATE = 86, SP_DRAWER = 148, SP_DONE = 164,
    SP_COLOUR = 160, SP_BLEND = 168, SP_GE_MODE = 169,
};
#define GROUP_ALONE 3   /* a sprite that does not draw the sprites linked to it */

/* The draw state (offsets used here) */
enum { DS_X = 0, DS_Y = 2, DS_ATTR = 44, DS_COLOUR = 48, DS_FLIP_X = 56, DS_FLIP_Y = 60,
       DS_POS_X = 80, DS_POS_Y = 82, DS_BLEND = 85 };


/* Moves the sprite's position by its anchor: a scrolling anchor (kind 2) first adds its share of the
   scroll; then the anchor's own offset; then the screen shake if the sprite shakes. */
static void sprite_anchor(gaddr ds, gaddr sp) {
    uint8_t anchor = *B(sp + SP_ANCHOR);
    if (anchor != 0) {
        if (anchor == 2) {
            int16_t share = S16(sp + SP_SCROLL_X);
            if (share != 0) {
                int32_t scrolled = (int32_t)((uint32_t)*W(SCROLL + 28) * (uint32_t)(int32_t)share) >> 16;
                *H(ds + DS_POS_X) = (uint16_t)(S16(ds + DS_POS_X) + scrolled);
                anchor = *B(sp + SP_ANCHOR);
            }
        }
        gaddr rec = *W(ANCHORS + anchor * 4u);
        *H(ds + DS_POS_X) = (uint16_t)(S16(ds + DS_POS_X) - S16(rec + 18));
        *H(ds + DS_POS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(rec + 22));
    }
    if (*B(sp + SP_SHAKES) != 0)
        *H(ds + DS_POS_Y) = (uint16_t)(S16(ds + DS_POS_Y) - S16(BATTLE + 74));
}

static void switch_ge_mode(Cpu *c, gaddr sp, uint32_t on) {
    if ((int8_t)*B(sp + SP_GE_MODE) != 0 && *H(MODE_SWITCH_OFF) == 0) CALL1(f_089E0C4C, on);
}

void f_0880C308(Cpu *c);   /* this function: the linked sprites go through it, so --reference-only applies */
void f_0880E6E8(Cpu *c);   /* sprite_anchor */

/* WIDE NATIVE: some screens' side pieces move out to the 16:9 screen's edges, with the sprites linked
   to them (only where they are drawn: game memory keeps the 4:3 places). widescreen_edge_shift (in
   extensions/widescreen.c) says by how much for a sprite, widescreen_edge_copy whether it is drawn again
   moved (up to twice); a linked sprite takes its parent's, plus widescreen_linked_shift's. */
extern int rt_widescreen;
int widescreen_edge_shift(gaddr sp);
int widescreen_edge_copy(gaddr sp, int n);   /* the nth extra drawing, moved by this much (0: none) */
int widescreen_linked_shift(gaddr sp);       /* a linked sprite's own move, added to its parent's */
static int edge_shift, edge_depth;

/* WIDE NATIVE, an ending's frame (extensions/widescreen.c, scaled_endings.c): with the scaled endings its
   sprites are drawn across the whole 16:9 screen (a character walking in or out is seen whole, as in the
   fights), a picture of a scaled scene drawn 1.25 times larger (sprite_drawers.c) and a profile picture
   moved out to the 16:9 edge. A sprite not yet seen in the scene (it hasn't moved on the screen, walking
   or with the camera, and its cells haven't reached INTO pixels into the picture: someone waiting to walk
   in) is still cut at the 4:3 sides, so it isn't seen standing in the margin. With the scaled endings off every sprite is cut at the 4:3 picture's sides and nothing moves. */
int widescreen_in_ending(void);
int scaled_endings_oversize(gaddr sp);
int scaled_endings_profile_shift(gaddr sp);
int scaled_endings_on(void);
uint64_t rt_vblank_count(void);
extern int sprite_grid_clip_x;                  /* sprite_grid.c */
extern float sprite_grid_clip_lo, sprite_grid_clip_hi;
#define SCALE_X  0x08E759E8u
#define OFFSET_X 0x08E75A04u
static float fl(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }
int sprite_ending_oversize;   /* set while a scene's picture and what is linked to it are drawn (scaled_endings_oversize,
                                 sprite_drawers.c): 1 over-sized, 2 / 4 a backdrop not yet measured, 3 at its size,
                                 5 a backdrop over a stage drawn as it is, over-sized */
#define INTO   16            /* how far into the 4:3 picture a sprite's cells must reach to count as in it */
/* By sprite: what it is (kind, type), when it was last drawn, where on the screen its anchor was then (pos,
   after the camera: a pan moves it there though its own x stays), how far its cells reached (reach: known),
   and whether it has been seen in this scene (moved on the screen, or reached INTO into the picture): from
   then on it is drawn whole, so it doesn't pop at the 4:3 side when the camera pans or it stops. */
static struct { gaddr sp; uint16_t what; long last; int has_pos, reach, shown; int16_t pos; int32_t lo, hi; } seen[64];
static int seen_now = -1;            /* the entry of the sprite being drawn */
static int16_t pos_now; static int pos_got;   /* its anchor on the screen this frame (draw_sprite) */
extern int sprite_grid_extent_on;
extern int32_t sprite_grid_extent_lo, sprite_grid_extent_hi;
/* a sprite still waiting off the picture: never seen in it in this scene */
static int parked(gaddr sp) {
    long now = (long)rt_vblank_count();
    uint16_t what = (uint16_t)(*B(sp + 2) << 8 | *B(sp + 3));
    int i, free_ = -1, oldest = 0;
    for (i = 0; i < 64; i++) {
        if (seen[i].sp == sp) break;
        if (!seen[i].sp && free_ < 0) free_ = i;
        if (seen[i].last < seen[oldest].last) oldest = i;
    }
    if (i == 64) { i = free_ >= 0 ? free_ : oldest; seen[i].sp = sp; seen[i].last = -10; }
    if (now - seen[i].last > 2 || seen[i].what != what) {    /* new here, or the slot holds another sprite now */
        seen[i].what = what; seen[i].has_pos = seen[i].reach = seen[i].shown = 0;
    }
    seen[i].last = now;
    seen_now = i;
    return !seen[i].shown;
}
/* after the sprite is drawn: what it showed this frame */
static void parked_after(float lo43, float hi43) {
    if (seen_now < 0) return;
    int i = seen_now;
    if (pos_got) {
        if (seen[i].has_pos && seen[i].pos != pos_now) seen[i].shown = 1;   /* moved on the screen */
        seen[i].pos = pos_now; seen[i].has_pos = 1;
    }
    seen[i].reach = sprite_grid_extent_lo <= sprite_grid_extent_hi;
    seen[i].lo = sprite_grid_extent_lo; seen[i].hi = sprite_grid_extent_hi;
    /* reaching INTO into the picture (a cell's edge is often transparent) */
    if (seen[i].reach && seen[i].hi > (int32_t)lo43 + INTO && seen[i].lo < (int32_t)hi43 - INTO) seen[i].shown = 1;
}
static int ending_sides(gaddr sp, int *shift) {
    *shift = 0;
    if (!widescreen_in_ending()) return 0;
    int ov = scaled_endings_oversize(sp);
    if (ov) sprite_ending_oversize = ov;
    if (ov && ov != 4) return 0;      /* 4: maybe scaled (sprite_drawers.c), else cut as any sprite here */
    int px = scaled_endings_profile_shift(sp);
    float sx = fl(SCALE_X), ox = fl(OFFSET_X);
    int whole = scaled_endings_on() && (px != 0 || !parked(sp));
    float lo = whole ? -48.0f : 0.0f, hi = whole ? 432.0f : 384.0f;
    lo = lo * sx; lo = lo + ox;
    hi = hi * sx; hi = hi + ox;
    sprite_grid_clip_lo = lo; sprite_grid_clip_hi = hi;
    if (px != 0 && sx > 0.0f) { float u = (float)px / sx; *shift = (int)(u < 0.0f ? u - 0.5f : u + 0.5f); }
    return 1;
}

/* Draws the sprite at `sp` if it is active and shown: the two sprites linked before it, itself, then
   (unless it stands alone) the one linked after it and the next in its chain. */
static void draw_sprite(Cpu *c, gaddr sp);
static void draw_sprite_wide(Cpu *c, gaddr sp) {
    if (!rt_widescreen) { draw_sprite(c, sp); return; }
    int keep = edge_shift;
    int top = edge_depth == 0, clip_keep = sprite_grid_clip_x, ending = 0;
    if (top) edge_shift = widescreen_edge_shift(sp);
    else edge_shift += widescreen_linked_shift(sp);
    if (top) { int shift; seen_now = -1; ending = ending_sides(sp, &shift); if (ending) { sprite_grid_clip_x = 1; edge_shift += shift; } }
    int measuring = top && ending && seen_now >= 0;
    if (measuring) { sprite_grid_extent_on = 1; sprite_grid_extent_lo = INT32_MAX; sprite_grid_extent_hi = INT32_MIN; pos_got = 0; }
    edge_depth++;
    for (int n = 0; top && n < 2; n++) {
        int copy = widescreen_edge_copy(sp, n);
        if (copy) { int k = edge_shift; edge_shift = copy; draw_sprite(c, sp); edge_shift = k; }
    }
    draw_sprite(c, sp);
    edge_depth--;
    edge_shift = keep;
    if (ending) sprite_grid_clip_x = clip_keep;
    if (measuring) {
        sprite_grid_extent_on = 0;
        float sx = fl(SCALE_X), ox = fl(OFFSET_X), lo43 = 0.0f * sx, hi43 = 384.0f * sx;
        lo43 = lo43 + ox; hi43 = hi43 + ox;
        parked_after(lo43, hi43);
    }
    if (top) sprite_ending_oversize = 0;
}
static void draw_sprite(Cpu *c, gaddr sp) {
    if (*B(sp + SP_ACTIVE) == 0 || *B(sp + SP_SHOWN) == 0 || *W(DRAW_OFF) != 0) return;
    gaddr ds = DRAW_STATE;
    if (*B(sp + SP_GROUP) != GROUP_ALONE) {
        if (*W(sp + SP_BEFORE_1) != 0) CALL1(f_0880C308, *W(sp + SP_BEFORE_1));
        if (*W(sp + SP_BEFORE_2) != 0) CALL1(f_0880C308, *W(sp + SP_BEFORE_2));
    }
    *H(ds + DS_POS_X) = (uint16_t)S16(sp + SP_X);
    *H(ds + DS_POS_Y) = (uint16_t)(0xF0 - S16(sp + SP_Y));
    CALL2(f_0880E6E8, ds, sp);
    if (sprite_grid_extent_on && edge_depth == 1 && !pos_got) { pos_now = S16(ds + DS_POS_X); pos_got = 1; }   /* WIDE NATIVE, an ending (above) */
    if (edge_shift) *H(ds + DS_POS_X) = (uint16_t)(S16(ds + DS_POS_X) + edge_shift);
    *H(ds + DS_X) = *H(ds + DS_POS_X);
    *H(ds + DS_Y) = *H(ds + DS_POS_Y);
    uint8_t flip = *B(sp + SP_FLIP);
    *W(ds + DS_FLIP_X) = flip & 1u;
    *W(ds + DS_FLIP_Y) = flip >> 1;
    const uint8_t *colour = B(sp + SP_COLOUR);
    *W(ds + DS_COLOUR) = (uint32_t)colour[0] << 24 | (uint32_t)colour[1] << 16 | (uint32_t)colour[2] << 8 | colour[3];
    *B(ds + DS_BLEND) = *B(sp + SP_BLEND);
    *W(ds + DS_ATTR) = *H(sp + SP_ATTR);
    if (*H(sp + SP_STATE) == 5) *W(sp + SP_DONE) = 1;

    switch_ge_mode(c, sp, 1);
    c->r[4] = ds; c->r[5] = sp;
    rt_call_indirect(c, *W(DRAWERS + *W(sp + SP_DRAWER) * 4u));
    switch_ge_mode(c, sp, 0);

    if (*B(sp + SP_GROUP) == GROUP_ALONE) return;
    if (*W(sp + SP_AFTER) != 0) CALL1(f_0880C308, *W(sp + SP_AFTER));
    if (*W(sp + SP_NEXT) != 0) CALL1(f_0880C308, *W(sp + SP_NEXT));
}

/* void draw_sprite(sprite *a0): wrapped so WIDE NATIVE's stage extensions (stagestrips.c) can add art to a
   foreground stage object right after it */
SFA_FUNC_WRAPPED(0880C308, game_draw_sprite, stagestrips_object) { c->r[29] -= 0x20; draw_sprite_wide(c, ARG(0)); c->r[29] += 0x20; }
SFA_FUNC(0880E6E8, game_sprite_anchor) { sprite_anchor(ARG(0), ARG(1)); }

/* ---- 0.82.0 ---- */

/* (draw state, sprite, size, offset): a part's place: the offset (two s16) scaled about the part's
   centre (size bytes +4 / +5, 16 pixels a step) by the sprite's scale (+32 / +34, 8.8), then turned by
   its angle (+36's high byte, a 2 × 2 matrix of 8.8 values at 0x08A0432C), added to the draw state's
   +80 / +82 into +0 / +2. */
SFA_FUNC(0880D214, sprite_part_place) {
    gaddr d = ARG(0), s = ARG(1), size = ARG(2), off = ARG(3);
    int32_t cw = *B(size + 4) << 4, ch = *B(size + 5) << 4;
    int32_t px = (int16_t)*H(off), py = (int16_t)*H(off + 2);
    int64_t m = (int64_t)(int16_t)(cw + px + 8) * ((int16_t)*H(s + 32) >> 8);
    int32_t sx = (int16_t)(uint32_t)m >> 4;
    uint32_t r2 = (uint32_t)sx << 16;
    m = (int64_t)(int16_t)(ch + py + 8) * ((int16_t)*H(s + 34) >> 8);
    int16_t x = (int16_t)(px + (int16_t)sx);
    int16_t y = (int16_t)(py + (int16_t)((int16_t)(uint32_t)m >> 4));
    uint32_t angle = (uint32_t)*H(s + 36) >> 8;
    if (angle != 0) {
        gaddr e = 0x08A0432Cu + angle * 8u;
        int32_t c0 = (int16_t)*H(e), c1 = (int16_t)*H(e + 2), c2 = (int16_t)*H(e + 4), c3 = (int16_t)*H(e + 6);
        int32_t a = (int16_t)(cw + x + 8), b = (int16_t)(ch + y + 8);
        int32_t ac0 = (int32_t)((uint32_t)a * (uint32_t)c0), bc1 = (int32_t)((uint32_t)b * (uint32_t)c1);
        int32_t ac2 = (int32_t)((uint32_t)a * (uint32_t)c2);
        m = (int64_t)b * c3;
        int16_t rx = (int16_t)((int16_t)(ac0 >> 8) + (bc1 >> 8));
        int16_t ry = (int16_t)(((int32_t)(uint32_t)m >> 8) + (int16_t)(ac2 >> 8));
        x = (int16_t)(rx - cw - 8);
        y = (int16_t)(ry - ch - 8);
        r2 = (uint32_t)bc1;
    }
    c->lo = (uint32_t)m; c->hi = (uint32_t)((uint64_t)m >> 32);
    *H(d) = (uint16_t)((int16_t)*H(d + 80) + x);
    *H(d + 2) = (uint16_t)((int16_t)*H(d + 82) + y);
    RETURN(r2);
}
