/* OPTION MODE > SCREEN SETTING gets two more settings between WALLPAPER and DEFAULT: VSYNC (ON / OFF)
 * and FRAME TIMING (AUTO / SMOOTH / ORIGINAL: pacing.c).
 *
 * The game's SCREEN SETTING (logic 0x088B85BC, drawing 0x088B78DC, its green bar 0x088BAA90) has four
 * rows in the option context's +7: SCREEN SIZE, WALLPAPER, DEFAULT, EXIT, 0x18 apart from y 0x40. Here
 * (0.60.0: centred on the screen, row_y, as seven rows no longer fit from 0x40)
 * the row in +7 is ours, 0-6: SCREEN SIZE, WALLPAPER, VSYNC, FRAME TIMING, FILTER, DEFAULT, EXIT (FILTER,
 * a shader over the finished picture: src/platform/shader.h, only while the shaders feature is on; 0-5 without it).
 *   - Up / down move over the six rows here, the way the game moves over its four (pressed: at once,
 *     repeat count 0x14; held: every 6 frames after that; sound 0x38).
 *   - VSYNC and FRAME TIMING change with left / right through the game's own value changer
 *     (0x088B7D08), so they sound and repeat like SCREEN SIZE. They apply at once (host_set_vsync;
 *     pacing.c checks every frame) and are kept in a3mx.cfg ([screen setting] vsync, frame_timing),
 *     written when SCREEN SETTING closes if either changed.
 *   - The other rows are the game's: each frame the game runs with its own row number in +7.
 *   - DEFAULT also sets VSYNC back to ON, FRAME TIMING to AUTO and FILTER to OFF.
 * The whole screen is drawn here (WIDE FORCED as the size's name while it is on: widescreen.c).
 *   - A row whose setting has no effect at the moment is drawn disabled, label and value, in text
 *     colour 9: light grey with no gradient. Font 1's colours are not tints: each is its own set of
 *     glyphs drawn into the font's texture (0x08812520 offsets the glyph by colour * 144), so a light
 *     grey with a dark grey outline needs a glyph set of our own (to come; colour 9's outline is brown).
 *     The cursor and the green bar still go onto it, but left / right do nothing. Today: VSYNC when the display can't wait for the monitor (host_vsync_available).
 *     FRAME TIMING always applies (0.49.0): where the monitor can't set the pace, SMOOTH runs on a 60.00 Hz timer.
 *     SMOOTH and ORIGINAL are always what they say; AUTO chooses one when this screen closes and when the
 *     MODE MENU opens (pacing.c, pacing_decide), not while a screen is up.
 *     Saved values are kept.
 *   - FRAME TIMING's values, left to right: ORIGINAL, SMOOTH, AUTO (rt_frame_timing 0, 1, 2). What the
 *     player actually gets is shown on the MODE MENU (padicons.c).
 *   - The green bar is as wide as the longest value any row can show (widescreen.c), on every row, so
 *     a longer name never runs past it.
 * With --no-extensions the game's own four rows are untouched: vsync stays ON, FRAME TIMING AUTO.
 */
#include "rt.h"
#include "host.h"
#include "ext_features.h"
#include "shader.h"
#include <string.h>
void inner_f_088B85BC(Cpu *c); void inner_f_088B78DC(Cpu *c); void inner_f_088BAA90(Cpu *c);   /* option_mode.c wraps them (decomp.h) */

void f_0880A6CC(Cpu *c);                    /* text: a0 string, a1 font, a2 x, a3 y, t0 colour */
void f_0887C4CC(Cpu *c);                    /* sound effect: a0 0, a1 id, a2 0xC0 */
void f_088B7D08(Cpu *c);                    /* a value, left / right: a0 pad slot, a1 value, a2 last value */
void settings_save(void);
int widescreen_value(void);                 /* widescreen.c: 0 NORMAL, 1 WIDE, 2 WIDE FORCED */
extern int rt_vsync, rt_frame_timing;

#define OPT        0x08E75912u              /* option context: +2 submenu state, +7 row */
#define OPTS       0x08A058A0u              /* the options block: +0x58 screen size, +0x1FE wallpaper */
#define SLOTS      0x08E75BA0u              /* per-slot input, 0x2C each: +4 held (repeating), +0xC new presses */
#define TIMER      0x08B02E0Cu              /* +0 frames since the screen opened (input from 5), +2 repeat count */
#define SIZE_NAMES 0x08A37A78u              /* NORMAL, WIDE, then the wallpapers' names NONE, 01 - 10 */
#define WALL_NAMES 0x08A37A80u
#define ONOFF      0x08A37AACu              /* OFF, ON */
#define S_TITLE    0x089FF8ECu              /* SCREEN SETTING */
#define S_SIZE     0x089FF8FCu
#define S_WALL     0x089FF908u
#define S_DEFAULT  0x089FF8A4u
#define S_EXIT     0x089FF8ACu
#define B_UP       0x8u
#define B_DOWN     0x4u
#define B_CONFIRM  0x8080u

/* The rows, by what they are. FILTER (shader.h) is there only while the shaders feature is on, so the
   row in the option context's + 7 is a position, 0 to rows() - 1, turned into one of these by id_of(). */
enum { R_SIZE, R_WALL, R_VSYNC, R_TIMING, R_FILTER, R_DEFAULT, R_EXIT, R_ROWS };
static const int game_row_of[R_ROWS] = { 0, 1, -1, -1, -1, 2, 3 };   /* ours to the game's (-1: ours) */
static const int our_id[4] = { R_SIZE, R_WALL, R_DEFAULT, R_EXIT };
static int rows(void) { return FEATURE(SHADERS) ? R_ROWS : R_ROWS - 1; }
static int id_of(int pos) { return !FEATURE(SHADERS) && pos >= R_FILTER ? pos + 1 : pos; }
static int pos_of(int id) { return !FEATURE(SHADERS) && id > R_FILTER ? id - 1 : id; }
static int game_row(int pos) { return game_row_of[id_of(pos)]; }
static int ours(int pos) { return game_row(pos) < 0; }
#define C_TEXT     0xA                      /* the game's colours: a row, the row with the cursor */
#define C_CURSOR   0xB
#define C_DISABLED 0x9                      /* light grey, no gradient (outline brown: see above) */
/* whether a row's setting can't take effect now (drawn grey; left / right ignored) */
int host_vsync_available(void);
static int disabled(int pos) {
    if (id_of(pos) == R_VSYNC) return !host_vsync_available();         /* the display can't sync */
    return 0;
}
/* Rows keep the game's spacing (0x18), and the block is centred between the title and the bottom of
   the screen (the 2D library's y; its 224 rows start at y 8: LESSONS L34), however many rows there are:
   7 with FILTER, 6 without. The game's own four started at 0x40, which put a seventh row on the bottom
   edge. 12: a row of font 1. */
#define ROWS_MID   0x76
static int row_y(int r) { return ROWS_MID - ((rows() - 1) * 0x18 + 12) / 2 + 0x18 * r; }
static uint32_t wf_name;                    /* "WIDE FORCED" in guest memory */
static int vsync_saved = -1, timing_saved, filter_saved;   /* VSYNC, FRAME TIMING and FILTER as last written to a3mx.cfg (-1: not seen yet) */
static int filter(void) { return rt_filter >= 0 && rt_filter < SHADER_FILTER_COUNT ? rt_filter : 0; }

static const char *const timing_names[3] = { "ORIGINAL", "SMOOTH", "AUTO" };   /* rt_frame_timing 0, 1, 2 */
static int timing(void) { return rt_frame_timing >= 0 && rt_frame_timing <= 2 ? rt_frame_timing : 2; }
static void sound(Cpu *c, int id) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = 0; c->r[5] = (uint32_t)id; c->r[6] = 0xC0; f_0887C4CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static void text(Cpu *c, uint32_t s, int font, int x, int y, int colour) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = s; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static void text_disabled(Cpu *c, uint32_t s, int font, int x, int y) { text(c, s, font, x, y, C_DISABLED); }
static void vsync_set(int on) {
    on = !!on;
    if (on == rt_vsync) return;
    rt_vsync = on;
    host_set_vsync(on);
}
/* on leaving: keep a changed VSYNC or FRAME TIMING in a3mx.cfg (the game's own settings are saved by its auto-save) */
static void left(void) {
    if (vsync_saved >= 0 && (vsync_saved != rt_vsync || timing_saved != rt_frame_timing || filter_saved != rt_filter)) {
        settings_save();
        rt_log("screen setting: vsync %s, frame timing %s, filter %s, saved", rt_vsync ? "ON" : "OFF", timing_names[timing()], SHADER_FILTERS[filter()].name);
    }
    vsync_saved = rt_vsync; timing_saved = rt_frame_timing; filter_saved = rt_filter;
    { void pacing_decide(const char *where); pacing_decide("leaving SCREEN SETTING"); }   /* AUTO chooses again from what is set now */
}

/* up / down over our rows, as the game moves over its four (0x088B85BC) */
static int move(Cpu *c, int r) {
    uint32_t pressed = RW(SLOTS + 0xC) | RW(SLOTS + 0x2C + 0xC), held = RW(SLOTS + 4) | RW(SLOTS + 0x2C + 4);
    int step = 0;
    if (pressed & (B_UP | B_DOWN)) {
        step = (pressed & B_UP) ? -1 : 1;
        WB(TIMER + 2, 0x14);
    } else if (held & (B_UP | B_DOWN)) {
        int n = (int8_t)RB(TIMER + 2) - 1;
        WB(TIMER + 2, (uint32_t)n);
        if (n >= 0) return r;
        step = (held & B_UP) ? -1 : 1;
        WB(TIMER + 2, 6);
    } else return -1;                       /* no up / down */
    sound(c, 0x38);
    return (r + step + rows()) % rows();
}

int screensetting_frame(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    if (vsync_saved < 0) { vsync_saved = rt_vsync; timing_saved = rt_frame_timing; filter_saved = rt_filter; }
    int state = (int8_t)RB(OPT + 2);
    if (state != 0) {                       /* 1 closing (after EXIT or back), others opening: the game's */
        if (state == 1) left();
        inner_f_088B85BC(c);
        return 1;
    }
    int r = (int8_t)RB(OPT + 7);
    if (r < 0 || r >= rows()) r = 0;
    int id = id_of(r);
    int8_t t = (int8_t)RB(TIMER);
    if (t + 1 < 5) { WB(OPT + 7, (uint32_t)(ours(r) ? 0 : game_row(r))); inner_f_088B85BC(c); WB(OPT + 7, (uint32_t)r); return 1; }   /* no input yet: the game counts */
    WB(TIMER, 5);
    int m = move(c, r);
    if (m >= 0) { WB(OPT + 7, (uint32_t)m); return 1; }
    uint32_t pressed = RW(SLOTS + 0xC) | RW(SLOTS + 0x2C + 0xC);
    if (ours(r)) {
        if (pressed & 0x100) {              /* back: as the game's (sound 0x2F; state 1 closes next frame) */
            sound(c, 0x2F);
            WB(OPT + 2, 1);
            return 1;
        }
        if (disabled(r)) return 1;            /* (highlighted, but not changed) */
        uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
        int v = id == R_VSYNC ? rt_vsync : id == R_TIMING ? timing() : filter();
        for (int k = 0; k < 2; k++) {
            c->r[4] = SLOTS + 0x2Cu * (uint32_t)k; c->r[5] = (uint32_t)v; c->r[6] = id == R_VSYNC ? 1 : id == R_TIMING ? 2 : (uint32_t)(SHADER_FILTER_COUNT - 1);
            f_088B7D08(c); v = (int)(int16_t)c->r[2];
        }
        memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
        if (id == R_VSYNC) vsync_set(v == 1);
        else if (id == R_TIMING) { if (v >= 0 && v <= 2 && v != rt_frame_timing) { rt_frame_timing = v; rt_log("screen setting: frame timing %s", timing_names[v]); } }
        else if (v >= 0 && v < SHADER_FILTER_COUNT && v != rt_filter) { rt_filter = v; rt_log("screen setting: filter %s", SHADER_FILTERS[v].name); }   /* (shader_gl.c sees it at the next frame) */
        return 1;
    }
    /* one of the game's rows: the game, with its own row number (no up / down this frame, so it stays) */
    WB(TIMER, 4);                           /* (it counts this frame's tick itself) */
    WB(OPT + 7, (uint32_t)game_row(r));
    inner_f_088B85BC(c);
    int g = (int8_t)RB(OPT + 7);
    WB(OPT + 7, (uint32_t)(g >= 0 && g < 4 ? pos_of(our_id[g]) : r));
    if (id == R_DEFAULT && (pressed & B_CONFIRM)) { vsync_set(1); rt_frame_timing = 2; rt_filter = 0; }
    return 1;
}

/* widescreen.c: the green bar's width, the same on every row: from its left end (x 0x60) past the end
   of the longest value any row can show, from x 0xDC in 8-pixel letters, with 10 to spare (222 for
   WIDE FORCED, the longest today). New values are counted here, so they never run past the bar. */
#define BAR_X     0x60
#define VALUE_X   0xDC
#define LETTER_W  8
int screensetting_bar_width(void) {
    static const char *const ours_v[] = { "WIDE FORCED", "AUTO", "SMOOTH", "ORIGINAL" };
    size_t longest = 0;
    for (size_t k = 0; k < sizeof ours_v / sizeof *ours_v; k++) if (strlen(ours_v[k]) > longest) longest = strlen(ours_v[k]);
    for (int k = 0; k < SHADER_FILTER_COUNT; k++) if (strlen(SHADER_FILTERS[k].name) > longest) longest = strlen(SHADER_FILTERS[k].name);
    const uint32_t tables[][2] = { { SIZE_NAMES, 2 }, { WALL_NAMES, 11 }, { ONOFF, 2 } };   /* the game's values */
    for (size_t t = 0; t < 3; t++)
        for (uint32_t k = 0; k < tables[t][1]; k++) {
            uint32_t p = RW(tables[t][0] + 4u * k);
            const char *str = p ? (const char *)HOST(p) : NULL;
            if (str && strlen(str) > longest && strlen(str) < 32) longest = strlen(str);
        }
    return VALUE_X - BAR_X + LETTER_W * (int)longest + 10;
}

int screensetting_text(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    if (!wf_name) { wf_name = rt_alloc(16, 1, 4, "wide forced"); if (wf_name) strcpy((char *)HOST(wf_name), "WIDE FORCED"); }
    int r = (int8_t)RB(OPT + 7);
    static const uint32_t names[R_ROWS] = { S_SIZE, S_WALL, 0, 0, 0, S_DEFAULT, S_EXIT };   /* by id; 0: ours */
    /* our words, 16 bytes each: VSYNC, FRAME TIMING, FILTER, then the values ORIGINAL, SMOOTH, AUTO, then every filter's name */
    enum { W_VSYNC, W_TIMING, W_FILTER, W_TIMINGS, W_FILTERS = W_TIMINGS + 3 };
    static uint32_t our_str;
    if (!our_str) {
        our_str = rt_alloc(16u * (uint32_t)(W_FILTERS + SHADER_FILTER_COUNT), 1, 4, "screen setting");
        if (our_str) { strcpy((char *)HOST(our_str + 16u * W_VSYNC), "VSYNC"); strcpy((char *)HOST(our_str + 16u * W_TIMING), "FRAME TIMING");
                       strcpy((char *)HOST(our_str + 16u * W_FILTER), "FILTER");
                       for (int k = 0; k < 3; k++) strcpy((char *)HOST(our_str + 16u * (uint32_t)(W_TIMINGS + k)), timing_names[k]);
                       for (int k = 0; k < SHADER_FILTER_COUNT; k++) { char *d = (char *)HOST(our_str + 16u * (uint32_t)(W_FILTERS + k)); strncpy(d, SHADER_FILTERS[k].name, 15); d[15] = 0; } }
    }
    text(c, S_TITLE, 2, 0x50, 5, 0xB);
    for (int i = 0; i < rows(); i++) {
        int id = id_of(i);
        uint32_t s = names[id] ? names[id] : our_str ? our_str + 16u * (uint32_t)(id == R_VSYNC ? W_VSYNC : id == R_TIMING ? W_TIMING : W_FILTER) : 0;
        if (s && disabled(i)) text_disabled(c, s, 1, 0x6C, row_y(i));
        else if (s) text(c, s, 1, 0x6C, row_y(i), i == r ? C_CURSOR : C_TEXT);
    }
    int size = widescreen_value();
    uint32_t sname = size == 2 && wf_name ? wf_name : RW(SIZE_NAMES + 4u * (uint32_t)(size == 1));
    text(c, sname, 1, 0xDC, row_y(pos_of(R_SIZE)), 0xB);
    text(c, RW(WALL_NAMES + 4u * (uint32_t)RB(OPTS + 0x1FE)), 1, 0xDC, row_y(pos_of(R_WALL)), 0xB);
    text(c, RW(ONOFF + 4u * (uint32_t)!!rt_vsync), 1, 0xDC, row_y(pos_of(R_VSYNC)), 0xB);
    if (our_str) text(c, our_str + 16u * (uint32_t)(W_TIMINGS + timing()), 1, 0xDC, row_y(pos_of(R_TIMING)), 0xB);
    if (our_str && FEATURE(SHADERS)) text(c, our_str + 16u * (uint32_t)(W_FILTERS + filter()), 1, 0xDC, row_y(R_FILTER), 0xB);
    return 1;
}

/* the green bar (object kind 0xC, type 0xA; y from the table 0x08A37DA8[row], 240 - the row's y): on our row */
int screensetting_bar(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    uint32_t o = c->r[4];
    int r = (int8_t)RB(OPT + 7);
    if (RB(o + 0x5C) != 0xA || r < 0 || r >= rows()) { inner_f_088BAA90(c); return 1; }
    WB(OPT + 7, (uint32_t)(ours(r) ? 0 : game_row(r)));
    inner_f_088BAA90(c);
    WB(OPT + 7, (uint32_t)r);
    if (RB(OPT + 6) && RB(OPT + 5) == 3) WH(o + 0xA, (uint32_t)(240 - row_y(r)));   /* (still SCREEN SETTING's) */
    return 1;
}
