/* Small functions the scenarios reach, 114 - 116 instructions: a projectile's start through a
   shared speed block, a fighter's picture parts all reset. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_0886E638(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (projectile): on (+4, +9 / +73 / +123 cleared), its owner's (+88) +24 / +86 / +132 / +164, +352 /
   +356 = 0x08A910E0 / 0x08A91100, pictures 0x08A91788 / 0x08A91690 (+128 / +200, 0x08876998), +260 =
   (0x15 - 2 * +10) & 0x1F; its strength + kind (+346 + +3) row of 0x08A92BA0 (16 bytes: x step, x
   change, y step, y change) through the block 0x08B0400C - 14 (x offset 0x4D), turned for its side,
   0x5B up, +344 from 0x08A92C00 and 0x0887684C (+344). */
SFA_FUNC(08982D04, projectile_start_9) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *H(o + 24) = *H(p + 24);
    *H(o + 132) = (uint16_t)S16(p + 132);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 352) = 0x08A910E0u;
    *W(o + 356) = 0x08A91100u;
    *W(o + 128) = 0x08A91788u;
    *W(o + 200) = 0x08A91690u;
    *W(c->r[29] + 20) = ra;
    c->r[5] = 0x08A910E0u; c->r[6] = 0x08A90000u;
    call_on(c, o, 0x08982D80u, f_08876998);
    *B(o + 260) = (uint8_t)(0x15 - 2u * *B(o + 10));
    *B(o + 123) = 0;
    *B(o + 260) = (uint8_t)((uint32_t)S8(o + 260) & 0x1F);
    *B(o + 73) = 0;
    *W(o + 100) = 0;
    const gaddr vx = 0x08B0400Cu, ax = 0x08B04010u, dx = 0x08B04014u;
    *W(dx) = 0x4D;
    gaddr e = 0x08A92BA0u + 16u * ((uint32_t)S8(o + 346) + *B(o + 3));
    *W(vx) = *W(e);
    *W(ax) = *W(e + 4);
    *W(o + 100) = *W(e + 8);
    *W(o + 108) = *W(e + 12);
    if (*B(o + 11) == 0) {
        *W(vx) = (uint32_t)-(int32_t)*W(vx);
        *W(ax) = (uint32_t)-(int32_t)*W(ax);
        *W(dx) = (uint32_t)-(int32_t)*W(dx);
    }
    *W(o + 96) = *W(vx);
    *W(o + 104) = *W(ax);
    *H(o + 18) = (uint16_t)(S16(o + 18) + (int32_t)*W(dx));
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x5B);
    *B(o + 344) = *B(0x08A92C00u + (uint32_t)S8(o + 346) + *B(o + 3));
    call2(c, o, (uint32_t)S8(o + 344) & 0xFFFF, 0x08982EBCu, f_0887684C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): every picture part of its table (+416) reset (0x0886E638 with the table's +196 / +192):
   +0x9D4 - +0x9EC as 1 - 7, +0x9F0 as 9, +0x9FC as 0x10, +0x10 / +0x18 / +0x20 / +0x28 / +0x2C as 0 / 8 /
   0xC / 0xD / 0xE, +0x80 and +0x84 (twice) as 0xF; then +2504 - +2512 = +0x9D4, +20 = +0x9C8, +28 =
   +0x9F0, +36 = +0x9FC. */
SFA_FUNC(089C393C, fighter_parts_reset) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    gaddr t = *W(f + 416);
    c->r[19] = t;
    *W(sp + 24) = sv[2];
    c->r[18] = t + 0x9D4;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 32) = sv[4]; *W(sp + 36) = ra;
    static const uint32_t off[17] = { 0x9D4, 0x9D8, 0x9DC, 0x9E0, 0x9E4, 0x9E8, 0x9EC, 0x9F0, 0x9FC,
        0x10, 0x18, 0x20, 0x28, 0x2C, 0x80, 0x84, 0x84 };
    static const uint32_t id[17] = { 1, 2, 3, 4, 5, 6, 7, 9, 0x10, 0, 8, 0xC, 0xD, 0xE, 0xF, 0xF, 0xF };
    static const uint32_t back[17] = { 0x089C3974u, 0x089C3988u, 0x089C399Cu, 0x089C39B0u, 0x089C39C4u,
        0x089C39D8u, 0x089C39ECu, 0x089C3A04u, 0x089C3A1Cu, 0x089C3A30u, 0x089C3A44u, 0x089C3A58u,
        0x089C3A6Cu, 0x089C3A80u, 0x089C3A94u, 0x089C3AACu, 0x089C3AC0u };
    for (int i = 0; i < 17; i++) {
        if (i == 7) c->r[17] = t + 0x9F0;
        else if (i == 8) c->r[16] = t + 0x9FC;
        else if (i == 15) c->r[20] = t + 0x84;
        ARG(0) = t + off[i]; ARG(1) = *W(t + 196); ARG(2) = *W(t + 192); ARG(3) = id[i];
        c->r[31] = back[i]; f_0886E638(c);
    }
    for (uint32_t i = 0; i < 3; i++) *W(t + 2504 + 4 * i) = t + 0x9D4;
    *W(t + 20) = t + 0x9C8;
    *W(t + 28) = t + 0x9F0;
    *W(t + 36) = t + 0x9FC;
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
