/* Move steps of 12 - 30 instructions: the move start for a fresh round (0x0886469C), jumps with four
   speeds by strength, landings with a cue, timed charges and slides. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EC0(Cpu *c);
void f_08864494(Cpu *c); void f_088646CC(Cpu *c); void f_08911550(Cpu *c); void f_08911058(Cpu *c);
void f_08929118(Cpu *c); void f_0887C614(Cpu *c); void f_0892EA9C(Cpu *c); void f_0893F1D0(Cpu *c);
void f_0896A34C(Cpu *c); void f_0886E294(Cpu *c); void f_08862004(Cpu *c); void f_088D35D0(Cpu *c);
void f_088D2DBC(Cpu *c); void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c); void f_088D4094(Cpu *c);
void f_088D3180(Cpu *c); void f_088D35C8(Cpu *c); void f_08908C04(Cpu *c); void f_08908BA8(Cpu *c);

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

/* (fighter): 0x08864494, then move 0 started with the hit state cleared (0x088646CC). */
SFA_FUNC(0886469C, move_round_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08864494, 0x088646B0u);
    call2(c, o, 0, f_088646CC, 0x088646BCu);
    leave(c, sv);
}

/* (fighter): 0x08911550; the +82 charge timer runs down; when out, or with +508 below 32, released
   (0x08911058); else ticked. */
SFA_FUNC(08910D94, move_charge_release) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08911550, 0x08910DA8u);
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) call1(c, o, f_08911058, 0x08910DCCu);
    else if (S16(o + 508) < 32) call1(c, o, f_08911058, 0x08910DE8u);
    else call1(c, o, f_08861848, 0x08910DF8u);
    leave(c, sv);
}

/* (fighter): 0x08929118; when it lands +7 steps on at the floor, +20 / +73 / +262 cleared, cue 0x22 / 0x23
   and ticked; else ticked until a frame flag. */
SFA_FUNC(08927004, move_land_cue_tick_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08929118, 0x08927018u);
    if (c->r[2] == 0) {
        int32_t y = S16(o + 132);
        bump(o + 7);
        *H(o + 22) = (uint16_t)y; *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 262) = 0;
        call1(c, o, f_0887C614, 0x08927060u);
        call1(c, o, f_08861848, 0x08927068u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08927034u);
    leave(c, sv);
}

/* (fighter): 0x0892EA9C; on reaching its target x (+16 = +108) step 3, +82 = 72 and move 0x23 (0x36 when
   the opponent is in state 11); else ticked. */
SFA_FUNC(08929D48, move_reach_target) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0892EA9C, 0x08929D5Cu);
    if (*W(o + 16) == *W(o + 108)) {
        *B(o + 7) = 3;
        gaddr opp = *W(o + 92);
        *H(o + 82) = 0x48;
        call2(c, o, S8(opp + 338) == 0xB ? 0x36 : 0x23, f_0886163C, 0x08929DACu);
    } else call1(c, o, f_08861848, 0x08929D74u);
    leave(c, sv);
}

/* (fighter): while +96 holds it flies (+20 by +100 by +108, +16 by +96 toward the facing, +96 by +104);
   once it turns, step 6; ticked. */
SFA_FUNC(0892C440, move_fly_until_turn) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t v = *W(o + 96);
    frame_in(c, &ra);
    if ((int32_t)v < 0) *B(o + 7) = 6;
    else {
        uint32_t vy = *W(o + 100);
        *W(o + 20) = *W(o + 20) + vy;
        *W(o + 100) = vy + *W(o + 108);
        uint32_t d = *B(o + 11) == 0 ? 0u - v : v;
        *W(o + 16) = *W(o + 16) + d;
        *W(o + 96) = v + *W(o + 104);
    }
    call1(c, o, f_08861848, 0x0892C4A8u);
    frame_out(c, ra);
}

/* (fighter): ticked; a frame flag starts a jump: +7 on, x 3.5, y by strength (0x08A6DB60), fall -0.375. */
SFA_FUNC(089310F4, move_tick_flag_jump3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931108u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 96) = 0x38000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFA000u;
        *W(o + 100) = *W(by_strength(0x08A6DB60u, o, 4));
    }
    leave(c, sv);
}

/* (fighter): ticked, 0x0893F1D0; at or below the floor it lands: +7 on, +20 / +73 cleared, cue 0x22 /
   0x23, move 0x11. */
SFA_FUNC(0893F22C, move_tick_land_11) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0893F240u);
    call1(c, o, f_0893F1D0, 0x0893F248u);
    int32_t floor = S16(o + 132);
    if (!((int32_t)((uint32_t)floor & 0xFFFF) < (int32_t)((uint32_t)S16(o + 22) & 0xFFFF))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x0893F284u);
        call2(c, o, 0x11, f_0886163C, 0x0893F290u);
    }
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, +73 = 1 and the four speeds by strength (16 bytes each;
   order x, y, x brake, y fall unless `xy_swapped`); ticked. */
static void tick_77_speeds(Cpu *c, gaddr tab, int xy_swapped, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        bump(o + 7);
        gaddr e = by_strength(tab, o, 16);
        *B(o + 73) = 1;
        if (!xy_swapped) {
            *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4); *W(o + 104) = *W(e + 8); *W(o + 108) = *W(e + 12);
        } else {
            *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
        }
    }
    call1(c, o, f_08861848, back);
    frame_out(c, ra);
}
SFA_FUNC(0895D930, move_tick_77_speeds_a) { tick_77_speeds(c, 0x08A85A70u, 0, 0x0895D998u); }
SFA_FUNC(0898FAD4, move_tick_77_speeds_b) { tick_77_speeds(c, 0x08A96664u, 0, 0x0898FB3Cu); }
SFA_FUNC(0899C06C, move_tick_77_speeds_c) { tick_77_speeds(c, 0x08A9AE5Cu, 1, 0x0899C0D4u); }

/* (fighter): without a frame flag +7 steps on, +73 = 1, +83 = 5, +123 cleared and a dash by strength
   (0x08A85B9C) with the other speeds cleared; ticked. */
SFA_FUNC(0895E94C, move_tick_77_dash2) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        bump(o + 7); *B(o + 73) = 1; *B(o + 0x53) = 5; *B(o + 123) = 0;
        uint32_t v = *W(by_strength(0x08A85B9Cu, o, 4));
        *W(o + 104) = 0; *W(o + 96) = v; *W(o + 100) = 0; *W(o + 108) = 0;
    }
    call1(c, o, f_08861848, 0x0895E9B4u);
    frame_out(c, ra);
}

/* (fighter): +7 steps on, +82 = 60, facing +697; in battle setting 3 (+56) its +692 of 2 / 3 moves to 4 /
   5; move 0. */
SFA_FUNC(08961188, move_intro_wait) {
    gaddr o = ARG(0);
    uint32_t ra;
    bump(o + 7);
    *B(o + 0x52) = 0x3C;
    *B(o + 11) = *B(o + 697);
    int32_t setting = S16(BATTLE + 56);
    frame_in(c, &ra);
    if (setting == 3) {
        int32_t m = S8(o + 692);
        if (m == 2 || m == 3) *B(o + 692) = (uint8_t)(m + 2);
    }
    call2(c, o, 0, f_0886163C, 0x089611F0u);
    frame_out(c, ra);
}

/* (fighter): 0x0896A34C; once moving back, +7 steps on, x cleared and a fall (y -4.0 / -0.29), ticked;
   else ticked until a frame flag. */
SFA_FUNC(08968B70, move_back_fall) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896A34C, 0x08968B84u);
    if ((int32_t)*W(o + 96) < 0) {
        uint32_t st = *B(o + 7);
        *W(o + 96) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 104) = 0; *W(o + 100) = 0xFFFC0000u; *W(o + 108) = 0xFFFFB400u;
        call1(c, o, f_08861848, 0x08968BD4u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08968BA4u);
    leave(c, sv);
}

/* (fighter): ticked; unless frame byte +76 is negative +237 cleared, +7 steps on, 0x0886E294 and +573
   from 0x08A9AEA8 by its +194 (6 with +81). */
SFA_FUNC(0899C2C8, move_tick_573_n) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0899C2DCu);
    if (S8(o + 76) >= 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x0899C300u);
        *B(o + 573) = S8(o + 81) != 0 ? 6 : *B(0x08A9AEA8u + (uint32_t)S8(o + 194));
    }
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing +7 steps on at the floor, +73 / +76 / +77 cleared (step 4 for
   strength 0); ticked without frame byte +76. */
SFA_FUNC(089A3CC8, move_rise_land_76) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A3CDCu);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7);
        *W(o + 20) = (uint32_t)y << 16;
        *B(o + 73) = 0; *B(o + 76) = 0;
        int32_t k = S8(o + 194);
        *B(o + 77) = 0;
        if (k == 0) *B(o + 7) = 4;
    }
    if (S8(o + 76) == 0) call1(c, o, f_08861848, 0x089A3D2Cu);
    leave(c, sv);
}

/* (character): the part ticked, 0x08862004; when 0x088D35D0 gives 1, step 13, +73 cleared, move 0x15,
   cue 0x22, 0x088D36D0 and 0x088D3960 (0, 0). */
SFA_FUNC(089BA6F0, part_air_teleport) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BA704u);
    call1(c, o, f_08862004, 0x089BA70Cu);
    call1(c, o, f_088D35D0, 0x089BA714u);
    if (c->r[2] == 1) {
        *B(o + 7) = 0xD; *B(o + 73) = 0;
        call2(c, o, 0x15, f_088616EC, 0x089BA734u);
        call1(c, o, f_088D2DBC, 0x089BA73Cu);
        call1(c, o, f_088D36D0, 0x089BA744u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089BA754u; f_088D3960(c);
    }
    leave(c, sv);
}

/* (character): the part ticked; without frame byte +76 +237 cleared, +7 steps on, 0x088D4094, x 4.0,
   0x088D3180, +828 = 3 + its +194 and 0x088D35C8 (16). */
SFA_FUNC(089C8930, part_tick_76_charge) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C8944u);
    if (S8(o + 76) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_088D4094, 0x089C8968u);
        *W(o + 96) = 0x40000; *W(o + 104) = 0;
        call1(c, o, f_088D3180, 0x089C897Cu);
        *W(o + 828) = (uint32_t)(S8(o + 194) + 3);
        call2(c, o, 0x10, f_088D35C8, 0x089C8994u);
    }
    leave(c, sv);
}

/* (fighter): flies (+16 by +96, +20 by +100 by +108); once falling +7 steps on, +820 = its +194 and move
   9 + its +194; else ticked. */
SFA_FUNC(088EFA64, move_fly_then_9) {
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
        call2(c, o, (uint32_t)(k + 9) & 0xFFFF, f_088616EC, 0x088EFAD0u);
    } else call1(c, o, f_08861848, 0x088EFAA4u);
    frame_out(c, ra);
}

/* (fighter): 0x08908C04; at or below the floor it lands (+7 on, +262 / +73 / +20 cleared, cue 0x22 /
   0x23); ticked until a frame flag. */
SFA_FUNC(08906C7C, move_air_land_cue) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08908C04, 0x08906C90u);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x08906CD0u);
    }
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08906CE4u);
    leave(c, sv);
}

/* (fighter): with a frame flag (3: x speeds by strength from 0x08A53ADC) and 0x08908BA8 negative the x
   speeds stop; ticked. */
SFA_FUNC(08907700, move_flag_speed_check) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        if (k == 3) {
            gaddr e = by_strength(0x08A53ADCu, o, 4);
            *W(o + 96) = *W(e); *W(o + 104) = *W(e + 12);
        }
        call1(c, o, f_08908BA8, 0x08907750u);
        if ((int32_t)c->r[2] < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
    }
    call1(c, o, f_08861848, 0x08907768u);
    leave(c, sv);
}
