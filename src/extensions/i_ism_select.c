/* I-ISM select (the i_ism_select feature): entry characters (EDIT / ENTRY, shown as I-ISM) for the
 * other side, where a mode lets a second player or the computer have one. The entries' wipe and ranking
 * are i_ism_display.c (the i_ism_display feature).
 *
 * Free Battle's select (and Survival's, Training's ...: battle_select.c) keeps the six entries on row 6
 * of its grid, and the grid's lock (0x08A2B3A9, by row * 7 + column) keeps those boxes from any box
 * whose player isn't present: the box a player picks for the computer could not land on them. Offline
 * the entries are the save's, everyone's, so while a cursor moves (0x088A6A10) row 6 is opened; the
 * lock is put back after. A network session (0x088D5E44) keeps it: there each side has its own. */
#include "rt.h"

#define BGRID_LOCK    0x08A2B3A9u
void f_088D5E44(Cpu *c);
static int8_t saved_lock[7];
int i_ism_select_bselect_grid(Cpu *c, uint32_t a0, int after) {
    (void)a0;
    static int opened;
    if (!after) {
        opened = 0;
        uint32_t s4 = c->r[4], s5 = c->r[5], s6 = c->r[6], s31 = c->r[31];
        f_088D5E44(c);
        int net = c->r[2] != 0;
        c->r[4] = s4; c->r[5] = s5; c->r[6] = s6; c->r[31] = s31;
        if (net) return 0;
        for (uint32_t k = 0; k < 7; k++) { saved_lock[k] = (int8_t)RB(BGRID_LOCK + 42 + k); WB(BGRID_LOCK + 42 + k, 0xFF); }
        opened = 1;
    } else if (opened) {
        for (uint32_t k = 0; k < 7; k++) WB(BGRID_LOCK + 42 + k, (uint8_t)saved_lock[k]);
        opened = 0;
    }
    return 0;
}
