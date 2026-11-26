/* Small functions the scenarios reach, 38 instructions: fighter objects' starts and super moves, a
   fighter's picture table and its list-0x24 helper, an effect set up from its owner, box-flag markers
   following their fighter. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_0886C6EC(Cpu *c);
void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c); void f_0882196C(Cpu *c); void f_0882324C(Cpu *c);
void f_089BE714(Cpu *c); void f_089BE790(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object, k): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k; +5 / +7 / +196 -
   +199 cleared, +11 = +697) and `then`, answering 0; 1 when it is not allowed. */
static void start_inv(Cpu *c, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t k = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[17] = k; c->r[16] = o;
    *W(c->r[29] + 24) = ra;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] == 0) c->r[2] = 1;
    else {
        *B(o + 236) = (uint8_t)k;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
        *B(o + 235) = 1;
        call_on(c, o, back2, then);
        c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08934FCC, object_start_inv_dbe8) { start_inv(c, f_0886DBE8, 0x08934FECu, 0x08935038u); }
SFA_FUNC(08935598, object_start_inv_db94) { start_inv(c, f_0886DB94, 0x089355B8u, 0x08935604u); }

/* (object): with +386 clear, a full gauge (+366 from 144), +0x33D set and 0x0886C6EC allowing it, a
   super move 0xE (+4 = 1, +6 = 8, +194 / +552 = 2; +11 = +697, +235 left); 1 then, else 0. */
SFA_FUNC(08983E64, object_super_e) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 386) == 0 && !(S16(o + 366) < 144) && S8(o + 0x33C + 1) != 0) {
        c->r[31] = 0x08983EA8u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 0xE;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 194) = 2;
            c->r[17] = 1;
            *B(o + 552) = 2;
        }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A9BD84, 0x0886EB48
   (0x21, 0x08A9BE20), +164 = 1 and a list-0x24 helper (0x08822F54) linked both ways (its +88, the
   fighter's +40). */
SFA_FUNC(0899DB50, fighter_table_copy_21) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A9BD84u + 4 * n);
    ARG(0) = f; ARG(1) = 0x21; ARG(2) = 0x08A9BE20u;
    c->r[31] = 0x0899DBB0u; f_0886EB48(c);
    *W(f + 164) = 1;
    call2(c, 0x24, 0, 0x0899DBC4u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) { *W(n + 88) = f; *W(f + 40) = n; }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): +9 cleared, +4 on, the owner's +12 - +15 / +24 / +38 / +164 and side (+337, as +86)
   taken; its animation by kind +3 (0x08A9C178 into 0x08A9B2B0). */
SFA_FUNC(089A2220, effect_from_owner_kind) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 9) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 14) = *B(p + 14);
    *B(o + 15) = *B(p + 15);
    *H(o + 24) = *H(p + 24);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    *W(c->r[29] + 16) = ra;
    call2(c, o, *W(0x08A9B2B0u + ((uint32_t)*B(0x08A9C178u + *B(o + 3)) << 2)), 0x089A22ACu, f_0882196C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 clear, a full gauge (+366 as unsigned from 144) and 0x0886C6EC allowing it, a
   super move 0xA (+4 = 1, +6 = 8, +235 = 1, +194 / +552 = 2; +11 = +697); 1 then, else 0. */
SFA_FUNC(089B0FF4, object_super_a) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int ok = 0;
    if (S8(o + 386) == 0 && !((int32_t)*H(o + 366) < 144)) {
        c->r[31] = 0x089B1028u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 0xA;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1; *B(o + 552) = 2; *B(o + 194) = 2;
            ok = 1;
        }
    }
    c->r[2] = ok ? 1 : 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (marker): gone (0x0882324C) once its +139 no longer matches its fighter's (+88); else `follow`, the
   fighter's place (+11, +16 / +20, +24) and +140 = the fighter's + `d`. */
static void marker_with(Cpu *c, void (*follow)(Cpu *), int d, uint32_t back_gone, uint32_t back_follow) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 139) != S8(p + 139)) call_on(c, o, back_gone, f_0882324C);
    else {
        call_on(c, o, back_follow, follow);
        *B(o + 11) = *B(p + 11);
        *W(o + 16) = *W(p + 16);
        *W(o + 20) = *W(p + 20);
        *H(o + 24) = *H(p + 24);
        *B(o + 140) = (uint8_t)(S8(p + 140) + d);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089BE570, marker_with_10) { marker_with(c, f_089BE714, -1, 0x089BE59Cu, 0x089BE5B8u); }
SFA_FUNC(089BE608, marker_with_11) { marker_with(c, f_089BE790, 1, 0x089BE634u, 0x089BE650u); }
