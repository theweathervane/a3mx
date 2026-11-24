/* More steps (0.124.0): screens that reset the fight as they start, a hit's effect step and its score,
   a hit's damage worked out, a character's data unpacked (two sets), whether an input counts for a
   fighter (Training's settings, auto-guard) and a fighter's name panel with its win count. Offsets stay
   numbers until something shows what they are. Arguments of callees, and what the original leaves in
   a0 - t0 at a call, are set as the original sets them; s0 - s8 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define HIT     0x09358410u

#include "../guest_mem.h"

void f_08821FC8(Cpu *c);   /* fight_reset_objects */
void f_08821F9C(Cpu *c);   /* fight_reset_layers */
void f_08821FEC(Cpu *c);   /* fight_reset_all */
void f_0880A39C(Cpu *c);   /* fill_092DCA40 */
void f_088586C8(Cpu *c);   /* hit_score_points */
void f_08821368(Cpu *c);   /* game_random */
void f_08873054(Cpu *c);   /* hit_less_for_the_lead */
void f_08873094(Cpu *c);   /* scale_by_setting */
void f_08873160(Cpu *c);   /* hit_scale_by_count */
void f_0886E638(Cpu *c);   /* unpack_offsets */
void f_0880A6CC(Cpu *c);   /* game_text */

/* ---------------------------------------------------------------- screens starting */

/* The step words +2 .. +22 a screen record clears as it starts (all but +12) */
static void clear_steps(gaddr r, int with_14, int with_2_4) {
    *H(r + 22) = 0; *H(r + 20) = 0; *H(r + 18) = 0; *H(r + 16) = 0;
    if (with_14) *H(r + 14) = 0;
    *H(r + 10) = 0; *H(r + 8) = 0; *H(r + 6) = 0;
    if (with_2_4) { *H(r + 4) = 0; *H(r + 2) = 0; }
}

/* (screen): unless +38, its next step (+4) with +6 - +22 cleared, and the fight reset. */
SFA_FUNC(08896190, screen_start_reset) {
    gaddr r = ARG(0);
    c->r[29] -= 0x10;
    if (S8(r + 38) == 0) {
        c->r[5] = (uint32_t)*H(r + 4) + 1;
        *H(r + 22) = 0;
        *H(r + 4) = (uint16_t)c->r[5];
        *H(r + 20) = 0; *H(r + 18) = 0; *H(r + 16) = 0; *H(r + 10) = 0; *H(r + 8) = 0; *H(r + 6) = 0;
        f_08821FC8(c);
    }
    c->r[29] += 0x10;
}

/* (screen, record): its next screen (+0) with +2 - +22 cleared, the record's +6 = 0, and the fight reset
   with the layers. */
SFA_FUNC(089DE3F4, screen_next_reset_layers) {
    gaddr r = ARG(0), b = ARG(1);
    c->r[29] -= 0x10;
    c->r[6] = (uint32_t)*H(r) + 1;
    *H(r) = (uint16_t)c->r[6];
    clear_steps(r, 1, 1);
    *B(b + 6) = 0;
    f_08821F9C(c);
    c->r[29] += 0x10;
}

/* (): GAME's next screen with its steps cleared, and the whole fight reset. */
SFA_FUNC(088B2FE8, game_next_screen_reset) {
    c->r[29] -= 0x10;
    clear_steps(GAME, 1, 1);
    uint32_t n = *H(GAME);
    c->r[4] = n + 1;
    *H(GAME) = (uint16_t)c->r[4];
    c->r[5] = 0x08E70000u; c->r[6] = GAME; c->r[7] = n;
    f_08821FEC(c);
    c->r[29] += 0x10;
}

/* (screen): unless +93, its next screen with the steps cleared, the whole fight reset and 0x092DCA40
   filled. */
SFA_FUNC(088BE118, screen_next_reset_all) {
    gaddr r = ARG(0);
    c->r[29] -= 0x10;
    if (S8(r + 93) == 0) {
        c->r[5] = (uint32_t)*H(r) + 1;
        *H(r) = (uint16_t)c->r[5];
        clear_steps(r, 1, 1);
        f_08821FEC(c);
        f_0880A39C(c);
    }
    c->r[29] += 0x10;
}

/* ---------------------------------------------------------------- hits */

/* (effect, fighter): the effect's step +6: from the fighter's +356 (halved, then cleared), or from
   0x08A17170 by the fighter's +769 when it changes (+141); a fighter's +248 with bit 7 (a hit to score)
   gives its step from 0x08A17160 (unless negative, or the step is already 7 or more), loses the bit and
   scores; then the step's routine (0x08A17120). */
SFA_FUNC(08857BD0, hit_effect_step) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[17] = o; c->r[16] = f;
    int32_t v = S8(f + 356);
    if (v != 0) {
        *B(o + 6) = (uint8_t)(v >> 1);
        *B(o + 7) = 0;
        *B(f + 356) = 0;
    } else {
        int8_t a = S8(f + 769);
        if (a != S8(o + 141)) {
            *B(o + 141) = (uint8_t)a;
            int8_t t = S8(0x08A17170u + (uint32_t)(int32_t)S8(o + 141));
            if (t != 0) { *B(o + 6) = (uint8_t)t; *B(o + 7) = 0; }
        }
    }
    int32_t h = S8(f + 248);
    c->r[6] = 0x08A10000u;
    if (h < 0) {
        int8_t step = S8(0x08A17160u + (uint32_t)(h & 0xF));
        c->r[6] = 0x08A17160u;
        if (step >= 0) {
            c->r[6] = *B(o + 6) < 7;
            if (c->r[6]) {
                *B(o + 6) = (uint8_t)step;
                h = S8(f + 248);
            }
        }
        *B(f + 248) = (uint8_t)(h & 0x7F);
        c->r[4] = o; c->r[5] = (uint32_t)h;
        f_088586C8(c);
    }
    gaddr fn = *W(0x08A17120u + (uint32_t)*B(o + 6) * 4);
    c->r[4] = o; c->r[5] = f;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, hit, strength): a hit's damage. 0 for no strength. The base by character and level
   (0x08A1D0C8, 32 a character), less 4 for ISM -1 and 24 with +309, less for a lead in health; plus the
   combo's row (+126, from 31: 31; 0x08A21820 by the hit's +29, or row 1 with +637) and, at 48 or less
   health, a random 0x08A1DB48 term (and another, 0x08A1DB68, at 16 or less). The strength (x 4 against an
   opponent holding +792 0x200 in a guard state, 0 - 127) through 0x08A1DC5C by the total (to 32), or
   added to 0x08A1ECDC's entry for a negative total; then scaled by the setting and the hit count;
   1 - 127. */
SFA_FUNC(08872D20, hit_damage) {
    gaddr f = ARG(0), h = ARG(1);
    uint32_t s[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    c->r[19] = *W(HIT + 24);
    c->r[20] = ARG(2);
    c->r[16] = f; c->r[17] = h;
    c->r[18] = ARG(2);
    if (ARG(2) == 0) { RETURN(0); goto out; }
    int32_t base = S8(0x08A1D0C8u + (uint32_t)(S8(f + 338) << 5) + (uint32_t)(int32_t)S8(f + 375));
    if (S8(f + 386) == -1) base -= 4;
    if (S8(f + 309) != 0) base -= 24;
    c->r[4] = f; c->r[5] = (uint32_t)base;
    f_08873054(c);
    c->r[20] = c->r[2];
    int32_t combo = S8(f + 126);
    c->r[2] = (uint32_t)combo;
    int32_t row;
    if (!(combo < 31)) row = 31;
    else if (S8(f + 637) != 0) row = S8(0x08A21820u + (uint32_t)combo + 32);
    else row = S8(0x08A21820u + ((uint32_t)*B(h + 29) << 5) + (uint32_t)combo);
    c->r[20] += (uint32_t)row;
    if (!(0x30 < S16(f + 112))) {
        f_08821368(c);
        c->r[20] += (uint32_t)(int32_t)S8(0x08A1DB48u + (c->r[2] & 0x1F));
        if (!(0x10 < S16(f + 112))) {
            f_08821368(c);
            c->r[20] += (uint32_t)(int32_t)S8(0x08A1DB68u + (c->r[2] & 0x1F));
        }
    }
    gaddr opp = c->r[19];
    if ((*W(opp + 792) & 0x200) && *B(opp + 5) != 1 && *B(opp + 6) != 8 && S8(opp + 777) != 0)
        c->r[18] <<= 2;
    if ((int32_t)c->r[18] < 0) c->r[18] = 0;
    if (!((int32_t)c->r[18] < 128)) c->r[18] = 0x7F;
    int32_t total = (int32_t)c->r[20];
    uint32_t st = c->r[18];
    if (total >= 0) {
        if (!(total < 33)) total = 32;
        c->r[20] = (uint32_t)(int32_t)S8(0x08A1DC5Cu + st + (((uint32_t)total & 0xFF) << 7)) & 0xFF;
    } else {
        total = -total;
        if (!(total < 33)) total = 32;
        int32_t e = S8(0x08A1ECDCu + st + (((uint32_t)total & 0x7F) << 7));
        c->r[20] = (st + (uint32_t)e) & 0xFF;
    }
    c->r[4] = h; c->r[5] = c->r[20];
    f_08873094(c);
    c->r[4] = f; c->r[5] = c->r[2];
    f_08873160(c);
    uint32_t d = c->r[2];
    if (!((int32_t)d < 128)) d = 0x7F;
    if (d == 0) d = 1;
    c->r[20] = d;
    RETURN(d);
out:
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4];
}

/* (fighter, a1, kind, buttons): whether an input counts: never for kinds 4 and 7 - 14; in Training, a
   human of a side the setting names (0x0950FF65 = 5 with the side in GAME + 53, or otherwise without
   it) by 0x0950FF71's three bits for the side against the buttons not held (a guard: +696 by them, or
   with +73 only without button 4); otherwise none held is 1; a fighter holding +792 0x00800020 sets
   +696 from button 1 (without +73) or is held off by button 4 (with it); then kind 5 / 6 / others by
   buttons 1 / 2 / 1 << (kind - 1): held 0, else one more. */
SFA_FUNC(088758E8, input_counts) {
    gaddr f = ARG(0);
    uint32_t kind = ARG(1) & 0xFF, k = ARG(2) & 0xFF;
    c->r[6] = kind; c->r[5] = k;
    uint32_t r = 0;
    if (kind == 4 || (kind >= 7 && kind <= 14)) { RETURN(0); return; }
    if (S8(GAME + 114) == 2) {
        int side = S8(f + 337);
        int marked = (S8(GAME + 53) & (1 << (side & 31))) != 0;
        int forced = *B(0x0950FF65u) == 5 ? marked : !marked;
        if (forced && S8(f + 373) == 0) {
            uint32_t set = ((uint32_t)*B(0x0950FF71u) >> ((uint32_t)(side << 2) & 31)) & 7;
            if (((~k) & 7 & set) == 0) { RETURN(0); return; }
            if (S8(f + 73) != 0) { RETURN((k & 4) ? 0 : 1); return; }
            if (set != 7) *B(f + 696) = (set & 1) ? 0 : 1;
            else if (S8(f + 696) != 1) *B(f + 696) = (k & 1) ? 1 : 0;
            else *B(f + 696) = (k & 2) ? 0 : 1;
            RETURN(1);
            return;
        }
    }
    if (k == 0) { RETURN(1); return; }
    if (*W(f + 792) & 0x00800020u) {
        if (S8(f + 73) == 0) { r = 1; *B(f + 696) = (k & 1) ? 1 : 0; }
        else if ((k & 4) == 0) r = 1;
    }
    uint32_t bit = kind == 5 ? 1 : kind == 6 ? 2 : 1u << ((kind - 1) & 31);
    RETURN((k & bit) ? r : r + 1);
}

/* ---------------------------------------------------------------- data */

/* (fighter, a1, offsets): the character's lists (its routine table +416) unpacked, entry by entry, from
   its data (+196, table +192) by the offsets' 21 entries (two sets of these places: lists at +0xC8,
   +0x308, +0x548, +0x788 (kept at +0 - +12), +0x9D4 (kept at +0x9C8 x 3, its address at +20), +0x9D8 -
   +0x9EC, +0x9F0 (+28), +0x9FC (+36), +0x10 - +0x2C, +0x80 - +0x88). */
static const uint16_t PLACES[21] = { 0xC8, 0x308, 0x548, 0x788, 0x9D4, 0x9D8, 0x9DC, 0x9E0, 0x9E4, 0x9E8,
    0x9EC, 0x9F0, 0x9FC, 0x10, 0x18, 0x20, 0x28, 0x2C, 0x80, 0x84, 0x88 };
static void unpack_lists(Cpu *c, gaddr t, const uint16_t *places, const int8_t *n, int count, int s8_at) {
    for (int i = 0; i < count; i++) {
        gaddr place = t + places[i];
        switch (places[i]) {
        case 0xC8: c->r[17] = place; break;
        case 0x308: c->r[18] = place; break;
        case 0x548: c->r[19] = place; break;
        case 0x788: c->r[20] = place; break;
        case 0x9D4: c->r[21] = place; break;
        case 0x9F0: c->r[22] = place; break;
        case 0x9FC: c->r[23] = place; break;
        }
        if (s8_at >= 0 && i == s8_at) c->r[30] = place;
        c->r[4] = place; c->r[5] = *W(t + 196); c->r[6] = *W(t + 192); c->r[7] = (uint32_t)(int32_t)n[i];
        f_0886E638(c);
    }
    for (int k = 0; k < 3; k++) *W(t + 2504 + 4 * k) = c->r[21];
    *W(t + 20) = t + 0x9C8;
    *W(t) = c->r[17]; *W(t + 4) = c->r[18]; *W(t + 8) = c->r[19]; *W(t + 12) = c->r[20];
    *W(t + 28) = c->r[22]; *W(t + 36) = c->r[23];
}
#define SAVE_S(s)    do { for (int i_ = 0; i_ < 8; i_++) (s)[i_] = c->r[16 + i_]; (s)[8] = c->r[30]; } while (0)
#define RESTORE_S(s) do { for (int i_ = 0; i_ < 8; i_++) c->r[16 + i_] = (s)[i_]; c->r[30] = (s)[8]; } while (0)

SFA_FUNC(0886EB48, fighter_unpack_lists) {
    uint32_t s[9];
    SAVE_S(s);
    c->r[29] -= 0x40;
    gaddr t = *W(ARG(0) + 416), offs = ARG(2);
    c->r[16] = t; c->r[30] = offs;
    int8_t n[21];
    for (int i = 0; i < 21; i++) n[i] = S8(offs + (uint32_t)i);
    unpack_lists(c, t, PLACES, n, 21, -1);
    c->r[29] += 0x40;
    RESTORE_S(s);
}

/* (fighter): the same with fixed entries; +0x84 twice and +0x88 not (as the original). */
SFA_FUNC(08988FAC, fighter_unpack_lists_fixed) {
    static const uint16_t P[21] = { 0xC8, 0x308, 0x548, 0x788, 0x9D4, 0x9D8, 0x9DC, 0x9E0, 0x9E4, 0x9E8,
        0x9EC, 0x9F0, 0x9FC, 0x10, 0x18, 0x20, 0x28, 0x2C, 0x80, 0x84, 0x84 };
    static const int8_t N[21] = { 1, 2, 3, 4, 6, 7, 8, 9, 0xA, 0xE, 0xF, 0x11, 0x18, 5, 0x10, 0x14, 0x15,
        0x16, 0x17, 0x17, 0x17 };
    uint32_t s[9];
    SAVE_S(s);
    c->r[29] -= 0x40;
    gaddr t = *W(ARG(0) + 416);
    c->r[16] = t;
    unpack_lists(c, t, P, N, 21, 19);
    c->r[29] += 0x40;
    RESTORE_S(s);
}

/* ---------------------------------------------------------------- the name panel */

/* (fighter, show): with a1 set, its win count (+462: pictures 0x08A26F18 by digit, at 0x08A26F40 by side,
   +4 with BATTLE + 79 or in mode 0x12), the label for the count (0x08A26F04, from 5 the fifth) and its
   name (+456's four letters through 0x08A26DC0 into +464); then, with BATTLE + 79 or in mode 0x12, the
   side's tag (0x089FF184 / F188). */
SFA_FUNC(08892020, name_panel_texts) {
    gaddr f = ARG(0);
    uint32_t s[9];
    SAVE_S(s);
    c->r[29] -= 0x50;
    c->r[16] = f; c->r[17] = BATTLE;
    int32_t b79 = S8(BATTLE + 79);
    int tagged = b79 != 0 || S8(GAME + 114) == 0x12;
#define TEXT(str, font, x, y, col) do { c->r[4] = (str); c->r[5] = (font); c->r[6] = (x); c->r[7] = (y); \
        c->r[8] = (col); f_0880A6CC(c); } while (0)
    if (ARG(1) != 0) {
        const gaddr digits = 0x08A26F18u, places = 0x08A26F40u;
        c->r[21] = digits;
        c->r[23] = 0x08A26F04u;
        int32_t i = (tagged ? 4 : 0) + (S8(f + 337) & 1) * 2;
        i = (int8_t)i;
        uint32_t x = (uint32_t)(int32_t)S16(places + (uint32_t)i * 2);
        uint32_t y = (uint32_t)(int32_t)S16(places + (uint32_t)(int32_t)(int8_t)(i + 1) * 2);
        c->r[18] = x; c->r[19] = y;
        int32_t n = S8(f + 462);
        c->r[20] = (uint32_t)n;
        c->r[22] = x + 0x10;
        uint32_t lab = n < 5 ? (uint32_t)n : 4;
        c->r[23] = 0x08A26F04u + lab * 4;
        if (n < 10) {
            TEXT(*W(digits + (uint32_t)n * 4), 2, c->r[18], c->r[19], 4);
            c->r[20] = *W(c->r[23]);
            c->r[18] += 0x20;
        } else {
            c->r[30] = 10;
            DIV(c, (uint32_t)n, 10);
            TEXT(*W(digits + c->lo * 4), 2, c->r[18], c->r[19], 4);
            DIV(c, (uint32_t)n, 10);
            c->r[18] = (uint32_t)(int32_t)(int16_t)c->r[22];
            TEXT(*W(digits + c->hi * 4), 2, c->r[18], c->r[19], 4);
            c->r[22] = c->r[18] + 0x10;
            c->r[20] = *W(c->r[23]);
            c->r[18] += 0x20;
        }
        TEXT(c->r[20], 0, c->r[22], c->r[19] + 8, 4);
        for (uint32_t k = 0; k < 4; k++) *B(f + 464 + k) = *B(0x08A26DC0u + (uint32_t)(int32_t)S8(f + 456 + k));
        c->r[22] = 4;
        *B(f + 468) = 0;
        TEXT(f + 0x1D0, 2, c->r[18], c->r[19], 4);
    }
    if (tagged) {
        if (S8(f + 337) & 1) TEXT(0x089FF184u, 3, 0x10, 0x8C, 1);
        else TEXT(0x089FF188u, 3, 0x10, 0x74, 6);
    }
#undef TEXT
    c->r[29] += 0x50;
    RESTORE_S(s);
}
