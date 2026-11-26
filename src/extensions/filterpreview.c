/* SCREEN SETTING > PREVIEW (screensetting.c: Cross on the row): a real fight to see the FILTER in use
 * on, HUD and all, as the player will see it: two random fighters, both played by the computer, on a
 * random stage, for one round. The game has no CPU-against-CPU mode, so it is Free Battle's ARCADE-MODE
 * (mode 16, kind 1: player 1 against the computer) with its two menus answered here and player 1's
 * fighter handed to the computer as well:
 *   1. Cross asks; the next time task 0 waits for a frame (quitkey_task_wait, task.c) OPTION MODE is left
 *      as the title task leaves for the MODE MENU (sound stopped, screen and game state reset) and Free
 *      Battle is started as the MODE MENU's dispatch starts it (mode_menu.c: player 1 chose it; the
 *      battle task as task 2, task 0 restarted at the fight loop). Nothing is saved: the settings
 *      changed in SCREEN SETTING are kept by leaving it as usual later. The screen shown last stays up
 *      meanwhile (rt_hold_display, not paced: these frames run at full speed).
 *   2. Free Battle's kind menu (fight screen 0, step 1): ARCADE-MODE, as if chosen (BATTLE + 159 = 1,
 *      the menu's cursor at 0x08B01F34), and on (step 2).
 *   3. Its select (screen 1, the frame step 2: battle_select.c): each fighter's choice written as the
 *      select's own rows leave it (the character from the grid's boxes, 0x08A2B378, at random; a random
 *      ISM X / A / V, colours, NORMAL style), the stage drawn as the select's RANDOM draws it
 *      (0x08A2B3E6[rand & 63]: BATTLE + 0 / + 100), and both players marked done (GAME + 10 = 3): the
 *      select's own frame then ends it (the X-ISM characters, colours kept apart) and the fight starts.
 *   4. The fight (screen 2): player 1's fighter is the computer's (+0x175 = 1, +336 = 0, each frame,
 *      over what the select set up for a player); the held picture lets go, and PRESS (Circle) TO EXIT
 *      is drawn in the banners' yellow. While the preview runs the pads give the game nothing
 *      (filterpreview_pad, main.c): no pause, no joining in.
 *   5. Circle (any pad), at any time, or the round's end (the KO or the time, once shown: the round's
 *      part GAME + 6 at 5, the winner's pose; or the fight's step leaving 3): what
 *      leaving a mode does (0x088325DC, mode_menu.c: voices stopped, the battle task ended, the game
 *      state cleared), then task 0 goes straight to OPTION MODE as the MODE MENU starts it (the mode
 *      OPTION, "back from a mode" set so the MODE MENU later saves the settings as after any OPTION MODE
 *      visit), then to SCREEN SETTING by the buttons a player would press (fed to the game in place of
 *      the pads, behind the held picture), with the cursor on PREVIEW (filterpreview_back) and OPTION
 *      MODE's music.
 * The fight saves nothing (Free Battle keeps no records). F2 / SELECT + START during it go back to the
 * MODE MENU as from any mode. With the shaders feature only. */
#include "rt.h"
#include "ext_features.h"
#include "shader.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define CUR_TASK    0x08E75C40u
#define TASK0       0x08AD8FE8u
#define MODE_MENU   0x0888C268u
#define OPTION_TASK 0x088B5F48u
#define FIGHT_LOOP  0x088233ECu             /* task 0 while a mode fights (fight.c) */
#define BATTLE_TASK 0x08833B6Cu             /* task 2 (fight.c) */
#define GAME        0x08E75910u
#define BATTLE      0x092E1228u             /* +0 the stage */
#define FIGHTER(k)  (0x092E3EA0u + 0x498u * (uint32_t)(k))
#define BACK_FROM_MODE 0x0951CE54u          /* u16: a mode is returning to the MODE MENU (its auto-save of the settings) */
#define KIND_CURSOR 0x08B01F34u             /* s8: Free Battle's kind menu's cursor */
#define BGRID_BOXES 0x08A2B378u             /* s8 by row * 7 + column: the character, 0x40 + a random box's kind, < 0 none */
#define RANDOM_STAGES 0x08A2B3E6u           /* s8 by a random number & 0x3F: the select's RANDOM stage */
#define SIDE_LIST_0 0x09517A28u             /* the select's lists, player 1's (GAME + 53 = 1) then the other */
#define SIDE_LIST_1 0x09517A50u
#define MODE_OPTION 5
#define MODE_FREE   16
#define KIND_ARCADE 1
#define PSP_CIRCLE  0x2000u
#define PSP_CROSS   0x4000u
#define PSP_DOWN    0x40u
#define ROUND_OVER  5                       /* the round's part (GAME + 6) to go back at: after the KO, before the pose */
#define OVER_WAIT   600                     /* frames of the KO's part before going back, at most */
#define PROMPT_Y    0xB8                    /* PRESS (Circle) TO EXIT: above the super gauges, in the middle */

void f_0887C4CC(Cpu *c); void f_0880A6CC(Cpu *c); void f_0880A7B0(Cpu *c);
void f_08814320(Cpu *c); void f_0880A39C(Cpu *c); void f_08821FEC(Cpu *c); void f_088AC618(Cpu *c);
void f_08804A74(Cpu *c); void f_08813744(Cpu *c); void f_08805614(Cpu *c); void f_088054C8(Cpu *c);
void f_08823304(Cpu *c); void f_08822934(Cpu *c); void f_08809FC8(Cpu *c); void f_08805564(Cpu *c);
void f_08822BD8(Cpu *c); void f_0882E0D0(Cpu *c); void f_089E0B64(Cpu *c);
void rt_hold_display(int frames); void rt_hold_display_set(int frames);

enum { P_OFF, P_ASKED, P_FIGHT, P_BACK };
static int phase, chosen, shown, fought, over, back;   /* back: SCREEN SETTING is to put its cursor on PREVIEW */
static int step, step_wait;                 /* the way back's button, and frames on it */
static uint32_t edges;                      /* PSP buttons newly pressed (filterpreview_pad) */
static uint32_t macro_buttons;              /* the way back: the button held this frame */
static uint32_t buf;
static uint32_t seed = 0x9E3779B9u;         /* our own random numbers (xorshift): the game's are left alone */
static int pick(int n) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; if (!seed) seed = 1; return n > 0 ? (int)(seed % (uint32_t)n) : 0; }
int filterpreview_on(void) { return phase != P_OFF; }
int filterpreview_back(void) { int b = back; back = 0; return b; }

static void call(Cpu *c, void (*f)(Cpu *), uint32_t a0) { c->r[4] = a0; c->r[31] = 0; f(c); }
static void text(Cpu *c, uint32_t s, int font, int x, int y, int colour) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = s; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static void symbol(Cpu *c, int id, int x, int y) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = (uint32_t)id; c->r[5] = (uint32_t)x; c->r[6] = (uint32_t)y; f_0880A7B0(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static void sound(Cpu *c, int id) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = 0; c->r[5] = (uint32_t)id; c->r[6] = 0xC0; f_0887C4CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}

void filterpreview_start(Cpu *c) {
    if (!FEATURE(SHADERS) || phase != P_OFF) return;
    seed ^= (uint32_t)time(NULL) * 2654435761u ^ (uint32_t)rt_vblank_count() * 40503u;   /* a new pick each time */
    phase = P_ASKED; edges = 0;
    sound(c, 0x39);
    rt_log("filter preview: a fight with %s", filter_get(rt_filter >= 0 && rt_filter < filter_count() ? rt_filter : 0)->name);
}

/* main.c, each frame, each pad (PSP bits): while the preview runs the game gets nothing but what the
   way back presses; what was newly pressed is kept for the task (edges) */
uint32_t filterpreview_pad(int i, uint32_t raw) {
    static uint32_t prev[4];
    if (i < 0 || i > 3) return raw;
    uint32_t now = raw & ~prev[i];
    prev[i] = raw;
    if (phase == P_OFF) return raw;
    edges |= i == 0 ? now : now & PSP_CIRCLE;   /* (any pad's Circle leaves) */
    return i == 0 ? macro_buttons : 0;
}

/* 1: out of OPTION MODE, into Free Battle (as the MODE MENU's dispatch starts it, mode_menu.c) */
static void start_fight(Cpu *c) {
    phase = P_FIGHT; chosen = shown = fought = over = 0; edges = 0;
    rt_hold_display(900);
    call(c, f_08814320, 0);
    for (uint32_t off = 0; off <= 0x16; off += 2) WH(GAME + off, 0);
    call(c, f_0880A39C, GAME); call(c, f_08821FEC, 0); call(c, f_088AC618, 0); call(c, f_08804A74, 0); call(c, f_08813744, 0);
    call(c, f_08823304, 0); call(c, f_08822934, 0);
    WB(GAME + 50, 0); WB(GAME + 72, 0); WB(GAME + 0x88, 0); WB(GAME + 43, 0); WB(GAME + 42, 0); WB(GAME + 41, 0);
    WH(GAME + 22, 0); WH(BATTLE, 0);
    for (uint32_t off = 2; off <= 20; off += 2) WH(GAME + off, 0);
    WB(GAME + 0x5D, 0); WB(GAME + 92, 0); WB(BATTLE + 105, 0); WB(BATTLE + 155, 0); WB(GAME + 0x89, 0);
    call(c, f_0880A39C, 0);
    WB(GAME + 35, 0); WH(GAME + 0, 0); WB(GAME + 0xBF, 0); WB(BATTLE + 90, 0); WB(GAME + 0x34, 1);
    WB(FIGHTER(0) + 337, 0); WB(FIGHTER(1) + 337, 1);
    WB(GAME + 0x72, MODE_FREE); WB(GAME + 0x35, 1); WB(GAME + 0x88, 1);   /* Free Battle, chosen by player 1 */
    c->r[4] = 2; c->r[5] = BATTLE_TASK; c->r[31] = 0; f_088054C8(c);
    call(c, f_08805614, FIGHT_LOOP);                       /* (does not come back) */
}

/* 3: the select's choices, for both fighters */
static int random_character(void) {
    int pick_from[64], n = 0;
    for (uint32_t k = 0; k < 49; k++) { int8_t b = (int8_t)RB(BGRID_BOXES + k); if (b >= 0 && b < 0x40 && n < 64) pick_from[n++] = b; }
    return n ? pick_from[pick(n)] : 0;
}
static void choose(void) {
    static const char *const isms[3] = { "X", "A", "V" };
    for (int k = 0; k < 2; k++) {
        uint32_t f = FIGHTER(k), list = k ? SIDE_LIST_1 : SIDE_LIST_0;
        int ch = random_character(), ism = pick(3) - 1, colour = pick(6);
        WB(f + 338, (uint32_t)ch); WB(f + 448, (uint32_t)ch);                          /* the character (and the last one) */
        WB(f + 386, (uint32_t)(int8_t)ism); WB(f + 450, (uint32_t)(int8_t)ism);         /* the ISM: -1 X, 0 A, 1 V */
        WB(f + 376, (uint32_t)colour); WB(f + 449, (uint32_t)colour);
        WB(f + 438, 0); WB(f + 428, 0); WB(f + 385, 0); WB(f + 431, 0); WB(f + 439, 0);   /* NORMAL, no extras */
        WB(f + 127, 0); WB(f + 81, 0); WB(f + 126, 0); WB(f + 429, 0);                    /* not an entry, not a random box */
        WH(f + 424, 0x197);                                                               /* every row done */
        uint32_t m = list + (uint32_t)(int32_t)(int8_t)RB(list + 1);
        WB(m + 3, (uint32_t)ch); WB(m + 6, (uint32_t)(int8_t)ism); WB(m + 15, 0); WB(m + 18, 0);
        rt_log("filter preview: fighter %d is character %d, %s-ISM, colours %d", k + 1, ch, isms[ism + 1], colour);
    }
    int stage = (int8_t)RB(RANDOM_STAGES + (uint32_t)pick(64));
    WH(BATTLE + 0, (uint32_t)stage); WH(BATTLE + 100, (uint32_t)stage); WH(BATTLE + 24, 0);
    WH(GAME + 10, 3);                                       /* both done: the select ends itself */
    rt_log("filter preview: stage %d", stage);
}

/* 5: out of the fight, into OPTION MODE (leaving a mode as 0x088325DC does, then as the dispatch starts
   OPTION MODE) */
static void leave_fight(Cpu *c) {
    phase = P_BACK; back = 0; macro_buttons = 0; edges = 0; step = step_wait = 0;
    rt_hold_display(900);
    call(c, f_08814320, 0);
    call(c, f_08809FC8, 0);
    call(c, f_08805564, 2);                                 /* the battle task */
    call(c, f_08822BD8, 0); call(c, f_0882E0D0, 0); call(c, f_0880A39C, 0); call(c, f_08821FEC, 0);
    c->r[4] = c->r[5] = c->r[6] = 0xFF000000u; c->r[31] = 0; f_089E0B64(c);
    call(c, f_08823304, 0); call(c, f_08822934, 0);
    for (uint32_t off = 0; off <= 0x16; off += 2) WH(GAME + off, 0);
    WH(BATTLE, 0); WB(GAME + 0x5D, 0); WB(GAME + 0x35, 0); WB(GAME + 0x88, 0); WB(GAME + 0x89, 0);
    call(c, f_0880A39C, 0);
    WB(GAME + 0x72, MODE_OPTION); WH(BACK_FROM_MODE, 1);
    rt_log("filter preview: back to SCREEN SETTING");
    call(c, f_08805614, OPTION_TASK);                      /* (does not come back) */
}

/* 2 - 4: each frame of the fight loop while the preview runs */
static void fight_frame(Cpu *c) {
    int screen = RH(GAME + 2), st = RH(GAME + 4);
    if (screen == 0 && st == 1) { WB(KIND_CURSOR, KIND_ARCADE); WB(BATTLE + 159, KIND_ARCADE); WH(GAME + 4, 2); }
    if (screen == 1 && st == 2 && RH(GAME + 6) >= 1 && !chosen) { choose(); chosen = 1; }
    if (screen != 2) return;
    if (!shown) { shown = 1; rt_hold_display_set(0); }
    WB(FIGHTER(0) + 0x175, 1); WB(FIGHTER(0) + 336, 0);   /* player 1's fighter: the computer's too */
    /* the round: step 3 (screen 10), its own steps in GAME + 6: 2 - 3 the fighting, 4 the KO or the time,
       5 the winner's pose, 6 - 7 the round's end */
    int part = RH(GAME + 6);
    if (st == 3 && part >= 2 && part <= 3) fought = 1;
    if (fought && (st != 3 || part >= ROUND_OVER || (part == 4 && ++over > OVER_WAIT))) {
        if (!(edges & PSP_CIRCLE)) rt_log("filter preview: the round is over");
        edges |= PSP_CIRCLE;
    }
    if (!buf && (buf = rt_alloc(32, 1, 16, "filter preview"))) { strcpy((char *)HOST(buf), "PRESS"); strcpy((char *)HOST(buf + 16), "TO EXIT"); }
    if (buf) { text(c, buf, 2, 0x4D, PROMPT_Y, 0xB); symbol(c, 0, 0xA2, PROMPT_Y + 2); text(c, buf + 16, 2, 0xBD, PROMPT_Y, 0xB); }
}

/* The way back, in OPTION MODE: it opens on GAME OPTION (exsetting.c); down twice to SCREEN SETTING and
   Cross, a press (4 frames) every 30 frames once it has faded in; SCREEN SETTING then puts its cursor on
   PREVIEW (filterpreview_back). */
static const struct { uint32_t b; int wait; } way_back[] = { { 0, 90 }, { PSP_DOWN, 30 }, { PSP_DOWN, 30 }, { PSP_CROSS, 40 } };

/* quitkey_task_wait (task.c's frame wait): 1 if task 0 was restarted here */
int filterpreview_task_wait(Cpu *c) {
    if (phase == P_OFF || RW(CUR_TASK) != TASK0) return 0;
    uint32_t task = RW(TASK0 + 4);
    if (phase == P_ASKED) {
        if (task != OPTION_TASK) { phase = P_OFF; return 0; }
        start_fight(c);
        return 1;
    }
    if (task == MODE_MENU) {                                    /* F2 or SELECT + START: the MODE MENU, as from any mode */
        phase = P_OFF; macro_buttons = 0; rt_hold_display_set(0);
        return 0;
    }
    if (phase == P_FIGHT && task == FIGHT_LOOP) {
        fight_frame(c);
        int leave = (edges & PSP_CIRCLE) != 0;
        edges = 0;
        if (leave) { leave_fight(c); return 1; }
        return 0;
    }
    if (phase == P_BACK && task == OPTION_TASK) {
        macro_buttons = 0;
        if (step >= (int)(sizeof way_back / sizeof *way_back)) {
            phase = P_OFF; step = step_wait = 0; back = 1;
            rt_hold_display_set(2);
            return 0;
        }
        rt_hold_display(30);
        if (step_wait < 4) macro_buttons = way_back[step].b;
        if (++step_wait >= way_back[step].wait) { step++; step_wait = 0; }
    }
    return 0;
}
