/* Steps of the characters that use the part animation (0x088D3EC0 ticks it), 9 - 24 instructions: the
   step moved on by frame flags (+76 / +77) with voices, speeds and helpers, and back to standing when the
   animation ends. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D3EC0(Cpu *c); void f_088D2C8C(Cpu *c); void f_088D2E04(Cpu *c); void f_088D3140(Cpu *c);
void f_088D4094(Cpu *c); void f_0886163C(Cpu *c); void f_088D407C(Cpu *c); void f_088D2DDC(Cpu *c);
void f_088D3848(Cpu *c); void f_089CB650(Cpu *c); void f_088616EC(Cpu *c); void f_088D31EC(Cpu *c);
void f_089CA064(Cpu *c); void f_088D2E54(Cpu *c);

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

/* The part animation ticked (s0 kept), then `post` looks at the character. */
static void ptick_then(Cpu *c, void (*post)(Cpu *, gaddr), uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, back);
    post(c, o);
    leave(c, sv);
}

/* (character): +7 steps on and the part ticked. */
SFA_FUNC(089B4480, part_step_tick) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    bump(o + 7);
    call1(c, o, f_088D3EC0, 0x089B4498u);
    frame_out(c, ra);
}

/* (character): +7 steps on, state voice 2 (0x088D2C8C) and the part ticked. */
SFA_FUNC(089B84E4, part_step_voice_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    bump(o + 7);
    call2(c, o, 2, f_088D2C8C, 0x089B8508u);
    call1(c, o, f_088D3EC0, 0x089B8510u);
    leave(c, sv);
}

/* The voices of frame flags 2 - 4 (bank cues 9 / 0x13 / 0x15), flag 5 clearing +262; the flag taken. */
static void post_flag_voices(Cpu *c, gaddr o) {
    int32_t k = S8(o + 77);
    if (k == 2) { call2(c, o, 9, f_088D2E04, 0x089B5F70u); call2(c, o, 0x13, f_088D2E04, 0x089B5F7Cu); }
    else if (k == 3) call2(c, o, 0x13, f_088D2E04, 0x089B5F7Cu);
    else if (k == 4) { bump(o + 7); call2(c, o, 9, f_088D2E04, 0x089B5F9Cu); call2(c, o, 0x15, f_088D2E04, 0x089B5FA8u); }
    else if (k == 5) *B(o + 262) = 0;
    *B(o + 77) = 0;
}
SFA_FUNC(089B5F08, part_tick_flag_voices) { ptick_then(c, post_flag_voices, 0x089B5F1Cu); }

static void post_3140_flag1(Cpu *c, gaddr o) {
    call1(c, o, f_088D3140, 0x089B5EDCu);
    if (S8(o + 77) == 1) bump(o + 7);
}
SFA_FUNC(089B5EC0, part_tick_3140_flag1) { ptick_then(c, post_3140_flag1, 0x089B5ED4u); }

static void post_76_4094(Cpu *c, gaddr o) {
    if (S8(o + 76) == 0) { *B(o + 237) = 0; bump(o + 7); call1(c, o, f_088D4094, 0x089B697Cu); }
}
SFA_FUNC(089B6944, part_tick_76_4094) { ptick_then(c, post_76_4094, 0x089B6958u); }

static void post_76_speed12(Cpu *c, gaddr o) {
    if (S8(o + 76) == 1) { *B(o + 76) = 0; bump(o + 7); *W(o + 96) = 0x120000; *W(o + 104) = 0; }
}
SFA_FUNC(089B6A10, part_tick_76_speed) { ptick_then(c, post_76_speed12, 0x089B6A24u); }

static void post_76_407c(Cpu *c, gaddr o) {
    if (S8(o + 76) == 0) {
        *B(o + 237) = 0; bump(o + 7);
        call1(c, o, f_088D407C, 0x089C8E1Cu);
        *B(o + 194) = (uint8_t)*W(o + 840);
    }
}
SFA_FUNC(089C8DE4, part_tick_76_407c) { ptick_then(c, post_76_407c, 0x089C8DF8u); }

static void post_3140_76_stop(Cpu *c, gaddr o) {
    call1(c, o, f_088D3140, 0x089B6A7Cu);
    if (S8(o + 76) == 1) { *B(o + 76) = 0; bump(o + 7); *W(o + 96) = 0; *W(o + 104) = 0; }
}
SFA_FUNC(089B6A60, part_tick_3140_76_stop) { ptick_then(c, post_3140_76_stop, 0x089B6A74u); }

static void post_77_hold_opp(Cpu *c, gaddr o) {
    if (S8(o + 77) == 0) {
        gaddr opp = *W(o + 92);
        bump(o + 7); *B(o + 237) = 0;
        *B(opp + 127) = 2; *B(o + 573) = 6;
    }
}
SFA_FUNC(089BBDE0, part_tick_77_hold) { ptick_then(c, post_77_hold_opp, 0x089BBDF4u); }

static void post_77_4094_speed_a(Cpu *c, gaddr o) {
    if (S8(o + 77) == 0) {
        *B(o + 237) = 0; bump(o + 7);
        call1(c, o, f_088D4094, 0x089C1FC0u);
        *W(o + 96) = 0x140000; *W(o + 104) = 0;
    }
}
static void post_77_4094_speed_b(Cpu *c, gaddr o) {
    if (S8(o + 77) == 0) {
        *B(o + 237) = 0; bump(o + 7);
        call1(c, o, f_088D4094, 0x089C226Cu);
        *W(o + 96) = 0x140000; *W(o + 104) = 0;
    }
}
SFA_FUNC(089C1F88, part_tick_77_4094) { ptick_then(c, post_77_4094_speed_a, 0x089C1F9Cu); }
SFA_FUNC(089C2234, part_tick_77_4094_b) { ptick_then(c, post_77_4094_speed_b, 0x089C2248u); }

/* On a frame flag +7 steps on, +73 = 1, 0x088D2DDC and 0x088D3848. */
static void flag_2ddc(Cpu *c, gaddr o, uint32_t b1, uint32_t b2) {
    if (S8(o + 77) != 0) { bump(o + 7); *B(o + 73) = 1; call1(c, o, f_088D2DDC, b1); call1(c, o, f_088D3848, b2); }
}
static void post_2ddc_a(Cpu *c, gaddr o) { flag_2ddc(c, o, 0x089C805Cu, 0x089C8064u); }
static void post_2ddc_b(Cpu *c, gaddr o) { flag_2ddc(c, o, 0x089C8C10u, 0x089C8C18u); }
static void post_2ddc_c(Cpu *c, gaddr o) { flag_2ddc(c, o, 0x089C9458u, 0x089C9460u); }
SFA_FUNC(089C8020, part_tick_flag_2ddc) { ptick_then(c, post_2ddc_a, 0x089C8034u); }
SFA_FUNC(089C8BD4, part_tick_flag_2ddc_b) { ptick_then(c, post_2ddc_b, 0x089C8BE8u); }
SFA_FUNC(089C941C, part_tick_flag_2ddc_c) { ptick_then(c, post_2ddc_c, 0x089C9430u); }

static void post_77_b650(Cpu *c, gaddr o) {
    if (S8(o + 77) != 0) {
        bump(o + 7);
        call2(c, o, 0, f_089CB650, 0x089C7A10u);
        *B(o + 536) = (uint8_t)(*B(o + 536) + 1);
        *H(o + 82) = 4;
    }
}
SFA_FUNC(089C79D8, part_tick_77_b650) { ptick_then(c, post_77_b650, 0x089C79ECu); }

static void post_31ec_fall_2a(Cpu *c, gaddr o) {
    call1(c, o, f_088D31EC, 0x089C8AC4u);
    call1(c, o, f_089CA064, 0x089C8ACCu);
    if ((int32_t)*W(o + 100) < 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        call2(c, o, (uint32_t)(k + 0x2A), f_088616EC, 0x089C8AF4u);
    }
}
SFA_FUNC(089C8AA8, part_tick_fall_2a) { ptick_then(c, post_31ec_fall_2a, 0x089C8ABCu); }

static void post_77_voices(Cpu *c, gaddr o) {
    if (S8(o + 77) == 0) {
        bump(o + 7);
        call2(c, o, 0x3E, f_088D2E54, 0x089B5E98u);
        call2(c, o, 0x1F, f_088D2E04, 0x089B5EA4u);
        call2(c, o, 0x13, f_088D2E04, 0x089B5EB0u);
    }
}
SFA_FUNC(089B5E60, part_tick_77_voices) { ptick_then(c, post_77_voices, 0x089B5E74u); }

/* (character): when the animation ends back to standing (step 1 / 0 / 0 / 0, +372 cleared, move 0, and
   with `all` also +3 / +820 / +824 cleared); else the part ticked. */
static void end_or_ptick(Cpu *c, int all, uint32_t back_stand, uint32_t back_tick) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 372) = 0;
        if (all) { *B(o + 3) = 0; *W(o + 820) = 0; *W(o + 824) = 0; }
        call2(c, o, 0, f_0886163C, back_stand);
    } else call1(c, o, f_088D3EC0, back_tick);
    frame_out(c, ra);
}
SFA_FUNC(089B44A4, part_end_or_tick) { end_or_ptick(c, 0, 0x089B44D4u, 0x089B44E8u); }
SFA_FUNC(089B8520, part_end_or_tick_b) { end_or_ptick(c, 0, 0x089B8550u, 0x089B8564u); }
SFA_FUNC(089C62C4, part_end_or_tick_c) { end_or_ptick(c, 0, 0x089C62F4u, 0x089C6308u); }
SFA_FUNC(089BF140, part_end_or_tick_all) { end_or_ptick(c, 1, 0x089BF17Cu, 0x089BF190u); }

/* (character): +828 counts down; without a frame flag the part ticks, with one, once the count is out,
   +7 steps on and +74 = 1. */
SFA_FUNC(089C6268, part_count_828) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t v = *W(o + 828) - 1;
    int32_t k = S8(o + 77);
    frame_in(c, &ra);
    *W(o + 828) = v;
    if (k == 0) call1(c, o, f_088D3EC0, 0x089C62B8u);
    else if ((int32_t)*W(o + 828) < 0) { bump(o + 7); *B(o + 74) = 1; }
    frame_out(c, ra);
}
