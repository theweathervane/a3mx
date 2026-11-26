/* Small functions the scenarios reach, 41 instructions: effects rising, bouncing and restarting,
   fighter objects' starts, an effect at its owner's fighter, an effect held by its owner's move.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_08821424(Cpu *c); void f_088219B0(Cpu *c); void f_088231C8(Cpu *c); void f_0887DF88(Cpu *c);
void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08876520(Cpu *c); void f_088768F0(Cpu *c);
void f_08933350(Cpu *c); void f_0882196C(Cpu *c); void f_08987938(Cpu *c); void f_088764A8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static int halted(void) { return (S8(BATTLE + 37) | S8(BATTLE + 78)) != 0; }

/* (effect): step 0: rising (y step 0x40000, +72 = 1, +12 / +13 cleared), 0x0887DF88 (0x23); step 1:
   y by its step, on when +75 goes negative, 0x088219B0 and drawn; later steps gone (0x088231C8). */
SFA_FUNC(0890D3F4, effect_rise_steps) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    uint32_t st = *B(o + 4);
    if (st == 0) {
        *B(o + 4) = 1;
        *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 72) = 1;
        *W(o + 100) = 0x40000;
        call2(c, o, 0x23, 0x0890D438u, f_0887DF88);
    } else if (st == 1) {
        *W(o + 20) = *W(o + 20) + *W(o + 100);
        if (S8(o + 75) < 0) *B(o + 4) = 2;
        call_on(c, o, 0x0890D470u, f_088219B0);
        call_on(c, o, 0x0890D478u, f_08821424);
    } else call_on(c, o, 0x0890D488u, f_088231C8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when it has no effects (+536), or under two while +251 is set, and 0x0886BF84 allows it,
   on (+4 = 1, +6 = 7, +235 = 1, +236 = k236; +5 / +7 cleared, +11 = +697) and 0x0886DB94; 1 then,
   else 0. */
static void start_few(Cpu *c, uint32_t k236, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int few = S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0;
    c->r[2] = 0;
    if (few) {
        c->r[31] = back1; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 236) = (uint8_t)k236;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, back2, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0892AC60, object_start_few_3) { start_few(c, 1, 0x0892AC94u, 0x0892ACCCu); }
SFA_FUNC(0892FE1C, object_start_few_0) { start_few(c, 0, 0x0892FE4Cu, 0x0892FE84u); }

/* (effect): unless halted, first frame 0x08933350, +82 = 2 and 0x088768F0, then +82 counted down to 0
   when it restarts (+0 = +4 = 1, +5 - +7 cleared); then its timed restart (0x08876520). */
SFA_FUNC(08933184, effect_restart_3350) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (!halted()) {
        if (*B(o + 6) == 0) {
            *B(o + 6) = 1;
            call_on(c, o, 0x089331C8u, f_08933350);
            *H(o + 82) = 2;
            call_on(c, o, 0x089331D8u, f_088768F0);
        } else {
            int16_t t = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)t;
            if (t == 0) { *B(o) = 1; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        }
    }
    call_on(c, o, 0x08933218u, f_08876520);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 = -1 or character 0x13 and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1,
   +236 = 2; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(08949430, object_start_386m1_char13) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) == -1 || S8(o + 338) == 0x13) {
        c->r[31] = 0x08949468u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 236) = 2;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, 0x089494A4u, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +1 = 1, +11 - +13 cleared, its fighter's (+92) +24 / +14 and character (+338, as +3),
   at its owner's (+88) x / y, its animation by the fighter's side (0x08A92D7C / 0x08A92D80), then
   0x08987938. */
SFA_FUNC(08987894, effect_at_fighter_side) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr f = *W(o + 92), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 11) = 0;
    *H(o + 24) = *H(f + 24);
    *B(o + 3) = (uint8_t)S8(f + 338);
    *B(o + 14) = *B(f + 14);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    uint32_t k = S8(f + 337) != 0 ? 6 : 5;
    *W(c->r[29] + 20) = ra;
    call2(c, o, *W(0x08A92D68u + (k << 2)), 0x08987920u, f_0882196C);
    call_on(c, o, 0x08987928u, f_08987938);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): flies by its steps (x +96, y +100 falling by +108); on reaching its owner's (+88) floor
   (+132) it bounces (+5 on, y on the floor, y step halved and turned, the fall doubled); 0x088219B0
   and drawn. */
SFA_FUNC(089882C0, effect_bounce_owner_floor) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    uint32_t dy = *W(o + 100), fall = *W(o + 108);
    *W(o + 20) = *W(o + 20) + dy;
    *W(o + 100) = dy + fall;
    *W(c->r[29] + 20) = ra;
    if (!(S16(p + 132) < S16(o + 22))) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        uint32_t v = *W(o + 100);
        *H(o + 22) = (uint16_t)S16(p + 132);
        *H(o + 20) = 0;
        *W(o + 100) = (uint32_t)((int32_t)(0u - v) >> 1);
        *W(o + 108) = fall << 1;
    }
    call_on(c, o, 0x0898834Cu, f_088219B0);
    call_on(c, o, 0x08988354u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its owner's (+88) +502 = 1; with +347 set and the fight not halted, x step by +104 and it
   ends (step 3) once the owner leaves its step 1 / 0 / 8 or +73 counts down to 0; then 0x088764A8. */
SFA_FUNC(0899D47C, effect_held_by_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(p + 502) = 1;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 347) != 0 && !halted()) {
        *W(o + 96) = *W(o + 96) + *W(o + 104);
        if (*B(p + 4) != 1 || *B(p + 5) != 0 || *B(p + 6) != 8) *B(o + 4) = 3;
        else {
            *B(o + 73) = (uint8_t)(S8(o + 73) - 1);
            if (S8(o + 73) == 0) *B(o + 4) = 3;
        }
    }
    c->r[31] = 0x0899D514u; f_088764A8(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
