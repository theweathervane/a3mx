/* Small functions the scenarios reach, 27 instructions: fighter objects' starts, effect steps and
   moves. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886DBE8(Cpu *c); void f_0886DB94(Cpu *c); void f_089439DC(Cpu *c);
void f_08821424(Cpu *c); void f_088768F0(Cpu *c); void f_0896F62C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7; +5 / +7 cleared), 0x089439DC (kind) and
   then `then`; 1 then, else 0. */
static void start_39dc(Cpu *c, uint32_t kind, void (*then)(Cpu *), const uint32_t backs[3]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = backs[0]; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        call2(c, o, kind, backs[1], f_089439DC);
        call_on(c, o, backs[2], then);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08943644, object_start_39dc_8) {
    static const uint32_t b[3] = { 0x08943658u, 0x08943680u, 0x08943688u };
    start_39dc(c, 8, f_0886DBE8, b);
}
SFA_FUNC(089436B0, object_start_39dc_6) {
    static const uint32_t b[3] = { 0x089436C4u, 0x089436ECu, 0x089436F4u };
    start_39dc(c, 6, f_0886DB94, b);
}

/* (effect): its step by +5 (0x08A8E078) unless the fight is halted (BATTLE +37 / +78); halted it is
   only drawn (when live). */
SFA_FUNC(0896FBE8, effect_step_e078) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
        gaddr h = *W(0x08A8E078u + ((uint32_t)*B(o + 5) << 2));
        ARG(1) = h;
        c->r[31] = 0x0896FC2Cu; rt_call_indirect(c, h);
    } else if (*B(o) != 0) { c->r[31] = 0x0896FC48u; f_08821424(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x0896F62C; falling (+100 negative) to its floor (+132 at or under y) it lands (+4 = 2,
   +5 - +7 cleared); then 0x088768F0 and drawn. */
SFA_FUNC(0896FF9C, effect_fall_land) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x0896FFB0u; f_0896F62C(c);
    if ((int32_t)*W(o + 100) < 0 && !((int32_t)*H(o + 132) < (int32_t)*H(o + 22))) {
        *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
    }
    call_on(c, o, 0x0896FFF0u, f_088768F0);
    call_on(c, o, 0x0896FFF8u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +83 counted down (on when it goes negative); drawn on alternate frames (GAME +59 + +66). */
SFA_FUNC(0897041C, effect_countdown_blink) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int8_t v = (int8_t)(S8(o + 83) - 1);
    *B(o + 83) = (uint8_t)v;
    *W(c->r[29] + 16) = ra;
    if (v < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    if (((S8(GAME + 59) + S8(GAME + 66)) & 1) != 0) { c->r[31] = 0x0897047Cu; f_08821424(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): y (+20) by its step (+100, itself stepped by +108); under y 40 the next step (+5 on) from
   y 40 with step 0x20000 and -0x4000. Then 0x088768F0. */
SFA_FUNC(08980B5C, effect_rise) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t step = *W(o + 100);
    *W(o + 20) = *W(o + 20) + step;
    *W(o + 100) = step + *W(o + 108);
    *W(c->r[29] + 16) = ra;
    if (S16(o + 22) < 40) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *W(o + 20) = 0x280000;
        *W(o + 100) = 0x20000;
        *W(o + 108) = 0xFFFFC000u;
    }
    c->r[31] = 0x08980BBCu; f_088768F0(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +236 = 5; +5 / +7 cleared, +11 = +697) and
   0x0886DB94; 1 then, else 0. */
SFA_FUNC(08984128, object_start_db94_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = 0x08984144u; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 236) = 5;
        c->r[17] = 1;
        call_on(c, o, 0x0898417Cu, f_0886DB94);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its steps (x +96 / +104, y +100 / +108) from the 16-byte row +82 of 0x08A9D684, x turned
   for +11 clear. */
SFA_FUNC(089A45C8, effect_steps_from_row) {
    gaddr o = ARG(0);
    gaddr row = 0x08A9D684u + ((uint32_t)(int32_t)S16(o + 82) << 4);
    uint32_t dir = *B(o + 11);
    if (dir == 0) {
        *W(o + 96) = 0u - *W(row);
        *W(o + 104) = 0u - *W(row + 4);
    } else {
        *W(o + 96) = *W(row);
        *W(o + 104) = *W(row + 4);
    }
    *W(o + 100) = *W(row + 8);
    *W(o + 108) = *W(row + 12);
    ARG(1) = *W(row + 12); ARG(2) = row; ARG(3) = dir;
}
