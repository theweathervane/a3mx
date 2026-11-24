/* Larger steps (0.122.0): a fighter's round start (its life back by mode, a guard step), the hit's push
   and damage counted into a fighter, a part copying its parent, a spark from a hit record, the select
   screen's cursor reset and the entries put into its grid, the entry names under the grid, a fighter
   near another's box, and a screen's objects made. Offsets stay numbers until something shows what they
   are. Arguments of callees, and what the original leaves in a0 - t0 at a call, are set as the original
   sets them; s0 - s8 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define P1      0x092E3EA0u
#define P2      0x092E4338u
#define P3      0x092E47D0u
#define P4      0x092E4C68u
#define HIT     0x09358410u     /* the hit's working record (+24 the other fighter, +44 / +46 flags) */

#include "../guest_mem.h"

void f_0885ECE8(Cpu *c);   /* fighter_guard_choice */
void f_0886B4D4(Cpu *c);   /* fighter_battle_word */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08871CA4(Cpu *c);   /* push_with_spread */
void f_08871D60(Cpu *c);   /* damage_scale */
void f_08871E40(Cpu *c);   /* fighter_push_by_647 */
void f_08871ED0(Cpu *c);   /* guard_damage_cut */
void f_08873448(Cpu *c);   /* hit_count_for_14 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08822F1C(Cpu *c);   /* object_make_16 */
void f_08822F54(Cpu *c);   /* object_make_42 */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_0880A6CC(Cpu *c);   /* game_text */
void f_088212F4(Cpu *c);   /* to_bcd */
void f_088627D0(Cpu *c);   /* fighter_in_box_range */
void f_0889472C(Cpu *c);   /* game_select_barred */
void f_088A4F1C(Cpu *c);   /* game_select_styles_offered */
void f_088D5ECC(Cpu *c);   /* net_waits_and_36 */
void f_088D7C4C(Cpu *c);   /* net_battle_and_35 */
void f_088939AC(Cpu *c);   /* mode_is_not_team */

/* ---------------------------------------------------------------- fighters */

/* +366 down by 0x48, not under 0; whether it stayed at 0 or above */
static int take_48(gaddr f) {
    int16_t v = (int16_t)(S16(f + 366) - 0x48);
    *H(f + 366) = (uint16_t)v;
    if (v < 0) { *H(f + 366) = 0; return 0; }
    return 1;
}

/* (fighter): the round's next part (+7 on), several flags cleared (+270, +560 - 562, +573; +656 unless
   +572); if +252 was set it is cleared with +251 / +502 and +366 comes down by 0x48 (the fighter's own
   without BATTLE + 56, else both fighters of its side: P1 / P2, or P3 / P4 with +337 bit 1); BATTLE + 53
   unset becomes -1 and the fighter's battle word's +356 = 8; +82 = 8, then the guard choice. */
SFA_FUNC(0885EB74, fighter_round_part_reset) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 270) = 0;
    *B(f + 7) = (uint8_t)(*B(f + 7) + 1);
    *B(f + 573) = 0;
    *B(f + 560) = 0;
    *B(f + 561) = 0;
    int8_t v572 = S8(f + 572);
    c->r[17] = 8;
    *B(f + 562) = 0;
    if (v572 == 0) *B(f + 656) = 0;
    if (S16(f + 252) != 0) {
        *H(f + 252) = 0;
        *B(f + 251) = 0;
        *B(f + 502) = 0;
        if (S16(BATTLE + 56) == 0) {
            take_48(f);
        } else if (S8(f + 337) & 2) {
            take_48(P3); take_48(P4);
        } else {
            take_48(P1); take_48(P2);
        }
    }
    c->r[4] = BATTLE;
    c->r[5] = (uint32_t)(int32_t)S8(BATTLE + 53);
    if (c->r[5] == 0) {
        c->r[5] = 0xFFFFFFFFu;
        *B(BATTLE + 53) = 0xFF;
        c->r[4] = f;
        f_0886B4D4(c);
        *B(c->r[2] + 356) = 8;
    } else {
        c->r[5] = 0xFFFFFFFFu;
    }
    *H(f + 82) = 8;
    c->r[4] = f;
    f_0885ECE8(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): its life (+112 / +114) at the start by mode. Survival (7) in its kinds 7, 8 and 10 (BATTLE +
   79): +420 (a computer's), and +422 into +366 for a human; kinds 4 and 5: +420 / +422 for the side
   without +337 bit 1; others +420 only. Mode 6 and Free Battle's TEAM: +420, with +422 unless +441 (and
   a human only with +413; a computer of side bit 1 puts +413 in +1). Otherwise, with GAME + 137: in the
   0x09517E00 kind 2, a human's +420 (0x90 when BATTLE + 2 is 0) and +422; a computer of the sides in
   BATTLE + 156 takes 0x09517E08's byte, and with +413 and side bit 1 puts +413 in +1. */
SFA_FUNC(08865BEC, fighter_life_by_mode) {
    gaddr f = ARG(0);
    int8_t m = S8(GAME + 114);
    if (m == 7) {
        int8_t kind = S8(BATTLE + 79);
        if (kind == 7 || kind == 10 || kind == 8) {
            int16_t life = S16(f + 420);
            if (S8(f + 373) == 0) {
                int16_t v = S16(f + 422);
                *H(f + 114) = (uint16_t)life;
                *H(f + 366) = (uint16_t)v;
                *H(f + 112) = (uint16_t)life;
                return;
            }
            int8_t side = S8(f + 337);
            *H(f + 114) = (uint16_t)life;
            *H(f + 112) = (uint16_t)life;
            if (side & 2) *B(f + 1) = (uint8_t)S8(f + 413);
            return;
        }
        if (kind == 4 || kind == 5) {
            if (S8(f + 337) & 2) return;
            int16_t v = S16(f + 422), life = S16(f + 420);
            *H(f + 366) = (uint16_t)v;
            *H(f + 114) = (uint16_t)life;
            *H(f + 112) = (uint16_t)life;
            return;
        }
        int16_t life = S16(f + 420);
        *H(f + 114) = (uint16_t)life;
        *H(f + 112) = (uint16_t)life;
        return;
    }
    if (m == 6 || (m == 0x10 && S8(BATTLE + 159) == 2)) {
        if (S8(f + 373) == 0) {
            if (S8(f + 413) == 0) return;
            int16_t life = S16(f + 420);
            int8_t keep = S8(f + 441);
            *H(f + 114) = (uint16_t)life;
            *H(f + 112) = (uint16_t)life;
            if (keep != 0) return;
            *H(f + 366) = (uint16_t)S16(f + 422);
            return;
        }
        int16_t life = S16(f + 420);
        int8_t side = S8(f + 337);
        *H(f + 114) = (uint16_t)life;
        *H(f + 112) = (uint16_t)life;
        if (side & 2) *B(f + 1) = (uint8_t)S8(f + 413);
        if (S8(f + 441) != 0) return;
        *H(f + 366) = (uint16_t)S16(f + 422);
        return;
    }
    if (S8(GAME + 137) == 0) return;
    gaddr t = 0x09517A78u;
    int8_t cpu = S8(f + 373);
    if (S8(t + 904) == 2 && cpu == 0) {
        if (S16(BATTLE + 2) == 0) *H(f + 420) = 0x90;
        int16_t life = S16(f + 420), v = S16(f + 422);
        *H(f + 114) = (uint16_t)life;
        *H(f + 112) = (uint16_t)life;
        *H(f + 366) = (uint16_t)v;
    }
    if (cpu == 0) return;
    int8_t side = S8(f + 337);
    if ((S8(BATTLE + 156) & (1 << (side & 31))) == 0) return;
    uint32_t life = *B(t + 912);
    int8_t e = S8(f + 413);
    *H(f + 114) = (uint16_t)life;
    *H(f + 112) = (uint16_t)life;
    if (e == 0) return;
    if ((side & 2) == 0) return;
    *B(f + 1) = (uint8_t)e;
}

/* (fighter, hit): the hit's push and damage into the fighter (not in Training with 0x0950FF69 clear, nor
   with BATTLE + 56, +313, +792 flags 0x00820010, or the hit's +26 = 0x80): the strength (+26, or +8,
   less its high bit; halved if the other fighter's +8) through the push table 0x08A1ECDC (rows 0x700
   with +19; 0xB00 / 0xC00 by the other's +696 without it, or halved again with its +73), spread, scaled,
   pushed and cut by a guard; at least 1, added to +557 / +559 and kept in +650; past +556, +590 marks
   the fighter's +122 bit and +573 = 1. */
SFA_FUNC(08871AC4, fighter_take_hit_amount) {
    gaddr f = ARG(0), h = ARG(1);
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = f;
    gaddr other = *W(HIT + 24);
    c->r[18] = other;
    c->r[17] = h;
    if (S8(GAME + 114) == 2 && *B(0x0950FF60u + 9) == 0) goto out;
    if (S16(BATTLE + 56) != 0) goto out;
    if (S8(f + 313) != 0) goto out;
    if (*W(f + 792) & 0x00820010u) goto out;
    uint32_t strength = *B(h + 26);
    if (strength == 0x80) goto out;
    uint32_t r6 = *B(h + 19);
    strength = strength != 0 ? strength & 0x7F : *B(h + 8) & 0x7F;
    uint32_t r7 = strength;
    if (*B(other + 8) != 0) r7 = strength >> 1;
    const gaddr tab = 0x08A1ECDCu;
    if (r6 == 0) {
        if (S8(other + 73) != 0) r7 >>= 1;
        else {
            r7 &= 0x7F;
            r7 = (uint32_t)(int32_t)S8(tab + r7 + (S8(other + 696) != 0 ? 0xB00u : 0xC00u));
        }
    } else {
        if (r6 == 2) r7 = (r7 >> 1) + (r7 >> 2);
        r7 = (uint32_t)(int32_t)S8(tab + r7 + 0x700) & 0x7F;
    }
    c->r[4] = r7;
    f_08871CA4(c);
    c->r[4] = other; c->r[5] = f; c->r[6] = h; c->r[7] = c->r[2];
    f_08871D60(c);
    c->r[4] = f; c->r[5] = c->r[2];
    f_08871E40(c);
    c->r[4] = f; c->r[5] = c->r[2];
    f_08871ED0(c);
    uint32_t n = c->r[2];
    if (n == 0) n = 1;
    *B(f + 557) = (uint8_t)(S8(f + 557) + (int32_t)n);
    *B(f + 650) = (uint8_t)n;
    *B(f + 559) = (uint8_t)(S8(f + 559) + (int32_t)n);
    if (S8(f + 556) < S8(f + 557)) {
        *B(f + 590) = (uint8_t)(S8(f + 590) | (1 << (S8(f + 122) & 31)));
        *B(f + 573) = 1;
    }
out:
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (fighter, hit): a spark for the hit (kind 4 of the 32-pool): type and count from the hit record (+20
   / +15; with +117 bit 7 and +18, the pair at 0x08A2210A + 2 * +20; 13 / 0 without +18), or 12 / 18
   while +590 is set; the count through 0x08873448 for the other fighter. Placed between the fighter's
   two boxes (+714 - 720, from +728 / +736 / +744 by the smallest of +722 / +724 / +726), facing as the
   other fighter's side, +139 from the hit's flags or a fighter with no life. */
SFA_FUNC(08875DBC, fighter_hit_spark) {
    gaddr f = ARG(0), rec = ARG(1);
    uint32_t s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    c->r[16] = f;
    c->r[17] = HIT;
    uint32_t a = *H(f + 722), b = *H(f + 724), cc = *H(f + 726);
    gaddr from;
    if ((int32_t)a < (int32_t)b && (int32_t)a < (int32_t)cc) from = 728;
    else if ((int32_t)a < (int32_t)b) from = 744;
    else if ((int32_t)b < (int32_t)cc) from = 736;
    else from = 744;
    for (int i = 0; i < 4; i++) *H(f + 714 + 2 * i) = (uint16_t)S16(f + from + 2 * i);
    gaddr other = *W(HIT + 24);
    uint32_t type, count;
    if (S8(f + 590) != 0) { type = 0xC; count = 0x12; }
    else if ((*B(f + 117) & 0x80) == 0) { type = *B(rec + 20); count = *B(rec + 15); }
    else if (*B(rec + 18) == 0) { type = 0xD; count = 0; }
    else {
        gaddr e = 0x08A2210Au + (uint32_t)*B(rec + 20) * 2;
        type = *B(e); count = *B(e + 1);
    }
    c->r[19] = type; c->r[18] = count;
    c->r[4] = other; c->r[5] = count;
    f_08873448(c);
    c->r[18] = c->r[2];
    c->r[4] = 4; c->r[5] = type;
    f_08822EAC(c);
    gaddr o = c->r[2];
    c->r[19] = o;
    if (o != 0) {
        *B(o + 119) = (uint8_t)c->r[18];
        *W(o + 88) = f;
        uint8_t side = (uint8_t)S8(*W(HIT + 24) + 121);
        *B(o + 135) = 0;
        *B(o + 121) = side;
        *H(o + 82) = 0;
        *B(o + 139) = 0;
        *H(o + 24) = *H(*W(HIT + 24) + 24);
        if (*B(HIT + 44) != 0) { *H(o + 82) = 2; *B(o + 139) = 2; }
        if (S16(f + 112) < 0) *B(o + 139) = 1;
        else if (*B(HIT + 46) != 0) *B(o + 139) = 1;
        int32_t x1 = S16(f + 714), x2 = S16(f + 716), y1 = S16(f + 718), y2 = S16(f + 720);
        *H(o + 18) = (uint16_t)(((x1 - x2) >> 1) + x2);
        *H(o + 22) = (uint16_t)(((y1 - y2) >> 1) + y2);
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (object): 1 when one of the 24 records at 0x0950FFB0 / 0x09511090 (0x168 bytes, +0 = 1 in use, +136
   bit 7 off) of the other side (+121) stands within 80 of its x; 2 when a fighter of its +263 bits
   (0x08A17F88) has +251 or +502 set, or with +235 is in its box's range; else 0. */
SFA_FUNC(088625C0, near_other_side) {
    gaddr o = ARG(0);
    uint32_t s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    c->r[16] = o;
    int8_t side = S8(o + 121);
    int16_t x = S16(o + 18);
    c->r[18] = 1;
    uint32_t r = 0;
    static const gaddr LISTS[2] = { 0x0950FFB0u, 0x09511090u };
    for (int l = 0; l < 2; l++) {
        for (uint32_t k = 0; k < 12; k++) {
            gaddr e = LISTS[l] + k * 0x168;
            if (*B(e) != 1 || (S8(e + 136) & 0x80) || S8(e + 121) == side) continue;
            if ((uint32_t)(S16(e + 18) - x + 0x50) < 0xA0u) { r = 1; goto out; }
        }
    }
    c->r[19] = 0x08A17F88u;
    for (uint32_t k = 0; k < 4; k++, c->r[19] += 4) {
        c->r[17] = k;
        if ((S8(o + 263) & (1 << k)) == 0) continue;
        gaddr f = *W(c->r[19]);
        if (S8(f + 251) != 0 || S8(f + 502) != 0) { r = 2; goto out; }
        if (S8(f + 235) == 0) continue;
        c->r[4] = o; c->r[5] = f;
        f_088627D0(c);
        if (c->r[2] != 0) { r = 2; goto out; }
    }
out:
    RETURN(r);
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* ---------------------------------------------------------------- objects */

/* (part, parent): +4 on; drawer, picture list, +164, facing and +171 from the parent; by type (+3) its
   +12 / +13 (5: 1 / 1; 6, 7: 1 / 0x60; 8, 9: 1 / 2; 3: -1 / 1; others the parent's); +14, +15, +24, +26,
   +38 the parent's, +75 = 0x40, animation 0x08A302F8 by type, place 0x08A30330 by type (types 5 - 9
   the parent's place). */
SFA_FUNC(088B1ED0, part_from_parent) {
    gaddr o = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[17] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = *W(p + 148);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *B(o + 9) = 0;
    *B(o + 11) = *B(p + 11);
    *B(o + 171) = (uint8_t)S8(p + 171);
    uint32_t t = *B(o + 3);
    c->r[16] = p;
    switch (t) {
    case 5: *B(o + 12) = 1; *B(o + 13) = 1; *H(o + 24) = *H(p + 24); break;
    case 6: case 7: *B(o + 12) = 1; *B(o + 13) = 0x60; *H(o + 24) = *H(p + 24); break;
    case 8: case 9: *B(o + 12) = 1; *B(o + 13) = 2; *H(o + 24) = *H(p + 24); break;
    case 3: *B(o + 12) = 0xFF; *B(o + 13) = 1; *H(o + 24) = 7; c->r[7] = 1; break;
    default: *B(o + 12) = (uint8_t)S8(p + 12); *H(o + 24) = 7; *B(o + 13) = *B(p + 13); break;
    }
    *B(o + 14) = *B(p + 14);
    *B(o + 15) = *B(p + 15);
    *H(o + 24) = *H(p + 24);
    *H(o + 26) = *H(p + 26);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *B(o + 75) = 0x40;
    c->r[4] = o; c->r[5] = *W(0x08A302F8u + t * 4); c->r[6] = 0x40;
    f_0882196C(c);
    t = *B(o + 3);
    gaddr e = 0x08A30330u + t * 4;
    *H(o + 18) = (uint16_t)S16(e);
    *H(o + 22) = (uint16_t)S16(e + 2);
    if (t >= 5 && t < 10) {
        *H(o + 18) = (uint16_t)S16(p + 18);
        *H(o + 22) = (uint16_t)S16(p + 22);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (): the six entries' names (0x08A3C994's six strings at 0x08A3C97C's places, less 0x78) and, for an
   entry in use (0x09519588 + 0x354 n), its number (+4 + 1) in two digits (no leading 0: 0x08A3C7D0's
   blank, then 0x08A3C7C6's digits) into its 0x12-byte line at 0x08A3C751, 0x18 right of the place;
   otherwise 0x08A000C0 0x58 left of it, a row lower. Every text in colour 6 (+3). */
SFA_FUNC(088CE33C, entry_names) {
    uint32_t s[10];
    for (int i = 0; i < 8; i++) s[i] = c->r[16 + i];
    s[8] = c->r[30];
    c->r[29] -= 0x60;
    gaddr places = 0x08A3C97Cu, names = 0x08A3C994u, none = 0x08A000C0u;
    gaddr digits = 0x08A3C7C6u, blanks = 0x08A3C7D0u;
    c->r[21] = 0x08A3C751u; c->r[19] = blanks; c->r[18] = digits; c->r[22] = 0x09519588u;
    c->r[23] = 0; c->r[30] = 6;
    for (uint32_t i = 0; i < 6; i++, names += 4) {
        gaddr pl = places + i * 4;
        c->r[20] = pl;
        c->r[4] = *W(names); c->r[5] = 1;
        c->r[6] = (uint32_t)(int32_t)(int16_t)(*H(pl) - 0x78);
        c->r[7] = (uint32_t)(int32_t)(int16_t)*H(pl + 2);
        c->r[8] = 0xB;
        f_0880A6CC(c);
        *B(c->r[2] + 3) = 6;
        gaddr entry = c->r[22];
        if (S8(entry) == 0) {
            c->r[4] = none; c->r[5] = 2;
            c->r[6] = (uint32_t)(int32_t)(int16_t)(*H(pl) - 0x58);
            c->r[7] = (uint32_t)(int32_t)(int16_t)(*H(pl + 2) + 1);
            c->r[8] = 0xB;
            f_0880A6CC(c);
            *B(c->r[2] + 3) = 6;
        } else {
            c->r[17] = 1; c->r[16] = 4;
            c->r[4] = (uint32_t)*B(entry + 4) + 1;
            f_088212F4(c);
            uint32_t bcd = c->r[2];
            gaddr out = c->r[21];
            int lead = 1;
            for (uint32_t shift = 4; shift != 0; shift = (shift - 4) & 0xFFFF) {
                uint32_t d = (bcd >> shift) & 0xF;
                if (d != 0) lead = 0;
                *B(out) = (uint8_t)S8((lead ? blanks : digits) + d);
                out++;
            }
            c->r[16] = 0; c->r[17] = (uint32_t)lead;
            *B(out) = (uint8_t)S8(digits + (bcd & 0xF));
            c->r[4] = c->r[21]; c->r[5] = 1;
            c->r[6] = (uint32_t)(int32_t)(int16_t)(*H(pl) + 0x18);
            c->r[7] = (uint32_t)(int32_t)(int16_t)*H(pl + 2);
            c->r[8] = 0xB;
            f_0880A6CC(c);
            *B(c->r[2] + 3) = 6;
        }
        c->r[23] = i + 1;
        c->r[22] += 0x354;
        c->r[21] += 0x12;
    }
    c->r[29] += 0x60;
    for (int i = 0; i < 8; i++) c->r[16 + i] = s[i];
    c->r[30] = s[8];
}

/* (): a screen's objects: eight kind-0x85 ones (types 0 - 7, with eleven free in pool 4) and type 0xB
   with owner 0x08B02E70 (+88); third-list objects 6 / 0x10 - 0x14, 0x19 (stopping at the first that
   can't be made), 0x12 / 1 - 6 owned by it (+124); six rows (k): 0x85 / 0xA with +78 = k, 0x85 / 0xC
   following it, its 0x26 / 1 shadow at its place (+40), a 0x12 / 7 at its place (+94 = k) and a 0x7D / 4
   (+78 = k); then 0x7D / 3, two 0x12 / 0xE (+94), 0x12 / 0xF, two 0x12 / 0x10 (+96) and 0x85 / 0x1A,
   0x1B. */
SFA_FUNC(088CD7E4, screen_objects_2e70) {
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    gaddr owner = 0x08B02E70u;
    c->r[16] = owner; c->r[18] = 0x09510000u;
    for (uint32_t k = 0; k < 8; k++) {
        c->r[17] = k;
        c->r[5] = k & 0xFF;
        if (S8(0x09510000u + 30451) < 11) continue;
        c->r[4] = 0x85;
        f_08822EAC(c);
        *W(c->r[2] + 88) = owner;
    }
    c->r[17] = 8;
    c->r[4] = 0x85; c->r[5] = 0xB;
    f_08822EAC(c);
    *W(c->r[2] + 88) = owner;
    static const uint8_t FIRST[6] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x19 };
    for (int i = 0; i < 6; i++) {
        c->r[4] = 6; c->r[5] = FIRST[i];
        f_08823338(c);
        if (c->r[2] == 0) goto out;
    }
    for (uint32_t k = 1; k < 7; k++) {
        c->r[17] = k;
        c->r[4] = 0x12; c->r[5] = k & 0xFF;
        f_08823338(c);
        if (c->r[2] != 0) *W(c->r[2] + 124) = owner;
    }
    for (uint32_t k = 0; k < 6; k++) {
        c->r[17] = k;
        c->r[4] = 0x85; c->r[5] = 0xA;
        f_08822EAC(c);
        gaddr row = c->r[2];
        c->r[18] = row;
        *B(row + 78) = (uint8_t)k;
        c->r[4] = 0x85; c->r[5] = 0xC;
        f_08822EAC(c);
        *W(c->r[2] + 88) = row;
        *B(c->r[2] + 78) = (uint8_t)k;
        c->r[4] = 0x26; c->r[5] = 1;
        f_08822F54(c);
        gaddr sh = c->r[2];
        if (sh != 0) {
            *W(sh + 88) = row;
            *W(row + 40) = sh;
            *H(sh + 18) = (uint16_t)S16(row + 18);
            *H(sh + 22) = (uint16_t)S16(row + 22);
        }
        c->r[4] = 0x12; c->r[5] = 7;
        f_08823338(c);
        gaddr e = c->r[2];
        *W(e + 124) = row;
        *H(e + 6) = (uint16_t)S16(row + 18);
        *H(e + 94) = (uint16_t)k;
        *H(e + 10) = (uint16_t)S16(row + 22);
        c->r[4] = 0x7D; c->r[5] = 4;
        f_08822F1C(c);
        *B(c->r[2] + 78) = (uint8_t)k;
    }
    c->r[17] = 6;
    c->r[4] = 0x7D; c->r[5] = 3;
    f_08822F1C(c);
    *W(c->r[2] + 88) = owner;
    for (uint32_t k = 0; k < 2; k++) {
        c->r[17] = k;
        c->r[4] = 0x12; c->r[5] = 0xE;
        f_08823338(c);
        *H(c->r[2] + 94) = (uint16_t)k;
        *W(c->r[2] + 124) = owner;
    }
    c->r[17] = 2;
    c->r[4] = 0x12; c->r[5] = 0xF;
    f_08823338(c);
    *W(c->r[2] + 124) = owner;
    for (uint32_t k = 0; k < 2; k++) {
        c->r[17] = k;
        c->r[4] = 0x12; c->r[5] = 0x10;
        f_08823338(c);
        *H(c->r[2] + 96) = (uint16_t)k;
        *W(c->r[2] + 124) = owner;
    }
    c->r[17] = 2;
    c->r[4] = 0x85; c->r[5] = 0x1A;
    f_08822EAC(c);
    c->r[4] = 0x85; c->r[5] = 0x1B;
    f_08822EAC(c);
out:
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* ---------------------------------------------------------------- the select screen */

/* (cursor): its state and flags cleared (+4, +81, +126; +424 keeps its low three bits, + bit 3 without
   GAME + 118); bit 2 (no style to choose) when only NORMAL is offered or without GAME + 156, copied to
   +78; character 0x13 becomes 0x11, and a barred one the first that isn't; outside a network wait, in
   mode 1 / 6 or a network session, the style flags (+373, +385, +428, +431) cleared. In a network
   session: GAME + 185 picks the style (bit 2; 1 MAZI +428, 2 SAIKYO +385, 3 CLASSICAL: +431, ISM -1, bit
   1) unless GAME + 156; GAME + 184 from 2 the ISM (it less 3, bit 1). Bit 2 also in a network battle
   with +35. */
SFA_FUNC(08896DE8, select_cursor_reset) {
    gaddr me = ARG(0);
    uint32_t s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    *B(me + 81) = 0;
    *B(me + 4) = 0;
    *H(me + 424) = (uint16_t)(S16(me + 424) & 7);
    c->r[18] = GAME;
    *B(me + 126) = 0;
    c->r[17] = 1;
    c->r[16] = me;
    if (S8(GAME + 118) == 0) *H(me + 424) = (uint16_t)(S16(me + 424) | 8);
    c->r[4] = me;
    f_088A4F1C(c);
    uint32_t offered = (c->r[2] & ~0x10u) & 0xFF;
    if (offered == 1) *H(me + 424) = (uint16_t)(S16(me + 424) | 4);
    int32_t ch = S8(me + 338);
    if (S8(GAME + 156) == 0) *H(me + 424) = (uint16_t)(S16(me + 424) | 4);
    *B(me + 78) = (uint8_t)S16(me + 424);
    if (ch == 0x13) { *B(me + 338) = 0x11; ch = S8(me + 338); }
    c->r[4] = (uint32_t)ch;
    f_0889472C(c);
    c->r[19] = 0;
    if (c->r[2] != 0) {
        for (uint32_t k = 0; k < 36; k++) {
            c->r[19] = k;
            c->r[4] = k;
            f_0889472C(c);
            if (c->r[2] == 0) { *B(me + 338) = (uint8_t)k; break; }
            c->r[19] = k + 1;
        }
    }
    f_088D5ECC(c);
    if (c->r[2] == 0) {
        int8_t m = S8(GAME + 114);
        if (m == 1 || m == 6 || S8(GAME + 146) != 0) {
            *B(me + 431) = 0;
            *B(me + 428) = 0;
            *B(me + 385) = 0;
            *B(me + 373) = 0;
        }
    }
    if (S8(GAME + 146) != 0) {
        if (S8(GAME + 156) == 0 && S8(GAME + 185) != 0) {
            *H(me + 424) = (uint16_t)(S16(me + 424) | 4);
            int8_t style = S8(GAME + 185);
            if (style == 1) *B(me + 428) = 1;
            else if (style == 2) *B(me + 385) = 1;
            else if (style == 3) {
                uint16_t v = (uint16_t)S16(me + 424);
                *B(me + 431) = 1;
                *B(me + 386) = 0xFF;
                *B(me + 450) = 0xFF;
                *H(me + 424) = (uint16_t)(v | 2);
            }
        }
        if (S8(GAME + 184) >= 2) {
            *H(me + 424) = (uint16_t)(S16(me + 424) | 2);
            uint8_t ism = (uint8_t)(S8(GAME + 184) - 3);
            *B(me + 386) = ism;
            *B(me + 450) = ism;
        }
    }
    f_088D7C4C(c);
    if (c->r[2] != 0) *H(me + 424) = (uint16_t)(S16(me + 424) | 4);
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (): the six entries put into the character grid (0x08A26F60) at their boxes (0x08A26FC2): an entry in
   use (0x09519588 + 0x354 n, not in World Tour, mode 3) by its character (+1), its box's picture
   (0x08A28418 + 6 n: +4 the character's face 0x08A27020, +286 = 0x27), otherwise none (-1, +4 = 0x4000,
   +286 its own 0x08A27074); then the 36 characters' boxes (0x08A26FCE, pictures by 0x08A2708C) shown or,
   for a barred one, emptied (+4 / +688 swapped, keeping bit 15). In Dramatic and Variable Battle the
   players' entries (0x09358528, two of 0x354) are cleared instead. */
SFA_FUNC(088939DC, select_entries_into_grid) {
    uint32_t s[7];
    for (int i = 0; i < 7; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    f_088939AC(c);
    if (c->r[2] == 0) {
        for (uint32_t i = 0; i < 2 * 0x354; i++) *B(0x09358528u + i) = 0;
        goto out;
    }
    const gaddr grid = 0x08A26F60u, pics = 0x08A28418u, faces = 0x08A27020u;
    c->r[18] = pics; c->r[17] = grid; c->r[22] = faces; c->r[21] = 0x08A2708Cu; c->r[19] = 0xFFFFFFFFu;
    int8_t mode = S8(GAME + 114);
    for (uint32_t k = 0; k < 6; k++) {
        gaddr entry = 0x09519588u + k * 0x354, pic = pics + k * 6;
        gaddr box = grid + (uint32_t)(int32_t)S8(0x08A26FC2u + k);
        if (S8(entry) == 0 || mode == 3) {
            *B(box) = 0xFF;
            *H(pic + 4) = 0x4000;
            *H(pic + 286) = *H(0x08A27074u + k * 2);
        } else {
            int8_t ch = S8(entry + 1);
            *B(box) = (uint8_t)ch;
            *H(pic + 286) = 0x27;
            *H(pic + 4) = *H(faces + (uint32_t)(int32_t)ch * 2);
        }
    }
    c->r[20] = 0x08A26FCEu;
    for (uint32_t i = 0; i < 36; i++) {
        c->r[16] = i;
        c->r[4] = i;
        f_0889472C(c);
        gaddr pic = pics + (uint32_t)(int32_t)S8(0x08A2708Cu + i) * 6;
        uint16_t face = *H(c->r[22]);
        if (c->r[2] != 0) {
            *B(grid + (uint32_t)(int32_t)S8(0x08A26FCEu + i)) = 0xFF;
            *H(pic + 4) = (uint16_t)((*H(pic + 4) & 0x8000) | 0x4000);
            *H(pic + 688) = (uint16_t)((*H(pic + 688) & 0x8000) | face);
        } else {
            *H(pic + 688) = (uint16_t)((*H(pic + 688) & 0x8000) | 0x4000);
            *H(pic + 4) = (uint16_t)((*H(pic + 4) & 0x8000) | face);
        }
        c->r[22] += 2;
    }
out:
    c->r[29] += 0x30;
    for (int i = 0; i < 7; i++) c->r[16 + i] = s[i];
}
