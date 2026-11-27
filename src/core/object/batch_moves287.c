/* The counter out of a block and its kin: the shared checks every character's command list ends
   with, and each character's counter start, which is the same few lines around its own move. No test
   scenario plays these (they need a choice other than the default, or a blocked attack to counter):
   they are written from the original code alone. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u

void f_08836E60(Cpu *c); void f_088632C4(Cpu *c); void f_0886CA2C(Cpu *c); void f_0886B468(Cpu *c);
void f_08837130(Cpu *c); void f_08864D40(Cpu *c); void f_0886DC3C(Cpu *c); void f_0886C164(Cpu *c);
void f_0886C08C(Cpu *c); void f_0886C380(Cpu *c);
void f_088EEDE8(Cpu *c); void f_088F2E60(Cpu *c); void f_088F7AA0(Cpu *c); void f_08902134(Cpu *c);
void f_089066E0(Cpu *c); void f_08909A6C(Cpu *c); void f_08918F74(Cpu *c); void f_08920DB0(Cpu *c);
void f_08925FB8(Cpu *c); void f_0892ABD8(Cpu *c); void f_0892FFD0(Cpu *c); void f_0893D604(Cpu *c);
void f_08943830(Cpu *c); void f_08949A88(Cpu *c); void f_0894FBFC(Cpu *c); void f_089536AC(Cpu *c);
void f_08958768(Cpu *c); void f_0895CC20(Cpu *c); void f_08962DAC(Cpu *c); void f_089675B8(Cpu *c);
void f_0896C9A4(Cpu *c); void f_089736F4(Cpu *c); void f_0898A6E8(Cpu *c); void f_0898F078(Cpu *c);
void f_08994988(Cpu *c); void f_089A5FA0(Cpu *c); void f_089AAED8(Cpu *c); void f_089B0A9C(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (fighter): 0x08836E60 (3) for a player's fighter, unless both players fight (GAME + 57 = 3). A frame
   with only the return address. */
SFA_FUNC(08837130, player_mark_3) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(GAME + 57) != 3 && S8(ARG(0) + 373) == 0) call1(c, 3, f_08836E60, 0x08837164u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the counter out of a block, when it can be paid for: not with choice -1; the meter
   (+366) holds 72 (48 with choice 0); +816 clear; in state 1 / 1 with +117 = 255, on the ground, the
   fight running, 0x088632C4 clear and +584 under 8. Then the meter pays (0x0886CA2C), the opponent's
   (0x0886B468) +356 = 10, 0x08837130 and 0x08864D40, the limit +556 loses 16 (16 at least, its old
   value kept in +646 and its lowest in +557), +695 set, +81 = +564, +696 / +270 cleared and
   0x0886DC3C: 1. A frame keeping s0, s1. */
SFA_FUNC(0886C380, counter_pay) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int32_t choice = S8(o + 386);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    int32_t cost = choice == 0 ? 0x30 : 0x48;
    uint32_t done = 0;
    c->r[17] = (uint32_t)cost;
    if (choice == -1 || S16(o + 366) < cost || *W(o + 816) != 0 || *B(o + 4) != 1 || *B(o + 5) != 1 ||
        *B(o + 117) != 0xFF || S8(o + 73) != 0 || (S8(BATTLE + 13) | S8(BATTLE + 14) | S8(GAME + 38)) != 0)
        goto out;
    call1(c, o, f_088632C4, 0x0886C438u);
    if (c->r[2] != 0 || !(S16(o + 584) < 8)) goto out;
    call2(c, o, (uint32_t)-cost, f_0886CA2C, 0x0886C458u);
    call1(c, o, f_0886B468, 0x0886C460u);
    *B(c->r[2] + 356) = 10;
    call1(c, o, f_08837130, 0x0886C470u);
    call1(c, o, f_08864D40, 0x0886C478u);
    int32_t limit = S8(o + 556);
    c->r[17] = 1;
    *B(o + 556) = (uint8_t)(limit - 16);
    *B(o + 646) = (uint8_t)limit;
    limit = S8(o + 556);
    *B(o + 695) = 1;
    if (limit < 16) { *B(o + 556) = 0x10; limit = S8(o + 556); }
    if (limit < S8(o + 557)) *B(o + 557) = (uint8_t)limit;
    int8_t v = S8(o + 564);
    *B(o + 696) = 0; *B(o + 81) = (uint8_t)v; *B(o + 270) = 0;
    call1(c, o, f_0886DC3C, 0x0886C4F0u);
    done = 1;
out:
    c->r[2] = done;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the move out of a guard, when the meter (+366) holds 72 and +251 is clear: straight
   from state 1 / 1 / 2 / 0 (0x0886C08C with 1, 0); else on the ground when 0x0886C164 (1, 1) allows
   it, 0x0886C08C with 1, 1; its result, or 0. A frame keeping s0..s2. */
SFA_FUNC(0886C7E4, guard_move_try) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t meter = S16(o + 366);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    c->r[18] = 1; c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    int go = 0;
    if (!(meter < 72) && S8(o + 251) == 0) {
        if (*B(o + 4) == 1 && *B(o + 5) == 1 && *B(o + 6) == 2 && *B(o + 7) == 0) go = 1;
        else {
            c->r[17] = 1;
            if (S8(o + 73) == 0) {
                c->r[6] = 1;
                call2(c, o, 1, f_0886C164, 0x0886C870u);
                go = c->r[2] != 0;
            }
        }
    }
    if (go) { c->r[6] = c->r[17]; call2(c, o, c->r[18], f_0886C08C, 0x0886C888u); }
    else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0 in state 1 / 0 / 5, else 1. No frame. */
SFA_FUNC(0886C9F0, not_in_state_5) {
    gaddr o = ARG(0);
    RETURN(!(*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 5));
}

/* (fighter): a move's timings (+548, +662, +664) by the buttons pressed or held (+760 | +762): the
   two light ones 0 / 15 / 5, the two middle ones 0 / 48 / 16, else 4 / 81 / 27; +270 set. No frame. */
SFA_FUNC(0886DD68, timings_by_buttons) {
    gaddr o = ARG(0);
    uint32_t b = (*H(o + 760) | *H(o + 762)) & 0x3F0;
    uint32_t kind = b == 0x90 ? 0 : b == 0x120 ? 1 : 2;
    static const uint8_t v[3][3] = { { 0, 0xF, 5 }, { 0, 0x30, 0x10 }, { 4, 0x51, 0x1B } };
    *B(o + 548) = v[kind][0]; *H(o + 662) = v[kind][1]; *H(o + 664) = v[kind][2];
    *B(o + 270) = 1;
}

/* A character's counter: when it can be paid for (0x0886C380), the character's own start: 1; else 0. */
static void counter_try(Cpu *c, void (*start)(Cpu *), uint32_t back, uint32_t back_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = back; f_0886C380(c);
    if (c->r[2] != 0) { call1(c, o, start, back_start); c->r[2] = 1; }
    else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): each character's counter. */
SFA_FUNC(088EED9C, counter_try_eed9c) { counter_try(c, f_088EEDE8, 0x088EEDB0u, 0x088EEDC0u); }
SFA_FUNC(088F2E14, counter_try_f2e14) { counter_try(c, f_088F2E60, 0x088F2E28u, 0x088F2E38u); }
SFA_FUNC(088F7A54, counter_try_f7a54) { counter_try(c, f_088F7AA0, 0x088F7A68u, 0x088F7A78u); }
SFA_FUNC(089020E8, counter_try_020e8) { counter_try(c, f_08902134, 0x089020FCu, 0x0890210Cu); }
SFA_FUNC(08906694, counter_try_06694) { counter_try(c, f_089066E0, 0x089066A8u, 0x089066B8u); }
SFA_FUNC(08909A20, counter_try_09a20) { counter_try(c, f_08909A6C, 0x08909A34u, 0x08909A44u); }
SFA_FUNC(08918F28, counter_try_18f28) { counter_try(c, f_08918F74, 0x08918F3Cu, 0x08918F4Cu); }
SFA_FUNC(08920D64, counter_try_20d64) { counter_try(c, f_08920DB0, 0x08920D78u, 0x08920D88u); }
SFA_FUNC(08925F6C, counter_try_25f6c) { counter_try(c, f_08925FB8, 0x08925F80u, 0x08925F90u); }
SFA_FUNC(0892AB8C, counter_try_2ab8c) { counter_try(c, f_0892ABD8, 0x0892ABA0u, 0x0892ABB0u); }
SFA_FUNC(0892FF84, counter_try_2ff84) { counter_try(c, f_0892FFD0, 0x0892FF98u, 0x0892FFA8u); }
SFA_FUNC(0893D5B8, counter_try_3d5b8) { counter_try(c, f_0893D604, 0x0893D5CCu, 0x0893D5DCu); }
SFA_FUNC(089437E4, counter_try_437e4) { counter_try(c, f_08943830, 0x089437F8u, 0x08943808u); }
SFA_FUNC(08949980, counter_try_49980) { counter_try(c, f_08949A88, 0x08949994u, 0x089499A4u); }
SFA_FUNC(0894FBB0, counter_try_4fbb0) { counter_try(c, f_0894FBFC, 0x0894FBC4u, 0x0894FBD4u); }
SFA_FUNC(08953660, counter_try_53660) { counter_try(c, f_089536AC, 0x08953674u, 0x08953684u); }
SFA_FUNC(0895871C, counter_try_5871c) { counter_try(c, f_08958768, 0x08958730u, 0x08958740u); }
SFA_FUNC(0895CBD4, counter_try_5cbd4) { counter_try(c, f_0895CC20, 0x0895CBE8u, 0x0895CBF8u); }
SFA_FUNC(08962D60, counter_try_62d60) { counter_try(c, f_08962DAC, 0x08962D74u, 0x08962D84u); }
SFA_FUNC(0896756C, counter_try_6756c) { counter_try(c, f_089675B8, 0x08967580u, 0x08967590u); }
SFA_FUNC(0896C958, counter_try_6c958) { counter_try(c, f_0896C9A4, 0x0896C96Cu, 0x0896C97Cu); }
SFA_FUNC(089736A8, counter_try_736a8) { counter_try(c, f_089736F4, 0x089736BCu, 0x089736CCu); }
SFA_FUNC(0898A69C, counter_try_8a69c) { counter_try(c, f_0898A6E8, 0x0898A6B0u, 0x0898A6C0u); }
SFA_FUNC(0898F02C, counter_try_8f02c) { counter_try(c, f_0898F078, 0x0898F040u, 0x0898F050u); }
SFA_FUNC(0899493C, counter_try_9493c) { counter_try(c, f_08994988, 0x08994950u, 0x08994960u); }
SFA_FUNC(089A5F54, counter_try_a5f54) { counter_try(c, f_089A5FA0, 0x089A5F68u, 0x089A5F78u); }
SFA_FUNC(089AAE8C, counter_try_aae8c) { counter_try(c, f_089AAED8, 0x089AAEA0u, 0x089AAEB0u); }
SFA_FUNC(089B0A50, counter_try_b0a50) { counter_try(c, f_089B0A9C, 0x089B0A64u, 0x089B0A74u); }
