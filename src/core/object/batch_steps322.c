/* More of what the small ones set free: the block's second step, a character's punch move by
   +192, and a held move's player side. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0885CA48(Cpu *c); void f_08882634(Cpu *c); void f_0885CC1C(Cpu *c); void f_08861724(Cpu *c);
void f_0885CD4C(Cpu *c); void f_0886D678(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c);
void f_08861674(Cpu *c); void f_089013B0(Cpu *c); void f_08901688(Cpu *c); void f_089049E4(Cpu *c);
void f_08864888(Cpu *c); void f_08861848(Cpu *c); void f_089028AC(Cpu *c); void f_088632C4(Cpu *c);
void f_0886DC3C(Cpu *c); void f_08837098(Cpu *c); void f_0886E3CC(Cpu *c); void f_088616EC(Cpu *c);
void f_0886175C(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (fighter): a hit is blocked: step 2, facing as +697 says, the count +647 up. With the guard
   (+556) below what the hit takes (+557) the block lands hard (0x0885CA48). Otherwise, on the
   ground (+116 = 1, +82 cleared): within 16 of breaking, +656 = 0xD and the copy object
   (0x08882634); 0x0885CC1C; then the animation: a player's by its own crouch (0xF or 0xC); the
   cpu's by the hit: crouching (+696 set, 0xF) against the opponent's +310 or a crouching opponent
   on the ground, else standing (0xC), and with its +127 clear 0x0885CD4C. In the air (+116 = 2,
   +73 = -1, +543 = 10) the animation 0x12 and the same 0x0885CD4C. A frame keeping s0, s1. */
SFA_FUNC(0885C8A8, block_second) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    gaddr other = *W(o + 92);
    c->r[17] = other;
    int8_t facing = (int8_t)S8(o + 697);
    int32_t count = S8(o + 647);
    *B(o + 7) = 2;
    *B(o + 11) = (uint8_t)facing;
    int32_t guard = S8(o + 556);
    int32_t air = S8(o + 73);
    *B(o + 647) = (uint8_t)(count + 1);
    int32_t takes = S8(o + 557);
    int hard = guard < takes;
    if (air == 0) {
        *B(o + 116) = 1;
        *H(o + 82) = 0;
        if (hard) call1(c, o, f_0885CA48, 0x0885C908u);
        else {
            if (guard - takes < 17) {
                *B(o + 656) = 0xD;
                call1(c, o, f_08882634, 0x0885C92Cu);
            }
            call1(c, o, f_0885CC1C, 0x0885C934u);
            if (S8(o + 373) == 0) {
                if (S8(o + 696) != 0) call2(c, o, 0xF, f_08861724, 0x0885C968u);
                else call2(c, o, 0xC, f_08861724, 0x0885C958u);
            } else {
                if (S8(other + 310) != 0) {
                    *B(o + 696) = 1;
                    call2(c, o, 0xF, f_08861724, 0x0885C988u);
                } else if (S8(other + 73) == 0 && S8(other + 696) != 0) {
                    *B(o + 696) = 1;
                    call2(c, o, 0xF, f_08861724, 0x0885C9B8u);
                } else {
                    *B(o + 696) = 0;
                    call2(c, o, 0xC, f_08861724, 0x0885C9D0u);
                }
                if (S8(o + 127) == 0) call1(c, o, f_0885CD4C, 0x0885C9E4u);
            }
        }
    } else {
        *B(o + 116) = 2;
        *B(o + 73) = 0xFF;
        *B(o + 543) = 0xA;
        if (hard) call1(c, o, f_0885CA48, 0x0885CA0Cu);
        else {
            call2(c, o, 0x12, f_08861724, 0x0885CA20u);
            if (S8(o + 127) == 0) call1(c, o, f_0885CD4C, 0x0885CA34u);
        }
    }
    leave2(c, sv);
}

/* The usual first step of a move: facing as +697 says, +235 set, 0x08863114 and 0x0886CA2C with
   the strength. */
static void first_step(Cpu *c, gaddr o, uint32_t back0, uint32_t back1) {
    int8_t facing = (int8_t)S8(o + 697);
    *B(o + 7) = 1;
    *B(o + 11) = (uint8_t)facing;
    *B(o + 235) = 1;
    call1(c, o, f_08863114, back0);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, back1);
}

/* (fighter): a move by +192. 0, standing: two steps, the first with the animation twice the
   strength (6 more with +193), one more when far (a player with the choice 1: back not held; the
   others: further than the word from 0x08A512BC by strength), then 0x0886D678. 1, crouching: with
   +193 0x089013B0, else two steps, the first with the animation 0xC + strength. Others, unless
   +199 is set: with +123 the leap 0x08901688; else 0x089049E4, and when that is below 0 at or
   under the ground 0x08864888, otherwise ticked. A frame keeping s0, s1. */
SFA_FUNC(089011C0, move_by_192_011c0) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    int32_t k = S8(o + 192);
    c->r[17] = 1;
    if (k == 0) {
        uint8_t step = *B(o + 7);
        *B(o + 696) = 0;
        if (step != 0) call1(c, o, f_0886D678, 0x089012B4u);
        else {
            first_step(c, o, 0x0890120Cu, 0x08901218u);
            uint32_t anim = S8(o + 193) != 0 ? 6 : 0;
            anim = (anim + (uint32_t)(S8(o + 194) << 1)) & 0xFFFF;
            int far;
            if (S8(o + 373) == 0 && S8(o + 386) == 1) far = !(*H(o + 290) & 1);
            else far = S16(0x08A512BCu + (uint32_t)(((int32_t)anim >> 1) * 2)) < S16(o + 508);
            if (far) anim = (anim + 1) & 0xFFFF;
            call2(c, o, anim, f_08861674, 0x089012A4u);
        }
    } else if (k == 1) {
        int32_t alt = S8(o + 193);
        *B(o + 696) = 1;
        if (alt != 0) call1(c, o, f_089013B0, 0x08901334u);
        else if (*B(o + 7) != 0) call1(c, o, f_0886D678, 0x08901324u);
        else {
            first_step(c, o, 0x089012F4u, 0x08901300u);
            call2(c, o, (uint32_t)(S8(o + 194) + 0xC) & 0xFFFF, f_08861674, 0x08901314u);
        }
    } else if (S8(o + 199) == 0) {
        if (S8(o + 123) != 0) call1(c, o, f_08901688, 0x08901358u);
        else {
            call1(c, o, f_089049E4, 0x08901368u);
            if ((int32_t)c->r[2] < 0 && !(S16(o + 132) < S16(o + 22))) call1(c, o, f_08864888, 0x0890138Cu);
            else call1(c, o, f_08861848, 0x0890139Cu);
        }
    }
    leave2(c, sv);
}

/* (fighter): a held move's step for a player (the cpu's is 0x089028AC). While the fight is held,
   or when 0x088632C4 says so, it goes on: the next step, +262 cleared, the animation 0xA + twice
   the strength. Else the opponent's +592 becomes its +270, and on the animation's cue 0x0886DC3C
   and 0x08837098 run and +551 becomes -1 if it was 0. Then a kick pressed while more time (+82)
   is left than the word for it at 0x08A512CC puts the time back to 0xF with a shake (0x0886E3CC
   from the bytes at 0x08A512C8): the same strength is ticked, another one changes to it, keeping
   the frame's place (ten bytes each by the cue +77) in the animation 9 + twice the strength.
   Otherwise the time runs down: ticked until it is out, then it goes on. A frame keeping
   s0..s2. */
SFA_FUNC(08902698, held_move_player) {
    gaddr o = ARG(0);
    int32_t cpu = S8(o + 373);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = o;
    if (cpu != 0) { call1(c, o, f_089028AC, 0x089026C0u); goto out; }
    int go = (S8(BATTLE + 13) | S8(BATTLE + 14) | S8(GAME + 38)) != 0;
    if (!go) {
        call1(c, o, f_088632C4, 0x0890270Cu);
        go = c->r[2] != 0;
    }
    if (!go) {
        gaddr other = *W(o + 92);
        if (S8(other + 592) != 0) { *B(other + 270) = 1; *B(other + 592) = 0; }
        if (S8(o + 76) != 0 && S8(o + 74) == 1) {
            *B(o + 76) = 0;
            call1(c, o, f_0886DC3C, 0x08902754u);
            call1(c, o, f_08837098, 0x0890275Cu);
            if (S8(o + 551) == 0) *B(o + 551) = 0xFF;
        }
        uint32_t kicks = *H(o + 760) & 0x380;
        int32_t t = S16(o + 82);
        if (kicks != 0) {
            uint32_t which = (kicks & 0x80) ? 0 : (kicks & 0x100) ? 1 : 2;
            c->r[17] = which; c->r[18] = which;
            if (S16(0x08A512CCu + which * 2) < t) {
                *H(o + 82) = 0xF;
                call3(c, o, *B(0x08A512C8u + which), 8, f_0886E3CC, 0x089027E0u);
                if (which == (uint32_t)(int32_t)S8(o + 194)) { call1(c, o, f_08861848, 0x089027F4u); goto out; }
                *B(o + 194) = (uint8_t)which;
                int32_t strength = S8(o + 194);
                uint32_t cue = (uint32_t)S8(o + 77) & 0xFF;
                c->r[17] = cue;
                call2(c, o, (uint32_t)((strength << 1) + 9) & 0xFFFF, f_088616EC, 0x0890281Cu);
                *W(o + 28) = *W(o + 28) + cue * 10;
                call1(c, o, f_0886175C, 0x0890283Cu);
                goto out;
            }
        }
        t -= 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t != 0) { call1(c, o, f_08861848, 0x08902894u); goto out; }
    }
    {
        uint8_t step = *B(o + 7);
        int32_t strength = S8(o + 194);
        *B(o + 7) = (uint8_t)(step + 1);
        *B(o + 262) = 0;
        call2(c, o, (uint32_t)((strength << 1) + 0xA) & 0xFFFF, f_088616EC, 0x08902884u);
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
