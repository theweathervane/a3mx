/* Move steps of 31 instructions: jumps with four speeds by strength, landings into the next move with a
   cue, the timed recovery step (also for the part animation), a projectile spawned on a frame flag
   (0x089612D0) and a timed multi-hit. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c); void f_0886DB2C(Cpu *c);
void f_088D3EC0(Cpu *c); void f_08961448(Cpu *c); void f_0896134C(Cpu *c); void f_08822EAC(Cpu *c);
void f_08862004(Cpu *c); void f_0887C614(Cpu *c); void f_089930D8(Cpu *c); void f_0899CF6C(Cpu *c);
void f_089A45C8(Cpu *c);

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

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1 and the four speeds by strength (`tab`, 16
   bytes each: x, y, x brake, y fall). */
static void tick_flag_speeds(Cpu *c, gaddr tab, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, back);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        gaddr e = tab + ((uint32_t)S8(o + 194) << 4);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1;
        *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4); *W(o + 104) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    leave(c, sv);
}
SFA_FUNC(0895DB68, move_tick_flag_speeds_a) { tick_flag_speeds(c, 0x08A85AF0u, 0x0895DB7Cu); }
SFA_FUNC(08995698, move_tick_flag_speeds_b) { tick_flag_speeds(c, 0x08A98838u, 0x089956ACu); }

/* (fighter): 0x08961448; a frame flag sets +73; once moving back +7 steps on with move 0x25 + strength;
   else ticked. */
SFA_FUNC(0895E8D0, move_back_25_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08961448, 0x0895E8E4u);
    if (S8(o + 73) != 0 && S8(o + 77) != 0) *B(o + 73) = 1;
    if ((int32_t)*W(o + 96) < 0) { bump(o + 7); call2(c, o, str_move(o, 0x25), f_088616EC, 0x0895E92Cu); }
    else call1(c, o, f_08861848, 0x0895E93Cu);
    leave(c, sv);
}

/* (fighter): a frame flag fires: unless 0x0896134C says no, +656 = 0x22 and projectile 0x78 / 1 is
   spawned, carrying the fighter's +86 / +164 / +148 and tied to it (+88); ticked. */
SFA_FUNC(089612D0, move_flag_projectile) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        *B(o + 77) = 0;
        call1(c, o, f_0896134C, 0x089612F4u);
        if (c->r[2] == 0) {
            *B(o + 656) = 0x22;
            ARG(0) = 0x78; ARG(1) = 1; c->r[31] = 0x0896130Cu; f_08822EAC(c);
            gaddr e = c->r[2];
            if (e != 0) {
                *H(e + 86) = *H(o + 86);
                *W(e + 164) = *W(o + 164);
                uint32_t w = *W(o + 148);
                *W(e + 88) = o;
                *W(e + 148) = w;
            }
        }
    }
    call1(c, o, f_08861848, 0x0896133Cu);
    leave(c, sv);
}

/* Ended: standing. Else the +82 timer (when set) runs down, clearing +262 as it runs out; 0x0886DB2C when
   it is out (or was not set); then `tick`. */
static void timer_db2c(Cpu *c, void (*tick)(Cpu *), uint32_t b_end, uint32_t b_out, uint32_t b_unset, uint32_t b_tick) {
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
        call1(c, o, tick, b_tick);
    }
    leave(c, sv);
}
SFA_FUNC(08963C28, move_recover_timer_c) {
    timer_db2c(c, f_08861848, 0x08963C48u, 0x08963C8Cu, 0x08963C64u, 0x08963C94u);
}
SFA_FUNC(089B5D10, part_recover_timer) {
    timer_db2c(c, f_088D3EC0, 0x089B5D30u, 0x089B5D74u, 0x089B5D4Cu, 0x089B5D7Cu);
}
SFA_FUNC(089B63D0, part_recover_timer_b) {
    timer_db2c(c, f_088D3EC0, 0x089B63F0u, 0x089B6434u, 0x089B640Cu, 0x089B643Cu);
}

/* (fighter): ticked until a frame flag; each flag counts the +82 wait down, and when bit 7 turns on +7
   steps on with move 0x29. */
SFA_FUNC(089897FC, move_flag_count_29) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08989818u);
    else {
        *B(o + 77) = 0;
        gaddr w = o + 0x52;
        uint32_t t = (uint32_t)*B(w) - 1;
        *B(w) = (uint8_t)t;
        if (t & 0x80) { bump(o + 7); call2(c, o, 0x29, f_088616EC, 0x08989858u); }
        else call1(c, o, f_08861848, 0x0898986Cu);
    }
    frame_out(c, ra);
}

/* (fighter): 0x08862004; in the air ticked; on the floor +7 steps on, +20 cleared, cue 0x22 / 0x23,
   +262 / +73 cleared and move 0x24 + strength. */
SFA_FUNC(0898B304, move_land_24) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x0898B318u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x0898B334u);
    else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x0898B354u);
        int32_t k = S8(o + 194);
        *B(o + 262) = 0; *B(o + 73) = 0;
        call2(c, o, (uint32_t)(k + 0x24) & 0xFFFF, f_088616EC, 0x0898B370u);
    }
    leave(c, sv);
}

/* (fighter): 0x089930D8; in the air ticked; on the floor +7 steps on, +73 / +235 cleared, cue 0x22 /
   0x23 and move 0x75 + strength. */
SFA_FUNC(08991674, move_land_75) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089930D8, 0x08991688u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x089916A4u);
    else {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0; *B(o + 235) = 0;
        call1(c, o, f_0887C614, 0x089916CCu);
        call2(c, o, str_move(o, 0x75), f_088616EC, 0x089916E0u);
    }
    leave(c, sv);
}

/* (fighter): 0x0899CF6C; in the air ticked; on the floor +7 steps on, +73 / +262 cleared, cue 0x22 /
   0x23 and move 0xC + 2 x strength. */
SFA_FUNC(0899BBC0, move_land_c2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0899CF6C, 0x0899BBD4u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x0899BBF0u);
    else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 0; *B(o + 262) = 0;
        call1(c, o, f_0887C614, 0x0899BC14u);
        call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0xC) & 0xFFFF, f_088616EC, 0x0899BC2Cu);
    }
    leave(c, sv);
}

/* (fighter): a frame flag starts the next hit: step 3, +73 = 1, flags cleared, +82 from 0x08A9D6B4 by the
   hit count (+820), 0x089A45C8 and the count up; ticked. */
SFA_FUNC(089A46B8, move_flag_next_hit) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        *B(o + 7) = 3; *B(o + 73) = 1;
        uint32_t n = *W(o + 820);
        *B(o + 76) = 0; *B(o + 77) = 0;
        *H(o + 82) = (uint16_t)S8(0x08A9D6B4u + n);
        call1(c, o, f_089A45C8, 0x089A4710u);
        *W(o + 820) = *W(o + 820) + 1;
    }
    call1(c, o, f_08861848, 0x089A4724u);
    leave(c, sv);
}
