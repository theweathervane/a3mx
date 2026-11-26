/* Small functions the scenarios reach, 41 - 42 instructions: fighter objects' starts, an effect's
   tables set from its owner, effects' countdowns and timed restarts. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_089A536C(Cpu *c);
void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_08904F84(Cpu *c); void f_088764A8(Cpu *c);
void f_08933288(Cpu *c); void f_08821424(Cpu *c); void f_088768F0(Cpu *c); void f_08965F58(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static int halted(void) { return (S8(BATTLE + 37) | S8(BATTLE + 78)) != 0; }
static int few_effects(gaddr o) {
    uint32_t n = *B(o + 536);
    return S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0;
}

/* (object): with few effects and 0x0886BF84 allowing it, 0x089A536C, on (+4 = 1, +6 = 7, +235 = 1,
   +236 = 4; +11 = +697) and 0x0886DB94, whose answer it gives; else 0. */
SFA_FUNC(089A3234, object_start_536c_few) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    int done = 0;
    if (few_effects(o)) {
        c->r[31] = 0x089A3264u; f_0886BF84(c);
        if (c->r[2] != 0) {
            call_on(c, o, 0x089A3274u, f_089A536C);
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 4;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, 0x089A32ACu, f_0886DB94);
            done = 1;
        }
    }
    if (!done) c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4 = 1, +9 cleared, +344 = -1), its owner's (+88) +24 / +86 / +164, its tables (+128 =
   0x08A4C6E8, +200 = 0x08A4C6B0, +352 = 0x08A4C0B4, +356 = 0x08A4C0B8), 0x08876998, +73 cleared, +112 =
   0xFF, 0x0887684C (0) and 0x08904F84. */
SFA_FUNC(08904EDC, effect_tables_set_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = 1; *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 128) = 0x08A4C6E8u;
    *W(o + 200) = 0x08A4C6B0u;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x08904F3Cu, f_08876998);
    *B(o + 73) = 0;
    *H(o + 112) = 0xFF;
    *W(o + 352) = 0x08A4C0B4u;
    *W(o + 356) = 0x08A4C0B8u;
    call2(c, o, 0, 0x08904F6Cu, f_0887684C);
    call_on(c, o, 0x08904F74u, f_08904F84);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): unless halted it ends (step 2, then 0x08933288) once +75 goes negative (with a kind +3) or
   +139 counts down to 0 (kind 0); otherwise 0x088764A8. */
SFA_FUNC(089330DC, effect_end_by_kind) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int end = 0;
    uint32_t back = 0;
    if (!halted()) {
        if (*B(o + 3) != 0) { if (S8(o + 75) < 0) { end = 1; back = 0x0893312Cu; } }
        else {
            int8_t t = (int8_t)(S8(o + 139) - 1);
            *B(o + 139) = (uint8_t)t;
            if (t == 0) { end = 1; back = 0x08933164u; }
        }
    }
    if (end) {
        *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        c->r[31] = back; f_08933288(c);
    } else { c->r[31] = 0x08933178u; f_088764A8(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +251 set or no effects (+536) and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 =
   1, +236 = k236; +5 / +7 / +193 cleared, +11 / +78 = +697) and `then`; 1 then, else 0. */
static void start_78(Cpu *c, uint32_t k236, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 251) != 0 || *B(o + 536) == 0) {
        c->r[31] = back1; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 7) = 0; *B(o + 236) = (uint8_t)k236;
            c->r[17] = 1;
            *B(o + 78) = *B(o + 11);
            *B(o + 193) = 0;
            *B(o + 235) = 1;
            call_on(c, o, back2, then);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0893DA50, object_start_78_2) { start_78(c, 2, f_0886DB94, 0x0893DA80u, 0x0893DAC8u); }
SFA_FUNC(0893DAF8, object_start_78_3) { start_78(c, 3, f_0886DBE8, 0x0893DB28u, 0x0893DB70u); }

/* (effect): unless halted, first frame 0x08965F58, +82 = 2 and 0x088768F0, then +82 counted down to 0
   when it restarts (+0 = +4 = 1, +5 - +7 cleared); drawn. */
SFA_FUNC(08965DA8, effect_restart_5f58) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) {
        if (*B(o + 6) == 0) {
            *B(o + 6) = 1;
            call_on(c, o, 0x08965DF0u, f_08965F58);
            *H(o + 82) = 2;
            call_on(c, o, 0x08965E00u, f_088768F0);
        } else {
            int16_t t = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)t;
            if (t == 0) { *B(o) = 1; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        }
    }
    call_on(c, o, 0x08965E40u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with few effects and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 4; +11 =
   +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0897C2EC, object_start_few_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[2] = 0;
    if (few_effects(o)) {
        c->r[31] = 0x0897C31Cu; f_0886BF84(c);
        if (c->r[2] != 0) {
            c->r[17] = 1;
            *B(o + 236) = 4; *B(o + 235) = 1;
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, 0x0897C358u, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
