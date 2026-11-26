/* Small functions the scenarios reach, 49 - 61 instructions: a palette animation's start from its
   timer's lists, fighters' move starts, a round-start object. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u
#define PART1  0x092E47D0u
#define PART2  0x092E4C68u

void f_088231C8(Cpu *c); void f_089CD120(Cpu *c); void f_0886BF84(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DBE8(Cpu *c); void f_0886DB94(Cpu *c); void f_08822EAC(Cpu *c); void f_0887EDD8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (animation, timer): once the timer's +2 reaches its +118 it starts (+4 on): with a list (timer +292)
   its rows (+88) and steps (timer +296, by +116: +121, +73, then the frames from +92 with the first
   count at +82) and the first step (0x089CD120); without one it is freed (0x088231C8). */
SFA_FUNC(089CD05C, palette_anim_start) {
    gaddr o = ARG(0), t = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (!(S16(t + 2) < (int16_t)*B(o + 118))) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        gaddr list = *W(t + 292);
        if (*W(list) == 0) call_on(c, o, 0x089CD114u, f_088231C8);
        else {
            uint32_t idx = (uint32_t)*B(o + 116) << 2;
            *W(o + 88) = *W(list + idx);
            gaddr q = *W(*W(t + 296) + idx);
            *B(o + 121) = (uint8_t)S8(q);
            *B(o + 73) = (uint8_t)S8(q + 1);
            *W(o + 92) = q + 2;
            *H(o + 82) = (uint16_t)(int32_t)S8(q + 2);
            call2(c, o, t, 0x089CD100u, f_089CD120);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The common tail: step 1/0/7, +235 = 1 and its start. */
static void go7(Cpu *c, gaddr f, void (*start)(Cpu *), uint32_t back) {
    *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
    *B(f + 235) = 1;
    call_on(c, f, back, start);
}

/* (fighter): with +386 negative, in the air once 0x0886C2B4 allows it (+192 = 2) or on the ground once
   0x0886BF84 does (+192 = 0, +11 = +697): +236 = 2 and the move; 1 then, else 0. */
SFA_FUNC(08901BFC, fighter_move_start_192) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    int ok = 0;
    if (S8(f + 386) < 0) {
        if (S8(f + 73) != 0) {
            call_on(c, f, 0x08901C2Cu, f_0886C2B4);
            if ((ok = c->r[2] != 0)) *B(f + 192) = 2;
        } else {
            call_on(c, f, 0x08901C88u, f_0886BF84);
            if ((ok = c->r[2] != 0)) { *B(f + 192) = 0; *B(f + 11) = (uint8_t)S8(f + 697); }
        }
    }
    uint32_t r = 0;
    if (ok) {
        c->r[17] = 1;
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
        *B(f + 236) = 2;
        *B(f + 235) = 1;
        call_on(c, f, 0x08901C68u, f_0886DBE8);
        r = 1;
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): once per round (BATTLE +84), a list-6 object (0x08822EAC, BATTLE +43) with BATTLE +43 / +68
   (as +3 / +73), the owner (+92) and its side (+78), set up (0x0887EDD8); BATTLE +84 = -1, +36 = -0x70,
   +96 and the four fighters' +590 cleared; 1 then, else 0. */
SFA_FUNC(0887E884, round_object_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2;
    c->r[18] = BATTLE;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    if (S8(BATTLE + 84) == 0) {
        call2(c, 6, (uint32_t)S8(BATTLE + 43) & 0xFF, 0x0887E8C0u, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *B(n + 3) = (uint8_t)S8(BATTLE + 43);
            *B(n + 73) = (uint8_t)S8(BATTLE + 68);
            *W(n + 92) = o;
            *B(n + 78) = *B(o + 11);
            *B(BATTLE + 84) = 0xFF;
            *B(BATTLE + 36) = 0x90;
            *B(BATTLE + 96) = 0;
            *B(PART2 + 590) = 0; *B(PART1 + 590) = 0; *B(P2 + 590) = 0; *B(P1 + 590) = 0;
            call_on(c, n, 0x0887E930u, f_0887EDD8);
            c->r[17] = 1;
        }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in the air with +78 above 0 and +386 = 1, at least 0x40 (0x58 falling) above its ground
   (+132), once 0x0886C2B4 allows it: step 1/0/7, +236 = 9, +235 = 1, +11 = +697, 0x0886DBE8; 1 then,
   else 0. */
SFA_FUNC(0895C6D8, fighter_air_move_9) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    uint32_t r = 0;
    if (S8(f + 73) != 0 && S8(f + 78) > 0) {
        c->r[17] = 1;
        if (S8(f + 386) == 1) {
            int32_t lim = (int32_t)*W(f + 100) < 0 ? 0x58 : 0x40;
            if (!(S16(f + 22) < S16(f + 132) + lim)) {
                call_on(c, f, 0x0895C740u, f_0886C2B4);
                if (c->r[2] != 0) {
                    *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
                    *B(f + 236) = 9;
                    *B(f + 235) = 1;
                    *B(f + 11) = (uint8_t)S8(f + 697);
                    call_on(c, f, 0x0895C778u, f_0886DBE8);
                    r = 1;
                }
            }
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): on the ground once 0x0886BF84 allows it (+236 = 8), in the air with +78 above 0 and over
   0x40 (0x58 falling) above its ground once 0x0886C2B4 does (+236 = 9): step 1/0/7, +235 = 1, +11 =
   +697, 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(089944C0, fighter_move_start_89) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    int ok = 0;
    if (S8(f + 73) == 0) {
        call_on(c, f, 0x089944E4u, f_0886BF84);
        if ((ok = c->r[2] != 0)) *B(f + 236) = 8;
    } else if (S8(f + 78) > 0) {
        int32_t lim = (int32_t)*W(f + 100) >= 0 ? 0x40 : 0x58;
        if (S16(f + 132) + lim < S16(f + 22)) {
            call_on(c, f, 0x08994574u, f_0886C2B4);
            if ((ok = c->r[2] != 0)) *B(f + 236) = 9;
        }
    }
    uint32_t r = 0;
    if (ok) {
        c->r[17] = 1;
        uint8_t side = (uint8_t)S8(f + 697);
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
        *B(f + 235) = 1;
        *B(f + 11) = side;
        call_on(c, f, 0x08994520u, f_0886DBE8);
        r = 1;
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): with few effects (none, or under two with +251), in the air over 0x30 above its ground once
   0x0886C2B4 allows it (+236 = 4), on the ground once 0x0886BF84 does (+236 = 1, +11 = +697): step 1/0/7,
   +235 = 1, 0x0886DB94; 1 then, else 0. */
SFA_FUNC(088F7CD0, fighter_move_start_few_41) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(f + 536);
    int few = S8(f + 251) != 0 ? (int32_t)n < 2 : n == 0;
    int ok = 0;
    if (few) {
        if (S8(f + 73) != 0) {
            call_on(c, f, 0x088F7D38u, f_0886C2B4);
            if (c->r[2] != 0 && S16(f + 132) + 0x30 < S16(f + 22)) { ok = 1; *B(f + 236) = 4; }
        } else {
            call_on(c, f, 0x088F7D70u, f_0886BF84);
            if (c->r[2] != 0) { ok = 1; *B(f + 236) = 1; *B(f + 11) = (uint8_t)S8(f + 697); }
        }
    }
    uint32_t r = 0;
    if (ok) { c->r[17] = 1; go7(c, f, f_0886DB94, 0x088F7DACu); r = 1; }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
