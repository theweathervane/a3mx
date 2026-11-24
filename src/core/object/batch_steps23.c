/* Steps (0.152.0): the first ready function of 201 - 400 instructions: a ranking row's cells (place,
   name with its shadow, score or count) built from its entry. Offsets stay numbers until something
   shows what they are. What the original leaves in a0 - t1 at a call is set as the original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define RANK_TABLE 0x09525DE8u   /* u32: the ranking table shown (38 entries of 20 bytes) */

void f_088BD2F0(Cpu *c);   /* index_in_3af8c */
void f_08822F54(Cpu *c);   /* object_make_42 */

/* One 6-byte cell: x, y, glyph. */
static gaddr cell(gaddr p, int32_t x, int32_t y, uint32_t g) {
    *H(p) = (uint16_t)x; *H(p + 2) = (uint16_t)y; *H(p + 4) = (uint16_t)g;
    return p + 6;
}

/* An object of kind 0x22 for a list of cells: type `type`, following the row (+88), kept at the row's
   `slot` and `slot2`. */
static void cells_object(Cpu *c, gaddr row, uint32_t type, uint32_t slot, uint32_t slot2) {
    c->r[4] = 0x22; c->r[5] = type; f_08822F54(c);
    gaddr o = c->r[2];
    if (o != 0) { *W(o + 88) = row; *W(row + slot) = o; *W(row + slot2) = o; }
}

/* (row): row n's (+10) cells from its entry (*0x09525DE8 + 20 n): into 0x08A3848C + 84 n its place
   (rank + 0x87 and a suffix 0x9A + n for the first three, else 0x9D; two digits + 0x63 from 10), the
   name's letters (index_in_3af8c + 0x6D, 16 apart from -0x68) with their shadows into 0x08A392CC +
   18 n (an object of type 4 at +104 / +48 for them), then the score's digits (+0, leading zeros left
   out, from -0x28) or for kinds 3 and up (+14) the count +12 with 0x9E; its last cell marked (bit 15)
   and an object of type 5 at +108 / +40. */
SFA_FUNC(088BD4A0, ranking_row_cells) {
    gaddr row = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    gaddr s21 = c->r[21], s22 = c->r[22], s23 = c->r[23], s30 = c->r[30];
    c->r[29] -= 0x40;
    *W(c->r[29] + 16) = row;
    uint32_t n = *B(row + 10);
    gaddr e = *W(RANK_TABLE) + n * 20;
    gaddr p = 0x08A3848Cu + n * 84, sh = 0x08A392CCu + n * 18;
    int32_t rank = (int16_t)(n + 1);
    const int32_t y = -12;
    if (rank < 10) {
        p = cell(p, -0x98, y, (uint32_t)(rank + 0x87));
        p = cell(p, -0x88, y, rank < 4 ? *B(row + 10) + 0x9Au : 0x9Du);
    } else {
        DIV(c, (uint32_t)rank, 10);
        p = cell(p, -0x98, y, c->lo + 0x63);
        p = cell(p, -0x90, y, c->hi + 0x63);
        p = cell(p, -0x88, y, 0x9D);
    }
    uint32_t letters = 0;
    int32_t x = -0x68, xs = -0x65;
    for (uint32_t ch = *B(e + 4); ch != 0; ch = *B(e + letters + 4), x += 0x10, xs += 0x10) {
        c->r[4] = ch; f_088BD2F0(c);
        int32_t g = (int32_t)c->r[2];
        letters++;
        if (g < 0) continue;
        p = cell(p, x, y, (uint32_t)(g + 0x6D));
        sh = cell(sh, xs, -10, (uint32_t)(g + 0x6D));
    }
    if (letters != 0) {
        *H(sh - 6 + 4) |= 0x8000;
        cells_object(c, row, 4, 104, 48);
    }
    gaddr last;
    if (S8(e + 14) < 3) {
        int drawn = 0;
        int32_t dx = -0x28;
        for (int32_t i = 6, shift = 0x18; i >= 0; i--, shift -= 4, dx += 0x10) {
            uint32_t d = (uint32_t)((int32_t)*W(e) >> shift) & 0xF;
            if (d == 0 && i != 0 && !drawn) continue;
            p = cell(p, dx, y, d + 0x87);
            drawn = 1;
        }
        last = p - 6;
    } else {
        int32_t v = S16(e + 12);
        uint32_t tens = (uint32_t)(v >> 4) & 0xF;
        if (tens != 0) { p = cell(p, -0x18, y, tens + 0x87); v = S16(e + 12); }
        p = cell(p, -8, y, ((uint32_t)v & 0xF) + 0x87);
        last = p;
        cell(p, 8, y, 0x9E);
    }
    *H(last + 4) |= 0x8000;
    cells_object(c, row, 5, 108, 40);
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
    c->r[21] = s21; c->r[22] = s22; c->r[23] = s23; c->r[30] = s30;
}
