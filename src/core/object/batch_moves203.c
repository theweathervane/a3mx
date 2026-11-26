/* Steps of 80 - 83 instructions: a part character's random pick of three follow-ups (0x089BEFCC), a part
   effect that lives on a timer then stops and fades (0x089CBB1C), a copy spawned on an end frame and
   a pair of glows that follow the fighter while its parent animates (0x089040B4). Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_08821368(Cpu *c); void f_088D3180(Cpu *c); void f_088D2E54(Cpu *c); void f_089C3B0C(Cpu *c);
void f_088D3EB8(Cpu *c); void f_088D4110(Cpu *c); void f_088D3D94(Cpu *c); void f_088D3DD8(Cpu *c);
void f_08821424(Cpu *c); void f_08822D2C(Cpu *c); void f_08822EAC(Cpu *c); void f_08904248(Cpu *c);
void f_0886E3CC(Cpu *c);

#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (character): +7 on, +824 cleared, and by a random number (0x08821368) mod 3 kept in +3: 0 move 0x22,
   1 a leap over (turned around, +82 = 4, +591 / +671 = 1, 356 across by its side, 80 above the floor,
   x 10.25 / -0.125, y 5.5 / -0.3125, 0x088D3180, move 0x31, 0x088D2E54 (0x54)), 2 0x089C3B0C (0, 0)
   and move 0x32 (+820 cleared for 0 and 2). */
SFA_FUNC(089BEFCC, part_random_pick3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *W(o + 824) = 0; *B(o + 7) = (uint8_t)(st + 1);
    c->r[31] = 0x089BEFF0u; f_08821368(c);
    DIV(c, c->r[2], 3);
    *B(o + 3) = (uint8_t)c->hi;
    uint32_t v = *B(o + 3);
    if (v == 0) { *W(o + 820) = 0; call2(c, o, 0x22, f_0886163C, 0x089BF024u); }
    else if (v == 1) {
        uint32_t f = *B(o + 11);
        *H(o + 82) = 4;
        *B(o + 11) = (uint8_t)(f ^ 1);
        *B(o + 591) = 1; *B(o + 671) = 1;
        int32_t x = S16(o + 18);
        uint32_t d = S8(o + 337) == 0 ? 0x164u : 0xFFFFFE9Cu;
        int32_t floor = S16(o + 132);
        *H(o + 18) = (uint16_t)((uint32_t)x + d);
        *H(o + 22) = (uint16_t)(floor + 0x50);
        *W(o + 96) = 0xA4000; *W(o + 104) = 0xFFFFE000u; *W(o + 100) = 0x58000; *W(o + 108) = 0xFFFFB000u;
        *B(o + 73) = 1;
        call1(c, o, f_088D3180, 0x089BF0C0u);
        call2(c, o, 0x31, f_0886163C, 0x089BF0CCu);
        call2(c, o, 0x54, f_088D2E54, 0x089BF0D8u);
    } else if (v == 2) {
        *W(o + 820) = 0;
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089BF0F0u; f_089C3B0C(c);
        call2(c, o, 0x32, f_0886163C, 0x089BF0FCu);
    }
    leave(c, sv);
}

static void next_state(gaddr o) {
    uint32_t n = *B(o + 4);
    *B(o + 1) = 0; *B(o + 4) = (uint8_t)(n + 1); *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
}

/* (effect): while the match runs and 0x088D3EB8 lets it live: when 0x088D4110 ends it, the next state;
   else the +143 timer runs down and past 0 it stops (once, +141: the picture 0x08AB2FDC from strength
   + 0xB, speeds cleared) and goes to the next state once its picture ends; then 0x088D3DD8 and it flies
   (+16 by +96 by +104, +20 by +100 by +108). Drawn (0x08821424). */
SFA_FUNC(089CBB1C, part_effect_timed) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(MATCH + 37) == 0 && S8(MATCH + 78) == 0) {
        call1(c, o, f_088D3EB8, 0x089CBB54u);
        if (c->r[2] != 0) { leave(c, sv); return; }
        call1(c, o, f_088D4110, 0x089CBB64u);
        if (c->r[2] == 0) next_state(o);
        else {
            int8_t t = (int8_t)(S8(o + 143) - 1);
            *B(o + 143) = (uint8_t)t;
            if (t < 0) {
                if (S8(o + 141) == 0) {
                    *B(o + 141) = 1;
                    ARG(0) = o; ARG(1) = 0x08AB2FDCu; ARG(2) = (uint32_t)(S8(o + 194) + 0xB);
                    c->r[31] = 0x089CBBCCu; f_088D3D94(c);
                    *W(o + 96) = 0; *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 108) = 0;
                }
                if (S8(o + 75) < 0) next_state(o);
            }
        }
        call1(c, o, f_088D3DD8, 0x089CBC0Cu);
        uint32_t vx = *W(o + 96), vy = *W(o + 100);
        *W(o + 16) = *W(o + 16) + vx;
        *W(o + 20) = *W(o + 20) + vy;
        *W(o + 96) = vx + *W(o + 104);
        *W(o + 100) = vy + *W(o + 108);
    }
    call1(c, o, f_08821424, 0x089CBC4Cu);
    leave(c, sv);
}

/* (fighter): ticked; ended: standing; on the end frame a frame flag (+76 / +77 cleared) spawns a copy
   (0x08822D2C 0 / 2) with the fighter's picture, palette, place and side: +346 = strength + 3, +112 =
   strength + 2, +255 = 3, +347 = 1, +260 from 0x08A8A954, +311 = 2, +312 = 8; kept at +228 and counted
   (+536). */
SFA_FUNC(08964458, move_end_frame_copy_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0896446Cu);
    int32_t e75 = S8(o + 75);
    if (e75 & 0x80) call1(c, o, f_0886469C, 0x08964484u);
    else if (e75 == 0 && S8(o + 77) != 0) {
        *B(o + 77) = 0; *B(o + 76) = 0;
        ARG(0) = o; ARG(1) = 0; ARG(2) = 2; c->r[31] = 0x089644B8u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 122) = *B(o + 122); *B(e + 81) = *B(o + 81); *B(e + 121) = *B(o + 121);
            *B(e + 346) = (uint8_t)(S8(o + 194) + 3);
            *H(e + 112) = (uint16_t)(S8(o + 194) + 2);
            *B(e + 255) = 3; *B(e + 347) = 1;
            *B(e + 14) = *B(o + 14);
            *H(e + 86) = (uint16_t)S8(o + 337);
            *W(e + 164) = *W(o + 164);
            *B(e + 11) = *B(o + 11); *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13);
            *H(e + 38) = *H(o + 38); *H(e + 18) = *H(o + 18);
            uint32_t y = *H(o + 22);
            *W(e + 88) = o; *H(e + 22) = (uint16_t)y;
            uint32_t n = *B(o + 536);
            *W(o + 228) = e;
            int32_t s = S8(o + 194);
            *B(o + 536) = (uint8_t)(n + 1);
            *B(e + 260) = (uint8_t)S8(0x08A8A954u + (uint32_t)s);
            *B(e + 311) = 2; *B(e + 312) = 8;
        }
    }
    leave(c, sv);
}

/* Glow `e` (kind +9 = `k`) takes the fighter's picture, palette, place and floor and is tied to it. */
static void glow_copy(gaddr e, gaddr o, uint32_t k, int with_palette) {
    *B(e + 9) = (uint8_t)k;
    *B(e + 14) = *B(o + 14); *B(e + 11) = *B(o + 11);
    *H(e + 38) = *H(o + 38);
    if (with_palette) { *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13); }
    *W(e + 16) = *W(o + 16); *W(e + 20) = *W(o + 20);
    *H(e + 132) = *H(o + 132);
    uint32_t pic = *H(o + 24);
    *W(e + 88) = o; *H(e + 24) = (uint16_t)pic;
}

/* (fighter): while its parent (+40) animates: ended, step 5 into move 0x24 + 2 x strength; else a frame
   flag (cleared) spawns a pair of glows (0x08822EAC 0x19 / 1 and 0x19 / 0) on it, then 0x08904248 and
   0x0886E3CC (9, 4). */
SFA_FUNC(089040B4, move_glow_pair) {
    gaddr o = ARG(0); uint32_t sv[2];
    gaddr par = *W(o + 40);
    enter(c, o, sv);
    if (S8(par + 75) < 0) {
        if (S8(o + 75) < 0) {
            *B(o + 7) = 5;
            call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0x24) & 0xFFFF, f_088616EC, 0x08904100u);
        } else {
            if (S8(o + 77) != 0) {
                *B(o + 77) = 0;
                ARG(0) = 0x19; ARG(1) = 1; c->r[31] = 0x08904120u; f_08822EAC(c);
                gaddr e = c->r[2];
                if (e != 0) {
                    glow_copy(e, o, 1, 1);
                    ARG(0) = 0x19; ARG(1) = 0; c->r[31] = 0x0890418Cu; f_08822EAC(c);
                    gaddr e2 = c->r[2];
                    if (e2 != 0) glow_copy(e2, o, 0, 0);
                }
            }
            call1(c, o, f_08904248, 0x089041E0u);
            ARG(0) = o; ARG(1) = 9; ARG(2) = 4; c->r[31] = 0x089041F0u; f_0886E3CC(c);
        }
    }
    leave(c, sv);
}
