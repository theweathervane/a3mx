/* The fighter animation tick (the most called function of a fight, 320,040 calls in the profile) and
   its neighbours: a move picked by list, hit checks that play the hit cues, a hit spark's start and a
   burst effect's end. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861900(Cpu *c); void f_08861AE0(Cpu *c); void f_0893A724(Cpu *c); void f_0886E3CC(Cpu *c);
void f_0893A760(Cpu *c); void f_0893A7E4(Cpu *c); void f_0887684C(Cpu *c); void f_088768F0(Cpu *c);
void f_08873BF0(Cpu *c); void f_0886AAE8(Cpu *c); void f_0887DFB8(Cpu *c); void f_08823338(Cpu *c);

#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u

/* (fighter): the step's frame count (+74) runs down. At 0 an ending step (+1 bit 6) ends the
   animation (+75 = -1), else the animation jumps by the step's +6 (10 bytes a step) and is applied
   (0x08861900). With one frame left an ending step ends it already. */
SFA_FUNC(08861848, fighter_anim_tick_main) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int8_t t = (int8_t)(S8(o + 74) - 1);
    gaddr st = *W(o + 28);
    *W(c->r[29] + 16) = ra;
    *B(o + 74) = (uint8_t)t;
    int32_t f = S8(st + 1);
    if (t != 0) {
        if (S8(o + 74) == 1 && (f & 0x40)) *B(o + 75) = 0xFF;
    } else if (f & 0x40) *B(o + 75) = 0xFF;
    else {
        *W(o + 28) = st + (uint32_t)(S16(st + 6) * 10);
        c->r[31] = (f & 0x80) ? 0x088618E0u : 0x088618F4u;
        f_08861900(c);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, n, list): move n (+84) from the character's list 0 - 3 (+416 +0 / +4 / +8 / +12) set
   (0x08861AE0); any other list sets step 0. */
SFA_FUNC(08861A38, fighter_anim_from_list) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1) & 0xFFFF, which = ARG(2) & 0xFFFF;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr st = 0;
    if (which < 4) st = *W(*W(*W(o + 416) + (which << 2)) + (n << 2));
    *H(o + 84) = (uint16_t)n;
    ARG(1) = st; c->r[31] = 0x08861AD4u; f_08861AE0(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x0893A724; while its animation runs (+75 >= 0) 0x088768F0; when it ends, 0x0886E3CC (its
   owner, 9, 12), +6 steps on, the 0x3B effects spawned (one at the owner, two at random offsets) and
   0x0887684C with 12 + its +346. */
SFA_FUNC(0893A5F8, burst_effect_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x0893A60Cu; f_0893A724(c);
    if (S8(o + 75) >= 0) { ARG(0) = o; c->r[31] = 0x0893A678u; f_088768F0(c); }
    else {
        ARG(0) = *W(o + 88); ARG(1) = 9; ARG(2) = 0xC;
        c->r[31] = 0x0893A628u; f_0886E3CC(c);
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        ARG(0) = o; c->r[31] = 0x0893A63Cu; f_0893A760(c);
        ARG(0) = o; c->r[31] = 0x0893A644u; f_0893A7E4(c);
        ARG(0) = o; c->r[31] = 0x0893A64Cu; f_0893A7E4(c);
        ARG(0) = o; ARG(1) = (uint32_t)(S8(o + 346) + 0xC) & 0xFFFF;
        c->r[31] = 0x0893A660u; f_0887684C(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The hit test (0x08873BF0) of the attacker's box k (+224, 32 bytes each) against the fighter's box
   set (+220, 8 bytes per entry, entry `ix`); on a hit the hit cues play (0x0886AAE8) and 1 is
   returned, else 0. */
static void hit_test(Cpu *c, gaddr a, gaddr f, uint32_t k, uint32_t ix, uint32_t back1, uint32_t back2) {
    gaddr box = *W(a + 224) + (k << 5);
    c->r[18] = box;
    ARG(0) = a; ARG(1) = f; ARG(2) = box; ARG(3) = *W(f + 220) + ix * 8;
    c->r[31] = back1; f_08873BF0(c);
    if (c->r[2] == 0) return;
    ARG(0) = a; ARG(1) = f; ARG(2) = box;
    c->r[31] = back2; f_0886AAE8(c);
    c->r[2] = 1;
}

static void enter3(Cpu *c, gaddr a, gaddr f, uint32_t sv[4]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[18]; sv[3] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[17] = a; c->r[16] = f;
    *W(c->r[29] + 24) = sv[2]; *W(c->r[29] + 28) = sv[3];
    c->r[18] = ARG(2);
}
static void leave3(Cpu *c, const uint32_t sv[4]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = sv[3];
    c->r[29] += 0x20;
}

/* (attacker, fighter, k): unless the fighter is out of reach (+135), the hit test against the
   fighter's current frame's box set (+256 +3). */
SFA_FUNC(0886AFB0, hit_check_box) {
    gaddr a = ARG(0), f = ARG(1);
    uint32_t k = ARG(2), sv[4];
    enter3(c, a, f, sv);
    c->r[2] = 0;
    if (S8(f + 135) == 0) hit_test(c, a, f, k, *B(*W(f + 256) + 3), 0x0886B00Cu, 0x0886B020u);
    leave3(c, sv);
}

/* (attacker, fighter, k): as above, not while the battle is paused (+14) or the fighter is out of the
   fight (+73) or reach (+135), and only for a frame with a hurt box (+256 +0 - +2) and a set (+3). */
SFA_FUNC(0886A9FC, hit_check_hurt) {
    gaddr a = ARG(0), f = ARG(1);
    uint32_t k = ARG(2), sv[4];
    int32_t off = S8(BATTLE + 14) | S8(f + 73) | S8(f + 135);
    enter3(c, a, f, sv);
    c->r[2] = 0;
    if (off == 0) {
        gaddr p = *W(f + 256);
        if ((*B(p) != 0 || *B(p + 1) != 0 || *B(p + 2) != 0) && *B(p + 3) != 0)
            hit_test(c, a, f, k, *B(p + 3), 0x0886AA9Cu, 0x0886AAB0u);
    }
    leave3(c, sv);
}

/* (spark): started: +4 steps on, +86 = 5, +24 = 7, +72 = 1, +14 from its owner (+88), the hit voice
   (0x0887DFB8); with +82 and neither fighter's +320, effect 4 / 3 (0x08823338) is tied to it (+124);
   +38 / +12 / +13 cleared and its kind's (+3) first step run (0x08A24284 table). */
SFA_FUNC(0887D57C, spark_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 86) = 5; *H(o + 24) = 7;
    gaddr ow = *W(o + 88);
    *B(o + 72) = 1;
    *B(o + 14) = *B(ow + 14);
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; ARG(1) = ow; c->r[31] = 0x0887D5C8u; f_0887DFB8(c);
    if (S16(o + 82) != 0 && (S8(P1 + 320) | S8(P2 + 320)) == 0) {
        ARG(0) = 4; ARG(1) = 3; c->r[31] = 0x0887D5FCu; f_08823338(c);
        *W(c->r[2] + 124) = o;
    }
    uint32_t kind = *B(o + 3);
    *H(o + 38) = 0; *B(o + 12) = 0; *B(o + 13) = 0;
    ARG(0) = o; c->r[31] = 0x0887D62Cu;
    rt_call_indirect(c, *W(0x08A24284u + (kind << 2)));
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
