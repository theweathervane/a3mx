/* Small functions the scenarios reach, 73 instructions: a fighter's value looked up by its pad,
   effects following their owners' levels, a fighter copy's countdown. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u
#define PART1  0x092E47D0u
#define PART2  0x092E4C68u

void f_088D7E24(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_088215C4(Cpu *c);
void f_0887CA88(Cpu *c); void f_0887CAF4(Cpu *c); void f_0891D374(Cpu *c); void f_0891D3D8(Cpu *c);
void f_08979D40(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (battle): the +1172 of whichever of the battle's fighters (by its +56 kind: 0 the players, 1 / 2
   one player and the partners, 3 the players and the first partner) has the pad 0x088D7E24 names
   (+775); 0 if none. */
SFA_FUNC(088E9EB0, fighter_value_by_pad) {
    gaddr bt = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    int32_t n = S16(bt + 56);
    *W(sp + 12) = ra;
    static const uint32_t lists[4][3] = {
        { P1, P2, 0 }, { P1, PART1, PART2 }, { P2, PART1, PART2 }, { P1, P2, PART1 } };
    if (n >= 0 && n <= 3)
        for (int i = 0; i < 3; i++) *W(sp + 4 * i) = lists[n][i];
    c->r[31] = 0x088E9F7Cu; f_088D7E24(c);
    uint32_t pad = c->r[2];
    uint32_t r = 0;
    for (int i = 0; i < 3; i++) {
        gaddr x = *W(sp + 4 * i);
        if (x == 0) break;
        if ((uint32_t)S8(x + 775) == pad) { r = (uint32_t)S8(x + 1172); break; }
    }
    c->r[2] = r;
    c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (effect, owner): off (+1) and gone (+4 on) once the owner is no longer its character (+338 = +7);
   with a level (+296's +10, or +11 for +3 set) at the owner's place and side and on: a new one (+78)
   takes its animation from 0x08A4C1EC (0x08A4C4EC with the owner's +386 negative, 0x08A4C3F8 for
   +3), else 0x088219B0. No level clears +78. */
SFA_FUNC(08905070, effect_owner_level_6) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = ra;
    if ((uint32_t)S8(p + 338) != *B(o + 7)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t k3 = *B(o + 3);
        int32_t lv = (int8_t)(k3 != 0 ? *B(*W(p + 296) + 11) : *B(*W(p + 296) + 10));
        if (lv == 0) *B(o + 78) = 0;
        else {
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 11) = *B(p + 11);
            *B(o + 1) = 1;
            if (lv == S8(o + 78)) { c->r[31] = 0x089050F0u; f_088219B0(c); }
            else {
                *B(o + 78) = (uint8_t)lv;
                gaddr tab; uint32_t back;
                if (k3 != 0) { tab = 0x08A4C3F8u; back = 0x08905164u; }
                else if (S8(p + 386) < 0) { tab = 0x08A4C4ECu; back = 0x08905144u; }
                else { tab = 0x08A4C1ECu; back = 0x08905128u; }
                call2(c, o, *W(tab + ((uint32_t)lv << 2)), back, f_0882196C);
            }
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (copy): 0x0891D374; while the round runs (BATTLE +7 / +13 clear) for a player owner (+88's +251
   clear) and +0x150 frames, every +82 frames (not while the owner's +572) it takes the owner's next
   picture (0x0887CA88 -> 0x0887CAF4, +296 = +356's entry for its frame), 0x0891D3D8 and is drawn
   with the owner (+1); otherwise off (+0 / +4 = 2, +13 = +348, +136 cleared). */
SFA_FUNC(0891D17C, fighter_copy_count) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    call_on(c, o, 0x0891D198u, f_0891D374);
    int off = 0;
    if (S8(BATTLE + 13) != 0 || S8(BATTLE + 7) != 0 || S8(p + 251) != 0) off = 1;
    else {
        int16_t t = (int16_t)(S16(o + 0x150) - 1);
        *H(o + 0x150) = (uint16_t)t;
        if (t < 0) off = 1;
        else if (S8(p + 572) == 0) {
            int8_t u = (int8_t)(S8(o + 0x52) - 1);
            *B(o + 0x52) = (uint8_t)u;
            if (u < 0) {
                *B(o + 0x52) = 0;
                call_on(c, o, 0x0891D218u, f_0887CA88);
                call2(c, o, c->r[2], 0x0891D224u, f_0887CAF4);
                *W(o + 296) = *W(o + 356) + 12u * *H(*W(o + 28) + 8);
                call_on(c, o, 0x0891D250u, f_0891D3D8);
                if (*B(p + 1) != 0) call_on(c, o, 0x0891D264u, f_088215C4);
            }
        }
    }
    if (off) {
        *B(o) = 2; *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        *B(o + 13) = (uint8_t)S8(o + 348);
        *B(o + 136) = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): step 0 takes its owner's (+88) character (+3), +12 - +15 / +24 / +38 / +86 / +148 / +164,
   +72 = 1, +9 = 2, +0x71 = -1; step 1 sits on the owner with its side and follows its +296's +11
   (a new one's animation from 0x08A8E920, else 0x088219B0), drawn; later steps 0x08979D40. */
SFA_FUNC(08978E70, effect_owner_level_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    uint32_t st = *B(o + 4);
    *W(c->r[29] + 20) = ra;
    if (st == 0) {
        *B(o + 4) = 1;
        *W(o + 88) = p;
        *B(o + 3) = (uint8_t)S8(p + 338);
        *B(o + 12) = (uint8_t)S8(p + 12);
        *B(o + 13) = *B(p + 13);
        *B(o + 14) = *B(p + 14);
        *B(o + 72) = 1;
        *H(o + 24) = *H(p + 24);
        *H(o + 38) = (uint16_t)S16(p + 38);
        *B(o + 15) = *B(p + 15);
        *H(o + 86) = *H(p + 86);
        *W(o + 164) = *W(p + 164);
        *W(o + 148) = *W(p + 148);
        *B(o + 0x71) = 0xFF;
        *B(o + 9) = 2;
    } else if (st == 1) {
        *H(o + 18) = (uint16_t)S16(p + 18);
        *H(o + 22) = (uint16_t)S16(p + 22);
        *B(o + 11) = *B(p + 11);
        uint32_t lv = *B(*W(p + 296) + 11);
        if (*B(o + 0x71) != lv) {
            *B(o + 0x71) = (uint8_t)lv;
            call2(c, o, *W(0x08A8E920u + (lv << 2)), 0x08978F5Cu, f_0882196C);
        } else call_on(c, o, 0x08978F6Cu, f_088219B0);
        call_on(c, o, 0x08978F74u, f_088215C4);
    } else call_on(c, o, 0x08978F84u, f_08979D40);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
