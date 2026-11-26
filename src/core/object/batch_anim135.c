/* A fighter's animation step applied (59 instructions each, four copies in the original, one of them
   called on every fighter frame) and the super gauges filled after a hit. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C408(Cpu *c); void f_08825908(Cpu *c);

#define P1    0x092E3EA0u
#define P2    0x092E4338u
#define PART1 0x092E47D0u
#define PART2 0x092E4C68u

/* The step (+28): its frame (the character's frame table, +416 +16, 12 bytes each by the step's +8) kept
   at +296, the step's four bytes at +74 - +77, the frame's picture (+200 + 4 * frame +0) at +256 / +204,
   the frame's box set (+300 + 4 * frame +2) copied to +304 - +307; the set's sound (+306, also kept at
   +79) plays through the fighter's cue list (+292) from its side. +152 = 1. */
static void anim_apply(Cpu *c, gaddr o, gaddr st, uint32_t back) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr fr = *W(*W(o + 416) + 16) + (uint32_t)*H(st + 8) * 12;
    *W(o + 296) = fr;
    *B(o + 74) = *B(st);
    *B(o + 75) = *B(st + 1);
    *B(o + 76) = *B(st + 2);
    *B(o + 77) = *B(st + 3);
    gaddr pic = *W(o + 200) + ((uint32_t)*B(fr) << 2);
    *W(o + 256) = pic; *W(o + 204) = pic;
    gaddr box = *W(o + 300) + ((uint32_t)*B(fr + 2) << 2);
    uint8_t b0 = *B(box), b1 = *B(box + 1), b2 = *B(box + 2);
    *B(o + 0x130) = b0;
    uint8_t b3 = *B(box + 3);
    *B(o + 0x131) = b1; *B(o + 0x132) = b2; *B(o + 0x133) = b3;
    uint32_t snd = *B(o + 306);
    *W(c->r[29] + 20) = ra;
    *B(o + 79) = (uint8_t)snd;
    if (snd != 0 && *W(o + 292) != 0) {
        int32_t n = S16(*W(o + 292) + snd + snd);
        if (n != 0) {
            ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = (uint32_t)n;
            c->r[31] = back; f_0887C408(c);
        }
    }
    *W(o + 152) = 1;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its current step (+28) applied. */
SFA_FUNC(0886175C, fighter_anim_apply_a) { anim_apply(c, ARG(0), *W(ARG(0) + 28), 0x08861830u); }
SFA_FUNC(08861900, fighter_anim_apply) { anim_apply(c, ARG(0), *W(ARG(0) + 28), 0x088619D4u); }

/* (fighter, step): the step made current (+28) and applied. */
static void anim_set(Cpu *c, uint32_t back) {
    gaddr o = ARG(0), st = ARG(1);
    *W(o + 28) = st;
    anim_apply(c, o, st, back);
}
SFA_FUNC(08861AE0, fighter_anim_set) { anim_set(c, 0x08861BB4u); }
SFA_FUNC(08861C70, fighter_anim_set_b) { anim_set(c, 0x08861D44u); }

/* (attacker, hit): unless the hit has +2, every gauge fills (0x08825908 for the four fighter slots)
   when the attack (+119 / +137) or the hit (+79) asks for it or in mode 2 (+114); otherwise only the
   partners' slots fill, and only with the attacker's +191. */
SFA_FUNC(0882581C, gauges_fill_after_hit) {
    gaddr a = ARG(0), h = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = a;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = h;
    static const uint32_t slots[4] = { P1, P2, PART1, PART2 };
    static const uint32_t backs[4] = { 0x08825884u, 0x08825898u, 0x088258ACu, 0x088258C0u };
    if (S16(h + 2) == 0) {
        int first = -1;
        if (S8(a + 119) != 0 || S8(a + 137) != 0 || S8(h + 79) != 0 || S8(a + 114) == 2) first = 0;
        else if (S8(a + 191) != 0) first = 2;
        if (first == 0)
            for (int i = 0; i < 4; i++) { ARG(0) = a; ARG(1) = h; ARG(2) = slots[i]; c->r[31] = backs[i]; f_08825908(c); }
        else if (first == 2) {
            ARG(0) = a; ARG(1) = h; ARG(2) = PART1; c->r[31] = 0x088258E0u; f_08825908(c);
            ARG(0) = a; ARG(1) = h; ARG(2) = PART2; c->r[31] = 0x088258F4u; f_08825908(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
