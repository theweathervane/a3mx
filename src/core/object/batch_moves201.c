/* Steps of 73 - 78 instructions: a lunge that flashes every 8 frames until it reaches its mark
   (0x08915584), a knock-up start by the opponent's character (0x0891F810), a bounce that flips its
   guard until the wait or a facing hit ends it (0x08922078), a hop / dash / land by flags
   (0x08995320) and an options row with left / right / confirm / back (0x08834AC8, 2,826 calls).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08822EAC(Cpu *c);
void f_08917840(Cpu *c); void f_0891F938(Cpu *c); void f_0886E3CC(Cpu *c); void f_08864D40(Cpu *c);
void f_08862004(Cpu *c); void f_08834FF0(Cpu *c); void f_0887C4CC(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
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

/* (fighter): +82 counts 0 - 7 (each 0 spawns flash 0x2A / 1 tied to it 108 up); it moves by +96 toward
   its facing; within 16 of its mark (+0x334), when +238 holds its facing + 1, or with +508 below 49 +7
   steps on (+0x33C = 8, x 8.0) into move 8 + strength with +656 = +76 / 2; else 0x08917840. */
SFA_FUNC(08915584, move_lunge_flash_8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *H(o + 82) = (uint16_t)(((uint32_t)S16(o + 82) + 1) & 7);
    int32_t t = S16(o + 82);
    *W(c->r[29] + 20) = s1;
    gaddr m = o + 0x334;
    c->r[17] = m;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (t == 0) {
        ARG(0) = 0x2A; ARG(1) = 1; c->r[31] = 0x089155C0u; f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *W(e + 88) = o;
            *H(e + 18) = (uint16_t)S16(o + 18);
            *H(e + 22) = (uint16_t)(S16(o + 22) + 0x6C);
        }
    }
    uint32_t f = *B(o + 11);
    uint32_t v = *W(o + 96);
    *W(o + 16) = *W(o + 16) + (f != 0 ? v : 0u - v);
    int32_t near = (int32_t)(((uint32_t)S16(o + 18) - (uint32_t)S16(m) + 0x10) & 0xFFFF) < 32;
    if (near || (S8(o + 238) & (int32_t)(f + 1)) != 0 || S16(o + 508) < 49) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(m + 8) = 8;
        int32_t s = S8(o + 194);
        *W(o + 96) = 0x80000;
        call2(c, o, (uint32_t)(s + 8) & 0xFFFF, f_088616EC, 0x08915688u);
        *B(o + 656) = (uint8_t)(S8(o + 76) >> 1);
    } else call1(c, o, f_08917840, 0x08915654u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): against character 0x16 (without +430) +6 = 4, +1 = 1 and move 0x33. Else a knock-up: +78
   cleared, +6 on, +73 = 1, fall -0.75, 48 above the floor, +10 cleared; with +430 (against character 9
   pushed 48 back, x kept at +104) 96 above the floor, +1 = 1, move 0x32 and flash 0x1E / 1 tied to it;
   then 0x0891F938. */
SFA_FUNC(0891F810, move_knockup_start) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t k = S8(o + 430);
    gaddr opp = *W(o + 92);
    if (k == 0 && S8(opp + 338) == 0x16) {
        *B(o + 6) = 4; *B(o + 1) = 1;
        call2(c, o, 0x33, f_0886163C, 0x0891F898u);
        leave(c, sv);
        return;
    }
    uint32_t n = *B(o + 6);
    *B(o + 78) = 0; *B(o + 6) = (uint8_t)(n + 1);
    *W(o + 104) = 0;
    int32_t floor = S16(o + 132);
    *B(o + 73) = 1;
    *W(o + 100) = 0xFFFF4000u;
    *H(o + 22) = (uint16_t)(floor + 0x30);
    *B(o + 10) = 0;
    if (k != 0) {
        if (S8(opp + 338) == 9) {
            *H(o + 0x68) = (uint16_t)S16(o + 18);
            int32_t x = S16(o + 18);
            *H(o + 18) = (uint16_t)(*B(o + 11) == 0 ? x + 0x30 : x - 0x30);
        }
        *H(o + 22) = (uint16_t)(S16(o + 132) + 0x60);
        *B(o + 1) = 1;
        call2(c, o, 0x32, f_0886163C, 0x0891F904u);
        ARG(0) = 0x1E; ARG(1) = 1; c->r[31] = 0x0891F910u; f_08822EAC(c);
        if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    }
    call1(c, o, f_0891F938, 0x0891F928u);
    leave(c, sv);
}

/* (fighter): +656 cleared; a pending +123 (cleared) counts +0x3A9 down (move 0x2C as it passes 0); the
   +83 wait runs down, and out or when +238 matches its facing + 1 +7 steps on, 0x08864D40, it lands (at
   the floor, +20 / +73 cleared) into move 0x2D + strength; else hit cue (8, 4), x by +96, 0x08864D40,
   the guard flipped (+13 by it) and ticked. */
SFA_FUNC(08922078, move_bounce_flip_2d) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    *B(o + 656) = 0;
    enter(c, o, sv);
    if (f != 0) {
        *B(o + 123) = 0;
        gaddr p = o + 0x3A0;
        int32_t n = S8(p + 9);
        if (n >= 0) {
            *B(p + 9) = (uint8_t)(n - 1);
            if ((int8_t)(n - 1) < 0) call2(c, o, 0x2C, f_088616EC, 0x089220C8u);
        }
    }
    gaddr w = o + 0x53;
    int8_t t = (int8_t)(S8(w) - 1);
    *B(w) = (uint8_t)t;
    if (t < 0 || (uint32_t)(S8(o + 238) - 1) == *B(o + 11)) {
        bump(o + 7);
        call1(c, o, f_08864D40, 0x0892216Cu);
        int32_t floor = S16(o + 132), s = S8(o + 194);
        *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0;
        call2(c, o, (uint32_t)(s + 0x2D) & 0xFFFF, f_088616EC, 0x08922190u);
    } else {
        ARG(0) = o; ARG(1) = 8; ARG(2) = 4; c->r[31] = 0x0892210Cu; f_0886E3CC(c);
        *W(o + 16) = *W(o + 16) + *W(o + 96);
        call1(c, o, f_08864D40, 0x08922124u);
        uint32_t g = (uint32_t)S8(o + 78) ^ 1;
        uint32_t b13 = *B(o + 13);
        *B(o + 78) = (uint8_t)g;
        *B(o + 13) = (uint8_t)(b13 + (uint32_t)S8(o + 78));
        call1(c, o, f_08861848, 0x08922148u);
    }
    leave(c, sv);
}

/* (fighter): with +10: a negative +123 hops (step 5, x -0.5, y 8.0 / -0.5) into move 9 + strength, or
   once the +82 wait runs out a dash (step 6, x 2.0, fall -0.375) into move 0x34; otherwise when
   0x08862004 lands it +7 steps on, at the floor, +73 / y speeds cleared, x by strength (0x08A987A0)
   and move 0x10; else ticked. */
SFA_FUNC(08995320, move_flags_hop_dash_land) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t b10 = *B(o + 10);
    enter(c, o, sv);
    if (b10 != 0) {
        if (S8(o + 123) < 0) {
            *B(o + 7) = 5; *W(o + 96) = 0xFFFF8000u;
            int32_t s = S8(o + 194);
            *W(o + 104) = 0; *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFF8000u;
            call2(c, o, (uint32_t)(s + 9) & 0xFFFF, f_088616EC, 0x08995374u);
            leave(c, sv);
            return;
        }
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) {
            *B(o + 7) = 6;
            *W(o + 96) = 0x20000; *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 108) = 0xFFFFA000u;
            call2(c, o, 0x34, f_088616EC, 0x089953C0u);
            leave(c, sv);
            return;
        }
    }
    call1(c, o, f_08862004, 0x089953D0u);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x089953E0u);
    else {
        uint32_t st = *B(o + 7);
        int32_t floor = S16(o + 132);
        *B(o + 7) = (uint8_t)(st + 1);
        int32_t s = S8(o + 194);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0; *W(o + 100) = 0; *W(o + 108) = 0;
        gaddr e = 0x08A987A0u + ((uint32_t)s << 3);
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
        call2(c, o, 0x10, f_088616EC, 0x0899543Cu);
    }
    leave(c, sv);
}

/* (menu, ARG1 / row ARG2): a new row (+1 = 0) waits for +10 = 2 (then +1 = 1 and drawn); else the pad
   of the first player in +89's bits (0x08E75BA0, 44 bytes each, copied to the stack) moves it: back
   (0x8000) or confirm (0x80; +9 keeps the choice, +8 1 for choice 2, else 0) go on with sound 0x39,
   right (8) / left (4) cycle +8 through 0 - 2 with sound 0x38; drawn by 0x08834FF0 (not after a
   confirm). */
SFA_FUNC(08834AC8, menu_options_row) {
    gaddr a = ARG(0), b = ARG(1), p = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 68) = s2; c->r[18] = a;
    uint32_t started = *B(p + 1);
    *W(sp + 64) = s1; c->r[17] = b;
    *W(sp + 60) = s0; *W(sp + 72) = ra;
    c->r[16] = p;
    int draw = 1;
    uint32_t back = 0x08834CE8u;
    if (started == 0) {
        if (*B(p + 10) == 2) { *B(p + 1) = 1; back = 0x08834B18u; }
        else draw = 0;
    } else {
        int32_t bits = S8(a + 89);
        uint32_t i = 0;
        while (i < 3 && !(bits & (1 << i))) i++;
        gaddr pad = 0x08E75BA0u + 44 * i;
        for (int k = 0; k < 11; k++) *W(sp + 16 + 4 * k) = *W(pad + 4 * k);
        uint32_t btn = *W(sp + 28);
        if (btn & 0x8000) {
            uint32_t n = *B(p);
            *B(p + 1) = 0; *B(p) = (uint8_t)(n + 1); *B(p + 8) = 0; *B(p + 9) = 0;
            ARG(0) = 6; ARG(1) = 0x39; ARG(2) = 0xC0; c->r[31] = 0x08834BF8u; f_0887C4CC(c);
        } else if (btn & 0x80) {
            uint32_t n = *B(p), v = *B(p + 8);
            *B(p) = (uint8_t)(n + 1); *B(p + 1) = 0; *B(p + 9) = (uint8_t)v;
            *B(p + 8) = v == 2 ? 1 : 0;
            ARG(0) = 6; ARG(1) = 0x39; ARG(2) = 0xC0; c->r[31] = 0x08834C4Cu; f_0887C4CC(c);
            draw = 0;
        } else if (btn & 8) {
            ARG(0) = 6; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = 0x08834C7Cu; f_0887C4CC(c);
            uint32_t v = *B(p + 8);
            *B(p + 8) = (uint8_t)(v != 0 ? v - 1 : 2);
        } else if (btn & 4) {
            ARG(0) = 6; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = 0x08834CB4u; f_0887C4CC(c);
            uint32_t v = *B(p + 8) + 1;
            *B(p + 8) = (uint8_t)v;
            if ((v & 0xFF) == 3) *B(p + 8) = 0;
        }
    }
    if (draw) { ARG(0) = a; ARG(1) = b; ARG(2) = p; c->r[7] = 0; c->r[31] = back; f_08834FF0(c); }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x50;
}
