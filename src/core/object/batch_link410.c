/* The link play's lobby: its list of players drawn. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define NETP 0x09523980u
#define LIST 0x09523720u /* the list's state: +0 its first entry, +8 its lock, +12 the cursor,
                            +14 how many there are, +16 a count of frames, +18 the first one
                            shown */

void f_088D591C(Cpu *c); void f_0880E788(Cpu *c); void f_0880A7B0(Cpu *c); void f_088DDB68(Cpu *c);
void f_089F31AC(Cpu *c); void f_0880A6CC(Cpu *c); void f_088DDC30(Cpu *c); void f_088DC948(Cpu *c);
void f_088D595C(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

/* A box from (x0, y0) to (x1, y1) in the colour given. */
static void box(Cpu *c, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t colour, uint32_t back) {
    ARG(0) = x0; ARG(1) = y0; ARG(2) = x1; ARG(3) = y1; c->r[8] = colour; c->r[9] = 0xE;
    c->r[31] = back; f_0880E788(c);
}
/* An entry's name (its +4) on the row at y, in the style given. */
static void name(Cpu *c, gaddr entry_name, uint32_t y, uint32_t style, uint32_t back) {
    ARG(0) = entry_name; ARG(1) = 0x70; ARG(2) = y; ARG(3) = style; c->r[8] = 0x10;
    c->r[31] = back; f_088DDC30(c);
}
/* An entry that is skipped has what hangs from its +148 let go. */
static void entry_release(Cpu *c, gaddr entry, uint32_t back) {
    ARG(0) = *W(entry + 148);
    if (ARG(0) != 0) {
        c->r[31] = back; f_088DDB68(c);
        *W(entry + 148) = 0;
    }
}

/* The lobby's list is drawn: a frame of as many rows as the routine at 0x088D591C says less one
   (lower down for three), each row's box, then up to seven entries from the first one shown,
   which follows the cursor (never more than six above it). Each entry by its kind (+154): 0 a
   player to pick, drawn brighter with the cursor's box (pulsing with the game's frame count)
   when the cursor is on it, and in its "ready" style when its +152 is above 0 and its +144 is
   1; 1 nothing; anything else a seat's label ("1P", "2P"..., unless the link's +43 is 4) with
   its name. A cursor on what cannot be picked moves on by itself (up, or down from the top)
   once the count at +16 runs out. Arrows show when there is more above or below. Last, a small
   box with how many players there are of how many the room takes. The list's lock is held
   while its entries are read. A 0x60 frame keeping s0..s8, with a count at +32, the cursor's
   colour at +40 and the move asked for at +48. */
SFA_FUNC(088DDDF0, lobby_list_draw) {
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x60;
    gaddr sp = c->r[29];
    *W(sp + 80) = sv[6];
    c->r[22] = LIST;
    *W(sp + 32) = 0;
    *W(sp + 56) = sv[0]; *W(sp + 60) = sv[1]; *W(sp + 64) = sv[2]; *W(sp + 68) = sv[3];
    *W(sp + 72) = sv[4]; *W(sp + 76) = sv[5]; *W(sp + 84) = sv[7]; *W(sp + 88) = sv[8];
    *W(sp + 92) = sv[9];
    c->r[31] = 0x088DDE30u; f_088D591C(c);
    int32_t rows = (int32_t)c->r[2] - 1;
    c->r[20] = (uint32_t)rows;
    c->r[19] = 0x68;
    c->r[21] = 2;
    c->r[18] = 0xC0000026u;
    *W(sp + 52) = 0x08A3F20Cu;
    uint32_t top_y = rows == 2 ? 0x60 : 0x2E;
    c->r[16] = top_y;
    c->r[17] = top_y;
    int32_t pulse = S8(GAME + 32);
    if (pulse < 0) pulse = (int8_t)(0 - pulse);
    *W(sp + 40) = 0x7A802880u + ((uint32_t)pulse << 24);
    c->r[23] = top_y + (uint32_t)rows * 24;
    box(c, 0x62, top_y - 0x10, 0x11A, c->r[23] - 0xE, 0x2040A0FFu, 0x088DDECCu);
    box(c, 0x5E, top_y - 0x14, 0x116, c->r[23] - 0x11, 0xC0000026u, 0x088DDEE8u);
    c->r[18] = 0x3A010000u;
    if (rows > 0) {
        c->r[18] = 0x3A0080FFu;
        for (c->r[16] = (uint32_t)rows; (int32_t)c->r[16] > 0; c->r[16]--, c->r[17] += 0x18)
            box(c, 0x68, c->r[17], 0x10E, c->r[17] + 4, c->r[18], 0x088DDF14u);
    }
    int32_t cursor = S16(LIST + 12), first = S16(LIST + 18);
    uint32_t y = rows == 2 ? 0x61 : 0x2F;
    *W(sp + 36) = 0x6A;
    *W(sp + 48) = 0;
    c->r[20] = y;
    if (cursor < first) { *H(LIST + 18) = (uint16_t)cursor; first = S16(LIST + 18); }
    else if (first + 6 < cursor) { *H(LIST + 18) = (uint16_t)(cursor - 6); first = S16(LIST + 18); }
    if (first < 0) { *H(LIST + 18) = 0; first = S16(LIST + 18); }
    if (first != 0) {
        ARG(2) = (uint32_t)(int16_t)(y - 0x15); ARG(1) = 0x114; ARG(0) = 5;
        c->r[31] = 0x088DDFA0u; f_0880A7B0(c);
    }
    if (*W(LIST + 8) != 0) {
        ARG(0) = *W(LIST + 8); ARG(1) = 1; ARG(2) = 0;
        c->r[31] = 0x088DDFB4u; hle_sceKernelWaitSema(c);
    }
    gaddr entry = *W(LIST);
    c->r[17] = entry;
    for (c->r[16] = (uint32_t)S16(LIST + 18); entry != 0 && (int32_t)c->r[16] > 0 && *W(entry) != 0; c->r[16]--) {
        entry_release(c, entry, 0x088DDFECu);
        entry = *W(entry);
        c->r[17] = entry;
    }
    c->r[21] = *W(sp + 52);
    c->r[23] = NETP;
    c->r[19] = 0;
    c->r[30] = 0x1E;
    *W(sp + 44) = 0x08A012E4u;
    while (entry != 0 && (int32_t)c->r[19] < 7) {
        c->r[8] = *W(sp + 40);
        int32_t kind = S8(entry + 154);
        int on_cursor = S16(LIST + 12) == (int32_t)c->r[19] + S16(LIST + 18);
        uint32_t row_y = (uint32_t)(int16_t)(c->r[20] - 9);
        if (kind == 0) {
            c->r[16] = row_y;
            c->r[18] = entry + 4;
            c->r[6] = 1;
            if (S16(entry + 152) > 0 && *W(entry + 144) == 1) {
                if (on_cursor) {
                    *H(LIST + 16) = 0x1E;
                    box(c, *W(sp + 36), c->r[20], 0x111, c->r[20] + 4, c->r[8], 0x088DE118u);
                    name(c, entry + 4, row_y, 0xA, 0x088DE130u);
                } else {
                    name(c, entry + 4, row_y, 0, 0x088DE150u);
                }
            } else {
                if (on_cursor) {
                    int32_t wait = S16(LIST + 16);
                    if (wait != 0) *H(LIST + 16) = (uint16_t)(wait - 1);
                    else *W(sp + 48) = S16(LIST + 12) != 0 ? 0xFFFFFFFFu : 1;
                    box(c, *W(sp + 36), c->r[20], 0x111, c->r[20] + 4, c->r[8], 0x088DE19Cu);
                    name(c, entry + 4, row_y, 9, 0x088DE1B4u);
                } else {
                    name(c, entry + 4, row_y, 9, 0x088DE1E8u);
                }
                if (*B(NETP + 44) == 1) *W(sp + 32) = *W(sp + 32) + 1;
            }
        } else {
            if (kind != 1) {
                c->r[16] = row_y;
                if (*B(NETP + 43) != 4) {
                    ARG(2) = c->r[19] - *W(sp + 32) + 2;
                    ARG(0) = *W(c->r[21]);
                    ARG(1) = *W(sp + 44);
                    c->r[31] = 0x088DE070u; f_089F31AC(c);
                    ARG(3) = (uint32_t)(int16_t)(c->r[20] - 0xD);
                    ARG(0) = *W(c->r[21]); ARG(1) = 1; ARG(2) = 0x56; c->r[8] = 8;
                    c->r[31] = 0x088DE090u; f_0880A6CC(c);
                }
                name(c, entry + 4, row_y, 0, 0x088DE0A8u);
            }
            if (S16(LIST + 12) == (int32_t)c->r[19] + S16(LIST + 18)) *H(LIST + 16) = 0x1E;
        }
        entry = *W(entry);
        c->r[17] = entry;
        c->r[20] += 0x18;
        c->r[19]++;
        c->r[21] += 4;
    }
    /* A cursor outside what is shown moves on by itself too. */
    cursor = S16(LIST + 12);
    first = S16(LIST + 18);
    if (cursor < first || cursor >= first + (int32_t)c->r[19]) {
        int32_t wait = S16(LIST + 16);
        if (wait != 0) *H(LIST + 16) = (uint16_t)(wait - 1);
        else *W(sp + 48) = cursor != 0 ? 0xFFFFFFFFu : 1;
    }
    c->r[16] = *W(sp + 48);
    if (entry != 0) {
        ARG(2) = (uint32_t)(int16_t)(c->r[20] - 0x22); ARG(1) = 0x114; ARG(0) = 4;
        c->r[31] = 0x088DE284u; f_0880A7B0(c);
        while (entry != 0) {
            entry_release(c, entry, 0x088DE298u);
            entry = *W(entry);
            c->r[17] = entry;
        }
    }
    if (*W(LIST + 8) != 0) {
        ARG(0) = *W(LIST + 8); ARG(1) = 1;
        c->r[31] = 0x088DE2BCu; hle_sceKernelSignalSema(c);
    }
    if (c->r[16] != 0) { ARG(0) = c->r[16]; c->r[31] = 0x088DE2CCu; f_088DC948(c); }
    box(c, 0x11F, 0xB3, 0x175, 0xC9, 0x2040A0FFu, 0x088DE2ECu);
    box(c, 0x11D, 0xB1, 0x173, 0xC7, 0xC0000026u, 0x088DE30Cu);
    c->r[17] = (uint32_t)S16(LIST + 14) - *W(sp + 32);
    c->r[16] = 0x08A3EB60u;
    c->r[18] = 0x08A012E8u;
    c->r[31] = 0x088DE32Cu; f_088D595C(c);
    ARG(3) = c->r[2] - 1;
    ARG(0) = c->r[16]; ARG(1) = c->r[18]; ARG(2) = c->r[17];
    c->r[31] = 0x088DE340u; f_089F31AC(c);
    ARG(2) = 0x120; ARG(3) = 0xB4; ARG(0) = c->r[16]; ARG(1) = 2; c->r[8] = 4;
    c->r[31] = 0x088DE358u; f_0880A6CC(c);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9];
    c->r[29] += 0x60;
}
