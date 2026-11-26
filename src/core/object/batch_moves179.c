/* Move steps of 39 instructions: a frame flag that spawns a pair of effects on the fighter (0x089154E8),
   jumps with their speeds by strength, a leap scaled by the distance to the opponent (0x08936020), a
   charged-input jump (0x089429CC) and a rise that marks a cancel. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08861674(Cpu *c); void f_088616EC(Cpu *c); void f_08822EAC(Cpu *c);
void f_08917840(Cpu *c); void f_0887C614(Cpu *c); void f_08942A68(Cpu *c); void f_0894CAF4(Cpu *c);
void f_08961428(Cpu *c); void f_08877258(Cpu *c);

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

/* (fighter): a frame flag steps +7 on (+82 cleared) and spawns effect 0x2A / 1 tied to it 108 up, then
   0x2A / 0 at its place; 0x08917840. */
SFA_FUNC(089154E8, move_flag_effect_pair) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        uint32_t st = *B(o + 7);
        *H(o + 82) = 0; *B(o + 7) = (uint8_t)(st + 1);
        ARG(0) = 0x2A; ARG(1) = 1; c->r[31] = 0x0891551Cu; f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *W(e + 88) = o;
            *H(e + 18) = (uint16_t)S16(o + 18);
            *H(e + 22) = (uint16_t)(S16(o + 22) + 0x6C);
            ARG(0) = 0x2A; ARG(1) = 0; c->r[31] = 0x0891554Cu; f_08822EAC(c);
            gaddr e2 = c->r[2];
            if (e2 != 0) {
                *W(e2 + 88) = o;
                *H(e2 + 18) = (uint16_t)S16(o + 18);
                *H(e2 + 22) = (uint16_t)S16(o + 22);
            }
        }
    }
    call1(c, o, f_08917840, 0x08915574u);
    leave(c, sv);
}

/* (fighter): ended: +7 steps on with the four speeds by strength (0x08A643A8: x, y, x brake, fall; x
   ones toward the facing) and move 8; else ticked. */
SFA_FUNC(08919990, move_end_jump_8) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        gaddr e = 0x08A643A8u + ((uint32_t)s << 4);
        *W(o + 100) = *W(e + 4); *W(o + 108) = *W(e + 12);
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(e); *W(o + 104) = 0u - *W(e + 8); }
        else { *W(o + 96) = *W(e); *W(o + 104) = *W(e + 8); }
        call2(c, o, 8, f_088616EC, 0x08919A20u);
    } else call1(c, o, f_08861848, 0x089199ACu);
    frame_out(c, ra);
}

/* (fighter): it falls (+20 by +100 by +108); at or below the floor +7 steps on, +235 / +20 / +73
   cleared, at the floor, cue 0x22 / 0x23 and ticked; in the air ticked until a frame flag. */
SFA_FUNC(089317A4, move_fall_land_cue_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *W(o + 100) = vy + *W(o + 108);
    if (!(floor < y)) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 235) = 0; *H(o + 20) = 0; *B(o + 73) = 0; *H(o + 22) = (uint16_t)floor;
        call1(c, o, f_0887C614, 0x08931828u);
        call1(c, o, f_08861848, 0x08931830u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089317F8u);
    leave(c, sv);
}

/* (fighter): ended: a leap (+7 on, +73 = 1, y 6.5 / -0.28) whose x speed is (384 - the x distance to
   the opponent) / 32 toward the facing, and move 0x28; else ticked. */
SFA_FUNC(08936020, move_end_leap_by_gap) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1;
        *W(o + 100) = 0x68000; *W(o + 108) = 0xFFFFB800u;
        uint32_t d = *W(*W(o + 92) + 16) - *W(o + 16);
        if ((int32_t)d >= 0) d = 0u - d;
        d += 0x1800000u;
        if (*B(o + 11) != 0) d = 0u - d;
        *W(o + 96) = (uint32_t)((int32_t)d >> 5);
        *W(o + 104) = 0;
        call2(c, o, 0x28, f_088616EC, 0x0893609Cu);
    } else call1(c, o, f_08861848, 0x089360B0u);
    frame_out(c, ra);
}

/* (fighter): for choice 1 (+386) at strength 1 with +290 bit 1 a jump (step 2, x 10.0 / -1/16 toward the
   facing, y 4.5 / -0.375) into move 0x21 of its list; else 0x08942A68. */
SFA_FUNC(089429CC, move_pick_jump_21) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 386) == 1 && S8(o + 194) == 1 && (*H(o + 290) & 2)) {
        *B(o + 7) = 2;
        *W(o + 100) = 0x48000; *W(o + 108) = 0xFFFFA000u;
        if (*B(o + 11) == 0) { *W(o + 96) = 0xFFF60000u; *W(o + 104) = 0x1000; }
        else { *W(o + 96) = 0xA0000; *W(o + 104) = 0xFFFFF000u; }
        call2(c, o, 0x21, f_08861674, 0x08942A5Cu);
    } else call1(c, o, f_08942A68, 0x08942A38u);
    frame_out(c, ra);
}

/* (fighter): with +123 +7 steps on into a slow hop (x 0.75, y 1.5625 / -1/16, +82 = 48, +820 = 1); then
   once 0x0894CAF4 says so, step 5, fall -0.375 and move 0x2E; else ticked. */
SFA_FUNC(0894A014, move_hop_then_2e) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    enter(c, o, sv);
    if (f != 0) {
        bump(o + 7);
        *W(o + 96) = 0xC000; *W(o + 100) = 0x19000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFF000u;
        *H(o + 82) = 0x30; *W(o + 820) = 1;
    }
    call1(c, o, f_0894CAF4, 0x0894A070u);
    if (c->r[2] != 0) {
        *B(o + 7) = 5; *W(o + 108) = 0xFFFFA000u;
        call2(c, o, 0x2E, f_088616EC, 0x0894A0A0u);
    } else call1(c, o, f_08861848, 0x0894A080u);
    leave(c, sv);
}

/* (fighter): 0x08961428; below y speed 1.0 +7 steps on with fall -1/32, +234 = 1, +82 cleared; else the
   cancel mark (+74 = 1) by 0x08877258 (0x08A8582C) when locked (+373), by +760 when not; ticked. */
SFA_FUNC(0895EE54, move_rise_cancel_mark) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08961428, 0x0895EE68u);
    if ((int32_t)*W(o + 100) < 0x10000) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 108) = 0xFFFFF800u; *B(o + 234) = 1; *H(o + 82) = 0;
    } else if (S8(o + 373) != 0) {
        call2(c, o, 0x08A8582Cu, f_08877258, 0x0895EE94u);
        if (c->r[2] != 0) *B(o + 74) = 1;
    } else if (*H(o + 760) & 0x3FF) *B(o + 74) = 1;
    call1(c, o, f_08861848, 0x0895EEE0u);
    leave(c, sv);
}

/* (fighter): ticked; a frame flag starts a jump: +7 on, +73 = 1, the four speeds by strength (0x08A95720:
   x, x brake, y, fall; x ones toward the facing). */
SFA_FUNC(0898B20C, move_tick_flag_jump7) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0898B220u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        gaddr e = 0x08A95720u + ((uint32_t)s << 4);
        *B(o + 73) = 1;
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    }
    leave(c, sv);
}
