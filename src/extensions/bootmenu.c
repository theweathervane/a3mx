/* Boot straight to the MODE MENU (feature boot_to_menu, off by default).
 *
 * The PSP's way in: the boot screens (src/core/task/boot.c: fades, NOW LOADING, set-up loads), then
 * task 0 restarts at the title (0x0888B398, src/core/task/title.c), whose first screen after boot is 8:
 * fade in, the title music, a 60-frame wait, the SYSTEM DATA load (0x08816BBC; with Silent saves,
 * a3mx.dat: sysdata.c), fade out. Then screen 0 goes on to the logos, the story and the attract demos
 * until a button press: screen 7 restarts task 0 at the MODE MENU (0x0888C268,
 * src/core/task/mode_menu.c), whose screen 1 is the title picture with PRESS START BUTTON; Start there,
 * once it has faded in, opens the menu (screen 2).
 *
 * With this feature, once per run:
 *   - the display is held from the first boot screen (rt_hold_display: frames run off screen at full
 *     speed), so nothing is shown and nothing is waited for until the menu is up;
 *   - the title task starts at screen 0 rather than 8 (TITLE_SEEN set), and screen 0 does the load
 *     itself, with what screen 8 does after it, then goes to screen 7: no title music, wait or fades.
 *     If the load fails (the game would ask a question), the display is let go and screen 8 runs as
 *     usual, so the question can be answered; the MODE MENU follows it;
 *   - the MODE MENU's screen 1, once faded in, does what Start does (on to screen 2, the object list
 *     cleared), and the hold ends a few frames later with the menu drawn.
 * When the MODE MENU is later left alone and goes back to the title, the title runs as on the PSP. */
#include "rt.h"

#define TITLE_SEEN    0x08A26BA0u           /* s8: the title task starts at screen 8 until it is set */
#define TITLE_CHOICE  0x09525880u           /* s8: the load's result (screen 8) */
#define G_FADE_TASK   0x5D                  /* s8: a fade is running (fade.c) */
#define TO_MODE_MENU  7                     /* the title's screen 7: on to the MODE MENU */
#define HOLD_CAP      1800                  /* frames: in case the MODE MENU never comes */
#define HOLD_AFTER    6                     /* frames held after Start, until the menu is drawn */

void rt_hold_display(int frames);           /* main.c */
void rt_hold_display_set(int frames);
void f_08816BBC(Cpu *c);                    /* the SYSTEM DATA load: r2 its result */
void f_08821FEC(Cpu *c); void f_0880A39C(Cpu *c); void f_08816F5C(Cpu *c);
void f_08823304(Cpu *c);                    /* clear the object list (src/core/object/lists.c) */

static int boot_done, title_seen_set, loaded, start_pending;
static void call(Cpu *c, void (*f)(Cpu *)) { c->r[31] = 0; f(c); }

/* Around the first boot screen (0x08805734): hold the display from the start */
int bootmenu_boot_start(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    if (after || boot_done) return 0;
    boot_done = 1;
    rt_hold_display(HOLD_CAP);
    return 0;
}

/* Around the title task's start (0x0888B398): the first time, start at screen 0, not 8 */
int bootmenu_title_task(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    if (after || title_seen_set) return 0;
    title_seen_set = 1;
    if (!RB(TITLE_SEEN)) WB(TITLE_SEEN, 1);
    return 0;
}

/* Around the title's screen 0 (0x0888B4D8, a0 = the game state): 1 = skipped */
int bootmenu_title_first(Cpu *c, uint32_t g, int after) {
    if (after || start_pending < 0) return 0;
    if (!loaded) {
        loaded = 1;
        c->r[29] -= 0x20;
        WB(TITLE_CHOICE, 0);
        call(c, f_08816BBC);
        int r = (int8_t)c->r[2];
        WB(TITLE_CHOICE, (uint32_t)r);
        if (r == 2 || r >= 7) {                     /* the load failed: screen 8 shows it and asks */
            c->r[29] += 0x20;
            rt_hold_display_set(0);
            rt_log("boot_to_menu: the SYSTEM DATA load failed (%d): the title's load screen asks", r);
            WH(g + 0, 8); WH(g + 2, 0);
            return 1;
        }
        for (uint32_t off = 0; off <= 0x16; off += 2) WH(g + off, 0);   /* what screen 8 does after it */
        call(c, f_08821FEC); call(c, f_0880A39C); call(c, f_08816F5C);
        c->r[29] += 0x20;
    }
    start_pending = 1;
    WH(g + 0, TO_MODE_MENU); WH(g + 2, 0);
    rt_log("boot_to_menu: SYSTEM DATA loaded, straight to the MODE MENU");
    return 1;
}

/* After the MODE MENU's screen 1 (0x0888C3A0, a0 = the game state): Start, once, when it would count */
int bootmenu_press_start(Cpu *c, uint32_t g, int after) {
    if (!after || start_pending != 1) return 0;
    if (RH(g + 0) != 1) { start_pending = -1; rt_hold_display_set(0); return 0; }   /* (left some other way) */
    if ((int8_t)RB(g + G_FADE_TASK) != 0) return 0;          /* still fading in */
    start_pending = -1;
    WH(g + 0, 2);
    c->r[31] = 0; f_08823304(c);
    rt_hold_display_set(HOLD_AFTER);
    return 0;
}
