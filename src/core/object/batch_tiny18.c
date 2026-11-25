/* Small functions the scenarios reach, 15 - 27 instructions: a list lookup with two fallbacks, fighter
   objects' starts, effect steps and set-ups. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088DF068(Cpu *c); void f_088DF7BC(Cpu *c); void f_088E357C(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886DBE8(Cpu *c); void f_0886DB94(Cpu *c); void f_08876998(Cpu *c); void f_08821424(Cpu *c);
void f_0887684C(Cpu *c); void f_088768F0(Cpu *c); void f_08821384(Cpu *c); void f_089439DC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

/* 0x088DF068, else the last live entry (0x088DF7BC), else 0x088E357C. */
SFA_FUNC(088DC018, find_entry) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    call0(c, 0x088DC028u, f_088DF068);
    if (c->r[2] == 0) {
        call0(c, 0x088DC038u, f_088DF7BC);
        if (c->r[2] == 0) call0(c, 0x088DC048u, f_088E357C);
    }
    c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +236 = 0xA; +5 / +7 cleared, +11 = +697)
   and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(08909EB0, object_start_dbe8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    call0(c, 0x08909ECCu, f_0886BF84);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0xA;
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x08909F04u, f_0886DBE8);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7; +5 / +7 / +236 cleared, +11 = +697) and
   0x0886DB94; 1 then, else 0. */
SFA_FUNC(089100E8, object_start_db94) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call0(c, 0x089100FCu, f_0886BF84);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x0891012Cu, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): its picture list (+128) from 0x08A6455C by +348, or with +347 set from 0x08A6457C by
   4 x +346 + +112; then 0x08876998. */
SFA_FUNC(0891CCEC, object_pictures_by) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 347) == 0) *W(o + 128) = *W(0x08A6455Cu + ((uint32_t)(int32_t)S8(o + 348) << 2));
    else *W(o + 128) = *W(0x08A6457Cu + ((((uint32_t)(int32_t)S8(o + 346) << 2) + (uint32_t)(int32_t)S16(o + 112)) << 2));
    call0(c, 0x0891CD4Cu, f_08876998);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): first frame 0x0887684C (3); then on when +75 goes negative, 0x088768F0; drawn. */
SFA_FUNC(08923A44, effect_play_once) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (*B(o + 7) == 0) {
        *B(o + 7) = 1;
        call2(c, o, 3, 0x08923A70u, f_0887684C);
    } else {
        if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        call_on(c, o, 0x08923A98u, f_088768F0);
    }
    call_on(c, o, 0x08923AA0u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): unless the fight is halted (BATTLE +37 / +78) its step by +6 (0x08A70B3C) and 0x08821384;
   then drawn. */
SFA_FUNC(0893A514, effect_step_b3c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
        gaddr h = *W(0x08A70B3Cu + ((uint32_t)*B(o + 6) << 2));
        ARG(0) = o; ARG(1) = h;
        c->r[31] = 0x0893A560u; rt_call_indirect(c, h);
        call_on(c, o, 0x0893A568u, f_08821384);
    }
    call_on(c, o, 0x0893A570u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, its owner's (+88) +24 / +12 / +13 / +11 / +121 copied, then its set-up by kind +3
   (0x08A70BA8). */
SFA_FUNC(0893AA44, effect_copy_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = (uint8_t)S8(p + 12);
    uint32_t kind = *B(o + 3);
    *B(o + 13) = *B(p + 13);
    *B(o + 11) = *B(p + 11);
    *B(o + 121) = (uint8_t)S8(p + 121);
    gaddr h = *W(0x08A70BA8u + (kind << 2));
    *W(c->r[29] + 16) = ra;
    ARG(0) = o; ARG(1) = h; ARG(2) = kind << 2; ARG(3) = kind;
    c->r[31] = 0x0893AAA4u; rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7; +5 / +7 cleared), 0x089439DC (7) and
   0x0886DB94; 1 then, else 0. */
SFA_FUNC(089435D8, object_start_39dc) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call0(c, 0x089435ECu, f_0886BF84);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        call2(c, o, 7, 0x08943614u, f_089439DC);
        call_on(c, o, 0x0894361Cu, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
