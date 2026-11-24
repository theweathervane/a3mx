/* The attract mode's ranking screen (after the third demo): its start.
 *
 * The rankings are in the system data at 0x08A05AA8: eight tables of 38 records (0x2F8 bytes each),
 * 0x14 bytes a record: +0 the score (or wins), +4 the name (three letters), +8 the character, +9 its
 * colour, +14 the table's kind (0 - 2 X-, A-, V-ISM scores, 3 - 5 their VS wins), +15 a new record.
 * Tables 0 - 5 are those kinds; 6 and 7 are the entries' (I-ISM) score and VS wins: the record writer
 * (0x08892A98) puts an entry's run there (kind 2 and 5 in its records, V-ISM's look), but the attract
 * mode only shows tables 0 - 2 (0x09358400, the attract cycle, title.c).
 *
 * 0x088BC164 (the screen's step, a0 the task): the table of the attract cycle becomes the screen's
 * (0x09525DE8), the fill and the objects reset, the background layer 0x092E8800 set, and the palettes
 * loaded: the first place's portrait (its character's 0x80 colours from 0x09336B80 by its colour +9;
 * for characters 0x14, 0x1C and 0x29 sixteen more from 0x09337240) and the screen's own five, by the
 * kind's ISM (+14 % 3: X, A, V), from 0x09335A00 on. Wrapped by extensions/i_ism_display.c, which gives an I-ISM
 * table its own look.
 */
#include "../decomp.h"

#include "../guest_mem.h"

void f_0880A39C(Cpu *c);   /* 0x2800 bytes at 0x092DCA40 set to 0xFF */
void f_08821FEC(Cpu *c);   /* the object lists reset */
void f_089E11C8(Cpu *c);   /* (to, from, count): colours copied */
void f_0881CE4C(Cpu *c);   /* the screen's backdrop (a0, a1) */
void f_0887C6E4(Cpu *c);   /* a sound */

#define CYCLE        0x09358400u   /* s8: the attract cycle */
#define TABLES       0x08A05AA8u
#define TABLE_SIZE   0x2F8u
#define SCREEN_TABLE 0x09525DE8u   /* the table shown */
#define PALETTES     0x092DF980u   /* 16 colours (u16) each */
#define PALETTES_DIRTY 0x092E0980u
#define LAYER        0x092E8800u
#define PORTRAITS    0x09336B80u   /* 0x80 colours by colour */
#define PORTRAITS_2  0x09337240u   /* 16 more for characters 0x14, 0x1C, 0x29 */
#define SCREEN_COLS  0x09335A00u

static void colours(Cpu *c, gaddr to, gaddr from, uint32_t n) { CALL3(f_089E11C8, to, from, n); }

static void ranking_start(Cpu *c, gaddr task) {
    *H(task + 4) = (uint16_t)(*H(task + 4) + 1);
    gaddr t = TABLES + (uint32_t)(int32_t)S8(CYCLE) * TABLE_SIZE;
    *W(SCREEN_TABLE) = t;
    c->r[4] = t;   /* left in a0 as the original leaves it */
    f_0880A39C(c);
    f_08821FEC(c);
    *H(LAYER + 18) = 0;
    *H(LAYER + 22) = 0x16;
    *W(LAYER + 132) = 0x8000;
    *W(LAYER + 136) = 0x20000;
    gaddr first = *W(SCREEN_TABLE);
    colours(c, PALETTES, PORTRAITS + ((uint32_t)(int32_t)S8(first + 9) << 8), 0x80);
    colours(c, PALETTES + 0x1A0, 0x09337180u + ((uint32_t)(int32_t)S8(*W(SCREEN_TABLE) + 9) << 5), 0x10);
    colours(c, PALETTES + 0x2E0, 0x09336B60u, 0x10);
    colours(c, PALETTES + 0x2A0, 0x09335B80u, 0x10);
    int8_t ch = S8(*W(SCREEN_TABLE) + 8);
    if (ch == 0x1C || ch == 0x14 || ch == 0x29)
        colours(c, PALETTES + 0x2C0, PORTRAITS_2 + ((uint32_t)(int32_t)S8(*W(SCREEN_TABLE) + 9) << 5), 0x10);
    DIV(c, (uint32_t)(int32_t)S8(*W(SCREEN_TABLE) + 14), 3);
    int32_t ism = (int32_t)c->hi;
    if (ism == 0) {
        colours(c, PALETTES + 0x100, SCREEN_COLS + 0xC0, 0x10);
        colours(c, PALETTES + 0x120, SCREEN_COLS, 0x10);
        colours(c, PALETTES + 0x140, SCREEN_COLS + 0xE0, 0x10);
        colours(c, PALETTES + 0x160, SCREEN_COLS + 0x20, 0x20);
        colours(c, PALETTES + 0x280, SCREEN_COLS + 0x120, 0x10);
    } else if (ism == 1) {
        colours(c, PALETTES + 0x100, SCREEN_COLS, 0x10);
        colours(c, PALETTES + 0x120, SCREEN_COLS + 0x60, 0x10);
        colours(c, PALETTES + 0x140, SCREEN_COLS + 0x20, 0x10);
        colours(c, PALETTES + 0x160, SCREEN_COLS + 0x80, 0x20);
        colours(c, PALETTES + 0x280, SCREEN_COLS + 0x140, 0x10);
    } else if (ism == 2) {
        colours(c, PALETTES + 0x100, SCREEN_COLS + 0x60, 0x10);
        colours(c, PALETTES + 0x120, SCREEN_COLS + 0xC0, 0x10);
        colours(c, PALETTES + 0x140, SCREEN_COLS + 0x80, 0x10);
        colours(c, PALETTES + 0x160, SCREEN_COLS + 0xE0, 0x20);
        colours(c, PALETTES + 0x280, SCREEN_COLS + 0x160, 0x10);
    }
    *W(PALETTES_DIRTY) = 1;
    CALL2(f_0881CE4C, 0, 1);
    CALL1(f_0887C6E4, 0x107);
}

SFA_FUNC_WRAPPED(088BC164, ranking_start_step, i_ism_display_ranking_start) {
    gaddr task = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    ranking_start(c, task);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}
