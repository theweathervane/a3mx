/* The character select screen's memory: shared by screen.c (the screen) and cursor.c (each player's
 * cursor). Each player is a fighter object (0x498 bytes) that holds the cursor's state while the
 * screen runs; the fields below are the ones the screen uses (more in docs/PSP_PROGRAM_MAP.md).
 */
#pragma once
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }

#define GAME           0x08E75910u
#define BATTLE         0x092E1228u
#define SYS            0x08A058A0u   /* system data (the save's options and records) */
#define FIGHTER_1      0x092E3EA0u
#define FIGHTER_2      0x092E4338u
#define FIGHTER_3      0x092E47D0u
#define FIGHTER_4      0x092E4C68u

/* GAME fields */
#define G_SUB          4      /* u16: the screen's step in the fight task */
#define G_STEP         6      /* u16: 0 until the screen has started */
#define G_DONE         10     /* u16: bit per player who has finished choosing */
#define G_SPEED        20     /* u16: the speed choice (low byte: 0 / 1) and, high byte, set once shown */
#define G_FRAME        22     /* s16: frame counter (names blink on & 3) */
#define G_CHOSE        41     /* s8: players who chose */
#define G_PRESENT      53     /* s8: players present, bit per side */
#define G_FIGHTS       57     /* s8: players who fight */
#define G_MODE         114    /* s8 */
#define G_SPEED_BASE   118    /* s8: added to the speed choice to index SPEEDS */
#define G_NET_ENTRY    144    /* s8: the entry a network player chose */
#define G_HALT_3       145    /* s8 */
#define G_NET          146    /* s8: a network session is on */
#define G_BARRED       148    /* u32[2]: characters that can't be picked (bit per character) */
#define G_NET_LOBBY    184    /* s8 */
#define G_NET_RULE     185    /* s8: 3 = every network player's ISM is decided for them */
#define MODE_WORLD_TOUR 3
#define MODE_DRAMATIC   8
#define MODE_VARIABLE   15

/* BATTLE fields */
#define BT_SPEED       22     /* u8: the game speed chosen */
#define BT_P2_FIRST    90     /* s8: player 2's cursor runs first (and, network, decides for both) */

/* SYS fields */
#define SYS_UNLOCKS    82     /* u16: bit 0 unlocks the select screen's SELECT extras */
#define SYS_CHOICES    174    /* u16 by character × 8 + ISM × 2 (ISM -1 to 2): how often each was picked */

/* fighter fields while the screen runs (the cursor's own state) */
#define F_STATE        4      /* u8: the cursor's state (CURSOR_STATES) */
#define F_STEP         5      /* u8: the state's step */
#define F_COLUMN       18     /* s16: the cursor's box on the grid, 0-6 */
#define F_ROW          22     /* s16: 0-6 */
#define F_CHOSEN       78     /* s8: what is chosen so far (CHOSE_*), read by the other player */
#define F_RANDOM_FLAG  80     /* s8 */
#define F_RANDOM_BOX   81     /* s8: on a random box, its kind + 0x2A (0 otherwise) */
#define F_TIMER        82     /* s16: frames left in a step (or, network, before it decides) */
#define F_RANDOM_HIGH  126    /* s8: bit 0x80 of the box kept in F_BOX */
#define F_ENTRY_BOX    127    /* s8: on one of the six entries' boxes, 1-6 (0 otherwise) */
#define F_SLOT         337    /* s8: the player's slot (bit 1 << slot) */
#define F_CHARACTER    338    /* s8 */
#define F_STYLE_4_ON   373    /* s8: set to -1 by style 4 */
#define F_COLOUR       376    /* s8: the button that chose the character (0-5): its colours */
#define F_STYLE_2_ON   385    /* s8: set to 1 by style 2 (SAIKYO) */
#define F_ISM          386    /* s8: -1, 0, 1 (the three ISMs) */
#define F_KEPT_400     400    /* u32, with +404 (u16) and +411 (u8): cleared when the character changes */
#define F_DONE         424    /* s16: the parts done (DONE_*) */
#define F_STYLE_1_ON   428    /* s8: set to 1 by style 1 (MAZI) */
#define F_BOX          429    /* s8: the box to come back to (F_RANDOM_BOX | its high bit; entries + 0x2D) */
#define F_ISM_FIXED    431    /* s8: set to 1 by style 3 (CLASSICAL; and a network rule): the ISM is decided, not chosen */
#define F_STYLE        438    /* s8: the play style in the screen's order: 0 NORMAL, 1 MAZI, 2 SAIKYO, 3 CLASSICAL;
                                 4 has a flag but the screen never offers it (cursor.c, styles_offered) */
#define F_FINAL_BISON  439    /* s8: M. Bison chosen with SELECT held (where allowed) */
#define F_LAST_CHARACTER 448  /* s8: the character chosen last time */
#define F_COLOUR_COPY  449    /* s8 */
#define F_LAST_ISM     450    /* s8: the ISM chosen last time (offered again with the same character) */
#define F_CURSOR_ON    824    /* u32: the cursor runs */
#define F_NET_ENTRY    1173   /* s8: the network entry chosen + 1 */

/* F_CHOSEN bits (set as each choice is made) and F_DONE bits (set once its step is over): the same values */
#define CHOSE_CHARACTER 1
#define CHOSE_ISM       2
#define CHOSE_STYLE     4
#define CHOSE_SPEED     8
#define DONE_CHARACTER  1
#define DONE_ISM        2
#define DONE_STYLE      4
#define DONE_SPEED      8

/* the cursor's states (fighter + 4), by CURSOR_STATES: 0-3 run their steps (+5) from a table of their
   own; 4 is the end; 5-8 are state 0's steps, reachable directly */
#define CURSOR_STATES  0x08A271D8u
#define STEPS_CHARACTER 0x08A271ECu   /* state 0: enter, move, hand over, wait */
#define STEPS_STYLE    0x08A27264u    /* state 1: enter, choose, wait */
#define STEPS_ISM      0x08A27258u    /* state 2: enter, choose, wait */
#define STEPS_SPEED    0x08A27270u    /* state 3: enter, choose, wait */

/* Chun-Li and Sodom have an X-ISM character of their own: the one for the ISM (-1 if no change). */
static inline int32_t x_ism_character(int32_t ch, int32_t ism) {
    if (ism < 0) { if (ch == 4) return 0x12; if (ch == 6) return 0x14; }
    else         { if (ch == 0x12) return 4; if (ch == 0x14) return 6; }
    return -1;
}

#define SELECT_FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
