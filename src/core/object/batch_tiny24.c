/* Small functions the scenarios reach, 29 - 30 instructions: fighter objects' starts, effects set up
   from their owners, the fight's three teams' flags, a thrown object's launch. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_0887DF88(Cpu *c);
void f_089C47B0(Cpu *c); void f_089E0038(Cpu *c); void f_089E00B0(Cpu *c); void f_089E0128(Cpu *c);
void f_08821424(Cpu *c); void f_0887684C(Cpu *c); void f_088768F0(Cpu *c); void f_0891E914(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

static void on_7(gaddr o, uint32_t k236) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = (uint8_t)k236;
    *B(o + 11) = (uint8_t)S8(o + 697);
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 2; +5 / +7 cleared, +11
   = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(089840B4, object_start_235_2b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    call0(c, 0x089840D0u, f_0886BF84);
    if (c->r[2] != 0) {
        on_7(o, 2);
        *B(o + 235) = 1;
        c->r[17] = 1;
        call_on(c, o, 0x08984110u, f_0886DB94);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 4; +11 = +697) and
   0x0886DBE8, whose answer it gives; else 0. */
SFA_FUNC(0899F084, object_start_235_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call0(c, 0x0899F098u, f_0886BF84);
    if (c->r[2] != 0) {
        on_7(o, 4);
        *B(o + 235) = 1;
        call_on(c, o, 0x0899F0D4u, f_0886DBE8);
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +9 / +12 / +82 cleared, its owner's (+88) +11 / +14 / +24 and x / y taken (x also at
   +0x62), +86 = 5, then 0x0887DF88 (0xF). */
SFA_FUNC(089B387C, effect_from_owner_df88) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *B(o + 14) = *B(p + 14);
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = 0;
    *B(o + 11) = *B(p + 11);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    *H(o + 0x62) = (uint16_t)S16(p + 18);
    *H(o + 82) = 0;
    *H(o + 86) = 5;
    *W(c->r[29] + 16) = ra;
    ARG(2) = 5; ARG(3) = o + 0x60;
    call2(c, o, 0xF, 0x089B38E4u, f_0887DF88);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +1 / +5 / +12 / +13 cleared, 0x089C47B0 (8), +140 = 1, +86 = 5 and its owner's
   (+88) +11 / +24 taken. */
SFA_FUNC(089C4398, effect_from_owner_47b0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 5) = 0; *B(o + 1) = 0;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 8, 0x089C43D0u, f_089C47B0);
    *H(o + 24) = *H(p + 24);
    *B(o + 140) = 1;
    *B(o + 12) = 0; *B(o + 13) = 0;
    *B(o + 11) = *B(p + 11);
    *H(o + 86) = 5;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (match): BATTLE +0 = its +134; the three 152-byte team blocks from 0x0951F950 on (+0 = 1, +2
   cleared) through 0x089E0038 / 0x089E00B0 / 0x089E0128; its +33 / +37 / +41 = 1. */
SFA_FUNC(089DE9F0, teams_on) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0;
    c->r[16] = o;
    *H(BATTLE) = (uint16_t)S16(o + 134);
    *W(c->r[29] + 4) = s1;
    const gaddr t = 0x0951F950u;
    c->r[17] = t;
    *B(t) = 1; *B(t + 152) = 1; *B(t + 304) = 1;
    *W(c->r[29] + 8) = ra;
    ARG(0) = 0x09520000u; ARG(1) = 0x092E0000u; ARG(2) = 1;
    *B(t + 2) = 0;
    call0(c, 0x089DEA30u, f_089E0038);
    *B(t + 154) = 0;
    call0(c, 0x089DEA38u, f_089E00B0);
    *B(t + 306) = 0;
    call0(c, 0x089DEA40u, f_089E0128);
    *B(o + 41) = 1; *B(o + 37) = 1; *B(o + 33) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (effect): first frame its owner's (+88) +228 let go and 0x0887684C (3); then step 3 when +75 goes
   negative, 0x088768F0; drawn. */
SFA_FUNC(088F5A68, effect_play_then_3) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(*W(o + 88) + 228) = 0;
        call2(c, o, 3, 0x088F5A9Cu, f_0887684C);
    } else {
        if (S8(o + 75) < 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        call_on(c, o, 0x088F5AC8u, f_088768F0);
    }
    call_on(c, o, 0x088F5AD0u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 not negative and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +236 = 0xE; +11 =
   +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(088F74B8, object_start_386) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 386) >= 0) {
        call0(c, 0x088F74E0u, f_0886BF84);
        if (c->r[2] != 0) {
            on_7(o, 0xE);
            c->r[17] = 1;
            call_on(c, o, 0x088F7518u, f_0886DB94);
        }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): launched: +9 = 1, +82 = 0x10, x step -0xC0000 slowing by 0xC000, y step 0x60 by 0x60
   (both x turned with +11), x moved 0x60 its way, then 0x0891E914 (0xD). */
SFA_FUNC(0891E1D0, object_launch) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 9) = 1;
    *H(o + 82) = 0x10;
    *W(o + 96) = 0xFFF40000u;
    *W(o + 104) = 0xC000;
    *W(o + 108) = 0x60;
    *W(c->r[29] + 16) = ra;
    *W(o + 100) = 0x60;
    int32_t dx = 0x60;
    if (*B(o + 11) != 0) {
        *W(o + 96) = 0u - *W(o + 96);
        *W(o + 104) = 0u - *W(o + 104);
        dx = -0x60;
    }
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    call2(c, o, 0xD, 0x0891E23Cu, f_0891E914);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
