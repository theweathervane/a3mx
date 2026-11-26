/* Steps of 51 - 65 instructions: a throw slam that slows the match (0x08976A50), a throw hold that turns
   both fighters when its wait runs out (0x088EDD74), the stage-edge throw start (0x088F2210), a hop that
   picks its target spot (0x08987200), guard dashes with a follow-up pick, a part projectile's start
   (0x089C58A4) and a carry that rides another object (0x08906ED4). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886DDCC(Cpu *c);
void f_0886E3CC(Cpu *c); void f_0887DD30(Cpu *c); void f_08978890(Cpu *c); void f_08861BCC(Cpu *c);
void f_08821330(Cpu *c); void f_08987300(Cpu *c); void f_0886E294(Cpu *c); void f_089B2188(Cpu *c);
void f_088D3D94(Cpu *c); void f_088D3E9C(Cpu *c); void f_089C59A4(Cpu *c); void f_08862004(Cpu *c);
void f_089070C4(Cpu *c); void f_089B298C(Cpu *c); void f_089B2A18(Cpu *c);

#define MATCH 0x092E1228u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, gaddr o, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = a; ARG(2) = b; c->r[31] = back; fn(c);
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
/* s0 / s1 / ra frame, s1 = `s1v`. */
static void enter2(Cpu *c, gaddr o, uint32_t s1v, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = sv[1]; c->r[17] = s1v;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (fighter): until its move ends 0x08978890; then the slam: +7 on, at the floor, +73 cleared, 48
   toward its facing, hit cues (10, 10) on both, 0x0887DD30 (0x36), the match slowed (+35 = 30), move
   0x38 and the opponent placed. */
SFA_FUNC(08976A50, throw_slam_38) {
    gaddr o = ARG(0); uint32_t sv[3];
    gaddr opp = *W(o + 92);
    int32_t e = S8(o + 75);
    enter2(c, o, opp, sv);
    if (!(e & 0x80)) call1(c, o, f_08978890, 0x08976A7Cu);
    else {
        uint32_t st = *B(o + 7);
        int32_t floor = S16(o + 132);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0;
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + 0x30 : x - 0x30);
        call3(c, o, 0xA, 0xA, f_0886E3CC, 0x08976AC8u);
        call3(c, opp, 0xA, 0xA, f_0886E3CC, 0x08976AD8u);
        call2(c, o, 0x36, f_0887DD30, 0x08976AE4u);
        *B(MATCH + 35) = 0x1E;
        call2(c, o, 0x38, f_088616EC, 0x08976B00u);
        call1(c, o, f_0886DDCC, 0x08976B08u);
    }
    leave2(c, sv);
}

/* (fighter): the opponent kept at our x and facing; ended, the +82 wait runs down and out +6 steps on,
   we turn around, both hold (+135 = 1), move 0 and the opponent placed; otherwise a frame flag (cleared)
   gives hit cues (8, 8) on both, then ticked and 0x08861BCC. */
SFA_FUNC(088EDD74, throw_hold_turn) {
    gaddr o = ARG(0); uint32_t sv[3];
    gaddr opp = *W(o + 92);
    c->r[29] -= 0x20;
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    *W(c->r[29] + 20) = sv[1]; c->r[17] = opp;
    *H(opp + 18) = *H(o + 18);
    uint32_t f = *B(o + 11);
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(opp + 11) = (uint8_t)f;
    int32_t e = S8(o + 75);
    *W(c->r[29] + 24) = sv[2];
    if (e < 0) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) {
            uint32_t n = *B(o + 6), g = *B(o + 11);
            *B(o + 6) = (uint8_t)(n + 1);
            *B(o + 11) = (uint8_t)(g ^ 1);
            *B(o + 135) = 1; *B(opp + 135) = 1;
            call2(c, o, 0, f_088616EC, 0x088EDDF4u);
            call1(c, o, f_0886DDCC, 0x088EDDFCu);
            leave2(c, sv);
            return;
        }
    }
    if (S8(o + 77) != 0) {
        *B(o + 77) = 0;
        call3(c, o, 8, 8, f_0886E3CC, 0x088EDE20u);
        call3(c, opp, 8, 8, f_0886E3CC, 0x088EDE30u);
    }
    call1(c, o, f_08861848, 0x088EDE38u);
    call1(c, o, f_08861BCC, 0x088EDE40u);
    leave2(c, sv);
}

/* (fighter): +6 = 0: +1 = 1, and against character 0 the edge start (+6 = 2, x kept in +96, guard -1,
   placed 288 / 96 from the screen's +18 by its side and faced accordingly, move 0x30), else +6 = 1 and
   move 0x22. +6 = 1: standing once ended, else ticked. Later: without a guard +6 = 1 and move 0x22. */
SFA_FUNC(088F2210, throw_edge_start) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t n = *B(o + 6);
    gaddr opp = *W(o + 92);
    frame_in(c, &ra);
    if (n == 0) {
        *B(o + 6) = 1; *B(o + 1) = 1;
        uint32_t mv = 0x22;
        if (S8(opp + 338) == 0) {
            *B(o + 6) = 2;
            *W(o + 96) = *W(o + 16);
            int32_t side = S8(o + 337);
            *B(o + 78) = 0xFF;
            gaddr scr = 0x092E88A0u;
            if (side == 0) { *B(o + 11) = 0; *H(o + 18) = (uint16_t)(S16(scr + 18) + 0x120); }
            else { *B(o + 11) = 1; *H(o + 18) = (uint16_t)(S16(scr + 18) + 0x60); }
            mv = 0x30;
        }
        call2(c, o, mv, f_0886163C, 0x088F2294u);
    } else if (n == 1) {
        if (S8(o + 75) < 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            call2(c, o, 0, f_0886163C, 0x088F22D0u);
        } else call1(c, o, f_08861848, 0x088F22E4u);
    } else if (S8(o + 78) == 0) { *B(o + 6) = 1; call2(c, o, 0x22, f_0886163C, 0x088F2304u); }
    frame_out(c, ra);
}

/* (fighter): without a frame flag a hop (+7 on, +73 = 1, y 4.9 / -0.1875) toward a spot by
   0x08987300: unlocked with +362 bit 1 the object's x, bit 0 384 past it; else 384 past it when more
   than 192 beyond it; facing by +96's sign; ticked. */
SFA_FUNC(08987200, move_hop_to_spot) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        bump(o + 7);
        *B(o + 73) = 1;
        *W(o + 104) = 0; *W(o + 100) = 0x4E666; *W(o + 108) = 0xFFFFD000u;
        call1(c, o, f_08821330, 0x0898724Cu);
        gaddr e = c->r[2];
        uint32_t ex = *W(e + 16);
        uint32_t h = *H(o + 362);
        if (S8(o + 373) == 0 && (h & 3) != 0) {
            if (h & 2) call2(c, o, ex, f_08987300, 0x08987280u);
            else call2(c, o, ex + 0x1800000u, f_08987300, 0x08987294u);
        } else if (S16(e + 18) + 0xC0 < S16(o + 18)) call2(c, o, ex + 0x1800000u, f_08987300, 0x089872C4u);
        else call2(c, o, ex, f_08987300, 0x089872D4u);
        int32_t v = (int32_t)*W(o + 96);
        *B(o + 11) = 0;
        if (v >= 0) *B(o + 11) = 1;
    }
    call1(c, o, f_08861848, 0x089872F0u);
    leave(c, sv);
}

/* (fighter): +543 = 2, ticked; without a negative frame flag +7 steps on, +237 cleared, +573 from
   0x08AA3D0C (by strength, or by 0x0886E294 with +81), fall's low half 0x40, a dash (8.0 / -0.625), +83
   = 12 and move 0x2B (stance 0xB with no choice made, or as 0x089B2188 says) or 0x28 + strength. */
SFA_FUNC(089B1EDC, move_guard_dash_pick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *B(o + 543) = 2;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    call1(c, o, f_08861848, 0x089B1F00u);
    if (S8(o + 77) >= 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x089B1F24u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08AA3D0Cu + i);
        *H(o + 0x6C) = 0x40;
        *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF6000u;
        *B(o + 0x53) = 0xC;
        uint32_t base;
        if (S8(o + 386) < 0) base = S8(o + 236) == 0xB ? 0x2B : 0x28;
        else {
            c->r[17] = 0x2B;
            call1(c, o, f_089B2188, 0x089B1F8Cu);
            base = c->r[2] == 0 ? 0x28 : 0x2B;
        }
        c->r[17] = base;
        call2(c, o, (base + (uint32_t)S8(o + 194)) & 0xFFFF, f_088616EC, 0x089B1FC8u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (part projectile): its next state with life cleared, +255 = 5, its tables (+356 0x08AAE4AC, +128
   0x08AAF22C, +200 0x08AAEFA6), 0x088D3E9C (0), +0 = 2, the picture 0x08AAE05C from 0xB, and its owner's
   place, picture, palette and facing; then 0x089C59A4. */
SFA_FUNC(089C58A4, part_shot_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint32_t n = *B(o + 4);
    *W(c->r[29] + 20) = s1;
    gaddr own = *W(o + 88);
    c->r[17] = own;
    *H(o + 112) = 0; *B(o + 4) = (uint8_t)(n + 1);
    int32_t life = S16(o + 112);
    *B(o + 123) = 0; *H(o + 114) = (uint16_t)life;
    *B(o + 255) = 5;
    *W(o + 356) = 0x08AAE4ACu; *W(o + 128) = 0x08AAF22Cu; *W(o + 200) = 0x08AAEFA6u;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 0, f_088D3E9C, 0x089C5910u);
    *B(o + 0) = 2; *B(o + 78) = 0; *B(o + 142) = 0;
    call3(c, o, 0x08AAE05Cu, 0xB, f_088D3D94, 0x089C5938u);
    *W(o + 16) = *W(own + 16); *W(o + 20) = *W(own + 20);
    uint32_t pic = *H(own + 24);
    *B(o + 140) = 1; *H(o + 24) = (uint16_t)pic;
    *B(o + 12) = *B(own + 12); *B(o + 13) = *B(own + 13);
    uint32_t f = *B(own + 11);
    *B(o + 1) = 1; *B(o + 11) = (uint8_t)f;
    call1(c, o, f_089C59A4, 0x089C5990u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): rides 0x08821330's object (x moved by its x change, kept at +0x334); when 0x08862004 lands
   it 0x089070C4; else with +238 set and falling, or rising from 145 up, +7 steps on into move 6 + 2 x
   strength; otherwise ticked until a frame flag. */
SFA_FUNC(08906ED4, move_ride_object) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08821330, 0x08906EE8u);
    gaddr m = o + 0x334;
    uint32_t ex = *W(c->r[2] + 16);
    *W(o + 16) = *W(o + 16) + (ex - *W(m));
    *W(m) = ex;
    call1(c, o, f_08862004, 0x08906F10u);
    if (c->r[2] != 0) { call1(c, o, f_089070C4, 0x08906FC8u); leave(c, sv); return; }
    int go;
    uint32_t back;
    if ((int32_t)*W(o + 100) < 0) { go = S8(o + 238) != 0; back = 0x08906F68u; }
    else { go = S16(o + 22) >= 145 && S8(o + 238) != 0; back = 0x08906FB0u; }
    if (go) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, (uint32_t)((s << 1) + 6), f_088616EC, back);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08906F44u);
    leave(c, sv);
}

/* (fighter): ticked on a frame flag; else (with +98 set 0x089B2A18, 0x089B298C, x not below 0) ended: a
   dash (8.0 / -0.625, +83 = 12) into move 0x2B (as 0x089B2188 says) or 0x28 + strength, or 0x2E with +7
   on once +100's low half, dropping by 256, turns negative. */
SFA_FUNC(089B1FDC, move_guard_dash_pick_b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    int32_t k = S8(o + 77);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int tick = k != 0;
    if (!tick) {
        if (S16(o + 98) != 0) {
            call1(c, o, f_089B2A18, 0x089B2010u);
            call1(c, o, f_089B298C, 0x089B2018u);
            if ((int32_t)*W(o + 96) < 0) *W(o + 96) = 0;
        }
        if (S8(o + 75) >= 0) tick = 1;
        else {
            *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF6000u;
            *B(o + 0x53) = 0xC;
            c->r[17] = 0x2B;
            call1(c, o, f_089B2188, 0x089B2060u);
            uint32_t base = c->r[2] == 0 ? 0x28 : 0x2B;
            gaddr p = o + 100;
            *H(p) = (uint16_t)(*H(p) - 0x100);
            if ((int16_t)*H(p) < 0) { bump(o + 7); base = 0x2E; }
            c->r[17] = base;
            call2(c, o, (base + (uint32_t)S8(o + 194)) & 0xFFFF, f_088616EC, 0x089B20B0u);
        }
    }
    if (tick) call1(c, o, f_08861848, 0x089B20CCu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
