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

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

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
