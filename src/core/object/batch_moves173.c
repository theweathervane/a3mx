/* Move steps of 35 - 36 instructions: landings into the next move, the down-and-out check after a hit
   (0x0885D21C, 8,061 calls), a block stance that copies the opponent's crouch (0x0887709C), and part
   characters' timed recoveries. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c);
void f_0886469C(Cpu *c); void f_0886DB2C(Cpu *c); void f_0887C614(Cpu *c); void f_0896F62C(Cpu *c);
void f_089A8524(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D3140(Cpu *c); void f_088D3B18(Cpu *c);
void f_088D31EC(Cpu *c); void f_089CA064(Cpu *c); void f_088D35D0(Cpu *c); void f_088D36D0(Cpu *c);
void f_088D2DBC(Cpu *c); void f_088D3960(Cpu *c); void f_08863E1C(Cpu *c);

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
static int at_floor16(gaddr o) {
    return !((int32_t)((uint32_t)S16(o + 132) & 0xFFFF) < (int32_t)((uint32_t)S16(o + 22) & 0xFFFF));
}

/* (fighter): it falls (+20 by +100 by +108); at or below the floor step 7, height to the floor, +235 /
   +73 cleared, cue 0x22 / 0x23 and ticked; in the air ticked until a frame flag. */
SFA_FUNC(08965704, move_fall_land_7_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *W(o + 100) = vy + *W(o + 108);
    if (!(floor < y)) {
        *B(o + 7) = 7; *H(o + 22) = (uint16_t)floor; *B(o + 235) = 0; *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x08965778u);
        call1(c, o, f_08861848, 0x08965780u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08965758u);
    leave(c, sv);
}

/* (fighter): 0x0896F62C; at or below the floor +7 steps on, +20 / +73 / +83 cleared and move 7 + 2 x
   strength; else ticked. */
SFA_FUNC(0896DDB4, move_land_7_2k) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896F62C, 0x0896DDC8u);
    if (at_floor16(o)) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)S16(o + 132);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 0x53) = 0;
        call2(c, o, (uint32_t)((S8(o + 194) << 1) + 7) & 0xFFFF, f_088616EC, 0x0896DE20u);
    } else call1(c, o, f_08861848, 0x0896DE30u);
    leave(c, sv);
}

/* (fighter): ticked, 0x0896F62C; falling at or below the floor step 6, +20 / +73 cleared, +74 = 1, +82 =
   strength + 1 and move 0x16 + 2 x strength. */
SFA_FUNC(0896E118, move_tick_fall_land_16) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0896E12Cu);
    call1(c, o, f_0896F62C, 0x0896E134u);
    if ((int32_t)*W(o + 100) < 0 && at_floor16(o)) {
        int32_t floor = S16(o + 132);
        *B(o + 7) = 6;
        int32_t k = S8(o + 194);
        *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 74) = 1;
        *H(o + 82) = (uint16_t)(k + 1);
        call2(c, o, (uint32_t)((k << 1) + 0x16) & 0xFFFF, f_088616EC, 0x0896E194u);
    }
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, 0x089A8524, +73 = 1 and a jump by strength (0x08A9E80C: x
   toward the facing, y; fall -0.25); ticked. */
SFA_FUNC(089A7180, move_flag_jump_8524) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        bump(o + 7);
        call1(c, o, f_089A8524, 0x089A71ACu);
        int32_t s = S8(o + 194);
        *B(o + 73) = 1;
        gaddr e = 0x08A9E80Cu + ((uint32_t)s << 3);
        *W(o + 108) = 0xFFFFC000u; *W(o + 104) = 0;
        *W(o + 100) = *W(e + 4);
        uint32_t vx = *W(e);
        *W(o + 96) = *B(o + 11) == 0 ? 0u - vx : vx;
    }
    call1(c, o, f_08861848, 0x089A71FCu);
    leave(c, sv);
}

/* (character): the part ticked; ended: standing; else a frame flag runs 0x088D3140, and the +82 timer
   (when set) runs down, 0x0886DB2C when it is out (clearing +262) or was not set. */
SFA_FUNC(089B9F74, part_flag_timer) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089B9F88u);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089B9F9Cu);
    else {
        if (S8(o + 77) != 0) call1(c, o, f_088D3140, 0x089B9FB4u);
        int32_t t = S16(o + 82);
        if (t == 0) call1(c, o, f_0886DB2C, 0x089B9FC8u);
        else {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, 0x089B9FF0u); }
        }
    }
    leave(c, sv);
}

/* (character): the part ticked; at end byte 1 it is reset (0x088D3B18 (0)) with +82 = 8; ended: standing;
   else the +82 timer runs down (clearing +262 as it runs out) and 0x0886DB2C. */
SFA_FUNC(089C1BA8, part_reset_timer) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C1BBCu);
    if (S8(o + 75) == 1) {
        *B(o + 75) = 0;
        ARG(0) = 0; c->r[31] = 0x089C1BD8u; f_088D3B18(c);
        *H(o + 82) = 8;
    }
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089C1BF4u);
    else {
        int32_t t = S16(o + 82);
        if (t > 0) {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) *B(o + 262) = 0;
        }
        call1(c, o, f_0886DB2C, 0x089C1C24u);
    }
    leave(c, sv);
}

/* (character): the part ticked, 0x088D31EC, 0x089CA064; when 0x088D35D0 gives 1, +73 cleared, +7 steps
   on, 0x088D36D0, cue 0x22, 0x088D3960 (0, 0), move 0x38 + strength and +262 cleared. */
SFA_FUNC(089C9470, part_arrive_38_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C9484u);
    call1(c, o, f_088D31EC, 0x089C948Cu);
    call1(c, o, f_089CA064, 0x089C9494u);
    call1(c, o, f_088D35D0, 0x089C949Cu);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_088D36D0, 0x089C94C0u);
        call1(c, o, f_088D2DBC, 0x089C94C8u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089C94D8u; f_088D3960(c);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x38), f_088616EC, 0x089C94E8u);
        *B(o + 262) = 0;
    }
    leave(c, sv);
}

/* (fighter): after a hit: when 0x08863E1C (0) says so +7 steps on with +560 - +562 cleared; while alive
   (+112) or with +584 below 8 ticked; else knocked out (step 3, move 0x1B of list 2). */
SFA_FUNC(0885D21C, fighter_hit_ko_check) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call2(c, o, 0, f_08863E1C, 0x0885D234u);
    if (c->r[2] != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 560) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 561) = 0; *B(o + 562) = 0;
    }
    if (S16(o + 112) >= 0) call1(c, o, f_08861848, 0x0885D268u);
    else if (S16(o + 584) < 8) call1(c, o, f_08861848, 0x0885D284u);
    else { *B(o + 7) = 3; call2(c, o, 0x1B, f_08861724, 0x0885D29Cu); }
    leave(c, sv);
}

/* (fighter): blocking: step 1 / 0 / 6 / 0; crouching (+696, step 2, move 0x18) when the opponent is in
   the fight and crouched (+310 / +696), else move 0x15; the block flags +480 / +481 = 1, +482 = 6, +483
   cleared. */
SFA_FUNC(0887709C, move_block_start) {
    gaddr o = ARG(0); uint32_t ra;
    *B(o + 4) = 1; *B(o + 6) = 6; *B(o + 7) = 0;
    gaddr opp = *W(o + 92);
    *B(o + 5) = 0;
    int32_t out = S8(opp + 73);
    frame_in(c, &ra);
    uint32_t crouch = 0, n = 0x15;
    if (out == 0 && (S8(opp + 310) != 0 || S8(opp + 696) != 0)) { crouch = 1; *B(o + 7) = 2; n = 0x18; }
    *B(o + 696) = (uint8_t)crouch;
    *B(o + 481) = 1; *B(o + 480) = 1; *B(o + 482) = 6; *B(o + 483) = 0;
    call2(c, o, n, f_0886163C, 0x08877120u);
    frame_out(c, ra);
}
