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
 *   - on FILTER: Cross opens the FILTER screen (filterscreen.c, filterscreen_open_pause) over the paused
 *     fight, run here in place of the menu's input with every new press of the pausing player taken
 *     out (the game's menu sees none), and drawn in place of the menu's texts; Cross there chooses a
 *     filter (in use at once, kept in a3mx.cfg) and Circle cancels, both back to the menu on FILTER.
 *     The game is paused, so a heavy preset's build (a frame or more) is fine. START still closes the
 *     menu on the menu's rows;
 *   - the menu's bar (0x088889A0: kind 3 type 1, +6 x, +10 y from the halfwords at 0x08A261BC by +8:
 *     135, 119, 103 and, past the game's three rows, 87; +14 width, +93 shown): after the game placed
 *     it, at our row's, once it has started (+129: the panel has opened; until then the game leaves it
 *     where it is not seen); on the FILTER screen on the cursor's row, as wide as the rows;
 *   - the menu's panel (0x088888E8: kind 3 type 0, +6 x 112, +10 y 160, growing to +14 160 wide, +26 80
 *     high, half its width as it grows): a row (16) higher, so FILTER and MODE MENU are in it; on the
 *     FILTER screen as big as the screen's rows, the fight showing through it as through the menu;
 *   - the texts: while the game draws them, its table has FILTER as the third entry and MODE MENU
 *     moved to the fourth (y 152), and +8 our row so the right one is lit; both put back straight after,
 *     so the game's memory is as it was. On the FILTER screen, its texts instead.
 * Whoever paused (1P, 2P, 3P: the game's +89 bits; the menu is the same for each) uses it.
 * Without the shaders feature (and in every parity test) none of this runs: the menu is the game's. */
#include "rt.h"
#include "ext_features.h"
#include "shader.h"
#include <string.h>
#include <stdio.h>

void f_0887C4CC(Cpu *c);                    /* sound effect: a0 channel, a1 id, a2 0xC0 */
void settings_save(void);
int filterscreen_in_pause(void); void filterscreen_open_pause(Cpu *c, uint32_t pad); void filterscreen_frame(Cpu *c);
void filterscreen_text(Cpu *c); int filterscreen_bar_y(void); int filterscreen_bar_x(void); int filterscreen_bar_w(void);

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
/* the pause menu, choosing (a0 the game, a2 the record): wraps the decompiled 0x08834AC8
   (core/object/batch_moves201.c, menu_options_row; a hook until 0.165.0 decompiled it). Before it: our
   row, the game's +8, and the presses we take out; after it: those presses put back. */
static uint32_t taken_pad, taken_bits;
int pausefilter_options_row(Cpu *c, uint32_t a0, int after) {
    if (after) {
        if (taken_pad) WW(taken_pad, RW(taken_pad) | taken_bits);
        taken_pad = taken_bits = 0;
        return 0;
    }
    taken_pad = taken_bits = 0;
    if (!on()) return 0;
    uint32_t g = a0, rec = c->r[6];
    long now = (long)rt_vblank_count();
    int game_row = RB(rec + 8) <= 2 ? RB(rec + 8) : 0;
    if (now - seen > 2 || row != R_FILTER) row = our_row_of[game_row];   /* opened, or back from BUTTON CONFIG */
    seen = now;
    if (!RB(rec + 1)) return 0;                                      /* (its first frame: no input) */
    int pl = 0;
    while (pl < 3 && !((int8_t)RB(g + 89) & (1 << pl))) pl++;
    uint32_t pad = PADS + 44u * (uint32_t)(pl < 3 ? pl : 0), pressed = RW(pad + 12), taken = 0;
    if (filterscreen_in_pause()) {                                   /* the FILTER screen: all of the input */
        filterscreen_frame(c);
        taken = pressed;
    } else if (!(pressed & 0x8000)) {                                /* (START closes the menu, as the game's) */
        if (pressed & (B_UP | B_DOWN)) {
            row = (row + ((pressed & B_UP) ? N_ROWS - 1 : 1)) % N_ROWS;
            sound(c, 0x38);
            taken = pressed & (B_UP | B_DOWN | B_LEFT | B_RIGHT | B_CROSS);
        } else if (row == R_FILTER) {
            taken = pressed & (B_LEFT | B_RIGHT | B_CROSS | B_L | B_R);
            if (pressed & B_CROSS) filterscreen_open_pause(c, pad);
        }
    }
    WB(rec + 8, (uint32_t)game_row_of[row]);
    WW(pad + 12, pressed & ~taken);
    taken_pad = pad + 12; taken_bits = pressed & taken;              /* put back after the game's menu (after) */
    return 0;
}

/* its bar (0x088889A0, a0 the bar): after, on our row, once the menu has opened (the game's own +129);
   on the FILTER screen on its cursor's row, as wide */
#define BAR_X  112
#define BAR_W  160
int screensetting_pause_cursor(Cpu *c, uint32_t o, int after) {
    (void)c;
    if (!after || !on() || (long)rt_vblank_count() - seen > 2 || RB(0x0950FF40u) != 1 || !RB(o + 93) || !RB(o + 129)) return 0;
    if (filterscreen_in_pause()) {
        WH(o + 6, (uint32_t)filterscreen_bar_x()); WH(o + 14, (uint32_t)filterscreen_bar_w());
        WH(o + 10, (uint32_t)(240 - filterscreen_bar_y()));
        return 0;
    }
    WH(o + 6, BAR_X); WH(o + 14, BAR_W);
    WH(o + 10, RH(0x08A261BCu + 2u * (uint32_t)row));
    return 0;
}

/* its panel (0x088888E8, a0 the panel), after: a row higher (as it grows, half its width plus that
   much: the game's is half its width); on the FILTER screen, behind all of that screen */
#define PANEL_Y   160                       /* +10: 240 - its top (y 80) */
#define PANEL_ROW 16
#define BIG_X     0x18                      /* the FILTER screen: from its banner down to its last line */
#define BIG_W     (384 - 2 * BIG_X)
#define BIG_TOP   0x18
#define BIG_H     (0xE0 - BIG_TOP)
int pausefilter_panel(Cpu *c, uint32_t o, int after) {
    (void)c;
    if (!after || !on() || (long)rt_vblank_count() - seen > 2 || RB(0x0950FF40u) != 1) return 0;
    if (filterscreen_in_pause()) {
        WH(o + 6, BIG_X); WH(o + 14, BIG_W); WH(o + 10, 240 - BIG_TOP); WH(o + 26, BIG_H);
        return 0;
    }
    int w = (int16_t)RH(o + 14);
    if ((int16_t)RH(o + 6) != BAR_X && (int16_t)RH(o + 6) == BIG_X) { WH(o + 6, BAR_X); WH(o + 10, PANEL_Y); w = BAR_W; WH(o + 14, BAR_W); }   /* back from the FILTER screen */
    WH(o + 26, (uint32_t)(w / 2 + PANEL_ROW * w / BAR_W));
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
        if (filterscreen_in_pause()) { filterscreen_text(c); return 1; }           /* the FILTER screen, not the menu's texts */
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
    return 0;
}
