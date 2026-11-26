/* Steps of 11 - 72 instructions: the hit spark's step table and its combo-voice check (0x0887F380,
   18,446 calls, and 0x0887FAE8), a throw start that knocks a special opponent over in stage 10
   (0x089001B8), a hop that lands beside another object and nudges the opponent (0x08921C9C), stance
   hops into move 0x1C, and a glide with a guard pick. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088801F8(Cpu *c);
void f_088231C8(Cpu *c); void f_08822EAC(Cpu *c); void f_08821330(Cpu *c); void f_0896F62C(Cpu *c);
void f_0896F678(Cpu *c); void f_0886E294(Cpu *c);

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

/* (hit spark): when the combo voice check (0x088801F8) says it counts, 0x088231C8. */
SFA_FUNC(0887FAE8, hit_spark_count) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = ra; *W(c->r[29] + 16) = o;
    c->r[31] = 0x0887FAF8u; f_088801F8(c);
    ARG(0) = *W(c->r[29] + 16);
    if (c->r[2] != 0) { c->r[31] = 0x0887FB08u; f_088231C8(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (hit spark): its step from 0x08A245A0 by +3, then the combo voice check. */
SFA_FUNC(0887F380, hit_spark_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t fn = *W(0x08A245A0u + ((uint32_t)*B(o + 3) << 2));
    ARG(0) = o; ARG(1) = fn;
    c->r[31] = 0x0887F3B0u; rt_call_indirect(c, fn);
    call1(c, o, f_088801F8, 0x0887F3B8u);
    leave(c, sv);
}

/* (fighter): a throw start, +6 counts: against character 0x20 move 0x33 with the match's +61; in stage
   10 against character 10 locked with +430, a knock-over (+6 = 2, +82 = 0x50xx, turned around, still,
   fall -0.14, 240 up, an effect 0x2F tied to it) into move 0x32; else move 0x22. */
SFA_FUNC(089001B8, throw_start_knockover) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
    gaddr opp = *W(o + 92);
    int32_t st = S8(opp + 338);
    uint32_t mv = 0x22;
    if (st == 0x20 && S8(MATCH + 61) != 0) mv = 0x33;
    else if (S16(MATCH) == 10 && st == 10 && S8(opp + 373) != 0 && S8(opp + 430) != 0) {
        *H(o + 82) = (uint16_t)((uint32_t)S16(o + 82) & 0xFF);
        *B(o + 6) = 2;
        *H(o + 82) = (uint16_t)((uint32_t)S16(o + 82) | 0x5000);
        *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
        *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 96) = 0; *W(o + 108) = 0xFFFFDC00u;
        *H(o + 22) = 0xF0;
        ARG(0) = 0x2F; ARG(1) = 0; c->r[31] = 0x08900288u; f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 73) = *B(o + 11);
            *H(e + 86) = (uint16_t)S8(o + 337);
            uint32_t pic = *W(o + 164);
            *W(e + 88) = o; *W(e + 164) = pic;
        }
        mv = 0x32;
    }
    call2(c, o, mv, f_0886163C, 0x089002C4u);
    leave(c, sv);
}

/* (fighter): the +83 wait runs down; past 0 +7 steps on (+653 = -1 when its guard bit is in +238), it
   lands beside 0x08821330's object (by the offset from 0x08A66DB4 for +0x3A6; facing left 384 past
   it), the opponent nudged one step its way when its +238 is set (then cleared), and move 9. */
SFA_FUNC(08921C9C, move_wait_land_beside_9) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[3];
    for (int i = 0; i < 3; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x20;
    gaddr w = o + 0x53;
    int8_t t = (int8_t)(S8(w) - 1);
    *W(c->r[29] + 16) = s[0]; c->r[16] = o;
    *B(w) = (uint8_t)t;
    *W(c->r[29] + 20) = s[1]; *W(c->r[29] + 24) = s[2]; *W(c->r[29] + 28) = ra;
    if (t < 0) {
        uint32_t st = *B(o + 7);
        int32_t g = S8(o + 78);
        int32_t bits = S8(o + 238);
        *B(o + 7) = (uint8_t)(st + 1);
        c->r[17] = 0x08A66DB4u; c->r[18] = o + 0x3A0;
        if (bits & (1 << (g & 31))) *B(o + 653) = 0xFF;
        call1(c, o, f_08821330, 0x08921D18u);
        int32_t d = S16(0x08A66DB4u + (uint32_t)(S8(o + 0x3A6) * 2));
        int32_t ex = S16(c->r[2] + 18);
        c->r[18] = (uint32_t)ex;
        uint32_t x = *B(o + 11) != 0 ? (uint32_t)(d + ex)
                                     : (uint32_t)((int16_t)(-d) + (int16_t)(ex + 0x180));
        gaddr opp = *W(o + 92);
        *H(o + 18) = (uint16_t)x;
        if (S8(opp + 238) != 0) {
            int32_t ox = S16(opp + 18);
            *H(opp + 18) = (uint16_t)(*B(opp + 11) != 0 ? ox + 1 : ox - 1);
            *B(opp + 238) = 0;
        }
        call2(c, o, 9, f_088616EC, 0x08921DA0u);
    }
    for (int i = 0; i < 3; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a pending +123 sets step 2 with 0x0896F678; else 0x0896F62C, and once falling (no choice
   made) or moving back (choice made) +7 steps on into move 0x1C + 2 x strength (still with fall
   -0.4375; moving back a hop of x -3.5 / +0.09); otherwise ticked. */
SFA_FUNC(0896E3B8, move_stance_hop_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t ch = S8(o + 386), f = S8(o + 123);
    if (f != 0) { *B(o + 7) = 2; call1(c, o, f_0896F678, 0x0896E4C4u); }
    else if (ch < 0) {
        call1(c, o, f_0896F62C, 0x0896E3E4u);
        if ((int32_t)*W(o + 100) < 0) {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 96) = 0; *W(o + 108) = 0xFFFF9000u;
            call2(c, o, (uint32_t)(s + s + 0x1C) & 0xFFFF, f_088616EC, 0x0896E428u);
        } else call1(c, o, f_08861848, 0x0896E438u);
    } else {
        call1(c, o, f_0896F62C, 0x0896E450u);
        if ((int32_t)*W(o + 96) < 0) {
            uint32_t st = *B(o + 7);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 96) = 0xFFFC8000u;
            int32_t s = S8(o + 194);
            *W(o + 104) = 0x1800; *W(o + 100) = 0; *W(o + 108) = 0xFFFF9000u;
            call2(c, o, (uint32_t)(s + s + 0x1C) & 0xFFFF, f_088616EC, 0x0896E4A0u);
        } else call1(c, o, f_08861848, 0x0896E4B0u);
    }
    leave(c, sv);
}

/* (fighter): +543 = 2, ticked; a negative frame flag starts a hop (step 3, x and y by strength from
   0x08A6DBA8, brake -0.5, fall -0.375); else flag 2 (cleared) pushes it (x 8.0 / -0.5), and without a
   flag (+237: cleared, +573 from 0x08A6DBB4 by strength or 0x0886E294) it slides by +96 toward its
   facing until that turns. */
SFA_FUNC(089315A8, move_glide_guard_pick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = o; c->r[17] = 2;
    *W(c->r[29] + 24) = ra;
    *B(o + 543) = 2;
    call1(c, o, f_08861848, 0x089315C8u);
    int32_t k = S8(o + 77);
    if (k < 0) {
        *B(o + 7) = 3;
        uint32_t i = (uint32_t)S8(o + 194) << 2;
        *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
        uint32_t v = *W(0x08A6DBA8u + i);
        *W(o + 96) = v; *W(o + 100) = v;
    } else {
        if (k == 2) { *B(o + 77) = 0; *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF8000u; k = S8(o + 77); }
        if (k == 0) {
            if (S8(o + 237) != 0) {
                *B(o + 237) = 0;
                call1(c, o, f_0886E294, 0x08931650u);
                uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
                *B(o + 573) = (uint8_t)S16(0x08A6DBB4u + i + i);
            }
            uint32_t f = *B(o + 11);
            uint32_t x = *W(o + 16), v = *W(o + 96);
            *W(o + 16) = f == 0 ? x - v : x + v;
            uint32_t nv = v + *W(o + 104);
            *W(o + 96) = nv;
            if ((int32_t)nv < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
