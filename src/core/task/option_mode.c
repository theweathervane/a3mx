/* OPTION MODE: the mode's own task (0x088B5F48, started by the MODE MENU's dispatch), its menu's
 * pictures, GAME OPTION, BUTTON CONFIG and the pause menu's BUTTON CONFIG (scenarios option_tour,
 * option_tour_original and pause_config). Its context is GAME + 2 (0x08E75912): + 0 the screen's state
 * (the table at 0x08A37A04), + 1 the menu's cursor (the submenus by cursor, 0x08A37A48), + 5 a submenu
 * is shown, + 6 in a submenu. exsetting.c, screensetting.c and widescreen.c hook the functions they
 * change (tools/data/hooks.txt); those stay translated until they are decompiled here and their hooks
 * become wrapped extensions.
 *
 * SYS (0x08A058A0) holds the options as the save keeps them; OPTION MODE edits them there and
 * 0x088B8DFC copies them into the game state.
 */
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }

#define GAME          0x08E75910u
#define OPT           0x08E75912u   /* GAME + 2: + 0 state, + 1 cursor */
#define SYS           0x08A058A0u
#define PADS          0x08E75BA0u   /* 44 bytes each: + 4 repeat, + 12 newly pressed */
#define TIMER         0x08B02E0Cu   /* + 1 SAVE / LOAD's place, + 2 the held direction's repeat count */
#define OPT_STATES    0x08A37A04u   /* the task's states by OPT + 0, called with SYS */
#define OPT_SUBMENUS  0x08A37A48u   /* each frame's submenu by the cursor */
#define OBJECTS       0x09512770u   /* 32 objects, 0xC0 bytes: + 0 in use, + 2 kind */
#define OBJ_COUNT     32
#define OBJ_SIZE      0xC0u
#define KIND_PICTURE  0x93          /* BUTTON CONFIG's pictures */
#define UNLOCKS       0x095176F3u   /* s8: how far the game has been played (more pictures past 12 / 24) */
#define CONFIG_PADS   0x095176F9u   /* s8: 4 or more: the per-player CONFIG buttons */
#define PAD_CONFIRM   0x8080u
#define SOUND_MOVE    0x38
#define SOUND_OK      0x39

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_088054F0(Cpu *c);   /* task.c: wait for frames */
void f_0887C4CC(Cpu *c);   /* sound effect (0, id, volume) */
void f_08887AFC(Cpu *c); void f_08837690(Cpu *c); void f_0880B6CC(Cpu *c); void f_088D2B5C(Cpu *c);   /* each frame's drawing */
void f_088155C0(Cpu *c);
void f_089E0BF4(Cpu *c);   /* (SYS + 84, SYS + 86): the screen's place, as far as is known */
void f_089E40B4(Cpu *c);   /* (SCREEN SIZE: 0 NORMAL, 1 WIDE) */
void f_0880A39C(Cpu *c); void f_08822934(Cpu *c); void f_08823304(Cpu *c); void f_088D2B54(Cpu *c);
void f_08823338(Cpu *c);   /* objects: make one (kind, type); 0 if there is no room */
void f_08822EAC(Cpu *c);   /* objects: create (kind, type); 0 if no room */
void f_08817834(Cpu *c);   /* save task (a, b) */
void f_08815478(Cpu *c);
void f_089E3E00(Cpu *c); void f_089E32CC(Cpu *c);   /* memory stick: start a check, its state (-1 busy) */
void f_088231C8(Cpu *c);   /* (object): remove */
void f_088B81FC(Cpu *c); void f_088B6108(Cpu *c); void f_088B7270(Cpu *c); void f_088B8078(Cpu *c);
void f_088B7D08(Cpu *c); void f_088B8DFC(Cpu *c);   /* below */

static int confirm_pressed(void) { return ((*W(PADS + 12) | *W(PADS + 56)) & PAD_CONFIRM) != 0; }
static void sound(Cpu *c, uint32_t id) { CALL3(f_0887C4CC, 0, id, 0xC0); }

/* The task (0x088B5F48): each frame the sound flags, the state's function, the drawing, wait */
static void option_task(Cpu *c) {
    for (;;) {
        CALL1(f_088B81FC, 0);
        gaddr fn = *W(OPT_STATES + (uint32_t)(S8(OPT) * 4));
        c->r[4] = SYS; c->r[5] = fn;
        rt_call_indirect(c, fn);
        CALL0(f_08887AFC);
        CALL0(f_08837690);
        CALL0(f_0880B6CC);
        CALL0(f_088D2B5C);
        CALL1(f_088054F0, 1);
    }
}

/* SAVE / LOAD's place in its list (0x088B6108, TIMER + 1) from the save's state (+ 118, + 48) */
static void save_place(gaddr state) {
    int8_t a = S8(state + 118);
    int8_t v;
    if (a == 1) v = 1;
    else if (a == 2) v = 2;
    else if (a == 0 && S8(state + 48) == 0) v = 0;
    else v = (int8_t)(S8(state + 48) + 2);
    *B(TIMER + 1) = (uint8_t)v;
}

/* A value goes round 0 - max with left / right (0x088B7D08: pad, value, max): pressed moves it at
   once (and waits 20 frames), held repeats it every 6 */
static int32_t value_by_pad(Cpu *c, gaddr pad, int32_t v, int32_t max) {
    gaddr frame = c->r[29];                    /* the original copies the pad's 11 words to its stack */
    for (uint32_t k = 0; k < 11; k++) *W(frame + 60 + 4 * k) = *W(pad + 4 * k);
    max = (int16_t)max;
    uint32_t pressed = *W(pad + 12), repeat = *W(pad + 4);
    int dir = 0, wait = 0x14;
    if (pressed & 2) dir = -1;
    else if (pressed & 1) dir = 1;
    else if (repeat & 3) {
        int8_t left = (int8_t)(*B(TIMER + 2) - 1);
        *B(TIMER + 2) = (uint8_t)left;
        if (left >= 0) return v;
        dir = (repeat & 2) ? -1 : 1;
        wait = 6;
    } else return v;
    sound(c, SOUND_MOVE);
    *B(TIMER + 2) = (uint8_t)wait;
    v += dir;
    if (dir < 0 && v < 0) v = max;
    if (dir > 0 && max < v) v = 0;
    return v;
}

/* WALLPAPER (0x088B7064: SYS, + 510, 0 - 7) by player 1's then player 2's pad */
static void sound_value(Cpu *c, gaddr obj) {
    *B(obj + 510) = (uint8_t)CALL3(f_088B7D08, PADS, *B(obj + 510), 7);
    *B(obj + 510) = (uint8_t)CALL3(f_088B7D08, PADS + 0x2C, *B(obj + 510), 7);
    f_088155C0(c);
}

/* The screen from the options (0x088B70C0: SYS + 84 / + 86, then SCREEN SIZE at + 88) */
static void apply_screen(Cpu *c) {
    CALL2(f_089E0BF4, (uint32_t)(int32_t)S16(SYS + 84), (uint32_t)(int32_t)S16(SYS + 86));
    CALL1(f_089E40B4, (uint32_t)(int32_t)S16(SYS + 88));
}

/* Back to the menu from a submenu (0x088B7270): clear the screen and the objects, make the menu's again
   (the CONFIG pads' with 4 or more pads, the items and their frame), then the cursor's item's */
static void submenu_close(Cpu *c) {
    CALL0(f_0880A39C);
    CALL0(f_08822934);
    CALL0(f_08823304);
    CALL0(f_088D2B54);
    for (uint32_t i = 0; i < 5; i++)
        if (S8(CONFIG_PADS) >= 4) CALL2(f_08823338, 0xC, (i + 0xF) & 0xFF);
    static const uint8_t made[][2] = { { 6, 0x10 }, { 6, 0x11 }, { 6, 0x12 }, { 6, 0x13 }, { 6, 0x14 }, { 0xC, 0xD } };
    for (int k = 0; k < 6; k++)
        if (CALL2(f_08823338, made[k][0], made[k][1]) == 0) return;
    CALL1(f_088B8078, (uint32_t)(int32_t)S8(OPT + 1) & 0xFFFF);
}

/* A submenu waits for confirm, then closes (0x088B7364: saving its changes, 0x08817834 (1, 0)) */
static void submenu_confirm_save(Cpu *c) {
    if (!confirm_pressed()) return;
    sound(c, SOUND_OK);
    CALL2(f_08817834, 1, 0);
    CALL0(f_088B7270);
}

/* ... the same with the options copied into the game state, SAVE / LOAD's place and the players'
   present bit set again (0x088B73B8) */
static void submenu_confirm_apply(Cpu *c, gaddr a0, gaddr a1) {
    if (!confirm_pressed()) return;
    sound(c, SOUND_OK);
    CALL2(f_08817834, 0, 0);
    CALL0(f_088B8DFC);
    CALL2(f_088B6108, a0, a1);
    *B(GAME + 53) = (uint8_t)(1u << (*B(SYS + 511) & 31));
    CALL0(f_088B7270);
    CALL0(f_08815478);
}

/* Confirm checks the memory stick (0x088B745C), a frame at a time until it answers */
static void stick_check(Cpu *c) {
    if (!confirm_pressed()) return;
    CALL0(f_089E3E00);
    do CALL1(f_088054F0, 1); while ((int32_t)CALL0(f_089E32CC) == -1);
}

/* Each frame's submenu by the cursor (0x088B7538) */
static void submenu_frame(Cpu *c) {
    gaddr fn = *W(OPT_SUBMENUS + (uint32_t)(S8(OPT + 1) * 4));
    c->r[4] = fn; c->r[5] = OPT_SUBMENUS;
    rt_call_indirect(c, fn);
}

/* The menu's objects for cursor `item` (0x088B8078): 0 BUTTON CONFIG's pictures (30 of them once
   UNLOCKS reaches 24), 1 the extra pictures (7 more under 12), 2 / 3 their bars */
static void item_objects(Cpu *c, uint32_t item) {
    item &= 0xFFFF;
    int32_t n = (int32_t)item;
    if (n == 0) {
        if (S8(UNLOCKS) >= 24)
            for (uint32_t i = 0; i < 30; i++) CALL2(f_08822EAC, KIND_PICTURE, i & 0xFF);
        CALL2(f_08823338, 0xC, 4);
    } else if (n == 1) {
        if (S8(UNLOCKS) >= 12)
            for (uint32_t i = 0; i < 7; i++) CALL2(f_08822EAC, KIND_PICTURE, (i + 0x24) & 0xFF);
        if (CALL2(f_08822EAC, KIND_PICTURE, 0x1E) != 0) CALL2(f_08823338, 0xC, 7);
    } else if (n == 2) CALL2(f_08823338, 0xC, 0xA);
    else if (n == 3) CALL2(f_08823338, 0xC, 0x14);
}

/* Remove every picture object (0x088B8188) */
static void remove_pictures(Cpu *c) {
    for (uint32_t i = 0; i < OBJ_COUNT; i++) {
        gaddr o = OBJECTS + i * OBJ_SIZE;
        if (*B(o) != 0 && *B(o + 2) == KIND_PICTURE) CALL1(f_088231C8, o);
    }
}

/* The options' flags (0x088B81FC): + 96 a button setting differs from the default (three players'
   six buttons at + 0, 20 bytes apart); unless `quiet`, + 97 SYS + 84 or + 86 set (as far as is known, the screen's place), + 98 WIDE (SYS + 88);
   then SCREEN SIZE again */
static void option_flags(Cpu *c, int8_t quiet) {
    int8_t changed = 0;
    for (uint32_t j = 0; j < 3; j++)
        for (uint32_t k = 0; k < 6; k++)
            if (*H(SYS + j * 20 + k * 2) != k) changed = (int8_t)(changed + 1);
    *B(SYS + 96) = changed ? 1 : 0;
    int16_t size = S16(SYS + 88);
    if (!quiet) {
        int16_t x = S16(SYS + 84);
        if (x != 0 || S16(SYS + 86) != 0) *B(SYS + 97) = 1;
        if (x == 0 && S16(SYS + 86) == 0) *B(SYS + 97) = 0;
        *B(SYS + 98) = size ? 1 : 0;
    }
    c->r[5] = SYS;
    CALL1(f_089E40B4, (uint32_t)(int32_t)size);
}

/* The options into the game state (0x088B8DFC) */
static void options_to_game(void) {
    static const uint16_t from[] = { 60, 61, 62, 63, 64, 66, 67, 68, 69, 70, 71, 516 };
    static const uint16_t to[]   = { 45, 47, 76, 77, 46, 116, 48, 118, 119, 120, 123, 156 };
    for (int k = 0; k < 12; k++) *B(GAME + to[k]) = *B(SYS + from[k]);
    *B(GAME + 188) = *B(SYS + 509);
    *B(GAME + 187) = *B(SYS + 508);
}

/* ---- the menu's pictures (the screen objects OPTION MODE sets up; r4 the object) ---- */
#define P_STEP      128    /* u8: the step table's index */
#define P_KIND      92     /* u8: which picture */
#define P_SHOWN     93     /* u8: drawn */
#define P_X         6      /* s16 */
#define P_Y         10     /* s16 */
#define P_U         12     /* s16: the texture's place and size: + 12 / + 24, + 14 / + 26 */
#define P_V         24
#define P_W         14
#define P_H         26
#define P_ALPHA     136    /* u8 */
#define P_PULSE     98     /* u16: 0x088BAC18's brightness */
#define P_PULSE_T   94     /* s16: its counter, 0 - 100 */
#define P_TINT_A    36     /* u8 [4] each: the corners' colours */
#define P_TINT_B    42
#define P_TINT_C    48
#define MENU_STEPS      0x08A37CB0u   /* 0x088BA238's step table */
#define MENU_SCALE      0x08A37CC0u   /* double: the menu's pictures' size */
#define ITEM_PLACES     0x08A37DBAu   /* s16 [2] by item: x, y */
#define ITEM_FRAMES     0x08A37C60u   /* 16 bytes by item: 0x088BEAD8's frame */
#define SUB_STEPS       0x08A3B0B8u   /* 0x088BE7DC's step table */
#define SUB_FRAMES      0x08A3B028u   /* 16 bytes by kind */
#define SUB_PLACES      0x08A3B0D0u   /* s16 [2] by kind - 1 */
#define SUB_SCALE       0x08A3B0C8u   /* double */
#define SAVE_STATE      0x092E1228u   /* + 79 s8: the save screen's choice */

void f_088233BC(Cpu *c);   /* (object): restart its step */
void f_089FB928(Cpu *c);   /* soft float: int to double (v0, v1) */
void f_089FB6A8(Cpu *c);   /* soft float: double * double */
void f_089FBA04(Cpu *c);   /* soft float: double to int */
void f_088BA3F0(Cpu *c); void f_088BADEC(Cpu *c); void f_088BA538(Cpu *c);   /* not reached yet */
void f_088BA82C(Cpu *c); void f_088BA8F4(Cpu *c); void f_088BA974(Cpu *c); void f_088BAA90(Cpu *c); void f_088BAB44(Cpu *c);
void f_088BA4A8(Cpu *c); void f_088BA5F8(Cpu *c); void f_088BAC84(Cpu *c); void f_088BAF94(Cpu *c);
void f_088BE970(Cpu *c); void f_088BEA0C(Cpu *c); void f_088BEAD8(Cpu *c); void f_088BEB1C(Cpu *c);
void f_088BEC7C(Cpu *c); void f_088BED50(Cpu *c);

static void fill4(gaddr at, uint8_t v) { for (int j = 3; j >= 0; j--) *B(at + (uint32_t)j) = v; }
static void tints(gaddr t, uint8_t a, uint8_t b, uint8_t cc) { fill4(t + P_TINT_A, a); fill4(t + P_TINT_B, b); fill4(t + P_TINT_C, cc); }
static void step_up(gaddr t) { *B(t + P_STEP) = (uint8_t)(*B(t + P_STEP) + 1); }

/* The picture's step by its table (0x088BA238 / 0x088BE7DC) */
static void run_step(Cpu *c, gaddr table) {
    gaddr fn = *W(table + (uint32_t)*B(ARG(0) + P_STEP) * 4);
    c->r[5] = fn;
    rt_call_indirect(c, fn);
}

/* An s16 field times a double, through the game's soft float (exact rounding as the original): the
   conversions, then the product, then back */
typedef struct { uint32_t lo, hi; } Double;
static Double to_double(Cpu *c, int16_t v) { CALL1(f_089FB928, (uint32_t)(int32_t)v); return (Double){ c->r[2], c->r[3] }; }
static uint16_t times(Cpu *c, Double v, gaddr k) {
    CALL4(f_089FB6A8, v.lo, v.hi, *W(k), *W(k + 4));
    return (uint16_t)CALL2(f_089FBA04, c->r[2], c->r[3]);
}
static void scale(Cpu *c, gaddr t, uint32_t field, gaddr k) { *H(t + field) = times(c, to_double(c, S16(t + field)), k); }

/* A frame from a 16-byte table entry (0x088BEAD8): size, texture, + 60 its sheet */
static void set_frame(gaddr t, gaddr f) {
    *H(t + P_U) = 0;
    *H(t + P_V) = 0;
    *H(t + P_W) = *H(f);
    *H(t + P_H) = *H(f + 2);
    *W(t + 60) = *W(f + 4);
    *H(t + 64) = *H(f + 8);
    *H(t + 76) = *H(f + 10);
    *H(t + 66) = *H(f + 12);
    *H(t + 78) = *H(f + 14);
}

/* The menu's pictures start (0x088BA26C) by kind; the items (15 - 19) at the menu's size */
static void menu_picture_start(Cpu *c, gaddr t) {
    int32_t k = *B(t + P_KIND);
    if (k == 4) CALL1(f_088BA3F0, t);
    else if (k >= 5 && k < 9) CALL1(f_088BADEC, t);
    else if (k >= 9 && k < 11) CALL1(f_088BAF94, t);
    else if (k == 13) CALL1(f_088BA4A8, t);
    else if (k == 14) CALL1(f_088BA538, t);
    else if (k >= 15 && k < 20) {
        CALL1(f_088BAC84, t);
        scale(c, t, P_U, MENU_SCALE);
        scale(c, t, P_W, MENU_SCALE);
        scale(c, t, P_X, MENU_SCALE);
    } else if (k == 20) CALL1(f_088BA5F8, t);
}

/* ... and each frame (0x088BA6CC); 14 waits for the submenu to close */
static void menu_picture_run(Cpu *c, gaddr t) {
    int32_t k = *B(t + P_KIND);
    if (k == 4) CALL1(f_088BA8F4, t);
    else if (k >= 5 && k < 9) CALL1(f_088BA974, t);
    else if (k >= 9 && k < 13) CALL1(f_088BAA90, t);
    else if (k == 14) { if (S8(OPT + 6) == 0) step_up(t); }
    else if (k >= 15 && k < 20) {
        CALL1(f_088BA82C, t);
        scale(c, t, P_U, MENU_SCALE);
        scale(c, t, P_W, MENU_SCALE);
    } else if (k == 20) CALL1(f_088BAB44, t);
}

/* Kind 13 (0x088BA4A8): the wide band behind the menu */
static void band_start(gaddr t) {
    step_up(t);
    *B(t + P_SHOWN) = 1; *B(t + 89) = 1; *B(t + 88) = 0;
    *B(t + 55) = 7;
    *H(t + P_X) = 0; *H(t + P_Y) = 9;
    *H(t + P_V) = 0; *H(t + P_U) = 0;
    *H(t + P_W) = 0x280; *H(t + P_H) = 0x20;
    *B(t + 56) = 1;
    *B(t + P_ALPHA) = 0xC0;
    tints(t, 0x76, 0x76, 0xBC);
}

/* The common start of kinds 9 / 10 / 20: shown while submenu `shown_in` is up, pulse reset */
static void framed_start(gaddr t, int8_t shown_in) {
    step_up(t);
    *B(t + P_SHOWN) = S8(OPT + 5) == shown_in ? 1 : 0;
    *B(t + 55) = 6; *B(t + 56) = 0; *B(t + 1) = 0; *B(t + 3) = 0;
    *B(t + P_ALPHA) = 0xFF;
    *H(t + P_U) = 0; *H(t + P_V) = 0;
    *H(t + P_PULSE) = 0xFF;
}

/* Kind 20 (0x088BA5F8), with submenu 4 */
static void kind20_start(gaddr t) {
    framed_start(t, 4);
    if (*B(t + P_KIND) == 20) {
        *B(t + 55) = 7;
        *B(t + P_ALPHA) = 0x7F;
        *H(t + P_X) = 0x7C; *H(t + P_Y) = 0xB6;
        tints(t, 0, 0xFF, 0);
        *H(t + P_W) = 0x80; *H(t + P_H) = 0x12;
    }
    *H(t + P_PULSE_T) = 0;
}

/* Kinds 9 / 10 (0x088BAF94), with submenu 3 */
static void kind9_start(gaddr t) {
    framed_start(t, 3);
    uint32_t k = *B(t + P_KIND);
    *H(t + P_PULSE_T) = 0;
    if (k == 9) {
        *B(t + P_ALPHA) = 0x7F;
        *H(t + P_X) = 0x66; *H(t + P_Y) = 0xB6;
        tints(t, 0, 0, 0);
        *H(t + P_W) = 0xB4; *H(t + P_H) = 0x70;
    } else if (k == 10) {
        *B(t + 55) = 7;
        *B(t + P_ALPHA) = 0x7F;
        *H(t + P_X) = 0x66; *H(t + P_Y) = 0xB1;
        tints(t, 0, 0xFF, 0);
        *H(t + P_W) = 0xA8; *H(t + P_H) = 0x12;
    }
}

/* The menu's items (kinds 15 - 19, 0x088BAC84): place and frame by item; the cursor's one bright and
   bigger */
static void item_start(gaddr t) {
    step_up(t);
    int32_t item = (int32_t)*B(t + P_KIND) - 15;
    *B(t + P_SHOWN) = S8(OPT + 5) == 0 ? 1 : 0;
    *B(t + 55) = 7; *B(t + 56) = 1; *B(t + 1) = 1; *B(t + 3) = 0;
    *B(t + 89) = 1; *B(t + 54) = 0x12; *B(t + 88) = 1;
    gaddr place = ITEM_PLACES + (uint32_t)(item * 4);
    *H(t + P_X) = *H(place);
    *H(t + P_Y) = *H(place + 2);
    *B(t + P_ALPHA) = 0xFF;
    set_frame(t, ITEM_FRAMES + (uint32_t)(item * 16));
    uint8_t v;
    if (S8(OPT + 1) == item) {
        *B(t + 56) = 0;
        *H(t + P_U) = (uint16_t)-4; *H(t + P_V) = (uint16_t)-4;
        *H(t + P_W) = 0xC8; *H(t + P_H) = 0x48;
        v = 0xFF;
    } else {
        *B(t + P_ALPHA) = 0x80;
        *H(t + P_U) = 0; *H(t + P_V) = 0;
        *H(t + P_W) = 0xC0; *H(t + P_H) = 0x40;
        v = 0x80;
    }
    tints(t, v, v, v);
    *B(t + 138) = 1;
}

/* The pulse (0x088BAC18): alpha 0x40 + half the brightness, which falls for 50 frames and rises for
   50 */
static void pulse(gaddr t) {
    uint16_t v = *H(t + P_PULSE);
    *B(t + P_ALPHA) = (uint8_t)(((v & 0xFF) >> 1) + 0x40);
    int16_t n = S16(t + P_PULSE_T);
    if (n < 50) *H(t + P_PULSE) = (uint16_t)(v - 5);
    else if (n > 50) *H(t + P_PULSE) = (uint16_t)(v + 5);
    n = (int16_t)(n + 1);
    *H(t + P_PULSE_T) = (uint16_t)(n > 100 ? 0 : n);
}

/* A submenu's pictures start (0x088BE810): the title (kind 0) or a line, at the submenu's size */
static void sub_picture_start(Cpu *c, gaddr t) {
    step_up(t);
    CALL1(f_088BEB1C, t);
    if (*B(t + P_KIND) == 0) CALL1(f_088BE970, t);
    else CALL1(f_088BEA0C, t);
    Double u = to_double(c, S16(t + P_U)), w = to_double(c, S16(t + P_W)), x = to_double(c, S16(t + P_X));
    *H(t + P_U) = times(c, u, SUB_SCALE);
    *H(t + P_W) = times(c, w, SUB_SCALE);
    *H(t + P_X) = times(c, x, SUB_SCALE);
}

static void sub_common(gaddr t) { *B(t + 54) = 0x12; *B(t + 88) = 1; *B(t + 55) = 7; }
static void sub_finish(gaddr t) { *B(t + 56) = 2; *B(t + P_ALPHA) = 0xC0; tints(t, 0x80, 0x80, 0x80); }

/* The title (0x088BE970) */
static void sub_title(Cpu *c, gaddr t) {
    sub_common(t);
    CALL2(f_088BEAD8, t, SUB_FRAMES);
    *H(t + P_X) = 0x100; *H(t + P_Y) = 0x180;
    sub_finish(t);
}

/* A line (0x088BEA0C) by kind */
static void sub_line(Cpu *c, gaddr t) {
    sub_common(t);
    uint32_t k = *B(t + P_KIND);
    CALL2(f_088BEAD8, t, SUB_FRAMES + k * 16);
    gaddr place = SUB_PLACES + (k - 1) * 4;
    *H(t + P_X) = *H(place);
    *H(t + P_Y) = (uint16_t)(*H(place + 2) - 0x20);
    sub_finish(t);
}

/* Shown, full colour (0x088BEB1C) */
static void sub_shown(gaddr t) {
    *B(t + P_SHOWN) = 1; *B(t + 3) = 0; *B(t + 89) = 1; *B(t + 137) = 1;
    *B(t + P_ALPHA) = 0xFF;
    tints(t, 0xFF, 0xFF, 0xFF);
    *B(t + 138) = 1;
}

/* The save screen's two choices (0x088BEB74): the chosen one bright and bigger */
static void choice_place(gaddr t, int chosen, uint16_t big, uint16_t big_h, uint16_t small, uint16_t small_h) {
    if (chosen) {
        *H(t + P_V) = (uint16_t)-4; *H(t + P_U) = (uint16_t)-4;
        *H(t + P_W) = big; *H(t + P_H) = big_h;
        *B(t + P_ALPHA) = 0xFF; tints(t, 0xFF, 0xFF, 0xFF);
        *B(t + 56) = 1;
    } else {
        *H(t + P_V) = 0; *H(t + P_U) = 0;
        *H(t + P_W) = small; *H(t + P_H) = small_h;
        *B(t + P_ALPHA) = 0xC0; tints(t, 0x80, 0x80, 0x80);
        *B(t + 56) = 2;
    }
}
/* 0x088BEC7C: kind 0, chosen when the choice is 0 */
static void choice_first(gaddr t) { choice_place(t, S8(SAVE_STATE + 79) == 0, 0x88, 0x28, 0x80, 0x20); }
/* 0x088BED50: the others, chosen when the choice is their kind */
static void choice_other(gaddr t) { choice_place(t, S8(SAVE_STATE + 79) == (int32_t)*B(t + P_KIND), 0x78, 0x78, 0x70, 0x70); }

static void choice_run(Cpu *c, gaddr t) {
    if (*B(t + P_KIND) == 0) CALL1(f_088BEC7C, t);
    else CALL1(f_088BED50, t);
    Double u = to_double(c, S16(t + P_U)), w = to_double(c, S16(t + P_W));
    *B(t + P_SHOWN) = 1;
    *H(t + P_U) = times(c, u, SUB_SCALE);
    *H(t + P_W) = times(c, w, SUB_SCALE);
}

/* ---- GAME OPTION (state 2; each row's handler gets (SYS, GAME)) ---- */
void f_0880AA74(Cpu *c);   /* fade.c: fade out */
void f_0880A6CC(Cpu *c);   /* text: queue (string, 1, x, y, colour) */
void f_089E6898(Cpu *c);   /* (music volume) */
void f_089E688C(Cpu *c);   /* (effects volume) */
#define TEXT_ROW      0xA           /* text colours: a row, the row under the cursor */
#define TEXT_CURSOR   0xB
#define GO_LABELS     0x089FF990u   /* the rows' names (12) */
#define GO_TIMES      0x08A379C0u   /* text by GAME + 47 (3, no limit, is a picture) */
#define GO_ROUNDS     0x08A379D0u   /* by GAME + 76 */
#define GO_SPEEDS     0x08A379DCu   /* by TIMER + 1 */
#define GO_GAUGES     0x08A37AE4u   /* by GAME + 119 */
#define GO_SELECTS    0x08A37AECu   /* by SYS + 516 */
#define GO_SIDES      0x08A37B34u   /* by SYS + 511 */
#define GO_VOLUMES    0x08A37AF4u   /* by GAME + 187 / + 188 */

/* A row's value by both players' pads (0x088B7D08 with player 1's pad, then player 2's) */
static int8_t pads_value(Cpu *c, gaddr at, int8_t v, int32_t max) {
    *B(at) = (uint8_t)CALL3(f_088B7D08, PADS, (uint32_t)(int32_t)v, (uint32_t)max);
    *B(at) = (uint8_t)CALL3(f_088B7D08, PADS + 0x2C, (uint32_t)(int32_t)S8(at), (uint32_t)max);
    return S8(at);
}
/* The plain rows: a GAME field 0 - max, copied to SYS (0x088B6318 DIFFICULTY 45 → 60, 0x088B6384 TIME
   47 → 61, 0x088B63F0 ROUNDS 76 → 62, 0x088B65B4 S.C. GAUGE 119 → 69) */
static void row_plain(Cpu *c, gaddr sys, gaddr game, uint32_t field, int32_t max, uint32_t to) {
    *B(sys + to) = (uint8_t)pads_value(c, game + field, S8(game + field), max);
}
/* DAMAGE (0x088B645C): 46 → SYS + 66, GAME + 116, SYS + 64 */
static void row_damage(Cpu *c, gaddr sys, gaddr game) {
    int8_t v = pads_value(c, game + 46, S8(game + 46), 3);
    *B(sys + 66) = (uint8_t)v; *B(game + 116) = (uint8_t)v; *B(sys + 64) = (uint8_t)v;
}
/* SPEED (0x088B64D0): one list (TIMER + 1, 0 - 9) for GAME + 118 (0 normal, 1 / 2 the TURBO kinds) and
   GAME + 48 (FREE's speed, list place - 2) */
static void row_speed(Cpu *c, gaddr sys, gaddr game) {
    int8_t v = pads_value(c, TIMER + 1, S8(TIMER + 1), 9);
    if (v == 0) { *B(game + 118) = 0; *B(game + 48) = 0; }
    else if (v == 1 || v == 2) { *B(game + 118) = (uint8_t)v; *B(game + 48) = 0; }
    else { *B(game + 118) = 0; *B(game + 48) = (uint8_t)(S8(TIMER + 1) - 2); }
    *B(sys + 67) = *B(game + 48);
    *B(sys + 68) = *B(game + 118);
}
/* MODE SELECT (0x088B6678, SYS + 516) and PLAYER SIDE (0x088B6620, SYS + 511, read unsigned) */
static void row_select(Cpu *c, gaddr sys) { pads_value(c, sys + 516, S8(sys + 516), 1); }
static void row_side(Cpu *c, gaddr sys) {
    *B(sys + 511) = (uint8_t)CALL3(f_088B7D08, PADS, *B(sys + 511), 1);
    *B(sys + 511) = (uint8_t)CALL3(f_088B7D08, PADS + 0x2C, *B(sys + 511), 1);
}
/* BGM / S.E. VOLUME (0x088B66D0 / 0x088B6744): GAME + 187 / + 188, 0 - 15, to SYS + 508 / + 509, heard
   at once */
static void row_volume(Cpu *c, gaddr sys, gaddr game, int music) {
    uint32_t field = music ? 187 : 188;
    int8_t v = pads_value(c, game + field, S8(game + field), 15);
    *B(sys + (music ? 508 : 509)) = (uint8_t)v;
    CALL1(music ? f_089E6898 : f_089E688C, (uint32_t)(int32_t)S8(game + field));
}
/* DEFAULT (0x088B67B8): on confirm, every row back to the game's defaults, into SYS too */
static void row_default(Cpu *c, gaddr sys, gaddr game) {
    if (!confirm_pressed()) return;
    sound(c, SOUND_OK);
    *B(game + 45) = 3; *B(game + 47) = 2; *B(game + 76) = 1; *B(game + 77) = 1; *B(game + 46) = 1;
    *B(game + 116) = 1; *B(game + 48) = 0; *B(game + 118) = 2; *B(game + 119) = 1;
    *B(sys + 516) = 0; *B(sys + 511) = 0;
    *B(game + 187) = 15; *B(game + 188) = 15;
    *B(TIMER + 1) = 2;
    static const uint16_t from[] = { 45, 47, 76, 77, 46, 116, 48, 118, 119, 123, 187, 188 };
    static const uint16_t to[]   = { 60, 61, 62, 63, 64, 66, 67, 68, 69, 71, 508, 509 };
    for (int k = 0; k < 12; k++) *B(sys + to[k]) = *B(game + from[k]);
    CALL1(f_089E6898, (uint32_t)(int32_t)S8(game + 187));
    CALL1(f_089E688C, (uint32_t)(int32_t)S8(game + 188));
}
/* EXIT (0x088B68D0): confirm closes the submenu (OPT + 2 = 1) */
static void row_exit(Cpu *c) {
    if (!confirm_pressed()) return;
    sound(c, SOUND_OK);
    *B(OPT + 2) = 1;
}
/* State 7 (0x088B62CC): leave OPTION MODE: fade out, the sound, clear, next state */
static void option_leave(Cpu *c) {
    CALL0(f_0880AA74);
    sound(c, SOUND_OK);
    CALL0(f_0880A39C);
    *B(OPT) = (uint8_t)(S8(OPT) + 1);
}

/* GAME OPTION's text (0x088B8894): the twelve names, the cursor's row (OPT + 7) brighter, then the
   values drawn as text (the stars, numbers and TIME's no-limit are pictures) */
static void go_text(Cpu *c) {
    static const uint16_t labels[12] = { 0x990, 0x99C, 0x9A4, 0x9AC, 0x9B4, 0x9BC, 0x9C8, 0x9D4, 0x9E0, 0x9EC, 0x8A4, 0x9F8 };
    static const uint8_t ys[12] = { 0x20, 0x2F, 0x3E, 0x4D, 0x5C, 0x6B, 0x7A, 0x8A, 0x99, 0xA8, 0xB7, 0xC6 };
    int8_t cur = S8(OPT + 7);
    for (int r = 0; r < 12; r++)
        CALL5(f_0880A6CC, 0x089FF000u + labels[r], 1, 0x30, ys[r], cur == r ? TEXT_CURSOR : TEXT_ROW);
    int8_t time = S8(GAME + 47);
    if (time != 3) CALL5(f_0880A6CC, *W(GO_TIMES + (uint32_t)(time * 4)), 1, 0xA0, 0x2F, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_ROUNDS + (uint32_t)(S8(GAME + 76) * 4)), 1, 0xA0, 0x3E, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_SPEEDS + (uint32_t)(S8(TIMER + 1) * 4)), 1, 0xA0, 0x5C, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_GAUGES + (uint32_t)(S8(GAME + 119) * 4)), 1, 0xA0, 0x6B, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_SELECTS + (uint32_t)(S8(SYS + 516) * 4)), 1, 0xA0, 0x7A, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_SIDES + (uint32_t)*B(SYS + 511) * 4), 1, 0xA0, 0x8A, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_VOLUMES + (uint32_t)(S8(GAME + 187) * 4)), 1, 0xA0, 0x99, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(GO_VOLUMES + (uint32_t)(S8(GAME + 188) * 4)), 1, 0xA0, 0xA8, TEXT_CURSOR);
}

/* ---- GAME OPTION's frame and the way back ---- */
void f_088051A0(Cpu *c);   /* (player side) */
void f_088B8188(Cpu *c);   /* below: remove the pictures */
void f_0880ABD4(Cpu *c); void f_08815644(Cpu *c);
void f_088325DC(Cpu *c);   /* (GAME, BATTLE): the next screen */
#define GO_ROWS       0x08A37AB4u   /* the twelve rows' handlers, called (SYS, GAME) */
#define GO_TITLE      0x089FF944u
#define BATTLE        0x092E1228u
#define PAD_UP        0x8
#define PAD_DOWN      0x4
#define PAD_CANCEL    0x100
#define SOUND_BACK    0x2F

static uint32_t pads_pressed(void) { return *W(PADS + 12) | *W(PADS + 56); }
static uint32_t pads_repeat(void)  { return *W(PADS + 4) | *W(PADS + 48); }

/* The cursor one row up or down through `rows`, wrapping; `wait` frames before a held key repeats */
static void cursor_step(Cpu *c, gaddr at, int dir, int8_t rows, uint8_t wait) {
    sound(c, SOUND_MOVE);
    int8_t v = (int8_t)(S8(at) + dir);
    *B(TIMER + 2) = wait;
    *B(at) = (uint8_t)v;
    if (dir < 0 && v < 0) *B(at) = (uint8_t)(rows - 1);
    if (dir > 0 && v >= rows) *B(at) = 0;
}
/* The same, held: counts TIMER + 2 down first */
static void cursor_held(Cpu *c, gaddr at, int dir, int8_t rows) {
    int8_t left = (int8_t)(*B(TIMER + 2) - 1);
    *B(TIMER + 2) = (uint8_t)left;
    if (left >= 0) return;
    cursor_step(c, at, dir, rows, 6);
}

/* GAME OPTION, each frame (0x088B82E4: SYS, GAME). OPT + 2: 0 the rows (after 5 frames), up / down
   move OPT + 7, cancel closes, else the row's handler; 1 closing: the player side's pads, back to the
   menu; anything else resets */
static void go_frame(Cpu *c, gaddr sys, gaddr game) {
    CALL5(f_0880A6CC, GO_TITLE, 2, 0x68, 5, TEXT_CURSOR);
    int8_t sub = S8(OPT + 2);
    if (sub == 0) {
        int8_t t = (int8_t)(S8(TIMER) + 1);
        *B(TIMER) = (uint8_t)t;
        if (t < 5) return;
        *B(TIMER) = 5;
        uint32_t pressed = pads_pressed();
        if (pressed & PAD_UP) cursor_step(c, OPT + 7, -1, 12, 0x14);
        else if (pressed & PAD_DOWN) cursor_step(c, OPT + 7, 1, 12, 0x14);
        else {
            uint32_t held = pads_repeat();
            if (held & PAD_UP) cursor_held(c, OPT + 7, -1, 12);
            else if (held & PAD_DOWN) cursor_held(c, OPT + 7, 1, 12);
            else if (pressed & PAD_CANCEL) { sound(c, SOUND_BACK); *B(OPT + 2) = (uint8_t)(S8(OPT + 2) + 1); }
            else {
                gaddr fn = *W(GO_ROWS + (uint32_t)(S8(OPT + 7) * 4));
                c->r[4] = sys; c->r[5] = game;
                rt_call_indirect(c, fn);
            }
        }
    } else if (sub == 1) {
        CALL1(f_088051A0, *B(sys + 511));
        *B(game + 53) = (uint8_t)(1u << (*B(sys + 511) & 31));
        uint32_t held = *W(PADS + 4) | *W(PADS + 48);
        if (*B(sys + 511) != 0) *W(PADS + 48) = held; else *W(PADS + 4) = held;
        *B(OPT + 2) = (uint8_t)(sub + 1);
        CALL0(f_088B8188);
        CALL0(f_0880A39C);
        *B(OPT + 6) = 0;
    } else {
        *B(OPT + 2) = 0;
        *B(OPT + 7) = 0;
    }
}

/* State 2 (0x088B74C4): once GAME + 93 is clear, OPTION MODE's 26 bytes cleared and on to the next
   screen */
static void option_done(Cpu *c) {
    if (S8(GAME + 93) != 0) return;
    CALL0(f_0880ABD4);
    for (uint32_t i = 0; i < 26; i++) *B(OPT + i) = 0;
    CALL0(f_08815644);
    CALL2(f_088325DC, GAME, BATTLE);
}

/* ---- BUTTON CONFIG (state 4): OPT + 3 the row (0 - 6 buttons, 7 COMMAND, 8 TYPE, 9 RAPID, 10 DEFAULT,
   11 EXIT), OPT + 8 the players who changed something since (bit by player); each player's settings at
   SYS + 20 x player: six buttons (u16) and more, + 16 COMMAND, + 17 TYPE, + 18 RAPID ---- */
void f_088B7EC0(Cpu *c);   /* the menu cursor (pad, cursor, last row): hooked by exsetting.c */
void f_088B6A58(Cpu *c); void f_088B6B7C(Cpu *c); void f_088B6CA0(Cpu *c); void f_088B6D10(Cpu *c);
void f_088B6EB4(Cpu *c); void f_088B6F84(Cpu *c);
void f_0880A7B0(Cpu *c); void f_0880A7F4(Cpu *c);   /* a button's picture (button, x, y) */
#define BC_TITLE      0x089FF854u
#define BC_DEFAULTS   0x08A37A24u   /* u16 [8]: the buttons' defaults */
#define BC_COMMANDS   0x08A37A5Cu   /* text by + 16 */
#define BC_TYPES      0x08A37A68u   /* by + 17 */
#define BC_RAPIDS     0x08A37A70u   /* by + 18 */
#define BC_ACTIONS    0x08A37B44u   /* text by a button's action (15 on; lower ones are pictures) */
#define BC_ROWS       12
#define BC_DEFAULT    10
#define BC_EXIT       11
#define PAD_SIZE      0x2Cu

/* BUTTON CONFIG, each frame (0x088B6924: SYS, -): any key clears the player's changed bit; OPT + 2:
   0 both players' input (after 5 frames), 1 closing, anything else resets */
static void button_config(Cpu *c, gaddr sys, uint32_t a1) {
    if (*W(PADS + 12) != 0 && (S8(OPT + 8) & 1)) *B(OPT + 8) = (uint8_t)(S8(OPT + 8) ^ 1);
    if (*W(PADS + 56) != 0 && (S8(OPT + 8) & 2)) *B(OPT + 8) = (uint8_t)(S8(OPT + 8) ^ 2);
    CALL5(f_0880A6CC, BC_TITLE, 2, 0x58, 5, TEXT_CURSOR);
    int8_t sub = S8(OPT + 2);
    if (sub == 0) {
        int8_t t = (int8_t)(S8(TIMER) + 1);
        *B(TIMER) = (uint8_t)t;
        if (t < 5) return;
        *B(TIMER) = 5;
        CALL2(f_088B6A58, sys, a1);
        CALL2(f_088B6B7C, sys, a1);
    } else if (sub == 1) {
        *B(OPT + 2) = 2;
        CALL0(f_088B8188);
        CALL0(f_0880A39C);
        *B(TIMER) = 0;
        *B(OPT + 12) = 0; *B(OPT + 11) = 0; *B(OPT + 6) = 0;
    } else {
        *B(OPT + 4) = 0; *B(OPT + 3) = 0; *B(OPT + 2) = 0;
    }
}

/* One player's input (0x088B6A58 player 1, 0x088B6B7C player 2): up / down the rows, left / right the
   row's value, confirm DEFAULT or EXIT, cancel closes */
static void button_config_player(Cpu *c, gaddr sys, uint32_t player) {
    gaddr pad = PADS + player * PAD_SIZE;
    uint32_t keys = *W(pad + 12) | *W(pad + 4);
    if (keys & 0xC) CALL2(f_088B6CA0, sys, player);
    else if (keys & 3) CALL3(f_088B6D10, sys, player, *B(OPT + 9 + player));
    uint32_t pressed = *W(pad + 12);
    if (pressed & PAD_CONFIRM) {
        int8_t row = S8(OPT + 3);
        if (row == BC_DEFAULT) { sound(c, SOUND_OK); CALL3(f_088B6F84, sys, player, 0); }
        else if (row == BC_EXIT) { sound(c, SOUND_OK); *B(OPT + 2) = (uint8_t)(S8(OPT + 2) + 1); }
    } else if (pressed & PAD_CANCEL) {
        sound(c, SOUND_BACK);
        *B(OPT + 2) = (uint8_t)(S8(OPT + 2) + 1);
    }
}

/* The row by the player's pad (0x088B6CA0) */
static void button_config_row(Cpu *c, uint32_t player) {
    *B(OPT + 3) = (uint8_t)CALL3(f_088B7EC0, PADS + (player & 0xFF ? PAD_SIZE : 0), (uint32_t)(int32_t)S8(OPT + 3), BC_ROWS - 1);
}

/* The row's value by the player's pad (0x088B6D10): a button's action, or COMMAND (0 - 2), TYPE, RAPID
   (0 - 1). The original hands a copy of the pad on its stack frame to the value functions. */
static void button_config_value(Cpu *c, gaddr sys, uint32_t player) {
    player &= 0xFF;
    gaddr copy = c->r[29] + 16;
    for (uint32_t k = 0; k < 11; k++) *W(copy + 4 * k) = *W(PADS + (player ? PAD_SIZE : 0) + 4 * k);
    int8_t row = S8(OPT + 3);
    gaddr base = sys + player * 20;
    if (row < 7) CALL4(f_088B6EB4, copy, sys, player, (uint32_t)(int32_t)row);
    else if (row == 7) *B(base + 16) = (uint8_t)CALL3(f_088B7D08, copy, (uint32_t)(int32_t)S8(base + 16), 2);
    else if (row == 8) *B(base + 17) = (uint8_t)CALL3(f_088B7D08, copy, (uint32_t)(int32_t)S8(base + 17), 1);
    else if (row == 9) *B(base + 18) = (uint8_t)CALL3(f_088B7D08, copy, (uint32_t)(int32_t)S8(base + 18), 1);
}

/* A button's action (0x088B6EB4: pad, SYS, player, button): 0 - 0x37, written only when it changes */
static void button_config_action(Cpu *c, gaddr pad, gaddr sys, uint32_t player, uint32_t button) {
    gaddr copy = c->r[29] + 60;
    for (uint32_t k = 0; k < 11; k++) *W(copy + 4 * k) = *W(pad + 4 * k);
    gaddr at = sys + (player & 0xFF) * 20 + (uint32_t)((int16_t)button * 2);
    int16_t old = S16(at);
    uint32_t v = CALL3(f_088B7D08, copy, (uint32_t)(int32_t)old, 0x37);
    if ((int16_t)v != old) *H(at) = (uint16_t)v;
}

/* DEFAULT (0x088B6F84: SYS, player, keep): the buttons back to the defaults, and unless `keep` the
   other settings too (and SYS + 72's bit); the player's changed bit set */
static void button_config_default(gaddr sys, uint32_t player, uint32_t keep) {
    player &= 0xFF;
    gaddr base = sys + player * 20;
    uint32_t bit = 1u << (player & 31);
    for (uint32_t k = 0; k < 8; k++) *H(base + 2 * k) = *H(BC_DEFAULTS + 2 * k);
    if ((keep & 0xFF) == 0) {
        *B(sys + 72) = (uint8_t)((uint32_t)(int32_t)S8(sys + 72) | bit);
        *B(base + 16) = 1; *B(base + 17) = 1; *B(base + 18) = 0;
    }
    *B(OPT + 8) = (uint8_t)((uint32_t)(int32_t)S8(OPT + 8) | bit);
}

/* BUTTON CONFIG's text (0x088B7578): the button pictures, the present player's COMMAND / TYPE / RAPID,
   the last five rows' names (the cursor's brighter), and the actions shown as text */
static void button_config_text(Cpu *c) {
    uint32_t cur = (uint32_t)(int32_t)S8(OPT + 3) & 0xFF;
    uint32_t player = (S8(GAME + 53) & 1) ? 0 : 1;
    CALL3(f_0880A7B0, 2, 0x68, 0x18);
    CALL3(f_0880A7B0, 1, 0x68, 0x28);
    CALL3(f_0880A7B0, 8, 0x6C, 0x38);
    CALL3(f_0880A7B0, 3, 0x68, 0x48);
    CALL3(f_0880A7B0, 0, 0x68, 0x58);
    CALL3(f_0880A7B0, 9, 0x6C, 0x68);
    CALL3(f_0880A7F4, 0, 0x68, 0x78);
    gaddr base = SYS + player * 20;
    CALL5(f_0880A6CC, *W(BC_COMMANDS + (uint32_t)(S8(base + 16) * 4)), 1, 0xB0, 0x88, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(BC_TYPES + (uint32_t)(S8(base + 17) * 4)), 1, 0xB0, 0x98, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(BC_RAPIDS + (uint32_t)(S8(base + 18) * 4)), 1, 0xB0, 0xA8, TEXT_CURSOR);
    static const uint16_t labels[5] = { 0x88C, 0x894, 0x89C, 0x8A4, 0x8AC };
    for (uint32_t r = 0; r < 5; r++)
        CALL5(f_0880A6CC, 0x089FF000u + labels[r], 1, 0x68, 0x88 + 0x10 * r, cur == 7 + r ? TEXT_CURSOR : TEXT_ROW);
    for (uint32_t i = 0; i < 7; i++) {
        uint32_t v = *H(base + 2 * i);
        if ((int32_t)v >= 15) CALL5(f_0880A6CC, *W(BC_ACTIONS + v * 4), 1, 0xB0, 16 * i + 0x18, TEXT_CURSOR);
    }
}

/* ---- more of the menu's pictures ---- */
#define BC_CURSOR_P1  0x08A37D40u   /* s16 [2] by player 1's BUTTON CONFIG row (OPT + 9): x, y */
#define BC_CURSOR_P2  0x08A37D48u   /* ... player 2's (OPT + 10) */
#define BC_BAR_P1     0x08A37D50u   /* by OPT + 3 */
#define BC_BAR_P2     0x08A37D80u   /* by OPT + 4 */
void f_088BABFC(Cpu *c); void f_088BAC18(Cpu *c);

/* Kind 4 (0x088BA3F0): GAME OPTION's bar, shown with submenu 1 */
static void kind4_start(gaddr t) {
    step_up(t);
    *B(t + P_SHOWN) = S8(OPT + 5) == 1 ? 1 : 0;
    *B(t + 55) = 6; *B(t + 56) = 0; *B(t + 1) = 0; *B(t + 3) = 0;
    *H(t + P_X) = 0x2E; *H(t + P_Y) = 0xD7;
    *B(t + P_ALPHA) = 0xFF;
    tints(t, 0, 0xFF, 0);
    *H(t + P_U) = (uint16_t)-2; *H(t + P_V) = 0;
    *H(t + P_W) = 0x128; *H(t + P_H) = 0x10;
    *H(t + P_PULSE) = 0xFF;
    *H(t + P_PULSE_T) = 0;
}

/* Kinds 5 - 8 (0x088BADEC): BUTTON CONFIG's cursors (5 / 6, one per player) and bars (7 / 8) */
static void kind5_start(gaddr t) {
    step_up(t);
    *B(t + P_SHOWN) = 1;
    *B(t + 55) = 6; *B(t + 56) = 0; *B(t + 1) = 0; *B(t + 3) = 0;
    *B(t + P_ALPHA) = 0xFF;
    *H(t + P_U) = 0; *H(t + P_V) = 0;
    *H(t + P_PULSE) = 0xFF;
    uint32_t k = *B(t + P_KIND);
    *H(t + P_PULSE_T) = 0;
    if (k == 5) { *H(t + P_X) = 0x1E; *H(t + P_Y) = 0xD0; tints(t, 0xFF, 0, 0); *H(t + P_W) = 0x38; }
    else if (k == 6) { *H(t + P_X) = 0xAC; *H(t + P_Y) = 0xD0; tints(t, 0, 0xFF, 0); *H(t + P_W) = 0x38; }
    else if (k == 7) { tints(t, 0, 0xFF, 0); *H(t + P_W) = 0xB0; }
    else if (k == 8) { tints(t, 0, 0xFF, 0xFF); *H(t + P_W) = 0x8A; }
    else return;
    *H(t + P_H) = 0x10;
}

/* ... each frame (0x088BA974): placed by the players' rows, the pulse, started again once the submenu
   has closed */
static void kind5_run(Cpu *c, gaddr t) {
    uint32_t k = *B(t + P_KIND);
    gaddr place = 0;
    if (k == 5) place = BC_CURSOR_P1 + *B(OPT + 9) * 4u;
    else if (k == 6) place = BC_CURSOR_P2 + *B(OPT + 10) * 4u;
    if (place) { *H(t + P_X) = *H(place); *H(t + P_Y) = *H(place + 2); }
    place = 0;
    if (k == 7) place = BC_BAR_P1 + (uint32_t)(S8(OPT + 3) * 4);
    else if (k == 8) place = BC_BAR_P2 + (uint32_t)(S8(OPT + 4) * 4);
    if (place) { *H(t + P_X) = *H(place); *H(t + P_Y) = *H(place + 2); }
    CALL1(f_088BAC18, t);
    if (S8(OPT + 6) == 0) CALL1(f_088BABFC, t);
}

/* ---- BUTTON CONFIG's pictures (object kind 0x93): + 3 the picture, + 4 the step (table 0x08A380EC) ---- */
void f_0882196C(Cpu *c);   /* (object, animation) */
#define PIC_STEPS     0x08A380ECu
#define PIC_KINDS     0x08A37DD0u   /* 8 bytes by picture: + 0 animation, + 2 x, + 4 y, + 6 layer */
#define PIC_ANIMS     0x08A38010u   /* animations by number */

static void picture_step(Cpu *c) {
    gaddr fn = *W(PIC_STEPS + *B(ARG(0) + 4) * 4u);
    c->r[5] = fn;
    rt_call_indirect(c, fn);
}

/* A picture starts (0x088BB104): placed from its entry; 0 - 35 their animation (some start hidden),
   36 - 42 the present player's button (an action under 15 has a picture, and hides the object), 43 on
   their animation */
static void picture_start(Cpu *c, gaddr o) {
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *W(o + 148) = 7; *H(o + 86) = 0x18; *W(o + 164) = 1;
    *B(o + 13) = 0; *B(o + 12) = 0; *H(o + 38) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 24) = 7;
    uint32_t kind = *B(o + 3);
    gaddr e = PIC_KINDS + kind * 8;
    *B(o + 9) = *B(e + 6);
    *H(o + 18) = (uint16_t)S16(e + 2);
    *H(o + 22) = (uint16_t)S16(e + 4);
    static const uint64_t hidden = (1ull << 0) | (1ull << 9) | (1ull << 18) | (1ull << 23) | (1ull << 28) | (0x3Full << 30);
    if (kind < 36 && (hidden >> kind) & 1) *B(o + 1) = 0;
    if (kind >= 36 && kind < 43) {
        uint32_t player = (S8(GAME + 53) & 1) ? 0 : 1;
        int16_t action = S16(SYS + player * 20 + kind * 2 - 72);
        if (action < 15) {
            CALL2(f_0882196C, o, *W(PIC_ANIMS + (uint32_t)(action * 4)));
            *B(o + 1) = 0;
        }
    } else CALL2(f_0882196C, o, *W(PIC_ANIMS + *B(e) * 4u));
}

/* ---- the pause menu's BUTTON CONFIG (in a fight) ---- */
void f_088B9F50(Cpu *c); void f_088B9EA8(Cpu *c); void f_088B95E0(Cpu *c); void f_088B9B30(Cpu *c);
void f_088B8FC4(Cpu *c); void f_088B90E4(Cpu *c); void f_088B9208(Cpu *c);   /* players 1 - 3' input: hooked by exsetting.c */
void f_088B9790(Cpu *c);   /* its drawing: hooked by exsetting.c */
void f_08822B88(Cpu *c); void f_08848630(Cpu *c);
void f_088231F4(Cpu *c);   /* objects: remove (object) */
void f_08822EE4(Cpu *c);   /* objects: create (kind, type) */
#define PAUSE_MENU    0x08A37B40u   /* u32: the pause menu's state (+ 3 its step, + 10) */
#define PAUSE_STEPS   0x08A37C24u   /* the pause BUTTON CONFIG's steps by that + 3 */
#define PAUSE_PICTURES 0x08A37C40u  /* u32: ... */
#define PB            0x08B02E1Cu   /* + 0 sub-state, + 2 frames since open, + 3 the held key's repeat,
                                       + 4 the player shown, + 6 a frame counter (s16) */
#define PB_CFG        0x08B02E24u   /* + 1, + 2, + 3 who pressed a key (bit by player), + 4 whose
                                       settings, + 5 open */
#define OBJECTS_16    0x09513F70u   /* 16 objects, 0xC0 bytes: + 0 in use, + 2 kind */
#define EFFECTS       0x092E2B88u   /* 32, 0x94 bytes: + 90 in use, + 91 kind, + 92 type */

static void sound6(Cpu *c, uint32_t id) { CALL3(f_0887C4CC, 6, id, 0xC0); }

/* The pause's value / row by a pad copy (0x088B9B30 left / right, 0x088B9CEC up / down: pad, value,
   max): pressed moves it (and waits 20), held repeats every 6 (PB + 3), wrapping 0 - max */
static int32_t pause_value(Cpu *c, gaddr pad, int32_t v, int32_t max, uint32_t less, uint32_t more) {
    gaddr copy = c->r[29] + 60;
    for (uint32_t k = 0; k < 11; k++) *W(copy + 4 * k) = *W(pad + 4 * k);
    uint32_t pressed = *W(copy + 12), held = *W(copy + 4);
    int dir;
    if (pressed & less) dir = -1;
    else if (pressed & more) dir = 1;
    else {
        if (held & less) dir = -1;
        else if (held & more) dir = 1;
        else return v;
        int8_t left = (int8_t)(*B(PB + 3) - 1);
        *B(PB + 3) = (uint8_t)left;
        if (left >= 0) return v;
        sound6(c, SOUND_MOVE);
        v += dir;
        *B(PB + 3) = 6;
        goto wrap;
    }
    sound6(c, SOUND_MOVE);
    *B(PB + 3) = 0x14;
    v += dir;
wrap:
    if (dir < 0 && v < 0) v = max;
    if (dir > 0 && max < v) v = 0;
    return v;
}

/* A player's row value (0x088B93BC: SYS, player 0 - 2, rows by player): as BUTTON CONFIG's, through
   0x088B9B30 */
static void pause_config_value(Cpu *c, gaddr sys, uint32_t player, gaddr rows) {
    player &= 0xFF;
    if (player > 2) return;
    gaddr copy = c->r[29] + 16;
    for (uint32_t k = 0; k < 11; k++) *W(copy + 4 * k) = *W(PADS + player * PAD_SIZE + 4 * k);
    int8_t row = S8(rows + player);
    gaddr base = sys + player * 20;
    if (row < 7) CALL4(f_088B95E0, copy, sys, player, (uint32_t)(int32_t)row);
    else if (row == 7) *B(base + 16) = (uint8_t)CALL3(f_088B9B30, copy, (uint32_t)(int32_t)S8(base + 16), 2);
    else if (row == 8) *B(base + 17) = (uint8_t)CALL3(f_088B9B30, copy, (uint32_t)(int32_t)S8(base + 17), 1);
    else if (row == 9) *B(base + 18) = (uint8_t)CALL3(f_088B9B30, copy, (uint32_t)(int32_t)S8(base + 18), 1);
}

/* A button's action (0x088B95E0), as 0x088B6EB4 through 0x088B9B30 */
static void pause_config_action(Cpu *c, gaddr pad, gaddr sys, uint32_t player, uint32_t button) {
    gaddr copy = c->r[29] + 60;
    for (uint32_t k = 0; k < 11; k++) *W(copy + 4 * k) = *W(pad + 4 * k);
    gaddr at = sys + (player & 0xFF) * 20 + (uint32_t)((int16_t)button * 2);
    int16_t old = S16(at);
    uint32_t v = CALL3(f_088B9B30, copy, (uint32_t)(int32_t)old, 0x37);
    if ((int16_t)v != old) *H(at) = (uint16_t)v;
}

/* Remove the objects of a kind (0x088B9F50: kind); kind 1 also restarts the effects of kind 3, types
   2 / 3 */
static void remove_kind(Cpu *c, int16_t kind) {
    for (uint32_t i = 0; i < 16; i++) {
        gaddr o = OBJECTS_16 + i * OBJ_SIZE;
        if (*B(o) != 0 && *B(o + 2) == (uint32_t)(int32_t)kind) CALL1(f_088231F4, o);
    }
    if (kind != 1) return;
    for (uint32_t i = 0; i < 32; i++) {
        gaddr e = EFFECTS + i * 0x94;
        if (*B(e + 90) == 0 || *B(e + 91) != 3) continue;
        uint32_t type = *B(e + 92);
        if (type == 2 || type == 3) CALL1(f_088233BC, e);
    }
}

/* The shown player's pictures (0x088B9EA8): the old ones removed, the panel (3, 5) and, once UNLOCKS
   reaches 8, eight button pictures */
static void pause_config_pictures(Cpu *c) {
    CALL1(f_088B9F50, 0);
    gaddr o = CALL2(f_08823338, 3, 5);
    if (o) { *W(o + 124) = PB_CFG; *H(o + 102) = (uint16_t)(int16_t)S8(PB_CFG + 4); }
    for (uint32_t i = 0; i < 8; i++)
        if (S8(UNLOCKS) >= 8) {
            gaddr p = CALL2(f_08822EE4, 0, (i + (uint32_t)(S8(PB_CFG + 4) * 8)) & 0xFF);
            *W(p + 88) = PB_CFG;
        }
}

/* The pause BUTTON CONFIG, each frame (0x088BA028: SYS, PB): keys clear their player's bit; PB + 0:
   0 (after 5 frames) the shown player's pictures when it changes and that player's input, 1 closing,
   else back to the pause menu; then the drawing */
static void pause_config(Cpu *c, gaddr sys, gaddr st) {
    for (uint32_t pl = 0; pl < 3; pl++)
        if (*W(PADS + pl * PAD_SIZE + 12) != 0 && (S8(PB_CFG + 3) & (1 << pl)))
            *B(PB_CFG + 3) = (uint8_t)(S8(PB_CFG + 3) ^ (1 << pl));
    CALL5(f_0880A6CC, 0x089FFCFCu, 2, 0x54, 5, TEXT_CURSOR);
    int8_t sub = S8(st);
    if (sub == 0) {
        *B(PB_CFG + 5) = 1;
        int8_t t = (int8_t)(S8(st + 2) + 1);
        *B(st + 2) = (uint8_t)t;
        if (t < 5) return;
        *B(st + 2) = 5;
        if (S8(st + 4) != S8(PB_CFG + 4)) CALL0(f_088B9EA8);
        *B(st + 4) = *B(PB_CFG + 4);
        int8_t pl = S8(PB_CFG + 4);
        if (pl == 0) CALL3(f_088B8FC4, sys, GAME, st);
        else if (pl == 1) CALL3(f_088B90E4, sys, GAME, st);
        else if (pl == 2) CALL3(f_088B9208, sys, GAME, st);
    } else if (sub == 1) {
        *B(st) = 2;
        CALL0(f_0880A39C);
        *B(st + 2) = 0;
        *B(PB_CFG + 5) = 0;
    } else {
        gaddr menu = *W(PAUSE_MENU);
        *B(menu + 3) = (uint8_t)(*B(menu + 3) + 1);
        *B(PB_CFG + 2) = 0; *B(PB_CFG + 1) = 0; *B(PB_CFG) = 0;
        *B(st) = 0;
    }
    CALL2(f_088B9790, sys, st);
    *H(st + 6) = (uint16_t)(S16(st + 6) + 1);
}

/* The pause's BUTTON CONFIG step (0x088B8E80): the flags (quiet), then its step by the pause menu's */
static void pause_config_step(Cpu *c) {
    CALL1(f_088B81FC, 1);
    gaddr fn = *W(PAUSE_STEPS + *B(*W(PAUSE_MENU) + 3) * 4u);
    c->r[4] = SYS; c->r[5] = PB;
    rt_call_indirect(c, fn);
}

/* Leaving the pause's BUTTON CONFIG (0x088B9728): outside GAME + 114 = 2 back to the pause menu (its
   + 0 = 1, + 10 = 2), else its pictures removed and on */
static void pause_config_leave(Cpu *c) {
    if (S8(GAME + 114) != 2) {
        CALL0(f_08822B88);
        gaddr p = *W(PAUSE_PICTURES);
        *B(p) = 1; *B(p + 10) = 2;
        return;
    }
    CALL1(f_088B9F50, 0);
    CALL0(f_08848630);
}

/* ---- the functions the extensions wrap (exsetting.c, screensetting.c, widescreen.c) ---- */
void f_08815478(Cpu *c); void f_088D2B54(Cpu *c); void f_088215C4(Cpu *c);
void f_088B7538(Cpu *c); void f_088B700C(Cpu *c); void f_088B7064(Cpu *c);
void f_088B932C(Cpu *c);   /* the pause BUTTON CONFIG's row up / down (not reached) */
void f_088B93BC(Cpu *c);   /* ... left / right (above) */
void f_088B96B0(Cpu *c);   /* ... its DEFAULT (not reached) */
#define MENU_ITEMS    0x08A37A10u   /* each item's submenu (OPT_STATES + 3 on): GAME OPTION, BUTTON CONFIG,
                                       SCREEN SETTING, SAVE/LOAD, EXIT */
#define SL_ROWS_FN    0x08A37A34u   /* SAVE/LOAD's rows: SAVE, LOAD, DELETE, AUTO SAVE, EXIT */
#define MENU_TITLE    0x089FF848u
#define SS_TITLE      0x089FF8ECu
#define SS_SIZES      0x08A37A78u   /* text by SYS + 88 (SCREEN SIZE) */
#define SS_WALLPAPERS 0x08A37A80u   /* by SYS + 510 */
#define SL_TITLE      0x089FF938u
#define SL_AUTO       0x08A37AACu   /* ON / OFF by SYS + 79 (AUTO SAVE) */
#define PB_COMMANDS   0x08A37C44u
#define PB_TYPES      0x08A37C50u
#define PB_RAPIDS     0x08A37C58u
#define GO_BAR_Y      0x08A37D28u   /* s16 by GAME OPTION's row */
#define SS_BAR_Y      0x08A37DA8u
#define SL_BAR_Y      0x08A37DB0u

/* A row or value by a pad, repeated while held (0x088B7EC0 the menus' cursor, 0x088B9B30 / 0x088B9CEC
   the pause's): `less` / `more` keys move it by one and wrap 0 - max; pressed waits 20 frames, held
   repeats every 6 (`counter`). The original copies the pad onto its frame first. */
static int32_t repeat_value(Cpu *c, gaddr pad, int32_t v, int32_t max, uint32_t less, uint32_t more,
                            uint32_t channel, gaddr counter) {
    gaddr copy = c->r[29] + 60;
    for (uint32_t k = 0; k < 11; k++) *W(copy + 4 * k) = *W(pad + 4 * k);
    uint32_t pressed = *W(copy + 12), held = *W(copy + 4);
    int dir;
    if (pressed & less) dir = -1;
    else if (pressed & more) dir = 1;
    else {
        if (held & less) dir = -1;
        else if (held & more) dir = 1;
        else return v;
        int8_t left = (int8_t)(*B(counter) - 1);
        *B(counter) = (uint8_t)left;
        if (left >= 0) return v;
        CALL3(f_0887C4CC, channel, SOUND_MOVE, 0xC0);
        v += dir;
        *B(counter) = 6;
        goto wrap;
    }
    CALL3(f_0887C4CC, channel, SOUND_MOVE, 0xC0);
    *B(counter) = 0x14;
    v += dir;
wrap:
    if (dir < 0 && v < 0) v = max;
    if (dir > 0 && max < v) v = 0;
    return v;
}

/* The menu's objects (the five items and their frame): 0 if one couldn't be made */
static int menu_objects(Cpu *c) {
    static const uint8_t made[][2] = { { 6, 0x10 }, { 6, 0x11 }, { 6, 0x12 }, { 6, 0x13 }, { 6, 0x14 }, { 0xC, 0xD } };
    for (int k = 0; k < 6; k++)
        if (CALL2(f_08823338, made[k][0], made[k][1]) == 0) return 0;
    return 1;
}

/* State 0 (0x088B5FB8: SYS): OPTION MODE starts: its bytes and the timers cleared, SAVE / LOAD's place,
   the CONFIG pads' objects (5 or more pads), the menu's objects */
static void option_start(Cpu *c, gaddr sys) {
    CALL0(f_08815478);
    for (uint32_t i = 0; i < 26; i++) *B(OPT + i) = 0;
    for (uint32_t i = 0; i < 16; i++) *B(TIMER + i) = 0;
    *B(OPT) = (uint8_t)(S8(OPT) + 1);
    *B(TIMER + 2) = 6;
    CALL2(f_088B6108, sys, GAME);
    for (uint32_t i = 0; i < 5; i++)
        if (S8(CONFIG_PADS) >= 5) CALL2(f_08823338, 0xC, (i + 0xF) & 0xFF);
    if (menu_objects(c)) CALL0(f_088D2B54);
}

/* State 1 (0x088B6164: SYS): the menu, or the open submenu (OPT + 6) and its frame. On the menu the
   cursor moves by both pads (the return addresses are the original's: exsetting.c's wrapper on
   0x088B7EC0 tells the menu's calls by them), confirm opens the item, cancel leaves */
static void option_menu(Cpu *c, gaddr sys) {
    if (S8(OPT + 6) != 0) {
        *B(OPT + 5) = (uint8_t)(S8(OPT + 1) + 1);
        gaddr fn = *W(MENU_ITEMS + (uint32_t)(S8(OPT + 1) * 4));
        c->r[4] = sys; c->r[5] = GAME;
        rt_call_indirect(c, fn);
        CALL0(f_088B7538);
        return;
    }
    *B(OPT + 5) = 0;
    CALL5(f_0880A6CC, MENU_TITLE, 2, 0x68, 5, TEXT_CURSOR);
    c->r[31] = 0x088B61FCu;
    *B(OPT + 1) = (uint8_t)CALL3(f_088B7EC0, PADS, (uint32_t)(int32_t)S8(OPT + 1), 4);
    c->r[31] = 0x088B6210u;
    uint32_t v = CALL3(f_088B7EC0, PADS + PAD_SIZE, (uint32_t)(int32_t)S8(OPT + 1), 4);
    uint32_t pressed = pads_pressed();
    *B(OPT + 1) = (uint8_t)v;
    if (pressed & PAD_CONFIRM) {
        if (S8(OPT + 6) == 0) sound(c, SOUND_OK);
        CALL0(f_0880A39C);
        *B(OPT + 7) = 0;
        *B(OPT + 6) = 1;
        *B(TIMER + 2) = 0x14;
        CALL1(f_088B8078, (uint32_t)(int32_t)S8(OPT + 1) & 0xFFFF);
    } else if (pressed & PAD_CANCEL) {
        CALL0(f_0880AA74);
        sound(c, SOUND_BACK);
        CALL0(f_0880A39C);
        *B(OPT) = (uint8_t)(S8(OPT) + 1);
    }
}

/* SCREEN SIZE (0x088B700C: SYS): SYS + 88, 0 NORMAL / 1 WIDE, by both pads */
static void screen_size(Cpu *c, gaddr sys) {
    *H(sys + 88) = (uint16_t)CALL3(f_088B7D08, PADS, (uint32_t)(int32_t)S16(sys + 88), 1);
    *H(sys + 88) = (uint16_t)CALL3(f_088B7D08, PADS + PAD_SIZE, (uint32_t)(int32_t)S16(sys + 88), 1);
}

/* SAVE/LOAD, each frame (0x088B70F8: SYS, -): the row (OPT + 7) by both pads, its handler (0x08A37A34),
   then cancel, or confirm on EXIT, closes */
static void save_load(Cpu *c, gaddr sys, uint32_t a1) {
    c->r[31] = 0x088B7144u;
    *B(OPT + 7) = (uint8_t)CALL3(f_088B7EC0, PADS, (uint32_t)(int32_t)S8(OPT + 7), 4);
    c->r[31] = 0x088B7158u;
    *B(OPT + 7) = (uint8_t)CALL3(f_088B7EC0, PADS + PAD_SIZE, (uint32_t)(int32_t)S8(OPT + 7), 4);
    gaddr fn = *W(SL_ROWS_FN + (uint32_t)(S8(OPT + 7) * 4));
    c->r[4] = sys; c->r[5] = a1;
    rt_call_indirect(c, fn);
    if (pads_pressed() & PAD_CANCEL) {
        sound(c, SOUND_BACK);
        CALL0(f_0880A39C);
        *B(OPT + 7) = 0; *B(OPT + 6) = 0;
    }
    if ((pads_pressed() & PAD_CONFIRM) && S8(OPT + 7) == 4) {
        sound(c, SOUND_OK);
        CALL0(f_0880A39C);
        *B(OPT + 7) = 0; *B(OPT + 6) = 0;
    }
}

/* SCREEN SETTING's text (0x088B78DC): four rows (the cursor's brighter), SCREEN SIZE and WALLPAPER */
static void screen_text(Cpu *c) {
    uint32_t cur = (uint32_t)(int32_t)S8(OPT + 7) & 0xFF;
    CALL5(f_0880A6CC, SS_TITLE, 2, 0x50, 5, TEXT_CURSOR);
    static const uint16_t labels[4] = { 0x8FC, 0x908, 0x8A4, 0x8AC };
    for (uint32_t r = 0; r < 4; r++)
        CALL5(f_0880A6CC, 0x089FF000u + labels[r], 1, 0x6C, 0x40 + 0x18 * r, cur == r ? TEXT_CURSOR : TEXT_ROW);
    CALL5(f_0880A6CC, *W(SS_SIZES + (uint32_t)(S16(SYS + 88) * 4)), 1, 0xDC, 0x40, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(SS_WALLPAPERS + *B(SYS + 510) * 4u), 1, 0xDC, 0x58, TEXT_CURSOR);
}

/* SAVE/LOAD's text (0x088B7AE8): five rows, the title, AUTO SAVE's ON / OFF */
static void save_load_text(Cpu *c) {
    uint32_t cur = (uint32_t)(int32_t)S8(OPT + 7) & 0xFF;
    static const uint16_t labels[5] = { 0x914, 0x91C, 0x924, 0x92C, 0x8AC };
    for (uint32_t r = 0; r < 5; r++)
        CALL5(f_0880A6CC, 0x089FF000u + labels[r], 1, 0x80, 0x38 + 0x18 * r, cur == r ? TEXT_CURSOR : TEXT_ROW);
    CALL5(f_0880A6CC, SL_TITLE, 2, 0x78, 5, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(SL_AUTO + (uint32_t)(S8(SYS + 79) * 4)), 1, 0xE0, 0x80, TEXT_CURSOR);
}

/* SCREEN SETTING, each frame (0x088B85BC: SYS, GAME): as GAME OPTION's frame, with four rows: SCREEN
   SIZE, WALLPAPER, DEFAULT (SYS + 84 / + 86 0, WALLPAPER 0, WIDE), EXIT */
static void screen_frame(Cpu *c, gaddr sys, gaddr game) {
    int8_t sub = S8(OPT + 2);
    if (sub == 0) {
        int8_t t = (int8_t)(S8(TIMER) + 1);
        *B(TIMER) = (uint8_t)t;
        if (t < 5) return;
        *B(TIMER) = 5;
        uint32_t pressed = pads_pressed();
        if (pressed & PAD_UP) cursor_step(c, OPT + 7, -1, 4, 0x14);
        else if (pressed & PAD_DOWN) cursor_step(c, OPT + 7, 1, 4, 0x14);
        else {
            uint32_t held = pads_repeat();
            if (held & PAD_UP) cursor_held(c, OPT + 7, -1, 4);
            else if (held & PAD_DOWN) cursor_held(c, OPT + 7, 1, 4);
            else if (pressed & PAD_CANCEL) { sound(c, SOUND_BACK); *B(OPT + 2) = (uint8_t)(S8(OPT + 2) + 1); }
            else {
                int8_t row = S8(OPT + 7);
                if (row == 0) CALL2(f_088B700C, sys, game);
                else if (row == 1) CALL2(f_088B7064, sys, game);
                else if (row == 2) {
                    if (pressed & PAD_CONFIRM) {
                        sound(c, SOUND_OK);
                        *H(sys + 86) = 0; *H(sys + 84) = 0; *B(sys + 510) = 0; *H(sys + 88) = 1;
                    }
                } else if (row == 3) {
                    if (pressed & PAD_CONFIRM) { sound(c, SOUND_OK); *B(OPT + 2) = (uint8_t)(S8(OPT + 2) + 1); }
                }
            }
        }
    } else if (sub == 1) {
        *B(OPT + 2) = 2;
        CALL0(f_088B8188);
        CALL0(f_0880A39C);
        *B(OPT + 6) = 0;
    } else {
        *B(OPT + 2) = 0;
        *B(OPT + 7) = 0;
    }
}

/* The pause's BUTTON CONFIG opens (0x088B8ED0: -, PB): its bytes cleared, the bar (3, 6), whose
   settings by who paused (GAME + 89's bits; the first only if the bar was made), the pictures */
static void pause_config_open(Cpu *c, gaddr st) {
    *B(0x0950FF40u + 10) = 3;
    gaddr menu = *W(PAUSE_MENU);
    *B(menu + 3) = (uint8_t)(*B(menu + 3) + 1);
    for (uint32_t i = 0; i < 8; i++) *B(PB + i) = 0;
    for (uint32_t i = 0; i < 6; i++) *B(PB_CFG + i) = 0;
    gaddr bar = CALL2(f_08823338, 3, 6);
    int8_t who = S8(GAME + 89);
    if (bar && (who & 1)) *B(PB_CFG + 4) = 0;
    if (who & 2) *B(PB_CFG + 4) = 1;
    if (who & 4) *B(PB_CFG + 4) = 2;
    *B(st + 4) = *B(PB_CFG + 4);
    CALL0(f_088B9EA8);
}

/* A player's input in the pause's BUTTON CONFIG (0x088B8FC4 / 90E4 / 9208: SYS, GAME, PB) */
static void pause_config_input(Cpu *c, gaddr sys, gaddr st, uint32_t player) {
    gaddr pad = PADS + player * PAD_SIZE;
    uint32_t keys = *W(pad + 12) | *W(pad + 4);
    if (keys & 0xC) CALL3(f_088B932C, sys, player, PB_CFG);
    else if (keys & 3) CALL3(f_088B93BC, sys, player, PB_CFG);
    uint32_t pressed = *W(pad + 12);
    if (pressed & 0x80) {
        int8_t row = S8(PB_CFG + player);
        if (row == BC_DEFAULT) { sound6(c, SOUND_OK); CALL2(f_088B96B0, sys, player); }
        else if (row == BC_EXIT) { sound6(c, SOUND_OK); CALL0(f_0880A39C); *B(st) = (uint8_t)(S8(st) + 1); }
    } else if (pressed & PAD_CANCEL) {
        sound6(c, SOUND_BACK);
        *B(st) = (uint8_t)(S8(st) + 1);
    }
}

/* The pause's BUTTON CONFIG drawing (0x088B9790: SYS): as BUTTON CONFIG's text, for the player whose
   settings they are (the cursor is the first player's row for player 1, the second's for the others) */
static void pause_config_text(Cpu *c, gaddr sys) {
    uint32_t cur = (uint32_t)(int32_t)(S8(PB_CFG + 4) == 0 ? S8(PB_CFG) : S8(PB_CFG + 1)) & 0xFF;
    CALL3(f_0880A7B0, 8, 0x6C, 0x38);
    CALL3(f_0880A7B0, 9, 0x6C, 0x68);
    CALL3(f_0880A7B0, 2, 0x68, 0x18);
    CALL3(f_0880A7B0, 1, 0x68, 0x28);
    CALL3(f_0880A7B0, 3, 0x68, 0x48);
    CALL3(f_0880A7B0, 0, 0x68, 0x58);
    CALL3(f_0880A7F4, 0, 0x68, 0x78);
    CALL5(f_0880A6CC, *W(PB_COMMANDS + (uint32_t)(S8(sys + (uint32_t)(S8(PB_CFG + 4) * 20) + 16) * 4)), 1, 0xB0, 0x88, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(PB_TYPES + (uint32_t)(S8(sys + (uint32_t)(S8(PB_CFG + 4) * 20) + 17) * 4)), 1, 0xB0, 0x98, TEXT_CURSOR);
    CALL5(f_0880A6CC, *W(PB_RAPIDS + (uint32_t)(S8(sys + (uint32_t)(S8(PB_CFG + 4) * 20) + 18) * 4)), 1, 0xB0, 0xA8, TEXT_CURSOR);
    static const uint16_t labels[5] = { 0xCD4, 0xCDC, 0xCE4, 0xCEC, 0xCF4 };
    for (uint32_t r = 0; r < 5; r++)
        CALL5(f_0880A6CC, 0x089FF000u + labels[r], 1, 0x68, 0x88 + 0x10 * r, cur == 7 + r ? TEXT_CURSOR : TEXT_ROW);
    gaddr base = sys + (uint32_t)(S8(PB_CFG + 4) * 20);
    for (uint32_t i = 0; i < 7; i++) {
        uint32_t v = *H(base + 2 * i);
        if ((int32_t)v >= 15) CALL5(f_0880A6CC, *W(BC_ACTIONS + v * 4), 1, 0xB0, 16 * i + 0x18, TEXT_CURSOR);
    }
}

/* The menu's items, each frame (0x088BA82C, kinds 15 - 19): the cursor's bright and bigger; shown on
   the menu */
static void item_run(gaddr t) {
    int32_t item = (int32_t)*B(t + P_KIND) - 15;
    uint8_t v;
    if (S8(OPT + 1) == item) {
        *B(t + 56) = 0;
        *B(t + P_ALPHA) = 0xFF;
        *H(t + P_U) = (uint16_t)-4; *H(t + P_V) = (uint16_t)-4;
        *H(t + P_W) = 0xC8; *H(t + P_H) = 0x48;
        v = 0xFF;
    } else {
        *B(t + 56) = 1;
        *B(t + P_ALPHA) = 0x80;
        *H(t + P_U) = 0; *H(t + P_V) = 0;
        *H(t + P_W) = 0xC0; *H(t + P_H) = 0x40;
        v = 0x80;
    }
    tints(t, v, v, v);
    *B(t + P_SHOWN) = S8(OPT + 5) == 0 ? 1 : 0;
}

/* GAME OPTION's bar, each frame (0x088BA8F4, kind 4): the pulse, on the cursor's row, shown with
   submenu 1, started again once it closes */
static void kind4_run(Cpu *c, gaddr t) {
    CALL1(f_088BAC18, t);
    *H(t + P_Y) = *H(GO_BAR_Y + (uint32_t)(S8(OPT + 7) * 2));
    *B(t + P_SHOWN) = S8(OPT + 5) == 1 ? 1 : 0;
    if (S8(OPT + 6) == 0) CALL1(f_088BABFC, t);
}

/* SCREEN SETTING's bar (0x088BAA90, kinds 9 / 10) and SAVE/LOAD's (0x088BAB44, kind 20): the bar (10 /
   20) on the row, the pulse while OPT + 11 is clear; shown with submenu 3 / 4 */
static void bar_run(Cpu *c, gaddr t, uint32_t bar_kind, gaddr ys, int8_t submenu) {
    if (*B(t + P_KIND) == bar_kind) {
        *H(t + P_Y) = *H(ys + (uint32_t)(S8(OPT + 7) * 2));
        if (S8(OPT + 11) == 0) { *H(t + P_H) = 0x12; CALL1(f_088BAC18, t); }
    }
    *B(t + P_SHOWN) = S8(OPT + 5) == submenu ? 1 : 0;
    if (S8(OPT + 6) == 0) CALL1(f_088BABFC, t);
}

/* BUTTON CONFIG's pictures, each frame (0x088BB268): which show: 1 - 8 DIFFICULTY's stars, 19 - 22
   DAMAGE's, 29 TIME's no-limit, 36 - 42 the buttons' (an action with a picture; its animation set
   again on the cursor's row or while a player's bit is up); then drawn */
static void picture_run(Cpu *c, gaddr o) {
    int32_t kind = *B(o + 3);
    uint8_t shown;
    if (kind >= 36 && kind < 43) {
        uint32_t player = (S8(GAME + 53) & 1) ? 0 : 1;
        int16_t action = S16(SYS + player * 20 + (uint32_t)kind * 2 - 72);
        if (action >= 15) shown = 0;
        else {
            if (kind - 36 == S8(OPT + 3) || (S8(OPT + 8) & 7))
                CALL2(f_0882196C, o, *W(PIC_ANIMS + (uint32_t)(action * 4)));
            shown = 1;
        }
    } else if (kind > 0 && kind < 9) shown = (S8(GAME + 45) + 1 < kind) ? 0 : 1;
    else if (kind >= 19 && kind < 23) shown = (S8(GAME + 46) + 0x13 < kind) ? 0 : 1;
    else if (kind == 29) shown = S8(GAME + 47) == 3 ? 1 : 0;
    else if (kind == 28 || kind == 30 || kind == 0 || kind == 9 || kind == 18 || kind == 23) shown = 0;
    else shown = 1;
    *B(o + 1) = shown;
    if (*B(o + 1)) CALL1(f_088215C4, o);
}

#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
SFA_FUNC(088B5F48, game_option_task)            { FRAME(0x20, option_task(c)); }
SFA_FUNC(088B6108, game_option_save_place)      { save_place(ARG(1)); }
SFA_FUNC(088B7064, game_option_sound_value)     { FRAME(0x20, sound_value(c, ARG(0))); }
SFA_FUNC(088B70C0, game_option_screen_apply)    { FRAME(0x20, apply_screen(c)); }
SFA_FUNC(088B7270, game_option_submenu_close)   { FRAME(0x20, submenu_close(c)); }
SFA_FUNC(088B7364, game_option_confirm_save)    { FRAME(0x20, submenu_confirm_save(c)); }
SFA_FUNC(088B73B8, game_option_confirm_apply)   { FRAME(0x20, submenu_confirm_apply(c, ARG(0), ARG(1))); }
SFA_FUNC(088B745C, game_option_stick_check)     { FRAME(0x20, stick_check(c)); }
SFA_FUNC(088B7538, game_option_submenu_frame)   { FRAME(0x10, submenu_frame(c)); }
SFA_FUNC(088B7D08, game_option_value_by_pad)    { int32_t v; FRAME(0x80, v = value_by_pad(c, ARG(0), (int32_t)ARG(1), (int32_t)ARG(2))); RETURN((uint32_t)v); }
SFA_FUNC(088B8078, game_option_item_objects)    { FRAME(0x20, item_objects(c, ARG(0))); }
SFA_FUNC(088B8188, game_option_remove_pictures) { FRAME(0x20, remove_pictures(c)); }
SFA_FUNC(088B81FC, game_option_flags)           { FRAME(0x20, option_flags(c, (int8_t)ARG(0))); }
SFA_FUNC(088B8DFC, game_option_to_game)         { options_to_game(); }
SFA_FUNC(088BA238, game_option_menu_step)       { FRAME(0x20, run_step(c, MENU_STEPS)); }
SFA_FUNC(088BA26C, game_option_menu_start)      { FRAME(0x20, menu_picture_start(c, ARG(0))); }
SFA_FUNC(088BA4A8, game_option_band_start)      { band_start(ARG(0)); }
SFA_FUNC(088BA5F8, game_option_kind20_start)    { kind20_start(ARG(0)); }
SFA_FUNC(088BA6CC, game_option_menu_run)        { FRAME(0x20, menu_picture_run(c, ARG(0))); }
SFA_FUNC(088BABFC, game_option_restart)         { FRAME(0x20, (c->r[31] = 0x088BAC0C, f_088233BC(c))); }
SFA_FUNC(088BAC18, game_option_pulse)           { pulse(ARG(0)); }
SFA_FUNC(088BAC84, game_option_item_start)      { item_start(ARG(0)); }
SFA_FUNC(088BAF94, game_option_kind9_start)     { kind9_start(ARG(0)); }
SFA_FUNC(088BE7DC, game_option_sub_step)        { FRAME(0x20, run_step(c, SUB_STEPS)); }
SFA_FUNC(088BE810, game_option_sub_start)       { FRAME(0x40, sub_picture_start(c, ARG(0))); }
SFA_FUNC(088BE970, game_option_sub_title)       { FRAME(0x20, sub_title(c, ARG(0))); }
SFA_FUNC(088BEA0C, game_option_sub_line)        { FRAME(0x20, sub_line(c, ARG(0))); }
SFA_FUNC(088BEAD8, game_option_set_frame)       { set_frame(ARG(0), ARG(1)); }
SFA_FUNC(088BEB1C, game_option_sub_shown)       { sub_shown(ARG(0)); }
SFA_FUNC(088BEB74, game_option_choice_run)      { FRAME(0x30, choice_run(c, ARG(0))); }
SFA_FUNC(088BEC7C, game_option_choice_first)    { choice_first(ARG(0)); }
SFA_FUNC(088BED50, game_option_choice_other)    { choice_other(ARG(0)); }
SFA_FUNC(088B62CC, game_option_leave)           { FRAME(0x20, option_leave(c)); }
SFA_FUNC(088B6318, game_option_row_difficulty)  { FRAME(0x20, row_plain(c, ARG(0), ARG(1), 45, 7, 60)); }
SFA_FUNC(088B6384, game_option_row_time)        { FRAME(0x20, row_plain(c, ARG(0), ARG(1), 47, 3, 61)); }
SFA_FUNC(088B63F0, game_option_row_rounds)      { FRAME(0x20, row_plain(c, ARG(0), ARG(1), 76, 2, 62)); }
SFA_FUNC(088B645C, game_option_row_damage)      { FRAME(0x20, row_damage(c, ARG(0), ARG(1))); }
SFA_FUNC(088B64D0, game_option_row_speed)       { FRAME(0x30, row_speed(c, ARG(0), ARG(1))); }
SFA_FUNC(088B65B4, game_option_row_gauge)       { FRAME(0x20, row_plain(c, ARG(0), ARG(1), 119, 1, 69)); }
SFA_FUNC(088B6620, game_option_row_side)        { FRAME(0x20, row_side(c, ARG(0))); }
SFA_FUNC(088B6678, game_option_row_select)      { FRAME(0x20, row_select(c, ARG(0))); }
SFA_FUNC(088B66D0, game_option_row_music)       { FRAME(0x20, row_volume(c, ARG(0), ARG(1), 1)); }
SFA_FUNC(088B6744, game_option_row_effects)     { FRAME(0x20, row_volume(c, ARG(0), ARG(1), 0)); }
SFA_FUNC(088B67B8, game_option_row_default)     { FRAME(0x20, row_default(c, ARG(0), ARG(1))); }
SFA_FUNC(088B68D0, game_option_row_exit)        { FRAME(0x20, row_exit(c)); }
SFA_FUNC(088B8894, game_option_text)            { FRAME(0x60, go_text(c)); }
SFA_FUNC(088B74C4, game_option_done)            { FRAME(0x20, option_done(c)); }
SFA_FUNC(088B81F4, game_option_nothing)         { }
SFA_FUNC(088B82E4, game_option_frame)           { FRAME(0x40, go_frame(c, ARG(0), ARG(1))); }
SFA_FUNC(088B6924, game_option_buttons)         { FRAME(0x30, button_config(c, ARG(0), ARG(1))); }
SFA_FUNC(088B6A58, game_option_buttons_p1)      { FRAME(0x20, button_config_player(c, ARG(0), 0)); }
SFA_FUNC(088B6B7C, game_option_buttons_p2)      { FRAME(0x20, button_config_player(c, ARG(0), 1)); }
SFA_FUNC(088B6CA0, game_option_buttons_row)     { FRAME(0x20, button_config_row(c, ARG(1))); }
SFA_FUNC(088B6D10, game_option_buttons_value)   { FRAME(0x50, button_config_value(c, ARG(0), ARG(1))); }
SFA_FUNC(088B6EB4, game_option_buttons_action)  { FRAME(0x80, button_config_action(c, ARG(0), ARG(1), ARG(2), ARG(3))); }
SFA_FUNC(088B6F84, game_option_buttons_default) { button_config_default(ARG(0), ARG(1), ARG(2)); }
SFA_FUNC(088B7578, game_option_buttons_text)    { FRAME(0x50, button_config_text(c)); }
SFA_FUNC(088BA3F0, game_option_kind4_start)     { kind4_start(ARG(0)); }
SFA_FUNC(088BA974, game_option_kind5_run)       { FRAME(0x20, kind5_run(c, ARG(0))); }
SFA_FUNC(088BADEC, game_option_kind5_start)     { kind5_start(ARG(0)); }
SFA_FUNC(088BB0D0, game_option_picture_step)    { FRAME(0x20, picture_step(c)); }
SFA_FUNC(088BB104, game_option_picture_start)   { FRAME(0x20, picture_start(c, ARG(0))); }
SFA_FUNC(088B8E80, game_pause_config_step)      { FRAME(0x20, pause_config_step(c)); }
SFA_FUNC(088B93BC, game_pause_config_value)     { FRAME(0x50, pause_config_value(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088B95E0, game_pause_config_action)    { FRAME(0x80, pause_config_action(c, ARG(0), ARG(1), ARG(2), ARG(3))); }
SFA_FUNC(088B9728, game_pause_config_leave)     { FRAME(0x20, pause_config_leave(c)); }
SFA_FUNC(088B9B30, game_pause_value_lr)         { int32_t v; FRAME(0x80, v = pause_value(c, ARG(0), (int32_t)ARG(1), (int16_t)ARG(2), 2, 1)); RETURN((uint32_t)v); }
SFA_FUNC(088B9CEC, game_pause_value_ud)         { int32_t v; FRAME(0x80, v = pause_value(c, ARG(0), (int32_t)ARG(1), (int8_t)ARG(2), 8, 4)); RETURN((uint32_t)v); }
SFA_FUNC(088B9EA8, game_pause_config_pictures)  { FRAME(0x20, pause_config_pictures(c)); }
SFA_FUNC(088B9F50, game_option_remove_kind)     { FRAME(0x30, remove_kind(c, (int16_t)ARG(0))); }
SFA_FUNC(088BA028, game_pause_config)           { FRAME(0x40, pause_config(c, ARG(0), ARG(1))); }

/* Wrapped by the extensions: see exsetting.c, screensetting.c and widescreen.c */
SFA_FUNC_WRAPPED(088B5FB8, game_option_start, exsetting_option_start)        { FRAME(0x20, option_start(c, ARG(0))); }
SFA_FUNC_WRAPPED(088B6164, game_option_menu, exsetting_option_menu)          { FRAME(0x30, option_menu(c, ARG(0))); }
SFA_FUNC_WRAPPED(088B700C, game_option_screen_size, widescreen_screen_size)  { FRAME(0x20, screen_size(c, ARG(0))); }
SFA_FUNC_WRAPPED(088B70F8, game_option_save_load, exsetting_save_load)       { FRAME(0x30, save_load(c, ARG(0), ARG(1))); }
SFA_FUNC_WRAPPED(088B78DC, game_option_screen_text, screensetting_text)      { FRAME(0x50, screen_text(c)); }
SFA_FUNC_WRAPPED(088B7AE8, game_option_save_load_text, exsetting_save_load_text) { FRAME(0x50, save_load_text(c)); }
SFA_FUNC_WRAPPED(088B7EC0, game_option_cursor, exsetting_menu_cursor)        { int32_t v; FRAME(0x80, v = repeat_value(c, ARG(0), (int32_t)ARG(1), (int8_t)ARG(2), PAD_UP, PAD_DOWN, 0, TIMER + 2)); RETURN((uint32_t)v); }
SFA_FUNC_WRAPPED(088B85BC, game_option_screen, screensetting_frame)          { FRAME(0x20, screen_frame(c, ARG(0), ARG(1))); }
SFA_FUNC_WRAPPED(088B8ED0, game_pause_config_open, exsetting_pause_open)     { FRAME(0x20, pause_config_open(c, ARG(1))); }
SFA_FUNC_WRAPPED(088B8FC4, game_pause_config_p1, exsetting_pause_input_1)    { FRAME(0x20, pause_config_input(c, ARG(0), ARG(2), 0)); }
SFA_FUNC_WRAPPED(088B90E4, game_pause_config_p2, exsetting_pause_input_2)    { FRAME(0x20, pause_config_input(c, ARG(0), ARG(2), 1)); }
SFA_FUNC_WRAPPED(088B9208, game_pause_config_p3, exsetting_pause_input_3)    { FRAME(0x20, pause_config_input(c, ARG(0), ARG(2), 2)); }
SFA_FUNC_WRAPPED(088B9790, game_pause_config_text, exsetting_pause_text)     { FRAME(0x50, pause_config_text(c, ARG(0))); }
SFA_FUNC_WRAPPED(088BA82C, game_option_item_run, exsetting_option_button)    { item_run(ARG(0)); }
SFA_FUNC_WRAPPED(088BA8F4, game_option_kind4_run, exsetting_green_bar)       { FRAME(0x20, kind4_run(c, ARG(0))); }
SFA_FUNC_WRAPPED(088BAA90, game_option_kind9_run, screensetting_bar)         { FRAME(0x20, bar_run(c, ARG(0), 10, SS_BAR_Y, 3)); }
SFA_FUNC_WRAPPED(088BAB44, game_option_kind20_run, exsetting_save_load_bar)  { FRAME(0x20, bar_run(c, ARG(0), 20, SL_BAR_Y, 4)); }
SFA_FUNC_WRAPPED(088BB268, game_option_picture_run, exsetting_attack_picture) { FRAME(0x20, picture_run(c, ARG(0))); }
