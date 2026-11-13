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
