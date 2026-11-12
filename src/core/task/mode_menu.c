/* The MODE MENU task: task 0's entry once the title is left (0x0888C268, restarted there by the title's
 * screen 7 and by leaving a mode). Every frame it runs the current screen from the table at 0x08A26C8C
 * (screen number at GAME + 0), then waits for the next frame.
 *
 *   0  set up (0x0888C2DC): back from a mode with auto-save on: on to 4; else to 2 if a step was asked
 *      for, otherwise to 1
 *   1  press Start (0x0888C3A0): Start opens the menu (2); 1800 frames without it end the MODE MENU
 *      (the dispatch with -1: back to the title)
 *   2  the menu (0x0888C498): its own steps from 0x08A26CA0: 0 set up (0x0888C4CC), 1 running
 *      (0x0888C5B0)
 *   3  back to the title (0x0888CAFC): fade out, then task 0 restarts at the title task
 *   4  back from a mode with auto-save on (0x0888D2B0): save, questions if the save failed, then the
 *      menu (sysdata.c runs around it: the save screens kept off screen)
 *
 * The menu is 3 columns of 5 modes. The cursor (0x08B02711, 0 - 14, column * 5 + row) picks an entry of the mode table
 * at 0x08A26C68; the chosen mode goes to GAME + 0x72. A mode can be switched off in the table of
 * 19 flags at 0x09525DD0 (copied from 0x08A26C77, all on); the cursor skips modes that are off.
 */
#include "../decomp.h"
#include "task.h"

#define GAME           0x08E75910u
#define STAGE_NUMBER   0x092E1228u     /* s16 (docs/STAGES.md) */
#define MENU_SCREENS   0x08A26C8Cu     /* void (*[5])(Game *, s16 *stage) */
#define MENU_STEPS     0x08A26CA0u     /* screen 2's steps: void (*[2])(Game *, s16 *stage) */
#define MENU_REPEAT    0x08B02710u     /* s8: frames until a held direction moves the cursor again */
#define MENU_CURSOR    0x08B02711u     /* s8: 0 - 14, column * 5 + row */
#define MENU_MODES     0x08A26C68u     /* u8[15]: the mode at each cursor position */
#define MODE_ON        0x09525DD0u     /* u8[19]: by mode, 0 = off (the cursor skips it) */
#define MODE_ON_INIT   0x08A26C77u     /* u8[19]: MODE_ON's start values */
#define BACK_FROM_MODE 0x0951CE54u     /* u16: set when a mode returns to the MODE MENU */
#define AUTO_SAVE      0x08A058EFu     /* s8: OPTION's auto-save setting */
#define PADS           0x08E75BA0u     /* the players' pads, 44 bytes each: +0 held (u16), +4 repeat, +12 pressed */
#define MENU_PALETTES  0x08A26BC8u     /* 5 palettes of 16 colours */
#define MENU_PALETTE_9 0x08A35038u
#define DEMO_SCRATCH   0x092DF980u     /* the palette RAM copy (title.c) */
#define FIGHTER_C      0x092D9E00u     /* (loader.h FIGHTER_C_TABLE) */
#define TITLE_TASK     0x0888B398u
#define SAVE_RESULT    0x09525880u     /* s8: the last save's result (and title.c's TITLE_CHOICE) */
#define SAVE_QUESTION  0x09523C60u     /* the save task's state: +9 the Yes / No cursor (s8, 1 / -1) */

#define MENU_ROWS      5
#define MENU_ITEMS     15
#define REPEAT_FIRST   0x14            /* frames before a held direction repeats */
#define REPEAT_NEXT    6               /* then every 6 + 1 frames */
#define IDLE_FRAMES    0x708           /* screen 1: 30 seconds */

/* the screens */
enum { SCREEN_SETUP, SCREEN_PRESS_START, SCREEN_MENU, SCREEN_TO_TITLE, SCREEN_FROM_MODE };

/* game state fields used here (GAME + n) */
#define G_SCREEN       0x00   /* u16 */
#define G_STEP         0x02   /* u16 */
#define G_RETURN_STEP  0x04   /* u16: screen 4's state */
#define G_PHASE        0x08   /* u16 */
#define G_TIMER_2      0x0E   /* s16 */
#define G_SAVE_TIMER   0x10   /* s16: screen 4 */
#define G_MENU_SHOWN   0x34   /* s8 */
#define G_CHOSEN_BY    0x35   /* s8: who chose the mode: 1 pad 1, 2 pad 2 */
#define G_SKIP_TO_DEMO 0x52   /* s8 (title.c) */
#define G_FADE_TASK    0x5D   /* s8 (fade.c) */
#define G_MODE         0x72   /* s8: the mode chosen (hooks.c) */

/* pad buttons, as the game maps them (directions as on Capcom's arcade boards) */
#define PAD_RIGHT      0x0001
#define PAD_LEFT       0x0002
#define PAD_DOWN       0x0004
#define PAD_UP         0x0008
#define PAD_CANCEL     0x0100
#define PAD_START      0x8000
#define PAD_CONFIRM    0x8080

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int fading(gaddr g) { return (int8_t)*B(g + G_FADE_TASK) != 0; }
/* a 16-bit countdown: true when it reaches 0 */
static int count_down(gaddr field) {
    int16_t left = (int16_t)(*H(field) - 1);
    *H(field) = (uint16_t)left;
    return left == 0;
}

void f_088054F0(Cpu *c);   /* task.c: wait for frames */
void f_08805614(Cpu *c);   /* task.c: restart the running task */
void f_0880ABD4(Cpu *c);   /* fade.c: fade in */
void f_0880AA74(Cpu *c);   /* fade.c: fade out */
void f_08821FEC(Cpu *c);   /* game state: defaults */
void f_0880A39C(Cpu *c);   /* sprites: clear */
void f_088B8DFC(Cpu *c);   /* OPTION settings into the game state */
void f_0887C4CC(Cpu *c);   /* sound effect (0, id, volume) */
void f_0880AEA8(Cpu *c);   /* palette: load (0, colours, 0, slot, count of 16) */
void f_089E11C8(Cpu *c);   /* copy */
void f_089F3430(Cpu *c);   /* memcpy */
void f_08823304(Cpu *c);   /* clear the object list */
void f_08823338(Cpu *c);   /* objects: remove (kind, number) */
void f_08822EAC(Cpu *c);   /* objects: create (kind, type) */
void f_0888BB24(Cpu *c);   /* title.c's caption: draw (0: with PRESS START) */
void f_08813770(Cpu *c); void f_08812DDC(Cpu *c); void f_08814ED8(Cpu *c); void f_08813D04(Cpu *c);
void f_0881CBDC(Cpu *c); void f_088157A0(Cpu *c);
void f_08837690(Cpu *c); void f_08887AFC(Cpu *c); void f_0881EE84(Cpu *c); void f_0881A040(Cpu *c);
void f_0880B6CC(Cpu *c);   /* text: flush */
void f_088D2B54(Cpu *c); void f_088D2B5C(Cpu *c);   /* empty */
void f_0888CB7C(Cpu *c);
void f_0881725C(Cpu *c);   /* auto-save: runs frames of its own, returns the result */
void f_08817834(Cpu *c);   /* save task: (1, 0) after a save that was off or not needed */
void f_08816F5C(Cpu *c);
void f_0888CC78(Cpu *c);   /* the save error question, Yes / No: 1 Yes, -1 No or back, 0 still asking */
void f_089E3E00(Cpu *c); void f_089E32CC(Cpu *c);   /* memory stick: start a check, its state (-1 busy) */
void f_0888CD94(Cpu *c);   /* the dispatch, below */
void f_088054C8(Cpu *c);   /* task.c: start a task (slot, entry) */
void f_089F3470(Cpu *c);   /* memset */
void f_08822934(Cpu *c); void f_08814574(Cpu *c); void f_0884A210(Cpu *c); void f_0884A0B4(Cpu *c);
void f_0888CD8C(Cpu *c);   /* empty */
void f_08805564(Cpu *c);   /* task.c: kill a task (slot) */
void f_08809FC8(Cpu *c);   /* sound: stop the voices */
void f_08822BD8(Cpu *c);   /* clear the game state (0xD8 bytes at GAME) */
void f_0882E0D0(Cpu *c); void f_089E0B64(Cpu *c);
void f_08809934(Cpu *c);   /* waits for the sound to settle, a frame at a time */
void f_0888C85C(Cpu *c); void f_0888C90C(Cpu *c); void f_0888C9B0(Cpu *c); void f_0888CA5C(Cpu *c);
void rt_call_indirect(Cpu *c, uint32_t addr);

/* Buttons from pads 1 and 2, and pad 3's when all three pads hold a button (as title.c reads them). */
typedef struct { uint16_t pressed, repeat; } MenuPads;
static MenuPads read_menu_pads(int with_repeat) {
    MenuPads p;
    p.pressed = (uint16_t)(*W(PADS + 12) | *W(PADS + 56));
    p.repeat = with_repeat ? (uint16_t)(*W(PADS + 4) | *W(PADS + 48)) : 0;
    if ((uint16_t)(*H(PADS) & *H(PADS + 44) & *H(PADS + 88)) != 0) {
        p.pressed = (uint16_t)(p.pressed | *W(PADS + 100));
        if (with_repeat) p.repeat = (uint16_t)(p.repeat | *W(PADS + 92));
    }
    return p;
}

/* ---------------------------------------------------------------- the task */

static void mode_menu_task(Cpu *c) {
    const gaddr g = GAME;
    *B(MENU_REPEAT) = REPEAT_FIRST;
    CALL0(f_088B8DFC);
    CALL0(f_08821FEC);
    CALL0(f_0880A39C);
    for (;;) {
        gaddr fn = W(MENU_SCREENS)[*H(g + G_SCREEN)];
        c->r[4] = g; c->r[5] = STAGE_NUMBER;
        rt_call_indirect(c, fn);
        CALL1(f_088054F0, 1);
    }
}

/* ---------------------------------------------------------------- the screens */

/* 0: set up, and on to the screen that comes first. */
static void screen_setup(Cpu *c, gaddr g) {
    CALL0(f_08813770);
    CALL1(f_08812DDC, 0x13);
    CALL0(f_08814ED8);
    if (*H(BACK_FROM_MODE) != 0 && (int8_t)*B(AUTO_SAVE) != 0) {
        /* back from a mode with auto-save on: screen 4 saves first */
        *H(g + G_SCREEN) = SCREEN_FROM_MODE;
        *H(g + G_RETURN_STEP) = 0;
        CALL2(f_08823338, 6, 0x10);
    } else {
        *H(BACK_FROM_MODE) = 0;
        if (*H(g + G_STEP) != 0) {
            *H(g + G_SCREEN) = SCREEN_MENU;
            *H(g + G_STEP) = 0;
        } else {
            *H(g + G_SCREEN) = SCREEN_PRESS_START;
            *H(g + G_STEP) = 0;
            *H(g + G_TIMER_2) = IDLE_FRAMES;
            CALL0(f_0888CB7C);
            CALL3(f_0887C4CC, 0, 0x121, 0xC0);
            *B(g + G_MODE) = 0;
        }
    }
    CALL0(f_0880ABD4);
}

/* 1: press Start; left alone for 30 seconds, the MODE MENU gives up (the dispatch with -1). */
static void screen_press_start(Cpu *c, gaddr g, gaddr stage) {
    uint16_t pressed = read_menu_pads(0).pressed;
    if (!fading(g)) {
        CALL1(f_0888BB24, 0);
        int16_t left = (int16_t)(*H(g + G_TIMER_2) - 1);
        *H(g + G_TIMER_2) = (uint16_t)left;
        if (left == 0) {
            CALL3(f_0888CD94, g, stage, (uint32_t)-1);
        } else if (pressed & PAD_START) {
            *H(g + G_SCREEN) = (uint16_t)(*H(g + G_SCREEN) + 1);
            CALL0(f_08823304);
        }
    }
    CALL0(f_08837690);
    CALL0(f_08887AFC);
    CALL0(f_0881EE84);
    CALL0(f_0880B6CC);
    CALL0(f_0881A040);
}

/* 2: the menu, by its own step. */
static void screen_menu(Cpu *c, gaddr g, gaddr stage) {
    gaddr fn = W(MENU_STEPS)[*H(g + G_STEP)];
    c->r[4] = g; c->r[5] = stage;
    rt_call_indirect(c, fn);
}

/* 2, step 0: the menu's objects, the cursor on the mode chosen last time, and every mode on. */
static void menu_setup(Cpu *c, gaddr g) {
    if (fading(g)) return;
    *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1);
    for (uint32_t i = 0; i < 28; i++) CALL2(f_08823338, 6, i & 0xFF);
    CALL2(f_08822EAC, 0x82, 0);
    /* The cursor goes to the mode in G_MODE (signed) as found in the mode table (unsigned): one that is
       not there leaves the cursor at 15, past the last entry. */
    *B(MENU_CURSOR) = 0;
    for (int8_t at = (int8_t)*B(MENU_CURSOR); at < MENU_ITEMS; at = (int8_t)*B(MENU_CURSOR)) {
        if ((int8_t)*B(g + G_MODE) == (int32_t)B(MENU_MODES)[at]) break;
        *B(MENU_CURSOR) = (uint8_t)(at + 1);
    }
    *B(g + G_MENU_SHOWN) = 1;
    CALL0(f_088D2B54);
    CALL0(f_088157A0);
    CALL3(f_089F3430, MODE_ON, MODE_ON_INIT, 0x13);
}

/* ---------------------------------------------------------------- the cursor */

/* The cursor lands on `at`: the mode there becomes the choice, with the cursor's sound. */
static void cursor_to(Cpu *c, gaddr g, int8_t at) {
    *B(MENU_CURSOR) = (uint8_t)at;
    *B(g + G_MODE) = B(MENU_MODES)[(int8_t)*B(MENU_CURSOR)];
    CALL3(f_0887C4CC, 0, 0x38, 0xC0);
}
static int mode_on(int8_t at) { return B(MODE_ON)[B(MENU_MODES)[at]] != 0; }

/* Each move steps in its direction, wrapping round its column or row, until a mode that is on.
   (Remainders as the game's signed division gives them.) */
static void cursor_down(Cpu *c, gaddr g) {
    int8_t at = (int8_t)*B(MENU_CURSOR);
    do at = (int8_t)(at % MENU_ROWS == MENU_ROWS - 1 ? at - (MENU_ROWS - 1) : at + 1);
    while (!mode_on(at));
    cursor_to(c, g, at);
}
static void cursor_right(Cpu *c, gaddr g) {
    int8_t at = (int8_t)*B(MENU_CURSOR);
    do at = (int8_t)(at + MENU_ROWS < MENU_ITEMS ? at + MENU_ROWS : at - (MENU_ITEMS - MENU_ROWS));
    while (!mode_on(at));
    cursor_to(c, g, at);
}
static void cursor_up(Cpu *c, gaddr g) {
    int8_t at = (int8_t)*B(MENU_CURSOR);
    do at = (int8_t)(at % MENU_ROWS == 0 ? at + (MENU_ROWS - 1) : at - 1);
    while (!mode_on(at));
    cursor_to(c, g, at);
}
static void cursor_left(Cpu *c, gaddr g) {
    int8_t at = (int8_t)*B(MENU_CURSOR);
    do at = (int8_t)(at - MENU_ROWS >= 0 ? at - MENU_ROWS : at + (MENU_ITEMS - MENU_ROWS));
    while (!mode_on(at));
    cursor_to(c, g, at);
}

/* A held direction moves the cursor after REPEAT_FIRST frames, then every REPEAT_NEXT + 1. */
static int repeat_due(void) {
    int8_t left = (int8_t)(*B(MENU_REPEAT) - 1);
    *B(MENU_REPEAT) = (uint8_t)left;
    if (left >= 0) return 0;
    *B(MENU_REPEAT) = REPEAT_NEXT;
    return 1;
}

/* 2, step 1: the menu runs: move the cursor (pads 1 and 2, or 3), then confirm on pad 1 or 2, or
   cancel. Each goes to the dispatch, with the pad that confirmed (0, 1) or -1 for cancel. Only one
   direction counts a frame: a new press first (down, right, up, left), else the first held one. */
static void menu_running(Cpu *c, gaddr g, gaddr stage) {
    static const struct { uint16_t button; GuestFunc move; } dirs[4] = {
        { PAD_DOWN, f_0888C85C }, { PAD_RIGHT, f_0888C90C }, { PAD_UP, f_0888C9B0 }, { PAD_LEFT, f_0888CA5C },
    };
    MenuPads p = read_menu_pads(1);
    int done = 0;
    for (int i = 0; i < 4 && !done; i++) {
        if (!(p.pressed & dirs[i].button)) continue;
        *B(MENU_REPEAT) = REPEAT_FIRST;
        CALL1(dirs[i].move, g);
        done = 1;
    }
    for (int i = 0; i < 4 && !done; i++) {
        if (!(p.repeat & dirs[i].button)) continue;
        if (repeat_due()) CALL1(dirs[i].move, g);
        done = 1;
    }
    if (*W(PADS + 12) & PAD_CONFIRM) {
        *B(g + G_CHOSEN_BY) = 1;
        CALL3(f_0888CD94, g, stage, 0);
    } else if (*W(PADS + 56) & PAD_CONFIRM) {
        *B(g + G_CHOSEN_BY) = 2;
        CALL3(f_0888CD94, g, stage, 1);
    } else if (p.pressed & PAD_CANCEL) {
        CALL3(f_0888CD94, g, stage, (uint32_t)-1);
    }
    CALL0(f_08837690);
    CALL0(f_08887AFC);
    CALL0(f_0880B6CC);
    CALL0(f_088D2B5C);
}

/* 3: back to the title: fade out, then task 0 restarts at the title task. */
static void screen_to_title(Cpu *c, gaddr g) {
    uint16_t phase = *H(g + G_PHASE);
    if (phase == 0) {
        CALL0(f_0880AA74);
        *H(g + G_PHASE) = (uint16_t)(*H(g + G_PHASE) + 1);
    } else if (phase == 1 && !fading(g)) {
        CALL1(f_08812DDC, 0x12);
        *B(g + G_SKIP_TO_DEMO) = 0;
        *H(g + G_SCREEN) = 0;
        *H(g + G_PHASE) = 0;
        CALL1(f_08805614, TITLE_TASK);
    }
}

/* 4: back from a mode with auto-save on: save, then the MODE MENU from screen 0. A failed save asks
   what to do (G_RETURN_STEP 3 or 5, with 0x0888CC78); No tries the save again. */
enum { SAVE_START, SAVE_WAIT, SAVE_SHOWN, SAVE_ASK_RETRY, SAVE_DONE, SAVE_ASK_STICK };
enum { RESULT_OFF = 4, RESULT_SAVED = 5, RESULT_NO_STICK = 0x0C, RESULT_NOT_NEEDED = 0x10 };

static void save_start(gaddr g) {
    *H(g + G_RETURN_STEP) = SAVE_WAIT;
    *H(g + G_SAVE_TIMER) = 4;
    *B(SAVE_RESULT) = 4;
}
static void ask_retry(gaddr g) {
    *B(SAVE_QUESTION + 9) = 0xFF;
    *H(g + G_RETURN_STEP) = SAVE_ASK_RETRY;
    *B(SAVE_RESULT) = 0x0F;
}

static void screen_from_mode(Cpu *c, gaddr g) {
    switch (*H(g + G_RETURN_STEP)) {
    case SAVE_START: {
        if (fading(g)) break;
        save_start(g);
        CALL2(f_08822EAC, 0x88, 5);
        gaddr obj = CALL2(f_08822EAC, 0x88, 5);
        *B(obj + 76) = 1;
        CALL2(f_08823338, 0x18, 0);
        break;
    }
    case SAVE_WAIT:
        if (!count_down(g + G_SAVE_TIMER)) break;
        *H(g + G_RETURN_STEP) = SAVE_SHOWN;
        *H(g + G_SAVE_TIMER) = 0x78;
        *B(SAVE_RESULT) = (uint8_t)CALL0(f_0881725C);
        break;
    case SAVE_SHOWN: {
        /* the result shows until its timer runs out or pad 1 or 2 confirms */
        if (!count_down(g + G_SAVE_TIMER) && ((*W(PADS + 12) | *W(PADS + 56)) & PAD_CONFIRM) == 0) break;
        int8_t r = (int8_t)*B(SAVE_RESULT);
        if (r == RESULT_NO_STICK) {
            *B(SAVE_QUESTION + 9) = 0xFF;
            *H(g + G_RETURN_STEP) = SAVE_ASK_STICK;
            *B(SAVE_RESULT) = 0x13;
        } else if (r == RESULT_NOT_NEEDED || r == RESULT_OFF) {
            CALL2(f_08817834, 1, 0);
            *H(g + G_RETURN_STEP) = SAVE_DONE;
            CALL0(f_0880AA74);
        } else if (r == 6 || r >= 8) {
            ask_retry(g);
        } else {
            *H(g + G_RETURN_STEP) = SAVE_DONE;
            CALL0(f_0880AA74);
        }
        break;
    }
    case SAVE_ASK_RETRY: {
        int32_t answer = (int32_t)CALL0(f_0888CC78);
        if (answer == 1) {
            *H(g + G_RETURN_STEP) = (uint16_t)(*H(g + G_RETURN_STEP) + 1);
            CALL0(f_0880AA74);
        } else if (answer == -1) {
            save_start(g);
        }
        break;
    }
    case SAVE_DONE:
        if (fading(g)) break;
        *H(g + G_SCREEN) = SCREEN_SETUP;
        *H(g + G_RETURN_STEP) = 0;
        *H(BACK_FROM_MODE) = 0;
        CALL0(f_08816F5C);
        CALL0(f_08821FEC);
        CALL0(f_0880A39C);
        break;
    case SAVE_ASK_STICK: {
        int32_t answer = (int32_t)CALL0(f_0888CC78);
        if (answer == 1) {
            /* wait for the memory stick check, a frame at a time */
            CALL0(f_089E3E00);
            do CALL1(f_088054F0, 1);
            while ((int32_t)CALL0(f_089E32CC) == -1);
            ask_retry(g);
        } else if (answer == -1) {
            ask_retry(g);
        }
        break;
    }
    }
    CALL0(f_08837690);
    CALL0(f_08887AFC);
    CALL0(f_0880B6CC);
}

/* ---------------------------------------------------------------- the dispatch */

/* The modes (GAME + 0x72), as the MODE MENU's table and the dispatch's know them. 1 is set by a
   network battle; 6, 10, 13 and 19 have no place on the menu. */
enum {
    MODE_ARCADE = 0, MODE_NETWORK_BATTLE = 1, MODE_TRAINING = 2, MODE_WORLD_TOUR = 3, MODE_ENTRY = 4,
    MODE_OPTION = 5, MODE_6 = 6, MODE_SURVIVAL = 7, MODE_DRAMATIC = 8, MODE_FINAL_BATTLE = 9,
    MODE_10 = 10, MODE_SCORE_RANKING = 11, MODE_NETWORK = 12, MODE_13 = 13, MODE_EDIT = 14,
    MODE_VARIABLE = 15, MODE_FREE_BATTLE = 16, MODE_R_DRAMATIC = 17, MODE_KUMITE = 18, MODE_19 = 19,
    MODE_COUNT = 20,
};

#define SYS_OPTS        0x08A058A0u    /* system data (sysdata.c) */
#define UNLOCKS         (SYS_OPTS + 0x52)   /* s16: bit 0x4000 the Dramatic / Variable variants, bit 1 Final Battle's */
#define BATTLE_TASK     0x08833B6Cu    /* task 2 for a fight */
#define FIGHT_LOOP      0x088233ECu    /* task 0 while a mode fights */
#define WORLD_TOUR_TASK 0x08837A44u    /* also NETWORK's */
#define ENTRY_TASK      0x088CC5A4u
#define OPTION_TASK     0x088B5F48u
#define RANKING_TASK    0x088BEF70u
#define KUMITE_SETUP    0x0881749Cu    /* (mode 10) */
#define EDIT_TASK       0x088D41B0u
#define FIGHTER_1       0x092E3EA0u    /* fighter structs, 0x498 bytes each */
#define FIGHTER_2       0x092E4338u
#define TRAINING_STATE  0x09525888u    /* 0x500 bytes */
#define NETWORK_OPTS    0x0951C0D0u    /* NETWORK's copy of the settings (0x9FC bytes from 0x09519588) */
#define NETWORK_PAD     0x09525DE4u

#define G_VARIANT       0xBF    /* s8: the mode's variant (L + R held on confirm) */
#define G_TWO_PLAYERS   0x88    /* s8 */
#define G_OWN_TASK      0x89    /* s8: the mode runs a task of its own */

/* The mode starts fighting: task 2 runs the battle, task 0 the fight loop. */
static void start_fighting(Cpu *c) {
    CALL2(f_088054C8, 2, BATTLE_TASK);
    CALL1(f_08805614, FIGHT_LOOP);
}

/* L + R held when confirming picks a mode's variant: Dramatic, Variable and R.Dramatic once unlocked
   (bit 0x4000), Final Battle with bit 1. A variant gets its own sound. */
static void choose_sound(Cpu *c, gaddr g) {
    int8_t mode = (int8_t)*B(g + G_MODE);
    uint32_t sound = 0x39;
    if (((*W(PADS + 4) | *W(PADS + 48)) & 0x240) == 0x240 && mode >= 8 && mode < 18) {
        uint32_t which = W(0x089FF0D0u)[mode - 8];   /* 0x0888CEC4, 0x0888CF0C or 0x0888CF54 */
        if (which == 0x0888CEC4u && ((int32_t)(int16_t)*H(UNLOCKS) & 0x4000)) {
            CALL3(f_0887C4CC, 0, 0x3A, 0xC0);
            *B(g + G_VARIANT) = 1;
            return;
        }
        if (which == 0x0888CF0Cu && ((int32_t)(int16_t)*H(UNLOCKS) & 1)) {
            *B(g + G_VARIANT) = 1;
            CALL3(f_0887C4CC, 0, 0x3A, 0xC0);
            return;
        }
    }
    CALL3(f_0887C4CC, 0, sound, 0xC0);
}

/* A choice on the MODE MENU (who: the pad that confirmed, 0 or 1, or -1 for back): the menu's state is
   cleared, a frame passes, then -1 goes back to the title (screen 3) and a mode is started. */
static void dispatch(Cpu *c, gaddr g, gaddr stage, int8_t who) {
    CALL0(f_08823304);
    CALL0(f_08822934);
    *B(g + 50) = 0; *B(g + 72) = 0; *B(g + G_TWO_PLAYERS) = 0;
    *B(g + 43) = 0; *B(g + 42) = 0; *B(g + 41) = 0;
    *H(g + 22) = 0;
    *H(stage) = 0;
    for (uint32_t off = 20; off >= 2; off -= 2) *H(g + off) = 0;
    *B(g + G_FADE_TASK) = 0; *B(g + 92) = 0;
    *B(stage + 105) = 0; *B(stage + 155) = 0;
    *B(g + G_OWN_TASK) = 0;
    CALL0(f_0880A39C);
    CALL1(f_088054F0, 1);
    *B(g + 35) = 0;
    if (who < 0) {
        *H(g + G_SCREEN) = SCREEN_TO_TITLE;
        *B(g + G_MENU_SHOWN) = 0;
        return;
    }
    *H(g + G_SCREEN) = 0;
    *B(g + G_VARIANT) = 0;
    *B(stage + 90) = 0;
    *B(g + G_MENU_SHOWN) = 1;
    *B(FIGHTER_1 + 337) = 0;
    *B(FIGHTER_2 + 337) = 1;
    choose_sound(c, g);

    int8_t mode = (int8_t)*B(g + G_MODE);
    if ((uint32_t)(int32_t)mode >= MODE_COUNT) { CALL1(f_08812DDC, 0x12); return; }
    switch (mode) {
    case MODE_ARCADE: case MODE_DRAMATIC: case MODE_FINAL_BATTLE:
        CALL0(f_0888CD8C);
        CALL1(f_08812DDC, 0x12);
        *H(BACK_FROM_MODE) = 1;
        start_fighting(c);
        break;
    case MODE_13:
        CALL0(f_0888CD8C);
        CALL1(f_08812DDC, 0x12);
        *B(g + G_CHOSEN_BY) = 3;
        *B(g + G_TWO_PLAYERS) = 1;
        start_fighting(c);
        break;
    case MODE_NETWORK_BATTLE: case MODE_6:
        CALL0(f_0888CD8C);
        CALL1(f_08812DDC, 0x12);
        *B(g + G_CHOSEN_BY) = 1;
        *B(g + G_TWO_PLAYERS) = 1;
        start_fighting(c);
        break;
    case MODE_TRAINING:
        CALL0(f_0888CD8C);
        CALL1(f_08812DDC, 0x12);
        *B(g + G_TWO_PLAYERS) = 1;
        CALL3(f_089F3470, TRAINING_STATE, 0, 0x500);
        *H(TRAINING_STATE + 1278) = 0x2000;
        CALL0(f_0884A210);
        CALL0(f_0884A0B4);
        *H(0x0950FF60u + 44) = 0; *H(0x0950FF60u + 42) = 0;
        *B(0x0950FF60u + 41) = 0; *B(0x0950FF60u + 40) = 0;
        start_fighting(c);
        break;
    case MODE_SURVIVAL:
        *B(g + G_TWO_PLAYERS) = 1;
        *H(BACK_FROM_MODE) = 1;
        start_fighting(c);
        break;
    case MODE_WORLD_TOUR:
        CALL0(f_0888CD8C);
        *B(g + G_OWN_TASK) = 1;
        CALL1(f_08812DDC, 0x12);
        CALL0(f_08814574);
        CALL1(f_08805614, WORLD_TOUR_TASK);
        break;
    case MODE_ENTRY:
        CALL1(f_08805614, ENTRY_TASK);
        break;
    case MODE_OPTION:
        *H(BACK_FROM_MODE) = 1;
        CALL1(f_08805614, OPTION_TASK);
        break;
    case MODE_SCORE_RANKING:
        CALL0(f_0888CD8C);
        CALL1(f_08805614, RANKING_TASK);
        break;
    case MODE_10:
        CALL1(f_08805614, KUMITE_SETUP);
        break;
    case MODE_NETWORK: {
        int8_t pad = (int8_t)*B(g + G_CHOSEN_BY);
        *B(g + G_OWN_TASK) = 1;
        *B(NETWORK_PAD) = (uint8_t)pad;
        CALL3(f_089F3430, NETWORK_OPTS, 0x09519588u, 0x9FC);
        CALL1(f_08805614, WORLD_TOUR_TASK);
        break;
    }
    case MODE_EDIT:
        CALL0(f_08814574);
        CALL1(f_08805614, EDIT_TASK);
        break;
    case MODE_VARIABLE: case MODE_R_DRAMATIC:
        CALL1(f_08812DDC, 0x12);
        *H(BACK_FROM_MODE) = 1;
        start_fighting(c);
        break;
    case MODE_FREE_BATTLE:
        *B(g + G_TWO_PLAYERS) = 1;
        start_fighting(c);
        break;
    case MODE_KUMITE:
        *B(g + G_TWO_PLAYERS) = 1;
        *H(BACK_FROM_MODE) = 1;
        CALL1(f_08812DDC, 0x12);
        CALL3(f_089F3470, 0x09517A20u, 0, 4);
        start_fighting(c);
        break;
    case MODE_19:
        CALL1(f_08812DDC, 0x12);
        *B(g + G_CHOSEN_BY) = 3;
        *B(g + G_TWO_PLAYERS) = 1;
        start_fighting(c);
        break;
    }
}

/* ---------------------------------------------------------------- back to the MODE MENU */

/* Back to the MODE MENU from a mode (0x088325DC: quitting from the pause menu, the end of a mode,
   F2 and many more): the battle task is stopped, the game state cleared except the mode chosen and
   GAME + 0x40, then task 0 restarts at the MODE MENU, on the menu itself (G_STEP 1). */
static void back_to_mode_menu(Cpu *c, gaddr g) {
    CALL0(f_08809FC8);
    CALL1(f_08805564, 2);
    int16_t keep = (int16_t)*H(g + 0x40);
    int8_t mode = (int8_t)*B(g + G_MODE);
    CALL0(f_08822BD8);
    *H(g + G_STEP) = 1;
    CALL0(f_0882E0D0);
    CALL0(f_0880A39C);
    CALL0(f_08821FEC);
    *H(g + 0x40) = (uint16_t)keep;
    *B(g + G_MODE) = (uint8_t)mode;
    *B(g + G_MENU_SHOWN) = 0;
    CALL3(f_089E0B64, 0xFF000000u, 0xFF000000u, 0xFF000000u);
    CALL3(f_08809934, 0, 0, 0);
    CALL1(f_08805614, 0x0888C268u);
}

/* Screen 1's pictures: the background, its palettes (slots 1 - 4, 8 and 9). */
static void load_menu_pictures(Cpu *c) {
    CALL0(f_08813D04);
    CALL0(f_0881CBDC);
    CALL3(f_089E11C8, DEMO_SCRATCH, *W(FIGHTER_C + 40), 0x200);
    *W(0x092E0980u) = 1;
    CALL2(f_08823338, 0x1D, 0);
    static const uint8_t slots[5] = { 1, 2, 3, 4, 8 };
    for (uint32_t i = 0; i < 5; i++) CALL5(f_0880AEA8, 0, MENU_PALETTES + 0x20 * i, 0, slots[i], 1);
    CALL5(f_0880AEA8, 0, MENU_PALETTE_9, 0, 9, 1);
}

/* ---------------------------------------------------------------- adapters (frames as the originals') */
#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
#define CURSOR_FRAME(f) FRAME(0x20, f(c, ARG(0)))
SFA_FUNC(0888C268, game_mode_menu_task)            { FRAME(0x20, mode_menu_task(c)); }
SFA_FUNC(0888C2DC, game_mode_menu_screen_setup)    { FRAME(0x20, screen_setup(c, ARG(0))); }
SFA_FUNC(0888C3A0, game_mode_menu_screen_start)    { FRAME(0x20, screen_press_start(c, ARG(0), ARG(1))); }
SFA_FUNC(0888C498, game_mode_menu_screen_menu)     { FRAME(0x20, screen_menu(c, ARG(0), ARG(1))); }
SFA_FUNC(0888C4CC, game_mode_menu_setup)           { FRAME(0x20, menu_setup(c, ARG(0))); }
/* padicons.c: the pad icons are drawn while the menu runs */
SFA_FUNC_WRAPPED(0888C5B0, game_mode_menu_running, padicons_mode_menu) { FRAME(0x30, menu_running(c, ARG(0), ARG(1))); }
SFA_FUNC(0888C85C, game_mode_menu_cursor_down)     { CURSOR_FRAME(cursor_down); }
SFA_FUNC(0888C90C, game_mode_menu_cursor_right)    { CURSOR_FRAME(cursor_right); }
SFA_FUNC(0888C9B0, game_mode_menu_cursor_up)       { CURSOR_FRAME(cursor_up); }
SFA_FUNC(0888CA5C, game_mode_menu_cursor_left)     { CURSOR_FRAME(cursor_left); }
SFA_FUNC(0888CAFC, game_mode_menu_screen_to_title) { FRAME(0x20, screen_to_title(c, ARG(0))); }
SFA_FUNC(0888CB7C, game_mode_menu_load_pictures)   { FRAME(0x30, load_menu_pictures(c)); }
/* hooks.c: players 2 and 3 have to press Start again to join */
SFA_FUNC_WRAPPED(0888CD94, game_mode_menu_dispatch, hooks_mode_chosen) { FRAME(0x30, dispatch(c, ARG(0), ARG(1), (int8_t)ARG(2))); }
/* The save error question's Yes / No (0x0888CC78), from pads 1 and 2: Up puts the cursor on Yes (1),
   Down on No (-1), each with a click if it moved; cancel (circle) answers -1, confirm answers the
   cursor; either clears it. 0 while still asking. */
#define SND_CURSOR 0x38
#define SND_CANCEL 0x2F
#define SND_OK     0x39
static void move_answer(Cpu *c, int8_t to) {
    if ((int8_t)*B(SAVE_QUESTION + 9) == to) return;
    CALL3(f_0887C4CC, 0, SND_CURSOR, 0xC0);
    *B(SAVE_QUESTION + 9) = (uint8_t)to;
}
static int32_t save_question(Cpu *c) {
    uint32_t pressed = *W(PADS + 12) | *W(PADS + 56);
    if (pressed & 0x8) move_answer(c, 1);
    else if (pressed & 0x4) move_answer(c, -1);
    if (pressed & 0x100) {
        CALL3(f_0887C4CC, 0, SND_CANCEL, 0xC0);
        *B(SAVE_QUESTION + 9) = 0;
        return -1;
    }
    if (pressed & PAD_CONFIRM) {
        CALL3(f_0887C4CC, 0, SND_OK, 0xC0);
        int32_t answer = (int8_t)*B(SAVE_QUESTION + 9);
        *B(SAVE_QUESTION + 9) = 0;
        return answer;
    }
    return 0;
}

SFA_FUNC(0888CC78, game_save_question)            { int32_t a; FRAME(0x30, a = save_question(c)); RETURN(a); }
SFA_FUNC(0888CD8C, game_mode_menu_stub)            { }
SFA_FUNC(088325DC, game_back_to_mode_menu)         { FRAME(0x20, back_to_mode_menu(c, ARG(0))); }
/* sysdata.c: the auto-save screens kept off screen */
SFA_FUNC_WRAPPED(0888D2B0, game_mode_menu_screen_from_mode, sysdata_back_from_mode) { FRAME(0x20, screen_from_mode(c, ARG(0))); }
