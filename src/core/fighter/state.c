/* Small steps and questions of a fighter's frame.
 *
 * A fighter is 0x498 bytes (the first at 0x092E3EA0). Fields used here; names are given where the code
 * shows what they are, and the rest keep their offset.
 */
#include "../decomp.h"

#define GAME       0x08E75910u
#define G_HALT_38  38            /* s8 */
#define G_FLAG_72  72            /* s8 */
#define BATTLE     0x092E1228u
#define BT_FLAG_13 13            /* s8 */
#define BT_FLAG_14 14            /* s8 */
#define BT_LAYOUT  56            /* s16: 3 is two players against one */
#define FIGHTER_1  0x092E3EA0u
#define FIGHTER_2  0x092E4338u
#define PADS       0x08E75BA0u   /* 44 bytes each: +12 the buttons pressed this frame */
#define PAD_START  0x8000

#define F_ACTIVE      0      /* u8: 1 while in play */
#define F_FACING      11     /* u8: 0 facing right */
#define F_X           18     /* s16 */
#define F_OPPONENT    92     /* u32: the fighter it is fighting */
#define F_SIDE        337    /* s8: bit 0 set for the second side */
#define F_HELD_366    366    /* s16 */
#define F_SLIDE       690    /* s8: frames of sliding left, an index into SLIDE_STEPS */
#define F_OPPONENT_SIDE 697  /* s8: 1 while the opponent is to the right */
#define F_DIRECTIONS  752    /* u16: 8 is up */

#define SLIDE_STEPS 0x08A190F7u  /* s8 per frame left: how far the fighter slides */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

/* Which side the opponent is on, noted only while the two are more than 24 apart (so it doesn't flip
   as they cross). */
SFA_FUNC(088593D8, fighter_note_opponent_side) {
    gaddr f = ARG(0);
    int16_t d = (int16_t)(S16(f + F_X) - S16(*W(f + F_OPPONENT) + F_X));
    if ((uint16_t)(d + 24) >= 49) *B(f + F_OPPONENT_SIDE) = d < 0;
}

/* The fighter faces away from its opponent. */
SFA_FUNC(088622CC, fighter_faces_away) { RETURN(S8(ARG(0) + F_OPPONENT_SIDE) != *B(ARG(0) + F_FACING)); }

/* One frame of a slide: forward by the table's step for the frames left. */
SFA_FUNC(08859C10, fighter_slide) {
    gaddr f = ARG(0);
    int8_t left = S8(f + F_SLIDE);
    if (left == 0) return;
    *B(f + F_SLIDE) = (uint8_t)(left - 1);
    int8_t step = S8(SLIDE_STEPS + (uint32_t)left);
    if (*B(f + F_FACING) == 0) *H(f + F_X) = (uint16_t)(S16(f + F_X) + step);
    else *H(f + F_X) = (uint16_t)(S16(f + F_X) - step);
}

/* Up is held, and nothing halts the fight. */
SFA_FUNC(08861E94, fighter_holds_up) {
    int halted = S8(BATTLE + BT_FLAG_14) | S8(BATTLE + BT_FLAG_13) | S8(GAME + G_HALT_38);
    RETURN(!halted && (*H(ARG(0) + F_DIRECTIONS) & 8) != 0);
}

/* The other side's first fighter, into both `out`s. */
SFA_FUNC(08863384, fighter_other_side) {
    gaddr other = (S8(ARG(0) + F_SIDE) & 1) ? FIGHTER_1 : FIGHTER_2;
    *W(ARG(2)) = other;
    *W(ARG(1)) = other;
}

/* Two players against one: the second fighter takes the first's +366. */
SFA_FUNC(0885971C, fighters_share_366) {
    if (S16(BATTLE + BT_LAYOUT) == 3) *H(FIGHTER_2 + F_HELD_366) = (uint16_t)S16(FIGHTER_1 + F_HELD_366);
}

/* Bit 0x40 of +248 is dropped unless +112 is 0x90. */
SFA_FUNC(08872994, fighter_drop_flag_40) {
    gaddr f = ARG(0);
    int8_t v = S8(f + 248);
    if (v != 0 && (v & 0x40) != 0 && S16(f + 112) != 0x90) *B(f + 248) = (uint8_t)(v ^ 0x40);
}

/* The three slots at +722 emptied (-1) and the six counters from +728 cleared. */
SFA_FUNC(08873D30, fighter_clear_722) {
    gaddr f = ARG(0);
    *H(f + 726) = 0xFFFF; *H(f + 724) = 0xFFFF; *H(f + 722) = 0xFFFF;
    *H(f + 748) = 0; *H(f + 744) = 0; *H(f + 740) = 0; *H(f + 736) = 0; *H(f + 732) = 0; *H(f + 728) = 0;
}

/* In play, with none of +270, +702, +135 and +573 set, the byte +296 points at set and +272 clear. */
SFA_FUNC(088741C8, fighter_free_741C8) {
    gaddr f = ARG(0);
    RETURN(*B(f + F_ACTIVE) == 1
           && (S8(f + 270) | S8(f + 702) | S8(f + 135) | S8(f + 573)) == 0
           && *B(*W(f + 296)) != 0
           && *W(f + 272) == 0);
}

/* The fighter is in routine 1's step 0 (+4, +5), and that step's part (+6) is none of 3, 5, 7 and 8. */
SFA_FUNC(0887BA84, fighter_in_plain_step) {
    gaddr f = ARG(0);
    uint8_t part = *B(f + 6);
    RETURN(*B(f + 4) == 1 && *B(f + 5) == 0 && part != 3 && part != 5 && part != 7 && part != 8);
}

/* +251 set, +262 clear, the opponent's +112 not negative, and the high byte of the word +528 points at
   3 or 4. */
SFA_FUNC(0887BD00, fighter_check_7BD00) {
    gaddr f = ARG(0);
    if (S8(f + 251) == 0 || S8(f + 262) != 0 || S16(*W(f + F_OPPONENT) + 112) < 0) { RETURN(0); return; }
    int kind = S16(*W(f + 528)) & 0xFF00;
    RETURN(kind == 0x300 || kind == 0x400);
}

/* +412 takes the value given, or 0 while GAME + 72. */
SFA_FUNC(088347B4, fighter_set_412) {
    *B(ARG(0) + 412) = S8(GAME + G_FLAG_72) != 0 ? 0 : (uint8_t)ARG(1);
}

/* Start pressed this frame on the pad of the fighter's side (pad 1 for side 0, pad 2 otherwise), unless
   the second argument's +6 or the first's +43 or +40 is set. */
SFA_FUNC(08834604, side_pressed_start) {
    gaddr a = ARG(0), b = ARG(1), fighter = ARG(2);
    if (S8(b + 6) != 0 || S8(a + 43) != 0 || S8(a + 40) != 0) { RETURN(0); return; }
    uint32_t pressed = *W(PADS + (S8(fighter + F_SIDE) != 0 ? 44u : 0u) + 12);
    RETURN(pressed & PAD_START);
}

#define BT_CLOCK 9               /* s8: the round's clock as shown, 99 down to 0 */
#define F_HEALTH 112             /* s16 */

/* How pressed the fighter is, 0 to 3, into +485: the worse of the clock (under 80, 60, 40) and its
   health (under 112, 72, 32). */
SFA_FUNC(0883727C, fighter_pressure) {
    gaddr f = ARG(0);
    int clock = S8(BATTLE + BT_CLOCK), health = S16(f + F_HEALTH);
    int by_clock = clock < 40 ? 3 : clock < 60 ? 2 : clock < 80 ? 1 : 0;
    int by_health = health < 32 ? 3 : health < 72 ? 2 : health < 112 ? 1 : 0;
    *B(f + 485) = (uint8_t)(by_health < by_clock ? by_clock : by_health);
}

/* How far the fighter is from the near edge of its opponent, into +508: the opponent's x moved by the
   box its current frame names (byte 3 of the data at +256 picks one of the 8-byte boxes at +220: an
   offset and a half-width, one of them turned round by which side the fighter is on). 0 for a fighter
   not in play. */
SFA_FUNC(08859238, fighter_distance) {
    gaddr f = ARG(0), opponent = *W(f + F_OPPONENT);
    int16_t distance = 0;
    if (*B(f + F_ACTIVE) != 0) {
        int16_t offset = 0, width = 0;
        uint8_t box = *B(*W(opponent + 256) + 3);
        if (box != 0) {
            gaddr b = *W(opponent + 220) + box * 8u;
            width = S16(b); offset = S16(b + 4);
            if ((S8(f + F_OPPONENT_SIDE) ^ 1) != 0) width = (int16_t)-width;
            else offset = (int16_t)-offset;
        }
        int16_t edge = (int16_t)(offset + S16(opponent + F_X) + width);
        distance = (int16_t)(S16(f + F_X) - edge);
        if (distance < 0) distance = (int16_t)-distance;
    }
    *H(f + 508) = (uint16_t)distance;
}

#define F_ISM       386          /* s8: 1 is V-ISM */
#define F_BUTTONS   760          /* u16: buttons held */
#define F_BUTTONS_2 762          /* u16 */
#define F_PRESSED   764          /* u16 */

/* A punch and the kick of the same strength are held (0x90, 0x120 or 0x240, and nothing else) with
   +366 at 72 or more and +777 clear, by a V-ISM fighter or one with 0x80080 in +792. */
SFA_FUNC(0886A1E4, fighter_holds_punch_and_kick) {
    gaddr f = ARG(0);
    if (S8(f + F_ISM) != 1 && (*W(f + 792) & 0x80080) == 0) { RETURN(0); return; }
    if (S16(f + F_HELD_366) < 72 || S8(f + 777) != 0) { RETURN(0); return; }
    uint16_t held = (uint16_t)(*H(f + F_BUTTONS) | *H(f + F_BUTTONS_2));
    RETURN(held == 0x90 || held == 0x120 || held == 0x240);
}

/* A punch and the kick of the same strength are among the buttons held, and bit 1 of +764 is set. */
SFA_FUNC(0886A264, fighter_pressed_punch_and_kick) {
    gaddr f = ARG(0);
    uint16_t held = (uint16_t)(*H(f + F_BUTTONS) | *H(f + F_BUTTONS_2));
    int pair = ((held & 0x10) && (held & 0x80)) || ((held & 0x20) && (held & 0x100)) || ((held & 0x40) && (held & 0x200));
    RETURN(pair && (*H(f + F_PRESSED) & 2) != 0);
}
