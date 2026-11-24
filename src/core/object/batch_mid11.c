/* More steps (0.116.0): set-ups by kind from tables, a chain of four objects, the fight's two fighters
 * paired, the CPU's aggression nudged at random, the camera layers set from their tables, and panels
 * by type. Offsets stay numbers until something shows what they are. Arguments of native callees, and
 * what the original leaves in a0 - a3 / t0 at a call through a table, are set as the original sets
 * them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define P1      0x092E3EA0u
#define P2      0x092E4338u
#define P3      0x092E47D0u
#define P4      0x092E4C68u

#include "../guest_mem.h"

void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08822C4C(Cpu *c);   /* fighters_3_4_clear */
void f_088BE788(Cpu *c);   /* flip_376_if_same */
void f_08821368(Cpu *c);   /* game_random */
void f_089DE61C(Cpu *c);   /* steps_6bc_650_754 */
void f_089DDB68(Cpu *c);   /* turn_then_free_with_parent */

/* ---------------------------------------------------------------- objects */

/* (object): shown, +4 moves on, set up at the place of its kind (+3) in 0x08A3C94C (six bytes: the
   animation's index in 0x08A3C87C, x, y), kind 5 as picture 3; kinds 6 and 7 hidden with +82 = 0. */
SFA_FUNC(088CE18C, setup_by_kind_c94c) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t k = *B(o + 3);
    gaddr e = 0x08A3C94Cu + k * 6;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *W(o + 148) = 0;
    *H(o + 86) = 0x12;
    *W(o + 164) = 0;
    *B(o + 13) = 0; *B(o + 12) = 0;
    *H(o + 38) = 0;
    *B(o + 14) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *H(o + 24) = 6;
    if (k == 5) *B(o + 9) = 3;
    *H(o + 18) = *H(e + 2);
    *H(o + 22) = *H(e + 4);
    c->r[4] = o; c->r[5] = *W(0x08A3C87Cu + (uint32_t)(int32_t)S8(e) * 4);
    f_0882196C(c);
    uint32_t k2 = *B(o + 3);
    if (k2 == 6 || k2 == 7) { *H(o + 82) = 0; *B(o + 1) = 0; }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +3 counts GAME +10's high byte (then cleared); with four free in pool 4 (0x095176F3), four
   kind-0x7D objects (steps 4 - 7, +10 = 3); GAME +22's low byte 0xF. */
SFA_FUNC(088B3A84, make_four_7d) {
    gaddr o = ARG(0), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = GAME;
    if (*H(GAME + 10) & 0xFF00) {
        *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
        *H(GAME + 10) = (uint16_t)(*H(GAME + 10) & 0xFF);
    }
    if (!(S8(0x095176F3u) < 4)) {
        c->r[17] = 1; c->r[18] = 3;
        for (uint32_t step = 4; step <= 7; step++) {
            c->r[4] = 0x7D; c->r[5] = step;
            f_08822EAC(c);
            gaddr r = c->r[2];
            *B(r) = 1;
            *B(r + 10) = 3;
            *B(r + 78) = 0;
        }
    }
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 0xF);
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (kind, step): four objects of a kind (+10 = 2, +78 = 3, 2, 1, 0), each pointing at the next (+88). */
SFA_FUNC(088B3258, make_chain_of_four) {
    gaddr s[7] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22] };
    c->r[29] -= 0x30;
    uint32_t kind = (uint32_t)(int32_t)(int16_t)ARG(0) & 0xFF, step = (uint32_t)(int32_t)(int16_t)ARG(1) & 0xFF;
    c->r[16] = kind; c->r[17] = step; c->r[19] = 1; c->r[20] = 2;
    gaddr r[4];
    for (int i = 0; i < 4; i++) {
        c->r[4] = kind; c->r[5] = step;
        f_08822EAC(c);
        r[i] = c->r[2];
        *B(r[i]) = 1;
        *B(r[i] + 10) = 2;
        *B(r[i] + 78) = (uint8_t)(3 - i);
        if (i == 0) c->r[18] = r[0];
        if (i == 1) c->r[21] = r[1];
        if (i == 2) c->r[22] = r[2];
    }
    *W(r[2] + 88) = r[3];
    *W(r[1] + 88) = r[2];
    *W(r[0] + 88) = r[1];
    c->r[29] += 0x30;
    for (int i = 0; i < 7; i++) c->r[16 + i] = s[i];
}

/* (object): its three camera tables (0x0951F950, 152 bytes apart) moved down by +160 (a negative +160
   also sets their high bytes), copied to the camera layers 0x092E8800 / 88A0 / 8940 (+18, +22, +122);
   +160 cleared. */
SFA_FUNC(089DE52C, camera_layers_from_tables) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_089DE61C(c);
    gaddr t = 0x0951F950u;
    int32_t a = S16(t + 14), b = S16(t + 166), d = S16(t + 318);
    if (S16(o + 160) != 0) {
        *H(t + 14) = (uint16_t)(a + S16(o + 160));
        *H(t + 166) = (uint16_t)(b + S16(o + 160));
        *H(t + 318) = (uint16_t)(d + S16(o + 160));
        a = S16(t + 14); b = S16(t + 166); d = S16(t + 318);
        if (S16(o + 160) < 0) {
            *H(t + 14) = (uint16_t)(a | 0xFF00);
            *H(t + 166) = (uint16_t)(b | 0xFF00);
            *H(t + 318) = (uint16_t)(d | 0xFF00);
            a = S16(t + 14); b = S16(t + 166); d = S16(t + 318);
        }
    }
    static const gaddr L[3] = { 0x092E8800u, 0x092E88A0u, 0x092E8940u };
    int32_t y[3] = { a, b, d };
    for (int i = 0; i < 3; i++) {
        gaddr e = t + (uint32_t)i * 152;
        *H(L[i] + 18) = (uint16_t)S16(e + 10);
        *H(L[i] + 22) = (uint16_t)y[i];
        *B(L[i] + 122) = *B(e + 28);
    }
    *H(o + 160) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (panel): +128 moves on; shown with blend 7 / 6 (types 2 and 6 at alpha 0x80, type 3 with 3 / 3, type 5
   blend 2), its colour from 0x08AB6F20 by type (+92); then turned / freed with its parent (0x089DDB68). */
SFA_FUNC(089DDA68, panel_by_type_6f20) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 2) = 0;
    *B(o + 55) = 7;
    *B(o + 56) = 6;
    uint32_t k = *B(o + 92);
    if (k == 2 || k == 6) *B(o + 136) = 0x80;
    if (k == 3) { *B(o + 55) = 3; *B(o + 56) = 3; }
    if (k == 5) *B(o + 56) = 2;
    *B(o + 3) = 0;
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    gaddr e = 0x08AB6F20u + k * 6;
    uint8_t r = (uint8_t)S16(e), g = (uint8_t)S16(e + 2), bl = (uint8_t)S16(e + 4);
    for (int i = 36; i <= 39; i++) *B(o + i) = r;
    for (int i = 42; i <= 45; i++) *B(o + i) = g;
    for (int i = 48; i <= 51; i++) *B(o + i) = bl;
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)(int32_t)S16(e + 4); c->r[6] = o;
    f_089DDB68(c);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters */

/* (?, record): a fight's two fighters: P1 and P2 in use and facing each other (+92), P3 / P4 cleared,
   the record's +136 / +140 at them, and each human's colour (+376) from its ISM, flipped if both match. */
SFA_FUNC(088BE69C, pair_fighters) {
    gaddr b = ARG(1), s[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    c->r[16] = P2; c->r[17] = P1; c->r[18] = b; c->r[19] = P3; c->r[20] = P4;
    *H(b + 56) = 0;
    f_08822C4C(c);
    *B(P1) = 1; *B(P2) = 1;
    *B(P3) = 0; *B(P4) = 0;
    *B(P3 + 415) = 0; *B(P4 + 415) = 0;
    *B(P3 + 372) = 0; *B(P4 + 372) = 0;
    *W(P1 + 92) = P2;
    *W(P2 + 92) = P1;
    *W(b + 136) = P1;
    *W(b + 140) = P2;
    if (S8(P1 + 373) != 0) {
        *B(P1 + 376) = (uint8_t)((S8(P1 + 386) + 1) << 1);
        c->r[4] = P2; c->r[5] = P1;
        f_088BE788(c);
    }
    if (S8(P2 + 373) != 0) {
        *B(P2 + 376) = (uint8_t)((S8(P2 + 386) + 1) << 1);
        c->r[4] = P1; c->r[5] = P2;
        f_088BE788(c);
    }
    c->r[29] += 0x30;
    for (int i = 0; i < 5; i++) c->r[16 + i] = s[i];
}

/* (fighter): the CPU's aggression (+589, 0 - 15) nudged by its character's base (0x08A2141E into
   0x08A19226), its +559 (0x08A1922A; past 31, eight less) and a random 0x08A1924A, at most 8 either way. */
SFA_FUNC(0885CC1C, cpu_aggression_nudge) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = f;
    gaddr base = 0x08A19226u + (uint32_t)(int32_t)S8(0x08A2141Eu + (uint32_t)(int32_t)S8(f + 338));
    int32_t k = S8(f + 559);
    int32_t b0 = S8(base);
    int32_t v = (int32_t)((uint32_t)k & 0xFF) < 32 ? b0 + S8(0x08A1922Au + (uint32_t)k) : b0 - 8;
    c->r[17] = (uint32_t)v;
    c->r[4] = (uint32_t)b0;
    f_08821368(c);
    v += S8(0x08A1924Au + (c->r[2] & 0x1F));
    if (v < -8) v = -8;
    else if (v >= 9) v = 8;
    int32_t n = S8(f + 589) + v;
    *B(f + 589) = (uint8_t)n;
    if (n & 0x80) *B(f + 589) = 0;
    else if (!(S8(f + 589) < 15)) *B(f + 589) = 15;
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}
