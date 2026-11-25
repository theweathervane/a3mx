/* The smallest functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 2 - 6
   instructions each: empty handlers and fixed answers, counters, flags and fields set or copied, and a
   few getters of the network session (0x09523820) and the BATTLE record. Many are the same code at
   several addresses (each character's or object kind's own copy). Offsets stay numbers until
   something shows what they are.
 */
#include "../decomp.h"
#include "../guest_mem.h"

#define BATTLE   0x092E1228u
#define SESSION  0x09523820u   /* the network session's records (0x088DDxxx) */

/* ---------------------------------------------------------------- empty handlers and fixed answers */
SFA_FUNC(0884FED0, nothing_0884FED0) { (void)c; }
SFA_FUNC(088D2DB4, nothing_088D2DB4) { (void)c; }
SFA_FUNC(088D2DDC, nothing_088D2DDC) { (void)c; }
SFA_FUNC(088D35B8, nothing_088D35B8) { (void)c; }
SFA_FUNC(088D35C0, nothing_088D35C0) { (void)c; }
SFA_FUNC(088D35C8, nothing_088D35C8) { (void)c; }
SFA_FUNC(088D3848, nothing_088D3848) { (void)c; }
SFA_FUNC(088D3960, nothing_088D3960) { (void)c; }
SFA_FUNC(088D6438, nothing_088D6438) { (void)c; }
SFA_FUNC(089238A0, nothing_089238A0) { (void)c; }
SFA_FUNC(0894D0E4, nothing_0894D0E4) { (void)c; }
SFA_FUNC(0898AE60, nothing_0898AE60) { (void)c; }
SFA_FUNC(089A536C, nothing_089A536C) { (void)c; }
SFA_FUNC(089B7590, nothing_089B7590) { (void)c; }
SFA_FUNC(089B7598, nothing_089B7598) { (void)c; }
SFA_FUNC(089CE804, nothing_089CE804) { (void)c; }
SFA_FUNC(089DA960, nothing_089DA960) { (void)c; }

SFA_FUNC(08816FBC, answer_0_08816FBC) { RETURN(0); }
SFA_FUNC(088D3EB8, answer_0_088D3EB8) { RETURN(0); }
SFA_FUNC(089E1D94, answer_0_089E1D94) { RETURN(0); }
SFA_FUNC(089E1F70, answer_0_089E1F70) { RETURN(0); }
SFA_FUNC(089E30F0, answer_0_089E30F0) { RETURN(0); }
SFA_FUNC(088EF404, answer_1_088EF404) { RETURN(1); }
SFA_FUNC(088F7E6C, answer_1_088F7E6C) { RETURN(1); }
SFA_FUNC(08906868, answer_1_08906868) { RETURN(1); }
SFA_FUNC(08909FD8, answer_1_08909FD8) { RETURN(1); }
SFA_FUNC(089104F8, answer_1_089104F8) { RETURN(1); }
SFA_FUNC(0892AE6C, answer_1_0892AE6C) { RETURN(1); }
SFA_FUNC(0895CD7C, answer_1_0895CD7C) { RETURN(1); }
SFA_FUNC(08984738, answer_1_08984738) { RETURN(1); }
SFA_FUNC(08994ACC, answer_1_08994ACC) { RETURN(1); }
SFA_FUNC(089A3790, answer_1_089A3790) { RETURN(1); }

/* (a, b): b. */
SFA_FUNC(08878468, second_argument) { RETURN(ARG(1)); }

/* ---------------------------------------------------------------- counters */
static void inc8(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void inc16(gaddr a, int by) { *H(a) = (uint16_t)(*H(a) + by); }

/* (object): +4 counted on (the next step). */
SFA_FUNC(08899AB8, step_on_4_a) { inc8(ARG(0) + 4); }
SFA_FUNC(0889D430, step_on_4_b) { inc8(ARG(0) + 4); }
SFA_FUNC(088EC048, step_on_4_c) { inc8(ARG(0) + 4); }
SFA_FUNC(08905224, step_on_4_d) { inc8(ARG(0) + 4); }
SFA_FUNC(0899D25C, step_on_4_e) { inc8(ARG(0) + 4); }
SFA_FUNC(089A9404, step_on_4_f) { inc8(ARG(0) + 4); }
/* (object): +5 counted on. */
SFA_FUNC(088ADE8C, step_on_5_a) { inc8(ARG(0) + 5); }
SFA_FUNC(089811A4, step_on_5_b) { inc8(ARG(0) + 5); }
/* (object): +9 counted on. */
SFA_FUNC(088A9A10, count_9_a) { inc8(ARG(0) + 9); }
SFA_FUNC(088A9AB0, count_9_b) { inc8(ARG(0) + 9); }
SFA_FUNC(088A9BE0, count_9_c) { inc8(ARG(0) + 9); }
SFA_FUNC(088A9E14, count_9_d) { inc8(ARG(0) + 9); }
SFA_FUNC(088A9FC4, count_9_e) { inc8(ARG(0) + 9); }
SFA_FUNC(088AA49C, count_9_f) { inc8(ARG(0) + 9); }
SFA_FUNC(088AAACC, count_9_g) { inc8(ARG(0) + 9); }
SFA_FUNC(088AAEC8, count_9_h) { inc8(ARG(0) + 9); }
SFA_FUNC(088AB28C, count_9_i) { inc8(ARG(0) + 9); }
SFA_FUNC(088AB420, count_9_j) { inc8(ARG(0) + 9); }
SFA_FUNC(088ABE88, count_9_k) { inc8(ARG(0) + 9); }
SFA_FUNC(088AC008, count_9_l) { inc8(ARG(0) + 9); }
SFA_FUNC(088AC090, count_9_m) { inc8(ARG(0) + 9); }
/* (object): +17 counted on. */
SFA_FUNC(088AB4A8, count_17_a) { inc8(ARG(0) + 17); }
SFA_FUNC(088AB77C, count_17_b) { inc8(ARG(0) + 17); }
SFA_FUNC(088AB804, count_17_c) { inc8(ARG(0) + 17); }
SFA_FUNC(088AB9FC, count_17_d) { inc8(ARG(0) + 17); }
/* (object): +22 counted on. */
SFA_FUNC(088E3B18, count_22) { inc8(ARG(0) + 22); }
/* (object): +128 counted on. */
SFA_FUNC(08887EBC, count_128_a) { inc8(ARG(0) + 128); }
SFA_FUNC(088B2A30, count_128_b) { inc8(ARG(0) + 128); }
SFA_FUNC(088B5B84, count_128_c) { inc8(ARG(0) + 128); }
SFA_FUNC(088EB8CC, count_128_d) { inc8(ARG(0) + 128); }
/* (record): +32 / +36 / +40 counted on. */
SFA_FUNC(089DE684, count_32) { inc8(ARG(0) + 32); }
SFA_FUNC(089DE6F0, count_36) { inc8(ARG(0) + 36); }
SFA_FUNC(089DE788, count_40) { inc8(ARG(0) + 40); }
/* (record): the half-word +6 counted on by 1 or 2; +18 by 2. */
SFA_FUNC(089DF86C, count16_6) { inc16(ARG(0) + 6, 1); }
SFA_FUNC(088B2744, count16_6_by_2) { inc16(ARG(0) + 6, 2); }
SFA_FUNC(088AD20C, count16_18_by_2) { inc16(ARG(0) + 18, 2); }

/* (object): +20 or +16 (16.16 places) moved by -0.5, or +20 by +1.5. */
static void add32(gaddr a, uint32_t v) { *W(a) += v; }
SFA_FUNC(088AD460, up_half_20_a) { add32(ARG(0) + 20, 0xFFFF8000u); }
SFA_FUNC(088AD4BC, up_half_20_b) { add32(ARG(0) + 20, 0xFFFF8000u); }
SFA_FUNC(088AD4CC, back_half_16_a) { add32(ARG(0) + 16, 0xFFFF8000u); }
SFA_FUNC(088AD528, back_half_16_b) { add32(ARG(0) + 16, 0xFFFF8000u); }
SFA_FUNC(088AD288, down_1_5_20) { add32(ARG(0) + 20, 0x18000u); }

/* The word at 0x08B027E4 counted down by 2. */
SFA_FUNC(088A9AC0, count_down_08B027E4) { add32(0x08B027E4u, 0xFFFFFFFEu); }

/* (object): +4 on, +5 = 0 ... and the like: a step's first moves. */
SFA_FUNC(088554F0, step_on_5_speed) { gaddr o = ARG(0); inc8(o + 5); *W(o + 96) = 0xFFF00000u; }
SFA_FUNC(0889A800, step_on_4_86) { gaddr o = ARG(0); inc8(o + 4); *H(o + 86) = 8; }
SFA_FUNC(088BD110, step_on_4_clear_92) { gaddr o = ARG(0); *W(o + 92) = 0; inc8(o + 4); }
SFA_FUNC(088AC0D4, count_9_clear_13) { gaddr o = ARG(0); *B(o + 13) = 0; inc8(o + 9); }

/* ---------------------------------------------------------------- fields set */
static void zero4(gaddr a) { *B(a + 3) = 0; *B(a + 2) = 0; *B(a + 1) = 0; *B(a) = 0; }

/* (object, a, b): +40 = a, +41 = b. */
SFA_FUNC(08804EA4, set_40_41) { gaddr o = ARG(0); *B(o + 40) = (uint8_t)ARG(1); *B(o + 41) = (uint8_t)ARG(2); }
/* (record): its half-word +0 and words +4 - +20 cleared. */
SFA_FUNC(08804EB0, clear_0_to_20) {
    gaddr r = ARG(0);
    *H(r) = 0;
    for (int i = 4; i <= 20; i += 4) *W(r + i) = 0;
}
/* (v): the word at 0x08A03788 = v. */
SFA_FUNC(08808418, set_08A03788) { *W(0x08A03788u) = ARG(0); }
/* (object, -, out): *out = its +22 (y). */
SFA_FUNC(0881EB9C, y_into) { *H(ARG(2)) = (uint16_t)S16(ARG(0) + 22); }
SFA_FUNC(08864BB0, set_14_2) { *B(ARG(0) + 14) = 2; }
SFA_FUNC(08864BBC, set_24_7) { *H(ARG(0) + 24) = 7; }
SFA_FUNC(0887D710, set_4_2) { *B(ARG(0) + 4) = 2; }
SFA_FUNC(088EB2CC, set_93_1) { *B(ARG(0) + 93) = 1; }
SFA_FUNC(08944E2C, set_7_1) { *B(ARG(0) + 7) = 1; }
/* (fighter): +194 = 1, and 1 back. */
SFA_FUNC(08909F9C, set_194_1) { *B(ARG(0) + 194) = 1; RETURN(1); }
/* (object): +7 cleared, then 0 or 1 back. */
SFA_FUNC(08868208, clear_7_answer_0) { *B(ARG(0) + 7) = 0; RETURN(0); }
SFA_FUNC(08868214, clear_7_answer_1) { *B(ARG(0) + 7) = 0; RETURN(1); }
/* (object): +0 and +7 cleared, 0 back. */
SFA_FUNC(088681F8, clear_0_7) { gaddr o = ARG(0); *B(o) = 0; *B(o + 7) = 0; RETURN(0); }
/* (fighter, other): +117 = the other's +13. */
SFA_FUNC(08871954, take_13_as_117) { *B(ARG(0) + 117) = *B(ARG(1) + 13); }
/* (fighter, other): +698 = 1, +117 = the other's +13. */
SFA_FUNC(088718F0, mark_698_take_13_a) { gaddr f = ARG(0); *B(f + 698) = 1; *B(f + 117) = *B(ARG(1) + 13); }
SFA_FUNC(08871960, mark_698_take_13_b) { gaddr f = ARG(0); *B(f + 698) = 1; *B(f + 117) = *B(ARG(1) + 13); }
/* (fighter): +698 = 1 and +117 = a fixed value. */
static void mark_698_117(gaddr f, uint8_t v) { *B(f + 117) = v; *B(f + 698) = 1; }
SFA_FUNC(088718DC, mark_698_117_0D_a) { mark_698_117(ARG(0), 0x0D); }
SFA_FUNC(0887199C, mark_698_117_0D_b) { mark_698_117(ARG(0), 0x0D); }
SFA_FUNC(08871974, mark_698_117_15) { mark_698_117(ARG(0), 0x15); }
SFA_FUNC(08871988, mark_698_117_18) { mark_698_117(ARG(0), 0x18); }
SFA_FUNC(08871A30, mark_698_117_21) { mark_698_117(ARG(0), 0x21); }
/* (record): its +68 into +2. */
SFA_FUNC(089DFC08, copy_68_to_2) { gaddr r = ARG(0); *H(r + 2) = (uint16_t)S16(r + 68); }
/* (record): +0 = 0, 0 back. */
SFA_FUNC(089E1E40, clear_word_answer_0) { *W(ARG(0)) = 0; RETURN(0); }
/* (fighter): +628, +629, +639 cleared. */
SFA_FUNC(0885C2B4, clear_628_629_639) { gaddr f = ARG(0); *B(f + 639) = 0; *B(f + 629) = 0; *B(f + 628) = 0; }
/* (fighter): its opponent's (+92) +543 = 5. */
SFA_FUNC(0886A530, opponent_543_5) { *B(*W(ARG(0) + 92) + 543) = 5; }
/* (fighter, v): +506 = +494, v back. */
SFA_FUNC(08878470, copy_494_to_506) { gaddr f = ARG(0); *H(f + 506) = (uint16_t)S16(f + 494); RETURN(ARG(1)); }
/* (fighter): the CPU's first script and command (+481 - +483) cleared. */
SFA_FUNC(0887B1A8, clear_481_to_483) { gaddr f = ARG(0); *B(f + 483) = 0; *B(f + 482) = 0; *B(f + 481) = 0; }
/* The camera's record (0x092E88A0) +123 cleared. */
SFA_FUNC(088B3DCC, clear_092E891B) { *B(0x092E88A0u + 123) = 0; }
/* (object): +20 = 0, +22 = its +132. */
SFA_FUNC(088D36D0, set_20_22_from_132) { gaddr o = ARG(0); int16_t v = S16(o + 132); *H(o + 20) = 0; *H(o + 22) = (uint16_t)v; }
/* (object or 0): +13 cleared. */
SFA_FUNC(088DDB68, clear_13_if_any) { gaddr o = ARG(0); if (o) *B(o + 13) = 0; }
/* (v): the half-word at 0x08B2334C = v. */
SFA_FUNC(089E0B80, set_08B2334C) { *H(0x08B23348u + 4) = (uint16_t)ARG(0); }
/* (a, b): the words at a and b swapped. */
SFA_FUNC(0886D860, swap_words) { gaddr a = ARG(0), b = ARG(1); uint32_t x = *W(a), y = *W(b); *W(b) = x; *W(a) = y; }
/* The word at 0x08B0282C = 0xFD000000. */
SFA_FUNC(088AA368, set_08B0282C) { *W(0x08B02828u + 4) = 0xFD000000u; }
/* (object): +24 = 7, +9 = 1. */
SFA_FUNC(088B4484, set_24_7_9_1) { gaddr o = ARG(0); *H(o + 24) = 7; *B(o + 9) = 1; }
/* BATTLE +35 = 0x16. */
SFA_FUNC(088D3B18, battle_35_16) { *B(BATTLE + 35) = 0x16; }
/* The session's +240 = 1. */
SFA_FUNC(088DD02C, session_240_1) { *B(SESSION + 240) = 1; }
/* (object): +11 cleared when +76 is set. */
SFA_FUNC(0891D5FC, clear_11_if_76) { gaddr o = ARG(0); if (S8(o + 76) != 0) *B(o + 11) = 0; }
/* (record): +33, +37, +41 = 1. */
SFA_FUNC(089DFE4C, set_33_37_41) { gaddr r = ARG(0); *B(r + 41) = 1; *B(r + 37) = 1; *B(r + 33) = 1; }
/* The byte at 0x08A077CC = the options block's (0x08A058A0) +79. */
SFA_FUNC(08816EC4, copy_option_79) { *B(0x08A077CCu) = (uint8_t)S8(0x08A058A0u + 79); }
/* The record at 0x09523C60: +2, +3, +9 cleared. */
SFA_FUNC(0881781C, clear_09523C60) { gaddr r = 0x09523C60u; *B(r + 2) = 0; *B(r + 3) = 0; *B(r + 9) = 0; }
/* (fighter): +580 / +292 from its routine table (+416)'s +32 / +144. */
SFA_FUNC(08864B98, table_32_144) {
    gaddr f = ARG(0), t = *W(f + 416);
    *W(f + 580) = *W(t + 32);
    *W(f + 292) = *W(t + 144);
}
/* (object): +4 = +5 = 1, +20 = 0x3E0000. */
SFA_FUNC(0889EAE0, start_20_3E) { gaddr o = ARG(0); *B(o + 5) = 1; *B(o + 4) = 1; *W(o + 20) = 0x3E0000u; }
/* (object): +8 - +11, +12 - +15, +16 - +19 or +20 - +23 cleared. */
SFA_FUNC(088A9350, clear_8_to_11) { zero4(ARG(0) + 8); }
SFA_FUNC(088A9368, clear_12_to_15) { zero4(ARG(0) + 12); }
SFA_FUNC(088A9380, clear_16_to_19) { zero4(ARG(0) + 16); }
SFA_FUNC(088A9398, clear_20_to_23) { zero4(ARG(0) + 20); }
/* (object): at its owner's (+88) place (+18, +22). */
SFA_FUNC(088BD918, at_owner_place) {
    gaddr o = ARG(0), w = *W(o + 88);
    *H(o + 18) = (uint16_t)S16(w + 18);
    *H(o + 22) = (uint16_t)S16(w + 22);
}
/* (fighter): its opponent's (+92) +127 = 4, its own +573 = 0xC. */
SFA_FUNC(088D407C, opponent_127_4) { gaddr f = ARG(0); *B(*W(f + 92) + 127) = 4; *B(f + 573) = 0xC; }
/* (fighter): move number (+656) 0x11 unless +80 is set. */
static void move_11_unless_80(gaddr f) { if (S8(f + 80) == 0) *B(f + 656) = 0x11; }
SFA_FUNC(088F9788, move_11_unless_80_a) { move_11_unless_80(ARG(0)); }
SFA_FUNC(08964F18, move_11_unless_80_b) { move_11_unless_80(ARG(0)); }
/* (fighter, v): +236 = v, facing (+11) = the opponent's side (+697), +235 = 1. */
SFA_FUNC(089439DC, face_opponent_236) {
    gaddr f = ARG(0);
    int8_t side = S8(f + 697);
    *B(f + 236) = (uint8_t)ARG(1);
    *B(f + 11) = (uint8_t)side;
    *B(f + 235) = 1;
}
/* (object): step 2 (+4 = 2, +5 - +7 cleared). */
SFA_FUNC(0896FA20, step_2) { gaddr o = ARG(0); *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
/* (fighter): +260 = 8, +311 = 2, +312 = 8. */
SFA_FUNC(089A3B6C, set_260_311_312) { gaddr f = ARG(0); *B(f + 260) = 8; *B(f + 311) = 2; *B(f + 312) = 8; }
/* (record): +106 = the top four bits of +66, plus 1. */
SFA_FUNC(089DFCC8, top_bits_66_to_106) {
    gaddr r = ARG(0);
    *B(r + 106) = (uint8_t)((((int32_t)S16(r + 66) & 0xF000) >> 12) + 1);
}
/* (record, a, b, c, d): +4 / +8 / +12 / +16 = a / b / c / d, 0 back. */
SFA_FUNC(089E1D9C, set_4_to_16) {
    gaddr r = ARG(0);
    *W(r + 4) = ARG(1); *W(r + 8) = ARG(2); *W(r + 12) = ARG(3); *W(r + 16) = c->r[8];
    RETURN(0);
}

/* ---------------------------------------------------------------- questions and getters */
/* (fighter): 1 while +536 is 0. */
SFA_FUNC(088EF41C, no_536_a) { RETURN(*B(ARG(0) + 536) == 0); }
SFA_FUNC(088F7E74, no_536_b) { RETURN(*B(ARG(0) + 536) == 0); }
SFA_FUNC(0892AE74, no_536_c) { RETURN(*B(ARG(0) + 536) == 0); }
SFA_FUNC(089A37B0, no_536_d) { RETURN(*B(ARG(0) + 536) == 0); }
/* (object): 1 while +73 is 0. */
SFA_FUNC(08994AD4, no_73) { RETURN(S8(ARG(0) + 73) == 0); }
/* (fighter): by its ISM (+386): set; not -1; not negative. */
SFA_FUNC(089A3798, ism_set) { RETURN(S8(ARG(0) + 386) != 0); }
SFA_FUNC(088EF40C, ism_not_minus_1) { RETURN(S8(ARG(0) + 386) + 1 != 0); }
SFA_FUNC(08906858, ism_not_negative) { RETURN(S8(ARG(0) + 386) >= 0); }
/* (fighter): 1 when +366 is 144 or more (a full gauge). */
SFA_FUNC(088D32C8, gauge_366_full) { RETURN(S16(ARG(0) + 366) >= 144); }
/* (fighter): 1 when +494 has reached +508. */
SFA_FUNC(08876B48, count_494_reached_508) { gaddr f = ARG(0); RETURN(!(S16(f + 494) < S16(f + 508))); }
/* BATTLE +37 set. */
SFA_FUNC(088D3B50, battle_37_set) { RETURN(S8(BATTLE + 37) != 0); }
/* The session's +236 / +228 words, and the addresses of its +4 and +172. */
SFA_FUNC(088DD114, session_236) { RETURN(*W(SESSION + 236)); }
SFA_FUNC(088DD124, session_228) { RETURN(*W(SESSION + 228)); }
SFA_FUNC(088DD22C, session_at_4) { RETURN(SESSION + 4); }
SFA_FUNC(088DD304, session_at_172) { RETURN(SESSION + 0xAC); }
/* 0x09523930's word, 0x09523977's byte. */
SFA_FUNC(088E357C, word_09523930) { RETURN(*W(0x09523920u + 16)); }
SFA_FUNC(088E3FD0, byte_09523977) { RETURN((int32_t)S8(0x09523960u + 23)); }
/* (n): the s8 at 0x08A0F3D2 + n. */
SFA_FUNC(08830334, s8_08A0F3D2) { RETURN((int32_t)S8(0x08A0F3D2u + ARG(0))); }
/* (-, pair): the pair's second half-word less its first, as an s16. */
SFA_FUNC(0886CF78, pair_difference) { gaddr p = ARG(1); RETURN((int32_t)(int16_t)(S16(p + 4) - S16(p))); }
/* (fighter): the s8 at 0x08A22D30 by its character (+338). */
SFA_FUNC(0887C5B4, by_character_08A22D30) { RETURN((int32_t)S8(0x08A22D30u + (int32_t)S8(ARG(0) + 338))); }
/* (x): x truncated to a whole number, as a float (f0); f12 and a0 keep the whole number's bits. */
SFA_FUNC(089E0EF8, float_trunc) {
    int32_t n = CVT_TRUNC(c, FS(c, 12));
    c->f[12] = (uint32_t)n; c->r[4] = (uint32_t)n;
    float x = (float)n;
    FSET(c, 0, x);
}
