/* The character select screen most modes share (Arcade, the Dramatic and Variable kinds, VS ...):
 * a step of the fight task's screen 1 (0x08894040, a0 the game, a1 BATTLE), which starts the screen
 * once (0x08894138) and then runs it a frame at a time (0x08894214): each player's select cursor, the
 * players' names, and the check that everyone has chosen.
 *
 * Each player is a fighter object (FIGHTER_1 / FIGHTER_2, 0x498 bytes) whose cursor runs only while
 * its +0x338 is set. A cursor is a two-level state machine: +4 picks a state from CURSOR_STATES, and
 * the states that have steps pick one by +5 from their own table. Every state gets (this player, the
 * other player).
 */
#include "select.h"

void rt_call_indirect(Cpu *c, uint32_t addr);

#define NET_TEXT       0x09523980u   /* +35: a network message is up */
#define NET_NAMES      0x09523454u   /* per side 208 bytes: three names of 0x3C bytes (+0 shown) */
#define LOCAL_NAMES    0x09519588u   /* six name slots of 0x354 bytes (+0 shown) */
#define NAME_TEXT      0x08A27148u   /* u32 by slot (local) or 0x08A27160 by side (network): the text */
#define NAME_PLACE     0x08A27178u   /* { x, y } by slot: where it is drawn */
#define NAME_FONT      9

void f_088D7C1C(Cpu *c);   /* -> a continuing (network) match */
void f_08822600(Cpu *c);
void f_08894138(Cpu *c); void f_08894214(Cpu *c);   /* below */
void f_08837690(Cpu *c); void f_0883771C(Cpu *c); void f_0880B6CC(Cpu *c); void f_08819144(Cpu *c);
void f_0887C4CC(Cpu *c);   /* sound effect (0, id, volume) */
void f_088945C0(Cpu *c);   /* below: a player's cursor */
void f_088939AC(Cpu *c);   /* -> the local names are shown */
void f_0880A6CC(Cpu *c);   /* queues text (a0 text, a1, a2 x, a3 y, t0 font) */
void f_088D6378(Cpu *c);
void f_088E348C(Cpu *c);   /* (game, 1) */
void f_088969FC(Cpu *c);   /* (fighter): the choice counted (cursor.c) */

static int is_team_mode(gaddr g) { int8_t m = S8(g + G_MODE); return m == MODE_VARIABLE || m == MODE_DRAMATIC; }

/* ---- the screen ---- */

/* The screen's step (0x08894040): started once, then a frame at a time; the object lists and the end
   of the frame. In a continuing match it holds while GAME + 0x91 halts it. */
static void select_step(Cpu *c, gaddr g, gaddr battle) {
    if (CALL0(f_088D7C1C) != 0) {
        if (S8(g + G_HALT_3) != 0) {
            CALL0(f_0880B6CC);
            CALL0(f_08819144);
            return;
        }
        CALL0(f_08822600);
    }
    CALL2(*H(g + G_STEP) == 0 ? f_08894138 : f_08894214, g, battle);
    CALL0(f_08837690);
    CALL0(f_0883771C);
    CALL0(f_0880B6CC);
    CALL0(f_08819144);
}

/* The screen's start (0x08894138), once BATTLE + 90 is set or the frame counter's low byte is 7: the
   cursors of the players present (GAME + 53: 1, 2 or both) switched on, a sound, the first frame. */
static void select_start(Cpu *c, gaddr g, gaddr battle) {
    if (S8(battle + 90) == 0 && ((int16_t)*H(g + G_FRAME) & 0xFF) != 7) return;
    switch (S8(g + G_PRESENT)) {
    case 2: *W(FIGHTER_2 + F_CURSOR_ON) = 1; *W(FIGHTER_1 + F_CURSOR_ON) = 0; break;
    case 1: *W(FIGHTER_2 + F_CURSOR_ON) = 0; *W(FIGHTER_1 + F_CURSOR_ON) = 1; break;
    case 3: *W(FIGHTER_2 + F_CURSOR_ON) = 1; *W(FIGHTER_1 + F_CURSOR_ON) = 1; break;
    default: break;
    }
    *H(g + G_STEP) = (uint16_t)(*H(g + G_STEP) + 1);
    CALL3(f_0887C4CC, 0, 0x124, 0xC0);
    CALL2(f_08894214, g, battle);
}

static void draw_name(Cpu *c, gaddr text, gaddr place) {
    CALL5(f_0880A6CC, text, 0, *W(place), *W(place + 4), NAME_FONT);
}

/* The players' names, blinking (shown on frames where the counter's & 3 is set): a network match's
   three per side, unless a message or the lobby is up; offline, the six slots in use (not in World
   Tour). */
static void draw_names(Cpu *c, gaddr g) {
    if (CALL0(f_088D7C1C) != 0) {
        if (*B(NET_TEXT + 35) != 0 || S8(g + G_NET_LOBBY) != 0) return;
        for (uint32_t side = 0; side < 2; side++)
            for (uint32_t k = 0; k < 3; k++) {
                if (S8(NET_NAMES + side * 208 + k * 0x3C) == 0) continue;
                if (((int16_t)*H(g + G_FRAME) & 3) == 0) continue;
                draw_name(c, *W(NAME_TEXT + 24 + side * 4), NAME_PLACE + (k * 2 + side) * 8);
            }
        return;
    }
    if (CALL0(f_088939AC) == 0) return;
    for (uint32_t k = 0; k < 6; k++) {
        if (S8(LOCAL_NAMES + k * 0x354) == 0) continue;
        if (S8(g + G_MODE) == MODE_WORLD_TOUR) continue;
        if (((int16_t)*H(g + G_FRAME) & 3) == 0) continue;
        draw_name(c, *W(NAME_TEXT + k * 4), NAME_PLACE + k * 8);
    }
}

/* A frame of the screen (0x08894214): both cursors (player 2's first when BATTLE + 90), the names, and
   once the players who fight or chose (GAME + 57 | + 41) are all done (GAME + 10): on to the next step,
   with the team modes' players set from those present. selectscreen.c wraps it (JOIN / WAIT). */
static void select_frame(Cpu *c, gaddr g, gaddr battle) {
    gaddr first = FIGHTER_1, second = FIGHTER_2;
    if (S8(battle + 90) != 0) { first = FIGHTER_2; second = FIGHTER_1; }
    CALL2(f_088945C0, first, second);
    CALL2(f_088945C0, second, first);
    draw_names(c, g);
    if ((uint32_t)(int32_t)(S8(g + G_FIGHTS) | S8(g + G_CHOSE)) != *H(g + G_DONE)) return;
    *H(g + G_STEP) = 0;
    *H(g + G_SUB) = (uint16_t)(*H(g + G_SUB) + 1);
    CALL0(f_088D6378);
    CALL2(f_088E348C, g, 1);
    *B(g + 43) = 1;
    *B(g + 72) = 0xFF;
    *H(g + G_FRAME) = (uint16_t)((int16_t)*H(g + G_FRAME) | 0x100);
    if (is_team_mode(g)) {
        *B(g + G_FIGHTS) = *B(g + G_PRESENT);
        *B(g + G_CHOSE) = *B(g + G_PRESENT);
    }
}

/* ---- the cursors (their states' steps are in cursor.c) ---- */

/* A player's cursor (0x088945C0): its state's handler, while +0x338 is on. */
static void cursor(Cpu *c, gaddr me, gaddr other) {
    if (*W(me + F_CURSOR_ON) == 0) return;
    gaddr fn = *W(CURSOR_STATES + *B(me + F_STATE) * 4u);
    c->r[4] = me; c->r[5] = other; c->r[6] = other; c->r[7] = fn;   /* as the original leaves them */
    rt_call_indirect(c, fn);
}

/* A state's step by +5, from its own table (the four step dispatchers below) */
static void cursor_step(Cpu *c, gaddr table) {
    gaddr me = c->r[4];
    gaddr fn = *W(table + *B(me + F_STEP) * 4u);
    c->r[6] = fn; c->r[7] = table;
    rt_call_indirect(c, fn);
}

/* State 7 (0x0889494C): in the team modes the cursor goes over to the partner (GAME + 41 = both);
   either way, the next step. */
static void cursor_hand_over(gaddr me, gaddr other) {
    if (is_team_mode(GAME)) {
        *W(me + F_CURSOR_ON) = 0;
        *W(other + F_CURSOR_ON) = 1;
        *B(GAME + G_CHOSE) = 3;
    }
    *B(me + F_STEP) = (uint8_t)(*B(me + F_STEP) + 1);
}

/* State 4 (0x088954E0): the player has finished choosing, once: its bit in GAME + 10 and (if not
   yet) GAME + 57, BATTLE + 0 its character, the cursor to the other player, and the choice counted. */
static void cursor_done(Cpu *c, gaddr me, gaddr other) {
    uint16_t bit = (uint16_t)(1u << (S8(me + F_SLOT) & 31));
    uint16_t done = *H(GAME + G_DONE);
    if (done & bit) return;
    *H(GAME + G_DONE) = (uint16_t)(done | bit);
    int8_t fights = S8(GAME + G_FIGHTS);
    if (fights & bit) return;
    *B(GAME + G_FIGHTS) = (uint8_t)(fights | bit);
    *H(BATTLE + 0) = (uint16_t)(int16_t)S8(me + F_CHARACTER);
    *W(me + F_CURSOR_ON) = 0;
    *W(other + F_CURSOR_ON) = 1;
    CALL1(f_088969FC, me);
}

#define FRAME SELECT_FRAME
SFA_FUNC(08894040, game_select_step)       { FRAME(0x20, select_step(c, ARG(0), ARG(1))); }
SFA_FUNC(08894138, game_select_start)      { FRAME(0x20, select_start(c, ARG(0), ARG(1))); }
SFA_FUNC_WRAPPED(08894214, game_select_frame, selectscreen_frame) { FRAME(0x50, select_frame(c, ARG(0), ARG(1))); }
SFA_FUNC(088945C0, game_select_cursor)     { FRAME(0x20, cursor(c, ARG(0), ARG(1))); }
SFA_FUNC(08894610, game_select_state_0)    { FRAME(0x20, cursor_step(c, 0x08A271ECu)); }
SFA_FUNC(08894F8C, game_select_state_1)    { FRAME(0x20, cursor_step(c, 0x08A27264u)); }
SFA_FUNC(08894C40, game_select_state_2)    { FRAME(0x20, cursor_step(c, 0x08A27258u)); }
SFA_FUNC(08895304, game_select_state_3)    { FRAME(0x20, cursor_step(c, 0x08A27270u)); }
SFA_FUNC(0889494C, game_select_hand_over)  { cursor_hand_over(ARG(0), ARG(1)); }
SFA_FUNC(088954E0, game_select_done)       { FRAME(0x20, cursor_done(c, ARG(0), ARG(1))); }
