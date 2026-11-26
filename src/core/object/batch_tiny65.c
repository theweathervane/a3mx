/* Small functions the scenarios reach, 66 - 68 instructions: a fighter object's start keeping its
   last inputs, a fighter's buttons as a byte, a list-0x3B effect kept inside the screen, a
   projectile's start. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08821330(Cpu *c); void f_08822EAC(Cpu *c);
void f_0887684C(Cpu *c); void f_08876998(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): once 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 6; +11 = +697), +0x338
   cleared, its 8 bytes at +0x388 copied to +0x378 and +0x380, then 0x0886DB94's answer; else 0. */
SFA_FUNC(0899F178, object_start_keep_inputs) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x0899F18Cu; f_0886BF84(c);
    if (c->r[2] == 0) c->r[2] = 0;
    else {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = 6;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 0x338) = 0;
        for (int i = 0; i < 8; i++) *B(o + 0x378 + i) = *B(o + 0x388 + i);
        for (int i = 0; i < 8; i++) *B(o + 0x380 + i) = *B(o + 0x388 + i);
        call_on(c, o, 0x0899F25Cu, f_0886DB94);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its buttons as a byte (0x10 / 0x20 / 0x40 / 0x80 / 0x100 / 0x200 / 0x4000 / 0x8000 ->
   bits 0 - 7): its own +362 in mode 2 (GAME +114), else the pad its +775 names (0 / 1 / 2: the words
   at 0x08E75BA0 +4 / +48 / +92; above 2 none). */
SFA_FUNC(088678B0, fighter_buttons_byte) {
    gaddr f = ARG(0);
    uint32_t v;
    c->r[6] = (uint32_t)S8(GAME + 114);
    if (S8(GAME + 114) == 2) v = *H(f + 362);
    else {
        int32_t k = S8(f + 775);
        if (k < 0) v = (uint32_t)k;
        else if (k == 0) v = *W(0x08E75BA0u + 4) & 0xFFFF;
        else if (k == 1) v = *W(0x08E75BA0u + 48) & 0xFFFF;
        else if (k == 2) v = *W(0x08E75BA0u + 92) & 0xFFFF;
        else v = 0;
    }
    static const uint32_t mask[8] = { 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x4000, 0x8000 };
    uint32_t r = 0;
    for (int i = 0; i < 8; i++) if (v & mask[i]) r |= 1u << i;
    c->r[5] = v & 0x4000;
    c->r[4] = v & 0x8000;
    c->r[2] = r;
}

/* (owner): a list-0x3B effect (0x08822EAC) 0x40 ahead of it but not past the screen edge (0x08821330:
   its +68 + 0x20 / +70 + 0x3000), on its ground (+132) + 0xD8, +78 = 0x56, +9 cleared, its side and
   +164, pointing back at it (+88) and kept at its +0x3A4. */
SFA_FUNC(08937820, effect_ahead_clamped_3b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    call2(c, 0x3B, 0, 0x08937840u, f_08822EAC);
    gaddr n = c->r[2];
    c->r[17] = n;
    if (n != 0) {
        *B(n + 9) = 0;
        call_on(c, o, 0x08937858u, f_08821330);
        gaddr q = c->r[2];
        *B(n + 11) = *B(o + 11);
        int16_t x;
        if (*B(o + 11) == 0) {
            x = (int16_t)(S16(o + 18) - 0x40);
            int16_t lim = (int16_t)(S16(q + 68) + 0x20);
            if (!((uint16_t)lim < (uint16_t)x)) x = lim;
        } else {
            x = (int16_t)(S16(o + 18) + 0x40);
            int16_t lim = (int16_t)(S16(q + 70) + 0x3000);
            if (!((uint16_t)x < (uint16_t)lim)) x = lim;
        }
        *H(n + 86) = (uint16_t)(int32_t)S8(o + 337);
        *H(n + 18) = (uint16_t)x;
        *W(n + 164) = *W(o + 164);
        *H(n + 22) = (uint16_t)(S16(o + 132) + 0xD8);
        *B(n + 78) = 0x56;
        *W(n + 88) = o;
        *W(o + 0x3A4) = n;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (projectile): on (+4), +344 = -1, +352 / +356 = 0x08A652B0 / 0x08A652C4, pictures 0x08A64AA0 /
   0x08A649A8 (+128 / +200, 0x08876998), +9 / +73 / steps cleared, its owner's (+88) +24 / side / +164,
   0x30 ahead of the owner's facing and 0x46 up, its strength's (+346) speed (0x08A66E98) its way and
   0x0887684C (strength). */
SFA_FUNC(08923900, projectile_start_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 344) = 0xFF;
    *W(o + 352) = 0x08A652B0u;
    *W(o + 356) = 0x08A652C4u;
    *W(o + 128) = 0x08A64AA0u;
    *W(o + 200) = 0x08A649A8u;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x0892395Cu, f_08876998);
    *B(o + 9) = 0; *B(o + 73) = 0;
    *W(o + 96) = 0;
    gaddr p = *W(o + 88);
    *W(o + 100) = 0;
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    int32_t k = S8(o + 346);
    *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(p + 11) != 0 ? 0x30 : -0x30));
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x46);
    uint32_t sp = *W(0x08A66E98u + ((uint32_t)k << 2));
    *W(o + 96) = *B(o + 11) != 0 ? sp : (uint32_t)-(int32_t)sp;
    call2(c, o, (uint32_t)k & 0xFFFF, 0x08923A00u, f_0887684C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
