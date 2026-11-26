/* Small functions the scenarios reach, 36 - 44 instructions: a sum over list entries, fighter objects'
   starts, effects playing once and blinking, a fighter's palettes by its shade, a spark at random.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088DD3DC(Cpu *c); void f_088DD418(Cpu *c); void f_088E0DB8(Cpu *c); void f_0886BF84(Cpu *c);
void f_08821424(Cpu *c); void f_0887684C(Cpu *c); void f_088768F0(Cpu *c); void f_089BE714(Cpu *c);
void f_088215C4(Cpu *c); void f_088585B0(Cpu *c); void f_0880AEA8(Cpu *c); void f_0886DB94(Cpu *c);
void f_08821368(Cpu *c); void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (a, b): the sum of the positive answers of 0x088E0DB8 (entry +228, a, b, 0) over the list walked by
   0x088DD3DC / 0x088DD418 (b only for the first that answers). */
SFA_FUNC(088E0E64, entries_sum) {
    uint32_t a = ARG(0), b = ARG(1) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; *W(sp + 32) = s3;
    c->r[16] = a; c->r[19] = b;
    *W(sp + 24) = s1; *W(sp + 28) = s2; *W(sp + 36) = ra;
    c->r[18] = 0;
    c->r[31] = 0x088E0E8Cu; f_088DD3DC(c);
    *W(sp + 16) = c->r[2];
    c->r[17] = sp + 0x10;
    for (;;) {
        call_on(c, sp + 0x10, 0x088E0E9Cu, f_088DD418);
        gaddr e = c->r[2];
        if (e == 0) break;
        ARG(0) = *W(e + 228); ARG(1) = a; ARG(2) = c->r[19]; ARG(3) = 0;
        c->r[31] = 0x088E0EBCu; f_088E0DB8(c);
        if ((int32_t)c->r[2] > 0) { c->r[18] += c->r[2]; c->r[19] = 0; }
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): unless already in step 1 / 0 / 5, when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +236 = 8;
   +5 / +7 cleared, +11 = +697) answering 1; otherwise +627 on and 0. */
SFA_FUNC(0894984C, object_start_627) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int ok = 0;
    if (!(*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 5)) {
        c->r[31] = 0x08949890u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 236) = 8;
            *B(o + 11) = (uint8_t)S8(o + 697);
            ok = 1;
        }
    }
    if (ok) c->r[2] = 1;
    else { *B(o + 627) = (uint8_t)(S8(o + 627) + 1); c->r[2] = 0; }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): first frame 0x0887684C (7 with +347 set, else 6) and drawn; then, unless halted, on (+4)
   once +75 goes negative and 0x088768F0; drawn. */
SFA_FUNC(0899D590, effect_play_6_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (*B(o + 6) == 0) {
        *B(o + 6) = 1;
        if (S8(o + 347) != 0) call2(c, o, 7, 0x0899D5C4u, f_0887684C);
        else call2(c, o, 6, 0x0899D5D8u, f_0887684C);
        call_on(c, o, 0x0899D5E0u, f_08821424);
    } else {
        if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
            if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            call_on(c, o, 0x0899D624u, f_088768F0);
        }
        call_on(c, o, 0x0899D62Cu, f_08821424);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (marker): +4 on, +78 cleared, 0x089BE714, then its fighter's (+88) place (+16 / +20, +24, +12 / +13,
   side +337 as +86, +164), +11 cleared, +140 = -1, +170 / +171 = 1. */
SFA_FUNC(089BE41C, marker_start_10) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 78) = 0;
    *W(c->r[29] + 24) = ra;
    call_on(c, o, 0x089BE44Cu, f_089BE714);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 140) = 0xFF;
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 11) = 0;
    *B(o + 13) = *B(p + 13);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    *B(o + 170) = 1; *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (light, fighter): every 4 frames (+140) its shade (+127) flips between two; with the fight over
   (BATTLE +7) or the fighter without +769 it settles (+119 / +127 = 0x15; in the second case also
   0x088585B0, +7 on, +139 = 0x3C; with the fight over +7 = 3); drawn. */
SFA_FUNC(08858158, light_flicker) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    int8_t t = (int8_t)(S8(o + 140) - 1);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 140) = (uint8_t)t;
    *W(c->r[29] + 20) = ra;
    if (t < 0) {
        *B(o + 140) = 3;
        *B(o + 127) = (uint8_t)(S8(o + 127) ^ 3);
    }
    if (S8(BATTLE + 7) != 0) {
        *B(o + 7) = 3; *B(o + 127) = 0x15; *B(o + 119) = 0x15;
    } else if (S8(p + 769) == 0) {
        *B(o + 127) = 0x15; *B(o + 119) = 0x15;
        call_on(c, o, 0x088581C8u, f_088585B0);
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 139) = 0x3C;
    }
    call_on(c, o, 0x088581F8u, f_088215C4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, shade): unless it already has this shade (+657), its palettes again: slot +13 + 1 from
   the 32-byte row (shade + 3 x colour - 0x12) of +416's +132 list, slot +13 its colour (+376) from its
   side's list (0x092D9E00 + 4 x +337, 160 bytes a colour). */
SFA_FUNC(0886F684, fighter_shade_palettes) {
    gaddr f = ARG(0);
    int32_t k = (int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = f;
    gaddr tab = *W(*W(f + 416) + 132);
    *W(c->r[29] + 36) = ra;
    if (S8(f + 657) != k) {
        int32_t col = S8(f + 376);
        uint32_t idx = ((uint32_t)(k + 3 * col - 0x12) & 0xFFFF) << 5;
        ARG(0) = 0; ARG(1) = tab + idx; ARG(2) = 0; ARG(3) = *B(f + 13) + 1u; c->r[8] = 1;
        c->r[31] = 0x0886F6E4u; f_0880AEA8(c);
        gaddr base = *W(0x092D9E00u + ((uint32_t)(int32_t)S8(f + 337) << 2));
        ARG(0) = 0; ARG(1) = base + 160u * (uint32_t)(int32_t)S8(f + 376); ARG(2) = 0; ARG(3) = *B(f + 13); c->r[8] = 1;
        c->r[31] = 0x0886F724u; f_0880AEA8(c);
    } else ARG(0) = tab;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): with +386 not negative, few effects (none, or under two with +251) and 0x0886BF84 allowing
   it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 4; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(08901D44, object_start_386_few_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    uint32_t n = *B(o + 536);
    if (S8(o + 386) >= 0 && (S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0)) {
        c->r[31] = 0x08901D7Cu; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 4;
            c->r[17] = 1;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, 0x08901DB8u, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): a list-0x1D spark (0x08822EAC) at it with its +9 / +11 / +14 / +24, 0x5C - 0x6B ahead and 0
   - 15 higher at random, +86 = 5. */
SFA_FUNC(0890D498, effect_spark_1d) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    call2(c, 0x1D, 0, 0x0890D4B8u, f_08822EAC);
    gaddr n = c->r[2];
    c->r[17] = n;
    if (n != 0) {
        *B(n + 1) = 0;
        *B(n + 14) = *B(o + 14);
        *H(n + 24) = *H(o + 24);
        *B(n + 9) = *B(o + 9);
        *H(n + 18) = (uint16_t)S16(o + 18);
        *H(n + 22) = (uint16_t)S16(o + 22);
        c->r[31] = 0x0890D4F4u; f_08821368(c);
        uint32_t dir = *B(o + 11);
        int32_t d = (int32_t)(c->r[2] & 0xF) + 0x5C;
        *B(n + 11) = (uint8_t)dir;
        if (dir == 0) d = -d;
        c->r[16] = (uint32_t)(int32_t)S16(n + 22);
        *H(n + 18) = (uint16_t)(S16(n + 18) + d);
        c->r[31] = 0x0890D520u; f_08821368(c);
        *H(n + 22) = (uint16_t)(c->r[16] + (c->r[2] & 0xF));
        *H(n + 86) = 5;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
