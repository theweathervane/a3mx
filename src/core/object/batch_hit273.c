/* The hit check: which side of a hit lands (0x088261E0 by the hit's kind), the hit result written to the
   block at 0x092E1228, and the attack/defender pairing in the block at 0x09358410. The four fighters sit
   at 0x092E3EA0 + n * 0x498 (+112 is a fighter's hit slot). Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088262C0(Cpu *c); void f_088726EC(Cpu *c); void f_0887E84C(Cpu *c); void f_08871358(Cpu *c);
void f_088711F0(Cpu *c); void f_0887335C(Cpu *c); void f_0887DD64(Cpu *c); void f_08872B54(Cpu *c);
void f_08873094(Cpu *c); void f_08872F68(Cpu *c); void f_088729C4(Cpu *c); void f_088734B4(Cpu *c);
void f_088748FC(Cpu *c);

#define FIGHTER(n) (0x092E3EA0u + (n) * 0x498u)
#define SLOT(n) S16(FIGHTER(n) + 112)
#define HIT_RES 0x092E1228u
#define PAIRING 0x09358410u

/* (hit, attacker): for a hit of kind 0x12 with no +56 on the attacker: bit 0 when fighter 1's slot is
   free, bit 1 when fighter 0's is; both (3) gives 2, or 1 when the hit's +53 is 2. */
SFA_FUNC(08826260, hit_side_12) {
    gaddr x = ARG(0), p = ARG(1);
    uint32_t r = 0;
    if (S16(p + 56) == 0) {
        if (SLOT(1) < 0) r = 1;
        if (SLOT(0) < 0) r += 2;
        if (r == 3) r = S8(x + 53) == 2 ? 1 : 2;
    }
    c->r[2] = r;
}

/* (hit, attacker): which sides a hit can land on, by the attacker's +56 (0..3): bit 0 for the far side
   (fighter 1, or 2 / 3 in the team modes, unless +109), bit 1 for the near side (fighter 0, with 1 and
   0 swapped by the hit's +57 bit 1 when +79 is 7; 3 adds 2 when both or either of fighters 0 / 1 are
   free by the hit's +137). */
SFA_FUNC(0882A410, hit_side) {
    gaddr x = ARG(0), p = ARG(1);
    int32_t k = S16(p + 56);
    uint32_t r = 0;
    c->r[3] = k < 2;
    if (k == 0) {
        if (S8(p + 79) == 7) {
            int sw = (S8(x + 57) & 2) != 0;
            int32_t a = sw ? SLOT(1) : SLOT(0), b = sw ? SLOT(0) : SLOT(1);
            if (S8(p + 109) == 0 && b < 0) r = 1;
            if (a < 0) r |= 2;
        } else {
            if (SLOT(1) < 0) r = 1;
            if (SLOT(0) < 0) r |= 2;
        }
    } else if (k == 1) {
        if (S8(p + 109) == 0 && SLOT(2) < 0 && SLOT(3) < 0) r = 1;
        if (SLOT(0) < 0) r |= 2;
    } else if (k == 2) {
        if (S8(p + 109) == 0 && SLOT(2) < 0 && SLOT(3) < 0) r = 1;
        if (SLOT(1) < 0) r |= 2;
    } else if (k == 3) {
        if (S8(p + 109) == 0 && SLOT(2) < 0) r = 1;
        if (S8(x + 137) == 0) { if (SLOT(0) < 0 && SLOT(1) < 0) r += 2; }
        else if (SLOT(0) < 0 || SLOT(1) < 0) r += 2;
    }
    c->r[2] = r;
}

/* (hit, attacker): the sides a hit can land on: kind 0x12 by 0x08826260, an attacker with +79 by
   0x0882A410, otherwise 0x088262C0. */
SFA_FUNC(088261E0, hit_sides) {
    gaddr x = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[6] = p;
    if (S8(x + 114) == 0x12) { c->r[7] = c->r[8] = 0x12; c->r[31] = 0x08826210u; f_08826260(c); }
    else if (S8(p + 79) != 0) { c->r[31] = 0x08826238u; f_0882A410(c); }
    else { c->r[31] = 0x0882624Cu; f_088262C0(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, hit): when the hit can land (0x088261E0 on 0x08E75910 and the result block), the result's +64
   = the defender's +337, 0x088726EC (fighter, defender, hit), its +43 = the hit's +23 less 1 on the
   ground (bit 7: the low bits, flipped by bit 0 when facing left) or 9 in the air (the defender's +366
   dropped by 0x48, not below 0), and 0x0887E84C (defender). A 0x30 frame keeping s0..s3. */
SFA_FUNC(0887260C, hit_apply) {
    gaddr o = ARG(0), q = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = s3; c->r[19] = HIT_RES;
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    c->r[17] = o; c->r[16] = q;
    *W(sp + 24) = s2; *W(sp + 32) = ra;
    ARG(0) = 0x08E75910u; ARG(1) = HIT_RES; c->r[31] = 0x08872644u; f_088261E0(c);
    c->r[4] = 0x09360000u;
    if (c->r[2] != 0) {
        gaddr d = *W(PAIRING + 24);
        c->r[18] = d;
        c->r[6] = (uint32_t)S8(d + 337);
        *B(HIT_RES + 64) = (uint8_t)c->r[6];
        ARG(0) = o; ARG(1) = d; ARG(2) = q; c->r[31] = 0x0887266Cu; f_088726EC(c);
        uint32_t v; int go = 1;
        if (S8(o + 80) == 0) {
            uint32_t k = *B(q + 23);
            c->r[16] = k;
            if (k == 0) go = 0;
            else {
                if (k & 0x80) {
                    k &= 0x7F;
                    if (*B(o + 11) == 0) k = (k ^ 1) & 0xFF;
                    c->r[16] = k;
                }
                v = k - 1;
            }
        } else {
            int16_t h = (int16_t)(S16(d + 366) - 0x48);
            *H(d + 366) = (uint16_t)h;
            c->r[16] = 0xA;
            if (h < 0) *H(d + 366) = 0;
            v = 9;
        }
        if (go) {
            *B(HIT_RES + 43) = (uint8_t)v;
            ARG(0) = d; c->r[31] = 0x088726D0u; f_0887E84C(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter, defender, hit): the defender's slots +112 / +114 cleared to -1, +590 cleared, state 1 / 1 /
   0 / 0, the hit applied (0x0887260C), then 0x08871358 (defender, hit) without a pairing flag, else
   0x088711F0 (fighter, defender, hit). */
SFA_FUNC(08872570, hit_start) {
    gaddr x = ARG(0), o = ARG(1), q = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; c->r[18] = x;
    *W(c->r[29] + 20) = s1; c->r[17] = o;
    *H(o + 114) = 0xFFFF; *H(o + 112) = 0xFFFF;
    *B(o + 590) = 0; *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 0;
    *W(c->r[29] + 16) = s0; c->r[16] = q;
    *B(o + 7) = 0;
    *W(c->r[29] + 28) = ra;
    ARG(0) = x; ARG(1) = q; c->r[31] = 0x088725C0u; f_0887260C(c);
    if (*B(PAIRING + 72) == 0) { ARG(0) = o; ARG(1) = q; c->r[31] = 0x088725E0u; f_08871358(c); }
    else { ARG(0) = x; ARG(1) = o; ARG(2) = q; c->r[31] = 0x088725F4u; f_088711F0(c); }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, hit number, skip): the pairing set to fighter / opponent, the hit record (fighter's +224 +
   32 x number); unless skipped 0x0887335C (opponent) and 0x0887DD64. Not while the result block's +13 /
   +14 or 0x08E75936 are set. Else the hit's value through 0x08872B54 (when +8), 0x08873094 (with
   0x08E75999 on the cpu side), 0x08872F68 into the pairing's +32, 0x088729C4 and 0x088734B4; a taken
   slot ends it, else (outside mode 2) the opponent's slot -1, +698 = 1, 0x088748FC, the pairing set again
   and the hit applied: 1. A 0x30 frame keeping s0..s4. */
SFA_FUNC(0886B6C4, hit_check) {
    gaddr o = ARG(0); uint32_t k = ARG(1), f = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = s3; *W(sp + 32) = s4;
    gaddr p = *W(o + 92); c->r[19] = p;
    c->r[20] = PAIRING;
    *W(sp + 24) = s2;
    gaddr h = *W(o + 224) + (k << 5);
    *B(PAIRING + 72) = 0;
    *W(sp + 20) = s1;
    *W(PAIRING + 24) = o;
    c->r[17] = 0x08E75910u; c->r[18] = h;
    *W(PAIRING + 28) = p;
    *W(sp + 16) = s0; *W(sp + 36) = ra;
    c->r[16] = o;
    if (f == 0) {
        ARG(0) = p; c->r[31] = 0x0886B71Cu; f_0887335C(c);
        ARG(0) = o; ARG(1) = p; ARG(2) = h; c->r[31] = 0x0886B72Cu; f_0887DD64(c);
    }
    uint32_t r = 0;
    if ((S8(HIT_RES + 13) | S8(HIT_RES + 14) | S8(0x08E75910u + 38)) == 0) {
        *H(PAIRING + 32) = 0;
        int32_t v = S8(h + 8);
        if (v != 0) {
            ARG(0) = o; ARG(1) = p; ARG(2) = h; c->r[7] = (uint32_t)v; c->r[31] = 0x0886B778u; f_08872B54(c);
            v = (int8_t)c->r[2];
        }
        c->r[7] = (uint32_t)v;
        if (S8(0x08E75910u + 137) != 0 && S8(o + 373) != 0) {
            ARG(0) = h; ARG(1) = (uint32_t)v; c->r[31] = 0x0886B7A4u; f_08873094(c);
            v = (int8_t)c->r[2];
            c->r[7] = (uint32_t)v;
        }
        ARG(0) = o; ARG(1) = p; ARG(2) = h; c->r[31] = 0x0886B7C0u; f_08872F68(c);
        *H(PAIRING + 32) = (uint16_t)(int8_t)c->r[2];
        ARG(0) = o; ARG(1) = p; ARG(2) = h; c->r[31] = 0x0886B7E4u; f_088729C4(c);
        ARG(0) = p; ARG(1) = h; c->r[31] = 0x0886B7F0u; f_088734B4(c);
        if (S16(p + 112) < 0) {
            if (S8(0x08E75910u + 114) == 2) *H(p + 112) = 0;
            else {
                c->r[17] = 1;
                *H(p + 112) = 0xFFFF; *B(p + 698) = 1;
                ARG(0) = p; c->r[31] = 0x0886B848u; f_088748FC(c);
                *W(PAIRING + 24) = o; *W(PAIRING + 28) = p;
                ARG(0) = o; ARG(1) = h; c->r[31] = 0x0886B85Cu; f_0887260C(c);
                r = 1;
            }
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x30;
}


/* (fighter, hit number): the hit check, then the full follow-through. */
SFA_FUNC(0886B68C, hit_check_full) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(2) = 0; c->r[31] = 0x0886B69Cu; f_0886B6C4(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, hit number): the hit check without 0x0887335C / 0x0887DD64. */
SFA_FUNC(0886B6A8, hit_check_quick) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(2) = 1; c->r[31] = 0x0886B6B8u; f_0886B6C4(c);
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_08872540(Cpu *c); void f_08872D20(Cpu *c); void f_088721B8(Cpu *c); void f_08872080(Cpu *c);
void f_0886B4D4(Cpu *c); void f_0886CA2C(Cpu *c);

/* (fighter, defender, hit): a throw's hit. Not while the result block's +13 / +14 or 0x08E75936 are set,
   nor on a taken defender slot. Pairing +32 cleared, +36 = 1; with the hit's +8 its value from
   0x08872B54 and 0x08872540, scaled down (a quarter, an eighth or a sixteenth by both sides' choices,
   at least 1; times 3 or 2 by the attacker's +792 bits 16 / 12) unless the defender's +117 is set or the
   hit has no +18, the defender has +792 bit 3 or the attacker's +790 is 1 / 5 (2 / 4 unscaled), then
   0x08872D20; 0x08872F68 into the pairing's +32 and 0x088729C4. A free slot then starts the hit
   (0x08872570), or in mode 2 takes the slot; else in mode 0xD on team sides 1 / 2 +690 = 0x18 when the
   partner can follow up. A 0x40 frame keeping s0..s7. */
SFA_FUNC(08875B58, throw_hit) {
    gaddr o = ARG(0), d = ARG(1), h = ARG(2);
    uint32_t sv[8], ra = c->r[31];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3]; c->r[19] = PAIRING;
    *W(sp + 36) = sv[5];
    c->r[21] = HIT_RES;
    *W(sp + 32) = sv[4];
    gaddr a = *W(PAIRING + 24); c->r[20] = a;
    *W(sp + 40) = sv[6];
    const gaddr g = 0x08E75910u; c->r[22] = g;
    *W(sp + 16) = sv[0]; c->r[16] = o;
    *W(sp + 20) = sv[1]; c->r[17] = d;
    *W(sp + 24) = sv[2]; *W(sp + 44) = sv[7]; *W(sp + 48) = ra;
    c->r[18] = h;
    int stop = (S8(HIT_RES + 13) | S8(HIT_RES + 14) | S8(g + 38)) != 0;
    if (!stop) { c->r[23] = 1; stop = S16(d + 112) < 0; }
    if (!stop) {
        *H(PAIRING + 32) = 0; *H(PAIRING + 36) = 1;
        uint32_t v = *B(h + 8);
        c->r[7] = v;
        int full = 1;   /* 0: straight to the slot test */
        if (v != 0) {
            ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08875BF4u; f_08872B54(c);
            ARG(0) = d; ARG(1) = c->r[2]; c->r[31] = 0x08875C00u; f_08872540(c);
            int32_t x = (int32_t)c->r[2];
            int scale = 0;
            if (*B(d + 117) == 0xFF) {
                if (*B(h + 18) == 0 || (*W(d + 792) & 8)) full = 0;
                else {
                    int32_t k = S8(a + 790);
                    if (k == 1 || k == 5) full = -1;
                    else if (k != 2 && k != 4) scale = 1;
                }
            }
            if (full == 1) {
                if (scale) {
                    int32_t j = S8(a + 386) + S8(d + 386) + 2;
                    uint32_t m = *W(a + 792);
                    c->r[7] = m;
                    if (j == 0 || j == 1) x >>= 2;
                    else if (j != 2) x >>= 4;
                    else x >>= 3;
                    if (x == 0) x = 1;
                    if (m & 0x10000) x = x + (x << 1);
                    else if (m & 0x1000) x <<= 1;
                }
                ARG(0) = d; ARG(1) = h; c->r[6] = (uint32_t)x; c->r[31] = 0x08875CC8u; f_08872D20(c);
                c->r[7] = c->r[2];
            }
        }
        if (full == 1) {
            ARG(0) = a; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08875CE0u; f_08872F68(c);
            *H(PAIRING + 32) = (uint16_t)c->r[2];
            ARG(0) = a; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08875CF4u; f_088729C4(c);
        }
        int32_t slot = S16(d + 112);
        int32_t mode = S8(g + 114);
        c->r[22] = (uint32_t)mode;
        if (slot < 0) {
            if (mode == 2) *H(d + 112) = 0;
            else { ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08875D18u; f_08872570(c); }
        } else if (mode == 0xD) {
            int32_t side = S16(HIT_RES + 56);
            c->r[21] = (uint32_t)side;
            if ((side == 1 || side == 2) && *B(o + 8) == 0 && (S8(d + 337) & 2) == 0 && *B(d + 117) != 0xFF
                && S8(d + 135) == 0 && S8(PAIRING + (uint32_t)side + 81) != 0)
                *B(o + 690) = 0x18;
        }
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}

/* (fighter, defender, hit number): a throw lands: the pairing set, the defender to state 1 / 1 / 0 / 0
   with the hit's +10 / +9 / +13 / +28 copied to +655 / +119 / +117 / +283, facing bit 1 cleared and +538
   = 1; the meter by 1 outside state 8; +698 = 1 for a hit with +18; 0x088721B8, the throw hit
   (0x08875B58) and 0x088734B4; +135 / +579 / +572 / +637 cleared, +555 = 0x5A, +586 = the hit's +27 +
   0x18 and +587 a quarter of it, 0x08872080, +568 = -1, both +634 = 0x1E, +698 = 1 on a free slot; still
   held, the defender's +1145 marks the attacker (from +338 38) and 0x0886B4D4's +356 = 6. A 0x30 frame
   keeping s0..s3. */
SFA_FUNC(0886B2DC, throw_land) {
    gaddr o = ARG(0), d = ARG(1); uint32_t k = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = s3;
    gaddr h = *W(o + 224) + (k << 5); c->r[19] = h;
    *B(PAIRING + 72) = 0; *W(PAIRING + 24) = o; *W(PAIRING + 28) = d;
    *B(d + 4) = 1; *B(d + 5) = 1; *B(d + 6) = 0; *B(d + 7) = 0;
    *B(d + 655) = *B(h + 10);
    uint8_t f11 = *B(d + 11);
    *B(d + 119) = *B(h + 9); *B(d + 117) = *B(h + 13); *B(d + 283) = *B(h + 28);
    *W(sp + 24) = s2; c->r[18] = 1;
    *B(d + 11) = (uint8_t)(f11 & 0xFD); *B(d + 538) = 1;
    *W(sp + 20) = s1; c->r[17] = o;
    *W(sp + 16) = s0; *W(sp + 32) = ra;
    c->r[16] = d;
    if (*B(o + 6) != 8) { ARG(0) = o; ARG(1) = 1; c->r[31] = 0x0886B37Cu; f_0886CA2C(c); }
    if (*B(h + 18) != 0) *B(d + 698) = 1;
    ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x0886B398u; f_088721B8(c);
    ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x0886B3A8u; f_08875B58(c);
    ARG(0) = d; ARG(1) = h; c->r[31] = 0x0886B3B4u; f_088734B4(c);
    *B(o + 135) = 0; *B(d + 135) = 0; *B(d + 579) = 0; *B(d + 572) = 0; *B(d + 637) = 0;
    *B(d + 555) = 0x5A;
    *B(d + 586) = (uint8_t)(*B(h + 27) + 0x18);
    *B(d + 587) = (uint8_t)(S8(d + 586) >> 2);
    ARG(0) = d; ARG(1) = h; c->r[31] = 0x0886B3F4u; f_08872080(c);
    *B(d + 568) = 0xFF; *B(o + 634) = 0x1E;
    c->r[5] = (uint32_t)S16(d + 112);
    *B(d + 634) = 0x1E;
    if ((int32_t)c->r[5] < 0) *B(d + 698) = 1;
    if (S8(d + 698) == 0) {
        if (!(S8(o + 338) < 38)) *B(d + 1145) = (uint8_t)(1u << ((uint32_t)S8(o + 337) & 31));
        ARG(0) = o; c->r[31] = 0x0886B444u; f_0886B4D4(c);
        *B(c->r[2] + 356) = 6;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter, hit number): a throw lands on the opponent (0x0886B2DC). */
SFA_FUNC(0886B144, throw_land_opp) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = ARG(1);
    *W(c->r[29] + 16) = ra;
    ARG(1) = *W(o + 92); c->r[31] = 0x0886B158u; f_0886B2DC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_08861A14(Cpu *c);

/* (fighter, hit number): a hold throw lands: the pairing set, the defender to state 1 / 1 / 1 / 1 with
   +116 = 7, +117 = 0x20, +538 = 1 and +73 cleared, both +634 = 0x1E, the defender's +82 = 0x40, +568 = -1
   and +555 = 0x5A; the throw hit (0x08875B58) and 0x088734B4; +135 / +579 / +572 cleared, the defender's
   +134 = the facing, then 0x08861A14 (0x1B, 2). A frame keeping s0..s2. */
SFA_FUNC(0886B164, throw_hold_land) {
    gaddr o = ARG(0); uint32_t k = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 92); c->r[17] = p;
    *B(PAIRING + 72) = 0;
    *W(c->r[29] + 24) = s2;
    gaddr h = *W(o + 224) + (k << 5); c->r[18] = h;
    *W(PAIRING + 24) = o; *W(PAIRING + 28) = p;
    *B(p + 4) = 1; *B(p + 5) = 1; *B(p + 6) = 1; *B(p + 7) = 1;
    *B(p + 116) = 7; *B(p + 117) = 0x20; *B(p + 538) = 1; *B(p + 73) = 0;
    *B(o + 634) = 0x1E; *B(p + 634) = 0x1E;
    *H(p + 82) = 0x40; *B(p + 568) = 0xFF; *B(p + 555) = 0x5A;
    *W(c->r[29] + 28) = ra;
    ARG(0) = o; ARG(1) = p; ARG(2) = h; c->r[31] = 0x0886B204u; f_08875B58(c);
    ARG(0) = p; ARG(1) = h; c->r[31] = 0x0886B210u; f_088734B4(c);
    *B(o + 135) = 0; *B(p + 135) = 0; *B(p + 579) = 0; *B(p + 572) = 0;
    *B(p + 134) = *B(o + 11);
    ARG(0) = o; ARG(1) = 0x1B; ARG(2) = 2; c->r[31] = 0x0886B238u; f_08861A14(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08874C90(Cpu *c); void f_0886E3CC(Cpu *c); void f_0887C408(Cpu *c); void f_08872368(Cpu *c);
void f_088758E8(Cpu *c); void f_08821368(Cpu *c); void f_08871AC4(Cpu *c); void f_088724C0(Cpu *c);
void f_08872410(Cpu *c); void f_0887223C(Cpu *c); void f_08875DBC(Cpu *c); void f_0887347C(Cpu *c);
void f_08874CDC(Cpu *c); void f_08870AF8(Cpu *c);

/* (attacker, defender, hit): a hit lands. The hit's +14 recorded per attack slot (+122); a reaction
   handler (0x08870AF8, for reactions 4..0x7F) may take it over. Else 0x08874C90, +123 = 1, the
   attacker's +316 = the defender, the pairing's +72 / +46 / +44 cleared, the defender's +699 = 0xB4,
   +555 = 0x5A and +538 / +588 / +283 / +537 cleared; a stun (+702), or standing hit state 6, is broken
   (0x0886E3CC 0, 1 and 0x0887C408 0x2E). The slot's hit count on (unless the attack's +8 is 2), the hit's
   +10 / +28 / +9 copied (no +119 for a kind-0x22 follow-up), +637 from the attacker's air flag, a
   +1145 mark either turns it into kind 0x1C or is cleared; 0x08872368 and state 1 / 1 / 0 / 0. A
   reaction (0x088758E8 unless +816) that catches sets the pairing's +72 with +117 = -1, +555 = 0x78,
   +598 = 0x1E and 0x08871AC4; otherwise in mode 2 with the side's block flag (by 0x08821368 and +126 /
   +703) +588 / +575 and the pairing's +44, and unmarked 0x088711F0. Then 0x088724C0, 0x088721B8 and
   0x08872410. A 0x30 frame keeping s0..s6. */
SFA_FUNC(08875594, hit_land) {
    gaddr o = ARG(0), d = ARG(1), h = ARG(2);
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = o;
    *B(d + 0x23A + (uint32_t)S8(o + 122)) = (uint8_t)(*B(h + 14) & 0x7F);
    uint32_t r = *B(*W(d + 296) + 5);
    *W(sp + 20) = sv[1]; c->r[17] = d;
    *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    *W(sp + 40) = sv[6]; *W(sp + 44) = ra;
    c->r[18] = h;
    int done = 0;
    if (r != 0 && !(r & 0x80) && !(r < 4)) {
        ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08875608u; f_08870AF8(c);
        done = c->r[2] != 0;
    }
    if (!done) {
        ARG(0) = o; c->r[31] = 0x08875618u; f_08874C90(c);
        c->r[19] = 1;
        *B(o + 123) = 1;
        c->r[20] = PAIRING;
        *W(o + 316) = d;
        *B(PAIRING + 72) = 0;
        *B(d + 699) = 0xB4; *B(d + 555) = 0x5A;
        *B(d + 538) = 0; *B(d + 588) = 0; *B(d + 283) = 0;
        *B(PAIRING + 46) = 0; *B(PAIRING + 44) = 0;
        int32_t k = S8(d + 702);
        *B(d + 537) = 0;
        c->r[21] = 1; c->r[22] = 2;
        int brk = 0; uint32_t b1 = 0, b2 = 0;
        if (k != 0) {
            *B(d + 703) = 0; *B(d + 702) = 0;
            brk = 1; b1 = 0x08875680u; b2 = 0x08875690u;
        } else if (*B(d + 4) == 1 && *B(d + 5) == 1 && *B(d + 116) == 6) {
            brk = 1; b1 = 0x088756CCu; b2 = 0x088756DCu;
        }
        if (brk) {
            ARG(0) = d; ARG(1) = 0; ARG(2) = 1; c->r[31] = b1; f_0886E3CC(c);
            ARG(0) = d; ARG(1) = (uint32_t)S8(d + 337); ARG(2) = 0x2E; c->r[31] = b2; f_0887C408(c);
        }
        if (*B(o + 8) != 2) {
            gaddr n = d + 0x270 + (uint32_t)S8(o + 122);
            *B(n) = (uint8_t)(*B(n) + 1);
        }
        *B(d + 655) = *B(h + 10);
        *B(d + 283) = *B(h + 28);
        int32_t m = S8(d + 1145);
        if (*B(o + 8) != 0 && *B(o + 2) == 0x22) *B(d + 119) = 0;
        else *B(d + 119) = *B(h + 9);
        if (S8(d + 637) == 0) *B(d + 637) = (uint8_t)S8(o + 80);
        if (m != 0) {
            if (*B(o + 8) != 0) *B(d + 1145) = 0;
            else if ((uint32_t)m & (1u << ((uint32_t)S8(o + 337) & 31))) {
                *B(d + 117) = 0x1C; *B(d + 538) = 1; *B(d + 698) = 0;
            } else *B(d + 1145) = 0;
        }
        ARG(0) = d; c->r[31] = 0x08875794u; f_08872368(c);
        *B(d + 4) = 1; *B(d + 5) = 1; *B(d + 6) = 0;
        gaddr rp = *W(d + 296);
        *B(d + 7) = 0;
        r = *B(rp + 5);
        int caught = 0;
        if (r != 0 && !(r & 0x80) && *W(d + 816) == 0) {
            ARG(0) = d; ARG(1) = r; ARG(2) = *B(h + 21); c->r[31] = 0x088757D4u; f_088758E8(c);
            if (c->r[2] != 0) { *B(PAIRING + 72) = 1; caught = 1; }
        }
        if (caught) {
            *B(d + 117) = 0xFF; *B(d + 555) = 0x78; *B(d + 598) = 0x1E;
            ARG(0) = d; ARG(1) = h; c->r[31] = 0x08875898u; f_08871AC4(c);
        } else {
            if (S8(0x08E75910u + 114) == 2) {
                uint32_t mm = *B(0x0950FF60u + 14);
                if (mm & (1u << ((uint32_t)S8(d + 337) & 31))) {
                    int go = 1;
                    if (mm & 0x80) {
                        c->r[4] = mm & 0x80; c->r[31] = 0x0887581Cu; f_08821368(c);
                        go = (c->r[2] & 0x80) != 0;
                    }
                    if (go && (S8(d + 126) == 0 || S8(d + 703) != 0)) {
                        *B(d + 588) = 1;
                        *B(PAIRING + 44) = 1; *B(d + 575) = 1;
                    }
                }
            }
            if (S8(d + 1145) == 0) {
                ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08875868u; f_088711F0(c);
            }
        }
        ARG(0) = o; ARG(1) = d; c->r[31] = 0x088758A4u; f_088724C0(c);
        ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x088758B4u; f_088721B8(c);
        ARG(0) = d; ARG(1) = h; c->r[31] = 0x088758C0u; f_08872410(c);
    }
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (attacker, defender, hit): the pairing's +45 cleared, the defender's side marked in the paired
   attacker's +594, the hit lands (0x08875594); unless that set +45, the throw hit (0x08875B58),
   0x0887223C, 0x08875DBC, 0x0887347C, 0x088734B4 and 0x08874CDC. A 0x30 frame keeping s0..s3. */
SFA_FUNC(08870A24, hit_land_full) {
    gaddr x = ARG(0), d = ARG(1), h = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = d;
    *W(sp + 28) = s3; c->r[19] = PAIRING;
    *B(PAIRING + 45) = 0;
    *W(sp + 24) = s2;
    gaddr a = *W(PAIRING + 24);
    c->r[18] = x;
    *W(sp + 16) = s0; c->r[16] = h;
    c->r[7] = 1;
    *B(a + 594) = (uint8_t)(S8(a + 594) | (1u << ((uint32_t)S8(d + 337) & 31)));
    *W(sp + 32) = ra;
    ARG(0) = x; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08870A80u; f_08875594(c);
    if (*B(PAIRING + 45) == 0) {
        ARG(0) = x; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08870A98u; f_08875B58(c);
        ARG(0) = x; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08870AA8u; f_0887223C(c);
        ARG(0) = d; ARG(1) = h; c->r[31] = 0x08870AB4u; f_08875DBC(c);
        ARG(0) = x; ARG(1) = d; c->r[31] = 0x08870AC0u; f_0887347C(c);
        ARG(0) = d; ARG(1) = h; c->r[31] = 0x08870ACCu; f_088734B4(c);
        ARG(0) = x; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08870ADCu; f_08874CDC(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}


/* (hit object, defender): the hit lands (0x08870A24 with the defender's +272); unless a reaction took it
   (other than reaction 0xC), by the object's kind: 0x17 nothing, 0x2C / 0x2E +0 on, otherwise +0 on and
   +112 counts down, going to state 1 / 1 / 0 / 0, or 2 / 0 / 0 / 0 once spent; a taken-over hit +0 on.
   A frame keeping s0, s1. */
SFA_FUNC(088740E8, hit_object_land) {
    gaddr o = ARG(0), d = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = d;
    uint32_t h = *W(d + 272);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    ARG(0) = o; ARG(1) = d; ARG(2) = h; c->r[31] = 0x08874108u; f_08870A24(c);
    if (*B(PAIRING + 45) != 0 && *B(*W(d + 296) + 5) != 0xC) *B(o) = (uint8_t)(*B(o) + 1);
    else {
        uint32_t k = *B(o + 2);
        if (k != 0x17) {
            uint8_t n = (uint8_t)(*B(o) + 1);
            *B(o) = n;
            if (k != 0x2C && k != 0x2E) {
                int16_t v = (int16_t)(S16(o + 112) - 1);
                *H(o + 112) = (uint16_t)v;
                if (v < 0) { *B(o + 4) = 2; *B(o + 5) = 0; }
                else { *B(o + 4) = 1; *B(o + 5) = 1; }
                *B(o + 6) = 0; *B(o + 7) = 0;
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


/* (a, b, a's hit, b's hit): two live fighters' hits meet (neither +661). One hit alone lands on its
   target when that target is the other's +276 partner (the target's +195 from 0x08B0267E by both sides).
   Both: a +308 armour on one side wins for it (the loser's +669 / +670 / +605 = 6 / 9 / 0xC); the hits'
   +25 classes (high nibble kind, low strength) trade when equal, of different kinds, of kind 0x20, or
   when the strength gap is allowed by 0x08A220FA for 0x08821368's roll; else the stronger wins. A trade
   lands both, each with the pairing set. A 0x30 frame keeping s0..s4. */
SFA_FUNC(0887530C, hit_clash) {
    gaddr a = ARG(0), b = ARG(1), ha = ARG(2), hb = ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    c->r[8] = *B(a);
    *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = s3;
    c->r[19] = a; c->r[18] = b; c->r[17] = ha;
    *W(sp + 16) = s0; *W(sp + 32) = s4; *W(sp + 36) = ra;
    c->r[16] = hb;
    /* 0 none, 1 a's hit lands on b, 2 b's hit lands on a, 3 trade */
    int what = 0;
    if (c->r[8] != 0 && *B(b) != 0 && S8(a + 661) == 0 && S8(b + 661) == 0) {
        if (ha == 0) { if (hb != 0) what = 2; }
        else if (hb == 0) what = 1;
        else {
            int32_t pa = S8(a + 308), pb = S8(b + 308);
            int win = 0;   /* 1 a wins, 2 b wins, 3 trade */
            if (pa != 0 && pb == 0) win = 1;
            else if (pa == 0 && pb != 0) win = 2;
            else {
                uint32_t ka = *B(ha + 25), kb = *B(hb + 25);
                c->r[20] = kb;
                if (ka == kb || (ka & 0xF0) != (kb & 0xF0) || (ka & 0xF0) == 0x20) win = 3;
                else {
                    int32_t diff = (int32_t)(ka & 0xF) - (int32_t)(kb & 0xF);
                    if (diff < 0) diff = -diff;
                    c->r[20] = (uint32_t)diff;
                    c->r[4] = ka & 0xF; c->r[5] = ka & 0xF0; c->r[6] = 0x20;
                    c->r[31] = 0x088753E4u; f_08821368(c);
                    int32_t m = S16(0x08A220FAu + ((c->r[2] & 7) << 1));
                    if (m & (1 << diff)) win = 3;
                    else win = (int32_t)(*B(ha + 25) & 0xF) < (int32_t)(*B(hb + 25) & 0xF) ? 2 : 1;
                }
            }
            if (win == 3) what = 3;
            else {
                gaddr l = win == 1 ? b : a;
                *B(l + 669) = 6; *B(l + 670) = 9; *B(l + 605) = 0xC;
                what = win;
            }
        }
    }
    if (what == 3) {
        c->r[20] = PAIRING;
        *W(PAIRING + 24) = a; *W(PAIRING + 16) = a; *W(PAIRING + 28) = b; *W(PAIRING + 20) = b;
        ARG(0) = a; ARG(1) = b; ARG(2) = hb; c->r[31] = 0x0887545Cu; f_08870A24(c);
        *W(PAIRING + 24) = b; *W(PAIRING + 16) = b; *W(PAIRING + 28) = a; *W(PAIRING + 20) = a;
        ARG(0) = b; ARG(1) = a; ARG(2) = ha; c->r[31] = 0x0887547Cu; f_08870A24(c);
    } else if (what == 1) {
        if (*W(a + 276) == b) {
            *W(PAIRING + 24) = b; *W(PAIRING + 16) = b; *W(PAIRING + 28) = a; *W(PAIRING + 20) = a;
            *B(b + 195) = *B(0x08B0267Eu + (uint32_t)(S8(b + 337) * 4 + S8(a + 337)));
            ARG(0) = b; ARG(1) = a; ARG(2) = ha; c->r[31] = 0x08875504u; f_08870A24(c);
        } else c->r[4] = 0x09360000u;
    } else if (what == 2) {
        if (*W(b + 276) == a) {
            *W(PAIRING + 24) = a; *W(PAIRING + 16) = a; *W(PAIRING + 28) = b; *W(PAIRING + 20) = b;
            *B(a + 195) = *B(0x08B0267Eu + (uint32_t)(S8(a + 337) * 4 + S8(b + 337)));
            ARG(0) = a; ARG(1) = b; ARG(2) = hb; c->r[31] = 0x08875574u; f_08870A24(c);
        } else c->r[4] = 0x09360000u;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x30;
}


/* (a, b, a's box, b's box): do the boxes (+0 x, +2 y, +4 / +6 half sizes, x mirrored by the facing)
   overlap? The box centres go to b's +714 / +716 / +718 / +720; a's half sizes shrink by 4 (at least 1)
   and grow by 4 when the paired defender has +308 armour. */
SFA_FUNC(08873850, boxes_overlap) {
    gaddr a = ARG(0), b = ARG(1), ba = ARG(2), bb = ARG(3);
    int16_t xa = *B(a + 11) != 0 ? (int16_t)(S16(a + 18) - S16(ba)) : (int16_t)(S16(ba) + S16(a + 18));
    int16_t xb = *B(b + 11) != 0 ? (int16_t)(S16(b + 18) - S16(bb)) : (int16_t)(S16(bb) + S16(b + 18));
    c->r[8] = PAIRING;
    *H(b + 714) = (uint16_t)xa; *H(b + 716) = (uint16_t)xb;
    int16_t dx = (int16_t)(xb - xa);
    if (dx < 0) dx = (int16_t)-dx;
    int arm = S8(*W(PAIRING + 28) + 308) != 0;
    int16_t w = (int16_t)(S16(ba + 4) - 4);
    if (w <= 0) w = 1;
    w = (int16_t)(w + S16(bb + 4));
    if (arm) w = (int16_t)(w + 4);
    if (w < dx) { c->r[2] = 0; return; }
    int16_t ya = (int16_t)(S16(ba + 2) + S16(a + 22));
    int16_t yb = (int16_t)(S16(bb + 2) + S16(b + 22));
    *H(b + 718) = (uint16_t)ya; *H(b + 720) = (uint16_t)yb;
    int16_t dy = (int16_t)(yb - ya);
    if (dy < 0) dy = (int16_t)-dy;
    arm = S8(*W(PAIRING + 28) + 308) != 0;
    int16_t h = (int16_t)(S16(ba + 6) - 4);
    if (h <= 0) h = 1;
    h = (int16_t)(h + S16(bb + 6));
    if (arm) h = (int16_t)(h + 4);
    c->r[2] = h < dy ? 0 : 1;
}


void f_08873728(Cpu *c);

/* (a, b, a's box, b's box): the hit box test: with an armoured paired attacker the padded test
   (0x08873850), for a projectile (+8) 0x08873728, otherwise the plain overlap of the two boxes (centres
   to b's +714 / +716 / +718 / +720). */
SFA_FUNC(08873570, hit_boxes) {
    gaddr a = ARG(0), b = ARG(1), ba = ARG(2), bb = ARG(3);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(PAIRING + 24);
    *W(c->r[29] + 16) = ra;
    if (p != 0 && S8(p + 308) != 0) { c->r[31] = 0x0887361Cu; f_08873850(c); }
    else if (*B(a + 8) != 0) { c->r[31] = 0x088735F8u; f_08873728(c); }
    else {
        int16_t xa = *B(a + 11) != 0 ? (int16_t)(S16(a + 18) - S16(ba)) : (int16_t)(S16(ba) + S16(a + 18));
        int16_t xb = *B(b + 11) != 0 ? (int16_t)(S16(b + 18) - S16(bb)) : (int16_t)(S16(bb) + S16(b + 18));
        *H(b + 714) = (uint16_t)xa;
        int16_t dx = (int16_t)(xb - xa);
        *H(b + 716) = (uint16_t)xb;
        if (dx < 0) dx = (int16_t)-dx;
        uint32_t r = 0;
        if (!((int16_t)(S16(ba + 4) + S16(bb + 4)) < dx)) {
            int16_t ya = (int16_t)(S16(ba + 2) + S16(a + 22));
            int16_t yb = (int16_t)(S16(bb + 2) + S16(b + 22));
            *H(b + 718) = (uint16_t)ya; *H(b + 720) = (uint16_t)yb;
            int16_t dy = (int16_t)(yb - ya);
            if (dy < 0) dy = (int16_t)-dy;
            r = (int16_t)(S16(ba + 6) + S16(bb + 6)) < dy ? 0 : 1;
        }
        c->r[2] = r;
    }
    c->r[31] = ra; c->r[29] += 0x20;
}


/* (attacker, defender, hit number): the attacker's hit record against the defender's three hurt boxes
   (+256 lists the box per area, the boxes at +208 / +212 / +216): not a hit already taken in this
   attack slot (unless a projectile). Each overlapping area adds 1 / 2 / 4 to the attacker's +195 and
   keeps the box centres and their distance (+728.. / +736.. / +744.., distance at +722 / +724 / +726;
   the third keeps the attacker's own x and y with the sign of the difference). A touch sets the
   defender's +272 = the hit record and +276 = the attacker: 1. A frame keeping s0..s2. */
SFA_FUNC(08875090, hit_hurtboxes) {
    gaddr a = ARG(0), b = ARG(1); uint32_t k = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2;
    gaddr h = *W(a + 224) + (k << 5);
    *W(c->r[29] + 20) = s1; c->r[18] = h; c->r[17] = a;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = b;
    uint32_t r = 0;
    int skip = *B(a + 8) == 0 && *B(b + 0x23A + (uint32_t)S8(a + 122)) == *B(h + 14);
    gaddr q = *W(b + 256);
    if (!skip && (*B(q) | *B(q + 1) | *B(q + 2) | *B(q + 3)) != 0) {
        static const uint32_t backs[3] = { 0x08875148u, 0x088751D4u, 0x08875260u };
        for (int i = 0; i < 3; i++) {
            uint32_t n = *B(*W(b + 256) + (uint32_t)i);
            if (n == 0) continue;
            ARG(0) = a; ARG(1) = b; ARG(2) = h; ARG(3) = *W(b + 208 + 4 * (uint32_t)i) + (n << 3);
            c->r[31] = backs[i]; f_08873570(c);
            if (c->r[2] == 0) continue;
            *B(a + 195) = (uint8_t)(*B(a + 195) + (1u << i));
            int32_t x1 = S16(b + 714), x2 = S16(b + 716), y1 = S16(b + 718), y2 = S16(b + 720);
            gaddr sl = b + 728 + 8 * (uint32_t)i, dl = b + 722 + 2 * (uint32_t)i;
            *H(sl) = (uint16_t)x1; *H(sl + 2) = (uint16_t)x2;
            int32_t dx, dy;
            if (i < 2) {
                dx = x1 - x2; if (dx < 0) dx = -dx;
                dy = y1 - y2; if (dy < 0) dy = -dy;
            } else {
                dx = (x1 - x2 < 0) ? -x1 : x1;
                dy = (y1 - y2 < 0) ? -y1 : y1;
            }
            *H(dl) = (uint16_t)dx;
            *H(sl + 4) = (uint16_t)y1; *H(sl + 6) = (uint16_t)y2;
            *H(dl) = (uint16_t)(*H(dl) + (uint32_t)dy);
        }
        if (*B(a + 195) != 0) { *W(b + 272) = h; *W(b + 276) = a; r = 1; }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


/* (attacker, defender): may the attacker's current hit (+296's +1) touch the defender? With +281 bit 7
   always; with other +281 only a defender whose reaction (+296's +5) is 0xFF; without +281 anyone but a
   reaction-0xFF defender. Then the hurt box test (0x08875090). */
SFA_FUNC(08870918, hit_try) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t f = S8(a + 281);
    c->r[5] = a; c->r[7] = b;
    *W(c->r[29] + 16) = ra;
    int go; uint32_t back;
    if (f != 0) {
        if (f & 0x80) { go = 1; back = 0x0887095Cu; }
        else { go = *B(*W(b + 296) + 5) == 0xFF; back = 0x08870994u; }
    } else {
        uint32_t rb = *B(*W(b + 296) + 5);
        if (rb & 0x80) { go = rb != 0xFF || rb == 0xFE; back = 0x088709E0u; }
        else { go = 1; back = 0x08870A08u; }
    }
    uint32_t n = go ? *B(*W(a + 296) + 1) : 0;
    if (go && n != 0) {
        c->r[7] = b; ARG(0) = a; ARG(1) = b; ARG(2) = n; c->r[31] = back; f_08875090(c);
    } else c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}
