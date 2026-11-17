/* The network task's menu rows (0x088EC074 - 0x088EC724, one per row 1 - 11, called through a table):
 * each draws its row's text, lit if it is the chosen row, and the chosen row's cursor bar.
 *
 * WIDE FORCED: the bar's x is -240 instead of -192, so it stays centred in the 16:9 picture; that was
 * the 16:9 patch's (Rickow, sfa3-PSP-widescreen, MIT) word in each of the eleven, here native and
 * chosen by rt_widescreen as the patch's translation chooses it (docs/WIDESCREEN.md).
 *
 * network_match reaches eight of them; the rows with ids 0, 9 and 10 belong to menus no scenario opens.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE FORCED (extensions/widescreen.c; extensions_off.c: 0) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_0880A6CC(Cpu *c);   /* draws a text: a0 the text, a1, a2 its size, a3 its row's number, t0 its look (0xA plain, 0xB lit) */
void f_088D6D74(Cpu *c);   /* the cursor bar: a0, a1 the row's number, a2 its width, a3, t0 x, t1 */

#define NET        0x09523980u    /* the network task's state: +46 the chosen row, +47 the menu */
#define ROW_NUMBERS 0x08A3EA34u   /* u8[11] per menu: each row's number, by row id */
#define ROW_TEXTS   0x08A3F75Cu   /* u32[11]: each row's text */
#define BAR_X       0xFF40u       /* -192 as the game passes it (ori: not sign-extended) */
#define BAR_X_WIDE  0xFF10u       /* -240 */

/* A row: its id picks its number in the menu's table and is what the chosen row (+46) is compared
   with; its text is the next in ROW_TEXTS (the ids go 1 - 4, 7, 8, 0, 5, 6, 9, 10 in address order). */
static void menu_row(Cpu *c, int id, int text_n) {
    int32_t menu = (int8_t)*B(NET + 47);
    uint8_t number = *B(ROW_NUMBERS + (uint32_t)(menu * 11) + (uint32_t)id);
    gaddr text = *W(ROW_TEXTS + (uint32_t)text_n * 4u);
    if ((int8_t)*B(NET + 46) != id) { CALL5(f_0880A6CC, text, 1, 0x20, number, 0xA); return; }
    CALL5(f_0880A6CC, text, 1, 0x20, number, 0xB);
    c->r[9] = 0xE;                                       /* the sixth argument */
    CALL5(f_088D6D74, 0x1E, number, 0x140, 0x12, rt_widescreen ? BAR_X_WIDE : BAR_X);
}

static void row(Cpu *c, int id, int text_n) { c->r[29] -= 0x30; menu_row(c, id, text_n); c->r[29] += 0x30; }

SFA_FUNC(088EC074, net_menu_row_0) { row(c, 1, 0); }
SFA_FUNC(088EC11C, net_menu_row_1) { row(c, 2, 1); }
SFA_FUNC(088EC1C8, net_menu_row_2) { row(c, 3, 2); }
SFA_FUNC(088EC274, net_menu_row_3) { row(c, 4, 3); }
SFA_FUNC(088EC320, net_menu_row_4) { row(c, 7, 4); }
SFA_FUNC(088EC3CC, net_menu_row_5) { row(c, 8, 5); }
SFA_FUNC(088EC478, net_menu_row_6) { row(c, 0, 6); }
SFA_FUNC(088EC520, net_menu_row_7) { row(c, 5, 7); }
SFA_FUNC(088EC5CC, net_menu_row_8) { row(c, 6, 8); }
SFA_FUNC(088EC678, net_menu_row_9) { row(c, 9, 9); }
SFA_FUNC(088EC724, net_menu_row_10) { row(c, 10, 10); }
