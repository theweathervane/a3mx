/* Boot straight to the MODE MENU (feature boot_to_menu, off by default).
 *
 * After the boot screens (src/core/task/boot.c) task 0 restarts at the title (0x0888B398,
 * src/core/task/title.c). Its first screen after boot is 8, which auto-loads the SYSTEM DATA (and,
 * with Silent saves, a3mx.dat: sysdata.c); its step 6 clears the screen number, so the next frame runs
 * screen 0 (0x0888B4D8), which goes on to 1: the logos, the story, then the attract demos until a
 * button is pressed and screen 7 restarts task 0 at the MODE MENU (0x0888C268,
 * src/core/task/mode_menu.c). The MODE MENU's own screen 1 (0x0888C3A0) is the title picture with
 * PRESS START BUTTON; Start there (once its fade-in is done) opens the menu, screen 2.
 *
 * Two steps, each once per run:
 *   - the first time the title's screen 0 runs, it is skipped and screen 7 chosen instead: the game's
 *     own way from the title to the MODE MENU, as if a button had been pressed in the attract mode;
 *   - the first time the MODE MENU's screen 1 has faded in and is still waiting, it does what Start
 *     does there (on to screen 2, the object list cleared).
 * So the boot screens and the load still happen. When the MODE MENU is later left alone and goes back
 * to the attract mode, the title and PRESS START run as usual. */
#include "rt.h"

#define TO_MODE_MENU  7                     /* the title's screen 7: on to the MODE MENU */
#define G_FADE_TASK   0x5D                  /* s8: a fade is running (fade.c) */
void f_08823304(Cpu *c);                    /* clear the object list (src/core/object/lists.c) */

static int title_done, start_pending;

/* Around the title's screen 0 (0x0888B4D8, a0 = the game state): 1 = skipped */
int bootmenu_title_first(Cpu *c, uint32_t g, int after) {
    (void)c;
    if (after || title_done) return 0;
    title_done = 1; start_pending = 1;
    WH(g + 0, TO_MODE_MENU); WH(g + 2, 0);
    rt_log("boot_to_menu: straight to the MODE MENU");
    return 1;
}

/* After the MODE MENU's screen 1 (0x0888C3A0, a0 = the game state): Start, once, when it would count */
int bootmenu_press_start(Cpu *c, uint32_t g, int after) {
    if (!after || !start_pending) return 0;
    if (RH(g + 0) != 1) { start_pending = 0; return 0; }   /* (left some other way: Start pressed, timed out) */
    if ((int8_t)RB(g + G_FADE_TASK) != 0) return 0;          /* still fading in */
    start_pending = 0;
    WH(g + 0, 2);
    c->r[31] = 0; f_08823304(c);
    return 0;
}
