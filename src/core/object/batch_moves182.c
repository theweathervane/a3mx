/* Move steps of 15 - 43 instructions: the shadow step's entry, a chase that waits on the opponent's
   state, a landing with a knock-down cue, a creep step, a throw start that checks the opponent
   (0x0899CE64), part characters' counted hits that release the opponent's hold (+592 / +270), and a
   dash start (0x088F9568). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0893BB94(Cpu *c);
void f_0893BC3C(Cpu *c); void f_08862004(Cpu *c); void f_0887C614(Cpu *c); void f_0886E3CC(Cpu *c);
void f_0898F9DC(Cpu *c); void f_08993064(Cpu *c); void f_088D3140(Cpu *c); void f_088D3EC0(Cpu *c);
void f_089B7598(Cpu *c); void f_089BE80C(Cpu *c); void f_08837098(Cpu *c); void f_0887CE20(Cpu *c);
void f_088F9788(Cpu *c);

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

/* (fighter): at +6 = 0 the shadow is spawned (0x0893BB94), later 0x0893BC3C. */
SFA_FUNC(0893BB58, move_shadow_entry) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (*B(o + 6) == 0) call1(c, o, f_0893BB94, 0x0893BB74u);
    else call1(c, o, f_0893BC3C, 0x0893BB88u);
    frame_out(c, ra);
}

/* (fighter): ended: with the opponent alive (+112) and down (+73 negative) a leap (+7 on, x 12.0 /
   -0.75, y 6.0 / +1/16) into move 0x32 + strength; else step 0xB with a wait (+82) of 30; else
   ticked. */
SFA_FUNC(08968400, move_end_chase_32) {
    gaddr o = ARG(0); uint32_t ra;
    gaddr opp = *W(o + 92);
    frame_in(c, &ra);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x0896841Cu);
    else if (S16(opp + 112) >= 0 && S8(opp + 73) < 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 96) = 0xC0000; *W(o + 104) = 0xFFFF4000u; *W(o + 100) = 0x60000;
        int32_t s = S8(o + 194);
        *W(o + 108) = 0x1000;
        call2(c, o, (uint32_t)(s + 0x32) & 0xFFFF, f_088616EC, 0x0896849Cu);
    } else { *B(o + 7) = 0xB; *B(o + 0x52) = 0x1E; }
    frame_out(c, ra);
}

/* (fighter): ticked while a frame flag is up; else in the air (+73 = 1) until 0x08862004 lands it: +7
   on, cue 0x22 / 0x23, 0x0886E3CC (9, 10), at the floor, +73 / +20 / +262 cleared and move 0x13 +
   strength. */
SFA_FUNC(08973F34, move_air_land_13) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08861848, 0x08973F54u);
    else {
        *B(o + 73) = 1;
        call1(c, o, f_08862004, 0x08973F6Cu);
        if (c->r[2] == 0) call1(c, o, f_08861848, 0x08973F7Cu);
        else {
            bump(o + 7);
            call1(c, o, f_0887C614, 0x08973F94u);
            ARG(0) = o; ARG(1) = 9; ARG(2) = 0xA; c->r[31] = 0x08973FA4u; f_0886E3CC(c);
            int32_t floor = S16(o + 132);
            *B(o + 73) = 0;
            int32_t s = S8(o + 194);
            *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 262) = 0;
            call2(c, o, (uint32_t)(s + 0x13) & 0xFFFF, f_088616EC, 0x08973FCCu);
        }
    }
    leave(c, sv);
}

/* (fighter): a frame flag with +123 runs 0x0898F9DC; else +123 cleared, x stopped once +96 is negative,
   and falling +7 steps on with a creep (x 0.5) into move 6 + strength; else 0x08993064 and ticked. */
SFA_FUNC(0899002C, move_flag_or_creep_6) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0 && S8(o + 123) != 0) call1(c, o, f_0898F9DC, 0x08990058u);
    else {
        uint32_t v = *W(o + 96);
        *B(o + 123) = 0;
        if ((int32_t)v < 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
        if ((int32_t)*W(o + 100) < 0) {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 96) = 0x8000; *W(o + 104) = 0;
            call2(c, o, (uint32_t)(s + 6) & 0xFFFF, f_088616EC, 0x089900ACu);
        } else {
            call1(c, o, f_08993064, 0x089900BCu);
            call1(c, o, f_08861848, 0x089900C4u);
        }
    }
    leave(c, sv);
}

/* (fighter): step 0: step 1 and move 0x30 when the opponent is in state 3 and the match's +61 is set,
   else move 0x22; later standing once ended, else ticked. */
SFA_FUNC(0899CE64, move_throw_start_check) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t st = *B(o + 7);
    frame_in(c, &ra);
    if (st == 0) {
        gaddr opp = *W(o + 92);
        *B(o + 7) = 1;
        if (S8(opp + 338) == 3 && S8(MATCH + 61) != 0) call2(c, o, 0x30, f_0886163C, 0x0899CEBCu);
        else call2(c, o, 0x22, f_0886163C, 0x0899CEA8u);
    } else if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 7) = 0; *B(o + 6) = 0; *B(o + 5) = 0;
        call2(c, o, 0, f_0886163C, 0x0899CF00u);
    } else call1(c, o, f_08861848, 0x0899CEDCu);
    frame_out(c, ra);
}

/* (character): the opponent's hold (+592) is let go (+270 = 1) unless +251; frame flag +76 runs
   0x088D3140; the part ticked, 0x089B7598; each frame flag counts +828 down, and at 0 +7 steps on into
   move 3 + strength with +262 cleared. */
SFA_FUNC(089B5A04, part_count_hits_3) {
    gaddr o = ARG(0); uint32_t sv[2];
    gaddr opp = *W(o + 92);
    enter(c, o, sv);
    if (S8(opp + 592) != 0 && S8(o + 251) == 0) { *B(opp + 270) = 1; *B(opp + 592) = 0; }
    if (S8(o + 76) != 0) call1(c, o, f_088D3140, 0x089B5A4Cu);
    call1(c, o, f_088D3EC0, 0x089B5A54u);
    call1(c, o, f_089B7598, 0x089B5A5Cu);
    if (S8(o + 77) != 0) {
        uint32_t n = *W(o + 828) - 1;
        *B(o + 77) = 0;
        *W(o + 828) = n;
        if (n == 0) {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            call2(c, o, (uint32_t)(s + 3), f_088616EC, 0x089B5A98u);
            *B(o + 262) = 0;
        }
    }
    leave(c, sv);
}

/* (character): the opponent's hold (+592) is let go (+270 = 1); the part ticked; each frame flag counts
   +82 down (at 0 +7 steps on into move 9), and with +824 = 1 it is cleared and 0x089BE80C (1). */
SFA_FUNC(089BB844, part_count_9) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    gaddr opp = *W(o + 92);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(opp + 592) != 0) { *B(opp + 270) = 1; *B(opp + 592) = 0; }
    call1(c, o, f_088D3EC0, 0x089BB878u);
    if (S8(o + 77) != 0) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *B(o + 77) = 0;
        *H(o + 82) = (uint16_t)t;
        c->r[17] = 1;
        if ((int16_t)t == 0) { bump(o + 7); call2(c, o, 9, f_088616EC, 0x089BB8BCu); }
        if (*W(o + 824) == 1) { *W(o + 824) = 0; call2(c, o, 1, f_089BE80C, 0x089BB8D8u); }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a dash: +262 = 1, step 1; locked (+373) +235 cleared and the guard (+78) from the facing
   and +497, else +644 = 2; 0x08837098, x 8.0 (11.5 without +193) / -0.1875, +549 = -1 (0 with +80),
   move 0x14, 0x0887CE20 and 0x088F9788. */
SFA_FUNC(088F9568, move_dash_start_14) {
    gaddr o = ARG(0); uint32_t sv[2];
    *B(o + 262) = 1;
    int32_t lock = S8(o + 373);
    *B(o + 7) = 1;
    enter(c, o, sv);
    if (lock != 0) {
        uint32_t f = *B(o + 11);
        int32_t g = S8(o + 497);
        *B(o + 235) = 0;
        *B(o + 78) = (uint8_t)(f ^ (uint32_t)g);
    } else *B(o + 644) = 2;
    call1(c, o, f_08837098, 0x088F95B4u);
    *W(o + 104) = 0xFFFFD000u;
    *W(o + 96) = S8(o + 193) != 0 ? 0x80000u : 0xB8000u;
    int32_t h = S8(o + 80);
    *B(o + 549) = 0xFF;
    if (h != 0) *B(o + 549) = 0;
    call2(c, o, 0x14, f_088616EC, 0x088F95F4u);
    call1(c, o, f_0887CE20, 0x088F95FCu);
    call1(c, o, f_088F9788, 0x088F9604u);
    leave(c, sv);
}
