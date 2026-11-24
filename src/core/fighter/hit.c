/* Small adjustments made when a hit lands, and around it.
 *
 * Most take a value (damage, a count) and answer it changed. Fields keep their offsets until the hit
 * routine itself is decompiled. 0x09358410 is the hit's working record: +24 the other fighter, +44 a
 * flag.
 */
#include "../decomp.h"

#define BATTLE     0x092E1228u
#define HIT        0x09358410u
#define F_HEALTH   112
#define F_CHARACTER 338

#include "../guest_mem.h"

/* One of 48 flags of a fighter: bits 0 - 31 in the word +400, 32 on in the s16 +404. Answers the bit,
   not 0 or 1. */
SFA_FUNC(08830384, fighter_flag_400) {
    int32_t n = (int32_t)ARG(0);
    gaddr f = ARG(1);
    if (n < 32) RETURN(*W(f + 400) & (1u << (n & 31)));
    else RETURN((uint32_t)S16(f + 404) & (1u << ((n - 32) & 31)));
}

/* +586 grows by byte 27 of the hit's data; +587 is a quarter of it, or half while +588. */
SFA_FUNC(08872174, hit_add_586) {
    gaddr f = ARG(1);
    int8_t fast = S8(f + 588);
    *B(f + 586) = (uint8_t)(S8(f + 586) + *B(ARG(2) + 27));
    *B(f + 587) = (uint8_t)(S8(f + 586) >> (fast != 0 ? 1 : 2));
}

/* A fighter with +235 set and bit 7 of +307 clear: +588 and the hit record's flag are set; answers 1. */
SFA_FUNC(088723D4, hit_check_235) {
    gaddr f = ARG(0);
    if (S8(f + 235) == 0 || (*B(f + 307) & 0x80) != 0) { RETURN(0); return; }
    *B(f + 588) = 1;
    *B(HIT + 44) = 1;
    RETURN(1);
}

/* The value halved for a fighter with +135 set and +698 clear, or with +1145 set. */
SFA_FUNC(08872540, hit_halve_135) {
    gaddr f = ARG(0);
    int32_t v = (int32_t)ARG(1);
    if ((S8(f + 135) != 0 && S8(f + 698) == 0) || S8(f + 1145) != 0) v >>= 1;
    RETURN(v);
}

/* The value halved for an object of kind 0x22 with +8 set, or quartered when its owner has bit 30 of
   +792. */
SFA_FUNC(08872CD8, hit_halve_kind_22) {
    gaddr o = ARG(0);
    int32_t v = (int32_t)ARG(1);
    if (*B(o + 8) != 0 && *B(o + 2) == 0x22) v >>= (*W(*W(o + 88) + 792) & 0x40000000u) ? 2 : 1;
    RETURN(v);
}

/* The value less a sixteenth of how far the fighter's health is above the other's (nothing under 17
   above). */
SFA_FUNC(08873054, hit_less_for_the_lead) {
    int32_t v = (int32_t)ARG(1);
    int16_t lead = (int16_t)(S16(ARG(0) + F_HEALTH) - S16(*W(HIT + 24) + F_HEALTH));
    if (lead >= 17) v -= (lead >> 4) & 0x1F;
    RETURN(v);
}

/* A count as a byte; for character 0x14, 1 to 3 become 9 to 11. */
SFA_FUNC(08873448, hit_count_for_14) {
    uint32_t n = ARG(1) & 0xFF;
    if (S8(ARG(0) + F_CHARACTER) == 0x14 && n > 0 && n < 4) n = (n + 8) & 0xFF;
    RETURN(n);
}

/* When byte 117 of the second argument has bit 7: the first's +123 = -1 and, if its +8 is set, the other
   fighter's +537 = -1. */
SFA_FUNC(0887347C, hit_mark_123) {
    gaddr o = ARG(0);
    if ((*B(ARG(1) + 117) & 0x80) == 0) return;
    *B(o + 123) = 0xFF;
    if (*B(o + 8) != 0) *B(*W(HIT + 24) + 537) = 0xFF;
}

/* A fighter's +638 (unless its +8 is set): 12 with any of 0x300001 in +792, otherwise its character's
   value from 0x08A220D0. */
SFA_FUNC(08874C90, fighter_set_638) {
    gaddr f = ARG(0);
    if (*B(f + 8) != 0) return;
    *B(f + 638) = (*W(f + 792) & 0x300001u) != 0 ? 12 : *B(0x08A220D0u + (uint32_t)S8(f + F_CHARACTER));
}

/* Puts the fighter back at the start of routine 1 (steps +4 to +7 = 1, 0, 0, 0) unless it is there
   already (+7 aside); answers the second argument. */
SFA_FUNC(08877F44, fighter_to_routine_1) {
    gaddr f = ARG(0);
    if (!(*B(f + 4) == 1 && *B(f + 5) == 0 && *B(f + 6) == 0)) {
        *B(f + 4) = 1; *B(f + 7) = 0; *B(f + 6) = 0; *B(f + 5) = 0;
    }
    RETURN(ARG(1));
}

/* A move number (+656) outside 0 to 3 is put back to 0. */
SFA_FUNC(08864620, fighter_keep_move_0_3) {
    int8_t m = S8(ARG(0) + 656);
    if (m < 0 || m > 3) *B(ARG(0) + 656) = 0;
}

/* +13 (the fighter's fade slot) from the table 0x08A19622 by layout and side. */
SFA_FUNC(08864D40, fighter_set_slot) {
    gaddr f = ARG(0);
    *B(f + 13) = *B(0x08A19622u + (uint32_t)S16(BATTLE + 56) * 4u + (uint32_t)S8(f + 337));
}

/* Clears +644, +1146 and +80 (+80 set again while +251), and the opponent's +624, or +625 when the
   fighter's +122 is set. */
SFA_FUNC(0886DC3C, fighter_clear_644) {
    gaddr f = ARG(0), opponent = *W(f + 92);
    *B(f + 644) = 0;
    *B(f + 1146) = 0;
    *B(f + 80) = S8(f + 251) != 0;
    *B(opponent + (S8(f + 122) != 0 ? 625 : 624)) = 0;
}

/* The other fighter (not an object with +8) has +73 set, and this one has +588. */
SFA_FUNC(08871A44, hit_other_73_and_588) {
    gaddr other = *W(HIT + 24);
    RETURN(*B(other + 8) == 0 && S8(other + 73) != 0 && S8(ARG(0) + 588) != 0);
}

/* When the fighter at the record's +16 (not an object with +8) has +73 set, a +119 under 3 is cleared. */
SFA_FUNC(08871A84, hit_clear_119) {
    gaddr who = *W(HIT + 16), o = ARG(0);
    if (*B(who + 8) == 0 && S8(who + 73) != 0 && *B(o + 119) < 3) *B(o + 119) = 0;
}

/* ---- 0.68.0 ---- */

void f_088723D4(Cpu *c);   /* hit_check_235 */
#define CAMERA_LAYER 0x092E88A0u

/* Which way the one hit falls (+134 of the second argument): the attacker's facing; or, by byte 11 of
   the hit's data: with bit 7, away from the attacker; at 2, by which half of the screen it is in;
   otherwise the facing turned by that byte. */
SFA_FUNC(088721B8, hit_set_fall_side) {
    gaddr a = ARG(0), b = ARG(1);
    uint8_t how = *B(ARG(2) + 11);
    *B(b + 134) = *B(a + 11);
    if (how & 0x80) *B(b + 134) = S16(a + 18) < S16(b + 18);
    else if (how == 2) *B(b + 134) = S16(b + 18) - S16(CAMERA_LAYER + 18) < 193;
    else *B(b + 134) = (uint8_t)(S8(b + 134) ^ how);
}

/* +575: 3 with +637, else 2 with bit 7 of +73, else 1 when 0x088723D4 says so, else 0. */
SFA_FUNC(08872368, hit_set_575) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    uint32_t v = 0;
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (S8(f + 637) != 0) v = 3;
    else if (S8(f + 73) & 0x80) v = 2;
    else { c->r[17] = 0; c->r[4] = f; f_088723D4(c); if (c->r[2] != 0) v = 1; }
    *B(f + 575) = (uint8_t)v;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* A hit by an object (+8 set): the one hit gets +661; and unless the object is of kind 0x17 or 0x22,
   the other fighter's +537 = 1 and the one hit's = -1, and (for a hit with byte 117 not 0xFF, health
   left, and the object's +271 set) BATTLE + 28 to + 30 = 15, 2, 2. */
SFA_FUNC(088724C0, hit_by_object) {
    gaddr o = ARG(0), f = ARG(1);
    if (*B(o + 8) == 0) return;
    *B(f + 661) = 1;
    if (*B(o + 2) == 0x17 || *B(o + 2) == 0x22) return;
    *B(*W(HIT + 24) + 537) = 1;
    *B(f + 537) = 0xFF;
    if (*B(f + 117) == 0xFF || S16(f + F_HEALTH) < 0 || S8(o + 271) == 0) return;
    *B(BATTLE + 28) = 0xF; *B(BATTLE + 29) = 2; *B(BATTLE + 30) = 2;
}

/* Damage by the other fighter's ISM (+386): a fifth less for 1, the value never under
   1, a fifth more for the others unless the hit's byte 19 is 2 (never over 127), unchanged for 0;
   then one less while the other's +308 is set, never under 1. Values of 0 or less are left. */
SFA_FUNC(08872C50, hit_damage_by_ism) {
    gaddr other = *W(HIT + 24);
    int32_t v = (int32_t)ARG(1);
    if (v > 0) {
        int32_t fifth = v / 5;
        int8_t ism = S8(other + 386);
        if (ism == 1) { v -= fifth; if (v <= 0) v = 1; }
        else if (ism != 0 && *B(ARG(0) + 19) != 2) { v += fifth; if (v >= 128) v = 0x7F; }
    }
    if (S8(other + 308) != 0) { v -= 1; if (v <= 0) v = 1; }
    RETURN(v);
}

/* The value scaled by +644 of the one hit (or of the first argument, when the one hit has +235 without
   bit 7 of +307 and the first's is set): doubled for 1, a quarter more for 2, a half more for 3. Not
   for objects (+8). */
SFA_FUNC(088732E0, hit_scale_by_644) {
    gaddr a = ARG(0), f = ARG(1);
    int32_t v = (int32_t)ARG(2);
    if (*B(a + 8) == 0) {
        int8_t k = S8(f + 644);
        if (S8(f + 235) != 0 && S8(a + 644) != 0 && (*B(f + 307) & 0x80) == 0) k = S8(a + 644);
        if (k == 1) v = (int32_t)((uint32_t)v << 1);
        else if (k == 2) v += v >> 2;
        else if (k == 3) v += v >> 1;
    }
    RETURN(v);
}

/* ---- 0.77.0 ---- */

/* (attacker, defender, hit, v): the damage of a hit: v (or the attacker's +260 when set) through the
   ISM and kind-0x22 adjustments; then, unless that leaves 0 (bits 0 - 6), the s8 from 0x08A1C0C8 by it
   and the hit record fighter's CPU level (+375), three quarters for a hit whose +19 is 2, at least 1,
   half as much more with the hit record fighter's +309, scaled by +644 and kept under 128. */
SFA_FUNC(08872B54, hit_damage) {
    gaddr a = ARG(0), d = ARG(1), hit = ARG(2), other = *W(HIT + 24);
    gaddr s[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    uint32_t v = ARG(3);
    c->r[29] -= 0x30;
    c->r[16] = hit; c->r[17] = d; c->r[18] = a; c->r[19] = other; c->r[20] = HIT;
    if (S8(a + 260) != 0) v = (uint32_t)(int32_t)S8(a + 260);
    v = CALL2(f_08872C50, hit, v);
    v = CALL2(f_08872CD8, a, v) & 0x7F;
    if (v != 0) {
        int32_t n = S8(0x08A1C0C8u + (v << 5) + (uint32_t)(int32_t)S8(*W(HIT + 24) + 375));
        if (*B(hit + 19) == 2) n = (n >> 1) + (n >> 2);
        if (n == 0) n = 1;
        if (S8(other + 309) != 0) n += n >> 1;
        n = (int32_t)CALL3(f_088732E0, a, d, (uint32_t)n);
        v = (uint32_t)(n < 128 ? n : 0x7F);
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4];
    RETURN(v);
}

/* ---- 0.80.0 ---- */

/* (attacker, defender, hit): the frames both fighters are held (+127 of each) from the pair at
   0x08A21EC4 by the hit's +16 (the second half of it for a defender with +117 bit 7); none for +16
   with bit 7, and nothing while the attacker's +136. A defender with +590 holds at least 16 each; one
   with +637 half (the defender's only for a hit not from an object, +8); while the hit record's +46,
   16 more each, at most 30. */
SFA_FUNC(0887223C, hit_hold_frames) {
    gaddr a = ARG(0), d = ARG(1);
    uint8_t k = *B(ARG(2) + 16);
    if (k & 0x80) { *B(a + 127) = 0; *B(d + 127) = 0; return; }
    if (S8(a + 136) != 0) return;
    gaddr p = 0x08A21EC4u + k * 4u;
    if (*B(d + 117) & 0x80) p += 2;
    if (S8(d + 590) != 0) {
        int8_t v = S8(p);
        *B(a + 127) = (uint8_t)(v < 16 ? 16 : v);
        v = S8(p + 1);
        *B(d + 127) = (uint8_t)(v < 16 ? 16 : v);
        return;
    }
    if (S8(d + 637) == 0) {
        *B(a + 127) = (uint8_t)S8(p);
        *B(d + 127) = (uint8_t)S8(p + 1);
    } else {
        *B(a + 127) = (uint8_t)(S8(p) >> 1);
        if (*B(a + 8) == 0) *B(d + 127) = (uint8_t)(S8(p + 1) >> 1);
    }
    if (*B(HIT + 46) == 0) return;
    *B(a + 127) = (uint8_t)(S8(a + 127) + 16);
    *B(d + 127) = (uint8_t)(S8(d + 127) + 16);
    if (S8(a + 127) >= 31) *B(a + 127) = 0x1E;
    if (S8(d + 127) >= 31) *B(d + 127) = 0x1E;
}

/* ---- 0.82.0 ---- */

void f_08821368(Cpu *c);   /* a random number */

/* (attacker, v): v lessened by the count of hits in a row (+126; the s8 at 0x08A21F5E, 3 from 16 on):
   0 halves it, 1 takes a quarter off, 2 an eighth, 3 a sixteenth, 4 a 32nd. +669 / +670 become 4 /
   10 unless +670 is 11. Only for a human attacker pressing a button (+760) against a CPU or an idle
   human, while the hit record's +44 is clear; in Training the side the flags at 0x0950FF70 name does it
   always (with bit 7, at random). */
SFA_FUNC(08873160, hit_scale_by_count) {
    gaddr a = ARG(0), other = *W(HIT + 24);
    int32_t v = (int32_t)ARG(1);
    gaddr s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x20;
    c->r[16] = (uint32_t)v; c->r[17] = a; c->r[18] = other; c->r[19] = HIT;
    int scale = 0;
    if (S8(0x08E75910u + 114) == 2) {   /* GAME + 114, the mode: Training */
        uint8_t flags = *B(0x0950FF70u);
        if (flags & (1u << (S8(a + 337) & 31))) {
            if ((flags & 0x80) == 0) scale = 1;
            else { f_08821368(c); if (c->r[2] & 0x80) scale = 1; }
        }
    }
    if (!scale) {
        if (S8(a + 373) != 0 || *B(HIT + 44) != 0 || *H(a + 760) == 0) goto done;
        if (S8(other + 373) == 0 && *H(other + 760) != 0) goto done;
    }
    if (S8(a + 670) != 0xB) { *B(a + 669) = 4; *B(a + 670) = 0xA; }
    int8_t n = S8(a + 126);
    uint32_t k = n < 16 ? (uint32_t)(int32_t)S8(0x08A21F5Eu + (uint32_t)(int32_t)n) : 3;
    switch (k) {
    case 0: v >>= 1; break;
    case 1: v -= v >> 2; break;
    case 2: v -= v >> 3; break;
    case 3: v -= v >> 4; break;
    case 4: v -= v >> 5; break;
    default: break;
    }
done:
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
    RETURN(v);
}

void f_0886E3CC(Cpu *c);   /* fighter_call_08805310_by_kind (state.c) */

/* (attacker, defender, hit): 0x0886E3CC for the defender (and for some kinds the hit record's fighter
   too) with a kind and a length: 10 / 0x80 for a defender at health -1; with the hit record's +72, 9 /
   0x10 when the defender's +590 has the bit for its +122, else 7 / 5 for both; with +588, 9 and its
   +127; otherwise by the hit's +19: 0 → 8, 1 → 9 for both, 2 → 10 for both, with the length from the
   u16 at 0x08A1FF60 by the hit record fighter's +194 (entry 16 for a +194 of 2 with kind 2). */
SFA_FUNC(08874CDC, hit_pad_effects) {
    gaddr d = ARG(1), hit = ARG(2), other = *W(HIT + 24);
    gaddr s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    uint32_t k1 = 0xFF, len1 = 0, k2 = 0xFF, len2 = 0;
    c->r[29] -= 0x30;
    c->r[16] = d; c->r[19] = 0xFF;
    if (S16(d + 112) == -1) { k2 = 10; len2 = 0x80; }
    else if (*B(HIT + 72) != 0) {
        if (S8(d + 590) & (1 << (S8(d + 122) & 31))) { k2 = 9; len2 = 0x10; }
        else { k2 = 7; len2 = 5; k1 = 7; len1 = 5; }
    } else if (S8(d + 588) != 0) { k2 = 9; len2 = *B(d + 127); }
    else {
        uint8_t q = *B(hit + 19);
        int32_t which = S8(other + 194);
        if (q == 0) { k2 = 8; len2 = *H(0x08A1FF60u + (uint32_t)(which * 2)) & 0xFF; }
        else if (q == 1) { k2 = 9; len2 = *H(0x08A1FF60u + (uint32_t)(which * 2)) & 0xFF; k1 = k2; len1 = len2; }
        else if (q == 2) {
            k2 = 10; k1 = 10;
            len2 = (which == 2 ? *H(0x08A1FF60u + 32) : *H(0x08A1FF60u + (uint32_t)(which * 2))) & 0xFF;
            len1 = len2;
        }
    }
    c->r[17] = k2; c->r[18] = len2;
    if (k1 != 0xFF) { c->r[4] = other; c->r[5] = k1; c->r[6] = len1; f_0886E3CC(c); }
    if (k2 != 0xFF) { c->r[4] = d; c->r[5] = k2; c->r[6] = len2; f_0886E3CC(c); }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}
