/* The objects of a fight's intro (the pieces that slide in and out before "FIGHT"): their routines'
 * steps. BATTLE + 44 counts the intro's stages; the objects move in x at their speed (+96, with +104
 * the acceleration) until they pass a place, then settle on their goal (+112) and step on.
 */
#include "step.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define BT_INTRO 44

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_088215C4(Cpu *c);   /* object_draw */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08822E74(Cpu *c);   /* object_make_8 */

static void next_step(gaddr o)    { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); }
static void next_substep(gaddr o) { *B(o + 5) = (uint8_t)(*B(o + 5) + 1); }
static void settle(gaddr o)       { next_substep(o); *H(o + 18) = (uint16_t)S16(o + 112); }
static void draw(Cpu *c, gaddr o) { c->r[29] -= 0x20; c->r[4] = o; f_088215C4(c); c->r[29] += 0x20; }

/* x by its speed; with `accelerate`, the speed by its acceleration first. Answers the speed and the x. */
static uint32_t move_x(gaddr o, int accelerate, uint32_t *speed) {
    uint32_t v = *W(o + 96);
    if (accelerate) { v += *W(o + 104); *W(o + 96) = v; }
    *W(o + 16) += v;
    *speed = v;
    return *W(o + 16);
}
/* Past `place` in the direction of travel. */
static int passed(uint32_t v, uint32_t x, uint32_t place) { return (int32_t)(v ^ (x - place)) >= 0; }

/* ---- the routines: each object kind's step, giving way to a halt ---- */

SFA_FUNC(08854CD0, intro_routine_a) { run_step_or_halt(c, ARG(0), 0x08A16D40u, 0x08854D00u); }
SFA_FUNC(08854F94, intro_routine_b) { run_step_or_halt(c, ARG(0), 0x08A16D64u, 0x08854FC4u); }
SFA_FUNC(08855EEC, intro_routine_c) { run_step_or_halt(c, ARG(0), 0x08A16E58u, 0x08855F1Cu); }
SFA_FUNC(08856168, intro_routine_d) { run_step_or_halt(c, ARG(0), 0x08A16E78u, 0x08856198u); }
SFA_FUNC(08856364, intro_routine_e) { run_step_or_halt(c, ARG(0), 0x08A16E94u, 0x08856394u); }
SFA_FUNC(08856520, intro_routine_f) { run_step_or_halt(c, ARG(0), 0x08A16EB4u, 0x08856550u); }
SFA_FUNC(08857364, intro_routine_g) { run_step_or_halt(c, ARG(0), 0x08A16FACu, 0x08857394u); }

/* An object owned by another: its step's handler (0x08A17110) with the owner, unless BATTLE + 36 hides
   the fight's objects. */
SFA_FUNC(08857AC8, owned_object_routine) {
    if (S8(BATTLE + 36) != 0) return;
    run_step_with(c, *B(ARG(0) + 4), 0x08A17110u, *W(ARG(0) + 88), 0x08857B08u);
}

/* ---- moving ---- */

/* Accelerating, until past x 192: the next substep. */
SFA_FUNC(08856058, intro_accelerate_past_192) {
    uint32_t v, x = move_x(ARG(0), 1, &v);
    if (passed(v, x, 0xC00000)) next_substep(ARG(0));
}

/* Accelerating, until out of x 56 to 328: settles on its goal. */
SFA_FUNC(0885609C, intro_accelerate_out_of_56_328) {
    uint32_t v, x = move_x(ARG(0), 1, &v);
    if (x - 0x380000 >= 0x1100000u) settle(ARG(0));
}

/* Until out of x 40 to 520: the next step. */
SFA_FUNC(0885612C, intro_move_out_of_40_520) {
    uint32_t v, x = move_x(ARG(0), 0, &v);
    if (x - 0x280000 >= 0x1E00000u) next_step(ARG(0));
}

/* Until within x 144 to 224: settles on its goal. Drawn. */
SFA_FUNC(088562CC, intro_move_into_144_224) {
    uint32_t v, x = move_x(ARG(0), 0, &v);
    if (x - 0x900000 <= 0x500000u) settle(ARG(0));
    draw(c, ARG(0));
}

/* Until past x 96: the next substep. Drawn. */
SFA_FUNC(0885757C, intro_move_past_96) {
    uint32_t v, x = move_x(ARG(0), 0, &v);
    if (passed(v, x, 0x600000)) next_substep(ARG(0));
    draw(c, ARG(0));
}

/* Until out of x 56 to 136: settles on its goal. Drawn. */
SFA_FUNC(088575C8, intro_move_out_of_56_136) {
    uint32_t v, x = move_x(ARG(0), 0, &v);
    if (x - 0x380000 >= 0x500000u) settle(ARG(0));
    draw(c, ARG(0));
}

/* Down by 24 a frame; at y 288 the intro moves to stage 3 and the object steps on. */
SFA_FUNC(08854F54, intro_drop_to_288) {
    gaddr o = ARG(0);
    int32_t y = (int32_t)(*W(o + 20) + 0x180000);
    *W(o + 20) = (uint32_t)y;
    if (y < 0x1200000) return;
    *B(BATTLE + BT_INTRO) = 3;
    next_step(o);
}

/* ---- waiting ---- */

/* Counts the timer (+82) down; when it has run out, the next substep / the next step. */
static int timer_ran_out(gaddr o) {
    int16_t t = S16(o + 82);
    *H(o + 82) = (uint16_t)(t - 1);
    return t == 0;
}
SFA_FUNC(088550C0, intro_wait_then_substep) { if (timer_ran_out(ARG(0))) next_substep(ARG(0)); }
SFA_FUNC(08856684, intro_wait_then_step)    { if (timer_ran_out(ARG(0))) next_step(ARG(0)); }

/* Drawn; the next step once the intro is at stage 2. */
SFA_FUNC(08856324, intro_draw_until_stage_2) {
    if (S8(BATTLE + BT_INTRO) >= 2) next_step(ARG(0));
    draw(c, ARG(0));
}

/* Drawn while +139 counts down; when it runs out, or at once when the fighter's +769 is set, its part
   (+7) goes to 4 with +119 and +127 = 0x15. */
SFA_FUNC(0885806C, owned_draw_until_139) {
    gaddr o = ARG(0);
    int done = S8(ARG(1) + 769) != 0;
    if (!done) {
        int8_t left = (int8_t)(S8(o + 139) - 1);
        *B(o + 139) = (uint8_t)left;
        done = left < 0;
    }
    if (done) { *B(o + 7) = 4; *B(o + 127) = 0x15; *B(o + 119) = 0x15; }
    draw(c, o);
}

/* ---- setting up ---- */

/* (owner, type): a new object of kind 1 from the pool of 8, owned by `owner`; answers it, or 0. */
SFA_FUNC(08855D44, intro_make_owned) {
    gaddr owner = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = owner;
    c->r[4] = 1; c->r[5] = ARG(1) & 0xFF;
    f_08822E74(c);
    gaddr o = c->r[2];
    if (o != 0) { *B(o) = 1; *W(o + 88) = owner; }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object, layer, x, y) */
SFA_FUNC(08855D80, intro_place) {
    gaddr o = ARG(0);
    *B(o + 9) = (uint8_t)ARG(1);
    *B(o + 14) = 0; *H(o + 24) = 7; *W(o + 148) = 0; *H(o + 86) = 6; *W(o + 164) = 1;
    *B(o + 11) = 0; *B(o + 13) = 0; *B(o + 12) = 0;
    *H(o + 18) = (uint16_t)ARG(2); *H(o + 22) = (uint16_t)ARG(3);
}

static void set_animation(Cpu *c, gaddr entry) {
    c->r[29] -= 0x20;
    c->r[5] = entry;
    f_0882196C(c);
    c->r[29] += 0x20;
}
SFA_FUNC(08858418, owned_set_animation) {
    gaddr o = ARG(0);
    *W(o + 148) = 6;
    *W(o + 156) = 0x08A16FF8u;
    set_animation(c, *W(0x08A170C8u + (uint32_t)(int16_t)ARG(1) * 4u));
}
SFA_FUNC(08858B00, set_animation_17568) { set_animation(c, *W(0x08A17568u + (uint32_t)(int16_t)ARG(1) * 4u)); }

/* Falls (y by its speed, the speed by its acceleration) to y 128: there it stops with the timer at 32,
   the intro moves to stage 1 and the object steps on. */
SFA_FUNC(08854E78, intro_fall_to_128) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 100) + *W(o + 108);
    *W(o + 100) = v;
    *W(o + 20) += v;
    if ((int32_t)*W(o + 20) < 0x800000) return;
    next_substep(o);
    *H(o + 22) = 0x80; *H(o + 82) = 0x20;
    *B(BATTLE + BT_INTRO) = 1;
}

SFA_FUNC(08856578, intro_setup_08856578) {
    gaddr o = ARG(0);
    next_step(o);
    *B(o + 14) = 0; *H(o + 24) = 7; *B(o + 11) = 0; *B(o + 13) = 0; *B(o + 12) = 0;
    *W(o + 148) = 1; *H(o + 86) = 6; *W(o + 164) = 1;
    set_animation(c, *W(0x08A16B0Cu));
}

/* Rises with a growing speed (y by the speed, the speed by 2.0) while above y 128; at 128 or less it
   stops there for 16 frames, the intro moves to stage 4 and the object steps on. Drawn. */
SFA_FUNC(088550E8, intro_settle_at_128) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 100) + 0x20000;
    *W(o + 100) = v;
    *W(o + 20) += v;
    if ((int32_t)*W(o + 20) <= 0x800000) {
        next_substep(o);
        *H(o + 22) = 0x80; *H(o + 82) = 0x10;
        *B(BATTLE + BT_INTRO) = 4;
    }
    draw(c, o);
}

/* Bounces: while the timer runs, y is the low byte of the table 0x08A16D88's word for the frames left;
   then it rests at 128 for 16 frames and steps on. Drawn. */
SFA_FUNC(0885515C, intro_bounce) {
    gaddr o = ARG(0);
    int16_t left = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)left;
    *H(o + 22) = (uint16_t)(S16(0x08A16D88u + (uint32_t)(left * 2)) & 0xFF);
    if (left == 0) { next_substep(o); *H(o + 82) = 0x10; *H(o + 22) = 0x80; }
    draw(c, o);
}

void f_08823338(Cpu *c);   /* object_make_list_3 (pool.c) */

/* Drawn while the timer runs; when it has run out the object steps on, the intro moves to stage 5 and a
   third-list object of kind 5 is made, pointing back at this one (+124). */
SFA_FUNC(088551D0, intro_wait_then_stage_5) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (!timer_ran_out(o)) { c->r[4] = o; f_088215C4(c); }
    else {
        next_substep(o);
        *B(BATTLE + BT_INTRO) = 5;
        c->r[4] = 5; c->r[5] = 0;
        f_08823338(c);
        if (c->r[2] != 0) *W(c->r[2] + 124) = o;
    }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* ---- 0.70.0 ---- */

void f_08855D80(Cpu *c);   /* intro_object_setup (object, layer, x, y) */

SFA_FUNC(08854FEC, intro_setup_08854FEC) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 0; c->r[6] = 0xC0; c->r[7] = 0x110;
    f_08855D80(c);
    *W(o + 148) = 0xB;
    *W(o + 156) = 0x08A16962u;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 7) = 0xE; *B(o + 79) = 1; *H(o + 82) = 3;
    *W(o + 100) = 0xFFE00000u;
    c->r[4] = o; c->r[5] = *W(0x08A16A90u + 40);
    f_0882196C(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Comes in from the left at 32 a frame to x 168, or (BATTLE + 63 clear) from x 472 leftwards to 88. */
SFA_FUNC(088563BC, intro_setup_from_a_side) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 3; c->r[6] = 0xFFFFFF28u; c->r[7] = 0x69;
    f_08855D80(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 96) = 0x200000; *H(o + 112) = 0xA8;
    if (S8(BATTLE + 63) == 0) { *H(o + 18) = 0x1D8; *W(o + 96) = 0xFFE00000u; *H(o + 112) = 0x58; }
    c->r[4] = o; c->r[5] = *W(0x08A16A90u + 4);
    f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Two objects of kind 2 made, one for each side: at x 64 moving right with BATTLE + 136, at x 320
   moving left with BATTLE + 140. */
SFA_FUNC(08857A3C, intro_make_pair) {
    gaddr saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[4] = 2; c->r[5] = 0;
    f_08822E74(c);
    gaddr o = c->r[2];
    if (o != 0) {
        c->r[16] = BATTLE;
        *W(o + 88) = *W(BATTLE + 136); *H(o + 18) = 0x40; *W(o + 96) = 0x100000; *B(o + 9) = 0;
        c->r[4] = 2; c->r[5] = 1;
        f_08822E74(c);
        o = c->r[2];
        if (o != 0) { *W(o + 88) = *W(BATTLE + 140); *H(o + 18) = 0x140; *W(o + 96) = 0xFFF00000u; *B(o + 9) = 1; }
    }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

void f_08855D44(Cpu *c);

/* The intro's first object made (its routine table 0x08A166D8 at 0x092E3E98), with its four parts
   started by 0x08855D44; GAME + 20 = 0x100. */
SFA_FUNC(08855AA4, intro_make_first) {
    gaddr saved = c->r[16];
    c->r[29] -= 0x20;
    *W(0x092E3E98u) = 0x08A166D8u;
    c->r[4] = 0; c->r[5] = 0;
    f_08822E74(c);
    gaddr o = c->r[2];
    c->r[16] = o;
    if (o != 0) {
        *B(o) = 1;
        c->r[4] = o; c->r[5] = 0; c->r[6] = 1; f_08855D44(c);
        c->r[4] = o; c->r[5] = 1; c->r[6] = 0; f_08855D44(c);
        c->r[4] = o; c->r[5] = 2; c->r[6] = 0; f_08855D44(c);
        *H(GAME + 20) = 0x100;
        c->r[4] = o; c->r[5] = 0xD; c->r[6] = 0; f_08855D44(c);
    }
    c->r[29] += 0x20;
    c->r[16] = saved;
}
