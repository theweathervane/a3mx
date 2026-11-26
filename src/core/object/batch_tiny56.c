/* Small functions the scenarios reach, 50 - 51 instructions: whether a fighter may start a move,
   a 24-byte record kept once per slot, a fighter's picture table and list-0x19 helper, an effect
   set up from its owner, a fighter's two-check start, a list-0x2C helper of a fighter. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_088632C4(Cpu *c); void f_088DD124(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c);
void f_088D3B64(Cpu *c); void f_0886C08C(Cpu *c); void f_0886C164(Cpu *c); void f_08822D2C(Cpu *c);

/* (fighter): 1 when it may: in step 1/1/1/1, +73 set, +702 clear, +112 not negative, neither side
   held (BATTLE +13 / +14) nor GAME +38, and 0x088632C4 says no; else 0. */
SFA_FUNC(0886C508, fighter_may_start) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t r = 0;
    if (*B(f + 4) == 1 && *B(f + 5) == 1 && *B(f + 6) == 1 && *B(f + 7) == 1 &&
        S8(f + 73) != 0 && S8(f + 702) == 0 && S16(f + 112) >= 0) {
        uint32_t held = (uint32_t)(S8(BATTLE + 13) | S8(BATTLE + 14) | S8(GAME + 38));
        if (held == 0) {
            c->r[5] = 0; c->r[6] = (uint32_t)S8(GAME + 38); c->r[7] = GAME;
            c->r[31] = 0x0886C5A8u; f_088632C4(c);
            r = c->r[2] == 0;
        }
    }
    c->r[2] = r;
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (which, record): with 0x09523980 +45 on, the slot (the larger of 0x088DD124 and +12 for which,
   else the smaller) takes the 24-byte record (at +0x3E + 24 per slot) once (+254 per slot). */
SFA_FUNC(088E51DC, slot_record_once) {
    uint32_t which = ARG(0), rec = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 8) = s2;
    gaddr base = 0x09523980u;
    c->r[18] = base;
    *W(sp) = s0;
    c->r[16] = which;
    *W(sp + 4) = s1; *W(sp + 12) = ra;
    c->r[17] = rec;
    if (*B(base + 45) != 0) {
        c->r[6] = *B(base + 45);
        c->r[31] = 0x088E5210u; f_088DD124(c);
        int32_t a = (int32_t)c->r[2], b = (int32_t)*W(base + 12);
        int32_t lo = a, hi = b;
        if (b < a) { lo = b; hi = a; }
        uint32_t idx = (uint32_t)(which != 0 ? hi : lo);
        if (S8(base + idx + 254) == 0) {
            gaddr dst = base + idx * 24 + 0x3E;
            for (uint32_t i = 0; i < 24; i += 2) {
                *B(dst + i) = (uint8_t)S8(rec + i);
                *B(dst + i + 1) = (uint8_t)S8(rec + i + 1);
            }
            *B(base + idx + 254) = 1;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A8DB74, 0x0886EB48
   (0x19, 0x08A8DC10), +86 = its side, +164 = 1 and a list-0x19 helper (0x08822F54) of its
   character linked both ways (its +88, the fighter's +40), +82 cleared, its side and +164. */
SFA_FUNC(0896A964, fighter_table_copy_19) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A8DB74u + 4 * n);
    ARG(0) = f; ARG(1) = 0x19; ARG(2) = 0x08A8DC10u;
    c->r[31] = 0x0896A9C4u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    ARG(0) = 0x19; ARG(1) = *B(f + 338);
    c->r[31] = 0x0896A9ECu; f_08822F54(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *H(n + 82) = 0;
        *W(f + 40) = n;
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4, +1 = 1, +78 cleared), its animation set (0x088D3B64: 0x08AA75E8, 0x1D), then its
   owner's (+88) place, +24, +11 - +13, side and +164; +140 = -1, +170 / +171 = 1. */
SFA_FUNC(089BEA34, effect_from_owner_1d) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 5) = 0; *B(o + 1) = 1; *B(o + 78) = 0;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 0x08AA75E8u; ARG(2) = 0x1D;
    c->r[31] = 0x089BEA7Cu; f_088D3B64(c);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 140) = 0xFF;
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 11) = *B(p + 11);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    *B(o + 170) = 1; *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 0 in step 1/0/3/3; else 0x0886C08C's answer once 0x0886C164 allows it (both (f, 0, 1)),
   0 otherwise. */
SFA_FUNC(0886C2B4, fighter_start_checked) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[18] = 0; c->r[17] = 1;
    *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    c->r[16] = f;
    uint32_t r;
    if (*B(f + 4) == 1 && *B(f + 5) == 0 && *B(f + 6) == 3 && *B(f + 7) == 3) r = 0;
    else {
        c->r[19] = 0;
        ARG(0) = f; ARG(1) = 0; ARG(2) = 1;
        c->r[31] = 0x0886C320u; f_0886C164(c);
        if (c->r[2] != 0) {
            ARG(0) = f; ARG(1) = 0; ARG(2) = 1;
            c->r[31] = 0x0886C35Cu; f_0886C08C(c);
            c->r[19] = c->r[2];
        }
        r = c->r[19];
    }
    c->r[2] = r;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter, k): a list-0x2C helper (0x08822D2C) of its, kept at +228 (none: 0): its k (+3), +80 /
   +121 / +122 / +139 / +194 / +251 / +14 / +24 / side / +164, +346 = +194 + 3k, on (+0), pointing
   back at it (+88); the fighter's effect count (+536) counted. */
SFA_FUNC(089C4CFC, fighter_helper_2c) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    uint32_t k = ARG(1) & 0xFF;
    c->r[17] = k;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 24) = ra;
    ARG(1) = 0x2C; ARG(2) = 0;
    c->r[31] = 0x089C4D20u; f_08822D2C(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *B(n + 3) = (uint8_t)k;
        *B(n + 139) = (uint8_t)S8(f + 139);
        *B(n + 194) = (uint8_t)S8(f + 194);
        *B(n + 121) = (uint8_t)S8(f + 121);
        *B(n) = 1;
        *B(n + 251) = (uint8_t)S8(f + 251);
        *H(n + 24) = *H(f + 24);
        *B(f + 536) = (uint8_t)(*B(f + 536) + 1);
        *B(n + 14) = *B(f + 14);
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
        *B(n + 122) = (uint8_t)S8(f + 122);
        *B(n + 80) = (uint8_t)S8(f + 80);
        *B(n + 346) = (uint8_t)(S8(f + 194) + 3 * k);
    }
    *W(f + 228) = n;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
