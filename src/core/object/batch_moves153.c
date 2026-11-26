/* Move steps of 25 - 28 instructions: the walk step (0x08861E30), landings and jumps with their speeds by
   strength, an effect spawned with the step, and the per-strength move picks. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861674(Cpu *c);
void f_08861D78(Cpu *c); void f_08862004(Cpu *c); void f_0886E294(Cpu *c); void f_089B2A18(Cpu *c);
void f_089B298C(Cpu *c); void f_08821368(Cpu *c); void f_0885D134(Cpu *c); void f_0887C408(Cpu *c);
void f_088F3FA4(Cpu *c); void f_0887C614(Cpu *c); void f_08822EAC(Cpu *c); void f_089088E4(Cpu *c);
void f_08923864(Cpu *c); void f_0893E0B0(Cpu *c);

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

/* (fighter): walking: when the held buttons (+752) include the crouch mask for its guard (+78 bit 0,
   0x08A1959C) it crouches (0x08861D78); it moves on by +96. */
SFA_FUNC(08861E30, move_walk) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    uint32_t mask = *H(0x08A1959Cu + 2 * ((uint32_t)S8(o + 78) & 1));
    uint32_t held = *H(o + 752);
    enter(c, o, sv);
    if (held & mask) call1(c, o, f_08861D78, 0x08861E74u);
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    leave(c, sv);
}

/* (fighter): ticked; without the frame byte `flag`, +7 steps on, +237 cleared and +573 from `tab` by its
   +194 (with +81: the table's entry 3). */
static void tick_573_entry3(Cpu *c, uint32_t flag, gaddr tab, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, back);
    if (S8(o + flag) == 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 81);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 237) = 0;
        *B(o + 573) = (uint8_t)(k != 0 ? S8(tab + 3) : S8(tab + (uint32_t)S8(o + 194)));
    }
    leave(c, sv);
}
SFA_FUNC(089643EC, move_tick_573_f) { tick_573_entry3(c, 76, 0x08A8A950u, 0x08964400u); }

/* (fighter): ticked; without the frame flag +237 cleared, +7 steps on (or step 3) and +573 from `tab`
   by its +194 (with +81: by 0x0886E294). */
static void tick_573_e294(Cpu *c, int set3, int sext, gaddr tab, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, b1);
    if (S8(o + 77) == 0) {
        if (set3) *B(o + 7) = 3; else { uint32_t st = *B(o + 7); *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1); }
        if (set3) *B(o + 237) = 0;
        call1(c, o, f_0886E294, b2);
        uint32_t r = sext ? (uint32_t)(int32_t)(int8_t)c->r[2] : c->r[2];
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : r;
        *B(o + 573) = (uint8_t)S8(tab + i);
    }
    leave(c, sv);
}
SFA_FUNC(089A4BAC, move_tick_573_g) { tick_573_e294(c, 0, 0, 0x08A9D738u, 0x089A4BC0u, 0x089A4BE4u); }
SFA_FUNC(08907638, move_tick_573_h) { tick_573_e294(c, 0, 1, 0x08A53AD8u, 0x0890764Cu, 0x08907670u); }

/* (fighter): ticked; without a frame flag +7 steps on, 64 higher, +73 = 1 and a jump (y 8.5 / -0.375). */
SFA_FUNC(0896E5D0, move_tick_hop_up) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0896E5E4u);
    if (S8(o + 77) == 0) {
        int32_t y = S16(o + 22);
        bump(o + 7);
        *H(o + 22) = (uint16_t)(y + 0x40);
        *B(o + 73) = 1;
        *W(o + 104) = 0; *W(o + 96) = 0; *W(o + 100) = 0x88000; *W(o + 108) = 0xFFFFA000u;
    }
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing +7 steps on at the floor, +73 / +262 cleared and move 0x5D + its
   +194; else ticked. */
SFA_FUNC(089909D8, move_land_5d) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089909ECu);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7);
        *W(o + 20) = (uint32_t)y << 16;
        int32_t k = S8(o + 194);
        *B(o + 73) = 0; *B(o + 262) = 0;
        call2(c, o, (uint32_t)(k + 0x5D) & 0xFFFF, f_088616EC, 0x08990A34u);
    } else call1(c, o, f_08861848, 0x089909FCu);
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing +7 steps on, +73 / +262 / +20 cleared at the floor and ticked; else
   ticked while the frame flag allows (`nonneg`: not negative, else zero). */
static void rise_land(Cpu *c, int nonneg, uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, b0);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7); *B(o + 73) = 0; *B(o + 262) = 0; *H(o + 22) = (uint16_t)y; *H(o + 20) = 0;
        call1(c, o, f_08861848, b1);
    } else {
        int32_t k = S8(o + 77);
        if (nonneg ? k >= 0 : k == 0) call1(c, o, f_08861848, b2);
    }
    leave(c, sv);
}
SFA_FUNC(089A6974, move_rise_land_a) { rise_land(c, 0, 0x089A6988u, 0x089A69B8u, 0x089A69D0u); }
SFA_FUNC(089A720C, move_rise_land_b) { rise_land(c, 1, 0x089A7220u, 0x089A7250u, 0x089A7268u); }

/* (fighter): 0x089B2A18, 0x089B298C; moving back or on a frame flag +7 steps on; ticked. */
SFA_FUNC(089B211C, move_back_or_flag_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089B2A18, 0x089B2130u);
    call1(c, o, f_089B298C, 0x089B2138u);
    if ((int32_t)*W(o + 96) < 0 || S8(o + 77) != 0) { bump(o + 7); call1(c, o, f_08861848, 0x089B2178u); }
    else call1(c, o, f_08861848, 0x089B2158u);
    leave(c, sv);
}

/* (fighter): alive, a random one of its two voices (0x201 / 0x202 + its +414) from its side; else the
   one-off cue set (0x0885D134). */
SFA_FUNC(0885D080, fighter_voice_random) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t hp = S16(o + 112);
    enter(c, o, sv);
    if (hp < 0) call1(c, o, f_0885D134, 0x0885D0E0u);
    else {
        c->r[31] = 0x0885D0A0u; f_08821368(c);
        uint32_t side = (uint32_t)(int32_t)S8(o + 337);
        int32_t v = S8(o + 414);
        ARG(0) = o; ARG(1) = side;
        if (c->r[2] & 1) { ARG(2) = (uint32_t)(v + 0x202); c->r[31] = 0x0885D0D0u; }
        else { ARG(2) = (uint32_t)(v + 0x201); c->r[31] = 0x0885D0BCu; }
        f_0887C408(c);
    }
    leave(c, sv);
}

/* (fighter): 0x088F3FA4; when it lands, cue 0x22 / 0x23, at the floor, +73 / +20 cleared, step 8 and move
   0x13; else ticked. */
SFA_FUNC(088F4F64, move_land_13) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088F3FA4, 0x088F4F78u);
    if ((int32_t)c->r[2] < 0) {
        call1(c, o, f_0887C614, 0x088F4F88u);
        int32_t y = S16(o + 132);
        *H(o + 22) = (uint16_t)y; *B(o + 73) = 0; *H(o + 20) = 0; *B(o + 7) = 8;
        call2(c, o, 0x13, f_088616EC, 0x088F4FB4u);
    } else call1(c, o, f_08861848, 0x088F4FC4u);
    leave(c, sv);
}

/* (fighter): ticked; a frame flag starts a jump: step 2, +73 = 1, x by strength (0x08A49868), y 7.0 /
   -0.34. */
SFA_FUNC(088FA524, move_tick_flag_jump2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088FA538u);
    if (S8(o + 77) != 0) {
        int32_t k = S8(o + 194);
        *B(o + 7) = 2; *B(o + 77) = 0; *B(o + 73) = 1;
        uint32_t vx = *W(0x08A49868u + ((uint32_t)k << 2));
        *W(o + 104) = 0; *W(o + 96) = vx;
        *W(o + 100) = 0x70000; *W(o + 108) = 0xFFFFA800u;
    }
    leave(c, sv);
}

/* (fighter): without a frame flag step 4, +73 = 1 and the four speeds by strength (0x08A51630, 16 bytes
   each); ticked. */
SFA_FUNC(08903D30, move_tick_77_speeds4) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        gaddr e = 0x08A51630u + ((uint32_t)S8(o + 194) << 4);
        *B(o + 7) = 4; *B(o + 73) = 1;
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    call1(c, o, f_08861848, 0x08903D94u);
    frame_out(c, ra);
}

/* (fighter): on frame byte +76 +7 steps on and effect 0x76 / 12 spawned, tied to it (+88, on, speeds
   7 / 0x63); then 0x089088E4. */
SFA_FUNC(08908AD4, move_flag_effect_76) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 76);
    enter(c, o, sv);
    if (k != 0) {
        bump(o + 7);
        ARG(0) = 0x76; ARG(1) = 0xC; c->r[31] = 0x08908B04u; f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) { *B(e) = 1; *B(e + 1) = 0; *W(e + 88) = o; *W(e + 100) = 7; *W(e + 108) = 0x63; }
    }
    call1(c, o, f_089088E4, 0x08908B34u);
    leave(c, sv);
}

/* (fighter): without a frame flag 0x08923864 and, once at or below the floor, it lands (+7 on, +20 /
   +73 / +262 cleared); ticked. */
SFA_FUNC(089211B4, move_air_land_c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        call1(c, o, f_08923864, 0x089211D4u);
        int32_t floor = S16(o + 132);
        if (!((int32_t)((uint32_t)floor & 0xFFFF) < (int32_t)((uint32_t)S16(o + 22) & 0xFFFF))) {
            uint32_t st = *B(o + 7);
            *H(o + 22) = (uint16_t)floor;
            *B(o + 7) = (uint8_t)(st + 1);
            *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 262) = 0;
        }
    }
    call1(c, o, f_08861848, 0x08921214u);
    leave(c, sv);
}

/* (fighter): ticked, 0x0893E0B0; when the animation ends +7 steps on, +82 = 1, and on the ground (+73
   clear) +82 = 6, +262 cleared and standing (move 0). */
SFA_FUNC(0893DF38, move_tick_end_stand) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0893DF4Cu);
    call1(c, o, f_0893E0B0, 0x0893DF54u);
    if (S8(o + 75) < 0) {
        bump(o + 7);
        *B(o + 0x52) = 1;
        if (S8(o + 73) == 0) {
            *B(o + 0x52) = 6; *B(o + 262) = 0;
            call2(c, o, 0, f_0886163C, 0x0893DF98u);
        }
    }
    leave(c, sv);
}

/* (fighter): move 2k or 2k + 1 (k its +194, picked with its cancel data): the second when +508 is past
   16k + 32 (or, for choice 1 of +386 without +373, with +290 bit 0). */
SFA_FUNC(08942A68, move_pick_by_charge) {
    gaddr o = ARG(0);
    uint32_t ra;
    int32_t k = S8(o + 194);
    int32_t lock = S8(o + 373);
    frame_in(c, &ra);
    uint32_t n = (uint32_t)(k + k);
    int by_charge = 1;
    if (lock == 0 && S8(o + 386) == 1) {
        by_charge = 0;
        if (*H(o + 290) & 1) n += 1;
    }
    if (by_charge && ((k << 4) + 0x20) < S16(o + 508)) n += 1;
    call2(c, o, n & 0xFFFF, f_08861674, 0x08942ACCu);
    frame_out(c, ra);
}

/* (fighter): ticked; a frame flag starts a slide: step 2, x 6.0 with a brake by strength, +82 from
   0x08A7632C. */
SFA_FUNC(08944E38, move_tick_flag_slide) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08944E4Cu);
    if (S8(o + 77) != 0) {
        int32_t k = S8(o + 194);
        *B(o + 7) = 2; *B(o + 77) = 0;
        *W(o + 96) = 0x60000;
        *W(o + 104) = (uint32_t)(k << 11) + 0xFFFFC000u;
        *H(o + 82) = *B(0x08A7632Cu + (uint32_t)k);
    }
    leave(c, sv);
}

/* (fighter): the +82 timer runs down; at 0 +7 steps on, +237 cleared, +82 = 6, +832 = 48 and +96 = 4.5
   toward the facing; ticked. */
SFA_FUNC(0896506C, move_timer_dash) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    frame_in(c, &ra);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) {
        bump(o + 7); *B(o + 237) = 0; *H(o + 82) = 6; *W(o + 832) = 0x30;
        *W(o + 96) = *B(o + 11) == 0 ? 0xFFFB8000u : 0x48000;
    }
    call1(c, o, f_08861848, 0x089650D0u);
    frame_out(c, ra);
}

/* (fighter): +543 = 2; ticked; without a frame flag step 3, +237 cleared and +573 from 0x08A8A9F0. */
SFA_FUNC(089654E0, move_tick_573_i) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 543) = 2;
    call1(c, o, f_08861848, 0x08965500u);
    if (S8(o + 77) == 0) {
        *B(o + 7) = 3; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x0896551Cu);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08A8A9F0u + i);
    }
    leave(c, sv);
}
