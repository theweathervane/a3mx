/* The fighting modes' two tasks, started by the MODE MENU's dispatch (mode_menu.c):
 *
 *   task 0, the fight loop (0x088233EC)   every frame: the game's frame counter and frame skip (the
 *                                          game speed), then the steps of the fight itself, the network,
 *                                          the stage's effects and the screen; a frame the speed skips
 *                                          goes round again without waiting
 *   task 2, the battle task (0x08833B6C)  every frame: each CPU fighter's routine, then the CPU's
 *                                          common step (or, with two players, Free Battle's or
 *                                          Survival's own step)
 *
 * The state they share is BATTLE (0x092E1228, the stage number at +0): +22 the game speed (0 - 15),
 * +23 this frame is skipped (s8, -1), +45 the speed applies, +52 a skipped frame was just done.
 */
#include <string.h>
#include "../decomp.h"
#include "task.h"

#define GAME           0x08E75910u
#define BATTLE         0x092E1228u
#define FIGHTER_1      0x092E3EA0u     /* fighter structs, 0x498 bytes each */
#define FIGHTER_2      0x092E4338u
#define FIGHTER_3      0x092E47D0u
#define FIGHTER_4      0x092E4C68u
#define SPEED_FRAMES   0x08A0E09Cu     /* u32[16]: by game speed, the frames (bit = frame & 31) that are skipped */
#define CPU_ROUTINES   0x08A0F8B0u     /* by fighter + 340: a CPU fighter's routine (game, battle, fighter) */
#define NET            0x09523920u     /* the network's state (+0x25 up, +0x30 in a session) */
#define NET_STEP       0x09523960u     /* +0 running, +22 its step */
#define NET_STEPS      0x08A3EC30u
#define NET_TEXT       0x09523980u     /* +8 text to draw, +60 its flag */
#define STAGE_FX       0x092E5100u     /* the stage's effect: +0 on, +2 which */
#define STAGE_FX_STEPS 0x08A257ACu
#define SCREEN_STEPS   0x08A0E0DCu     /* the fight's screens after 0: (game, battle, the routine) */
#define SCREEN_2_STEPS 0x08A0E0F8u     /* by GAME + 4 */
#define SCREEN_9_STEPS 0x08A0E180u     /* by GAME + 6 */
#define SCREEN_10_STEPS 0x08A0E190u    /* by GAME + 6 */
#define SCREEN_11_STEPS 0x08A0E2E0u    /* by GAME + 6 */
#define PADS           0x08E75BA0u     /* the players' pads, 44 bytes each: +4 held, +12 pressed */
#define PAD_SLOTS      0x08A0E024u     /* s8[4] by BATTLE + 154: each fighter's pad slot (-1 none) */
#define PAD_HELD       0x08A0E074u     /* by slot: a pointer to that pad's held buttons */
#define SYS_OPTS       0x08A058A0u     /* system data (sysdata.c) */
#define SPEEDS         0x08A0E748u     /* u8 by GAME + 0x30: the game speed */
#define ROUND_TIMES    0x08A0F9E0u     /* s16 by GAME + 0x49 */
#define TIMER_DISPLAY  0x09524A68u     /* u16 x 3 */
#define FIGHT_SCRATCH  0x09358410u
#define PARTY_1        0x09517A28u     /* player 1's party of three (World Tour and the like) */
#define PARTY_2        0x09517A50u
#define WORLD_TOUR     0x09517A78u     /* World Tour's state: +904 the fight's kind, +905, +906, +908, +920 */
#define ROUND_PICK     0x09523C98u     /* s8: GAME + 0x2F kept for the round */
#define WT_EXTRA       0x095179E4u     /* u32 */
#define ROUND_START_T  0x08A0E098u     /* s8 by GAME + 0x2F: BATTLE + 9 */
#define MODE_6_STAGES  0x08A0E750u     /* u8[37]: mode 6 picks one at random */
#define ROUND_KINDS    0x08A0E10Cu     /* by BATTLE + 59: the round's own step (game, battle) */
#define CPU_LABELS     0x08A0F858u     /* by fighter + 412: { s16 x offset, char *text } */
#define SCREEN_1_STEPS 0x08A27128u     /* by GAME + 4: (game, battle) */
#define SCREEN_3_STEPS 0x08A0EEE4u     /* by GAME + 4 */
#define SCREEN_4_STEPS 0x08A0EEF8u     /* by GAME + 4 */
#define ROUND_BEHAVIOUR 0x08A0F434u    /* s8 by character + 42 · group: a fighter's round behaviour (0: keep) */

/* game state fields used here (GAME + n) */
#define G_FRAME        0x20   /* u8: counts the fight's frames (the speed's bit) */
#define G_FRAME_DONE   0x24   /* s8: -1 at a frame's start; 0 asks for another pass before waiting */
#define G_FADE_TASK    0x5D   /* s8 (fade.c) */
#define G_HALT         0x59   /* s8: the fight is stopped (pause and the like): frames are not counted */
#define G_HALT_2       0x8A   /* s8 */
#define G_HALT_3       0x91   /* s8 */
#define G_TWO_PLAYERS  0x88   /* s8: no CPU fighter */
#define G_MODE         0x72   /* s8 */
#define G_CHOSEN_BY    0x35   /* s8: the players' bits */

/* BATTLE fields */
#define B_SPEED        22     /* s8, & 15 */
#define B_SKIP         23     /* s8 */
#define B_SPEED_ON     45     /* s8 */
#define B_SKIPPED      52     /* s8 */

/* fighter fields */
#define F_HUMAN        336    /* s8: a player plays it */
#define F_SLOT         337    /* s8: its player slot (bit in G_CHOSEN_BY) */
#define F_CPU_ROUTINE  340    /* s8 */

#define MODE_TRAINING     2
#define MODE_6            6
#define MODE_SURVIVAL     7
#define MODE_DRAMATIC     8
#define MODE_NETWORK      12
#define MODE_VARIABLE     15
#define MODE_FINAL_BATTLE 9
#define MODE_13           13
#define MODE_FREE_BATTLE  16
#define MODE_17           17
#define MODE_18           18
#define MODE_19           19

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t S8(gaddr a) { return (int8_t)*B(a); }

void f_088054F0(Cpu *c);   /* task.c: wait for frames */
void f_08805564(Cpu *c);   /* task.c: kill a task (slot) */
void f_0880A39C(Cpu *c);   /* sprites: clear */
void f_088E30FC(Cpu *c);   /* the network, at a frame's start (game) */
void f_088E3550(Cpu *c);   /* below: the network is up */
void f_088235C0(Cpu *c);   /* below: frame skip */
void f_08820C94(Cpu *c);   /* by mode: the frame's mode-specific step */
void f_088D7FF4(Cpu *c);   /* (game, battle) */
void f_088DBC7C(Cpu *c);   /* (game, battle) */
void f_088E3FF0(Cpu *c);   /* below: the network's step (battle) */
void f_088859C8(Cpu *c);   /* below: the stage's effect */
void f_08823674(Cpu *c);   /* below: the fight's frame (game, battle) */
void f_088D80D8(Cpu *c);   /* below: the network's text */
void f_08828C40(Cpu *c);   /* below: the fight's set-up, screen 0 (game, battle) */
void f_088238B4(Cpu *c);   /* below: the fight's frame, other screens (game, battle) */
void f_08821FEC(Cpu *c);   /* game state: defaults */
void f_08821368(Cpu *c);   /* below: a random number */
void f_08820BE0(Cpu *c);   /* the random number generator (a Mersenne Twister: 624 words at 0x08B01564) */
void f_088236C8(Cpu *c); void f_08823844(Cpu *c); void f_08836538(Cpu *c);   /* below */
void f_088D5E44(Cpu *c);   /* below: a network battle */
void f_08892018(Cpu *c);   /* empty */
void f_088237DC(Cpu *c);   /* (game, battle): the partner set-up of the 2-on-1 modes */
void f_08812520(Cpu *c);   /* text pass (hooked by selectscreen.c) */
void f_0880F278(Cpu *c);
void f_08833BD4(Cpu *c);   /* below: a CPU fighter's routine */
void f_088347DC(Cpu *c);   /* below: the CPU's common step */
void f_08820DE8(Cpu *c);   /* the fighters' step, in the order given (4 fighters) */
void f_08834888(Cpu *c);   /* (fighter, ?, 0) */
void f_08832678(Cpu *c);   /* Free Battle's own step (game, battle) */
void f_08832898(Cpu *c);   /* Survival's own step (game, battle) */
void f_08823970(Cpu *c); void f_0882BEA0(Cpu *c); void f_0883087C(Cpu *c);   /* screen 0, by mode */
void f_088938C8(Cpu *c);   /* below: screen 1 */
void f_088E2304(Cpu *c); void f_088A0664(Cpu *c);
void f_08821FC8(Cpu *c); void f_0889562C(Cpu *c); void f_08895BD4(Cpu *c);
void f_088246DC(Cpu *c); void f_08832BB0(Cpu *c); void f_0882485C(Cpu *c);
void f_0882CD50(Cpu *c); void f_0882D254(Cpu *c); void f_088E9DD8(Cpu *c);
void f_0882D288(Cpu *c); void f_0882B204(Cpu *c);                       /* screen 3, by mode */
void f_0882B5FC(Cpu *c);   /* screen 4 */
void f_08814320(Cpu *c);   /* sound: stop */
void f_088325DC(Cpu *c);   /* mode_menu.c: back to the MODE MENU */
void f_08837690(Cpu *c); void f_088379B8(Cpu *c); void f_0883771C(Cpu *c); void f_08886E18(Cpu *c);
void f_08887AFC(Cpu *c); void f_0881DF10(Cpu *c);
void f_0880B6CC(Cpu *c);   /* text: flush */
void f_08823BEC(Cpu *c); void f_08823D08(Cpu *c); void f_088303B8(Cpu *c);   /* below (game, battle) */
void f_0883206C(Cpu *c);   /* (game, battle) */
void f_08823C60(Cpu *c);   /* below: a fighter's round reset (game, battle, fighter) */
void f_08831558(Cpu *c);   /* below: a fighter's round behaviour (game, battle, fighter) */
void f_0880A6CC(Cpu *c);   /* text: draw (string, kind, x, y, colour) */
void f_0884A07C(Cpu *c);   /* Training's round set-up */
void f_088D7C1C(Cpu *c);   /* a continuing match (nonzero) */
void f_088365A0(Cpu *c); void f_08821F9C(Cpu *c); void f_08819144(Cpu *c); void f_088D6438(Cpu *c);
void f_0881CE4C(Cpu *c);   /* (stage music?, 1) */
void f_088DC018(Cpu *c);   /* the network's next message (0: none) */
void f_088D5680(Cpu *c);   /* (network state, message) */
void rt_call_indirect(Cpu *c, uint32_t addr);

/* ---------------------------------------------------------------- small steps */

/* The network is up: in a session, or connected. */
static int network_active(void) {
    return S8(NET + 0x30) != 0 || S8(NET + 0x25) != 0;
}

/* Frame skip: a skipped frame is done (B_SKIPPED -1, the flag cleared); otherwise, with the speed on,
   this frame is skipped if the speed's table says so for the frame counter. */
static void frame_skip(gaddr g, gaddr battle) {
    *B(battle + B_SKIPPED) = 0;
    if (S8(battle + B_SKIP) != 0) {
        *B(battle + B_SKIPPED) = 0xFF;
        *B(battle + B_SKIP) = 0;
        return;
    }
    if (S8(battle + B_SPEED_ON) == 0) return;
    uint32_t frames = W(SPEED_FRAMES)[S8(battle + B_SPEED) & 15];
    if (frames & (1u << (*B(g + G_FRAME) & 31))) *B(battle + B_SKIP) = 0xFF;
}

/* The fight's frame: one routine on screen 0, another on the rest. */
static void fight_frame(Cpu *c, gaddr g, gaddr battle) {
    CALL3(*H(g) == 0 ? f_08828C40 : f_088238B4, g, battle, battle);
}

/* The network's text, once it has some to show. */
static void network_text(Cpu *c) {
    if (S8(NET_TEXT + 60) == 0 || *W(NET_TEXT + 8) == 0) return;
    CALL1(f_08812520, 0);
    CALL0(f_0880F278);
    *B(NET_TEXT + 60) = 0;
}

/* The network's step, unless this frame is skipped. */
static void network_step(Cpu *c, gaddr battle) {
    if (*W(NET_STEP) == 0 || S8(battle + B_SKIP) != 0) return;
    gaddr fn = W(NET_STEPS)[*B(NET_STEP + 22)];
    c->r[4] = NET_STEP; c->r[5] = NET_STEP;
    rt_call_indirect(c, fn);
}

/* The stage's effect (flashes and the like), while one runs. */
static void stage_effect(Cpu *c) {
    if (*B(STAGE_FX) == 0) return;
    gaddr fn = W(STAGE_FX_STEPS)[*B(STAGE_FX + 2)];
    c->r[4] = STAGE_FX; c->r[5] = fn;
    rt_call_indirect(c, fn);
}

/* The fighters' pads: each fighter's slot (+775) from the arrangement in BATTLE + 154, then for the
   first three their held buttons, last frame's (+364) and this frame's (+362). */
static void fighters_pads(gaddr f0, gaddr f1, gaddr f2, gaddr f3) {
    const gaddr f[4] = { f0, f1, f2, f3 };
    for (int k = 0; k < 4; k++)
        *B(f[k] + 775) = (uint8_t)S8(PAD_SLOTS + 4 * S8(BATTLE + 154) + k);
    for (int k = 0; k < 3; k++) *H(f[k] + 364) = *H(f[k] + 362);
    for (int k = 0; k < 3; k++) *H(f[k] + 362) = (uint16_t)*W(W(PAD_HELD + 4 * S8(f[k] + 775))[0]);
}

/* A network battle's input step, unless the frame is skipped: with no message pending, the next one
   is handled; with one, the local pads are emptied (a Cancel press kept in +33). Then, but for the
   state of BATTLE + 155 with GAME + 0x91 clear, ... (0x08832678); +60 set either way. */
static void network_input(Cpu *c, gaddr g, gaddr battle) {
    if (*B(NET_TEXT + 34) == 0 || S8(battle + B_SKIP) != 0) return;
    if (*W(NET_TEXT + 8) == 0) {
        uint32_t msg = CALL0(f_088DC018);
        if (msg != 0) CALL2(f_088D5680, NET_TEXT, msg);
    } else {
        uint32_t any = ((*W(PADS + 12) | *W(PADS + 56) | *W(PADS + 100)) & 0x100) != 0;
        *B(NET_TEXT + 33) = (uint8_t)any;
        *W(PADS + 48) = 0; *W(PADS + 4) = 0; *W(PADS + 56) = 0; *W(PADS + 12) = 0;
    }
    if (S8(battle + 155) != 0 && S8(g + G_HALT_3) == 0) CALL2(f_08832678, GAME, battle);
    *B(NET_TEXT + 60) = 1;
}

/* The fighters' step. Modes 13, 16 and 19 order the four fighters by BATTLE + 56 (s16): 0 and 3 as
   they are, 1 with the second last, 2 the second pair first and back to front; other values none.
   Every other mode runs them as they are. */
static void fighters_step(Cpu *c) {
    static const gaddr f[4] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, FIGHTER_4 };
    static const uint8_t order[4][4] = { { 0, 1, 2, 3 }, { 0, 2, 3, 1 }, { 2, 3, 1, 0 }, { 0, 1, 2, 3 } };
    int8_t mode = S8(GAME + G_MODE);
    int k = 0;
    if (mode == MODE_13 || mode == MODE_19 || mode == MODE_FREE_BATTLE) {
        k = (int16_t)*H(BATTLE + 56);
        if (k < 0 || k > 3) return;
    }
    const uint8_t *o = order[k];
    CALL4(f_08820DE8, f[o[0]], f[o[1]], f[o[2]], f[o[3]]);
}

/* The CPU's common step: with the CPU playing, for both fighters; otherwise Free Battle's step (in
   its state 2) or Survival's. */
static void cpu_step(Cpu *c) {
    const gaddr g = GAME;
    if (S8(g + G_TWO_PLAYERS) == 0) {
        CALL3(f_08834888, FIGHTER_1, 0x28, 0);
        CALL3(f_08834888, FIGHTER_2, 0x100, 0);
        return;
    }
    int8_t mode = S8(g + G_MODE);
    if (mode == MODE_FREE_BATTLE && S8(BATTLE + 159) == 2) CALL2(f_08832678, g, BATTLE);
    else if (mode == MODE_SURVIVAL) CALL2(f_08832898, g, BATTLE);
}

/* ---------------------------------------------------------------- the fight's set-up (screen 0) */

/* A random number (the generator's next). */
static uint32_t random_number(Cpu *c) { return CALL0(f_08820BE0); }

/* A network battle: in NETWORK, or a session's flag. */
static int network_battle(void) {
    return S8(GAME + G_MODE) == MODE_NETWORK || *B(NET_TEXT + 34) != 0;
}

/* A continuing match (0x088D7C1C): NETWORK, or a network session on (GAME + 146). */
static int continuing_match(void) {
    return S8(GAME + G_MODE) == MODE_NETWORK || S8(GAME + 146) != 0;
}

/* A fighter at the start of a fight, in player slot `slot` with its pad's held buttons; its party's
   three entries reset (player 1's party for slot 0, the other for the rest). */
static void fighter_start(gaddr f, uint32_t slot, uint16_t held) {
    static const uint16_t zero_b[] = { 438, 1172, 429, 432, 413, 431, 428, 385, 430, 450, 386, 361, 411, 407,
                                       451, 435, 434, 433, 436, 437, 78 };
    *B(f) = 1;
    *B(f + 337) = (uint8_t)slot;
    *B(f + 2) = (uint8_t)slot;
    *B(f + 373) = 1;
    for (size_t i = 0; i < sizeof zero_b / sizeof zero_b[0]; i++) *B(f + zero_b[i]) = 0;
    *H(f + 404) = 0; *W(f + 400) = 0; *H(f + 424) = 0; *H(f + 426) = 0;
    *B(f + 360) = 1; *B(f + 376) = 2; *B(f + 449) = 2;
    *H(f + 420) = 0x90; *H(f + 422) = 0;
    *H(f + 362) = held;
    uint8_t slot_byte = (uint8_t)slot;
    gaddr party = slot_byte == 0 ? PARTY_1 : PARTY_2;
    for (uint32_t i = 0; i < 3; i++) {
        *B(party + i + 3) = slot_byte;
        *B(party + i + 6) = 0;
        *B(party + i + 15) = 0;
        *B(party + i + 9) = 2;
        *B(party + i + 12) = 0;
    }
}

/* In the 2-on-1 modes (BATTLE + 90): a few of both fighters' fields cleared, then the partner's
   set-up. */
static void pair_start(Cpu *c, gaddr g, gaddr battle, gaddr f1, gaddr f2) {
    if (S8(battle + 90) == 0) return;
    static const uint16_t off[] = { 432, 413, 431, 428, 385 };
    for (int i = 0; i < 5; i++) *B(f2 + off[i]) = 0;
    for (int i = 0; i < 5; i++) *B(f1 + off[i]) = 0;
    *B(battle + 79) = 0;
    *B(battle + 67) = 0;
    CALL4(f_088237DC, g, battle, battle, g);
}

/* The round's time, from the time setting (GAME + 0x49), and the timer's display. */
static void round_time(void) {
    *H(BATTLE + 46) = H(ROUND_TIMES)[S8(GAME + 73)];
    int16_t t = (int16_t)*H(BATTLE + 46);
    int8_t own = S8(GAME + 137);
    *H(TIMER_DISPLAY) = (uint16_t)(t << 3);
    *H(TIMER_DISPLAY + 2) = own != 0 ? 0x280 : 0x780;
    *H(TIMER_DISPLAY + 4) = 0;
}

/* A CPU fighter: in slot `slot`, routine 3. */
static void cpu_fighter_start(gaddr f, uint8_t slot) {
    *B(f + 338) = slot;
    *B(f + F_HUMAN) = 2;
    *B(f + F_CPU_ROUTINE) = 3;
    *B(f + 412) = 0;
    *B(f + 373) = 0;
}

/* Screen 0 of a fight: the round's state and the fighters set up, then on to screen 1. The players'
   bits (GAME + 0x35) say which fighters a player plays; the others, and every one in Training, are
   set up as CPU fighters. */
static void fight_setup(Cpu *c, gaddr g, gaddr battle) {
    *B(GAME + 186) = 0;
    *H(g) = (uint16_t)(*H(g) + 1);
    CALL0(f_08821FEC);
    *B(g + 124) = 1; *B(g + 43) = 1;
    *B(g + 72) = 0; *B(g + 38) = 0; *B(g + 39) = 0; *B(g + 57) = 0; *B(g + 58) = 0; *B(g + 59) = 0;
    *W(g + 60) = 0;
    *B(battle + B_SPEED_ON) = 0;
    *B(g + 69) = 0; *B(g + 71) = 0;
    *B(battle + 4) = 0; *B(battle + 67) = 0; *B(battle + 90) = 0; *B(battle + 79) = 0;
    *B(battle + 48) = 0xFF;
    *B(g + 41) = (uint8_t)S8(g + G_CHOSEN_BY);
    *B(battle + 154) = 0;
    if (CALL0(f_088D5E44) == 0) {
        *B(g + 159) = 0; *B(g + 158) = 0; *B(g + 157) = 0;
        *W(g + 152) = 0; *W(g + 148) = 0;
        *B(g + 146) = 0; *B(g + 145) = 0;
        *B(g + 156) = (uint8_t)S8(SYS_OPTS + 516);
    }
    if (S8(g + G_MODE) == MODE_DRAMATIC) {
        *B(battle + 90) = 1;
        *H(battle + 56) = 3;
        *B(battle + 145) = (uint8_t)(S8(g + G_CHOSEN_BY) >> 1);
    }
    if (S8(g + G_MODE) == MODE_VARIABLE) {
        *B(battle + 90) = 1;
        *H(battle + 56) = 3;
    }
    int8_t t45 = S8(g + 45);
    *B(g + 44) = 1;
    int8_t t76 = S8(g + 76);
    *B(g + 73) = (uint8_t)t45;
    *B(battle + 8) = (uint8_t)(t76 + 1);
    *B(battle + 31) = (uint8_t)S8(g + 44);
    *H(battle + 72) = 9;
    *B(battle + 33) = (uint8_t)CALL0(f_08821368);
    *B(battle + 34) = (uint8_t)CALL0(f_08821368);
    *B(battle + 80) = (uint8_t)CALL0(f_08821368);
    CALL3(f_088236C8, FIGHTER_1, 0, *W(PADS + 4) & 0xFFFF);
    CALL3(f_088236C8, FIGHTER_2, 1, *W(PADS + 48) & 0xFFFF);
    if ((S8(g + G_CHOSEN_BY) & 1) || S8(g + G_MODE) == MODE_TRAINING) {
        *B(FIGHTER_1 + 448) = 0;
        cpu_fighter_start(FIGHTER_1, 0);
    }
    if ((S8(g + G_CHOSEN_BY) & 2) || S8(g + G_MODE) == MODE_TRAINING) {
        *B(FIGHTER_2 + 448) = 1;
        cpu_fighter_start(FIGHTER_2, 1);
    }
    if ((S8(g + G_CHOSEN_BY) & 4) || S8(g + G_MODE) == MODE_TRAINING) {
        *B(FIGHTER_3 + 448) = 2;
        cpu_fighter_start(FIGHTER_3, 2);
        CALL3(f_088236C8, FIGHTER_3, 2, *W(PADS + 92) & 0xFFFF);
    }
    *B(battle + B_SPEED) = B(SPEEDS)[S8(g + 48)];
    CALL4(f_08823844, g, battle, FIGHTER_1, FIGHTER_2);
    CALL0(f_08892018);
    CALL0(f_08836538);
    CALL0(f_0880A39C);
    memset(guest_ptr(FIGHT_SCRATCH + 0x118), 0, 0x354);
    memset(guest_ptr(FIGHT_SCRATCH + 0x46C), 0, 0x354);
}

/* The fight's screens after 0, from their table. */
static void fight_screen(Cpu *c, gaddr g, gaddr battle) {
    gaddr fn = W(SCREEN_STEPS)[*H(g + 2)];
    c->r[4] = g; c->r[5] = battle; c->r[6] = fn;
    rt_call_indirect(c, fn);
}

/* ---------------------------------------------------------------- the fight's screens */

/* These screens mostly hand on to a routine for the mode. The originals set only the first two
   argument registers (game, battle) or fewer, and some of the routines read the third and fourth as the
   caller left them; so calls here set exactly what the original sets and leave the rest. */
static void call_keep(Cpu *c, GuestFunc f) { f(c); }
#define CALL_KEEP_RESULT(c, f) ((f)(c), (c)->r[2])
static void call_gb(Cpu *c, GuestFunc f, gaddr g, gaddr battle) { c->r[4] = g; c->r[5] = battle; f(c); }
static void call_table(Cpu *c, gaddr table, uint32_t index, gaddr g, gaddr battle) {
    gaddr fn = W(table)[index];
    c->r[4] = g; c->r[5] = battle; c->r[6] = fn;
    rt_call_indirect(c, fn);
}

/* 0: by mode: Survival (7) one routine, mode 16 another, the rest a third. */
static void screen_0(Cpu *c, gaddr g, gaddr battle) {
    int8_t mode = S8(g + G_MODE);
    c->r[6] = battle;
    c->r[7] = mode < 8 ? battle : 0x10;
    if (mode == MODE_SURVIVAL) call_gb(c, f_0882BEA0, g, battle);
    else if (mode == MODE_FREE_BATTLE) call_gb(c, f_0883087C, g, battle);
    else call_gb(c, f_08823970, g, battle);
}

/* 2, 9: the screen's own steps, from a table. */
static void screen_2(Cpu *c, gaddr g, gaddr battle) {
    gaddr fn = W(SCREEN_2_STEPS)[*H(g + 4)];
    c->r[6] = fn;
    (void)battle;
    rt_call_indirect(c, fn);
}
static void screen_9(Cpu *c, gaddr g) {
    gaddr fn = W(SCREEN_9_STEPS)[*H(g + 6)];
    c->r[6] = fn;
    rt_call_indirect(c, fn);
}

/* 3: by mode: 16, 18, a network battle, Survival and mode 6, the rest. */
static void screen_3(Cpu *c, gaddr g, gaddr battle) {
    int8_t mode = S8(g + G_MODE);
    c->r[6] = 0x10;
    if (mode == MODE_FREE_BATTLE) { call_gb(c, f_0882CD50, g, battle); return; }
    if (mode == MODE_18) { call_gb(c, f_0882D254, g, battle); return; }
    if (CALL_KEEP_RESULT(c, f_088D5E44)) { call_gb(c, f_088E9DD8, g, battle); return; }
    mode = S8(g + G_MODE);
    call_gb(c, (mode == MODE_SURVIVAL || mode == MODE_6) ? f_0882D288 : f_0882B204, g, battle);
}

/* 6: quit: the sound stopped, back to the MODE MENU. */
static void screen_6(Cpu *c, gaddr g, gaddr battle) {
    CALL0(f_08814320);
    CALL2(f_088325DC, g, battle);
}

/* 7: a round starts: GAME's steps reset, the round's state cleared, then by mode (World Tour from its
   own state, the team modes, Survival, Free Battle, 17, Kumite, Training, mode 6, and the rest by
   whether the match continues), then the common round set-up. */
static void match_carries_on(gaddr g, gaddr battle) {
    *B(battle + 8) = (uint8_t)(S8(g + 77) + 1);
    *W(FIGHTER_1 + 352) = (uint32_t)((int32_t)*W(FIGHTER_1 + 352) % 100);
    *W(FIGHTER_2 + 352) = (uint32_t)((int32_t)*W(FIGHTER_2 + 352) % 100);
}

static void round_mode_setup(Cpu *c, gaddr g, gaddr battle) {
    if (S8(g + 137) != 0) {   /* World Tour: the fight's kind from its own state */
        *H(battle + 72) = (uint16_t)(int16_t)S8(WORLD_TOUR + 908);
        *W(WORLD_TOUR + 920) = 0;
        int8_t pick = S8(g + 47);
        int8_t kind = S8(WORLD_TOUR + 904);
        *B(ROUND_PICK) = (uint8_t)pick;
        if (kind == 1 || kind == 5) {
            *B(battle + 8) = 1;
            *B(battle + 109) = 0;
            *W(WT_EXTRA) = 0;
            *B(battle + 79) = kind == 1 ? 7 : 10;
            *B(battle + 155) = 0;
        } else if (kind == 2) {
            *B(battle + 79) = 0;
            *B(battle + 155) = 1;
        } else {
            int8_t v = S8(WORLD_TOUR + 905);
            *B(battle + 79) = 0;
            *B(battle + 8) = (uint8_t)v;
            *B(battle + 155) = 0;
        }
        return;
    }
    int8_t mode = S8(g + G_MODE);
    if (mode == MODE_13 || mode == MODE_19) {
        *B(battle + 61) = 0xFF;
        *B(battle + 8) = (uint8_t)(S8(g + 77) + 1);
        *B(battle + 55) = 0;
        *B(battle + 16) = 1;
    } else if (mode == MODE_SURVIVAL) {
        *B(battle + 31) = 0;
        *B(battle + 55) = 0;
        *B(battle + 8) = 1;
        *B(battle + 109) = 0;
        *W(WT_EXTRA) = 0;
        *B(ROUND_PICK) = (uint8_t)S8(g + 47);
    } else if (mode == MODE_FREE_BATTLE) {
        int8_t state = S8(battle + 159);
        *H(battle + 72) = 1;
        if (state == 3 || state == 4 || state == 5) *B(battle + 59) = 3;
    } else if (mode == MODE_17) {
        *H(battle + 72) = 9;
        *B(battle + 59) = 3;
    } else if (mode == MODE_18) {
        *B(battle + 8) = 1;
        *H(battle + 72) = 0x63;
    } else if (mode == MODE_TRAINING) {
        CALL0(f_0884A07C);
        *B(ROUND_PICK) = (uint8_t)S8(g + 47);
        *B(g + 47) = 3;
    } else if (mode == MODE_6) {
        if (CALL0(f_088D5E44) != 0) {
            int16_t stage = (int16_t)*H(battle + 98);
            *H(battle + 100) = (uint16_t)stage;
            *H(battle) = (uint16_t)stage;
        } else {
            uint32_t r;
            do r = CALL0(f_08821368) & 0xFF; while ((int32_t)r >= 37);
            *H(battle) = B(MODE_6_STAGES)[r];
        }
        *B(battle + 155) = 1;
    } else if (CALL0(f_088D7C1C) != 0 || S8(GAME + 186) != 0) {
        match_carries_on(g, battle);
        if (S8(battle + 90) == 0) {
            *B(battle + 61) = 0xFF;
        } else {
            *H(battle + 56) = 3;
            *B(battle + 61) = 0;
        }
        *B(battle + 55) = 0;
    } else if (S8(g + 57) == 3 && S8(battle + 90) == 0) {
        match_carries_on(g, battle);
        *B(battle + 61) = 0xFF;
        *B(battle + 55) = 0;
    }
}

static void screen_7(Cpu *c, gaddr g, gaddr battle) {
    static const uint8_t cleared[] = { 69, 104, 16, 58, 35, 59, 61, 66, 60, 85, 96, 37, 63, 81, 30, 29, 28, 84,
                                       36, 44, 42, 11, 13 };
    int8_t mode = S8(g + G_MODE);
    *B(g + 157) = 0;
    if (mode != MODE_SURVIVAL) {
        *B(battle + 79) = 0;
        *H(battle + 72) = 9;
    }
    uint16_t step = *H(g + 4);
    *H(g + 10) = 0;
    *H(g + 4) = (uint16_t)(step + 1);
    *H(g + 8) = 0;
    *H(g + 6) = 0;
    *B(battle + 14) = 0xFF;
    *B(battle + 15) = 0;
    *B(g + 125) = 0;
    *B(g + 43) = 1;
    *B(g + 72) = 0xFF;
    for (size_t i = 0; i < sizeof cleared; i++) *B(battle + cleared[i]) = 0;
    *H(battle + 2) = 0;
    *B(battle + 7) = 0; *B(battle + 5) = 0; *B(battle + B_SPEED_ON) = 0; *B(battle + 108) = 0;
    *B(g + 69) = 0;
    int8_t t76 = S8(g + 76);
    int16_t stage = (int16_t)*H(battle + 100);
    *B(battle + 8) = (uint8_t)(t76 + 1);
    int8_t t44 = S8(g + 44);
    *H(battle) = (uint16_t)stage;
    *B(battle + 31) = (uint8_t)t44;
    *B(battle + 9) = S8(g + 137) != 0 ? (uint8_t)S8(WORLD_TOUR + 906) : (uint8_t)S8(ROUND_START_T + S8(g + 47));
    *B(battle + 10) = 0x3B;
    round_mode_setup(c, g, battle);

    CALL2(f_08823BEC, g, battle);
    CALL2(f_0883206C, g, battle);
    int8_t v57 = S8(g + 57);
    *B(g + 70) = (uint8_t)v57;
    if (v57 == 3) *B(battle + 16) = 1;
    CALL2(f_08823D08, g, battle);
    CALL2(f_088303B8, g, battle);
    CALL0(f_088365A0);
    CALL0(f_08821F9C);
    if (S8(g + 134) != 0) {
        *B(g + 134) = 0;
        CALL2(f_0881CE4C, (uint32_t)(int32_t)(int16_t)*H(battle + 102), 1);
        *B(0x092E8940u + 7) = 1;
        *B(0x092E88A0u + 7) = 1;
        CALL0(f_08819144);
    }
    if (CALL0(f_088D5E44) != 0) CALL0(f_088D6438);
    CALL0(f_0880B6CC);
}

/* A fighter's per-round fields reset; in Arcade, for a player slot 0 or 1 that isn't playing (GAME +
   0x39), its 0x354 bytes of FIGHT_SCRATCH cleared too. */
static void fighter_round_reset(gaddr g, gaddr f) {
    static const uint16_t zero[] = { 430, 381, 380, 379, 378, 357, 356, 374, 237 };
    for (size_t i = 0; i < sizeof zero / sizeof zero[0]; i++) *B(f + zero[i]) = 0;
    *H(f + 366) = 0;
    *B(f + 415) = 1;
    *B(f + 372) = 1;
    if (S8(g + G_MODE) != 0) return;
    int8_t slot = S8(f + F_SLOT);
    if (slot & 2) return;
    if ((uint32_t)(int32_t)S8(g + 57) & (1u << (slot & 31))) return;
    memset(guest_ptr(FIGHT_SCRATCH + 0x118 + (uint32_t)(int32_t)(slot * 0x354)), 0, 0x354);
}

static void fighters_round_reset(Cpu *c, gaddr g, gaddr battle) {
    static const gaddr f[4] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, FIGHTER_4 };
    for (int k = 0; k < 4; k++) CALL3(f_08823C60, g, battle, f[k]);
}

static void fighters_round_step(Cpu *c, gaddr g, gaddr battle) {
    static const gaddr f[4] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, FIGHTER_4 };
    for (int k = 0; k < 4; k++) CALL3(f_08831558, g, battle, f[k]);
}

/* A fighter's behaviour for the round (+371, starting from +386): 2 where World Tour (+440), Free
   Battle, a network battle or a human fighter (+439) asks for it; character 10 by chance in Kumite and
   Survival's eighth stage; otherwise from ROUND_BEHAVIOUR by character, in groups of 42 picked by the
   fighters' order (BATTLE + 56), the player slot (+337), Survival's stage (BATTLE + 79) and mode 17.
   Then +430 is set for character 10 with behaviour 2. */
static int8_t behaviour_group(gaddr battle, gaddr f, int8_t mode) {
    int16_t order = (int16_t)*H(battle + 56);
    int8_t stage = S8(battle + 79);
    if (order == 0) return (stage >= 1 && stage <= 6) ? 2 : 1;
    if (order == 1 || order == 2) {
        if ((S8(f + F_SLOT) & 2) == 0) return 1;
        if (stage == 6) return 4;
        return mode == MODE_17 ? 3 : 2;
    }
    if (order == 3) return (S8(f + F_SLOT) & 2) ? 2 : 0;
    return 0;
}

static void round_behaviour(Cpu *c, gaddr g, gaddr battle, gaddr f) {
    int8_t character = S8(f + 338);
    *B(f + 371) = (uint8_t)S8(f + 386);
    int8_t pick = 0;
    if (S8(g + 137) != 0) {
        if (S8(f + 440) != 0) *B(f + 371) = 2;
    } else if (S8(g + G_MODE) == MODE_FREE_BATTLE || CALL0(f_088D5E44) != 0 || S8(f + 373) == 0) {
        if (S8(f + 439) != 0) *B(f + 371) = 2;
    } else {
        int8_t mode = S8(g + G_MODE);
        if (mode == MODE_18 || (mode == MODE_SURVIVAL && S8(battle + 79) == 8)) {
            if (S8(f + 338) == 10 && S8(f + 386) == 0 && (CALL0(f_08821368) & 1)) *B(f + 371) = 2;
        } else {
            int index = character + 42 * behaviour_group(battle, f, mode);
            if (mode != MODE_SURVIVAL || S8(battle + 79) < 7) pick = S8(ROUND_BEHAVIOUR + (uint32_t)index);
            if (pick != 0) *B(f + 371) = (uint8_t)pick;
        }
    }
    *B(f + 430) = (S8(f + 371) == 2 && S8(f + 338) == 10) ? 1 : 0;
}

/* The round's own step, by its kind (BATTLE + 59). */
static void round_kind_step(Cpu *c, gaddr battle) {
    gaddr fn = W(ROUND_KINDS)[S8(battle + 59)];
    c->r[6] = fn;
    rt_call_indirect(c, fn);
}

/* A CPU fighter's label (by fighter + 412, none for 0) at (x, y). */
static void cpu_label(Cpu *c, gaddr f, int16_t x, int16_t y) {
    int8_t which = S8(f + 412);
    if (which == 0) return;
    gaddr entry = CPU_LABELS + 8u * (uint32_t)(int32_t)which;
    int16_t at = (int16_t)(x + (int16_t)*H(entry));
    CALL5(f_0880A6CC, *W(entry + 4), 4, (uint32_t)(int32_t)at, (uint32_t)(int32_t)y, 4);
}

/* Screen 0 for most modes: on to step 1 with the fight's state cleared; a continuing match also
   clears its own flags (0x088E2304). */
static void fight_start(Cpu *c, gaddr g, gaddr battle) {
    *B(battle + 105) = (uint8_t)S8(SYS_OPTS + 65);
    uint16_t step = *H(g + 2);
    *B(g + 56) = 1;
    *H(g + 2) = (uint16_t)(step + 1);
    *B(battle + 59) = 0; *B(battle + 45) = 0; *B(battle + 36) = 0;
    *B(g + 125) = 0;
    *B(battle + 15) = 0;
    *B(FIGHTER_4 + 1) = 0; *B(FIGHTER_3 + 1) = 0; *B(FIGHTER_2 + 1) = 0; *B(FIGHTER_1 + 1) = 0;
    c->r[4] = FIGHTER_1; c->r[5] = 1;
    call_keep(c, f_08821F9C);
    if (CALL_KEEP_RESULT(c, f_088D7C1C) != 0) {
        call_keep(c, f_088E2304);
        *B(battle + 105) = 0;
        *B(g + 186) = 0;
    }
    *B(g + 143) = 0;
    *B(g + 142) = 0;
    *B(g + 141) = 0;
}

/* Screen 1: Arcade and the modes like it (0, 8, 9, 15, 17, 18, and 1 in a continuing match) run their
   step from a table by GAME + 4; the rest hand on to 0x088A0664. */
static void screen_1(Cpu *c) {
    const gaddr g = GAME;
    int8_t mode = S8(g + G_MODE);
    int table = mode == 0 || mode == MODE_DRAMATIC || mode == MODE_FINAL_BATTLE || mode == MODE_VARIABLE ||
                mode == MODE_17 || mode == MODE_18;
    if (!table) {
        c->r[5] = 0x12;
        if (CALL_KEEP_RESULT(c, f_088D7C1C) != 0) {
            mode = S8(g + G_MODE);
            c->r[4] = (uint32_t)(int32_t)mode; c->r[5] = 1;
            table = mode == 1;
        }
    }
    if (table) call_table(c, SCREEN_1_STEPS, *H(g + 4), g, BATTLE);
    else call_keep(c, f_088A0664);
}

/* The step by GAME + 4 from a table, then the drawing; Arcade's screens 3 and 4 (screen 4 skips one
   part while BATTLE + 4 is 2). */
static void draw_after(Cpu *c, int skip) {
    call_keep(c, f_08837690);
    call_keep(c, f_088379B8);
    call_keep(c, f_08887AFC);
    if (!skip) call_keep(c, f_0881DF10);
    call_keep(c, f_0880B6CC);
}
static void screen_3_steps(Cpu *c, gaddr g) {
    gaddr fn = W(SCREEN_3_STEPS)[*H(g + 4)];
    c->r[6] = fn;
    rt_call_indirect(c, fn);
    draw_after(c, 0);
}
static void screen_4_steps(Cpu *c, gaddr g, gaddr battle) {
    gaddr fn = W(SCREEN_4_STEPS)[*H(g + 4)];
    c->r[5] = battle; c->r[6] = fn;
    rt_call_indirect(c, fn);
    call_keep(c, f_08837690);
    call_keep(c, f_088379B8);
    call_keep(c, f_08887AFC);
    int8_t kind = S8(battle + 4);
    c->r[4] = (uint32_t)(int32_t)kind; c->r[5] = 2;
    if (kind != 2) call_keep(c, f_0881DF10);
    call_keep(c, f_0880B6CC);
}

/* 8: Survival waits for its fighter (the one GAME + 0x39 bit 0 says) to be done; other modes run a
   routine by GAME + 6, or with BATTLE + 155 set move on with both fighters marked. */
static void screen_8(Cpu *c, gaddr g, gaddr battle) {
    int8_t mode = S8(g + G_MODE);
    if (mode != MODE_SURVIVAL) {
        if (S8(battle + 155) != 0) {
            *H(g + 4) = (uint16_t)(*H(g + 4) + 1);
            *B(FIGHTER_1 + 413) = 1;
            *B(FIGHTER_2 + 413) = 1;
            return;
        }
        uint16_t sub = *H(g + 6);
        c->r[6] = sub; c->r[7] = (uint32_t)(int32_t)mode;
        call_gb(c, sub != 0 ? f_08895BD4 : f_0889562C, g, battle);
        return;
    }
    gaddr f = (S8(g + 57) & 1) ? FIGHTER_1 : FIGHTER_2;
    int8_t state = S8(battle + 79);
    if (state == 7 && S8(f + 431) != 0) {
        *H(f + 422) = 0;
        *H(f + 366) = 0;
        state = S8(battle + 79);
    }
    if (state != 7) {
        int8_t flags = S8(f + 411);
        if ((flags & 1) == 0) { *B(f + 411) = (uint8_t)(flags | 1); return; }
    }
    uint16_t step = *H(g + 4);
    *H(g + 10) = 0;
    *H(g + 4) = (uint16_t)(step + 1);
    *H(g + 8) = 0;
    *H(g + 6) = 0;
    CALL0(f_08821FC8);
}

/* 10: the round's end by GAME + 6, then its follow-up; on the network, while GAME + 0x91 is clear, the
   network's own step between. */
static void screen_10(Cpu *c, gaddr g, gaddr battle) {
    if (!CALL_KEEP_RESULT(c, f_088D5E44)) {
        call_table(c, SCREEN_10_STEPS, *H(g + 6), g, battle);
        call_gb(c, f_088246DC, g, battle);
        return;
    }
    if (S8(g + G_HALT_3) == 0) {
        call_table(c, SCREEN_10_STEPS, *H(g + 6), g, battle);
        call_keep(c, f_08832BB0);
    }
    call_gb(c, f_0882485C, g, battle);
}

/* ---------------------------------------------------------------- pausing */

/* A fighter's pause check (0x08828AF4, once a frame for each fighter on screen; a0 the game, a1
   the fighter). Its pad is PADS + 44 · fighter + 775. Unless the fight is already halted or the
   fighter's player (+337) is barred (GAME + 135): Start (pressed bit 0x8000) pauses, GAME + 0x59 =
   the pad's bit; so does a pad that has gone (its +0 word is 0), which also sets GAME + 138 and
   shows the pause (0x08822BB0). hooks.c wraps it to let the pads of join-in players pause. */
void f_08822BB0(Cpu *c);
static void pause_check(Cpu *c, gaddr g, gaddr fighter) {
    int8_t slot = S8(fighter + 775);
    gaddr pad = PADS + (uint32_t)(int32_t)slot * 44u;
    int16_t pressed = (int16_t)*W(pad + 12);
    if (S8(g + G_HALT) != 0) return;
    if ((uint32_t)(int32_t)S8(g + 135) & (1u << (S8(fighter + F_SLOT) & 31))) return;
    uint8_t bit = (uint8_t)(1u << (slot & 31));
    if (pressed & 0x8000) { *B(g + G_HALT) = bit; return; }
    if (*H(pad) != 0) return;
    *B(g + G_HALT) = bit;
    *B(g + 138) = bit;
    CALL1(f_08822BB0, g);
}

/* ---------------------------------------------------------------- the round's kind */

void f_08830384(Cpu *c);   /* (boss, fighter) -> nonzero: the boss fight is not on */
void f_088D7C4C(Cpu *c);   /* -> a continuing match */

#define WT_RIVAL       0x095179E0u   /* +4 the rival (u32), +15 */
#define WT_SURVIVORS   0x095179F8u   /* by BATTLE + 20, 4 bytes: +1, +2 / +3 the pair's characters */
#define WT_COLOURS     0x095179B0u   /* u8 by BATTLE + 20 */
#define BOSS_OF        0x08A0F64Cu   /* s8 by character: the boss it may meet (-1 none) */
#define ENDING_A       0x08A0F6A0u   /* s8 by character */
#define ENDING_B       0x08A0F6CAu
#define ENDING_C       0x08A0F676u
#define STAGE_SPECIAL  0x1E

/* BATTLE fields the round's kind sets */
#define B_STAGE        0      /* s16 */
#define B_ROUND_IDX    20     /* s16: the fight's number in the run */
#define B_KIND         59     /* s8: the round's kind */
#define B_FIGHT_STATE  79     /* s8 */
#define B_KIND_FLAG    85
#define B_STAGE_2      100    /* s16 */
#define B_KIND_MARK    104
#define B_KIND_SIDE    106
#define B_RIVAL        109

static void set_kind(gaddr battle, uint8_t kind, uint8_t flag) {
    *B(battle + B_KIND) = kind;
    *B(battle + B_KIND_FLAG) = flag;
}

/* The two special stage-0x1E fighters: fighters 3 and 4 as 0x1E / 0x1F */
static void special_pair(void) {
    *B(FIGHTER_3 + 338) = STAGE_SPECIAL;
    *B(FIGHTER_4 + 338) = STAGE_SPECIAL + 1;
    *B(FIGHTER_4 + 386) = 0;
    *B(FIGHTER_3 + 386) = 0;
}

/* +85 = -1 when the character's entry in `table` is set */
static void kind_flag_by(gaddr battle, gaddr table, gaddr me) {
    if (*B(table + (uint32_t)(int32_t)S8(me + 338)) != 0) *B(battle + B_KIND_FLAG) = 0xFF;
}

/* World Tour (GAME + 137) */
static void kind_world_tour(Cpu *c, gaddr battle) {
    if (S8(battle + B_FIGHT_STATE) != 0) {
        int8_t rival = S8(WORLD_TOUR + 910);
        *B(battle + B_RIVAL) = (uint8_t)rival;
        *W(WT_RIVAL + 4) = (uint32_t)(int32_t)rival;
        *B(battle + B_KIND) = 3;
        *B(battle + B_KIND_SIDE) = 3;
        *B(WT_RIVAL + 15) = 0;
    }
    if (CALL0(f_088D5E44) != 0 && *W(WORLD_TOUR + 940) != 0) {
        int8_t k = S8(WORLD_TOUR + 904);
        set_kind(battle, (k == 0 || k == 2) ? 8 : 9, 1);
        return;
    }
    if (S8(battle + B_FIGHT_STATE) != 0) { *B(battle + B_KIND) = 3; return; }
    int8_t k = S8(WORLD_TOUR + 904);
    if (k == 3 || k == 4) { *B(battle + B_KIND) = 3; return; }
    *B(battle + B_KIND_MARK) = 0;
    set_kind(battle, 0, 0);
}

/* A fight already under way (BATTLE + 79 set) */
static void kind_under_way(gaddr g, gaddr battle, gaddr other) {
    *B(g + 47) = 3;
    int8_t state = S8(battle + B_FIGHT_STATE);
    int16_t idx = (int16_t)*H(battle + B_ROUND_IDX);
    gaddr pair = WT_SURVIVORS + (uint32_t)(int32_t)idx * 4u;
    if (state == 7) {
        int8_t rival = S8(pair);
        *W(WT_RIVAL + 4) = (uint32_t)(int32_t)rival;
        *B(battle + B_RIVAL) = (uint8_t)rival;
        *B(battle + B_KIND) = 3;
        *B(battle + B_KIND_SIDE) = 3;
        uint8_t colour = *B(WT_COLOURS + (uint32_t)(int32_t)idx);
        *B(FIGHTER_4 + 386) = colour;
        *B(FIGHTER_3 + 386) = colour;
        *B(FIGHTER_3 + 338) = *B(pair + 2);
        *B(FIGHTER_4 + 338) = *B(pair + 3);
        *B(WT_RIVAL + 15) = *B(pair + 1);
        return;
    }
    if (state == 8) {
        gaddr f = S8(g + 53) == 2 ? FIGHTER_1 : FIGHTER_2;
        *B(f + 386) = *B(WT_COLOURS + (uint32_t)(int32_t)idx);
        *B(f + 338) = *B(pair + 2);
        *B(battle + B_KIND) = 0;
        *B(battle + B_RIVAL) = 0;
        return;
    }
    if (state == 6) {
        *B(battle + B_KIND) = 3;
        *B(battle + B_KIND_SIDE) = 3;
        *B(battle + B_RIVAL) = 0;
        return;
    }
    int8_t foe = S8(other + 338);
    *B(other + 432) = (foe == 10 || foe == 0x17 || foe == 0x24 || state == 3) ? 1 : 0;
    state = S8(battle + B_FIGHT_STATE);
    if (state == 4 || state == 5) { set_kind(battle, 3, 0); return; }
    if (S8(other + 338) == STAGE_SPECIAL) {
        *H(battle + B_STAGE_2) = STAGE_SPECIAL;
        *H(battle + B_STAGE) = STAGE_SPECIAL;
        *B(battle + B_KIND_SIDE) = 3;
        *B(WT_RIVAL + 15) = 0;
        *B(battle + B_KIND) = 7;     /* the original stores the register still holding state 7's test */
        special_pair();
        return;
    }
    *B(battle + B_KIND) = 0;
    *B(battle + B_RIVAL) = 0;
}

/* Arcade's later fights by their number (BATTLE + 20): 4 and 8 the mid-boss fights, 9 the boss
   (the character's boss if it is on, else the usual one); other numbers leave the kind alone */
static void kind_by_fight(Cpu *c, gaddr g, gaddr battle, gaddr me, gaddr other, int8_t boss) {
    if (S8(battle + 90) != 0) { set_kind(battle, 3, 0); return; }
    int8_t mode = S8(g + G_MODE);
    if (mode == MODE_17) { set_kind(battle, 3, 0); return; }
    if (mode == MODE_18) { set_kind(battle, 0, 0); return; }
    int16_t idx = (int16_t)*H(battle + B_ROUND_IDX);
    if (idx == 4) {
        set_kind(battle, 4, 1);
        if ((S8(me + 411) & 2) == 0) *B(battle + B_KIND_MARK) = 0xFF;
        kind_flag_by(battle, ENDING_A, me);
        return;
    }
    if (idx == 8) {
        set_kind(battle, 5, 1);
        if ((S8(me + 411) & 8) == 0) *B(battle + B_KIND_MARK) = 0xFF;
        kind_flag_by(battle, ENDING_B, me);
        return;
    }
    if (idx != 9) return;
    set_kind(battle, 6, 0);
    if (boss < 0 || CALL2(f_08830384, (uint32_t)(int32_t)boss, me) != 0) {
        if ((S8(me + 411) & 0x40) == 0) *B(battle + B_KIND_MARK) = 0xFF;
        return;
    }
    set_kind(battle, 2, 1);
    if (boss == 0x15) {
        *B(battle + B_KIND_MARK) = 0xFF;
        *H(battle + B_STAGE_2) = STAGE_SPECIAL;
        *H(battle + B_STAGE) = STAGE_SPECIAL;
        if (S8(me + 411) & 0x20) {
            *B(other + 338) = (uint8_t)boss;
            *B(other + 386) = 0;
            *B(battle + B_KIND_MARK) = 0;
        }
        kind_flag_by(battle, ENDING_C, me);
        return;
    }
    *B(battle + B_KIND) = 1;   /* the original's delay slot: kind 1 whatever happens next */
    if (boss != STAGE_SPECIAL) {
        *B(battle + B_KIND_MARK) = 0;
        set_kind(battle, 0, 0);
        return;
    }
    *B(battle + B_KIND_MARK) = 0xFF;
    *B(FIGHTER_3 + 338) = STAGE_SPECIAL;
    *B(FIGHTER_4 + 338) = STAGE_SPECIAL + 1;
    *B(FIGHTER_3 + 386) = 0;
    *B(FIGHTER_4 + 386) = 0;
    *H(battle + B_STAGE_2) = STAGE_SPECIAL;
    *H(battle + B_STAGE) = STAGE_SPECIAL;
    if (S8(me + 411) & 0x20) *B(battle + B_KIND_MARK) = 0;
    kind_flag_by(battle, ENDING_C, me);
}

/* The round's kind (0x0883206C, from screen 7): which special fight this is, if any, by mode. */
static void round_kind(Cpu *c, gaddr g, gaddr battle) {
    if (S8(g + 74) != 0 || S8(battle + 61) != 0 || S8(g + 146) != 0) {
        int8_t mode = S8(g + G_MODE);
        if (mode == MODE_13 || mode == MODE_19 || CALL0(f_088D7C4C) != 0) { set_kind(battle, 3, 0); return; }
        *B(battle + B_KIND_MARK) = 0;
        set_kind(battle, 0, 0);
        return;
    }
    gaddr me = FIGHTER_1, other = FIGHTER_2;
    if ((S8(g + 57) & 1) == 0) { me = FIGHTER_2; other = FIGHTER_1; }
    int8_t boss = S8(BOSS_OF + (uint32_t)(int32_t)S8(me + 338));
    if (S8(g + 137) != 0) { kind_world_tour(c, battle); return; }
    if (S8(battle + B_FIGHT_STATE) != 0) { kind_under_way(g, battle, other); return; }
    kind_by_fight(c, g, battle, me, other, boss);
}

/* ---------------------------------------------------------------- screen 8: the round is won */

void f_08886E18(Cpu *c); void f_08819144(Cpu *c); void f_08895B6C(Cpu *c); void f_0881CE4C(Cpu *c);
void f_08809BE4(Cpu *c); void f_0882E110(Cpu *c); void f_08813AC4(Cpu *c); void f_0887C6E4(Cpu *c);
void f_08896190(Cpu *c); void f_088967C4(Cpu *c); void f_0881CEF4(Cpu *c); void f_0887C680(Cpu *c);
void f_088969BC(Cpu *c);

#define WIN_STEPS      0x08A27284u   /* by GAME + 10: screen 8's steps once the win has started (game, battle) */
#define WIN_STEPS_SUB  0x08A2729Cu   /* by GAME + 10: the same for sub-screen 1 (game; a1 is the routine) */
#define CAMERA         0x092E8800u   /* the camera layers (0xA0 each) */
#define VOICE_STATE    0x092DA0F0u   /* +124: a voice is playing */

/* The win waits while anything is still moving: BATTLE + 61, + 159, + 90, + 79; in Training,
   Final Battle, Variable, R.Dramatic and Kumite; or until the winner's +411 bit 0 has been set
   once (the frame that sets it starts the win). Waiting: 0x08895B6C and the HUD. */
static int win_waits(gaddr g, gaddr battle, gaddr *winner) {
    if ((S8(battle + 61) | S8(battle + 159) | S8(battle + 90) | S8(battle + 79)) != 0) return 1;
    switch (S8(g + G_MODE)) {
    case MODE_TRAINING: case MODE_FINAL_BATTLE: case MODE_VARIABLE: case MODE_17: case MODE_18: return 1;
    default: break;
    }
    gaddr f = (S8(g + 57) & 1) ? FIGHTER_1 : FIGHTER_2;
    *winner = f;
    int8_t flags = S8(f + 411);
    if (flags & 1) return 1;
    *B(f + 411) = (uint8_t)(flags | 1);
    return 0;
}

/* Screen 8, sub-screen 0 (0x0889562C): until the win starts, wait; on the frame it starts, reset
   the screen's steps, the winner's voice and pose (fighter + 338, + 376) and the win text (0x109);
   after that, the step from WIN_STEPS. */
static void win_screen(Cpu *c, gaddr g, gaddr battle) {
    if (*H(g + 8) != 0) {
        gaddr fn = W(WIN_STEPS)[*H(g + 10)];
        c->r[4] = g; c->r[5] = battle; c->r[6] = fn;
        rt_call_indirect(c, fn);
        call_keep(c, f_08837690);
        call_keep(c, f_08887AFC);
        call_keep(c, f_08886E18);
        call_keep(c, f_0880B6CC);
        call_keep(c, f_08819144);
        return;
    }
    gaddr winner = 0;
    if (win_waits(g, battle, &winner)) {
        c->r[4] = g;
        f_08895B6C(c);
        c->r[4] = (uint32_t)(int32_t)(int16_t)*H(battle + 102); c->r[5] = 1;
        f_0881CE4C(c);
        call_keep(c, f_0880B6CC);
        call_keep(c, f_08819144);
        return;
    }
    *H(g + 18) = 1;
    *H(g + 8) = (uint16_t)(*H(g + 8) + 1);
    *B(battle + 81) = 0;
    *B(g + 72) = 0;
    *H(g + 10) = 0;
    *H(g + 20) = 0;
    *B(g + 43) = 1;
    c->r[4] = 0x1E; f_08809BE4(c);
    call_keep(c, f_08821FC8);
    c->r[4] = (uint32_t)(int32_t)S8(winner + 338); f_0882E110(c);
    c->r[4] = (uint32_t)(int32_t)(int16_t)*H(battle + 102); c->r[5] = 1; f_0881CE4C(c);
    c->r[4] = (uint32_t)(int32_t)S8(winner + 338); c->r[5] = (uint32_t)(int32_t)S8(winner + 376); f_08813AC4(c);
    c->r[4] = 0x109; f_0887C6E4(c);
    call_keep(c, f_0880B6CC);
    call_keep(c, f_08819144);
}

/* The fighters' layer of the camera held still: x 0, y 0x100, +90 = -16 */
static void camera_hold(void) {
    gaddr layer = CAMERA;
    *H(layer + 18) = 0;
    *H(layer + 22) = 0x100;
    *H(layer + 90) = (uint16_t)-16;
}

/* Screen 8, sub-screen 1 (0x08895BD4; the round's end in the modes that use it): the countdown at
   GAME + 130 and the step from WIN_STEPS_SUB once started; before that it waits for BATTLE + 79,
   + 61, + 17, + 159, Training, R.Dramatic, Kumite, or GAME + 146, then starts: the camera set, a
   voice if one is due (GAME + 40 and none playing), the round's text. */
static void win_screen_sub(Cpu *c, gaddr g, gaddr battle) {
    if (*H(g + 8) != 0) {
        int16_t left = (int16_t)*H(g + 130);
        if (left != 0) *H(g + 130) = (uint16_t)(left - 1);
        gaddr fn = W(WIN_STEPS_SUB)[*H(g + 10)];
        c->r[4] = g; c->r[5] = fn;     /* as the original: a1 is the routine, not BATTLE */
        rt_call_indirect(c, fn);
        call_keep(c, f_08837690);
        call_keep(c, f_08887AFC);
        call_keep(c, f_0880B6CC);
        call_keep(c, f_08819144);
        return;
    }
    int8_t mode = S8(g + G_MODE);
    int waits = S8(battle + 79) != 0 || S8(battle + 61) != 0 || (S8(battle + 17) | S8(battle + 159)) != 0 ||
                mode == MODE_TRAINING || mode == MODE_17 || mode == MODE_18 || S8(g + 146) != 0;
    if (waits) {
        c->r[4] = g; f_08896190(c);
        call_keep(c, f_0880B6CC);
        camera_hold();
        call_keep(c, f_088967C4);
        c->r[4] = (uint32_t)(int32_t)(int16_t)*H(battle + 102); c->r[5] = 1; f_0881CEF4(c);
        call_keep(c, f_08819144);
        return;
    }
    *H(g + 8) = 1;
    *B(battle + 81) = 0;
    *B(g + 72) = 0;
    *B(g + 43) = 0;
    *H(g + 10) = 0;
    call_keep(c, f_08821FC8);
    call_keep(c, f_088967C4);
    if (S8(g + 40) != 0 && *W(VOICE_STATE + 124) == 0) call_keep(c, f_0887C680);
    *W(CAMERA + 136) = 0xFFE1E1E2u;
    *W(CAMERA + 144) = 0x0001E1E1u;
    camera_hold();
    c->r[4] = ((uint32_t)(int32_t)(int16_t)*H(BATTLE + 102) << 3) + 0xD9; c->r[5] = 10; c->r[6] = 8;
    f_088969BC(c);
    c->r[4] = (uint32_t)(int32_t)(int16_t)*H(battle + 102); c->r[5] = 0; f_0881CEF4(c);
    call_keep(c, f_08837690);
    call_keep(c, f_08887AFC);
    call_keep(c, f_0880B6CC);
    call_keep(c, f_08819144);
}

/* ---------------------------------------------------------------- the fight's frame (screen 10) */

void f_088368E8(Cpu *c); void f_08854C10(Cpu *c); void f_0882E28C(Cpu *c); void f_08858E80(Cpu *c);
void f_0886D874(Cpu *c); void f_0882D5D8(Cpu *c); void f_0882D998(Cpu *c);   /* the round's steps */
void f_088377A8(Cpu *c); void f_08873D5C(Cpu *c); void f_08837690(Cpu *c); void f_0883771C(Cpu *c);
void f_0888D64C(Cpu *c); void f_088379B8(Cpu *c); void f_08887AFC(Cpu *c);
void f_08870750(Cpu *c);                                                       /* the objects' steps */
void f_088D08E8(Cpu *c); void f_088D2B5C(Cpu *c);                             /* while halted */
void f_088289A4(Cpu *c); void f_08828B88(Cpu *c);                             /* (game, battle) */
void f_0881DF10(Cpu *c); void f_0880B6CC(Cpu *c); void f_0881D1A8(Cpu *c);

/* BATTLE fields of the round's clock */
#define B_PAUSED       15     /* s8: nothing runs this frame */
#define B_CLOCK        28     /* s8: the clock's seconds */
#define B_CLOCK_TICKS  29     /* s8: frames left in this second */
#define B_CLOCK_RATE   30     /* s8: frames per second */
#define B_SHAKE_LEFT   35     /* s8: the screen shake's countdown */
#define B_LAYER_OFF    36     /* s8 */
#define B_SHAKE        74     /* s16: this frame's shake */
#define B_TICKED       78     /* u8: 1 while the clock is between seconds */
#define B_SIDES        56     /* s16: which fighters are in (the 2-on-1 modes) */

/* The round's clock: a second goes by every B_CLOCK_RATE frames; B_TICKED says whether this frame
   was between seconds. The game's frame counter (GAME + 59) runs unless BATTLE + 52. */
static void round_clock(gaddr g, gaddr battle, int skipped) {
    uint8_t between = 0;
    int8_t secs = S8(battle + B_CLOCK);
    if (secs != 0) {
        int8_t ticks = (int8_t)(S8(battle + B_CLOCK_TICKS) - 1);
        *B(battle + B_CLOCK_TICKS) = (uint8_t)ticks;
        if (ticks == 0) {
            *B(battle + B_CLOCK) = (uint8_t)(secs - 1);
            *B(battle + B_CLOCK_TICKS) = *B(battle + B_CLOCK_RATE);
        } else {
            between = 1;
        }
    }
    *B(battle + B_TICKED) = between;
    if (!skipped) *B(g + 59) = (uint8_t)(*B(g + 59) + 1);
}

static void round_steps(Cpu *c, gaddr g, gaddr battle) {
    call_keep(c, f_088368E8);
    call_keep(c, f_08854C10);
    call_gb(c, f_0882E28C, g, battle);
    call_keep(c, f_08858E80);
    call_keep(c, f_0886D874);
    call_keep(c, f_0882D5D8);
    call_gb(c, f_0882D998, g, battle);
}

static void object_steps(Cpu *c) {
    static const GuestFunc steps[] = { f_088377A8, f_08873D5C, f_08837690, f_0883771C, f_08886E18,
                                       f_0888D64C, f_088379B8, f_08887AFC, f_08870750 };
    for (size_t i = 0; i < sizeof steps / sizeof steps[0]; i++) call_keep(c, steps[i]);
}

/* Screen 10's frame offline (0x088246DC): unless BATTLE + 15 pauses it, the clock and the round's
   steps (while GAME + 0x59 is clear), the objects (or, halted, three other steps), the round's
   checks, the effects and the end of the frame. BATTLE + 52 is read before the clock runs. */
static void fight_frame_offline(Cpu *c, gaddr g, gaddr battle) {
    if (S8(battle + B_PAUSED) != 0) return;
    if (S8(g + G_HALT) == 0) {
        int skipped = S8(battle + B_SKIPPED) != 0;
        round_clock(g, battle, skipped);
        round_steps(c, g, battle);
    }
    if (S8(g + G_HALT) == 0) {
        object_steps(c);
    } else {
        call_keep(c, f_0888D64C);
        call_keep(c, f_088D08E8);
        call_keep(c, f_088D2B5C);
    }
    if (S8(battle + B_SKIPPED) == 0) call_gb(c, f_088289A4, g, battle);
    if (S8(battle + B_SKIP) == 0) call_gb(c, f_08828B88, g, battle);
    call_keep(c, f_0881DF10);
    call_keep(c, f_0880B6CC);
}

/* Every fight frame's effects (0x0881DF10): the screen shake (BATTLE + 35 counts down; odd counts
   shake by count / 4, held at 0 while GAME + 0x59 and 0x91 halt), the fighters' flags (+591) gathered
   at 0x08B01560 by which fighters are in (BATTLE + 56), the camera layer's +123, then 0x0881D1A8. */
#define CAMERA_LAYER   0x092E88A0u
#define FIGHTER_FLAGS  0x08B01560u
static void fight_effects(Cpu *c) {
    gaddr g = GAME, battle = BATTLE;
    int16_t shake = 0;
    if (!(S8(g + G_HALT) != 0 && S8(g + G_HALT_3) != 0)) {
        int8_t left = S8(battle + B_SHAKE_LEFT);
        if (left != 0) {
            *B(battle + B_SHAKE_LEFT) = (uint8_t)(left - 1);
            if (left & 1) shake = (int16_t)(left >> 2);
        }
    }
    *H(battle + B_SHAKE) = (uint16_t)shake;

    int8_t f1 = S8(FIGHTER_1 + 591), f2 = S8(FIGHTER_2 + 591);
    *B(FIGHTER_FLAGS) = (uint8_t)(f1 | f2);
    switch ((int16_t)*H(battle + B_SIDES)) {   /* the original's choices, the left-out fighter included */
    case 1: *B(FIGHTER_FLAGS) = (uint8_t)(f1 | S8(FIGHTER_3 + 591) | S8(FIGHTER_4 + 591)); break;
    case 2: *B(FIGHTER_FLAGS) = (uint8_t)(f2 | S8(FIGHTER_3 + 591) | S8(FIGHTER_4 + 591)); break;
    case 3: *B(FIGHTER_FLAGS) = (uint8_t)(f1 | f2 | S8(FIGHTER_3 + 591)); break;
    default: break;
    }
    *B(CAMERA_LAYER + 123) = S8(battle + B_LAYER_OFF) != 0 ? 0 : 15;
    CALL0(f_0881D1A8);
}

/* 5: a mode's end. BATTLE + 69 (set by the step that ends the mode, 0x0882AE20) says how: 0 or 1
   ends the fight task's run the usual way. The first frame closes the fight down (0x088162FC, which
   waits out a fade itself; 0x08815870; 0x089E6050 (2)); the next starts the ending's task in the
   fight task's place. Any other value hands over to 0x088D1634 (the network modes' end). */
#define ENDING_TASK    0x089DE0E8u
void f_08805614(Cpu *c);   /* task.c: restart the running task as another */
void f_088162FC(Cpu *c); void f_08815870(Cpu *c); void f_089E6050(Cpu *c); void f_088D1634(Cpu *c);
static void screen_5(Cpu *c, gaddr g, gaddr battle) {
    int8_t how = S8(battle + 69);
    int usual = how == 0 || how == 1;
    if (*H(g + 4) == 0) {
        if (usual) {
            call_keep(c, f_088162FC);
            call_keep(c, f_08815870);
            CALL1(f_089E6050, 2);
        }
        *H(g + 4) = (uint16_t)(*H(g + 4) + 1);
        return;
    }
    if (usual) CALL1(f_08805614, ENDING_TASK);
    else call_keep(c, f_088D1634);
}

/* 11: while GAME + 0x91 is clear: a countdown (GAME + 130), the screen's step by GAME + 6 and the
   drawing; then, always, 0x0881DF10 and the text. */
static void screen_11(Cpu *c, gaddr g, gaddr battle) {
    if (S8(g + G_HALT_3) == 0) {
        int16_t left = (int16_t)*H(g + 130);
        if (left != 0) *H(g + 130) = (uint16_t)(left - 1);
        gaddr fn = W(SCREEN_11_STEPS)[*H(g + 6)];
        c->r[4] = g; c->r[5] = battle; c->r[6] = battle; c->r[7] = fn;
        rt_call_indirect(c, fn);
        call_keep(c, f_08837690);
        call_keep(c, f_088379B8);
        call_keep(c, f_0883771C);
        call_keep(c, f_08886E18);
        call_keep(c, f_08887AFC);
    }
    call_keep(c, f_0881DF10);
    call_keep(c, f_0880B6CC);
}

/* ---------------------------------------------------------------- task 0: the fight loop */

static int halted(gaddr g) { return S8(g + G_HALT) != 0 || S8(g + G_HALT_2) != 0 || S8(g + G_HALT_3) != 0; }

static void fight_loop(Cpu *c) {
    const gaddr g = GAME, battle = BATTLE;
    *B(g + 43) = 1;
    *B(g + 186) = 0; *B(g + G_HALT) = 0; *B(g + G_HALT_2) = 0; *B(g + G_HALT_3) = 0; *B(g + 135) = 0;
    CALL0(f_0880A39C);
    *B(g + 82) = 0; *B(g + 81) = 0; *B(g + G_FADE_TASK) = 0; *B(g + 92) = 0;
    *B(battle + B_SKIPPED) = 0;
    CALL1(f_08805564, 3);
    *B(g + 54) = 0xFF;
    CALL1(f_088054F0, 1);
    for (;;) {
        *B(g + G_FRAME_DONE) = 0xFF;
        CALL1(f_088E30FC, g);
        for (;;) {
            /* Offline, the counter stops while halted but the frame skip and the mode's step still
               run; on the network, a halted frame runs neither. */
            int net = CALL0(f_088E3550) != 0;
            if (!halted(g)) *B(g + G_FRAME) = (uint8_t)(*B(g + G_FRAME) + 1);
            if (!net || !halted(g)) {
                CALL2(f_088235C0, g, battle);
                CALL0(f_08820C94);
            }
            CALL2(f_088D7FF4, g, battle);
            CALL2(f_088DBC7C, g, battle);
            CALL1(f_088E3FF0, battle);
            CALL0(f_088859C8);
            CALL2(f_08823674, g, battle);
            CALL0(f_088D80D8);
            net = CALL0(f_088E3550) != 0;
            if (net && halted(g)) break;
            /* go round without waiting while a pass is asked for or this frame is skipped */
            if (S8(g + G_FRAME_DONE) == 0 || S8(battle + B_SKIP) != 0) continue;
            break;
        }
        CALL1(f_088054F0, 1);
    }
}

/* ---------------------------------------------------------------- task 2: the battle task */

/* A fighter's CPU routine, when the CPU plays it: not with two players, not a human's fighter, and in
   Final Battle only for a slot that chose the mode. */
static void cpu_fighter(Cpu *c, gaddr g, gaddr battle, gaddr f) {
    if (S8(g + G_TWO_PLAYERS) != 0) return;
    if (S8(g + G_MODE) == MODE_FINAL_BATTLE && ((uint32_t)(int32_t)S8(g + G_CHOSEN_BY) & (1u << (S8(f + F_SLOT) & 31))) == 0) return;
    if (S8(f + F_HUMAN) != 0) return;
    gaddr fn = W(CPU_ROUTINES)[S8(f + F_CPU_ROUTINE)];
    c->r[4] = g; c->r[5] = battle; c->r[6] = f;
    rt_call_indirect(c, fn);
}

static void battle_task(Cpu *c) {
    const gaddr g = GAME, battle = BATTLE;
    for (;;) {
        if (S8(g + G_HALT) == 0) {
            CALL3(f_08833BD4, g, battle, FIGHTER_1);
            CALL3(f_08833BD4, g, battle, FIGHTER_2);
            CALL0(f_088347DC);
        }
        CALL1(f_088054F0, 1);
    }
}

/* ---------------------------------------------------------------- adapters (frames as the originals') */
#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
SFA_FUNC(088233EC, game_fight_loop)       { FRAME(0x20, fight_loop(c)); }
SFA_FUNC(08833B6C, game_battle_task)      { FRAME(0x20, battle_task(c)); }
SFA_FUNC(08833BD4, game_cpu_fighter)      { FRAME(0x20, cpu_fighter(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(088E3550, game_network_active)   { RETURN((uint32_t)network_active()); }
SFA_FUNC(088235C0, game_frame_skip)       { frame_skip(ARG(0), ARG(1)); }
SFA_FUNC_WRAPPED(08823674, game_fight_frame, stagestrips_fight_frame) { FRAME(0x20, fight_frame(c, ARG(0), ARG(1))); }
SFA_FUNC(088D80D8, game_network_text)     { FRAME(0x20, network_text(c)); }
SFA_FUNC(088E3FF0, game_network_step)     { FRAME(0x20, network_step(c, ARG(0))); }
SFA_FUNC(088859C8, game_stage_effect)     { FRAME(0x20, stage_effect(c)); }
SFA_FUNC(08820C94, game_fighters_step)    { FRAME(0x20, fighters_step(c)); }
SFA_FUNC(088347DC, game_cpu_step)         { FRAME(0x20, cpu_step(c)); }
SFA_FUNC(08820DE8, game_fighters_pads)    { fighters_pads(ARG(0), ARG(1), ARG(2), ARG(3)); }
SFA_FUNC(088D7FF4, game_network_input)    { FRAME(0x20, network_input(c, ARG(0), ARG(1))); }
SFA_FUNC(08828C40, game_fight_setup)      { FRAME(0x40, fight_setup(c, ARG(0), ARG(1))); }
SFA_FUNC(088238B4, game_fight_screen)     { FRAME(0x20, fight_screen(c, ARG(0), ARG(1))); }
SFA_FUNC(08821368, game_random)           { uint32_t r; FRAME(0x10, r = random_number(c)); RETURN(r); }
SFA_FUNC(088D5E44, game_network_battle)   { RETURN((uint32_t)network_battle()); }
/* Callers here (call_keep) hand the registers on, so they are left as the original leaves them. */
SFA_FUNC(088D7C1C, game_continuing_match) {
    c->r[4] = GAME; c->r[5] = MODE_NETWORK; c->r[6] = (uint32_t)(int32_t)S8(GAME + G_MODE);
    if ((int32_t)c->r[6] != MODE_NETWORK) c->r[4] = (uint32_t)(int32_t)S8(GAME + 146);
    RETURN((uint32_t)continuing_match());
}
SFA_FUNC(088236C8, game_fighter_start)    { fighter_start(ARG(0), ARG(1), (uint16_t)ARG(2)); }
SFA_FUNC(08823844, game_pair_start)       { FRAME(0x20, pair_start(c, ARG(0), ARG(1), ARG(2), ARG(3))); }
SFA_FUNC(08836538, game_round_time)       { round_time(); }
SFA_FUNC(08892018, game_fight_stub)       { }
SFA_FUNC(088238E8, game_fight_screen_0)   { FRAME(0x20, screen_0(c, ARG(0), ARG(1))); }
SFA_FUNC(08823A34, game_fight_screen_1)   { FRAME(0x10, f_088938C8(c)); }
SFA_FUNC(088938C8, game_fight_screen_1_steps) { FRAME(0x20, screen_1(c)); }
SFA_FUNC(08823970, game_fight_start)      { FRAME(0x10, fight_start(c, ARG(0), ARG(1))); }
SFA_FUNC(0882B204, game_fight_screen_3_steps) { FRAME(0x20, screen_3_steps(c, ARG(0))); }
SFA_FUNC(0882B5FC, game_fight_screen_4_steps) { FRAME(0x20, screen_4_steps(c, ARG(0), ARG(1))); }
SFA_FUNC(08823BB8, game_fight_screen_2)   { FRAME(0x20, screen_2(c, ARG(0), ARG(1))); }
SFA_FUNC(08823A50, game_fight_screen_3)   { FRAME(0x20, screen_3(c, ARG(0), ARG(1))); }
SFA_FUNC(08823B0C, game_fight_screen_4)   { FRAME(0x20, f_0882B5FC(c)); }
SFA_FUNC(08828148, game_fight_screen_6)   { FRAME(0x20, screen_6(c, ARG(0), ARG(1))); }
SFA_FUNC(08828F60, game_fight_screen_7)   { FRAME(0x30, screen_7(c, ARG(0), ARG(1))); }
SFA_FUNC(08823BEC, game_fighters_round_reset) { FRAME(0x20, fighters_round_reset(c, ARG(0), ARG(1))); }
SFA_FUNC(08823C60, game_fighter_round_reset)  { fighter_round_reset(ARG(0), ARG(2)); }
SFA_FUNC(08823D08, game_round_kind_step)  { FRAME(0x20, round_kind_step(c, ARG(1))); }
SFA_FUNC(088303B8, game_fighters_round_step) { FRAME(0x20, fighters_round_step(c, ARG(0), ARG(1))); }
SFA_FUNC(08831558, game_round_behaviour)  { FRAME(0x20, round_behaviour(c, ARG(0), ARG(1), ARG(2))); }
SFA_FUNC(08834888, game_cpu_label)        { FRAME(0x30, cpu_label(c, ARG(0), (int16_t)ARG(1), (int16_t)ARG(2))); }
SFA_FUNC(0882429C, game_fight_screen_8)   { FRAME(0x20, screen_8(c, ARG(0), ARG(1))); }
SFA_FUNC(088243B4, game_fight_screen_9)   { FRAME(0x20, screen_9(c, ARG(0))); }
SFA_FUNC(0882462C, game_fight_screen_10)  { FRAME(0x20, screen_10(c, ARG(0), ARG(1))); }
SFA_FUNC_WRAPPED(08828AF4, game_pause_check, hooks_pause) { FRAME(0x10, pause_check(c, ARG(0), ARG(1))); }
SFA_FUNC(08823B28, game_fight_screen_5)   { FRAME(0x20, screen_5(c, ARG(0), ARG(1))); }
SFA_FUNC(08827B98, game_fight_screen_11)  { FRAME(0x20, screen_11(c, ARG(0), ARG(1))); }
SFA_FUNC(088246DC, game_fight_frame_offline) { FRAME(0x20, fight_frame_offline(c, ARG(0), ARG(1))); }
SFA_FUNC(0881DF10, game_fight_effects)    { FRAME(0x10, fight_effects(c)); }
SFA_FUNC(0883206C, game_round_kind)       { FRAME(0x30, round_kind(c, ARG(0), ARG(1))); }
SFA_FUNC(0889562C, game_win_screen)       { FRAME(0x20, win_screen(c, ARG(0), ARG(1))); }
SFA_FUNC(08895BD4, game_win_screen_sub)   { FRAME(0x30, win_screen_sub(c, ARG(0), ARG(1))); }
