/* Small steps of objects and records (0.83.0): counters, swaps and one-call wrappers that many
 * screens' objects share. Offsets stay numbers until something shows what they are.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_08881FB8(Cpu *c);   /* make_beside_jittered (screen_steps.c) */
void f_088232B0(Cpu *c);   /* record_reset (misc.c) */
void f_0882324C(Cpu *c);   /* object_free_42 (pool.c) */
void f_088219B0(Cpu *c);   /* animate */
void f_088215C4(Cpu *c);   /* object_draw */
void f_0880ABD4(Cpu *c);   /* game_start_fade_in (task/fade.c) */
void f_08821368(Cpu *c);   /* a random number */
void f_0882196C(Cpu *c);   /* set_animation */

static void step_on_5(gaddr o) { *B(o + 5) = (uint8_t)(*B(o + 5) + 1); }

/* (object): a kind-15 object of type 0 made beside it, shown, +78 = 0x15. */
SFA_FUNC(08881F70, make_beside_15) {
    c->r[29] -= 0x20;
    c->r[5] = 0; c->r[6] = 1; c->r[7] = 0x15;
    f_08881FB8(c);
    c->r[29] += 0x20;
}

/* (object): +96 / +74 and +100 / +28 swapped (+96 takes +74 sign-extended, +74 the low byte of +96). */
static void swap_96_74_100_28(gaddr o) {
    uint32_t w96 = *W(o + 96), w100 = *W(o + 100);
    int8_t b74 = S8(o + 74);
    *W(o + 96) = (uint32_t)(int32_t)b74;
    uint32_t w28 = *W(o + 28);
    *B(o + 74) = (uint8_t)w96;
    *W(o + 100) = w28;
    *W(o + 28) = w100;
}
SFA_FUNC(088B098C, swap_96_74_100_28_a) { swap_96_74_100_28(ARG(0)); }
SFA_FUNC(088B48F0, swap_96_74_100_28_b) { swap_96_74_100_28(ARG(0)); }

/* (object, owner): at the owner's place (x + its +268) and facing. */
SFA_FUNC(08987D68, place_at_owner_268) {
    gaddr o = ARG(0), w = ARG(1);
    *H(o + 18) = (uint16_t)(S16(w + 18) + S16(w + 268));
    *H(o + 22) = (uint16_t)S16(w + 22);
    *B(o + 11) = *B(w + 11);
}

/* (object): y (+20) moves by +100, which changes by +108; 1 while +100 is negative. */
SFA_FUNC(089993E4, fall_by_100) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 100);
    *W(o + 20) += speed;
    uint32_t next = speed + *W(o + 108);
    *W(o + 100) = next;
    RETURN((int32_t)next < 0);
}

/* (object): +105 = 1, +138 / +140 / +142 = +68 / +70 / +72. */
SFA_FUNC(089DFCA4, copy_68_to_138) {
    gaddr o = ARG(0);
    int16_t a = S16(o + 68), b = S16(o + 70), d = S16(o + 72);
    *B(o + 105) = 1;
    *H(o + 138) = (uint16_t)a; *H(o + 140) = (uint16_t)b; *H(o + 142) = (uint16_t)d;
}

/* (?, record): the s16 at 0x08A0F396 by the record's +55 (8 at most). */
SFA_FUNC(088301F4, value_by_55) {
    int32_t k = S8(ARG(1) + 55);
    if (k >= 8) k = 8;
    RETURN((uint32_t)(int32_t)S16(0x08A0F396u + (uint32_t)(k * 2)));
}

/* (object): +82 counts down (to -1 from 0), and the substep's count (+5) moves on when it was 0. */
SFA_FUNC(08856D30, count_down_82_then_5) {
    gaddr o = ARG(0);
    uint16_t t = *H(o + 82);
    *H(o + 82) = (uint16_t)(t - 1);
    if (t == 0) step_on_5(o);
}

/* (object): the substep's count (+5) on once BATTLE + 44 is 2 or more. */
SFA_FUNC(08856E18, step_5_after_battle_44) { if (S8(BATTLE + 44) >= 2) step_on_5(ARG(0)); }

/* (object): +9 on, +13 = 2, GAME + 20 bit 12 set. */
SFA_FUNC(088AA23C, step_9_report_1000) {
    gaddr o = ARG(0);
    *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    *B(o + 13) = 2;
    *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 0x1000);
}

/* Unless GAME + 93: GAME + 14 cleared and GAME + 4 on. */
static void game_step_unless_93(void) {
    if (S8(GAME + 93) != 0) return;
    uint16_t n = *H(GAME + 4);
    *H(GAME + 14) = 0;
    *H(GAME + 4) = (uint16_t)(n + 1);
}
SFA_FUNC(088AC334, game_step_unless_93_a) { game_step_unless_93(); }
SFA_FUNC(088B366C, game_step_unless_93_b) { game_step_unless_93(); }

/* (object): +88 counts to 21, then starts again and the answer is 0; otherwise 1. */
SFA_FUNC(088AC37C, count_88_to_21) {
    gaddr o = ARG(0);
    int32_t n = (int32_t)(*W(o + 88) + 1);
    *W(o + 88) = (uint32_t)n;
    if (n >= 21) { *W(o + 88) = 0; RETURN(0); return; }
    RETURN(1);
}

/* (object): freed into pool 6 (0x0882324C), its owner's (+88) pointer to it (+40 / +48) cleared. */
static void free_from_owner(Cpu *c, uint32_t at) {
    gaddr o = ARG(0), w = *W(o + 88);
    c->r[29] -= 0x20;
    if (*W(w + at) == o) *W(w + at) = 0;
    c->r[4] = o; c->r[5] = w;
    f_0882324C(c);
    c->r[29] += 0x20;
}
SFA_FUNC(088BDA60, free_from_owner_40) { free_from_owner(c, 40); }
SFA_FUNC(088BDA8C, free_from_owner_48) { free_from_owner(c, 48); }

/* (object): animated, then drawn. */
SFA_FUNC(089CD7D4, animate_then_draw) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = o; f_088219B0(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): +4 on, x 0x12C, and the fade in started. */
SFA_FUNC(089DEC5C, start_fade_in_at_12c) {
    gaddr o = ARG(0);
    *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
    *H(o + 18) = 0x12C;
    c->r[29] -= 0x10;
    c->r[4] = o; f_0880ABD4(c);
    c->r[29] += 0x10;
}

/* (object): +82 counts down; the substep's count (+5) moves on once it goes below 0. */
SFA_FUNC(088815A4, count_down_82_below_0) {
    gaddr o = ARG(0);
    int16_t v = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)v;
    if (v < 0) step_on_5(o);
}

/* (object): +82 counts down; at 0 it stays 0 and the substep's count (+5) moves on. */
SFA_FUNC(088B51D8, count_down_82_to_0) {
    gaddr o = ARG(0);
    int16_t v = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)v;
    if (v == 0) { *H(o + 82) = 0; step_on_5(o); }
}

/* One of the eight s8 at 0x08A3B01C at random. */
SFA_FUNC(088BE36C, random_of_eight) {
    c->r[29] -= 0x10;
    f_08821368(c);
    c->r[29] += 0x10;
    RETURN((uint32_t)(int32_t)S8(0x08A3B01Cu + (c->r[2] & 7)));
}

/* (object): +1 (or +93) set while GAME + 4 is set and GAME + 6 is not. */
static void flag_by_game_4_6(gaddr flag) {
    *B(flag) = 0;
    if (*H(GAME + 4) != 0 && *H(GAME + 6) == 0) *B(flag) = 1;
}
SFA_FUNC(088CEDA0, flag_1_by_game_4_6)  { flag_by_game_4_6(ARG(0) + 1); }
SFA_FUNC(088CFC70, flag_93_by_game_4_6) { flag_by_game_4_6(ARG(0) + 93); }

/* (object): x (+16) moved by the speed +96 (by its complement while +11 is 0, or its negative for
   0x08987474), the speed changed by +104. */
SFA_FUNC(08912FBC, move_x_by_speed_complement) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 96);
    *W(o + 16) += *B(o + 11) == 0 ? ~speed : speed;
    *W(o + 96) = speed + *W(o + 104);
}
SFA_FUNC(08987474, move_x_by_speed_signed) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 96);
    *W(o + 16) += *B(o + 11) == 0 ? 0u - speed : speed;
    *W(o + 96) = speed + *W(o + 104);
}

/* (object, n): animation 0x08A17E58[n]. */
SFA_FUNC(08858D04, set_animation_from_17e58) {
    gaddr o = ARG(0);
    int32_t n = (int16_t)ARG(1);
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = *W(0x08A17E58u + (uint32_t)(n << 2));
    f_0882196C(c);
    c->r[29] += 0x20;
}

/* ---- 0.87.0 ---- */

void f_0880A39C(Cpu *c);   /* fill_092DCA40 */
void f_08822934(Cpu *c);   /* pool_4_reset (pool.c) */
void f_08823304(Cpu *c);   /* game_objects_3_clear (pool.c) */
void f_088AD60C(Cpu *c);   /* record_bob_22 */
void f_08821330(Cpu *c);   /* the camera object */
void f_089DE650(Cpu *c);
void f_089DE6BC(Cpu *c);
void f_089DE754(Cpu *c);
void f_08836E60(Cpu *c);   /* add_to_09524A68 (reset.c) */

#define TASK_DATA 0x09358BE0u   /* the second argument of the 0x08AB6xxx step tables */
#define DIRECTOR  0x08B0274Cu

/* (object): placed by its owner (+88): x - 0xA0, y + 8, the owner's +12 and +13 - 2. */
SFA_FUNC(088BCD70, place_by_owner_left) {
    gaddr o = ARG(0), w = *W(o + 88);
    *H(o + 18) = (uint16_t)(S16(w + 18) - 0xA0);
    *H(o + 22) = (uint16_t)(S16(w + 22) + 8);
    *B(o + 12) = *B(w + 12);
    *B(o + 13) = (uint8_t)(*B(w + 13) - 2);
}

/* (object): +1 set while GAME + 4 is 0 and GAME + 6 is 2. */
SFA_FUNC(088CEE80, flag_1_by_game_4_6_is_2) {
    gaddr o = ARG(0);
    *B(o + 1) = 0;
    if (*H(GAME + 4) == 0 && *H(GAME + 6) == 2) *B(o + 1) = 1;
}

/* (fighter): while +375 is below 16 and BATTLE is 0x1C, +833 = 0 and +236 = 0xF. Answers 1. */
SFA_FUNC(08984604, set_236_on_battle_1c) {
    gaddr f = ARG(0);
    if (S8(f + 375) < 16 && S16(BATTLE) == 0x1C) {
        *B(f + 833) = 0;
        *B(f + 236) = 0xF;
    }
    RETURN(1);
}

/* (object): y (+20) moved by +100, which changes by +108, then x as 0x08987474. */
SFA_FUNC(08987440, move_y_then_x_signed) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 100);
    *W(o + 20) += speed;
    *W(o + 100) = speed + *W(o + 108);
    c->r[29] -= 0x20;
    c->r[4] = o; f_08987474(c);
    c->r[29] += 0x20;
}

/* (object, n): animation n of the list at TASK_DATA + 280. */
SFA_FUNC(089CEF04, set_animation_from_task_list) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1) & 0xFFFF;
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = *W(*W(TASK_DATA + 280) + n * 4);
    f_0882196C(c);
    c->r[29] += 0x20;
}

/* (object): +96, +100, +104 and +108 times 256. */
static void speeds_times_256(gaddr o) {
    for (int k = 0; k < 4; k++) *W(o + 96 + k * 4) <<= 8;
}
SFA_FUNC(089CEF38, speeds_times_256_a) { speeds_times_256(ARG(0)); }
SFA_FUNC(089D7808, speeds_times_256_b) { speeds_times_256(ARG(0)); }

/* (object): 0x089DE6BC, 0x089DE650 and 0x089DE754 on it. */
SFA_FUNC(089DE61C, steps_6bc_650_754) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = o; f_089DE6BC(c);
    c->r[4] = o; f_089DE650(c);
    c->r[4] = o; f_089DE754(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): 0x092DCA40 filled, pool 4 reset, game objects 3 cleared, +2 = 0. */
SFA_FUNC(08817C94, reset_pools_clear_2) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_0880A39C(c);
    f_08822934(c);
    f_08823304(c);
    *B(o + 2) = 0;
    c->r[29] += 0x10;
    c->r[16] = saved;
}

/* (object, x): +20 counts down; below 0, +8 moves on and x's +7 = 0xFF. */
SFA_FUNC(08826844, count_down_20_then_8) {
    gaddr o = ARG(0), x = ARG(1);
    int16_t v = (int16_t)(S16(o + 20) - 1);
    *H(o + 20) = (uint16_t)v;
    if (v < 0) {
        *H(o + 8) = (uint16_t)(*H(o + 8) + 1);
        *B(x + 7) = 0xFF;
    }
}

/* (object): +82 counts down; when it was 0, BATTLE + 44 = 7 and +4 moves on. */
SFA_FUNC(08855550, count_down_82_battle_44_7) {
    gaddr o = ARG(0);
    int16_t t = S16(o + 82);
    *H(o + 82) = (uint16_t)(t - 1);
    if (t == 0) {
        *B(BATTLE + 44) = 7;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
}

/* (fighter): the first time (+660 = 1), BATTLE + 28 / 29 / 30 = 0xF / 8 / 2. */
SFA_FUNC(088748FC, set_battle_28_once) {
    gaddr f = ARG(0);
    if (S8(f + 660) != 0) return;
    *B(f + 660) = 1;
    *B(BATTLE + 28) = 0xF;
    *B(BATTLE + 29) = 8;
    *B(BATTLE + 30) = 2;
}

/* (object): the substep's count (+5) on when 0x088AD60C answers non-zero. */
SFA_FUNC(088AD250, step_5_after_ad60c) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088AD60C(c);
    if (c->r[2] != 0) step_on_5(o);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): y (+22) moves 8 toward the middle: up from 191 or more while +78 is 0, down from below
   65 otherwise. */
SFA_FUNC(088AE200, nudge_y_by_78) {
    gaddr o = ARG(0);
    int32_t y = S16(o + 22);
    if (S8(o + 78) == 0) { if (y >= 191) *H(o + 22) = (uint16_t)(y - 8); }
    else if (y < 65)     *H(o + 22) = (uint16_t)(y + 8);
}

/* (object): when GAME + 10 has any of bits 5..10, it is cleared, +3 moves on and +5 = 0xC. */
SFA_FUNC(088B38C0, step_3_on_game_10) {
    gaddr o = ARG(0);
    if ((*H(GAME + 10) & 0x7E0) == 0) return;
    *H(GAME + 10) = 0;
    *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
    *B(o + 5) = 0xC;
}

/* Pool 4 reset, game objects 3 cleared, 0x08B0284C + 10 / 9 / 8 = 0. */
SFA_FUNC(088B3D94, reset_pools_clear_2854) {
    c->r[29] -= 0x10;
    f_08822934(c);
    f_08823304(c);
    *B(0x08B0284Cu + 10) = 0;
    *B(0x08B0284Cu + 9) = 0;
    *B(0x08B0284Cu + 8) = 0;
    c->r[4] = 0x08B0284Cu;
    c->r[29] += 0x10;
}

/* (object): the 7 pairs at +0x70 = its place (x +18, y +22). */
SFA_FUNC(088B4578, fill_trail_with_place) {
    gaddr o = ARG(0);
    for (int k = 0; k < 7; k++) {
        *H(o + 0x70 + k * 4) = (uint16_t)S16(o + 18);
        *H(o + 0x72 + k * 4) = (uint16_t)S16(o + 22);
    }
}

/* One of the 36 s8 at 0x08A3AFF8 at random. */
SFA_FUNC(088BE334, random_of_36) {
    c->r[29] -= 0x10;
    f_08821368(c);
    DIV(c, c->r[2], 0x24);
    c->r[29] += 0x10;
    RETURN((uint32_t)(int32_t)S8(0x08A3AFF8u + c->hi));
}

/* (object): +11 = 0; 1 when it is right of the camera's x + 0xC0. */
SFA_FUNC(0892EB34, right_of_camera) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_08821330(c);
    gaddr cam = c->r[2];
    *B(o + 11) = 0;
    RETURN(S16(cam + 18) + 0xC0 < S16(o + 18));
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): the step of the table by +4, given (object, TASK_DATA). */
static void step_by_4(Cpu *c, gaddr table) {
    gaddr o = ARG(0);
    uint32_t fn = *W(table + (uint32_t)*B(o + 4) * 4);
    c->r[29] -= 0x20;
    c->r[6] = fn; c->r[5] = TASK_DATA;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}
SFA_FUNC(089CD248, step_by_4_6148) { step_by_4(c, 0x08AB6148u); }
SFA_FUNC(089CF574, step_by_4_6348) { step_by_4(c, 0x08AB6348u); }
SFA_FUNC(089D9994, step_by_4_69f0) { step_by_4(c, 0x08AB69F0u); }

/* The step 0x08A05864[BATTLE + 56]. */
SFA_FUNC(08812F90, step_by_battle_56) {
    uint32_t fn = *W(0x08A05864u + (uint32_t)((int32_t)S16(BATTLE + 56) << 2));
    c->r[29] -= 0x10;
    c->r[4] = fn; c->r[5] = 0x08A05864u;
    rt_call_indirect(c, fn);
    c->r[29] += 0x10;
}

/* (n): n in binary-coded decimal, a digit to each 4 bits. */
SFA_FUNC(088212F4, to_bcd) {
    uint32_t n = ARG(0), out = 0, shift = 0;
    do {
        DIV(c, n, 10);
        out += c->hi << (shift & 31);
        n = c->lo;
        shift += 4;
    } while (n != 0);
    c->r[4] = n; c->r[5] = 10; c->r[6] = shift;
    RETURN(out);
}

/* (record, k): bit k set (bits 0..31 in +400, the ones after in +404). */
SFA_FUNC(08830348, set_bit_400) {
    gaddr r = ARG(0);
    int32_t k = (int32_t)ARG(1);
    if (k < 32) *W(r + 400) |= 1u << (k & 31);
    else        *H(r + 404) = (uint16_t)(*H(r + 404) | (1u << ((k - 32) & 31)));
}

/* (object): x (+16) moved by +96; +4 moves on once x + 0x200000 is past 0x1C00000. */
SFA_FUNC(08856E44, move_x_until_1c00000) {
    gaddr o = ARG(0);
    uint32_t x = *W(o + 16) + *W(o + 96);
    *W(o + 16) = x;
    if ((int32_t)0x1C00000 < (int32_t)(x + 0x200000)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* (object, ?, x): +82 counts down; when it was 0: +4 on, +5 = 0, +424 bit 5 set, x's +1 = 0. */
SFA_FUNC(088A27CC, count_down_82_set_424) {
    gaddr o = ARG(0), x = ARG(2);
    int16_t t = S16(o + 82);
    *H(o + 82) = (uint16_t)(t - 1);
    if (t != 0) return;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 5) = 0;
    *H(o + 424) = (uint16_t)(*H(o + 424) | 0x20);
    *B(x + 1) = 0;
}

/* The director's step from a table by one of its fields. */
static void director_step(Cpu *c, gaddr table, uint32_t k) {
    uint32_t fn = *W(table + k * 4);
    c->r[29] -= 0x20;
    c->r[4] = DIRECTOR; c->r[5] = fn; c->r[6] = table;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}
SFA_FUNC(088A9F54, director_step_by_8_c608)  { director_step(c, 0x08A2C608u, *B(DIRECTOR + 8)); }
SFA_FUNC(088AAA28, director_step_by_2_c69c)  { director_step(c, 0x08A2C69Cu, *H(DIRECTOR + 2)); }
SFA_FUNC(088AB438, director_step_by_16_c720) { director_step(c, 0x08A2C720u, *B(DIRECTOR + 16)); }
SFA_FUNC(088AC020, director_step_by_8_c780)  { director_step(c, 0x08A2C780u, *B(DIRECTOR + 8)); }

/* The step 0x08A36EF0[GAME + 4]. */
SFA_FUNC(088B2E2C, step_by_game_4) {
    uint32_t fn = *W(0x08A36EF0u + (uint32_t)*H(GAME + 4) * 4);
    c->r[29] -= 0x10;
    c->r[4] = fn; c->r[5] = 0x08A36EF0u;
    rt_call_indirect(c, fn);
    c->r[29] += 0x10;
}

/* (object): +7 on, speeds +96 / +100 / +104 / +108 = 0x40000 / 0x28000 / 0 / -0xA000, +73 = 1. */
SFA_FUNC(0890A19C, start_jump_speeds) {
    gaddr o = ARG(0);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *W(o + 96) = 0x40000;
    *W(o + 100) = 0x28000;
    *W(o + 104) = 0;
    *W(o + 108) = 0xFFFF6000u;
    *B(o + 73) = 1;
}

/* (object, owner): +4 on, +24 / +12 / +13 / +14 from the owner, +3 = the owner's +338, +127 = 0. */
SFA_FUNC(08913098, copy_from_owner_24) {
    gaddr o = ARG(0), w = ARG(1);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(w + 24);
    *B(o + 3) = *B(w + 338);
    *B(o + 12) = *B(w + 12);
    *B(o + 13) = *B(w + 13);
    *B(o + 127) = 0;
    *B(o + 14) = *B(w + 14);
}

/* (object): y (+20) by +100 (changed by +108), x (+16) by +96 (changed by +104). */
SFA_FUNC(08987404, move_x_y_by_speeds) {
    gaddr o = ARG(0);
    uint32_t vy = *W(o + 100), vx = *W(o + 96);
    *W(o + 20) += vy; *W(o + 100) = vy + *W(o + 108);
    *W(o + 16) += vx; *W(o + 96) = vx + *W(o + 104);
}

/* (object, n): +100 = n; below the table's size, +84 = table[n] and +148 = 3; +74 = 0xFF. */
static void set_100_from_table(gaddr o, int32_t n, int32_t size, gaddr table) {
    *H(o + 100) = (uint16_t)n;
    if (n < size) {
        *H(o + 84) = *H(table + (uint32_t)(n * 2));
        *W(o + 148) = 3;
    }
    *B(o + 74) = 0xFF;
}
SFA_FUNC(08987D8C, set_100_from_2e30) { set_100_from_table(ARG(0), (int32_t)ARG(1), 345, 0x08A92E30u); }
SFA_FUNC(089880E4, set_100_from_30e2) { set_100_from_table(ARG(0), (int32_t)ARG(1), 346, 0x08A930E2u); }

/* (object, x): +16 counts down; below 0, +4 moves on, x's +81 = 0 and +16 = 0x78. */
SFA_FUNC(089DF0BC, count_down_16_then_4) {
    gaddr o = ARG(0), x = ARG(1);
    int16_t v = (int16_t)(S16(o + 16) - 1);
    *H(o + 16) = (uint16_t)v;
    if (v < 0) {
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        *B(x + 81) = 0;
        *H(o + 16) = 0x78;
    }
}

/* (object): 1 when the pad word (0x08E75BA0 + 12 when +53 is 1, + 56 otherwise) has a mask bit. */
static void pad_has(Cpu *c, uint32_t mask) {
    gaddr pad = 0x08E75BA0u + (S8(ARG(0) + 53) == 1 ? 12 : 56);
    uint32_t v = *W(pad) & 0xFFFF;
    c->r[4] = v;
    RETURN((v & mask) != 0);
}
SFA_FUNC(089DF1E0, pad_has_83f0) { pad_has(c, 0x83F0); }
SFA_FUNC(089E04B0, pad_has_8000) { pad_has(c, 0x8000); }

/* (?, object): the step 0x08AB79F0[bits 8..11 of +66] on the object. */
SFA_FUNC(089DFA0C, step_by_66) {
    gaddr o = ARG(1);
    uint32_t fn = *W(0x08AB79F0u + (uint32_t)((S16(o + 66) >> 8) & 0xF) * 4);
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[6] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (object): +100 = 1, +156 / +150 / +152 / +154 = +68 / +70 / +72 / +74, +103 = the top 4 bits of +66. */
SFA_FUNC(089DFD04, copy_68_to_150) {
    gaddr o = ARG(0);
    *B(o + 100) = 1;
    *H(o + 156) = *H(o + 68);
    *H(o + 150) = *H(o + 70);
    *H(o + 152) = *H(o + 72);
    *H(o + 154) = *H(o + 74);
    *B(o + 103) = (uint8_t)((*H(o + 66) & 0xF000) >> 12);
}

/* The first free of the 8 slots of 20 at 0x095177C8, marked used (or 0 when none is free). */
SFA_FUNC(088230B4, slot_7c8_take) {
    for (int k = 0; k < 8; k++) {
        gaddr s = 0x095177C8u + (uint32_t)k * 20;
        if (S8(s) == 0) { *B(s) = 1; c->r[4] = 1; RETURN(s); return; }
    }
    RETURN(0);
}

/* (fighter): unless GAME + 57 is 3 or the fighter's +373 is set, 1 added by 0x08836E60. */
SFA_FUNC(08837058, add_one_unless_373) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    if (S8(GAME + 57) != 3 && S8(f + 373) == 0) { c->r[4] = 1; f_08836E60(c); }
    c->r[29] += 0x20;
}

/* (object): +74 counts down; at 0 the substep's count (+5) moves on; then drawn. */
SFA_FUNC(088569B0, count_down_74_then_draw) {
    gaddr o = ARG(0);
    int8_t v = (int8_t)(S8(o + 74) - 1);
    *B(o + 74) = (uint8_t)v;
    if (v == 0) step_on_5(o);
    c->r[29] -= 0x20;
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
}
