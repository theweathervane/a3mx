/* Move steps of 24 - 33 instructions: endings that stand or crouch by a flag, slides that stop
   themselves, a flying slide (0x0892776C), and moves picked by charge or by a lock. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861674(Cpu *c); void f_0886469C(Cpu *c); void f_08864740(Cpu *c);
void f_0886DB2C(Cpu *c); void f_088D3EC0(Cpu *c); void f_0886E294(Cpu *c); void f_088FE670(Cpu *c);
void f_08908BD8(Cpu *c); void f_0887C614(Cpu *c); void f_0890EFE0(Cpu *c); void f_08876A0C(Cpu *c);

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
static int ended(gaddr o) { return S8(o + 75) < 0; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): ended: crouching with +696, else standing; not ended ticked. */
SFA_FUNC(0885B640, move_end_by_696) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) & 0x80) {
        if (S8(o + 696) != 0) call1(c, o, f_08864740, 0x0885B680u);
        else call1(c, o, f_0886469C, 0x0885B66Cu);
    } else call1(c, o, f_08861848, 0x0885B694u);
    frame_out(c, ra);
}

/* (character): ended: standing for stance 2 (+236), else crouching; not ended the part ticks. */
SFA_FUNC(089C7D4C, part_end_by_stance) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) {
        if (S8(o + 236) == 2) call1(c, o, f_0886469C, 0x089C7D78u);
        else call1(c, o, f_08864740, 0x089C7D8Cu);
    } else call1(c, o, f_088D3EC0, 0x089C7DA0u);
    frame_out(c, ra);
}

/* (fighter): ended: standing below stance 2 (+236), else crouching; not ended ticked. */
SFA_FUNC(089B144C, move_end_by_stance) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) {
        if ((int32_t)*B(o + 236) < 2) call1(c, o, f_0886469C, 0x089B147Cu);
        else call1(c, o, f_08864740, 0x089B1490u);
    } else call1(c, o, f_08861848, 0x089B14A4u);
    frame_out(c, ra);
}

/* (fighter): the +82 timer runs down (past it +262 cleared and 0x0886DB2C); ended: crouching, else ticked. */
SFA_FUNC(08926DDC, move_timer_crouch) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    enter(c, o, sv);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, 0x08926E14u); }
    if (S8(o + 75) & 0x80) call1(c, o, f_08864740, 0x08926E2Cu);
    else call1(c, o, f_08861848, 0x08926E3Cu);
    leave(c, sv);
}

/* (fighter): ended: standing. Not moving: ticked. Moving: it slides by +96 toward its facing, +96 by
   +104, stopping (speeds cleared) once that turns. */
SFA_FUNC(088F0E1C, move_slide_stop) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) call1(c, o, f_0886469C, 0x088F0E38u);
    else {
        uint32_t v = *W(o + 96);
        if (v == 0) call1(c, o, f_08861848, 0x088F0E54u);
        else {
            *W(o + 16) = *W(o + 16) + toward(o, v);
            uint32_t v2 = v + *W(o + 104);
            *W(o + 96) = v2;
            if ((int32_t)v2 < 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
        }
    }
    frame_out(c, ra);
}

/* (fighter): ticked; on frame byte +76 +237 cleared, +7 steps on, 0x0886E294 and +573 from 0x08A4BAAC by
   strength (0 with +81); without a frame flag 0x088FE670. */
SFA_FUNC(088FE2FC, move_tick_573_s) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088FE310u);
    if (S8(o + 76) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x088FE334u);
        *B(o + 573) = S8(o + 81) != 0 ? 0 : *B(0x08A4BAACu + (uint32_t)S8(o + 194));
    }
    if (S8(o + 77) == 0) call1(c, o, f_088FE670, 0x088FE370u);
    leave(c, sv);
}

/* (fighter): it falls faster (+108 down 1/8), 0x08908BD8; at or below the floor it lands (+7 on, +262 /
   +73 / +20 cleared, cue 0x22 / 0x23); ticked unless the frame flag is negative. */
SFA_FUNC(08906A58, move_fall_land_cue) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *W(o + 108) = *W(o + 108) - 0x2000;
    call1(c, o, f_08908BD8, 0x08906A78u);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x08906AB8u);
    }
    if (S8(o + 77) >= 0) call1(c, o, f_08861848, 0x08906ACCu);
    leave(c, sv);
}

/* (fighter): locked (+373) or not choice 1 (+386): 0x0890EFE0; else move 2k (with +290 bit 0) or 2k + 1. */
SFA_FUNC(0890EF5C, move_pick_lock) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 373) != 0) call1(c, o, f_0890EFE0, 0x0890EF78u);
    else if (S8(o + 386) != 1) call1(c, o, f_0890EFE0, 0x0890EF98u);
    else {
        uint32_t n = (uint32_t)S8(o + 194) << 1;
        if (*H(o + 290) & 1) call2(c, o, n & 0xFFFF, f_08861674, 0x0890EFD4u);
        else call2(c, o, (n + 1) & 0xFFFF, f_08861674, 0x0890EFC0u);
    }
    frame_out(c, ra);
}

/* (fighter): when 0x08876A0C says so, back to step 0 (stronger versions also step 23 forward / 24
   back by the facing); ticked. */
SFA_FUNC(0890F03C, move_check_reset) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08876A0C, 0x0890F050u);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x0890F060u);
    else if (S8(o + 194) == 0) { *B(o + 7) = 0; call1(c, o, f_08861848, 0x0890F07Cu); }
    else {
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + 0x17 : x + 0xFFE8);
        *B(o + 7) = 0;
        call1(c, o, f_08861848, 0x0890F0B0u);
    }
    leave(c, sv);
}

/* (fighter): flies (+20 by +100 by +108, +16 by +96 toward the facing, +96 by +104) until +96 turns, then
   +7 steps on with the x speeds cleared; ticked. */
SFA_FUNC(0892776C, move_fly_slide) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t v = *W(o + 96);
    frame_in(c, &ra);
    if ((int32_t)v < 0) {
        uint32_t st = *B(o + 7);
        *W(o + 104) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 96) = 0;
    } else {
        uint32_t vy = *W(o + 100);
        *W(o + 20) = *W(o + 20) + vy;
        *W(o + 100) = vy + *W(o + 108);
        uint32_t x = *W(o + 16) + toward(o, v);
        uint32_t b = *W(o + 104);
        *W(o + 16) = x;
        *W(o + 96) = v + b;
    }
    call1(c, o, f_08861848, 0x089277E4u);
    frame_out(c, ra);
}
