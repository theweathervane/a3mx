/* Small functions the scenarios reach, 22 - 46 instructions: a direction-input step's start, effects
   set up from their owners, a fighter's picture table and list-0xB helper, a drifting effect, the
   select screen's other textures freed, a fighter object's start. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_088690EC(Cpu *c); void f_08924210(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c);
void f_0882196C(Cpu *c); void f_08881F94(Cpu *c); void f_089E0540(Cpu *c); void f_089E0C24(Cpu *c);
void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter, step, input): the step's start (+0 on, its count +4 = the input's first byte, 6 for the
   CPU with +251 set), then the direction step (0x088690EC). */
SFA_FUNC(08869094, input_step_start) {
    gaddr f = ARG(0), t = ARG(1), in = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(t) = (uint8_t)(*B(t) + 1);
    *W(c->r[29] + 16) = ra;
    *B(t + 4) = S8(f + 251) == 0 ? (uint8_t)*H(in) : 6;
    ARG(0) = f; ARG(1) = t; ARG(2) = in;
    c->r[31] = 0x088690E0u; f_088690EC(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, +9 = 2, +72 = 1, its owner's (+88) +24 / side (+337 as +86) / +164, then its frame
   palette (0x08924210). */
SFA_FUNC(089240F4, effect_from_owner_4210) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 2; *B(o + 72) = 1;
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(c->r[29] + 16) = ra;
    *W(o + 164) = *W(p + 164);
    c->r[31] = 0x08924140u; f_08924210(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A6A20C, 0x0886EB48 (0xB,
   0x08A6A2A8), +86 = its side, +164 = 1, +820 cleared and a list-0xB helper (0x08822F54) of its
   character linked both ways (its +88, the fighter's +40). */
SFA_FUNC(0892484C, fighter_table_copy_b) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A6A20Cu + 4 * n);
    ARG(0) = f; ARG(1) = 0xB; ARG(2) = 0x08A6A2A8u;
    c->r[31] = 0x089248ACu; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    *W(f + 820) = 0;
    call2(c, 0xB, 0, 0x089248D4u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) { *W(n + 88) = f; *W(f + 40) = n; *B(n + 3) = (uint8_t)S8(f + 338); }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, its owner's (+88) character (+338, as +3), +11 - +14 / +24 / +86 / +164 and place,
   0x90 higher and 8 back, falling (y step -0x10000), its animation 0x08A92D68's word. */
SFA_FUNC(0898899C, effect_above_owner_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(p + 24);
    *B(o + 3) = (uint8_t)S8(p + 338);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 14) = *B(p + 14);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    *B(o + 11) = *B(p + 11);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x90);
    *W(c->r[29] + 16) = ra;
    *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) != 0 ? -8 : 8));
    *W(o + 100) = 0xFFFF0000u;
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    call2(c, o, *W(0x08A92D68u), 0x08988A44u, f_0882196C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): while +0x334 is under +0x336 it drifts by its x step (+96, itself stepped by +104), +0x334
   following the distance moved its way, and every 4th frame (GAME +59) 0x08881F94; then both steps
   cleared. */
SFA_FUNC(089A04D8, object_drift) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr q = o + 0x334;
    *W(c->r[29] + 16) = ra;
    if (!(S16(q) < S16(q + 2))) { *W(o + 96) = 0; *W(o + 104) = 0; }
    else {
        uint32_t step = *W(o + 96);
        int32_t x0 = S16(o + 18);
        *W(o + 16) = *W(o + 16) + step;
        *W(o + 96) = step + *W(o + 104);
        int32_t dx = (int16_t)(S16(o + 18) - x0);
        *H(q) = (uint16_t)(*B(o + 11) != 0 ? S16(q) + dx : S16(q) - dx);
        if ((S8(GAME + 59) & 3) == 0) { c->r[31] = 0x089A0580u; f_08881F94(c); }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The select screen's other textures freed: every 12-byte entry of list 39's tile list (0x08A050B0
   +156, its +8 texture, 0x089E0C24) but the one at 0x08A05A9E - 1, then list 39's palette buffer
   (0x08A05818) let go (0x089E0540). */
SFA_FUNC(08815644, select_textures_free_2) {
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4];
    gaddr list = *W(0x08A050B0u + 156);
    c->r[20] = list;
    *W(sp + 28) = sv[3];
    c->r[19] = 0;
    *W(sp + 16) = sv[0];
    c->r[16] = 0x08A0577Cu;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 36) = ra;
    if (c->r[19] < *W(list + 4)) {
        c->r[18] = 0x08A058A0u; c->r[17] = 0;
        for (;;) {
            if ((uint32_t)*B(0x08A058A0u + 510) - 1u != c->r[19])
                call_on(c, *W(*W(list) + c->r[17] + 8), 0x088156B0u, f_089E0C24);
            c->r[19]++;
            c->r[17] += 12;
            if (!(c->r[19] < *W(list + 4))) break;
        }
    }
    if (*W(0x08A0577Cu + 156) != 0) {
        call_on(c, *W(0x08A0577Cu + 156), 0x088156D8u, f_089E0540);
        *W(0x08A0577Cu + 156) = 0;
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): with few effects (none, or under two with +251) and 0x0886BF84 allowing it, on (+4 = 1, +6 =
   7, +235 = 1, +236 = 1; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(088EEF64, object_start_few_1b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int ok = 0;
    if (S8(o + 251) != 0) {
        if ((int32_t)n < 2) { c->r[31] = 0x088EEFFCu; f_0886BF84(c); ok = c->r[2] != 0; }
    } else if (n == 0) { c->r[31] = 0x088EEF94u; f_0886BF84(c); ok = c->r[2] != 0; }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        c->r[17] = 1;
        *B(o + 236) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 235) = 1;
        call_on(c, o, 0x088EEFD0u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
