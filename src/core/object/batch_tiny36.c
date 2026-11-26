/* Small functions the scenarios reach, 35 - 36 instructions: fighter objects' starts, effects tied to
   their owners, effects playing once. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_089A536C(Cpu *c);
void f_0882324C(Cpu *c); void f_08821424(Cpu *c); void f_0887684C(Cpu *c); void f_088768F0(Cpu *c);
void f_089238A0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void on_7(gaddr o, uint32_t k236) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
    *B(o + 236) = (uint8_t)k236;
}

/* (object): with +386 set and 0x0886BF84 allowing it, 0x089A536C, on (+4 = 1, +6 = 7, +235 = 1, +236
   = 7; +11 = +697) and 0x0886DBE8, whose answer it gives; else 0. */
SFA_FUNC(089A33D8, object_start_536c_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int done = 0;
    if (S8(o + 386) != 0) {
        c->r[31] = 0x089A33F8u; f_0886BF84(c);
        if (c->r[2] != 0) {
            call_on(c, o, 0x089A3408u, f_089A536C);
            on_7(o, 7);
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, 0x089A3440u, f_0886DBE8);
            done = 1;
        }
    }
    if (!done) c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): gone (0x0882324C) once its +139 no longer matches its owner's (+88); else +1 = 1, at the
   owner's x + its +268 / y (+20), +11, +24, and +84 = the owner's picture (+28's +4) - 0x2B3. */
SFA_FUNC(089CCC28, effect_on_owner_268) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 16) = ra;
    if (S8(o + 139) != S8(p + 139)) { c->r[31] = 0x089CCC94u; f_0882324C(c); }
    else {
        *B(o + 1) = 1;
        *H(o + 24) = *H(p + 24);
        *H(o + 18) = (uint16_t)(S16(p + 18) + S16(p + 268));
        *W(o + 20) = *W(p + 20);
        *B(o + 11) = *B(p + 11);
        gaddr q = *W(p + 28);
        if (q != 0) *H(o + 84) = (uint16_t)(*H(q + 4) - 0x2B3);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): first frame 0x0887684C (6); then, unless halted, on (+4) while +75 is not negative and
   0x088768F0; drawn. */
SFA_FUNC(08900B2C, effect_play_6_on) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (*B(o + 6) == 0) {
        *B(o + 6) = 1;
        call2(c, o, 6, 0x08900B58u, f_0887684C);
        call_on(c, o, 0x08900B60u, f_08821424);
    } else {
        if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
            if (S8(o + 75) >= 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            call_on(c, o, 0x08900BA4u, f_088768F0);
        }
        call_on(c, o, 0x08900BACu, f_08821424);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, effect): the effect takes the owner's +121 / +122 / +80 / +11 - +14 / +38 / x / y and +194 (as
   its +346), +255 / +347 / +112 cleared, +260 = 0xA, +311 / +312 = 2, and points back at it (+88); the
   owner's +228 = it and +536 on. */
SFA_FUNC(08903620, effect_take_owner_xy_3) {
    gaddr a = ARG(0), b = ARG(1);
    *B(b + 122) = (uint8_t)S8(a + 122);
    *B(b + 121) = (uint8_t)S8(a + 121);
    *B(b + 346) = (uint8_t)S8(a + 194);
    *B(b + 347) = 0;
    *B(b + 80) = (uint8_t)S8(a + 80);
    *B(b + 255) = 0;
    *B(b + 14) = *B(a + 14);
    *B(b + 11) = *B(a + 11);
    *B(b + 12) = (uint8_t)S8(a + 12);
    *B(b + 13) = *B(a + 13);
    *H(b + 38) = (uint16_t)S16(a + 38);
    *W(b + 16) = *W(a + 16);
    *H(b + 112) = 0;
    *W(b + 20) = *W(a + 20);
    *W(b + 88) = a;
    *W(a + 228) = b;
    *B(a + 536) = (uint8_t)(*B(a + 536) + 1);
    *B(b + 260) = 0xA;
    *B(b + 311) = 2; *B(b + 312) = 2;
    ARG(0) = 0xA; ARG(2) = 2;
}

/* (object, k): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k; +5 / +7 cleared),
   0x089238A0, +11 = +697 and `then`; 1 then, else 0. */
static void start_k_38a0(Cpu *c, void (*then)(Cpu *), uint32_t back1, uint32_t back2, uint32_t back3) {
    gaddr o = ARG(0);
    uint32_t k = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[17] = k; c->r[16] = o;
    *W(c->r[29] + 24) = ra;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o, k);
        call_on(c, o, back2, f_089238A0);
        c->r[17] = 1;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, back3, then);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08920918, object_start_k_dbe8) { start_k_38a0(c, f_0886DBE8, 0x08920938u, 0x08920960u, 0x08920978u); }
SFA_FUNC(089209A8, object_start_k_db94) { start_k_38a0(c, f_0886DB94, 0x089209C8u, 0x089209F0u, 0x08920A08u); }

/* (object): with +386 not negative and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1, +236 =
   0xA; +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(0893048C, object_start_386_a) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) >= 0) {
        c->r[31] = 0x089304B0u; f_0886BF84(c);
        if (c->r[2] != 0) {
            on_7(o, 0xA);
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, 0x089304ECu, f_0886DBE8);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): first frame its owner's (+88) +228 let go and 0x0887684C (6 + +346, or + +77 with a kind
   +3); then step 3 once +75 goes negative, 0x088768F0; drawn. */
SFA_FUNC(08933288, effect_play_by_level) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(*W(o + 88) + 228) = 0;
        int32_t k = *B(o + 3) == 0 ? S8(o + 346) : S8(o + 77);
        call2(c, o, (uint32_t)(k + 6) & 0xFFFF, 0x089332D4u, f_0887684C);
    } else {
        if (S8(o + 75) < 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        call_on(c, o, 0x08933300u, f_088768F0);
    }
    call_on(c, o, 0x08933308u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
