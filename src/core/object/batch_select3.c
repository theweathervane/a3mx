/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in): the
   colour clash check across a team's three fighters, the frame-list palettes, an ENTRY's (edited
   character's) record shown on a panel, Survival's objects, the CPU's script choices. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0889FCCC(Cpu *c); void f_08824164(Cpu *c); void f_0880AEA8(Cpu *c); void f_0887B6B8(Cpu *c);
void f_0887B7CC(Cpu *c); void f_0887B1A8(Cpu *c); void f_08822EAC(Cpu *c); void f_08823338(Cpu *c);
void f_088969BC(Cpu *c);

static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WRAP20(back, fn) wrap(c, 0x20, 16, back, fn)
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- palettes from frame lists */
/* (x, reader): the reader's count +4 down; at 0, the list's next palette (0x0889FCCC (x, reader)). */
SFA_FUNC(0889FC84, palette_list_count) {
    uint32_t x = ARG(0); gaddr r = ARG(1);
    int16_t n = (int16_t)(S16(r + 4) - 1);
    *H(r + 4) = (uint16_t)n;
    ARG(2) = r;
    if (n != 0) { ARG(0) = r; ARG(1) = x; return; }
    ARG(0) = x; ARG(1) = r;
    WRAP20(0x0889FCC0u, f_0889FCCC);
}
/* (x, k, reader): the reader started on list k of the stage's colour set (0x08A2AF04 by BATTLE +102),
   then its first palette. */
SFA_FUNC(0889FC34, palette_list_start) {
    gaddr r = ARG(2);
    uint32_t i = ((uint32_t)(int32_t)S16(BATTLE + 102) << 2) + (ARG(1) & 0xFFFF);
    uint32_t list = *W(0x08A2AF04u + i * 4);
    *H(r + 6) = 0;
    *W(r) = list;
    ARG(3) = 0x08A2AF04u; c->r[8] = 0x08A30000u;
    ARG(1) = r;
    WRAP20(0x0889FC78u, f_0889FCCC);
}

/* ---------------------------------------------------------------- colours */
/* (a, b, c, flag): no two of a team's fighters in the same colour (0x08824164 on each pair: with flag 1
   b and c against a, else a against b and c, then b against c and a against c again). */
SFA_FUNC(08824200, team_colours) {
    gaddr a = ARG(0), b = ARG(1), cc = ARG(2);
    uint32_t flag = ARG(3);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[18] = a; c->r[17] = b; c->r[16] = cc;
    c->r[8] = 1;
    if (flag == 1) {
        call2(c, b, a, 0x08824234u, f_08824164);
        call2(c, cc, a, 0x08824240u, f_08824164);
        call2(c, b, a, 0x0882424Cu, f_08824164);
    } else {
        call2(c, a, b, 0x08824260u, f_08824164);
        call2(c, a, cc, 0x0882426Cu, f_08824164);
        call2(c, b, cc, 0x08824278u, f_08824164);
        call2(c, a, cc, 0x08824284u, f_08824164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (fighter, n): unless +657 is n, two palettes of its routine table's (+416 +132) row 5 x colour + 3
   into slots +13 + 3 on, and its colour's palette by side (0x092D9E00) into slot +13. */
SFA_FUNC(0886F5D8, palettes_effect_and_side) {
    gaddr f = ARG(0);
    int8_t n = (int8_t)ARG(1);
    uint32_t base = *W(*W(f + 416) + 132);
    uint32_t idx = ((uint32_t)(int32_t)S8(f + 376) * 5 + 3) & 0xFFFF;
    ARG(1) = base;
    if (S8(f + 657) == n) { ARG(0) = idx; return; }
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = f;
    ARG(1) = base + (idx << 5); ARG(3) = *B(f + 13) + 3u;
    ARG(0) = 0; ARG(2) = 0; c->r[8] = 2;
    c->r[31] = 0x0886F634u; f_0880AEA8(c);
    uint32_t colour = (uint32_t)(int32_t)S8(f + 376);
    ARG(1) = *W(0x092D9E00u + (uint32_t)(int32_t)S8(f + 337) * 4) + (colour << 7) + (colour << 5);
    ARG(3) = *B(f + 13);
    ARG(0) = 0; ARG(2) = 0; c->r[8] = 1;
    c->r[31] = 0x0886F674u; f_0880AEA8(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* ---------------------------------------------------------------- panels and objects */
/* (panel): +1 = whether ENTRY +78's record (0x09519588, 0x354 bytes each) is in use; if so its place
   from 0x08A3C9DC, +84 = 0x2B + the byte at 0x08ACCC0C by the record's +1, and that record's palette
   (0x08ACC480 + 32 x its +1) into slot 7 + the entry. */
SFA_FUNC(088CE89C, entry_panel) {
    gaddr o = ARG(0);
    int32_t k = S8(o + 78);
    int64_t m = (int64_t)k * 0x354;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr e = 0x09519588u + c->lo;
    *B(o + 1) = 0;
    if (S8(e) == 0) { ARG(1) = e; ARG(3) = 1; return; }
    *B(o + 1) = 1;
    gaddr t = 0x08A3C9DCu + (uint32_t)k * 4;
    *H(o + 18) = *H(t);
    *H(o + 22) = *H(t + 2);
    *H(o + 84) = (uint16_t)(*B(0x08ACCC0Cu + (uint32_t)(int32_t)S8(e + 1)) + 0x2B);
    ARG(1) = 0x08ACC480u + ((uint32_t)(int32_t)S8(e + 1) << 5);
    ARG(3) = (uint32_t)(k + 7);
    ARG(0) = 2; ARG(2) = 0; c->r[8] = 1;
    wrap(c, 0x30, 32, 0x088CE93Cu, f_0880AEA8);
}
/* With 3 or more free objects in pool 4 (0x095176F3): three list-0xD objects (types 4 - 6: the second at
   y 0x100 falling at -16, the third at y 0 rising at 16 with +10 = 1); then a list-4 object of type 1
   (+98 = 0x10, +104 = 8) and 4 palettes from 0x68D + 4 x BATTLE +102 into slot 0x1B. */
SFA_FUNC(08881CF0, survival_objects) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20; *W(c->r[29] + 16) = ra;
    if (!(S8(0x095176F3u) < 3)) {
        call2(c, 0xD, 4, 0x08881D18u, f_08822EAC);
        call2(c, 0xD, 5, 0x08881D24u, f_08822EAC);
        gaddr n2 = c->r[2];
        *W(n2 + 20) = 0x1000000u; *W(n2 + 100) = 0xFFF00000u;
        call2(c, 0xD, 6, 0x08881D40u, f_08822EAC);
        gaddr n3 = c->r[2];
        *W(n3 + 20) = 0; *W(n3 + 100) = 0x100000u; *B(n3 + 10) = 1;
    }
    call2(c, 4, 1, 0x08881D60u, f_08823338);
    gaddr n = c->r[2];
    *H(n + 98) = 0x10;
    *W(n + 104) = 8;
    ARG(0) = ((uint32_t)(int32_t)S16(BATTLE + 102) << 2) + 0x68D;
    ARG(1) = 0x1B; ARG(2) = 4;
    c->r[31] = 0x08881D90u; f_088969BC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- the CPU's script choices */
/* (fighter): 0x0887B7CC when 0x0887B6B8 says so; else its first script and command cleared when +6 is
   0, +238 is set, or +490 is within 3 of +508; otherwise +497 = whether +490 has reached +508. */
SFA_FUNC(0887ABC4, cpu_script_near_508) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887ABD8u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887ABE8u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887AC00u, f_0887B1A8);
    else if (S8(f + 238) != 0) call_on(c, f, 0x0887AC18u, f_0887B1A8);
    else {
        int32_t a = S16(f + 490), b = S16(f + 508);
        if ((int32_t)(((uint32_t)(a - b) + 3) & 0xFFFF) < 6) call_on(c, f, 0x0887AC44u, f_0887B1A8);
        else {
            *B(f + 497) = 0;
            if (!(a < b)) *B(f + 497) = (uint8_t)(S8(f + 497) + 1);
        }
    }
    END_S0();
}
/* (fighter): as 0x0887AAA4 with the count +506 counted down first. */
SFA_FUNC(0887ACFC, cpu_script_count_490) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887AD10u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887AD20u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887AD38u, f_0887B1A8);
    else {
        int16_t v = S16(f + 506);
        if (v == 0) call_on(c, f, 0x0887AD50u, f_0887B1A8);
        else {
            *H(f + 506) = (uint16_t)(v - 1);
            if (!(S16(f + 490) < S16(f + 508))) call_on(c, f, 0x0887AD78u, f_0887B1A8);
            else if (S8(f + 78) != 0 && S8(f + 238) != 0) call_on(c, f, 0x0887AD9Cu, f_0887B1A8);
        }
    }
    END_S0();
}
