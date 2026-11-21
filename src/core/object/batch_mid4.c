/* More middle-sized steps (0.109.0): screen panels set up, objects that make their companions, counters
 * that flash the screen's tables, a fighter's first state, numbers turned into pictures, and the stage
 * picked for a fight. Offsets stay numbers until something shows what they are. Arguments of native
 * callees, and what the original leaves in a0 - a3 / t0 at a call through a table, are set as the
 * original sets them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08822E74(Cpu *c);   /* object_make_8 */
void f_08855D44(Cpu *c);   /* intro_make_owned */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_088215C4(Cpu *c);   /* object_draw */
void f_0888AC70(Cpu *c);   /* step_0888AC70 */
void f_088AADD8(Cpu *c);   /* next_2810_entry */
void f_088BD268(Cpu *c);   /* drop_to_routine_y */
void f_0887BA84(Cpu *c);   /* fighter_in_plain_step */
void f_0883021C(Cpu *c);   /* game_pick_at_random */
void f_088BE210(Cpu *c);   /* random_character_and_ism */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }
static void colours(gaddr o, uint8_t a, uint8_t b, uint8_t d) {
    for (int k = 36; k <= 39; k++) *B(o + k) = a;
    for (int k = 42; k <= 45; k++) *B(o + k) = b;
    for (int k = 48; k <= 51; k++) *B(o + k) = d;
}

/* ---------------------------------------------------------------- objects */

/* (object): first time, list-3 objects of kind 7, types 9, 10 and 12 (or nothing more this frame when
   one can't be made); then its step (+9) from 0x08A2C6B8. */
SFA_FUNC(088AAC80, make_9_10_12_then_step) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int ok = 1;
    if (*B(o + 92) == 0) {
        for (c->r[17] = 9; c->r[17] < 11; ) {
            c->r[4] = 7; c->r[5] = c->r[17] & 0xFF;
            f_08823338(c);
            c->r[17]++;
            if (c->r[2] == 0) { ok = 0; break; }
        }
        if (ok) {
            c->r[4] = 7; c->r[5] = 0xC;
            f_08823338(c);
            if (c->r[2] == 0) ok = 0; else *B(o + 92) = 1;
            c->r[4] = 1;
        }
    }
    if (ok) {
        gaddr fn = *W(0x08A2C6B8u + (uint32_t)*B(o + 9) * 4);
        c->r[4] = o; c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +75 counts down; at 0 +17 moves on and the two camera layers (0x092E8800 / 8940) start
   scrolling (0x08B027C8 / 2828), +64 = 0, +70 = 0x100, +56 = 0x30. */
SFA_FUNC(088ABB18, count_75_start_scroll) {
    gaddr o = ARG(0);
    int8_t n = (int8_t)(S8(o + 75) - 1);
    *B(o + 75) = (uint8_t)n;
    if (n != 0) return;
    *B(o + 17) = (uint8_t)(*B(o + 17) + 1);
    *B(0x092E8800u + 122) = 4;
    *W(0x08B027C8u) = 0;
    *W(0x08B027CCu) = 0xFF000000u;
    *B(0x092E8940u + 122) = 2;
    *W(0x08B02828u) = 0;
    *W(0x08B0282Cu) = 0xFE000000u;
    *H(o + 64) = 0;
    *H(o + 70) = 0x100;
    *W(o + 56) = 0x30;
}

/* (panel): +128 moves on; a white panel 0x180 x 0xF0 from (0, 0x00FF0000). */
SFA_FUNC(088B5AA0, panel_white_full) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 55) = 7;
    *B(o + 56) = 0;
    *B(o + 1) = 0;
    *B(o + 3) = 0;
    *W(o + 4) = 0;
    *W(o + 8) = 0xFF0000;
    *B(o + 136) = 0xFF;
    colours(o, 0xFF, 0xFF, 0xFF);
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0x180;
    *H(o + 26) = 0xF0;
    *H(o + 98) = 0;
}

/* (object): at its parent's (+88) place + (0xA0, 0x14); once GAME +10 is positive, +5 moves on with the
   animation +73 of 0x08A3AE50; otherwise animated. */
SFA_FUNC(088BCA0C, beside_parent_start_on_10) {
    gaddr o = ARG(0), p = *W(o + 88);
    *H(o + 18) = (uint16_t)(S16(p + 18) + 0xA0);
    *H(o + 22) = (uint16_t)(S16(p + 22) + 0x14);
    uint32_t k = *B(o + 5);
    c->r[29] -= 0x20;
    if (k == 0 && S16(GAME + 10) > 0) {
        *B(o + 5) = 1;
        c->r[6] = 0x08A3AE50u;
        set_anim(c, o, *W(0x08A3AE50u + (uint32_t)(int32_t)S8(o + 73) * 4));
    } else {
        c->r[5] = k;
        f_088219B0(c);
    }
    c->r[29] += 0x20;
}

/* (place, value, divisor): the value's digits (as many as the divisor has) as pictures 0x49 + digit, six
   bytes apart from +4; leading zeros are 0x4000 (blank), and a lone 0 is drawn as 0x49 at the last. */
SFA_FUNC(088576EC, digits_as_pictures) {
    gaddr p = ARG(0);
    uint32_t v = ARG(1), d = ARG(2), started = 0, t = d;
    int32_t n = -1;
    do { DIV(c, t, 10); t = c->lo; n++; } while (t != 0);
    if (n >= 0) {
        do {
            DIVU(c, v, d);
            if ((c->lo | started) == 0) *H(p + 4) = 0x4000;
            else { DIVU(c, v, d); started = 1; *H(p + 4) = (uint16_t)(c->lo + 0x49); }
            DIVU(c, v, d);
            n--;
            v = c->hi;
            DIVU(c, d, 10);
            d = c->lo;
            p += 6;
        } while (n >= 0);
    }
    if (started == 0) *H(p - 2) = 0x49;
    c->r[5] = 0x49;
    RETURN(p);
}

/* (object): +4 moves on and it is set up as picture 0x14 with draw state 0x08A38208 and the animation of
   the screen record's (0x09525DE8) +14 in 0x08A38474. */
SFA_FUNC(088BCB9C, setup_by_screen_14) {
    gaddr o = ARG(0);
    *B(o + 12) = 1;
    *B(o + 13) = 0x14;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 3;
    *H(o + 24) = 7;
    *B(o + 14) = 0;
    *B(o + 11) = 0;
    *W(o + 156) = 0x08A38208u;
    *H(o + 86) = 0x16;
    *W(o + 164) = 1;
    *W(o + 148) = 6;
    gaddr e = 0x08A38470u + (uint32_t)(int32_t)S8(*W(0x09525DE8u) + 14) * 4;
    c->r[29] -= 0x20;
    c->r[6] = 0x08A38470u;
    set_anim(c, o, *W(e + 4));
    c->r[29] += 0x20;
}

/* (): the intro's owner (kind 0, type 3, from the pool of 8) with its owned objects 10 and 11 (unless
   BATTLE +13) and 12 (with BATTLE +83); none while BATTLE +76. */
SFA_FUNC(08855CB0, intro_owner_and_parts) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[17] = BATTLE;
    c->r[4] = 0; c->r[5] = 3;
    f_08822E74(c);
    gaddr r = c->r[2];
    c->r[16] = r;
    if (r != 0) {
        *B(r) = 1;
        if (S8(BATTLE + 76) == 0) {
            if (S8(BATTLE + 13) == 0) {
                c->r[4] = r; c->r[5] = 0xA; c->r[6] = 1; f_08855D44(c);
                c->r[4] = r; c->r[5] = 0xB; c->r[6] = 1; f_08855D44(c);
            }
            if (S8(BATTLE + 83) != 0) { c->r[4] = r; c->r[5] = 0xC; c->r[6] = 1; f_08855D44(c); }
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +5 is set at battle state 5; before it +82 counts down to it; after it, +4 moves on at
   battle state 6, drawn otherwise. */
SFA_FUNC(0885777C, wait_battle_5_draw) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 44) == 5) *B(o + 5) = 1;
    if (*B(o + 5) == 0) {
        int32_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        if (n == 0) *B(o + 5) = 1;
        return;
    }
    if (S8(BATTLE + 44) == 6) { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); return; }
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)(int32_t)S8(BATTLE + 44); c->r[6] = 6;
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (panel): a black panel 0x180 x 0x18 at y 0xE0 (type 0) or 0x20, its step by +92 (0x0888AC70). */
SFA_FUNC(0888AB48, panel_black_strip) {
    gaddr o = ARG(0);
    *B(o + 55) = 7;
    *B(o + 56) = 0;
    *B(o + 2) = 0;
    *B(o + 136) = 0xA0;
    *H(o + 6) = 0;
    *H(o + 10) = *B(o + 92) == 0 ? 0xE0 : 0x20;
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0x180;
    *H(o + 26) = 0x18;
    colours(o, 0, 0, 0xFF);
    c->r[29] -= 0x20;
    c->r[5] = 0xFF; c->r[6] = 0x18;
    f_0888AC70(c);
    c->r[29] += 0x20;
}

/* (panel): a 0xB0 x 0x18 panel at (0x68, 0xB8), its step by +92 (0x0888AC70). */
SFA_FUNC(0888ABDC, panel_strip_68_b8) {
    gaddr o = ARG(0);
    *B(o + 55) = 7;
    *B(o + 56) = 0;
    *B(o + 2) = 0;
    *B(o + 136) = 0xF0;
    *H(o + 6) = 0x68;
    *H(o + 10) = 0xB8;
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0xB0;
    *H(o + 26) = 0x18;
    colours(o, 0, 8, 0xFF);
    *B(o + 131) = 0;
    c->r[29] -= 0x20;
    c->r[5] = 8; c->r[6] = 0xFF;
    f_0888AC70(c);
    c->r[29] += 0x20;
}

/* (object): GAME +22 = +2 << 8 with the low byte from 0x08A2C62C[+2 * 2 + 1], 0x0951CE52 from its other
   byte, and +14 from 0x08A2C63C[+2]. */
SFA_FUNC(088AA398, set_22_by_2) {
    gaddr o = ARG(0), g = GAME;
    int32_t n = S16(o + 2);
    *H(g + 22) = 0;
    *H(g + 22) = (uint16_t)(S16(g + 22) & 0xFF);
    *H(g + 22) = (uint16_t)(S16(g + 22) | (n << 8));
    *B(o + 14) = (uint8_t)S8(0x08A2C63Cu + (uint32_t)n);
    gaddr e = 0x08A2C62Cu + (uint32_t)(int32_t)(int16_t)(n << 1);
    int32_t v = S8(e);
    *H(g + 22) = (uint16_t)(S16(g + 22) & 0xFF00);
    *H(0x0951CE52u) = (uint16_t)v;
    *H(g + 22) = (uint16_t)(S16(g + 22) | S8(e + 1));
}

/* (object): +13 counts down: at 0 +9 moves on, +80 / +79 cleared and GAME +22's low byte 0x20;
   in between +14 counts down from 4, at 0 the next 0x08B02810 entry (0x088AADD8). */
SFA_FUNC(088AAD44, count_13_14_entries) {
    gaddr o = ARG(0);
    uint8_t n = (uint8_t)(*B(o + 13) - 1);
    *B(o + 13) = n;
    if (n == 0) {
        *B(o + 80) = 0;
        *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
        *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
        *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 0x20);
        *B(o + 79) = 0;
        return;
    }
    int8_t m = (int8_t)(S8(o + 14) - 1);
    *B(o + 14) = (uint8_t)m;
    if (m != 0) return;
    *B(o + 14) = 4;
    c->r[29] -= 0x20;
    c->r[5] = 4;
    f_088AADD8(c);
    c->r[29] += 0x20;
}

/* (object): +4 moves on; set up at (0, 0xF0), picture 0x20, draw state 0x08A2FA88, animation 0x08A2FB78. */
SFA_FUNC(088B1078, setup_0_f0_pic_20) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 6;
    *H(o + 86) = 0xC;
    *W(o + 164) = 0;
    *W(o + 156) = 0x08A2FA88u;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 82) = 0;
    *B(o + 12) = 0xFF;
    *B(o + 13) = 0x20;
    *H(o + 24) = 1;
    *H(o + 26) = 2;
    *H(o + 18) = 0;
    *H(o + 22) = 0xF0;
    c->r[29] -= 0x20;
    c->r[6] = 1; c->r[7] = 6;
    set_anim(c, o, *W(0x08A2FB78u));
    c->r[29] += 0x20;
}

/* (panel): +94 from GAME +16's low byte, +128 moves on; a black panel 0x180 x 0x40 from (0, +8). */
static void panel_black(gaddr o, uint32_t at8) {
    *H(o + 94) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 56) = 1;
    *B(o + 55) = 5;
    *B(o + 3) = 0;
    *B(o + 1) = 0;
    *W(o + 4) = 0;
    *W(o + 8) = at8;
    colours(o, 0, 0, 0);
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0x180;
    *H(o + 26) = 0x40;
}
SFA_FUNC(088B2204, panel_black_108) { (void)c; panel_black(ARG(0), 0x1080000); }
SFA_FUNC(088B2298, panel_black_38)  { (void)c; panel_black(ARG(0), 0x380000); }

/* (object): +82 counts down to its move (+5 = 1): x (+16) by +96, +96 by +104, until y passes +132 from
   the side +98 heads (then held there and +5 = 2). */
SFA_FUNC(088BC868, wait_then_fall_to_132) {
    gaddr o = ARG(0);
    uint32_t s = *B(o + 5);
    if (s == 0) {
        int32_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        if (n < 0) { *B(o + 5) = 1; s = *B(o + 5); }
    }
    if (s != 1) return;
    uint32_t v = *W(o + 96);
    *W(o + 16) += v;
    *W(o + 96) = v + *W(o + 104);
    int32_t t = S16(o + 132), y = S16(o + 18);
    if ((int16_t)((uint32_t)(t - y) ^ (uint32_t)(int32_t)S16(o + 98)) < 0) {
        *B(o + 5) = (uint8_t)(s + 1);
        *H(o + 18) = (uint16_t)t;
    }
}

/* (object): +5 moves on; two kind-0x23 objects (steps 2 and 5) owned by it, the first with a list-3
   object (kind 11) beside it; then the drop (0x088BD268). */
SFA_FUNC(088BD1D4, make_23_pair_then_drop) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    c->r[4] = 0x23; c->r[5] = 2;
    f_08822EAC(c);
    c->r[17] = c->r[2];
    if (c->r[17] != 0) {
        gaddr a = c->r[17];
        *W(a + 88) = o;
        c->r[4] = 0xB; c->r[5] = 0;
        f_08823338(c);
        gaddr b = c->r[2];
        if (b != 0) { *W(b + 124) = a; *W(a + 92) = b; }
        c->r[4] = 0x23; c->r[5] = 5;
        f_08822EAC(c);
        c->r[17] = c->r[2];
        if (c->r[17] != 0) *W(c->r[17] + 88) = o;
    }
    c->r[4] = o;
    f_088BD268(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, side, keys): a fighter's first state: in use, its side (+337, +2), +362 the keys, +420 =
   0x90, +360 / +373 = 1 and the rest of these fields cleared. */
SFA_FUNC(088BE4E0, fighter_first_state) {
    gaddr f = ARG(0);
    uint32_t side = ARG(1), keys = ARG(2);
    *B(f) = 1;
    *B(f + 337) = (uint8_t)side;
    *B(f + 2) = (uint8_t)side;
    *B(f + 373) = 1;
    *B(f + 429) = 0;
    *H(f + 404) = 0;
    *W(f + 400) = 0;
    static const uint16_t BYTES[] = { 432, 413, 431, 428, 385, 430, 450, 386, 361, 411, 407, 451 };
    for (unsigned i = 0; i < sizeof BYTES / sizeof BYTES[0]; i++) *B(f + BYTES[i]) = 0;
    *H(f + 424) = 0;
    *H(f + 426) = 0;
    *B(f + 360) = 1;
    *H(f + 420) = 0x90;
    *H(f + 422) = 0;
    *H(f + 362) = (uint16_t)keys;
    c->r[5] = 0; c->r[7] = 0; c->r[8] = 0; c->r[9] = 1;
}

/* (fighter, effect): the effect takes the fighter's side, colours, picture and place, points back at it
   (+88; the fighter's +228 at the effect, +536 counting them), +260 = 10, +311 = 2, +312 = 8. */
SFA_FUNC(0892B31C, effect_from_fighter) {
    gaddr f = ARG(0), e = ARG(1);
    *B(e + 122) = (uint8_t)S8(f + 122);
    *B(e + 121) = (uint8_t)S8(f + 121);
    *B(e + 346) = (uint8_t)S8(f + 194);
    *B(e + 347) = 0;
    *B(e + 80) = (uint8_t)S8(f + 80);
    *B(e + 255) = 0;
    *B(e + 14) = *B(f + 14);
    *B(e + 11) = *B(f + 11);
    *B(e + 12) = *B(f + 12);
    *B(e + 13) = *B(f + 13);
    *H(e + 38) = *H(f + 38);
    *H(e + 18) = *H(f + 18);
    *H(e + 112) = 0;
    *H(e + 22) = *H(f + 22);
    *W(e + 88) = f;
    *W(f + 228) = e;
    *B(f + 536) = (uint8_t)(*B(f + 536) + 1);
    *B(e + 260) = 10;
    *B(e + 311) = 2;
    *B(e + 312) = 8;
    c->r[4] = 8; c->r[6] = 2;
}

/* (object): its step (+92 kinds other than 4 and 6) from 0x08A3CA7C; kinds 4 and 6 shown (+93) in mode 2
   while GAME +4 is set, never while 0x09525881 is set. */
SFA_FUNC(088CF598, step_or_show_4_6) {
    gaddr o = ARG(0);
    uint32_t k = *B(o + 92);
    c->r[29] -= 0x20;
    if (k == 4 || k == 6) {
        *B(o + 93) = 0;
        if (*H(GAME + 6) == 2) *B(o + 93) = *H(GAME + 4) != 0 ? 1 : 0;
        if (S8(0x09525881u) != 0) *B(o + 93) = 0;
    } else {
        gaddr fn = *W(0x08A3CA7Cu + k * 4);
        c->r[5] = fn; c->r[6] = 0x08A3CA7Cu;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, x): x moved by the push table 0x08A1ECDC: row 4 * +647 (less 16 for character 0x1B with ISM
   1), up to 32 either way; a negative row adds, a positive one takes away. */
SFA_FUNC(08871E40, fighter_push_by_647) {
    gaddr f = ARG(0);
    uint32_t x = ARG(1);
    int32_t r = 0;
    if (S8(f + 338) == 0x1B && S8(f + 386) == 1) r = -16;
    r += S8(f + 647) * 4;
    gaddr t = 0x08A1ECDCu;
    if (r >= 0) {
        if (r >= 33) r = 32;
        RETURN(x - (uint32_t)(int32_t)S8(((uint32_t)r << 7) + x + t));
    } else {
        r = -r;
        if (r >= 33) r = 32;
        RETURN(x + (uint32_t)(int32_t)S8(((uint32_t)r << 7) + x + t));
    }
}

/* (fighter): close enough to throw? In the plain step (0x0887BA84) with the opponent (+92) standing
   (+73 clear, step 1.0, the frame's first byte set), +235 set and the level +508 under 48. */
SFA_FUNC(0887B9EC, fighter_can_throw) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    f_0887BA84(c);
    uint32_t ok = 0;
    if (c->r[2] != 0) {
        gaddr q = *W(f + 92);
        ok = S8(q + 73) == 0 && S16(f + 508) < 48 && S8(q + 235) != 0 && *B(q + 4) == 1 && *B(q + 5) == 0
             && *B(*W(q + 296)) != 0;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
    RETURN(ok);
}

/* (): both fighters' characters and ISMs at random (P2 first), P1 in its second colour when they match,
   and the stage (BATTLE +0) picked at random by P1's character: 10 and 31 become 30. */
SFA_FUNC(088BE178, random_fighters_and_stage) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    gaddr p1 = 0x092E3EA0u, p2 = 0x092E4338u;
    c->r[29] -= 0x20;
    c->r[16] = p2; c->r[18] = p1;
    c->r[4] = p2; f_088BE210(c);
    c->r[4] = p1; f_088BE210(c);
    int32_t a = S8(p2 + 338), b = S8(p1 + 338);
    c->r[16] = 0x1E; c->r[17] = 0x092E0000u;
    c->r[5] = (uint32_t)a;
    if (a == b) { *B(p1 + 376) = 1; c->r[5] = 1; }
    c->r[4] = (uint32_t)b;
    f_0883021C(c);
    *H(BATTLE) = (uint16_t)c->r[2];
    int32_t s = S16(BATTLE);
    if (s == 10 || s == 0x1E || s == 0x1F) *H(BATTLE) = 0x1E;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}
