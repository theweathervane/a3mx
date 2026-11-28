/* A round's set-up. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define F1 0x092E3EA0u
#define F2 0x092E4338u
#define F3 0x092E47D0u
#define F4 0x092E4C68u

void f_08826148(Cpu *c); void f_0887C6E4(Cpu *c); void f_08836624(Cpu *c); void f_0881DF10(Cpu *c);
void f_088379B8(Cpu *c); void f_0880B6CC(Cpu *c);

/* (fighter, n): the fighter's count of win marks (+374) is set to n, and each of its first n
   marks (from +378) that is empty becomes 4. */
SFA_FUNC(08828378, win_marks_fill) {
    gaddr f = ARG(0);
    int32_t n = (int32_t)ARG(1);
    c->r[6] = 4;
    *B(f + 374) = (uint8_t)n;
    gaddr mark = f + 0x17A;
    for (int32_t i = n - 1; i >= 0; i--, mark++) {
        c->r[7] = (uint32_t)S8(mark);
        if (c->r[7] == 0) *B(mark) = 4;
    }
    ARG(1) = mark;
    ARG(0) = 0xFFFFFFFFu;
}

/* (a, b): a round is set up: a's step (+6) goes on and a long run of a's, b's and the four
   fighters' fields are cleared; the two sides' fighters are found (to +16 and +20 of the
   frame). Then whether this is the deciding round: in the link's game (b's +155) the rounds to
   win (b's +8) become one more than the larger of the two sides' wins, and it is the deciding
   round when each side's pair of bytes (at 0x09517A28 and 0x09517A50) match (in the first round
   both fighters' marks are cleared instead);
   otherwise each side needs b's +8 wins, or just 1 when its fighter has +428 set without +373
   (the other side's marks then start at one short in the first round), and the round is the
   deciding one when it is the last the two could need. The deciding round sets b's +63 and
   starts the music 0x114 (and, outside the link's game, fills every fighter's marks). Last,
   the frame's routines. A 0x40 frame keeping s0..s6. */
SFA_FUNC(08829788, round_setup) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[0];
    c->r[16] = a;
    *H(a + 10) = 0;
    *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
    *W(sp + 28) = sv[1]; *W(sp + 48) = sv[6];
    c->r[17] = b;
    *H(a + 8) = 0;
    c->r[22] = 0xFFFFFFFFu;
    *B(b + 14) = 0xFF;
    *B(b + 15) = 1;
    *B(a + 43) = 1;
    *B(a + 72) = 0;
    static const uint8_t b_first[7] = { 13, 76, 53, 96, 84, 36, 44 };
    for (int i = 0; i < 7; i++) *B(b + b_first[i]) = 0;
    *H(b + 38) = 0;
    *B(b + 7) = 0; *B(b + 11) = 0; *B(b + 12) = 0;
    *W(b + 112) = 0;
    *W(b + 116) = 0;
    static const uint8_t b_second[7] = { 30, 29, 28, 58, 35, 64, 42 };
    for (int i = 0; i < 7; i++) *B(b + b_second[i]) = 0;
    *H(0x09523C9Au) = 0; *H(0x09523C9Cu) = 0; *H(0x09523C9Eu) = 0; *H(0x09523CA0u) = 0;
    *B(b + 65) = 0;
    *B(b + 66) = 0;
    *W(sp + 44) = sv[5];
    *B(b + 50) = 0xFF;
    c->r[21] = F4;
    *W(sp + 40) = sv[4];
    *B(b + 51) = 0xFF;
    c->r[20] = F3;
    *W(sp + 36) = sv[3];
    c->r[19] = F2;
    *B(F4 + 237) = 0; *B(F3 + 237) = 0; *B(F2 + 237) = 0;
    *B(F4 + 356) = 0;
    *W(sp + 32) = sv[2];
    c->r[18] = F1;
    *B(F3 + 356) = 0; *B(F2 + 356) = 0;
    *B(F1 + 237) = 0;
    *B(F1 + 356) = 0;
    *W(F1 + 396) = *W(F1 + 352);
    *W(F2 + 396) = *W(F2 + 352);
    *W(sp + 52) = ra;
    ARG(1) = sp + 0x10; ARG(2) = sp + 0x14; ARG(0) = b;
    c->r[31] = 0x088298C8u; f_08826148(c);
    int32_t round = S16(b + 2);
    if (S8(b + 155) != 0) {
        int32_t wins_a = S8(0x09517A50u), wins_b = S8(0x09517A28u);
        *B(b + 8) = (uint8_t)((wins_a < wins_b ? wins_b : wins_a) + 1);
        int deciding = 0;
        if (round == 0) {
            *B(*W(sp + 16) + 374) = 0;
            *B(*W(sp + 20) + 374) = 0;
        } else {
            deciding = S8(0x09517A28u) == S8(0x09517A29u) && S8(0x09517A50u) == S8(0x09517A51u);
        }
        if (deciding) {
            *B(b + 63) = 0xFF;
            ARG(0) = 0x114; c->r[31] = 0x0882995Cu; f_0887C6E4(c);
        }
    } else {
        int32_t rounds = S8(b + 8);
        gaddr fa = *W(sp + 16), fb = *W(sp + 20);
        int32_t need_a = rounds, need_b = rounds;
        if (S8(fa + 373) == 0 && S8(fa + 428) != 0) {
            need_a = 1;
            if (round == 0) {
                *B(fb + 374) = (uint8_t)(rounds - 1);
                round = S16(b + 2);
                fb = *W(sp + 20);
            }
        }
        if (S8(fb + 373) == 0 && S8(fb + 428) != 0) {
            need_b = 1;
            if (round == 0) {
                *B(*W(sp + 16) + 374) = (uint8_t)(S8(b + 8) - 1);
                round = S16(b + 2);
            }
        }
        if (need_a + need_b - 1 == round) {
            *B(b + 63) = 0xFF;
            ARG(0) = 0x114; c->r[31] = 0x088299F0u; f_0887C6E4(c);
            static const uint32_t fighters[4] = { F1, F2, F3, F4 };
            for (uint32_t i = 0; i < 4; i++) {
                ARG(1) = (uint32_t)(S8(b + 8) - 1);
                ARG(0) = fighters[i];
                c->r[31] = 0x08829A00u + 0x10 * i; f_08828378(c);
            }
        }
    }
    if (S8(a + 145) == 0) { c->r[31] = 0x08829A44u; f_08836624(c); }
    c->r[31] = 0x08829A4Cu; f_0881DF10(c);
    if (S8(a + 145) == 0) { c->r[31] = 0x08829A60u; f_088379B8(c); }
    c->r[31] = 0x08829A68u; f_0880B6CC(c);
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}
