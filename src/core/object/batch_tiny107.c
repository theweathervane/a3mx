/* Small functions the scenarios reach, 44 - 61 instructions: an effect on its fighter by level or
   during its move, a fighter's change into character 0x11 / 0x13. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_0899D25C(Cpu *c); void f_08862D88(Cpu *c);
void f_0886CD5C(Cpu *c); void f_0894E548(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect, fighter): ends (0x0899D25C) once the fighter is no longer its character (+338 = +73).
   Kind 0 (+3) follows the fighter's level (+296's +10: none clears +78; on at the fighter's place and
   side; a new level's animation from 0x08A98E78; else 0x088219B0 unless halted or the fighter's +127
   without its +77). Other kinds sit 0x3F up and 0x15 ahead of the fighter in step 1 (else end, as when
   +75 goes negative), 0x088219B0; halted only animate. */
SFA_FUNC(0899D054, effect_on_fighter_kind) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 3) == 0) {
        *B(o + 1) = 0;
        if (S8(p + 338) != S8(o + 73)) call2(c, o, p, 0x0899D0B0u, f_0899D25C);
        else {
            gaddr q = *W(p + 296);
            if (*B(q + 10) == 0) *B(o + 78) = 0;
            else {
                *H(o + 18) = (uint16_t)S16(p + 18);
                *H(o + 22) = (uint16_t)S16(p + 22);
                *B(o + 1) = 1;
                *B(o + 11) = *B(p + 11);
                uint32_t lv = *B(q + 10);
                if ((uint32_t)S8(o + 78) != lv) {
                    *B(o + 78) = (uint8_t)lv;
                    call2(c, o, *W(0x08A98E78u + ((uint32_t)S8(o + 78) << 2)), 0x0899D12Cu, f_0882196C);
                } else if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0 &&
                           !(S8(p + 127) != 0 && S8(o + 77) == 0))
                    call_on(c, o, 0x0899D160u, f_088219B0);
            }
        }
    } else if (S8(BATTLE + 37) != 0 || S8(BATTLE + 78) != 0) call_on(c, o, 0x0899D234u, f_088219B0);
    else if (S8(p + 338) != S8(o + 73)) call2(c, o, p, 0x0899D220u, f_0899D25C);
    else if (*B(p + 4) != 1) call2(c, o, p, 0x0899D208u, f_0899D25C);
    else if (S8(o + 75) < 0) call2(c, o, p, 0x0899D1F0u, f_0899D25C);
    else {
        *B(o + 11) = *B(p + 11);
        *H(o + 22) = (uint16_t)(S16(p + 22) + 0x3F);
        *H(o + 18) = (uint16_t)(S16(p + 18) + (*B(o + 11) != 0 ? 0x15 : -0x15));
        call_on(c, o, 0x0899D250u, f_088219B0);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, character): +338 = it; for 0x11 / 0x13 the first 5 table words from 0x08A793B4 / 0x08A793C8,
   its parts (0x0894E548) and table +64 / +68 = 0x0894D0EC / 0x08948E94 or 0x0894D408 / 0x08948F7C; then
   0x08862D88 and 0x0886CD5C. */
SFA_FUNC(0894CC44, fighter_change_11_13) {
    gaddr f = ARG(0);
    uint32_t ch = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 338) = (uint8_t)ch;
    *W(c->r[29] + 20) = ra;
    uint32_t k = ch & 0xFF;
    if (k == 0x11 || k == 0x13) {
        gaddr dst = *W(f + 416);
        gaddr src = k == 0x11 ? 0x08A793B4u : 0x08A793C8u;
        for (uint32_t n = 0; n < 5; n++) *W(dst + 4 * n) = *W(src + 4 * n);
        call_on(c, f, k == 0x11 ? 0x0894CC9Cu : 0x0894CCF8u, f_0894E548);
        *W(*W(f + 416) + 64) = k == 0x11 ? 0x0894D0ECu : 0x0894D408u;
        *W(*W(f + 416) + 68) = k == 0x11 ? 0x08948E94u : 0x08948F7Cu;
    }
    call_on(c, f, 0x0894CD20u, f_08862D88);
    call_on(c, f, 0x0894CD28u, f_0886CD5C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
