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
