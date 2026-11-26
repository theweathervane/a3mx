/* Small functions the scenarios reach, 105 - 109 instructions: a menu's state reset with six
   list-0x34 objects, a level effect blinking on its owner. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_088AC3A8(Cpu *c); void f_0882196C(Cpu *c);
void f_088219B0(Cpu *c); void f_0882324C(Cpu *c); void f_0891D5FC(Cpu *c); void f_0891D610(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void pal1(Cpu *c, uint32_t from, uint32_t slot, uint32_t back) {
    ARG(0) = 1; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* (menu): counted (+8), 0x088AC3A8, its three cursors (0x08B027B0 / 0x08B027E0 / 0x08B02810) cleared but
   the last's +4 = 0xFB000000, +9 cleared, +80 = 1, 0x092E8940 +122 = 2, GAME +10 cleared, GAME +22's
   low byte = 0x1F, GAME +20 bit 0x100 flipped; with 0x095176F3 at 6 or more six list-0x34 objects
   (0 - 5) on; then three palettes (0x08A2C148 / 0x08A2C168 into 0, 0x08A2C188 into 0x1E). */
SFA_FUNC(088AAADC, menu_reset_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 36) = s1;
    c->r[17] = o;
    *W(c->r[29] + 32) = s0;
    c->r[16] = 0;
    *W(c->r[29] + 40) = ra;
    *B(o + 8) = (uint8_t)(*B(o + 8) + 1);
    call_on(c, o, 0x088AAB04u, f_088AC3A8);
    static const uint32_t cur[3] = { 0x08B02810u, 0x08B027E0u, 0x08B027B0u };
    for (int i = 0; i < 3; i++) { *H(cur[i] + 18) = 0; *H(cur[i] + 16) = 0; }
    for (int i = 1; i < 3; i++) { *W(cur[i] + 4) = 0; *W(cur[i]) = 0; }
    *B(o + 9) = 0;
    *H(GAME + 10) = 0;
    c->r[16] = 1;
    *B(o + 80) = 1;
    *B(0x092E8940u + 122) = 2;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
    *W(0x08B02810u) = 0;
    *W(0x08B02810u + 4) = 0xFB000000u;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 0x1F);
    *H(GAME + 20) = (uint16_t)(S16(GAME + 20) ^ 0x100);
    if (S8(0x095176F3u) >= 6) {
        static const uint32_t back[6] = { 0x088AABC4u, 0x088AABD4u, 0x088AABE4u, 0x088AABF4u, 0x088AAC04u, 0x088AAC14u };
        for (uint32_t i = 0; i < 6; i++) {
            call2(c, 0x34, i, back[i], f_08822EAC);
            *B(c->r[2]) = 1;
        }
    }
    pal1(c, 0x08A2C148u, 0, 0x088AAC34u);
    pal1(c, 0x08A2C168u, 0, 0x088AAC50u);
    pal1(c, 0x08A2C188u, 0x1E, 0x088AAC6Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (effect): step 0 takes its owner's (+88) +12 - +14 / +24 (+78 / +82 cleared); step 1 lasts while
   the owner is its character (+338 = +3) and, with a level (+296's +10), sets itself up from it
   (0x0891D610) and shows on (+1): a new level (+78) takes its animation from 0x08A60C70, an unchanged
   one blinks every 2 frames (+0x52 / +0x53: +12 = -1, +13 one on) with 0x088219B0; 0x0891D5FC
   either way (no level: +78 cleared); steps 2 - 3 end it (the owner's +44 let go, 0x0882324C). */
SFA_FUNC(0891D448, effect_level_blink) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 4);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    if (st == 0) {
        *B(o + 4) = 1;
        *H(o + 24) = *H(p + 24);
        *B(o + 12) = (uint8_t)S8(p + 12);
        *B(o + 13) = *B(p + 13);
        *H(o + 82) = 0;
        *B(o + 14) = *B(p + 14);
        *B(o + 78) = 0;
    } else if (st == 1) {
        *B(o + 1) = 0;
        if ((uint32_t)S8(p + 338) != *B(o + 3)) *B(o + 4) = 2;
        else {
            uint32_t lv = *B(*W(p + 296) + 10);
            c->r[18] = lv;
            if (lv == 0) *B(o + 78) = 0;
            else {
                ARG(0) = o; ARG(1) = p; ARG(2) = lv;
                c->r[31] = 0x0891D4ECu; f_0891D610(c);
                *B(o + 12) = (uint8_t)S8(p + 12);
                *B(o + 13) = *B(p + 13);
                *B(o + 1) = 1;
                if ((uint32_t)S8(o + 78) != lv) {
                    *B(o + 78) = (uint8_t)lv;
                    call2(c, o, *W(0x08A60C70u + (lv << 2)), 0x0891D52Cu, f_0882196C);
                    call_on(c, o, 0x0891D534u, f_0891D5FC);
                } else {
                    gaddr t = o + 0x52;
                    int8_t v = (int8_t)(S8(t) - 1);
                    *B(t) = (uint8_t)v;
                    if (v < 0) {
                        *B(t) = 0;
                        *B(o + 12) = (uint8_t)S8(p + 12);
                        *B(o + 13) = *B(p + 13);
                        *B(t + 1) = (uint8_t)(*B(t + 1) ^ 1);
                        if (*B(t + 1) != 0) {
                            *B(o + 12) = 0xFF;
                            *B(o + 13) = (uint8_t)(*B(o + 13) + 1);
                        }
                    }
                    call_on(c, o, 0x0891D5A4u, f_088219B0);
                    call_on(c, o, 0x0891D5ACu, f_0891D5FC);
                }
            }
        }
    } else if (st == 2) *B(o + 4) = 3;
    else if (st == 3) {
        if (*W(p + 44) == o) *W(p + 44) = 0;
        call_on(c, o, 0x0891D5E4u, f_0882324C);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
