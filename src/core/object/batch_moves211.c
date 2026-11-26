/* Steps of 109 - 110 instructions: the win-pose picker by +692 that walks toward a facing opponent of
   character 1 or picks at random (0x088ED874), and a menu value stepped by left / right with key
   repeat (0x088CD120). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886163C(Cpu *c); void f_08821368(Cpu *c); void f_0887C4CC(Cpu *c);

#define MATCH 0x092E1228u

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

/* (fighter): a win pose, +6 counts, +82 = 60, by +692: 8 (from 6, or with +828) move 0x26; 1 - 4 / 7+
   their move from 0x08A439D0; 0 / 5: against a facing character 1 at 112 or more in a timed round (the
   match's +5, not over, +366 clear) a walk toward it (pose 7, x 3.0, move 2), else (also 0 against a
   locked opponent) a random pose from 0x08A439C0 and its move. */
SFA_FUNC(088ED874, win_pose_pick) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    gaddr opp = *W(o + 92);
    *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
    int32_t m = S8(o + 692);
    *H(o + 82) = 0x3C;
    int walk_check = 0, pick = 0;
    if (m == 5) walk_check = 1;
    else if (m == 0 || m == 6) {
        if (*W(o + 828) != 0 || m == 6) {
            *B(o + 692) = 8;
            call2(c, o, 0x26, f_0886163C, 0x088ED8F8u);
            leave(c, sv);
            return;
        }
        if (S8(opp + 373) != 0) pick = 1; else walk_check = 1;
    } else {
        call2(c, o, *H(0x08A439D0u + (uint32_t)(m + m)), f_0886163C, 0x088EDA18u);
        leave(c, sv);
        return;
    }
    if (walk_check) {
        int32_t x = S16(o + 18), ox = S16(opp + 18);
        int32_t d = x - ox;
        if (d < 0) d = ox - x;
        if (S8(MATCH + 13) != 0 || S8(opp + 338) != 1 || S8(MATCH + 5) == 0 || S16(o + 366) != 0
            || *B(o + 11) == *B(opp + 11) || d + 0x70 < 225) pick = 1;
        else {
            *B(o + 692) = 7;
            *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 108) = 0;
            if (S16(opp + 18) < x) { *B(o + 11) = 0; *W(o + 96) = 0xFFFD0000u; }
            else { *B(o + 11) = 1; *W(o + 96) = 0x30000; }
            call2(c, o, 2, f_0886163C, 0x088ED9C8u);
            leave(c, sv);
            return;
        }
    }
    if (pick) {
        ARG(0) = opp; c->r[31] = 0x088ED9E0u; f_08821368(c);
        *B(o + 692) = *B(0x08A439C0u + (c->r[2] & 0xF));
        int32_t p = S8(o + 692);
        call2(c, o, *H(0x08A439D0u + (uint32_t)(p + p)), f_0886163C, 0x088EDA18u);
    }
    leave(c, sv);
}

/* (pad at ARG0, value, top): the pad's 44 bytes copied to the stack; a press of left / right (+12 bits
   1 / 0) steps the value down / up with wrap (cursor sound 0x38, repeat delay 20 at 0x08B02E76), held
   (+4 bits 1 / 0) it steps every 6 frames once the delay runs out. Gives the value. */
SFA_FUNC(088CD120, menu_value_repeat) {
    gaddr pad = ARG(0);
    int32_t v = (int32_t)ARG(1);
    int32_t top = (int8_t)(ARG(2) & 0xFF);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x80;
    gaddr sp = c->r[29];
    *W(sp + 108) = s1;
    for (int k = 0; k < 11; k++) *W(sp + 60 + 4 * k) = *W(pad + 4 * k);
    c->r[17] = (uint32_t)top;
    uint32_t btn = *W(sp + 72);
    *W(sp + 104) = s0; *W(sp + 112) = s2; *W(sp + 116) = ra;
    c->r[16] = (uint32_t)v;
    gaddr rep = 0x08B02E70u;
    if (btn & 2) {
        ARG(0) = 0; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = 0x088CD1B4u; f_0887C4CC(c);
        v -= 1; *B(rep + 6) = 0x14;
        if (v < 0) v = top;
    } else if (btn & 1) {
        ARG(0) = 0; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = 0x088CD1ECu; f_0887C4CC(c);
        v += 1; *B(rep + 6) = 0x14;
        if (top < v) v = 0;
    } else {
        uint32_t hold = *W(sp + 64);
        if (hold & 3) {
            c->r[18] = rep;
            int8_t t = (int8_t)(S8(rep + 6) - 1);
            *B(rep + 6) = (uint8_t)t;
            if (t < 0) {
                if (hold & 2) {
                    ARG(0) = 0; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = 0x088CD254u; f_0887C4CC(c);
                    v -= 1; *B(rep + 6) = 6;
                    if (v < 0) v = top;
                } else {
                    ARG(0) = 0; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = 0x088CD2A0u; f_0887C4CC(c);
                    v += 1; *B(rep + 6) = 6;
                    if (top < v) v = 0;
                }
            }
        } else c->r[18] = 0x08B00000u;
    }
    c->r[2] = (uint32_t)v;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x80;
}
