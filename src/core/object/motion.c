/* Steps that objects' routines share: moving by a speed, waiting, and stepping on.
 *
 * The objects here (0xC0 bytes, and fighters, which begin the same way) keep:
 *   +4, +5      u8   the routine's step and the step within it
 *   +11         u8   facing (bit 0: mirrored)
 *   +16, +20    s32  x and y, 16.16 fixed point (so +18 and +22 are the whole x and y)
 *   +82         s16  a timer
 *   +88         u32  the object's owner
 *   +96, +100   s32  speed; +104, +108 s32 acceleration
 *   +112, +114  s16  a position to reach, and the one being worked towards
 *   +132        s16  the floor's y
 * GAME + 16 / + 20 / + 22 are words the screen's fade and scroll leave for objects to wait on.
 */
#include "../decomp.h"

#define GAME 0x08E75910u
#define BATTLE 0x092E1228u

#define O_STEP    4
#define O_SUBSTEP 5
#define O_FACING  11
#define O_X       16
#define O_XI      18
#define O_Y       20
#define O_YI      22
#define O_TIMER   82
#define O_OWNER   88
#define O_VX      96
#define O_VY      100
#define O_AX      104
#define O_AY      108
#define O_GOAL    112
#define O_TARGET  114
#define O_FLOOR   132
#define O_FRAME_DATA 296   /* u32: the current frame's data (fighters) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

static void next_step(gaddr o) { *B(o + O_STEP) = (uint8_t)(*B(o + O_STEP) + 1); }
static void next_substep(gaddr o) { *B(o + O_SUBSTEP) = (uint8_t)(*B(o + O_SUBSTEP) + 1); }

/* x and y by their speeds, the speeds by their accelerations; answers 1 while the object (its y plus
   the frame's height byte) is still short of the floor. */
SFA_FUNC(08862004, object_fall) {
    gaddr o = ARG(0);
    uint32_t vx = *W(o + O_VX), vy = *W(o + O_VY);
    *W(o + O_X) += vx; *W(o + O_VX) = vx + *W(o + O_AX);
    *W(o + O_Y) += vy; *W(o + O_VY) = vy + *W(o + O_AY);
    RETURN(S16(o + O_YI) + *B(*W(o + O_FRAME_DATA) + 3) < S16(o + O_FLOOR));
}

/* x by its speed and the speed by its acceleration; answers 1 once the speed points away from the
   target (the object has reached or passed it). */
SFA_FUNC(0889CE64, object_move_x_to_target) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + O_VX);
    *W(o + O_X) += v;
    v += *W(o + O_AX);
    *W(o + O_VX) = v;
    RETURN((int32_t)(v ^ (uint32_t)(S16(o + O_XI) - S16(o + O_TARGET))) >= 0);
}

/* The same for y. */
SFA_FUNC(0889CEA4, object_move_y_to_target) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + O_VY);
    *W(o + O_Y) += v;
    v += *W(o + O_AY);
    *W(o + O_VY) = v;
    RETURN((int32_t)(v ^ (uint32_t)(S16(o + O_YI) - S16(o + O_TARGET))) >= 0);
}

/* y by its speed, kept within 0 to the height the object's two bytes at +76 give (it scrolls round). */
SFA_FUNC(0889E704, object_scroll_y) {
    gaddr o = ARG(0);
    int32_t height = (uint16_t)((S8(o + 76) << 8) + S8(o + 77));
    *W(o + O_Y) += *W(o + O_VY);
    int32_t y = S16(o + O_YI);
    if (y < 0) {
        do y = (int16_t)(y + height); while (y < 0);
        *H(o + O_YI) = (uint16_t)y;
        y = S16(o + O_YI);
    }
    if (y >= height) {
        do y = (int16_t)(y - height); while (y >= height);
        *H(o + O_YI) = (uint16_t)y;
    }
}

/* Slows to a stop moving left: once the speed is down to 2.0 the next step starts at that speed, with
   0xC00 set in the timer. */
SFA_FUNC(088ADA3C, object_brake_left) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + O_VX);
    *W(o + O_X) -= v;
    v -= *W(o + O_AX);
    *W(o + O_VX) = v;
    if ((int32_t)v > 0x20000) return;
    next_substep(o);
    *W(o + O_VX) = 0x20000;
    *H(o + O_TIMER) = (uint16_t)(S16(o + O_TIMER) | 0xC00);
}

/* Moves left while the timer's high byte counts down; the next step when it reaches 0. */
SFA_FUNC(088ADA8C, object_left_for_a_while) {
    gaddr o = ARG(0);
    int16_t timer = S16(o + O_TIMER);
    *W(o + O_X) -= *W(o + O_VX);
    uint16_t left = (uint16_t)(((timer & 0xFF00) >> 8) - 1);
    if (left == 0) next_substep(o);
    *H(o + O_TIMER) = (uint16_t)((timer & 0xFF) | (left << 8));
}

/* Right by 4 a frame to x 48; there the timer is set to 150 and the next routine step starts. */
SFA_FUNC(088BBB88, object_slide_in_to_48) {
    gaddr o = ARG(0);
    int16_t x = S16(o + O_XI);
    if (x == 0x30) { *H(o + O_TIMER) = 0x96; next_step(o); }
    else *H(o + O_XI) = (uint16_t)(x + 4);
}

/* Counts the timer down; the next routine step at 0. */
SFA_FUNC(088BBBBC, object_wait_timer) {
    gaddr o = ARG(0);
    int16_t t = (int16_t)(S16(o + O_TIMER) - 1);
    *H(o + O_TIMER) = (uint16_t)t;
    if (t == 0) next_step(o);
}

/* Down by 2 a frame to the floor; there the timer is set to 150 and the next routine step starts. */
SFA_FUNC(088BBDC8, object_drop_to_floor) {
    gaddr o = ARG(0);
    int16_t y = S16(o + O_YI);
    if (y == S16(o + O_FLOOR)) { *H(o + O_TIMER) = 0x96; next_step(o); }
    else *H(o + O_YI) = (uint16_t)(y - 2);
}

/* Waits for GAME + 20 to go negative, then aims at its goal from a standstill. Two objects have a copy
   each. */
static void aim_when_ready(gaddr o) {
    if (S16(GAME + 20) >= 0) return;
    next_substep(o);
    *H(o + O_TARGET) = (uint16_t)S16(o + O_GOAL);
    *W(o + O_VX) = 0;
}
SFA_FUNC(0889E674, object_aim_when_ready_a) { aim_when_ready(ARG(0)); }
SFA_FUNC(0889F01C, object_aim_when_ready_b) { aim_when_ready(ARG(0)); }

/* Waits for the high byte of GAME + 22, then aims: at its goal, or (objects with +10 set) at the x in
   +118 with the y acceleration from +119. */
SFA_FUNC(0889BE20, object_aim_by_kind) {
    gaddr o = ARG(0);
    if ((S16(GAME + 22) & 0xFF00) == 0) return;
    next_substep(o);
    int16_t target = S16(o + O_GOAL);
    if (*B(o + 10) != 0) {
        target = *B(o + 118);
        *W(o + O_AY) = (uint32_t)*B(o + 119) << 16;
    }
    *H(o + O_TARGET) = (uint16_t)target;
    *W(o + O_VY) = 0;
}

/* The next step once bit 15 of GAME + 20 is set. */
SFA_FUNC(088B1454, object_wait_game_20) {
    if (S16(GAME + 20) & 0x8000) next_substep(ARG(0));
}

/* +93 cleared while bit 8 of GAME + 20 is set. */
SFA_FUNC(088B2634, object_clear_93) {
    if (S16(GAME + 20) & 0x100) *B(ARG(0) + 93) = 0;
}

/* Takes its owner's place and facing, moved by the offset the table gives for the owner's frame (byte 9
   of its frame data): two s8 per entry, x (mirrored with the owner) and y. */
SFA_FUNC(0887DA58, object_follow_owner) {
    gaddr o = ARG(0), table = ARG(1), owner = *W(o + O_OWNER);
    *H(o + O_XI) = *H(owner + O_XI);
    *H(o + O_YI) = *H(owner + O_YI);
    uint8_t facing = *B(owner + O_FACING);
    gaddr offset = table + *B(*W(owner + O_FRAME_DATA) + 9) * 2u;
    *B(o + O_FACING) = facing;
    int16_t x = S16(o + O_XI), y = S16(o + O_YI);
    *H(o + O_XI) = (uint16_t)((facing & 1) ? x - S8(offset) : x + S8(offset));
    *H(o + O_YI) = (uint16_t)(y + S8(offset + 1));
}

/* Starts animation `n` of the table at 0x08A24120. */
void f_0882196C(Cpu *c);   /* object_set_animation (animation.c) */
SFA_FUNC(0887DF88, object_set_animation_n) {
    c->r[5] = *W(0x08A24120u + (ARG(1) & 0xFFFF) * 4u);
    f_0882196C(c);
}

/* The object's four corners take the stage's colour: a 15-bit colour from 0x08A25B38 by BATTLE + 102,
   spread to 8 bits a part (four bytes each from +36, +42 and +48). */
SFA_FUNC(08887C24, object_take_stage_colour) {
    gaddr o = ARG(0);
    uint16_t colour = *H(0x08A25B38u + (uint32_t)S16(BATTLE + 102) * 2u);
    uint8_t a = (uint8_t)((colour & 0x1F) << 3), b = (uint8_t)((colour & 0x3E0) >> 2), d = (uint8_t)((colour & 0x7C00) >> 7);
    for (int i = 3; i >= 0; i--) *B(o + 36 + i) = a;
    for (int i = 3; i >= 0; i--) *B(o + 42 + i) = b;
    for (int i = 3; i >= 0; i--) *B(o + 48 + i) = d;
}

/* While the high byte of GAME + 22 is set: the next step, and the aim (0x0889BE20, which steps on
   again). Otherwise, for an owner with bit 0 of +78: the next step with the x acceleration halved. */
void f_0889BE20(Cpu *c);
SFA_FUNC(0889BD9C, object_aim_or_slow) {
    gaddr o = ARG(0), owner = ARG(1);
    if ((S16(GAME + 22) & 0xFF00) != 0) {
        next_substep(o);
        c->r[29] -= 0x20;
        c->r[4] = o; c->r[5] = owner; c->r[6] = owner;
        f_0889BE20(c);
        c->r[29] += 0x20;
    } else if (S8(owner + 78) & 1) {
        next_substep(o);
        *W(o + O_AX) = (uint32_t)((int32_t)*W(o + O_AX) >> 1);
    }
}

/* An object that copies another (`src`): its place, facing and, 0x2A on, its picture. It shows only
   while the source's fighter (+92) is character 0x29, and its routine steps on when the source goes. */
SFA_FUNC(088A0464, object_copy_other) {
    gaddr o = ARG(0), src = ARG(1);
    if (*B(src) == 0) { next_step(o); return; }
    if (S8(*W(src + 92) + 338) == 0x29) {
        *B(o + 1) = 1;
        *B(o + 171) = *B(src + 171);
    } else *B(o + 1) = 0;
    *W(o + O_X) = *W(src + O_X);
    *W(o + O_Y) = *W(src + O_Y);
    *B(o + O_FACING) = *B(src + O_FACING);
    gaddr entry = *W(src + 28);
    if (entry != 0) *H(o + 84) = (uint16_t)(*H(entry + 4) + 0x2A);
}

/* A counter that runs +35 up to +34 - 1, one every +32 frames (+40 counts them); at the end, or at once
   when BATTLE + 82 is already set, it stops there, the object steps on and BATTLE + 82 is set. */
SFA_FUNC(08887018, object_count_up) {
    gaddr o = ARG(0);
    uint8_t last = (uint8_t)(*B(o + 34) - 1);
    if (S8(BATTLE + 82) == 0) {
        int16_t wait = (int16_t)(S16(o + 40) - 1);
        *H(o + 40) = (uint16_t)wait;
        if (wait >= 0) return;
        *H(o + 40) = *B(o + 32);
        uint8_t n = (uint8_t)(*B(o + 35) + 1);
        *B(o + 35) = n;
        if (n < last) return;
    }
    *B(o + 35) = last;
    next_substep(o);
    *B(BATTLE + 82) = 1;
}

/* The round's clock as two digits: tens and units into +10 and +11, and their pictures (0xC0 + digit)
   into the object's two parts (+0x74 and +0xB4, each +60). Not while GAME + 47 is 3. */
SFA_FUNC(0888D8BC, clock_digits) {
    gaddr o = ARG(0);
    if (S8(GAME + 47) == 3) return;
    int clock = S8(BATTLE + 9);
    *B(o + 10) = (uint8_t)(clock / 10);
    *B(o + 11) = (uint8_t)(clock % 10);
    *H(o + 0x74 + 60) = (uint16_t)(*B(o + 10) + 0xC0);
    *H(o + 0xB4 + 60) = (uint16_t)(*B(o + 11) + 0xC0);
}

void f_08821368(Cpu *c);   /* a random number */
void f_0887DF88(Cpu *c);   /* object_set_animation_n, above */
void f_08899550(Cpu *c);   /* does nothing (empty.c) */

/* An object starts to fall: the next routine step, 8 lower, its sideways speed taken from +16, a random
   downward acceleration (0 to 0x30 in steps of 0x10), and the animation its +78 names. */
SFA_FUNC(08881E2C, object_start_falling) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    next_step(o);
    *B(o + 12) = 0;
    *B(o + 72) = 1;
    *W(o + O_VX) = *W(o + O_X);
    *H(o + O_YI) = (uint16_t)(S16(o + O_YI) + 8);
    *W(o + O_AX) = 0; *W(o + O_AY) = 0; *W(o + O_VY) = 0;
    f_08821368(c);
    *W(o + O_AY) = c->r[2] & 0x30;
    *H(o + O_TIMER) = 0;
    *H(o + 86) = 5;
    c->r[4] = o; c->r[5] = (uint32_t)S8(o + 78) & 0xFFFF;
    f_0887DF88(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Waits for the high byte of GAME + 22, then steps on and sets off from the side GAME + 41 gives (or from
   0, 0 when GAME + 57 is 3) towards -0x300, 0xC0. Objects with +10 set also call an empty routine for
   each of the first two fighters. */
SFA_FUNC(08899430, object_enter_from_side) {
    gaddr o = ARG(0);
    if ((S16(GAME + 22) & 0xFF00) != 0) {
        uint16_t x = 0, y = 0;
        next_substep(o);
        if (S8(GAME + 57) != 3) {
            if (S8(GAME + 41) & 1) { x = 0x300; y = 0xFF40; }
            else { x = 0xFD00; y = 0xC0; }
        }
        *H(o + O_X) = x; *H(o + O_Y) = y;
        *H(o + O_GOAL) = 0xFD00; *H(o + O_TARGET) = 0xC0;
    }
    if (*B(o + 10) != 0) {
        c->r[29] -= 0x20;
        c->r[4] = 0x092E3EA0u; f_08899550(c);
        c->r[4] = 0x092E4338u; f_08899550(c);
        c->r[29] += 0x20;
    }
}

/* An object placed on an 8 by 8 grid from a fighter's x and y (+18, +22), which are column and row
   numbers here: +78 = the fighter's character, +139 / +140 the row and column, and the object's own x
   and y from the grid's columns and rows. The original builds the two tables on its stack and indexes
   them with the signed low byte of each number, so they are built there here too. */
SFA_FUNC(0889995C, object_place_on_grid) {
    static const uint16_t GRID[16] = { 0x78, 0x90, 0xA8, 0xC0, 0xD8, 0xF0, 0x108, 0x1C0,     /* columns' x */
                                       0xC8, 0xB0, 0x98, 0x80, 0x68, 0x50, 0x38, 0x110 };    /* rows' y */
    gaddr o = ARG(0), f = ARG(1);
    c->r[29] -= 0x20;
    gaddr table = c->r[29];
    for (int i = 0; i < 16; i++) *H(table + (uint32_t)i * 2u) = GRID[i];
    int16_t column = S16(f + 18), row = S16(f + 22);
    *B(o + 78) = *B(f + 338);
    *B(o + 139) = (uint8_t)row;
    *B(o + 140) = (uint8_t)column;
    *H(o + O_XI) = *H(table + (uint32_t)((int8_t)column * 2));
    *H(o + O_YI) = *H(table + 16 + (uint32_t)((int8_t)row * 2));
    c->r[29] += 0x20;
}

/* ---- steps that wait on the fight's intro counter (BATTLE + 44) and GAME + 20 ---- */

void f_088215C4(Cpu *c);   /* object_draw (frame.c) */
void f_088219B0(Cpu *c);   /* object_animate (animation.c) */
void f_0882196C(Cpu *c);   /* object_set_animation (animation.c) */

/* Counts the timer down; once it has run out, the next step, with GAME + 20 set to 59. */
SFA_FUNC(08854ED8, object_wait_then_59) {
    gaddr o = ARG(0);
    int16_t t = S16(o + O_TIMER);
    *H(o + O_TIMER) = (uint16_t)(t - 1);
    if (t != 0) return;
    next_substep(o);
    *H(GAME + 20) = 0x3B;
}

/* Counts GAME + 20 down; at 0 the next step, with BATTLE + 44 set to 2. */
SFA_FUNC(08854F10, object_count_game_20) {
    gaddr o = ARG(0);
    int16_t v = S16(GAME + 20);
    if (v == 0) { next_substep(o); *B(BATTLE + 44) = 2; *H(GAME + 20) = 0; }
    else *H(GAME + 20) = (uint16_t)(v - 1);
}

/* When BATTLE + 44 reaches 2: the next step, moving at 48 a frame, to the left if BATTLE + 63. */
SFA_FUNC(088560EC, object_go_at_2) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 44) != 2) return;
    next_substep(o);
    *W(o + O_VX) = S8(BATTLE + 63) != 0 ? 0xFFD00000u : 0x300000u;
}

/* The next step when BATTLE + 44 is 1 (two objects have a copy each), or once it is 5 or more (the timer
   then set to 1). */
static void step_at_1(gaddr o) { if (S8(BATTLE + 44) == 1) next_substep(o); }
SFA_FUNC(088562A0, object_step_at_1_a) { step_at_1(ARG(0)); }
SFA_FUNC(08857550, object_step_at_1_b) { step_at_1(ARG(0)); }
SFA_FUNC(08856650, object_step_at_5) {
    if (S8(BATTLE + 44) < 5) return;
    next_substep(ARG(0));
    *H(ARG(0) + O_TIMER) = 1;
}

/* x by its speed until it reaches 0 from the far side (x and the speed then have the same sign): it
   stops at 0 and steps on. */
SFA_FUNC(0885661C, object_move_x_to_0) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + O_VX), x = v + *W(o + O_X);
    *W(o + O_X) = x;
    if ((int32_t)(v ^ x) < 0) return;
    *W(o + O_X) = 0;
    next_substep(o);
}

/* Draws the object while +139 counts down (the step at +7 moves on when it runs out); the first also
   moves it by its speed. */
static void count_139_and_draw(Cpu *c, gaddr o) {
    int8_t left = (int8_t)(S8(o + 139) - 1);
    *B(o + 139) = (uint8_t)left;
    if (left < 0) *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    c->r[29] -= 0x20;
    c->r[4] = o;
    f_088215C4(c);
    c->r[29] += 0x20;
}
SFA_FUNC(08857D70, object_move_and_draw_139) { *W(ARG(0) + O_X) += *W(ARG(0) + O_VX); count_139_and_draw(c, ARG(0)); }
SFA_FUNC(08857E24, object_draw_139) { count_139_and_draw(c, ARG(0)); }

/* Animates and draws the object; once its animation has ended (+75 negative) its routine is set to 2. */
SFA_FUNC(0887E10C, object_animate_draw_end) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(o + 75) < 0) *B(o + O_STEP) = 2;
    c->r[4] = o; f_088219B0(c);
    c->r[4] = c->r[16]; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Starts the animation for the object's type (+3): 0x08A242DC[type] is its number in 0x08A24120. */
SFA_FUNC(0887D990, object_set_animation_by_type) {
    c->r[29] -= 0x20;
    c->r[5] = *W(0x08A24120u + *B(0x08A242DCu + *B(ARG(0) + 3)) * 4u);
    f_0882196C(c);
    c->r[29] += 0x20;
}
