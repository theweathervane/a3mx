/* Move steps of 13 - 31 instructions: the most-used recovery step with its +82 timer (0x088EF90C, 7,149
   calls), knock-down landings by weight (0x0885C514), jumps with their speeds, and a chain step timer.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c); void f_0886469C(Cpu *c);
void f_0886DB2C(Cpu *c); void f_089178BC(Cpu *c); void f_08993064(Cpu *c); void f_08862004(Cpu *c);
void f_0887C614(Cpu *c); void f_0885C374(Cpu *c); void f_088FF65C(Cpu *c); void f_0891DE70(Cpu *c);
void f_08822EAC(Cpu *c); void f_08921850(Cpu *c); void f_08921940(Cpu *c); void f_0886E294(Cpu *c);

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
static uint32_t str_move(gaddr o, int32_t base) { return (uint32_t)(S8(o + 194) + base) & 0xFFFF; }

/* (fighter): the +82 wait runs down; past 0 the chain's next hit (0x089178BC). */
SFA_FUNC(089161C4, move_chain_wait) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    frame_in(c, &ra);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) call1(c, o, f_089178BC, 0x089161ECu);
    frame_out(c, ra);
}

/* (fighter): ticked until a frame flag; 0x08993064; on the floor +7 steps on, +73 cleared and move 0x4A +
   strength. */
SFA_FUNC(089900D4, move_tick_land_4a_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) call1(c, o, f_08861848, 0x089900F4u);
    call1(c, o, f_08993064, 0x089900FCu);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0;
        call2(c, o, str_move(o, 0x4A), f_088616EC, 0x0899013Cu);
    }
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing +7 steps on, +74 = 1, at the floor, +73 / +262 cleared, cue 0x22 /
   0x23; a frame flag sets the fall -0.375; ticked. */
SFA_FUNC(089A42D0, move_rise_land_cue_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A42E4u);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7); *B(o + 74) = 1;
        *W(o + 20) = (uint32_t)y << 16;
        *B(o + 73) = 0; *B(o + 262) = 0;
        call1(c, o, f_0887C614, 0x089A431Cu);
    }
    if (S8(o + 77) != 0) { *B(o + 77) = 0; *W(o + 108) = 0xFFFFA000u; }
    call1(c, o, f_08861848, 0x089A4338u);
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, +73 = 1, a hop (y 2.0 / -0.25) with x by strength
   (0x08A9E8A4) toward the facing; ticked. */
SFA_FUNC(089A7BE8, move_tick_77_hop2) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        bump(o + 7); *B(o + 73) = 1;
        int32_t k = S8(o + 194);
        *W(o + 108) = 0xFFFFC000u; *W(o + 104) = 0; *W(o + 100) = 0x20000;
        uint32_t v = *W(0x08A9E8A4u + ((uint32_t)k << 2));
        if (*B(o + 11) == 0) v = 0u - v;
        *W(o + 96) = v;
    }
    call1(c, o, f_08861848, 0x089A7C54u);
    frame_out(c, ra);
}

/* (fighter): 0x08862004; on landing +7 steps on, +20 / +73 cleared, at the floor, move 0x1A + 2 x
   strength; else ticked unless the frame flag is negative. */
SFA_FUNC(089A7C60, move_rise_land_1a) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A7C74u);
    if (c->r[2] != 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        int32_t y = S16(o + 132);
        *H(o + 20) = 0; *B(o + 73) = 0; *H(o + 22) = (uint16_t)y;
        call2(c, o, (uint32_t)((k << 1) + 0x1A) & 0xFFFF, f_088616EC, 0x089A7CB0u);
    } else if (S8(o + 77) >= 0) call1(c, o, f_08861848, 0x089A7CC8u);
    leave(c, sv);
}

/* (fighter): knocked down (kind 4, +631 cleared, +73 = -1, +125 counted): from weight 16 (+655) a full
   landing (0x0885C374); lighter, held at the floor and the landing move for its weight (0x08A191E0). */
SFA_FUNC(0885C514, move_knockdown_weight) {
    gaddr o = ARG(0);
    uint32_t ra;
    int32_t n = S8(o + 125);
    *B(o + 116) = 4; *B(o + 631) = 0; *B(o + 73) = 0xFF;
    int32_t wgt = S8(o + 655);
    *B(o + 125) = (uint8_t)(n + 1);
    frame_in(c, &ra);
    if (wgt >= 16) call1(c, o, f_0885C374, 0x0885C554u);
    else {
        int32_t floor = S16(o + 132);
        if (S16(o + 22) < floor) *H(o + 22) = (uint16_t)floor;
        call2(c, o, *B(0x08A191E0u + (uint32_t)wgt), f_08861724, 0x0885C584u);
    }
    frame_out(c, ra);
}

/* (fighter): ended: standing. Else the +82 timer (when set) runs down, clearing +262 as it runs out;
   0x0886DB2C when it is out (or was not set); ticked. */
static void timer_db2c(Cpu *c, uint32_t b_end, uint32_t b_out, uint32_t b_unset, uint32_t b_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, b_end);
    else {
        int32_t t = S16(o + 82);
        if (t == 0) call1(c, o, f_0886DB2C, b_unset);
        else {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, b_out); }
        }
        call1(c, o, f_08861848, b_tick);
    }
    leave(c, sv);
}
SFA_FUNC(088EF90C, move_recover_timer) { timer_db2c(c, 0x088EF92Cu, 0x088EF970u, 0x088EF948u, 0x088EF978u); }
SFA_FUNC(089309AC, move_recover_timer_b) { timer_db2c(c, 0x089309CCu, 0x08930A00u, 0x08930A10u, 0x08930A18u); }

/* (fighter): 0x088FF65C; once falling +7 steps on, a slow drift (x 1.0 toward the facing, fall -1.0) and
   move 0x2E; else ticked. */
SFA_FUNC(088FF4A0, move_fall_drift_2e) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088FF65C, 0x088FF4B4u);
    if ((int32_t)*W(o + 100) < 0) {
        uint32_t st = *B(o + 7);
        *W(o + 100) = 0;
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 104) = 0;
        uint32_t f = *B(o + 11);
        *W(o + 108) = 0xFFFF0000u;
        *W(o + 96) = f != 0 ? 0x10000 : 0xFFFF0000u;
        call2(c, o, 0x2E, f_088616EC, 0x088FF50Cu);
    } else call1(c, o, f_08861848, 0x088FF4C8u);
    leave(c, sv);
}

/* (fighter): +82 counts down, ticked, 0x0891DE70; a negative frame flag steps +7 on; any frame flag (low
   7 bits) steps it that far toward its facing. */
SFA_FUNC(0891B7FC, move_tick_flag_shift) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
    call1(c, o, f_08861848, 0x0891B81Cu);
    call1(c, o, f_0891DE70, 0x0891B824u);
    int32_t k = S8(o + 77);
    if (k < 0) { uint32_t st = *B(o + 7); *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1); }
    if (k & 0x7F) {
        int32_t d = *B(o + 11) == 0 ? -k : k;
        int32_t x = S16(o + 18);
        *B(o + 77) = 0;
        *H(o + 18) = (uint16_t)(x + d);
    }
    leave(c, sv);
}

/* (fighter): falls (+20 by +100); 32 above the floor +6 steps on and effect 0x1E is spawned, tied to it;
   ticked. */
SFA_FUNC(0891FA10, move_fall_effect_1e) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    *W(o + 20) = *W(o + 20) + *W(o + 100);
    int32_t floor = S16(o + 132);
    int near = (int32_t)(((uint32_t)S16(o + 22) - 0x20) & 0xFFFF) < (int32_t)((uint32_t)floor & 0xFFFF);
    enter(c, o, sv);
    if (near) {
        bump(o + 6);
        ARG(0) = 0x1E; ARG(1) = 0; c->r[31] = 0x0891FA64u; f_08822EAC(c);
        if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    }
    call1(c, o, f_08861848, 0x0891FA7Cu);
    leave(c, sv);
}

/* (fighter): 0x08921850; when set 0x08921940; else with +123 +7 steps on, the opponent's +127 = 16 and
   move 0xC; else ticked. */
SFA_FUNC(08921498, move_check_123_c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08921850, 0x089214ACu);
    if (c->r[2] != 0) call1(c, o, f_08921940, 0x08921504u);
    else if (S8(o + 123) != 0) {
        gaddr opp = *W(o + 92);
        bump(o + 7);
        *B(opp + 127) = 0x10;
        call2(c, o, 0xC, f_088616EC, 0x089214E4u);
    } else call1(c, o, f_08861848, 0x089214F4u);
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +237 cleared, +7 steps on, +573 from the halfwords at 0x08A6DBD0
   by its +194 (with +81: by 0x0886E294), and x 3.0. */
SFA_FUNC(08931974, move_tick_573_q) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931988u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x089319ACu);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S16(0x08A6DBD0u + i + i);
        *W(o + 96) = 0x30000; *W(o + 104) = 0;
    }
    leave(c, sv);
}

/* (fighter): a frame flag starts a jump (+7 on, +73 = 1, x 5.0 / -0.09 toward the facing, y 4.5 /
   -0.3125); ticked. */
SFA_FUNC(089387E4, move_tick_flag_jump4) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) != 0) {
        bump(o + 7);
        uint32_t f = *B(o + 11);
        *B(o + 73) = 1;
        if (f == 0) { *W(o + 96) = 0x50000; *W(o + 104) = 0xFFFFE800u; }
        else { *W(o + 96) = 0xFFFB0000u; *W(o + 104) = 0x1800; }
        *W(o + 100) = 0x48000; *W(o + 108) = 0xFFFFB000u;
    }
    call1(c, o, f_08861848, 0x08938854u);
    frame_out(c, ra);
}
