/* Steps of 56 - 59 instructions: a landing that stuns a dizzy opponent's partner (0x0891FA8C), a glide
   with a frame-flag push, a creep by frame flags, the roll-out after a knock-down (0x0885E2F0), the
   jump landing that guards when asked (0x0885B3C4), projectile spawns with an extra one at full meter
   and a lunge that stops at its mark. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c);
void f_0887C614(Cpu *c); void f_0887C408(Cpu *c); void f_0886469C(Cpu *c); void f_0886DB2C(Cpu *c);
void f_08881F94(Cpu *c); void f_08863208(Cpu *c); void f_08862004(Cpu *c); void f_08862484(Cpu *c);
void f_088628EC(Cpu *c); void f_08822D2C(Cpu *c); void f_088F38E4(Cpu *c); void f_088F84BC(Cpu *c);

#define GAME 0x08E75910u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (fighter): +20 by +100; within 16 of the floor +6 steps on, +78 = 2, cue 0x22 / 0x23, at the floor
   (+20 / +73 cleared); with +430 against an opponent in state 9 +6 = 5 and move 0x19, else (cue 0x211
   with +430) move 0x31; above that ticked. */
SFA_FUNC(0891FA8C, move_near_floor_31) {
    gaddr o = ARG(0); uint32_t sv[2];
    *W(o + 20) = *W(o + 20) + *W(o + 100);
    int32_t floor = S16(o + 132);
    int32_t y = (int32_t)(((uint32_t)S16(o + 22) - 0x10) & 0xFFFF);
    enter(c, o, sv);
    if (y < (floor & 0xFFFF)) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *B(o + 78) = 2;
        call1(c, o, f_0887C614, 0x0891FAE4u);
        int32_t fl = S16(o + 132);
        *H(o + 20) = 0; *H(o + 22) = (uint16_t)fl;
        int32_t k = S8(o + 430);
        *B(o + 73) = 0;
        if (k != 0) {
            if (S8(*W(o + 92) + 338) == 9) {
                *B(o + 6) = 5;
                call2(c, o, 0x19, f_088616EC, 0x0891FB44u);
                leave(c, sv);
                return;
            }
            ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x211; c->r[31] = 0x0891FB20u; f_0887C408(c);
        }
        call2(c, o, 0x31, f_0886163C, 0x0891FB2Cu);
    } else call1(c, o, f_08861848, 0x0891FB5Cu);
    leave(c, sv);
}

/* (fighter): +543 = 2, ticked; a negative frame flag starts a hop (step 4, by strength from 0x08A8A9F4 /
   0x08A8AA00, brake -0.5, fall -0.375); else frame flag 2 (cleared) pushes it (x 8.0 / -0.5) and it
   slides by +96 toward its facing until that turns. */
SFA_FUNC(08965550, move_glide_push) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = o; c->r[17] = 2;
    *W(c->r[29] + 24) = ra;
    *B(o + 543) = 2;
    call1(c, o, f_08861848, 0x08965570u);
    int32_t k = S8(o + 77);
    if (k < 0) {
        *B(o + 7) = 4;
        uint32_t i = (uint32_t)S8(o + 194) << 2;
        *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
        *W(o + 96) = *W(0x08A8A9F4u + i);
        *W(o + 100) = *W(0x08A8AA00u + i);
    } else {
        uint32_t x = *W(o + 16);
        if (k == 2) { *B(o + 77) = 0; *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF8000u; }
        uint32_t v = *W(o + 96);
        *W(o + 16) = *B(o + 11) != 0 ? x + v : x - v;
        uint32_t nv = v + *W(o + 104);
        *W(o + 96) = nv;
        if ((int32_t)nv < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ended: facing the opponent and standing; else ticked, frame flag +76 steps x toward its
   facing (cleared), a negative +77 keeps its low bits and clears +262, and a set +77 moves it by +96
   (with 0x08881F94 unless the game's +59 bits 0 - 1); then 0x0886DB2C. */
SFA_FUNC(0897CEA0, move_creep_by_flags) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    int32_t e = S8(o + 75);
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = GAME;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (e & 0x80) {
        *B(o + 11) = (uint8_t)S8(o + 697);
        call1(c, o, f_0886469C, 0x0897CED8u);
    } else {
        call1(c, o, f_08861848, 0x0897CEE8u);
        int32_t d = S8(o + 76), k = S8(o + 77);
        if (d != 0) {
            int32_t x = S16(o + 18);
            *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + d : x - d);
            *B(o + 76) = 0;
        }
        if (k & 0x80) { *B(o + 77) = (uint8_t)(k & 0x7F); *B(o + 262) = 0; k = S8(o + 77); }
        if (k != 0) {
            *W(o + 16) = *W(o + 16) + *W(o + 96);
            if ((S8(GAME + 59) & 3) == 0) call1(c, o, f_08881F94, 0x0897CF64u);
        }
        call1(c, o, f_0886DB2C, 0x0897CF6Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): rolling (+16 by +96); ended, the +82 wait runs down; out, knocked out without life, else
   with +702 +239 cleared, +116 = 4, step 5 and move 0x19 (list 2), without it the get-up (state 1 / 1 /
   2 / 0, hit marks cleared, facing the opponent, crouched); ticked otherwise. */
SFA_FUNC(0885E2F0, fighter_roll_getup) {
    gaddr o = ARG(0); uint32_t ra;
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    int32_t e = S8(o + 75);
    frame_in(c, &ra);
    int done = 0;
    if (e & 0x80) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) {
            done = 1;
            if (S16(o + 112) < 0) call1(c, o, f_08863208, 0x0885E348u);
            else if (S8(o + 702) != 0) {
                *B(o + 239) = 0; *B(o + 116) = 4; *B(o + 7) = 5;
                call2(c, o, 0x19, f_08861724, 0x0885E3B8u);
            } else {
                *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 2; *B(o + 7) = 0;
                *B(o + 539) = 0; *B(o + 239) = 0;
                int32_t face = S8(o + 697);
                *B(o + 541) = 0; *B(o + 698) = 0;
                *B(o + 11) = (uint8_t)face; *B(o + 696) = 1;
            }
        }
    }
    if (!done) call1(c, o, f_08861848, 0x0885E3CCu);
    frame_out(c, ra);
}

/* (fighter): when 0x08862004 lands it: +73 / +235 / +773 cleared, at the floor, cue 0x22 / 0x23, then
   unless locked (+373) 0x08862484 may put it in the guard stance (0x088628EC), else step 2 and move
   0x11. In the air the +82 wait runs down; out, the fall stance (1 / 0 / 3 / 1) with +773 counted
   (+592 cleared) when +238 is set; else ticked. */
SFA_FUNC(0885B3C4, fighter_jump_land) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x0885B3D8u);
    if (c->r[2] != 0) {
        int32_t floor = S16(o + 132);
        *B(o + 73) = 0;
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 235) = 0; *B(o + 773) = 0;
        call1(c, o, f_0887C614, 0x0885B400u);
        int guard = 0;
        if (S8(o + 373) == 0) { call1(c, o, f_08862484, 0x0885B414u); guard = c->r[2] != 0; }
        if (guard) call1(c, o, f_088628EC, 0x0885B440u);
        else { *B(o + 7) = 2; call2(c, o, 0x11, f_0886163C, 0x0885B430u); }
    } else {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t != 0) call1(c, o, f_08861848, 0x0885B4A0u);
        else {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3;
            int32_t b = S8(o + 238);
            *B(o + 7) = 1;
            if (b != 0) { int32_t n = S8(o + 773); *B(o + 592) = 0; *B(o + 773) = (uint8_t)(n + 1); }
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag sets step 2 and +82 = `wait` and spawns projectile `kind` (set up by
   `setup`; +260 = 0xB, +311 = 2, +312 = 8); with +251 and the meter (+286) at +662 or more a second one,
   its +280 = +662. */
static void flag_projectiles(Cpu *c, uint32_t wait, uint32_t kind, void (*setup)(Cpu *), const uint32_t b[5]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[5];
    for (int i = 0; i < 5; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    for (int i = 0; i < 5; i++) *W(c->r[29] + 16 + 4 * i) = s[i];
    *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, b[0]);
    if (S8(o + 77) != 0) {
        *B(o + 7) = 2; *H(o + 82) = (uint16_t)wait;
        ARG(0) = o; ARG(1) = kind; ARG(2) = 0; c->r[31] = b[1]; f_08822D2C(c);
        gaddr e = c->r[2];
        c->r[20] = e;
        if (e != 0) {
            ARG(0) = o; ARG(1) = e; c->r[31] = b[2]; setup(c);
            c->r[19] = 0xB; c->r[18] = 2; c->r[17] = 8;
            *B(e + 260) = 0xB; *B(e + 311) = 2; *B(e + 312) = 8;
            if (S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662))) {
                ARG(0) = o; ARG(1) = kind; ARG(2) = 0; c->r[31] = b[3]; f_08822D2C(c);
                gaddr e2 = c->r[2];
                c->r[20] = e2;
                if (e2 != 0) {
                    ARG(0) = o; ARG(1) = e2; c->r[31] = b[4]; setup(c);
                    uint32_t m = (uint32_t)S16(o + 662);
                    *B(e2 + 260) = 0xB; *B(e2 + 280) = (uint8_t)m; *B(e2 + 311) = 2; *B(e2 + 312) = 8;
                }
            }
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
SFA_FUNC(088F37F8, move_flag_projectiles_1) {
    static const uint32_t b[5] = { 0x088F381Cu, 0x088F3844u, 0x088F3858u, 0x088F389Cu, 0x088F38B0u };
    flag_projectiles(c, 3, 1, f_088F38E4, b);
}
SFA_FUNC(088F83D0, move_flag_projectiles_2) {
    static const uint32_t b[5] = { 0x088F83F4u, 0x088F841Cu, 0x088F8430u, 0x088F8474u, 0x088F8488u };
    flag_projectiles(c, 4, 2, f_088F84BC, b);
}

/* (fighter): moves by +96; it steps on into move 0x3C (0x3D at strength 2) once past its mark (+0x334),
   when +238 asks for its way of moving (1 back, 2 forward), or within 80 of the opponent's x (32
   ahead); else ticked. */
SFA_FUNC(08916900, move_lunge_to_mark_3c) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t x = *W(o + 16);
    gaddr opp = *W(o + 92);
    uint32_t v = *W(o + 96);
    *W(o + 16) = x + v;
    uint32_t f = *B(o + 11);
    int32_t px = S16(o + 18), m = S16(o + 0x334);
    frame_in(c, &ra);
    int16_t d = (int16_t)(f != 0 ? m - px : px - m);
    int go = 1;
    if (d >= 0 && S8(o + 238) != ((int32_t)v < 0 ? 1 : 2)) {
        uint32_t off = f == 0 ? 0xFFE0u : 0x20u;
        int32_t r = (int32_t)((off + (uint32_t)px - (uint32_t)S16(opp + 18) + 0x50) & 0xFFFF);
        if (0xA0 < r) go = 0;
    }
    if (go) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, s == 2 ? 0x3D : 0x3C, f_088616EC, 0x089169E0u);
    } else call1(c, o, f_08861848, 0x089169C8u);
    frame_out(c, ra);
}
