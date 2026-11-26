/* Move steps of 22 - 30 instructions: landings into the next move, a combo-chain step from a per-strength
   list (0x089178BC), throws placed beside the opponent, and the usual speeds by strength. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_0886A530(Cpu *c); void f_0886DB2C(Cpu *c); void f_08963984(Cpu *c); void f_08963840(Cpu *c);
void f_089120F0(Cpu *c); void f_08921850(Cpu *c); void f_08921940(Cpu *c); void f_08929118(Cpu *c);
void f_0886E294(Cpu *c); void f_08821330(Cpu *c); void f_0892B168(Cpu *c); void f_0887C614(Cpu *c);
void f_0886E638(Cpu *c); void f_08961448(Cpu *c); void f_0896F62C(Cpu *c); void f_08993064(Cpu *c);

#define BATTLE 0x092E1228u

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
static gaddr by_strength(gaddr tab, gaddr o, uint32_t size) { return tab + (uint32_t)S8(o + 194) * size; }
static uint32_t str_move(gaddr o, int32_t base) { return (uint32_t)(S8(o + 194) + base) & 0xFFFF; }

/* (fighter): 0x08963984; in the air ticked until a frame flag; on the floor it lands (0x08963840). */
SFA_FUNC(089637E8, move_air_or_land_d) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08963984, 0x089637FCu);
    if (S16(o + 132) < S16(o + 22)) { if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08963830u); }
    else call1(c, o, f_08963840, 0x08963818u);
    leave(c, sv);
}

/* (fighter): the +82 timer (when set) runs down, clearing +262 as it runs out; ended: 0x0886A530 and
   standing; else 0x0886DB2C and ticked. */
SFA_FUNC(089036B0, move_timer_end_a530) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int32_t t = S16(o + 82);
    if (t != 0) {
        *H(o + 82) = (uint16_t)(t - 1);
        if ((int16_t)(t - 1) == 0) *B(o + 262) = 0;
    }
    if (S8(o + 75) < 0) { call1(c, o, f_0886A530, 0x089036F8u); call1(c, o, f_0886469C, 0x08903700u); }
    else { call1(c, o, f_0886DB2C, 0x08903710u); call1(c, o, f_08861848, 0x08903718u); }
    leave(c, sv);
}

/* (fighter): while the battle is paused ticked; else +7 steps on with the move for its +692 (0x08A53BAC). */
SFA_FUNC(0890886C, move_after_pause_b) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(BATTLE + 36) == 0 && S8(BATTLE + 28) == 0) {
        int32_t k = S8(o + 692);
        bump(o + 7);
        call2(c, o, (uint32_t)S8(0x08A53BACu + (uint32_t)k) & 0xFFFF, f_0886163C, 0x089088C4u);
    } else call1(c, o, f_08861848, 0x089088D8u);
    frame_out(c, ra);
}

/* (fighter): ticked, 0x089120F0; without a frame flag +7 steps on, it turns round and takes the x speeds
   by strength (0x08A5AEC0). */
SFA_FUNC(0891200C, move_tick_turn_speed2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08912020u);
    call1(c, o, f_089120F0, 0x08912028u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7), f = *B(o + 11);
        gaddr e = by_strength(0x08A5AEC0u, o, 8);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 11) = (uint8_t)(f ^ 1);
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
    }
    leave(c, sv);
}

/* (fighter): the next hit of a chain: entry (the mark's +6) of its strength's list (0x08A60054); a
   negative entry ends the chain (+7 steps on), else step 2 and that move, the entry counted. */
SFA_FUNC(089178BC, move_chain_next) {
    gaddr o = ARG(0);
    uint32_t ra;
    gaddr list = *W(0x08A60054u + ((uint32_t)S8(o + 194) << 2));
    gaddr m = o + 0x334;
    int32_t n = S16(list + 2 * (uint32_t)S8(m + 6));
    frame_in(c, &ra);
    if (n < 0) bump(o + 7);
    else {
        *B(o + 7) = 2;
        *B(m + 6) = (uint8_t)(S8(m + 6) + 1);
        call2(c, o, (uint32_t)n & 0xFFFF, f_088616EC, 0x08917928u);
    }
    frame_out(c, ra);
}

/* (fighter): 0x08921850; when set 0x08921940; else once falling +7 steps on with move 0xE + strength;
   else ticked. */
SFA_FUNC(08921420, move_check_fall_e) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08921850, 0x08921434u);
    if (c->r[2] != 0) call1(c, o, f_08921940, 0x08921488u);
    else if ((int32_t)*W(o + 100) < 0) { bump(o + 7); call2(c, o, str_move(o, 0xE), f_088616EC, 0x08921468u); }
    else call1(c, o, f_08861848, 0x08921478u);
    leave(c, sv);
}

/* (fighter): 0x08929118; once falling +7 steps on, a slow drift (x 1.0 toward the facing, fall -1.0) and
   move 0xB; else ticked. */
SFA_FUNC(08926F8C, move_fall_drift_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08929118, 0x08926FA0u);
    if ((int32_t)*W(o + 100) < 0) {
        uint32_t f = *B(o + 11);
        bump(o + 7);
        *W(o + 96) = f != 0 ? 0x10000 : 0xFFFF0000u;
        *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 108) = 0xFFFF0000u;
        call2(c, o, 0xB, f_088616EC, 0x08926FF4u);
    } else call1(c, o, f_08861848, 0x08926FB4u);
    leave(c, sv);
}

/* (fighter): ticked; without frame byte +76 +237 cleared, +7 steps on and +573 from 0x08A6A4E8 by its
   +194 (with +81: by 0x0886E294). */
SFA_FUNC(089274E8, move_tick_573_o) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089274FCu);
    if (S8(o + 76) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x08927520u);
        uint32_t i = S8(o + 81) != 0 ? (uint32_t)(int32_t)(int16_t)c->r[2] : (uint32_t)S8(o + 194);
        *B(o + 573) = (uint8_t)S8(0x08A6A4E8u + i);
    }
    leave(c, sv);
}

/* (fighter): a throw: +1 = 1, step 2, +108 keeps its x, placed 0x230 / -0xB0 from the opponent
   (0x08821330) by its side, x 8.0, move 0x22. */
SFA_FUNC(08929CD0, move_throw_place) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 1) = 1;
    uint32_t x = *W(o + 16);
    *B(o + 7) = 2; *W(o + 108) = x;
    call1(c, o, f_08821330, 0x08929D00u);
    int32_t d = S8(o + 337) != 0 ? 0x230 : -0xB0;
    *H(o + 18) = (uint16_t)(d + S16(c->r[2] + 18));
    *W(o + 96) = 0x80000; *W(o + 104) = 0;
    call2(c, o, 0x22, f_0886163C, 0x08929D38u);
    leave(c, sv);
}

/* (fighter): 0x0892B168; in the air ticked until a frame flag; on the floor step 4 with +73 / +262 /
   +235 cleared, cue 0x22 / 0x23 and ticked. */
SFA_FUNC(0892B0A0, move_air_or_land_4) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0892B168, 0x0892B0B4u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) { if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0892B0DCu); }
    else {
        *B(o + 7) = 4; *B(o + 73) = 0; *H(o + 22) = (uint16_t)floor; *B(o + 262) = 0; *B(o + 235) = 0;
        call1(c, o, f_0887C614, 0x0892B100u);
        call1(c, o, f_08861848, 0x0892B108u);
    }
    leave(c, sv);
}

/* (fighter): the +83 wait runs down; past 0 +196 counts, +573 cleared, 0x0886E638 on the character's
   table (+416: +16, +196, +192, 5) and move 0x33. */
SFA_FUNC(089372C8, move_wait_then_33) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    gaddr w = o + 0x53;
    int8_t t = (int8_t)(S8(w) - 1);
    enter(c, o, sv);
    *B(w) = (uint8_t)t;
    if (t < 0) {
        gaddr tab = *W(o + 416);
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1);
        *B(o + 573) = 0;
        ARG(0) = tab + 0x10; ARG(1) = *W(tab + 196); ARG(2) = *W(tab + 192); ARG(3) = 5;
        c->r[31] = 0x08937324u; f_0886E638(c);
        call2(c, o, 0x33, f_088616EC, 0x08937330u);
    }
    leave(c, sv);
}

/* (fighter): ticked until a frame flag; 0x08961448; on the floor +7 steps on, +20 / +73 cleared and move
   0x4A + strength. */
SFA_FUNC(0895EB18, move_tick_land_4a) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) call1(c, o, f_08861848, 0x0895EB38u);
    call1(c, o, f_08961448, 0x0895EB40u);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0;
        call2(c, o, str_move(o, 0x4A), f_088616EC, 0x0895EB80u);
    }
    leave(c, sv);
}

/* (fighter): flies (+16 by +96, +20 by +100 by +108); once falling +7 steps on, +820 = strength and move
   9 + strength; else ticked. */
SFA_FUNC(08963D80, move_fly_then_9_b) {
    gaddr o = ARG(0);
    uint32_t ra;
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    uint32_t v = vy + *W(o + 108);
    frame_in(c, &ra);
    *W(o + 100) = v;
    if ((int32_t)v < 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        *W(o + 820) = (uint32_t)k;
        call2(c, o, (uint32_t)(k + 9) & 0xFFFF, f_088616EC, 0x08963DECu);
    } else call1(c, o, f_08861848, 0x08963DC0u);
    frame_out(c, ra);
}

/* (fighter): +543 = 2; ticked; unless the frame flag is negative +237 cleared, +7 steps on and +573 from
   0x08A8DEB4 by its +194 (with +81: by 0x0886E294). */
SFA_FUNC(0896D9AC, move_tick_573_p) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 543) = 2;
    call1(c, o, f_08861848, 0x0896D9CCu);
    if (S8(o + 77) >= 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x0896D9F0u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08A8DEB4u + i);
    }
    leave(c, sv);
}

/* (fighter): 0x0896F62C; at or below the floor it lands (+7 on, +20 / +73 cleared, +83 = 90, cue 0x22 /
   0x23); ticked. */
SFA_FUNC(0896DA94, move_land_90_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896F62C, 0x0896DAA8u);
    int32_t floor = S16(o + 132);
    if (!((int32_t)((uint32_t)floor & 0xFFFF) < (int32_t)((uint32_t)S16(o + 22) & 0xFFFF))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 0x53) = 0x5A;
        call1(c, o, f_0887C614, 0x0896DAF4u);
    }
    call1(c, o, f_08861848, 0x0896DAFCu);
    leave(c, sv);
}

/* (fighter): 0x08993064; a frame flag clears +73; once moving back +7 steps on with move 0x25 + strength;
   else ticked. */
SFA_FUNC(0898FE9C, move_back_25) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08993064, 0x0898FEB0u);
    if (S8(o + 73) != 0 && S8(o + 77) != 0) *B(o + 73) = 0;
    if ((int32_t)*W(o + 96) < 0) { bump(o + 7); call2(c, o, str_move(o, 0x25), f_088616EC, 0x0898FF04u); }
    else call1(c, o, f_08861848, 0x0898FEE0u);
    leave(c, sv);
}
