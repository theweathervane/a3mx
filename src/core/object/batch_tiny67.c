/* Small functions the scenarios reach, 70 - 71 instructions: a falling effect landing with a
   list-0x12 helper, whether a fighter may start a throw-like move, a fighter's palette for a mode,
   a menu screen's frame. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define PALS   0x092D9E00u

void f_08821368(Cpu *c); void f_08821424(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c);
void f_08822F54(Cpu *c); void f_088632C4(Cpu *c); void f_0886C040(Cpu *c); void f_0880AEA8(Cpu *c);
void f_088B01A0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): falls (y step +100 changing by +108); on reaching its owner's (+88) ground (+132) it lands
   (+5 on) with one of two animations (0x08A92E1C +12 / +16, at random) and, without +139 and outside
   modes 0xD / 0x13 (GAME +114), a list-0x12 helper (0x08822F54) kept at its +40, pointing at it (+88),
   its owner (+92) and the owner's +0x33D (+96), with its +86 / +164. 0x088219B0 and drawn. */
SFA_FUNC(08988364, effect_fall_land_helper) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    *W(o + 100) = vy + *W(o + 108);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 24) = ra;
    if (!(S16(p + 132) < S16(o + 22))) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        int32_t g = S16(p + 132);
        *H(o + 20) = 0;
        *H(o + 22) = (uint16_t)g;
        call_on(c, (uint32_t)g, 0x089883C8u, f_08821368);
        call2(c, o, *W(0x08A92E1Cu + ((c->r[2] & 1) << 2) + 12), 0x089883E8u, f_0882196C);
        if (S8(o + 139) == 0 && S8(GAME + 114) != 0xD && S8(GAME + 114) != 0x13) {
            call2(c, 0x12, 0, 0x08988418u, f_08822F54);
            gaddr n = c->r[2];
            if (n != 0) {
                *B(n + 1) = 0;
                *W(n + 88) = o;
                *W(n + 92) = *W(o + 88);
                *W(n + 96) = p + 0x33D;
                *W(o + 40) = n;
                *H(n + 86) = *H(o + 86);
                *W(n + 164) = *W(o + 164);
            }
        }
    }
    call_on(c, o, 0x08988460u, f_088219B0);
    call_on(c, o, 0x08988468u, f_08821424);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 1 when it may start (then 0x0886C040 (fighter, kind)): in step 1/1/2/0 (kind 0), or a
   player (+251 clear) on the ground (+73 clear) in step 1/0 outside 7 - 9 without +702 (kind 1);
   then +125 / +126 / +778 cleared, neither side held (BATTLE +13 / +14) nor GAME +38, and 0x088632C4
   says no. Else 0. */
SFA_FUNC(0886C5D0, fighter_may_start_2) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t s4 = *B(f + 4), s5 = *B(f + 5), s6 = *B(f + 6), s7 = *B(f + 7);
    int ok;
    if (s4 == 1 && s5 == 1 && s6 == 2 && s7 == 0) { c->r[17] = 0; ok = 1; }
    else {
        c->r[17] = 1;
        ok = S8(f + 251) == 0 && S8(f + 73) == 0 && s4 == 1 && s5 == 0 &&
             s6 != 7 && s6 != 8 && s6 != 9 && S8(f + 702) == 0;
    }
    uint32_t r = 0;
    if (ok) {
        *B(f + 125) = 0; *B(f + 126) = 0;
        *H(f + 778) = 0;
        if ((S8(BATTLE + 13) | S8(BATTLE + 14) | S8(GAME + 38)) == 0) {
            call_on(c, f, 0x0886C6ACu, f_088632C4);
            if (c->r[2] == 0) {
                call2(c, f, c->r[17], 0x0886C6BCu, f_0886C040);
                r = 1;
            }
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, mode): unless already in it (+657), its two palettes (0x0880AEA8) from its side's bank
   (PALS + 4 * +337) for its +376: character 0x29 by 256-byte rows (+0x80 into +13 + 4, 4 rows; then
   the row into +13, 2), the others by 160-byte rows (+0x60 into +13 + 3; the row into +13; 2 each). */
SFA_FUNC(0886EFEC, fighter_mode_palette) {
    gaddr f = ARG(0);
    int32_t mode = (int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = f;
    if (S8(f + 657) != mode) {
        c->r[17] = PALS;
        uint32_t bank = *W(PALS + ((uint32_t)S8(f + 337) << 2));
        uint32_t m = (uint32_t)S8(f + 376);
        if (S8(f + 338) == 0x29) {
            ARG(0) = 0; ARG(1) = bank + (m << 8) + 0x80; ARG(2) = 0; ARG(3) = *B(f + 13) + 4u; c->r[8] = 4;
            c->r[31] = 0x0886F05Cu; f_0880AEA8(c);
            bank = *W(PALS + ((uint32_t)S8(f + 337) << 2));
            m = (uint32_t)S8(f + 376);
            ARG(0) = 0; ARG(1) = bank + (m << 8); ARG(2) = 0; ARG(3) = *B(f + 13); c->r[8] = 2;
            c->r[31] = 0x0886F08Cu; f_0880AEA8(c);
        } else {
            ARG(0) = 0; ARG(1) = bank + m * 160 + 0x60; ARG(2) = 0; ARG(3) = *B(f + 13) + 3u; c->r[8] = 2;
            c->r[31] = 0x0886F0BCu; f_0880AEA8(c);
            bank = *W(PALS + ((uint32_t)S8(f + 337) << 2));
            m = (uint32_t)S8(f + 376);
            ARG(0) = 0; ARG(1) = bank + m * 160; ARG(2) = 0; ARG(3) = *B(f + 13); c->r[8] = 2;
            c->r[31] = 0x0886F0F4u; f_0880AEA8(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (menu screen): its palette (0x0880AEA8: 1, 0x08A30718, 0, 0x1F, 1); screen 9 animated
   (0x088219B0); screens 8 / 9 / 6 react (0x088B01A0) to buttons 1 / 2 / 4 of GAME +22; 3 moves
   screen 8 on (+4), 7 screen 9. */
SFA_FUNC(088B0084, menu_screen_frame) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    *W(sp + 36) = sv[1]; *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3]; *W(sp + 48) = ra;
    ARG(0) = 1; ARG(1) = 0x08A30718u; ARG(2) = 0; ARG(3) = 0x1F; c->r[8] = 1;
    c->r[31] = 0x088B00BCu; f_0880AEA8(c);
    c->r[17] = GAME; c->r[19] = 9; c->r[18] = 8;
    uint32_t k = *B(o + 3);
    if (k == 9) { call_on(c, o, 0x088B00DCu, f_088219B0); k = *B(o + 3); }
    uint32_t b = (uint32_t)S16(GAME + 22) & 0xFF;
    if ((k == 8 && (b & 1)) || (k == 9 && (b & 2)) || (k == 6 && (b & 4))) {
        call_on(c, o, 0x088B012Cu, f_088B01A0);
        b = (uint32_t)S16(GAME + 22) & 0xFF;
    }
    if (b == 3 && *B(o + 3) == 8) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        b = (uint32_t)S16(GAME + 22) & 0xFF;
    }
    if (b == 7 && *B(o + 3) == 9) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
