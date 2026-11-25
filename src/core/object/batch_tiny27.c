/* Small functions the scenarios reach, 31 - 32 instructions: effect steps, a thrown effect, a fighter's
   box flags followed by its marker effects, leaving a screen for good, an object's start. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_08821424(Cpu *c); void f_0882196C(Cpu *c); void f_088215C4(Cpu *c); void f_088219B0(Cpu *c);
void f_089B29F8(Cpu *c); void f_088D3B64(Cpu *c); void f_088D3B88(Cpu *c); void f_08805564(Cpu *c);
void f_0880A39C(Cpu *c); void f_08814320(Cpu *c); void f_08821F9C(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886DB94(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

static int halted(void) { return (S8(BATTLE + 37) | S8(BATTLE + 78)) != 0; }

/* (effect): its step by +5 (0x08A8E054) and drawn; while the fight is halted only drawn (when live). */
SFA_FUNC(0896F8C4, effect_step_e054) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (halted()) { if (*B(o) != 0) call_on(c, o, 0x0896F930u, f_08821424); }
    else {
        gaddr h = *W(0x08A8E054u + ((uint32_t)*B(o + 5) << 2));
        ARG(0) = o; ARG(1) = h;
        c->r[31] = 0x0896F910u; rt_call_indirect(c, h);
        call_on(c, o, 0x0896F918u, f_08821424);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): thrown: x step -0x28000 slowing 0x500, y step 0x60000 falling 0xE000, +112 = 6 (x turned
   for +11 clear), +114 = 6; its animation from 0x08A90F94's word. */
SFA_FUNC(08980E14, effect_thrown_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(o + 96) = 0xFFFD8000u;
    *W(o + 104) = 0x500;
    *W(o + 100) = 0x60000;
    *W(o + 108) = 0xFFFF2000u;
    *H(o + 112) = 6;
    *W(c->r[29] + 16) = ra;
    *H(o + 114) = 6;
    if (*B(o + 11) == 0) {
        *W(o + 96) = 0x28000;
        *W(o + 104) = 0xFFFFFB00u;
        *H(o + 112) = 0xFFFA;
    }
    call2(c, o, *W(0x08A90CF0u + 676), 0x08980E84u, f_0882196C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): unless halted 0x089B29F8, then on (+4) once +75 goes negative, else 0x088219B0 and drawn;
   halted just drawn (0x088215C4). */
SFA_FUNC(089B3554, effect_step_29f8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (halted()) call_on(c, o, 0x089B35C0u, f_088215C4);
    else {
        call_on(c, o, 0x089B3588u, f_089B29F8);
        if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        else {
            call_on(c, o, 0x089B359Cu, f_088219B0);
            call_on(c, o, 0x089B35A4u, f_088215C4);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (marker): follows its fighter's (+88) current box flag at `at` (+296's box): off (+1 / +78 cleared)
   at 0; on (+1 = 1) and, when the flag changed, +78 = it and 0x088D3B64 (marker, 0x08AA75E8, flag),
   else 0x088D3B88. */
static void marker_follow(Cpu *c, uint32_t at, uint32_t back_same, uint32_t back_new) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 16) = ra;
    uint32_t v = *B(*W(p + 296) + at);
    ARG(1) = 0x08AA75E8u;
    if (v == 0) { *B(o + 1) = 0; *B(o + 78) = 0; }
    else {
        *B(o + 1) = 1;
        v = *B(*W(p + 296) + at);
        if ((uint32_t)(int32_t)S8(o + 78) != v) {
            *B(o + 78) = (uint8_t)v;
            ARG(2) = v;
            c->r[31] = back_new; f_088D3B64(c);
        } else {
            ARG(2) = p;
            c->r[31] = back_same; f_088D3B88(c);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089BE714, marker_follow_10) { marker_follow(c, 10, 0x089BE758u, 0x089BE770u); }
SFA_FUNC(089BE790, marker_follow_11) { marker_follow(c, 11, 0x089BE7D4u, 0x089BE7ECu); }

/* (screen): leaving for good unless +53 is set: +0 on, its timers +2 - +22 cleared, 0x08E75C44 =
   0xFF, 0x08805564 (2), 0x08821F9C, 0x0880A39C and the portraits freed. */
SFA_FUNC(089DF164, screen_quit) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 53) == 0) {
        uint32_t n = *H(o) + 1u;
        *H(o + 22) = 0;
        *H(o) = (uint16_t)n;
        *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 16) = 0; *H(o + 10) = 0;
        *H(o + 8) = 0; *H(o + 6) = 0; *H(o + 4) = 0; *H(o + 2) = 0;
        *H(0x08E75C44u) = 0xFF;
        ARG(1) = 0x08E70000u;
        call_on(c, 2, 0x089DF1BCu, f_08805564);
        call0(c, 0x089DF1C4u, f_08821F9C);
        call0(c, 0x089DF1CCu, f_0880A39C);
        call0(c, 0x089DF1D4u, f_08814320);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1; +5 / +7 / +236 cleared, +11 =
   +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(088EEEE4, object_start_235_c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call0(c, 0x088EEEFCu, f_0886BF84);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0;
        c->r[17] = 1;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x088EEF34u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
