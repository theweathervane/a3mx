/* Small functions the scenarios reach, 123 - 128 instructions: a fighter's list-9 helpers (a
   second one for the CPU), a fighter's three gauge pieces by its colour mode. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08822D2C(Cpu *c); void f_0891CDCC(Cpu *c); void f_0887CAD0(Cpu *c); void f_0887CC00(Cpu *c);

/* One list-9 helper's set-up from its fighter, kept at its +228 and counted (+536), then 0x0891CDCC. */
static void helper9(Cpu *c, gaddr f, gaddr n, uint8_t k126, int cpu, uint32_t back) {
    *B(n + 122) = (uint8_t)S8(f + 122);
    *B(n + 121) = (uint8_t)S8(f + 121);
    *B(n + 80) = (uint8_t)S8(f + 80);
    *B(n + 346) = (uint8_t)S8(f + 194);
    *B(n + 255) = 0;
    *B(n + 348) = *B(f + 0x338);
    *B(n + 347) = 0;
    *B(n + 14) = *B(f + 14);
    *B(n + 11) = *B(f + 11);
    *B(n + 12) = (uint8_t)S8(f + 12);
    *B(n + 13) = *B(f + 13);
    *H(n + 112) = 0;
    *H(n + 38) = (uint16_t)S16(f + 38);
    *B(n + 336) = 1; *B(n + 337) = 0; *B(n + 338) = 7; *B(n + 339) = 0;
    *W(n + 88) = f;
    *B(n + 126) = k126;
    if (cpu) *B(n + 280) = (uint8_t)S16(f + 662);
    *W(f + 228) = n;
    *B(f + 536) = (uint8_t)(*B(f + 536) + 1);
    ARG(0) = f; ARG(1) = n;
    c->r[31] = back; f_0891CDCC(c);
    *H(n + 86) = *H(f + 86);
    *W(n + 164) = *W(f + 164);
}

/* (fighter): a list-9 helper (0x08822D2C (9, 0)) set up from it (+126 = 0); the CPU (+251) with +286
   not under +662 gets a second one (+126 = 1, +280 = +662). */
SFA_FUNC(0891F0C4, fighter_helpers_9) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = f;
    for (int i = 1; i < 5; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 36) = ra;
    ARG(1) = 9; ARG(2) = 0;
    c->r[31] = 0x0891F0F0u; f_08822D2C(c);
    gaddr n = c->r[2];
    c->r[20] = n;
    if (n != 0) {
        c->r[19] = f + 0x334; c->r[18] = 1; c->r[17] = 7;
        helper9(c, f, n, 0, 0, 0x0891F198u);
        if (S8(f + 251) != 0 && !(S16(f + 286) < S16(f + 662))) {
            ARG(0) = f; ARG(1) = 9; ARG(2) = 0;
            c->r[31] = 0x0891F1D4u; f_08822D2C(c);
            gaddr m = c->r[2];
            c->r[20] = m;
            if (m != 0) helper9(c, f, m, 1, 1, 0x0891F280u);
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (owner, fighter, value): its three gauge pieces (0x08B02698 / 94 / 90) given the owner's +14, 0x0887CAD0's
   value for the fighter's side (+96), +82 = 8 / 0x10 / 0x18, 0x0887CC00 (fighter, piece, 0 - 2), +72 = 1,
   +100 = value; then their colour rows (+13) moved on by the fighter's colour mode (+593: 3 -> 3 / 3 /
   3, 4 -> 3 / 4 / 4 with +12 = 1, 5 -> 4 / 6 / 6, else 3 / 4 / 4). */
SFA_FUNC(0887D348, fighter_gauge_pieces_mode) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t val = ARG(2);
    uint32_t ra = c->r[31];
    uint32_t sv[8] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    c->r[20] = f; c->r[16] = o; c->r[21] = val;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 40) = sv[6]; *W(sp + 44) = sv[7];
    *W(sp + 48) = ra;
    ARG(0) = (uint32_t)S8(f + 337);
    c->r[31] = 0x0887D384u; f_0887CAD0(c);
    uint32_t v = c->r[2];
    c->r[22] = v;
    c->r[19] = 0x08B00000u;
    static const uint32_t at[3] = { 0x08B02698u, 0x08B02694u, 0x08B02690u };
    static const uint32_t back[3] = { 0x0887D3B4u, 0x0887D3ECu, 0x0887D420u };
    c->r[18] = 1;
    for (uint32_t k = 0; k < 3; k++) {
        gaddr g = *W(at[k]);
        if (k == 0) c->r[17] = g; else c->r[23] = g;
        *B(g + 14) = *B(o + 14);
        *W(g + 96) = v;
        *H(g + 82) = (uint16_t)(8 * (k + 1));
        ARG(0) = f; ARG(1) = g; ARG(2) = k;
        c->r[31] = back[k]; f_0887CC00(c);
        *B(g + 72) = 1;
        *W(g + 100) = val;
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
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
