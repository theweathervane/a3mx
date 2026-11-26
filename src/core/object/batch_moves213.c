/* Steps of 111 - 112 instructions: an intro that drops in with a ring of eight sparks (0x08993C6C) and
   a copy spawn with an extra one at full meter, each set up by 0x089A3B6C (0x089A55F4). Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_08862004(Cpu *c); void f_08822EAC(Cpu *c);
void f_08822D2C(Cpu *c); void f_089A3B6C(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter): intro by +6: 0 with a partner (+600) a drop (+6 = 1, +82 = 24, 224 above the floor, y -2.0 /
   -0.375; 32 back on the clock's +56 = 2) into move 0x33, without one +6 = 3 and move 0x22 (+1 = 1);
   1 waits for the game's +6 = 2, then +656 = 39 and eight sparks 0x44 / 8 tied to it (+78 = 7 .. 0);
   2 the +82 wait, then lands when 0x08862004 says so; 3 standing once ended, else ticked. */
SFA_FUNC(08993C6C, intro_drop_sparks) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    uint32_t n = *B(o + 6);
    c->r[17] = n;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (n == 0) {
        uint32_t mv;
        if (*W(o + 600) == 0) { *B(o + 6) = 3; mv = 0x22; }
        else {
            int32_t floor = S16(o + 132);
            *B(o + 6) = 1; *B(o + 1) = 0; *H(o + 82) = 0x18;
            *H(o + 22) = (uint16_t)(floor + 0xE0);
            *W(o + 96) = 0; *W(o + 104) = 0; *W(o + 100) = 0xFFFE0000u; *W(o + 108) = 0xFFFFA000u;
            if (S16(MATCH + 56) == 2) *H(o + 18) = (uint16_t)(S16(o + 18) - 0x20);
            mv = 0x33;
        }
        *B(o + 1) = 1;
        call2(c, o, mv, f_0886163C, 0x08993D08u);
    } else if (n == 1) {
        if (*H(GAME + 6) == 2) {
            *B(o + 6) = 2; *B(o + 1) = 1; *B(o + 656) = 0x27;
            int32_t k = 7;
            while (k >= 0) {
                c->r[17] = (uint32_t)k;
                ARG(0) = 0x44; ARG(1) = 8; c->r[31] = 0x08993D54u; f_08822EAC(c);
                gaddr e = c->r[2];
                if (e == 0) { k = -2; break; }
                *B(e + 78) = (uint8_t)k; *W(e + 88) = o;
                k--;
            }
            c->r[17] = (uint32_t)k;
        }
    } else if (n == 2) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) {
            call1(c, o, f_08862004, 0x08993DACu);
            if (c->r[2] != 0) {
                *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
                *W(o + 20) = (uint32_t)S16(o + 132) << 16;
                *B(o + 73) = 0;
            }
        }
    } else if (n == 3) {
        if (S8(o + 75) < 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            call2(c, o, 0, f_0886163C, 0x08993E04u);
        } else call1(c, o, f_08861848, 0x08993E14u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The copy `e` takes the fighter's picture, palette, place and side (+346 = strength, +255 / +347 /
   +112 cleared), is kept at +228, tied to it and counted (+536). */
static void copy_fighter(gaddr e, gaddr o) {
    *B(e + 122) = *B(o + 122); *B(e + 121) = *B(o + 121);
    *B(e + 346) = *B(o + 194);
    uint32_t k80 = *B(o + 80);
    *B(e + 255) = 0; *B(e + 80) = (uint8_t)k80; *B(e + 347) = 0;
    *B(e + 14) = *B(o + 14);
    *H(e + 86) = (uint16_t)S8(o + 337);
    *W(e + 164) = *W(o + 164);
    *B(e + 11) = *B(o + 11); *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13);
    *H(e + 38) = *H(o + 38); *H(e + 18) = *H(o + 18);
    uint32_t y = *H(o + 22);
    *H(e + 112) = 0; *H(e + 22) = (uint16_t)y;
    *W(o + 228) = e; *W(e + 88) = o;
}

/* (fighter): ticked; a frame flag steps +7 on and spawns a copy (0x08822D2C 0x26, set up by 0x089A3B6C);
   with +251 and the meter (+286) at +662 or more a second one, its +280 = +662. */
SFA_FUNC(089A55F4, move_flag_copy_pair) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x089A5608u);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        ARG(0) = o; ARG(1) = 0x26; ARG(2) = 0; c->r[31] = 0x089A5630u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            copy_fighter(e, o);
            *B(o + 536) = (uint8_t)(*B(o + 536) + 1);
            call1(c, e, f_089A3B6C, 0x089A56D0u);
            if (S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662))) {
                ARG(0) = o; ARG(1) = 0x26; ARG(2) = 0; c->r[31] = 0x089A56FCu; f_08822D2C(c);
                gaddr e2 = c->r[2];
                if (e2 != 0) {
                    copy_fighter(e2, o);
                    uint32_t n = *B(o + 536);
                    int32_t m = S16(o + 662);
                    *B(o + 536) = (uint8_t)(n + 1);
                    *B(e2 + 280) = (uint8_t)m;
                    call1(c, e2, f_089A3B6C, 0x089A57A4u);
                }
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
