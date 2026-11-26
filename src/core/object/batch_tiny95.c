/* Small functions the scenarios reach, 36 - 40 instructions: a fighter's picture table with its
   parts reset and a list-0x32 helper, fighters' move / super starts, a level effect's start, a
   menu screen's ten list-0x7D objects. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_089C393C(Cpu *c); void f_089C3B0C(Cpu *c); void f_0886C508(Cpu *c); void f_089CCD1C(Cpu *c);
void f_08822EAC(Cpu *c); void f_088B3C34(Cpu *c); void f_088B3D94(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886C2B4(Cpu *c); void f_0886DBE8(Cpu *c); void f_0893D7C0(Cpu *c); void f_0886C5D0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter): +171 = 1, +12 = -1, its 39-word picture table (+416) from 0x08AB16DC, its parts reset
   (0x089C393C, 0x28), +86 = its side, +164 = 1 and a list-0x32 helper (0x089C3B0C (4, 0)). */
SFA_FUNC(089C33E4, fighter_table_parts_32) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 1;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08AB16DCu + 4 * n);
    call2(c, f, 0x28, 0x089C343Cu, f_089C393C);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    ARG(0) = f; ARG(1) = 4; ARG(2) = 0;
    c->r[31] = 0x089C3464u; f_089C3B0C(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): without +386's bit 0x80 or +568, once 0x0886C508 allows it, step 1/0/7 with +236 = 5,
   +235 = 1, +11 = +697, +196 cleared, +194 = 1; 1 then, else 0. */
SFA_FUNC(0897C088, fighter_move_start_5) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = ra;
    uint32_t r = 0;
    if ((S8(f + 386) & 0x80) == 0 && S8(f + 568) == 0) {
        call_on(c, f, 0x0897C0BCu, f_0886C508);
        if (c->r[2] != 0) {
            *B(f + 236) = 5; *B(f + 235) = 1;
            *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
            *B(f + 11) = (uint8_t)S8(f + 697);
            *B(f + 196) = 0;
            *B(f + 194) = 1;
            r = 1;
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4, +78 cleared), its level step (0x089CCD1C), then its owner's (+88) place, +11 - +13,
   side and +164; +171 = 1. */
SFA_FUNC(089CC984, effect_level_start_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 78) = 0;
    *W(c->r[29] + 24) = ra;
    call_on(c, o, 0x089CC9B4u, f_089CCD1C);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 11) = *B(p + 11);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    *B(o + 171) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen): counted (+2), +8 - +10 cleared, 0x088B3D94, +3 / GAME +10 cleared; with 0x095176F3 at 11 or
   more (unsigned) ten list-0x7D objects (kind 9, +78 = 10 - 1, on); then its eleven palettes
   (0x088B3C34). */
SFA_FUNC(088B36CC, menu_screen_start_7d) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 2) = (uint8_t)(*B(o + 2) + 1);
    *B(o + 10) = 0;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *B(o + 9) = 0;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    *B(o + 8) = 0;
    call_on(c, o, 0x088B3704u, f_088B3D94);
    *B(o + 3) = 0;
    *H(GAME + 10) = 0;
    c->r[18] = 0xA;
    if (!((uint32_t)S8(0x095176F3u) < 0xBu)) {
        c->r[17] = 1;
        for (uint32_t k = 0xA; k != 0; k--) {
            call2(c, 0x7D, 9, 0x088B373Cu, f_08822EAC);
            *B(c->r[2] + 78) = (uint8_t)k;
            c->r[18] = k - 1;
            *B(c->r[2]) = 1;
        }
    }
    call_on(c, o, 0x088B3754u, f_088B3C34);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in the air (+73) once 0x0886C2B4 allows it (+236 = 3), on the ground once 0x0886BF84 does
   (+236 = 2, +11 = +697): step 1/0/7, +235 = 1, 0x0886DBE8; 1 then, else 0. */
static void move_start_air_ground(Cpu *c, uint32_t b_air, uint32_t b_ground, uint32_t b_start) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if (S8(f + 73) != 0) {
        call_on(c, f, b_air, f_0886C2B4);
        if (c->r[2] != 0) { c->r[17] = 1; *B(f + 236) = 3; }
    } else {
        call_on(c, f, b_ground, f_0886BF84);
        if (c->r[2] != 0) {
            *B(f + 236) = 2;
            c->r[17] = 1;
            *B(f + 11) = (uint8_t)S8(f + 697);
        }
    }
    if (c->r[17] != 0) {
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
        *B(f + 235) = 1;
        call_on(c, f, b_start, f_0886DBE8);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088F3228, fighter_move_start_air_1) { move_start_air_ground(c, 0x088F3250u, 0x088F326Cu, 0x088F32B0u); }
SFA_FUNC(088F7C30, fighter_move_start_air_2) { move_start_air_ground(c, 0x088F7C58u, 0x088F7C74u, 0x088F7CB8u); }

/* (fighter): in the air (unless +386 = -1 or +0x3B8) once 0x0886C2B4 allows it, on the ground once
   0x0886BF84 does: its move (0x0893D7C0); 1 then, else 0. */
SFA_FUNC(0893D720, fighter_move_start_7c0) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = f;
    uint32_t r = 0;
    if (S8(f + 73) != 0) {
        if (S8(f + 386) != -1 && S8(f + 0x3B8) == 0) {
            call_on(c, f, 0x0893D75Cu, f_0886C2B4);
            if (c->r[2] != 0) { call_on(c, f, 0x0893D76Cu, f_0893D7C0); r = 1; }
        }
    } else {
        call_on(c, f, 0x0893D788u, f_0886BF84);
        if (c->r[2] != 0) { call_on(c, f, 0x0893D798u, f_0893D7C0); r = 1; }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a super (gauge +366 at 144 or more, +386 not 1) once 0x0886C5D0 allows it: step 1/0/8,
   +236 = 0xD, +11 = +697, +194 / +552 = 2; 1 then, else 0. */
SFA_FUNC(0896327C, fighter_super_start_d) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    uint32_t r = 0;
    if (S8(f + 386) != 1 && !(S16(f + 366) < 144)) {
        call_on(c, f, 0x089632B4u, f_0886C5D0);
        if (c->r[2] != 0) {
            *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 8; *B(f + 7) = 0;
            *B(f + 236) = 0xD;
            *B(f + 11) = (uint8_t)S8(f + 697);
            *B(f + 194) = 2; *B(f + 552) = 2;
            r = 1;
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
