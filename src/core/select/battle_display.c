/* The Survival / Free Battle select's display tasks: the state handlers that display.c's entries run
 * (0x088A712C with table 0x08A2BB08, 0x088A7694 with 0x08A2BB4C, and so on: docs/PSP_PROGRAM_MAP.md).
 * Each is called with a0 = the task, a1 = a2 = its player (a fighter) and a3 = the handler.
 *
 * A task is an object (0x94+ bytes): + 4 its state (the entry's table), + 5 the state's step, + 9 its
 * kind for 0x088A8664, + 10 which of its kind it is (0 / 1 / ...), + 12 / + 13 shown / its picture,
 * + 18 / + 22 x / y, + 78 what it shows now (a character, -1 none), + 82 a counter, + 86 / + 148 /
 * + 164 drawing set-up, + 139 / + 140 the grid row / column it follows, + 160 its alpha.
 *
 * Decompiled here so far: the backdrop's and the cursor frames' handlers 0x088A6FF8 - 0x088A762C, and
 * the Dramatic kinds' cursor frames 0x088A7CEC - 0x088A7FD0 (the same steps, by pad slot), and mode 6 /
 * TEAM's portraits and team list 0x088A76CC - 0x088A7A4C; the Dramatic kinds' portraits 0x088A8070 -
 * 0x088A8388, the panels that blink 0x088A8698 - 0x088A89E4 and the backdrop's banner 0x088A8F14 /
 * 0x088A8F48. Not reached by any scenario, left translated: 0x088A8488 - 0x088A8E64 (22 functions).
 */
#include "select.h"

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_0887C4CC(Cpu *c);   /* (bank, sound, volume) */
void f_088215C4(Cpu *c);   /* (object): draw it by its kind */
void f_0882196C(Cpu *c);   /* (object, animation): start an animation */
void f_088A8664(Cpu *c); void f_088A75D0(Cpu *c); void f_088A7540(Cpu *c); void f_088A762C(Cpu *c);   /* below */
void f_088A7E20(Cpu *c); void f_088A7FD0(Cpu *c);
void f_0880A6CC(Cpu *c);   /* (text, font, x, y, colour) */
void f_088233BC(Cpu *c);   /* (task): a panel's parts changed: start again */
void f_088231C8(Cpu *c);   /* (task) */
void f_0880AEA8(Cpu *c);   /* (1, set, 0, 1, 2): load a sprite set */

/* task fields */
#define T_SUB        3      /* u8: the sub-state (0x088A6FF8's table) */
#define T_STATE      4      /* u8 */
#define T_STEP       5      /* u8 */
#define T_KIND       9      /* u8 */
#define T_WHICH      10     /* u8: which of its kind (0, 1, ...) */
#define T_SHOWN      12     /* u8 */
#define T_PICTURE    13     /* u8 */
#define T_X          18     /* s16 */
#define T_Y          22     /* s16 */
#define T_LAYER      24     /* s16 */
#define T_SHOWS      78     /* s8: the character it shows (-1 none) */
#define T_COUNTER    82     /* s16 */
#define T_DRAW_86    86     /* s16 */
#define T_PAD        112    /* s16: the player's pad slot (the Dramatic kinds' frames) */
#define T_SLOT_COPY  121    /* u8 */
#define T_GRID_ROW   139    /* u8 */
#define T_GRID_COL   140    /* u8 */
#define T_DRAW_148   148    /* u32 */
#define T_ALPHA      160    /* u8 */
#define T_DRAW_164   164    /* u32 */

#define G_FRAME20    32     /* u8: GAME + 0x20, the fight's frame counter */
#define G_TURN       197    /* s8: bit 0x80 while the first player's turn is over (training_turn) */
#define B_FREE_KIND  159
#define MODE_6       6
#define MODE_FREE    16

#define ANIMS        0x08A29FC4u   /* animation pointers: + 4 * kind */
#define CURRENT_SET  0x092E3E98u   /* u32: the display's sprite set */
#define SPRITE_SET   0x08A29A28u
#define GRID_X       0x08A2BB2Cu   /* s16 by column */
#define GRID_Y       0x08A2BB3Cu   /* s16 by row */

static int8_t mode_now(void) { return S8(GAME + G_MODE); }
static int team_mode(void) { return mode_now() == MODE_6 || (mode_now() == MODE_FREE && S8(BATTLE + B_FREE_KIND) == 2); }
static void step_up(gaddr t, int field) { *B(t + field) = (uint8_t)(*B(t + field) + 1); }

/* Clear a task's drawing state for a new kind (0x088A8664) */
static void task_reset(gaddr t, uint32_t kind) {
    *B(t + T_KIND) = (uint8_t)kind;
    *H(t + 20) = 0; *H(t + 16) = 0; *H(t + 38) = 0;
    *B(t + 15) = 0; *B(t + 14) = 0; *B(t + T_PICTURE) = 0; *B(t + T_SHOWN) = 0; *B(t + 11) = 0;
    *H(t + T_LAYER) = 7;
}

/* A sub-handler from a table by a task's byte, the registers as the original leaves them (a1 = the
   handler, a2 = the table) */
static void by_table(Cpu *c, gaddr t, int field, gaddr table) {
    gaddr fn = *W(table + *B(t + field) * 4u);
    c->r[4] = t; c->r[5] = fn; c->r[6] = table;
    rt_call_indirect(c, fn);
}
/* A step handler (task, player), as 0x088A7248 calls it: a1 = a2 = the player, a3 = the handler */
static void run_step(Cpu *c, gaddr t, gaddr p, gaddr table) {
    gaddr fn = *W(table + *B(t + T_STEP) * 4u);
    c->r[4] = t; c->r[5] = p; c->r[6] = p; c->r[7] = fn;
    rt_call_indirect(c, fn);
}

/* 0x088A6FF8 / 0x088A702C: by sub-state (+ 3) and by state (+ 4) */
static void by_sub(Cpu *c, gaddr t)   { by_table(c, t, T_SUB, 0x08A2BAD8u); }
static void by_state(Cpu *c, gaddr t) { by_table(c, t, T_STATE, 0x08A2BAF8u); }

/* 0x088A7060: the backdrop starts: sprite set, place, the animation for its kind */
static void backdrop_start(Cpu *c, gaddr t) {
    step_up(t, T_STATE);
    CALL2(f_088A8664, t, ((uint32_t)*B(t + T_WHICH) * 2 + 1) & 0xFF);
    *W(t + T_DRAW_148) = 1;
    *H(t + T_DRAW_86) = 0xB;
    *W(t + T_DRAW_164) = 1;
    *W(CURRENT_SET) = SPRITE_SET;
    *H(t + T_X) = 0xD0; *H(t + T_Y) = 0x8C; *H(t + T_LAYER) = 7;
    CALL2(f_0882196C, t, *W(ANIMS + (*B(t + T_WHICH) ? 3u : 2u) * 4));
}

/* 0x088A7164: a cursor frame starts, over its player's box (not in mode 6 / TEAM unless its player
   has chosen) */
static void frame_start(Cpu *c, gaddr t, gaddr p) {
    if (!(S16(p + F_DONE) & 0x20)) {
        if (mode_now() == MODE_6) return;
        if (mode_now() == MODE_FREE && S8(BATTLE + B_FREE_KIND) == 2) return;
    }
    step_up(t, T_STATE);
    *H(t + T_COUNTER) = 0;
    *W(t + T_DRAW_148) = 0;
    *H(t + T_DRAW_86) = 8;
    *B(t + T_SHOWS) = 0xFF;
    *B(t + T_GRID_ROW) = (uint8_t)S16(p + F_ROW);
    *B(t + T_GRID_COL) = (uint8_t)S16(p + F_COLUMN);
    CALL2(f_088A8664, t, ((uint32_t)*B(t + T_WHICH) * 2) & 0xFF);
    int8_t slot = S8(p + F_SLOT);
    *B(t + T_PICTURE) = (uint8_t)slot;
    *B(t + T_SLOT_COPY) = (uint8_t)slot;
    *B(t + T_SHOWN) = (uint8_t)slot ? 0xFF : 0;
    if (*B(t + T_WHICH)) *B(t + T_ALPHA) = 0x80;
}

/* 0x088A7248: a cursor frame's frame: its counter, then its step (0x08A2BB18) while its player
   fights, or while it is the other player's turn and that one is still choosing */
static void frame_run(Cpu *c, gaddr t, gaddr p) {
    int16_t n = (int16_t)(S16(t + T_COUNTER) + 0x80);
    *H(t + T_COUNTER) = (uint16_t)(n < 2560 ? n : 0);
    int16_t players = (int16_t)(S8(GAME + G_FIGHTS) | S8(GAME + G_CHOSE));
    int8_t slot = S8(p + F_SLOT);
    if (((1u << (slot & 31)) & 0xFFFF) & (uint16_t)players) { run_step(c, t, p, 0x08A2BB18u); return; }
    gaddr other = slot != 0 ? FIGHTER_1 : FIGHTER_2;
    if ((S8(GAME + G_TURN) & 0x80) && (S16(other + F_DONE) & 1)) run_step(c, t, p, 0x08A2BB18u);
}

/* 0x088A732C: next step */
static void next_step(gaddr t) { step_up(t, T_STEP); }

/* 0x088A733C: step 0: on to step 1, or 3 if its player's character is done; follow the cursor */
static void frame_step0(Cpu *c, gaddr t, gaddr p) {
    step_up(t, T_STEP);
    if (S16(p + F_DONE) & 1) *B(t + T_STEP) = (uint8_t)(*B(t + T_STEP) + 2);
    c->r[6] = p;
    CALL2(f_088A75D0, t, p);
}

/* 0x088A7390: step 1, choosing: follow the cursor, the frame's picture by its counter; shown while
   its player's cursor runs (or the character is done) */
static void frame_step1(Cpu *c, gaddr t, gaddr p) {
    if (S8(p + F_CHOSEN) & 1) step_up(t, T_STEP);
    CALL2(f_088A7540, t, p);
    uint32_t frame = ((uint32_t)(int32_t)(uint16_t)S16(t + T_COUNTER) >> 8) & 0xFFFF;
    CALL3(f_088A762C, t, p, frame);
    if ((S16(p + F_DONE) & 1) || *W(p + F_CURSOR_ON)) CALL1(f_088215C4, t);
}

/* 0x088A7420: step 2, chosen: blinks (every other frame) */
static void frame_step2(Cpu *c, gaddr t, gaddr p) {
    if (S16(p + F_DONE) & 1) step_up(t, T_STEP);
    CALL3(f_088A762C, t, p, 0);
    if ((uint32_t)(*B(GAME + G_FRAME20) + S8(p + F_SLOT)) & 1) CALL1(f_088215C4, t);
}

/* 0x088A749C: step 3, done: back to the start if its player takes the character back (mode 6 /
   TEAM); on to the next state once the speed is set */
static void frame_step3(Cpu *c, gaddr t, gaddr p) {
    if (team_mode() && !(S16(p + F_DONE) & 1)) { *B(t + T_STEP) = 0; *B(t + T_STATE) = 0; }
    if (S16(GAME + G_FRAME) & 0xFF00) step_up(t, T_STATE);
    CALL3(f_088A762C, t, p, 0);
    CALL1(f_088215C4, t);
}

/* 0x088A7540: follow the cursor, with a tick when it moves onto a random box (every fourth frame) */
static void frame_follow(Cpu *c, gaddr t, gaddr p) {
    if (!(S8(p + F_CHOSEN) & 1) && *B(t + T_WHICH) == 0 && S8(p + F_RANDOM_BOX) != 0 && S8(p + F_BOX) == 0
        && (*B(GAME + G_FRAME20) & 3) == 0)
        CALL3(f_0887C4CC, 0, 0x38, 0xC0);
    CALL2(f_088A75D0, t, p);
}

/* 0x088A75D0: the frame onto its player's box */
static void frame_place(gaddr t, gaddr p) {
    *B(t + T_SHOWS) = (uint8_t)S8(p + F_CHARACTER);
    int16_t col = S16(p + F_COLUMN), row = S16(p + F_ROW);
    *B(t + T_GRID_ROW) = (uint8_t)row;
    *B(t + T_GRID_COL) = (uint8_t)col;
    *H(t + T_X) = (uint16_t)S16(GRID_X + (uint32_t)((int8_t)col * 2));
    *H(t + T_Y) = (uint16_t)S16(GRID_Y + (uint32_t)((int8_t)row * 2));
}

/* 0x088A762C: the frame's animation: by its player's slot, or for the second frame by frame parity
   (6 / 7), at picture `frame` */
static void frame_anim(Cpu *c, gaddr t, gaddr p, uint32_t frame) {
    uint32_t kind = (uint8_t)S8(p + F_SLOT);
    if (*B(t + T_WHICH)) kind = (((kind + *B(GAME + G_FRAME20)) & 0xFF & 1) + 6) & 0xFF;
    CALL2(f_0882196C, t, *W(ANIMS + kind * 4) + (frame & 0xFFFF) * 8);
}

/* ---- mode 6 / TEAM: the portraits and the team list ---- */
#define B_FIGHT_STATE 79
#define MODE_SURVIVAL 7
#define TEAM_LIST_1  0x09517A28u   /* a side's team: + 0 last member, + 1 member now, + 3 / + 6 by member */
#define TEAM_LIST_2  0x09517A50u
#define TEAM_NAMES   0x08A2BB80u   /* char *: by member (+ 0), characters' names by character (+ 16) */
#define S_TEAM_MARK  0x08A2BB98u
#define NAME_COLOURS 0x08A2BB78u   /* u16: + 2 by character */
#define PORTRAIT_UP  0x08A2BB6Cu   /* s16 by which: how far above 0xCC */

static int player_up(gaddr p) {   /* its player fights, or it's the first player's turn over */
    int players = S8(GAME + G_FIGHTS) | S8(GAME + G_CHOSE);
    return (players & (1 << (S8(p + F_SLOT) & 31))) || (S8(GAME + G_TURN) & 0x80);
}
static void text0(Cpu *c, gaddr s, int32_t x, int32_t y, uint32_t colour) { CALL5(f_0880A6CC, s, 0, (uint32_t)x, (uint32_t)y, colour); }

/* 0x088A76CC: a big portrait starts, placed by mode (Survival's fights 4 / 5 by who is present) */
static void portrait_start(Cpu *c, gaddr t, gaddr p) {
    if (!player_up(p)) return;
    step_up(t, T_STATE);
    *W(t + T_DRAW_148) = 0;
    *H(t + T_DRAW_86) = 0xB;
    *W(t + T_DRAW_164) = 1;
    CALL2(f_088A8664, t, 0);
    int8_t state = S8(BATTLE + B_FIGHT_STATE);
    *B(t + T_SHOWS) = 0xFF;
    if (mode_now() == MODE_SURVIVAL && (state == 4 || state == 5)) {
        *H(t + T_Y) = 0xB4;
        int8_t slot = S8(p + F_SLOT);
        if (S8(GAME + G_PRESENT) == 2) *H(t + T_X) = slot == 0 ? 0x130 : 0x158;
        else *H(t + T_X) = slot == 0 ? 0x28 : 0x50;
        return;
    }
    *H(t + T_X) = S8(p + F_SLOT) ? 0x130 : 0x50;
    *H(t + T_Y) = 0xCC;
}

/* 0x088A781C: the portrait shows its player's character (the first) or the second's picture; the
   first only while its player chooses or has chosen */
static void portrait_run(Cpu *c, gaddr t, gaddr p) {
    uint32_t ch = (uint8_t)S8(p + F_CHARACTER);
    int same = (int32_t)S8(t + T_SHOWS) == (int32_t)ch;
    if (*B(t + T_WHICH) == 0) {
        if (!same) CALL2(f_0882196C, t, *W(ANIMS + 16) + ch * 8);
        if (!(S16(p + F_DONE) & 1) && !*W(p + F_CURSOR_ON)) return;
        *B(t + T_SHOWS) = (uint8_t)ch;
        CALL1(f_088215C4, t);
        return;
    }
    if (!same) CALL2(f_0882196C, t, *W(ANIMS + 20));
    *B(t + T_SHOWS) = (uint8_t)ch;
    CALL1(f_088215C4, t);
}

/* 0x088A7918: a team member's mark starts (which < 3: member which's) */
static void member_start(Cpu *c, gaddr t, gaddr p) {
    if (player_up(p)) {
        step_up(t, T_STATE);
        *W(t + T_DRAW_148) = 0;
        *H(t + T_DRAW_86) = 0xB;
        *W(t + T_DRAW_164) = 1;
        CALL2(f_088A8664, t, 0);
        int16_t x = S8(p + F_SLOT) ? 0x130 : 0x50;
        *B(t + T_SHOWS) = 0xFF;
        *H(t + T_X) = (uint16_t)x;
        *H(t + T_Y) = (uint16_t)(0xCC - S16(PORTRAIT_UP + *B(t + T_WHICH) * 2u));
    }
    int16_t w = (int16_t)(*B(t + T_WHICH) & 0xFF);
    if (w < 3) {
        *B(t + T_SHOWN) = 1;
        *B(t + T_PICTURE) = (uint8_t)w;
        CALL2(f_0882196C, t, *W(ANIMS + 20) + (uint32_t)w * 8 + 8);
    } else CALL2(f_0882196C, t, *W(ANIMS + 20));
}

/* 0x088A7A4C: a team member's line: step 0 its mark (lit for the member choosing now), then its name
   and character in the side's list */
static void member_run(Cpu *c, gaddr t, gaddr p) {
    int8_t slot = S8(p + F_SLOT);
    gaddr list = slot ? TEAM_LIST_2 : TEAM_LIST_1;
    int32_t x = slot ? 0x148 : 0x20;
    uint32_t which = *B(t + T_WHICH);
    if (*B(t + T_STEP) == 0) {
        if ((int32_t)which < 3) {
            *B(t + T_PICTURE) = S8(list) == (int32_t)which ? (uint8_t)(int8_t)(slot + 4) : 6;
            if (S16(p + F_DONE) & 0x20) {
                if (S8(list) < (int32_t)which) step_up(t, T_STATE);
                else { *B(t + T_STEP) = (uint8_t)(*B(t + T_STEP) + 1); *B(t + T_SHOWN) = 0; }
            }
        } else {
            int32_t member = (int32_t)which - 3;
            *B(t + T_ALPHA) = S8(list) == member ? 0xFF : 0xC0;
            if ((S16(p + F_DONE) & 0x20) && S8(list) < member) step_up(t, T_STATE);
        }
        CALL1(f_088215C4, t);
        return;
    }
    int32_t w = (int8_t)which;
    int32_t y = (int16_t)(w * 24 + 0x20);
    text0(c, *W(TEAM_NAMES + (uint32_t)w * 4), x, y - 1, 4);
    text0(c, S_TEAM_MARK, x, y, 9);
    if (!(S8(list + 1) < (int32_t)*B(t + T_WHICH))) {
        if (slot == 0) x = (int16_t)(x + 0x10);
        int32_t ch = S8(list + (uint32_t)w + 6);
        text0(c, *W(TEAM_NAMES + (uint32_t)ch * 4 + 16), x, (int16_t)(y + 8), *H(NAME_COLOURS + (uint32_t)ch * 2 + 2));
    }
    int32_t want = S8(list + *B(t + T_WHICH) + 3);
    if ((int32_t)S8(t + T_SHOWS) != want) {
        CALL2(f_0882196C, t, *W(ANIMS + 16) + (uint32_t)want * 8);
        *B(t + T_SHOWS) = (uint8_t)want;
    }
    if (!(S8(list + 1) < (int32_t)*B(t + T_WHICH))) CALL1(f_088215C4, t);
}

/* ---- the Dramatic kinds' cursor frames (by the player's pad slot, + 775) ---- */
#define F_PAD_SLOT   775    /* s8 */

/* 0x088A7CEC: start */
static void dframe_start(Cpu *c, gaddr t, gaddr p) {
    step_up(t, T_STATE);
    *H(t + T_COUNTER) = 0;
    *W(t + T_DRAW_148) = 0;
    *H(t + T_DRAW_86) = 8;
    *B(t + T_SHOWS) = 0xFF;
    *B(t + T_GRID_ROW) = (uint8_t)S16(p + F_ROW);
    *B(t + T_GRID_COL) = (uint8_t)S16(p + F_COLUMN);
    CALL2(f_088A8664, t, *B(t + T_WHICH));
    *B(t + T_SHOWN) = S8(p + F_PAD_SLOT) ? 0xFF : 0;
    int8_t pad = S8(p + F_PAD_SLOT);
    *H(t + T_PAD) = (uint16_t)(int16_t)pad;
    *B(t + T_PICTURE) = (uint8_t)pad;
    *B(t + T_SLOT_COPY) = (uint8_t)pad;
    if (*B(t + T_WHICH)) *B(t + T_ALPHA) = 0x80;
}

/* 0x088A7D94: a frame: the counter, then the step (0x08A2BBAC) */
static void dframe_run(Cpu *c, gaddr t, gaddr p) {
    int16_t n = (int16_t)(S16(t + T_COUNTER) + 0x80);
    *H(t + T_COUNTER) = (uint16_t)(n < 2560 ? n : 0);
    run_step(c, t, p, 0x08A2BBACu);
}

/* 0x088A7DFC: step 0, then (0x088A7E20) on as 0x088A733C does */
static void dframe_step0(Cpu *c, gaddr t, gaddr p) {
    step_up(t, T_STEP);
    c->r[6] = *B(t + T_STEP);
    CALL2(f_088A7E20, t, p);
}

/* 0x088A7E74: choosing (as 0x088A7390, with 0x088A7FD0) */
static void dframe_step1(Cpu *c, gaddr t, gaddr p) {
    if (S8(p + F_CHOSEN) & 1) step_up(t, T_STEP);
    CALL2(f_088A7540, t, p);
    CALL3(f_088A7FD0, t, p, (uint32_t)((int32_t)S16(t + T_COUNTER) >> 8) & 0xFFFF);
    if ((S16(p + F_DONE) & 1) || *W(p + F_CURSOR_ON)) CALL1(f_088215C4, t);
}

/* 0x088A7F00: chosen: blinks (by the pad slot) */
static void dframe_step2(Cpu *c, gaddr t, gaddr p) {
    if (S16(p + F_DONE) & 1) step_up(t, T_STEP);
    CALL3(f_088A7FD0, t, p, 0);
    if ((uint32_t)(*B(GAME + G_FRAME20) + S16(t + T_PAD)) & 1) CALL1(f_088215C4, t);
}

/* 0x088A7F74: done: on to the next state once the speed is set */
static void dframe_step3(Cpu *c, gaddr t, gaddr p) {
    if (S16(GAME + G_FRAME) & 0xFF00) step_up(t, T_STATE);
    CALL3(f_088A7FD0, t, p, 0);
    CALL1(f_088215C4, t);
}

/* 0x088A7FD0: the frame's animation, by pad slot (the second frame: 3 / 4 by frame parity) */
static void dframe_anim(Cpu *c, gaddr t, uint32_t frame) {
    uint32_t kind = (uint32_t)S16(t + T_PAD) & 0xFF;
    if (*B(t + T_WHICH)) kind = (((kind + *B(GAME + G_FRAME20)) & 0xFF & 1) + 3) & 0xFF;
    CALL2(f_0882196C, t, *W(ANIMS + kind * 4 + 32) + (frame & 0xFFFF) * 8);
}

/* ---- the Dramatic kinds' portraits (by layout, side and pad slot) ---- */
#define B_LAYOUT     56     /* s16: 1 / 2 / 3 */
#define DRAMA_PICS_1 0x08A2BBD0u   /* u8 by (layout - 1) * 4 + slot: the portrait */
#define DRAMA_X_1    0x08A2BBDCu   /* s16 by layout * 3 (+ 6 with two players present, - 3 with one) + picture */
#define DRAMA_PICS_2 0x08A2BC10u
#define DRAMA_X_2    0x08A2BC1Cu

static int32_t drama_place(int32_t pic) {
    int32_t layout = S16(BATTLE + B_LAYOUT);
    int32_t k = (int16_t)(S8(GAME + G_PRESENT) == 2 ? layout * 3 + 6 : layout * 3 - 3);
    return k + pic;
}

/* 0x088A8070: the opponent's portrait starts */
static void dportrait_start(Cpu *c, gaddr t, gaddr p) {
    step_up(t, T_STATE);
    *W(t + T_DRAW_148) = 0;
    *H(t + T_DRAW_86) = 0xB;
    *W(t + T_DRAW_164) = 1;
    *H(t + T_COUNTER) = 0;
    CALL2(f_088A8664, t, 0);
    int32_t at = (S16(BATTLE + B_LAYOUT) - 1) * 4 + S8(p + F_SLOT);
    *H(t + T_PAD) = *B(DRAMA_PICS_1 + (uint32_t)at);
    *B(t + T_SHOWS) = 0xFF;
    *H(t + T_X) = (uint16_t)S16(DRAMA_X_1 + (uint32_t)(drama_place(S16(t + T_PAD)) * 2));
    *H(t + T_Y) = 0xB4;
}

/* 0x088A8174: the portrait shows its player's character, while that one chooses or has chosen */
static void dportrait_run(Cpu *c, gaddr t, gaddr p) {
    int8_t ch = S8(p + F_CHARACTER);
    if ((int32_t)S8(t + T_SHOWS) != (int32_t)(uint8_t)ch) {
        CALL2(f_0882196C, t, *W(ANIMS + 16) + (uint32_t)(uint8_t)ch * 8);
        *B(t + T_SHOWS) = (uint8_t)ch;
    }
    if ((S16(p + F_DONE) & 1) || *W(p + F_CURSOR_ON)) CALL1(f_088215C4, t);
}

/* 0x088A8254: a player's marker starts (by pad slot; the second one a pixel higher, half seen) */
static void dmarker_start(Cpu *c, gaddr t, gaddr p) {
    step_up(t, T_STATE);
    *W(t + T_DRAW_148) = 0;
    *H(t + T_DRAW_86) = 8;
    *H(t + T_COUNTER) = 0;
    CALL2(f_088A8664, t, 0);
    int32_t at = (S16(BATTLE + B_LAYOUT) - 1) * 4 + S8(p + F_SLOT);
    *H(t + T_PAD) = *B(DRAMA_PICS_2 + (uint32_t)at);
    *B(t + T_PICTURE) = (uint8_t)S8(p + F_PAD_SLOT);
    *B(t + T_SHOWN) = S8(p + F_PAD_SLOT) ? 0xFF : 0;
    *B(t + T_SHOWS) = 0xFF;
    *H(t + T_X) = (uint16_t)S16(DRAMA_X_2 + (uint32_t)(drama_place(S16(t + T_PAD)) * 2));
    *H(t + T_Y) = 0xB4;
    if (*B(t + T_WHICH)) {
        *B(t + T_ALPHA) = 0x80;
        *H(t + T_Y) = (uint16_t)(S16(t + T_Y) - 1);
    }
}

/* 0x088A8388: the marker: its pad slot's picture, or (the second) by its counter */
static void dmarker_run(Cpu *c, gaddr t, gaddr p) {
    if (*B(t + T_WHICH) == 0) CALL2(f_0882196C, t, *W(ANIMS + 20) + (uint32_t)(S8(p + F_PAD_SLOT) * 8) + 0x20);
    else CALL2(f_0882196C, t, *W(ANIMS + 56) + (uint32_t)(((int32_t)S16(t + T_COUNTER) >> 8) * 8));
    CALL1(f_088215C4, t);
}

/* ---- the panels that blink ---- */
#define P_STATE      128    /* u8: 0x088A8698's table */
#define P_KIND       92     /* u8: which of the fighter's parts it watches (0: none) */
#define P_FIGHTER    124    /* u32 */
#define P_SEEN       140    /* u8: the part's value it was set up for */
#define P_RISING     131    /* u8: the fade's direction */
#define P_FADE       48     /* u8 [4]: the corners' alpha */
#define PANEL_PLACES 0x08A2BC80u   /* s16 [6] by kind */
#define PANEL_TINTS  0x08A2BD64u   /* u8 [3] by mode */

/* 0x088A86CC: a panel: started again when its part changes; then its fade between 0x40 and 0xF0 */
static void panel_run(Cpu *c, gaddr t) {
    gaddr f = *W(t + P_FIGHTER);
    uint32_t kind = *B(t + P_KIND);
    if (f) {
        uint32_t seen = *B(t + P_SEEN), now;
        int32_t k = (int32_t)kind;
        if (k >= 11 && k < 15) now = *B(f + 6);
        else if (k >= 17 && k < 19) now = (uint32_t)((int32_t)S16(f + F_DONE) >> 8) & 0xFF;
        else now = (uint32_t)S16(f + F_DONE) & 0xFF;
        if (now != seen) { CALL1(f_088233BC, t); kind = *B(t + P_KIND); }
    }
    if (kind == 0) return;
    int rising = *B(t + P_RISING);
    int32_t a = (int32_t)*B(t + P_FADE) + (rising ? 4 : -4);
    if (rising ? a >= 241 : a < 64) {
        *B(t + P_RISING) = (uint8_t)(rising ^ 1);
        for (int i = 0; i < 4; i++) *B(t + P_FADE + (uint32_t)i) = rising ? 0xF0 : 0x40;
        return;
    }
    *B(t + P_FADE) = (uint8_t)a;
    for (int i = 1; i < 4; i++) *B(t + P_FADE + (uint32_t)i) = (uint8_t)(*B(t + P_FADE + (uint32_t)i) + (rising ? 4 : -4));
}

/* 0x088A885C: a panel is set up: its places by kind, and (kind 0) the mode's colours, else a fade */
static void panel_setup(gaddr t) {
    step_up(t, P_STATE);
    *B(t + 93) = 1;
    uint32_t kind = *B(t + P_KIND);
    *B(t + 55) = kind ? 7 : 6;
    *B(t + 56) = 1;
    *B(t + 1) = 1;
    *B(t + T_SUB) = 0;
    *B(t + 136) = 0xC0;
    uint32_t i = (kind * 6) & 0xFFFF;
    *H(t + 6) = (uint16_t)S16(PANEL_PLACES + i * 2);
    *H(t + 10) = (uint16_t)S16(PANEL_PLACES + (i + 1) * 2);
    if (kind == 0) {
        gaddr tint = PANEL_TINTS + (uint32_t)(S8(GAME + G_MODE) * 3);
        for (int k = 0; k < 3; k++) {
            uint8_t v = *B(tint + (uint32_t)k);
            gaddr at = t + (k == 0 ? 36u : k == 1 ? 42u : 48u);
            for (int j = 3; j >= 0; j--) *B(at + (uint32_t)j) = v;
        }
    } else {
        for (int j = 3; j >= 0; j--) { *B(t + 42 + (uint32_t)j) = 0; }
        for (int j = 3; j >= 0; j--) { *B(t + 36 + (uint32_t)j) = 0; }
        for (int j = 3; j >= 0; j--) { *B(t + 48 + (uint32_t)j) = 0xF0; }
        *B(t + P_RISING) = 0;
    }
    *H(t + 12) = (uint16_t)S16(PANEL_PLACES + (i + 2) * 2);
    *H(t + 14) = (uint16_t)S16(PANEL_PLACES + (i + 3) * 2);
    *H(t + 24) = (uint16_t)S16(PANEL_PLACES + (i + 4) * 2);
    *H(t + 26) = (uint16_t)S16(PANEL_PLACES + (i + 5) * 2);
}

/* ---- the backdrop's banner ---- */
#define BANNER_SET   0x08A2BFA4u
#define BANNER_ANIM  0x08A2BFA0u   /* u32: its animation */

/* 0x088A8F48: the banner starts, by its sub-state: 0 loads its sprite set (at 0, 0x100), 1 / 2 at
   (0xC0, 0x28) / (0xC0, 0x80) */
static void banner_start(Cpu *c, gaddr t) {
    step_up(t, T_STATE);
    *W(t + T_DRAW_148) = 0;
    *H(t + T_DRAW_86) = 0xE;
    *B(t + T_PICTURE) = 0; *B(t + T_SHOWN) = 0;
    *H(t + 38) = 0; *B(t + 14) = 0; *B(t + 11) = 0;
    *H(t + T_LAYER) = 5;
    uint32_t sub = *B(t + T_SUB);
    if (sub == 0) {
        *B(t + T_KIND) = 1;
        *H(t + T_X) = 0; *H(t + T_Y) = 0x100;
        CALL5(f_0880AEA8, 1, BANNER_SET, 0, 1, 2);
    } else {
        *B(t + T_KIND) = 0;
        *H(t + T_X) = 0xC0; *H(t + T_Y) = sub == 1 ? 0x28 : 0x80;
    }
    CALL2(f_0882196C, t, *W(BANNER_ANIM));
}

SFA_FUNC(088A8664, game_bdisp_reset)        { task_reset(ARG(0), ARG(1)); c->r[5] = 7; c->r[6] = 0; }   /* (a1 / a2 as it leaves them: translated callers) */
SFA_FUNC(088A6FF8, game_bdisp_by_sub)       { SELECT_FRAME(0x20, by_sub(c, ARG(0))); }
SFA_FUNC(088A702C, game_bdisp_by_state)     { SELECT_FRAME(0x20, by_state(c, ARG(0))); }
SFA_FUNC(088A7060, game_bdisp_backdrop)     { SELECT_FRAME(0x20, backdrop_start(c, ARG(0))); }
SFA_FUNC(088A7110, game_bdisp_draw)         { SELECT_FRAME(0x20, CALL1(f_088215C4, ARG(0))); }
SFA_FUNC(088A7164, game_bdisp_frame_start)  { SELECT_FRAME(0x20, frame_start(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7248, game_bdisp_frame_run)    { SELECT_FRAME(0x20, frame_run(c, ARG(0), ARG(1))); }
SFA_FUNC(088A732C, game_bdisp_next_step)    { next_step(ARG(0)); }
SFA_FUNC(088A733C, game_bdisp_frame_step0)  { SELECT_FRAME(0x20, frame_step0(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7390, game_bdisp_frame_step1)  { SELECT_FRAME(0x20, frame_step1(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7420, game_bdisp_frame_step2)  { SELECT_FRAME(0x20, frame_step2(c, ARG(0), ARG(1))); }
SFA_FUNC(088A749C, game_bdisp_frame_step3)  { SELECT_FRAME(0x20, frame_step3(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7540, game_bdisp_frame_follow) { SELECT_FRAME(0x20, frame_follow(c, ARG(0), ARG(1))); }
SFA_FUNC(088A75D0, game_bdisp_frame_place)  { frame_place(ARG(0), ARG(1)); }
SFA_FUNC(088A762C, game_bdisp_frame_anim)   { SELECT_FRAME(0x20, frame_anim(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088A7CEC, game_bdisp_dframe_start) { SELECT_FRAME(0x20, dframe_start(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7D94, game_bdisp_dframe_run)   { SELECT_FRAME(0x20, dframe_run(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7DFC, game_bdisp_dframe_step0) { SELECT_FRAME(0x20, dframe_step0(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7E20, game_bdisp_dframe_on)    { SELECT_FRAME(0x20, frame_step0(c, ARG(0), ARG(1))); }   /* (the same as 0x088A733C) */
SFA_FUNC(088A7E74, game_bdisp_dframe_step1) { SELECT_FRAME(0x20, dframe_step1(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7F00, game_bdisp_dframe_step2) { SELECT_FRAME(0x20, dframe_step2(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7F74, game_bdisp_dframe_step3) { SELECT_FRAME(0x20, dframe_step3(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7FD0, game_bdisp_dframe_anim)  { SELECT_FRAME(0x20, dframe_anim(c, ARG(0), ARG(2))); }
SFA_FUNC(088A76CC, game_bdisp_portrait_start) { SELECT_FRAME(0x20, portrait_start(c, ARG(0), ARG(1))); }
SFA_FUNC(088A781C, game_bdisp_portrait_run)   { SELECT_FRAME(0x20, portrait_run(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7918, game_bdisp_member_start)   { SELECT_FRAME(0x30, member_start(c, ARG(0), ARG(1))); }
SFA_FUNC(088A7A4C, game_bdisp_member_run)     { SELECT_FRAME(0x40, member_run(c, ARG(0), ARG(1))); }
SFA_FUNC(088A8070, game_bdisp_dportrait_start) { SELECT_FRAME(0x20, dportrait_start(c, ARG(0), ARG(1))); }
SFA_FUNC(088A8174, game_bdisp_dportrait_run)   { SELECT_FRAME(0x20, dportrait_run(c, ARG(0), ARG(1))); }
SFA_FUNC(088A8254, game_bdisp_dmarker_start)   { SELECT_FRAME(0x20, dmarker_start(c, ARG(0), ARG(1))); }
SFA_FUNC(088A8388, game_bdisp_dmarker_run)     { SELECT_FRAME(0x20, dmarker_run(c, ARG(0), ARG(1))); }
SFA_FUNC(088A8648, game_bdisp_8648)            { SELECT_FRAME(0x20, f_088231C8(c)); }
SFA_FUNC(088A8698, game_bdisp_panel_by_state)  { SELECT_FRAME(0x20, by_table(c, ARG(0), P_STATE, 0x08A2BD94u)); }
SFA_FUNC(088A86CC, game_bdisp_panel_run)       { SELECT_FRAME(0x20, panel_run(c, ARG(0))); }
SFA_FUNC(088A885C, game_bdisp_panel_setup)     { panel_setup(ARG(0)); }
SFA_FUNC(088A8F14, game_bdisp_banner_by_state) { SELECT_FRAME(0x20, by_table(c, ARG(0), T_STATE, 0x08A2BFE4u)); }
SFA_FUNC(088A8F48, game_bdisp_banner_start)    { SELECT_FRAME(0x30, banner_start(c, ARG(0))); }
