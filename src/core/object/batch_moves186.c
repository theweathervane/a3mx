/* Steps after a hit and move steps of 20 - 47 instructions: the get-up checks that knock out a fighter
   with no life left (0x0885D584, 0x0885D2AC, 0x0885DDEC, 0x0885E550), the throw-hold pose that turns
   the opponent by the throw table (0x0886E17C), rises into a move with a hit cue, and a landing that
   sets the match's slow-down (0x08919D50). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861724(Cpu *c); void f_088616EC(Cpu *c); void f_08863208(Cpu *c);
void f_0886E238(Cpu *c); void f_0886E3CC(Cpu *c); void f_0887C614(Cpu *c); void f_088FE670(Cpu *c);
void f_0891B9E0(Cpu *c); void f_0886DB2C(Cpu *c);

#define MATCH 0x092E1228u

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

/* (fighter): out of life (+112) knocked out; else the get-up (state 1 / 1 / 2 / 0, +698 / +541
   cleared). */
SFA_FUNC(0885D584, fighter_ko_or_getup) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S16(o + 112) < 0) call1(c, o, f_08863208, 0x0885D5A0u);
    else { *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 2; *B(o + 7) = 0; *B(o + 698) = 0; *B(o + 541) = 0; }
    frame_out(c, ra);
}

/* (fighter): as 0x0885D584, the get-up also clearing +539 / +656. */
SFA_FUNC(0885D2AC, fighter_ko_or_getup_b) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S16(o + 112) < 0) call1(c, o, f_08863208, 0x0885D2C8u);
    else {
        *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 2; *B(o + 7) = 0;
        *B(o + 698) = 0; *B(o + 541) = 0; *B(o + 539) = 0; *B(o + 656) = 0;
    }
    frame_out(c, ra);
}

/* (fighter): down: the +283 wait runs down (ticked); out, knocked out without life, else +599 cleared,
   +7 steps on and move 0x19 of list 2. */
SFA_FUNC(0885DDEC, fighter_down_wait) {
    gaddr o = ARG(0); uint32_t ra;
    int8_t t = (int8_t)(S8(o + 283) - 1);
    frame_in(c, &ra);
    *B(o + 283) = (uint8_t)t;
    if (t != 0) call1(c, o, f_08861848, 0x0885DE14u);
    else if (S16(o + 112) < 0) call1(c, o, f_08863208, 0x0885DE34u);
    else {
        uint32_t st = *B(o + 7);
        *B(o + 599) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, 0x19, f_08861724, 0x0885DE54u);
    }
    frame_out(c, ra);
}

/* (fighter): the +82 wait runs down (ticked); out, knocked out without life; else +116 = 4 and with
   +702 step 5 and move 0x19, without it step 4, +283 by 8, at the floor and move 0x1B (list 2). */
SFA_FUNC(0885E550, fighter_hit_wait_down) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    frame_in(c, &ra);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t != 0) call1(c, o, f_08861848, 0x0885E578u);
    else if (S16(o + 112) < 0) call1(c, o, f_08863208, 0x0885E598u);
    else if (S8(o + 702) != 0) {
        *B(o + 116) = 4; *B(o + 7) = 5;
        call2(c, o, 0x19, f_08861724, 0x0885E5F4u);
    } else {
        int32_t k = S8(o + 283);
        *B(o + 116) = 4;
        int32_t floor = S16(o + 132);
        *B(o + 7) = 4;
        *B(o + 283) = (uint8_t)(k + 8);
        *W(o + 20) = (uint32_t)floor << 16;
        call2(c, o, 0x1B, f_08861724, 0x0885E5D4u);
    }
    frame_out(c, ra);
}

/* (fighter): holding a throw (+135): the opponent's pose from the throw table (+416 +36, by its state
   +338, frame +296 +8): +261 = -1 when the entry is negative, its facing = ours ^ the entry's bits 0 -
   1, and 0x0886E238 with the entry's picture. */
SFA_FUNC(0886E17C, throw_hold_pose) {
    gaddr o = ARG(0); uint32_t ra;
    gaddr opp = *W(o + 92);
    frame_in(c, &ra);
    if (S8(o + 135) != 0) {
        uint32_t fr = *B(*W(o + 296) + 8);
        int32_t st = S8(opp + 338);
        gaddr tab = *W(*W(*W(o + 416) + 36) + ((uint32_t)st << 2));
        int32_t i = (int16_t)(fr << 2);
        gaddr p = tab + (uint32_t)(i + i) + 4;
        int32_t h = S16(p);
        *B(opp + 261) = h < 0 ? 0xFF : 0;
        uint32_t f = (uint32_t)(int16_t)(*B(o + 11) & 0xFF);
        *B(opp + 11) = (uint8_t)(f ^ ((uint32_t)S16(p) & 3));
        ARG(0) = opp; ARG(1) = (uint32_t)S16(p + 2) & 0xFFFF; c->r[6] = p;
        c->r[31] = 0x0886E22Cu; f_0886E238(c);
    }
    frame_out(c, ra);
}

/* (fighter): ticked on a frame flag; else it rises (+73 = 1, +20 by +100 by +108) and once falling +7
   steps on, +820 = 2 x strength + 1, move 0x1D + strength and the hit cue 0x0886E3CC (9, 12 x +820 +
   12); else ticked. */
SFA_FUNC(088F0380, move_rise_1d_cue) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08861848, 0x088F03A0u);
    else {
        uint32_t vy = *W(o + 100);
        *B(o + 73) = 1;
        *W(o + 20) = *W(o + 20) + vy;
        uint32_t nv = vy + *W(o + 108);
        *W(o + 100) = nv;
        if ((int32_t)nv >= 0) call1(c, o, f_08861848, 0x088F03D8u);
        else {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 820) = (uint32_t)((s << 1) + 1);
            call2(c, o, (uint32_t)(s + 0x1D) & 0xFFFF, f_088616EC, 0x088F040Cu);
            uint32_t n = *W(o + 820);
            ARG(0) = o; ARG(1) = 9; ARG(2) = ((n + n + n) << 2) + 0xC; c->r[31] = 0x088F042Cu; f_0886E3CC(c);
        }
    }
    leave(c, sv);
}

/* (fighter): ticked on a frame flag; else in the air (+73 = 1) 0x088FE670 and it falls (+20 by +100 by
   +108); at or below the floor at the floor, +73 cleared, cue 0x22 / 0x23, step 6 and move 0x22 +
   strength; else ticked. */
SFA_FUNC(088FE56C, move_air_land_22) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08861848, 0x088FE58Cu);
    else {
        *B(o + 73) = 1;
        call1(c, o, f_088FE670, 0x088FE5ACu);
        uint32_t vy = *W(o + 100);
        *W(o + 20) = *W(o + 20) + vy;
        int32_t floor = S16(o + 132);
        *W(o + 100) = vy + *W(o + 108);
        if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x088FE618u);
        else {
            *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0;
            call1(c, o, f_0887C614, 0x088FE5ECu);
            int32_t s = S8(o + 194);
            *B(o + 7) = 6;
            call2(c, o, (uint32_t)(s + 0x22) & 0xFFFF, f_088616EC, 0x088FE608u);
        }
    }
    leave(c, sv);
}

/* (fighter): a pending slow-down (+0x33C) with the opponent standing sets the match's +35 = 24;
   0x0891B9E0; below the floor it lands (+7 on, at the floor, +262 / +73 / +20 cleared, 0x0886DB2C, cue
   0x22 / 0x23, move 0xC); else ticked. */
SFA_FUNC(08919D50, move_land_slowdown_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    gaddr p = o + 0x334;
    if (*B(p + 8) != 0 && S8(*W(o + 92) + 73) == 0) { *B(p + 8) = 0; *B(MATCH + 35) = 0x18; }
    call1(c, o, f_0891B9E0, 0x08919D9Cu);
    int32_t floor = S16(o + 132);
    if (S16(o + 22) < floor) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0886DB2C, 0x08919DE8u);
        call1(c, o, f_0887C614, 0x08919DF0u);
        call2(c, o, 0xC, f_088616EC, 0x08919DFCu);
    } else call1(c, o, f_08861848, 0x08919DB8u);
    leave(c, sv);
}
