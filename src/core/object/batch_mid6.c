/* More middle-sized steps (0.111.0): the random number's tempering, the fighters' routine tables for a
 * fight, script commands that set a CPU state, a frame record stepped on, a value chosen on the select
 * screen, the score for a hit, objects' colours from a palette, and more set-ups. Offsets stay numbers
 * until something shows what they are. Arguments of native callees, and what the original leaves in a0 -
 * a3 / t0 at a call through a table, are set as the original sets them; s0 - s4 as the original sets
 * them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define P1      0x092E3EA0u
#define P2      0x092E4338u
#define P3      0x092E47D0u
#define P4      0x092E4C68u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_08820A68(Cpu *c);   /* random_twist */
void f_088D7EA0(Cpu *c);   /* net_count_264 */
void f_08812F90(Cpu *c);   /* step_by_battle_56 */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_0887B528(Cpu *c);   /* fighter_cpu_state_from_word */
void f_08811FF4(Cpu *c);   /* mask_layer_picture */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08821330(Cpu *c);   /* object_routine_by_14 */
void f_08821368(Cpu *c);   /* game_random */
void f_088211CC(Cpu *c);   /* game_add_score */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088A21C4(Cpu *c);   /* game_bselect_net_timeout */
void f_088A507C(Cpu *c);   /* game_bselect_pad_pressed */
void f_088A4E90(Cpu *c);   /* game_bselect_sound_choose */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }

/* ---------------------------------------------------------------- the game's random number */

/* (): the next number of the Mersenne twister at 0x08B01564 (index at 0x08B01F24, twisted at 0), tempered;
   its low 16 bits to GAME +64, the low byte answered. */
SFA_FUNC(08820BE0, random_next) {
    gaddr s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x10;
    gaddr mt = 0x08B01564u;
    uint32_t i = *H(mt + 2496);
    c->r[16] = i; c->r[17] = mt; c->r[18] = GAME;
    if (i == 0) f_08820A68(c);
    uint32_t y = *W(mt + i * 4);
    uint32_t next = i == 0x26F ? 0 : (i + 1) & 0xFFFF;
    y ^= y >> 11;
    y ^= (y << 7) & 0x9D2C5680u;
    *H(mt + 2496) = (uint16_t)next;
    y ^= (y << 15) & 0xEFC60000u;
    y ^= y >> 18;
    *H(GAME + 64) = (uint16_t)y;
    c->r[4] = y; c->r[5] = y & 0xFFFF; c->r[6] = 0xEFC60000u; c->r[7] = 0x9D2C5680u;
    c->r[29] += 0x10;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
    RETURN(y & 0xFF);
}

/* ---------------------------------------------------------------- fighters */

/* (): the fighters' routine tables for a fight (0x0951CE68 + 0xAA8 each), character 0x15 for the humans
   of a battle mode 2, the step by BATTLE +56 for P1, with the network count up and down around it. */
SFA_FUNC(088288F0, fight_routine_tables) {
    c->r[29] -= 0x20;
    c->r[4] = 1; f_088D7EA0(c);
    if (S8(BATTLE + 59) == 2) {
        if (S8(P1 + 373) != 0) *B(P1 + 338) = 0x15;
        if (S8(P2 + 373) != 0) *B(P2 + 338) = 0x15;
    }
    *W(P1 + 416) = 0x0951CE68u;
    *W(P2 + 416) = 0x0951D910u;
    *W(P3 + 416) = 0x0951E3B8u;
    *W(P4 + 416) = 0x0951EE60u;
    c->r[4] = P1; c->r[5] = P1; c->r[6] = P4;
    f_08812F90(c);
    c->r[4] = 0; f_088D7EA0(c);
    c->r[29] += 0x20;
}

/* (fighter, script): script command: +490 / +506 from the script; within 2 of the level (+508) read
   on, otherwise the script ends (state 1, step 2, +483 = 7) with +78 / +497 set when +490 is at or
   past the level. */
SFA_FUNC(088782C8, fighter_script_cmd_near) {
    gaddr f = ARG(0), p = ARG(1);
    int32_t v = S16(p), w = S16(p + 2), lv = S16(f + 508);
    uint32_t d = ((uint32_t)v & 0xFFFF) - (uint32_t)lv;
    c->r[2] = p + 4;
    *H(f + 506) = (uint16_t)w;
    *H(f + 490) = (uint16_t)v;
    c->r[29] -= 0x20;
    if ((int32_t)(((d & 0xFFFF) + 3) & 0xFFFF) < 6) {
        c->r[5] = p + 4; c->r[6] = 1;
        f_0887A10C(c);
    } else {
        *B(f + 482) = 1; *B(f + 483) = 7;
        *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
        uint8_t past = !((int32_t)((uint32_t)S16(f + 490) & 0xFFFF) < lv);
        *B(f + 78) = past; *B(f + 497) = past;
        c->r[5] = past; c->r[6] = (uint32_t)S16(f + 490) & 0xFFFF; c->r[7] = 2;
    }
    c->r[29] += 0x20;
}

/* (fighter, script): script command: routine 1.3.7 with +490 / +492 / +494 from the script, the CPU
   state from +490 (0x0887B528), +496 / +497 from +492's bytes, then command +492 / 2 of 0x08A2254C. */
SFA_FUNC(088783B0, fighter_script_cmd_state) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 482) = 2; *B(f + 483) = 0;
    *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 3; *B(f + 7) = 7;
    *H(f + 490) = (uint16_t)S16(p);
    *H(f + 492) = (uint16_t)S16(p + 2);
    *H(f + 494) = (uint16_t)S16(p + 4);
    c->r[17] = p + 6;
    c->r[4] = f; c->r[5] = (uint32_t)S16(f + 490) & 0xFFFF; c->r[6] = (uint32_t)(int32_t)S16(f + 490);
    f_0887B528(c);
    int32_t a = S8(f + 492), b = S8(f + 493);
    *B(f + 497) = (uint8_t)b;
    *B(f + 496) = (uint8_t)a;
    gaddr fn = *W(0x08A2254Cu + (uint32_t)(a >> 1) * 4);
    c->r[4] = f; c->r[5] = p + 6; c->r[6] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (x): x moved by the push table 0x08A1ECDC for P1's opponent's character (0x08A1DB88 row, negative
   adds), plus a random spread from 0x08A1DBDC (row 0x08A1DBB2); at least 1. */
SFA_FUNC(08871CA4, push_with_spread) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    uint32_t x = ARG(0);
    c->r[29] -= 0x10;
    int32_t ch = S8(*W(0x09358410u + 24) + 338);
    int32_t v = S8(0x08A1DB88u + (uint32_t)ch);
    int neg = v < 0;
    if (neg) v = -v;
    int32_t a = S8(((uint32_t)v << 7) + x + 0x08A1ECDCu);
    if (neg) a = -a;
    c->r[16] = x + (uint32_t)a;
    c->r[17] = (uint32_t)(int32_t)S8(0x08A1DBB2u + (uint32_t)ch) << 5;
    c->r[4] = (uint32_t)(int32_t)S8(0x08A1DBB2u + (uint32_t)ch);
    f_08821368(c);
    int32_t r = (int32_t)(c->r[16] + (uint32_t)(int32_t)S8(c->r[17] + (c->r[2] & 0x1F) + 0x08A1DBDCu));
    if (r <= 0) r = 1;
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
    RETURN((uint32_t)r);
}

/* (object, hit): the score for a hit (the hit's +8, capped at 63, from 0x08A21F6E or with +308 0x08A21FAE)
   to P1's opponent, unless BATTLE +61, a human there, or (without +135) +117 bit 7. */
SFA_FUNC(088734B4, score_for_hit) {
    gaddr o = ARG(0), h = ARG(1);
    gaddr f = *W(0x09358410u + 24);
    if (S8(BATTLE + 61) != 0 || S8(f + 373) != 0) return;
    if (S8(o + 135) == 0 && (*B(o + 117) & 0x80)) return;
    uint32_t k = *B(h + 8) & 0x7F;
    if ((int32_t)k >= 64) k = 0x3F;
    gaddr t = S8(f + 308) != 0 ? 0x08A21FAEu : 0x08A21F6Eu;
    c->r[29] -= 0x20;
    c->r[4] = (uint32_t)(int32_t)S8(t + k); c->r[5] = f;
    f_088211CC(c);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- objects */

/* (object): its colours from the 15-bit palette 0x092DF980 (+2 * 16 + +140); while +108 moves it, +8 by
   +108 until it passes 0xF00000, then back to +116. */
SFA_FUNC(08887D54, colour_from_palette) {
    gaddr o = ARG(0);
    uint32_t col = *H(0x092DF980u + ((uint32_t)*B(o + 2) * 16 + *B(o + 140)) * 2);
    uint8_t r = (uint8_t)((col & 0x7C00) >> 7), g = (uint8_t)((col & 0x3E0) >> 2), b = (uint8_t)((col & 0x1F) << 3);
    for (int k = 36; k <= 39; k++) *B(o + k) = r;
    for (int k = 42; k <= 45; k++) *B(o + k) = g;
    for (int k = 48; k <= 51; k++) *B(o + k) = b;
    uint32_t v = *W(o + 108);
    if (v == 0) return;
    uint32_t n = *W(o + 8) + v;
    *W(o + 8) = n;
    if (!(0xD00000 < (int32_t)(n - 0x200000u))) {
        *W(o + 108) = 0;
        *W(o + 8) = *W(o + 116);
    }
}

/* (me): on the select screen, a choice of two (+441) moved by the pad (bit 8 back, bit 4 on), and on any
   button (or the network's time out) the choose sound and +5 moves on. */
SFA_FUNC(088A44B8, bselect_choice_441) {
    gaddr me = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = me;
    f_088A21C4(c);
    int done = c->r[2] != 0;
    if (!done) {
        c->r[4] = me;
        f_088A507C(c);
        uint32_t k = c->r[2];
        int32_t v = S8(me + 441), w;
        if (k & 8) { *B(me + 441) = (uint8_t)(v - 1); w = S8(me + 441) + 2; }
        else if (k & 4) { *B(me + 441) = (uint8_t)(v + 1); w = S8(me + 441) + 2; }
        else w = v + 2;
        *B(me + 441) = (uint8_t)w;
        int32_t n = S8(me + 441);
        *B(me + 441) = (uint8_t)(n >= 0 ? n & 1 : -(n & 1));
        c->r[5] = k;
        c->r[4] = k & 0x3F0;
        done = (k & 0x3F0) != 0;
    }
    if (done) {
        f_088A4E90(c);
        *B(me + 5) = (uint8_t)(*B(me + 5) + 1);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +13 counts down; at 0 +9 moves on, GAME +10's low byte 2, and 0x08B027E0 restarts at the
   entry of 0x08A2C628 for GAME +22 (+4 = 0x300); +76 cleared, +77 from 0x0951CE52's high byte. */
SFA_FUNC(088AA2BC, count_13_restart_27e0) {
    gaddr o = ARG(0);
    uint8_t n = (uint8_t)(*B(o + 13) - 1);
    *B(o + 13) = n;
    if (n != 0) return;
    *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    *H(GAME + 10) = (uint16_t)(*H(GAME + 10) & 0xFF00);
    *H(GAME + 10) = (uint16_t)(*H(GAME + 10) | 2);
    gaddr l = 0x08B027E0u;
    *H(l + 18) = 0; *H(l + 16) = 0;
    *W(l + 4) = 0; *W(l) = 0;
    *W(l) = *H(0x08A2C628u + ((uint32_t)(int32_t)S16(GAME + 22) & 0xFF) * 2);
    *W(l + 4) = 0x300;
    *B(o + 76) = 0;
    *B(o + 77) = (uint8_t)((*H(0x0951CE52u) & 0xFF00) >> 8);
}

/* (object): once it is 0xD8 below its routine entry, step 2 and a kind-0x23 object (step 11) with +10
   counting down (its +3 = 10 until the last); with +121, step 1 instead, GAME +10 = 0xFF78, +82 = 48. */
SFA_FUNC(088BCFC0, drop_make_23) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_08821330(c);
    int32_t lim = (int32_t)(((uint32_t)S16(c->r[2] + 22) + 0xD8) & 0xFFFF);
    if (!(lim < (int32_t)((uint32_t)S16(o + 22) & 0xFFFF))) {
        *B(o + 5) = 2;
        c->r[4] = 0x23; c->r[5] = 0xB;
        f_08822EAC(c);
        gaddr r = c->r[2];
        if (r != 0) {
            uint8_t n = (uint8_t)(*B(o + 10) - 1);
            *B(o + 10) = n;
            if (n != 0) *B(r + 3) = 10;
            *B(r + 10) = *B(o + 10);
            if (S8(o + 121) != 0) {
                *B(o + 5) = 1;
                *H(GAME + 10) = 0xFF78;
                *H(o + 82) = 0x30;
            }
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): drawn until battle state 6; then +5 moves on, +82 = 4, speed 0x100000 and +73 list-3 objects
   (kind 5, types 2 on) owned by it. */
SFA_FUNC(088569F0, at_battle_6_make_parts) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(BATTLE + 44) != 6) {
        c->r[5] = 6; c->r[6] = (uint32_t)(int32_t)S8(BATTLE + 44);
        f_088215C4(c);
    } else {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 4;
        *W(o + 96) = 0x100000;
        for (c->r[17] = 0; (int32_t)c->r[17] < S8(o + 73); c->r[17] = (c->r[17] + 1) & 0xFF) {
            c->r[4] = 5; c->r[5] = (c->r[17] + 2) & 0xFF;
            f_08823338(c);
            if (c->r[2] != 0) *W(c->r[2] + 124) = o;
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (name): a four-letter name's letters from '.' on made '-', then, if it is one of the 32 entries of
   0x08A26DF4 (eight bytes each), its first three letters taken from the entry's +4 - +6. */
SFA_FUNC(08892438, name_replace) {
    gaddr p = ARG(0);
    for (int i = 0; i < 4; i++) if (!((int32_t)*B(p + i) < 46)) *B(p + i) = 0x2D;
    uint32_t last = 0;
    gaddr e = 0x08A26DF4u;
    for (int n = 0; n < 32; n++, e += 8) {
        int j;
        for (j = 0; j < 4; j++) {
            last = *B(e + j);
            if (*B(p + j) != last) break;
        }
        if (j == 4) {
            *B(p) = *B(e + 4);
            *B(p + 1) = *B(e + 5);
            *B(p + 2) = *B(e + 6);
            break;
        }
    }
    RETURN(last);
}

/* (object): the animation for its +3 and 0x08B02E08 from 0x08A3770C's pairs (into 0x08A372A4); +26 = 1
   when the pair's first is set, else 3. */
SFA_FUNC(088B544C, anim_by_3_and_2e08) {
    gaddr o = ARG(0);
    int32_t i = (int16_t)((int16_t)((int32_t)(int16_t)*B(o + 3) << 2) + S8(0x08B02E08u) * 2);
    gaddr e = 0x08A3770Cu + (uint32_t)i * 4;
    int32_t a = (int16_t)*W(e), b = (int16_t)*W(e + 4);
    gaddr t = 0x08A372A4u + (uint32_t)b * 4;
    *H(o + 26) = a != 0 ? 1 : 3;
    c->r[29] -= 0x20;
    c->r[6] = a != 0 ? 1 : 3; c->r[7] = 0x08A372A4u;
    set_anim(c, o, *W(t));
    c->r[29] += 0x20;
}

/* (object): +4 moves on; a copy of its parent's (+88) look and place with the animation 0x08A8ABFC, drawn
   if on screen. */
SFA_FUNC(0896A424, copy_parent_look) {
    gaddr o = ARG(0), p = *W(o + 88), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1;
    *B(o + 11) = *B(p + 11);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 14) = *B(p + 14);
    *H(o + 18) = *H(p + 18);
    *H(o + 22) = *H(p + 22);
    *H(o + 24) = *H(p + 24);
    *H(o + 38) = *H(p + 38);
    *H(o + 132) = *H(p + 132);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 148) = *W(p + 148);
    set_anim(c, o, *W(0x08A8ABFCu));
    c->r[4] = o;
    f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (screen, record): the next round's flags: +0 on, the four fighters' +1 / +336 / +424 cleared and
   +373 set, and 0x08A058A0 +82's bit 0 unless the record's +69 or the screen's +114 (Arcade cleared:
   wrapped by extensions/cheats_unlocks.c, which follows what is earned). */
SFA_FUNC_WRAPPED(089DE194, next_round_flags, cheats_unlocks_next_round) {
    (void)c;
    gaddr a = ARG(0), b = ARG(1);
    *H(a) = (uint16_t)(*H(a) + 1);
    *B(a + 125) = 1;
    *B(a + 43) = 1;
    *H(b + 56) = 0;
    *B(b + 55) = 0; *B(b + 84) = 0; *B(b + 36) = 0;
    *B(a + 69) = 0; *B(a + 72) = 0; *B(a + 93) = 0; *B(a + 92) = 0;
    *B(P1 + 373) = 1; *B(P2 + 373) = 1;
    *H(P2 + 424) = 0; *H(P1 + 424) = 0;
    *B(P2 + 336) = 0; *B(P1 + 336) = 0;
    *B(P4 + 1) = 0; *B(P3 + 1) = 0; *B(P2 + 1) = 0; *B(P1 + 1) = 0;
    if (S8(b + 69) == 0 && S8(a + 114) == 0) *H(0x08A058A0u + 82) = (uint16_t)(S16(0x08A058A0u + 82) | 1);
}

/* (object): the three camera tables at 0x0951F950 (152 bytes apart) set from +112 / +114 / +116 (the high
   byte / 16 as x, the low byte * 16 negated as y), their speeds and 0x09358BE0 +134 cleared. */
SFA_FUNC(089DEA64, camera_tables_from_112) {
    (void)c;
    gaddr o = ARG(0), b = 0x0951F950u;
    *W(b + 16) = 0; *W(b + 20) = 0;
    *W(b + 168) = 0; *W(b + 172) = 0;
    *W(b + 320) = 0; *W(b + 324) = 0;
    *H(0x09358BE0u + 134) = 0;
    for (uint32_t k = 0; k < 3; k++) {
        int32_t v = S16(o + 112 + k * 2);
        *H(b + 10 + k * 152) = (uint16_t)((uint32_t)(v & 0xFF00) >> 4);
        *H(b + 14 + k * 152) = (uint16_t)(-(int32_t)(int16_t)((v & 0xFF) << 4));
    }
    *B(o + 41) = 0; *B(o + 37) = 0; *B(o + 33) = 0;
}

/* (layer, object): the object's mask pictures: from its frame's (+28) +4 in the list +86 of 0x08E75AD8,
   each twelve-byte entry up to and with the first whose +10 is 0 (none when bit 15 is set). */
SFA_FUNC(0880D3B0, mask_pictures) {
    gaddr a = ARG(0), ob = ARG(1), s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr list = *W(0x08E75AD8u + (uint32_t)*H(ob + 86) * 4);
    uint32_t idx = *H(*W(ob + 28) + 4);
    c->r[16] = ob; c->r[17] = a; c->r[19] = list; c->r[18] = idx;
    if ((idx & 0x8000) == 0) {
        gaddr e = list + idx * 12;
        while (S16(e + 10) != 0) {
            c->r[18] = e;
            c->r[4] = a; c->r[5] = ob; c->r[6] = e; c->r[7] = list;
            f_08811FF4(c);
            e += 12;
        }
        c->r[18] = e;
        c->r[4] = a; c->r[5] = ob; c->r[6] = e; c->r[7] = list;
        f_08811FF4(c);
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (object): while GAME +16's high byte is clear, first time: list-3 objects of kind 7, types 2, 3, 8
   and 11 (or nothing more this frame); then its step (+9) from 0x08A2C618. */
SFA_FUNC(088AA184, make_2_3_8_11_then_step) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int ok = 1;
    if ((S16(GAME + 16) & 0xFF00) == 0 && *B(o + 92) == 0) {
        for (c->r[17] = 2; c->r[17] < 4; ) {
            c->r[4] = 7; c->r[5] = c->r[17] & 0xFF;
            f_08823338(c);
            c->r[17]++;
            if (c->r[2] == 0) { ok = 0; break; }
        }
        if (ok) { c->r[4] = 7; c->r[5] = 8; f_08823338(c); ok = c->r[2] != 0; }
        if (ok) { c->r[4] = 7; c->r[5] = 0xB; f_08823338(c); ok = c->r[2] != 0; if (ok) *B(o + 92) = 1; }
    }
    if (ok) {
        gaddr fn = *W(0x08A2C618u + (uint32_t)*B(o + 9) * 4);
        c->r[4] = o; c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): its frame (+74) counts down; at 0 the next record (+6 records of ten bytes on; +152 set when
   its +4 differs) or, with flag 0x40, +75 = -1; at 1 with flag 0x40 +75 = -1 too. */
SFA_FUNC(08821AA8, object_next_frame) {
    gaddr o = ARG(0), r = *W(o + 28);
    int8_t n = (int8_t)(S8(o + 74) - 1);
    *B(o + 74) = (uint8_t)n;
    if (n != 0) {
        if (S8(o + 74) == 1 && (S8(r + 1) & 0x40)) *B(o + 75) = 0xFF;
        return;
    }
    if (S8(r + 1) & 0x40) { *B(o + 75) = 0xFF; return; }
    gaddr nr = r + (uint32_t)(S16(r + 6) * 10);
    *W(o + 152) = *H(r + 4) != *H(nr + 4) ? 1 : 0;
    *W(o + 28) = nr;
    *B(o + 74) = *B(nr);
    *B(o + 75) = (uint8_t)S8(nr + 1);
    *B(o + 76) = (uint8_t)S8(nr + 2);
    *B(o + 77) = (uint8_t)S8(nr + 3);
}
