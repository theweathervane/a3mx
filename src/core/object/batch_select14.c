/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 26 - 81
   instructions: the select screen's portrait cleared, leaving the select screen for a fighter's own
   pictures, the VS screen's fighter name plate. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_08813AC4(Cpu *c); void f_088054F0(Cpu *c); void f_088075F8(Cpu *c); void f_088D5E44(Cpu *c);
void f_089E0C24(Cpu *c); void f_0889693C(Cpu *c); void f_088983D0(Cpu *c); void f_0889844C(Cpu *c);
void f_0889858C(Cpu *c); void f_088988FC(Cpu *c); void f_0889CFE8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (screen): the select screen's portrait cleared (0x08813AC4 (-1, 0)), BATTLE +82 = 2, +6 on, +43 and
   +8 / +10 / +18 / +20 / +22 cleared. */
SFA_FUNC(08895B6C, select_portrait_clear) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call2(c, 0xFFFFFFFFu, 0, 0x08895B88u, f_08813AC4);
    *B(BATTLE + 82) = 2;
    *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
    *B(o + 43) = 0;
    *H(o + 22) = 0; *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 10) = 0; *H(o + 8) = 0;
    ARG(0) = 0; ARG(1) = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (wait): unless 0x088D5E44 says no, leaving the select screen for the fighter's own pictures: the
   human side's fighter (GAME +57 bit 0: player 1, else player 2), textures 0x70 / 0x71 freed,
   0x08A05074 = 1, slot 26's picture list and tile set put back (0x08A0562C / D4), the current 216-byte
   record's +202 - +204 / +208 cleared, 0x088075F8 (the fighter's archive by +338 from 0x08A03500, 0,
   0) and, with `wait`, a frame at a time (0x088054F0 (1)) until 0x08E7606C is set. BATTLE +151 is
   cleared either way. */
SFA_FUNC(08814D94, select_leave_for_fighter) {
    uint32_t wait = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = wait;
    c->r[31] = 0x08814DACu; f_088D5E44(c);
    if (c->r[2] == 0) {
        gaddr f = (S8(GAME + 57) & 1) ? 0x092E3EA0u : 0x092E4338u;
        c->r[17] = f;
        call_on(c, 0x70, 0x08814DE8u, f_089E0C24);
        call_on(c, 0x71, 0x08814DF0u, f_089E0C24);
        *W(0x08A05070u + 4) = 1;
        gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
        *W(0x08E75AD8u + 104) = *W(0x08A0562Cu + 104);
        *W(0x092D9D40u + 104) = *W(0x08A056D4u + 104);
        *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
        ARG(0) = *W(0x08A03140u + 960 + ((uint32_t)(int32_t)S8(f + 338) << 2)); ARG(1) = 0; ARG(2) = 0;
        c->r[31] = 0x08814E88u; f_088075F8(c);
        if (wait != 0) {
            c->r[16] = 0x08E76068u;
            while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08814EACu, f_088054F0);
        }
    }
    ARG(0) = BATTLE;
    *B(BATTLE + 151) = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (plate): the VS screen's fighter name plate (its fighter at +92): +4 on, set up (0x0889CFE8, +148 =
   3, +86 = 8, +82 = 0xF, +12 = -1); by its fighter's side (0x088988FC) +13 / +11 / x / step x / +112
   (0x15, 1, -0x140, 0x18, 0x40 for side 0; 0x19, 0, 0x2C0, -0x18, 0x140 for side 1), y and step y by
   whether the fighter is the CPU (+373: 0x48 / -8, else 0xC8 / 8), +78 = the character (+338); then
   its pictures: 0x0889844C (fighter, character), 0x088983D0 (fighter, +376), 0x0889693C (those two,
   +386, +13) and 0x0889858C (plate, the first). */
SFA_FUNC(088985B0, vs_name_plate) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    gaddr f = *W(o + 92);
    c->r[17] = f;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    call2(c, o, 0, 0x088985E4u, f_0889CFE8);
    *W(o + 148) = 3;
    *H(o + 86) = 8;
    *H(o + 82) = 0xF;
    *B(o + 12) = 0xFF;
    call2(c, f, 0xFFFFFFFFu, 0x0889860Cu, f_088988FC);
    if (c->r[2] == 0) {
        *B(o + 13) = 0x15; *B(o + 11) = 1; *H(o + 18) = 0xFEC0; *W(o + 96) = 0x18; *H(o + 112) = 0x40;
    } else {
        *B(o + 13) = 0x19; *B(o + 11) = 0; *H(o + 18) = 0x2C0; *W(o + 96) = 0xFFFFFFE8u; *H(o + 112) = 0x140;
    }
    if (S8(f + 373) == 0) { *H(o + 22) = 0xC8; *W(o + 100) = 8; }
    else { *H(o + 22) = 0x48; *W(o + 100) = 0xFFFFFFF8u; }
    int32_t ch = S8(f + 338);
    *B(o + 78) = (uint8_t)ch;
    call2(c, f, (uint32_t)ch, 0x088986A8u, f_0889844C);
    uint32_t pics = c->r[2];
    c->r[18] = pics;
    call2(c, f, (uint32_t)(int32_t)S8(f + 376), 0x088986B8u, f_088983D0);
    ARG(0) = pics; ARG(1) = (uint32_t)(int32_t)S8(f + 386); ARG(2) = c->r[2]; ARG(3) = *B(o + 13);
    c->r[31] = 0x088986CCu; f_0889693C(c);
    call2(c, o, (uint32_t)(int32_t)(int16_t)pics, 0x088986DCu, f_0889858C);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
