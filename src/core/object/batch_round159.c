/* More end-of-move steps, 16 - 24 instructions: standing again once the animation ends after clearing
   flags, releasing the opponent or a cue; a part character's dash steps (0x089C1FDC). Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_0886A530(Cpu *c); void f_0886DB2C(Cpu *c);
void f_08944018(Cpu *c); void f_08999408(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D3140(Cpu *c);
void f_08911550(Cpu *c); void f_0887C614(Cpu *c); void f_089A04D8(Cpu *c); void f_088D3A80(Cpu *c);
void f_089B7598(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static int ended(gaddr o) { return S8(o + 75) < 0; }

/* (fighter): a frame flag starts the hold (step 2, 0x08944018); else ticked. */
SFA_FUNC(08943FD8, move_flag_hold) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) != 0) { *B(o + 7) = 2; *B(o + 77) = 0; call1(c, o, f_08944018, 0x0894400Cu); }
    else call1(c, o, f_08861848, 0x08943FF4u);
    frame_out(c, ra);
}

/* (fighter): ended: the opponent released (with +262 cleared when `c262`) and standing; else 0x0886DB2C
   and ticked. */
static void end_release_db2c(Cpu *c, int c262, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) {
        gaddr opp = *W(o + 92);
        if (c262) *B(o + 262) = 0;
        *B(opp + 543) = 5;
        call1(c, o, f_0886469C, b1);
    } else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_08861848, b3); }
    leave(c, sv);
}
SFA_FUNC(0895E518, move_end_release_db2c_c) { end_release_db2c(c, 0, 0x0895E544u, 0x0895E554u, 0x0895E55Cu); }
SFA_FUNC(0896D540, move_end_release_db2c_d) { end_release_db2c(c, 0, 0x0896D56Cu, 0x0896D57Cu, 0x0896D584u); }
SFA_FUNC(08944870, move_end_release_262) { end_release_db2c(c, 1, 0x089448A0u, 0x089448B0u, 0x089448B8u); }
SFA_FUNC(08945264, move_end_release_262_b) { end_release_db2c(c, 1, 0x08945294u, 0x089452A4u, 0x089452ACu); }

/* (fighter): a frame flag runs 0x08999408; then ticked, or standing once ended. */
SFA_FUNC(089968F4, move_flag_helper_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08999408, 0x08996914u);
    if (!ended(o)) call1(c, o, f_08861848, 0x08996928u);
    else call1(c, o, f_0886469C, 0x08996938u);
    leave(c, sv);
}

/* (fighter): frame byte +76 = 1 clears +262; standing once ended, else ticked. */
SFA_FUNC(089A3B84, move_76_end_round) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 76) == 1) { *B(o + 76) = 0; *B(o + 262) = 0; }
    if (ended(o)) call1(c, o, f_0886469C, 0x089A3BB8u);
    else call1(c, o, f_08861848, 0x089A3BCCu);
    frame_out(c, ra);
}

/* (fighter): ended: standing; else a negative frame flag clears +262, 0x0886DB2C and ticked (then
   `extra`). */
static void end_or_flag262(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3, void (*extra)(Cpu *), uint32_t b4) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, b1);
    else {
        if (S8(o + 77) < 0) *B(o + 262) = 0;
        call1(c, o, f_0886DB2C, b2);
        call1(c, o, f_08861848, b3);
        if (extra) call1(c, o, extra, b4);
    }
    leave(c, sv);
}
SFA_FUNC(089A3DD4, move_end_or_flag262_b) { end_or_flag262(c, 0x089A3DF4u, 0x089A3E10u, 0x089A3E18u, 0, 0); }
SFA_FUNC(089A0350, move_end_or_flag262_mark) {
    end_or_flag262(c, 0x089A0370u, 0x089A038Cu, 0x089A0394u, f_089A04D8, 0x089A039Cu);
}

/* (character): the part ticked; ended: standing. Frame byte +76 1 / 2 stops (2: x -41.0 first) with
   0x088D3140; a frame flag then starts a jump (6.0 each way toward the facing, fall -0.25), +73 = 1. */
SFA_FUNC(089C1FDC, part_dash_steps) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_088D3EC0, 0x089C1FF4u);
    if (ended(o)) call1(c, o, f_0886469C, 0x089C2008u);
    else {
        int32_t f = S8(o + 76);
        c->r[17] = 1;
        if (f == 1) { *B(o + 76) = 0; call1(c, o, f_088D3140, 0x089C2028u); }
        else if (f == 2) {
            *B(o + 76) = 0; *W(o + 96) = 0xFFD70000u; *W(o + 104) = 0;
            call1(c, o, f_088D3140, 0x089C2054u);
        }
        if (S8(o + 77) != 0) {
            uint32_t st = *B(o + 7);
            *B(o + 73) = 1;
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 96) = 0x60000; *W(o + 104) = 0; *W(o + 100) = 0x60000;
            uint32_t face = *B(o + 11);
            *W(o + 108) = 0xFFFFC000u;
            if (face == 0) *W(o + 96) = 0u - *W(o + 96);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 0x08911550; ended: +73 cleared, 0x0886A530 and standing; else ticked. */
SFA_FUNC(089114F8, move_helper_end_a530) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08911550, 0x0891150Cu);
    if (S8(o + 75) & 0x80) {
        *B(o + 73) = 0;
        call1(c, o, f_0886A530, 0x08911528u);
        call1(c, o, f_0886469C, 0x08911530u);
    } else call1(c, o, f_08861848, 0x08911540u);
    leave(c, sv);
}

/* (fighter): ended: 0x0886A530 and standing; else 0x0886DB2C and ticked. */
SFA_FUNC(089117A8, move_end_a530_or_db2c_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e & 0x80) { call1(c, o, f_0886A530, 0x089117D0u); call1(c, o, f_0886469C, 0x089117D8u); }
    else { call1(c, o, f_0886DB2C, 0x089117E8u); call1(c, o, f_08861848, 0x089117F0u); }
    leave(c, sv);
}

/* (fighter): once ended the +82 wait runs down, then standing; until then ticked. */
SFA_FUNC(0896E1A4, move_end_wait_round) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    int stand = 0;
    if (ended(o)) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        stand = (int16_t)t < 0;
    }
    if (stand) call1(c, o, f_0886469C, 0x0896E1DCu);
    else call1(c, o, f_08861848, 0x0896E1F0u);
    frame_out(c, ra);
}

/* (fighter): ended: facing +697 and standing; else 0x0886DB2C and ticked. */
SFA_FUNC(0897D2B8, move_end_face_round) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e & 0x80) { *B(o + 11) = *B(o + 697); call1(c, o, f_0886469C, 0x0897D2E8u); }
    else { call1(c, o, f_0886DB2C, 0x0897D2F8u); call1(c, o, f_08861848, 0x0897D300u); }
    leave(c, sv);
}

/* (fighter): ended: cue 0x22 / 0x23, 0x0886A530 and standing; else 0x0886DB2C and ticked. */
SFA_FUNC(0899FC38, move_end_cue_a530) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) {
        call1(c, o, f_0887C614, 0x0899FC58u);
        call1(c, o, f_0886A530, 0x0899FC60u);
        call1(c, o, f_0886469C, 0x0899FC68u);
    } else { call1(c, o, f_0886DB2C, 0x0899FC78u); call1(c, o, f_08861848, 0x0899FC80u); }
    leave(c, sv);
}

/* (fighter): ended (bit 7): standing; else a negative frame flag clears +262, 0x0886DB2C and ticked. */
SFA_FUNC(0898FBF4, move_end80_or_flag262) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e & 0x80) call1(c, o, f_0886469C, 0x0898FC1Cu);
    else {
        if (S8(o + 77) & 0x80) *B(o + 262) = 0;
        call1(c, o, f_0886DB2C, 0x0898FC38u);
        call1(c, o, f_08861848, 0x0898FC40u);
    }
    leave(c, sv);
}

/* (character): 0x088D3A80; frame flag n steps it n forward (+18 by the facing); then 0x089B7598. */
SFA_FUNC(089B6AB4, part_step_forward) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3A80, 0x089B6AC8u);
    int32_t d = S8(o + 77);
    if (d != 0) {
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + d : x - d);
        *B(o + 77) = 0;
    }
    call1(c, o, f_089B7598, 0x089B6B00u);
    leave(c, sv);
}

/* (fighter): frame byte +76 = 1 clears +262; ended: standing; else 0x0886DB2C and ticked. */
SFA_FUNC(088FDE94, move_76_end_or_db2c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t f = S8(o + 76);
    enter(c, o, sv);
    if (f == 1) { *B(o + 262) = 0; *B(o + 76) = 0; }
    if (ended(o)) call1(c, o, f_0886469C, 0x088FDECCu);
    else { call1(c, o, f_0886DB2C, 0x088FDEDCu); call1(c, o, f_08861848, 0x088FDEE4u); }
    leave(c, sv);
}
