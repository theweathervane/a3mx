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
     the portrait, the small character sprite and the MODE / SPEED boxes;
   - the map screen (NEXT STAGE, kind 19): the portrait at the top left, with the scrolling banner
     text in the same sprite tree (type 7), as on the VS screen;
   - the VS screen (kind 14): the portraits (type 0). Its name bars (type 2, at x 0 and 384) are
     drawn a second time moved out, so they reach the edges without leaving a gap in the middle.
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
    if (kind == 16 && RB(GAME + 2) == 1 && type >= 2 && type <= 4) return away_from_middle(sp, margin_units());
    if (RB(GAME + 2) != 2 || RH(GAME + 4) > 2) return 0;   /* the map (step 1) and VS (2) screens */
    if (kind == 14 && type == 0) return away_from_middle(sp, margin_units());
    if (kind == 19 && type == 7) return -margin_units();
    return 0;
}
int widescreen_edge_copy(uint32_t sp) {
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    if (RB(GAME + 2) == 2 && RH(GAME + 4) <= 2 && kind == 14 && type == 2) return away_from_middle(sp, margin_units());
    return 0;
}
