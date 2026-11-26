/* Small functions the scenarios reach, 51 - 53 instructions: an object following its owner's level,
   a sliding effect, a CPU fighter's list-0x21 helper, an effect set up above its owner, a palette
   list's colours copied, an effect tied to its fighter's step, a fighter's picture table and
   list-3 helper. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088D3B64(Cpu *c); void f_088D3B88(Cpu *c); void f_08821384(Cpu *c); void f_08821424(Cpu *c);
void f_088768F0(Cpu *c); void f_08940DF0(Cpu *c); void f_08822D2C(Cpu *c); void f_0897CAD4(Cpu *c);
void f_089E11C8(Cpu *c); void f_088215C4(Cpu *c); void f_0888038C(Cpu *c); void f_088804E0(Cpu *c);
void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (object): follows its owner's (+88) level (+296's +10): with none, off (+1, +78 cleared); with
   one, on (+1); a new level is taken (+78) and set up (0x088D3B64 with 0x08AB26B8), an unchanged
   one 0x088D3B88; +170 = it outside 1 - 5. */
SFA_FUNC(089CCD1C, object_follow_level_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t lv = *B(*W(p + 296) + 10);
    if (lv == 0) { *B(o + 1) = 0; *B(o + 78) = 0; }
    else {
        *B(o + 1) = 1;
        lv = *B(*W(p + 296) + 10);
        c->r[17] = 1;
        if ((uint32_t)S8(o + 78) == lv) call_on(c, o, 0x089CCD6Cu, f_088D3B88);
        else {
            *B(o + 78) = (uint8_t)lv;
            c->r[7] = 0x08AB26B8u;
            ARG(0) = o; ARG(1) = 0x08AB26B8u; ARG(2) = *B(*W(p + 296) + 10);
            c->r[31] = 0x089CCD90u; f_088D3B64(c);
        }
        int32_t v = S8(o + 78);
        *B(o + 170) = (v >= 1 && v <= 5) ? 0 : 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): unless halted, 0x088768F0, its steps halved once (+77), and it slides by them
   (0x08821384) until +75 goes negative: then off (+4 = 2) via 0x08940DF0, not drawn. */
SFA_FUNC(08940AC8, effect_slide_halve) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
        call_on(c, o, 0x08940AFCu, f_088768F0);
        if (S8(o + 77) != 0) {
            *B(o + 77) = 0;
            *W(o + 96) = (uint32_t)((int32_t)*W(o + 96) >> 1);
            *W(o + 100) = (uint32_t)((int32_t)*W(o + 100) >> 1);
        }
        if (S8(o + 75) < 0) {
            *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            call_on(c, o, 0x08940B88u, f_08940DF0);
            c->r[16] = s0; c->r[31] = ra;
            c->r[29] += 0x20;
            return;
        }
        *W(o + 16) += *W(o + 96);
        *W(o + 20) += *W(o + 100);
        call_on(c, o, 0x08940B58u, f_08821384);
    }
    call_on(c, o, 0x08940B60u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): for the CPU (+251) with +662 under +286, a list-0x21 helper (0x08822D2C) set up by
   0x0897CAD4 and given its +80 / +194 / +132, +82 (as +10), +662 (as +280), +255 = 5, +260 = 0xD,
   +311 = 2, +312 = 8; the fighter's effect count (+536) counted. */
SFA_FUNC(0897D3E8, fighter_cpu_helper_21) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if (S8(f + 251) != 0 && S16(f + 662) < S16(f + 286)) {
        ARG(0) = f; ARG(1) = 0x21; ARG(2) = 0;
        c->r[31] = 0x0897D424u; f_08822D2C(c);
        gaddr n = c->r[2];
        c->r[17] = n;
        if (n != 0) {
            *B(n) = 1; *B(n + 1) = 0; *B(n + 3) = 0;
            ARG(0) = f; ARG(1) = n;
            c->r[31] = 0x0897D448u; f_0897CAD4(c);
            *B(n + 80) = (uint8_t)S8(f + 80);
            *B(n + 255) = 5;
            *B(n + 346) = (uint8_t)S8(f + 194);
            *B(n + 347) = 0;
            *H(n + 112) = 0;
            *H(n + 132) = (uint16_t)S16(f + 132);
            *B(n + 10) = (uint8_t)S16(f + 82);
            *B(f + 536) = (uint8_t)(*B(f + 536) + 1);
            *B(n + 280) = (uint8_t)S16(f + 662);
            *B(n + 260) = 0xD; *B(n + 311) = 2; *B(n + 312) = 8;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4, +1 = 1, +78 cleared), its animation set (0x088D3B64: 0x08AA75E8, 0x1C), then its
   owner's (+88) place with y its +132 + 0x58, +24, +11 - +13, side and +164; +140 = -1, +170 /
   +171 = 1. */
SFA_FUNC(089BE8F4, effect_from_owner_1c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1; *B(o + 78) = 0;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 0x08AA75E8u; ARG(2) = 0x1C;
    c->r[31] = 0x089BE938u; f_088D3B64(c);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *H(o + 22) = (uint16_t)(S16(p + 132) + 0x58);
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

/* (n): palette list n (0x09358BE0 +288) of 4-byte entries (flags, to, from, count; bit 0x80 ends
   it): each with no low flag bits copies count * 16 colours (0x089E11C8) from the bank (+284)
   into 0x092DF980 and marks them changed (0x092E0980 = 1). */
SFA_FUNC(089E0254, palette_list_copy) {
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = sv[5];
    gaddr st = 0x09358BE0u;
    c->r[21] = st;
    int32_t n = (int16_t)ARG(0);
    *W(sp + 32) = sv[4];
    gaddr e = *W(*W(st + 288) + ((uint32_t)n << 2));
    c->r[20] = e;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 40) = ra;
    c->r[18] = 0x092E0000u;
    if (e != 0) {
        c->r[17] = 1;
        c->r[18] = 0x092DF980u;
        c->r[16] = 0x092E0000u;
        for (;;) {
            uint32_t flags = *B(e), to = *B(e + 1), from = *B(e + 2), count = *B(e + 3);
            c->r[19] = flags;
            e += 4;
            c->r[20] = e;
            if ((flags & 0x7F) == 0) {
                uint32_t bank = *W(st + 284);
                c->r[7] = bank;
                c->r[8] = (to << 5) + 0x092DF980u;
                ARG(0) = c->r[8]; ARG(1) = bank + (from << 5); ARG(2) = count << 4;
                c->r[31] = 0x089E02F0u; f_089E11C8(c);
                *W(0x092E0000u + 2432) = 1;
            }
            c->r[6] = e + 1;
            if (flags & 0x80) break;
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (effect): unless halted, while its fighter (+88) still does what made it (its +10: bit 0x80
   always; else 0 and the fighter not held (+112), with +702 or in step 1/1 and +116 = 6), +78
   counts round 0 - 63, 0x088804E0 and 0x0888038C; otherwise off (+4 = 2), not drawn. */
SFA_FUNC(088803DC, effect_fighter_step_tied) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (S8(BATTLE + 37) == 0) {
        uint32_t v = *B(o + 10);
        gaddr p = *W(o + 88);
        int go;
        if (v & 0x80) go = 1;
        else if (v != 0 || S16(p + 112) < 0) go = 0;
        else if (S8(p + 702) != 0) go = 1;
        else go = *B(p + 4) == 1 && *B(p + 5) == 1 && *B(p + 116) == 6;
        if (!go) {
            *B(o + 4) = 2;
            c->r[16] = s0; c->r[31] = ra;
            c->r[29] += 0x20;
            return;
        }
        *B(o + 78) = (uint8_t)(S8(o + 78) + 1);
        uint32_t k = (uint32_t)S8(o + 78) & 0x3F;
        *B(o + 78) = (uint8_t)k;
        ARG(0) = o; ARG(1) = p; ARG(2) = k;
        c->r[31] = 0x0888047Cu; f_088804E0(c);
        call_on(c, o, 0x08880484u, f_0888038C);
    }
    call_on(c, o, 0x0888048Cu, f_088215C4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A4B88C, 0x0886EB48 (3,
   0x08A4B928), +549 cleared, +86 = its side, +164 = 1 and a list-3 helper (0x08822F54) linked both
   ways (its +88, the fighter's +40) with its side, +164, +12 and +14. */
SFA_FUNC(088FC974, fighter_table_copy_3) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A4B88Cu + 4 * n);
    ARG(0) = f; ARG(1) = 3; ARG(2) = 0x08A4B928u;
    c->r[31] = 0x088FC9D4u; f_0886EB48(c);
    *B(f + 549) = 0;
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    ARG(0) = 3; ARG(1) = 0;
    c->r[31] = 0x088FC9FCu; f_08822F54(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
        *W(f + 40) = n;
        *B(n + 12) = (uint8_t)S8(f + 12);
        *B(n + 14) = *B(f + 14);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
