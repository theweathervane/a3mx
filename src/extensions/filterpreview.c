/* The FILTER screen's live preview (filterscreen.c: Triangle on a filter). The game has no CPU-against-
 * CPU mode of its own, but its attract mode has one: the title task's screen 3 (src/core/task/title.c,
 * "demo 2") is a fight between two computer fighters on a stage, with the www.capcom.com banner, as the
 * attract mode shows it between the title and the score ranking. The preview is that fight, looped,
 * drawn through the filter under the FILTER screen's cursor:
 *   1. Triangle asks; the next time task 0 waits for a frame (quitkey_task_wait, task.c) OPTION MODE is
 *      left as the title's screen 7 leaves for the MODE MENU (sound stopped, screen and game state
 *      reset) and task 0 restarts at the title task (0x0888B398). Nothing is saved on the way: the
 *      settings changed in SCREEN SETTING are kept by leaving it as usual later (the filter shown is
 *      rt_filter_preview, not the setting).
 *   2. The title's first screen goes on to screen 1; here it goes to 3, the demo fight, instead, and
 *      when the fight is over (screen 4 next) to 3 again: the same machinery picks its fighters and
 *      stage each time.
 *   3. While it runs the players' pads give the game nothing (filterpreview_pad, main.c): left / right
 *      change the filter shown within its category, Circle ends the preview. PRESS (Circle) TO EXIT is
 *      drawn bottom centre above the www.capcom.com banner, in the banners' yellow (font 2, colour 0xB)
 *      with the game's Circle symbol (0x0880A7B0).
 *   4. Circle: task 0 goes straight to OPTION MODE, as the MODE MENU starts it (its state cleared, the mode
 *      OPTION, "back from a mode" set so the MODE MENU later keeps the settings as after any OPTION MODE
 *      visit), then to SCREEN SETTING by the buttons a player would press (fed to the game in place of
 *      the pads, a press every 30 frames), and the FILTER screen opens on the filter previewed.
 * The attract mode saves nothing, so records, unlocks and saves are untouched: only the settings are
 * saved, by the game, when OPTION MODE is left as usual. With the shaders feature only. */
#include "rt.h"
#include "ext_features.h"
#include "shader.h"
#include <string.h>

#define CUR_TASK    0x08E75C40u
#define TASK0       0x08AD8FE8u
#define TITLE_TASK  0x0888B398u
#define MODE_MENU   0x0888C268u
#define GAME        0x08E75910u
#define G_PRESSED   0x35
#define PSP_LEFT    0x80u
#define PSP_RIGHT   0x20u
#define PSP_CIRCLE  0x2000u
#define PSP_CROSS   0x4000u
#define PSP_START   0x8u
#define PSP_DOWN    0x40u
#define PSP_UP      0x10u

void f_0887C4CC(Cpu *c); void f_0880A6CC(Cpu *c); void f_0880A7B0(Cpu *c);
void f_08814320(Cpu *c); void f_0880A39C(Cpu *c); void f_08821FEC(Cpu *c); void f_088AC618(Cpu *c);
void f_08804A74(Cpu *c); void f_08813744(Cpu *c); void f_08805614(Cpu *c);
void filterscreen_reopen(int f);            /* filterscreen.c: open on F when SCREEN SETTING is next up */

enum { P_OFF, P_ASKED, P_DEMO, P_LEAVING, P_BACK };
static int phase, shown;                    /* shown: the filter previewed */
static uint32_t edges;                      /* PSP buttons newly pressed (filterpreview_pad) */
static uint32_t buf;
int filterpreview_on(void) { return phase != P_OFF; }

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

void filterpreview_start(Cpu *c, int f) {
    if (!FEATURE(SHADERS) || phase != P_OFF) return;
    shown = f; rt_filter_preview = f; phase = P_ASKED; edges = 0;
    { void filterscreen_close(Cpu *c, int snd); filterscreen_close(NULL, 0); }   /* (opened again on the way back) */
    sound(c, 0x39);
    rt_log("filter preview: %s", filter_get(f)->name);
}

/* main.c, each frame, each pad (PSP bits): while the preview runs the game gets nothing but what the
   way back presses; what was pressed is kept for the task (edges) */
static uint32_t macro_buttons;              /* the way back: the button held this frame */
uint32_t filterpreview_pad(int i, uint32_t raw) {
    static uint32_t prev;
    if (phase == P_OFF) { prev = raw; return raw; }
    if (i == 0) { edges |= raw & ~prev; prev = raw; }
    else edges |= raw & (PSP_CIRCLE | PSP_LEFT | PSP_RIGHT) & ~prev;   /* (any pad: player 1's edges are enough to tell) */
    return i == 0 ? macro_buttons : 0;
}

/* The way back, in OPTION MODE: it opens on GAME OPTION (exsetting.c); down twice to SCREEN SETTING and
   Cross, a press (4 frames) every 30 frames once it has faded in; the FILTER screen then opens itself
   (filterscreen_back) on the filter previewed. */
static const struct { uint32_t b; int wait; } way_back[] = { { 0, 90 }, { PSP_DOWN, 30 }, { PSP_DOWN, 30 }, { PSP_CROSS, 60 } };
static int step, step_wait;
#define OPTION_TASK    0x088B5F48u
#define BACK_FROM_MODE 0x0951CE54u          /* u16: a mode is returning to the MODE MENU (its auto-save of the settings) */
#define STAGE_NUMBER   0x092E1228u
#define MODE_OPTION    5
void f_08823304(Cpu *c); void f_08822934(Cpu *c);

/* quitkey_task_wait (task.c's frame wait): 1 if the task was restarted here */
int filterpreview_task_wait(Cpu *c) {
    if (phase == P_OFF || RW(CUR_TASK) != TASK0) return 0;
    uint32_t task = RW(TASK0 + 4);
    if (phase == P_ASKED) {                                     /* leave OPTION MODE for the title task */
        phase = P_DEMO; edges = 0;
        call(c, f_08814320, 0);
        for (uint32_t off = 0; off <= 0x16; off += 2) WH(GAME + off, 0);
        call(c, f_0880A39C, GAME); call(c, f_08821FEC, 0); call(c, f_088AC618, 0); call(c, f_08804A74, 0); call(c, f_08813744, 0);
        call(c, f_08805614, TITLE_TASK);                       /* (does not come back) */
        return 1;
    }
    if (phase == P_DEMO && task == TITLE_TASK) {
        int screen = RH(GAME + 0);
        if (screen == 1 || screen == 4) { WH(GAME + 0, 3); WH(GAME + 2, 0); }   /* the demo fight, again */
        if (edges & (PSP_LEFT | PSP_RIGHT)) {
            const FilterEntry *e = filter_get(shown);
            int first = filter_category_first(e->category), n = filter_category_size(e->category);
            shown = first + (shown - first + ((edges & PSP_RIGHT) ? 1 : n - 1)) % n;
            rt_filter_preview = shown;
            rt_log("filter preview: %s", filter_get(shown)->name);
        }
        if (edges & PSP_CIRCLE) phase = P_LEAVING;
        edges = 0;
        if (screen >= 2 && screen <= 6) {                       /* PRESS (Circle) TO EXIT, bottom centre */
            if (!buf && (buf = rt_alloc(32, 1, 16, "filter preview"))) { strcpy((char *)HOST(buf), "PRESS"); strcpy((char *)HOST(buf + 16), "TO EXIT"); }
            if (buf) { text(c, buf, 2, 0x4D, 0xB4, 0xB); symbol(c, 0, 0xA2, 0xB6); text(c, buf + 16, 2, 0xBD, 0xB4, 0xB); }
        }
        return 0;
    }
    if (phase == P_LEAVING) {                                   /* straight into OPTION MODE, as the MODE MENU starts it */
        phase = P_BACK; step = 0; step_wait = 0; filterscreen_reopen(shown);
        call(c, f_08814320, 0); call(c, f_08823304, 0); call(c, f_08822934, 0);
        for (uint32_t off = 0; off <= 0x16; off += 2) WH(GAME + off, 0);
        WH(STAGE_NUMBER, 0); WB(GAME + 0x5D, 0); WB(GAME + 0x35, 0);
        call(c, f_0880A39C, GAME); call(c, f_08821FEC, 0); call(c, f_088AC618, 0); call(c, f_08804A74, 0); call(c, f_08813744, 0);
        WB(GAME + 0x72, MODE_OPTION); WH(BACK_FROM_MODE, 1);
        call(c, f_08805614, OPTION_TASK);                      /* (does not come back) */
        return 1;
    }
    if (phase == P_BACK && task == OPTION_TASK) {
        macro_buttons = 0;
        if (step >= (int)(sizeof way_back / sizeof *way_back)) { phase = P_OFF; rt_filter_preview = -1; return 0; }
        if (step_wait < 4) macro_buttons = way_back[step].b;
        if (++step_wait >= way_back[step].wait) { step++; step_wait = 0; }
    }
    return 0;
}
