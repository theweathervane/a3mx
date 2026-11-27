/* Character move starts no test scenario plays: the counters each character answers a block with,
   move starts behind a command check, and the objects some moves bring out. Written from the original
   code alone. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

void f_0886BF84(Cpu *c); void f_088616EC(Cpu *c); void f_0886C8BC(Cpu *c); void f_0886DBE8(Cpu *c);
void f_0886DC3C(Cpu *c); void f_08863114(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c);
void f_08837098(Cpu *c); void f_08822EAC(Cpu *c); void f_08822F54(Cpu *c); void f_0886DEC8(Cpu *c);
void f_08828C04(Cpu *c); void f_0886C2B4(Cpu *c); void f_0886DD68(Cpu *c); void f_08821330(Cpu *c);
void f_0886163C(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* A 0x20 frame with s0 at +16 and the return address at +20; s0 holds the fighter. */
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
/* The same with s1 as well (+20) and the return address at +24; s1 starts at 0. */
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o; c->r[17] = 0;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
static void state(gaddr o, uint32_t a, uint32_t b, uint32_t d, uint32_t e) {
    *B(o + 4) = (uint8_t)a; *B(o + 5) = (uint8_t)b; *B(o + 6) = (uint8_t)d; *B(o + 7) = (uint8_t)e;
}

/* A counter's start: state 1 / 0 / 7 / 0 with the move, facing the opponent, +235 set, +696 / +127
   cleared, the opponent's +127 marked, the wait +573, optionally +193 and +777, and the animation. A
   0x20 frame with only the return address. */
typedef struct { uint8_t move, mark, wait, anim; int8_t v193; uint8_t set777; } Counter;
static void counter_start(Cpu *c, const Counter *k, uint32_t back) {
    gaddr o = ARG(0), other = *W(o + 92);
    state(o, 1, 0, 7, 0);
    *B(o + 236) = k->move;
    *B(o + 11) = (uint8_t)S8(o + 697);
    *B(o + 235) = 1; *B(o + 696) = 0; *B(o + 127) = 0;
    *B(other + 127) = k->mark;
    *B(o + 573) = k->wait;
    if (k->v193 >= 0) *B(o + 193) = (uint8_t)k->v193;
    if (k->set777) *B(o + 777) = 1;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(1) = k->anim; c->r[31] = back; f_088616EC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(0899B32C, counter_start_9b32c) { static const Counter k = { 3, 0x18, 0x1C, 5, -1, 0 }; counter_start(c, &k, 0x0899B388u); }
SFA_FUNC(089151D0, counter_start_151d0) { static const Counter k = { 7, 0x16, 0x1A, 4, -1, 1 }; counter_start(c, &k, 0x08915230u); }
SFA_FUNC(0891523C, counter_start_1523c) { static const Counter k = { 5, 0x15, 0x19, 3, -1, 1 }; counter_start(c, &k, 0x0891529Cu); }
SFA_FUNC(089844E0, counter_start_844e0) { static const Counter k = { 7, 0x15, 0x19, 7, -1, 1 }; counter_start(c, &k, 0x08984540u); }
SFA_FUNC(0899EE54, counter_start_9ee54) { static const Counter k = { 3, 0x18, 0x1C, 0x36, 0, 1 }; counter_start(c, &k, 0x0899EEB8u); }
SFA_FUNC(089A36EC, counter_start_a36ec) { static const Counter k = { 3, 0x18, 0x1C, 0x1F, 1, 1 }; counter_start(c, &k, 0x089A3750u); }

/* (fighter): move 1 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent, +235 cleared: 1; else
   0. A frame keeping s0, s1. */
SFA_FUNC(08984250, move_1_try) {
    gaddr o = ARG(0); uint32_t sv[3]; enter2(c, o, sv);
    c->r[31] = 0x0898426Cu; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0;
        int8_t facing = S8(o + 697);
        *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 11) = (uint8_t)facing; *B(o + 235) = 0;
        c->r[17] = 1;
        *B(o + 236) = 1;
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* (fighter): move 12 behind 0x0886C8BC: state 1 / 0 / 7 / 0, +235 set and 0x0886DBE8: 1; else 0. A
   frame keeping s0, s1. */
SFA_FUNC(08909C50, move_12_try) {
    gaddr o = ARG(0); uint32_t sv[3]; enter2(c, o, sv);
    c->r[31] = 0x08909C6Cu; f_0886C8BC(c);
    if (c->r[2] != 0) {
        state(o, 1, 0, 7, 0);
        *B(o + 236) = 0xC;
        c->r[17] = 1;
        *B(o + 235) = 1;
        call1(c, o, f_0886DBE8, 0x08909CA4u);
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* (fighter): step 2 with +193 set, 0x0886DC3C, 31 further on by its facing, 0x08863114 and the
   animation 0x23. */
SFA_FUNC(0890F268, step_2_shift_31_anim_23) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 2; *B(o + 193) = 1;
    *W(c->r[29] + 20) = sv[1];
    call1(c, o, f_0886DC3C, 0x0890F290u);
    uint32_t x = (uint32_t)S16(o + 18) + (*B(o + 11) != 0 ? 0x1Fu : 0xFFE0u);
    *H(o + 18) = (uint16_t)x;
    call2(c, o, x, f_08863114, 0x0890F2B8u);
    call2(c, o, 0x23, f_08861674, 0x0890F2C4u);
    leave(c, sv);
}

/* (fighter): step 2 with +262 set, the meter by 3 (1 once +836 is set), 0x08837098 and the animation
   0x10. */
SFA_FUNC(0898B67C, step_2_meter_anim_10) {
    gaddr o = ARG(0); uint32_t sv[2];
    *B(o + 7) = 2;
    uint32_t again = *W(o + 836);
    *B(o + 262) = 1;
    enter(c, o, sv);
    if (again == 0) call2(c, o, 3, f_0886CA2C, 0x0898B6B0u);
    else call2(c, o, 1, f_0886CA2C, 0x0898B6C4u);
    call1(c, o, f_08837098, 0x0898B6CCu);
    call2(c, o, 0x10, f_088616EC, 0x0898B6D8u);
    leave(c, sv);
}

/* (fighter): a new object of kind 0x22 / 0 owned by the fighter, with its +86, +164 and +148: 1; 0
   with no room. */
SFA_FUNC(0898DAAC, spawn_22_like_owner) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call2(c, 0x22, 0, f_08822EAC, 0x0898DAC8u);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = o;
        *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
        *W(n + 148) = *W(o + 148);
        c->r[2] = 1;
    } else c->r[2] = 0;
    leave(c, sv);
}

/* (game, battle): who is ahead into the battle's +12: 1 or 2 by the two players' vitality in line-up
   0 (other line-ups compare the line-up's number with what the caller left in a3); level: 1 when the
   game's +53 is 2, else 2. No frame. */
SFA_FUNC(08828308, ahead_by_vitality) {
    gaddr g = ARG(0), b = ARG(1);
    int32_t second = S16(b + 56), first = (int32_t)c->r[7];
    if (second == 0) { first = S16(FIGHTER_1 + 112); second = S16(FIGHTER_2 + 112); }
    if (first == second) *B(b + 12) = S8(g + 53) == 2 ? 1 : 2;
    else *B(b + 12) = second < first ? 1 : 2;
}

/* Two new pieces of kind 0x1F / 0 owned by the fighter, hidden: the first (+10 = 0) kept in the
   fighter's +48, the second (+10 = 1) in +44; none after one that found no room. */
static void two_pieces(Cpu *c, uint32_t back, uint32_t back2) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call2(c, 0x1F, 0, f_08822F54, back);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 1) = 0; *B(n + 10) = 0; *W(n + 88) = o; *W(o + 48) = n;
        call2(c, 0x1F, 0, f_08822F54, back2);
        n = c->r[2];
        if (n != 0) { *B(n + 1) = 0; *B(n + 10) = 1; *W(n + 88) = o; *W(o + 44) = n; }
    }
    leave(c, sv);
}
SFA_FUNC(088F1AD4, two_pieces_f1ad4) { two_pieces(c, 0x088F1AF0u, 0x088F1B18u); }
SFA_FUNC(08966370, two_pieces_66370) { two_pieces(c, 0x0896638Cu, 0x089663B4u); }

/* (fighter): step 6: 4.0 up with gravity 0x5800 and 2.0 across by its facing, the meter by 8, the
   animation 0x2F and 0x0886DEC8. */
SFA_FUNC(088FA880, step_6_hop_anim_2f) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 6;
    *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA800u; *W(o + 104) = 0;
    uint8_t facing = *B(o + 11);
    *W(c->r[29] + 20) = sv[1];
    *W(o + 96) = facing != 0 ? 0x20000u : 0xFFFE0000u;
    call2(c, o, 8, f_0886CA2C, 0x088FA8CCu);
    call2(c, o, 0x2F, f_088616EC, 0x088FA8D8u);
    call1(c, o, f_0886DEC8, 0x088FA8E0u);
    leave(c, sv);
}

/* (fighter): move 10 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent and 0x08828C04: 1;
   else 0. */
SFA_FUNC(0892A6D8, move_10_try) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x0892A6ECu; f_0886BF84(c);
    if (c->r[2] != 0) {
        state(o, 1, 0, 7, 0);
        int8_t facing = S8(o + 697);
        *B(o + 236) = 0xA; *B(o + 11) = (uint8_t)facing;
        call1(c, o, f_08828C04, 0x0892A720u);
        c->r[2] = 1;
    } else c->r[2] = 0;
    leave(c, sv);
}

/* (fighter): with +251 clear, move 0 behind 0x0886C2B4: state 1 / 0 / 8 / 0 and the timings by the
   buttons (0x0886DD68): 1; else 0. */
SFA_FUNC(08943448, move_0_try_8) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t off = S8(o + 251);
    enter(c, o, sv);
    uint32_t done = 0;
    if (off == 0) {
        call1(c, o, f_0886C2B4, 0x08943468u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 8, 0);
            *B(o + 236) = 0;
            call1(c, o, f_0886DD68, 0x08943490u);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* (fighter): the next step, +100 taking +16, 0x08821330, facing by its side (+337), set 0x30 or
   0x150 across from what that returned (+18), and the cue 0x29. */
SFA_FUNC(0894885C, next_step_place_cue_29) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    uint8_t step = *B(o + 7);
    uint32_t x = *W(o + 16);
    *B(o + 7) = (uint8_t)(step + 1);
    *W(o + 100) = x;
    *W(c->r[29] + 20) = sv[1];
    call1(c, o, f_08821330, 0x08948888u);
    int32_t side = S8(o + 337);
    *B(o + 11) = (uint8_t)side;
    int32_t at = S16(c->r[2] + 18);
    *H(o + 18) = (uint16_t)(((uint32_t)side & 0xFF) != 0 ? at + 0x30 : at + 0x150);
    call2(c, o, 0x29, f_0886163C, 0x089488BCu);
    leave(c, sv);
}

/* (fighter): with +251 or +254, move 9 behind 0x0886BF84: state 1 / 0 / 7 / 0 facing the opponent
   and 0x08828C04. */
SFA_FUNC(0896CAC8, move_9_try_251) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t a = S8(o + 251);
    enter(c, o, sv);
    if (a != 0 || S8(o + 254) != 0) {
        call1(c, o, f_0886BF84, 0x0896CAF4u);
        if (c->r[2] != 0) {
            state(o, 1, 0, 7, 0);
            int8_t facing = S8(o + 697);
            *B(o + 236) = 9; *B(o + 11) = (uint8_t)facing;
            call1(c, o, f_08828C04, 0x0896CB28u);
        }
    }
    leave(c, sv);
}
