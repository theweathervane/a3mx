/* Small functions the scenarios reach, 49 - 65 instructions: a falling banner's start, a fighter's
   list-0x2D / 0x2E helper, a stage object's palette cycle, a select background's start. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define PALS   0x092D9E00u

void f_088215C4(Cpu *c); void f_0882196C(Cpu *c); void f_08855D80(Cpu *c); void f_088D5E44(Cpu *c);
void f_088E9EB0(Cpu *c); void f_08822D2C(Cpu *c); void f_089CB748(Cpu *c); void f_0880AEA8(Cpu *c);
void f_0887EDD8(Cpu *c); void f_088AE2E8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (banner): 0x08855D80 (3, 0x1E4, 0x98), on (+4, +11 / +123 cleared), +148 = 0xB, its cells 0x08A16962
   (+156), +7 = 0xF, +79 = 3 (4 when 0x088D5E44 holds and 0x088E9EB0 is negative), falling (+96 =
   -0x100000) for 0x12C frames (+82), the animation 0x08A16A90 +92 and drawn. */
SFA_FUNC(088552C0, banner_fall_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = o;
    c->r[17] = 3;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 3; ARG(2) = 0x1E4; ARG(3) = 0x98;
    c->r[31] = 0x088552E8u; f_08855D80(c);
    *B(o + 11) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 123) = 0;
    *W(o + 148) = 0xB;
    *W(o + 156) = 0x08A16962u;
    *B(o + 7) = 0xF;
    *B(o + 79) = 3;
    *W(o + 96) = 0xFFF00000u;
    *H(o + 82) = 0x12C;
    ARG(0) = 0xFFF00000u; ARG(1) = 0x12C;
    c->r[31] = 0x08855330u; f_088D5E44(c);
    if (c->r[2] != 0) {
        call_on(c, BATTLE, 0x08855344u, f_088E9EB0);
        if ((int32_t)c->r[2] < 0) *B(o + 79) = 4;
    }
    call2(c, o, *W(0x08A16A90u + 92), 0x08855368u, f_0882196C);
    call_on(c, o, 0x08855370u, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, k): a list-0x2D (k = 0) or 0x2E helper (0x08822D2C) kept at +228 (none: 0), on, pointing back
   (+88) with its +80 / +121 / +122 / +139 / +194 / +251 / +14 / +24 / side / +164, +3 = k, +112 cleared,
   +346 = +194 + 3k; then 0x089CB748 (fighter, k); the helper returned. */
SFA_FUNC(089CB650, fighter_helper_2d_2e) {
    gaddr f = ARG(0);
    uint32_t k = ARG(1) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    c->r[18] = k;
    c->r[16] = f;
    *W(c->r[29] + 28) = ra;
    ARG(0) = f; ARG(1) = k != 0 ? 0x2E : 0x2D; ARG(2) = 0;
    c->r[31] = 0x089CB688u; f_08822D2C(c);
    gaddr n = c->r[2];
    c->r[17] = n;
    if (n == 0) *W(f + 228) = 0;
    else {
        *W(n + 88) = f;
        *B(n + 3) = (uint8_t)k;
        *B(n + 139) = (uint8_t)S8(f + 139);
        *B(n + 194) = (uint8_t)S8(f + 194);
        *B(n + 121) = (uint8_t)S8(f + 121);
        *B(n) = 1;
        *B(n + 251) = (uint8_t)S8(f + 251);
        *H(n + 24) = *H(f + 24);
        *B(n + 14) = *B(f + 14);
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
        *B(n + 122) = (uint8_t)S8(f + 122);
        *H(n + 112) = 0;
        *B(n + 121) = (uint8_t)S8(f + 121);
        *B(n + 80) = (uint8_t)S8(f + 80);
        *B(n + 346) = (uint8_t)(S8(f + 194) + 3 * k);
        *W(f + 228) = n;
    }
    call2(c, f, k, 0x089CB72Cu, f_089CB748);
    c->r[2] = n;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (stage object): every 2 frames (+134) a palette cycle step: +126 counts down to its end (0x0887EDD8);
   meanwhile +132 moves by +135 and that row is loaded into line 0x3F (kinds 7 / 8 also +112 by +114
   into 0x14), from PALS +68. */
SFA_FUNC(0887EF10, stage_object_palette_cycle) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    *W(c->r[29] + 36) = s1;
    c->r[17] = PALS;
    int8_t t = (int8_t)(S8(o + 134) - 1);
    uint32_t bank = *W(PALS + 68);
    *W(c->r[29] + 40) = ra;
    *B(o + 134) = (uint8_t)t;
    if (t == 0) {
        *B(o + 134) = 2;
        int8_t u = (int8_t)(S8(o + 126) - 1);
        *B(o + 126) = (uint8_t)u;
        if (u == 0) call_on(c, o, 0x0887EFF8u, f_0887EDD8);
        else {
            *H(o + 132) = (uint16_t)(S16(o + 132) + S8(o + 135));
            ARG(0) = 0; ARG(1) = bank + ((uint32_t)S16(o + 132) << 5); ARG(2) = 0; ARG(3) = 0x3F; c->r[8] = 1;
            c->r[31] = 0x0887EF9Cu; f_0880AEA8(c);
            uint32_t k = *B(o + 3);
            if (k == 7 || k == 8) {
                *H(o + 112) = (uint16_t)(S16(o + 112) + S16(o + 114));
                ARG(0) = 0; ARG(1) = *W(PALS + 68) + ((uint32_t)S16(o + 112) << 5); ARG(2) = 0; ARG(3) = 0x14; c->r[8] = 1;
                c->r[31] = 0x0887EFE8u; f_0880AEA8(c);
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (background): on (+4), +32 = GAME +16's low byte, +148 / +164 / +38 / +12 - +15 cleared, +86 = 0xC,
   +9 = 2, +24 = 5, +11 = GAME +22's low byte ^ 1; its set-up (0x088AE2E8), +78 = 0x0951CE52's high
   byte and its page's animation (0x08A2DCF0 / 0x08A2DD50 by 0x09525DE3). */
SFA_FUNC(088ADD54, select_background_begin) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(c->r[29] + 20) = s1;
    c->r[17] = GAME;
    *H(o + 32) = (uint16_t)((uint32_t)S16(GAME + 16) & 0xFF);
    *W(o + 148) = 0;
    *H(o + 86) = 0xC;
    *W(o + 164) = 0;
    *B(o + 9) = 2;
    *H(o + 38) = 0;
    *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 15) = 0; *B(o + 14) = 0;
    *H(o + 24) = 5;
    *B(o + 11) = (uint8_t)((int8_t)((uint32_t)S16(GAME + 22) & 0xFF) ^ 1);
    *W(c->r[29] + 24) = ra;
    call_on(c, o, 0x088ADDE0u, f_088AE2E8);
    *B(o + 78) = (uint8_t)(*H(0x0951CE52u) >> 8);
    uint32_t h = (uint32_t)((int32_t)((uint32_t)S16(GAME + 22) & 0xFF00) >> 8) << 2;
    if (S8(0x09525DE3u) != 0) call2(c, o, *W(0x08A2DCF0u + h), 0x088ADE24u, f_0882196C);
    else call2(c, o, *W(0x08A2DD50u + h), 0x088ADE44u, f_0882196C);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
