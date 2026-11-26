/* Steps of 54 - 56 instructions: a part effect's life check and homing step (0x089CC17C, 0x089CC2BC),
   a frame-flag hop, a guard pick with speeds by strength, a part dash by strength, the guard stance
   start that crouches on a held down (0x088628EC), the round-end pose that picks a win / lose / draw
   move by life (0x08864284, 206,801 calls) and a landing placed beside another object. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EB8(Cpu *c);
void f_088D3DD8(Cpu *c); void f_08821424(Cpu *c); void f_088D3D94(Cpu *c); void f_08911550(Cpu *c);
void f_0886E294(Cpu *c); void f_088D3EC0(Cpu *c); void f_0886469C(Cpu *c); void f_088D31AC(Cpu *c);
void f_0886DB2C(Cpu *c); void f_08912FEC(Cpu *c); void f_08821330(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
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

/* (effect): while the match runs (+37 / +78 clear) and 0x088D3EB8 lets it live, a hit (+123 = 1) or a
   negative +123 or the end of its picture moves it to the next state (+141 = 1 unless hit) before
   0x088D3DD8; then drawn (0x08821424). */
SFA_FUNC(089CC17C, part_effect_life) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(MATCH + 37) == 0 && S8(MATCH + 78) == 0) {
        call1(c, o, f_088D3EB8, 0x089CC1B4u);
        if (c->r[2] != 0) { leave(c, sv); return; }
        int32_t f = S8(o + 123);
        if (f == 1) {
            uint32_t s = *B(o + 4);
            *B(o + 5) = 0; *B(o + 4) = (uint8_t)(s + 1); *B(o + 6) = 0; *B(o + 7) = 0;
        } else if (f < 0 || S8(o + 75) < 0) {
            uint32_t s = *B(o + 4);
            *B(o + 141) = 1; *B(o + 4) = (uint8_t)(s + 1); *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        }
        call1(c, o, f_088D3DD8, 0x089CC23Cu);
    }
    call1(c, o, f_08821424, 0x089CC244u);
    leave(c, sv);
}

/* (effect): +5 counts; unless +141 (or with a hit) it plays 0x08AB2FDC from 0xF with +5 counted again;
   else 0x088D3D94 (0x08AB2FDC, 0x11) and it homes on its owner's opponent (speeds 1/16 of the distance,
   +140 = 16). */
SFA_FUNC(089CC2BC, part_effect_home) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr own = *W(o + 88);
    uint32_t n = *B(o + 5);
    *W(c->r[29] + 20) = s1;
    gaddr tgt = *W(own + 92);
    c->r[17] = tgt;
    int32_t k = S8(o + 141);
    *B(o + 5) = (uint8_t)(n + 1);
    *W(c->r[29] + 24) = ra;
    gaddr tab = 0x08AB2FDCu;
    if (k == 0 || S8(o + 123) == 1) {
        ARG(0) = o; ARG(1) = tab; ARG(2) = 0x11; c->r[31] = 0x089CC314u; f_088D3D94(c);
        int32_t dx = (int32_t)(*W(tgt + 16) - *W(o + 16));
        *W(o + 96) = (uint32_t)((dx + (int32_t)(((uint32_t)(dx >> 4)) >> 28)) >> 4);
        int32_t dy = (int32_t)(*W(tgt + 20) - *W(o + 20));
        *W(o + 100) = (uint32_t)((dy + (int32_t)(((uint32_t)(dy >> 4)) >> 28)) >> 4);
        *W(o + 104) = 0; *W(o + 108) = 0; *B(o + 140) = 0x10;
    } else {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        ARG(0) = o; ARG(1) = tab; ARG(2) = 0xF; c->r[31] = 0x089CC380u; f_088D3D94(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; ended: step 5 into move 0x1C + 2 x strength; frame flags +76 = 3 / +77 = -1 a hop
   back (+7 on, x -3.5, y 9.0 / -1.0, +73 = 1); otherwise 0x08911550, a frame flag (cleared) first
   giving x 8.0 / -0.5. */
SFA_FUNC(0891133C, move_tick_hop_or_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08911350u);
    if (S8(o + 75) & 0x80) {
        int32_t s = S8(o + 194);
        *B(o + 7) = 5;
        call2(c, o, (uint32_t)((s << 1) + 0x1C) & 0xFFFF, f_088616EC, 0x08911380u);
    } else {
        int32_t k76 = S8(o + 76), k77 = S8(o + 77);
        if (k76 == 3 && k77 == -1) {
            bump(o + 7);
            *W(o + 96) = 0xFFFC8000u; *W(o + 104) = 0; *W(o + 100) = 0x90000; *W(o + 108) = 0xFFFF0000u;
            *B(o + 73) = 1;
        } else if (k77 != 0) {
            *B(o + 77) = 0;
            *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF8000u;
            call1(c, o, f_08911550, 0x08911408u);
        } else call1(c, o, f_08911550, 0x089113E8u);
    }
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, +237 cleared and the guard pick (0x0886E294): without +81
   the four speeds from 0x08A8B874 by the strength's entry (0x08A8B870), and +573 from 0x08A8B86C;
   ticked. */
SFA_FUNC(08968240, move_guard_pick_speeds) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x08968270u);
        uint32_t i = c->r[2] & 0xFFFF;
        if (S8(o + 81) == 0) {
            i = (uint32_t)S8(o + 194) & 0xFFFF;
            gaddr ent = 0x08A8B870u + i;
            gaddr base = 0x08A8B874u;
            *W(o + 96) = *W(base + ((uint32_t)*B(ent) << 2));
            *W(o + 104) = *W(base + ((uint32_t)*B(ent) << 2) + 4);
            *W(o + 100) = *W(base + ((uint32_t)*B(ent) << 2) + 8);
            *W(o + 108) = *W(base + ((uint32_t)*B(ent) << 2) + 12);
        }
        *B(o + 573) = *B(0x08A8B86Cu + i);
    }
    call1(c, o, f_08861848, 0x0896830Cu);
    leave(c, sv);
}

/* (character): the part ticked; ended: standing. Else frame flag +76 sets the dash speed (2: by strength
   16.0 / 43.0 / 36.0; else from 0x08AB1590 by the flag) toward its facing, +76 cleared, 0x088D31AC, a
   positive frame flag clears itself and +262, then 0x0886DB2C. */
SFA_FUNC(089C1364, part_dash_by_flag) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C1378u);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089C138Cu);
    else {
        int32_t k = S8(o + 76);
        if (k == 2) {
            int32_t s = S8(o + 194);
            if (s == 0) *W(o + 96) = 0x100000;
            else if (s == 1) *W(o + 96) = 0x2B0000;
            else if (s == 2) *W(o + 96) = 0x240000;
        } else *W(o + 96) = *W(0x08AB1590u + ((uint32_t)k << 2));
        uint32_t f = *B(o + 11);
        *B(o + 76) = 0;
        if (f == 0) *W(o + 96) = 0u - *W(o + 96);
        call1(c, o, f_088D31AC, 0x089C1414u);
        if (S8(o + 77) > 0) { *B(o + 77) = 0; *B(o + 262) = 0; }
        call1(c, o, f_0886DB2C, 0x089C1430u);
    }
    leave(c, sv);
}

/* (fighter): the guard stance (state 1 / 0 / 6): +266 cleared, facing the opponent (+697), crouched
   (+696, step 2, move 0x18) with down held (+752 bit 2; in mode 2 by the pad flags at 0x0950FF71 for
   its side), else step 0 and move 0x15. */
SFA_FUNC(088628EC, move_guard_stance_start) {
    gaddr o = ARG(0); uint32_t ra;
    int32_t face = S8(o + 697);
    *B(o + 266) = 0;
    *B(o + 11) = (uint8_t)face;
    int32_t mode = S8(GAME + 114);
    frame_in(c, &ra);
    int crouch;
    if (mode == 2) {
        uint32_t bits = (*B(0x0950FF71u) & (7u << ((S8(o + 337) << 2) & 31))) & 0xFF;
        if (bits & 1) crouch = 0;
        else if (bits & 2) crouch = 1;
        else crouch = (*H(o + 752) & 4) != 0;
    } else crouch = (*H(o + 752) & 4) != 0;
    if (crouch) {
        *B(o + 696) = 1; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 6; *B(o + 7) = 2;
        call2(c, o, 0x18, f_0886163C, 0x08862950u);
    } else {
        *B(o + 696) = 0; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 6; *B(o + 7) = 0;
        call2(c, o, 0x15, f_0886163C, 0x088629C0u);
    }
    frame_out(c, ra);
}

/* (fighter): once the game's +38 says the round is over, the end pose (state 1 / 0 / 0 / 3, facing the
   opponent; in state 2 turned): a draw on equal life (move 0x29) or losing (0x28) keeps that facing;
   winning picks 0x27 (states 0xD / 0x1D), 0x25 (0xF / 0x16) or 0x23, facing the opponent. Gives 1, or
   0 while the round runs. */
SFA_FUNC(08864284, fighter_round_end_pose) {
    gaddr o = ARG(0); uint32_t ra;
    gaddr opp = *W(o + 92);
    frame_in(c, &ra);
    if (S8(GAME + 38) == 0) { c->r[2] = 0; frame_out(c, ra); return; }
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0;
    int32_t face = S8(o + 697);
    *B(o + 7) = 3;
    int32_t st = S8(o + 338);
    *B(o + 11) = (uint8_t)face;
    int32_t life = S16(o + 112);
    if (st == 2) *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
    int32_t ol = S16(opp + 112);
    uint32_t n;
    if (life == ol) n = 0x29;
    else if (life < ol) n = 0x28;
    else {
        if (st == 0xD || st == 0x1D) n = 0x27;
        else if (st == 0xF || st == 0x16) n = 0x25;
        else n = 0x23;
        *B(o + 11) = (uint8_t)face;
    }
    call2(c, o, n, f_0886163C, 0x08864354u);
    c->r[2] = 1;
    frame_out(c, ra);
}

/* (fighter): 0x08912FEC; falling below the floor +7 steps on, at the floor (x's low half cleared),
   +73 / +262 cleared and placed beside 0x08821330's object (96 / 288 by its side); ticked until a
   frame flag. */
SFA_FUNC(0890E2E0, move_fall_land_beside) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08912FEC, 0x0890E2F4u);
    if (*W(o + 100) & 0x80000000u) {
        int32_t floor = S16(o + 132);
        if (S16(o + 22) < floor) {
            uint32_t st = *B(o + 7);
            uint32_t x = *W(o + 16);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 20) = (uint32_t)floor << 16;
            *W(o + 16) = x & 0xFFFF0000u;
            *B(o + 73) = 0; *B(o + 262) = 0;
            call1(c, o, f_08821330, 0x0890E350u);
            uint32_t d = S8(o + 337) != 0 ? 0x120u : 0x60u;
            *H(o + 18) = (uint16_t)(S16(c->r[2] + 18) + d);
            if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0890E380u);
        } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0890E398u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0890E3B0u);
    leave(c, sv);
}
