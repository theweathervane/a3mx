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
#define F_YY          22     /* s16 */
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
#define F_PRESSED   760          /* u16: the bits of + 752 that came on this frame */
#define F_RELEASED  762          /* u16: those that went off */
#define F_OTHER     764          /* u16: a second set of bits (bits 0 and 1 turn with the fighter) */

/* A punch and the kick of the same strength changed together this frame (pressed or released: 0x90,
   0x120 or 0x240, and nothing else) with +366 at 72 or more and +777 clear, for a V-ISM fighter or one
   with 0x80080 in +792. */
SFA_FUNC(0886A1E4, fighter_punch_and_kick_alone) {
    gaddr f = ARG(0);
    if (S8(f + F_ISM) != 1 && (*W(f + 792) & 0x80080) == 0) { RETURN(0); return; }
    if (S16(f + F_HELD_366) < 72 || S8(f + 777) != 0) { RETURN(0); return; }
    uint16_t changed = (uint16_t)(*H(f + F_PRESSED) | *H(f + F_RELEASED));
    RETURN(changed == 0x90 || changed == 0x120 || changed == 0x240);
}

/* A punch and the kick of the same strength are among the buttons that changed this frame, and bit 1
   of +764 is set. */
SFA_FUNC(0886A264, fighter_punch_and_kick_among) {
    gaddr f = ARG(0);
    uint16_t held = (uint16_t)(*H(f + F_PRESSED) | *H(f + F_RELEASED));
    int pair = ((held & 0x10) && (held & 0x80)) || ((held & 0x20) && (held & 0x100)) || ((held & 0x40) && (held & 0x200));
    RETURN(pair && (*H(f + F_OTHER) & 2) != 0);
}

#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define F_FRAME_DATA 296         /* u32: the current frame's data */

static int frame_byte_1_clear(gaddr f) { return *B(f) != 0 && *B(*W(f + F_FRAME_DATA) + 1) == 0; }

/* Each fighter in play whose frame's byte 1 is clear clears a flag on the fighters across from it:
   fighters 1 and 3 clear +570, fighters 2 and 4 clear +571 (1 and 2 spare each other unless the layout
   is 0). */
SFA_FUNC(0885911C, fighters_clear_570) {
    int two_sided = S16(BATTLE + BT_LAYOUT) == 0;
    if (frame_byte_1_clear(FIGHTER_1)) {
        if (two_sided) *B(FIGHTER_2 + 570) = 0;
        *B(FIGHTER_3 + 570) = 0; *B(FIGHTER_4 + 570) = 0;
    }
    if (frame_byte_1_clear(FIGHTER_2)) {
        if (two_sided) *B(FIGHTER_1 + 571) = 0;
        *B(FIGHTER_3 + 571) = 0; *B(FIGHTER_4 + 571) = 0;
    }
    if (frame_byte_1_clear(FIGHTER_3)) { *B(FIGHTER_1 + 570) = 0; *B(FIGHTER_2 + 570) = 0; }
    if (frame_byte_1_clear(FIGHTER_4)) { *B(FIGHTER_1 + 571) = 0; *B(FIGHTER_2 + 571) = 0; }
}

static uint16_t swap_left_right(uint16_t bits) { return (uint16_t)((bits & ~3u) | ((bits & 2) >> 1) | ((bits & 1) << 1)); }

/* The fighter's input for the frame, from the bits 0x08864DFC left in +752: left and right are turned
   round for a fighter facing left, so 1 is always forward; then, against last frame's bits (+754), the
   bits that came on (+760) and went off (+762), and in +666 everything that came on in any of the last
   three frames (+754, +756, +758 are the frames before). +290 keeps the turned bits. +764's left and
   right are turned by which side the opponent is on. */
SFA_FUNC(08859650, fighter_input_edges) {
    gaddr f = ARG(0);
    uint16_t now = *H(f + 752);
    if (*B(f + F_FACING) != 0) { now = swap_left_right(now); *H(f + 752) = now; }
    uint16_t h1 = *H(f + 754), h2 = *H(f + 756), h3 = *H(f + 758);
    uint16_t pressed = (uint16_t)(~h1 & now);
    *H(f + 290) = now;
    *H(f + F_PRESSED) = pressed;
    *H(f + 666) = (uint16_t)((~h3 & h2) | (~h2 & h1) | pressed);
    *H(f + F_RELEASED) = (uint16_t)(~now & h1);
    if (S8(f + F_OPPONENT_SIDE) != 0) *H(f + F_OTHER) = swap_left_right(*H(f + F_OTHER));
}

/* ---- routines picked from tables ---- */

void rt_call_indirect(Cpu *c, uint32_t addr);

/* A fighter's move routine: the one for +656, or for +670 while +669 is set, called with the fighter and
   that number; afterwards +657 remembers +656. */
SFA_FUNC(0886EDA8, fighter_run_move) {
    gaddr f = ARG(0), saved = c->r[16];
    uint32_t n = (uint32_t)(S8(f + 669) != 0 ? S8(f + 670) : S8(f + 656));
    gaddr fn = *W(0x08A1BFF0u + n * 4u);
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[4] = f; c->r[5] = n; c->r[6] = n; c->r[7] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
    *B(f + 657) = *B(f + 656);
    c->r[16] = saved;
}

/* Runs the fighter's script: one command after another (the handler for the low four bits of +489,
   each given what the last one answered, the first the second argument) until the count of commands
   left to run this frame (0x08B0268E) is used up. */
#define SCRIPT_BUDGET 0x08B0268Eu
SFA_FUNC(08877564, fighter_run_script) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    uint32_t value = ARG(1);
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = 0x08A22498u; c->r[18] = 0x08B00000u;
    uint16_t left = (uint16_t)(*H(SCRIPT_BUDGET) - 1);
    for (;;) {
        *H(SCRIPT_BUDGET) = left;
        gaddr fn = *W(0x08A22498u + (uint32_t)(S8(f + 489) & 0xF) * 4u);
        c->r[4] = f; c->r[5] = value; c->r[6] = fn;
        rt_call_indirect(c, fn);
        value = c->r[2];
        left = *H(SCRIPT_BUDGET);
        if (left == 0) break;
        left--;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* Whether both leading fighters are in routine 1, step 6 (the two are found by the layout's routine,
   which fills two pointers on the stack), and the fight is not at state 7 or 10 with BATTLE + 7 clear. */
SFA_FUNC(088632C4, fighters_both_in_step_6) {
    gaddr saved = c->r[16];
    gaddr fn = *W(0x08A195A0u + (uint32_t)S16(BATTLE + BT_LAYOUT) * 4u);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[16] = BATTLE; c->r[5] = sp + 16; c->r[6] = sp + 20; c->r[7] = fn; c->r[8] = 0x08A195A0u;
    rt_call_indirect(c, fn);
    gaddr a = *W(sp + 16), b = *W(sp + 20);
    c->r[29] += 0x20;
    c->r[16] = saved;
    if (*B(a + 4) != 1 || *B(a + 5) != 6 || *B(b + 4) != 1 || *B(b + 5) != 6) { RETURN(0); return; }
    int8_t state = S8(BATTLE + 79);
    RETURN(!((state == 7 || state == 10) && S8(BATTLE + 7) == 0));
}

/* ---- more questions ---- */

static int in_step_1_6(gaddr f) { return *B(f + 4) == 1 && *B(f + 5) == 6; }

/* The fighter this one answers to is in routine 1, step 6: in layouts 1 and 2 the third fighter, for a
   fighter whose +810 is 4; in layout 3 (while GAME + 137) the first fighter of the other side, for
   fighters without bit 1 of their side. */
SFA_FUNC(08862B84, fighter_partner_in_step_6) {
    gaddr f = ARG(0);
    int16_t layout = S16(BATTLE + BT_LAYOUT);
    if (layout == 1 || layout == 2) { RETURN(S8(f + 810) == 4 && in_step_1_6(FIGHTER_3)); return; }
    if (layout != 3 || S8(GAME + 137) == 0 || (S8(f + F_SIDE) & 2) != 0) { RETURN(0); return; }
    RETURN(in_step_1_6(S8(f + F_SIDE) != 0 ? FIGHTER_1 : FIGHTER_2));
}

/* Pushes the opponent back by `push` (less the fighter's own speed when its +73 and +541 are set) while
   the fighter is in routine 1, step 1 and nothing forbids it; then, for a fighter with +238 and bit 7 of
   +73, sets +592 once. */
SFA_FUNC(08862F28, fighter_push_opponent) {
    gaddr f = ARG(0), opponent = ARG(1);
    int16_t push = (int16_t)ARG(2);
    if (*B(f + 4) == 1) {
        int ok = *B(f + 5) == 1 && S8(f + 537) == 0;
        if (ok && S8(f + 73) != 0) {
            if (S8(f + 541) == 0) ok = 0;
            else push = (int16_t)(push - ((int32_t)*W(f + 96) >> 16));
        }
        if (ok && S8(f + 239) != 0) ok = 0;
        if (ok && S8(opponent + 713) == 0 && (S8(opponent + 73) != 0 || S8(opponent + 596) != 0)) ok = 0;
        if (ok) *H(opponent + F_X) = (uint16_t)(S16(opponent + F_X) - push);
    }
    if (S8(f + 238) != 0 && (S8(f + 73) & 0x80) != 0 && S8(f + 592) == 0) *B(f + 592) = 1;
}

/* A jump off the wall: characters 4, 7, 0x12 and 0x1C, more than 32 above the floor, with +78 set, down
   not held, touching a wall (+238: 1 or 2) while moving towards it and holding away from it. */
SFA_FUNC(08864140, fighter_can_wall_jump) {
    gaddr f = ARG(0);
    int8_t who = S8(f + 338), wall = S8(f + 238);
    uint16_t held = *H(f + 362);
    int32_t speed = (int32_t)*W(f + 96);
    if (who != 0x1C && who != 7 && who != 4 && who != 0x12) { RETURN(0); return; }
    if (!(S16(f + 132) + 0x20 < S16(f + F_YY)) || S8(f + 78) == 0 || (held & 4) != 0 || wall == 0) { RETURN(0); return; }
    if (speed > 0) RETURN(wall == 2 && (held & 2) != 0);
    else if (speed < 0) RETURN(wall == 1 && (held & 1) != 0);
    else RETURN(0);
}

/* ---- once a frame, for all four ---- */

void f_0883727C(Cpu *c);   /* fighter_pressure */
void f_08859238(Cpu *c);   /* fighter_distance */
void f_08821368(Cpu *c);   /* a random number */

static void for_each_fighter(Cpu *c, GuestFunc fn) {
    static const gaddr FIGHTERS[4] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, FIGHTER_4 };
    c->r[29] -= 0x20;
    for (int i = 0; i < 4; i++) { c->r[4] = FIGHTERS[i]; fn(c); }
    c->r[29] += 0x20;
}
SFA_FUNC(08837238, fighters_pressure) { for_each_fighter(c, f_0883727C); }
SFA_FUNC(088591F4, fighters_distance) { for_each_fighter(c, f_08859238); }

/* The frame's random entry: the hit record's first word points at one of the 32 words at 0x08A18FC0. */
SFA_FUNC(088590E4, frame_random_entry) {
    c->r[29] -= 0x10;
    f_08821368(c);
    *W(0x09358410u) = 0x08A18FC0u + (c->r[2] & 0x1F) * 4u;
    c->r[29] += 0x10;
}

/* +254 set: for a fighter with +309 clear whose ISM is chosen (+386 not -1), or otherwise with bit 26 of
   +792. */
SFA_FUNC(088597AC, fighter_set_254) {
    gaddr f = ARG(0);
    if ((S8(f + 309) == 0 && S8(f + F_ISM) != -1) || (*W(f + 792) & 0x4000000u) != 0) *B(f + 254) = 1;
}

/* When the second argument's +4, +70 and +6 are all clear the fighter's +340 goes up by one; otherwise
   its +412 is cleared. */
SFA_FUNC(08833C58, fighter_count_340) {
    gaddr b = ARG(1), f = ARG(2);
    if ((S8(b + 4) | S8(b + 70) | S8(b + 6)) != 0) *B(f + 412) = 0;
    else *B(f + 340) = (uint8_t)(S8(f + 340) + 1);
}

/* ---- the character's own routines (the table +416 points at) ---- */

static void run_character_routine(Cpu *c, gaddr f, uint32_t which) {
    gaddr fn = *W(*W(f + 416) + which);
    c->r[29] -= 0x20;
    c->r[5] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

SFA_FUNC(0885F838, fighter_character_routine_76) { run_character_routine(c, ARG(0), 76); }

/* Three countdowns (+638, +643, +542) tick, then the character's routine at +64. */
SFA_FUNC(0885B7D4, fighter_tick_and_routine_64) {
    gaddr f = ARG(0);
    int8_t a = S8(f + 638), b = S8(f + 643), d = S8(f + 542);
    if (a != 0) *B(f + 638) = (uint8_t)(a - 1);
    if (b != 0) *B(f + 643) = (uint8_t)(b - 1);
    if (d != 0) *B(f + 542) = (uint8_t)(d - 1);
    c->r[6] = (uint32_t)(b != 0 ? b - 1 : b);
    run_character_routine(c, f, 64);
}

/* +551 and +539 cleared, then the character's routine at +88, or at +92 while +373 is set. */
SFA_FUNC(0885C1EC, fighter_routine_88_or_92) {
    gaddr f = ARG(0);
    *B(f + 551) = 0; *B(f + 539) = 0;
    c->r[6] = (uint32_t)S8(f + 373);
    run_character_routine(c, f, S8(f + 373) != 0 ? 92 : 88);
}

/* ---- 0.67.0 ---- */

void f_08872994(Cpu *c);   /* fighter_drop_flag_40 */
void f_0887A10C(Cpu *c);   /* fighter_script_read (object/screen_steps.c) */
SFA_FUNC(08872950, fighters_drop_flag_40) { for_each_fighter(c, f_08872994); }

/* The fighter's walking speed: forward (+78 = 1) unless back is held, from the character's pair in the
   table +704 points at (s16, in 256ths), towards where it faces. */
SFA_FUNC(08861DE4, fighter_set_walk_speed) {
    gaddr f = ARG(0);
    uint32_t forward = (*H(f + 752) & 2) ? 0 : 1;
    *B(f + 78) = (uint8_t)forward;
    int32_t v = S16(*W(f + 704) + ((uint32_t)(S8(f + 338) << 1) + forward) * 2u) << 8;
    *W(f + 96) = (uint32_t)(*B(f + F_FACING) != 0 ? -v : v);
}

/* +564 cleared, then the character's routine at +80. */
SFA_FUNC(08863918, fighter_routine_80) { *B(ARG(0) + 564) = 0; run_character_routine(c, ARG(0), 80); }

/* A change of character asked for (bit 0 of +627): characters 0x11 and 0x13 become each other, and the
   first routine of the character table's +140 list is called with the new one. */
SFA_FUNC(08864494, fighter_swap_11_13) {
    gaddr f = ARG(0);
    if ((S8(f + 627) & 1) == 0) return;
    *B(f + 627) = 0;
    int8_t who = S8(f + 338);
    if (who != 0x11 && who != 0x13) return;
    *B(f + 338) = (uint8_t)(who ^ 2);
    gaddr fn = *W(*W(*W(f + 416) + 140));
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S8(f + 338); c->r[6] = c->r[5]; c->r[7] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* +593 from the table 0x08A19632 by character and ISM. */
SFA_FUNC(08864D70, fighter_set_593) {
    gaddr f = ARG(0);
    *B(f + 593) = *B(0x08A19632u + (uint32_t)(S8(f + 338) << 2) + (uint32_t)S8(f + F_ISM) + 1);
}

/* The second fighter is within 128 of the first (to either side) and its +551 is clear. */
SFA_FUNC(08876F44, fighter_near_and_551_clear) {
    gaddr a = ARG(0), b = ARG(1);
    uint16_t d = (uint16_t)(S16(b + F_X) - S16(a + F_X) + 0x80);
    RETURN(d < 257 && S8(b + 551) == 0);
}

/* A chance by the fighter's level (+375): a random one of the 32 bits of the table's word for that
   level. Answers the bit, not 0 or 1. */
SFA_FUNC(08877258, fighter_chance_by_level) {
    gaddr f = ARG(0), table = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = table; c->r[17] = f;
    f_08821368(c);
    RETURN(*W(table + (uint32_t)S8(f + 375) * 4u) & (1u << (c->r[2] & 0x1F)));
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* Script commands (each is given the fighter and the place in the script after its own word).
   A nested command: the depth (+501) up by one, the next word into +490, and that command's handler
   (0x08A22598) on what follows. */
SFA_FUNC(08878A20, script_nested) {
    gaddr f = ARG(0), p = ARG(1);
    *B(f + 501) = (uint8_t)(S8(f + 501) + 1);
    *H(f + 490) = *H(p);
    gaddr fn = *W(0x08A22598u + (uint32_t)S16(f + 490) * 4u);
    c->r[29] -= 0x20;
    c->r[5] = p + 2; c->r[6] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* The end of a nested command: the depth down by one, and the next command read. */
SFA_FUNC(08879D98, script_nested_end) {
    gaddr f = ARG(0);
    *B(f + 501) = (uint8_t)(S8(f + 501) - 1);
    c->r[29] -= 0x20;
    f_0887A10C(c);
    c->r[29] += 0x20;
}

/* (level, bits): for a fighter above that level (+375) the bits are set in +544; then the next command. */
SFA_FUNC(08879FC4, script_set_544_above_level) {
    gaddr f = ARG(0), p = ARG(1);
    if (S16(p) < S8(f + 375)) *H(f + 544) = (uint16_t)(S16(f + 544) | S16(p + 2));
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p + 4; c->r[6] = p + 4;
    f_0887A10C(c);
    c->r[29] += 0x20;
}

/* Which side of the screen a fighter's display is on: the table 0x08A2A116 by layout and side, turned
   round in layout 3 unless bit 0 of GAME + 57. */
SFA_FUNC(088988FC, fighter_display_side) {
    int16_t layout = S16(BATTLE + BT_LAYOUT);
    uint32_t side = *B(0x08A2A116u + (uint32_t)(layout << 2) + (uint32_t)S8(ARG(0) + F_SIDE));
    if (layout == 3 && (S8(GAME + 57) & 1) == 0) side = (side ^ 1) & 0xFF;
    RETURN(side);
}

/* ---- 0.68.0 ---- */

/* The fighter's character number as the voices and names use it: 4 and 6 become 0x12 and 0x14 for a
   fighter whose ISM is negative, and anyone with +373 is 0x15 while BATTLE + 59 is 2. */
SFA_FUNC(08812FD4, fighter_character_for_display) {
    gaddr f = ARG(0);
    uint32_t who = (uint32_t)S8(f + 338) & 0xFFFF;
    if (who == 4 && S8(f + F_ISM) < 0) who = 0x12;
    if (who == 6 && S8(f + F_ISM) < 0) who = 0x14;
    if (S8(BATTLE + 59) == 2 && S8(f + 373) != 0) who = 0x15;
    RETURN(who);
}

/* A countdown (+557, from +556) that walks +555 through its row (by +589) of the 32-wide table
   0x08A1FD5C, never above +598; past the row's 32 entries it stays at +598. */
SFA_FUNC(08859AD8, fighter_step_555) {
    gaddr f = ARG(0);
    int8_t left = S8(f + 557);
    if (left == 0) return;
    *B(f + 557) = (uint8_t)(left - 1);
    int32_t done = S8(f + 556) - S8(f + 557);
    int8_t cap = S8(f + 598), v = cap;
    if (done < 32) {
        int8_t t = S8(0x08A1FD5Cu + (uint32_t)((S8(f + 589) << 5) + done));
        if (t < cap) v = t;
    }
    *B(f + 555) = (uint8_t)v;
}

/* The fighter's ISM data: the character table's +20 list holds a pointer for each ISM (after its first
   word) to seven words, kept in +128 (the pointer), +208 to +224, +200 and +300. */
SFA_FUNC(08862D88, fighter_load_ism_data) {
    gaddr f = ARG(0);
    gaddr p = *W(*W(*W(f + 416) + 20) + (uint32_t)(S8(f + F_ISM) << 2) + 4);
    *W(f + 128) = p;
    for (int i = 0; i < 5; i++) *W(f + 208 + (uint32_t)i * 4u) = W(p)[i];
    *W(f + 200) = W(p)[5];
    *W(f + 300) = W(p)[6];
}

/* An object moved by the character's offset (the s16 pair at 0x08A196DA): x towards where the fighter
   faces, y as given. */
SFA_FUNC(08864D98, fighter_offset_object) {
    gaddr f = ARG(0), o = ARG(1);
    gaddr t = 0x08A196DAu + (uint32_t)(S8(f + 338) << 2);
    *H(o + 22) = (uint16_t)(S16(o + 22) + S16(t + 2));
    *H(o + 18) = (uint16_t)(*B(f + F_FACING) != 0 ? S16(o + 18) - S16(t) : S16(o + 18) + S16(t));
}

/* One of BATTLE's two words at +136 and +140 for the fighter (by its +121), the other way round in
   layouts other than 2 and (unless bit 0 of GAME + 57) 3. */
SFA_FUNC(0886B4D4, fighter_battle_word) {
    int second = S8(ARG(0) + 121) != 0;
    int16_t layout = S16(BATTLE + BT_LAYOUT);
    int straight = layout == 2 || (layout == 3 && (S8(GAME + 57) & 1) == 0);
    RETURN(*W(BATTLE + ((second == straight) ? 140 : 136)));
}

/* By layout, a routine (0x08A22188) for a fighter with +251 and +816 both clear; otherwise 0. */
#include "../object/step.h"
SFA_FUNC(08876BE4, fighter_run_by_layout) {
    gaddr f = ARG(0);
    RETURN(0);
    if (S8(f + 251) != 0 || *W(f + 816) != 0) return;
    run_step(c, (uint32_t)S16(BATTLE + BT_LAYOUT), 0x08A22188u, 5, 0x08876C44u);
}

/* A script command that skips to the end of its block: words are passed over, 0x7600 opening a nested
   block and 0x7602 closing one, until the close that ends this block; then the next command is read. */
SFA_FUNC(08879D1C, script_skip_block) {
    gaddr f = ARG(0), p = ARG(1), next;
    int8_t start = S8(f + 501);
    for (;;) {
        int16_t w = S16(p);
        next = p + 2;
        if (w == 0x7600) *B(f + 501) = (uint8_t)(S8(f + 501) + 1);
        if (w == 0x7602) {
            *B(f + 501) = (uint8_t)(S8(f + 501) - 1);
            if (start == S8(f + 501) + 1) break;
        }
        p = next;
    }
    c->r[29] -= 0x20;
    c->r[5] = next;
    f_0887A10C(c);
    c->r[29] += 0x20;
}

/* ---- 0.69.0: the CPU's scripts ---- */

void f_088D5E44(Cpu *c);   /* game_network_battle (task/fight.c) */
void f_08877258(Cpu *c);   /* fighter_chance_by_level */
void f_08876BE4(Cpu *c);   /* fighter_run_by_layout */
void f_08822AEC(Cpu *c);   /* fighter_reset_steps (reset.c) */

static uint32_t random32(Cpu *c) { f_08821368(c); return c->r[2]; }

/* In a network battle with +815 set, a script is chosen by chance instead of by the fighter's pressure:
   one time in four the first, otherwise the second, third or fourth, each only if the character's byte
   for this opponent (the table's +172, by the opponent's character) allows it (bits 0 to 2). */
static int network_choice(Cpu *c, gaddr f, uint32_t *choice) {
    f_088D5E44(c);
    if (c->r[2] == 0 || S8(f + 815) == 0) return 0;
    gaddr table = *W(f + 416);
    int8_t allowed = S8(*W(table + 172) + (uint32_t)S8(*W(f + F_OPPONENT) + 338));
    uint32_t r = random32(c) & 3;
    *choice = (r != 0 && (allowed & (1 << (r - 1)))) ? r : 0;
    return 1;
}

static void start_script(gaddr f, gaddr script) {
    *H(f + 486) = *H(script);
    *B(f + 565) = 0; *B(f + 501) = 0;
}

/* Picks the fighter's script from the character's first list (+48: one for each pressure, +485; or +156
   in the network case) and starts it; +511 keeps which. */
SFA_FUNC(0887B1B8, fighter_pick_script) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    uint32_t n;
    c->r[29] -= 0x10;
    c->r[16] = f;
    *B(f + 550) = 0; *B(f + 511) = 0;
    gaddr table = *W(f + 416), script;
    if (network_choice(c, f, &n)) script = *W(*W(table + 156) + n * 4u);
    else { n = *B(f + 485); script = *W(*W(table + 48) + n * 4u); }
    *B(f + 511) = (uint8_t)n;
    *W(f + 512) = script + 2;
    *W(f + 532) = script + 2;
    start_script(f, script);
    RETURN(script + 2);
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* Picks from the second list (+52, or +160 in the network case): each entry is a table of 32 bytes and a
   list of scripts, and a random byte of the table (moved on by +499 offline) picks the script. */
SFA_FUNC(0887C25C, fighter_pick_script_at_random) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    uint32_t n;
    c->r[29] -= 0x10;
    c->r[16] = f;
    *B(f + 550) = 0;
    gaddr table = *W(f + 416), script;
    if (network_choice(c, f, &n)) {
        gaddr entry = *W(table + 160) + n * 8u;
        *B(f + 511) = (uint8_t)n;
        gaddr bytes = *W(entry), scripts = *W(entry + 4);
        script = *W(scripts + (uint32_t)(S8(bytes + (random32(c) & 0x1F)) << 2));
    } else {
        gaddr list = *W(table + 52), bytes = *W(list + *B(f + 485) * 8u);
        int8_t at = (int8_t)((random32(c) & 0x1F) + (uint32_t)S8(f + 499));
        int8_t pick = S8(bytes + (uint32_t)at);
        *B(f + 511) = (uint8_t)pick;
        script = *W(*W(list + *B(f + 485) * 8u + 4) + (uint32_t)(pick << 2));
    }
    *W(f + 516) = script + 2;
    start_script(f, script);
    RETURN(script + 2);
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* Whether a CPU fighter acts now: not with +503 or +251; then by chance for its level (the table
   0x08A22A20; a random number is drawn and thrown away first), and if the layout's routine agrees. */
SFA_FUNC(088771DC, fighter_cpu_acts) {
    gaddr f = ARG(0), saved = c->r[16];
    uint32_t yes = 0;
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (S8(f + 503) == 0 && S8(f + 251) == 0) {
        f_08821368(c);
        c->r[4] = f; c->r[5] = 0x08A22A20u;
        f_08877258(c);
        if (c->r[2] != 0) {
            c->r[4] = f;
            f_08876BE4(c);
            yes = c->r[2] != 0;
        }
    }
    RETURN(yes);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The CPU's wait before it acts (+506): its level's value from 0x08A2271C plus a random one of the 32
   at 0x08A226DC, never under 0. */
SFA_FUNC(0887B48C, fighter_cpu_set_wait) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = f;
    *H(f + 506) = *H(0x08A2271Cu + (uint32_t)(S8(f + 375) * 2));
    int16_t base = S16(f + 506);
    c->r[17] = (uint32_t)base;
    int16_t v = (int16_t)(base + S16(0x08A226DCu + (random32(c) & 0x1F) * 2u));
    *H(f + 506) = (uint16_t)(v < 0 ? 0 : v);
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* Three fields of a script word: bits 4 - 9 into +236, bit 10 into +193, bits 13 - 15 into +194. */
SFA_FUNC(0887B648, fighter_unpack_script_word) {
    gaddr f = ARG(0);
    uint32_t w = ARG(1) & 0xFFFF;
    *B(f + 236) = (uint8_t)((w >> 4) & 0x3F);
    *B(f + 193) = (uint8_t)((w >> 10) & 1);
    *B(f + 194) = (uint8_t)((w >> 13) & 7);
}

/* All four fighters' steps reset (3, 4, 1, 2). */
SFA_FUNC(08822AA8, fighters_reset_steps) {
    static const gaddr ORDER[4] = { FIGHTER_3, FIGHTER_4, FIGHTER_1, FIGHTER_2 };
    c->r[29] -= 0x20;
    for (int i = 0; i < 4; i++) { c->r[4] = ORDER[i]; f_08822AEC(c); }
    c->r[29] += 0x20;
}

void f_0887B1B8(Cpu *c);   /* fighter_pick_script */
void f_0887A154(Cpu *c);   /* the script command's handler (object/steps.c) */

/* The CPU's script restarted from where it was kept (+512, with +504 as its first word); a command
   whose high byte is -1 picks a new script first. */
SFA_FUNC(0887A350, fighter_restart_script) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 565) = 0; *B(f + 501) = 0; *B(f + 484) = 0;
    *H(f + 486) = (uint16_t)S16(f + 504);
    *W(f + 528) = *W(f + 512);
    *H(f + 488) = *H(*W(f + 512));
    if (S8(f + 489) == -1) {
        c->r[4] = f; f_0887B1B8(c);
        c->r[4] = f; c->r[5] = c->r[2]; f_0887A154(c);
    }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* ---- 0.70.0 ---- */

void f_0887B48C(Cpu *c);   /* fighter_cpu_set_wait */
void f_08877F44(Cpu *c);

/* (fighter, script): the wait set, then the script read on, or (with +506) +482 and +483 cleared and
   0x08877F44. */
SFA_FUNC(08877BC4, fighter_cpu_wait_then_read) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    f_0887B48C(c);
    c->r[4] = f; c->r[5] = p;
    if (S16(f + 506) == 0) f_0887A10C(c);
    else { *B(f + 483) = 0; *B(f + 482) = 0; f_08877F44(c); }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* A script's block left: words passed over (0x7600 opens a nested block, 0x7602 closes one) to this
   block's close or its 0x7601, and the script read on from the word after. */
SFA_FUNC(08879C8C, fighter_script_leave_block) {
    gaddr f = ARG(0), p = ARG(1);
    int32_t depth = S8(f + 501), now = depth;
    for (;; p += 2) {
        if (S16(p) == 0x7600) { *B(f + 501) = (uint8_t)(now + 1); now = S8(f + 501); }
        if (S16(p) == 0x7602) {
            *B(f + 501) = (uint8_t)(now - 1); now = S8(f + 501);
            if (depth == now + 1) break;
        }
        if (S16(p) == 0x7601 && depth == now) break;
    }
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p + 2; c->r[6] = p + 2;
    f_0887A10C(c);
    c->r[29] += 0x20;
}

void f_0887BA84(Cpu *c);

/* Characters 4, 7 and 28 with +496, more than 32 below +132, with +78 set: 1 when +238 is 1 moving
   left or up (+96 negative) or 2 the other way. */
SFA_FUNC(08877148, fighter_asks_08877148) {
    gaddr f = ARG(0);
    int8_t ch = S8(f + 338), k = S8(f + 238);
    int32_t speed = (int32_t)*W(f + 96);
    RETURN(0);
    if (S8(f + 496) == 0 || (ch != 4 && ch != 7 && ch != 0x1C)) return;
    if (!(S16(f + 132) + 0x20 < S16(f + 22)) || S8(f + 78) == 0 || k == 0 || speed == 0) return;
    RETURN(k == (speed <= 0 ? 1 : 2));
}

/* 0x0887BA84 says yes and the opponent is in routine 1, step 1, part 1, +7 = 1, with +116 = 2 and
   +698 set. */
SFA_FUNC(0887BAD8, fighter_asks_of_opponent) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    f_0887BA84(c);
    int yes = 0;
    if (c->r[2] != 0) {
        gaddr o = *W(f + 92);
        c->r[16] = o;
        yes = *B(o + 4) == 1 && *B(o + 5) == 1 && *B(o + 6) == 1 && *B(o + 7) == 1 && *B(o + 116) == 2 && S8(o + 698) != 0;
    }
    RETURN(yes);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* ---- 0.71.0: the CPU's script branches ---- */

void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887C25C(Cpu *c);   /* fighter_pick_script_at_random */

/* (fighter, script): the script read on into the block when `yes`, or the block left. */
static void script_branch(Cpu *c, gaddr f, gaddr p, int yes) {
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (yes) f_0887A10C(c); else f_08879C8C(c);
}

/* Into the block one time in two. */
SFA_FUNC(08878B30, fighter_script_if_random) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    f_08821368(c);
    uint32_t heads = c->r[2] & 1;
    c->r[4] = f; c->r[5] = p;
    if (heads) f_08879C8C(c); else f_0887A10C(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* Into the block while the opponent's +73 is 1 / while the opponent's +698 is set. */
SFA_FUNC(08878C24, fighter_script_if_opponent_73) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    c->r[7] = p;
    script_branch(c, f, p, S8(*W(f + 92) + 73) == 1);
    c->r[29] += 0x20;
}
SFA_FUNC(08878CD8, fighter_script_if_opponent_698) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    c->r[7] = p;
    script_branch(c, f, p, S8(*W(f + 92) + 698) != 0);
    c->r[29] += 0x20;
}

/* Into the block when the routine by layout (0x08876BE4) says yes. */
SFA_FUNC(08878EEC, fighter_script_if_layout) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    f_08876BE4(c);
    uint32_t yes = c->r[2];
    c->r[4] = f; c->r[5] = p;
    if (yes) f_0887A10C(c); else f_08879C8C(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, script): this script kept to come back to (+504 its first word, +512 where), and one
   picked at random read instead. */
SFA_FUNC(08879E10, fighter_script_call_random) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *H(f + 504) = (uint16_t)S16(f + 486);
    *B(f + 484) = 1;
    *W(f + 512) = ARG(1);
    c->r[4] = f;
    f_0887C25C(c);
    c->r[4] = f; c->r[5] = c->r[2];
    f_0887A10C(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* A script picked at random (after one picked by level, the first time) and started: +528 its place,
   +488 its first word; answers the word after. */
SFA_FUNC(0887A188, fighter_start_random_script) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (*B(f + 484) == 0) { c->r[4] = f; f_0887B1B8(c); }
    *B(f + 484) = 1;
    c->r[4] = f;
    f_0887C25C(c);
    gaddr p = c->r[2];
    *W(f + 528) = p;
    c->r[4] = (uint32_t)(int32_t)S16(p);
    *H(f + 488) = (uint16_t)S16(p);
    RETURN(p + 2);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

void f_08821330(Cpu *c);
void f_08862F28(Cpu *c);
void f_088632C4(Cpu *c);
void f_08877BC4(Cpu *c);   /* fighter_cpu_wait_then_read */

/* Kept off the edges: past the left (+238 = 1) or the right (+238 = 2) by its width (+232, 24 more
   with 0x08A0588A) from the screen (0x08821330's x), the fighter is pushed back in and 0x08862F28 told
   how far. Pushed while +73 bit 7 sets +592. */
SFA_FUNC(08862DF8, fighter_keep_on_screen) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = f;
    int32_t margin = S8(0x08A0588Au) != 0 ? 0x18 : 0;
    c->r[18] = (uint32_t)margin;
    *B(f + 238) = 0;
    if (S8(f + 671) == 0) {
        gaddr opp = *W(f + 92);
        c->r[17] = opp;
        c->r[4] = f;
        f_08821330(c);
        int16_t x = S16(f + 18), width = S16(f + 232);
        int16_t d = (int16_t)((int16_t)(x - width - margin) - S16(c->r[2] + 18));
        int16_t by = 0;
        uint8_t edge = 0;
        if (d <= 0) { by = d; edge = 1; }
        else {
            int16_t e = (int16_t)((int16_t)(width * 2 + margin * 2 + d) - 0x180);
            if (e >= 0) { by = e; edge = 2; }
        }
        if (edge) {
            *H(f + 18) = (uint16_t)(x - by);
            *B(f + 238) = edge;
            c->r[4] = f; c->r[5] = opp; c->r[6] = (uint32_t)(int32_t)by;
            f_08862F28(c);
        }
    }
    if (S8(f + 238) != 0 && (S8(f + 73) & 0x80) && S8(f + 592) == 0) *B(f + 592) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (fighter, n): the speeds +96 - +108 from the record (0x08A182C0, 16 bytes) picked through two tables
   by +655, +575 and n; x ones turned round unless +134. */
SFA_FUNC(08863168, fighter_set_speeds) {
    gaddr f = ARG(0);
    uint32_t a = *B(0x08A180C8u + (uint32_t)(S8(f + 655) * 4 + S8(f + 575)));
    uint32_t k = *B(0x08A18218u + a * 2u + ARG(1));
    gaddr e = 0x08A182C0u + k * 16u;
    int keep = S8(f + 134) != 0;
    *W(f + 96)  = keep ? *W(e) : 0u - *W(e);
    *W(f + 104) = keep ? *W(e + 4) : 0u - *W(e + 4);
    *W(f + 100) = *W(e + 8);
    *W(f + 108) = *W(e + 12);
}

/* 1 once (+498 cleared) when nothing halts the fight, 0x088632C4 says no, +498 and +123 are set and
   the record at +296 is shown. */
SFA_FUNC(08876A0C, fighter_asks_08876A0C) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    int yes = 0;
    if ((S8(GAME + G_HALT_38) | S8(BATTLE + BT_FLAG_13) | S8(BATTLE + BT_FLAG_14)) == 0) {
        c->r[4] = f;
        f_088632C4(c);
        if (c->r[2] == 0 && S8(f + 498) != 0 && S8(f + 123) != 0 && *B(*W(f + 296) + 1) != 0) {
            *B(f + 498) = 0;
            yes = 1;
        }
    }
    RETURN(yes);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The question numbered by +492 (table 0x08A22178, by its value / 2) asked of the fighter: 0 with
   +788, or with +688 while less than 32 above +132, or for 0 or 1. */
SFA_FUNC(08876AA8, fighter_ask_by_492) {
    gaddr f = ARG(0);
    RETURN(0);
    if (*B(f + 788) != 0) return;
    if (S8(f + 688) != 0 && S16(f + 22) - S16(f + 132) < 32) return;
    uint32_t m = (uint32_t)(int32_t)S8(f + 492) & 0xFE;
    if (m == 0) return;
    gaddr fn = *W(0x08A22178u + (m >> 1) * 4u);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = c->r[31];
    c->r[4] = f; c->r[5] = fn;
    c->r[31] = 0x08876B3Cu;
    rt_call_indirect(c, fn);
    c->r[31] = *W(c->r[29] + 16);
    c->r[29] += 0x20;
}

/* (fighter, script): a first script waits (0x08877BC4); after that the wait set and the script read
   on, or (with +506) +482 = 0, +483 = 8 and 0x08877F44. */
SFA_FUNC(08877A38, fighter_cpu_wait_or_read) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    if (*B(f + 484) == 0) { c->r[4] = f; c->r[5] = p; f_08877BC4(c); }
    else {
        c->r[4] = f; c->r[5] = p;
        f_0887B48C(c);
        c->r[4] = f; c->r[5] = p;
        if (S16(f + 506) != 0) { *B(f + 482) = 0; *B(f + 483) = 8; f_08877F44(c); }
        else f_0887A10C(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, script): +492 = the script's word; into the block while it is at least +508. */
SFA_FUNC(08878A68, fighter_script_if_492_reached) {
    gaddr f = ARG(0), p = ARG(1);
    *H(f + 492) = (uint16_t)S16(p);
    c->r[29] -= 0x20;
    script_branch(c, f, p + 2, !(S16(f + 492) < S16(f + 508)));
    c->r[29] += 0x20;
}

/* With the opponent's health not below 0, by the script's command (+528's word, high byte): 3 yes; 4
   while +536 and +228 are clear; 5 for ISM other than 1 while +366 is at least 48 (ISM 0) or 144. */
SFA_FUNC(0887BD60, fighter_script_command_allowed) {
    gaddr f = ARG(0);
    RETURN(0);
    if (S16(*W(f + 92) + 112) < 0) return;
    uint32_t command = (uint32_t)(int32_t)S16(*W(f + 528)) & 0xFF00;
    if (command == 0x300) { RETURN(1); return; }
    if (command == 0x400) { RETURN(*B(f + 536) == 0 && *W(f + 228) == 0); return; }
    int8_t ism = S8(f + F_ISM);
    if (ism == 1 || command != 0x500) return;
    RETURN(S16(f + 366) >= (ism == 0 ? 48 : 144));
}

/* ---- 0.72.0 ---- */

void f_08877564(Cpu *c);

/* (fighter, out): the button pressed this frame as strength and kind: 0 - 2 (light, medium, heavy)
   and 0 punch or 1 kick; 0, 0 with none. */
SFA_FUNC(08863A9C, fighter_button_pressed) {
    uint16_t b = *H(ARG(0) + 760);
    uint8_t strength = 0, kind = 0;
    if (b & 0x10) { }
    else if (b & 0x20)  strength = 1;
    else if (b & 0x40)  strength = 2;
    else if (b & 0x80)  kind = 1;
    else if (b & 0x100) { strength = 1; kind = 1; }
    else if (b & 0x200) { strength = 2; kind = 1; }
    *B(ARG(1)) = strength;
    *B(ARG(1) + 1) = kind;
}

/* A fighter's move state cleared: its speeds and many flags, +574 of all four fighters, and the
   opponent's +586 / +587. */
SFA_FUNC(088647C4, fighter_clear_move_state) {
    gaddr f = ARG(0), opp = *W(f + 92);
    static const uint16_t bytes[] = { 502, 541, 235, 1146, 768, 713, 310, 260, 693, 562, 671, 596, 689, 688,
                                      281, 282, 591, 668, 566, 644, 573, 270, 777 };
    *H(f + 584) = 0;
    for (int i = 0; i < 4; i++) *B(FIGHTER_1 + 0x498u * (uint32_t)i + 574) = 0;
    *W(f + 108) = 0; *W(f + 100) = 0; *W(f + 104) = 0; *W(f + 96) = 0;
    *W(f + 816) = 0;
    for (size_t i = 0; i < sizeof bytes / sizeof *bytes; i++) *B(f + bytes[i]) = 0;
    *B(opp + 587) = 0; *B(opp + 586) = 0;
}

/* An entry of the fighter's table of values (the shared 0x08A1FFEE with +792's bits 0, 20, 21, else
   the character's from 0x08A2002C, or 0x08A20006 for state 2 with +327): row by +192 (0 - 1, 6, or 12),
   3 more with +193, column +194. */
SFA_FUNC(0886BCB8, fighter_value_by_state) {
    gaddr f = ARG(0);
    gaddr table = (*W(f + 792) & 0x300001u) ? 0x08A1FFEEu : *W(0x08A2002Cu + (uint32_t)(S8(f + 338) << 2));
    int8_t state = S8(f + 192);
    int32_t row;
    if (state < 2) row = state <= 0 ? 0 : 6;
    else if (state >= 3) row = 0;
    else { row = 0xC; if (S8(f + 327) != 0) table = 0x08A20006u; }
    if (S8(f + 193) != 0) row = (int16_t)(row + 3);
    int32_t at = (int16_t)(row + S8(f + 194));
    RETURN(*H(table + (uint32_t)(at * 2)));
}

/* The next script command read (+488, a new script picked first for -1) and run by 0x08877564; where
   it left off kept in +528 and, by the nesting (+484, 0 - 3), in +512 - +524. */
SFA_FUNC(088774A8, fighter_script_step) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    gaddr p = *W(f + 528);
    *H(f + 488) = (uint16_t)S16(p);
    *H(0x08B0268Eu) = 1;
    p += 2;
    if (S8(f + 489) == -1) { c->r[4] = f; c->r[5] = p; f_0887A154(c); p = c->r[2]; }
    c->r[4] = f; c->r[5] = p;
    f_08877564(c);
    gaddr at = c->r[2];
    *W(f + 528) = at;
    uint8_t depth = *B(f + 484);
    if (depth < 4) *W(f + 512 + depth * 4u) = at;
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* ---- 0.73.0 ---- */

void f_08863A9C(Cpu *c);   /* fighter_button_pressed */
void f_088774A8(Cpu *c);   /* fighter_script_step */
void f_0887A400(Cpu *c);
void f_08877258(Cpu *c);
void f_08879E10(Cpu *c);   /* fighter_script_call_random */

/* A human fighter's button just pressed kept as +194 (strength) and +193 (kind), its held directions
   in +290 (the pair is read into the guest stack, as the original does). */
SFA_FUNC(08863A50, fighter_keep_button) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (S8(f + 373) == 0) {
        gaddr pair = c->r[29] + 16;
        c->r[4] = f; c->r[5] = pair;
        f_08863A9C(c);
        *B(f + 194) = *B(pair);
        *B(f + 193) = *B(pair + 1);
        *H(f + 290) = *H(f + 752);
    }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The CPU's first frame: a script picked (by level, or at random), then the first command run from
   the script kept for the nesting (+512 or +516); then 0x0887A400. */
static void cpu_first_frame(Cpu *c, GuestFunc pick, uint32_t kept) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (*B(f + 480) == 0) { *B(f + 480) = 1; c->r[4] = f; pick(c); }
    if (*B(f + 481) == 0) {
        *B(f + 481) = 1;
        *W(f + 528) = *W(f + kept);
        c->r[4] = f; f_088774A8(c);
    }
    c->r[4] = f; f_0887A400(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(08877328, fighter_cpu_frame_by_level)  { cpu_first_frame(c, f_0887B1B8, 512); }
SFA_FUNC(08877388, fighter_cpu_frame_at_random) { cpu_first_frame(c, f_0887C25C, 516); }

/* (fighter, script): a script called at random when 0x08877258 (with the list 0x08A22B20) says yes,
   else the script read on. */
SFA_FUNC(08879E58, fighter_script_maybe_call_random) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    c->r[5] = 0x08A22B20u;
    f_08877258(c);
    uint32_t yes = c->r[2];
    c->r[4] = f; c->r[5] = p;
    if (yes) f_08879E10(c); else f_0887A10C(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, script): +490 = the script's word; within 2 of +508 the script is read on, otherwise the
   fighter is set to routine 1.0.2.0 (+482 = 1, +483 = 2), with +497 and +78 saying whether +490
   reached +508. */
SFA_FUNC(088780A0, fighter_script_if_near_508) {
    gaddr f = ARG(0), p = ARG(1);
    int16_t word = S16(p);
    int32_t target = S16(f + 508);
    *H(f + 490) = (uint16_t)word;
    RETURN(p + 2);
    uint32_t apart = ((uint32_t)(uint16_t)word - (uint32_t)target) & 0xFFFF;
    if (((apart + 3) & 0xFFFF) < 6) {
        c->r[29] -= 0x20;
        c->r[4] = f; c->r[5] = p + 2;
        f_0887A10C(c);
        c->r[29] += 0x20;
        return;
    }
    *B(f + 482) = 1; *B(f + 483) = 2;
    *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
    uint8_t reached = !((int32_t)(uint16_t)S16(f + 490) < target);
    *B(f + 497) = reached; *B(f + 78) = reached;
}

/* ---- 0.74.0 ---- */

void f_08879E58(Cpu *c);   /* fighter_script_maybe_call_random */
void f_0886DC3C(Cpu *c);   /* fighter_clear_644 (hit.c) */
void f_088623BC(Cpu *c);   /* fighter_holds_down (buttons.c) */
void f_0885B7D4(Cpu *c);   /* fighter_tick_and_routine_64 */

/* (fighter, script): with a script command (+488) a script maybe called at random, otherwise one
   called at random. */
SFA_FUNC(08879DBC, fighter_script_call_by_command) {
    gaddr f = ARG(0), p = ARG(1);
    int8_t command = S8(f + 488);
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p;
    if (command != 0) f_08879E58(c); else f_08879E10(c);
    c->r[29] += 0x20;
}

/* (fighter): routine 1, 0, 5, 0; facing the opponent's side (+697); +644 cleared, +123 cleared, the
   button kept; +192 and +696 say whether down is held; then the frame's tick and routine 64. */
SFA_FUNC(088639CC, fighter_start_routine_1_5) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 5;
    int8_t side = S8(f + F_OPPONENT_SIDE);
    *B(f + 7) = 0; *B(f + 266) = 0;
    *B(f + F_FACING) = (uint8_t)side;
    c->r[4] = f; f_0886DC3C(c);
    *B(f + 123) = 0;
    c->r[4] = f; f_08863A50(c);
    c->r[4] = f; f_088623BC(c);
    uint8_t down = c->r[2] != 0;
    *B(f + 192) = down; *B(f + 696) = down;
    c->r[4] = f; f_0885B7D4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

void f_088639C0(Cpu *c);   /* fighter_pressed_attacks (buttons.c) */

/* (fighter): a human fighter (+373 clear) with +80 set and +262 clear, whose opponent still has
   health (+112 not negative), starts routine 1 / 5 when an attack button is pressed. */
SFA_FUNC(0886DB2C, fighter_attack_starts_1_5) {
    gaddr f = ARG(0), saved = c->r[16];
    if (S8(f + 373) != 0 || S8(f + 80) == 0 || S8(f + 262) != 0 || S16(*W(f + F_OPPONENT) + F_HEALTH) < 0) return;
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[4] = f; f_088639C0(c);
    if (c->r[2] != 0) { c->r[4] = f; f_088639CC(c); }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* ---- 0.75.0 ---- */

void f_0887E230(Cpu *c);   /* jittered_copy_16 (object/screen_steps.c) */
void f_0887E254(Cpu *c);   /* jittered_copy_17 */

/* (fighter): a jittered copy (+78 = 17 or 16) moved by the character's offset when one is made. */
static void offset_jittered_copy(Cpu *c, GuestFunc copy) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[4] = f; copy(c);
    gaddr o = c->r[2];
    c->r[5] = o;
    if (o != 0) { c->r[4] = f; f_08864D98(c); }
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(088638A8, fighter_jittered_copy_17) { offset_jittered_copy(c, f_0887E254); }
SFA_FUNC(088638E0, fighter_jittered_copy_16) { offset_jittered_copy(c, f_0887E230); }

/* (record, v): v by a setting (GAME + 116 when the record's +19 is 2, else GAME + 46): 0 takes an
   eighth off, 2 adds an eighth, 3 a quarter; anything else leaves it. */
SFA_FUNC(08873094, scale_by_setting) {
    int32_t v = (int32_t)ARG(1);
    int8_t level = S8(GAME + (*B(ARG(0) + 19) == 2 ? 116 : 46));
    switch (level) {
    case 0: v -= v >> 3; break;
    case 2: v += v >> 3; break;
    case 3: v += v >> 2; break;
    default: break;
    }
    RETURN(v);
}

/* (fighter): the script read on from the start: with no nesting (+484) a script picked; otherwise the
   one left off at the first level (+504 into +486, +512). Its first word is the command (+488); a
   command of -1 (+489) runs at once, on the word after it for a picked script or on a newly picked
   script for one left off. Answers where it reads next. */
SFA_FUNC(0887A1D8, fighter_script_restart) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = 0xFFFFFFFFu;
    if (*B(f + 484) == 0) {
        *B(f + 484) = 0;
        c->r[4] = f; f_0887B1B8(c);
        gaddr p = c->r[2];
        *W(f + 528) = p;
        *H(f + 488) = (uint16_t)S16(p);
        RETURN(p + 2);
        if (S8(f + 489) == -1) { c->r[4] = f; c->r[5] = p + 2; f_0887A154(c); }
    } else {
        int16_t at = S16(f + 504);
        *B(f + 484) = 0;
        gaddr p = *W(f + 512);
        *H(f + 486) = (uint16_t)at;
        *W(f + 528) = p;
        *H(f + 488) = (uint16_t)S16(p);
        RETURN(p + 2);
        if (S8(f + 489) == -1) {
            c->r[4] = f; f_0887B1B8(c);
            c->r[4] = f; c->r[5] = c->r[2]; f_0887A154(c);
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): a CPU fighter of characters 6, 15 or 20, not in +702 / +568, with health, not ISM -1, with
   neither BATTLE + 13 / + 14 nor GAME + 38 set and not both fighters in step 6, answers the chance by
   its level from the list 0x08A22CA0; otherwise 0. */
SFA_FUNC(0885E9A4, fighter_cpu_chance_by_character) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    uint32_t answer = 0;
    int8_t ch = S8(f + 338);
    if (S8(f + 373) != 0 && (ch == 6 || ch == 20 || ch == 15) && S8(f + 702) == 0 && S8(f + 568) == 0 &&
        S16(f + F_HEALTH) >= 0 && S8(f + F_ISM) != -1 &&
        (S8(BATTLE + BT_FLAG_13) | S8(BATTLE + BT_FLAG_14) | S8(GAME + G_HALT_38)) == 0) {
        c->r[29] -= 0x20;
        c->r[16] = f; c->r[17] = 0;
        c->r[4] = f; f_088632C4(c);
        if (c->r[2] == 0) { c->r[4] = f; c->r[5] = 0x08A22CA0u; f_08877258(c); answer = c->r[2]; }
        c->r[29] += 0x20;
        c->r[16] = s16; c->r[17] = s17;
    }
    RETURN(answer);
}

/* ---- 0.76.0 ---- */

void f_08821368(Cpu *c);   /* a random number */

/* (fighter): with +117 = 5, a jittered copy (+78 16 or 17, at random). */
SFA_FUNC(08863850, fighter_jittered_copy_by_117) {
    gaddr f = ARG(0), saved = c->r[16];
    if (*B(f + 117) != 5) return;
    c->r[29] -= 0x20;
    c->r[16] = f;
    f_08821368(c);
    c->r[4] = f;
    if (c->r[2] & 1) f_088638E0(c); else f_088638A8(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (attacker, defender, record, v): v adjusted by both fighters' values: the attacker's +796 (+798 when
   the record's +19 is 2; 100 when its +792 has bit 25) and +802, and the defender's +800 (75 is added
   instead when its +792 has bit 25) and +802 taken away, each counted from 100; v grows or shrinks by
   that many 200ths, and is at least 1. Answered as it is for v = 0 or the defender's +117 = 0xFF. */
SFA_FUNC(08872F68, damage_by_values) {
    gaddr a = ARG(0), d = ARG(1), r = ARG(2);
    int32_t v = (int32_t)ARG(3);
    if (v == 0 || *B(d + 117) == 0xFF) { RETURN(v); return; }
    int32_t k;
    if (*W(a + 792) & 0x2000000u) k = 100;
    else k = (*B(r + 19) == 2 ? S16(a + 798) : S16(a + 796)) - 100;
    k += S16(a + 802) - 100;
    if (*W(d + 792) & 0x2000000u) k += 75;
    else k = k - S16(d + 800) + 100;
    k = k - S16(d + 802) + 100;
    if (k > 0)      { DIV(c, (uint32_t)v * (uint32_t)k, 200);  v += (int32_t)c->lo; }
    else if (k < 0) { DIV(c, (uint32_t)v * (uint32_t)-k, 200); v -= (int32_t)c->lo; }
    if (v <= 0) v = 1;
    RETURN(v);
}

/* (record): +126 counts up to 99, and the highest count is kept in +357 of the hit record's fighter
   (in the single layout, 0) and of the pair it belongs to: fighters 1 and 2 in layout 3 unless it is on
   side 2 or 3, otherwise fighters 3 and 4 when it is. */
static void keep_highest(gaddr f, int8_t n) { if (S8(f + 357) < n) *B(f + 357) = (uint8_t)n; }
SFA_FUNC(0887335C, count_126_keep_highest) {
    gaddr r = ARG(0);
    int8_t n = S8(r + 126);
    if (n == 99) return;
    *B(r + 126) = (uint8_t)(n + 1);
    gaddr f = *W(0x09358410u + 24);
    if (S16(BATTLE + BT_LAYOUT) == 0) keep_highest(f, S8(r + 126));
    int second_pair = S8(f + F_SIDE) & 2;
    if (S16(BATTLE + BT_LAYOUT) == 3) {
        if (second_pair) return;
        keep_highest(FIGHTER_1, S8(r + 126)); keep_highest(FIGHTER_2, S8(r + 126));
    } else {
        if (!second_pair) return;
        keep_highest(FIGHTER_3, S8(r + 126)); keep_highest(FIGHTER_4, S8(r + 126));
    }
}

/* Every fighter in play (by the layout: 0 the first two, 1 the first and the pair 3 / 4, 2 the second
   and 3 / 4, 3 the first three) notes its opponent's side. */
SFA_FUNC(088592E4, fighters_note_opponent_side) {
    static const gaddr none[] = { 0 };
    static const gaddr l0[] = { FIGHTER_1, FIGHTER_2, 0 }, l1[] = { FIGHTER_1, FIGHTER_3, FIGHTER_4, 0 },
                       l2[] = { FIGHTER_2, FIGHTER_3, FIGHTER_4, 0 }, l3[] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, 0 };
    int16_t layout = S16(BATTLE + BT_LAYOUT);
    const gaddr *list = layout == 0 ? l0 : layout == 1 ? l1 : layout == 2 ? l2 : layout == 3 ? l3 : none;
    c->r[29] -= 0x20;
    for (; *list; list++) { c->r[4] = *list; f_088593D8(c); }
    c->r[29] += 0x20;
}

/* (fighter, hit): a gauge (+700) filled by the hit's +12 (a quarter more with +588; 20 or 30 when the
   hit record's fighter's +810 is 2 or 3), unless +810 is 7. Reaching +701 sets +702, empties the
   gauge and +699, and (unless +810 is 8) raises +701 by 5, never past 160. Nothing for a fighter out
   of health, with +637, +792 bit 11 or character 0x24. */
SFA_FUNC(08872080, fighter_fill_gauge_700) {
    gaddr f = ARG(0), hit = ARG(1), other = *W(0x09358410u + 24);
    if (S16(f + F_HEALTH) < 0 || S8(f + 637) != 0 || (*W(f + 792) & 0x800) || S8(f + 338) == 0x24) return;
    int8_t add = (int8_t)*B(hit + 12);
    if (S8(f + 588) != 0) add = (int8_t)(add + (add >> 2));
    int8_t kind = S8(other + 810);
    if (kind == 2) add = 20; else if (kind == 3) add = 30;
    int8_t own = S8(f + 810);
    uint8_t limit = *B(f + 701);
    if (own != 7) *B(f + 700) = (uint8_t)(*B(f + 700) + add);
    if (*B(f + 700) < limit) return;
    *B(f + 702) = 1; *B(f + 700) = 0; *B(f + 699) = 0;
    if (own != 8) { *B(f + 701) = (uint8_t)(limit + 5); limit = *B(f + 701); }
    if (limit >= 160) *B(f + 701) = 0xA0;
}

/* ---- 0.77.0 ---- */

/* (fighter): its speeds from the character's table (+708: 16 s16 a character): x and y speed, then x
   and y acceleration (+96 - +108, each << 8), from row 0 / 8 / 12 by what it is doing (a CPU by +497:
   none, 8, or 12 with bit 7; a human by its held direction: none, 12 for forward, 8 for back). ISM -1
   characters 17 and 19 take their x acceleration unshifted. Facing left (+11) turns the x values. */
SFA_FUNC(08862068, fighter_speeds_by_direction) {
    gaddr f = ARG(0);
    int8_t ch = S8(f + 338);
    uint32_t row;
    if (S8(f + 373) != 0) {
        int8_t v = S8(f + 497);
        row = v == 0 ? 0 : (v & 0x80) ? 12 : 8;
    } else {
        uint16_t d = *H(f + 752);
        row = (d & 3) == 0 ? 0 : (d & 2) ? 8 : 12;
    }
    gaddr p = *W(f + 708) + ((uint32_t)((int32_t)ch << 4) + row) * 2u;
    *W(f + 96) = (uint32_t)(int32_t)S16(p) << 8;
    *W(f + 100) = (uint32_t)(int32_t)S16(p + 2) << 8;
    int unshifted = S8(f + F_ISM) == -1 && (ch == 0x11 || ch == 0x13);
    *W(f + 104) = unshifted ? (uint32_t)(int32_t)S16(p + 4) : (uint32_t)(int32_t)S16(p + 4) << 8;
    *W(f + 108) = (uint32_t)(int32_t)S16(p + 6) << 8;
    if (*B(f + F_FACING) != 0) {
        *W(f + 96) = 0u - *W(f + 96);
        *W(f + 104) = 0u - *W(f + 104);
    }
}

/* (fighter): the character's routine (0x08A19040), then three lists of { index, value } pairs ending
   at -1 (routine table + 116, + 112, + 120) written into the word tables 0x08A10040, 0x08A10130 and
   0x08A0FDC8. */
static void write_pairs(gaddr list, gaddr table) {
    for (gaddr e = list; *W(e) != 0xFFFFFFFFu; e += 8) *W(table + *W(e) * 4u) = *W(e + 4);
}
SFA_FUNC(088597F0, fighter_character_values) {
    gaddr f = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    gaddr fn = *W(0x08A19040u + ((uint32_t)(int32_t)S8(f + 338) << 2));
    c->r[4] = f; c->r[5] = fn;
    rt_call_indirect(c, fn);
    write_pairs(*W(*W(f + 416) + 116), 0x08A10040u);
    write_pairs(*W(*W(f + 416) + 112), 0x08A10130u);
    write_pairs(*W(*W(f + 416) + 120), 0x08A0FDC8u);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (fighter): a CPU script picked by the opponent's +307: from the list at routine table + 164 in a
   network battle with +815, else + 56 (shifted by +499); one of 32 random choices (s8) names the script
   in the entry's second list. +511 = the choice, +486 = its first word, +520 where it reads on, +550 /
   +565 / +501 cleared. Answers +520. */
SFA_FUNC(0887B308, fighter_pick_script_by_opponent) {
    gaddr f = ARG(0), opp = *W(f + 92);
    gaddr s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = opp;
    *B(f + 550) = 0;
    f_088D5E44(c);
    int net = c->r[2] != 0;
    gaddr table = *W(f + 416);
    int by_network = net && S8(f + 815) != 0;
    gaddr lists = *W(table + (by_network ? 164 : 56));
    gaddr choices = *W(lists + *B(opp + 307) * 8u);
    c->r[18] = lists; c->r[19] = choices;
    f_08821368(c);
    uint32_t r = c->r[2] & 0x1F;
    int8_t pick = by_network ? S8(choices + r)
                             : S8(choices + (uint32_t)(int32_t)(int8_t)(r + (uint32_t)(int32_t)S8(f + 499)));
    gaddr p = *W(*W(lists + *B(opp + 307) * 8u + 4) + ((uint32_t)(int32_t)pick << 2));
    *B(f + 511) = (uint8_t)pick;
    *W(f + 520) = p + 2;
    *H(f + 486) = (uint16_t)S16(p);
    *B(f + 565) = 0; *B(f + 501) = 0;
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
    RETURN(p + 2);
}

void f_08805310(Cpu *c);   /* does nothing (empty.c) */

/* (fighter, kind, length): for a human fighter (in Training: only the side GAME + 57 names) 0x08805310
   is called with its pad slot (+775), the two bytes for `kind` at 0x08A1BFD8 and the length. Nothing
   for a length of 0. */
SFA_FUNC(0886E3CC, fighter_call_08805310_by_kind) {
    gaddr f = ARG(0);
    uint32_t kind = ARG(1) & 0xFF, length = ARG(2) & 0xFFFF;
    if (length == 0) return;
    int call;
    if (S8(GAME + 114) == 2) call = (S8(GAME + 57) == 1) ? S8(f + 337) == 0 : S8(f + 337) != 0;
    else call = S8(f + 373) == 0;
    if (!call) return;
    gaddr e = 0x08A1BFD8u + kind * 2u;
    c->r[29] -= 0x20;
    c->r[4] = (uint32_t)(int32_t)S8(f + 775) & 0xFF;
    c->r[5] = *B(e);
    c->r[6] = (uint32_t)(int32_t)S8(e + 1);
    c->r[7] = length;
    f_08805310(c);
    c->r[29] += 0x20;
}
