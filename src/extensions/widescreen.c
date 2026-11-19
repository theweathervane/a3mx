/* WIDE FORCED: 16:9 with a 4:3 play area, as a setting.
 *
 * Rickow's SFA3MAX 16:9 patch (sfa3-PSP-widescreen, MIT) showed what to change; until 0.99.1 it was
 * downloaded and translated in as an overlay. Every function it changed is now native and reads
 * rt_widescreen itself (docs/WIDESCREEN.md lists them).
 *
 * WIDE FORCED draws for the game's NORMAL screen size (4:3, un-stretched), so OPTION MODE > SCREEN
 * SETTING > SCREEN SIZE gets a third value, WIDE FORCED: the game's own value (0x08A058A0 + 0x58, and
 * + 0x62) is NORMAL and rt_widescreen is set. Choosing NORMAL or WIDE, or anything setting the game's
 * value to WIDE (DEFAULT, a LOAD, an IMPORT), turns it off. Kept in a3mx.cfg as screen_size=WIDE FORCED;
 * the PSP save (EXPORT) says WIDE.
 *
 * SCREEN SETTING: 0x088B700C changes SCREEN SIZE (0x088B7D08: a0 pad, a1 value, a2 last value; for
 * both pads); screensetting.c draws the screen (0x088B78DC), the value's name from 0x08A37A78[value].
 */
#include "rt.h"
#include <string.h>

int rt_widescreen;
#define OPTS       0x08A058A0u
#define SIZE       (OPTS + 0x58)
#define SIZE2      (OPTS + 0x62)
#define SLOTS      0x08E75BA0u
void f_088B7D08(Cpu *c);

int widescreen_value(void) { return rt_widescreen ? 2 : RH(SIZE) ? 1 : 0; }
void widescreen_set(int v) {                   /* 0 NORMAL, 1 WIDE, 2 WIDE FORCED */
    rt_widescreen = v == 2;
    WH(SIZE, v == 1); WH(SIZE2, v == 1);
}
/* once a frame: the game's value turning from NORMAL to WIDE (DEFAULT, a load, an IMPORT) ends WIDE
 * FORCED (at boot the game's default is WIDE until the system data is in: that is not a change) */
void widescreen_frame(void) {
    static int last = -1, was = -1;
    int now = RH(SIZE) != 0;
    if (rt_widescreen && was == 0 && now) rt_widescreen = 0;
    was = now;
    if (rt_widescreen != last) { rt_log("widescreen: %s", rt_widescreen ? "WIDE FORCED on" : "off"); last = rt_widescreen; }
}

/* SCREEN SIZE, left / right */
int widescreen_screen_size(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    int v = widescreen_value();
    for (int k = 0; k < 2; k++) {
        c->r[4] = SLOTS + 0x2Cu * (uint32_t)k; c->r[5] = (uint32_t)v; c->r[6] = 2;
        f_088B7D08(c); v = (int)(int16_t)c->r[2];
    }
    if (v < 0 || v > 2) v = 0;
    widescreen_set(v);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    return 1;
}

/* the screen, with WIDE FORCED as the value's name while it is on, is drawn by screensetting.c */

/* SCREEN SETTING's green bar (an object, kind 0xC type 0xA, 168 wide; the width is only set when it is
 * made) is as wide as the longest value any row can show, on every row (screensetting.c works it out:
 * 222 for WIDE FORCED today), so no value runs past it */
#define OBJS     0x092E2B88u
#define OBJ_SZ   0x94u
#define BAR_W    168
int screensetting_bar_width(void);
void widescreen_bar(void) {                    /* exsetting.c: once a frame on SCREEN SETTING */
    static int wide;
    if (!wide) wide = screensetting_bar_width();
    if (wide < BAR_W) wide = BAR_W;
    for (int k = 0; k < 32; k++) {
        uint32_t o = OBJS + OBJ_SZ * (uint32_t)k;
        if (!RB(o + 0x5A) || RB(o + 0x5B) != 0xC || RB(o + 0x5C) != 0xA) continue;
        int w = (int16_t)RH(o + 0xE);
        if (w == BAR_W || w == wide) WH(o + 0xE, (uint32_t)wide);
    }
}

/* ---------------------------------------------------------------- side pieces to the screen's edges */
/* WIDE FORCED: some screens' side pieces move out by the margin, away from the middle, so they sit at
   the 16:9 screen's edges as they sat at the 4:3 one's. The sprite drawer (core/object/sprite.c)
   asks for each sprite tree it draws; only the drawing moves, game memory keeps the 4:3 places.
   - the character select screen (GAME + 2 = 1): the select tasks (object kind 16) of types 2 - 4,
     the portrait, the small character sprite and the MODE / SPEED boxes; type 6, the chosen MODE /
     ISM panel and the name under the portrait; and the ISM panel's parts (kind 33);
   - the map screen (NEXT STAGE, kind 19): the portrait at the top left, with the scrolling banner
     text in the same sprite tree (type 7), as on the VS screen; the ladder of stages with the next
     opponent's name bar (types 4 - 6) on the other side; the profile screen before it (kind 113): its
     scrolling columns at the sides (types 3 and 4);
   - the win screen after a fight (step 4, kind 18): its scrolling columns at the sides (types 4, 6);
   - the VS screen (kind 14): the portraits (type 0). Its name bars (type 2, at x 0 and 384) are
     drawn a second time moved out, so they reach the edges without leaving a gap in the middle.
   - the select screen's name ticker (kind 16 type 5, the top one): drawn one period further each way,
     so its words reach the 16:9 sides instead of appearing in the right margin.
   In the sprite's own units: the screen's 48 pixels through SCALE_X. */
#define SCALE_X 0x08E759E8u
#define GAME    0x08E75910u
static int margin_units(void) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return scale > 0.f ? (int)(48.f / scale + 0.5f) : 0;
}
static int away_from_middle(uint32_t sp, int units) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return (float)(int16_t)RH(sp + 18) * scale < 192.f ? -units : units;
}
int widescreen_edge_shift(uint32_t sp) {
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    if (RB(GAME + 2) == 1) {                               /* the character select screen */
        if (kind == 16 && type >= 2 && type <= 4) return away_from_middle(sp, margin_units());
        if (kind == 16 && type == 6) return away_from_middle(sp, margin_units());   /* the chosen MODE / ISM panel, the name */
        if (kind == 33) return away_from_middle(sp, margin_units());                /* the ISM panel's parts */
    }
    /* the win screen after a fight (GAME + 4 = 4): its scrolling columns at the sides (kind 18 types 4, 6) */
    if (RB(GAME + 2) == 2 && RH(GAME + 4) == 4 && kind == 18 && (type == 4 || type == 6))
        return away_from_middle(sp, margin_units());
    if (RB(GAME + 2) != 2 || RH(GAME + 4) > 2) return 0;   /* the map (step 1) and VS (2) screens */
    if (kind == 14 && type == 0) return away_from_middle(sp, margin_units());
    if (kind == 19 && type == 7) return away_from_middle(sp, margin_units());   /* P1 left, P2 right */
    if (kind == 19 && type >= 4 && type <= 6) return away_from_middle(sp, margin_units());   /* the ladder, opposite */
    if (RH(GAME + 4) == 1 && kind == 113 && (type == 3 || type == 4)) return away_from_middle(sp, margin_units());   /* the profile screen's side columns */
    return 0;
}
int widescreen_edge_copy(uint32_t sp, int n) {
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    if (n == 0 && RB(GAME + 2) == 2 && RH(GAME + 4) <= 2 && kind == 14 && type == 2) return away_from_middle(sp, margin_units());
    /* the select screen's name ticker at the top (kind 16 type 5, y 232): a strip that scrolls and jumps
       back by 320 when it has gone that far, too short for the 16:9 sides: copies one period each way */
    if (RB(GAME + 2) == 1 && kind == 16 && type == 5 && (int16_t)RH(sp + 22) >= 200) return n ? -320 : 320;
    return 0;
}

/* WIDE FORCED: sprites whose pictures are bands across the 4:3 picture, continued into the margins
   (core/object/sprite_drawers.c, sprite_grid.c). The answer says which pieces (1 the solid ones, 2 the
   half-alpha after-images) and how (4: the left margin mirrors the glow before the right end; 8: the
   left margin only). In the fight (GAME + 2 = 2, step 3), drawer 11: the ROUND n banner (kind 0 type 0),
   its dark band (1 + 4); FIGHT (type 1), its translucent box (2), as it flies in and settles (the word
   itself is edge to edge already). The continue screen (GAME + 2 = 3): its count's bar (kind 13 type 2,
   1 + 8: its right end breaks up). The GAME OVER screen (GAME + 2 = 4): its band (kind 13 types 4 - 6;
   pictures narrower than the screen, the words, are left alone by sprite_grid.c). */
int widescreen_edge_cells(uint32_t sp) {
    if (RB(GAME + 2) == 4 && RB(sp + 2) == 13 && RB(sp + 3) >= 4 && RB(sp + 3) <= 6) return 1;   /* GAME OVER's band */
    if (RB(GAME + 2) == 3 && RB(sp + 2) == 13 && RB(sp + 3) == 2) return 1 | 8;                    /* the continue bar */
    if (RB(GAME + 2) != 2 || RH(GAME + 4) != 3 || RB(sp + 2) != 0 || RW(sp + 148) != 11) return 0;
    uint32_t type = RB(sp + 3);
    return type == 0 ? 1 | 4 : type == 1 ? 2 : 0;
}

/* ... and rows of pieces continued one piece into the margins (sprite_drawers.c), in the fight: the two
   rows of FIGHT FIGHT ... that slide in with the FIGHT banner (kind 1 type 4, drawer 1) and the
   scrolling KNOCK OUT row (kind 0 type 3, drawer 1), which stopped at the 4:3 picture's right edge (1).
   And 2: the continue screen's count (kind 13 type 3, drawer 1), its numbers carried on to the left. */
int widescreen_edge_row(uint32_t sp) {
    if (RB(GAME + 2) == 3 && RB(sp + 2) == 13 && RB(sp + 3) == 3 && RW(sp + 148) == 1) return 2;   /* the continue count */
    if (RB(GAME + 2) != 2 || RH(GAME + 4) != 3 || RW(sp + 148) != 1) return 0;
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    return (kind == 1 && type == 4) || (kind == 0 && type == 3);
}

/* ... and the map screen's beam (GAME + 2 = 2, step 1; kind 19 type 2, drawer 0, on anchor 3): a wedge
   from the map's middle toward the next stage, cut where its picture ends; sprite_drawers.c draws scaled
   copies behind it so it carries on to the screen's sides. */
int widescreen_beam(uint32_t sp) {
    return RB(GAME + 2) == 2 && RH(GAME + 4) == 1 && RB(sp + 2) == 19 && RB(sp + 3) == 2 &&
           RW(sp + 148) == 0 && RB(sp + 14) == 3;
}

/* ... and panels the width of the 4:3 picture drawn widened, their halves moved out to the 16:9 sides
   and their plain middle stretched (core/object/sprite_drawers.c, drawer 3; sprite_grid.c): the profile screen before the map (GAME + 2 = 2,
   step 1, kind 113), its header (type 0) and its text box (type 1); the text itself stays centred. */
int widescreen_widen(uint32_t sp) {
    return RB(GAME + 2) == 2 && RH(GAME + 4) == 1 && RB(sp + 2) == 113 && RB(sp + 3) <= 1;
}

/* ... and plain rectangles of the third object list (core/object/draw_kinds.c, kind 0) whose left and
   right edges move by this much: the win screen's pink band under the quote (type 1, 48 out each side,
   to the side columns once they move out); on the profile screen (step 1) the name plate's box (type 0,
   48 left with the header) and the box behind the text (type 1, 48 out each side with the text box). */
void widescreen_plain_adjust(uint32_t ob, int *left, int *right) {
    *left = *right = 0;
    if (RB(GAME + 2) != 2 || (RB(ob + 88) & 0x3F) != 0) return;
    uint32_t type = RB(ob + 92), step = RH(GAME + 4);
    int16_t w = (int16_t)RH(ob + 14);   /* the map screen (also step 1) has a full-width bar: by size */
    if (step == 4 && type == 1) { *left = -48; *right = 48; }
    if (step == 1 && type == 1 && w == 304) { *left = -48; *right = 48; }
    if (step == 1 && type == 0 && w == 112) { *left = *right = -48; }
}
