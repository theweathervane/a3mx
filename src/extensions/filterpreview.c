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
 *      over what the select set up for a player); the held picture lets go, and SCORE RANKING's PRESS (Circle) TO
 *      EXIT is drawn above the gauges in its small yellow font, flashing. While the preview runs the pads give the game nothing
 *      (filterpreview_pad, main.c) but player 1's START once the fight is up (0.173.0): it pauses, and the
 *      pause is the FILTER screen over the fight (pausefilter.c), player 1's buttons going to it; Cross
 *      chooses (kept), Circle cancels, and either ends the pause. No joining in.
 *   5. Circle (any pad), at any time, or the round's end (the KO or the time, once shown: the round's
 *      part GAME + 6 at 5, the winner's pose; or the fight's step leaving 3): the game's fade to black
 *      (as its fade task draws it: fade_step, 16 frames), then, behind that black picture held up, what leaving a mode does
 *      (0x088325DC, mode_menu.c: voices stopped, the battle task ended, the game state cleared), task 0
 *      straight to OPTION MODE as the MODE MENU starts it (the mode OPTION, "back from a mode" set so
 *      the MODE MENU later saves the settings as after any OPTION MODE visit), and on its menu's first
 *      frames straight into SCREEN SETTING as the menu's Cross does, without its sound
 *      (open_screen_setting). SCREEN SETTING puts its cursor on PREVIEW (filterpreview_back), the
 *      picture lets go and SCREEN SETTING fades in from the black (16 frames), where it was, with OPTION
 *      MODE's music. No buttons are pressed for the player on the way: no menu sounds.
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
#define PSP_START   0x0008u
#define OPT         0x08E75912u             /* OPTION MODE's context: +0 state, +1 the menu's cursor, +6 in a submenu, +7 its row */
#define OPT_STATES  0x08A37A04u             /* OPTION MODE's states by OPT + 0 */
#define OPTION_MENU 0x088B6164u             /* the state of its menu */
#define SUB_TIMER   0x08B02E0Cu             /* +2 the held direction's repeat count */
#define ITEM_SCREEN 2                       /* the menu's SCREEN SETTING */
#define FADE_LEVEL  0x08E75B9Cu             /* float: the fade, 0 clear - 1 the full colour (fade.c) */
#define FADE_UPDATED 0x08E75B9Au            /* u16: set to 1 when the level changes */
#define FADE_WAIT   60                      /* frames the fade out may take, at most */
#define SHOW_WAIT   300                     /* frames SCREEN SETTING may take to open, at most */
#define ROUND_OVER  5                       /* the round's part (GAME + 6) to go back at: after the KO, before the pose */
#define OVER_WAIT   600                     /* frames of the KO's part before going back, at most */
#define PRESS_EXIT  0x089FFDDCu             /* "PRESS    TO EXIT": SCORE RANKING's, font 1, the Circle symbol in its gap */
#define PROMPT_X    0x7A                    /* where SCORE RANKING has it (0x088C0D00), the symbol 0x30 further */
#define PROMPT_Y    0xB8                    /* above the super gauges */
#define PROMPT_BLINK 20                     /* frames on, then off */

void f_0887C4CC(Cpu *c); void f_0880A6CC(Cpu *c); void f_0880A7B0(Cpu *c);
void f_08814320(Cpu *c); void f_0880A39C(Cpu *c); void f_08821FEC(Cpu *c); void f_088AC618(Cpu *c);
void f_08804A74(Cpu *c); void f_08813744(Cpu *c); void f_08805614(Cpu *c); void f_088054C8(Cpu *c);
void f_08823304(Cpu *c); void f_08822934(Cpu *c); void f_08809FC8(Cpu *c); void f_08805564(Cpu *c);
void f_08822BD8(Cpu *c); void f_0882E0D0(Cpu *c); void f_089E0B64(Cpu *c); void f_089E0BD8(Cpu *c);
void f_088B8078(Cpu *c);                    /* option_mode.c: a menu item's objects (item) */
void rt_hold_display(int frames); void rt_hold_display_set(int frames);

enum { P_OFF, P_ASKED, P_FIGHT, P_FADE, P_BACK, P_SHOW, P_FADE_IN };
static int phase, chosen, shown, fought, over, back;   /* back: SCREEN SETTING is to put its cursor on PREVIEW */
static int faded, waited;                   /* the fade out was started; frames waited in this phase */
static uint32_t edges;                      /* PSP buttons newly pressed (filterpreview_pad) */
static unsigned blink;
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

/* main.c, each frame, each pad (PSP bits): while the preview runs the game gets nothing but player 1's
   START in the fight (it pauses, and the pause is the FILTER screen: pausefilter.c), and while paused
   player 1's buttons (that screen's); what was newly pressed is kept for the task (edges), but not while
   paused (Circle there closes the FILTER screen, not the preview) */
int filterscreen_in_pause(void);
uint32_t filterpreview_pad(int i, uint32_t raw) {
    static uint32_t prev[4];
    if (i < 0 || i > 3) return raw;
    uint32_t now = raw & ~prev[i];
    prev[i] = raw;
    if (phase == P_OFF) return raw;
    int paused = phase == P_FIGHT && (RB(GAME + 0x59) || filterscreen_in_pause());
    if (!paused) edges |= i == 0 ? now : now & PSP_CIRCLE;   /* (any pad's Circle leaves) */
    if (i == 0 && phase == P_FIGHT && shown) return paused ? raw : raw & PSP_START;
    return 0;
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
    phase = P_BACK; back = 0; edges = 0; waited = 0;
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

/* The fade to black, a step a frame, as the game's fade task steps it (fade.c: the level 0 - 1 by 1/16,
   drawn as a full-screen black). Done here rather than by that task (0x0880AA74), which the fight loop
   then waits on for good. 1 once black. */
static int fade_step(Cpu *c) {
    float f;
    if (!faded) { faded = 1; c->r[4] = 0xFF000000u; c->r[31] = 0; f_089E0BD8(c); f = 0.0f; }
    else memcpy(&f, HOST(FADE_LEVEL), 4);
    f += 1.0f / 16;
    if (f > 1.0f) f = 1.0f;
    memcpy(HOST(FADE_LEVEL), &f, 4); WH(FADE_UPDATED, 1);
    return f >= 1.0f || ++waited > FADE_WAIT;
}
/* ... and back from black, on SCREEN SETTING: 1 once clear */
static int unfade_step(void) {
    float f;
    memcpy(&f, HOST(FADE_LEVEL), 4);
    f -= 1.0f / 16;
    if (!(f > 0.0f)) f = 0.0f;
    memcpy(HOST(FADE_LEVEL), &f, 4); WH(FADE_UPDATED, 1);
    return f <= 0.0f;
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
    if (phase == P_FIGHT && fought && (st != 3 || part >= ROUND_OVER || (part == 4 && ++over > OVER_WAIT))) {
        rt_log("filter preview: the round is over");
        edges |= PSP_CIRCLE;
    }
    if (phase != P_FIGHT || RB(GAME + 0x59) || filterscreen_in_pause()) return;   /* (no prompt while fading out, or paused: the FILTER screen) */
    if ((++blink / PROMPT_BLINK) & 1) return;              /* flashing: on, off, ... */
    text(c, PRESS_EXIT, 1, PROMPT_X, PROMPT_Y, 0xB); symbol(c, 0, PROMPT_X + 0x30, PROMPT_Y);   /* as SCORE RANKING draws it */
}

/* Into SCREEN SETTING at once, as the menu's Cross on it does (0x088B6164: no sound): the screen and its
   row cleared, in a submenu, the repeat count, the item's objects (SCREEN SETTING's bar); SCREEN SETTING
   then puts its cursor on PREVIEW (filterpreview_back). The menu's cursor is left on SCREEN SETTING, so
   EXIT comes back to it there. */
static void open_screen_setting(Cpu *c) {
    WB(OPT + 1, ITEM_SCREEN);
    call(c, f_0880A39C, 0);
    WB(OPT + 7, 0); WB(OPT + 6, 1); WB(SUB_TIMER + 2, 0x14);
    call(c, f_088B8078, ITEM_SCREEN);
}

/* quitkey_task_wait (task.c's frame wait): 1 if task 0 was restarted here. When it goes on to wait, the
   wait's own argument (a0: the frames) and the other registers are as they came. */
static int task_wait(Cpu *c);
int filterpreview_task_wait(Cpu *c) {
    if (phase == P_OFF) return 0;
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    int r = task_wait(c);
    if (!r) { memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra; }
    return r;
}
static int task_wait(Cpu *c) {
    if (phase == P_OFF || RW(CUR_TASK) != TASK0) return 0;
    uint32_t task = RW(TASK0 + 4);
    if (phase == P_ASKED) {
        if (task != OPTION_TASK) { phase = P_OFF; return 0; }
        start_fight(c);
        return 1;
    }
    if (task == MODE_MENU) {                                    /* F2 or SELECT + START: the MODE MENU, as from any mode */
        phase = P_OFF; rt_hold_display_set(0);
        return 0;
    }
    if ((phase == P_FIGHT || phase == P_FADE) && task == FIGHT_LOOP) {
        fight_frame(c);
        if (phase == P_FIGHT && (edges & PSP_CIRCLE)) { phase = P_FADE; faded = 0; waited = 0; }   /* 5: fade to black first */
        edges = 0;
        if (phase == P_FADE && fade_step(c)) { rt_hold_display(900); leave_fight(c); return 1; }   /* the black picture stays up */
        return 0;
    }
    if (phase == P_BACK && task == OPTION_TASK) {               /* its menu up (and laid out): straight into SCREEN SETTING */
        rt_hold_display(30);
        if (RW(OPT_STATES + 4u * (uint32_t)(int32_t)(int8_t)RB(OPT)) == OPTION_MENU && !RB(OPT + 6) && ++waited >= 2) {
            open_screen_setting(c);
            phase = P_SHOW; back = 1; waited = 0;
        }
        return 0;
    }
    if (phase == P_SHOW) {                                      /* until SCREEN SETTING takes input (it took back) */
        if (!back || ++waited > SHOW_WAIT) { phase = P_FADE_IN; back = 0; rt_hold_display_set(1); }
        else rt_hold_display(30);
        return 0;
    }
    if (phase == P_FADE_IN && unfade_step()) phase = P_OFF;      /* SCREEN SETTING fades in from the black */
    return 0;
}
