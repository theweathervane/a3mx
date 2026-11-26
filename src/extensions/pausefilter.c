/* The fight's pause menu gets a FILTER row (0.166.0), with the shaders feature (shader.h): CONTINUE,
 * BUTTON CONFIG, FILTER, MODE MENU. The game's menu (0x08834AC8 once a frame while choosing; its record
 * a2: +0 state, +1 frames, +8 the lit row 0 - 2, +9 the row chosen; texts 0x08834FF0 from the table
 * 0x08A0F96C: text, font, x, y, 12 bytes, four entries, the fourth empty at y 152) knows three rows.
 * Here:
 *   - the row shown lit is ours, 0 - 3 (row): 2 is FILTER, 3 the game's MODE MENU (its row 2). The
 *     game's +8 is kept on its own rows (on FILTER: BUTTON CONFIG's), so whatever the game does with it
 *     is what it would do;
 *   - up / down are ours over the four rows (the game's sound), taken out of the pausing player's new
 *     presses while the game's menu runs, then put back;
 *   - on FILTER: left / right change the filter within its category, L / R turn to the previous / next
 *     category (its first filter, or the one in use); Cross does nothing there. The game is paused, so
 *     a heavy preset's build (a frame or more) is fine. Kept in a3mx.cfg at once. START still closes the
 *     menu, as on any row;
 *   - the menu's bar (0x088889A0, y from the halfwords at 0x08A261BC by +8: 135, 119, 103 and, past the
 *     game's three rows, 87): after the game placed it, at our row's;
 *   - the texts: while the game draws them, its table has FILTER as the third entry and MODE MENU
 *     moved to the fourth (y 152), and +8 our row so the right one is lit; both put back straight after,
 *     so the game's memory is as it was. On FILTER, two lines in the small font under the menu: the
 *     filter in use (CATEGORY: NAME) and what the buttons do.
 * Without the shaders feature (and in every parity test) none of this runs: the menu is the game's. */
#include "rt.h"
#include "ext_features.h"
#include "shader.h"
#include <string.h>
#include <stdio.h>

void orig_f_08834AC8(Cpu *c);
void f_0887C4CC(Cpu *c);                    /* sound effect: a0 channel, a1 id, a2 0xC0 */
void f_0880A6CC(Cpu *c);                    /* text: a0 string, a1 font, a2 x, a3 y, t0 colour */
void settings_save(void);

#define PADS       0x08E75BA0u              /* +12 new presses, 44 bytes a player */
#define TABLE      0x08A0F96Cu              /* the menu's rows: text, font, x, y (12 bytes) */
#define B_UP       0x8u
#define B_DOWN     0x4u
#define B_LEFT     0x2u
#define B_RIGHT    0x1u
#define B_CROSS    0x80u
#define B_L        0x40u
#define B_R        0x200u
enum { R_CONTINUE, R_CONFIG, R_FILTER, R_MODE_MENU, N_ROWS };
static const int game_row_of[N_ROWS] = { 0, 1, 1, 2 };
static const int our_row_of[3] = { R_CONTINUE, R_CONFIG, R_MODE_MENU };
static int row;
static long seen = -100;
static uint32_t buf;                         /* guest memory: the FILTER row's text, the hint line */

static int on(void) { return FEATURE(SHADERS) && FEATURE(MENUS); }
static void sound(Cpu *c, int id) {
    uint32_t keep[8], ra = c->r[31], v0 = c->r[2]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = 6; c->r[5] = (uint32_t)id; c->r[6] = 0xC0; f_0887C4CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra; c->r[2] = v0;
}
static void text(Cpu *c, uint32_t s, int font, int x, int y, int colour) {
    uint32_t keep[8], ra = c->r[31], v0 = c->r[2]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = s; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra; c->r[2] = v0;
}
static int filter(void) { return rt_filter >= 0 && rt_filter < filter_count() ? rt_filter : 0; }
static void set_filter(int f) {
    if (f == rt_filter) return;
    rt_filter = f;
    rt_log("pause menu: filter %s", filter_get(f)->name);
    settings_save();
}

/* the pause menu, choosing (a0 the game, a2 the record) */
void hook_f_08834AC8(Cpu *c) {
    if (!on()) { orig_f_08834AC8(c); return; }
    uint32_t g = c->r[4], rec = c->r[6];
    long now = (long)rt_vblank_count();
    int game_row = RB(rec + 8) <= 2 ? RB(rec + 8) : 0;
    if (now - seen > 2 || row != R_FILTER) row = our_row_of[game_row];   /* opened, or back from BUTTON CONFIG */
    seen = now;
    if (!RB(rec + 1)) { orig_f_08834AC8(c); return; }               /* (its first frame: no input) */
    int pl = 0;
    while (pl < 3 && !((int8_t)RB(g + 89) & (1 << pl))) pl++;
    uint32_t pad = PADS + 44u * (uint32_t)(pl < 3 ? pl : 0), pressed = RW(pad + 12), taken = 0;
    if (!(pressed & 0x8000)) {                                       /* (START closes the menu, as the game's) */
        if (pressed & (B_UP | B_DOWN)) {
            row = (row + ((pressed & B_UP) ? N_ROWS - 1 : 1)) % N_ROWS;
            sound(c, 0x38);
            taken = pressed & (B_UP | B_DOWN | B_LEFT | B_RIGHT | B_CROSS);
        } else if (row == R_FILTER) {
            taken = pressed & (B_LEFT | B_RIGHT | B_CROSS | B_L | B_R);
            const FilterEntry *e = filter_get(filter());
            if (pressed & (B_LEFT | B_RIGHT)) {
                int first = filter_category_first(e->category), n = filter_category_size(e->category);
                set_filter(first + (filter() - first + ((pressed & B_RIGHT) ? 1 : n - 1)) % n);
                sound(c, 0x38);
            } else if (pressed & (B_L | B_R)) {
                int k = filter_categories(), cat = (e->category + ((pressed & B_R) ? 1 : k - 1)) % k;
                set_filter(filter_category_first(cat));
                sound(c, 0x38);
            }
        }
    }
    WB(rec + 8, (uint32_t)game_row_of[row]);
    WW(pad + 12, pressed & ~taken);
    orig_f_08834AC8(c);
    WW(pad + 12, RW(pad + 12) | (pressed & taken));
}

/* its bar (0x088889A0, a0 the bar): after, on our row */
int screensetting_pause_cursor(Cpu *c, uint32_t o, int after) {
    (void)c;
    if (!after || !on() || (long)rt_vblank_count() - seen > 2 || RB(0x0950FF40u) != 1 || !RB(o + 93)) return 0;
    WH(o + 10, RH(0x08A261BCu + 2u * (uint32_t)row));
    return 0;
}

/* its texts (0x08834FF0: a0 the game, a2 the record, a3 the question asked): before, the table and +8
   as we show them; after, put back, and the hint on FILTER */
static uint8_t kept_table[24];
static uint32_t kept_row, kept_rec;         /* (a2 is not kept by the call: the record is kept here) */
static int patched;
int screensetting_pause_texts(Cpu *c, uint32_t a0, int after) {
    if (!on()) return 0;
    uint32_t rec = after ? kept_rec : c->r[6];
    if (!after) {
        patched = 0;
        if (c->r[7] != 0 || (long)rt_vblank_count() - seen > 2) return 0;          /* the question, or not our menu */
        if (!buf && !(buf = rt_alloc(0x100, 1, 16, "pause filter"))) return 0;
        if (RW(TABLE + 36) != 0) return 0;                                        /* (not the table we know) */
        strcpy((char *)HOST(buf), "FILTER");
        memcpy(kept_table, HOST(TABLE + 24), 24);
        kept_row = RB(rec + 8); kept_rec = rec;
        memcpy(HOST(TABLE + 36), HOST(TABLE + 24), 12);                           /* MODE MENU to the fourth row (y 152) */
        WH(TABLE + 36 + 8, 152);
        WW(TABLE + 24, buf); WH(TABLE + 24 + 6, 150);                             /* FILTER: centred as the others (6 a letter) */
        WB(rec + 8, (uint32_t)row);
        patched = 1;
        return 0;
    }
    if (!patched) return 0;
    memcpy(HOST(TABLE + 24), kept_table, 24);
    WB(rec + 8, kept_row);
    patched = 0;
    if (row == R_FILTER) {                    /* which: under the menu, in the small font */
        const FilterEntry *e = filter_get(filter());
        char h[2][48];
        snprintf(h[0], sizeof h[0], "%s: %s", filter_category_name(e->category), e->name);
        if (strlen(h[0]) > 44) { h[0][43] = '.'; h[0][44] = 0; }
        snprintf(h[1], sizeof h[1], "LEFT RIGHT - FILTER   L R - CATEGORY");
        for (int k = 0; k < 2; k++) {
            strcpy((char *)HOST(buf + 0x20 + 0x30u * (uint32_t)k), h[k]);
            text(c, buf + 0x20 + 0x30u * (uint32_t)k, 0, 192 - 4 * (int)strlen(h[k]), 172 + 10 * k, k ? 9 : 0xB);
        }
    }
    return 0;
}
