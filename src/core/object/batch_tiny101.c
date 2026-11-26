/* Small functions the scenarios reach, 91 - 106 instructions: an object's push boxes kept apart from
   the fighters of the other side, an attacker's hit box tested against every fighter. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u
#define PART1  0x092E47D0u
#define PART2  0x092E4C68u

void f_08876304(Cpu *c); void f_089701B0(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): while on (+0), pushed apart (0x08876304) from the fighters facing its side (+121) by the
   battle's kind (BATTLE +56: 0 the other player; 1 player 1 or both partners; 2 player 2 or both
   partners; 3 partner 1 or both players). */
SFA_FUNC(08874934, object_push_apart) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (*B(o) != 0) {
        int32_t n = S16(BATTLE + 56);
        int side = S8(o + 121) != 0;
        switch (n) {
        case 0:
            if (side) call2(c, o, P1, 0x088749ACu, f_08876304);
            else call2(c, o, P2, 0x088749C0u, f_08876304);
            break;
        case 1:
            if (side) call2(c, o, P1, 0x088749E0u, f_08876304);
            else { call2(c, o, PART1, 0x088749F4u, f_08876304); call2(c, o, PART2, 0x08874A04u, f_08876304); }
            break;
        case 2:
            if (side) call2(c, o, P2, 0x08874A28u, f_08876304);
            else { call2(c, o, PART1, 0x08874A3Cu, f_08876304); call2(c, o, PART2, 0x08874A4Cu, f_08876304); }
            break;
        case 3:
            if (!side) call2(c, o, PART1, 0x08874A6Cu, f_08876304);
            else { call2(c, o, P1, 0x08874A80u, f_08876304); call2(c, o, P2, 0x08874A90u, f_08876304); }
            break;
        default: break;
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (attacker): for kinds 1 / 2 (+0), the fighter its hit box touches (0x089701B0): its owner (+88)
   first, then (kind 2) each of the four fighters that is on and not the owner; 0 when none. */
SFA_FUNC(08970008, attacker_hit_any) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t r = 0;
    if ((uint32_t)(((uint32_t)*B(o) - 1) & 0xFF) < 2) {
        call2(c, o, *W(o + 88), 0x08970040u, f_089701B0);
        if (c->r[2] != 0) r = *W(o + 88);
        else if (*B(o) != 1) {
            static const gaddr who[4] = { P1, P2, PART1, PART2 };
            static const uint32_t back[4] = { 0x08970098u, 0x089700E4u, 0x08970130u, 0x08970160u };
            for (int i = 0; i < 4; i++) {
                gaddr x = who[i];
                c->r[17] = x;
                if (*B(x) == 0 || x == *W(o + 88)) continue;
                call2(c, o, x, back[i], f_089701B0);
                if (c->r[2] != 0) { r = x; break; }
            }
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
