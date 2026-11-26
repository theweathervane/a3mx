/* Small functions the scenarios reach, 42 - 43 instructions: fighter objects' starts, effects set up
   on their owner, an effect landing and sliding, a fighter's held-button count, a list entry's call,
   an effect following its owner's move. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08821424(Cpu *c); void f_0882196C(Cpu *c);
void f_089BE790(Cpu *c); void f_088D31EC(Cpu *c); void f_088D3B88(Cpu *c); void f_088D3BEC(Cpu *c);
void f_08877258(Cpu *c); void f_088DD124(Cpu *c); void f_088DD314(Cpu *c); void f_088E0CC8(Cpu *c);
void f_088215C4(Cpu *c); void f_0887CA88(Cpu *c); void f_0887CAF4(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): when it has no effects (+536), or under two while +251 is set, and 0x0886BF84 allows it,
   on (+4 = 1, +6 = 7, +235 = 1, +236 = 5; +5 / +7 cleared, +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0899B424, object_start_few_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int few = S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0;
    c->r[2] = 0;
    if (few) {
        c->r[31] = 0x0899B454u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 5) = 0; *B(o + 236) = 5;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, 0x0899B490u, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): +4 on, +9 = 1, at the owner's place (+16 / +20, +11, +12 / +13) 0x8D higher and
   0x30 ahead of it, its animation 0x08A9D998's word, drawn. */
SFA_FUNC(089A8C38, effect_above_owner) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1;
    *B(o + 13) = *B(p + 13);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 11) = *B(p + 11);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x8D);
    *W(c->r[29] + 20) = ra;
    *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) != 0 ? 0x30 : -0x30));
    call2(c, o, *W(0x08A9D968u + 48), 0x089A8CC8u, f_0882196C);
    call_on(c, o, 0x089A8CD0u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (marker): +4 on, +78 cleared, 0x089BE790, then its fighter's (+88) place (+16 / +20, +24, +12 / +13,
   side +337 as +86, +164), +11 cleared, +140 / +170 / +171 = 1. */
SFA_FUNC(089BE4C8, marker_start_11) {
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
    call_on(c, o, 0x089BE4F8u, f_089BE790);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 140) = 1;
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 11) = 0;
    *B(o + 13) = *B(p + 13);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *B(o + 170) = 1;
    *W(o + 164) = *W(p + 164);
    *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): step 0: 0x088D31EC and on reaching its owner's (+88) floor (+132) it lands (+5 on, +74 =
   1); step 1: x by its step (+96), on (+4) once 0x088D3BEC says the other fighter is out of range;
   then 0x088D3B88. */
SFA_FUNC(089C42BC, effect_land_slide) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        call_on(c, o, 0x089C42E4u, f_088D31EC);
        int32_t fl = S16(p + 132);
        c->r[17] = (uint32_t)fl;
        if (!(fl < S16(o + 22))) {
            *H(o + 22) = (uint16_t)fl;
            *H(o + 20) = 0;
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *B(o + 74) = 1;
        }
    } else {
        *W(o + 16) = *W(o + 16) + *W(o + 96);
        call_on(c, o, 0x089C4330u, f_088D3BEC);
        if (c->r[2] == 0) { *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1); }
    }
    call_on(c, o, 0x089C4350u, f_088D3B88);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): how many of its two kinds of input are in: for a player, a change in the low 4 bits of
   +752 / +754 and any of bits 4 - 9 of +760; for the CPU two 0x08877258 checks (0x08A22BA0). */
SFA_FUNC(08863D70, fighter_inputs_in) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2;
    c->r[18] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[16] = f;
    if (S8(f + 373) == 0) {
        if (((*H(f + 752) ^ *H(f + 754)) & 0xF) != 0) c->r[18] = 1;
        if (*H(f + 760) & 0x3F0) c->r[18] = (c->r[18] + 1) & 0xFFFF;
    } else {
        c->r[17] = 0x08A22BA0u;
        call2(c, f, 0x08A22BA0u, 0x08863DDCu, f_08877258);
        if (c->r[2] != 0) c->r[18] = 1;
        call2(c, f, 0x08A22BA0u, 0x08863DF0u, f_08877258);
        if (c->r[2] != 0) c->r[18] = (c->r[18] + 1) & 0xFFFF;
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (n, a, b, d): with no block (0x095236C0) and n not the current entry (0x088DD124), 0x088E0CC8 (entry n,
   a, b, d); otherwise the block's word. */
SFA_FUNC(088E0DB8, entry_call_cc8) {
    uint32_t n = ARG(0), a = ARG(1), b = ARG(2) & 0xFF, d = ARG(3);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    c->r[19] = 0x095236C0u;
    *W(sp + 32) = sv[4];
    c->r[20] = b;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[18] = n; c->r[17] = a;
    *W(sp + 16) = sv[0]; *W(sp + 36) = ra;
    c->r[16] = d;
    int done = 0;
    if (*W(0x095236C0u) == 0) {
        c->r[31] = 0x088E0DFCu; f_088DD124(c);
        if (n != c->r[2]) {
            call_on(c, n, 0x088E0E0Cu, f_088DD314);
            ARG(0) = c->r[2]; ARG(1) = a; ARG(2) = b; ARG(3) = d;
            c->r[31] = 0x088E0E20u; f_088E0CC8(c);
            done = 1;
        }
    }
    if (!done) c->r[2] = *W(0x095236C0u);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (effect): while its owner (+88) is in its step 1 / 0 / 8: 0x0887CA88 / 0x0887CAF4, its box (+296) the
   current frame's 12-byte row of +356, drawn; otherwise it ends (step 3). */
SFA_FUNC(0891D2A0, effect_follow_owner_move) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    uint32_t key = ((uint32_t)*B(p + 4) << 24) | ((uint32_t)*B(p + 5) << 16) | ((uint32_t)*B(p + 6) << 8);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (key == 0x01000800u) {
        call_on(c, o, 0x0891D2E8u, f_0887CA88);
        call2(c, o, c->r[2], 0x0891D2F4u, f_0887CAF4);
        *W(o + 296) = *W(o + 356) + 12u * *H(*W(o + 28) + 8);
        call_on(c, o, 0x0891D320u, f_088215C4);
    } else { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
