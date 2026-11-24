/* Small steps of objects and records (0.83.0): counters, swaps and one-call wrappers that many
 * screens' objects share. Offsets stay numbers until something shows what they are.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

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

/* ---- 0.88.0 ---- */

void f_0880A6CC(Cpu *c);   /* game_text (text/queue.c) */
void f_0880AA74(Cpu *c);   /* game_start_fade_out (task/fade.c) */
void f_08887AFC(Cpu *c);   /* game_objects_3 (lists.c) */
void f_0880B6CC(Cpu *c);   /* game_end_of_frame (lists.c) */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen (frame.c) */
void f_0887D990(Cpu *c);   /* object_set_animation_by_type (motion.c) */
void f_0887DAC8(Cpu *c);   /* steps.c */
void f_088B1D10(Cpu *c);   /* draw_by_game_16 (screen_steps.c) */
void f_088B4AB4(Cpu *c);   /* empty */
void f_08821080(Cpu *c);   /* game_add_score_team (task/fight.c) */
void f_0887A10C(Cpu *c);   /* fighter_script_read */

#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

/* The first fighter, or the second in layout 3 when GAME + 57's bit 0 is clear; also BATTLE + 112. */
SFA_FUNC(08857678, fighter_by_layout_3) {
    uint32_t r = *W(BATTLE + 112);
    if (S16(BATTLE + 56) == 3 && (S8(GAME + 57) & 1) == 0) r = FIGHTER_2;
    RETURN(r);
}

/* (fighter): +208 .. +224 = the five words at its +128. */
SFA_FUNC(08876998, copy_five_from_128) {
    gaddr f = ARG(0), w = *W(f + 128);
    for (int k = 0; k < 5; k++) *W(f + 208 + k * 4) = *W(w + k * 4);
}

/* 0 only when the s8 at 0x08A058A0 + 60 / 62 / 64 are 3 / 1 / 1; otherwise 1. */
SFA_FUNC(08892A58, check_058a0) {
    gaddr t = 0x08A058A0u;
    RETURN(!(S8(t + 60) == 3 && S8(t + 62) == 1 && S8(t + 64) == 1));
}

/* (a, b): 0x09517A50 / 0x09517A28 into *a / *b when GAME + 53 is 2, the other way round otherwise. */
SFA_FUNC(088A1750, pick_7a50_7a28) {
    gaddr a = ARG(0), b = ARG(1);
    int two = S8(GAME + 53) == 2;
    *W(a) = two ? 0x09517A50u : 0x09517A28u;
    *W(b) = two ? 0x09517A28u : 0x09517A50u;
}

/* (object): a flag byte set, then the step table[+3] on the object. */
static void flag_then_step_by_3(Cpu *c, gaddr flag, gaddr table) {
    gaddr o = ARG(0);
    *B(flag) = 1;
    uint32_t fn = *W(table + (uint32_t)*B(o + 3) * 4);
    c->r[29] -= 0x20;
    c->r[5] = fn; c->r[6] = table;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}
SFA_FUNC(088B0248, step_by_3_f21c) { flag_then_step_by_3(c, 0x08B02848u, 0x08A2F21Cu); }
SFA_FUNC(088B3DDC, step_by_3_731c) { flag_then_step_by_3(c, 0x08B02E08u, 0x08A3731Cu); }

/* (object): drawn while GAME + 10 has bit 8 and not bit 9. */
SFA_FUNC(088B0434, draw_on_game_10_100) {
    uint32_t h = *H(GAME + 10) & 0xFF00;
    c->r[29] -= 0x20;
    if (!(h & 0x200) && (h & 0x100)) f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): +8 grows by 0x18000; +128 on when GAME + 16's high byte is set. */
SFA_FUNC(088B2838, grow_8_step_128) {
    gaddr o = ARG(0);
    *W(o + 8) += 0x18000;
    if (S16(GAME + 16) & 0xFF00) *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
}

/* The step 0x08A3AFE8[GAME + 4], given (GAME, BATTLE). */
SFA_FUNC(088BDDA8, step_by_game_4_afe8) {
    uint32_t fn = *W(0x08A3AFE8u + (uint32_t)*H(GAME + 4) * 4);
    c->r[29] -= 0x20;
    c->r[4] = GAME; c->r[5] = BATTLE; c->r[6] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (object): x (+16) moved by the speed +96 (backwards while +11 is 0), the speed changed by +104;
   the new speed. */
static uint32_t move_x_by_facing(gaddr o) {
    uint32_t speed = *W(o + 96);
    *W(o + 16) += *B(o + 11) == 0 ? 0u - speed : speed;
    uint32_t next = speed + *W(o + 104);
    *W(o + 96) = next;
    return next;
}
SFA_FUNC(0890CC04, move_x_by_facing_a) { move_x_by_facing(ARG(0)); }
/* ... and 1 while the new speed is negative. */
SFA_FUNC(0899939C, move_x_by_facing_b) { RETURN((int32_t)move_x_by_facing(ARG(0)) < 0); }

/* (object): +38 from its owner's (+88), then the step table[+4] on it. */
static void step_by_4_after_owner_38(Cpu *c, gaddr table) {
    gaddr o = ARG(0);
    *H(o + 38) = *H(*W(o + 88) + 38);
    uint32_t fn = *W(table + (uint32_t)*B(o + 4) * 4);
    c->r[29] -= 0x20;
    c->r[5] = fn; c->r[6] = (uint32_t)*B(o + 4) << 2;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}
SFA_FUNC(089879F0, step_by_4_477c) { step_by_4_after_owner_38(c, 0x08A9477Cu); }
SFA_FUNC(08987DC8, step_by_4_47c4) { step_by_4_after_owner_38(c, 0x08A947C4u); }

/* (object): the step 0x08AB79CC[GAME + 6], given (GAME, object). */
SFA_FUNC(089DF82C, step_by_game_6) {
    gaddr o = ARG(0);
    uint32_t fn = *W(0x08AB79CCu + (uint32_t)*H(GAME + 6) * 4);
    c->r[29] -= 0x20;
    c->r[4] = GAME; c->r[5] = o; c->r[6] = fn; c->r[7] = 0x08AB79CCu;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (n, x, y): text n of a list (0x08A03CC0 kind 5, or 0x08A03CD4 kind 6) at (x, y). */
static void text_from_list(Cpu *c, gaddr list, uint32_t kind) {
    uint32_t n = ARG(0) & 0xFF;
    int32_t x = (int16_t)ARG(1), y = (int16_t)ARG(2);
    c->r[29] -= 0x30;
    c->r[4] = list + n * 2; c->r[5] = kind;
    c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = 0;
    f_0880A6CC(c);
    c->r[29] += 0x30;
}
SFA_FUNC(0880A7B0, text_from_list_3cc0) { text_from_list(c, 0x08A03CC0u, 5); }
SFA_FUNC(0880A7F4, text_from_list_3cd4) { text_from_list(c, 0x08A03CD4u, 6); }

/* (object, p): +28 = p, +74 .. +77 = p's first four bytes, +152 = 0. */
SFA_FUNC(08821A64, set_28_from_record) {
    gaddr o = ARG(0), p = ARG(1);
    *W(o + 28) = p;
    for (int k = 0; k < 4; k++) *B(o + 74 + k) = *B(p + k);
    *W(o + 152) = 0;
    c->r[5] = 1; c->r[6] = (uint32_t)(int32_t)S8(p + 3);
}

/* (game, x, fighter): unless x's +61 or the fighter's +373, its +452 = +352; in mode 0x12 also
   +478 and +451 = +426. */
SFA_FUNC(08827A94, keep_352_in_452) {
    gaddr g = ARG(0), x = ARG(1), f = ARG(2);
    if (S8(x + 61) != 0 || S8(f + 373) != 0) return;
    *W(f + 452) = *W(f + 352);
    if (S8(g + 114) != 0x12) return;
    int16_t v = S16(f + 426);
    *H(f + 478) = (uint16_t)v;
    *B(f + 451) = (uint8_t)v;
}

/* (object): the step 0x08A12230[+6], then game objects 3 and the end of the frame. */
SFA_FUNC(0883A388, step_by_6_then_frame_end) {
    gaddr o = ARG(0);
    uint32_t fn = *W(0x08A12230u + (uint32_t)*H(o + 6) * 4);
    c->r[29] -= 0x20;
    c->r[6] = fn; c->r[7] = 0x08A12230u;
    rt_call_indirect(c, fn);
    f_08887AFC(c);
    f_0880B6CC(c);
    c->r[29] += 0x20;
}

/* (?, n): four u16 from 0x08A2C6F8 + n * 2 into 0x08B027B0 / B4 and 0x08B02810 / 14. */
SFA_FUNC(088AB1D8, set_27b0_2810) {
    gaddr p = 0x08A2C6F8u + ARG(1) * 2;
    *W(0x08B027B0u) = *H(p);
    *W(0x08B027B4u) = *H(p + 2);
    *W(0x08B02810u) = *H(p + 4);
    *W(0x08B02814u) = *H(p + 6);
}

/* (object): a word counts down; at 0 the fade out starts and a byte moves on. */
static void count_down_then_fade_out(Cpu *c, uint32_t count, uint32_t step) {
    gaddr o = ARG(0), saved = c->r[16];
    uint32_t n = *W(o + count) - 1;
    *W(o + count) = n;
    if (n != 0) return;
    c->r[29] -= 0x10;
    c->r[16] = o; c->r[5] = 0;
    f_0880AA74(c);
    *B(o + step) = (uint8_t)(*B(o + step) + 1);
    c->r[29] += 0x10;
    c->r[16] = saved;
}
SFA_FUNC(088AC2F0, count_84_then_fade_out)   { count_down_then_fade_out(c, 84, 9); }
SFA_FUNC(088B3628, count_12_then_fade_out_a) { count_down_then_fade_out(c, 12, 3); }
SFA_FUNC(088B3BC4, count_12_then_fade_out_b) { count_down_then_fade_out(c, 12, 3); }

/* (k): where k is in the zero-ended bytes at 0x08A3AF8C, or -1. */
SFA_FUNC(088BD2F0, index_in_3af8c) {
    uint32_t k = ARG(0) & 0xFF;
    for (uint32_t i = 0; *B(0x08A3AF8Cu + i) != 0; i++)
        if (*B(0x08A3AF8Cu + i) == k) { RETURN(i); return; }
    RETURN(-1);
}

/* (object, p): +12 / +24 = 0; +14, +26, +60 (word), +64, +76, +66, +78 from p's 16 bytes. */
SFA_FUNC(088EAC84, set_from_16_bytes) {
    gaddr o = ARG(0), p = ARG(1);
    *H(o + 12) = 0; *H(o + 24) = 0;
    *H(o + 14) = *H(p);
    *H(o + 26) = *H(p + 2);
    *W(o + 60) = *W(p + 4);
    *H(o + 64) = *H(p + 8);
    *H(o + 76) = *H(p + 10);
    *H(o + 66) = *H(p + 12);
    *H(o + 78) = *H(p + 14);
}

/* (object): +4 on while +75 is negative; animated, then drawn if on the screen. */
SFA_FUNC(0896A4D4, step_4_on_75_animate_draw) {
    gaddr o = ARG(0), saved = c->r[16];
    if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = o; f_088219B0(c);
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): x (+16) by +96, which drops by 0x10000; once x is -0x680000 or less, +5 on and +82 = 10. */
SFA_FUNC(08855508, move_x_left_until) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 96), x = speed + *W(o + 16);
    *W(o + 16) = x;
    *W(o + 96) = speed - 0x10000;
    if (!((int32_t)0xFF980000u < (int32_t)x)) {
        step_on_5(o);
        *H(o + 82) = 10;
    }
}

/* (object): +9 = 2, its animation by type, then 0x0887DAC8 when it is below its +92 pointer. */
SFA_FUNC(0887D900, animation_by_type_9_2) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 9) = 2;
    c->r[4] = o; f_0887D990(c);
    if (o < *W(o + 92)) { c->r[4] = o; f_0887DAC8(c); }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): drawn while GAME + 10 has no bit 9 and +3 is 4. */
SFA_FUNC(088B0608, draw_on_3_is_4) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (!(*H(GAME + 10) & 0x200) && *B(o + 3) == 4) f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): +100 cleared (set first while GAME + 16's high byte is 0), then 0x088B1D10. */
SFA_FUNC(088B1578, clear_100_then_draw_by_16) {
    gaddr o = ARG(0);
    if (((S16(GAME + 16) & 0xFF00) >> 8) == 0) *W(o + 100) = 1;
    c->r[5] = *W(o + 100);
    *W(o + 100) = 0;
    c->r[29] -= 0x20;
    f_088B1D10(c);
    c->r[29] += 0x20;
}

/* (object): +93 = 1 when GAME + 10's low byte is 2; +128 on while GAME + 16's low byte is not +94. */
SFA_FUNC(088B2930, watch_game_10_16) {
    gaddr o = ARG(0);
    if ((*H(GAME + 10) & 0xFF) == 2) *B(o + 93) = 1;
    if ((S16(GAME + 16) & 0xFF) != S16(o + 94)) *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
}

/* (object): GAME + 22's low byte: bit 1 without bit 2 flips +93; 7 clears it. */
SFA_FUNC(088B2978, flip_93_by_game_22) {
    gaddr o = ARG(0);
    uint32_t v = S16(GAME + 22) & 0xFF;
    if (!(v & 4) && (v & 2)) { *B(o + 93) ^= 1; return; }
    if (v == 7) *B(o + 93) = 0;
}

/* (object): +79 counts; from 2, +4 moves on while GAME + 10's high byte is 0. */
SFA_FUNC(088B4E74, count_79_then_4) {
    gaddr o = ARG(0);
    uint8_t n = (uint8_t)(*B(o + 79) + 1);
    *B(o + 79) = n;
    if (n >= 2 && ((*H(GAME + 10) & 0xFF00) >> 8) == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* (object): +4 = 2 unless the camera's y + 4 is below its y (both as u16). */
SFA_FUNC(088BD0C8, step_4_by_camera_y) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_08821330(c);
    gaddr cam = c->r[2];
    int32_t a = (S16(cam + 22) + 4) & 0xFFFF, b = S16(o + 22) & 0xFFFF;
    if (!(a < b)) *B(o + 4) = 2;
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object): y (+20) by +100; once y is within 0x740000 .. 0x8C0000, +5 on, +22 = +112, +82 = 0x10. */
SFA_FUNC(08856D58, move_y_until_band) {
    gaddr o = ARG(0);
    uint32_t y = *W(o + 20) + *W(o + 100);
    *W(o + 20) = y;
    if (!(0x180000u < y - 0x740000u)) {
        step_on_5(o);
        *H(o + 22) = *H(o + 112);
        *H(o + 82) = 0x10;
    }
}

/* (object): +128 on, +93 = 1, +3 = 0, then the step 0x08A26B3C[+92]. */
SFA_FUNC(0888AAFC, step_by_92) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 3) = 0;
    uint32_t fn = *W(0x08A26B3Cu + (uint32_t)*B(o + 92) * 4);
    c->r[29] -= 0x20;
    c->r[5] = fn; c->r[6] = 0x08A26B3Cu;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (object): once GAME + 16's low byte is 24 or more: +80 / +78 = 0, +17 on, GAME + 20 bit 15, +75 = 0x2D. */
SFA_FUNC(088ABACC, step_17_at_game_16_24) {
    gaddr o = ARG(0);
    if ((S16(GAME + 16) & 0xFF) < 24) return;
    *B(o + 80) = 0;
    *B(o + 78) = 0;
    *B(o + 17) = (uint8_t)(*B(o + 17) + 1);
    *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 0x8000);
    *B(o + 75) = 0x2D;
}

/* (object): +5 on, +96 = +74 (signed), +100 = +28, then 0x088B4AB4 and the swap 0x088B48F0. */
SFA_FUNC(088B4878, step_5_then_swap) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    step_on_5(o);
    *W(o + 96) = (uint32_t)(int32_t)S8(o + 74);
    *W(o + 100) = *W(o + 28);
    c->r[4] = o; f_088B4AB4(c);
    c->r[4] = o; f_088B48F0(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (record, x): +6 on; +22 / +20 / +18 / +10 / +8 = 0; then +8 = 1 when +137 is 0 and x's +79 or
   +155 is set. */
SFA_FUNC(08826A14, next_6_clear) {
    gaddr r = ARG(0), x = ARG(1);
    *H(r + 6) = (uint16_t)(*H(r + 6) + 1);
    *H(r + 22) = 0; *H(r + 20) = 0; *H(r + 18) = 0; *H(r + 10) = 0; *H(r + 8) = 0;
    if (S8(r + 137) == 0 && (S8(x + 79) != 0 || S8(x + 155) != 0)) *H(r + 8) = 1;
}

/* (object): x (+16) moves left by 0x100000 down to 0x700000 (there +5 on, +82 = 0x12C); drawn if on the screen. */
SFA_FUNC(08855420, move_left_to_700000) {
    gaddr o = ARG(0);
    int32_t x = (int32_t)(*W(o + 16) - 0x100000);
    if (x < 0x700000) {
        step_on_5(o);
        *H(o + 82) = 0x12C;
        x = 0x700000;
    }
    *W(o + 16) = (uint32_t)x;
    c->r[29] -= 0x20;
    f_08821424(c);
    c->r[29] += 0x20;
}

/* (object): unless BATTLE + 78, x (+16) by +96 and, while x is not negative, +18 - 0x60; drawn. */
SFA_FUNC(08855934, move_x_unless_battle_78) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 78) == 0) {
        uint32_t x = *W(o + 16) + *W(o + 96);
        *W(o + 16) = x;
        if ((int32_t)x >= 0) *H(o + 18) = (uint16_t)(S16(o + 18) - 0x60);
    }
    c->r[29] -= 0x20;
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): x (+16) moves right by 0x100000; from 0xF00000, +5 on and +74 = 4; drawn. */
SFA_FUNC(08856960, move_right_to_f00000) {
    gaddr o = ARG(0);
    int32_t x = (int32_t)(*W(o + 16) + 0x100000);
    *W(o + 16) = (uint32_t)x;
    if (x >= 0xF00000) {
        step_on_5(o);
        *B(o + 74) = 4;
    }
    c->r[29] -= 0x20;
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (fighter): the opponent (+92), unless it is the computer, scores 0x08A1BE04[+126] (99 at most;
   nothing for 0), as a team score. */
SFA_FUNC(0886D29C, opponent_score_by_126) {
    gaddr f = ARG(0), opp = *W(f + 92);
    if (S8(opp + 373) != 0) return;
    int32_t k = S8(f + 126);
    if (k == 0) return;
    if (k >= 99) k = 99;
    c->r[29] -= 0x20;
    c->r[4] = *W(0x08A1BE04u + (uint32_t)(k * 4)); c->r[5] = opp; c->r[6] = 0x08A1BE04u;
    f_08821080(c);
    c->r[29] += 0x20;
}

/* (fighter, x): the script read with x when +696 is set; otherwise routine 1 started afresh
   (+482 = 0, +483 / +6 / +4 = 1, +7 / +5 = 0) and the answer is x. */
SFA_FUNC(0887763C, script_read_or_restart) {
    gaddr f = ARG(0);
    uint32_t x = ARG(1);
    if (S8(f + 696) != 0) {
        c->r[29] -= 0x20;
        c->r[2] = x;
        f_0887A10C(c);
        c->r[29] += 0x20;
        return;
    }
    RETURN(x);
    c->r[5] = 1;
    *B(f + 482) = 0; *B(f + 483) = 1;
    *B(f + 6) = 1; *B(f + 4) = 1;
    *B(f + 7) = 0; *B(f + 5) = 0;
}

/* (record, ?, fighter): the fighter's +340 on, +341 .. +343 / +346 / +348 / +361 / +412 cleared,
   and its side's bit (+337) cleared in the record's +42. */
SFA_FUNC(08834318, clear_side_bit_42) {
    gaddr r = ARG(0), f = ARG(2);
    *B(f + 340) = (uint8_t)(*B(f + 340) + 1);
    *B(f + 361) = 0;
    *H(f + 348) = 0; *H(f + 346) = 0;
    *B(f + 343) = 0; *B(f + 342) = 0; *B(f + 341) = 0;
    uint32_t bit = 1u << (S8(f + 337) & 31);
    *B(r + 42) = (uint8_t)((uint32_t)(int32_t)S8(r + 42) & (0xFFFFu - bit));
    *B(f + 412) = 0;
}
