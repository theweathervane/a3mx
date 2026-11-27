/* Throw holds: once a throw has caught, the step that lands it (0x0886B144 / 0x0886B68C with the throw's
   hit number) when the animation flags it, and the launches that follow. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886B144(Cpu *c); void f_0886B68C(Cpu *c); void f_0886B6A8(Cpu *c);
void f_0886DDCC(Cpu *c); void f_0886E3CC(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c);
void f_0886A890(Cpu *c); void f_088D3EC0(Cpu *c);

#define HIT_RES 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, gaddr o, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = a; ARG(2) = b; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (fighter): on +77 step 4 and the throw lands (0x57); ticked. */
SFA_FUNC(0892E634, hold_57) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[5] = (uint32_t)S8(o + 77);
    if (c->r[5] != 0) { *B(o + 7) = 4; call2(c, o, 0x57, f_0886B144, 0x0892E660u); }
    call1(c, o, f_08861848, 0x0892E668u);
    leave(c, sv);
}

/* (fighter): on +77 step 2 and the throw lands (0x3F, 0x40 with +193); ticked. */
SFA_FUNC(0890C83C, hold_3f) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[5] = (uint32_t)S8(o + 77);
    if (c->r[5] != 0) {
        *B(o + 7) = 2;
        call2(c, o, S8(o + 193) != 0 ? 0x40 : 0x3F, f_0886B144, 0x0890C878u);
    }
    call1(c, o, f_08861848, 0x0890C880u);
    leave(c, sv);
}

/* (fighter): ticked and 0x0886DDCC; on +77 bit 7 +7 on and the throw lands (0x4A). */
SFA_FUNC(089984BC, hold_4a) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x089984D0u);
    call1(c, o, f_0886DDCC, 0x089984D8u);
    if (S8(o + 77) & 0x80) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, 0x4A, f_0886B144, 0x08998500u);
    }
    leave(c, sv);
}

/* (fighter): on +77 +7 on, the result's +35 = 0x30, the hit 0x51 (0x0886B68C) and 0x0886E3CC (9, 5) on
   both sides; ticked and 0x0886DDCC. */
SFA_FUNC(0899843C, hold_51) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[5] = (uint32_t)S8(o + 77);
    if (c->r[5] != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(HIT_RES + 35) = 0x30;
        call2(c, o, 0x51, f_0886B68C, 0x0899847Cu);
        call3(c, o, 9, 5, f_0886E3CC, 0x0899848Cu);
        call3(c, *W(o + 92), 9, 5, f_0886E3CC, 0x0899849Cu);
    }
    call1(c, o, f_08861848, 0x089984A4u);
    call1(c, o, f_0886DDCC, 0x089984ACu);
    leave(c, sv);
}

/* (fighter): ticked once its animation ends; before that the throw lands by strength (0x08A58362), +7 on,
   a jump (x -2.0, y 4.25, brake -0.28125), +73 = 1 and move 0xD. */
SFA_FUNC(0890A6A8, hold_tab_8362) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) < 0) call1(c, o, f_08861848, 0x0890A6C8u);
    else {
        call2(c, o, *H(0x08A58362u + (uint32_t)(S8(o + 194) * 2)), f_0886B144, 0x0890A6F0u);
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 96) = 0xFFFE0000u; *W(o + 100) = 0x44000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFB800u;
        *B(o + 73) = 1;
        call2(c, o, 0xD, f_088616EC, 0x0890A730u);
    }
    leave(c, sv);
}

/* (fighter): 0x088D3EC0. Without a +228 part: +82 counts up from 0 (+74 = 1, +7 on and +536 down once it
   started). With one: +828 counts down; at 0 the grab 0x5D + strength (0x0886A890): caught step 6, the
   part's +123 = 1, the hit 0x60, +591 = 1, the opponent nudged 2 off a wall (+238 cleared), +828 / +832
   = 0x3C, move 0x32, the opponent's move 6 (0x08861724) and a lift (y 6.0); missed, once the part's +4
   reaches 2, +74 = 1 and +7 on. A frame keeping s0..s2. */
SFA_FUNC(089C8EA4, hold_part_5d) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    gaddr part = *W(o + 228); c->r[17] = part;
    call1(c, o, f_088D3EC0, 0x089C8EC4u);
    if (part == 0) {
        int16_t h = S16(o + 82);
        if (h != 0) {
            *B(o + 74) = 1;
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 536) = (uint8_t)(*B(o + 536) - 1);
        } else *H(o + 82) = (uint16_t)(h + 1);
    } else {
        int32_t t = (int32_t)*W(o + 828);
        if (t > 0) *W(o + 828) = (uint32_t)(t - 1);
        else {
            call2(c, o, (uint32_t)(S8(o + 194) + 0x5D), f_0886A890, 0x089C8F1Cu);
            if (c->r[2] != 0) {
                *B(o + 7) = 6;
                c->r[18] = 1;
                *B(part + 123) = 1;
                gaddr p = *W(o + 92); c->r[17] = p;
                call2(c, o, 0x60, f_0886B68C, 0x089C8F68u);
                *B(o + 591) = 1;
                if (S8(p + 238) != 0) {
                    uint8_t f = *B(p + 11);
                    *B(p + 238) = 0;
                    int32_t x = S16(p + 18);
                    *H(p + 18) = (uint16_t)(f != 0 ? x + 2 : x - 2);
                }
                *W(o + 828) = 0x3C; *W(o + 832) = 0x3C;
                call2(c, o, 0x32, f_088616EC, 0x089C8FB4u);
                call2(c, p, 6, f_08861724, 0x089C8FC0u);
                *W(p + 96) = 0; *W(p + 100) = 0x60000; *W(p + 104) = 0; *W(p + 108) = 0;
            } else if (!(*B(part + 4) < 2)) {
                *B(o + 74) = 1;
                *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08937744(Cpu *c); void f_0886DB2C(Cpu *c); void f_088D39D0(Cpu *c); void f_088D2E54(Cpu *c);
void f_088D2D74(Cpu *c); void f_08862004(Cpu *c);

/* (fighter): ticked until its animation ends; then +7 on, the opponent's +698 = 1, the throw lands
   (0x4C), the opponent's +541 cleared, a jump (x -2.0, y 4.25, brake -0.28125), +73 = 1 and move 0xD. A
   frame keeping s0..s2. */
SFA_FUNC(0890BC68, hold_4c) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x0890BC90u);
    else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        gaddr p = *W(o + 92); c->r[17] = p; c->r[18] = 1;
        *B(p + 698) = 1;
        call2(c, o, 0x4C, f_0886B144, 0x0890BCBCu);
        *B(p + 541) = 0;
        *W(o + 96) = 0xFFFE0000u; *W(o + 100) = 0x44000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFB800u;
        *B(o + 73) = 1;
        call2(c, o, 0xD, f_088616EC, 0x0890BCF0u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0x08937744 and ticked; once its animation ends +196 on, the result's +35 = 0x18, +77
   cleared, the part at +0x3A0 set moving (y -8.0, brake -0.5), the hit 0x3E + strength checked quickly
   and in full, and move 0x34. */
SFA_FUNC(08937518, hold_3e_burst) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08937744, 0x0893752Cu);
    call1(c, o, f_08861848, 0x08937534u);
    if (S8(o + 75) < 0) {
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1);
        *B(HIT_RES + 35) = 0x18;
        *B(o + 77) = 0;
        gaddr q = o + 0x3A0;
        *W(q + 16) = 0; *W(q + 8) = 0; *W(q + 12) = 0xFFF80000u; *W(q + 20) = 0xFFFF8000u;
        c->r[6] = 0xFFFF8000u;
        call2(c, o, (uint32_t)(S8(o + 194) + 0x3E), f_0886B6A8, 0x0893758Cu);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x3E), f_0886B68C, 0x0893759Cu);
        call2(c, o, 0x34, f_088616EC, 0x089375A8u);
    }
    leave(c, sv);
}

/* (fighter): 0x088D3EC0; on +77 it is cleared, the throw lands (0x2A) and +262 cleared; on +76 +7 on, +73
   = 1 and a hop (x 3.0 back from the facing, y 4.0, brake -0.375), else 0x0886DB2C; 0x0886DDCC. */
SFA_FUNC(089BA648, hold_2a) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BA65Cu);
    if (S8(o + 77) != 0) {
        *B(o + 77) = 0;
        call2(c, o, 0x2A, f_0886B144, 0x089BA678u);
        *B(o + 262) = 0;
    }
    if (S8(o + 76) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 73) = 1;
        *W(o + 96) = 0x30000; *W(o + 104) = 0; *W(o + 100) = 0x40000;
        c->r[5] = 0xFFFFA000u;
        *W(o + 108) = 0xFFFFA000u;
        if (*B(o + 11) != 0) *W(o + 96) = 0u - *W(o + 96);
    } else call1(c, o, f_0886DB2C, 0x089BA6D8u);
    call1(c, o, f_0886DDCC, 0x089BA6E0u);
    leave(c, sv);
}

/* (fighter): 0x088D3EC0 and 0x08862004; on reaching the wall (+238 = +78) +7 on, the hit 0x50 + strength,
   0x088D39D0 (-0x20, 0x3E), 0x088D2E54 (0x34), 0x088D2D74 (opponent) and move 0x20; 0x0886DDCC. A frame
   keeping s0, s1 (the opponent). */
SFA_FUNC(089BC110, hold_wall_50) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92); c->r[17] = p;
    call1(c, o, f_088D3EC0, 0x089BC12Cu);
    call1(c, o, f_08862004, 0x089BC134u);
    if (S8(o + 238) == S8(o + 78)) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x50), f_0886B68C, 0x089BC160u);
        call3(c, o, 0xFFFFFFE0u, 0x3E, f_088D39D0, 0x089BC170u);
        call2(c, o, 0x34, f_088D2E54, 0x089BC17Cu);
        call1(c, p, f_088D2D74, 0x089BC184u);
        call2(c, o, 0x20, f_088616EC, 0x089BC190u);
    }
    call1(c, o, f_0886DDCC, 0x089BC198u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0886163C(Cpu *c); void f_08946074(Cpu *c); void f_0887E560(Cpu *c);

/* (fighter): +82 counted down; past 0 +7 on, turned round, the opponent's +698 / +568 = 1, the throw
   lands (0x4A), +73 = 1, a toss (x 1.5 towards the old facing, y 5.0, brake -0.3125) and move 0x30
   (0x0886163C). A frame keeping s0, s1. */
SFA_FUNC(0891623C, hold_toss_4a) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int16_t h = (int16_t)(S16(o + 82) - 1);
    gaddr p = *W(o + 92);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    *H(o + 82) = (uint16_t)h;
    if (h < 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 11) ^= 1;
        c->r[17] = 1;
        *B(p + 698) = 1; *B(p + 568) = 1;
        call2(c, o, 0x4A, f_0886B144, 0x0891629Cu);
        *B(o + 73) = 1;
        *W(o + 96) = *B(o + 11) != 0 ? 0xFFFE8000u : 0x18000u;
        *W(o + 104) = 0; *W(o + 100) = 0x50000; *W(o + 108) = 0xFFFFB000u;
        call2(c, o, 0x30, f_0886163C, 0x089162DCu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): once its animation ends step 5, +73 = 1, +640 cleared, +824 = 2, 0x08946074 and move 0x14;
   before that on +77 = 0xF the hit 0x6B, +77 cleared, the result's +35 = 8 x strength + 0x3C,
   0x0886E3CC (8, 0x10) on both sides and 0x0887E560, then ticked. */
SFA_FUNC(089463A4, hold_6b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) < 0) {
        *B(o + 7) = 5; *B(o + 73) = 1; *B(o + 640) = 0; *W(o + 824) = 2;
        call1(c, o, f_08946074, 0x089463E0u);
        call2(c, o, 0x14, f_088616EC, 0x089463ECu);
    } else {
        if (S8(o + 77) == 0xF) {
            call2(c, o, 0x6B, f_0886B68C, 0x0894640Cu);
            *B(o + 77) = 0;
            *B(HIT_RES + 35) = (uint8_t)((S8(o + 194) << 3) + 0x3C);
            call3(c, o, 8, 0x10, f_0886E3CC, 0x08946438u);
            call3(c, *W(o + 92), 8, 0x10, f_0886E3CC, 0x08946448u);
            call1(c, o, f_0887E560, 0x08946450u);
        }
        call1(c, o, f_08861848, 0x08946458u);
    }
    leave(c, sv);
}

/* (fighter): 0x088D3EC0; on +77 it is cleared and the throw lands (0x4A + strength, 0x30 + strength with
   +828); without +76 +7 on and a hop (x / y 4.0 along the facing, brake -0.375); 0x0886DDCC. */
SFA_FUNC(089BC5CC, hold_4a_30) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BC5E0u);
    if (S8(o + 77) != 0) {
        uint32_t t = *W(o + 828);
        *B(o + 77) = 0;
        if (t == 0) call2(c, o, (uint32_t)(S8(o + 194) + 0x4A), f_0886B144, 0x089BC608u);
        else call2(c, o, (uint32_t)(S8(o + 194) + 0x30), f_0886B144, 0x089BC61Cu);
    }
    if (S8(o + 76) == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 96) = 0x40000; *W(o + 104) = 0; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA000u;
        if (*B(o + 11) != 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    }
    call1(c, o, f_0886DDCC, 0x089BC674u);
    leave(c, sv);
}


void f_0887DD30(Cpu *c);

/* (fighter): on +77 +82 = 0x10, +77 cleared, the result's +35 = 0x18, +7 on and the hit by strength from
   0x08A5FFB8 plus +826 (one more past step 2), with 0x0886E3CC (9, 8) on both sides; ticked. A frame
   keeping s0, s1 (the opponent). */
SFA_FUNC(089160E4, hold_tab_ffb8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 92); c->r[17] = p;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[5] = (uint32_t)S8(o + 77);
    if (c->r[5] != 0) {
        *H(o + 82) = 0x10; *B(o + 77) = 0;
        *B(HIT_RES + 35) = 0x18;
        int16_t n = (int16_t)(S16(0x08A5FFB8u + (uint32_t)(S8(o + 194) * 2)) + S8(o + 0x334 + 6));
        uint32_t st = *B(o + 7);
        if (st != 2) n = (int16_t)(n + 1);
        *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, (uint32_t)(int32_t)n, f_0886B68C, 0x08916188u);
        call3(c, o, 9, 8, f_0886E3CC, 0x08916198u);
        call3(c, p, 9, 8, f_0886E3CC, 0x089161A8u);
    }
    call1(c, o, f_08861848, 0x089161B0u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked, 0x0886E3CC (7, 4) on +76; without +77 0x08937744 and, once the opponent's +22 is
   below its +132, 0x0886E3CC (8, 8) on it, +7 on, the result's +35 = 0x18, +196 cleared, the opponent
   set down (+20 / +16 cleared, +22 = +132, +698 = 1), 0x0887DD30 (0x43), the throw lands (0x43) and the
   part at +0x3A4 gets +5 on and +78 = 0x57. A frame keeping s0, s1. */
SFA_FUNC(089375B8, hold_43_drop) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x089375D0u);
    if (S8(o + 76) != 0) call3(c, o, 7, 4, f_0886E3CC, 0x089375E8u);
    if (S8(o + 77) == 0) {
        call1(c, o, f_08937744, 0x089375FCu);
        gaddr p = *W(o + 92); c->r[17] = p;
        if (S16(p + 22) < S16(p + 132)) {
            call3(c, p, 8, 8, f_0886E3CC, 0x08937620u);
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(HIT_RES + 35) = 0x18;
            *B(o + 196) = 0;
            int16_t y = S16(p + 132);
            *H(p + 20) = 0; *H(p + 22) = (uint16_t)y; *H(p + 16) = 0; *B(p + 698) = 1;
            call2(c, o, 0x43, f_0887DD30, 0x08937664u);
            call2(c, o, 0x43, f_0886B144, 0x08937670u);
            gaddr q = *W(o + 0x3A0 + 4); c->r[16] = q;
            if (q != 0) { *B(q + 5) = (uint8_t)(*B(q + 5) + 1); *B(q + 78) = 0x57; }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0890CC44(Cpu *c); void f_0890CC04(Cpu *c); void f_0887C408(Cpu *c);

/* (fighter): 0x0890CC44; while still above +132 0x0890CC04 and ticked; landed: +7 on, the hit by strength
   from 0x08A5835C, +73 cleared, +22 = +132, +822 = 0x10, the result's +35 by strength from 0x08A58358,
   0x0886E3CC (by strength from 0x08A58184, 4) on both sides and move 8. A frame keeping s0, s1. */
SFA_FUNC(0890A55C, hold_land_835c) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_0890CC44, 0x0890A574u);
    if (S16(o + 132) < S16(o + 22)) {
        call1(c, o, f_0890CC04, 0x0890A590u);
        call1(c, o, f_08861848, 0x0890A598u);
    } else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, *H(0x08A5835Cu + (uint32_t)(S8(o + 194) * 2)), f_0886B68C, 0x0890A5C8u);
        *B(o + 73) = 0; *H(o + 22) = *H(o + 132);
        *B(o + 0x334 + 2) = 0x10;
        *B(HIT_RES + 35) = *B(0x08A58358u + (uint32_t)S8(o + 194));
        const gaddr t = 0x08A58184u; c->r[17] = t;
        call3(c, o, *B(t + (uint32_t)S8(o + 194)), 4, f_0886E3CC, 0x0890A620u);
        call3(c, *W(o + 92), *B(t + (uint32_t)S8(o + 194) + 1), 4, f_0886E3CC, 0x0890A640u);
        call2(c, o, 8, f_088616EC, 0x0890A64Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): without +77 ticked; on +77 = 0xF it is cleared, the result's +35 = 8 x strength + 0x3C,
   0x0886E3CC (8, 0x10) on both sides, 0x0887E560, ticked, 0x0887C408 (+337, the value at 0x08A749F6) and
   the hit 0x68 + strength; on any other +77 step 8, +73 = 1, +77 cleared, the opponent's +568 / +698 = 1,
   the throw lands (0x62) and a leap (y 6.0, brake -0.5, x 3.5 along the facing). */
SFA_FUNC(08946198, hold_68_62) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    int32_t k = S8(o + 77);
    c->r[5] = 0xF;
    if (k == 0) call1(c, o, f_08861848, 0x089461BCu);
    else if (k == 0xF) {
        *B(o + 77) = 0;
        *B(HIT_RES + 35) = (uint8_t)((S8(o + 194) << 3) + 0x3C);
        call3(c, o, 8, 0x10, f_0886E3CC, 0x089461F8u);
        call3(c, *W(o + 92), 8, 0x10, f_0886E3CC, 0x08946208u);
        call1(c, o, f_0887E560, 0x08946210u);
        call1(c, o, f_08861848, 0x08946218u);
        call3(c, o, (uint32_t)S8(o + 337), *H(0x08A749E4u + 18), f_0887C408, 0x08946230u);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x68), f_0886B68C, 0x08946240u);
    } else {
        *B(o + 7) = 8; *B(o + 73) = 1;
        gaddr p = *W(o + 92);
        *B(o + 77) = 0; *B(p + 568) = 1; *B(p + 698) = 1;
        call2(c, o, 0x62, f_0886B144, 0x08946270u);
        *W(o + 100) = 0x60000; *W(o + 104) = 0; *W(o + 108) = 0xFFFF8000u;
        *W(o + 96) = *B(o + 11) != 0 ? 0x38000u : 0xFFFC8000u;
    }
    leave(c, sv);
}

/* (fighter): on +77 bit 7 +7 on, turned back (+96 negated), set down (+22 = +132, +20 cleared) and the
   throw lands by strength from 0x08A90200; otherwise drifting while +77 is clear, and at +77 = 1 it goes
   to 2, the result's +35 = 0x1E, 0x0886E3CC (0xA, 0xA) on both sides and 0x0887DD30 (0x36); ticked and
   0x0886DDCC. A frame keeping s0, s1 (the opponent). */
SFA_FUNC(08976B74, hold_drift_36) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int32_t k = S8(o + 77);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92); c->r[17] = p;
    if (k & 0x80) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 96) = 0u - *W(o + 96);
        *H(o + 22) = *H(o + 132); *H(o + 20) = 0;
        call2(c, o, (uint32_t)S16(0x08A90200u + (uint32_t)(S8(o + 194) * 2)), f_0886B144, 0x08976BDCu);
    } else {
        if (k == 0) *W(o + 16) += *W(o + 96);
        if (k == 1) {
            *B(o + 77) = 2;
            *B(HIT_RES + 35) = 0x1E;
            call3(c, o, 0xA, 0xA, f_0886E3CC, 0x08976C2Cu);
            call3(c, p, 0xA, 0xA, f_0886E3CC, 0x08976C3Cu);
            call2(c, o, 0x36, f_0887DD30, 0x08976C48u);
        }
        call1(c, o, f_08861848, 0x08976C50u);
        call1(c, o, f_0886DDCC, 0x08976C58u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter): 0x088D3EC0 and 0x08862004; on reaching the wall (+238 = +78) 0x088D39D0 (-0x20, 0x3E) facing
   the other way, the hit 0x50 + strength, 0x088D2E54 (0x34) and 0x088D2D74 (opponent); then by strength
   +7 on and move 0x21 (light) / 0x24 (medium), heavy step 4 and move 0x25 while +820 counts down, else
   +7 on and move 0x24; 0x0886DDCC. A frame keeping s0, s1. */
SFA_FUNC(089BC280, hold_wall_50b) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92); c->r[17] = p;
    call1(c, o, f_088D3EC0, 0x089BC29Cu);
    call1(c, o, f_08862004, 0x089BC2A4u);
    if (S8(o + 238) == S8(o + 78)) {
        *B(o + 11) ^= 1;
        call3(c, o, 0xFFFFFFE0u, 0x3E, f_088D39D0, 0x089BC2D0u);
        *B(o + 11) ^= 1;
        call2(c, o, (uint32_t)(S8(o + 194) + 0x50), f_0886B68C, 0x089BC2ECu);
        call2(c, o, 0x34, f_088D2E54, 0x089BC2F8u);
        call1(c, p, f_088D2D74, 0x089BC300u);
        int32_t s = S8(o + 194); c->r[17] = (uint32_t)s;
        if (s == 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x21, f_088616EC, 0x089BC32Cu);
        } else if (s == 1) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x24, f_088616EC, 0x089BC364u);
        } else if (s == 2) {
            uint32_t t = *W(o + 820);
            if (t != 0) {
                *B(o + 7) = 4; *W(o + 820) = t - 1;
                call2(c, o, 0x25, f_088616EC, 0x089BC38Cu);
            } else {
                *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
                call2(c, o, 0x24, f_088616EC, 0x089BC3ACu);
            }
        }
    }
    call1(c, o, f_0886DDCC, 0x089BC3B4u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked. Once its animation ends: the first pass of a medium / heavy one sets +824 = 1 and
   move 0x13; otherwise step 5, +73 = 1, +640 cleared, move 0x14, +824 = 1 for heavy else cleared, and
   0x08946074. Before that +76 (cleared) runs 0x0887C408 (+337, 0x08A749F4 for light at 1, 0x08A749F2
   for medium otherwise); +77 = 1 gives the hit by strength and +824 from 0x08A76348, +77 cleared, the
   result's +35 = 0x3C, 0x0887C408 for medium / heavy, and 0x0886E3CC (8, 0x10) on both sides with
   0x0887E560; 2 steps it 0x58 along its facing; 3 for light is move 0x13. A frame keeping s0..s2. */
SFA_FUNC(08945E3C, hold_multi_6348) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x08945E58u);
    int32_t s = S8(o + 194); c->r[17] = (uint32_t)s;
    c->r[18] = 1;
    const gaddr tab = 0x08A749E4u;
    if (S8(o + 75) < 0) {
        if (s != 0 && *W(o + 824) == 0) {
            *W(o + 824) = 1;
            call2(c, o, 0x13, f_088616EC, 0x08945EB4u);
        } else {
            *B(o + 7) = 5; *B(o + 73) = 1; *B(o + 640) = 0;
            call2(c, o, 0x14, f_088616EC, 0x08945E94u);
            *W(o + 824) = 0;
            if (s == 2) *W(o + 824) = 1;
            call1(c, o, f_08946074, 0x08945ED8u);
        }
    } else {
        int32_t k = S8(o + 76);
        if (k == 1) {
            *B(o + 76) = 0;
            if (s == 0) call3(c, o, (uint32_t)S8(o + 337), *H(tab + 16), f_0887C408, 0x08945F14u);
        } else if (k != 0) {
            *B(o + 76) = 0;
            if (s == 1) call3(c, o, (uint32_t)S8(o + 337), *H(tab + 14), f_0887C408, 0x08945F3Cu);
        }
        int32_t m = S8(o + 77);
        if (m == 1) {
            uint32_t i = (uint32_t)(s + s) + *W(o + 824);
            call2(c, o, (uint32_t)S16(0x08A76348u + i + i), f_0886B68C, 0x08945FCCu);
            *B(o + 77) = 0;
            *B(HIT_RES + 35) = 0x3C;
            if (s != 0) {
                if (*W(o + 824) != 0) call3(c, o, (uint32_t)S8(o + 337), *H(tab + 16), f_0887C408, 0x08946034u);
                else if (s == 2) call3(c, o, (uint32_t)S8(o + 337), *H(tab + 14), f_0887C408, 0x08946014u);
            }
            call3(c, o, 8, 0x10, f_0886E3CC, 0x08946044u);
            call3(c, *W(o + 92), 8, 0x10, f_0886E3CC, 0x08946054u);
            call1(c, o, f_0887E560, 0x0894605Cu);
        } else if (m == 2) {
            *B(o + 77) = 0;
            int32_t x = S16(o + 18);
            *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + 0x58 : x - 0x58);
        } else if (m == 3) {
            *B(o + 77) = 0;
            if (s == 0) call2(c, o, 0x13, f_088616EC, 0x08945F9Cu);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
