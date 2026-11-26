/* Small functions the scenarios reach, 36 instructions: fighter objects' starts, effects made at a
   fighter. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_08822EAC(Cpu *c);
void f_08821424(Cpu *c); void f_0898C5DC(Cpu *c); void f_0899B624(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void on_7(gaddr o, uint32_t k236) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
    *B(o + 236) = (uint8_t)k236;
}

#define S01_BEGIN() \
    gaddr o = ARG(0); \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; \
    *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; \
    c->r[16] = o
#define S01_END() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)

/* (fighter object): for character 0x11 when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 =
   1; +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(089493A0, object_start_char11_1) {
    S01_BEGIN();
    c->r[2] = 0;
    if (S8(o + 338) == 0x11) {
        c->r[31] = 0x089493C8u; f_0886BF84(c);
        if (c->r[2] != 0) {
            on_7(o, 1);
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, 0x08949400u, f_0886DBE8);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    S01_END();
}

/* (object): when +386 is (`want_1`) or is not 1 and 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235,
   +236 = k236; +11 = +697) and 0x0886DB94; 1 then, else 0. */
static void start_386_1(Cpu *c, int want_1, uint32_t k236, uint32_t k235, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1_;
    c->r[17] = 1;
    *W(c->r[29] + 16) = s0_; *W(c->r[29] + 24) = ra_;
    c->r[16] = o;
    c->r[2] = 0;
    if ((S8(o + 386) == 1) == want_1) {
        c->r[31] = back1; f_0886BF84(c);
        if (c->r[2] != 0) {
            on_7(o, k236);
            *B(o + 235) = (uint8_t)k235;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, back2, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    S01_END();
}
SFA_FUNC(0895C82C, object_start_386_not1) { start_386_1(c, 0, 4, 1, 0x0895C854u, 0x0895C88Cu); }
SFA_FUNC(0895C940, object_start_386_is1)  { start_386_1(c, 1, 0xC, 0, 0x0895C968u, 0x0895C9A0u); }

/* (fighter): two list-0x27 type-0xE effects (0x08822EAC) at its x on its floor (+132), +9 = 1 / 2,
   +78 = 0 / 1, pointing back at it (+88). */
SFA_FUNC(0896F568, effects_make_two_27) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x27, 0xE, 0x0896F584u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 9) = 1; *B(n + 78) = 0;
        *H(n + 18) = (uint16_t)S16(o + 18);
        *W(n + 88) = o;
        *H(n + 22) = (uint16_t)S16(o + 132);
        call2(c, 0x27, 0xE, 0x0896F5BCu, f_08822EAC);
        n = c->r[2];
        if (n != 0) {
            *B(n + 9) = 2; *B(n + 78) = 1;
            *H(n + 18) = (uint16_t)S16(o + 18);
            *W(n + 88) = o;
            *H(n + 22) = (uint16_t)S16(o + 132);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a list-0x16 effect (0x08822EAC) of its character (+3, +73 = 2, +72 = 1) with its +12 -
   +15 / +24 / +38, on its floor (+132), linked both ways (+88). */
SFA_FUNC(089789AC, effect_make_16) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x16, 0, 0x089789C8u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 3) = (uint8_t)S8(o + 338);
        *B(n + 73) = 2;
        *W(n + 88) = o;
        *B(n + 12) = (uint8_t)S8(o + 12);
        *B(n + 13) = *B(o + 13);
        *B(n + 14) = *B(o + 14);
        *B(n + 72) = 1;
        *H(n + 24) = *H(o + 24);
        *H(n + 38) = (uint16_t)S16(o + 38);
        *B(n + 15) = *B(o + 15);
        *H(n + 22) = (uint16_t)S16(o + 132);
        *W(o + 88) = n;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on frame 0 of GAME +59 a list-0x1C type-5 effect (0x08822EAC: +0 = 1, +2 / +3 = 0x1C / 5)
   following it (+92) with its owner (+88) and the owner's +86 / +164; then drawn. */
SFA_FUNC(089814CC, effect_make_1c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(GAME + 59) == 0) {
        call2(c, 0x1C, 5, 0x08981500u, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x1C; *B(n + 3) = 5;
            *W(n + 92) = o;
            *W(n + 88) = *W(o + 88);
            *H(n + 86) = *H(p + 86);
            *W(n + 164) = *W(p + 164);
        }
    }
    call_on(c, o, 0x08981548u, f_08821424);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 0xA; +5 / +7 / +836
   cleared), 0x0898C5DC, +11 = +697 and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0898A040, object_start_c5dc_a) {
    S01_BEGIN();
    c->r[31] = 0x0898A058u; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o, 0xA);
        *W(o + 836) = 0;
        call_on(c, o, 0x0898A088u, f_0898C5DC);
        c->r[17] = 1;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x0898A0A0u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    S01_END();
}

/* (object): when 0x0886BF84 allows it, 0x0899B624, on (+4 = 1, +6 = 7, +235 = 1, +236 = 4; +11 =
   +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(0899B394, object_start_b624) {
    S01_BEGIN();
    c->r[31] = 0x0899B3ACu; f_0886BF84(c);
    if (c->r[2] != 0) {
        call_on(c, o, 0x0899B3BCu, f_0899B624);
        on_7(o, 4);
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 235) = 1;
        call_on(c, o, 0x0899B3F4u, f_0886DBE8);
        c->r[2] = 1;
    } else c->r[2] = 0;
    S01_END();
}
