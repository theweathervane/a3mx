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
