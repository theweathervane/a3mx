/* More end-of-move steps, 25 - 29 instructions: landings that stand the fighter up with a cue, timed
   endings that turn it to face the opponent, and frame-flag steps on the way. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_0886A530(Cpu *c); void f_0886DB2C(Cpu *c);
void f_0887C614(Cpu *c); void f_0890CC04(Cpu *c); void f_0890CC44(Cpu *c); void f_08917840(Cpu *c);
void f_088D3EC0(Cpu *c); void f_088D3140(Cpu *c); void f_089120F0(Cpu *c); void f_0891DE70(Cpu *c);
void f_08911550(Cpu *c); void f_088D3A80(Cpu *c); void f_088D2E04(Cpu *c); void f_08916A60(Cpu *c);

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
static int ended(gaddr o) { return S8(o + 75) < 0; }
static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }

/* (fighter): 0x0890CC44; in the air 0x0890CC04; on the floor +73 cleared, cue 0x22 / 0x23, +262 cleared,
   height to the floor and standing. */
SFA_FUNC(0890A740, move_air_or_stand) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0890CC44, 0x0890A754u);
    if (S16(o + 132) < S16(o + 22)) call1(c, o, f_0890CC04, 0x0890A770u);
    else {
        *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x0890A780u);
        int32_t y = S16(o + 132);
        *B(o + 262) = 0; *H(o + 22) = (uint16_t)y;
        call1(c, o, f_0886469C, 0x0890A794u);
    }
    leave(c, sv);
}

/* (fighter): the mark's +8 counts down (+262 cleared as it passes 0); standing once ended; then
   0x0886DB2C and 0x08917840. */
SFA_FUNC(08915728, move_mark8_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    gaddr m = o + 0x334 + 8;
    int32_t t = S8(m);
    enter(c, o, sv);
    *B(m) = (uint8_t)(t - 1);
    if (t == 0) *B(o + 262) = 0;
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x0891576Cu);
    call1(c, o, f_0886DB2C, 0x08915774u);
    call1(c, o, f_08917840, 0x0891577Cu);
    leave(c, sv);
}

/* (character): the part ticked; ended: the opponent's (+92) +125 cleared and standing; else a frame flag
   runs 0x088D3140. */
SFA_FUNC(089C2288, part_end_or_flag_3140) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr opp = *W(o + 92);
    c->r[17] = opp;
    call1(c, o, f_088D3EC0, 0x089C22A4u);
    if (ended(o)) { *B(opp + 125) = 0; call1(c, o, f_0886469C, 0x089C22BCu); }
    else if (S8(o + 77) != 0) { *B(o + 77) = 0; call1(c, o, f_088D3140, 0x089C22D8u); }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ended: standing; else a frame flag steps it 2.0 forward, and ticked. */
SFA_FUNC(088EE514, move_end_or_step2) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) call1(c, o, f_0886469C, 0x088EE530u);
    else {
        if (S8(o + 77) != 0) {
            uint32_t x = *W(o + 16);
            *W(o + 16) = *B(o + 11) != 0 ? x + 0x20000 : x - 0x20000;
        }
        call1(c, o, f_08861848, 0x088EE570u);
    }
    frame_out(c, ra);
}

/* (fighter): ticked; frame byte +76 = 1 turns it to face the opponent (+697); a frame flag clears +656;
   standing once ended. */
SFA_FUNC(08921DB8, move_tick_turn_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08921DCCu);
    if (S8(o + 76) == 1) { *B(o + 76) = 0; *B(o + 11) = *B(o + 697); }
    if (S8(o + 77) != 0) { *B(o + 77) = 0; *B(o + 656) = 0; }
    if (ended(o)) call1(c, o, f_0886469C, 0x08921E10u);
    leave(c, sv);
}

/* (fighter): the +82 timer runs down; at 0 it faces the opponent, 0x0886A530 and stands; else 0x0886DB2C
   and ticked. */
static void timer_face_end(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    enter(c, o, sv);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) {
        *B(o + 11) = *B(o + 697);
        call1(c, o, f_0886A530, b1); call1(c, o, f_0886469C, b2);
    } else { call1(c, o, f_0886DB2C, b3); call1(c, o, f_08861848, b4); }
    leave(c, sv);
}
SFA_FUNC(088F971C, move_timer_face_end) { timer_face_end(c, 0x088F9758u, 0x088F9760u, 0x088F9770u, 0x088F9778u); }
SFA_FUNC(08964EAC, move_timer_face_end_b) { timer_face_end(c, 0x08964EE8u, 0x08964EF0u, 0x08964F00u, 0x08964F08u); }

/* (fighter): 0x0890CC44; in the air 0x0890CC04; on the floor it lands (+262 / +73 / +20 cleared), cue
   0x22 / 0x23 and stands. */
SFA_FUNC(0890BD08, move_air_or_land_stand) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0890CC44, 0x0890BD1Cu);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_0890CC04, 0x0890BD38u);
    else {
        *H(o + 22) = (uint16_t)floor;
        *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x0890BD5Cu);
        call1(c, o, f_0886469C, 0x0890BD64u);
    }
    leave(c, sv);
}

/* (fighter): 0x089120F0; ended: 0x0886A530 and standing; else 0x0886DB2C, ticked, and a frame flag clears
   +262. */
SFA_FUNC(08912084, move_helper_end_or_flag) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089120F0, 0x08912098u);
    if (S8(o + 75) & 0x80) { call1(c, o, f_0886A530, 0x089120B0u); call1(c, o, f_0886469C, 0x089120B8u); }
    else {
        call1(c, o, f_0886DB2C, 0x089120C8u);
        call1(c, o, f_08861848, 0x089120D0u);
        if (S8(o + 77) != 0) *B(o + 262) = 0;
    }
    leave(c, sv);
}

/* (fighter): ended: +16's high half cleared and standing; else a frame flag clears +262 with 0x0886DB2C
   (without one 0x0891DE70), and ticked. */
SFA_FUNC(0891B928, move_end_or_flag_de70) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { *H(o + 16) = 0; call1(c, o, f_0886469C, 0x0891B94Cu); }
    else {
        if (S8(o + 77) != 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, 0x0891B96Cu); }
        else call1(c, o, f_0891DE70, 0x0891B97Cu);
        call1(c, o, f_08861848, 0x0891B984u);
    }
    leave(c, sv);
}

/* (fighter): landed: height to the floor, +262 / +235 / +73 cleared, cue 0x22 / 0x23; locked (+373)
   standing, else +7 steps on and ticked. */
SFA_FUNC(08963840, move_land_lock) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int32_t y = S16(o + 132);
    *B(o + 262) = 0; *H(o + 22) = (uint16_t)y; *B(o + 235) = 0; *B(o + 73) = 0;
    call1(c, o, f_0887C614, 0x0896386Cu);
    if (S8(o + 373) == 0) { bump(o + 7); call1(c, o, f_08861848, 0x0896388Cu); }
    else call1(c, o, f_0886469C, 0x0896389Cu);
    leave(c, sv);
}

/* (fighter): ended: 0x0886A530 and standing; else without a frame flag 0x08911550, and ticked. */
SFA_FUNC(08910E6C, move_end_a530_or_1550) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e & 0x80) { call1(c, o, f_0886A530, 0x08910E94u); call1(c, o, f_0886469C, 0x08910E9Cu); }
    else if (S8(o + 77) == 0) { call1(c, o, f_08911550, 0x08910EB4u); call1(c, o, f_08861848, 0x08910EBCu); }
    else call1(c, o, f_08861848, 0x08910ECCu);
    leave(c, sv);
}

/* (fighter): the +82 timer runs down; past it +262 cleared and 0x0886DB2C; standing once ended, else
   ticked. */
SFA_FUNC(08926CA4, move_timer_db2c_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    enter(c, o, sv);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, 0x08926CDCu); }
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x08926CF4u);
    else call1(c, o, f_08861848, 0x08926D04u);
    leave(c, sv);
}

/* (fighter): ticked; ended: standing; else without frame byte +76 it moves by +96 toward its facing. */
SFA_FUNC(08972C04, move_tick_end_or_walk) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08972C18u);
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x08972C30u);
    else if (S8(o + 76) == 0) {
        uint32_t x = *W(o + 16), v = *W(o + 96);
        *W(o + 16) = *B(o + 11) != 0 ? x + v : x + (0u - v);
    }
    leave(c, sv);
}

/* (character): 0x088D3A80; frame flag 1 plays bank voice 0x15 (and 0x22 without +824), steps +7 on,
   clears +820 / +824 and sets +82 = 3. */
SFA_FUNC(089B6360, part_flag_voices) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3A80, 0x089B6374u);
    if (S8(o + 77) == 1) {
        call2(c, o, 0x15, f_088D2E04, 0x089B638Cu);
        if (*W(o + 824) == 0) call2(c, o, 0x22, f_088D2E04, 0x089B63A0u);
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0; *W(o + 820) = 0; *W(o + 824) = 0;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 82) = 3;
    }
    leave(c, sv);
}

/* (fighter): at strength 2 in step 6 0x08916A60; else standing once ended, else ticked. */
SFA_FUNC(089169EC, move_step6_or_end) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 194) == 2 && *B(o + 7) == 6) call1(c, o, f_08916A60, 0x08916A40u);
    else if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x08916A54u);
    else call1(c, o, f_08861848, 0x08916A2Cu);
    frame_out(c, ra);
}
