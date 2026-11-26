/* Small functions the scenarios reach, 73 instructions: a fighter's palettes from its picture
   table for a mode, a fighter's three gauge marks set up, a list-4 hit spark made at an offset,
   the battle's settings taken from the network block. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_0887CAD0(Cpu *c); void f_0887CB3C(Cpu *c); void f_0887CC00(Cpu *c);
void f_08822EAC(Cpu *c); void f_088DD114(Cpu *c);

/* One palette load: 0x0880AEA8 (0, from, 0, slot, 1). */
static void pal_load(Cpu *c, uint32_t from, uint32_t slot, uint32_t back) {
    ARG(0) = 0; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* (fighter, mode): unless already in it (+657), palettes from its picture table's +128 bank: its +376
   + 8 row (into +13) and the shared row +0x1C0 (into +13 + 4); character 0x29 also +0x4C0 (+13 + 5)
   and its row + 0x300 (+13 + 1). */
SFA_FUNC(0886EEC8, fighter_mode_palette_2) {
    gaddr f = ARG(0);
    int32_t mode = (int8_t)ARG(1);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 48) = sv[4];
    gaddr bank = *W(*W(f + 416) + 128);
    c->r[20] = bank;
    *W(sp + 40) = sv[2];
    uint32_t row = ((uint32_t)S8(f + 376) + 8) & 0xFFFF;
    c->r[18] = row;
    *W(sp + 32) = sv[0]; *W(sp + 36) = sv[1]; *W(sp + 44) = sv[3]; *W(sp + 52) = ra;
    c->r[16] = f;
    if (S8(f + 657) != mode) {
        row <<= 5;
        c->r[18] = row;
        c->r[19] = bank + 0x1C0;
        c->r[17] = bank + row;
        if (S8(f + 338) == 0x29) {
            pal_load(c, bank + 0x4C0, *B(f + 13) + 5u, 0x0886EF40u);
            pal_load(c, bank + 0x1C0, *B(f + 13) + 4u, 0x0886EF5Cu);
            pal_load(c, bank + row + 0x300, *B(f + 13) + 1u, 0x0886EF7Cu);
            pal_load(c, bank + row, *B(f + 13), 0x0886EF94u);
        } else {
            pal_load(c, bank + 0x1C0, *B(f + 13) + 4u, 0x0886EFB4u);
            pal_load(c, bank + row, *B(f + 13), 0x0886EFCCu);
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* (fighter): with 0x0887CB3C, +656 = 4 and its three gauge marks (the objects 0x08A22E14 points at)
   set up (0x0887CC00 with 0x08A22E20's bytes), +2 = 0x11, 0x0887CAD0's value for its side (+96), its
   +14, +72 = 1, +82 from 0x08A22E24, +112 = its +12 / +13, +139 - +142 = 1 / 0 / 8 / 0. */
SFA_FUNC(0887D020, fighter_gauge_marks) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[0];
    c->r[16] = f;
    for (int i = 1; i < 8; i++) *W(sp + 20 + 4 * i) = sv[i];
    *W(sp + 52) = sv[8]; *W(sp + 56) = ra;
    ARG(0) = (uint32_t)S8(f + 337);
    c->r[31] = 0x0887D058u; f_0887CAD0(c);
    uint32_t v = c->r[2];
    c->r[30] = v;
    c->r[31] = 0x0887D060u; f_0887CB3C(c);
    if (c->r[2] != 0) {
        *B(f + 656) = 4;
        c->r[23] = 0; c->r[20] = 0x11; c->r[18] = 1; c->r[17] = 8;
        c->r[22] = 0x08A22E14u; c->r[21] = 0x08A22E20u; c->r[19] = 0x08A22E24u;
        for (uint32_t i = 0; i < 3; i++) {
            gaddr g = *W(*W(0x08A22E14u + 4 * i));
            *W(sp + 16) = 0x08A22E20u;
            c->r[21] = g;
            ARG(0) = f; ARG(1) = g; ARG(2) = *B(0x08A22E20u + i);
            c->r[31] = 0x0887D0B8u; f_0887CC00(c);
            *B(g + 2) = 0x11;
            *W(g + 96) = v;
            *B(g + 14) = *B(f + 14);
            *B(g + 72) = 1;
            *H(g + 82) = (uint16_t)S16(0x08A22E24u + 2 * i);
            *H(g + 112) = (uint16_t)(((uint32_t)*B(f + 12) << 8) | *B(f + 13));
            *B(g + 139) = 1; *B(g + 140) = 0; *B(g + 141) = 8; *B(g + 142) = 0;
            c->r[23] = i + 1;
            c->r[22] = 0x08A22E14u + 4 * (i + 1);
            c->r[19] = 0x08A22E24u + 2 * (i + 1);
            c->r[21] = 0x08A22E20u;
        }
    } else ARG(0) = 4;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* (object, x, y, kind, b, at): a list-4 spark (0x08822EAC, kind) pointing at its owner (+88, or +92
   without +8) with +119 = b, its +121, +135 = -1, +139 = 2 for an owner held (+112 negative); at
   (x, y) itself or (at = 0) that far from it (x turned for its side); +86 = 5. */
SFA_FUNC(088D3724, spark_make_4) {
    gaddr o = ARG(0);
    int32_t x = (int16_t)ARG(1), y = (int16_t)ARG(2);
    uint32_t kind = ARG(3), b = c->r[8] & 0xFF;
    int32_t at = (int16_t)c->r[9];
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 32) = sv[4];
    *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3];
    c->r[16] = o; c->r[17] = (uint32_t)x; c->r[18] = (uint32_t)y; c->r[19] = b; c->r[20] = (uint32_t)at;
    *W(sp + 36) = sv[5]; *W(sp + 40) = ra;
    gaddr owner = *B(o + 8) != 0 ? *W(o + 88) : *W(o + 92);
    c->r[21] = owner;
    ARG(0) = 4; ARG(1) = kind;
    c->r[31] = 0x088D3788u; f_08822EAC(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = owner;
        *B(n + 119) = (uint8_t)b;
        *B(n + 121) = (uint8_t)S8(o + 121);
        *B(n + 135) = 0xFF;
        *H(n + 82) = 0;
        *B(n + 139) = 0;
        if (S16(owner + 112) < 0) *B(n + 139) = 2;
        if (at != 0) {
            *H(n + 18) = (uint16_t)x;
            *H(n + 22) = (uint16_t)y;
        } else {
            uint32_t side = *B(o + 11) & 1;
            *B(n + 11) = (uint8_t)side;
            int32_t dx = side ? (int16_t)-x : x;
            *H(n + 18) = (uint16_t)(S16(o + 18) + dx);
            *H(n + 22) = (uint16_t)(S16(o + 22) + y);
        }
        *H(n + 86) = 5;
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (match): the battle's settings from the network block 0x09523C00 into GAME (+45 / +73, +47, +76 /
   +77, +46 / +116, +48, +118, +119, +156) and its +45; for kinds 2 / 3 (+42) +265 / +266 = 1 / 0x18
   when 0x088DD114 is 2 or more, else both cleared; then GAME's round records (+148 - +185, +126 /
   +127, +160 - +175) and its +35 reset. */
SFA_FUNC(088D6564, match_settings_from_net) {
    gaddr m = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = s0;
    c->r[16] = m;
    gaddr src = 0x09523C00u;
    *W(sp + 4) = s1;
    *B(m + 45) = (uint8_t)S8(src);
    c->r[17] = GAME;
    uint8_t v = (uint8_t)S8(src + 1); *B(GAME + 73) = v; *B(GAME + 45) = v;
    *B(GAME + 47) = (uint8_t)S8(src + 2);
    v = (uint8_t)S8(src + 3); *B(GAME + 77) = v; *B(GAME + 76) = v;
    v = (uint8_t)S8(src + 4); *B(GAME + 46) = v; *B(GAME + 116) = v;
    *B(GAME + 48) = (uint8_t)S8(src + 5);
    *B(GAME + 118) = (uint8_t)S8(src + 6);
    *B(GAME + 119) = (uint8_t)S8(src + 7);
    *B(GAME + 156) = (uint8_t)S8(src + 8);
    uint32_t k = *B(m + 42);
    *W(sp + 8) = ra;
    if (k == 3 || k == 2) {
        c->r[31] = 0x088D65FCu; f_088DD114(c);
        if ((int32_t)c->r[2] < 2) { *B(m + 265) = 0; *B(m + 266) = 0; }
        else { *B(m + 265) = 1; *B(m + 266) = 0x18; }
    } else { *B(m + 265) = 0; *B(m + 266) = 0; }
    *W(GAME + 152) = 0; *W(GAME + 148) = 0;
    *B(GAME + 158) = 0;
    *H(GAME + 180) = 0x64; *H(GAME + 182) = 0x64;
    *B(GAME + 184) = 0;
    *W(GAME + 176) = 0;
    *B(GAME + 185) = 0; *B(GAME + 127) = 0; *B(GAME + 126) = 0; *B(GAME + 159) = 0;
    *B(m + 35) = 0;
    for (uint32_t i = 0; i < 8; i++) { *B(GAME + 160 + i) = 0; *B(GAME + 168 + i) = 0; }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x10;
}
