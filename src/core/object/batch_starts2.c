/* Small functions the scenarios reach, 27 - 34 instructions: fighter objects' starts (the start values
   as numbers, the first ones written out by a pattern script), effects' steps, a fighter's effect
   chain moved, an effect thrown from its owner. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_08821424(Cpu *c);
void f_088764A8(Cpu *c); void f_08904DFC(Cpu *c); void f_08821368(Cpu *c); void f_0882196C(Cpu *c);
void f_088768F0(Cpu *c); void f_0896F62C(Cpu *c); void f_08881F94(Cpu *c); void f_0887DF88(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* A fighter object's start: when 0x0886BF84 allows it, its start values (written between BEGIN and
   END), then 0x0886DB94 / 0x0886DBE8. S1: s0 / s1 saved, s1 = 1 for the call and the answer (0 when
   not allowed). */
#define START_S1_BEGIN(back1) \
    gaddr o = ARG(0); \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; \
    *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; \
    c->r[16] = o; \
    c->r[31] = (back1); f_0886BF84(c); \
    if (c->r[2] != 0) {
#define START_S1_END(then, back2) \
        c->r[17] = 1; \
        call_on(c, o, (back2), then); \
        c->r[2] = 1; \
    } else c->r[2] = 0; \
    c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; \
    c->r[29] += 0x20
SFA_FUNC(0898A344, object_start_0898a344) {
    START_S1_BEGIN(0x0898A35Cu);
        *B(o + 4) = (uint8_t)1;
        *B(o + 5) = (uint8_t)0;
        *B(o + 6) = (uint8_t)7;
        *B(o + 7) = (uint8_t)0;
        *B(o + 236) = (uint8_t)4;
        *W(o + 836) = 0u;
        *B(o + 235) = (uint8_t)1;
        *B(o + 11) = (uint8_t)S8(o + 697);
    START_S1_END(f_0886DBE8, 0x0898A39Cu);
}
SFA_FUNC(0898A3CC, object_start_0898a3cc) {
    START_S1_BEGIN(0x0898A3E4u);
        *B(o + 4) = (uint8_t)1;
        *B(o + 5) = (uint8_t)0;
        *B(o + 6) = (uint8_t)7;
        *B(o + 7) = (uint8_t)0;
        *B(o + 236) = (uint8_t)3;
        *W(o + 836) = 0u;
        *B(o + 235) = (uint8_t)1;
        *B(o + 11) = (uint8_t)S8(o + 697);
    START_S1_END(f_0886DB94, 0x0898A424u);
}
SFA_FUNC(0898A454, object_start_0898a454) {
    START_S1_BEGIN(0x0898A46Cu);
        *W(o + 836) = 0u;
        *B(o + 4) = (uint8_t)1;
        *B(o + 5) = (uint8_t)0;
        *B(o + 6) = (uint8_t)7;
        *B(o + 7) = (uint8_t)0;
        *B(o + 236) = (uint8_t)2;
        *B(o + 235) = (uint8_t)1;
        *B(o + 11) = (uint8_t)S8(o + 697);
    START_S1_END(f_0886DBE8, 0x0898A4ACu);
}
SFA_FUNC(0898A1F8, object_start_0898a1f8) {
    START_S1_BEGIN(0x0898A210u);
        *B(o + 4) = (uint8_t)1;
        *B(o + 5) = (uint8_t)0;
        *B(o + 6) = (uint8_t)7;
        *B(o + 7) = (uint8_t)0;
        *B(o + 236) = (uint8_t)5;
        *W(o + 836) = 0u;
        *B(o + 193) = (uint8_t)0;
        *B(o + 235) = (uint8_t)1;
        *B(o + 11) = (uint8_t)S8(o + 697);
    START_S1_END(f_0886DB94, 0x0898A254u);
}

/* (effect): unless halted, +139 counted down; at 0 step 2 (+5 - +7 cleared) and 0x08904DFC; otherwise
   0x088764A8. */
SFA_FUNC(08904D14, effect_countdown_139) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) != 0) { c->r[31] = 0x08904D90u; f_088764A8(c); }
    else {
        int8_t t = (int8_t)(S8(o + 139) - 1);
        *B(o + 139) = (uint8_t)t;
        if (t != 0) { c->r[31] = 0x08904D7Cu; f_088764A8(c); }
        else {
            *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            c->r[31] = 0x08904D68u; f_08904DFC(c);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, out): one of 16 animations at random (0x08A64842 into 0x08A6129C), then its +28 and +74 - +77
   copied to out. */
SFA_FUNC(0891DB50, effect_random_anim) {
    gaddr o = ARG(0), d = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = d;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = o;
    c->r[31] = 0x0891DB6Cu; f_08821368(c);
    uint32_t k = *B(0x08A64842u + (c->r[2] & 0xF));
    call2(c, o, *W(0x08A6129Cu + (k << 2)), 0x0891DB9Cu, f_0882196C);
    *W(d) = *W(o + 28);
    *B(d + 4) = (uint8_t)S8(o + 74); *B(d + 5) = (uint8_t)S8(o + 75);
    *B(d + 6) = (uint8_t)S8(o + 76); *B(d + 7) = (uint8_t)S8(o + 77);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x0896F62C; falling (+100 negative) to its floor (+132 at or under y) it moves on (+4);
   then 0x088768F0 and drawn on alternate frames (GAME +59 + +66). */
SFA_FUNC(08970374, effect_fall_blink) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08970388u; f_0896F62C(c);
    if ((int32_t)*W(o + 100) < 0 && !((int32_t)*H(o + 132) < (int32_t)*H(o + 22)))
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    call_on(c, o, 0x089703C4u, f_088768F0);
    if (((S8(GAME + 59) + S8(GAME + 66)) & 1) != 0) call_on(c, o, 0x089703ECu, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): with a kind (+3) it ends (+4 = 3, +0 = 2) and is drawn; else its step by +5 (0x08A92898)
   and drawn on alternate frames (GAME +59 bit 0). */
SFA_FUNC(08980AA4, effect_step_2898) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (*B(o + 3) != 0) {
        *B(o + 4) = 3; *B(o) = 2;
        call_on(c, o, 0x08980B1Cu, f_08821424);
    } else {
        gaddr h = *W(0x08A92898u + ((uint32_t)*B(o + 5) << 2));
        ARG(0) = o; ARG(1) = h;
        c->r[31] = 0x08980ADCu; rt_call_indirect(c, h);
        if (S8(GAME + 59) & 1) call_on(c, o, 0x08980AFCu, f_08821424);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its two chained effects (0x08881F94 each time) moved by 0x08A9E918 / 0x08A9E916 its way. */
SFA_FUNC(089A8524, fighter_effects_shift) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0x08A9E914u;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = s2;
    c->r[18] = 2; c->r[16] = o;
    *W(c->r[29] + 28) = ra;
    for (;;) {
        call_on(c, o, 0x089A8550u, f_08881F94);
        gaddr n = c->r[2];
        if (n == 0) break;
        int32_t dx = S16(0x08A9E914u + 2u * c->r[18]);
        int32_t x = S16(n + 18) + (*B(o + 11) != 0 ? -dx : dx);
        c->r[18] = (uint32_t)(int32_t)(int16_t)(c->r[18] - 1);
        *H(n + 18) = (uint16_t)x;
        if (!((int32_t)c->r[18] > 0)) break;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +9 = 1, +12 cleared, its owner's (+88) +14 / +24 taken, 0x30 ahead of it on its
   floor (+132), rising (step y 0, +0x3000 a frame), +86 = 5, then 0x0887DF88 (0x15). */
SFA_FUNC(089B34CC, effect_from_owner_rise) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1;
    *B(o + 14) = *B(p + 14);
    *B(o + 12) = 0;
    *H(o + 24) = *H(p + 24);
    *W(c->r[29] + 16) = ra;
    int32_t x = S16(p + 18);
    *H(o + 18) = (uint16_t)(*B(p + 11) != 0 ? x - 0x30 : x + 0x30);
    *H(o + 22) = (uint16_t)S16(p + 132);
    *W(o + 100) = 0;
    *W(o + 108) = 0x3000;
    *H(o + 86) = 5;
    call2(c, o, 0x15, 0x089B3548u, f_0887DF88);
    c->r[31] = ra;
    c->r[29] += 0x20;
}