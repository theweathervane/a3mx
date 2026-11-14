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

/* ---- 0.69.0 ---- */

void f_088AA90C(Cpu *c);   /* run_by_game_22_high_arg */
void f_088AA980(Cpu *c);   /* next_pair_from_table (screen_steps.c) */

/* The handler for GAME + 22's high byte, then the one for the director's step +9 (0x08A2C654). */
SFA_FUNC(088AA6B0, director_run_22_then_9) {
    gaddr d = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = d;
    f_088AA90C(c);
    gaddr fn = *W(0x08A2C654u + *B(d + 9) * 4u);
    c->r[4] = d; c->r[5] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The pair of bytes at +72 / +73 (a count of frames and the next entry of the table 0x08A2C67C): started
   from entry 0 on the director, and stepped on an object when its count runs out. */
SFA_FUNC(088AA890, director_start_pairs) {
    *B(DIRECTOR + 73) = 0;
    c->r[29] -= 0x20;
    c->r[4] = DIRECTOR;
    f_088AA980(c);
    c->r[29] += 0x20;
}
SFA_FUNC(088AA950, count_pair) {
    gaddr o = ARG(0);
    *B(o + 72) = (uint8_t)(*B(o + 72) - 1);
    if (*B(o + 72) != 0) return;
    c->r[29] -= 0x20;
    f_088AA980(c);
    c->r[29] += 0x20;
}

/* ---- 0.71.0 ---- */

void f_08822934(Cpu *c);   /* pool_4_reset */
void f_0882222C(Cpu *c);   /* pool_6_reset */
void f_08822444(Cpu *c);   /* pool_5_reset */
void f_08823304(Cpu *c);   /* clear the object list */

static void director_clear_78(void) {
    *B(DIRECTOR + 80) = 0; *B(DIRECTOR + 79) = 0; *B(DIRECTOR + 78) = 0; *B(DIRECTOR + 92) = 0;
}

/* Pools 4 and 6 and the object list emptied, and the director's +78 - +80 and +92 cleared. */
SFA_FUNC(088AC3A8, director_clear_pools_4_6) {
    c->r[29] -= 0x10;
    f_08822934(c);
    f_08823304(c);
    f_0882222C(c);
    c->r[4] = DIRECTOR;
    director_clear_78();
    c->r[29] += 0x10;
}

/* Pool 5 emptied, and the same fields cleared. */
SFA_FUNC(088AC3EC, director_clear_pool_5) {
    c->r[29] -= 0x10;
    f_08822444(c);
    c->r[4] = DIRECTOR;
    director_clear_78();
    c->r[29] += 0x10;
}

/* ---- 0.72.0 ---- */

void f_08822EAC(Cpu *c);   /* object_make (kind, type) */
void f_08821368(Cpu *c);   /* a random number */

/* (row, count): count objects made from row `row` of the table at 0x08A2C5CC (three bytes each: kind,
   type, +78), if pool 4 has that many free. */
SFA_FUNC(088A9AD8, director_make_row) {
    int32_t count = (int32_t)ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    if (S8(0x095176F3u) < count) return;
    c->r[29] -= 0x30;
    gaddr e = 0x08A2C5CCu + ARG(0) * 3u;
    int32_t left = count - 1;
    c->r[17] = 1;
    do {
        uint32_t kind = (uint32_t)S8(e) & 0xFF, type = (uint32_t)S8(e + 1) & 0xFF;
        int8_t number = S8(e + 2);
        e += 3;
        c->r[16] = (uint32_t)left; c->r[18] = e; c->r[19] = (uint32_t)(int32_t)number;
        c->r[4] = kind; c->r[5] = type;
        f_08822EAC(c);
        gaddr o = c->r[2];
        *B(o) = 1;
        left--;
        *B(o + 78) = (uint8_t)number;
    } while (left >= 0);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* The s16 at 0x0951CE52 made both bytes its low one; its old high byte to +77 and into the low byte
   of GAME + 22 (as an s8); the second scroll record set to 0x2287 / -893. */
SFA_FUNC(088A9E24, director_take_ce52) {
    gaddr o = ARG(0);
    *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    uint16_t was = *H(0x0951CE52u);
    uint16_t low = was & 0xFF;
    *H(0x0951CE52u) = (uint16_t)(low | (low << 8));
    uint8_t high = (uint8_t)(was >> 8);
    *B(o + 77) = high;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | (int32_t)(int8_t)high);
    *B(o + 76) = 0;
    *H(SCROLL_B + 10) = 0x2287;
    *H(SCROLL_B + 14) = 0xFC83;
}

/* The word at 0x08B027F8 moves by its speed (+8) × 256, kept above 0x2000000 (the object's +17
   one more each time it is held); with GAME + 33's low three bits clear a random number is drawn and
   GAME + 20 bit 0 set. */
SFA_FUNC(088ABA4C, director_grow_27F8) {
    gaddr o = ARG(0), r = 0x08B027F8u, saved = c->r[16];
    c->r[29] -= 0x10;
    *W(r) += (uint32_t)(S16(r + 8) << 8);
    c->r[16] = GAME;
    if (!((int32_t)0x2000000 < (int32_t)*W(r))) {
        *B(o + 17) = (uint8_t)(*B(o + 17) + 1);
        *W(r) = 0x2000000;
    }
    if ((*B(GAME + 33) & 7) == 0) {
        f_08821368(c);
        *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 1);
    }
    c->r[29] += 0x10;
    c->r[16] = saved;
}

void f_08823338(Cpu *c);   /* object_make_list_3 */

/* Every eighth frame (+12) an object of kind 0x31 is made, numbered by +13; +80 is set while GAME + 16's
   high byte is clear. Then the handler by +9 (0x08A2C5F4). */
SFA_FUNC(088A9D58, director_make_every_8th) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 1;
    *B(o + 80) = 1;
    if (S16(GAME + 16) & 0xFF00) *B(o + 80) = 0;
    *B(o + 12) = (uint8_t)((*B(o + 12) + 1) & 7);
    if (*B(o + 12) == 0) {
        c->r[4] = 0x31; c->r[5] = 1;
        f_08822EAC(c);
        gaddr made = c->r[2];
        if (made != 0) {
            *B(made) = 1;
            uint8_t number = *B(o + 13);
            *B(o + 13) = (uint8_t)(number + 1);
            *B(made + 78) = number;
        }
    }
    gaddr fn = *W(0x08A2C5F4u + *B(o + 9) * 4u);
    c->r[4] = o; c->r[5] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* +79 says whether GAME + 16 has a high byte: then, below 24 in its low byte and the first time (+92),
   two objects of kind 7 are made in the third list (types 0 and 1); if one cannot be made nothing
   more is done. Then the handler by +17 (0x08A2C748). */
SFA_FUNC(088AB948, director_make_pair_then_run) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 79) = 0;
    int16_t g = S16(GAME + 16);
    if (g & 0xFF00) {
        if ((g & 0xFF) < 24 && *B(o + 92) == 0) {
            for (uint32_t k = 0; k < 2; k++) {
                c->r[17] = k;
                c->r[4] = 7; c->r[5] = k & 0xFF;
                f_08823338(c);
                c->r[17] = k + 1;
                if (c->r[2] == 0) goto done;
            }
            *B(o + 92) = 1;
        }
        *B(o + 79) = 1;
    }
    {
        gaddr fn = *W(0x08A2C748u + *B(o + 17) * 4u);
        c->r[4] = o; c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
done:
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
