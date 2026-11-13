/* The record at 0x08B0274C that directs a sequence of screens (100 bytes): its steps at +2 (u16), +8,
 * +9 and +16 pick handlers from tables, as GAME's own words at +4, +16 and +22 do. Around it:
 * 0x08B027E0 and 0x08B02810, two records holding scroll positions.
 */
#include "step.h"

#define GAME     0x08E75910u
#define DIRECTOR 0x08B0274Cu
#define SCROLL_A 0x08B027E0u
#define SCROLL_B 0x08B02810u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

SFA_FUNC(088A9328, director_clear) { for (uint32_t i = 0; i < 0x64u; i++) *B(DIRECTOR + i) = 0; }

/* The director's handlers, each called with the director. */
static void run_director(Cpu *c, uint32_t step, gaddr table, uint32_t back) {
    c->r[4] = DIRECTOR;
    run_step(c, step, table, 5, back);
}
SFA_FUNC(088A996C, director_run_2)   { run_director(c, *H(DIRECTOR + 2), 0x08A2C5A4u, 0x088A999Cu); }
SFA_FUNC(088A9A40, director_run_8_a) { run_director(c, *B(DIRECTOR + 8), 0x08A2C5BCu, 0x088A9A70u); }
SFA_FUNC(088A9B70, director_run_8_b) { run_director(c, *B(DIRECTOR + 8), 0x08A2C5E4u, 0x088A9BA0u); }
SFA_FUNC(088AA42C, director_run_8_c) { run_director(c, *B(DIRECTOR + 8), 0x08A2C644u, 0x088AA45Cu); }
SFA_FUNC(088ABE18, director_run_8_d) { run_director(c, *B(DIRECTOR + 8), 0x08A2C768u, 0x088ABE48u); }
SFA_FUNC(088AB794, director_run_16)  { run_director(c, *B(DIRECTOR + 16), 0x08A2C738u, 0x088AB7C4u); }

/* Handlers by GAME's words, called with no argument of their own (the handler is left in a0 and the
   table in a1; a 0x10-byte frame): by +4, and by the high bytes of +16 and +22. */
static void run_by_game(Cpu *c, uint32_t step, gaddr table) {
    gaddr fn = *W(table + step * 4u);
    c->r[4] = fn; c->r[5] = table;
    c->r[29] -= 0x10;
    rt_call_indirect(c, fn);
    c->r[29] += 0x10;
}
static uint32_t high_byte(gaddr a) { return (uint32_t)((S16(a) & 0xFF00) >> 8); }
SFA_FUNC(088A9204, run_by_game_4)       { run_by_game(c, *H(GAME + 4), 0x08A2C23Cu); }
SFA_FUNC(088A9748, run_by_game_16_high) { run_by_game(c, high_byte(GAME + 16), 0x08A2C59Cu); }
SFA_FUNC(088AA84C, run_by_game_22_high) { run_by_game(c, high_byte(GAME + 22), 0x08A2C664u); }

/* The same choice by GAME + 22's high byte, for a handler that takes the caller's argument. */
SFA_FUNC(088AA90C, run_by_game_22_high_arg) { run_step(c, high_byte(GAME + 22), 0x08A2C670u, 5, 0x088AA944u); }

/* Scroll B moves up, its speed (+10, in 256ths) growing by +14; at y -3024 the director's step (+9)
   moves on, with 11 frames to wait (+14). */
SFA_FUNC(088A9EA8, scroll_b_rise) {
    gaddr d = ARG(0);
    int16_t speed = S16(SCROLL_B + 10);
    *W(SCROLL_B + 4) -= (uint32_t)(speed << 8);
    *H(SCROLL_B + 10) = (uint16_t)(speed + S16(SCROLL_B + 14));
    if ((int32_t)*W(SCROLL_B + 4) > (int32_t)0xF4300000) return;
    *B(d + 9) = (uint8_t)(*B(d + 9) + 1);
    *B(d + 14) = 0xB;
}

/* Scroll B held at -3024 while the wait runs out; then back to step 0 at -2848, with GAME + 10 = 1. */
SFA_FUNC(088A9EFC, scroll_b_hold) {
    gaddr d = ARG(0);
    *W(SCROLL_B + 4) = 0xF4300000u;
    int8_t left = (int8_t)(S8(d + 14) - 1);
    *B(d + 14) = (uint8_t)left;
    if (left != 0) return;
    *B(d + 9) = 0;
    *W(SCROLL_B + 4) = 0xF4E00000u;
    *H(GAME + 10) = 1;
}

/* Every 7 frames (+12): the director's step on, and GAME + 10 up by one. */
SFA_FUNC(088AA7C0, director_every_7) {
    gaddr d = ARG(0);
    *B(d + 12) = (uint8_t)(*B(d + 12) - 1);
    if (*B(d + 12) != 0) return;
    *B(d + 9) = (uint8_t)(*B(d + 9) + 1);
    *B(d + 12) = 7;
    *H(GAME + 10) = (uint16_t)(*H(GAME + 10) + 1);
}

/* Scroll A's position: 72 for each count in GAME + 10's low byte. */
SFA_FUNC(088AA808, scroll_a_by_game_10) {
    uint32_t n = *H(GAME + 10) & 0xFF;
    *W(SCROLL_A + 4) = (uint32_t)(int16_t)((int16_t)(n << 6) + (int16_t)(n << 3));
}
