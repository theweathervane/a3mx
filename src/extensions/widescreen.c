/* WIDE FORCED: the SFA3MAX 16:9 patch (Rickow, sfa3-PSP-widescreen, MIT) as a setting.
 *
 * prepare.py applies the patch to a copy of the program and records what it changes
 * (work/boot.widescreen.json): 35 words of .text and code in unused .data. The translation keeps both
 * meanings of those words and runs the patched one while rt_widescreen is set (psp_recomp.py); the
 * added code is translated too. Without the patch (prepare.py --no-widescreen, or no download) the
 * setting is not offered.
 *
 * The patch draws for the game's NORMAL screen size (4:3, un-stretched), so OPTION MODE > SCREEN
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
void inner_f_088B700C(Cpu *c);   /* option_mode.c wraps it (decomp.h) */

int rt_widescreen;
extern const int rt_widescreen_available;      /* generated (dispatch.c) */
#define OPTS       0x08A058A0u
#define SIZE       (OPTS + 0x58)
#define SIZE2      (OPTS + 0x62)
#define SLOTS      0x08E75BA0u
void f_088B7D08(Cpu *c);

int widescreen_value(void) { return rt_widescreen ? 2 : RH(SIZE) ? 1 : 0; }
void widescreen_set(int v) {                   /* 0 NORMAL, 1 WIDE, 2 WIDE FORCED */
    if (v == 2 && !rt_widescreen_available) v = 0;
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
    if (!rt_widescreen_available) { inner_f_088B700C(c); return 1; }
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
