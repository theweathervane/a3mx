/* More steps of objects and fighters (0.105.0 on): set-ups by type, speeds that turn round, effects made
 * beside a fighter, the human's and CPU's guard choice, and the fighters drawn. Offsets stay numbers until
 * something shows what they are. Arguments of native callees are set as the original passes them; s0 - s4
 * as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08821368(Cpu *c);   /* game_random */
void f_08877258(Cpu *c);   /* fighter_chance_by_level */
void f_0880BA94(Cpu *c);   /* object_draw_with_parts */
void f_08876894(Cpu *c);   /* fighter_frame_record */
void f_08881D9C(Cpu *c);   /* setup_kind_at */
void f_088BD334(Cpu *c);   /* make_five_23 */
void f_088B098C(Cpu *c);   /* swap_96_74_100_28_a */
void f_088B09B8(Cpu *c);   /* tick_32_34_a */
void f_088B48F0(Cpu *c);   /* swap_96_74_100_28_b */
void f_088B4ABC(Cpu *c);   /* tick_32_34_b */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }
static void colours(gaddr o, uint8_t a, uint8_t b, uint8_t d) {
    for (int k = 36; k <= 39; k++) *B(o + k) = a;
    for (int k = 42; k <= 45; k++) *B(o + k) = b;
    for (int k = 48; k <= 51; k++) *B(o + k) = d;
}

/* ---------------------------------------------------------------- one-call wrappers */

/* (object): the swap 0x088B098C / 0x088B48F0, then the counters' tick 0x088B09B8 / 0x088B4ABC. */
static void swap_then_tick(Cpu *c, void (*swap)(Cpu *), void (*tick)(Cpu *)) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    swap(c);
    c->r[4] = o;
    tick(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(088B0960, swap_then_tick_a) { swap_then_tick(c, f_088B098C, f_088B09B8); }
SFA_FUNC(088B48C4, swap_then_tick_b) { swap_then_tick(c, f_088B48F0, f_088B4ABC); }

/* (record): +6 moves on, +16 = 0xF0, and the five kind-0x23 objects (0x088BD334). */
SFA_FUNC(088BC044, next_6_make_five) {
    gaddr r = ARG(0);
    uint32_t n = (uint32_t)*H(r + 6) + 1;
    *H(r + 6) = (uint16_t)n;
    *H(r + 16) = 0xF0;
    c->r[29] -= 0x10;
    c->r[5] = n; c->r[6] = 0xF0;
    f_088BD334(c);
    c->r[29] += 0x10;
}

/* (fighter, n): its frame record (+28) becomes entry n of +352's list, read (0x08876894); +152 = 1. */
SFA_FUNC(0887684C, fighter_set_frame) {
    gaddr f = ARG(0), s16 = c->r[16];
    uint32_t rec = *W(*W(f + 352) + (ARG(1) & 0xFFFF) * 4);
    *W(f + 28) = rec;
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[4] = f; c->r[5] = rec;
    f_08876894(c);
    *W(f + 152) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the four fighters drawn, the fourth first (0x092E22F4, F0, EC, E8). */
SFA_FUNC(0880BA48, draw_four_fighters) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = 0x092E0000u; c->r[17] = 0x092E22E8u;
    c->r[4] = *W(0x092E22F4u); f_0880BA94(c);
    c->r[4] = *W(0x092E22F0u); f_0880BA94(c);
    c->r[4] = *W(0x092E22ECu); f_0880BA94(c);
    c->r[4] = *W(0x092E22E8u); f_0880BA94(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +4 moves on; set up as kind 0 at (0xB0, 0x88) with the animation 0x08A29A00. */
SFA_FUNC(088813EC, setup_b0_88) {
    gaddr o = ARG(0), s16 = c->r[16];
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 0; c->r[6] = 0xB0; c->r[7] = 0x88;
    f_08881D9C(c);
    set_anim(c, o, *W(0x08A299FCu + 4));
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- objects */

/* (object): moving by speeds +96 / +98 (changed by +100 / +102) on the axes +104 / +105 switch on, for +73
   frames; then +5 moves on, +79 = 2, and each moving axis turns round: a speed going the negative way is
   reversed (and +106 / +107 = 0), a positive one kept with its acceleration reversed (+106 / +107 = -1);
   the acceleration doubles either way. */
SFA_FUNC(088B5578, move_then_turn) {
    gaddr o = ARG(0);
    for (int axis = 0; axis < 2; axis++) {
        if (S8(o + 104 + axis) == 0) continue;
        gaddr sp = o + 96 + axis * 2;
        *H(sp) = (uint16_t)(S16(sp) + S16(sp + 4));
        *W(o + 16 + axis * 4) += (uint32_t)(int32_t)S16(sp) << 8;
    }
    int8_t left = (int8_t)(S8(o + 73) - 1);
    *B(o + 73) = (uint8_t)left;
    if (left != 0) return;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *B(o + 79) = 2;
    for (int axis = 0; axis < 2; axis++) {
        if (S8(o + 104 + axis) == 0) continue;
        gaddr sp = o + 96 + axis * 2;
        int16_t v = S16(sp), acc = S16(sp + 4);
        if (v == 0) continue;
        if (v > 0) {
            *H(sp) = (uint16_t)v;
            *H(sp + 4) = (uint16_t)(int16_t)((int16_t)-acc * 2);
            *B(o + 106 + axis) = 0xFF;
        } else {
            *H(sp) = (uint16_t)(int16_t)-v;
            *H(sp + 4) = (uint16_t)(int16_t)(acc * 2);
            *B(o + 106 + axis) = 0;
        }
    }
}

/* (object): +4 moves on; by type +3: kind (+148 / +86 / +164: types 0 - 3, 8: 7 / 0x13 / 1; 4 - 6, 9:
   0 / 0xC / 0; 7: 0 / 0x14 / 1), +160 = 0x7F under 4, +12 / +13 (8: 1 / 0; 4, 5: 0xFF / 0x20); place, +100,
   +14 and the animation from the 10-byte entry at 0x08A2EA10. */
SFA_FUNC(088AED5C, setup_by_type_ea10) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    uint32_t t = *B(o + 3);
    if ((int32_t)t < 4) *B(o + 160) = 0x7F;
    if (t < 10) {
        static const uint8_t KIND[10] = { 0, 0, 0, 0, 1, 1, 1, 2, 0, 1 };
        switch (KIND[t]) {
        case 0: *W(o + 148) = 7; *H(o + 86) = 0x13; *W(o + 164) = 1; break;
        case 1: *W(o + 148) = 0; *H(o + 86) = 0xC;  *W(o + 164) = 0; break;
        default: *W(o + 148) = 0; *H(o + 86) = 0x14; *W(o + 164) = 1; break;
        }
    }
    *H(o + 32) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *B(o + 78) = 0;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *H(o + 26) = 1;
    *H(o + 24) = 5;
    if (t == 8) { *B(o + 12) = 1; *B(o + 13) = 0; }
    else if (t == 4 || t == 5) { *B(o + 12) = 0xFF; *B(o + 13) = 0x20; }
    gaddr e = 0x08A2EA10u + t * 10;
    *H(o + 18) = *H(e);
    *H(o + 22) = *H(e + 2);
    *W(o + 100) = *H(e + 4);
    *B(o + 14) = (uint8_t)*H(e + 8);
    c->r[29] -= 0x20;
    set_anim(c, o, *W(0x08A2E988u + (uint32_t)*H(e + 6) * 4));
    c->r[29] += 0x20;
}

/* (object): +128 on, shown (+93), +55 = 7, +3 = the owner's (+124) +14, +136 = 0x80; at the owner's place.
   Without +92: 0xBB right and 0x60 up unless the character (0x08ACCD54 by 0x09525DE8's +8) is 1, a
   0x120 x 0x7E white box. With +92 = 1 / 2 / 3: a 0x60 x 0x10 box in red, green or blue tints. */
SFA_FUNC_WRAPPED(088BDBB8, setup_name_box, i_ism_name_box) {
    gaddr o = ARG(0), w = *W(o + 124);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 55) = 7;
    *B(o + 2) = 0;
    *B(o + 3) = *B(w + 14);
    *B(o + 136) = 0x80;
    uint32_t k = *B(o + 92);
    int32_t x = S16(w + 18);
    if (k == 0) {
        if (*B(0x08ACCD54u + (uint32_t)(int32_t)S8(*W(0x09525DE8u) + 8)) != 1) {
            *H(o + 6) = (uint16_t)(x + 0xBB);
            *H(o + 10) = (uint16_t)(S16(w + 22) - 0x60);
        } else {
            *H(o + 6) = (uint16_t)x;
            *H(o + 10) = (uint16_t)S16(w + 22);
        }
        *B(o + 56) = 3;
        *H(o + 12) = 0xFF70;
        *H(o + 24) = 0x4F;
        *H(o + 14) = 0x120;
        *H(o + 26) = 0x7E;
        colours(o, 0xFF, 0xFF, 0xFF);
        return;
    }
    *H(o + 6) = (uint16_t)x;
    *H(o + 10) = (uint16_t)S16(w + 22);
    *B(o + 56) = 0;
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0x60;
    *H(o + 26) = 0x10;
    if (k == 1) colours(o, 0xE0, 0x50, 0);
    else if (k == 2) colours(o, 0x50, 0xE0, 0);
    else if (k == 3) colours(o, 0x50, 0x50, 0xE0);
}

/* (fighter, owner, description): a kind-4 effect (type the description's +20) made beside the fighter
   (or the owner, with +14): +88 the owner, +119 / +121 / +135 / +139 from the description, the fighter and
   the owner's +112; its place a random spread (+4 / +6 wide) around the offset (+0 / +2), mirrored by the
   facing; facing ^ the description's +11; kind 5. */
SFA_FUNC(0887DD64, make_effect_beside) {
    gaddr f = ARG(0), w = ARG(1), d = ARG(2);
    gaddr s[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    c->r[16] = d; c->r[17] = w; c->r[18] = f;
    c->r[4] = 4; c->r[5] = *B(d + 20);
    f_08822EAC(c);
    gaddr x = c->r[2];
    c->r[20] = x;
    if (x != 0) {
        *W(x + 88) = w;
        *B(x + 119) = *B(d + 15);
        *B(x + 135) = 0xFF;
        *B(x + 121) = (uint8_t)S8(f + 121);
        *H(x + 82) = 0;
        *B(x + 139) = 0;
        if (S16(w + 112) < 0) *B(x + 139) = 2;
        f_08821368(c);
        int32_t h6 = S16(d + 6);
        int16_t dy = (int16_t)((int32_t)(c->r[2] & (uint32_t)h6) - (h6 >> 1) + S16(d + 2));
        c->r[19] = (uint32_t)(int32_t)dy;
        f_08821368(c);
        int32_t h4 = S16(d + 4);
        int16_t dx = (int16_t)((int32_t)(c->r[2] & (uint32_t)h4) - (h4 >> 1) + S16(d + 0));
        gaddr at = *B(d + 14) == 0 ? f : w;
        uint8_t facing = (uint8_t)(*B(at + 11) & 1);
        *B(x + 11) = facing;
        if (facing) dx = (int16_t)-dx;
        *H(x + 18) = (uint16_t)(S16(at + 18) + dx);
        *H(x + 22) = (uint16_t)(S16(at + 22) + dy);
        uint32_t turned = (uint32_t)*B(x + 11) ^ *B(d + 11);
        c->r[18] = turned;
        *B(x + 11) = (uint8_t)turned;
        *H(x + 86) = 5;
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4];
}

/* ---------------------------------------------------------------- fighters */

/* (fighter): while +112 is not negative, the count +82 runs down. At 0 the CPU guards (+651 = 1, +698 = 0)
   by a chance from 0x08A22AA0. Before 0 a human guards when holding back (+752 bits 0 - 1) with one of
   eight button pairs pressed (+760); in mode 2 at count 6, a side marked in 0x0950FF6B guards as well
   (in half the cases when 0x0950FF6A is 2). */
SFA_FUNC(0885ECE8, fighter_guard_choice) {
    gaddr f = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17];
    if (S16(f + 112) < 0) return;
    int32_t n = S16(f + 82);
    if (n == 0) return;
    *H(f + 82) = (uint16_t)(n - 1);
    int16_t left = (int16_t)(n - 1);
    int cpu = S8(f + 373) != 0;
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (left == 0) {
        if (cpu) {
            c->r[4] = f; c->r[5] = 0x08A22AA0u;
            f_08877258(c);
            if (c->r[2] != 0) { *B(f + 698) = 0; *B(f + 651) = 1; }
        }
    } else if (!cpu) {
        int checked = 0;
        if (S8(GAME + 114) == 2 && S16(f + 82) == 6) {
            c->r[17] = 1;
            uint32_t bit = 1u << (S8(f + 337) & 31);
            if (*B(0x0950FF6Bu) & bit) {
                checked = 1;
                if (*B(0x0950FF6Au) != 2) { *B(f + 698) = 0; *B(f + 651) = 1; }
                else {
                    f_08821368(c);
                    if ((c->r[2] & 1) == 0) { *B(f + 698) = 0; *B(f + 651) = 1; }
                }
            }
        }
        if (!checked && (*H(f + 752) & 3) != 0) {
            uint32_t p = *H(f + 760) & 0x3F0;
            if (p == 0x280 || p == 0x380 || p == 0x60 || p == 0x70 || p == 0x300 || p == 0x180 || p == 0x30 || p == 0x50) {
                *B(f + 698) = 0; *B(f + 651) = 1;
            }
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

void f_08822F54(Cpu *c);   /* object_make_42 */
void f_088988FC(Cpu *c);   /* fighter_display_side */
void f_088215C4(Cpu *c);   /* object_draw */

/* (object, source): once GAME + 10's low byte is set, +4 moves on and the object copies the source's kind,
   facing, place, size and frame, with +9 = 3, +12 = 0xFF, +75 = 0x40, +13 and +96 from the tables at
   0x08A2E198 / 0x08A2E190 by +78; +171 by GAME + 22's high byte (0, 1, 2, 7: 1) unless 0x09525DE3; with
   neither, a kind-0x23 object of type +78 + 6 is made and kept at +44 (its +88 this one). */
SFA_FUNC(088AE5A8, copy_source_make_23) {
    gaddr o = ARG(0), src = ARG(1), s16 = c->r[16];
    if ((*H(GAME + 10) & 0xFF) == 0) return;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 32) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *W(o + 148) = *W(src + 148);
    *H(o + 86) = *H(src + 86);
    *B(o + 9) = 3;
    *W(o + 164) = *W(src + 164);
    *B(o + 11) = *B(src + 11);
    *B(o + 12) = 0xFF;
    *B(o + 14) = *B(src + 14);
    *B(o + 15) = *B(src + 15);
    *H(o + 18) = (uint16_t)S16(src + 18);
    *H(o + 22) = (uint16_t)S16(src + 22);
    *H(o + 24) = *H(src + 24);
    *H(o + 26) = *H(src + 26);
    *H(o + 38) = 0;
    *W(o + 28) = *W(src + 28);
    *B(o + 75) = 0x40;
    *H(o + 18) = (uint16_t)S16(src + 18);
    int32_t k = S8(o + 78);
    *B(o + 13) = (uint8_t)(*B(o + 13) + S16(0x08A2E198u + (uint32_t)(k * 2)) + 3);
    *W(o + 96) = (uint32_t)(int32_t)S16(0x08A2E190u + (uint32_t)(k * 2));
    int32_t g = (S16(GAME + 22) & 0xFF00) >> 8;
    if (S8(0x09525DE3u) != 0) *B(o + 171) = 0;
    else *B(o + 171) = (g == 0 || g == 1 || g == 2 || g == 7) ? 1 : 0;
    if (S8(0x09525DE3u) != 0) return;
    if (((S16(GAME + 22) & 0xFF00) >> 8) != 0) return;
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = 0x23; c->r[5] = (uint32_t)(k + 6) & 0xFF;
    f_08822F54(c);
    gaddr x = c->r[2];
    if (x != 0) {
        *B(x) = 1;
        *W(x + 88) = o;
        *W(o + 44) = x;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): a step of name entry, by its side's pad (0x08E75BA0 + 4 / + 12, or + 48 / + 56): nothing held,
   +349 = 0xF0; otherwise +349 grows by 16 and, when it wraps, the letter at +456 + the cursor (+463)
   changes: a button enters it (0x2F fills the rest with 0x2D and ends; 0x2E deletes back; at the third
   letter the next becomes 0x2F), up / down step through the letters (0x2F / 0x2E / 0 at the ends). */
SFA_FUNC(0889229C, name_entry_step) {
    gaddr f = ARG(0);
    const gaddr P = 0x08E75BA0u;
    int second = S8(f + 337) != 0;
    uint32_t held = *W(P + (second ? 48 : 4)) & 0xFFFF;
    uint32_t press = *W(P + (second ? 56 : 12)) & 0xFFFF;
    if (held == 0) { *B(f + 349) = 0xF0; return; }
    uint8_t t = (uint8_t)(*B(f + 349) + 0x10);
    *H(f + 346) = 0x258;
    *B(f + 349) = t;
    if (t >= 16) return;
    int32_t pos = S8(f + 463);
    gaddr at = f + 0x1C8 + (uint32_t)pos;
    int32_t ch = S8(at);
    if ((press & 0x3F0) != 0) {
        if (ch == 0x2F) {
            do {
                *B(at) = 0x2D;
                *B(f + 463) = (uint8_t)(S8(f + 463) + 1);
                at++;
            } while (S8(f + 463) != 4);
            *B(f + 459) = 0x2D;
        } else if (ch == 0x2E) {
            uint8_t put = pos == 3 ? 0x2D : 0x30;
            *B(f + 463) = (uint8_t)(pos - 1);
            if ((int8_t)(pos - 1) < 0) { *B(f + 463) = 0; put = 0x2E; }
            *B(at) = put;
        } else {
            *B(at + 1) = (uint8_t)ch;
            *B(f + 463) = (uint8_t)(S8(f + 463) + 1);
            if (S8(f + 463) == 3) *B(at + 1) = 0x2F;
        }
        return;
    }
    int8_t put = (int8_t)ch;
    if (held & 2) {
        put = (int8_t)(ch - 1);
        if (put < 0) put = 0x2F;
        else if (pos == 3) {
            if (!(put < 46)) { *B(at) = (uint8_t)put; return; }
            put = 0x2F;
        }
    } else if (held & 1) {
        put = (int8_t)(ch + 1);
        if (!(put < 48)) put = pos != 3 ? 0 : 0x2E;
    }
    *B(at) = (uint8_t)put;
}

/* (object): drawn; first time (+5), unless BATTLE + 81, after +82 frames: +5 = 1, its owner's (+88) +128 =
   2, +13 / +12 the fighter's (+92) display side (0xFF with it), kind 8, GAME + 20 gets bit 5 + side; the
   side's 8-byte entry at 0x08A287FE gets its picture (+4: 0x127 + the character's byte at 0x08ACCAE6, or
   0x14C in layouts 1 / 2 for some) and becomes the animation. */
SFA_FUNC(088980F4, show_fighter_label) {
    gaddr o = ARG(0);
    gaddr s[7] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22] };
    c->r[29] -= 0x30;
    c->r[16] = o; c->r[17] = 0; c->r[20] = *B(o + 5);
    int draw = 1;
    if (*B(o + 5) == 0) {
        c->r[18] = BATTLE;
        int go = 1;
        if (S8(BATTLE + 81) == 0) {
            *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
            if (S16(o + 82) != 0) { go = 0; draw = 0; }
        }
        if (go) {
            *B(o + 5) = 1;
            gaddr w = *W(o + 88);
            c->r[20] = GAME; c->r[21] = 1; c->r[22] = 0x08A287FEu;
            if (w != 0) *B(w + 128) = 2;
            gaddr fi = *W(o + 92);
            c->r[19] = fi;
            c->r[4] = fi;
            f_088988FC(c);
            uint32_t side = c->r[2];
            uint8_t mark = (side & 0xFF) != 0 ? 0xFF : 0;
            c->r[17] = mark;
            *B(o + 13) = (uint8_t)side;
            *B(o + 12) = mark;
            *W(o + 148) = 0;
            *H(o + 86) = 8;
            *H(GAME + 20) = (uint16_t)((uint32_t)(int32_t)S16(GAME + 20) | (0x20u << (side & 31)));
            int32_t own = S8(fi + 337);
            c->r[17] = (uint32_t)own;
            uint32_t layout_bit = 1u << (S16(BATTLE + 56) & 31);
            gaddr entry = (uint32_t)(own & 0xFF) * 8 + 0x08A287FEu;
            int32_t ch = S8(fi + 338);
            int plain = (layout_bit & 6) == 0 || ch == 10 || (S8(GAME + 57) & (1 << (own & 31))) != 0 ||
                        S8(BATTLE + 79) != 0 || S8(o + 127) == 4;
            *H(entry + 4) = plain ? (uint16_t)(*B(0x08ACCAE6u + (uint32_t)ch) + 0x127) : 0x14C;
            set_anim(c, o, entry);
        }
    }
    if (draw) { c->r[4] = o; f_088215C4(c); }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4]; c->r[21] = s[5]; c->r[22] = s[6];
}

void f_0889CFE8(Cpu *c);   /* record_setup_layer */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_088980F4(Cpu *c);   /* show_fighter_label */

/* (fighter, damage): the damage a guard takes off. A human guard (no button +760 against a human
   attacker, the hit record 0x09358410 not marked +44, and back held now but not before: +754 / +756 / +758),
   or in mode 2 a side marked in 0x0950FF6F (bit 7: in half the cases), sets +669 = 4 and +670 = 11, and the
   damage is cut by the row 0x08A21EAC[+647 + kind] (0: by half, 1 - 4: by 1/4 - 1/32; +647 from 16: 1/32). */
SFA_FUNC(08871ED0, guard_damage_cut) {
    gaddr f = ARG(0);
    uint32_t dmg = ARG(1);
    gaddr s[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    const gaddr HIT = 0x09358410u;
    gaddr att = *W(HIT + 24);
    uint32_t kind = 0;
    int guard = 0;
    c->r[29] -= 0x20;
    c->r[16] = dmg; c->r[17] = f; c->r[18] = 0; c->r[19] = att; c->r[20] = HIT;
    if (S8(GAME + 114) == 2) {
        uint32_t m = *B(0x0950FF6Fu);
        if (m & (1u << (S8(f + 337) & 31))) {
            if ((m & 0x80) == 0) guard = 1;
            else { f_08821368(c); if (c->r[2] & 0x80) guard = 1; }
        }
    }
    if (!guard && S8(f + 373) == 0 && *B(HIT + 44) == 0) {
        int go = 1;
        if (S8(att + 373) == 0 && *H(att + 760) != 0) go = 0;
        if (go) {
            if (*H(f + 760) != 0) guard = 1;
            else {
                uint32_t now = *H(f + 756);
                kind = (*H(f + 754) & ~now) == 0 ? 2 : 1;
                c->r[18] = kind;
                if ((now & ~(uint32_t)*H(f + 758)) != 0) guard = 1;
            }
        }
    }
    if (guard) {
        *B(f + 669) = 4;
        *B(f + 670) = 0xB;
        int32_t row = S8(f + 647);
        uint32_t cut = 4;
        if (row < 16) cut = (uint32_t)(int32_t)S8(0x08A21EACu + (uint32_t)row + kind);
        if (cut < 5) {
            if (cut == 0) dmg = dmg >> 1;
            else dmg -= dmg >> (cut + 1);
        }
    }
    RETURN(dmg);
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4];
}

/* (fighter): a fighter's state cleared for a round (some 60 flags and counters, y = +132, +268 = 0,
   facing = +697); the opponent's (+92) +592 / +770 settle (+270 set where they end). */
SFA_FUNC(0886731C, fighter_round_clear) {
    gaddr f = ARG(0), w = *W(f + 92);
    int32_t st = S8(w + 592);
    if (st != 0) {
        if (S8(w + 773) != 0) *B(w + 270) = 1;
        else if (st == 3) { *B(w + 592) = 0; *B(w + 270) = 1; st = S8(w + 592); }
        if (st == 1) *B(w + 592) = 2;
    }
    if (S8(w + 770) != 0) { *B(w + 270) = 1; *B(w + 770) = 0; *B(w + 592) = 0; }
    *B(w + 587) = 0; *B(w + 586) = 0;
    static const uint16_t CLEAR[] = { 542, 541, 537, 239, 237, 698, 73, 773, 1146, 1145, 372, 768, 713, 310, 260,
        606, 693, 604, 538, 691, 671, 596, 234, 770, 592, 689, 539, 281, 282, 591, 559, 647, 595, 588, 575, 270,
        651, 503, 500, 496, 645, 644, 641, 81, 80, 637, 668, 627, 626, 625, 624, 262, 568, 567, 549, 563, 562,
        561, 560, 553, 551, 702 };
    *W(f + 20) = (uint32_t)(int32_t)S16(f + 132) << 16;
    *H(f + 268) = 0;
    for (unsigned i = 0; i < sizeof CLEAR / sizeof CLEAR[0]; i++) *B(f + CLEAR[i]) = 0;
    if (S8(f + 338) == 0x1C) { *W(f + 824) = 0; *W(f + 820) = 0; }
    *B(f + 656) = 0;
    *B(f + 11) = (uint8_t)S8(f + 697);
    if (S8(f + 251) == 0) *B(f + 235) = 0;
    if (S8(w + 251) == 0) *B(f + 502) = 0;
    *B(f + 777) = 0;
}

/* (object): as 0x088B5578's moves, the speeds' signs flipped by +106 / +107; an axis moving the negative
   way that reaches its target (+92 / +94) stops there, turns its speed round and sets its bit in +6; with
   any bit set +6 clears and +79 counts down; at 0 +5 moves on and bit +3 of GAME + 22's high byte is set. */
SFA_FUNC(088B56E4, bounce_at_target) {
    gaddr o = ARG(0);
    for (int axis = 0; axis < 2; axis++) {
        if (S8(o + 104 + axis) == 0) continue;
        gaddr sp = o + 96 + axis * 2;
        int16_t v = (int16_t)(S16(sp) + S16(sp + 4));
        *H(sp) = (uint16_t)v;
        if (S8(o + 106 + axis) != 0) v = (int16_t)-v;
        *W(o + 16 + axis * 4) += (uint32_t)(int32_t)v << 8;
        if (S16(sp) >= 0) continue;
        int16_t target = S16(o + 92 + axis * 2);
        int16_t diff = (int16_t)(S16(o + 18 + axis * 4) - target);
        if (diff != 0 && (int16_t)(diff ^ v) < 0) continue;
        *H(o + 18 + axis * 4) = (uint16_t)target;
        *H(o + 16 + axis * 4) = 0;
        *H(sp) = (uint16_t)(-S16(sp));
        uint8_t bits = *B(o + 6), bit = (uint8_t)(1 << axis);
        if ((bits & bit) == 0) *B(o + 6) = (uint8_t)(bits | bit);
    }
    if (*B(o + 6) == 0) return;
    *B(o + 6) = 0;
    uint8_t n = (uint8_t)(*B(o + 79) - 1);
    *B(o + 79) = n;
    if (n != 0) return;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    uint32_t bit = (1u << (*B(o + 3) & 31)) << 8, now = (uint32_t)(int32_t)S16(GAME + 22);
    if ((now & 0xFF00 & bit) == 0) *H(GAME + 22) = (uint16_t)(now | bit);
}

/* (object): a fighter's label: first time (+5) its record set up (layer 0), +12 = 1, +13 = 0x1B + the
   display side, x from 0x08A2A05C by side, y 0x1F (in layout 3 by the fighter's side, and in modes 0xD /
   0x13 / 0x11 or some others from 0x08A2A060 for sides 2 - 3). Then once BATTLE + 81 or GAME + 20 reaches
   0x1F, +4 moves on: with + 81 the label shown (0x088980F4), otherwise a kind-5 object of type 0xC made
   (its +124 this one) and kept at +88, +82 = 0x12. */
SFA_FUNC(08897F34, fighter_label_setup) {
    gaddr o = ARG(0);
    gaddr s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = *B(o + 5); c->r[18] = BATTLE;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        gaddr fi = *W(o + 92);
        c->r[17] = fi;
        c->r[4] = o; c->r[5] = 0;
        f_0889CFE8(c);
        c->r[4] = fi;
        f_088988FC(c);
        int16_t side = (int16_t)(c->r[2] & 0xFF);
        *B(o + 12) = 1;
        *B(o + 13) = (uint8_t)(side + 0x1B);
        *H(o + 18) = (uint16_t)S16(0x08A2A05Cu + (uint32_t)(side * 2));
        uint16_t y = 0x1F;
        if (S16(BATTLE + 56) == 3) y = (uint16_t)S16(0x08A2A060u + (uint32_t)(S8(fi + 337) * 2));
        int32_t mode = S8(GAME + 114);
        int by_side;
        if (mode == 0xD || mode == 0x13 || mode == 0x11) by_side = 1;
        else if (S8(BATTLE + 79) != 0 || S8(GAME + 137) != 0) by_side = S8(o + 127) == 4;
        else if (S8(BATTLE + 159) != 0) by_side = S8(o + 127) == 4;
        else by_side = 0;
        if (by_side) {
            int32_t own = S8(fi + 337);
            c->r[17] = (uint32_t)own;
            if (own & 2) y = (uint16_t)S16(0x08A2A060u + (uint32_t)((own & 1) * 2));
        }
        *H(o + 22) = y;
    } else if (S8(BATTLE + 81) != 0) {
        uint8_t k = *B(o + 4);
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(k + 1);
        c->r[4] = o;
        f_088980F4(c);
    } else if ((S16(GAME + 20) & 0xFF) == 0x1F) {
        uint8_t k = *B(o + 4);
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(k + 1);
        c->r[4] = 5; c->r[5] = 0xC;
        f_08823338(c);
        gaddr x = c->r[2];
        if (x != 0) *W(x + 124) = o;
        *W(o + 88) = x;
        *H(o + 82) = 0x12;
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}
