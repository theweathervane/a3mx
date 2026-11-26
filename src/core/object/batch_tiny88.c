/* Small functions the scenarios reach, 129 instructions: a fighter's three gauge pieces by its
   colour mode, a menu cursor's start. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0887CAD0(Cpu *c); void f_0887CB3C(Cpu *c); void f_0887CC00(Cpu *c); void f_0880AEA8(Cpu *c);
void f_0882196C(Cpu *c);

/* (fighter): with 0x0887CB3C, +656 = 4 and its three gauge pieces (0x08B02698 / 94 / 90) given its +14,
   0x0887CAD0's value for its side (+96), +82 = 8 / 0x10 / 0x18, +72 = 1, +100 = 0x01000800, 0x0887CC00
   (fighter, piece, 0); then their colour rows (+13) moved on by its colour mode (+593: 3 -> 3 / 3 / 3,
   4 -> 3 / 4 / 4 with +12 = 1, 5 -> 4 / 6 / 6, else 3 / 4 / 4). */
SFA_FUNC(0887D144, fighter_gauge_pieces_mode_2) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[7] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = f;
    for (int i = 1; i < 7; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 44) = ra;
    ARG(0) = (uint32_t)S8(f + 337);
    c->r[31] = 0x0887D174u; f_0887CAD0(c);
    uint32_t v = c->r[2];
    c->r[17] = v;
    c->r[31] = 0x0887D17Cu; f_0887CB3C(c);
    c->r[20] = f;
    if (c->r[2] != 0) {
        *B(f + 656) = 4;
        c->r[19] = 0x08B00000u; c->r[21] = v; c->r[18] = 1; c->r[22] = 0x01000800u;
        static const uint32_t at[3] = { 0x08B02698u, 0x08B02694u, 0x08B02690u };
        static const uint32_t back[3] = { 0x0887D1CCu, 0x0887D1FCu, 0x0887D22Cu };
        for (uint32_t k = 0; k < 3; k++) {
            gaddr g = *W(at[k]);
            *B(g + 14) = *B(f + 14);
            *W(g + 96) = v;
            *H(g + 82) = (uint16_t)(8 * (k + 1));
            *B(g + 72) = 1;
            *W(g + 100) = 0x01000800u;
            ARG(0) = f; ARG(1) = g; ARG(2) = 0;
            c->r[31] = back[k]; f_0887CC00(c);
            if (k == 0) c->r[17] = 0x08B00000u;
            if (k == 1) c->r[16] = 0x08B00000u;
        }
        int32_t m = S8(f + 593);
        uint32_t add[3]; int lit = 0;
        if (m < 3 || m >= 6) { add[0] = 3; add[1] = 4; add[2] = 4; }
        else if (m == 3) { add[0] = 3; add[1] = 3; add[2] = 3; }
        else if (m == 4) { add[0] = 3; add[1] = 4; add[2] = 4; lit = 1; }
        else { add[0] = 4; add[1] = 6; add[2] = 6; }
        for (int k = 0; k < 3; k++) {
            gaddr g = *W(at[k]);
            *B(g + 13) = (uint8_t)(*B(g + 13) + add[k]);
        }
        if (lit) for (int k = 0; k < 3; k++) *B(*W(at[k]) + 12) = 1;
    }
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (cursor): once GAME +10's low byte is 2, on (+4), +32 = GAME +16's low byte, +148 = 1, +86 = 0xC,
   +164 / +9 / +11 / +14 / +15 / +38 cleared, +24 / +26 = 5 / 1, +12 = 1, +13 = its row (+78) + 7, its
   steps' halves ORed from 0x08A2E138 (8 bytes a row), the screen's table pointer 0x092E3E98 =
   0x08A2DDB0 / 0x08A2DF30 (by 0x09525DE3), x from 0x08A2E118 / 0x08A2E128 by GAME +22's high byte, y
   0x106 (row 0) or -7, the palette 0x08A31798 into 7 (2) and its animation (0x08A2DCF0 / 0x08A2DD50
   +64 by the same byte). */
SFA_FUNC(088ADFB4, menu_cursor_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 36) = s1;
    c->r[17] = GAME;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 40) = s2; *W(c->r[29] + 44) = ra;
    c->r[16] = o;
    if ((*H(GAME + 10) & 0xFF) == 2) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *H(o + 32) = (uint16_t)((uint32_t)S16(GAME + 16) & 0xFF);
        *W(o + 148) = 1;
        *H(o + 86) = 0xC;
        *W(o + 164) = 0;
        c->r[18] = 0x09520000u;
        int sel = S8(0x09525DE3u) != 0;
        *W(0x092E3E98u) = sel ? 0x08A2DDB0u : 0x08A2DF30u;
        int32_t k = S8(o + 78);
        uint32_t vy = *W(o + 100), ay = *W(o + 108);
        *H(o + 38) = 0;
        *B(o + 14) = 0; *B(o + 15) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
        *H(o + 24) = 5;
        *H(o + 26) = 1;
        *B(o + 13) = (uint8_t)(k + 7);
        *B(o + 12) = 1;
        gaddr e = 0x08A2E138u + 8 * (uint32_t)k;
        vy |= (uint32_t)S16(e);
        *W(o + 100) = vy;
        ay |= (uint32_t)S16(e + 2);
        *W(o + 108) = ay;
        *W(o + 100) = vy | ((uint32_t)S16(e + 4) << 16);
        *W(o + 108) = ay | ((uint32_t)S16(e + 6) << 16);
        uint32_t h = (uint32_t)((int32_t)((uint32_t)S16(GAME + 22) & 0xFF00) >> 8);
        *H(o + 18) = (uint16_t)S16((S8(0x09525DE3u) != 0 ? 0x08A2E118u : 0x08A2E128u) + 2 * h);
        *H(o + 22) = k != 0 ? (uint16_t)-7 : 0x106;
        ARG(0) = 1; ARG(1) = 0x08A31798u; ARG(2) = 0; ARG(3) = 7; c->r[8] = 2;
        c->r[31] = 0x088AE150u; f_0880AEA8(c);
        uint32_t h2 = (uint32_t)((int32_t)((uint32_t)S16(GAME + 22) & 0xFF00) >> 8) << 2;
        c->r[17] = h2;
        if (S8(0x09525DE3u) != 0) {
            ARG(0) = o; ARG(1) = *W(0x08A2DCF0u + h2 + 64);
            c->r[31] = 0x088AE180u; f_0882196C(c);
        } else {
            ARG(0) = o; ARG(1) = *W(0x08A2DD50u + h2 + 64);
            c->r[31] = 0x088AE1A0u; f_0882196C(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}
