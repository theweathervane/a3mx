/* Small functions the scenarios reach, 86 - 88 instructions: a blinking marker over its owner, a
   screen's eleven palettes, a hit box against a fighter's body box. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088219B0(Cpu *c); void f_0891DDF0(Cpu *c); void f_0891E914(Cpu *c); void f_0891E940(Cpu *c);
void f_0891E97C(Cpu *c); void f_0880AEA8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (marker): while its owner (+88) is shown (+1): every 3 frames (+0x5C) it takes the owner's +12 /
   +13 and blinks (+0x5D; lit: +12 = -1, +13 one on); step 0 starts (+6 = 1, +0x5E / +0x5F = 0x1F);
   step 1 runs (0x0891E940, 0x0891E97C, 0x0891DDF0) until the round's BATTLE +14 or the owner's +77
   ends it (+6 = 2, 0x0891E914 (8)); step 2 waits for +75 to go negative (then +4 on). 0x088219B0. */
SFA_FUNC(0891E380, marker_blink) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 20) = ra;
    if (*B(p + 1) != 0) {
        gaddr t = o + 0x5C;
        int8_t v = (int8_t)(S8(t) - 1);
        *B(t) = (uint8_t)v;
        if (v < 0) {
            *B(t) = 2;
            *B(o + 12) = (uint8_t)S8(p + 12);
            *B(o + 13) = *B(p + 13);
            *B(t + 1) = (uint8_t)(S8(t + 1) ^ 1);
            if (S8(t + 1) != 0) {
                *B(o + 12) = 0xFF;
                *B(o + 13) = (uint8_t)(*B(o + 13) + 1);
            }
        }
        uint32_t st = *B(o + 6);
        if (st == 0) {
            *B(o + 6) = 1;
            *B(t + 2) = 0x1F; *B(t + 3) = 0x1F;
            call_on(c, o, 0x0891E424u, f_088219B0);
        } else if (st == 1) {
            if (S8(BATTLE + 14) == 0 || S8(p + 77) != 0) {
                *B(o + 6) = 2;
                call2(c, o, 8, 0x0891E464u, f_0891E914);
            } else {
                call_on(c, o, 0x0891E474u, f_0891E940);
                call_on(c, o, 0x0891E47Cu, f_0891E97C);
                call_on(c, o, 0x0891E484u, f_0891DDF0);
                call_on(c, o, 0x0891E48Cu, f_088219B0);
            }
        } else if (st == 2) {
            if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            call_on(c, o, 0x0891E4C8u, f_088219B0);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* A screen's eleven palettes (0x0880AEA8): 0x08A35330 +0 / +0x80 / +0x100 / +0x180 / +0x200 into 1 /
   2 / 3 / 4 / 8, 0x08A36EB0 / +0x20 into 5 / 9, 0x08A363D0 / 0x08A36A90 / 0x08A36B10 / 0x08A36C30
   into 0 / 0 / 1 / 0 (all list 2), then 0x08A35038 into 0x10 (list 0). */
SFA_FUNC(088B3C34, screen_eleven_palettes) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    static const uint32_t from[12] = { 0x08A35330u, 0x08A353B0u, 0x08A35430u, 0x08A354B0u, 0x08A35530u,
        0x08A36EB0u, 0x08A36ED0u, 0x08A363D0u, 0x08A36A90u, 0x08A36B10u, 0x08A36C30u, 0x08A35038u };
    static const uint32_t slot[12] = { 1, 2, 3, 4, 8, 5, 9, 0, 0, 1, 0, 0x10 };
    static const uint32_t back[12] = { 0x088B3C60u, 0x088B3C78u, 0x088B3C90u, 0x088B3CA8u, 0x088B3CC0u,
        0x088B3CE0u, 0x088B3CF8u, 0x088B3D14u, 0x088B3D34u, 0x088B3D4Cu, 0x088B3D68u, 0x088B3D84u };
    for (int i = 0; i < 12; i++) {
        if (i == 0) c->r[16] = 0x08A35330u;
        else if (i == 5) c->r[16] = 0x08A36EB0u;
        else if (i == 8) c->r[16] = 0x08A364D0u;
        ARG(0) = i == 11 ? 0 : 2; ARG(1) = from[i]; ARG(2) = 0; ARG(3) = slot[i]; c->r[8] = 1;
        c->r[31] = back[i]; f_0880AEA8(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (attacker, fighter): 1 when the attacker's current hit box (its +296 frame's entry through its
   +200 list into 0x08A8BA70, 8 bytes: x, y, half w, half h) overlaps the fighter's body box (its
   +296's +1 entry of +224, 32 bytes each), both placed at their owners (x turned for +11); else 0. */
SFA_FUNC(089701B0, hitbox_vs_body) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t r = 0;
    uint32_t fa = *B(*W(a + 296));
    if (fa != 0) {
        gaddr ba = 0x08A8BA70u + 8u * *B(*W(a + 200) + (fa << 2));
        uint32_t kb = *B(*W(b + 296) + 1);
        c->r[6] = ba;
        if (kb != 0) {
            gaddr bb = *W(b + 224) + (kb << 5);
            c->r[7] = bb;
            int16_t ya = (int16_t)(S16(ba + 2) + S16(a + 22));
            int16_t dy = (int16_t)((S16(bb + 2) + S16(b + 22)) - ya);
            if (dy < 0) dy = (int16_t)-dy;
            if ((int16_t)(dy - (int16_t)(S16(ba + 6) + S16(bb + 6))) < 0) {
                int16_t xa = (int16_t)(S16(a + 18) + (*B(a + 11) != 0 ? (int16_t)-S16(ba) : S16(ba)));
                int16_t xb = (int16_t)(S16(b + 18) + (*B(b + 11) != 0 ? (int16_t)-S16(bb) : S16(bb)));
                int16_t dx = (int16_t)(xb - xa);
                if (dx < 0) dx = (int16_t)-dx;
                if (!((int16_t)(dx - (int16_t)(S16(ba + 4) + S16(bb + 4))) > 0)) r = 1;
            }
        }
    }
    c->r[2] = r;
}
