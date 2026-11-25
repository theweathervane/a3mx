/* Small functions the scenarios reach, 13 - 29 instructions: an effect maker's timer, fighter objects'
   starts, an effect's bounce off and set-up. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089CAC7C(Cpu *c); void f_08869FEC(Cpu *c); void f_08821424(Cpu *c); void f_088219B0(Cpu *c);
void f_0886BF84(Cpu *c); void f_089633BC(Cpu *c); void f_0882196C(Cpu *c); void f_08971DA8(Cpu *c);
void f_0886DBE8(Cpu *c); void f_0886DB94(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (owner): +820 counted down; at 0 back to 9 and a kind-0 list-0x35 effect (0x089CAC7C). */
SFA_FUNC(089CA064, effect_timer_35) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t v = (int32_t)*W(o + 820) - 1;
    *W(c->r[29] + 16) = ra;
    *W(o + 820) = (uint32_t)v;
    if (!(v > 0)) {
        *W(o + 820) = 9;
        call2(c, o, 0, 0x089CA08Cu, f_089CAC7C);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): on (+4 = 1, +6 = 7; +5 / +7 / +194 cleared); unless +760 / +762 hold all of bits 4 - 6,
   +0x3A6 on when 0x08869FEC says no. */
SFA_FUNC(08920EB4, object_start_760) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 194) = 0;
    uint32_t bits = (uint32_t)(*H(o + 760) | *H(o + 762)) & 0x70;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (bits != 0x70) {
        call_on(c, o, 0x08920F04u, f_08869FEC);
        if (c->r[2] == 0) *B(o + 0x3A0 + 6) = (uint8_t)(S8(o + 0x3A0 + 6) + 1);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x088219B0; when +75 goes negative its next step (+5 on, +82 = 0x1A) bouncing off: x step
   -/+0x8000 (by +11), y step 0 falling by 0x1000; drawn. */
SFA_FUNC(08941B7C, effect_bounce_off) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08941B90u; f_088219B0(c);
    if (S8(o + 75) < 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 0x1A;
        *W(o + 96) = *B(o + 11) != 0 ? 0xFFFF8000u : 0x8000u;
        *W(o + 100) = 0;
        *W(o + 108) = 0xFFFFF000u;
    }
    call_on(c, o, 0x08941BE0u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +236 = k236; +5 / +7 cleared, +11 = +697,
   with `also78` +78 too, with `clear696` +696 cleared) and `then`; 1 then, else 0. */
static void start_k(Cpu *c, uint32_t k236, int also78, int clear696, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        uint8_t v = (uint8_t)S8(o + 697);
        *B(o + 236) = (uint8_t)k236;
        if (clear696) *B(o + 696) = 0;
        *B(o + 11) = v;
        if (also78) *B(o + 78) = v;
        call_on(c, o, back2, then);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0896341C, object_start_c_78)   { start_k(c, 0xC, 1, 0, f_089633BC, 0x08963430u, 0x08963468u); }
SFA_FUNC(08973910, object_start_c_696)  { start_k(c, 0xC, 0, 1, f_0886DBE8, 0x08973924u, 0x0897395Cu); }
SFA_FUNC(08973984, object_start_d_696)  { start_k(c, 0xD, 0, 1, f_0886DB94, 0x08973998u, 0x089739D0u); }
SFA_FUNC(08973AF8, object_start_10_696) { start_k(c, 0x10, 0, 1, f_0886DBE8, 0x08973B0Cu, 0x08973B44u); }

/* (effect): +4 on, 0x08971DA8, +12 / +13 cleared, +86 / +164 from its owner (+88: side +337, +164),
   and its animation by +78 (0x08A8CBAC). */
SFA_FUNC(089717D4, effect_setup_owner_anim) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[31] = 0x089717F4u; f_08971DA8(c);
    *B(o + 12) = 0;
    gaddr p = *W(o + 88);
    *B(o + 13) = 0;
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    call2(c, o, *W(0x08A8CBACu + ((uint32_t)(int32_t)S8(o + 78) << 2)), 0x08971838u, f_0882196C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
