/* Small functions the scenarios reach, 64 - 66 instructions: fighters' picture tables with their
   list-9 / list-4 helpers, an effect on its fighter by its level, the select screen's textures
   freed, an effect copied from its owner's motion. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define PICS   0x08E75AD8u
#define TILES  0x092D9D40u
#define PALS   0x092D9E00u
#define KEPT   0x08A055DCu

void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c);
void f_08981D94(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E0540(Cpu *c); void f_089E0C24(Cpu *c);
void f_089C47B0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* The start shared by the fighters' picture-table set-ups: +171 cleared, +12 = -1, the 39-word table
   (+416) from src, 0x0886EB48 (k, tab), +86 = its side, +164 = 1. */
static void table_copy(Cpu *c, gaddr f, gaddr src, uint32_t k, gaddr tab, uint32_t back) {
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(src + 4 * n);
    ARG(0) = f; ARG(1) = k; ARG(2) = tab;
    c->r[31] = back; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
}

/* (fighter): its picture table from 0x08A64238 (9, 0x08A642D4), +0x338 / +0x33C cleared and a list-9
   helper (0x08822F54) of its character linked both ways (its +88, the fighter's +44) with its place,
   +11 - +14, side and +164. */
SFA_FUNC(08917FE0, fighter_table_copy_9) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = ra;
    table_copy(c, f, 0x08A64238u, 9, 0x08A642D4u, 0x08918040u);
    *B(f + 0x33C) = 0; *B(f + 0x338) = 0;
    call2(c, 9, *B(f + 338), 0x08918074u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *W(f + 44) = n;
        *W(n + 16) = *W(f + 16);
        *W(n + 20) = *W(f + 20);
        *B(n + 11) = *B(f + 11);
        *B(n + 14) = *B(f + 14);
        *B(n + 12) = (uint8_t)S8(f + 12);
        *B(n + 13) = *B(f + 13);
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, fighter): on (+1), on the fighter (x + its +268) and its side; a new level (+296's +10,
   kept at +127) takes its animation from 0x08A90CF0, else 0x088219B0 (not while halted, BATTLE +37,
   unless the fighter's +237); then 0x08981D94 (effect, fighter). */
SFA_FUNC(08981C50, effect_on_fighter_level) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = o;
    *B(o + 1) = 1;
    *B(o + 11) = *B(p + 11);
    *H(o + 18) = (uint16_t)(S16(p + 18) + S16(p + 268));
    *H(o + 22) = (uint16_t)S16(p + 22);
    uint32_t lv = *B(*W(p + 296) + 10);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = p;
    uint32_t back;
    if (lv != *B(o + 127)) {
        *B(o + 127) = (uint8_t)lv;
        call2(c, o, *W(0x08A90CF0u + 4 * *B(*W(p + 296) + 10)), 0x08981CCCu, f_0882196C);
        back = 0x08981CD8u;
    } else if (S8(p + 237) != 0) {
        call_on(c, o, 0x08981D30u, f_088219B0);
        back = 0x08981D3Cu;
    } else if (S8(BATTLE + 37) != 0) back = 0x08981D04u;
    else {
        call_on(c, o, 0x08981D14u, f_088219B0);
        back = 0x08981D20u;
    }
    call2(c, o, p, back, f_08981D94);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The select screen's textures freed (between 0x088D7EA0 (1) and (0)): every 12-byte entry of list
   35's tile list (0x08A050B0 +140, its +8 texture, 0x089E0C24), then the kept pictures / tiles /
   palette buffers (KEPT +4 / +8 / +12, 0x089E0540) with PICS / TILES / PALS +52 cleared. */
SFA_FUNC(08815374, select_textures_free_1) {
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    *W(sp + 36) = ra;
    call_on(c, 1, 0x08815398u, f_088D7EA0);
    gaddr list = *W(0x08A050B0u + 140);
    c->r[20] = list;
    c->r[19] = 0;
    c->r[17] = PICS;
    c->r[16] = KEPT;
    if (c->r[19] < *W(list + 4)) {
        c->r[18] = 0;
        for (;;) {
            call_on(c, *W(*W(list) + c->r[18] + 8), 0x088153D8u, f_089E0C24);
            c->r[19]++;
            c->r[18] += 12;
            if (!(c->r[19] < *W(list + 4))) break;
        }
    }
    if (*W(PICS + 52) != 0) {
        call_on(c, *W(KEPT + 4), 0x08815400u, f_089E0540);
        *W(PICS + 52) = 0;
        *W(KEPT + 4) = 0;
    }
    if (*W(KEPT + 8) != 0) {
        call_on(c, *W(KEPT + 8), 0x0881541Cu, f_089E0540);
        *W(TILES + 52) = 0;
        *W(KEPT + 8) = 0;
    }
    if (*W(KEPT + 12) != 0) {
        call_on(c, *W(KEPT + 12), 0x08815440u, f_089E0540);
        *W(PALS + 52) = 0;
        *W(KEPT + 12) = 0;
    }
    call_on(c, 0, 0x08815458u, f_088D7EA0);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (effect): on (+4, +5 / +78 cleared, +1 = 1), 0x089C47B0 (4), then its owner's (+88) +11 - +13 / +24,
   place and steps, 0x28 ahead and 0x27 down, side and +164; +140 = -1, +171 = 1. */
SFA_FUNC(089C41B8, effect_owner_motion) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 5) = 0; *B(o + 1) = 1; *B(o + 78) = 0;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 4, 0x089C41F8u, f_089C47B0);
    *H(o + 24) = *H(p + 24);
    *B(o + 140) = 0xFF;
    *B(o + 13) = *B(p + 13);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 11) = *B(p + 11);
    *W(o + 96) = *W(p + 96);
    *W(o + 104) = *W(p + 104);
    *W(o + 100) = *W(p + 100);
    *W(o + 108) = *W(p + 108);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) != 0 ? 0x28 : -0x28));
    *H(o + 22) = (uint16_t)(S16(o + 22) - 0x27);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its picture table from 0x08A51384 (4, 0x08A51420) and two list-4 helpers (0x08822F54
   (4, 0) as its +44, (4, 1) as its +40) pointing back at it (+88), with its side, +164 and character
   (as +7). */
SFA_FUNC(08900DF4, fighter_table_copy_4) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = ra;
    table_copy(c, f, 0x08A51384u, 4, 0x08A51420u, 0x08900E54u);
    for (uint32_t k = 0; k < 2; k++) {
        call2(c, 4, k, k ? 0x08900EB8u : 0x08900E78u, f_08822F54);
        gaddr n = c->r[2];
        if (n == 0) break;
        *W(n + 88) = f;
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
        *W(f + (k ? 40 : 44)) = n;
        *B(n + 7) = (uint8_t)S8(f + 338);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
