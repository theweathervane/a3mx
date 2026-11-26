/* Small functions the scenarios reach, 81 instructions: four list-0x15 helpers of a fighter, a
   random spot round an object by its kind, a level effect scattered round its owner. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08822EAC(Cpu *c); void f_08821368(Cpu *c); void f_088D3B64(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter): four list-0x15 helpers (0x08822EAC (0x15, 0 / 1 / 0 / 1)), each on (+0 = 1, +2 = 0x15,
   +3 = which), pointing back at it (+88) with its side and +164. */
SFA_FUNC(0890E6C0, fighter_helpers_15) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 24) = s2;
    c->r[18] = 0x15;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    static const uint32_t back[4] = { 0x0890E6E8u, 0x0890E72Cu, 0x0890E770u, 0x0890E7B4u };
    for (uint32_t k = 0; k < 4; k++) {
        uint32_t which = k & 1;
        call2(c, 0x15, which, back[k], f_08822EAC);
        gaddr n = c->r[2];
        if (k == 0) c->r[17] = 1;
        if (n != 0) {
            *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x15; *B(n + 3) = (uint8_t)which;
            *W(n + 88) = f;
            *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
            *W(n + 164) = *W(f + 164);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object, out): a random spot (0x08821368) by its kind (+3) into out +8 - +14: kind 0 a step pair
   from 0x08A64854 (x turned for +11) at it + 0 - 15 and 0x30 up; kind 1 the pair's y only at it + 0 -
   7; kind 2 an x step from 0x08A64874 (turned) at it and 0x40 - 0x4F up. */
SFA_FUNC(0891DBD8, object_random_spot) {
    gaddr o = ARG(0), d = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = o;
    uint32_t k = *B(o + 3);
    *W(sp) = s0; *W(sp + 8) = ra;
    c->r[16] = d;
    if (k == 0) {
        c->r[31] = 0x0891DC00u; f_08821368(c);
        uint32_t r = c->r[2];
        gaddr e = 0x08A64854u + 2 * (r & 0xE);
        *H(d + 8) = (uint16_t)S16(e);
        if (*B(o + 11) != 0) *H(d + 8) = (uint16_t)-S16(d + 8);
        *H(d + 10) = (uint16_t)S16(e + 2);
        *H(d + 12) = (uint16_t)((r & 0xF) + S16(o + 18));
        *H(d + 14) = (uint16_t)(S16(o + 22) + 0x30);
    } else if (k == 1) {
        c->r[31] = 0x0891DC6Cu; f_08821368(c);
        uint32_t r = c->r[2];
        *H(d + 8) = 0;
        *H(d + 10) = (uint16_t)S16(0x08A64854u + 2 * (r & 0xE) + 2);
        *H(d + 12) = (uint16_t)((r & 7) + S16(o + 18));
        *H(d + 14) = (uint16_t)S16(o + 22);
    } else if (k == 2) {
        c->r[31] = 0x0891DCB8u; f_08821368(c);
        uint32_t r = c->r[2];
        *H(d + 8) = (uint16_t)S16(0x08A64874u + 2 * (r & 7));
        if (*B(o + 11) != 0) *H(d + 8) = (uint16_t)-S16(d + 8);
        *H(d + 10) = 0;
        *H(d + 12) = (uint16_t)S16(o + 18);
        *H(d + 14) = (uint16_t)((r & 0xF) + S16(o + 22) + 0x40);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (effect): on (+4, +78 cleared), one of four animations at random (0x088D3B64: 0x08AB26B8, 0x1A -
   0x1D), at its owner's (+88) place (or its owner's +92 object's, with +3), scattered by up to 0x3F
   back from 0x20 ahead and 0x40 - 0x5F up, the owner's +11 - +13; +1 / +170 = 1. */
SFA_FUNC(089CB33C, effect_level_scatter) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = o;
    *W(sp + 32) = sv[4];
    gaddr p = *W(o + 88);
    c->r[20] = p;
    *W(sp + 28) = sv[3];
    gaddr q = *W(p + 92);
    c->r[19] = q;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(sp + 36) = sv[5];
    c->r[21] = 0x08AB26B8u;
    *B(o + 78) = 0;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 40) = ra;
    ARG(0) = *B(o + 4);
    c->r[31] = 0x089CB384u; f_08821368(c);
    int32_t r = (int32_t)c->r[2];
    c->r[18] = o + 0x10; c->r[17] = o + 0x14;
    int32_t k = r >= 0 ? (r & 3) + 0x1A : -((-r) & 3) + 0x1A;
    ARG(0) = o; ARG(1) = 0x08AB26B8u; ARG(2) = (uint32_t)k;
    c->r[31] = 0x089CB3C0u; f_088D3B64(c);
    gaddr src = *B(o + 3) != 0 ? q : p;
    *W(o + 16) = *W(src + 16);
    *W(o + 20) = *W(src + 20);
    int32_t x = S16(o + 18);
    c->r[18] = (uint32_t)x;
    c->r[31] = 0x089CB40Cu; f_08821368(c);
    uint32_t nx = (uint32_t)x - (c->r[2] & 0x3F) + 0x20;
    *H(o + 18) = (uint16_t)nx;
    ARG(0) = nx;
    c->r[31] = 0x089CB420u; f_08821368(c);
    *H(o + 22) = (uint16_t)((c->r[2] & 0x1F) + S16(o + 22) + 0x40);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 11) = *B(p + 11);
    *B(o + 1) = 1;
    *B(o + 170) = 1;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
