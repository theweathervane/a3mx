/* Round set-up for the fighters at 0x092E3EA0 + n * 0x498: who is active, who faces whom, the cpu's
   choices and the cpu hit slots for the timed modes. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08822C4C(Cpu *c); void f_08824164(Cpu *c); void f_08823ED4(Cpu *c);

#define FIGHTER(n) (0x092E3EA0u + (n) * 0x498u)

/* (match, round): a one-on-one round: the round's +56 cleared, 0x08822C4C, fighters 0 / 1 active and
   paired (each other's +92, the round's +136 / +140), 2 / 3 off with their sides 2 / 3; outside mode
   0x10 each cpu side gets +376 = 2 x (choice + 1) and 0x08824164 (other, self). In the match's mode 7
   the round's +109 counts down, the cpu sides' hit slots are set (0x08823ED4) and attack kind 8 moves
   the timer at 0x095179E8 on. A 0x30 frame keeping s0..s5. */
SFA_FUNC(08823D3C, round_setup_1v1) {
    gaddr x = ARG(0), q = ARG(1);
    uint32_t sv[6], ra = c->r[31];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1]; c->r[17] = x;
    *W(sp + 16) = sv[0]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    const gaddr f0 = FIGHTER(0), f1 = FIGHTER(1), f2 = FIGHTER(2), f3 = FIGHTER(3);
    c->r[20] = f2; c->r[16] = q;
    *H(q + 56) = 0;
    c->r[19] = f0; c->r[18] = f1;
    *W(sp + 36) = sv[5]; *W(sp + 40) = ra;
    c->r[21] = f3;
    ARG(0) = 0x092E0000u; ARG(1) = q; c->r[31] = 0x08823D8Cu; f_08822C4C(c);
    *B(f0) = 1; *B(f1) = 1; *B(f2) = 0; *B(f3) = 0;
    *B(f2 + 415) = 0; *B(f3 + 415) = 0; *B(f2 + 372) = 0; *B(f3 + 372) = 0;
    *B(f2 + 337) = 2; *B(f2 + 2) = 2; *B(f3 + 337) = 3; *B(f3 + 2) = 3;
    *W(f0 + 92) = f1; *W(f1 + 92) = f0;
    *W(q + 136) = f0; *W(q + 140) = f1;
    if (S8(0x08E75910u + 114) != 0x10) {
        if (S8(f0 + 373) > 0) {
            *B(f0 + 376) = (uint8_t)((S8(f0 + 386) + 1) << 1);
            ARG(0) = f1; ARG(1) = f0; c->r[31] = 0x08823E18u; f_08824164(c);
        }
        if (S8(f1 + 373) > 0) {
            *B(f1 + 376) = (uint8_t)((S8(f1 + 386) + 1) << 1);
            ARG(0) = f0; ARG(1) = f1; c->r[31] = 0x08823E40u; f_08824164(c);
        }
    }
    if (S8(x + 114) == 7) {
        int32_t k = S8(q + 109);
        if (k != 0) *B(q + 109) = (uint8_t)(k - 1);
        if (S8(f0 + 373) != 0) { ARG(0) = x; ARG(1) = q; ARG(2) = f0; c->r[31] = 0x08823E78u; f_08823ED4(c); }
        if (S8(f1 + 373) != 0) { ARG(0) = x; ARG(1) = q; ARG(2) = f1; c->r[31] = 0x08823E90u; f_08823ED4(c); }
        c->r[4] = 0x09510000u;
        if (S8(q + 79) == 8) *H(0x095179E0u + 8) = (uint16_t)(S16(0x095179E0u + 8) + 1);
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}


void f_08824114(Cpu *c); void f_08824200(Cpu *c);

/* (round, a, b, solo, off): a two-on-one round: solo active, off out (+415 / +372 cleared), the round's
   +136 / +140 by its side count +56 (1: solo first, 2: a first), a and b facing solo and solo facing a.
   Without +79: unless mode 0xD / 0x13, 0x08824114 on a and b and 0x08824200 (solo, a, b, 0). With +79:
   each of a / b marked in +106 by side gets 0x08824114; while +109 counts down (the first only while
   0x095179EF is clear) each gets cpu hit slots (0x08823ED4) and the timer at 0x095179E8 moves on; then
   0x08824200. A 0x30 frame keeping s0..s5. */
SFA_FUNC(08823F50, round_setup_2v1) {
    gaddr q = ARG(0), a = ARG(1), b = ARG(2), solo = ARG(3), off = c->r[8];
    uint32_t sv[6], ra = c->r[31];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    c->r[9] = 1;
    *B(solo) = 1;
    *W(sp + 28) = sv[3]; c->r[19] = q;
    *B(off) = 0;
    int32_t k = S16(q + 56);
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 32) = sv[4];
    c->r[20] = 1; c->r[18] = a; c->r[17] = b;
    *W(sp + 16) = sv[0]; *W(sp + 36) = sv[5]; *W(sp + 40) = ra;
    c->r[16] = solo;
    if (k == 1) { *W(q + 136) = solo; *W(q + 140) = a; }
    if (k == 2) { *W(q + 136) = a; *W(q + 140) = solo; }
    *B(off + 415) = 0;
    *B(off + 372) = 0;
    *W(a + 92) = solo; *W(b + 92) = solo; *W(solo + 92) = a;
    const gaddr g = 0x08E75910u;
    if (S8(q + 79) == 0) {
        int32_t mode = S8(g + 114);
        if (mode != 0xD && mode != 0x13) {
            ARG(0) = a; c->r[31] = 0x08823FF8u; f_08824114(c);
            ARG(0) = b; c->r[31] = 0x08824000u; f_08824114(c);
            ARG(0) = solo; ARG(1) = a; ARG(2) = b; ARG(3) = 0; c->r[31] = 0x08824014u; f_08824200(c);
        }
    } else {
        const gaddr tm = 0x095179E0u;
        if (S8(q + 106) & (1 << ((uint32_t)S8(a + 337) & 1))) { ARG(0) = a; c->r[31] = 0x08824040u; f_08824114(c); }
        int32_t n = S8(q + 109);
        c->r[21] = 0x09510000u;
        if (n != 0) {
            c->r[21] = tm;
            if (S8(tm + 15) == 0) {
                *B(q + 109) = (uint8_t)(n - 1);
                ARG(0) = g; ARG(1) = q; ARG(2) = a; c->r[31] = 0x08824074u; f_08823ED4(c);
                *H(tm + 8) = (uint16_t)(S16(tm + 8) + 1);
            }
        }
        if (S8(q + 106) & (1 << ((uint32_t)S8(b + 337) & 1))) { ARG(0) = b; c->r[31] = 0x088240A4u; f_08824114(c); }
        n = S8(q + 109);
        if (n != 0) {
            *B(q + 109) = (uint8_t)(n - 1);
            ARG(0) = g; ARG(1) = q; ARG(2) = b; c->r[31] = 0x088240C8u; f_08823ED4(c);
            *H(tm + 8) = (uint16_t)(S16(tm + 8) + 1);
        }
        ARG(0) = solo; ARG(1) = a; ARG(2) = b; ARG(3) = 0; c->r[31] = 0x088240F0u; f_08824200(c);
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}


void f_088237DC(Cpu *c); void f_088D7C4C(Cpu *c); void f_088D5E44(Cpu *c); void f_08823F50(Cpu *c);

static void setup_2v1(Cpu *c, gaddr q, gaddr a, gaddr b, gaddr solo, gaddr off, uint32_t back) {
    ARG(0) = q; ARG(1) = a; ARG(2) = b; ARG(3) = solo; c->r[8] = off; c->r[31] = back; f_08823F50(c);
}

/* (match, round): the round's line-up. With +90 (a continued match) 0x088237DC, and when 0x088D7C4C
   allows the cpu choices as in the one-on-one set-up. Otherwise the kinds that fix three sides (+137
   with 0x09517E00 = 4, +159 = 3 / 4, attack kind 4 / 5 with +72 = 0x25, kind 0xA counting +109 down)
   take the three-fighter line-up; the team modes 0xD / 0x13 run the two-on-one set-up by +56 (and in
   the alternate mode, unless +53 bit 2, the first fighter with +775 = 2 becomes the cpu); the others a
   two-on-one by +57 bit 1. The three-fighter line-up: +56 = 3, fighters 0 / 1 active against 2, 3 out,
   the round's +136 / +140 by +57 bit 0, 0x08824114 (2) and 0x08824200 (2, 0, 1, 1). A 0x50 frame
   keeping s0..s7. */
SFA_FUNC(08829460, round_lineup) {
    gaddr m = ARG(0), q = ARG(1);
    uint32_t sv[8], ra = c->r[31];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    const gaddr f0 = 0x092E3EA0u, f1 = 0x092E4338u, f2 = 0x092E47D0u, f3 = 0x092E4C68u;
    *W(sp + 48) = sv[4]; c->r[20] = f2;
    *W(sp + 52) = sv[5]; *W(sp + 56) = sv[6]; c->r[22] = 2;
    c->r[21] = f3;
    *W(sp + 60) = sv[7];
    *B(f2 + 337) = 2; c->r[23] = 3; *B(f3 + 337) = 3;
    *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3];
    int32_t cont = S8(q + 90);
    c->r[18] = f0; c->r[19] = f1;
    *W(sp + 32) = sv[0]; c->r[16] = m;
    *W(sp + 36) = sv[1]; *W(sp + 64) = ra;
    c->r[17] = q;
    int three = 0;   /* 1: the three-fighter line-up */
    if (cont != 0) {
        ARG(0) = m; ARG(1) = q; c->r[31] = 0x08829680u; f_088237DC(c);
        c->r[31] = 0x08829688u; f_088D7C4C(c);
        if (c->r[2] != 0) {
            if (S8(f0 + 373) > 0) {
                *B(f0 + 376) = (uint8_t)((S8(f0 + 386) + 1) << 1);
                ARG(0) = f1; ARG(1) = f0; c->r[31] = 0x088296B8u; f_08824164(c);
            }
            if (S8(f1 + 373) > 0) {
                *B(f1 + 376) = (uint8_t)((S8(f1 + 386) + 1) << 1);
                ARG(0) = f0; ARG(1) = f1; c->r[31] = 0x088296E0u; f_08824164(c);
            }
        }
        three = 1;
    } else {
        int32_t k = S8(q + 159), a = S8(q + 79);
        if (S8(m + 137) != 0 && S8(0x09517A78u + 904) == 4) three = 1;
        else if (k == 4 || k == 3) three = 1;
        else if (a == 4 || a == 5) { *H(q + 72) = 0x25; three = 1; }
        else if (a == 0xA) { *B(q + 109) = (uint8_t)(S8(q + 109) - 1); three = 1; }
        else {
            int32_t md = S8(m + 114);
            if (md == 0xD || md == 0x13) {
                c->r[23] = 1;
                if (S16(q + 56) == 1) setup_2v1(c, q, f2, f3, f0, f1, 0x08829578u);
                c->r[4] = q;
                if (S16(q + 56) == 2) setup_2v1(c, q, f2, f3, f1, f0, 0x08829598u);
                *B(f3 + 373) = 0; *B(f2 + 373) = 0;
                c->r[31] = 0x088295A4u; f_088D5E44(c);
                if (c->r[2] != 0 && !(S8(m + 53) & 4)) {
                    const gaddr order[4] = { f0, f1, f2, f3 };
                    for (int i = 0; i < 4; i++)
                        if (S8(order[i] + 775) == 2) { *B(order[i] + 373) = 1; break; }
                }
                *B(f3 + 372) = 0;
                *B(f2 + 372) = 0; *B(f1 + 372) = 0; *B(f0 + 372) = 0;
                *B(f3 + 415) = 0; *B(f2 + 415) = 0; *B(f1 + 415) = 0; *B(f0 + 415) = 0;
            } else if (S8(m + 57) & 2) {
                *H(q + 56) = 2;
                setup_2v1(c, q, f2, f3, f1, f0, 0x08829648u);
            } else {
                *H(q + 56) = 1;
                setup_2v1(c, q, f2, f3, f0, f1, 0x0882966Cu);
            }
        }
    }
    if (three) {
        *H(q + 56) = 3;
        *B(f0) = 1; *B(f1) = 1;
        *B(f2 + 372) = 0; *B(f3 + 372) = 0; *B(f1 + 372) = 0; *B(f0 + 372) = 0;
        *B(f2 + 415) = 0; *B(f3 + 415) = 0; *B(f1 + 415) = 0; *B(f0 + 415) = 0;
        *B(f3) = 0;
        *W(f2 + 92) = f0; *W(f0 + 92) = f2; *W(f1 + 92) = f2;
        if (S8(m + 57) & 1) { *W(q + 136) = f0; *W(q + 140) = f2; }
        else { *W(q + 140) = f1; *W(q + 136) = f2; }
        ARG(0) = f2; c->r[31] = 0x08829748u; f_08824114(c);
        ARG(0) = f2; ARG(1) = f0; ARG(2) = f1; ARG(3) = 1; c->r[31] = 0x0882975Cu; f_08824200(c);
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x50;
}
