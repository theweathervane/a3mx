/* The boot screens: what task 0 runs first (task.c screens_task calls one a frame from the table at
 * 0x08A02EB0, by the screen number at GAME + 0), before it restarts at the title (0x0888B398).
 *
 *   0  reset the game state; the screen black (fade level 1, alpha 255)
 *   1  fade in                                  2  wait for the fade
 *   3  set-up, then fade out (with BUILD_OPTION set, a timed hold instead: not in the shipped game)
 *   4  wait for the fade, set-up                5  wait, then fade in
 *   6  wait, then task 0 restarts at the title  (7: an empty function, 0x088059C8, never reached)
 */
#include "../decomp.h"
#include "task.h"
#include <string.h>

#define GAME          0x08E75910u
#define SCREEN        (GAME + 0x00)   /* u16 */
#define SCREEN_STEP   (GAME + 0x02)   /* u16 */
#define HOLD_TIMER    (GAME + 0x0C)   /* s16: screen 3's hold */
#define HOLD_TIMER_2  (GAME + 0x0E)   /* s16 */
#define GAME_KEEP     (GAME + 0x40)   /* s16: kept when the state is reset */
#define FADE_TASK     (GAME + 0x5D)   /* s8: -1 while a fade task runs (fade.c) */
#define GAME_SIZE     0xD8
#define BUILD_OPTION  0x08A05888u     /* s8: never written by the program: 0 in the shipped game, so the
                                         branches for non-zero (screen 3's hold) never run; a build option */
#define SCREENS_ON    0x08E75B98u     /* u8 */
#define FADE_LEVEL    0x08E75B9Cu     /* float (fade.c) */
#define FADE_ALPHA    0x08E75C44u     /* s16 (fade.c) */
#define COLOURS       0x08E75C48u     /* u16[4], then u16 at +8, +0xA, +0xC */
#define TITLE_TASK    0x0888B398u     /* task 0's next entry: the title and attract mode */

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int fade_task_running(void) { return (int8_t)*B(FADE_TASK) != 0; }

void f_089F3470(Cpu *c);   /* memset (libc/string.c) */
void f_08821FEC(Cpu *c);   /* game state: defaults */
void f_08814B4C(Cpu *c);   /* settings: defaults (first boot) */
void f_0880A39C(Cpu *c);   /* sprites: clear */
void f_089E0BD0(Cpu *c);   /* graphics: fade on */
void f_089E0BD8(Cpu *c);   /* graphics: the fade colour (0xAABBGGRR) */
void f_089E0BA8(Cpu *c);   /* graphics: the fade level (float, in $f12) */
void f_089E0B64(Cpu *c);   /* graphics: three colours */
void f_08813F44(Cpu *c);
void f_08813E70(Cpu *c);
void f_088156FC(Cpu *c);
void f_0880B57C(Cpu *c);
void f_08814260(Cpu *c);
void f_0880ABD4(Cpu *c);   /* fade.c: fade in */
void f_0880AA74(Cpu *c);   /* fade.c: fade out */
void f_08805614(Cpu *c);   /* task.c: restart the running task */

/* 0: everything in the game state back to zero but one field; the screen starts black. */
static void screen_reset(Cpu *c) {
    int16_t keep = (int16_t)*H(GAME_KEEP);
    CALL3(f_089F3470, GAME, 0, GAME_SIZE);
    *H(GAME_KEEP) = (uint16_t)keep;
    CALL0(f_08821FEC);
    if ((int8_t)*B(BUILD_OPTION) == 0) CALL0(f_08814B4C);
    *H(SCREEN) = 1;
    CALL0(f_0880A39C);
    *H(FADE_ALPHA) = 255;
    float one = 1.0f;
    memcpy(guest_ptr(FADE_LEVEL), &one, 4);
    CALL0(f_089E0BD0);
    CALL1(f_089E0BD8, 0xFF000000u);
    memcpy(&c->f[12], guest_ptr(FADE_LEVEL), 4);         /* the float argument */
    CALL0(f_089E0BA8);
    *B(FADE_TASK) = 0;
    CALL3(f_089E0B64, 0xFF000000u, 0xFF000000u, 0xFF000000u);
}

/* 1: fade in, then screen 2. */
static void screen_fade_in(Cpu *c) {
    CALL0(f_0880ABD4);
    *H(SCREEN) = 2;
    *H(HOLD_TIMER) = 1;
    *H(HOLD_TIMER_2) = 1;
}

/* 2: once the fade is done, the next screen. */
static void screen_wait_fade(void) {
    if (!fade_task_running()) *H(SCREEN) = (uint16_t)(*H(SCREEN) + 1);
}

/* 3: set-up and a fade out. (With BUILD_OPTION set: a hold of HOLD_TIMER frames instead, kept as the
   PSP code has it although the shipped game never takes it.) */
static void screen_hold(Cpu *c) {
    if ((int8_t)*B(BUILD_OPTION) != 0) {
        int16_t left = (int16_t)(*H(HOLD_TIMER) - 1);
        *H(HOLD_TIMER) = (uint16_t)left;
        if (left != 0) return;
        CALL0(f_08813F44);
        *H(HOLD_TIMER) = 0;
        *H(SCREEN) = 4;
        CALL0(f_0880B57C);
        return;
    }
    for (int i = 0; i < 4; i++) *H(COLOURS + 2 * i) = 0xF0F0;
    CALL0(f_08813E70);
    *H(COLOURS + 8) = 0x8080;
    *H(COLOURS + 0xA) = 0x8080;
    *H(COLOURS + 0xC) = 0;
    *H(HOLD_TIMER) = 0;
    *H(SCREEN) = 4;
    CALL0(f_088156FC);
    CALL0(f_0880B57C);
    CALL0(f_0880AA74);
}

/* 4: once the fade is done: screen 5, after set-up. */
static void screen_setup(Cpu *c) {
    if (fade_task_running()) return;
    *H(SCREEN) = 5;
    *H(SCREEN_STEP) = 0;
    *B(SCREENS_ON) = 0;
    CALL0(f_0880A39C);
    CALL0(f_08814260);
}

/* 5: once the fade is done: fade in, screen 6. */
static void screen_fade_in_again(Cpu *c) {
    if (fade_task_running()) return;
    *H(SCREEN) = 6;
    *H(SCREEN_STEP) = 0;
    CALL0(f_0880ABD4);
}

/* 6: once the fade is done, task 0 starts again at the title. */
static void screen_to_title(Cpu *c) {
    if (fade_task_running()) return;
    CALL1(f_08805614, TITLE_TASK);
}

/* ---------------------------------------------------------------- adapters (frames as the originals') */
#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
SFA_FUNC(08805734, game_screen_reset)          { FRAME(0x20, screen_reset(c)); }
SFA_FUNC(088057F0, game_screen_fade_in)        { FRAME(0x10, screen_fade_in(c)); }
SFA_FUNC(08805828, game_screen_wait_fade)      { screen_wait_fade(); }
SFA_FUNC(08805850, game_screen_hold)           { FRAME(0x10, screen_hold(c)); }
SFA_FUNC(08805920, game_screen_setup)          { FRAME(0x10, screen_setup(c)); }
SFA_FUNC(08805964, game_screen_fade_in_again)  { FRAME(0x10, screen_fade_in_again(c)); }
SFA_FUNC(08805998, game_screen_to_title)       { FRAME(0x20, screen_to_title(c)); }
