/* Move steps of 26 - 27 instructions: landings, jumps with speeds by strength, the standing step's
   start (0x08859E14), effects spawned on a frame flag and recoveries. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EC0(Cpu *c);
void f_08937744(Cpu *c); void f_089376E4(Cpu *c); void f_0896A34C(Cpu *c); void f_08999370(Cpu *c);
void f_08862004(Cpu *c); void f_088D4094(Cpu *c); void f_08822EAC(Cpu *c); void f_08861DE4(Cpu *c);
void f_088EFBF4(Cpu *c); void f_0887C614(Cpu *c); void f_088F3FA4(Cpu *c); void f_088F8AF8(Cpu *c);
void f_0886E294(Cpu *c); void f_08908BD8(Cpu *c); void f_08908B7C(Cpu *c);

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
static int below_floor(gaddr o) { return S16(o + 22) < S16(o + 132); }

/* (fighter): 0x08937744, 0x089376E4; below the floor it lands (+196 counted, height to the floor, +74 = 1,
   +73 / +20 cleared); ticked. */
SFA_FUNC(089374B0, move_air_land_196) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08937744, 0x089374C4u);
    call1(c, o, f_089376E4, 0x089374CCu);
    if (below_floor(o)) {
        int32_t y = S16(o + 132);
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1);
        *H(o + 22) = (uint16_t)y; *B(o + 74) = 1; *B(o + 73) = 0; *H(o + 20) = 0;
    }
    call1(c, o, f_08861848, 0x08937508u);
    leave(c, sv);
}

/* (fighter): ticked; on a frame flag +7 steps on, +73 = 1 and the rise speed by strength (`tab`, +108 =
   -0.34). */
static void tick_flag_rise(Cpu *c, gaddr tab, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, back);
    if (S8(o + 77) != 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        *B(o + 73) = 1;
        *W(o + 100) = *W(tab + ((uint32_t)k << 2));
        *W(o + 108) = 0xFFFFA800u;
    }
    leave(c, sv);
}
SFA_FUNC(0895EDEC, move_tick_flag_rise) { tick_flag_rise(c, 0x08A85BD8u, 0x0895EE00u); }
SFA_FUNC(089914DC, move_tick_flag_rise_b) { tick_flag_rise(c, 0x08A967FCu, 0x089914F0u); }

/* (fighter): once the animation has ended +820 counts down; when bit 7 turns on +7 steps on with move 8;
   otherwise it moves on by +96 and ticks. */
SFA_FUNC(08963DF8, move_slide_count_8_b) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        uint32_t v = *W(o + 820) - 1;
        *W(o + 820) = v;
        if (v & 0x80) { bump(o + 7); call2(c, o, 8, f_088616EC, 0x08963E38u); frame_out(c, ra); return; }
    }
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    call1(c, o, f_08861848, 0x08963E54u);
    frame_out(c, ra);
}

/* (fighter): 0x0896A34C; below the floor it lands (+7 steps on, +20 / +262 / +73 cleared); ticked until
   a frame flag. */
SFA_FUNC(08968C18, move_air_land_flag) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896A34C, 0x08968C2Cu);
    if (below_floor(o)) {
        int32_t y = S16(o + 132);
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)y;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 262) = 0; *B(o + 73) = 0;
    }
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08968C70u);
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, +73 = 1, +82 = 5, +123 / +104 cleared and +96 by strength
   (0x08A966F0); ticked. */
SFA_FUNC(0898FF14, move_tick_77_dash) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        bump(o + 7);
        int32_t k = S8(o + 194);
        *B(o + 73) = 1; *H(o + 82) = 5; *B(o + 123) = 0;
        uint32_t v = *W(0x08A966F0u + ((uint32_t)k << 2));
        *W(o + 104) = 0; *W(o + 96) = v;
    }
    call1(c, o, f_08861848, 0x0898FF70u);
    frame_out(c, ra);
}

/* (fighter): 0x08999370; above the floor ticked; else step 4 at the floor, +73 / +262 cleared and move
   0x2C. */
SFA_FUNC(08996BA0, move_land_2c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08999370, 0x08996BB4u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x08996BD0u);
    else {
        *B(o + 7) = 4; *W(o + 20) = (uint32_t)floor << 16; *B(o + 73) = 0; *B(o + 262) = 0;
        call2(c, o, 0x2C, f_088616EC, 0x08996BF8u);
    }
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing +7 steps on, +20 / +73 cleared, at the floor, ticked; else ticked
   until a frame flag. */
SFA_FUNC(089A7E60, move_rise_land_flag) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A7E74u);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7); *H(o + 20) = 0; *B(o + 73) = 0; *H(o + 22) = (uint16_t)y;
        call1(c, o, f_08861848, 0x089A7EA0u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089A7EB8u);
    leave(c, sv);
}

/* (character): the part ticked; without a frame flag +237 cleared, +7 steps on, a dash (+96 = 16.0 toward
   the facing, +104 cleared) and 0x088D4094. */
SFA_FUNC(089BBAF4, part_tick_dash) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BBB08u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 104) = 0;
        uint32_t f = *B(o + 11);
        *W(o + 96) = 0x100000;
        if (f == 0) *W(o + 96) = 0u - *W(o + 96);
        call1(c, o, f_088D4094, 0x089BBB4Cu);
    }
    leave(c, sv);
}

/* (fighter): standing begins: +7 steps on, +372 cleared, move 0 (0x30 in state 10; with +373 and +696
   +7 steps on again and move 1). */
SFA_FUNC(08859E14, move_stand_start) {
    gaddr o = ARG(0);
    uint32_t ra;
    bump(o + 7);
    int32_t k = S8(o + 373);
    frame_in(c, &ra);
    *B(o + 372) = 0;
    uint32_t n = 0;
    if (k != 0) { if (S8(o + 696) != 0) { bump(o + 7); n = 1; } }
    else if (S8(o + 338) == 0xA) n = 0x30;
    call2(c, o, n, f_0886163C, 0x08859E74u);
    frame_out(c, ra);
}

/* (fighter): in states 6 / 0x14 a frame flag spawns effect 0x3A / 5 tied to it (+88); ticked. */
SFA_FUNC(0885A1DC, move_flag_effect_3a) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int32_t s = S8(o + 338);
    if ((s == 6 || s == 0x14) && S8(o + 77) != 0) {
        *B(o + 77) = 0;
        ARG(0) = 0x3A; ARG(1) = 5; c->r[31] = 0x0885A220u; f_08822EAC(c);
        if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    }
    call1(c, o, f_08861848, 0x0885A238u);
    leave(c, sv);
}

/* (fighter): step 1 / 0 / 2 / 1, 0x08861DE4, move 2 with +752 bit 1, else 3. */
SFA_FUNC(08861D78, move_crouch_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 2; *B(o + 7) = 1;
    call1(c, o, f_08861DE4, 0x08861DA8u);
    if (*H(o + 752) & 2) call2(c, o, 2, f_0886163C, 0x08861DC4u);
    else call2(c, o, 3, f_0886163C, 0x08861DD4u);
    leave(c, sv);
}

/* (fighter): `check`; when it says landed, cue 0x22 / 0x23, at the floor, +73 cleared, +7 (or step 4)
   and move 0x11; else ticked. */
SFA_FUNC(088EFB40, move_land_11_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088EFBF4, 0x088EFB54u);
    if (c->r[2] != 0) {
        call1(c, o, f_0887C614, 0x088EFB74u);
        int32_t y = S16(o + 132);
        uint32_t st = *B(o + 7);
        *W(o + 20) = (uint32_t)y << 16; *B(o + 73) = 0;
        *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, 0x11, f_0886163C, 0x088EFB9Cu);
    } else call1(c, o, f_08861848, 0x088EFB64u);
    leave(c, sv);
}

static void land_4_11(Cpu *c, void (*check)(Cpu *), uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, check, b0);
    if ((int32_t)c->r[2] < 0) {
        int32_t y = S16(o + 132);
        *B(o + 7) = 4; *H(o + 22) = (uint16_t)y; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, b1);
        call2(c, o, 0x11, f_0886163C, b2);
    } else call1(c, o, f_08861848, b3);
    leave(c, sv);
}
SFA_FUNC(088F3B88, move_land_4_11) { land_4_11(c, f_088F3FA4, 0x088F3B9Cu, 0x088F3BC8u, 0x088F3BD4u, 0x088F3BE4u); }
SFA_FUNC(088F877C, move_land_4_11_b) { land_4_11(c, f_088F8AF8, 0x088F8790u, 0x088F87BCu, 0x088F87C8u, 0x088F87D8u); }

/* (fighter): ticked; without frame byte +76 +7 steps on, +237 cleared and +573 from 0x08A43B30 by its
   +194 (with +81: its entry 3). */
SFA_FUNC(088F0078, move_tick_573_d) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088F008Cu);
    if (S8(o + 76) == 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 81);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 237) = 0;
        *B(o + 573) = (uint8_t)(k != 0 ? S8(0x08A43B33u) : S8(0x08A43B30u + (uint32_t)S8(o + 194)));
    }
    leave(c, sv);
}

/* (fighter): ticked until a frame flag; then +7 steps on, the flag taken and +96 = 2.125 toward the
   facing. */
SFA_FUNC(088FE104, move_tick_flag_step) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x088FE120u);
    else {
        uint32_t f = *B(o + 11);
        bump(o + 7);
        *B(o + 77) = 0;
        *W(o + 96) = f != 0 ? 0x22000 : 0xFFFDE000u;
    }
    frame_out(c, ra);
}

/* (fighter): ticked; without a frame flag +237 cleared, +7 steps on and +573 from 0x08A53A5C by its +194
   (with +81: by 0x0886E294). */
SFA_FUNC(0890731C, move_tick_573_e) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08907330u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x08907354u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08A53A5Cu + i);
    }
    leave(c, sv);
}

/* (fighter): 0x08908BD8; above the floor 0x08908B7C; else +7 steps on, at the floor, +73 / +20 cleared
   and cue 0x22 / 0x23. */
SFA_FUNC(08907778, move_air_or_land_cue) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08908BD8, 0x0890778Cu);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08908B7C, 0x089077A8u);
    else {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x089077D4u);
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag starts a jump (+7 on, flag taken, +73 = 1, x -2.25 / +0.02, y 6.0 /
   -0.1875). */
SFA_FUNC(0893F0B4, move_tick_flag_jump) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0893F0C8u);
    if (S8(o + 77) != 0) {
        bump(o + 7); *B(o + 77) = 0; *B(o + 73) = 1;
        *W(o + 96) = 0xFFFDC000u; *W(o + 100) = 0x60000; *W(o + 104) = 0x500; *W(o + 108) = 0xFFFFD000u;
    }
    leave(c, sv);
}

/* (fighter): a frame flag steps +7 on, +73 = 1, +83 = 7 and +96 by strength (0x08A85A14), +104 =
   -0.1875; ticked. */
SFA_FUNC(0895D110, move_tick_flag_dash) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) != 0) {
        bump(o + 7); *B(o + 73) = 1; *B(o + 0x53) = 7;
        *W(o + 96) = *W(0x08A85A14u + ((uint32_t)S8(o + 194) << 2));
        *W(o + 104) = 0xFFFFD000u;
    }
    call1(c, o, f_08861848, 0x0895D170u);
    frame_out(c, ra);
}

/* (fighter): while the battle is paused (+36 / +28) ticked; else +7 steps on with the move for its +692
   (0x08A85ED0). */
SFA_FUNC(089611FC, move_after_pause) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(BATTLE + 36) != 0 || S8(BATTLE + 28) != 0) call1(c, o, f_08861848, 0x0896122Cu);
    else {
        int32_t k = S8(o + 692);
        bump(o + 7);
        call2(c, o, *H(0x08A85ED0u + 2 * (uint32_t)k), f_0886163C, 0x0896125Cu);
    }
    frame_out(c, ra);
}
