/* The select screens' display tasks (the boxes, portraits, sprites, name strips, the ISM panel ...):
 * each kind of task runs through one entry (a0 = the task, its player at +0x5C) that calls the task's
 * state handler from the kind's table by the task's +4 (docs/PSP_PROGRAM_MAP.md). selectscreen.c
 * wraps each entry: offline in Dramatic Battle with both players present, each player is drawn on its
 * own side.
 *
 * Decompiled here: the thirteen kinds the scenarios reach (the character select screen's and the
 * Survival / Free Battle select's, its TEAM and Dramatic kinds included). 0x088A81FC first steps a
 * counter at +82 (by 0x80, back to 0 at 2560). Still hooked, until a scenario reaches it: 0x088A8404,
 * which steps the same counter.
 */
#include "select.h"

void rt_call_indirect(Cpu *c, uint32_t addr);

#define T_STATE   4      /* u8: the task's state */
#define T_PLAYER  92     /* u32: the player (fighter) it shows */
#define T_COUNTER 82     /* s16: 0x088A81FC's counter */

/* A task's state handler (task, player), the registers as the original leaves them. */
static void run_task(Cpu *c, gaddr task, gaddr table) {
    gaddr fn = *W(table + *B(task + T_STATE) * 4u);
    gaddr player = *W(task + T_PLAYER);
    c->r[4] = task; c->r[5] = player; c->r[6] = player; c->r[7] = fn;
    rt_call_indirect(c, fn);
}

/* One line each (the translator finds SFA_FUNC lines, not macros that expand to them). */
#define KIND(table) SELECT_FRAME(0x20, run_task(c, ARG(0), table))
SFA_FUNC_WRAPPED(08899558, game_select_task_08899558, selectscreen_own_side) { KIND(0x08A2A8A8u); }
SFA_FUNC_WRAPPED(08899A80, game_select_task_08899A80, selectscreen_own_side) { KIND(0x08A2A8CCu); }
SFA_FUNC_WRAPPED(0889A7C8, game_select_task_0889A7C8, selectscreen_own_side) { KIND(0x08A2A994u); }   /* sprite; also the ISM banner */
SFA_FUNC_WRAPPED(0889B048, game_select_task_0889B048, selectscreen_own_side) { KIND(0x08A2A9F8u); }   /* portrait; also the ISM column */
SFA_FUNC_WRAPPED(0889BAB0, game_select_task_0889BAB0, selectscreen_own_side) { KIND(0x08A2AA94u); }
SFA_FUNC_WRAPPED(0889BF30, game_select_task_0889BF30, selectscreen_own_side) { KIND(0x08A2AAD4u); }   /* name strip; also the ISM banner */
SFA_FUNC_WRAPPED(0889E434, game_select_task_0889E434, selectscreen_own_side) { KIND(0x08A2B0A0u); }
SFA_FUNC_WRAPPED(088A712C, game_select_task_088A712C, selectscreen_own_side) { KIND(0x08A2BB08u); }
SFA_FUNC_WRAPPED(088A7694, game_select_task_088A7694, selectscreen_own_side) { KIND(0x08A2BB4Cu); }
SFA_FUNC_WRAPPED(088A78E0, game_select_task_088A78E0, selectscreen_own_side) { KIND(0x08A2BB5Cu); }   /* Free Battle TEAM */
SFA_FUNC_WRAPPED(088A7CB4, game_select_task_088A7CB4, selectscreen_own_side) { KIND(0x08A2BB9Cu); }   /* Free Battle's Dramatic kinds */
SFA_FUNC_WRAPPED(088A8038, game_select_task_088A8038, selectscreen_own_side) { KIND(0x08A2BBC0u); }   /* Free Battle's Dramatic kinds */
SFA_FUNC_WRAPPED(088A81FC, game_select_task_088A81FC, selectscreen_own_side) {                       /* Free Battle's Dramatic kinds */
    gaddr task = ARG(0);
    int16_t n = (int16_t)(S16(task + T_COUNTER) + 0x80);
    *H(task + T_COUNTER) = n < 2560 ? (uint16_t)n : 0;
    KIND(0x08A2BC00u);
}
