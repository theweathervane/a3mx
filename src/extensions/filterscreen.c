/* SCREEN SETTING > FILTER's own screen (0.166.0). Cross on the FILTER row of SCREEN SETTING
 * (screensetting.c) opens it in place of SCREEN SETTING's rows, laid out like EX. SETTING's pages
 * (exsetting.c): the banner FILTER, then one category of filters (shader.h, shader_list.c) at a time:
 *   - the category's name, with EX. SETTING's side arrows (exsetting_other_arrows) at the screen's
 *     sides on its line, and its number (3/29) where EX. SETTING has its page number; L / R, or left /
 *     right, turn to the previous / next category;
 *   - a little below it, its filters, nine rows at a time, scrolling with up / down (held: repeating);
 *     the one in use is marked with a > before it; a name too long for the row is cut, ending in '.', except the cursor's, which scrolls (a marquee), as does a description too long for its line;
 *   - a line's gap under them, then SCORE RANKING's up and down arrows, side by side in the middle: up
 *     while there are rows above the ones shown, down while there are rows below;
 *   - under them, in the small font: what the filter is (a favourite's description, else its preset's
 *     path), and HEAVY when the catalogue says it wants a real GPU; or why it can't be shown;
 *   - SCREEN SETTING's green bar under the cursor's row, as wide as the rows.
 * Cross or START chooses the filter under the cursor: it applies at once and SCREEN SETTING is back,
 * its FILTER row showing the choice (leaving SCREEN SETTING keeps it in a3mx.cfg). Circle goes back
 * without changing it: the filter in use when the screen opened stays. (The preview fight is SCREEN
 * SETTING's own PREVIEW row: filterpreview.c.)
 * A filter whose preset file isn't there is drawn in EX. SETTING's greyed colours and can't be chosen.
 * Nothing here touches the game's memory but the screen's own row (+7, kept) and its text and bar.
 *
 * The fight's pause menu opens the same screen on its FILTER row (pausefilter.c, filterscreen_open_pause):
 * the same rows and keys, read from the pausing player's pad only, the pause's sound channel, over the
 * paused fight (the pause menu's own see-through panel, made bigger, behind it, and its bar on the
 * cursor's row: pausefilter.c places them by filterscreen_bar_y); Cross / Circle go back to the pause
 * menu. The side and scroll arrows there are the game's button symbols (font 5: left / right, up /
 * down), as OPTION MODE's objects aren't there in a fight. */
#include "rt.h"
#include "ext_features.h"
#include "shader.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void f_0880A6CC(Cpu *c);                    /* text: a0 string, a1 font, a2 x, a3 y, t0 colour */
void f_0887C4CC(Cpu *c);                    /* sound effect: a0 0, a1 id, a2 0xC0 */
int exsetting_grey_font(void);
int shader_gl_ready(void);                  /* shader_gl.c: librashader and the shaders are there */

#define SLOTS      0x08E75BA0u              /* per-slot input, 0x2C each: +4 held, +0xC new presses */
#define B_UP       0x8u
#define B_DOWN     0x4u
#define B_LEFT     0x2u
#define B_RIGHT    0x1u
#define B_CONFIRM  0x8080u
#define B_BACK     0x100u
#define B_L        0x40u
#define B_R        0x200u
#define VISIBLE    9                        /* rows shown */
#define LIST_X     0x30
#define MARK_X     0x24
#define NAME_LEN   38                       /* letters of a name on a row (font 1: 8 wide, to x 0x160) */
#define WHAT_Y     0xCE                     /* the description, under the scroll arrows */
#define WHAT_LEN   41                       /* letters of the description (font 0: 8 wide, to x 0x178) */
#define PAGE_Y     0xBC                     /* the category's number, on the scroll arrows' line */
#define CAT_Y      0x1C                     /* EX. SETTING's first row: the category */
static int row_y(int i) { return CAT_Y + 14 + 6 + 14 * i; }   /* the list, 6 more below the category than EX. SETTING's rows */
#define SCROLL_Y   (row_y(VISIBLE) + 14 + 2)  /* the scroll arrows' tops: a line's gap under the last row */
#define SCROLL_X   0xC0                     /* the middle of the 4:3 picture */
void exsetting_other_arrows(Cpu *c, int y, int x, int up_y, int down_y);

static unsigned tick;                        /* frames the cursor's filter has been shown (its marquees) */
static int on, cat, cursor, top, repeat, entry_filter;
static int in_pause;                         /* opened from the pause menu: its pad, its sound channel */
static uint32_t pause_pad;                   /* (the pausing player's input record) */   /* entry_filter: in use when the screen opened */
static uint32_t buf;                         /* our strings in guest memory, written each frame */
#define SLOT(i)    (buf + 64u * (uint32_t)(i))
enum { T_TITLE, T_CAT, T_PAGE, T_WHAT1, T_WHAT2, T_MARK, T_ROWS, T_COUNT = T_ROWS + VISIBLE };
static int grey_ok;
static signed char *present;                 /* per filter: 0 not looked, 1 there, 2 missing */
static int present_n;

static void sound(Cpu *c, int id) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = in_pause ? 6 : 0; c->r[5] = (uint32_t)id; c->r[6] = 0xC0; f_0887C4CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static void text(Cpu *c, uint32_t s, int font, int x, int y, int colour) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = s; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
void f_0880A7B0(Cpu *c);                    /* button symbol a0 (font 5: 0 Circle, 1 Triangle, 2 Square, 3 Cross, 4 down, 5 up, 6 left, 7 right, 8 L, 9 R) at a1, a2 */
#define SYM_UP     5
#define SYM_DOWN   4
#define SYM_LEFT   6
#define SYM_RIGHT  7
static void symbol(Cpu *c, int id, int x, int y) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = (uint32_t)id; c->r[5] = (uint32_t)x; c->r[6] = (uint32_t)y; f_0880A7B0(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
/* S into slot I, in capitals, at most N letters (cut ending in '.') */
static uint32_t put(int i, const char *s, int n) {
    char *d = (char *)HOST(SLOT(i));
    int l = 0;
    for (; s[l] && l < 63; l++) d[l] = s[l] >= 'a' && s[l] <= 'z' ? (char)(s[l] - 32) : s[l];
    d[l] = 0;
    if (l > n) { while (n > 2 && d[n - 2] == ' ') n--; d[n - 1] = '.'; d[n] = 0; }
    return SLOT(i);
}
/* S into slot I as a marquee, N letters of it at a time: when it is longer, the window waits MARQUEE_WAIT
   frames at the start, moves on a letter every MARQUEE_STEP frames to the end, waits there, and starts
   over. T: frames since the text was first shown. */
#define MARQUEE_WAIT 60
#define MARQUEE_STEP 8
static uint32_t put_marquee(int i, const char *s, int n, unsigned t) {
    int l = (int)strlen(s);
    if (l <= n) return put(i, s, n);
    int moves = l - n, cycle = 2 * MARQUEE_WAIT + moves * MARQUEE_STEP;
    int at = (int)(t % (unsigned)cycle) - MARQUEE_WAIT;
    at = at <= 0 ? 0 : at / MARQUEE_STEP > moves ? moves : at / MARQUEE_STEP;
    char w[64]; int k = 0;
    for (; k < n && k < 63 && s[at + k]; k++) w[k] = s[at + k];
    w[k] = 0;
    return put(i, w, n);
}

int filterscreen_on(void) { return on && !in_pause; }
int filterscreen_in_pause(void) { return on && in_pause; }
static int filter(void) { return rt_filter >= 0 && rt_filter < filter_count() ? rt_filter : 0; }
static int there(int f) {                    /* the preset file is there (OFF always) */
    if (f <= 0) return 1;
    if (present_n != filter_count()) { free(present); present_n = filter_count(); present = (signed char *)calloc((size_t)present_n, 1); }
    if (!present) return 1;
    if (!present[f]) {
        const FilterEntry *e = filter_get(f);
        char p[2200];
        if (e->custom) snprintf(p, sizeof p, "%s", e->path); else snprintf(p, sizeof p, "%s/%s", filters_shaders_dir(), e->path);
        FILE *fp = fopen(p, "rb");
        present[f] = fp ? 1 : 2;
        if (fp) fclose(fp);
    }
    return present[f] == 1;
}
static void show(int f) {                    /* the cursor on F, scrolled into view */
    cursor = f;
    int first = filter_category_first(cat), n = filter_category_size(cat), at = f - first;
    if (at < top) top = at;
    if (at >= top + VISIBLE) top = at - VISIBLE + 1;
    if (top > n - VISIBLE) top = n - VISIBLE;
    if (top < 0) top = 0;
}
static void turn(int step) {
    int n = filter_categories();
    cat = (cat + step + n) % n;
    top = 0;
    const FilterEntry *e = filter_get(filter());
    show(e && e->category == cat ? filter() : filter_category_first(cat));   /* on the one in use, if it is here */
}

static void open_screen(Cpu *c);
void filterscreen_open(Cpu *c) { in_pause = 0; open_screen(c); }
void filterscreen_open_pause(Cpu *c, uint32_t pad) { in_pause = 1; pause_pad = pad; open_screen(c); }
static void open_screen(Cpu *c) {
    if (!buf && !(buf = rt_alloc(64u * T_COUNT, 1, 16, "filter screen"))) return;
    const FilterEntry *e = filter_get(filter());
    on = 1; cat = e ? e->category : 0; top = 0; repeat = 0; entry_filter = rt_filter;
    show(filter());
    sound(c, 0x39);
    rt_log("filter screen: open (%d filters, %d categories)", filter_count(), filter_categories());
}
void filterscreen_close(Cpu *c, int snd) {
    on = 0;
    if (c) { if (!in_pause) exsetting_other_arrows(c, -1, 0, -1, -1); sound(c, snd); }
}

/* once a frame while open, in place of SCREEN SETTING's input (screensetting.c) */
void filterscreen_frame(Cpu *c) {
    uint32_t p = in_pause ? RW(pause_pad + 0xC) : RW(SLOTS + 0xC) | RW(SLOTS + 0x2C + 0xC);
    uint32_t held = in_pause ? RW(pause_pad + 4) : RW(SLOTS + 4) | RW(SLOTS + 0x2C + 4);
    int first = filter_category_first(cat), n = filter_category_size(cat), at = cursor - first;
    int step = 0;
    if (p & (B_UP | B_DOWN)) { step = (p & B_UP) ? -1 : 1; repeat = 0x14; }
    else if (held & (B_UP | B_DOWN)) { if (--repeat < 0) { step = (held & B_UP) ? -1 : 1; repeat = 4; } }
    if (step) {
        if (step < 0 && at == 0 && !(p & B_UP)) step = 0;                  /* held: stops at the ends */
        else if (step > 0 && at == n - 1 && !(p & B_DOWN)) step = 0;
        if (step) { show(first + (at + step + n) % n); sound(c, 0x38); }
        return;
    }
    if (p & (B_L | B_LEFT)) { turn(-1); sound(c, 0x38); return; }
    if (p & (B_R | B_RIGHT)) { turn(1); sound(c, 0x38); return; }
    if (p & B_CONFIRM) {                     /* chosen: in use at once, back to SCREEN SETTING */
        if (!there(cursor)) { sound(c, 0x2F); return; }
        if (cursor != rt_filter) { rt_filter = cursor; rt_log("filter screen: filter %s", filter_get(cursor)->name); if (in_pause) { void settings_save(void); settings_save(); } }   /* (SCREEN SETTING saves when it closes) */
        filterscreen_close(c, 0x39);
        return;
    }
    if (p & B_BACK) {                        /* cancelled: the filter in use when it opened */
        if (rt_filter != entry_filter) { rt_filter = entry_filter; rt_log("screen setting: filter back to %s", filter_get(filter())->name); }
        filterscreen_close(c, 0x2F);
        return;
    }
}

void filterscreen_text(Cpu *c) {
    if (!buf) return;
    grey_ok = exsetting_grey_font();
    static int shown_cursor = -1;                 /* the marquees start over on another filter */
    if (cursor != shown_cursor) { shown_cursor = cursor; tick = 0; } else tick++;
    const char *title = "FILTER";
    text(c, put(T_TITLE, title, 20), 2, 0xC0 - 8 * (int)strlen(title), 5, 0xB);
    /* the category between EX. SETTING's side arrows; the scroll arrows when there are more rows */
    const char *cn = filter_category_name(cat);
    text(c, put(T_CAT, cn, 30), 1, 0xC0 - 4 * (int)strlen(cn), CAT_Y, 0xB);
    int first = filter_category_first(cat), n = filter_category_size(cat);
    if (!in_pause) exsetting_other_arrows(c, filter_categories() > 1 ? CAT_Y : -1, SCROLL_X, top > 0 ? SCROLL_Y : -1, top + VISIBLE < n ? SCROLL_Y : -1);
    else {                                        /* the game's symbols: left / right, up / down */
        if (filter_categories() > 1) { symbol(c, SYM_LEFT, LIST_X - 0x10, CAT_Y); symbol(c, SYM_RIGHT, 0x158, CAT_Y); }
        if (top > 0) symbol(c, SYM_UP, SCROLL_X - 0x14, SCROLL_Y);
        if (top + VISIBLE < n) symbol(c, SYM_DOWN, SCROLL_X + 0x4, SCROLL_Y);
    }
    { char s[16]; snprintf(s, sizeof s, "%d/%d", cat + 1, filter_categories()); text(c, put(T_PAGE, s, 8), 2, 0x178 - 16 * (int)strlen(s), PAGE_Y, 9); }   /* (font 2: 16 wide, as EX. SETTING's) */
    for (int i = 0; i < VISIBLE && top + i < n; i++) {
        int f = first + top + i, y = row_y(i), cur = f == cursor;
        const FilterEntry *e = filter_get(f);
        int colour = there(f) ? (cur ? 0xB : 0xA) : grey_ok ? (cur ? 15 : 14) : 9;
        text(c, cur ? put_marquee(T_ROWS + i, e->name, NAME_LEN, tick) : put(T_ROWS + i, e->name, NAME_LEN), 1, LIST_X, y, colour);   /* the cursor's: all of it, scrolling */
        if (f == filter()) text(c, put(T_MARK, ">", 2), 1, MARK_X, y, 0xB);
    }
    /* what it is */
    const FilterEntry *e = filter_get(cursor);
    char w1[200] = "", w2[200] = "";
    if (cursor < SHADER_BUILTIN_COUNT) snprintf(w1, sizeof w1, "%s", SHADER_FILTERS[cursor].what);
    else if (e->custom) { const char *b = e->path; for (const char *s = e->path; *s; s++) if (*s == '/' || *s == '\\') b = s + 1; snprintf(w1, sizeof w1, "YOURS: %s", b); }
    else snprintf(w1, sizeof w1, "%s", e->path);
    if (!there(cursor)) snprintf(w2, sizeof w2, "ITS PRESET IS MISSING.");
    else if (cursor > 0 && !shader_gl_ready()) snprintf(w2, sizeof w2, "NO LIBRASHADER: NOT SHOWN.");
    else if (e->heavy) snprintf(w2, sizeof w2, "HEAVY - WANTS A STRONG GPU.");
    text(c, put_marquee(T_WHAT1, w1, WHAT_LEN, tick), 0, LIST_X, WHAT_Y, 9);
    if (w2[0]) text(c, put(T_WHAT2, w2, WHAT_LEN), 0, LIST_X, WHAT_Y + 10, 9);
}

/* the text y of the cursor's row (pausefilter.c: the pause menu's bar there) */
int filterscreen_bar_y(void) { return row_y(cursor - filter_category_first(cat) - top); }
int filterscreen_bar_x(void) { return LIST_X - 0xC; }
int filterscreen_bar_w(void) { return 8 * NAME_LEN + 0x18; }

/* SCREEN SETTING's green bar (object kind 0xC type 0xA), after the game placed it: on our row, wide */
#define BAR_X  (LIST_X - 0xC)
#define BAR_W  (8 * NAME_LEN + 0x18)
static int16_t kept_x = -1, kept_w;
void filterscreen_bar(uint32_t o) {
    if (on) {
        if (kept_x < 0) { kept_x = (int16_t)RH(o + 6); kept_w = (int16_t)RH(o + 0xE); }
        int at = cursor - filter_category_first(cat) - top;
        WH(o + 0xA, (uint32_t)(240 - row_y(at)));
        WH(o + 6, BAR_X); WH(o + 0xE, BAR_W);
    } else if (kept_x >= 0) { WH(o + 6, (uint32_t)kept_x); WH(o + 0xE, (uint32_t)kept_w); kept_x = -1; }
}
