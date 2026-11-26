/* The fighter's per-frame step (0x08859C58, 289,564 calls) and a mash-to-extend hop (0x0894A0B0).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_088619EC(Cpu *c); void f_08862DF8(Cpu *c);
void f_088772C0(Cpu *c); void f_0894CAF4(Cpu *c); void f_08877258(Cpu *c); void f_0887C408(Cpu *c);
void f_0894A35C(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void indirect(Cpu *c, gaddr o, uint32_t fn, uint32_t back) {
    ARG(0) = o; ARG(1) = fn; c->r[31] = back; rt_call_indirect(c, fn);
}

/* (fighter): during hit-stop (+127 counting down) 0x088619EC (and character 4's own hook from +416
   +140); otherwise its step by +6 from 0x08A19118, except that a locked (+373) fighter outside a
   super (+553) while the round runs first goes through 0x088772C0 (unless it is in stance 6 of the
   1 / 0 / 7 state of characters 6 / 0xF / 0x14), and in state 1 / 2 then runs 0x08A17FA0's step for
   it instead. Then 0x08862DF8. */
SFA_FUNC(08859C58, fighter_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t k = S8(o + 127);
    if (k != 0) {
        *B(o + 127) = (uint8_t)(k - 1);
        call1(c, o, f_088619EC, 0x08859C80u);
        if (S8(o + 338) == 4) indirect(c, o, *W(*W(*W(o + 416) + 140)), 0x08859CA4u);
    } else {
        if (S8(o + 373) != 0 && S8(o + 553) == 0
            && (S8(MATCH + 14) | S8(MATCH + 13) | S8(GAME + 38)) == 0) {
            int skip = 0;
            if (*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 7) {
                int32_t st = S8(o + 338);
                if ((st == 6 || st == 0x14 || st == 0xF) && S8(o + 236) == 6) skip = 1;
            }
            if (!skip) {
                call1(c, o, f_088772C0, 0x08859D70u);
                if (*B(o + 4) == 1 && *B(o + 5) == 2) {
                    indirect(c, o, *W(0x08A17FA0u + ((uint32_t)*B(o + 5) << 2)), 0x08859DD0u);
                    leave(c, sv);
                    return;
                }
            }
        }
        indirect(c, o, *W(0x08A19118u + ((uint32_t)*B(o + 6) << 2)), 0x08859DB0u);
    }
    call1(c, o, f_08862DF8, 0x08859CACu);
    leave(c, sv);
}

/* (fighter): 0x0894CAF4; when the +82 wait is out step 5, fall -0.375 and move 0x2E. Else it counts
   down (+820 counting the frames) and a pending +123 with the strength's button (0x08A790D8) held, or
   (locked) late in the wait just below the opponent with 0x08877258 (0x08A78DF0), adds a mash (+199):
   the wait shortened by the frames counted, voice 0x208, +820 / +123 cleared, x 0.75, y 1.56, and on
   the last mash (+198) +7 steps on with x brake -0.125 and fall -0.3125; 0x0894A35C. Otherwise
   ticked. */
SFA_FUNC(0894A0B0, move_mash_hop) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr opp = *W(o + 92);
    c->r[17] = opp;
    call1(c, o, f_0894CAF4, 0x0894A0CCu);
    int32_t t = S16(o + 82);
    if (t == 0) {
        *B(o + 7) = 5; *W(o + 108) = 0xFFFFA000u;
        call2(c, o, 0x2E, f_088616EC, 0x0894A15Cu);
    } else {
        uint32_t n = *W(o + 820);
        *H(o + 82) = (uint16_t)(t - 1);
        int32_t f = S8(o + 123);
        *W(o + 820) = n + 1;
        int mash = 0;
        if (f != 0) {
            if (S8(o + 373) == 0) {
                uint32_t mask = *H(0x08A790D8u + (uint32_t)(S8(o + 194) * 2));
                mash = (*H(o + 760) & mask) != 0;
            } else if (S16(o + 82) < 6 && S16(opp + 22) - S16(o + 22) < 16) {
                call2(c, o, 0x08A78DF0u, f_08877258, 0x0894A1A0u);
                mash = c->r[2] != 0;
            }
        }
        if (!mash) call1(c, o, f_08861848, 0x0894A1B0u);
        else {
            int32_t m = S8(o + 199);
            uint32_t cnt = *W(o + 820);
            *B(o + 199) = (uint8_t)(m + 1);
            *H(o + 82) = (uint16_t)(cnt != 0 ? cnt - 1 : cnt);
            ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x208; c->r[31] = 0x0894A1DCu; f_0887C408(c);
            *W(o + 820) = 0;
            int32_t last = S8(o + 198);
            *B(o + 123) = 0;
            if (last == S8(o + 199)) {
                uint32_t st = *B(o + 7);
                *B(o + 7) = (uint8_t)(st + 1);
                *W(o + 104) = 0xFFFFE000u; *W(o + 108) = 0xFFFFB000u;
            }
            *W(o + 96) = 0xC000; *W(o + 100) = 0x19000;
            call1(c, o, f_0894A35C, 0x0894A22Cu);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
