/* Small functions the scenarios reach, 22 - 36 instructions: effects set up from their owners,
   fighter objects' starts by count and character, the VS fighters' slots and colours, a fighter's
   move choice, a trail effect. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define P1   0x092E3EA0u
#define P2   0x092E4338u

void f_089CCF94(Cpu *c); void f_0893D9C4(Cpu *c); void f_089CCC28(Cpu *c); void f_089209A8(Cpu *c);
void f_088054F0(Cpu *c); void f_0881304C(Cpu *c); void f_088158A4(Cpu *c); void f_08815D60(Cpu *c);
void f_0887B1A8(Cpu *c); void f_0887B6B8(Cpu *c); void f_0887B7CC(Cpu *c); void f_08822EAC(Cpu *c);
void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* (effect): +4 on, its owner's (+88) +12 / +13 / +164 / +171 and +86 (its +86, or with `side` its
   side +337) taken, +148 = 3, then `then`. */
static void from_owner(Cpu *c, int side, void (*then)(Cpu *), uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *H(o + 86) = side ? (uint16_t)(int32_t)S8(p + 337) : *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 148) = 3;
    *W(c->r[29] + 16) = ra;
    *B(o + 171) = (uint8_t)S8(p + 171);
    c->r[31] = back; then(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089CCF3C, effect_from_owner_cf94) { from_owner(c, 0, f_089CCF94, 0x089CCF88u); }
SFA_FUNC(089CCAC0, effect_from_owner_cc28) { from_owner(c, 1, f_089CCC28, 0x089CCB14u); }

/* (object): its start (0x0893D9C4) when it has no effects (+536), or under two while +251 is set;
   else 0. */
SFA_FUNC(0893D964, object_start_if_few_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t n = *B(o + 536);
    ARG(1) = n;
    if (S8(o + 251) == 0) {
        if (n == 0) { c->r[31] = 0x0893D988u; f_0893D9C4(c); } else c->r[2] = 0;
    } else if ((int32_t)n < 2) { c->r[31] = 0x0893D9A8u; f_0893D9C4(c); }
    else c->r[2] = 0;
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): 0x089209A8 (0xB) with +386 negative; else (2) when it has no effects (+536), or under two
   while +251 is set; else 0. */
SFA_FUNC(08920AB0, object_start_by_count) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 386) < 0) call2(c, o, 0xB, 0x08920AE0u, f_089209A8);
    else {
        uint32_t n = *B(o + 536);
        if (S8(o + 251) == 0) {
            if (n == 0) call2(c, o, 2, 0x08920AFCu, f_089209A8); else c->r[2] = 0;
        } else if ((int32_t)n < 2) call2(c, o, 2, 0x08920B18u, f_089209A8);
        else c->r[2] = 0;
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The VS fighters' slots: their select-slot pictures (0x08815D60), a wait for the loader
   (0x08E7606C), the characters at 0x092E89DC copied back to 0x08E75C48, 0x088158A4 and both fighters'
   colours (0x0881304C (player 1, player 2, 0)). */
SFA_FUNC(08813268, vs_fighters_slots) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[31] = 0x0881327Cu; f_08815D60(c);
    c->r[16] = 0x08E76068u;
    while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08813298u, f_088054F0);
    *H(0x08E75C48u) = (uint16_t)S16(0x092E89DCu);
    *H(0x08E75C48u + 2) = (uint16_t)S16(0x092E89DEu);
    *H(0x08E75C48u + 4) = (uint16_t)S16(0x092E89E0u);
    call_on(c, 0x08E75C48u, 0x088132D0u, f_088158A4);
    call3(c, P1, P2, 0, 0x088132E8u, f_0881304C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 0x0887B7CC when 0x0887B6B8 says so; else 0x0887B1A8 on its first step (+6), while +490
   is under +508, or with +78 and +238 both set. */
SFA_FUNC(0887AB34, fighter_move_choice) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = f;
    call_on(c, f, 0x0887AB48u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887AB58u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887AB70u, f_0887B1A8);
    else if (S16(f + 490) < S16(f + 508)) call_on(c, f, 0x0887AB90u, f_0887B1A8);
    else if (S8(f + 78) != 0 && S8(f + 238) != 0) call_on(c, f, 0x0887ABB4u, f_0887B1A8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): every 8th frame (GAME +59) a list-0x3B type-1 trail effect (+9 = 3, +78 = 0x4E) at its x +
   its +0x3B8 offset, on its floor (+132), its side (+86) and +164, pointing back at it (+88). */
SFA_FUNC(08937790, fighter_trail_3b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if ((S8(GAME + 59) & 7) == 0) {
        call2(c, 0x3B, 1, 0x089377C0u, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *B(n + 9) = 3;
            *W(n + 16) = *W(o + 16) + *W(o + 0x3A0 + 24);
            *B(n + 78) = 0x4E;
            *H(n + 22) = (uint16_t)S16(o + 132);
            *W(n + 88) = o;
            *H(n + 86) = (uint16_t)(int32_t)S8(o + 337);
            *W(n + 164) = *W(o + 164);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter object): for character 0x11 when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1; +5 / +7
   / +236 cleared, +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(08949310, object_start_char11) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 338) == 0x11) {
        c->r[31] = 0x08949338u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, 0x08949370u, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
