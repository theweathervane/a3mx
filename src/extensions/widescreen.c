/* WIDE NATIVE: 16:9 with a 4:3 play area, as a setting.
 *
 * Rickow's SFA3MAX 16:9 patch (sfa3-PSP-widescreen, MIT) showed what to change; until 0.99.1 it was
 * downloaded and translated in as an overlay. Every function it changed is now native and reads
 * rt_widescreen itself (docs/WIDESCREEN.md lists them).
 *
 * WIDE NATIVE draws for the game's NORMAL screen size (4:3, un-stretched), so OPTION MODE > SCREEN
 * SETTING > SCREEN SIZE gets a third value, WIDE NATIVE: the game's own value (0x08A058A0 + 0x58, and
 * + 0x62) is NORMAL and rt_widescreen is set. Choosing NORMAL or WIDE, or anything setting the game's
 * value to WIDE (DEFAULT, a LOAD, an IMPORT), turns it off. Kept in a3mx.cfg as screen_size=WIDE NATIVE;
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
void widescreen_set(int v) {                   /* 0 NORMAL, 1 WIDE, 2 WIDE NATIVE */
    rt_widescreen = v == 2;
    WH(SIZE, v == 1); WH(SIZE2, v == 1);
}
/* once a frame: the game's value turning from NORMAL to WIDE (DEFAULT, a load, an IMPORT) ends WIDE
 * NATIVE (at boot the game's default is WIDE until the system data is in: that is not a change) */
void widescreen_frame(void) {
    static int last = -1, was = -1;
    int now = RH(SIZE) != 0;
    if (rt_widescreen && was == 0 && now) rt_widescreen = 0;
    was = now;
    if (rt_widescreen != last) { rt_log("widescreen: %s", rt_widescreen ? "WIDE NATIVE on" : "off"); last = rt_widescreen; }
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

/* the screen, with WIDE NATIVE as the value's name while it is on, is drawn by screensetting.c */

/* SCREEN SETTING's green bar (an object, kind 0xC type 0xA, 168 wide; the width is only set when it is
 * made) is as wide as the longest value any row can show, on every row (screensetting.c works it out:
 * 222 for WIDE NATIVE today), so no value runs past it */
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
/* WIDE NATIVE: some screens' side pieces move out by the margin, away from the middle, so they sit at
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
   - the select screen's name ticker (kind 16 type 5, the top one) and the 1P / 2P name rows (kind 16
     type 6, drawer 2): drawn one period further each way, so their words reach the 16:9 sides instead
     of appearing in the right margin.
   In the sprite's own units: the screen's 48 pixels through SCALE_X. */
#define SCALE_X 0x08E759E8u
#define GAME    0x08E75910u
static int margin_units(void) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return scale > 0.f ? (int)(48.f / scale + 0.5f) : 0;
}
/* the dialogue before a rival or final fight: its portraits stand 16 in from the 4:3 picture's sides, so
   they go 48 + 16 out to stand at the 16:9 screen's sides (their lines with them) */
static int dialogue_units(void) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return scale > 0.f ? (int)(64.f / scale + 0.5f) : 0;
}
static int away_from_middle(uint32_t sp, int units) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return (float)(int16_t)RH(sp + 18) * scale < 192.f ? -units : units;
}
/* WIDE NATIVE: the end of a round won (fight step 3): WIN and PERFECT fly in from the right and stop
   at x 112, the results panel (COMPLETE ... SCORE) flies in from the left and stops at x 240. They are
   moved by an amount that follows their place, so they enter from the 16:9 screen's sides and stop at
   its edges: for WIN / PERFECT -48 at x 112 or left of it, 0 at the middle (192), +48 at x 384 or
   right of it; for the panel -48 at x -144 (just off the 16:9 side) or left of it, +48 at x 240 or
   right of it. In the screen's pixels, through the scale. */
static int lerp_shift(int x, int x0, int s0, int x1, int s1) {
    if (x <= x0) return s0;
    if (x >= x1) return s1;
    return s0 + (s1 - s0) * (x - x0) / (x1 - x0);
}
static int win_shift_px(int x) { return x < 192 ? lerp_shift(x, 112, -48, 192, 0) : lerp_shift(x, 192, 0, 384, 48); }
static int panel_shift_px(int x) { return lerp_shift(x, -144, -48, 240, 48); }
static int px_to_units(int px) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return scale > 0.f ? (int)((float)px / scale + (px < 0 ? -0.5f : 0.5f)) : 0;
}
static int units_x(uint32_t sp) {
    uint32_t w = RW(SCALE_X); float scale; memcpy(&scale, &w, 4);
    return (int)((float)(int16_t)RH(sp + 18) * scale);
}
static int round_win(void) { return RB(GAME + 2) == 2 && RH(GAME + 4) == 3; }
static int win_piece(uint32_t kind, uint32_t type) { return (kind == 0 && type == 2) || (kind == 1 && type == 12); }
/* the panel's two parts, as 0x08855BD8 picks them from 0x08A16DF8: types 9 / 15 (five rows: COMPLETE ...
   SCORE), or 8 / 7 (four rows: TIME, the ISM's score, MAX COMBO, VITAL) in Survival (mode 7), mode 6 and
   Free Battle's TEAM (mode 0x10, BATTLE + 159 = 2); both stop at x 240 */
static int panel_piece(uint32_t kind, uint32_t type) { return kind == 1 && (type == 9 || type == 15 || type == 7 || type == 8); }
/* the 8-pool's piece of that sort, shown, if any (its x through *x) */
static int find_piece(int panel, int *x) {
    for (uint32_t o = 0x09512170u; o < 0x09512170u + 8u * 0xC0u; o += 0xC0u) {
        if (!RB(o) || !RB(o + 1)) continue;
        uint32_t k = RB(o + 2), t = RB(o + 3);
        if (panel ? panel_piece(k, t) : win_piece(k, t)) { *x = units_x(o); return 1; }
    }
    return 0;
}

/* the score ranking's rows: how far their place, name, score and profile move (widescreen_piece_shift) */
static const int RANK_SHIFT[4] = { -48, 0, 12, 48 };
int widescreen_edge_shift(uint32_t sp) {
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    /* the attract mode's score ranking (GAME + 0 = 4): its two RANKING columns (kind 35 types 3, 4) */
    if (RH(GAME) == 4 && kind == 35 && (type == 3 || type == 4)) return away_from_middle(sp, margin_units());
    if (RH(GAME) == 4 && kind == 35 && type == 1) return px_to_units(RANK_SHIFT[3]);   /* a ranking row's profile's face (widescreen_piece_shift) */
    /* the attract mode's red speed-line screens (GAME + 0 = 2): the STREET FIGHTER ALPHA 3 strip (kind 48
       type 3, one picture 480 wide that sways from x -15 to -5) starts at the 16:9 screen's left side,
       and is drawn again after itself (widescreen_edge_copy) */
    if (RH(GAME) == 2 && kind == 48 && type == 3) return px_to_units(-48);
    if (RB(GAME + 2) == 1) {                               /* the character select screen */
        if (kind == 16 && type >= 2 && type <= 4) return away_from_middle(sp, margin_units());
        if (kind == 16 && type == 6 && RW(sp + 148) == 2) return 0;                  /* the 1P / 2P name rows (widescreen_edge_copy) */
        if (kind == 16 && type == 6) return away_from_middle(sp, margin_units());   /* the chosen MODE / ISM panel, the name */
        if (kind == 33) return away_from_middle(sp, margin_units());                /* the ISM panel's parts */
    }
    /* the win screen after a fight (GAME + 4 = 4): its scrolling columns at the sides (kind 18 types 4, 6) */
    if (RB(GAME + 2) == 2 && RH(GAME + 4) == 4 && kind == 18 && (type == 4 || type == 6))
        return away_from_middle(sp, margin_units());
    if (round_win() && win_piece(kind, type)) return px_to_units(win_shift_px(units_x(sp)));
    if (round_win() && panel_piece(kind, type)) return px_to_units(panel_shift_px(units_x(sp)));
    /* the portraits of the dialogue before a rival or final fight (step 3, kind 14 type 6) */
    if (RB(GAME + 2) == 2 && RH(GAME + 4) == 3 && kind == 14 && type == 6) return away_from_middle(sp, dialogue_units());
    if (RB(GAME + 2) != 2 || RH(GAME + 4) > 2) return 0;   /* the map (step 1) and VS (2) screens */
    if (kind == 14 && type == 0) return away_from_middle(sp, margin_units());
    if (kind == 19 && type == 7) return away_from_middle(sp, margin_units());   /* P1 left, P2 right */
    if (kind == 19 && type >= 4 && type <= 6) return away_from_middle(sp, margin_units());   /* the ladder, opposite */
    if (RH(GAME + 4) == 1 && kind == 113 && (type == 3 || type == 4)) return away_from_middle(sp, margin_units());   /* the profile screen's side columns */
    return 0;
}
/* the columns of the picture a sprite shows now (its frame in its picture list) */
static int picture_cols(uint32_t sp) {
    uint32_t frame = RH(RW(sp + 28) + 4);
    if (frame & 0x8000) return 0;
    uint32_t list = RW(0x08E75AD8u + RH(sp + 86) * 4u);
    return RB(list + frame * 12u + 4u);
}
int widescreen_edge_copy(uint32_t sp, int n) {
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    if (n == 0 && RH(GAME) == 2 && kind == 48 && type == 3) return px_to_units(-48 + 480);
    /* the attract mode's flames before M. Bison (kind 52 types 0 - 3, drawer 7, four pieces across the
       4:3 picture): the rightmost again a picture's width to the left, the leftmost to the right */
    if (n == 0 && RH(GAME) == 2 && kind == 52 && type == 3) return px_to_units(-384);
    if (n == 0 && RH(GAME) == 2 && kind == 52 && type == 2) return px_to_units(384);
    /* the attract mode's band of fighters' faces behind a word (kind 54 type 3, a picture 384 wide; the
       earth band is 512 and needs nothing): drawn again one band to the left and one to the right, so the
       margins show half a face from the band's other end, not a copy of the face beside them */
    if (RH(GAME) == 2 && kind == 54 && type == 3 && picture_cols(sp) == 24) return px_to_units(n ? 384 : -384);
    if (n == 0 && RB(GAME + 2) == 2 && RH(GAME + 4) <= 2 && kind == 14 && type == 2) return away_from_middle(sp, margin_units());
    /* the select screen's name ticker at the top (kind 16 type 5, y 232): a strip that scrolls and jumps
       back by 320 when it has gone that far, too short for the 16:9 sides: copies one period each way */
    if (RB(GAME + 2) == 1 && kind == 16 && type == 5 && (int16_t)RH(sp + 22) >= 200) return n ? -320 : 320;
    /* the bottom one (STREET FIGHTER ALPHA3, y 24) scrolls the other way and jumps back by 480 */
    if (RB(GAME + 2) == 1 && kind == 16 && type == 5 && (int16_t)RH(sp + 22) < 200) return n ? -480 : 480;
    /* the 1P / 2P name rows ("1P RYU 1P RYU ...", kind 16 type 6 drawn by drawer 2: player 1's at the top,
       player 2's at the foot): they scroll and jump back by 64, filling just the 4:3 picture; they stay
       where they are (not moved out as the other type 6 pieces) and are drawn a period further each way */
    if (RB(GAME + 2) == 1 && kind == 16 && type == 6 && RW(sp + 148) == 2) return n ? -64 : 64;
    return 0;
}

/* WIDE NATIVE: sprites whose pictures are bands across the 4:3 picture, continued into the margins
   (core/object/sprite_drawers.c, sprite_grid.c). The answer says which pieces (1 the solid ones, 2 the
   half-alpha after-images) and how (4: the left margin mirrors the glow before the right end; 8: the
   left margin only). In the fight (GAME + 2 = 2, step 3), drawer 11: the ROUND n banner (kind 0 type 0),
   its dark band (1 + 4); FIGHT (type 1), its translucent box (2), as it flies in and settles (the word
   itself is edge to edge already). The continue screen (GAME + 2 = 3): its count's bar (kind 13 type 2,
   1 + 8: its right end breaks up). The GAME OVER screen (GAME + 2 = 4 after the continue; GAME + 0 = 3, + 2 = 0 at the end of an
   ending, with TRY AGAIN): its band (kind 13 types 4 - 6;
   pictures narrower than the screen, the words, are left alone by sprite_grid.c). */
int widescreen_edge_cells(uint32_t sp) {
    /* textured pictures across the 4:3 picture: its side cells mirrored out, then as they are (16).
       The QSound screen (GAME + 0 / + 2 = 1 / 6 - 8: fading in, shown, fading out; kind 122) and the attract mode's word screens (TRIUMPH
       OR DIE, BORN TO FIGHT ...: GAME + 0 = 2, kind 51 types 0 - 2, the words and the top word's noisy
       banner). The band that lights up behind a word (kind 54 type 3) is the earth, already 512 wide,
       or the fighters' faces, drawn again a band to each side (widescreen_edge_copy). Their words don't reach the 4:3 sides,
       so only the banners and the QSound screen's plain background go on. */
    if (RH(GAME) == 1 && RB(GAME + 2) >= 6 && RB(GAME + 2) <= 8 && RB(sp + 2) == 122) return 1 | 16;   /* 6 / 8: fading in / out */
    /* the attract mode's speed-line screens behind the fighters' close-ups: the green (kind 127 type 6)
       and the red (kind 54 type 5) */
    if (RH(GAME) == 2 && ((RB(sp + 2) == 127 && RB(sp + 3) == 6) || (RB(sp + 2) == 54 && RB(sp + 3) == 5))) return 1 | 16;
    if (RH(GAME) == 2 && RB(sp + 2) == 51 && RB(sp + 3) <= 2) return 1 | 16;
    int game_over = RB(GAME + 2) == 4 || (RH(GAME) == 3 && RB(GAME + 2) == 0);   /* after the continue; after an ending */
    if (game_over && RB(sp + 2) == 13 && RB(sp + 3) >= 4 && RB(sp + 3) <= 6) return 1;   /* GAME OVER's band */
    if (RB(GAME + 2) == 3 && RB(sp + 2) == 13 && RB(sp + 3) == 2) return 1 | 8;                    /* the continue bar */
    if (RB(GAME + 2) != 2 || RH(GAME + 4) != 3 || RB(sp + 2) != 0 || RW(sp + 148) != 11) return 0;
    uint32_t type = RB(sp + 3);
    return type == 0 ? 1 | 4 : type == 1 ? 2 : 0;
}

/* ... and backdrops across the whole 4:3 picture drawn 1.25 times over-sized about its middle instead
   (core/object/sprite_drawers.c, drawer 0), as the super finish's swirl and burst are: the attract mode's
   Akuma screens (GAME + 0 = 2), the blue swirl behind his back (kind 54 types 8 and 9, 9 for the few
   frames before the kanji) and the burst behind the red kanji (kind 54 type 6) */
int widescreen_oversize(uint32_t sp) {
    uint32_t type = RB(sp + 3);
    return RH(GAME) == 2 && RB(sp + 2) == 54 && (type == 6 || type == 8 || type == 9);
}

/* ... and rows of pieces continued one piece into the margins (sprite_drawers.c), in the fight: the two
   rows of FIGHT FIGHT ... that slide in with the FIGHT banner (kind 1 type 4, drawer 1) and the
   scrolling KNOCK OUT row (kind 0 type 3, drawer 1), which stopped at the 4:3 picture's right edge (1).
   And 2: the continue screen's count (kind 13 type 3, drawer 1), its numbers carried on to the left.
   The attract mode's red speed-line screens: the rows of a fighter's name (kind 48 type 4, drawer 1, a
   name every 96) (1); its blue screens' name banners (kind 50 type 7, drawer 1) (1); and 3: on its name
   rows over black, the four coloured panels behind them (kind 126 type 5, drawer 6, 96 apart) carried
   on round (the names and outlines are holes in a black layer, core/object/mask_layer.c). */
int widescreen_edge_row(uint32_t sp) {
    if (RH(GAME) == 2 && RB(sp + 2) == 48 && RB(sp + 3) == 4 && RW(sp + 148) == 1) return 1;   /* the attract's name rows */
    if (RH(GAME) == 2 && RB(sp + 2) == 50 && RB(sp + 3) == 7 && RW(sp + 148) == 1) return 1;   /* the attract's name banners */
    if (RH(GAME) == 2 && RB(sp + 2) == 126 && RB(sp + 3) == 5 && RW(sp + 148) == 6) return 3;  /* the panels behind the name rows */
    if (RB(GAME + 2) == 3 && RB(sp + 2) == 13 && RB(sp + 3) == 3 && RW(sp + 148) == 1) return 2;   /* the continue count */
    if (RB(GAME + 2) != 2 || RH(GAME + 4) != 3 || RW(sp + 148) != 1) return 0;
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    return (kind == 1 && type == 4) || (kind == 0 && type == 3);
}

/* ... and bars drawn longer at both ends (core/object/sprite_drawers.c drawer 3, sprite_grid.c): the
   attract mode's score ranking (GAME + 0 = 4), its score bars (kind 35 type 9), 48 pixels more at each end
   in their plain colours, as its RANKING columns stand 48 further out (widescreen_edge_shift), so the
   ladder meets them as on the 4:3 screen (the answer: the pixels at each end, 0 none) */
int widescreen_bar_ends(uint32_t sp) {
    return RH(GAME) == 4 && RB(sp + 2) == 35 && RB(sp + 3) == 9 ? 48 : 0;
}

/* ... and the rows of the score ranking (GAME + 0 = 4), whose bars are 48 longer at each end
   (widescreen_bar_ends): what sits on a bar is spread out with it, in the screen's pixels. A row's text
   is one sprite of pieces (kind 35 type 10, the first place's type 11; drawer 6), its black shadow the
   same pieces in a linked sprite (kind 34 type 4); the pieces by their x in the row: the place ("1st",
   "20th": x -152 to -120), the name (-104 to -72), the score (-8 to 56). The profile at the bar's right
   end is a sprite of its own (kind 35 type 1, the face), a linked one (kind 34 type 5, the fighter's
   name under it) and a translucent box behind them (a plain rectangle, types 1 - 3 by ISM, widescreen_plain_adjust). The
   place and the profile go the whole 48 to the bar's ends, so each stands as far from its end as on the
   4:3 screen (10 and 7 pixels from the left end, the face 17 past the right); the name stays and the
   score goes 12 right, so the gaps between them come out nearly even (first place 62 / 59 / 59, the
   others 62 / 62 / 50; on the 4:3 screen 14 / 47 / 23 and 14 / 50 / 14). RANK_SHIFT (place, name, score,
   profile), in pixels. */
static int ranking_row(uint32_t sp) {
    uint32_t kind = RB(sp + 2), type = RB(sp + 3);
    return RH(GAME) == 4 && ((kind == 35 && (type == 10 || type == 11)) || (kind == 34 && type == 4));
}
int widescreen_piece_shift(uint32_t sp, int dx) {
    if (!ranking_row(sp)) return 0;
    return px_to_units(RANK_SHIFT[dx <= -112 ? 0 : dx < -40 ? 1 : 2]);
}
/* the profile's linked sprite (kind 34 type 5) moves with the face (kind 35 type 1, widescreen_edge_shift):
   a linked sprite's own move, added to its parent's (core/object/sprite.c) */
int widescreen_linked_shift(uint32_t sp) {
    if (RH(GAME) == 4 && RB(sp + 2) == 34 && RB(sp + 3) == 5) return px_to_units(RANK_SHIFT[3]);
    return 0;
}

/* ... and the ALPHA letter tiles before the title (GAME + 0 = 5; kind 125 type 9, drawer 1: ten columns of
   seven tiles, each column a sprite, ~41 pixels apart, sliding in from the right and closing up into a
   grid that narrows): once the leftmost is within two columns' spacing of the left side, the
   grid carries on round: the last two columns are drawn again before the first, the first two after the
   last, at the spacing of the end they join, so the 16:9 screen has no black side columns. The answer:
   how many copies of this column (0 - 2) and their moves in pixels (dx). */
int widescreen_column_copies(uint32_t sp, int dx[2]) {
    if (RH(GAME) != 5 || RB(sp + 2) != 125 || RB(sp + 3) != 9) return 0;
    int xs[32], n = 0, me = units_x(sp);
    for (uint32_t o = 0x09512770u; o < 0x09512770u + 32u * 0xC0u && n < 32; o += 0xC0u)
        if (RB(o) && RB(o + 1) && RB(o + 2) == 125 && RB(o + 3) == 9) xs[n++] = units_x(o);
    if (n < 4) return 0;
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && xs[j] < xs[j - 1]; j--) { int t = xs[j]; xs[j] = xs[j - 1]; xs[j - 1] = t; }
    int step_l = xs[1] - xs[0], step_r = xs[n - 1] - xs[n - 2];
    if (step_l <= 0 || step_r <= 0 || xs[0] > 2 * step_l) return 0;   /* still sliding in */
    int k = 0;
    if (me == xs[n - 1]) dx[k++] = xs[0] - step_l - me;           /* before the first */
    if (me == xs[n - 2]) dx[k++] = xs[0] - 2 * step_l - me;
    if (me == xs[0]) dx[k++] = xs[n - 1] + step_r - me;           /* after the last */
    if (me == xs[1]) dx[k++] = xs[n - 1] + 2 * step_r - me;
    return k > 2 ? 2 : k;
}

/* ... and the map screen's beam (GAME + 2 = 2, step 1; kind 19 type 2, drawer 0, on anchor 3): a wedge
   from the map's middle toward the next stage, cut where its picture ends; sprite_drawers.c carries its
   noise on past that, 1:1, to the screen's side (0.99.34; before, scaled copies, which blew up the lit
   wings in the pictures toward England, Italy, Hong Kong and Thailand). */
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
   48 left with the header) and the box behind the text (type 1, 48 out each side with the text box); on
   the attract mode's score ranking, the box behind the first place's portrait (48 out each side). */
void widescreen_plain_adjust(uint32_t ob, int *left, int *right) {
    *left = *right = 0;
    /* the endings (screen 1): the black sides that narrow a scene to a panel (type 1, at a 4:3 side or past
       it: 96 wide at x 0 and 288 in Ryu's ending, 64 at 0 and 320 around Dee Jay's punch) go on to the 16:9
       screen's sides, so the scene's tiles and backdrop past them stay hidden */
    int16_t pw = (int16_t)RH(ob + 14);
    if (RB(GAME + 2) == 1 && (RB(ob + 88) & 0x3F) == 0 && RB(ob + 92) == 1 && pw > 0 && pw < 384) {
        int16_t x = (int16_t)RH(ob + 6);
        if (x <= 0) *left = -48;
        if (x + pw >= 384) *right = 48;
        return;
    }
    /* the attract mode's score ranking (GAME + 0 = 4): the translucent box behind the first place's
       portrait (type 0, 288 wide) reaches under the coloured panels at the RANKING columns, which stand 48
       further out (widescreen_edge_shift), as on the 4:3 screen: 48 out each side */
    if (RH(GAME) == 4 && (RB(ob + 88) & 0x3F) == 0 && RB(ob + 92) == 0 && (int16_t)RH(ob + 14) == 288) {
        *left = -48; *right = 48;
        return;
    }
    /* ... and each row's translucent box behind the profile (96 wide; type 1 on the X-ISM ranking, 2 on
       the A-ISM, 3 on the V-ISM): moved with the face */
    if (RH(GAME) == 4 && (RB(ob + 88) & 0x3F) == 0 && RB(ob + 92) >= 1 && RB(ob + 92) <= 3 && (int16_t)RH(ob + 14) == 96) {
        *left = *right = RANK_SHIFT[3];
        return;
    }
    if (RB(GAME + 2) != 2 || (RB(ob + 88) & 0x3F) != 0) return;
    uint32_t type = RB(ob + 92), step = RH(GAME + 4);
    int16_t w = (int16_t)RH(ob + 14);   /* the map screen (also step 1) has a full-width bar: by size */
    if (step == 4 && type == 1) { *left = -48; *right = 48; }
    /* the end of a round won: WIN's band (type 1) moves with WIN, the panel's bars (types 2 - 6) with it */
    int px;
    if (step == 3 && type == 1 && find_piece(0, &px)) *left = *right = win_shift_px(px);
    if (step == 3 && type >= 2 && type <= 6 && find_piece(1, &px)) *left = *right = panel_shift_px(px);
    if (step == 1 && type == 1 && w == 304) { *left = -48; *right = 48; }
    if (step == 1 && type == 0 && w == 112) { *left = *right = -48; }
}

/* ... and of those, the ones drawn in two halves so the renderer does not carry them on to the screen's
   sides (it does for any rectangle across the whole 4:3 picture): the profile screen's box behind the
   text (step 1, type 1, 304 wide), which, widened, spans the picture exactly; carried on, it filled the
   frame's socket left of the box (screen x 0 - 48) navy */
int widescreen_plain_halves(uint32_t ob) {
    return RB(GAME + 2) == 2 && RH(GAME + 4) == 1 && (RB(ob + 88) & 0x3F) == 0 && RB(ob + 92) == 1
        && (int16_t)RH(ob + 14) == 304;
}

/* ... and the dialogue's lines (core/object/text_boxes.c): moved with the portrait (kind 14 type 6, in
   the 32-pool) in their half of the screen, top or bottom (screen y = 0xF0 - the sprite's y). */
int widescreen_text_shift(uint32_t rec) {
    if (RB(GAME + 2) != 2 || RH(GAME + 4) != 3) return 0;
    int top = (int16_t)RH(rec + 30) < 112;
    for (uint32_t o = 0x09512770u; o < 0x09512770u + 32u * 0xC0u; o += 0xC0u) {
        if (!RB(o) || !RB(o + 1) || RB(o + 2) != 14 || RB(o + 3) != 6) continue;
        if ((0xF0 - (int16_t)RH(o + 22) < 112) == top) return away_from_middle(o, dialogue_units());
    }
    return 0;
}

/* ---- The endings (core/task/ending.c) ----
   Each frame the ending's task steps (0x089DE2C0, or 0x089DE390 while it only draws) is noted, so the
   drawers can tell an ending's frame (its scenes, its dialogue, the staff credits) from the rest of the
   fight screen it shares. Once the staff credits are on the screen (a text asked for through the text
   queue, 0x0880A6CC, core/text/queue.c, above y 240: the credits are the ending's only such text, rising
   from the bottom) they roll to the ending's end. */
uint64_t rt_vblank_count(void);
static long ending_frame = -100;
static int ending_credits;
int widescreen_ending_frame(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    long now = (long)rt_vblank_count();
    if (!after) {
        if (now - ending_frame > 1) ending_credits = 0;   /* a new ending */
        ending_frame = now;
    }
    return 0;
}
/* a further frame of the same ending drawn inside one call (core/task/ending.c, 0x089DE390's loop) */
void widescreen_ending_more(void) { ending_frame = (long)rt_vblank_count(); }
int widescreen_in_ending(void) { return rt_widescreen && (long)rt_vblank_count() - ending_frame <= 1; }
int widescreen_ending_credits(void) { return widescreen_in_ending() && ending_credits; }
void widescreen_text_asked(int y) { if (y < 240 && widescreen_in_ending()) ending_credits = 1; }
