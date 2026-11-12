/* The game's save data, made friendlier: a save is always there, AUTO SAVE is always on and
 * everything is unlocked.
 *
 * Saves: STFALPH3.SYS (system data, 0x3600 bytes) and STFALPH3.WTR (World Tour, 0x1A00 bytes), kept in
 * a3mx.dat / a3mx.cfg (savestore.c, settings.c), not on the memory stick. Both carry 0x3399 at +0x280; there is no
 * checksum. Loading (0x08816FC4) copies the file in pieces into the game's working copy:
 *
 *   SYS  +0x284  0x208   -> 0x08A058A0   options and unlocks (below)
 *        +0x48C  0x17C0  -> 0x08A05AA8
 *        +0x1C4C 0x3C0   -> 0x08A07268
 *        +0x200C 0x140   -> 0x08A07628
 *        +0x214C 0x13F8  -> 0x09519588
 *        +0x3544 4       -> 0x08A05894
 *   WTR  +0x284  0x174C  -> 0x09517E38
 *
 * With no save the game says "No SYSTEM DATA found" and asks whether to start without
 * auto-loading. Instead the load is answered with a save made from those same places (the game's
 * defaults at that point, so loading it changes nothing) with MODE SELECT on, and it is kept.
 *
 * In 0x08A058A0 (system data):
 *   +0x4F byte  AUTO SAVE (OPTION > SAVE LOAD); 0x08A077CC is the session's copy, which the game
 *               clears when the memory stick is missing
 *   +0x50 byte  bits 0-2: cleared in the variants started with L + R (game bits 0x240) held on the MODE MENU
 *   +0x52 half  unlocks, set by 0x0882AE7C when a mode is cleared (bits 0x10-0x200 per level of one
 *               mode, 0x1000 / 0x3000 / 0x7000 by score, 0x8000, ...); bit 0x0001 and 0x4000 gate the
 *               MODE MENU variants (0x0888CEC4) and the select screens' extra choices (0x08897434)
 *   +0x68 word  game time in frames
 *   +0x200 word ISM-Pluses earned, one bit each in bits 0-27 (26 of them; bit 0 Alpha Combo, the last
 *               two Multiple Taunt and Air Chain Combo); set by 0x0883B850 and on, the mode rewards at
 *               0x0883B884 give bits 15-27. EDIT MODE lists the ones not earned as ???
 *   +0x204 byte GAME OPTION > MODE SELECT (NORMAL / MAZI / SAIKYO / CLASSIC on the select screens)
 */
#include "rt.h"
#include "ext_features.h"
#include <string.h>
#include <stdio.h>

#define SYS_OPTS   0x08A058A0u
#define AUTOSAVE   (SYS_OPTS + 0x4F)
#define AUTOSAVE_S 0x08A077CCu
#define CLEARED    (SYS_OPTS + 0x50)
#define UNLOCKS    (SYS_OPTS + 0x52)
#define ISM_PLUS   (SYS_OPTS + 0x200)          /* ISM-Pluses earned, one bit each */
#define ALL_ISM_PLUS 0x0FFFFFFFu                /* bits 0-27: the mode rewards (0x0883B884) use up to bit 27 */
#define MAGIC      0x3399u
#define SYS_MODE_SELECT 0x488u              /* in the file; 0x08A05AA4 in memory: GAME OPTION > MODE SELECT (1 on) */

typedef struct { uint32_t off, len, mem; } Piece;
static const Piece sys_map[] = {
    { 0x284, 0x208, 0x08A058A0u }, { 0x48C, 0x17C0, 0x08A05AA8u }, { 0x1C4C, 0x3C0, 0x08A07268u },
    { 0x200C, 0x140, 0x08A07628u }, { 0x214C, 0x13F8, 0x09519588u }, { 0x3544, 4, 0x08A05894u },
};
static const Piece wtr_map[] = { { 0x284, 0x174C, 0x09517E38u } };

static long loaded_at = -1;                 /* frame the system data was last loaded */

int sysdata_quiet;                          /* savestore.c: only keeping a copy of the defaults */
/* A default save for FILE in guest memory at BUF (room CAP). Returns its size, or -1 (not ours). */
long sysdata_default(const char *file, uint32_t buf, uint32_t cap) {
    const Piece *m; size_t n; uint32_t size;
    if (!strcmp(file, "STFALPH3.SYS")) { m = sys_map; n = sizeof sys_map / sizeof *m; size = 0x3600; }
    else if (!strcmp(file, "STFALPH3.WTR")) { m = wtr_map; n = sizeof wtr_map / sizeof *m; size = 0x1A00; }
    else return -1;
    if (cap < size || buf < USER_LO || buf + size > USER_HI) return -1;
    memset(HOST(buf), 0, size);
    WW(buf + 0x280, MAGIC);
    for (size_t i = 0; i < n; i++) memcpy(HOST(buf + m[i].off), HOST(m[i].mem), m[i].len);
    if (m == sys_map) WB(buf + SYS_MODE_SELECT, 1);   /* our defaults: GAME OPTION > MODE SELECT on */
    if (!sysdata_quiet) rt_log("sysdata: no %s yet, made one from the defaults", file);
    return size;
}

static int boot_loads = 0;                  /* the first system data load is the boot screen's */
void sysdata_loaded(const char *file) {
    if (strcmp(file, "STFALPH3.SYS")) return;
    loaded_at = (long)rt_vblank_count();
    if (boot_loads < 1) boot_loads = 1;
}

/* once a frame: from a moment after the system data is in, keep AUTO SAVE on and everything unlocked */
void rt_hold_display(int frames);          /* main.c */
void rt_hold_display_set(int frames);
void rt_hold_display_paced(int frames);    /* main.c: the same, in real time */
#define SAVE_RESULT 0x09525880u             /* the last save / load's result */
#define SL_TASK 0x09523C60u                 /* save / load task: +2 state, +4 message */
#define GAME    0x08E75910u                 /* +0x72 mode (3 World Tour) */
/* Loads and saves by the save / load task (World Tour's LOAD GAME, ENTRY MODE's LOAD and SAVE, the boot
 * screen's load, ...) run straight through (below); their in-between screens are kept off screen, and
 * frames kept off screen are not paced (kernel.c), so they take no time: checking the memory stick
 * (state 7, 9), the question (2, messages 0x41 / 0x18), loading or saving (3), "Load completed." /
 * "Save completed." (4, messages 0x11 / 0x0E), and the moment the next screen takes to draw. Anything
 * that waits for a button (a different question, an error) is shown. */
#define MSG_LOAD_WTR 0x41                   /* "Would you like to load previously saved WORLD TOUR DATA?" */
#define MSG_LOAD_SYS 0x18                   /* "Would you like to load SYSTEM DATA?" (ENTRY MODE: entry characters) */
#define MSG_OVERWRITE_SYS 0x31              /* "Would you like to overwrite previously saved SYSTEM DATA?" (SAVE LOAD > SAVE) */
static void hide_load_save(void) {
    static int was;
    int st = RB(SL_TASK + 2), msg = RB(SL_TASK + 4);
    int on = st == 7 || st == 9 || st == 3 || (st == 2 && (msg == MSG_LOAD_WTR || msg == MSG_LOAD_SYS)) || (st == 4 && (msg == 0x11 || msg == 0x0E));
    if (on) rt_hold_display(2);                       /* (the frame on screen was drawn a frame ago) */
    else if (was) rt_hold_display(20);
    was = on;
}

void exsetting_frame(void);                /* exsetting.c */
/* The boot screen's task (0x0888C028, context 0x08E75910: +2 state, +0xE timer) waits 60 frames, auto-loads
 * (0x08816BBC, result in 0x09525880: 1 loaded) and then shows "Auto-load completed." for 180 frames (state 4)
 * unless a button is pressed. Loaded: it moves on at once. */
#define BOOT_STATE 4
#define LOAD_OK 1
static void boot_load_done(void) {
    if (boot_loads != 1) return;
    if (RH(GAME + 2) == BOOT_STATE && RB(SAVE_RESULT) == LOAD_OK && (int16_t)RH(GAME + 0xE) > 1) WH(GAME + 0xE, 1);
    if ((long)rt_vblank_count() > loaded_at + 10) boot_loads = 2;
}
void widescreen_frame(void);               /* widescreen.c */
void sysdata_frame(void) {              /* once a frame, while any feature is on: each part by its own (ext_features.h) */
    extern int rt_widescreen;
    if (FEATURE(WIDESCREEN)) widescreen_frame(); else rt_widescreen = 0;   /* (a saved WIDE FORCED is not used) */
    if (FEATURE(MENUS)) exsetting_frame();
    if (FEATURE(SAVES)) { boot_load_done(); hide_load_save(); }
    /* from a moment after the system data is in. Unlocks without Silent saves: the game may have no
       save to load at all (an empty memory stick), so then from 15 seconds in */
    int ready = loaded_at >= 0 && (long)rt_vblank_count() >= loaded_at + 30;
    if (FEATURE(SAVES) && ready) {
        if (RB(AUTOSAVE) != 1) WB(AUTOSAVE, 1);
        if (RB(AUTOSAVE_S) != 1) WB(AUTOSAVE_S, 1);
    }
    if (!FEATURE(UNLOCKS) || !(ready || (!FEATURE(SAVES) && rt_vblank_count() > 900))) return;
    if (RH(UNLOCKS) != 0xFFFF) WH(UNLOCKS, 0xFFFF);
    if ((RB(CLEARED) & 7) != 7) WB(CLEARED, RB(CLEARED) | 7);
    if ((RW(ISM_PLUS) & ALL_ISM_PLUS) != ALL_ISM_PLUS) WW(ISM_PLUS, RW(ISM_PLUS) | ALL_ISM_PLUS);
}

/* The game's Yes / No question (0x08818BA4, a0 = its context: +4 message, +9 cursor 1 Yes / -1 No;
 * returns 1 or -1 when answered, -1 on back, 0 while waiting). World Tour's LOAD GAME asks "Would
 * you like to load previously saved WORLD TOUR DATA?" (message 0x41), ENTRY MODE's LOAD that or "Would
 * you like to load SYSTEM DATA?" (0x18); the data is always there: answer Yes. */
void hook_f_08818BA4(Cpu *c) {
    uint32_t q = c->r[4];
    int m = RB(q + 4);
    if (m == MSG_LOAD_WTR || m == MSG_LOAD_SYS || m == MSG_OVERWRITE_SYS) { WB(q + 9, 0); c->r[2] = 1; rt_log("sysdata: question %02X answered Yes", m); return; }
    orig_f_08818BA4(c);
}

/* The save / load task (context 0x09523C60: +2 state, +4 message, +5 result) shows its result in
 * state 4 (0x08817C10) and waits for a button. "Load completed." (message 0x11) moves on at once,
 * as the button would (state 5). Errors still wait. */
#define MSG_LOAD_DONE 0x11
#define MSG_SAVE_DONE 0x0E                  /* "Save completed." (SAVE LOAD > SAVE) */
void hook_f_08817C10(Cpu *c) {
    uint32_t t = c->r[4];
    if ((RB(t + 4) == MSG_LOAD_DONE || RB(t + 4) == MSG_SAVE_DONE) && RB(t + 5) == 0) { WB(t + 2, 5); return; }
    orig_f_08817C10(c);
}

/* Entering NETWORK (task 0x088335B8, a0 = task, +6 state) warns that unsaved system data will be
 * lost and asks "Save SYSTEM DATA now?" (state 1; Yes -> 2, No -> 3, back -> 4). AUTO SAVE keeps the
 * data saved, so it is answered No as soon as it is up. */
#define NET_HIDE 4
void hook_f_088335B8(Cpu *c) {
    uint32_t t = c->r[4];
    if (RH(t + 6) <= 3) rt_hold_display(NET_HIDE);  /* the question's screen is not shown */
    if (RH(t + 6) == 1) { WB(0x09524A29u, 1); WH(t + 6, 3); rt_log("sysdata: NETWORK entered without saving first"); }
    orig_f_088335B8(c);
}

/* Back from a mode to the MODE MENU (0x0888D2B0, decompiled in src/core/task/mode_menu.c, which runs
 * this around it: t = the game state, +4 state, +0x10 timer) auto-saves:
 *   0  start (timer 4)          1  timer runs out: auto-save (0x0881725C, runs its own frames while
 *   it shows "Auto-saving SYSTEM DATA..."); result in 0x09525880; timer 120
 *   2  "Auto-save completed." until the timer runs out or a button; then by result: 5 saved / 4 off /
 *      0x10 -> 4, other results -> 3 or 5 (questions about the error)
 *   4  fade, then the MODE MENU
 * All of it is kept off screen, and "Auto-save completed." is not waited on. A save that fails goes on
 * as answering Yes to "continue the game without auto-saving?" would (state 4 and the fade): no TRY
 * AGAIN, no question (docs/SAVES.md, S1 / S2); the failure goes to the log. --save-questions (tests)
 * keeps the game's own questions, so the code behind them can still be checked. */
int sysdata_save_questions;                                    /* main.c: --save-questions */
void f_0880AA74(Cpu *c);                                       /* the screen's fade out */
#define SAVE_FAILED(r) ((r) == 6 || ((r) >= 8 && (r) != 0x10))
int sysdata_back_from_mode(Cpu *c, uint32_t t, int after) {
    extern int quitkey_nosave;                                 /* quitkey.c: F2, back without saving */
    int st = RH(t + 4), r = RB(SAVE_RESULT);
    int silent_fail = !sysdata_save_questions && SAVE_FAILED(r);
    if (!after) {
        if (quitkey_nosave && (st == 0 || st == 1)) { WH(t + 4, 4); st = 4; }
        if (st == 4) quitkey_nosave = 0;
        if (st == 0 || st == 1) rt_hold_display_paced(600);   /* the save runs frames of its own (paced: the sound before it plays on) */
        if (st == 2 && (r == 5 || r == 4 || r == 0x10) && (int16_t)RH(t + 0x10) > 1) WH(t + 0x10, 1);
        if (st == 2 && silent_fail) {                          /* as Yes: state 4, the fade */
            rt_warn("sysdata: the auto-save failed (result 0x%02X): back to the MODE MENU without it", r);
            uint32_t keep[8]; memcpy(keep, &c->r[2], sizeof keep);
            f_0880AA74(c);
            memcpy(&c->r[2], keep, sizeof keep);
            WH(t + 4, 4);
        }
        return 0;
    }
    if (st == 0 || st == 1 || st == 4 || (st == 2 && (r == 5 || r == 4 || r == 0x10 || silent_fail))) rt_hold_display_set(3);
    else rt_hold_display_set(0);                                /* an error question (--save-questions): show it */
    return 0;
}
