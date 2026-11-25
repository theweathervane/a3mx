/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in): the
   select screen's start (slot 29, the twelve portrait frames), the VS screen's name plates, a fighter's
   three effect objects, the CPU's script checks. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_08814670(Cpu *c); void f_088CD7E4(Cpu *c); void f_08858B00(Cpu *c); void f_088589FC(Cpu *c);
void f_0887CAD0(Cpu *c); void f_0887CB3C(Cpu *c); void f_0887CC00(Cpu *c); void f_08822EAC(Cpu *c);
void f_08823338(Cpu *c); void f_088969BC(Cpu *c); void f_0889DB48(Cpu *c); void f_0887B1A8(Cpu *c);
void f_0887BD00(Cpu *c); void f_0887BD60(Cpu *c);

#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- select screen */
/* The select screen's start: GAME +2 on, slot 29 allocated and its palettes (0x08814670), the 0x13
   bytes at 0x08B02E70 cleared, 0x088CD7E4, and 0x09525881 cleared. */
SFA_FUNC(088CC73C, select_start) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10; *W(c->r[29]) = ra;
    uint16_t n = (uint16_t)(*H(GAME + 2) + 1);
    ARG(1) = n;
    *H(GAME + 2) = n;
    c->r[31] = 0x088CC75Cu; f_08814670(c);
    for (uint32_t i = 0; i < 0x13; i++) *B(0x08B02E70u + i) = 0;
    ARG(0) = 0x13; ARG(1) = 0x08B02E83u; ARG(2) = 0;
    c->r[31] = 0x088CC784u; f_088CD7E4(c);
    ARG(0) = 0x09520000u;
    *B(0x09525881u) = 0;
    c->r[31] = ra; c->r[29] += 0x10;
}
/* With 12 or more free objects in pool 4: the twelve list-0xD frames (types from 0x08A24C04, y from
   0x08A24C10), the last tied to BATTLE +112's fighter (else fighter 1 or 2 by GAME +53); then that
   fighter's colour choice palettes (0x0889DB48), 4 palettes from 0x68D + 4 x BATTLE +102 into slot 0x1B,
   a list-4 type-0 object (+98 / +104 cleared) and 0x08E75C44 = 0xFF. */
SFA_FUNC(08881BEC, select_frames) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = s3; *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 32) = ra;
    c->r[19] = 0xB; c->r[16] = BATTLE;
    gaddr f = c->r[5];
    if (0xB < S8(0x095176F3u)) {
        c->r[17] = 0x08A24C26u; c->r[18] = 0x08A24C04u;
        gaddr n = 0;
        for (;;) {
            uint32_t i = c->r[19];
            call2(c, 0xD, *B(0x08A24C04u + i), 0x08881C44u, f_08822EAC);
            n = c->r[2];
            *H(n + 22) = *H(c->r[17]);
            c->r[19] = i - 1;
            c->r[17] -= 2;
            if (i == 0) break;
        }
        f = *W(BATTLE + 112);
        if (f == 0) f = S8(GAME + 53) != 0 ? 0x092E3EA0u : 0x092E4338u;
        *W(n + 88) = f;
    }
    call2(c, (uint32_t)(int32_t)S8(f + 338), (uint32_t)(int32_t)S8(f + 376), 0x08881C9Cu, f_0889DB48);
    ARG(0) = ((uint32_t)(int32_t)S16(BATTLE + 102) << 2) + 0x68D; ARG(1) = 0x1B; ARG(2) = 4;
    c->r[31] = 0x08881CB4u; f_088969BC(c);
    call2(c, 4, 0, 0x08881CC0u, f_08823338);
    gaddr n = c->r[2];
    *H(n + 98) = 0;
    *W(n + 104) = 0;
    *H(0x08E75C44u) = 0xFF;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* ---------------------------------------------------------------- VS screen */
/* (plate): the VS screen's name plate: +4 on, at (0x08A1776C by +3 (+0x10 for side 1), 0xB6), +12 = 1,
   +13 = 0x17, +24 = 7, +82 = 9, +86 = 5, +148 = 6; its picture by +3 (0x08858B00), +156 = 0x08A17360;
   then 0x088589FC. */
SFA_FUNC(0885893C, vs_name_plate) {
    gaddr o = ARG(0); WITH_S0(o);
    *B(o + 11) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 14) = 0;
    *H(o + 24) = 7;
    *B(o + 12) = 1; *B(o + 13) = 0x17;
    *H(o + 82) = 9;
    uint8_t b3 = *B(o + 3);
    uint32_t i = S8(o + 78) != 0 ? b3 + 0x10u : b3;
    *H(o + 18) = (uint16_t)S16(0x08A1776Cu + i * 2);
    *H(o + 22) = 0xB6;
    *H(o + 86) = 5;
    *W(o + 148) = 6;
    ARG(2) = 6;
    call2(c, o, (uint32_t)(int32_t)(int16_t)b3, 0x088589D8u, f_08858B00);
    *W(o + 156) = 0x08A17360u;
    call_on(c, o, 0x088589ECu, f_088589FC);
    END_S0();
}

/* ---------------------------------------------------------------- fighters */
/* (fighter): unless +80 is set, when 0x0887CB3C allows it, its three effect objects (the first of the
   lists at 0x08A22DE0) set up from it (0x0887CC00, kinds from 0x08A22DF8) with +2 = 0xB, +96 = its
   side's value (0x0887CAD0), +14 its own, +82 from 0x08A22DEC and +13 moved by 0x08A22DF2. */
SFA_FUNC(0887CE20, effect_objects) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s[8], fp = c->r[30];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int k = 0; k < 8; k++) { s[k] = c->r[16 + k]; *W(sp + 16 + 4 * (uint32_t)k) = s[k]; }
    *W(sp + 48) = fp; *W(sp + 52) = ra;
    c->r[16] = f;
    if (S8(f + 80) == 0) {
        call_on(c, (uint32_t)(int32_t)S8(f + 337), 0x0887CE60u, f_0887CAD0);
        c->r[23] = c->r[2];
        c->r[31] = 0x0887CE68u; f_0887CB3C(c);
        c->r[21] = 0x08A20000u;
        if (c->r[2] != 0) {
            c->r[19] = 0xB;
            for (c->r[22] = 0; (int32_t)c->r[22] < 3; c->r[22]++) {
                uint32_t k = c->r[22];
                gaddr obj = *W(*W(0x08A22DE0u + 4 * k));
                c->r[30] = obj;
                ARG(0) = f; ARG(1) = obj; ARG(2) = (uint32_t)S8(0x08A22DF8u + k) & 0xFF;
                c->r[31] = 0x0887CEB4u; f_0887CC00(c);
                *B(obj + 2) = 0xB;
                *W(obj + 96) = c->r[23];
                *B(obj + 14) = *B(f + 14);
                *H(obj + 82) = (uint16_t)S16(0x08A22DECu + 2 * k);
                *B(obj + 13) = (uint8_t)(*B(obj + 13) + S16(0x08A22DF2u + 2 * k));
            }
        }
    }
    for (int k = 0; k < 8; k++) c->r[16 + k] = s[k];
    c->r[30] = fp; c->r[31] = ra;
    c->r[29] += 0x40;
}
/* (fighter): the CPU's script checks: its first script and command cleared (0x0887B1A8) when +6 is 0,
   or when +123 is set and 0x0887BD00 says so; with +483 set, also when +542 and +498 are set (+498
   cleared) and 0x0887BD60 says so. */
SFA_FUNC(0887AF50, cpu_script_checks) {
    gaddr f = ARG(0); WITH_S0(f);
    uint8_t b483 = *B(f + 483), b6 = *B(f + 6);
    if (b6 == 0) call_on(c, f, b483 == 0 ? 0x0887AF90u : 0x0887AFE4u, f_0887B1A8);
    else if (S8(f + 123) != 0) {
        call_on(c, f, b483 == 0 ? 0x0887AFA8u : 0x0887AFFCu, f_0887BD00);
        if (c->r[2] != 0) call_on(c, f, b483 == 0 ? 0x0887AFB8u : 0x0887B020u, f_0887B1A8);
        else if (b483 != 0 && S8(f + 542) != 0 && S8(f + 498) != 0) {
            *B(f + 498) = 0;
            call_on(c, f, 0x0887B044u, f_0887BD60);
            if (c->r[2] != 0) call_on(c, f, 0x0887B054u, f_0887B1A8);
        }
    }
    END_S0();
}
