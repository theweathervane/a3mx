/* Small functions the scenarios reach, 44 - 45 instructions: effects set up from their owners, a
   fighter's picture table and list-8 helper, an effect's frame palette, fighter objects' starts, a
   charge-input step (held / pressed directions). Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0890DAF4(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c); void f_0882196C(Cpu *c);
void f_0887CA88(Cpu *c); void f_0887CAF4(Cpu *c); void f_0886BF84(Cpu *c); void f_0886C8BC(Cpu *c);
void f_0886DB94(Cpu *c); void f_0882324C(Cpu *c); void f_088681F8(Cpu *c); void f_08868208(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): +4 on, +9 = 3, its owner's (+88) +11 - +14 / +24 / x / y / floor +132 / +86 / +164 taken,
   0x0890DAF4, then its set-up by kind +3 (0x08A584D0). */
SFA_FUNC(0890D57C, effect_from_owner_84d0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    *B(o + 9) = 3;
    *B(o + 14) = *B(p + 14);
    *H(o + 24) = *H(p + 24);
    *B(o + 11) = *B(p + 11);
    *H(o + 132) = (uint16_t)S16(p + 132);
    *H(o + 86) = *H(p + 86);
    *W(c->r[29] + 20) = ra;
    *W(o + 164) = *W(p + 164);
    call_on(c, o, 0x0890D5FCu, f_0890DAF4);
    gaddr h = *W(0x08A584D0u + ((uint32_t)*B(o + 3) << 2));
    ARG(0) = o; ARG(1) = h;
    c->r[31] = 0x0890D61Cu; rt_call_indirect(c, h);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A5FDD4, 0x0886EB48 (8,
   0x08A5FE70), +86 = its side, +164 = 1 and a list-8 helper (0x08822F54) facing its way, linked both
   ways (its +88, the fighter's +40). */
SFA_FUNC(08914344, fighter_table_copy_8) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A5FDD4u + 4 * n);
    ARG(0) = f; ARG(1) = 8; ARG(2) = 0x08A5FE70u;
    c->r[31] = 0x089143A4u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    call2(c, 8, 0, 0x089143C8u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) { *W(f + 40) = n; *B(n + 11) = *B(f + 11); *W(n + 88) = f; }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x0887CA88 / 0x0887CAF4, +12 / +13 from +0x71 / +0x70, then by its owner's (+88) current
   frame row's +10 byte: with bit 7 its animation (0x08A6569C by the low 7 bits), else +13 + kind (+3). */
SFA_FUNC(08924210, effect_frame_palette) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call_on(c, o, 0x08924224u, f_0887CA88);
    call2(c, o, c->r[2], 0x08924230u, f_0887CAF4);
    *B(o + 12) = *B(o + 0x71);
    *B(o + 13) = *B(o + 0x70);
    gaddr p = *W(o + 88);
    uint32_t v = *B(*W(*W(p + 416) + 16) + 12u * *H(*W(o + 28) + 8) + 10);
    if ((int8_t)v < 0) call2(c, o, *W(0x08A6569Cu + ((v & 0x7F) << 2)), 0x08924298u, f_0882196C);
    else *B(o + 13) = (uint8_t)(*B(o + 13) + *B(o + 3));
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 positive (0x0886BF84) or 0 (0x0886C8BC) allowing it, on (+4 = 1, +6 = 7, +235 =
   1, +236 = 4; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(089B0DF4, object_start_386_c8bc) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    int32_t k = S8(o + 386);
    int ok = 0;
    if (k > 0) { c->r[31] = 0x089B0E24u; f_0886BF84(c); ok = c->r[2] != 0; }
    else if (k == 0) { c->r[31] = 0x089B0E84u; f_0886C8BC(c); ok = c->r[2] != 0; }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 4;
        c->r[17] = 1;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x089B0E64u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): gone (0x0882324C) once its +139 no longer matches its owner's (+88); else +24 (the owner's,
   or with a kind the owner's partner's +92), +86 = the owner's side, +164 its, and unless halted
   (BATTLE +37) its step by +4 (0x08AB5FD4). */
SFA_FUNC(089CB28C, effect_step_5fd4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88), q = *W(p + 92);
    *W(c->r[29] + 16) = ra;
    if (S8(o + 139) != S8(p + 139)) { c->r[31] = 0x089CB2D4u; f_0882324C(c); }
    else {
        *H(o + 24) = *B(o + 3) != 0 ? *H(q + 24) : *H(p + 24);
        *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
        *W(o + 164) = *W(p + 164);
        if (S8(BATTLE + 37) == 0) {
            gaddr h = *W(0x08AB5FD4u + ((uint32_t)*B(o + 4) << 2));
            ARG(1) = h;
            c->r[31] = 0x089CB330u; rt_call_indirect(c, h);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, step, input): one step of a direction input (the 4 direction bits of input +2 against
   the fighter's held +764): with bit 0x2000 any of them held, else exactly them; then the step's
   count (+4) runs down, at 0 one more done (+0 on, +1 = 1), and 0x08868208; otherwise 0x088681F8. */
SFA_FUNC(088690EC, input_direction_step) {
    gaddr f = ARG(0), t = ARG(1), in = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t want = *H(in + 2);
    uint32_t dirs = want & 0xF, held = *H(f + 764) & 0xF;
    *W(c->r[29] + 16) = ra;
    ARG(0) = t;
    int match = (want & 0x2000) ? ((held & dirs) & 0xFFFF) != 0 : held == dirs;
    if (!match) { c->r[31] = (want & 0x2000) ? 0x08869158u : 0x08869174u; f_088681F8(c); }
    else {
        uint32_t left = (uint32_t)*B(t + 4) - 1u;
        *B(t + 4) = (uint8_t)left;
        if ((left & 0xFF) == 0) {
            *B(t) = (uint8_t)(*B(t) + 1);
            *B(t + 1) = 1;
            c->r[31] = 0x08869194u; f_08868208(c);
        } else { c->r[31] = 0x08869144u; f_08868208(c); }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with few effects (none, or under two with +251) and 0x0886BF84 allowing it, on (+4 = 1, +6 =
   7, +235 = 1, +236 = 0xA; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(088F7710, object_start_few_a) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    if (S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0) c->r[17] = 1;
    if (c->r[17] != 0) {
        c->r[31] = 0x088F7764u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 236) = 0xA;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 235) = 1;
            call_on(c, o, 0x088F77A0u, f_0886DB94);
        } else c->r[17] = 0;
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
