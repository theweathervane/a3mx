/* Small steps of screen objects, fighters and records (0.103.0): counters, table steps, places taken
 * from other objects, and one-call wrappers. Offsets stay numbers until something shows what they are.
 * Registers a callee could read (its arguments, and what the original leaves in a0 - a3 / t0 at a call
 * through a table) are set as the original sets them.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

void f_0899939C(Cpu *c);   /* move_x_by_facing_b (small_steps.c) */
void f_089993E4(Cpu *c);   /* fall_by_100 (small_steps.c) */
void f_088AB1D8(Cpu *c);   /* set_27b0_2810 (small_steps.c) */
void f_08855934(Cpu *c);   /* move_x_unless_battle_78 (small_steps.c) */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_08837690(Cpu *c);   /* game_objects_1 */
void f_088379B8(Cpu *c);   /* game_objects_2 */
void f_08887AFC(Cpu *c);   /* game_objects_3 */
void f_0880B6CC(Cpu *c);   /* game_end_of_frame */
void f_08877258(Cpu *c);   /* fighter_chance_by_level */
void f_0887A188(Cpu *c);   /* fighter_start_random_script */
void f_0887A1D8(Cpu *c);   /* fighter_script_restart */
void f_088774A8(Cpu *c);   /* fighter_script_step */
void f_0887A400(Cpu *c);   /* step_0887A400 */
void f_0887B308(Cpu *c);   /* fighter_pick_script_by_opponent */
void f_08862DF8(Cpu *c);   /* fighter_keep_on_screen */
void f_0888BB24(Cpu *c);   /* bottom_text */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_0882324C(Cpu *c);   /* object_free_42 */
void f_0881DF10(Cpu *c);   /* game_fight_effects */

/* ---------------------------------------------------------------- one-call wrappers */

/* (object): moved by its facing (0x0899939C), then falls by +100 (0x089993E4); the answer is the fall's. */
SFA_FUNC(08999370, move_then_fall) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_0899939C(c);
    c->r[4] = o;
    f_089993E4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): moved by its facing; once +96 is negative, +96 / +104 / +262 cleared and +7 moves on. */
SFA_FUNC(0899544C, move_until_96_negative) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_0899939C(c);
    c->r[4] = *W(o + 96);
    if ((int32_t)*W(o + 96) < 0) {
        *W(o + 96) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 104) = 0;
        *B(o + 262) = 0;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +9 moves on, +13 = 0x2E, and 0x088AB1D8 with entry 0. */
SFA_FUNC(088AB120, next_9_set_27b0) {
    gaddr o = ARG(0);
    *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    *B(o + 13) = 0x2E;
    c->r[29] -= 0x20;
    c->r[5] = 0; c->r[6] = 0x2E;
    f_088AB1D8(c);
    c->r[29] += 0x20;
}

/* (object): +82 counts down; at 0 (before the count), +6 moves on and BATTLE + 44 = 4. Then moved (0x08855934). */
SFA_FUNC(08855874, count_82_then_move) {
    gaddr o = ARG(0);
    int16_t t = S16(o + 82);
    *H(o + 82) = (uint16_t)(t - 1);
    c->r[7] = (uint32_t)(int32_t)t - 1;
    if (t != 0) { c->r[5] = o + 82; c->r[6] = (uint32_t)(int32_t)t; }
    else {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *B(BATTLE + 44) = 4;
        c->r[5] = BATTLE; c->r[6] = 4;
    }
    c->r[29] -= 0x20;
    f_08855934(c);
    c->r[29] += 0x20;
}

/* (object, n): animation 0x08A396A8[n] started, +86 = 0x16, +164 = 1, +156 = 0x08A392CC, +148 = 6. */
SFA_FUNC(088BD44C, start_anim_96a8) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1) & 0xFFFF;
    *H(o + 86) = 0x16;
    *W(o + 164) = 1;
    *W(o + 156) = 0x08A392CCu;
    *W(o + 148) = 6;
    c->r[29] -= 0x20;
    c->r[5] = *W(0x08A396A8u + n * 4); c->r[6] = 0x08A396A8u; c->r[7] = 6;
    f_0882196C(c);
    c->r[29] += 0x20;
}

/* (fighter, x): a chance by level from 0x08A227A0; taken, the script restarted (0x0887A1D8), otherwise one
   started at random (0x0887A188). */
SFA_FUNC(0887A2A4, fighter_restart_or_random) {
    gaddr f = ARG(0);
    uint32_t x = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = x; c->r[17] = f;
    c->r[5] = 0x08A227A0u;
    f_08877258(c);
    c->r[4] = f; c->r[5] = x;
    if (c->r[2] != 0) f_0887A1D8(c);
    else f_0887A188(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): the CPU's first script (+480) and first command (+481) the first time, then the step
   by +482 (0x0887A400). */
SFA_FUNC(088773E8, fighter_cpu_script_frame) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (*B(f + 480) == 0) {
        *B(f + 480) = 1;
        c->r[4] = f;
        f_0887B308(c);
    }
    if (*B(f + 481) == 0) {
        uint32_t at = *W(f + 520);
        *B(f + 481) = 1;
        *W(f + 528) = at;
        c->r[4] = f; c->r[5] = at;
        f_088774A8(c);
    }
    c->r[4] = f;
    f_0887A400(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): the character's routine +96 (a human) or +100 (the CPU), then kept on the screen unless
   +135 is set. */
SFA_FUNC(0885EACC, fighter_routine_by_cpu) {
    gaddr f = ARG(0), s16 = c->r[16];
    gaddr table = *W(f + 416);
    uint32_t fn = *W(table + (S8(f + 373) != 0 ? 100 : 96));
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[4] = f; c->r[5] = fn;
    rt_call_indirect(c, fn);
    c->r[4] = (uint32_t)(int32_t)S8(f + 135);
    if (S8(f + 135) == 0) { c->r[4] = f; f_08862DF8(c); }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- steps from a table */

/* (object): the step 0x08A16DA8[+4]; while GAME + 38 is set, +4 = 2 after it. */
SFA_FUNC(08855268, step_by_4_6da8) {
    gaddr o = ARG(0), s16 = c->r[16];
    uint32_t fn = *W(0x08A16DA8u + (uint32_t)*B(o + 4) * 4);
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = fn;
    rt_call_indirect(c, fn);
    if (S8(GAME + 38) != 0) *B(o + 4) = 2;
    c->r[4] = 2;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): when BATTLE + 44 is 5, +5 = 2 and y (+18) = 0xF0; then the step 0x08A16F14[+5]. */
SFA_FUNC(08856908, step_by_5_6f14) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 44) == 5) { *B(o + 5) = 2; *H(o + 18) = 0xF0; }
    uint32_t fn = *W(0x08A16F14u + (uint32_t)*B(o + 5) * 4);
    c->r[29] -= 0x20;
    c->r[5] = fn; c->r[6] = 0x08A16F14u;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (object): the step by +5 from 0x08A24B5C; afterwards, if it still follows one (+88), that one's +10
   takes this one's y (+22). The one it followed before the step is the one written. */
SFA_FUNC(08881544, step_by_5_4b5c_follow) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    uint32_t fn = *W(0x08A24B5Cu + (uint32_t)*B(o + 5) * 4);
    gaddr was = *W(o + 88);
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = was;
    c->r[5] = fn;
    rt_call_indirect(c, fn);
    if (*W(o + 88) != 0) *H(was + 10) = (uint16_t)S16(o + 22);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): when GAME + 20 is 4, the bottom text (0x0888BB24, all three lines); then the step by +3. */
static void bottom_text_then_step(Cpu *c, gaddr table) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 4; c->r[6] = (uint32_t)(int32_t)S16(GAME + 20);
    if (S16(GAME + 20) == 4) {
        c->r[4] = 0;
        f_0888BB24(c);
    }
    uint32_t fn = *W(table + (uint32_t)*B(o + 3) * 4);
    c->r[4] = o; c->r[5] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(088B3348, bottom_text_step_6f24) { bottom_text_then_step(c, 0x08A36F24u); }
SFA_FUNC(088B376C, bottom_text_step_6f40) { bottom_text_then_step(c, 0x08A36F40u); }

/* (object): freed (0x088231C8 / 0x0882324C) once GAME + 4 is 0; until then the step by +3 from the table. */
static void step_or_free(Cpu *c, gaddr table, void (*free_fn)(Cpu *)) {
    gaddr o = ARG(0);
    uint32_t screen = *H(GAME + 4);
    c->r[29] -= 0x20;
    if (screen == 0) {
        c->r[5] = 0;
        free_fn(c);
    } else {
        uint32_t fn = *W(table + (uint32_t)*B(o + 3) * 4);
        c->r[5] = fn; c->r[6] = table;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
}
SFA_FUNC(088BC45C, step_or_free_32_aef8) { step_or_free(c, 0x08A3AEF8u, f_088231C8); }
SFA_FUNC(088BD7D8, step_or_free_42_afc0) { step_or_free(c, 0x08A3AFC0u, f_0882324C); }

/* (record): the step 0x08AB76D8[+4], then object lists 1 - 3 and the end of the frame. */
SFA_FUNC(089DEF3C, step_76d8_then_frame) {
    gaddr r = ARG(0);
    uint32_t fn = *W(0x08AB76D8u + (uint32_t)*H(r + 4) * 4);
    c->r[29] -= 0x20;
    c->r[6] = fn; c->r[7] = 0x08AB76D8u;
    rt_call_indirect(c, fn);
    f_08837690(c);
    f_088379B8(c);
    f_08887AFC(c);
    f_0880B6CC(c);
    c->r[29] += 0x20;
}

/* (record): the step 0x08AB76BC[+4]; unless +4 is then 5, object list 1 and the end of the frame. */
SFA_FUNC(089DEB88, step_76bc_then_frame) {
    gaddr r = ARG(0), s16 = c->r[16];
    uint32_t fn = *W(0x08AB76BCu + (uint32_t)*H(r + 4) * 4);
    c->r[29] -= 0x20;
    c->r[16] = r;
    c->r[6] = fn;
    rt_call_indirect(c, fn);
    uint32_t k = *H(r + 4);
    c->r[4] = k; c->r[5] = 5;
    if (k != 5) {
        f_08837690(c);
        f_0880B6CC(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (?, record): the routine 0x08AB7A28[bits 8 - 11 of +66], given the record and its entry
   (+170 + 18 * bits 12 - 15). */
SFA_FUNC(089DFD68, routine_by_66) {
    gaddr p = ARG(1);
    int32_t v = S16(p + 66);
    uint32_t k = (uint32_t)((v & 0xF000) >> 12), i = (uint32_t)((v & 0xF00) >> 8);
    uint32_t fn = *W(0x08AB7A28u + i * 4);
    gaddr entry = p + k * 18 + 0xAA;
    c->r[29] -= 0x20;
    c->r[4] = p; c->r[5] = entry; c->r[6] = fn; c->r[7] = entry; c->r[8] = k * 17;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (record): +4 moves on; +16 = 1200, +6 - +22's halfwords cleared; the fight's effects and object list
   2 run; the camera layer 0x092E88A0's +123 = 15. */
SFA_FUNC(088BDFBC, reset_and_effects) {
    gaddr r = ARG(0);
    uint32_t n = (uint32_t)*H(r + 4) + 1;
    *H(r + 4) = (uint16_t)n;
    *H(r + 16) = 0x4B0;
    *H(r + 22) = 0; *H(r + 20) = 0; *H(r + 18) = 0;
    *H(r + 10) = 0; *H(r + 8) = 0; *H(r + 6) = 0;
    c->r[29] -= 0x10;
    c->r[5] = n; c->r[6] = 0x4B0;
    f_0881DF10(c);
    f_088379B8(c);
    *B(0x092E88A0u + 123) = 15;
    c->r[29] += 0x10;
}

/* ---------------------------------------------------------------- objects' fields */

/* (?): the CPU's search for BATTLE + 22 in 0x08A13867 (it is always there), then 0x0950FF7C and
   GAME + 48 = 2. */
SFA_FUNC(0884A210, mark_2_after_search) {
    uint32_t want = (uint32_t)(int32_t)S8(BATTLE + 22);
    for (uint32_t i = 0; want != *B(0x08A13867u + i); i++) { }
    *B(0x0950FF60u + 28) = 2;
    *B(GAME + 48) = 2;
}

/* (fighter, p): a script word at p: +490 takes it (and +192 / +193 its bytes), routine 1.2 starts
   (+4 = 1, +5 = 2, +6 / +7 = 0), +482 = 7, +483 / +123 / +696 = 0, +11 = +697 (the opponent's side). The
   answer is p + 2. */
SFA_FUNC(0887A0B8, fighter_routine_from_word) {
    gaddr f = ARG(0), p = ARG(1);
    *H(f + 490) = (uint16_t)S16(p);
    *B(f + 482) = 7;
    *B(f + 483) = 0;
    *B(f + 4) = 1;
    *B(f + 5) = 2;
    *B(f + 7) = 0;
    *B(f + 6) = 0;
    *B(f + 11) = (uint8_t)S8(f + 697);
    *B(f + 696) = 0;
    *B(f + 123) = 0;
    *B(f + 193) = (uint8_t)S8(f + 491);
    *B(f + 192) = (uint8_t)S8(f + 490);
    RETURN(p + 2);
}

/* (object): +13 counts down; at 0, +9 moves on, +13 = 4 and GAME + 10's low byte = 1. */
SFA_FUNC(088AA268, count_13_then_game_10) {
    gaddr o = ARG(0);
    uint8_t t = (uint8_t)(*B(o + 13) - 1);
    *B(o + 13) = t;
    if (t != 0) return;
    *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    *B(o + 13) = 4;
    *H(GAME + 10) = (uint16_t)(*H(GAME + 10) & 0xFF00);
    *H(GAME + 10) = (uint16_t)(*H(GAME + 10) | 1);
}

/* (object): +4 moves on; set up as kind 0x1B with +148 = 13; facing (+11) = +3. */
SFA_FUNC(088B0BCC, setup_1b) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 13;
    *H(o + 86) = 0x1B;
    *W(o + 164) = 0;
    *B(o + 12) = 1;
    *H(o + 16) = 0; *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 9) = 0; *B(o + 13) = 0;
    *H(o + 24) = 3;
    *B(o + 11) = *B(o + 3);
}

/* (fighter, other): when they are the same character, or 0x15 and 0x25 either way round, and their
   +376 are equal, the other's +376 flips. */
SFA_FUNC(088BE788, flip_376_if_same) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ca = *B(a + 338), cb = *B(b + 338);
    if (!(ca == cb || (ca == 0x15 && cb == 0x25) || (ca == 0x25 && cb == 0x15))) return;
    int8_t mine = S8(a + 376), theirs = S8(b + 376);
    if (mine == theirs) *B(b + 376) = (uint8_t)(theirs ^ 1);
}

/* (object): shown (+1) while the 0x354-byte record by +78 has its first byte set, at its owner's
   (+88) place, 0x72 right and 8 up. */
SFA_FUNC(088CEABC, show_by_record_78) {
    gaddr o = ARG(0);
    int64_t m = (int64_t)S8(o + 78) * 0x354;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr w = *W(o + 88);
    *B(o + 1) = 0;
    if (S8(0x09519588u + c->lo) == 0) return;
    *B(o + 1) = 1;
    *H(o + 18) = (uint16_t)(S16(w + 18) + 0x72);
    *H(o + 22) = (uint16_t)(S16(w + 22) - 8);
}

/* (object): +280 counts down while it is set: 1 then. When the owner's (+88) +539 is set instead,
   the object goes to routine 3 (+4 = 3, +5 - +7 = 0). 0 otherwise. */
SFA_FUNC(088378F8, count_280_or_routine_3) {
    gaddr o = ARG(0);
    int8_t n = S8(o + 280);
    RETURN(0);
    if (n == 0) return;
    gaddr w = *W(o + 88);
    if (w != 0 && S8(w + 539) != 0) {
        *B(o + 4) = 3; *B(o + 7) = 0; *B(o + 6) = 0; *B(o + 5) = 0;
        return;
    }
    *B(o + 280) = (uint8_t)(n - 1);
    RETURN(1);
}

/* (object): its place from the table at 0x08A2F2CC by +3 and the byte at 0x08B02848. */
SFA_FUNC(088B0A50, place_from_f2cc) {
    gaddr o = ARG(0);
    int16_t i = (int16_t)((int16_t)(*B(o + 3) * 4) + (int32_t)*B(0x08B02848u) * 2);
    gaddr p = 0x08A2F2CCu + (uint32_t)((int32_t)i * 2);
    *H(o + 18) = (uint16_t)S16(p);
    *H(o + 22) = (uint16_t)S16(p + 2);
}

/* (object): its place from the table at 0x08A376C2 by +3 and the signed byte at 0x08B02E08. */
SFA_FUNC(088B53F4, place_from_76c2) {
    gaddr o = ARG(0);
    int16_t i = (int16_t)((int16_t)(*B(o + 3) * 4) + (int32_t)S8(0x08B02E08u) * 2);
    gaddr p = 0x08A376C2u + (uint32_t)((int32_t)i * 2);
    *H(o + 18) = (uint16_t)S16(p);
    *H(o + 22) = (uint16_t)S16(p + 2);
}

/* (object): +98 counts up; from 5, the colours +36 - +39 = 0xFF, +42 - +45 / +48 - +51 = 0, and +128
   moves on (each frame after). */
SFA_FUNC(088B5B2C, count_98_then_white) {
    gaddr o = ARG(0);
    uint32_t n = *H(o + 98);
    if ((int32_t)n >= 5) {
        for (int k = 36; k <= 39; k++) *B(o + k) = 0xFF;
        for (int k = 42; k <= 45; k++) *B(o + k) = 0;
        for (int k = 48; k <= 51; k++) *B(o + k) = 0;
        *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    }
    *H(o + 98) = (uint16_t)(n + 1);
}

/* (object): x (+18) moves by +98; while it is past the limit +76:+77 (a big-endian u16), it is taken
   back by that much. Then animated. */
SFA_FUNC(088BCAFC, move_98_wrap_right) {
    gaddr o = ARG(0);
    int16_t step = S16(o + 98);
    uint32_t sum = (uint32_t)((int32_t)S16(o + 18) + step);
    *H(o + 18) = (uint16_t)sum;
    c->r[29] -= 0x20;
    *B(c->r[29] + 17) = *B(o + 76);
    *B(c->r[29] + 16) = *B(o + 77);
    uint32_t lim = *H(c->r[29] + 16);
    int32_t x = S16(o + 18);
    int less = (int32_t)lim < x;
    if (less) *H(o + 18) = (uint16_t)((uint32_t)x - lim);
    c->r[5] = (uint32_t)x - lim; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)less;
    f_088219B0(c);
    c->r[29] += 0x20;
}

/* (object): x (+18) moves by +98; while it is negative, the limit +76:+77 (a big-endian u16) is added.
   Then animated. */
SFA_FUNC(088BCC2C, move_98_wrap_left) {
    gaddr o = ARG(0);
    int16_t step = S16(o + 98);
    uint32_t sum = (uint32_t)((int32_t)S16(o + 18) + step);
    *H(o + 18) = (uint16_t)sum;
    int32_t x = S16(o + 18);
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)x; c->r[6] = sum; c->r[7] = (uint32_t)(int32_t)step;
    if (x < 0) {
        int8_t lo76 = S8(o + 76), lo77 = S8(o + 77);
        *B(c->r[29] + 17) = (uint8_t)lo76;
        *B(c->r[29] + 16) = (uint8_t)lo77;
        uint32_t lim = *H(c->r[29] + 16);
        *H(o + 18) = (uint16_t)((uint32_t)x + lim);
        c->r[5] = (uint32_t)x + lim; c->r[6] = lim; c->r[7] = (uint32_t)(int32_t)lo77;
    }
    f_088219B0(c);
    c->r[29] += 0x20;
}

/* (object): +36 - +39 / +42 - +45 / +48 - +51 and +136 = 0xFF; +93 / +89 / +137 / +138 = 1, +3 = 0. */
SFA_FUNC(088EACC8, setup_white_ff) {
    gaddr o = ARG(0);
    *B(o + 93) = 1;
    *B(o + 3) = 0;
    *B(o + 89) = 1;
    *B(o + 137) = 1;
    *B(o + 136) = 0xFF;
    for (int k = 36; k <= 39; k++) *B(o + k) = 0xFF;
    for (int k = 42; k <= 45; k++) *B(o + k) = 0xFF;
    for (int k = 48; k <= 51; k++) *B(o + k) = 0xFF;
    *B(o + 138) = 1;
}

/* (fighter): x (+16) moved by 4 pixels: the CPU towards the opponent (+92), a human by its held
   directions (+362: bit 1 back, bit 0 forward; neither, no move). */
SFA_FUNC(08987118, fighter_nudge_x) {
    gaddr f = ARG(0);
    uint32_t d;
    if (S8(f + 373) != 0) {
        gaddr opp = *W(f + 92);
        d = S16(opp + 18) < S16(f + 18) ? 0xFFFC0000u : 0x40000u;
    } else {
        uint32_t held = *H(f + 362);
        if ((held & 3) == 0) return;
        d = (held & 2) ? 0xFFFC0000u : 0x40000u;
    }
    *W(f + 16) += d;
}

/* (fighter): its frame's record (+28): +296 = +356 + 12 * the record's +8; +74 - +77 its first four
   bytes; +256 / +204 = +200 + 4 * the first byte at +296. */
SFA_FUNC(08876894, fighter_frame_record) {
    gaddr f = ARG(0);
    gaddr r = *W(f + 28);
    gaddr e = *W(f + 356) + (uint32_t)*H(r + 8) * 12;
    *W(f + 296) = e;
    *B(f + 74) = *B(r);
    *B(f + 75) = *B(r + 1);
    *B(f + 76) = *B(r + 2);
    *B(f + 77) = *B(r + 3);
    gaddr q = *W(f + 200) + (uint32_t)*B(e) * 4;
    *W(f + 256) = q;
    *W(f + 204) = q;
}

/* (object, kind, x, y): set up: +9 the kind, +24 = 7, +86 = 8, x / y (+16 / +20) in whole pixels,
   +11 - +15 / +38 / +148 cleared. */
SFA_FUNC(08881D9C, setup_kind_at) {
    gaddr o = ARG(0);
    *B(o + 9) = (uint8_t)ARG(1);
    *H(o + 24) = 7;
    *B(o + 13) = 0; *B(o + 12) = 0;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *W(o + 16) = (uint32_t)(int32_t)(int16_t)ARG(2) << 16;
    *W(o + 20) = (uint32_t)(int32_t)(int16_t)ARG(3) << 16;
    *H(o + 86) = 8;
    *W(o + 148) = 0;
}

/* (object): y (+20) moves by +100, which moves by +108; at the limit +88 the speed takes the limit's
   value, and the acceleration and the limit turn round (a bounce). */
SFA_FUNC(089CEFC8, bounce_by_88) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 100), acc = *W(o + 108);
    *W(o + 20) += speed;
    uint32_t next = speed + acc;
    *W(o + 100) = next;
    uint32_t lim = *W(o + 88), a = next, b = lim;
    if ((int32_t)acc < 0) { a = 0u - next; b = 0u - lim; }
    if ((int32_t)a < (int32_t)b) return;
    *W(o + 100) = lim;
    *W(o + 108) = 0u - acc;
    *W(o + 88) = 0u - lim;
}

/* (?, record): a new entry at +224 + 10 * the count (+264, which moves on, kept 0 - 3): 1, then +72 /
   +74 / +68 / +70. */
SFA_FUNC(089DFE78, add_entry_264) {
    gaddr p = ARG(1);
    int32_t n = S16(p + 264);
    *H(p + 264) = (uint16_t)(n + 1);
    *H(p + 264) = (uint16_t)(S16(p + 264) & 3);
    gaddr e = p + (uint32_t)(n * 10) + 0xE0;
    *H(e) = 1;
    *H(e + 6) = (uint16_t)S16(p + 68);
    *H(e + 8) = (uint16_t)S16(p + 70);
    *H(e + 2) = (uint16_t)S16(p + 72);
    *H(e + 4) = (uint16_t)S16(p + 74);
}

/* (object): every fourth frame (+82), the counters +34 and +32 move on, each kept under 14. */
static void tick_32_34(gaddr o) {
    int16_t t = S16(o + 82);
    if ((t & 3) == 0) {
        *H(o + 34) = (uint16_t)(S16(o + 34) + 1);
        if (S16(o + 34) >= 14) *H(o + 34) = 0;
        *H(o + 32) = (uint16_t)(S16(o + 32) + 1);
        if (S16(o + 32) >= 14) *H(o + 32) = 0;
    }
    *H(o + 82) = (uint16_t)(t + 1);
}
SFA_FUNC(088B09B8, tick_32_34_a) { tick_32_34(ARG(0)); }
SFA_FUNC(088B4ABC, tick_32_34_b) { tick_32_34(ARG(0)); }

/* (object, other): +1 = whether the other's +1 is set; types 5 - 9 (+3) also take the other's place. */
SFA_FUNC(088B1E18, follow_shown_and_place) {
    gaddr o = ARG(0), w = ARG(1);
    uint32_t type = *B(o + 3);
    *B(o + 1) = *B(w + 1) != 0 ? 1 : 0;
    if (type >= 5 && type <= 9) {
        *H(o + 18) = (uint16_t)S16(w + 18);
        *H(o + 22) = (uint16_t)S16(w + 22);
    }
}

/* (object): while GAME + 22's high byte is 0, +38 grows by 4; under 16, +5 = 0 and +73 = 2; from 16, +5
   moves on and +72 = 1. */
SFA_FUNC(088B4424, grow_38_by_4) {
    gaddr o = ARG(0);
    if (((S16(GAME + 22) & 0xFF00) >> 8) != 0) return;
    *H(o + 38) = (uint16_t)(S16(o + 38) + 4);
    if (S16(o + 38) < 16) { *B(o + 5) = 0; *B(o + 73) = 2; return; }
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *B(o + 72) = 1;
}
