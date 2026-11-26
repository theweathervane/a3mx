/* Steps of 107 - 108 instructions: a held-button charge that releases into a rush faster the sooner it
   lets go (0x0896DB0C), and a charge that spawns a pair of sparks and steps on when its timer or a
   check ends it (0x0896E7E0). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_08861900(Cpu *c); void f_0886E3CC(Cpu *c);
void f_0887C408(Cpu *c); void f_08822EAC(Cpu *c); void f_08822D2C(Cpu *c); void f_0896E990(Cpu *c);

#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter): ticked; while the round runs and its button (+0x394) is held (unlocked, no +648) the
   +83 charge runs down with hit cues (7, 4) and a spark (0x27 / 0xC) every 16 frames; released, out or
   otherwise: +7 on (facing the opponent with no choice made), +123 / y speeds cleared, +0x391 = +74,
   the picture by +76 (+416 +4 table +208), 0x08861900, +74 back, +82 = strength + 1, a rush (x 9.0 /
   8.0 / 7.0 by how much charge is left, plus 2.0 per strength, brake -1/16), +83 = 30 and voice
   0x20C. */
SFA_FUNC(0896DB0C, move_charge_release_rush) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[4];
    c->r[29] -= 0x30;
    for (int i = 0; i < 4; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x0896DB2Cu);
    gaddr w = o + 0x52, q = o + 0x390;
    c->r[18] = w; c->r[19] = q; c->r[17] = w + 1;
    int go = 1;
    if (S8(MATCH + 7) == 0 && S8(o + 648) == 0 && S8(o + 373) == 0 && (*H(o + 752) & *H(q + 4)) != 0) {
        int8_t t = (int8_t)(S8(w + 1) - 1);
        *B(w + 1) = (uint8_t)t;
        if (t != 0) {
            go = 0;
            ARG(0) = o; ARG(1) = 7; ARG(2) = 4; c->r[31] = 0x0896DB9Cu; f_0886E3CC(c);
            if ((*B(w + 1) & 0xF) == 0) {
                ARG(0) = 0x27; ARG(1) = 0xC; c->r[31] = 0x0896DBB4u; f_08822EAC(c);
                if (c->r[2] != 0) *W(c->r[2] + 88) = o;
            }
        }
    }
    if (go) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        if (S8(o + 386) < 0) *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 123) = 0; *W(o + 108) = 0;
        int32_t g = S8(o + 74);
        *W(o + 100) = 0;
        *B(q + 1) = (uint8_t)g;
        gaddr t = *W(*W(o + 416) + 4) + ((uint32_t)S8(o + 194) << 3);
        int32_t k = S8(o + 76);
        uint32_t pic = *W(t + 208);
        *W(o + 28) = pic;
        *W(o + 28) = pic + (uint32_t)(k * 10);
        call1(c, o, f_08861900, 0x0896DC34u);
        *B(o + 74) = (uint8_t)S8(q + 1);
        uint32_t left = *B(w + 1);
        uint32_t sp = left < 60 ? (left < 30 ? 0x90000u : 0x80000u) : 0x70000u;
        c->r[19] = sp;
        *B(w) = (uint8_t)(S8(o + 194) + 1);
        *W(o + 96) = sp + ((uint32_t)S8(o + 194) << 17);
        *W(o + 104) = 0xFFFFF000u;
        *B(w + 1) = 0x1E;
        ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x20C; c->r[31] = 0x0896DC9Cu; f_0887C408(c);
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter): unlocked with a button (+760) +82 counts down; ended: the match slowed (+35 = 24), two
   sparks (0x08822D2C 0x20: +346 = strength, +255 = 5, +73 = their index from +0x392, +347 = 1, tied
   to it; the last with +3 = 1); then once the round is over, 0x0896E990 fails or the +83 timer runs
   out, +7 steps on into move 0x25; else +82 from 0x08A8DFC8 (by timer and strength) and ticked. Else
   ticked. */
SFA_FUNC(0896E7E0, move_charge_sparks_25) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[6];
    int32_t lock = S8(o + 373);
    c->r[29] -= 0x30;
    for (int i = 0; i < 6; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 40) = ra;
    c->r[16] = o;
    gaddr w = o + 0x52;
    if (lock == 0 && (*H(o + 760) & 0x3FF) != 0) *B(w) = (uint8_t)(S8(w) - 1);
    if (S8(o + 75) < 0) {
        c->r[21] = MATCH;
        *B(MATCH + 35) = 0x18;
        gaddr q = o + 0x390;
        c->r[19] = q; c->r[18] = 5; c->r[17] = 1;
        gaddr e = 0;
        for (uint32_t i = 0; i < 2; i++) {
            c->r[20] = i;
            ARG(0) = o; ARG(1) = 0x20; ARG(2) = 0; c->r[31] = 0x0896E860u; f_08822D2C(c);
            e = c->r[2];
            if (e != 0) {
                *B(e + 346) = *B(o + 194);
                int32_t n = S8(q + 2);
                *B(e + 255) = 5; *B(e + 73) = (uint8_t)n; *B(e + 347) = 1;
                *W(e + 88) = o;
                *B(q + 2) = (uint8_t)(S8(q + 2) + 1);
            }
        }
        c->r[20] = 2;
        if (e != 0) *B(e + 3) = 1;
        int step = 1;
        if (S8(MATCH + 7) == 0) {
            call1(c, o, f_0896E990, 0x0896E8C4u);
            if (c->r[2] != 0) {
                int8_t t = (int8_t)(S8(w + 1) - 1);
                *B(w + 1) = (uint8_t)t;
                if (t != 0) {
                    step = 0;
                    *B(w) = (uint8_t)S8(0x08A8DFC8u + (uint32_t)(int32_t)t + ((uint32_t)S8(o + 194) << 3));
                    call1(c, o, f_08861848, 0x0896E940u);
                }
            }
        }
        if (step) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x25, f_088616EC, 0x0896E904u);
        }
    } else call1(c, o, f_08861848, 0x0896E96Cu);
    for (int i = 0; i < 6; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
