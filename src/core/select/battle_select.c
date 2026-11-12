/* The select screen of the modes that don't run the character select's own table (screen.c): Free
 * Battle, Survival, Training, World Tour and the rest (fight.c's screen 1 hands them to 0x088A0664).
 * Up to four fighters take part (two players, and in some layouts the opponents FIGHTER_3 / FIGHTER_4);
 * BATTLE + 56 is the layout:
 *
 *   0  two players, each with a fighter      2  player 2 against two opponents
 *   1  player 1 against two opponents        3  two players against one opponent
 *
 * Its steps run by GAME + 4 from 0x08A2B5E4 (0 the start's own sub-steps by GAME + 6, 2 the frame,
 * 3 the end). This file has the step dispatch and the frame: each fighter's cursor from the table at
 * 0x08A2B7D8, the title, SPEED:, the stage, the entries' names, and moving on
 * once everyone has chosen. selectscreen.c wraps the SPEED: line (0x088A1548): WAIT on the boxes.
 */
#include "select.h"

void rt_call_indirect(Cpu *c, uint32_t addr);

#define STEPS          0x08A2B5E4u   /* by GAME + 4 */
#define START_STEPS    0x08A2B5F4u   /* by GAME + 6 */
#define CURSOR_STEPS   0x08A2B7D8u   /* a fighter's cursor, by its + 4 */
#define COLOURS        0x08A2B608u   /* u16 by mode: the screen's text colour */
#define TEXT_COLOUR    0x08B02716u   /* u16: the colour now (low byte used) */
#define TITLES         0x08A2B630u   /* text by mode: the screen's title */
#define SPEED_LABEL    0x089FF444u   /* "SPEED:" */
#define SPEED_NAMES    0x08A2B688u   /* text by (speed - 1) × 2 + GAME + 124 bit 0 */
#define STAGE_LABEL    0x089FF44Cu   /* "STAGE:" */
#define STAGE_NAMES    0x08A2B428u   /* text by GAME + 193 (0 RANDOM, then the 38 stages) */
#define ENTRY_NAMES    0x08A2B698u   /* text by entry */
#define ENTRY_PLACES   0x08A2B6B0u   /* { x, y } (s32) by entry */
#define SAVED_ENTRIES  0x09519588u   /* six entries of 0x354 bytes (+0 in use) */
#define SIDE_LIST_0    0x09517A28u   /* per side, handed to the cursors (which one by GAME + 53) */
#define SIDE_LIST_1    0x09517A50u

#define B_LAYOUT       56     /* s16 */
#define B_FIGHT_STATE  79     /* s8 */
#define B_FREE_KIND    159    /* s8: Free Battle's kind (ARCADE-MODE, TEAM, VARIABLE, DRAMATIC, R.DRAMATIC ...) */

#define F_DRIVER       820    /* u32: which of the four fighters' pads moves this one's box (0 - 3) */
#define F_CURSOR_ON_W  824    /* u32 */
#define F_PLAYED       352    /* u32: cleared for both players in Survival's fights 4 and 5 */

#define MODE_TRAINING  2
#define MODE_6         6
#define MODE_SURVIVAL  7
#define MODE_13        13
#define MODE_EDIT      14
#define MODE_FREE      16
#define MODE_19        19

void f_0880A6CC(Cpu *c);   /* queues text (a0 text, a1, a2 x, a3 y, t0 font / colour): the entry */
void f_088D5E44(Cpu *c);   /* a network battle (fight.c) */
void f_088D7CE0(Cpu *c);   /* a network session that has started (mode 12 or 0x09523980 + 34, and + 42 = 1) */
void f_088E358C(Cpu *c);   /* the network lets the frame run */
void f_08837690(Cpu *c); void f_08887AFC(Cpu *c); void f_0880B6CC(Cpu *c); void f_08819144(Cpu *c);
void f_088D6378(Cpu *c);
void f_088E348C(Cpu *c);   /* (game, 1) */
void f_088A60F8(Cpu *c);   /* (fighter, which): its box and what it has chosen (below) */
void f_088A5600(Cpu *c);   /* (a, b, c): whose cursor runs, Free Battle */
void f_088A1790(Cpu *c);   /* (a, b, c): whose cursor runs, Survival */
void f_088A1750(Cpu *c);   /* (out a, out b): the two sides' cursor lists (0x09517A28 / A50), by GAME + 0x35 */
void f_088A5914(Cpu *c);   /* (a, b, c): whose cursor runs, modes 13 / 19 */
void f_088A1BA4(Cpu *c);   /* (a, b, c): three fighters' colours kept apart */
void f_088A06D8(Cpu *c); void f_088A1478(Cpu *c); void f_088A5B38(Cpu *c); void f_088A14B0(Cpu *c);
void f_088A1548(Cpu *c); void f_088A160C(Cpu *c); void f_088A167C(Cpu *c); void f_088A1AB8(Cpu *c);
void f_088A1D30(Cpu *c); void f_088A225C(Cpu *c); void f_088A1F38(Cpu *c); void f_088A1CE8(Cpu *c);
void f_088A1D08(Cpu *c); void f_088A1ED8(Cpu *c); void f_088A1828(Cpu *c);   /* below */

static int8_t mode_now(void) { return S8(GAME + G_MODE); }

/* ---- the steps ---- */

/* The screen's step (0x088A0664) and the start's sub-step (0x088A06A4), each from its table: (game,
   battle), with the handler left in a2 as the original leaves it. */
static void run_step(Cpu *c, gaddr table, uint32_t index) {
    gaddr fn = *W(table + index * 4u);
    c->r[4] = GAME; c->r[5] = BATTLE; c->r[6] = fn;
    rt_call_indirect(c, fn);
}
static void start_step(Cpu *c, gaddr g) {
    gaddr fn = *W(START_STEPS + *H(g + G_STEP) * 4u);
    c->r[6] = fn; c->r[7] = START_STEPS;   /* a0, a1 handed on as they came */
    rt_call_indirect(c, fn);
}

/* The frame step (0x088A13F8), while the network lets it run: the first frame sets the colour, the
   rest run the screen; then the object lists and the end of the frame. */
static void frame_step(Cpu *c, gaddr g, gaddr battle) {
    if (CALL0(f_088E358C) != 0) {
        CALL2(*H(g + G_STEP) == 0 ? f_088A1478 : f_088A5B38, g, battle);
        CALL0(f_08837690);
        CALL0(f_08887AFC);
    }
    CALL0(f_0880B6CC);
    CALL0(f_08819144);
}

/* The first frame (0x088A1478): the mode's text colour. */
static void first_frame(gaddr g) {
    *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1);
    *H(TEXT_COLOUR) = *H(COLOURS + (uint32_t)(int32_t)S8(GAME + G_MODE) * 2u);
}

/* ---- the text ---- */

static uint32_t text(Cpu *c, gaddr t, uint32_t x, uint32_t y, uint32_t font) {
    return CALL5(f_0880A6CC, t, 3, x, y, font);
}

/* The title (0x088A14B0); outside modes 13 and 19 its entry's + 3 is 6. */
static void title(Cpu *c, gaddr g) {
    uint32_t mode = (uint16_t)(int32_t)S8(g + G_MODE);
    gaddr e = text(c, *W(TITLES + mode * 4u), 0x58, 6, *H(TEXT_COLOUR) & 0xFF);
    if (mode != MODE_13 && mode != MODE_19) *B(e + 3) = 6;
}

/* SPEED: and the speed (0x088A1548), when there is one and not in EDIT. Wrapped by selectscreen.c. */
static void speed_line(Cpu *c, gaddr g) {
    uint32_t speed = (uint16_t)(int32_t)S8(g + G_SPEED_BASE);
    if (speed == 0 || S8(g + G_MODE) == MODE_EDIT) return;
    text(c, SPEED_LABEL, 0x78, 0xC2, *H(TEXT_COLOUR) & 0xFF);
    uint32_t k = (uint16_t)((uint16_t)((uint16_t)(speed - 1) * 2) + (*B(g + 124) & 1));
    text(c, *W(SPEED_NAMES + k * 4u), 0xC0, 0xC2, *H(TEXT_COLOUR) & 0xFF);
}

/* The stage chosen (0x088A160C), unless GAME + 195 hides it. */
static void stage_line(Cpu *c, gaddr g) {
    if (S8(g + 195) != 0) return;
    text(c, STAGE_LABEL, 0x72, 0xD2, 3);
    text(c, *W(STAGE_NAMES + (uint32_t)(int32_t)S8(g + 193) * 4u), 0xBA, 0xD2, 3);
}

/* Whether the mode shows the entries' names (0x088A06D8): not World Tour, EDIT, modes 13 / 19,
   Survival's fights 4 / 5, or Free Battle's kinds 3 / 4. */
static int names_shown(void) {
    int8_t mode = mode_now();
    int8_t state = S8(BATTLE + B_FIGHT_STATE), kind = S8(BATTLE + B_FREE_KIND);
    if (mode == MODE_WORLD_TOUR || mode == MODE_EDIT || mode == MODE_13 || mode == MODE_19) return 0;
    if (mode == MODE_SURVIVAL && (state == 4 || state == 5)) return 0;
    if (mode == MODE_FREE && (kind == 4 || kind == 3)) return 0;
    return 1;
}

/* The entries' names (0x088A167C), offline. */
static void entry_names(Cpu *c) {
    if (CALL0(f_088D5E44) != 0) return;
    for (uint32_t k = 0; k < 6; k++) {
        if (S8(SAVED_ENTRIES + k * 0x354u) == 0) continue;
        if (CALL0(f_088A06D8) == 0) continue;
        gaddr place = ENTRY_PLACES + k * 8u;
        CALL5(f_0880A6CC, *W(ENTRY_NAMES + k * 4u), 0, (uint32_t)(int16_t)*W(place),
              (uint32_t)(int16_t)*W(place + 4), 9);
    }
}

/* ---- who has chosen ---- */

/* The players expected to be done (0x088A1D30, a0 the game, a1 the battle), as GAME + 10 bits: World
   Tour, EDIT and Survival's early fights the players who fight or chose; Survival's fights 4 / 5
   both (3); mode 6 1 or 3; otherwise by the layout (0 → 3, 1 → 0xD, 2 → 0xE, 3 → 7). Another layout
   returns nothing: -1, and the caller keeps v0 as it was. */
static int32_t expected_done(Cpu *c, gaddr g, gaddr battle) {
    int8_t mode = S8(g + G_MODE);
    uint32_t players = (uint32_t)((int32_t)S8(g + G_FIGHTS) | (int32_t)S8(g + G_CHOSE)) & 0xFFFF;
    if (mode == MODE_WORLD_TOUR || mode == MODE_EDIT) return (int32_t)players;
    if (mode == MODE_SURVIVAL) {
        int8_t state = S8(battle + B_FIGHT_STATE);
        return state == 4 || state == 5 ? 3 : (int32_t)players;
    }
    if (mode == MODE_6) {
        if (CALL0(f_088D7CE0) != 0 &&
            (S8(GAME + G_FIGHTS) | S8(GAME + G_CHOSE) | S8(GAME + G_PRESENT)) == 1) return 1;
        return 3;
    }
    switch (S16(battle + B_LAYOUT)) {
    case 0: return 3;
    case 1: return 0xD;
    case 2: return 0xE;
    case 3: return 7;
    default: return -1;
    }
}

/* A network battle's player whose choice ran out of time (0x088A21C4): its bit expected, and its timer
   down to 0. (An unknown layout leaves v0 at 1, from 0x088D5E44.) */
static int network_time_out(Cpu *c, gaddr me) {
    if (CALL0(f_088D5E44) == 0) return 0;
    uint32_t bit = 1u << (S8(me + F_SLOT) & 31);
    uint32_t expected = CALL2(f_088A1D30, GAME, BATTLE);
    if ((bit & expected) == 0) return 0;
    int16_t t = (int16_t)(S16(me + F_TIMER) - 1);
    *H(me + F_TIMER) = (uint16_t)t;
    return t == 0;
}

/* ---- the cursors ---- */

/* A fighter's cursor (0x088A225C) while it runs: its state's handler from CURSOR_STEPS with (fighter,
   other, list, list); the original drops a3 (`first`) and passes the list twice. */
static void fighter_cursor(Cpu *c, gaddr me, gaddr other, gaddr list) {
    if (*W(me + F_CURSOR_ON_W) == 0) {
        c->r[4] = list; c->r[6] = me; c->r[7] = list; c->r[8] = 0;
        return;
    }
    gaddr fn = *W(CURSOR_STEPS + *B(me + F_STATE) * 4u);
    c->r[4] = me; c->r[5] = other; c->r[6] = list; c->r[7] = list; c->r[8] = fn;
    rt_call_indirect(c, fn);
}

/* Who chooses now in Training (0x088A1828): the other player once this one is done (GAME + 10), or
   when the other's or this one's parts (0x197) are done; otherwise this one, with GAME + 197 = 0x80. */
static void training_turn(gaddr a, gaddr b) {
    uint32_t bit = 1u << (S8(a + F_SLOT) & 31);
    if (*H(GAME + G_DONE) & bit) { *W(b + F_CURSOR_ON_W) = 1; *W(a + F_CURSOR_ON_W) = 0; return; }
    if ((S16(b + F_DONE) & 0x197) == 0x197) { *W(b + F_CURSOR_ON_W) = 0; *W(a + F_CURSOR_ON_W) = 1; return; }
    if ((S16(a + F_DONE) & 0x197) == 0x197) { *W(b + F_CURSOR_ON_W) = 1; *W(a + F_CURSOR_ON_W) = 0; return; }
    *B(GAME + 197) = 0x80;
    *W(b + F_CURSOR_ON_W) = 0;
    *W(a + F_CURSOR_ON_W) = 1;
}

/* Free Battle's turn among three fighters (0x088A5600, VARIABLE / DRAMATIC / R.DRAMATIC): exactly one
   of a, b, c gets its cursor. Written c, b, a, in that order, as the original does. */
static void cursor_on3(gaddr a, gaddr b, gaddr cc, int on_a, int on_b, int on_c) {
    *W(cc + F_CURSOR_ON_W) = (uint32_t)on_c;
    *W(b + F_CURSOR_ON_W) = (uint32_t)on_b;
    *W(a + F_CURSOR_ON_W) = (uint32_t)on_a;
}
static int parts_done(gaddr f) { return (S16(f + F_DONE) & 0x197) == 0x197; }

/* Free Battle's whose-turn (0x088A5600), by its kind (BATTLE + 159):
     ARCADE-MODE (1), TEAM (2): between a and b, as in Training (TEAM first asks 0x088A1750 for the two
       sides' lists, and drops the answer);
     VARIABLE (3), DRAMATIC (4): a, b or c (the opponent);
     R.DRAMATIC (5): the same, with the ties settled the other way. */
static void free_battle_turn(Cpu *c, gaddr a, gaddr b, gaddr cc) {
    int kind = S8(BATTLE + B_FREE_KIND);
    if ((uint32_t)(kind - 1) >= 5) return;
    uint16_t done = *H(GAME + G_DONE);
    switch (kind) {
    case 2:
        CALL2(f_088A1750, c->r[29] + 16, c->r[29] + 20);
        /* fall through */
    case 1:
        training_turn(a, b);
        return;
    case 3: case 4: {
        if (done & (1u << (S8(b + F_SLOT) & 31))) { cursor_on3(a, b, cc, 0, 0, 1); return; }
        if (done & (1u << (S8(a + F_SLOT) & 31))) { cursor_on3(a, b, cc, 0, 1, 0); return; }
        int16_t cf = S16(cc + F_DONE);
        if ((cf & 0x197) == 0x197) { cursor_on3(a, b, cc, 1, 0, 0); return; }
        int16_t bf = S16(b + F_DONE);
        if ((bf & 0x197) == 0x197) { cursor_on3(a, b, cc, 0, 0, 1); return; }
        int16_t af = S16(a + F_DONE);
        if ((af & 0x197) == 0x197) { cursor_on3(a, b, cc, 0, 1, 0); return; }
        if (bf & 1)                { cursor_on3(a, b, cc, 1, 0, 0); return; }
        if (af & 1)                { cursor_on3(a, b, cc, 0, 1, 0); return; }
        cursor_on3(a, b, cc, 1, 0, 0);
        return;
    }
    default: {                                   /* 5 */
        if (done & (1u << (S8(b + F_SLOT) & 31))) { cursor_on3(a, b, cc, 0, 0, 1); return; }
        if (done & (1u << (S8(a + F_SLOT) & 31))) { cursor_on3(a, b, cc, 0, 1, 0); return; }
        int16_t cf = S16(cc + F_DONE);
        if ((cf & 0x197) == 0x197) { cursor_on3(a, b, cc, 1, 0, 0); return; }
        int16_t bf = S16(b + F_DONE);
        if ((bf & 0x197) == 0x197) { cursor_on3(a, b, cc, 0, 0, 1); return; }
        if (cf & 1)                { cursor_on3(a, b, cc, 0, 1, 0); return; }
        if (bf & 1)                { cursor_on3(a, b, cc, 0, 0, 1); return; }
        if (parts_done(a))         { cursor_on3(a, b, cc, 0, 1, 0); return; }
        cursor_on3(a, b, cc, 1, 0, 0);
        return;
    }
    }
}

/* Whose cursors run this frame (0x088A1AB8), by mode. */
static void cursors_on(Cpu *c, gaddr a, gaddr b, gaddr third) {
    GuestFunc by_mode = NULL;
    switch (mode_now()) {
    case MODE_FREE:     by_mode = f_088A5600; break;
    case MODE_SURVIVAL: by_mode = f_088A1790; break;
    case MODE_TRAINING: CALL3(f_088A1828, a, b, third); return;
    case MODE_6:        *W(a + F_CURSOR_ON_W) = 1; *W(b + F_CURSOR_ON_W) = 1; return;
    case MODE_13: case MODE_19: by_mode = f_088A5914; break;
    default:            *W(a + F_CURSOR_ON_W) = 1; return;
    }
    c->r[4] = a; c->r[5] = b; c->r[6] = third; c->r[7] = third; c->r[8] = third;
    by_mode(c);
}

/* ---- once everyone has chosen ---- */

/* The X-ISM character for the ISM chosen (0x088A1F38). */
static void x_ism(gaddr f) {
    int32_t to = x_ism_character(S8(f + F_CHARACTER), S8(f + F_ISM));
    if (to >= 0) *B(f + F_CHARACTER) = (uint8_t)to;
}

/* Two fighters on the same character (Balrog's two numbers count as one) with the same colours
   (0x088A1ED8): the second takes the next colours (0-5). */
static void next_colours(gaddr a, gaddr b) {
    int8_t ca = S8(a + F_CHARACTER), cb = S8(b + F_CHARACTER);
    int same = ca == cb || (ca == 0x15 && cb == 0x25) || (ca == 0x25 && cb == 0x15);
    if (!same || S8(a + F_COLOUR) != S8(b + F_COLOUR)) return;
    *B(b + F_COLOUR) = (uint8_t)(S8(a + F_COLOUR) + 1);
    if (S8(b + F_COLOUR) >= 6) *B(b + F_COLOUR) = 0;
}

/* Survival's fights 4 and 5 (0x088A1D08): both players' + 352 cleared. */
static void survival_reset(gaddr battle, gaddr a, gaddr b) {
    int8_t state = S8(battle + B_FIGHT_STATE);
    if (state != 4 && state != 5) return;
    *W(b + F_PLAYED) = 0;
    *W(a + F_PLAYED) = 0;
}

/* ---- the frame ---- */

/* A frame of the screen (0x088A5B38): the text, each fighter's cursor and box by the layout, the
   names, and once the players expected are done (GAME + 10): the next step, the X-ISM characters, and
   colours kept apart. `first` / `second` are the players with player 2 first when it alone is present
   (GAME + 53 = 2), each with a list. */
static void screen_frame(Cpu *c, gaddr g, gaddr battle) {
    gaddr p1 = FIGHTER_1, p2 = FIGHTER_2;
    gaddr first = p1, second = p2, list_first = SIDE_LIST_0, list_second = SIDE_LIST_1;
    if (S8(g + G_PRESENT) == 2) { first = p2; second = p1; list_first = SIDE_LIST_1; list_second = SIDE_LIST_0; }
    CALL1(f_088A14B0, g);
    CALL1(f_088A1548, g);
    CALL1(f_088A160C, g);

    /* the original keeps the expected players in a stack slot that another layout leaves as it was */
    uint16_t expected = *H(c->r[29] + 28);
    int16_t layout = S16(BATTLE + B_LAYOUT);
    int8_t mode;
    switch (layout) {
    case 0:   /* two players */
        CALL3(f_088A1AB8, first, second, FIGHTER_3);
        expected = (uint16_t)CALL2(f_088A1D30, g, battle);
        CALL4(f_088A225C, second, first, list_second, 1);
        CALL4(f_088A225C, first, second, list_first, 0);
        CALL2(f_088A60F8, first, 0);
        mode = mode_now();
        if (mode == MODE_TRAINING) { CALL2(f_088A60F8, second, 1); break; }
        if (mode == MODE_6) {
            if (CALL0(f_088D7CE0) == 0 ||
                (S8(GAME + G_FIGHTS) | S8(GAME + G_CHOSE) | S8(GAME + G_PRESENT)) != 1)
                CALL2(f_088A60F8, second, 1);
            break;
        }
        if (mode == MODE_FREE && (S8(BATTLE + B_FREE_KIND) == 1 || S8(BATTLE + B_FREE_KIND) == 2))
            CALL2(f_088A60F8, second, 1);
        break;
    case 1:   /* player 1 against two */
    case 2: { /* player 2 against two */
        gaddr me = layout == 1 ? p1 : p2, mate = layout == 1 ? p2 : p1;
        CALL3(f_088A1AB8, me, FIGHTER_3, FIGHTER_4);
        expected = (uint16_t)CALL2(f_088A1D30, g, battle);
        CALL4(f_088A225C, me, mate, SIDE_LIST_0, 0);
        CALL4(f_088A225C, FIGHTER_3, FIGHTER_4, SIDE_LIST_1, 0);
        CALL4(f_088A225C, FIGHTER_4, FIGHTER_3, SIDE_LIST_1, 0);
        CALL2(f_088A60F8, me, 0);
        if (mode_now() != MODE_SURVIVAL || S8(BATTLE + B_FIGHT_STATE) != 6) CALL2(f_088A60F8, FIGHTER_3, 1);
        break;
    }
    case 3:   /* two against one */
        CALL3(f_088A1AB8, first, second, FIGHTER_3);
        expected = (uint16_t)CALL2(f_088A1D30, g, battle);
        CALL4(f_088A225C, second, first, list_first, 1);
        CALL4(f_088A225C, first, second, list_first, 0);
        CALL4(f_088A225C, FIGHTER_3, FIGHTER_4, list_second, 0);
        CALL2(f_088A60F8, first, 0);
        CALL2(f_088A60F8, FIGHTER_3, 1);
        break;
    default:
        break;
    }
    *H(c->r[29] + 28) = expected;
    CALL2(f_088A167C, g, battle);
    if (*H(g + G_DONE) != expected) return;

    /* everyone expected is done: the three fighters that fight, by the layout */
    gaddr a = first, b = second, d = p2;
    switch (S16(battle + B_LAYOUT)) {
    case 0: case 3: a = p1; b = p2; d = FIGHTER_3; break;
    case 1:         a = p1; b = FIGHTER_3; d = FIGHTER_4; break;
    case 2:         a = p2; b = FIGHTER_3; d = FIGHTER_4; break;
    default:        break;
    }
    *H(g + G_STEP) = 0;
    *H(g + G_SUB) = (uint16_t)(*H(g + G_SUB) + 1);
    CALL0(f_088D6378);
    CALL2(f_088E348C, g, 1);
    *B(g + 43) = 1;
    *B(g + 72) = 0xFF;
    *H(g + G_FRAME) = (uint16_t)(S16(g + G_FRAME) | 0x100);
    CALL1(f_088A1F38, a);
    CALL1(f_088A1F38, b);
    CALL1(f_088A1F38, d);
    mode = S8(g + G_MODE);
    int8_t kind = S8(battle + B_FREE_KIND);
    if (mode == MODE_13 || mode == MODE_19 || (mode == MODE_FREE && (kind == 4 || kind == 3 || kind == 5))) {
        CALL3(f_088A1BA4, a, b, d);
    } else {
        CALL4(f_088A1CE8, g, battle, a, b);
        CALL3(f_088A1D08, battle, a, b);
    }
}

/* ---- the start ---- */

#define BOX_TABLE      0x08A29A28u   /* 6 bytes per entry box: +4, +274, +280 (u16) */
#define BOX_INDEX      0x08A2B3DAu   /* s8 [6]: each entry's index into ENTRY_SLOT */
#define ENTRY_SLOT     0x08A2B378u   /* +42 + index: the character in the entry's box (-1 none) */
#define ENTRY_PICTURE  0x08A27020u   /* u16 by character */
#define ENTRY_EMPTY    0x08A27074u   /* u16 by entry */
#define BOX_NONE       0x4000
#define WAIT_CHOICE_B  0x258
#define BOX_ENTRY      0x27
#define PLACES         0x08A2B858u   /* u8 { column, row } by character */
#define FIGHTER_SETUP  0x08A2BAC2u   /* u8 by layout × 4 + slot: fighter + 114 */
#define SETUP_STYLE    0x089FF588u   /* the mode's set-up (a jump table in the original) */
#define TEAM_SLOTS     0x08A2B594u   /* s8 [4], modes 13 / 19 */
#define TEXT_CLEAR     0x08B02718u   /* 48 bytes cleared at the start */
#define CAMERA_A       0x092E88A0u
#define CAMERA_B       0x092E8940u
#define TASK_COUNT     0x095176F3u   /* s8 */
#define TASKS_TWO      0x08A2B996u   /* 12 × { kind, which player, + 10 } */
#define TASKS_ALL      0x08A2B978u   /* 10 × { kind (bit 7: modes 13 / 19 only), which, + 10 } */
#define TASK_KIND_SEL  0x47

void f_0882E264(Cpu *c); void f_08814260(Cpu *c); void f_0881CE4C(Cpu *c); void f_089E11C8(Cpu *c);
void f_088969BC(Cpu *c); void f_0880AEA8(Cpu *c); void f_0882E0D0(Cpu *c); void f_0887C6E4(Cpu *c);
void f_088E3604(Cpu *c); void f_08821F9C(Cpu *c); void f_088E336C(Cpu *c); void f_088A49F0(Cpu *c);
void f_088A4B30(Cpu *c); void f_0880AC10(Cpu *c); void f_088E36EC(Cpu *c); void f_088E36B8(Cpu *c);
void f_088E2304(Cpu *c); void f_0880AABC(Cpu *c); void f_088D63E8(Cpu *c); void f_088A2134(Cpu *c);
void f_0882E380(Cpu *c); void f_0882EACC(Cpu *c); void f_08823BEC(Cpu *c); void f_08822EAC(Cpu *c);
void f_08823338(Cpu *c); void f_088A12C8(Cpu *c); void f_088A4F1C(Cpu *c);
void f_088A078C(Cpu *c); void f_088A6C74(Cpu *c); void f_088A4874(Cpu *c); void f_088A283C(Cpu *c);   /* below */

static void box_none(gaddr box) {
    *H(box + 280) = BOX_NONE; *H(box + 4) = BOX_NONE; *H(box + 274) = BOX_NONE;
}

/* The entries' boxes (0x088A078C), offline: an entry in use shows its character, an empty one its
   empty picture; none where the mode hides the names. */
static void entry_boxes(Cpu *c) {
    *H(BOX_TABLE + 274) = 0x37;
    for (int32_t k = 0; k < 6; k++) {
        gaddr e = SAVED_ENTRIES + (uint32_t)k * 0x354u;
        int shown = CALL0(f_088A06D8) != 0;
        gaddr slot = ENTRY_SLOT + (uint32_t)(int32_t)S8(BOX_INDEX + (uint32_t)k) + 42;
        gaddr box = BOX_TABLE + (uint32_t)k * 6u;
        if (!shown) {
            box_none(box);
            *B(slot) = 0xFF;
        } else if (S8(e) != 0) {
            *B(slot) = *B(e + 1);
            uint16_t pic = *H(ENTRY_PICTURE + (uint32_t)(int32_t)S8(e + 1) * 2u);
            *H(box + 280) = BOX_ENTRY;
            *H(box + 4) = pic;
        } else {
            *B(slot) = 0xFF;
            *H(box + 4) = BOX_NONE;
            *H(box + 280) = *H(ENTRY_EMPTY + (uint32_t)k * 2u);
        }
    }
}

/* The start's first sub-step (0x088A08E4): the screen set up (sound, background, difficulty from
   BATTLE + 24, the entries' boxes), then either step 1 at once (GAME + 39) or the next sub-step. */
static void start_0(Cpu *c, gaddr g, gaddr battle) {
    CALL1(f_0882E264, 0);
    *B(g + 56) = 1;
    CALL0(f_08814260);
    CALL2(f_0881CE4C, (uint32_t)(int32_t)S16(battle + 102), 1);
    CALL3(f_089E11C8, 0x092DF980u, *W(0x092D9E20u), 0x200);
    *W(0x092E0980u) = 1;
    CALL3(f_088969BC, 0x6A1, 0x24, 8);
    *B(g + 56) = 1;
    *B(g + 134) = 0;
    CALL5(f_0880AEA8, 0, 0x08A2B5C4u, 0, 0x18, 1);
    *B(g + 195) = 0;
    *B(g + 193) = (uint8_t)S16(battle + 24);   /* the stage, from the last one */
    *B(g + 194) = 0;
    *B(g + 197) = 0;
    int8_t mode = S8(g + G_MODE);
    if (mode == MODE_13 || mode == MODE_19) *B(g + 196) = 0x80;
    CALL0(f_0882E0D0);
    for (uint32_t k = 0; k < 6; k++) {
        gaddr box = BOX_TABLE + k * 6u;
        *H(box + 4) = BOX_NONE;
        *B(ENTRY_SLOT + (uint32_t)(int32_t)S8(BOX_INDEX + k) + 42) = 0xFF;
        *H(box + 274) = BOX_NONE;
        *H(box + 280) = BOX_NONE;
    }
    if (CALL0(f_088D5E44) == 0) CALL1(f_088A078C, g);
    for (uint32_t k = 0; k < 48; k++) *B(TEXT_CLEAR + k) = 0;
    if (S8(g + 39) != 0) {
        *H(g + G_SUB) = (uint16_t)(*H(g + G_SUB) + 1);
        CALL2(f_088A12C8, g, battle);
        return;
    }
    *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1);
    *B(g + 72) = 0;
    *B(g + 43) = 0;
    *H(g + G_DONE) = 0; *H(g + 8) = 0; *H(g + 18) = 0; *H(g + 16) = 0;
    *H(g + G_SPEED) = 1;
    *H(g + G_FRAME) = 0;
    if (S8(g + G_MODE) != MODE_SURVIVAL) CALL1(f_0887C6E4, 0x100);
}

/* A fighter's box on the grid by its character, or the random box kept (0x088A283C). */
static void place_of(uint32_t ch, gaddr column, gaddr row) {
    gaddr p = PLACES + (ch & 0xFF) * 2u;
    *H(column) = *B(p);
    *H(row) = *B(p + 1);
}

/* The rows a mode skips (+ 112), also done (+ 424). */
static void skip_rows(gaddr f, uint16_t rows) {
    *H(f + 112) = rows;
    *H(f + F_DONE) = (uint16_t)(S16(f + F_DONE) | rows);
}

static void clear_styles(gaddr f) {
    *B(f + F_ISM_FIXED) = 0; *B(f + F_STYLE_1_ON) = 0; *B(f + F_STYLE_2_ON) = 0; *B(f + F_STYLE_4_ON) = 0;
}

/* A fighter set up for the screen (0x088A6C74, a1 its slot): cursor state 1, on its character's box;
   what its mode makes done already (+ 424, + 112: the rows it skips), the difficulty shown or not
   (GAME + 195); + 114 by the layout and slot. */
static void fighter_setup(Cpu *c, gaddr f, int8_t slot) {
    *B(f + F_STATE) = 1;
    *B(f + F_RANDOM_BOX) = 0;
    *W(f + 20) = 0;
    *W(f + 16) = 0;
    int8_t box = S8(f + F_BOX);
    *H(f + F_DONE) = 0;
    if (box != 0) CALL3(f_088A283C, (uint32_t)(box & 0x3F), f + F_COLUMN, f + F_ROW);
    else CALL3(f_088A283C, *B(f + F_CHARACTER), f + F_COLUMN, f + F_ROW);
    uint16_t done = (uint16_t)(S16(f + F_DONE) & 7);
    *B(f + F_SLOT) = (uint8_t)slot;
    *B(f + 2) = (uint8_t)slot;
    *W(f + 820) = (uint32_t)(int32_t)S8(f + F_SLOT);
    *W(f + F_CURSOR_ON_W) = 0;
    *B(f + F_RANDOM_HIGH) = 0;
    *H(f + F_DONE) = done;
    if (S8(GAME + G_SPEED_BASE) == 0) *H(f + F_DONE) = (uint16_t)(S16(f + F_DONE) | 8);
    if ((CALL1(f_088A4F1C, f) & 0xEF & 0xFF) == 1) *H(f + F_DONE) = (uint16_t)(S16(f + F_DONE) | 2);
    if (S8(GAME + 156) == 0) *H(f + F_DONE) = (uint16_t)(S16(f + F_DONE) | 2);
    *H(f + 112) = 0;

    int8_t mode = S8(GAME + G_MODE);
    switch (mode) {
    case MODE_FREE:
        *B(GAME + 195) = 0;
        if (S8(BATTLE + B_FREE_KIND) == 2) { skip_rows(f, 0x10); *B(f + F_STATE) = 0; }
        else skip_rows(f, 0x30);
        *W(f + 820) = S8(GAME + G_PRESENT) == 2;
        break;
    case MODE_TRAINING:
        *B(GAME + 195) = 0;
        skip_rows(f, 0x30);
        *W(f + 820) = S8(GAME + G_PRESENT) == 2;
        break;
    case MODE_SURVIVAL: {
        *B(GAME + 195) = 1;
        skip_rows(f, 0x70);
        int8_t state = S8(BATTLE + B_FIGHT_STATE);
        if (state == 4 || state == 5) *W(f + 820) = S8(GAME + G_PRESENT) == 2;
        break;
    }
    case MODE_6:
        *B(GAME + 195) = 1;
        skip_rows(f, 0x50);
        *B(f + F_STATE) = 0;
        clear_styles(f);
        break;
    case 1:
        *B(GAME + 195) = 1;
        clear_styles(f);
        break;
    case MODE_WORLD_TOUR: case MODE_DRAMATIC: case MODE_EDIT:
        *B(GAME + 195) = 1;
        *B(f + F_ISM_FIXED) = 0; *B(f + F_STYLE_1_ON) = 0; *B(f + F_STYLE_2_ON) = 0;
        skip_rows(f, mode == MODE_EDIT ? 0x7A : 0x72);
        break;
    case MODE_13: case MODE_19:
        *B(GAME + 195) = 1;
        clear_styles(f);
        *H(f + F_DONE) = (uint16_t)(S16(f + F_DONE) | 0x20);
        *H(f + 112) = 0x20;
        *H(f + F_DONE) = (uint16_t)(S16(f + F_DONE) & ~2);
        break;
    default:
        break;
    }
    *B(f + F_CHOSEN) = (uint8_t)*H(f + F_DONE);
    if (S8(f + F_CHARACTER) == 0x13) *B(f + F_CHARACTER) = 0x11;
    *H(f + 114) = *B(FIGHTER_SETUP + (uint32_t)(S16(BATTLE + B_LAYOUT) * 4 + S8(f + F_SLOT)));
}

/* The select's display tasks created (0x088A4874): in mode 6 and Free Battle's kind 2 twelve for the
   two players first; then (unless too many tasks are running) ten (or six) from TASKS_ALL, those
   marked only in modes 13 / 19; then 0x08823338 (10, 0). The player each shows is read back from the
   original's stack frame, where it put the two players. */
static void make_tasks(Cpu *c) {
    int8_t mode = S8(GAME + G_MODE);
    uint32_t n = 10;
    if (mode == MODE_6 || (mode == MODE_FREE && S8(BATTLE + B_FREE_KIND) == 2)) {
        gaddr t = TASKS_TWO;
        for (int k = 0; k < 12; k++, t += 3) {
            gaddr task = CALL2(f_08822EAC, TASK_KIND_SEL, *B(t));
            *W(task + 92) = *B(t + 1) == 0 ? FIGHTER_1 : FIGHTER_2;
            *B(task + 10) = *B(t + 2);
        }
        n = 6;
    }
    gaddr sp = c->r[29];
    *W(sp + 16) = FIGHTER_1;
    *W(sp + 20) = FIGHTER_2;
    if ((uint32_t)(int32_t)S8(TASK_COUNT) >= n) {
        gaddr t = TASKS_ALL;
        for (uint32_t k = 0; k < n; k++, t += 3) {
            uint8_t kind = *B(t);
            mode = S8(GAME + G_MODE);
            if (mode != MODE_13 && mode != MODE_19 && (kind & 0x80)) continue;
            gaddr task = CALL2(f_08822EAC, TASK_KIND_SEL, kind & 0x7F);
            *W(task + 92) = *W(sp + 16 + *B(t + 1) * 4u);
            *B(task + 10) = *B(t + 2);
        }
    }
    CALL2(f_08823338, 0xA, 0);
}

/* The start's second sub-step (0x088A0ADC), once the network allows: the cameras, the four fighters
   set up, and the display tasks (modes 13 / 19: their team slots instead; Free Battle's kinds 3-5:
   the opponents' boxes first). */
static void start_1(Cpu *c, gaddr g, gaddr battle) {
    if (CALL0(f_088E3604) == 0) return;
    *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1);
    CALL0(f_08821F9C);
    CALL2(f_088E336C, GAME, 1);
    *B(CAMERA_A + 123) = 6;
    *B(CAMERA_A + 122) = 2;
    *B(CAMERA_B + 122) = 1;
    *W(CAMERA_B + 16) = 0x400000; *W(CAMERA_A + 16) = 0x400000;
    *W(CAMERA_B + 20) = 0xFFF00000; *W(CAMERA_A + 20) = 0xFFF00000;
    CALL2(f_088A6C74, FIGHTER_1, 0);
    CALL2(f_088A6C74, FIGHTER_2, 1);
    CALL2(f_088A6C74, FIGHTER_3, 2);
    CALL2(f_088A6C74, FIGHTER_4, 3);
    *B(SIDE_LIST_0) = 0;
    *B(SIDE_LIST_1) = 0;
    int8_t mode = S8(g + G_MODE);
    if (mode == MODE_13 || mode == MODE_19) {
        *H(g + 18) = 0x258;
        *H(g + G_STEP) = 4;
        *H(g + G_DONE) = 0;
        CALL1(f_088A49F0, 0);
        for (uint32_t k = 0; k < 4; k++) {
            int8_t v = S8(TEAM_SLOTS + k);
            if (v != -1) *B(TEAM_SLOTS + k) = (uint8_t)(v & 0x7F);
        }
        if (S8(TEAM_SLOTS) == -1) {
            uint8_t v = *B(TEAM_SLOTS + 1); *B(TEAM_SLOTS + 1) = 0xFF; *B(TEAM_SLOTS) = v;
        } else if (S8(TEAM_SLOTS + 3) == -1) {
            uint8_t v = *B(TEAM_SLOTS + 2); *B(TEAM_SLOTS + 2) = 0xFF; *B(TEAM_SLOTS + 3) = v;
        }
        return;
    }
    int8_t kind = S8(battle + B_FREE_KIND);
    if (kind == 3 || kind == 4 || kind == 5) {
        *B(FIGHTER_3) = 1; *B(FIGHTER_3 + F_SLOT) = 2;
        *B(FIGHTER_4) = 1; *B(FIGHTER_4 + F_SLOT) = 3;
        CALL0(f_088A4B30);
    } else {
        CALL0(f_088A4874);
    }
}

/* The start's third sub-step (0x088A0CE4). */
static void start_2(Cpu *c, gaddr g) {
    if (CALL0(f_088E358C) != 0) {
        *H(g + 8) = 0;
        *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1);
        CALL0(f_0880AC10);
        CALL0(f_08837690);
        CALL0(f_08887AFC);
    }
    CALL0(f_0880B6CC);
    CALL0(f_08819144);
}

/* The start's fourth sub-step (0x088A0D44): waits (GAME + 93), then the frame step (GAME + 4 += 2). */
static void start_3(Cpu *c, gaddr g) {
    if (CALL0(f_088E358C) != 0) {
        uint16_t v = *H(g + 8);
        if (v == 0) {
            if (S8(g + 93) == 0) {
                CALL0(f_088E36B8);
                *H(g + 8) = (uint16_t)(*H(g + 8) + 1);
            }
        } else if (v == 1 && CALL0(f_088E36EC) != 0) {
            *H(g + 8) = 0;
            *H(g + G_SUB) = (uint16_t)(*H(g + G_SUB) + 2);
            *H(g + G_STEP) = 0;
            if (CALL0(f_088D5E44) != 0) CALL0(f_088E2304);
        }
        CALL0(f_08837690);
        CALL0(f_08887AFC);
    }
    CALL0(f_0880B6CC);
    CALL0(f_08819144);
}

/* The end step (0x088A1F98), once the network allows: a pause (World Tour first 0x0880AABC), then
   on to the fight's next screen with the screen's state cleared, the fighters set up for the fight
   by mode. */
static void end_step(Cpu *c, gaddr g, gaddr battle) {
    if (CALL0(f_088E3604) == 0) return;
    uint16_t s = *H(g + G_STEP);
    if (s == 0) {
        if (S8(GAME + G_MODE) == MODE_WORLD_TOUR) { *H(g + G_STEP) = 1; CALL0(f_0880AABC); }
        else *H(g + G_STEP) = 2;
        *H(g + 18) = 0x20;
        *B(g + G_FIGHTS) = *B(g + G_PRESENT);
        return;
    }
    if (s == 1) {
        if (S8(GAME + 93) == 0) *H(g + G_STEP) = 2;
        return;
    }
    int16_t t = (int16_t)(S16(g + 18) - 1);
    *H(g + 18) = (uint16_t)t;
    if (t != 0) return;
    *H(g + 2) = (uint16_t)(*H(g + 2) + 1);
    *H(g + 22) = 0; *H(g + 20) = 0; *H(g + 18) = 0; *H(g + 10) = 0;
    *H(g + 8) = 0; *H(g + 6) = 0; *H(g + 4) = 0;
    *B(g + 56) = 0;
    if (CALL0(f_088D5E44) != 0) CALL1(f_088D63E8, battle);
    int8_t mode = S8(GAME + G_MODE);
    if (mode == MODE_WORLD_TOUR) {
        CALL2(f_088A2134, g, battle);
    } else if (mode == MODE_EDIT) {
        CALL2(f_088A2134, g, battle);
        *H(g + G_SUB) = 0;
    } else if (S8(battle + B_FIGHT_STATE) == 0) {
        CALL2(f_0882E380, g, BATTLE);
    } else {
        CALL2(f_0882EACC, g, battle);
    }
    *B(g + G_CHOSE) = 0;
    CALL2(f_08823BEC, g, battle);
    CALL0(f_08821F9C);
}

/* ---- the cursors' states (by fighter + 4 from CURSOR_STEPS; their steps by + 5) ---- */

/* The list a cursor gets (SIDE_LIST_0 / 1): + 0 the last member, + 1 the member choosing now, and per
   member + 3 its character, + 6 its ISM, + 15 its box, + 18 its entry. */
#define L_LAST     0
#define L_NOW      1
#define L_CHAR     3
#define L_ISM      6
#define L_BOX      15
#define L_ENTRY    18
static gaddr member(gaddr list) { return list + (uint32_t)(int32_t)S8(list + L_NOW); }

#define F_HELD_NOW     362    /* u16: the pad's held buttons */
#define F_HELD_BEFORE  364    /* u16: last frame's */
#define F_PAD_SLOT     775    /* s8 */
#define F_PLAYER_PAD   820    /* u32 */
#define F_SKIPS        112    /* u16: the rows the mode skips */
#define STEPS_1        0x08A2B84Cu
#define STEPS_2        0x08A2B918u
#define STEPS_2_TEAM   0x08A2B924u   /* modes 13 / 19 */
#define STEPS_3        0x08A2B930u
#define BOX_ROWS       0x08A2B584u   /* s8 by layout × 4 + slot */
#define SOUND_MOVE     0x38
#define SOUND_CHOOSE   0x39

void f_088A5174(Cpu *c);   /* (fighter): which of the four fighters' pads drives it, -1 none (below; selectscreen.c wraps it) */
void f_0887C4CC(Cpu *c);   /* sound effect (0, id, volume) */
void f_088A6A10(Cpu *c);   /* (fighter, other, directions): the cursor moves on the grid (below) */
void f_088A2AF4(Cpu *c);   /* (fighter, other): a random box's character */
void f_088A2ED8(Cpu *c);   /* (fighter): plays as an entry */
void f_088A2CCC(Cpu *c);   /* (fighter): Final Bison where the layout allows */
void f_088A377C(Cpu *c);   /* (fighter, list): the team's next member */
void f_088A3AE0(Cpu *c);   /* (fighter, buttons): the ISM moved */
void f_088A21C4(Cpu *c); void f_088A2808(Cpu *c); void f_088A3414(Cpu *c); void f_088A37F8(Cpu *c);
void f_088A2870(Cpu *c); void f_088A29B4(Cpu *c); void f_088A315C(Cpu *c); void f_088A2FDC(Cpu *c);
void f_088A2D94(Cpu *c); void f_088A507C(Cpu *c); void f_088A5100(Cpu *c); void f_088A4E90(Cpu *c);
void f_088A4EB4(Cpu *c); void f_088A34A8(Cpu *c); void f_088A35E4(Cpu *c); void f_088A373C(Cpu *c);
void f_088A4E30(Cpu *c); void f_088A382C(Cpu *c); void f_088A3914(Cpu *c); void f_088A3B70(Cpu *c);
void f_088A39CC(Cpu *c); void f_088A3AA0(Cpu *c); void f_088A4C68(Cpu *c);   /* below */

/* A state's step by + 5 from its table (fighter, other, list handed on; fn in a3, the table in t0). */
static void state_step(Cpu *c, gaddr table) {
    gaddr fn = *W(table + *B(c->r[4] + F_STEP) * 4u);
    c->r[7] = fn; c->r[8] = table;
    rt_call_indirect(c, fn);
}

/* State 2's steps (0x088A3414): modes 13 / 19 have a table of their own. */
static void state_2_step(Cpu *c) {
    gaddr me = c->r[4], list = c->r[6];
    int8_t mode = S8(GAME + G_MODE);
    gaddr fn = *W((mode == MODE_13 || mode == MODE_19 ? STEPS_2_TEAM : STEPS_2) + *B(me + F_STEP) * 4u);
    c->r[6] = list; c->r[7] = fn; c->r[8] = list;
    c->r[9] = mode == MODE_13 ? list : MODE_19; c->r[10] = MODE_13;
    rt_call_indirect(c, fn);
}

/* The buttons of the pad driving a fighter (0x088A507C newly pressed, 0x088A5100 held): 0x088A5174
   picks one of the four fighters, whose + 362 / + 364 are its pad's held buttons now and before. The
   original keeps the four in its stack frame and reads the one picked from there. */
static uint32_t driving_pad(Cpu *c, gaddr me, int pressed) {
    gaddr sp = c->r[29];
    *W(sp + 16) = FIGHTER_1; *W(sp + 20) = FIGHTER_2; *W(sp + 24) = FIGHTER_3; *W(sp + 28) = FIGHTER_4;
    c->r[4] = me; c->r[5] = FIGHTER_4;
    f_088A5174(c);
    int32_t i = (int32_t)c->r[2];
    if (i == -1) return 0;
    gaddr f = *W(sp + 16 + (uint32_t)i * 4u);
    if (!pressed) return *H(f + F_HELD_NOW);
    return (uint32_t)(*H(f + F_HELD_NOW) & (uint16_t)~*H(f + F_HELD_BEFORE));
}

/* Which fighter's pad drives a box (0x088A5174): its + 820, when that is 0 - 3; otherwise -1. (The
   original counts up to it; a5 is left one past the count, which no caller reads.) */
static int32_t box_driver(gaddr me) {
    int32_t i = (int32_t)*W(me + F_DRIVER);
    return i >= 0 && i < 4 ? i : -1;
}

/* ---- the grid (0x088A6A10) ---- */
#define BGRID_LOCK     0x08A2B3A9u   /* s8 by row * 7 + column: -1 open to anyone; otherwise only to a player present */
#define BGRID_BOXES    0x08A2B378u   /* s8 by row * 7 + column: the character, or 0x40 + a random box's kind; < 0 no box */
static int16_t bgrid_next(int16_t v) { v = (int16_t)(v + 1); return v < 7 ? v : 0; }
static int16_t bgrid_prev(int16_t v) { return v == 0 ? 6 : (int16_t)(v - 1); }

/* The cursor moves on the grid (0x088A6A10; directions: 1 right, 2 left, 4 down, 8 up), along a row
   if it moves across at all, otherwise along its column, skipping empty boxes (and boxes a player
   present doesn't have, if this one isn't). On a random box the character is drawn (0x088A2AF4).
   Unless the character is done, the ISM goes back to the one last chosen (0 on a random box). */
static void bselect_grid(Cpu *c, gaddr me, gaddr other, uint32_t dirs) {
    dirs &= 0xFFFF;
    CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
    int16_t col = S16(me + F_COLUMN), row = S16(me + F_ROW);
    *B(me + F_BOX) = 0;
    int present = (1 << (S8(me + F_SLOT) & 31)) & S8(GAME + G_PRESENT);
    int32_t box;
    for (;;) {
        if (dirs & 3) {
            if (dirs & 1) col = bgrid_next(col);
            if (dirs & 2) col = bgrid_prev(col);
        } else {
            if (dirs & 4) row = bgrid_next(row);
            if (dirs & 8) row = bgrid_prev(row);
        }
        uint32_t at = (uint32_t)(row * 7 + col);
        box = S8(BGRID_BOXES + at);
        if (box < 0) continue;
        if (S8(BGRID_LOCK + at) != -1 && !present) continue;
        break;
    }
    int32_t character;
    if (box & 0x40) {
        *B(me + F_RANDOM_BOX) = (uint8_t)((box & 0xF) + 0x2A);
        character = (int32_t)CALL2(f_088A2AF4, me, other);
    } else {
        *B(me + F_RANDOM_HIGH) = 0;
        *B(me + F_RANDOM_BOX) = 0;
        character = box;
    }
    int done = S16(me + F_DONE) & 4;
    *B(me + F_CHARACTER) = (uint8_t)character;
    *H(me + F_COLUMN) = (uint16_t)col;
    *H(me + F_ROW) = (uint16_t)row;
    if (done) return;
    *B(me + F_ISM) = S8(me + F_RANDOM_BOX) != 0 ? 0 : *B(me + F_LAST_ISM);
}

static void sound_choose(Cpu *c) { CALL3(f_0887C4CC, 0, SOUND_CHOOSE, 0xC0); }

/* Whether a fighter's box is on the right (0x088A4EB4): by the layout and slot, flipped for the
   two-against-one layout when player 2 started alone. */
static uint32_t box_on_right(gaddr me) {
    int16_t layout = S16(BATTLE + B_LAYOUT);
    int8_t v = S8(BOX_ROWS + (uint32_t)(layout * 4 + S8(me + F_SLOT)));
    if (S8(GAME + G_PRESENT) == 2 && layout == 3) return ((uint32_t)(int32_t)v ^ 1) != 0;
    return v != 0;
}

/* ---- a fighter's box: its heading, what it has chosen, its record (0x088A60F8) ---- */
#define BOX_PLACES      0x08A2B4C4u   /* s16 x, y: by side (36 bytes each): heading, then rows; + 136 / + 144 Survival's headings */
#define BOX_HEADINGS    0x08A2BA48u   /* char *: by side (+ 0); the ISMs' names by ISM + 1 (+ 8); modes 13 / 19 by side (+ 20); Free Battle by box (+ 40) */
#define BOX_COLOURS     0x08A2BA78u   /* u8: the ISM's colour by ISM + 1; + 2 the heading's, by side or box */
#define BOX_NUMBERS     0x08A2BA7Cu   /* char *: two-digit number buffers, five per side */
#define BOX_RANKS       0x08A2B5B0u   /* char *: + 4 by rank (GAME + 126 + side) */
#define BOX_DIFF_NAMES  0x08A2B820u   /* char *: by difficulty (GAME + 190) */
#define BOX_STYLE_NAMES 0x08A2BA34u   /* char *: by style (+ 438) */
#define S_ISM           0x089FF564u   /* "ISM" */
#define S_WINS          0x089FF568u   /* "WIN" */
#define S_RECORD_1      0x089FF56Cu   /* " V:", " S:", "SS:", " P:" (4 bytes apart), then "VITAL" */
#define S_RECORD_5      0x089FF57Cu
#define S_DIFFICULTY    0x089FF4B8u   /* "DIFF" */
#define S_STYLE         0x089FF4DCu   /* "MODE" */
#define F_RECORD        433    /* s8 [4]: the record's numbers; + 437 the wins */
#define F_WINS          437    /* s8 */
#define G_RANK          126    /* u8 by side */
#define G_DIFFICULTY    190    /* s8 */
void f_088A22AC(Cpu *c);   /* (fighter): modes 13 / 19: the team's members */
void f_088D5E7C(Cpu *c);   /* nonzero: mode 6 shows no win count */
void f_088A2480(Cpu *c); void f_088A2578(Cpu *c); void f_088A4D30(Cpu *c);   /* below */

static uint32_t box_colour(void) { return *H(TEXT_COLOUR) & 0xFF; }
static void box_text(Cpu *c, gaddr s, int32_t x, int32_t y, uint32_t colour) {
    CALL5(f_0880A6CC, s, 3, (uint32_t)(int32_t)(int16_t)x, (uint32_t)(int32_t)(int16_t)y, colour);
}
/* a two-digit number into one of the side's buffers, drawn in colour 9 */
static void box_number(Cpu *c, uint32_t buffer, int8_t v, int32_t x, int32_t y) {
    gaddr s = *W(BOX_NUMBERS + buffer * 4u);
    *B(s) = (uint8_t)(v / 10 + '0');
    *B(s + 1) = (uint8_t)(v % 10 + '0');
    box_text(c, s, x, y, 9);
}

/* The difficulty line (0x088A2578): Free Battle, the second box */
static void box_difficulty(Cpu *c, gaddr me) {
    uint32_t side = CALL1(f_088A4EB4, me) & 0xFF;
    gaddr at = BOX_PLACES + side * 4u + 152;
    int16_t x = S16(at), y = S16(at + 2);
    box_text(c, S_DIFFICULTY, x, y, box_colour());
    box_text(c, *W(BOX_DIFF_NAMES + (uint32_t)((int32_t)S8(GAME + G_DIFFICULTY) * 4)), x + 0x40, y, box_colour());
}

/* The style line (0x088A4D30): for a player present, outside the two-against-one layout, when the
   mode offers more than one style */
static void box_style(Cpu *c, gaddr me, uint32_t colour) {
    colour &= 0xFF;
    if (S16(BATTLE + B_LAYOUT) == 3) return;
    if (!(S8(GAME + G_PRESENT) & (1 << (S8(me + F_SLOT) & 31)))) return;
    if (CALL1(f_088A4F1C, me) == 1) return;
    uint32_t style = (uint8_t)S8(me + F_STYLE);
    gaddr at = BOX_PLACES + (uint32_t)((S8(me + F_SLOT) * 2 + 18) * 4);
    box_text(c, S_STYLE, S16(at), S16(at + 2), colour);
    box_text(c, *W(BOX_STYLE_NAMES + style * 4u), S16(at + 4), S16(at + 6), colour);
}

/* Mode 6's and TEAM's lines (0x088A2480): two headings, then the team's size (+ 441: 0 or 1), in
   their places for the box's side */
#define TEAM_TEXTS     0x08A2B810u   /* char *: two headings, then by team size */
#define F_TEAM_SIZE    441    /* s8 */
static void box_team(Cpu *c, gaddr me) {
    uint32_t side = CALL1(f_088A4EB4, me) & 0xFF;
    gaddr at = BOX_PLACES + (side ? 0xB0u : 0xA0u);
    box_text(c, *W(TEAM_TEXTS), S16(at), S16(at + 2), box_colour());
    box_text(c, *W(TEAM_TEXTS + 4), S16(at + 4), S16(at + 6), box_colour());
    uint32_t colour = box_colour();
    int8_t size = S8(me + F_TEAM_SIZE);
    gaddr p = size ? at + 12 : at + 8;
    box_text(c, *W(TEAM_TEXTS + (uint32_t)(size * 4) + 8), S16(p), S16(p + 2), colour);
}

/* A fighter's box (0x088A60F8; which: 0 the first box, 1 the second): its heading, the ISM, the play
   style, the mode's extras (mode 6 and TEAM's members, the difficulty in Free Battle), mode 6's win
   count and mode 1's record */
static void fighter_box(Cpu *c, gaddr me, uint32_t which) {
    uint32_t side = CALL1(f_088A4EB4, me) & 0xFF;
    gaddr at = BOX_PLACES + side * 36u;
    int16_t x0 = S16(at), y0 = S16(at + 2);
    gaddr row = at + 4;                          /* where the next line goes */
    int8_t mode = mode_now(), kind = S8(BATTLE + B_FREE_KIND);

    /* the heading */
    if (mode == MODE_FREE && (kind == 3 || kind == 4 || kind == 5)) {   /* VARIABLE, DRAMATIC, R.DRAMATIC: by box */
        if (S8(me + F_SLOT) == 3) return;
        box_text(c, *W(BOX_HEADINGS + which * 4u + 40), side == 0 ? x0 - 16 : x0 - 40, y0 - 8, *B(BOX_COLOURS + which + 2));
        mode = mode_now();
    }
    uint32_t heading_colour = *B(BOX_COLOURS + side + 2);
    if (mode == MODE_13 || mode == MODE_19) {
        if (S8(me + F_SLOT) == 3) return;
        box_text(c, *W(BOX_HEADINGS + side * 4u + 20), side == 0 ? x0 - 16 : x0 - 40, y0 - 8, heading_colour);
        CALL1(f_088A22AC, me);
    } else {
        gaddr s = *W(BOX_HEADINGS + side * 4u);
        int8_t state = S8(BATTLE + B_FIGHT_STATE);
        if (mode == MODE_6 || (mode == MODE_FREE && S8(BATTLE + B_FREE_KIND) == 2)) box_text(c, s, x0, y0 - 20, heading_colour);
        else if (mode == MODE_SURVIVAL && (state == 4 || state == 5)) {
            gaddr p = BOX_PLACES + side * 4u + (S8(GAME + G_PRESENT) == 1 ? 136 : 144);
            box_text(c, s, S16(p), S16(p + 2), heading_colour);
        } else box_text(c, s, x0, y0, heading_colour);
    }
    mode = mode_now();
    uint32_t colour = *H(TEXT_COLOUR);

    /* the ISM (in Survival, only for a side with a player); while on a random box, in colour 9 */
    if (mode != MODE_SURVIVAL || (S8(GAME + G_PRESENT) & (1 << (side & 31)))) {
        box_text(c, S_ISM, S16(row), S16(row + 2), colour & 0xFF);
        uint32_t ism = (uint32_t)(S8(me + F_ISM) + 1) & 0xFF;
        uint32_t col = *B(BOX_COLOURS + ism);
        if (S8(me + F_RANDOM_BOX) != 0 && S8(me + F_RANDOM_FLAG) == 0 && S8(me + F_RANDOM_HIGH) == 0 && S8(me + F_BOX) == 0) col = 9;
        box_text(c, *W(BOX_HEADINGS + ism * 4u + 8), S16(row + 4), S16(row + 6), col);
        row += 8;
        colour = *H(TEXT_COLOUR);
    }
    CALL2(f_088A4D30, me, colour);

    mode = mode_now();
    if (mode == MODE_6 || (mode == MODE_FREE && S8(BATTLE + B_FREE_KIND) == 2)) { CALL2(f_088A2480, me, which); mode = mode_now(); }
    if (mode == MODE_FREE && which != 0) { CALL2(f_088A2578, me, which); mode = mode_now(); }

    if (mode == MODE_6) {
        if (CALL0(f_088D5E7C) == 0) {            /* the win count */
            int16_t x = S16(row), y = (int16_t)(S16(row + 2) + 0x38);
            box_number(c, (side * 5) & 0xFF, S8(me + F_WINS), x, y);
            box_text(c, S_WINS, x + 0x18, y, box_colour());
            return;
        }
    }
    if (which != 0 || mode_now() != 1) return;

    /* mode 1: the wins, the record's four rows, the rank */
    uint32_t buffer = (side * 5) & 0xFF;
    int16_t x = S16(row), y = S16(row + 2);
    box_number(c, buffer, S8(me + F_WINS), x, y);
    box_text(c, S_WINS, x + 0x18, y, box_colour());
    row += 4;
    for (int k = 0; k < 4; k++, row += 4) {
        x = S16(row); y = S16(row + 2);
        box_text(c, S_RECORD_1 + 4u * (uint32_t)k, x, y, box_colour());
        buffer = (buffer + 1) & 0xFF;
        box_number(c, buffer, S8(me + F_RECORD + k), x + 0x24, y);
    }
    x = S16(row); y = S16(row + 2);
    box_text(c, S_RECORD_5, x, y, box_colour());
    uint32_t rank = *B(GAME + G_RANK + side);
    box_text(c, *W(BOX_RANKS + rank * 4u + 4), x + 2, y + 8, box_colour());
}

/* A row's panel (0x08823338 (10, kind)) for the fighter, with its parts done; and the time to choose. */
static void row_panel(Cpu *c, gaddr me, uint32_t kind) {
    gaddr t = CALL2(f_08823338, 0xA, kind & 0xFF);
    if (t != 0) {
        *W(t + 124) = me;
        *B(t + 140) = (uint8_t)*H(me + F_DONE);
    }
    *H(me + F_TIMER) = WAIT_CHOICE_B;
}

/* A row's wait (0x088A373C, the style): then the next state, done. */
static void row_wait(gaddr me, uint16_t bit) {
    int16_t t = (int16_t)(S16(me + F_TIMER) - 1);
    *H(me + F_TIMER) = (uint16_t)t;
    if (t != 0) return;
    *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
    *B(me + F_STEP) = 0;
    *H(me + F_DONE) = (uint16_t)(S16(me + F_DONE) | bit);
}

/* ---- state 1: the character ---- */

static int team_kind(void) {   /* mode 6, or Free Battle's kind 2: a team chosen member by member */
    int8_t mode = S8(GAME + G_MODE);
    return mode == MODE_6 || (mode == MODE_FREE && S8(BATTLE + B_FREE_KIND) == 2);
}

/* Enter (0x088A2870): the cursor on the character's box (a team member's from the list), or the
   random box kept; then the ISM-style state at once if the character is done, otherwise the move step. */
static void character_enter(Cpu *c, gaddr me, gaddr other, gaddr list) {
    *B(me + F_ENTRY_BOX) = 0; *B(me + F_RANDOM_HIGH) = 0; *B(me + F_RANDOM_FLAG) = 0;
    *B(me + 73) = 0; *B(me + F_RANDOM_BOX) = 0;
    if (team_kind()) {
        *B(me + F_CHARACTER) = *B(member(list) + L_CHAR);
        *B(me + F_ISM) = *B(member(list) + L_ISM);
        uint8_t box = *B(member(list) + L_BOX);
        *B(me + F_RANDOM_BOX) = 0;
        *B(me + F_BOX) = box;
    }
    uint32_t at = (uint8_t)S8(me + F_CHARACTER);
    int8_t box = S8(me + F_BOX);
    if (box != 0) {
        *B(me + F_RANDOM_HIGH) = (uint8_t)(box & 0x80);
        *B(me + F_RANDOM_BOX) = (uint8_t)(box & 0x3F);
        *B(me + F_ISM) = *B(member(list) + L_ISM);
        at = (uint32_t)(box & 0x3F);
    }
    CALL3(f_088A283C, at, me + F_COLUMN, me + F_ROW);
    if (S16(me + F_DONE) & DONE_CHARACTER) {
        *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
        CALL3(f_088A3414, me, other, list);
        return;
    }
    *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
    *H(me + F_TIMER) = 0x4B0;
    *B(me + F_BOX) = *B(me + F_RANDOM_BOX);
}

/* Choosing as himself (0x088A2D94): a player's entry cleared (World Tour: marked in use), SELECT's
   extras (Shin Akuma, the first Balrog; Final Bison by 0x088A2CCC). */
static void as_himself(Cpu *c, gaddr me) {
    int8_t slot = S8(me + F_SLOT);
    gaddr mine = 0x09358528u + (uint32_t)(int32_t)slot * 0x354u;
    if (!(slot & 2)) for (uint32_t i = 0; i < 0x354; i++) *B(mine + i) = 0;
    if (S8(GAME + G_MODE) == MODE_WORLD_TOUR)
        *B(0x09358528u + (uint32_t)(int32_t)S8(me + F_SLOT) * 0x354u) = 1;
    uint32_t ch = (uint16_t)(int32_t)S8(me + F_CHARACTER);
    uint32_t held = CALL1(f_088A5100, me);
    *B(me + F_FINAL_BISON) = 0;
    *B(me + 430) = 0;
    int extras = 1;
    if (CALL0(f_088D5E44) == 0) extras = S16(SYS + SYS_UNLOCKS) & 1;
    if (!(held & 0x4000) || !extras) return;
    if (ch == 2)    { ch = 0x24; *B(me + F_CHARACTER) = 0x24; }
    if (ch == 0x25) { ch = 0x15; *B(me + F_CHARACTER) = 0x15; }
    if (ch == 0xA)  CALL1(f_088A2CCC, me);
}

/* Chosen (0x088A2FDC, a2 the button): as an entry or as himself; a new character clears + 400 / 404
   / 411; the colours by the button (an entry's own otherwise); a sound. */
static void character_chosen(Cpu *c, gaddr me, uint32_t buttons) {
    buttons &= 0xFFFF;
    *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
    *B(me + F_CHOSEN) = (uint8_t)(S8(me + F_CHOSEN) | 1);
    *B(me + F_FINAL_BISON) = 0;
    if (S8(me + F_ENTRY_BOX) == 0) CALL1(f_088A2D94, me);
    else CALL1(f_088A2ED8, me);
    uint32_t ch = (uint16_t)(int32_t)S8(me + F_CHARACTER);
    uint32_t last = (uint16_t)(int32_t)S8(me + F_LAST_CHARACTER);
    int same = (ch == 4 && last == 0x12) || (ch == 6 && last == 0x14) || ch == last;
    if (!same) {
        *B(me + 411) = 0;
        *H(me + F_KEPT_400 + 4) = 0;
        *W(me + F_KEPT_400) = 0;
        *B(me + F_LAST_CHARACTER) = (uint8_t)ch;
    }
    uint8_t colour;
    if (S8(me + F_ENTRY_BOX) != 0) {
        colour = *B(me + F_COLOUR);
    } else {
        colour = 0;
        if (buttons & 0x20)  colour = 2;
        if (buttons & 0x40)  colour = 4;
        if (buttons & 0x80)  colour = 1;
        if (buttons & 0x100) colour = 3;
        if (buttons & 0x200) colour = 5;
    }
    *B(me + F_COLOUR) = colour;
    *B(me + F_COLOUR_COPY) = colour;
    *H(me + F_TIMER) = 8;
    CALL0(f_088A4E90);
}

/* Move (0x088A29B4): which entry's box the cursor is on (they sit on row 6); then an attack button
   chooses, a direction moves (0x088A6A10), a random box draws; a network timeout chooses with 0x20.
   The member's character and entry go into the list. */
static void character_move(Cpu *c, gaddr me, gaddr other, gaddr list) {
    if (CALL1(f_088A21C4, me) != 0) {
        CALL4(f_088A2FDC, me, other, 0x20, list);
    } else {
        uint32_t row = (uint16_t)S16(me + F_ROW), col = (uint16_t)S16(me + F_COLUMN);
        uint32_t k = 0;
        for (; k < 6; k++)
            if (row == 6 && (uint32_t)(int32_t)S8(BOX_INDEX + k) == col) break;
        *B(me + F_ENTRY_BOX) = (uint8_t)(k == 6 ? 0 : k + 1);
        uint32_t pressed = CALL1(f_088A507C, me);
        if (pressed & 0x3F0) {
            CALL4(f_088A2FDC, me, other, pressed & 0x3F0, list);
        } else if (pressed & 0xF) {
            CALL3(f_088A6A10, me, other, pressed & 0xF);
        } else if (S8(me + F_RANDOM_BOX) != 0) {
            *B(me + F_CHARACTER) = (uint8_t)CALL2(f_088A2AF4, me, other);
        }
    }
    gaddr m = member(list);
    *B(m + L_CHAR) = *B(me + F_CHARACTER);
    *B(m + L_ENTRY) = *B(me + F_ENTRY_BOX);
}

/* Wait (0x088A315C): the character shown a while; then done, the box to come back to kept (an entry's
   + 0x2B, or the random box) in the fighter and the list; Final Bison's styles cleared. */
static void character_wait(gaddr me, gaddr list) {
    int16_t t = (int16_t)(S16(me + F_TIMER) - 1);
    *H(me + F_TIMER) = (uint16_t)t;
    if (t != 0) return;
    *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
    *B(me + F_STEP) = 0;
    *B(me + 6) = 0;
    *H(me + F_DONE) = (uint16_t)(S16(me + F_DONE) | DONE_CHARACTER);
    if (S8(me + F_FINAL_BISON) != 0) {
        *B(me + F_STYLE_1_ON) = 0; *B(me + F_STYLE_2_ON) = 0; *B(me + F_ISM_FIXED) = 0; *B(me + F_ISM) = 0;
    }
    int8_t entry = S8(me + F_ENTRY_BOX), v;
    if (entry != 0) v = (int8_t)(entry + 0x2B);
    else {
        int8_t r = S8(me + F_RANDOM_BOX);
        v = r != 0 ? (int8_t)(r | S8(me + F_RANDOM_HIGH)) : 0;
    }
    *B(me + F_BOX) = (uint8_t)v;
    *B(member(list) + L_BOX) = (uint8_t)v;
}

/* ---- state 2: the play style ---- */

/* The style's flags (0x088A4E30), as the character select's. */
static void style_flags(gaddr me) {
    *B(me + F_STYLE_4_ON) = 0; *B(me + F_STYLE_1_ON) = 0; *B(me + F_STYLE_2_ON) = 0;
    int8_t s = S8(me + F_STYLE);
    *B(me + F_ISM_FIXED) = 0;
    if (s == 4) *B(me + F_STYLE_4_ON) = 0xFF;
    if (s == 1) *B(me + F_STYLE_1_ON) = 1;
    if (s == 2) *B(me + F_STYLE_2_ON) = 1;
    if (s == 3) { *B(me + F_ISM_FIXED) = 1; *B(me + F_ISM) = 0xFF; }
}

/* Enter (0x088A34A8): skipped (straight to the wait) when another pad drives the fighter, for a team's
   later members, when one style is offered, for Final Bison, or when done; otherwise the panel. */
static void style_enter(Cpu *c, gaddr me, gaddr other, gaddr list) {
    int skip;
    if ((uint32_t)(int32_t)S8(me + F_PAD_SLOT) != *W(me + F_PLAYER_PAD) &&
        !(S8(GAME + G_PRESENT) & (1 << (S8(me + F_SLOT) & 31))))
        skip = 1;
    else if (team_kind() && S8(list + L_NOW) != 0) skip = 1;
    else if (CALL1(f_088A4F1C, me) == 1) skip = 1;
    else if (S8(me + F_FINAL_BISON) != 0) skip = 1;
    else skip = (S16(me + F_DONE) & 2) != 0;
    if (skip) {
        *H(me + F_TIMER) = 1;
        CALL3(f_088A373C, me, other, list);
        return;
    }
    *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
    uint32_t right = (uint16_t)(int32_t)(int8_t)CALL1(f_088A4EB4, me);
    row_panel(c, me, right + 6);
}

/* Choose (0x088A35E4): an attack button (or, network, time) chooses; up / down go round the styles
   offered (up to 4; 3 in a network battle). */
static void style_choose(Cpu *c, gaddr me) {
    int chosen = CALL1(f_088A21C4, me) != 0;
    int32_t pressed = 0;
    if (!chosen) {
        pressed = (int16_t)CALL1(f_088A507C, me);
        chosen = (pressed & 0x3F0) != 0;
    }
    if (chosen) {
        *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
        *H(me + F_TIMER) = 8;
        *B(me + F_CHOSEN) = (uint8_t)(S8(me + F_CHOSEN) | 4);
        CALL0(f_088A4E90);
        CALL1(f_088A4E30, me);
        return;
    }
    if (!(pressed & 0xC)) return;
    CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
    int16_t s = S8(me + F_STYLE);
    int32_t offered = (int16_t)(CALL1(f_088A4F1C, me) & 0xFF);
    int16_t last = CALL0(f_088D5E44) != 0 ? 3 : 4;
    if (pressed & 4) {
        do { int16_t was = s; s = (int16_t)(s + 1); if (was == last) s = 0; } while (!(offered & (1 << (s & 31))));
    } else if (pressed & 8) {
        do { int16_t was = s; s = (int16_t)(s - 1); if (was == 0) s = last; } while (!(offered & (1 << (s & 31))));
    }
    *B(me + F_STYLE) = (uint8_t)s;
}

/* ---- state 3: the ISM ---- */

/* An ISM chosen (0x088A3AA0). */
static void ism_chosen(Cpu *c, gaddr me) {
    *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
    *B(me + F_LAST_ISM) = *B(me + F_ISM);
    *B(me + F_CHOSEN) = (uint8_t)(S8(me + F_CHOSEN) | 4);
    *H(me + F_TIMER) = 8;
    CALL0(f_088A4E90);
}

/* Enter (0x088A382C): an ISM already decided (CLASSICAL, an entry, Final Bison, a random box) goes into
   the list and straight to the wait, as does one done; otherwise the panel. */
static void ism_enter(Cpu *c, gaddr me, gaddr other, gaddr list) {
    int decided = S8(me + F_ISM_FIXED) != 0 || S8(me + F_ENTRY_BOX) != 0 || S8(me + F_FINAL_BISON) != 0 ||
                  (S8(me + F_RANDOM_BOX) != 0 && S8(me + F_RANDOM_FLAG) == 0 && S8(me + F_RANDOM_HIGH) == 0);
    if (decided) {
        uint8_t ism = *B(me + F_ISM);
        *B(me + F_LAST_ISM) = ism;
        *B(member(list) + L_ISM) = ism;
    } else if (!(S16(me + F_DONE) & 4)) {
        *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
        row_panel(c, me, CALL1(f_088A4EB4, me) + 1);
        return;
    }
    *H(me + F_TIMER) = 1;
    CALL3(f_088A3B70, me, other, list);
}

/* Choose (0x088A3914): an attack button (or, network, time) chooses; up / down move (0x088A3AE0). The
   ISM goes into the list (not after a timeout). */
static void ism_choose(Cpu *c, gaddr me, gaddr other, gaddr list) {
    if (CALL1(f_088A21C4, me) != 0) { CALL4(f_088A3AA0, me, other, 0, list); return; }
    int32_t pressed = (int16_t)CALL1(f_088A507C, me);
    if (pressed & 0x3F0) CALL4(f_088A3AA0, me, other, (uint32_t)pressed & 0xFFFF, list);
    else if (pressed & 0xC) CALL2(f_088A3AE0, me, (uint32_t)pressed & 0xFFFF);
    *B(member(list) + L_ISM) = *B(me + F_ISM);
}

/* The X-ISM character for the ISM, and the colours flipped against the other fighter once it has its
   character or style (0x088A4C68). */
static void ism_character(gaddr me, gaddr other) {
    int32_t to = x_ism_character(S8(me + F_CHARACTER), S8(me + F_ISM));
    if (to >= 0) { *B(me + F_LAST_CHARACTER) = (uint8_t)to; *B(me + F_CHARACTER) = (uint8_t)to; }
    if ((S8(other + F_CHOSEN) & 5) == 0) return;
    if (S8(other + F_COLOUR) != S8(me + F_COLOUR)) return;
    int8_t a = S8(me + F_CHARACTER), b = S8(other + F_CHARACTER);
    if (a == b || (a == 0x15 && b == 0x25) || (a == 0x25 && b == 0x15))
        *B(me + F_COLOUR) = (uint8_t)(S8(me + F_COLOUR) ^ 1);
}

/* The modes where both share one ISM (0x088A39CC): 13, 19, Survival's fights 4 / 5, Free Battle's
   kinds 3-5: the other fighter takes this one's. Returns 0. */
static void shared_ism(gaddr me, gaddr other) {
    int8_t mode = S8(GAME + G_MODE), state = S8(BATTLE + B_FIGHT_STATE), kind = S8(BATTLE + B_FREE_KIND);
    int share = mode == MODE_13 || mode == MODE_19 ||
                (mode == MODE_SURVIVAL && (state == 5 || state == 4)) ||
                (mode == MODE_FREE && (kind == 3 || kind == 4 || kind == 5));
    if (!share) return;
    uint8_t ism = *B(me + F_ISM);
    *B(other + F_ISM) = ism;
    *B(other + F_LAST_ISM) = ism;
    *H(other + F_DONE) = (uint16_t)(S16(other + F_DONE) | 4);
}

/* Wait (0x088A3B70): then the next state; for a team, the member's ISM into the list and the next
   member (back to state 1) until the last. */
static void ism_wait(Cpu *c, gaddr me, gaddr other, gaddr list) {
    int16_t t = (int16_t)(S16(me + F_TIMER) - 1);
    *H(me + F_TIMER) = (uint16_t)t;
    if (t != 0) return;
    if (!team_kind()) {
        CALL2(f_088A39CC, me, other);
        CALL2(f_088A4C68, me, other);
        *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
        *B(me + F_STEP) = 0;
        *H(me + F_DONE) = (uint16_t)(S16(me + F_DONE) | 4);
        return;
    }
    *B(member(list) + L_ISM) = *B(me + F_ISM);
    CALL2(f_088A4C68, me, other);
    CALL2(f_088A377C, me, list);
    int8_t next = (int8_t)(S8(list + L_NOW) + 1);
    *B(list + L_NOW) = (uint8_t)next;
    uint16_t done = *H(me + F_DONE);
    if (S8(list + L_LAST) < next) {
        *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
        *B(me + F_STEP) = 0;
        *H(me + F_DONE) = (uint16_t)((int16_t)done | 4);
        return;
    }
    *B(me + F_STATE) = 1;
    *B(me + F_STEP) = 0;
    *H(me + F_DONE) = (uint16_t)((int16_t)done & 0x1F8);
    *B(me + F_CHOSEN) = (uint8_t)(S8(me + F_CHOSEN) & 0x1F8);
    gaddr m = member(list);
    *B(m + L_CHAR) = *B(me + F_CHARACTER);
    *B(m + L_ISM) = *B(me + F_ISM);
    *B(m + L_BOX) = *B(me + F_BOX);
}

/* ---- states 4-9 ---- */

#define STEPS_4        0x08A2B948u
#define STEPS_5        0x08A2B960u
#define STEPS_6        0x08A2B954u
#define STEPS_7        0x08A2B93Cu
#define STEPS_8        0x08A2B96Cu
#define SPEEDS_B       0x08A2B3E0u   /* u8 by GAME + 124 + GAME + 118 - 1: BATTLE + 22 */
#define RANDOM_STAGES  0x08A2B3E6u   /* s8 by stage choice - 1, or by a random number & 0x3F for RANDOM */
#define G_SPEED_PICK   124           /* u8: bit 0 the speed choice */
#define G_DIFFICULTY   190           /* s8: 0-7 */
#define G_STAGE        193           /* s8: 0 RANDOM, 1-38 */
#define G_STAGE_SHOWN  194           /* s8: the stage row is up (one fighter chooses for both) */

void f_088A3DE8(Cpu *c); void f_088A3EF8(Cpu *c); void f_088A3E80(Cpu *c); void f_08821368(Cpu *c);
void f_088969FC(Cpu *c);   /* (fighter): the choice counted (cursor.c) */

/* A state's step by + 5 from its table, second kind (the handler in a2, the table in a3). */
static void state_step_b(Cpu *c, gaddr table) {
    gaddr fn = *W(table + *B(c->r[4] + F_STEP) * 4u);
    c->r[6] = fn; c->r[7] = table;
    rt_call_indirect(c, fn);
}

/* A row's panel, kind `kind`, for the fighter (no timer), with its parts done (+ 424's low byte, or
   with `high` its high byte). */
static void panel_only(Cpu *c, gaddr me, uint32_t kind, int high) {
    gaddr t = CALL2(f_08823338, 0xA, kind & 0xFF);
    if (t == 0) return;
    *W(t + 124) = me;
    *B(t + 140) = (uint8_t)(high ? S16(me + F_DONE) >> 8 : S16(me + F_DONE));
}

static void step_by(gaddr me, int n) { *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + n); }
static void state_done(gaddr me, uint16_t bit) {
    *H(me + F_DONE) = (uint16_t)(S16(me + F_DONE) | bit);
    *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
    *B(me + F_STEP) = 0;
}

/* State 4's enter (0x088A3F98): done (0x10) on to the next state; otherwise its panel (kind 4, or 9 in
   modes 13 / 19, plus the box's side). */
static void state4_enter(Cpu *c, gaddr me) {
    *H(me + F_TIMER) = WAIT_CHOICE_B;
    if (S16(me + F_DONE) & 0x10) { *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1); return; }
    step_by(me, 1);
    uint32_t right = CALL1(f_088A4EB4, me);
    int8_t mode = S8(GAME + G_MODE);
    panel_only(c, me, (mode == MODE_13 || mode == MODE_19 ? 9 : 4) + right, 0);
}

/* State 5's enter (0x088A4410): for a team (mode 6, Free Battle's kind 2) its panel (kind 0x11 plus
   the side, + 424's high byte) and the time; otherwise straight to its last step. */
static void state5_enter(Cpu *c, gaddr me) {
    uint32_t right = CALL1(f_088A4EB4, me);
    if (!team_kind()) { step_by(me, 2); return; }
    step_by(me, 1);
    panel_only(c, me, right + 0x11, 1);
    *H(me + F_TIMER) = WAIT_CHOICE_B;
}

/* State 6's enter (0x088A4210, the difficulty): only in Free Battle, for the fighter on the starter's
   side; skipped to the wait when the other has chosen and the layout isn't two players. */
static void difficulty_enter(Cpu *c, gaddr me, gaddr other) {
    uint32_t right = CALL1(f_088A4EB4, me);
    uint32_t side = S8(GAME + G_PRESENT) == 2 ? 0 : 1;
    if (S8(GAME + G_MODE) != MODE_FREE || right != side) { step_by(me, 2); return; }
    if ((S16(other + F_DONE) & 0x80) && S16(BATTLE + B_LAYOUT) != 0) { step_by(me, 2); return; }
    step_by(me, 1);
    panel_only(c, me, right + 0xF, 0);
}

/* State 6's choice (0x088A42E0): up / down step GAME + 190 round 0-7; an attack button chooses. */
static void difficulty_choose(Cpu *c, gaddr me) {
    uint32_t pressed = CALL1(f_088A507C, me);
    gaddr d = GAME + G_DIFFICULTY;
    if (pressed & 8) {
        *B(d) = (uint8_t)(S8(d) - 1);
        CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
    } else if (pressed & 4) {
        *B(d) = (uint8_t)(S8(d) + 1);
        CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
    }
    *B(d) = (uint8_t)(S8(d) + 8);
    int32_t v = S8(d);
    *B(d) = (uint8_t)(v >= 0 ? (v & 7) : -((-v) & 7));
    if (pressed & 0x3F0) {
        CALL0(f_088A4E90);
        step_by(me, 1);
    }
}

/* State 7's enter (0x088A3D0C, the speed): shown by the first fighter to get here (GAME + 20's high
   byte), with its panel; without a speed (GAME + 118) or already shown, straight to the wait. */
static void speed_enter(Cpu *c, gaddr me, gaddr other) {
    if (!(S16(me + F_DONE) & 8) && !(S16(GAME + G_SPEED) & 0xFF00)) {
        step_by(me, 1);
        *H(GAME + G_SPEED) = (uint16_t)(S16(GAME + G_SPEED) | 0x101);
        *H(me + F_TIMER) = WAIT_CHOICE_B;
        panel_only(c, me, 3, 0);
        CALL2(f_088A3DE8, me, other);
        return;
    }
    if (S8(GAME + G_SPEED_BASE) == 0) {
        int16_t v = S16(GAME + G_SPEED);
        if ((v & 0xFF00) == 0) *H(GAME + G_SPEED) = (uint16_t)(v | 0x101);
    }
    *H(me + F_TIMER) = 1;
    CALL2(f_088A3EF8, me, other);
}

/* The speed chosen (0x088A3E80): BATTLE + 22 from SPEEDS_B. */
static void speed_chosen(Cpu *c, gaddr me) {
    step_by(me, 1);
    uint32_t k = (uint8_t)(uint16_t)(*B(GAME + G_SPEED_PICK) + S8(GAME + G_SPEED_BASE) - 1);
    *B(BATTLE + BT_SPEED) = *B(SPEEDS_B + k);
    *B(me + F_CHOSEN) = (uint8_t)(S8(me + F_CHOSEN) | 4);
    *H(me + F_TIMER) = 8;
    CALL0(f_088A4E90);
}

/* State 7's choice (0x088A3DE8): an attack button (or, network, time) chooses; up / down switch. */
static void speed_choose(Cpu *c, gaddr me) {
    if (CALL1(f_088A21C4, me) != 0) { CALL1(f_088A3E80, me); return; }
    int32_t pressed = (int16_t)CALL1(f_088A507C, me);
    if (pressed & 0x3F0) { CALL1(f_088A3E80, me); return; }
    if (!(pressed & 0xC)) return;
    CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
    *B(GAME + G_SPEED_PICK) = (uint8_t)(*B(GAME + G_SPEED_PICK) ^ 1);
}

/* State 7's wait (0x088A3EF8): then done; outside Training and Free Battle the stage's state too. */
static void speed_wait(gaddr me) {
    int16_t t = (int16_t)(S16(me + F_TIMER) - 1);
    *H(me + F_TIMER) = (uint16_t)t;
    if (t != 0) return;
    *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
    *B(me + F_STEP) = 0;
    *H(me + F_DONE) = (uint16_t)(S16(me + F_DONE) | 8);
    int8_t mode = S8(GAME + G_MODE);
    if (mode != MODE_TRAINING && mode != MODE_FREE) *B(me + F_STATE) = (uint8_t)(*B(me + F_STATE) + 1);
}

/* State 8's enter (0x088A45B8, the stage): done, or already up for the other fighter: the next
   state; otherwise up (GAME + 194), with its panel. */
static void stage_enter(Cpu *c, gaddr me) {
    *H(me + F_TIMER) = WAIT_CHOICE_B;
    if ((S16(me + F_DONE) & 0x40) || S8(GAME + G_STAGE_SHOWN) != 0) { state_done(me, 0x40); return; }
    step_by(me, 1);
    *B(GAME + G_STAGE_SHOWN) = 1;
    panel_only(c, me, 8, 0);
}

/* State 8's choice (0x088A4650): up / down go round the stages (0 RANDOM, 1-38); an attack button (or,
   network, time) chooses: BATTLE + 24. */
static void stage_choose(Cpu *c, gaddr me) {
    int chosen = CALL1(f_088A21C4, me) != 0;
    int32_t pressed = 0;
    if (!chosen) { pressed = (int16_t)CALL1(f_088A507C, me); chosen = (pressed & 0x3F0) != 0; }
    gaddr st = GAME + G_STAGE;
    if (chosen) {
        step_by(me, 1);
        *B(me + F_CHOSEN) = (uint8_t)(S8(me + F_CHOSEN) | 0x40);
        *H(me + F_TIMER) = 8;
        *H(BATTLE + 24) = (uint16_t)(int16_t)S8(st);
        CALL0(f_088A4E90);
    } else if (pressed & 4) {
        CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
        *B(st) = (uint8_t)(S8(st) + 1);
        if (S8(st) >= 39) *B(st) = 0;
    } else if (pressed & 8) {
        CALL3(f_0887C4CC, 0, SOUND_MOVE, 0xC0);
        *B(st) = (uint8_t)(S8(st) - 1);
        if (S8(st) < 0) *B(st) = 0x26;
    }
}

/* State 9 (0x088A47A0): the fighter is done, once (GAME + 10); unless it fights already (GAME + 57),
   BATTLE + 0 and + 100 the stage (RANDOM draws one), and the choice counted. */
static void fighter_done(Cpu *c, gaddr me) {
    uint16_t bit = (uint16_t)(1u << (S8(me + F_SLOT) & 31));
    uint16_t done = *H(GAME + G_DONE);
    if (done & bit) return;
    *H(GAME + G_DONE) = (uint16_t)(done | bit);
    if (S8(GAME + G_FIGHTS) & bit) return;
    int8_t stage = S8(GAME + G_STAGE);
    int32_t v;
    if (stage == 0) v = S8(RANDOM_STAGES + (CALL0(f_08821368) & 0x3F));
    else v = S8(RANDOM_STAGES + (uint32_t)(int32_t)stage - 1);
    *H(BATTLE + 0) = (uint16_t)v;
    *H(BATTLE + 100) = (uint16_t)v;
    CALL1(f_088969FC, me);
}

/* The frame's one-line handlers */
static void colours_two(Cpu *c, gaddr a, gaddr b) { CALL2(f_088A1ED8, a, b); }

SFA_FUNC(088A0664, game_bselect_step)        { SELECT_FRAME(0x20, run_step(c, STEPS, *H(GAME + G_SUB))); }
SFA_FUNC(088A06A4, game_bselect_start_step)  { SELECT_FRAME(0x20, start_step(c, ARG(0))); }
SFA_FUNC(088A13F8, game_bselect_frame_step)  { SELECT_FRAME(0x20, frame_step(c, ARG(0), ARG(1))); }
SFA_FUNC(088A1478, game_bselect_first_frame) { first_frame(ARG(0)); }
SFA_FUNC(088A5B38, game_bselect_frame)       { SELECT_FRAME(0x50, screen_frame(c, ARG(0), ARG(1))); }
SFA_FUNC(088A14B0, game_bselect_title)       { SELECT_FRAME(0x30, title(c, ARG(0))); }
SFA_FUNC_WRAPPED(088A1548, game_bselect_speed, selectscreen_speed) { SELECT_FRAME(0x30, speed_line(c, ARG(0))); }
SFA_FUNC(088A160C, game_bselect_stage_line)  { SELECT_FRAME(0x30, stage_line(c, ARG(0))); }
SFA_FUNC(088A06D8, game_bselect_names_shown) { RETURN(names_shown()); }
SFA_FUNC(088A167C, game_bselect_entry_names) { SELECT_FRAME(0x40, entry_names(c)); }
SFA_FUNC(088A1D30, game_bselect_expected) {
    int32_t r;
    SELECT_FRAME(0x10, r = expected_done(c, ARG(0), ARG(1)));
    if (r >= 0) RETURN(r);
}
SFA_FUNC(088A21C4, game_bselect_net_timeout) { int r; SELECT_FRAME(0x20, r = network_time_out(c, ARG(0))); RETURN(r); }
SFA_FUNC(088A225C, game_bselect_cursor)      { SELECT_FRAME(0x20, fighter_cursor(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A1828, game_bselect_training_turn) { training_turn(ARG(0), ARG(1)); }
SFA_FUNC_WRAPPED(088A5174, game_bselect_box_driver, selectscreen_box_driver) { RETURN(box_driver(ARG(0))); }
SFA_FUNC(088A60F8, game_bselect_fighter_box) { SELECT_FRAME(0x60, fighter_box(c, ARG(0), ARG(1))); }
SFA_FUNC(088A2578, game_bselect_difficulty)  { SELECT_FRAME(0x30, box_difficulty(c, ARG(0))); }
SFA_FUNC(088A4D30, game_bselect_style_line)  { SELECT_FRAME(0x40, box_style(c, ARG(0), ARG(1) & 0xFFFF)); }
SFA_FUNC(088A6A10, game_bselect_grid)        { SELECT_FRAME(0x30, bselect_grid(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A2480, game_bselect_team_lines)  { SELECT_FRAME(0x40, box_team(c, ARG(0))); }
SFA_FUNC(088A5600, game_bselect_free_turn)  { SELECT_FRAME(0x30, free_battle_turn(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A1AB8, game_bselect_cursors_on)  { SELECT_FRAME(0x20, cursors_on(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A1F38, game_bselect_x_ism)       { x_ism(ARG(0)); }
SFA_FUNC(088A1ED8, game_bselect_next_colours) { next_colours(ARG(0), ARG(1)); }
SFA_FUNC(088A1CE8, game_bselect_colours_two) { SELECT_FRAME(0x20, colours_two(c, ARG(2), ARG(3))); }
SFA_FUNC(088A1D08, game_bselect_survival_reset) { survival_reset(ARG(0), ARG(1), ARG(2)); }
SFA_FUNC(088A08E4, game_bselect_start_0)    { SELECT_FRAME(0x40, start_0(c, ARG(0), ARG(1))); }
SFA_FUNC(088A078C, game_bselect_entry_boxes) { SELECT_FRAME(0x30, entry_boxes(c)); }
SFA_FUNC(088A0ADC, game_bselect_start_1)    { SELECT_FRAME(0x40, start_1(c, ARG(0), ARG(1))); }
SFA_FUNC(088A0CE4, game_bselect_start_2)    { SELECT_FRAME(0x10, start_2(c, ARG(0))); }
SFA_FUNC(088A0D44, game_bselect_start_3)    { SELECT_FRAME(0x10, start_3(c, ARG(0))); }
SFA_FUNC(088A1F98, game_bselect_end)        { SELECT_FRAME(0x20, end_step(c, ARG(0), ARG(1))); }
SFA_FUNC(088A283C, game_bselect_place)      { place_of(ARG(0), ARG(1), ARG(2)); }
SFA_FUNC(088A6C74, game_bselect_fighter_setup) { SELECT_FRAME(0x30, fighter_setup(c, ARG(0), (int8_t)ARG(1))); }
SFA_FUNC(088A4874, game_bselect_make_tasks) { SELECT_FRAME(0x40, make_tasks(c)); }
SFA_FUNC(088A2808, game_bselect_state_1)    { SELECT_FRAME(0x20, state_step(c, STEPS_1)); }
SFA_FUNC(088A3414, game_bselect_state_2)    { SELECT_FRAME(0x20, state_2_step(c)); }
SFA_FUNC(088A37F8, game_bselect_state_3)    { SELECT_FRAME(0x20, state_step(c, STEPS_3)); }
SFA_FUNC(088A507C, game_bselect_pad_pressed) { uint32_t r; SELECT_FRAME(0x30, r = driving_pad(c, ARG(0), 1)); RETURN(r); }
SFA_FUNC(088A5100, game_bselect_pad_held)   { uint32_t r; SELECT_FRAME(0x30, r = driving_pad(c, ARG(0), 0)); RETURN(r); }
SFA_FUNC(088A4E90, game_bselect_sound_choose) { SELECT_FRAME(0x20, sound_choose(c)); }
SFA_FUNC(088A4EB4, game_bselect_box_right)  { RETURN(box_on_right(ARG(0))); }
SFA_FUNC(088A2870, game_bselect_char_enter) { SELECT_FRAME(0x20, character_enter(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A29B4, game_bselect_char_move)  { SELECT_FRAME(0x20, character_move(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A2FDC, game_bselect_char_chosen) { SELECT_FRAME(0x20, character_chosen(c, ARG(0), ARG(2))); }
SFA_FUNC(088A2D94, game_bselect_as_himself) { SELECT_FRAME(0x30, as_himself(c, ARG(0))); }
SFA_FUNC(088A315C, game_bselect_char_wait)  { character_wait(ARG(0), ARG(2)); }
SFA_FUNC(088A4E30, game_bselect_style_flags) { style_flags(ARG(0)); }
SFA_FUNC(088A34A8, game_bselect_style_enter) { SELECT_FRAME(0x30, style_enter(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A35E4, game_bselect_style_choose) { SELECT_FRAME(0x30, style_choose(c, ARG(0))); }
SFA_FUNC(088A373C, game_bselect_style_wait) { row_wait(ARG(0), 2); }
SFA_FUNC(088A3AA0, game_bselect_ism_chosen) { SELECT_FRAME(0x10, ism_chosen(c, ARG(0))); }
SFA_FUNC(088A382C, game_bselect_ism_enter)  { SELECT_FRAME(0x20, ism_enter(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A3914, game_bselect_ism_choose) { SELECT_FRAME(0x20, ism_choose(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A4C68, game_bselect_ism_character) { ism_character(ARG(0), ARG(1)); }
SFA_FUNC(088A39CC, game_bselect_shared_ism) { shared_ism(ARG(0), ARG(1)); RETURN(0); }
SFA_FUNC(088A3B70, game_bselect_ism_wait)   { SELECT_FRAME(0x20, ism_wait(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A3F64, game_bselect_state_4)    { SELECT_FRAME(0x20, state_step_b(c, STEPS_4)); }
SFA_FUNC(088A43DC, game_bselect_state_5)    { SELECT_FRAME(0x20, state_step(c, STEPS_5)); }
SFA_FUNC(088A41DC, game_bselect_state_6)    { SELECT_FRAME(0x20, state_step_b(c, STEPS_6)); }
SFA_FUNC(088A3CD8, game_bselect_state_7)    { SELECT_FRAME(0x20, state_step_b(c, STEPS_7)); }
SFA_FUNC(088A4584, game_bselect_state_8)    { SELECT_FRAME(0x20, state_step_b(c, STEPS_8)); }
SFA_FUNC(088A3F98, game_bselect_state4_enter) { SELECT_FRAME(0x20, state4_enter(c, ARG(0))); }
SFA_FUNC(088A4410, game_bselect_state5_enter) { SELECT_FRAME(0x20, state5_enter(c, ARG(0))); }
SFA_FUNC(088A4564, game_bselect_state5_done) { *H(ARG(0) + F_DONE) = (uint16_t)(S16(ARG(0) + F_DONE) | 0x100); *B(ARG(0) + F_STATE) = (uint8_t)(*B(ARG(0) + F_STATE) + 1); *B(ARG(0) + F_STEP) = 0; }
SFA_FUNC(088A4210, game_bselect_diff_enter) { SELECT_FRAME(0x20, difficulty_enter(c, ARG(0), ARG(1))); }
SFA_FUNC(088A42E0, game_bselect_diff_choose) { SELECT_FRAME(0x20, difficulty_choose(c, ARG(0))); }
SFA_FUNC(088A43BC, game_bselect_diff_done)  { state_done(ARG(0), 0x80); }
SFA_FUNC(088A3D0C, game_bselect_speed_enter) { SELECT_FRAME(0x20, speed_enter(c, ARG(0), ARG(1))); }
SFA_FUNC(088A3DE8, game_bselect_speed_choose) { SELECT_FRAME(0x20, speed_choose(c, ARG(0))); }
SFA_FUNC(088A3E80, game_bselect_speed_chosen) { SELECT_FRAME(0x10, speed_chosen(c, ARG(0))); }
SFA_FUNC(088A3EF8, game_bselect_speed_wait) { speed_wait(ARG(0)); }
SFA_FUNC(088A45B8, game_bselect_stage_enter) { SELECT_FRAME(0x20, stage_enter(c, ARG(0))); }
SFA_FUNC(088A4650, game_bselect_stage_choose) { SELECT_FRAME(0x20, stage_choose(c, ARG(0))); }
SFA_FUNC(088A4760, game_bselect_stage_wait) { row_wait(ARG(0), 0x40); }
SFA_FUNC(088A47A0, game_bselect_fighter_done) { SELECT_FRAME(0x30, fighter_done(c, ARG(0))); }
