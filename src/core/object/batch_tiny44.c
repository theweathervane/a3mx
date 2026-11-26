/* Small functions the scenarios reach, 39 - 41 instructions: effects landing and bouncing, effects made
   at a fighter, an effect's timed end, a super move, a fighter's move flags. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_088215C4(Cpu *c); void f_088219B0(Cpu *c); void f_08981AE4(Cpu *c); void f_08822EAC(Cpu *c);
void f_08821424(Cpu *c); void f_088768F0(Cpu *c); void f_0886C6EC(Cpu *c); void f_08924498(Cpu *c);
void f_08971D6C(Cpu *c); void f_0887B1A8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): 0x08981AE4; under its fighter's (+92) floor (+132) it bounces there (+5 on, y step 0x20000
   falling by 0x4000); 0x088219B0, and on alternate frames (GAME +59 bit 0) once more and drawn. */
SFA_FUNC(089811B4, effect_bounce_on_floor) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92);
    c->r[17] = p;
    c->r[31] = 0x089811D0u; f_08981AE4(c);
    if (S16(o + 22) < S16(p + 132)) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *W(o + 20) = (uint32_t)(int32_t)S16(p + 132) << 16;
        *W(o + 100) = 0x20000;
        *W(o + 108) = 0xFFFFC000u;
    }
    call_on(c, o, 0x08981214u, f_088219B0);
    if (S8(GAME + 59) & 1) {
        call_on(c, o, 0x08981234u, f_088219B0);
        call_on(c, o, 0x0898123Cu, f_088215C4);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, k): a list-0x83 effect of its character (0x08822EAC) with +73 = k, +72 = 1 and the
   fighter's +12 - +15 / +24 / +38 / +86 / +164, pointing back at it (+88). */
SFA_FUNC(089A8A18, effect_make_83) {
    gaddr o = ARG(0);
    uint32_t k = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = o; c->r[17] = k;
    *W(c->r[29] + 24) = ra;
    call2(c, 0x83, (uint32_t)S8(o + 338) & 0xFF, 0x089A8A44u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 73) = (uint8_t)k;
        *W(n + 88) = o;
        *B(n + 12) = (uint8_t)S8(o + 12);
        *B(n + 13) = *B(o + 13);
        *B(n + 14) = *B(o + 14);
        *B(n + 72) = 1;
        *H(n + 24) = *H(o + 24);
        *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
        *H(n + 38) = (uint16_t)S16(o + 38);
        *B(n + 15) = *B(o + 15);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): unless halted, first frame +82 = 2 and 0x088768F0, then +82 counted down to 0 when it
   restarts (+0 = +4 = 1, +5 - +7 cleared); drawn. */
SFA_FUNC(08876520, effect_timed_restart) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) {
        if (*B(o + 6) == 0) {
            *B(o + 6) = 1;
            *H(o + 82) = 2;
            call_on(c, o, 0x08876570u, f_088768F0);
        } else {
            int16_t t = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)t;
            if (t == 0) { *B(o) = 1; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        }
    }
    call_on(c, o, 0x088765B0u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 not 1, a full gauge (+366 from 144) and 0x0886C6EC allowing it, a super move 0xA
   (+4 = 1, +6 = 8, +194 / +552 = 2; +11 = +697, +235 left); 1 then, else 0. */
SFA_FUNC(0890FDF0, object_super_a_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int ok = 0;
    if (S8(o + 386) != 1 && !(S16(o + 366) < 144)) {
        c->r[31] = 0x0890FE28u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 0xA;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 552) = 2; *B(o + 194) = 2;
            ok = 1;
        }
    }
    c->r[2] = ok ? c->r[17] : 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, its owner's (+88) character (+338, as +73), +11 - +14 / +24 / +38, x and floor (+132)
   taken, its set-up by kind +3 (0x08A66F0C), then 0x08924498. */
SFA_FUNC(089242F4, effect_from_owner_6f0c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 73) = (uint8_t)S8(p + 338);
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 132);
    gaddr h = *W(0x08A66F0Cu + ((uint32_t)*B(o + 3) << 2));
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; ARG(1) = h;
    c->r[31] = 0x0892437Cu; rt_call_indirect(c, h);
    call_on(c, o, 0x08924384u, f_08924498);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its life (+82) counted down (step 2 when it runs out); 0x08971D6C, and on reaching its
   floor (+132 at or under y) it bounces (+5 on, y on the floor, y step halved and turned); 0x088219B0
   and drawn. */
SFA_FUNC(08970B68, effect_bounce_life) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    int8_t t = (int8_t)(S8(o + 82) - 1);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 82) = (uint8_t)t;
    *W(c->r[29] + 20) = ra;
    if (t < 0) *B(o + 4) = 2;
    else {
        call_on(c, o, 0x08970BA0u, f_08971D6C);
        if (!((int32_t)*H(o + 132) < (int32_t)*H(o + 22))) {
            *H(o + 22) = *H(o + 132);
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *W(o + 100) = 0u - (uint32_t)((int32_t)*W(o + 100) >> 1);
        }
        call_on(c, o, 0x08970BE4u, f_088219B0);
        call_on(c, o, 0x08970BECu, f_088215C4);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a list-0x43 type-3 effect (kind 4 with +386 set) above it (y + 0x58, 0x10 lower for
   level 1 of +194; +73 = level - 1), facing its way, pointing back at it (+88). */
SFA_FUNC(089A26D8, effect_make_43_3) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x43, 3, 0x089A26F4u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 11) = *B(o + 11);
        *H(n + 18) = (uint16_t)S16(o + 18);
        *W(n + 88) = o;
        *H(n + 22) = (uint16_t)S16(o + 22);
        if (S8(o + 386) != 0) *B(n + 3) = 4;
        *H(n + 18) = (uint16_t)S16(o + 18);
        *H(n + 22) = (uint16_t)(S16(o + 22) + 0x58);
        int8_t k = (int8_t)(S8(o + 194) - 1);
        *B(n + 73) = (uint8_t)k;
        if (k == 0) *H(n + 22) = (uint16_t)(S16(n + 22) - 0x10);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its move flags (+480 - +484): with +483 set they are cleared once it is past step +5 0,
   else at step 1 / 0 (+4 / +6) 0x0887B1A8; without +483, 0x0887B1A8 at step 1 / 0 / 0. */
SFA_FUNC(0887B0DC, fighter_move_flags) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(f + 483) != 0) {
        if (*B(f + 5) != 0) {
            *B(f + 484) = 0; *B(f + 483) = 0; *B(f + 482) = 0; *B(f + 481) = 0; *B(f + 480) = 0;
        } else if (*B(f + 4) == 1 && *B(f + 6) == 0) { c->r[31] = 0x0887B174u; f_0887B1A8(c); }
    } else if (*B(f + 4) == 1 && *B(f + 5) == 0 && *B(f + 6) == 0) { c->r[31] = 0x0887B120u; f_0887B1A8(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
