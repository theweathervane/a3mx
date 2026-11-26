/* Move steps of 11 - 25 instructions: knock-downs and their landings, throws with their speeds, timed
   recoveries and the part animation's end test (0x088D3EC0). Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c);
void f_0885C454(Cpu *c); void f_0885C350(Cpu *c); void f_0885C374(Cpu *c); void f_0886E3CC(Cpu *c);
void f_0895E620(Cpu *c); void f_0886E294(Cpu *c); void f_0899FC90(Cpu *c); void f_08862004(Cpu *c);
void f_0887C614(Cpu *c); void f_089B298C(Cpu *c); void f_0887C408(Cpu *c); void f_08880C38(Cpu *c);
void f_08880C54(Cpu *c); void f_0886314C(Cpu *c); void f_088EFBF4(Cpu *c); void f_0892B168(Cpu *c);

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

/* (fighter): +270 = 1, +586 = 0x7F, +568 = 1, then knocked down (0x0885C454). */
SFA_FUNC(0885C3B8, move_knockdown_launch) {
    gaddr o = ARG(0);
    uint32_t ra;
    *B(o + 270) = 1; *B(o + 586) = 0x7F;
    frame_in(c, &ra);
    *B(o + 568) = 1;
    call1(c, o, f_0885C454, 0x0885C3D8u);
    frame_out(c, ra);
}

/* (fighter): alive without +698 a heavy landing (+655 = 0x16, +117 = 12, 0x0885C350); else knocked down. */
SFA_FUNC(0885C590, move_heavy_or_knockdown) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 698) == 0 && S16(o + 112) >= 0) {
        *B(o + 655) = 0x16; *B(o + 117) = 0xC;
        call1(c, o, f_0885C350, 0x0885C5D4u);
    } else call1(c, o, f_0885C454, 0x0885C5B8u);
    frame_out(c, ra);
}

/* (fighter): ticked; a frame flag sets +656 = 31; when the animation ends +7 steps on and +83 = 12. */
SFA_FUNC(08921C3C, move_tick_656_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08921C50u);
    if (S8(o + 77) != 0) { *B(o + 77) = 0; *B(o + 656) = 0x1F; }
    if (S8(o + 75) < 0) { bump(o + 7); *B(o + 0x53) = 0xC; }
    leave(c, sv);
}

/* (fighter): without a frame flag +196 counts, +73 = 1 and the jump speeds are set (x 9.0 / -0.75, y
   5.0 / -0.5); ticked. */
SFA_FUNC(08937340, move_tick_77_jump3) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1);
        *B(o + 73) = 1;
        *W(o + 96) = 0x90000; *W(o + 104) = 0xFFFF4000u; *W(o + 100) = 0x50000; *W(o + 108) = 0xFFFF8000u;
    }
    call1(c, o, f_08861848, 0x08937394u);
    frame_out(c, ra);
}

/* (fighter): without a held opponent (+228) +7 steps on with move 0x15; else ticked; then 0x0886E3CC (8, 4). */
SFA_FUNC(0893E684, move_hold_or_15) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    gaddr h = *W(o + 228);
    enter(c, o, sv);
    if (h == 0) { bump(o + 7); call2(c, o, 0x15, f_088616EC, 0x0893E6B4u); }
    else call1(c, o, f_08861848, 0x0893E6C4u);
    call3(c, o, 8, 4, f_0886E3CC, 0x0893E6D4u);
    leave(c, sv);
}

/* (fighter): 0x0895E620; when it is clear +196 counts, +74 = 1 and ticked; else ticked until a frame flag. */
SFA_FUNC(0895E4B8, move_helper_count) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0895E620, 0x0895E4CCu);
    if (c->r[2] == 0) {
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1); *B(o + 74) = 1;
        call1(c, o, f_08861848, 0x0895E4F0u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0895E508u);
    leave(c, sv);
}

/* (fighter): +7 steps on, +262 = 1; without +80, in step 7 (+6) +254 counts down, else +1 = 1; +696
   cleared and move 0xE. */
SFA_FUNC(08969118, move_step_e) {
    gaddr o = ARG(0);
    uint32_t ra;
    bump(o + 7);
    int32_t k = S8(o + 80);
    frame_in(c, &ra);
    *B(o + 262) = 1;
    if (k == 0) {
        if (*B(o + 6) == 7) *B(o + 254) = (uint8_t)(S8(o + 254) - 1);
        else *B(o + 1) = 1;
    }
    *B(o + 696) = 0;
    call2(c, o, 0xE, f_088616EC, 0x0896916Cu);
    frame_out(c, ra);
}

/* (fighter): without a frame flag +237 cleared, +7 steps on, 0x0886E294, +573 cleared and move 3; else
   ticked. */
SFA_FUNC(089697D0, move_flag_or_3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x08969800u);
        *B(o + 573) = 0;
        call2(c, o, 3, f_088616EC, 0x08969810u);
    } else call1(c, o, f_08861848, 0x08969820u);
    leave(c, sv);
}

/* (fighter): 0x0899FC90; once falling +7 steps on with move 3 + its +194; else ticked. */
SFA_FUNC(0899FB48, move_fall_then_3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0899FC90, 0x0899FB5Cu);
    if ((int32_t)*W(o + 100) < 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        call2(c, o, (uint32_t)(k + 3) & 0xFFFF, f_088616EC, 0x0899FB88u);
    } else call1(c, o, f_08861848, 0x0899FB98u);
    leave(c, sv);
}

/* (fighter): 0x08862004; when it lands +7 steps on, +74 = 1, at the floor (+20 = +132), +73 cleared,
   cue 0x22 / 0x23; ticked either way. */
SFA_FUNC(089A4A20, move_rise_land_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A4A34u);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7); *B(o + 74) = 1;
        *W(o + 20) = (uint32_t)y << 16;
        *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x089A4A68u);
    }
    call1(c, o, f_08861848, 0x089A4A70u);
    leave(c, sv);
}

/* (fighter): 0x089B298C; moving back steps +7 on; ticked until a frame flag; 0x0886E3CC (9, 4). */
SFA_FUNC(089B2460, move_back_tick_e3cc) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089B298C, 0x089B2474u);
    if ((int32_t)*W(o + 96) < 0) bump(o + 7);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089B24A0u);
    call3(c, o, 9, 4, f_0886E3CC, 0x089B24B0u);
    leave(c, sv);
}

/* (fighter): +7 steps on, +82 = 3, +824 = 1, +123 cleared, +644 = 2, move 8 and +262 = 1. */
SFA_FUNC(089BB7E4, move_step_8_hold) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    bump(o + 7);
    *W(c->r[29] + 20) = s1;
    c->r[17] = 1;
    *H(o + 82) = 3; *W(o + 824) = 1; *B(o + 123) = 0; *B(o + 644) = 2;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 8, f_088616EC, 0x089BB82Cu);
    *B(o + 262) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): facing +697; step 0 with move 0x15 when the opponent (+92) is out (+73) or neither +310 nor
   +696 is set; else step 2 with move 0x18. */
SFA_FUNC(0885BEAC, move_guard_or_15) {
    gaddr o = ARG(0);
    uint32_t ra;
    gaddr opp = *W(o + 92);
    *B(o + 11) = *B(o + 697);
    uint32_t a = (uint32_t)(S8(o + 310) | S8(o + 696));
    uint32_t cond = (uint32_t)S8(opp + 73) | (a < 1u);
    frame_in(c, &ra);
    if (cond == 0) { *B(o + 7) = 2; call2(c, o, 0x18, f_0886163C, 0x0885BF04u); }
    else { *B(o + 7) = 0; call2(c, o, 0x15, f_0886163C, 0x0885BEECu); }
    frame_out(c, ra);
}

/* (fighter): landed with kind 4 (+116), +631 cleared, +656 = `mark`; the first time (+`once`) 0x`fn`;
   cue 0x2C and the landing (0x0885C374). */
static void land_once(Cpu *c, uint8_t mark, uint32_t once, void (*fn)(Cpu *), uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 116) = 4; *B(o + 631) = 0;
    int32_t k = S8(o + once);
    *B(o + 656) = mark;
    if (k == 0) { *B(o + once) = 1; call1(c, o, fn, b1); }
    call3(c, o, (uint32_t)(int32_t)S8(o + 337), 0x2C, f_0887C408, b2);
    call1(c, o, f_0885C374, b3);
    leave(c, sv);
}
SFA_FUNC(0885C660, move_land_once_561) { land_once(c, 1, 561, f_08880C38, 0x0885C69Cu, 0x0885C6ACu, 0x0885C6B4u); }
SFA_FUNC(0885C744, move_land_once_560) { land_once(c, 2, 560, f_08880C54, 0x0885C780u, 0x0885C790u, 0x0885C798u); }

/* (fighter): the +82 timer runs down; at 0 +7 steps on, +73 = -1, 0x0886314C and move 0x15 of list 2. */
SFA_FUNC(0885DCE4, move_timer_then_15) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    enter(c, o, sv);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) {
        bump(o + 7); *B(o + 73) = 0xFF;
        call1(c, o, f_0886314C, 0x0885DD2Cu);
        call2(c, o, 0x15, f_08861724, 0x0885DD38u);
    }
    leave(c, sv);
}

/* (part): an ending step ends the animation (+75 = -1), else ticked; returns -128 once ended, else 0. */
SFA_FUNC(088D3EC0, part_tick_ended) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    gaddr st = *W(o + 28);
    enter(c, o, sv);
    if (S8(st + 1) & 0x40) *B(o + 75) = 0xFF;
    else call1(c, o, f_08861848, 0x088D3EECu);
    c->r[2] = (uint32_t)(int32_t)(int8_t)(S8(o + 75) & 0x80);
    leave(c, sv);
}

/* (fighter): once the animation has ended the +820 count runs down; past it +7 steps on with move 8;
   otherwise it moves on (+16 by +96) and ticks. */
SFA_FUNC(088EFADC, move_slide_count_8) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    int done = 0;
    if (S8(o + 75) < 0) {
        uint32_t v = *W(o + 820) - 1;
        *W(o + 820) = v;
        if ((int32_t)v < 0) { bump(o + 7); call2(c, o, 8, f_088616EC, 0x088EFB14u); done = 1; }
    }
    if (!done) {
        *W(o + 16) = *W(o + 16) + *W(o + 96);
        call1(c, o, f_08861848, 0x088EFB34u);
    }
    frame_out(c, ra);
}

/* (fighter): 0x088EFBF4; when it lands +7 steps on, cue 0x22 / 0x23, at the floor (+20 = +132) and move
   0x2E; else ticked. */
SFA_FUNC(088F04BC, move_land_2e) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088EFBF4, 0x088F04D0u);
    if (c->r[2] != 0) {
        bump(o + 7);
        call1(c, o, f_0887C614, 0x088F04F8u);
        *W(o + 20) = (uint32_t)S16(o + 132) << 16;
        call2(c, o, 0x2E, f_088616EC, 0x088F0510u);
    } else call1(c, o, f_08861848, 0x088F04E0u);
    leave(c, sv);
}

/* (fighter): moves on (+16 by +96, +96 by +104); when the animation ends +7 steps on with move 0x3E;
   else ticked. */
SFA_FUNC(08916A60, move_slide_end_3e) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t v = *W(o + 96);
    *W(o + 16) = *W(o + 16) + v;
    int32_t e = S8(o + 75);
    *W(o + 96) = v + *W(o + 104);
    frame_in(c, &ra);
    if (e & 0x80) { bump(o + 7); call2(c, o, 0x3E, f_088616EC, 0x08916AB8u); }
    else call1(c, o, f_08861848, 0x08916A9Cu);
    frame_out(c, ra);
}

/* (fighter): ticked; on a frame flag +7 steps on, +73 = 1 and it steps 40 forward (+18 by the facing +11). */
SFA_FUNC(08926F28, move_tick_step40) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08926F3Cu);
    if (S8(o + 77) != 0) {
        bump(o + 7);
        uint32_t f = *B(o + 11);
        *B(o + 73) = 1;
        *H(o + 18) = (uint16_t)(S16(o + 18) + (f != 0 ? 0x28 : -0x28));
    }
    leave(c, sv);
}

/* (fighter): 0x0892B168; moves by +96 toward its facing (+11), +96 by +104; once that turns, step 3;
   ticked. */
SFA_FUNC(0892B03C, move_slide_facing) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0892B168, 0x0892B050u);
    uint32_t v = *W(o + 96);
    uint32_t d = *B(o + 11) == 0 ? 0u - v : v;
    *W(o + 16) = *W(o + 16) + d;
    uint32_t v2 = v + *W(o + 104);
    *W(o + 96) = v2;
    if ((int32_t)v2 < 0) *B(o + 7) = 3;
    call1(c, o, f_08861848, 0x0892B090u);
    leave(c, sv);
}
