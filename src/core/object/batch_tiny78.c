/* Small functions the scenarios reach, 88 - 89 instructions: a fighter's flash effect with its
   picture parts reset, two list-0x29 effects either side of an object. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08821368(Cpu *c); void f_08822EAC(Cpu *c); void f_08862D88(Cpu *c); void f_0886E638(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (owner, fighter): one time in 4 (0x08821368) a list-0 effect (0x08822EAC, 0x1C) copied from the owner
   (its +88 / +11 - +14 / +24 / place / +86 / +164, kept at the owner's +96), moved by the fighter's +77
   offset pair (0x08A9478C, x turned for its +11), +139 = the fighter's +373; the fighter's +0x33D
   cleared, the owner's +0x66 = 8; the fighter's picture parts +0x9D4 / +0x9D8 / +0x9E4 reset
   (0x0886E638 with its table's +196 / +192, 0xB / 0xC / 0xD), 0x08862D88 and its +292 = 0x08A933D8. */
SFA_FUNC(08987BCC, fighter_flash) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = o;
    c->r[31] = 0x08987BE8u; f_08821368(c);
    if ((c->r[2] & 3) == 0) {
        call2(c, 0, 0x1C, 0x08987BFCu, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *W(n + 88) = *W(o + 88);
            *B(n + 12) = (uint8_t)S8(o + 12);
            *B(n + 13) = *B(o + 13);
            *B(n + 14) = *B(o + 14);
            *B(n + 11) = *B(o + 11);
            *H(n + 24) = *H(o + 24);
            *W(n + 16) = *W(o + 16);
            *W(n + 20) = *W(o + 20);
            *H(n + 86) = *H(o + 86);
            *W(n + 164) = *W(o + 164);
            *W(o + 0x60) = n;
            gaddr e = 0x08A9478Cu + 4 * (uint32_t)S8(f + 77);
            int32_t dx = S16(e), dy = S16(e + 2);
            if (*B(f + 11) != 0) dx = -dx;
            c->r[17] = (uint32_t)dx;
            *H(n + 18) = (uint16_t)(S16(n + 18) + dx);
            *H(n + 22) = (uint16_t)(S16(n + 22) + dy);
            *B(n + 139) = (uint8_t)S8(f + 373);
            *B(f + 0x33D) = 0;
            *B(o + 0x66) = 8;
            static const uint32_t part[3] = { 0x9D4, 0x9D8, 0x9E4 };
            static const uint32_t back[3] = { 0x08987CD4u, 0x08987CECu, 0x08987D04u };
            for (int i = 0; i < 3; i++) {
                gaddr t = *W(f + 416);
                ARG(0) = t + part[i]; ARG(1) = *W(t + 196); ARG(2) = *W(t + 192); ARG(3) = 0xB + (uint32_t)i;
                c->r[31] = back[i]; f_0886E638(c);
            }
            call_on(c, f, 0x08987D0Cu, f_08862D88);
            *W(f + 292) = 0x08A933D8u;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): two list-0x29 effects (0x08822EAC (0x29, 0 / 1)) with its +11 - +14 / +24 / +38 / +86 /
   +164, pointing back at it (+88), 0x54 up and 0x32 behind / ahead of it. */
SFA_FUNC(0891EA80, effects_two_29) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    static const uint32_t back[2] = { 0x0891EA9Cu, 0x0891EB40u };
    for (uint32_t k = 0; k < 2; k++) {
        call2(c, 0x29, k, back[k], f_08822EAC);
        gaddr n = c->r[2];
        if (n == 0) break;
        *B(n + 14) = *B(o + 14);
        *B(n + 12) = (uint8_t)S8(o + 12);
        *B(n + 13) = *B(o + 13);
        *H(n + 38) = (uint16_t)S16(o + 38);
        *W(n + 88) = o;
        *H(n + 24) = *H(o + 24);
        *B(n + 11) = *B(o + 11);
        *H(n + 18) = (uint16_t)S16(o + 18);
        *H(n + 22) = (uint16_t)S16(o + 22);
        int ahead = (*B(n + 11) != 0) == (k == 1);
        *H(n + 18) = (uint16_t)(S16(n + 18) + (ahead ? 0x32 : -0x32));
        *H(n + 22) = (uint16_t)(S16(n + 22) + 0x54);
        *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
