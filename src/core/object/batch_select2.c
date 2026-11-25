/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 35 - 42
   instructions: a mirror match's second colour, palette cycling and animation by frame lists, the
   select screen's name panels, the CPU's script choices. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0887B6B8(Cpu *c); void f_0887B7CC(Cpu *c); void f_0887B1A8(Cpu *c); void f_088969BC(Cpu *c);
void f_088A0594(Cpu *c); void f_088A053C(Cpu *c); void f_08821368(Cpu *c); void f_088B1B00(Cpu *c);
void f_088B1D10(Cpu *c); void f_08830334(Cpu *c); void f_089E11C8(Cpu *c); void f_0880AEA8(Cpu *c);
void f_088B01A0(Cpu *c); void f_08858D38(Cpu *c); void f_088215C4(Cpu *c); void f_0882319C(Cpu *c);

static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
#define WITH_S01(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; c->r[16] = (o)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- the CPU's script choices */
/* (fighter): 0x0887B7CC when 0x0887B6B8 says so; else its first script and command cleared when +6 is
   0, when its count +506 runs out (counted down) / +490 reaches +508, or while +78 and +238 are both set. */
SFA_FUNC(0887AC70, cpu_script_count_78) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887AC84u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887AC94u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887ACACu, f_0887B1A8);
    else {
        int16_t v = S16(f + 506);
        if (v == 0) call_on(c, f, 0x0887ACC4u, f_0887B1A8);
        else {
            *H(f + 506) = (uint16_t)(v - 1);
            if (S8(f + 78) != 0 && S8(f + 238) != 0) call_on(c, f, 0x0887ACECu, f_0887B1A8);
        }
    }
    END_S0();
}
SFA_FUNC(0887AAA4, cpu_script_490_508) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887AAB8u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887AAC8u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887AAE0u, f_0887B1A8);
    else if (!(S16(f + 490) < S16(f + 508))) call_on(c, f, 0x0887AB00u, f_0887B1A8);
    else if (S8(f + 78) != 0 && S8(f + 238) != 0) call_on(c, f, 0x0887AB24u, f_0887B1A8);
    END_S0();
}

/* ---------------------------------------------------------------- colours */
/* (fighter, other): when the two are the same character (by 0x08830334, 0x15 and 0x25 counting as
   one) in the same colour (+376), the other's colour moves on (0 - 5). */
SFA_FUNC(08824164, mirror_match_colour) {
    gaddr f = ARG(0), g = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[17] = f; c->r[16] = g;
    ARG(0) = (uint32_t)(int32_t)S8(f + 338);
    c->r[31] = 0x08824188u; f_08830334(c);
    uint32_t a = c->r[2] & 0xFF;
    c->r[18] = a;
    ARG(0) = (uint32_t)(int32_t)S8(g + 338);
    c->r[31] = 0x08824194u; f_08830334(c);
    uint32_t b = c->r[2] & 0xFF;
    ARG(0) = b;
    if (a == b || (a == 0x15 && b == 0x25) || (a == 0x25 && b == 0x15)) {
        int8_t mine = S8(f + 376), theirs = S8(g + 376);
        ARG(0) = (uint32_t)(int32_t)mine;
        if (mine == theirs) {
            *B(g + 376) = (uint8_t)(theirs + 1);
            if (!(S8(g + 376) < 6)) *B(g + 376) = 0;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (fighter, n): unless +657 is n, its colour (+376) of the palettes by its side (0x092D9E00 [+337])
   into its slot (+13), three palettes (INGRID, 0x29: 256 bytes a colour; others 160). */
SFA_FUNC(0886EE28, palette_by_side) {
    gaddr f = ARG(0);
    if (S8(f + 657) == (int8_t)ARG(1)) return;
    gaddr t = *W(0x092D9E00u + (uint32_t)(int32_t)S8(f + 337) * 4);
    uint32_t colour = (uint32_t)(int32_t)S8(f + 376);
    int ingrid = S8(f + 338) == 0x29;
    ARG(3) = *B(f + 13);
    ARG(1) = t + (ingrid ? colour << 8 : (colour << 7) + (colour << 5));
    ARG(0) = 0; ARG(2) = 0; c->r[8] = 3;
    wrap(c, 0x30, 32, ingrid ? 0x0886EE8Cu : 0x0886EEBCu, f_0880AEA8);
}
/* (object): every third frame (+82), 16 colours from 0x092EEDC0 + 2 x +32 into 0x092E0160 (marked
   changed); +32 steps by 16, round to 0 at 48. */
SFA_FUNC(088AD7CC, cycle_colours) {
    gaddr o = ARG(0);
    int16_t n = (int16_t)(S16(o + 82) + 1);
    *H(o + 82) = (uint16_t)n;
    if (n != 3) return;
    WITH_S0(o);
    *H(o + 82) = 0;
    *H(o + 32) = (uint16_t)(S16(o + 32) + 0x10);
    ARG(1) = 0x092EEDC0u + (uint32_t)(int32_t)S16(o + 32) * 2;
    ARG(0) = 0x092E0160u; ARG(2) = 0x10;
    c->r[31] = 0x088AD838u; f_089E11C8(c);
    ARG(0) = 1;
    *W(0x092E0980u) = 1;
    if (!(S16(o + 32) < 48)) *H(o + 32) = 0;
    END_S0();
}
/* (object): one palette of 0x08A30398 + 32 x +10 into slot 0x1F (a0 0 with GAME +93 set, else 1);
   +10 cycles 0 - 27; then 0x088B01A0. */
SFA_FUNC(088AFFE0, cycle_palette_1F) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    ARG(1) = 0x08A30398u + ((uint32_t)*B(o + 10) << 5);
    ARG(2) = 0; ARG(3) = 0x1F; c->r[8] = 1;
    if (S8(GAME + 93) != 0) { ARG(0) = 0; c->r[31] = 0x088B0050u; }
    else { ARG(0) = 1; c->r[31] = 0x088B002Cu; }
    f_0880AEA8(c);
    uint32_t n = *B(o + 10) + 1u;
    *B(o + 10) = (uint8_t)n;
    if (!((int32_t)(n & 0xFF) < 28)) *B(o + 10) = 0;
    call_on(c, o, 0x088B0074u, f_088B01A0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
/* (-, reader): the next of a list of half-words (0x8000 ends it, back to the first), palette 0x20 + it
   into slot (the list's second half-word), one palette (0x088969BC). */
SFA_FUNC(0889FCCC, palette_from_list) {
    gaddr r = ARG(1), p = *W(r);
    uint32_t i = *H(r + 6);
    *H(r + 4) = *H(p);
    gaddr q = p + 4;
    uint32_t slot = *H(p + 2);
    uint32_t e = *H(q + i * 2);
    if (e & 0x8000) { *H(r + 6) = 0; i = 0; e = *H(q); }
    *H(r + 6) = (uint16_t)(i + 1);
    ARG(0) = e + 0x20; ARG(1) = slot; ARG(2) = 1;
    wrap(c, 0x20, 16, 0x0889FD4Cu, f_088969BC);
}

/* ---------------------------------------------------------------- objects */
/* (object, from): set up from `from` (+14, +15, +24, +38; +86 = 8, +11 / +36 / +148 cleared), +4 on;
   its speed (0x088A0594) and +36 = from's +77 (0x088A053C). */
SFA_FUNC(088A01BC, name_panel_setup_77) {
    gaddr o = ARG(0), src = ARG(1);
    WITH_S01(o);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 8;
    c->r[17] = src;
    *B(o + 11) = 0;
    *B(o + 14) = *B(src + 14);
    *B(o + 15) = *B(src + 15);
    *H(o + 24) = *H(src + 24);
    int16_t b38 = S16(src + 38);
    *H(o + 36) = 0;
    *H(o + 38) = (uint16_t)b38;
    ARG(1) = (uint32_t)(int32_t)b38;
    call_on(c, o, 0x088A021Cu, f_088A0594);
    ARG(1) = (uint32_t)(int32_t)S8(src + 77) & 0xFFFF;
    call_on(c, o, 0x088A0234u, f_088A053C);
    END_S01();
}
/* (object): x (+18) moved by +96; past 640, +5 cleared and +82's high byte a random 0 - 15; while GAME
   +16's low byte is under 24, 0x088B1B00 (palettes) and 0x088B1D10. */
SFA_FUNC(088B1810, ticker_move) {
    gaddr o = ARG(0);
    *H(o + 18) = (uint16_t)(S16(o + 18) + *W(o + 96));
    WITH_S0(o);
    if (!(S16(o + 18) < 641)) {
        *B(o + 5) = 0;
        ARG(0) = 0;
        c->r[31] = 0x088B1848u; f_08821368(c);
        uint32_t rnd = c->r[2] & 0xF;
        *H(o + 82) = (uint16_t)((uint32_t)(int32_t)S16(o + 82) & 0xFF);
        *H(o + 82) = (uint16_t)((uint32_t)(int32_t)S16(o + 82) | (rnd << 8));
    }
    if (((uint32_t)(int32_t)S16(GAME + 16) & 0xFF) < 24) {
        call_on(c, o, 0x088B188Cu, f_088B1B00);
        call_on(c, o, 0x088B1894u, f_088B1D10);
    }
    END_S0();
}
/* (object): step 0: x moved by +96 and +82 counted down (below 0: +5 = 1 and 0x08858D38), then drawn;
   step 1: 0x0882319C from intro stage 4 (BATTLE +44), else 0x08858D38 and drawn. */
SFA_FUNC(08858C5C, intro_object) {
    gaddr o = ARG(0); WITH_S0(o);
    if (*B(o + 5) == 0) {
        *W(o + 16) += *W(o + 96);
        int16_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        if (n < 0) { *B(o + 5) = 1; call_on(c, o, 0x08858CB0u, f_08858D38); }
        call_on(c, o, 0x08858CB8u, f_088215C4);
    } else if (!(S8(BATTLE + 44) < 4)) call_on(c, o, 0x08858CDCu, f_0882319C);
    else { call_on(c, o, 0x08858CECu, f_08858D38); call_on(c, o, 0x08858CF4u, f_088215C4); }
    END_S0();
}
