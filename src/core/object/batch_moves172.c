/* Move steps of 35 instructions: the guard stance start by held direction (0x08861EDC), slides in the
   air until they land, an effect on a frame flag, a pull toward the opponent (0x08937424) and landings
   that stand or step on. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_08862068(Cpu *c); void f_088EF7F8(Cpu *c); void f_088EF6D4(Cpu *c); void f_0887C614(Cpu *c);
void f_08822EAC(Cpu *c); void f_089376E4(Cpu *c); void f_08961428(Cpu *c); void f_08963984(Cpu *c);
void f_08963840(Cpu *c); void f_08963F34(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): guarding: step 1 / 0 / 3 / 0, +266 / +696 cleared, +543 = 5, 0x08862068, the guard
   (+78) from the held direction (+752 bit 1: 1, bit 0: -1, else 0) and move 0x10. */
SFA_FUNC(08861EDC, move_guard_start) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3; *B(o + 7) = 0;
    *B(o + 266) = 0; *B(o + 696) = 0; *B(o + 543) = 5;
    call1(c, o, f_08862068, 0x08861F1Cu);
    uint32_t h = *H(o + 752);
    *B(o + 78) = (h & 2) ? 1 : (h & 1) ? 0xFF : 0;
    call2(c, o, 0x10, f_0886163C, 0x08861F58u);
    leave(c, sv);
}

/* (fighter): `check`; on the floor it lands (`land`); in the air it slides by +96 toward its facing
   (+96 by +104, stepping +7 on once that turns) and ticks. */
static void air_slide(Cpu *c, void (*check)(Cpu *), void (*land)(Cpu *), uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, check, b0);
    if (!(S16(o + 132) < S16(o + 22))) call1(c, o, land, b1);
    else {
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + toward(o, v);
        uint32_t v2 = v + *W(o + 104);
        *W(o + 96) = v2;
        if ((int32_t)v2 < 0) bump(o + 7);
        call1(c, o, f_08861848, b2);
    }
    leave(c, sv);
}
SFA_FUNC(088EF5F0, move_air_slide) { air_slide(c, f_088EF7F8, f_088EF6D4, 0x088EF604u, 0x088EF620u, 0x088EF66Cu); }
SFA_FUNC(0896375C, move_air_slide_b) { air_slide(c, f_08963984, f_08963840, 0x08963770u, 0x0896378Cu, 0x089637D8u); }

/* (fighter): a frame flag lands it (+7 on, +73 cleared, cue 0x22 / 0x23); otherwise it slides by +96
   toward its facing (stopping once +96 is negative); ticked. */
SFA_FUNC(08902B0C, move_flag_land_or_slide) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0887C614, 0x08902B3Cu);
    } else {
        uint32_t v = *W(o + 96);
        if ((int32_t)v < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
        uint32_t d = *B(o + 11) == 0 ? 0u - v : v;
        uint32_t cur = *W(o + 96);
        *W(o + 16) = *W(o + 16) + d;
        *W(o + 96) = cur + *W(o + 104);
    }
    call1(c, o, f_08861848, 0x08902B88u);
    leave(c, sv);
}

/* (fighter): ended: standing (with +1 = 1); else a frame flag spawns effect 0x3A / 4 tied to it, and
   ticked. */
SFA_FUNC(08909028, move_end_or_effect_3a) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 1) = 1;
        call2(c, o, 0, f_0886163C, 0x08909064u);
    } else {
        if (S8(o + 77) != 0) {
            *B(o + 77) = 0;
            ARG(0) = 0x3A; ARG(1) = 4; c->r[31] = 0x08909088u; f_08822EAC(c);
            gaddr e = c->r[2];
            if (e != 0) { *B(e + 1) = 0; *W(e + 88) = o; }
        }
        call1(c, o, f_08861848, 0x089090A4u);
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag starts a jump: +7 on, +73 = 1, +123 / +82 / +104 cleared, fall -0.375,
   x and y by strength (0x08A6DBFC). */
SFA_FUNC(08931BEC, move_tick_flag_jump6) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931C00u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1; *W(o + 104) = 0; *B(o + 123) = 0;
        int32_t k = S8(o + 194);
        *H(o + 82) = 0;
        gaddr e = 0x08A6DBFCu + ((uint32_t)k << 3);
        *W(o + 108) = 0xFFFFA000u;
        *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    }
    leave(c, sv);
}

/* (fighter): the +83 wait runs down; past 0 0x089376E4, +196 counts and the pull record (+0x3A0: +8 the
   x distance to the opponent / 16, +12 / +16 cleared, +20 = 0x8000) is set; ticked. */
SFA_FUNC(08937424, move_wait_pull) {
    gaddr o = ARG(0); uint32_t sv[2];
    gaddr w = o + 0x53;
    int8_t t = (int8_t)(S8(w) - 1);
    enter(c, o, sv);
    *B(w) = (uint8_t)t;
    if (t < 0) {
        call1(c, o, f_089376E4, 0x08937460u);
        gaddr p = o + 0x3A0;
        *B(o + 196) = (uint8_t)(S8(o + 196) + 1);
        *W(p + 12) = 0; *W(p + 20) = 0x8000; *W(p + 16) = 0;
        int32_t dx = (int32_t)(*W(o + 16) - *W(*W(o + 92) + 16));
        *W(p + 8) = (uint32_t)(dx >> 4);
    }
    call1(c, o, f_08861848, 0x089374A0u);
    leave(c, sv);
}

/* (fighter): 0x08961428; when negative +7 steps on with a fall of -0.34 and move 0x37 + 2 x strength;
   else +82 counts up, keeping bit 0, and it ticks on the even frames. */
SFA_FUNC(0895EEF0, move_check_37_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08961428, 0x0895EF04u);
    if ((int32_t)c->r[2] < 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 108) = 0xFFFFA800u;
        call2(c, o, (uint32_t)((k << 1) + 0x37) & 0xFFFF, f_088616EC, 0x0895EF38u);
    } else {
        *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
        uint32_t b = (uint32_t)S16(o + 82) & 1;
        *H(o + 82) = (uint16_t)b;
        if (b == 0) call1(c, o, f_08861848, 0x0895EF6Cu);
    }
    leave(c, sv);
}

/* (fighter): 0x08963F34; when it lands, cue 0x22 / 0x23, at the floor, +73 / +20 cleared, and unlocked
   +7 steps on with move 0x11, locked (+373) standing; else ticked. */
SFA_FUNC(08963E60, move_land_lock_11) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08963F34, 0x08963E74u);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x08963E84u);
    else {
        call1(c, o, f_0887C614, 0x08963E94u);
        int32_t y = S16(o + 132);
        *H(o + 22) = (uint16_t)y; *B(o + 73) = 0;
        int32_t lock = S8(o + 373);
        *H(o + 20) = 0;
        if (lock == 0) { bump(o + 7); call2(c, o, 0x11, f_0886163C, 0x08963ECCu); }
        else call1(c, o, f_0886469C, 0x08963EDCu);
    }
    leave(c, sv);
}
