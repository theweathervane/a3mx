/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 75 - 77
   instructions: the fight HUD's frame (its 54 cells' shades, then its parts), the music's pace by the
   clock and health, the VS screen's mode banner, the select screen's slot-8 pictures put back. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_088075F8(Cpu *c); void f_089E0C24(Cpu *c); void f_0882196C(Cpu *c); void f_088988FC(Cpu *c);
void f_0889CEE4(Cpu *c); void f_0889CFE8(Cpu *c); void f_0889F5F8(Cpu *c); void f_0889F7F4(Cpu *c);
void f_0889FB94(Cpu *c); void f_088369AC(Cpu *c); void f_088369C8(Cpu *c); void f_08836E60(Cpu *c);
void f_08836E90(Cpu *c); void f_0888D8BC(Cpu *c); void f_0888D924(Cpu *c); void f_0888DA7C(Cpu *c);
void f_0888F0EC(Cpu *c); void f_0888F9D4(Cpu *c); void f_0888FF38(Cpu *c);

#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- the fight HUD */
/* (hud): each frame its 54 cells' shades (+0x34 + 64 n: the three colour bytes at +36 / +42 / +48 and
   the three after each) 0x40 while halted (BATTLE +37) or paused (GAME +89 / +145), else 0xFF; then
   its health bars, 0x0888D8BC, the pop-ups, the stun gauges, 0x0888D924 and 0x0888DA7C. */
SFA_FUNC(0888D788, hud_frame) {
    gaddr o = ARG(0); WITH_S0(o);
    uint8_t shade = (S8(BATTLE + 37) != 0 || S8(GAME + 89) != 0 || S8(GAME + 145) != 0) ? 0x40 : 0xFF;
    for (int n = 0; n < 54; n++) {
        gaddr p = o + 0x34 + 0x40 * (uint32_t)n;
        *B(p + 36) = shade; *B(p + 42) = shade; *B(p + 48) = shade;
        uint8_t a = *B(p + 36), b = *B(p + 42);
        *B(p + 39) = a; *B(p + 38) = a; *B(p + 37) = a;
        *B(p + 45) = b;
        uint8_t d = *B(p + 48);
        *B(p + 44) = b; *B(p + 43) = b;
        *B(p + 51) = d; *B(p + 50) = d; *B(p + 49) = d;
    }
    call_on(c, o, 0x0888D884u, f_0888F0EC);
    call_on(c, o, 0x0888D88Cu, f_0888D8BC);
    call_on(c, o, 0x0888D894u, f_0888F9D4);
    call_on(c, o, 0x0888D89Cu, f_0888FF38);
    call_on(c, o, 0x0888D8A4u, f_0888D924);
    call_on(c, o, 0x0888D8ACu, f_0888DA7C);
    END_S0();
}
/* The music's pace (0x08836E60) by the clock (BATTLE +9: 0xA under 20, 6, 4, 6, 0x10 from 80), by the
   player's fighter's health (BATTLE +112, a human: -8 under 16, 0, 8, 0xA, 0x10 from 112) and by BATTLE
   +13 (2 with no fighter, -8 for the CPU); then 0x088369C8, 0x08836E90 and 0x088369AC. Not with GAME
   +57 = 3 or fight state 7. */
SFA_FUNC(08836F24, music_pace) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = 0x092E0000u;
    if (S8(GAME + 57) != 3) {
        c->r[16] = BATTLE;
        if (S8(BATTLE + 79) != 7) {
            int32_t clock = S8(BATTLE + 9);
            int32_t v = clock >= 80 ? 0x10 : clock >= 60 ? 6 : clock >= 40 ? 4 : clock >= 20 ? 6 : 0xA;
            ARG(0) = (uint32_t)(int32_t)(int16_t)v;
            c->r[31] = 0x08836FA0u; f_08836E60(c);
            gaddr f = *W(BATTLE + 112);
            if (f != 0 && S8(f + 373) == 0) {
                int32_t h = S16(f + 112);
                int32_t w = h >= 112 ? 0x10 : h >= 80 ? 0xA : h >= 48 ? 8 : h >= 16 ? 0 : -8;
                ARG(0) = (uint32_t)(int32_t)(int16_t)w;
                c->r[31] = 0x08836FFCu; f_08836E60(c);
            }
            int32_t k = S8(BATTLE + 13);
            if (k != 0) {
                gaddr g = *W(BATTLE + 112);
                c->r[16] = g;
                int32_t x = g == 0 ? 2 : S8(g + 373) != 0 ? -8 : k;
                ARG(0) = (uint32_t)(int32_t)(int16_t)x;
                c->r[31] = 0x08837030u; f_08836E60(c);
            }
            c->r[31] = 0x08837038u; f_088369C8(c);
            c->r[31] = 0x08837040u; f_08836E90(c);
            c->r[31] = 0x08837048u; f_088369AC(c);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- VS screen */
/* (banner): the VS screen's mode banner: on its first frame its set-up (+12 = -1, +13 = 0xB, +148 = 2,
   +86 = 8, 0x0889CFE8), y = 0x0889FB94's answer + 16 x BATTLE +20, its pictures by its fighter's side
   (0x0889F5F8, 0x0889CEE4 (0x12 + side)) and the animation at 0x08A283EC (7, or 8 for side 1); then at
   once with BATTLE +81 (0x0889F7F4), or moved on at GAME +20 high byte 4. */
SFA_FUNC(0889F4C8, vs_mode_banner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[18] = *B(o + 5); c->r[17] = BATTLE; c->r[16] = o;
    if (c->r[18] == 0) {
        *B(o + 5) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 0xB;
        *W(o + 148) = 2;
        *H(o + 86) = 8;
        call2(c, o, 0, 0x0889F524u, f_0889CFE8);
        call_on(c, o, 0x0889F52Cu, f_0889FB94);
        *H(o + 22) = (uint16_t)(c->r[2] + ((uint32_t)(int32_t)S16(BATTLE + 20) << 4));
        c->r[18] = 7;
        call_on(c, *W(o + 92), 0x0889F54Cu, f_088988FC);
        c->r[17] = (uint32_t)(int32_t)(int16_t)c->r[2];
        if (c->r[17] != 0) c->r[18] = 8;
        call2(c, o, c->r[17] & 0xFFFF, 0x0889F568u, f_0889F5F8);
        call2(c, o, (c->r[17] + 0x12) & 0xFFFF, 0x0889F578u, f_0889CEE4);
        call2(c, o, *W(0x08A283ECu + (c->r[18] << 2)), 0x0889F594u, f_0882196C);
    } else if (S8(BATTLE + 81) != 0) call_on(c, o, 0x0889F5B0u, f_0889F7F4);
    else if (((uint32_t)(int32_t)S16(GAME + 20) & 0xFF00) == 0x400) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- select screen */
/* When slot 8's picture list, tile set and palettes (and slot 11's picture list and tile set) are all
   empty: put back from the copies at 0x08A0562C / 0x08A056D4 / 0x08A0577C (+32 / +44); textures 0xE0 -
   0xE5 freed if 0x08E75C54 was set (then set); the current 216-byte record's +202 - +204 / +208
   cleared; and 0x088075F8 (0x08A0319C's word, 0, 0). */
SFA_FUNC(08814134, select_slot_8_back) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr pics = 0x08E75AD8u, tiles = 0x092D9D40u, pals = 0x092D9E00u;
    if (*W(pics + 32) == 0 && *W(tiles + 32) == 0 && *W(pals + 32) == 0 && *W(pics + 44) == 0 && *W(tiles + 44) == 0) {
        uint32_t a = *W(0x08A0562Cu + 32), b = *W(0x08A0577Cu + 32);
        *W(pics + 32) = a;
        uint32_t d = *W(0x08A056D4u + 32);
        *W(pals + 32) = b;
        *W(tiles + 32) = d;
        uint32_t e = *W(0x08A0562Cu + 44), g = *W(0x08A056D4u + 44);
        *W(pics + 44) = e;
        *W(tiles + 44) = g;
        c->r[16] = 0x08E70000u;
        if (*H(0x08E75C54u) != 0) {
            for (c->r[17] = 0; (int32_t)c->r[17] < 6; c->r[17]++) call_on(c, c->r[17] + 0xE0, 0x088141E8u, f_089E0C24);
        }
        *H(0x08E75C54u) = 1;
        gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
        *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
        ARG(0) = *W(0x08A03140u + 92); ARG(1) = 0; ARG(2) = 0;
        c->r[31] = 0x0881424Cu; f_088075F8(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
