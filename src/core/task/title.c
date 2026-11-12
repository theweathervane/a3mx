/* The title task: task 0's entry after the boot screens (0x0888B398), until the MODE MENU replaces
 * it. Every frame it runs the current title screen from the table at 0x08A26BA4 (screen number at
 * GAME + 0, each screen's own step at GAME + 2), then a common step that lets a button press end the
 * attract mode (0x0888B92C).
 *
 *   0  first pass: on to 1, or to 4 when GAME + 0x52 asks       1  the title (logos, then "the story")
 *   2  attract: demo 1          3  attract: demo 2               4  attract: demo 3
 *   5  attract: demo 4          6  next attract cycle (back to 1)
 *   7  a button was pressed: on to the MODE MENU (task 0 restarts at 0x0888C268)
 *   8  PRESS START (the first title after boot, and after the attract mode)
 *
 * The screens follow one pattern: step 0 sets up and fades in, then a sub-handler or a timer runs,
 * then a fade out, and once the fade is done the next screen (next_title_screen).
 */
#include "../decomp.h"
#include "task.h"

#define GAME           0x08E75910u
#define STAGE_NUMBER   0x092E1228u     /* s16 (docs/STAGES.md) */
#define TITLE_SCREENS  0x08A26BA4u     /* void (*[9])(Game *, s16 *stage) */
#define TITLE_SEEN     0x08A26BA0u     /* s8: set once PRESS START has been shown; until then the title task starts there */
#define LOGOS_SEEN     0x08A0589Cu     /* s8: the logo part of the title shown once; later titles skip it */
#define PADS           0x08E75BA0u     /* the players' pad states: +0 held (u16), +12 pressed (u32), then 44 bytes a player */
#define TITLE_CHOICE   0x09525880u     /* s8: the PRESS START menu's state */
#define CAMERA_LAYER   0x092E8800u     /* the stage's layers, 0xA0 bytes each (docs/STAGES.md) */
#define DEMO_SCRATCH   0x092DF980u
#define FIGHTER_C      0x092D9E00u     /* (loader.h FIGHTER_C_TABLE) */

/* game state fields used here (GAME + n) */
#define G_SCREEN        0x00   /* u16 */
#define G_STEP          0x02   /* u16 */
#define G_TIMER         0x0C   /* s16 */
#define G_TIMER_2       0x0E   /* s16 */
#define G_FRAME         0x21   /* u8: counts every frame of the title task */
#define G_PRESSED       0x35   /* s8: a button that ends the attract mode was pressed (bits: 1 pad 1, 2 pad 2, 4 both) */
#define G_PRESS_OK      0x31   /* s8: presses count even while G_PRESSED is set */
#define G_SKIP_TO_DEMO  0x52   /* s8 */
#define G_FADE_TASK     0x5D   /* s8 (fade.c) */
#define G_DRAW_FLAGS    0x5F   /* u8 */
#define G_TITLE_MUSIC   0xBD   /* u8 */

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int fading(gaddr g) { return (int8_t)*B(g + G_FADE_TASK) != 0; }

void f_088054F0(Cpu *c);   /* task.c: wait for frames */
void f_08805614(Cpu *c);   /* task.c: restart the running task */
void f_0880ABD4(Cpu *c);   /* fade.c: fade in */
void f_0880AA74(Cpu *c);   /* fade.c: fade out */
void f_08804A74(Cpu *c);   /* task.c: frame pacing */
void f_08821FEC(Cpu *c);   /* game state: defaults */
void f_0880A39C(Cpu *c);   /* sprites: clear */
void f_0880B57C(Cpu *c);
void f_08821368(Cpu *c);   /* every frame of the title task */
void f_089E40B4(Cpu *c);   /* music: play (track) */
void f_0880A6CC(Cpu *c);   /* text: draw (string, ...) */
void f_0880B6CC(Cpu *c);   /* text: flush */
void f_0888BB24(Cpu *c);   /* the copyright, under the title (a0 1) */
void f_089E11C8(Cpu *c);   /* copy */
void f_08813C04(Cpu *c); void f_08813D04(Cpu *c); void f_08813770(Cpu *c); void f_08814574(Cpu *c);
void f_08814320(Cpu *c); void f_0881369C(Cpu *c); void f_08812DDC(Cpu *c); void f_0881494C(Cpu *c);
void f_0881CD20(Cpu *c); void f_0887C724(Cpu *c); void f_089E6050(Cpu *c); void f_089E600C(Cpu *c);
void f_08814B88(Cpu *c); void f_088AC618(Cpu *c); void f_08813744(Cpu *c); void f_0881CFD0(Cpu *c);
void f_08822EAC(Cpu *c); void f_0887C6E4(Cpu *c); void f_08837690(Cpu *c); void f_08823338(Cpu *c);
void f_08816BBC(Cpu *c); void f_08816F5C(Cpu *c); void f_08887AFC(Cpu *c);
void f_088A9204(Cpu *c);   /* demo 1's own steps */
void f_088BDDA8(Cpu *c);   /* demo 2's own steps */
void f_088BBFA0(Cpu *c);   /* demo 3's own steps (Game *) */
void f_088B2E2C(Cpu *c);   /* demo 4's own steps */
void f_0888BBB4(Cpu *c);   /* the PRESS START menu: -1 back, 1 chosen, else still choosing */
void f_0888BCC8(Cpu *c);   /* the PRESS START menu: draw */
void f_0888B92C(Cpu *c);
void f_0888BA08(Cpu *c);
void rt_call_indirect(Cpu *c, uint32_t addr);

#define MODE_MENU_TASK  0x0888C268u
#define TEXT_TITLE      0x089FEFD0u
#define TEXT_LOGO_1     0x089FF048u
#define TEXT_LOGO_2     0x089FF05Cu

/* The screen is done: the next one, from its step 0, with the game state's defaults. */
static void next_title_screen(Cpu *c, gaddr g) {
    uint16_t screen = *H(g + G_SCREEN);
    *H(g + 0x0A) = 0;
    *H(g + G_SCREEN) = (uint16_t)(screen + 1);
    *H(g + 0x08) = 0;
    *H(g + 0x06) = 0;
    *H(g + 0x04) = 0;
    *H(g + G_STEP) = 0;
    CALL0(f_08821FEC);
    CALL0(f_0880A39C);
}

static void set_step(gaddr g, uint32_t step) { *H(g + G_STEP) = (uint16_t)step; }
static void next_step(gaddr g) { *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1); }
/* a 16-bit countdown: true when it reaches 0 */
static int count_down(gaddr field) {
    int16_t left = (int16_t)(*H(field) - 1);
    *H(field) = (uint16_t)left;
    return left == 0;
}

/* ---------------------------------------------------------------- the task */

static void title_task(Cpu *c) {
    const gaddr g = GAME;
    *B(g + 0x23) = 1;
    *H(g + 0x06) = 0; *H(g + 0x04) = 0; *H(g + G_STEP) = 0; *H(g + G_SCREEN) = 0;
    if ((int8_t)*B(TITLE_SEEN) == 0) *H(g + G_SCREEN) = 8;
    *B(g + 0x7D) = 0; *H(g + 0x70) = 0; *B(g + 0x38) = 0; *B(g + 0x48) = 0; *B(g + 0x2B) = 0;
    *B(g + 0x5E) = 0; *B(g + G_FADE_TASK) = 0; *B(g + 0x5C) = 0; *B(g + 0x37) = 0; *B(g + 0x29) = 0;
    *B(g + G_PRESSED) = 0; *B(g + 0x2A) = 0;
    *H(STAGE_NUMBER) = 0;
    *H(0x0951CE52u) = 0;
    *B(0x09358400u) = 0;
    *B(0x09525DE3u) = 0;
    *W(CAMERA_LAYER + 0x140 + 0x60) = 0; *W(CAMERA_LAYER + 0x140 + 0x10) = 0;
    *W(CAMERA_LAYER + 0xA0 + 0x60) = 0;  *W(CAMERA_LAYER + 0xA0 + 0x10) = 0;
    *H(CAMERA_LAYER + 0x62) = 0;          *H(CAMERA_LAYER + 0x12) = 0;
    CALL0(f_0880B57C);
    CALL1(f_088054F0, 1);
    for (;;) {
        uint16_t screen = *H(g + G_SCREEN);
        *B(g + G_FRAME) = (uint8_t)(*B(g + G_FRAME) + 1);
        *B(g + G_DRAW_FLAGS) = 0x1F;
        gaddr fn = W(TITLE_SCREENS)[screen];
        c->r[4] = g; c->r[5] = STAGE_NUMBER;
        rt_call_indirect(c, fn);
        CALL2(f_0888B92C, g, STAGE_NUMBER);
        CALL0(f_08821368);
        CALL1(f_088054F0, 1);
    }
}

/* Reads the pads for a press that ends the attract mode: Start (pressed) on pad 1 or pad 2, or the
   three buttons held on ... (bits 1, 2, 4), into G_PRESSED. */
static void read_title_press(gaddr g) {
    uint8_t press = 0;
    if (*W(PADS + 12) & 0x8000) press = 1;
    if (*W(PADS + 56) & 0x8000) press = (uint8_t)(press | 2);
    uint16_t held = (uint16_t)(*H(PADS) & *H(PADS + 44) & *H(PADS + 88));
    if (held != 0 && (*W(PADS + 100) & 0x8000)) press = (uint8_t)(press | 4);
    if (press == 0) return;
    int8_t ok = (int8_t)*B(g + G_PRESS_OK), pressed = (int8_t)*B(g + G_PRESSED);
    if (ok != 0 || pressed == 0) *B(g + G_PRESSED) = (uint8_t)(pressed | press);
}

/* After every title screen: during the attract mode (screens 2 - 6), a press ends it: fade out, then
   screen 7 (the MODE MENU). */
static void title_step(Cpu *c, gaddr g, gaddr stage) {
    uint16_t screen = *H(g + G_SCREEN);
    if (screen == 0 || screen == 1 || screen == 8) return;
    uint16_t phase = *H(g + 0x08);
    if (phase == 0) {
        if (!fading(g)) CALL2(f_0888BA08, g, stage);
        if ((int8_t)*B(g + G_PRESSED) == 0) return;
        *H(g + 0x08) = (uint16_t)(*H(g + 0x08) + 1);
        CALL0(f_0880AA74);
    } else if (phase == 1) {
        if (fading(g)) return;
        *H(g + G_SCREEN) = 7;
        *H(g + 0x08) = (uint16_t)(phase + 1);
        *B(GAME + G_TITLE_MUSIC) = 0;
        CALL1(f_089E40B4, (uint32_t)(int32_t)(int16_t)*H(0x08A058F8u));
    }
}

/* ---------------------------------------------------------------- the screens */

/* 0: on to screen 1 (or straight to 4 when G_SKIP_TO_DEMO is set). */
static void screen_first(Cpu *c, gaddr g) {
    if ((int8_t)*B(g + G_SKIP_TO_DEMO) != 0) {
        *B(g + G_SKIP_TO_DEMO) = 0;
        *H(g + G_SCREEN) = 4;
    } else {
        *H(g + G_SCREEN) = (uint16_t)(*H(g + G_SCREEN) + 1);
    }
    *H(g + G_STEP) = 0;
    CALL0(f_08821FEC);
}

/* 1: the title: logos (once), then the rest, timed; fades between. */
static void screen_title(Cpu *c, gaddr g) {
    uint16_t step = *H(g + G_STEP);
    switch (step) {
    case 0:
        set_step(g, 1);
        CALL0(f_08821FEC);
        *B(GAME + G_TITLE_MUSIC) = 1;
        CALL1(f_089E40B4, 0);
        CALL0(f_08813C04);
        CALL3(f_089E11C8, DEMO_SCRATCH, *W(FIGHTER_C + 56), 0x200);
        *W(0x092E0980u) = 1;
        CALL0(f_0880ABD4);
        break;
    case 1:
        if (!fading(g)) set_step(g, 2);
        break;
    case 2:
        CALL2(f_0881CFD0, 0x80, 0x60);
        if (fading(g)) break;
        next_step(g);
        *H(g + G_TIMER) = 0xB4;
        break;
    case 3:
        CALL2(f_0881CFD0, 0x80, 0x60);
        if (!count_down(g + G_TIMER)) break;
        next_step(g);
        CALL0(f_0880AA74);
        break;
    case 4:
        CALL2(f_0881CFD0, 0x80, 0x60);
        if (fading(g)) break;
        if ((int8_t)*B(LOGOS_SEEN) != 0) {
            set_step(g, 8);
        } else {
            *B(LOGOS_SEEN) = 1;
            next_step(g);
        }
        break;
    case 5:
        set_step(g, 6);
        CALL2(f_08822EAC, 0x7A, 0);
        CALL1(f_0887C6E4, 0x106);
        CALL0(f_0880ABD4);
        break;
    case 6:
        if (fading(g)) break;
        set_step(g, 7);
        *H(g + G_TIMER) = 0xF0;
        break;
    case 7:
        if (!count_down(g + G_TIMER)) break;
        set_step(g, 8);
        CALL0(f_0880AA74);
        break;
    case 8:
        if (fading(g)) break;
        next_title_screen(c, g);
        *B(GAME + G_TITLE_MUSIC) = 0;
        CALL1(f_089E40B4, (uint32_t)(int32_t)(int16_t)*H(0x08A058F8u));
        break;
    }
    uint16_t now = *H(g + G_STEP);
    if ((uint16_t)(now - 3) < 6 && now != 5) CALL0(f_08837690);
    CALL5(f_0880A6CC, TEXT_LOGO_1, 0, 0x74, 0xC2, 0);
    CALL5(f_0880A6CC, TEXT_LOGO_2, 0, 0x18, 0xD6, 0);
    CALL0(f_0880B6CC);
}

/* 2: attract demo 1. */
static void screen_demo_1(Cpu *c, gaddr g) {
    switch (*H(g + G_STEP)) {
    case 0:
        set_step(g, 1);
        CALL1(f_0880ABD4, g);
        CALL0(f_08813770);
        CALL0(f_08814574);
        CALL0(f_08814320);
        CALL0(f_0881369C);
        CALL1(f_08812DDC, 0x13);
        CALL1(f_08812DDC, 4);
        CALL0(f_0881494C);
        CALL1(f_0881CD20, 0);
        CALL1(f_089E6050, CALL1(f_0887C724, 0x105) & 0xFF);
        CALL0(f_089E600C);
        break;
    case 1: if (!fading(g)) set_step(g, 2); break;
    case 2: CALL1(f_088A9204, g); break;
    case 3: set_step(g, 4); CALL1(f_0880AA74, g); break;
    case 4: if (!fading(g)) next_title_screen(c, g); break;
    }
}

/* 3: attract demo 2. */
static void screen_demo_2(Cpu *c, gaddr g) {
    uint16_t step = *H(g + G_STEP);
    if (step == 0) {
        set_step(g, 1);
        CALL1(f_0880ABD4, g);
    } else if (step == 1) {
        if (!fading(g)) set_step(g, 2);
    } else if (step == 2) {
        CALL1(f_088BDDA8, 1);
    }
}

/* 4: attract demo 3. */
static void screen_demo_3(Cpu *c, gaddr g) {
    switch (*H(g + G_STEP)) {
    case 0: set_step(g, 1); CALL1(f_0880ABD4, 1); break;
    case 1:
        if (fading(g)) break;
        CALL0(f_08814B88);
        next_step(g);
        *H(g + G_TIMER) = 0xB4;
        break;
    case 2: CALL1(f_088BBFA0, g); break;
    case 3: set_step(g, 4); CALL1(f_0880AA74, 4); break;
    case 4: if (!fading(g)) next_title_screen(c, g); break;
    }
}

/* 5: attract demo 4, with its caption. */
static void screen_demo_4(Cpu *c, gaddr g) {
    switch (*H(g + G_STEP)) {
    case 0:
        set_step(g, 1);
        CALL0(f_08813D04);
        CALL3(f_089E11C8, DEMO_SCRATCH, *W(FIGHTER_C + 40), 0x200);
        *W(0x092E0980u) = 1;
        CALL0(f_0880ABD4);
        break;
    case 1: if (!fading(g)) set_step(g, 2); break;
    case 2: CALL1(f_088B2E2C, g); break;
    case 3: set_step(g, 4); CALL0(f_0880AA74); break;
    case 4: if (!fading(g)) next_title_screen(c, g); break;
    }
    CALL5(f_0880A6CC, TEXT_TITLE, 0, 0x50, 0x50, 8);
    CALL0(f_0880B6CC);
}

/* 6: the attract mode starts over at screen 1, alternating ... (0x09525DE3) and counting cycles
   (0x09358400, 0 - 2). */
static void screen_cycle(gaddr g) {
    *H(g + G_SCREEN) = 1;
    int8_t cycle = (int8_t)(*B(0x09358400u) + 1);
    *B(0x09525DE3u) = (uint8_t)((int8_t)*B(0x09525DE3u) ^ 1);
    *B(0x09358400u) = (uint8_t)cycle;
    if (cycle >= 3) *B(0x09358400u) = 0;
    *H(g + 0x0A) = 0; *H(g + 0x08) = 0; *H(g + 0x06) = 0; *H(g + 0x04) = 0; *H(g + G_STEP) = 0;
    *H(g + G_TIMER) = 0;
}

/* 7: on to the MODE MENU: the title's state cleared, task 0 restarts there. */
static void screen_to_mode_menu(Cpu *c, gaddr g) {
    *B(g + G_PRESSED) = 1;
    if ((int8_t)*B(g + G_PRESSED) == 0) return;
    for (int off = 0x16; off >= 0; off -= 2) *H(g + (uint32_t)off) = 0;
    CALL1(f_0880A39C, g);
    CALL0(f_08821FEC);
    CALL0(f_088AC618);
    CALL1(f_08804A74, 0);
    CALL0(f_08813744);
    CALL1(f_08805614, MODE_MENU_TASK);
}

/* 8: PRESS START, and its menu. */
static void screen_press_start(Cpu *c, gaddr g) {
    switch (*H(g + G_STEP)) {
    case 0:
        CALL1(f_089E40B4, (uint32_t)(int32_t)(int16_t)*H(0x08A058F8u));
        next_step(g);
        *B(TITLE_SEEN) = 1;
        *B(TITLE_CHOICE) = 0;
        CALL0(f_0880ABD4);
        break;
    case 1: {
        if (fading(g)) break;
        set_step(g, 2);
        *H(g + G_TIMER_2) = 0x3C;
        if ((int8_t)*B(TITLE_CHOICE) == 0) next_step(g);
        CALL2(f_08822EAC, 0x88, 5);
        gaddr obj = CALL2(f_08822EAC, 0x88, 5);
        *B(obj + 76) = 1;
        CALL2(f_08823338, 0x18, 0);
        break;
    }
    case 2:
        if ((*W(PADS + 12) & 0x80) == 0) break;
        set_step(g, 3);
        *B(TITLE_CHOICE) = 0;
        break;
    case 3:
        if (!count_down(g + G_TIMER_2)) break;
        set_step(g, 4);
        *H(g + G_TIMER_2) = 0xB4;
        *B(TITLE_CHOICE) = (uint8_t)CALL0(f_08816BBC);
        break;
    case 4: {
        if (!count_down(g + G_TIMER_2) && ((*W(PADS + 12) | *W(PADS + 56)) & 0x8080) == 0) break;
        int8_t choice = (int8_t)*B(TITLE_CHOICE);
        if (choice != 2 && choice < 7) {
            set_step(g, 6);
            CALL0(f_0880AA74);
        } else {
            *B(0x09523C69u) = 0xFF;
            set_step(g, 5);
            *B(TITLE_CHOICE) = 0x0E;
        }
        break;
    }
    case 5: {
        int32_t r = (int32_t)CALL0(f_0888BBB4);
        if (r == 1) {
            next_step(g);
            CALL0(f_0880AA74);
        } else if (r == -1) {
            set_step(g, 3);
            *H(g + G_TIMER_2) = 0x3C;
            *B(TITLE_CHOICE) = 0;
        }
        break;
    }
    case 6:
        if (fading(g)) break;
        for (int off = 0x16; off >= 0; off -= 2) *H(g + (uint32_t)off) = 0;
        CALL0(f_08821FEC);
        CALL0(f_0880A39C);
        CALL0(f_08816F5C);
        return;
    }
    CALL0(f_0888BCC8);
    CALL0(f_08837690);
    CALL0(f_08887AFC);
    CALL0(f_0880B6CC);
}

/* ---------------------------------------------------------------- adapters (frames as the originals') */
#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)

/* 0x0888BCC8: the game's name over the boot screens, in the large font (font 2, colour 4: green),
   "STREET FIGHTER ALPHA3" with "MAX" centred under it, then the copyright (0x0888BB24). Every frame of
   the system data load (0x08816BBC) and of PRESS START (screen_press_start). Wrapped by
   src/platform/bootscreen.c, which runs with or without extensions: the version in the top right
   corner always, and A3MX in place of the name when any extension is on. */
#define TITLE_LINE_1   0x089FF02Cu     /* "STREET FIGHTER ALPHA3" */
#define TITLE_LINE_2   0x089FF044u     /* "MAX" */
#define FONT_LARGE     2
#define TITLE_GREEN    4
static void draw_title(Cpu *c) {
    CALL5(f_0880A6CC, TITLE_LINE_1, FONT_LARGE, 0x18, 0x20, TITLE_GREEN);   /* 21 letters, 16 wide: centred on 192 */
    CALL5(f_0880A6CC, TITLE_LINE_2, FONT_LARGE, 0xA8, 0x38, TITLE_GREEN);
    CALL1(f_0888BB24, 1);
}
SFA_FUNC(0888B398, game_title_task)            { FRAME(0x20, title_task(c)); }
SFA_FUNC(0888B92C, game_title_step)            { FRAME(0x20, title_step(c, ARG(0), ARG(1))); }
SFA_FUNC(0888BA08, game_read_title_press)      { read_title_press(ARG(0)); }
SFA_FUNC(0888B4D8, game_title_screen_first)    { FRAME(0x10, screen_first(c, ARG(0))); }
SFA_FUNC(0888BD1C, game_title_screen_title)    { FRAME(0x30, screen_title(c, ARG(0))); }
SFA_FUNC(0888B518, game_title_screen_demo_1)   { FRAME(0x20, screen_demo_1(c, ARG(0))); }
SFA_FUNC(0888B650, game_title_screen_demo_2)   { FRAME(0x10, screen_demo_2(c, ARG(0))); }
SFA_FUNC(0888B6C8, game_title_screen_demo_3)   { FRAME(0x20, screen_demo_3(c, ARG(0))); }
SFA_FUNC(0888B7B8, game_title_screen_demo_4)   { FRAME(0x30, screen_demo_4(c, ARG(0))); }
SFA_FUNC(0888B8D4, game_title_screen_cycle)    { screen_cycle(ARG(0)); }
SFA_FUNC(0888BA9C, game_title_screen_to_menu)  { FRAME(0x20, screen_to_mode_menu(c, ARG(0))); }
SFA_FUNC_WRAPPED(0888BFBC, game_title_screen_press_start, bootmsg_load_screen) { FRAME(0x20, screen_press_start(c, ARG(0))); }
SFA_FUNC_WRAPPED(0888BCC8, game_title_draw, bootscreen_title) { FRAME(0x30, draw_title(c)); }
