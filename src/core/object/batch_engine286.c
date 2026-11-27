/* More of the fighter's engine: the taunt check and its character tables, the jump back in after a
   landing, a cpu fighter's counter start, and the steps that end by trying each way out in turn
   (taunt, special, attack, stand-up). Also the fight's reset before a first round. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

void f_08863D24(Cpu *c); void f_08863114(Cpu *c); void f_0887BE00(Cpu *c); void f_08863D4C(Cpu *c);
void f_088BE4E0(Cpu *c); void f_08823BEC(Cpu *c); void f_088BE69C(Cpu *c); void f_088365A0(Cpu *c);
void f_0887A300(Cpu *c); void f_0886469C(Cpu *c); void f_08861848(Cpu *c); void f_088629CC(Cpu *c);
void f_088632C4(Cpu *c); void f_08863504(Cpu *c); void f_08862B84(Cpu *c); void f_08862C48(Cpu *c);
void f_08863B40(Cpu *c); void f_088639C0(Cpu *c); void f_088639CC(Cpu *c); void f_088622CC(Cpu *c);
void f_088622E8(Cpu *c); void f_08862484(Cpu *c); void f_088628EC(Cpu *c); void f_088623BC(Cpu *c);
void f_088623E4(Cpu *c); void f_08862004(Cpu *c); void f_0887C614(Cpu *c); void f_0886163C(Cpu *c);
void f_0886BB1C(Cpu *c); void f_08861F68(Cpu *c); void f_0886D29C(Cpu *c); void f_08864494(Cpu *c);
void f_0886731C(Cpu *c); void f_08864740(Cpu *c); void f_08876BE4(Cpu *c); void f_0887709C(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* A 0x20 frame with s0 at +16 and the return address at +20; s0 holds the fighter. */
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* A check and what follows when it passes: `then` returns 8 after `test`. */
static int try_then(Cpu *c, gaddr o, void (*test)(Cpu *), void (*then)(Cpu *), uint32_t back, uint32_t back_then) {
    call1(c, o, test, back);
    if (c->r[2] == 0) return 0;
    call1(c, o, then, back_then);
    return 1;
}

/* (n, bits): whether bit n (its low byte) is set in a table of bits. No frame. */
SFA_FUNC(08863D24, bit_in_table) {
    uint32_t n = ARG(0) & 0xFF;
    RETURN((*B(ARG(1) + (n >> 3)) & (1u << (n & 7))) != 0);
}

/* (fighter): the taunt's start: state 1 / 2 / 0 / 0, facing the opponent (+697). No frame. */
SFA_FUNC(08863D4C, taunt_start) {
    gaddr o = ARG(0);
    *B(o + 4) = 1; *B(o + 5) = 2;
    int8_t facing = S8(o + 697);
    *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 11) = (uint8_t)facing;
}

/* (fighter): whether a player's fighter taunts now: not while the fight is held or for a cpu, with a
   direction held (+752); on two punches or two kicks pressed together (+760: the second kind 1, +193),
   when the character has that taunt in the air (0x08A17F7A / 0x08A17F80, 32 above the ground), crouching
   (0x08A17F6E / 0x08A17F74) or standing (the kick one by 0x08A17F68); +192 says which: 2, 1, 0. A frame
   keeping s0..s2. */
SFA_FUNC(08863B40, taunt_check) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t stick = *H(o + 752);
    int held = S8(BATTLE + 14) | S8(BATTLE + 13) | ((stick & 3) == 0) | S8(GAME + 38) | S8(o + 373);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    uint32_t done = 0;
    if (held == 0) {
        uint32_t b = *H(o + 760) & 0x3F0, kind, where;
        gaddr bits; uint32_t back;
        if (b == 0x50 || b == 0x70 || b == 0x30 || b == 0x60) kind = 0;
        else if (b == 0x280 || b == 0x380 || b == 0x180 || b == 0x300) kind = 1;
        else goto out;
        c->r[17] = kind;
        if (S8(o + 73) != 0) {
            if (S16(o + 22) - S16(o + 132) < 32) goto out;
            where = 2; bits = kind ? 0x08A17F80u : 0x08A17F7Au; back = kind ? 0x08863C80u : 0x08863C48u;
        } else if (stick & 4) {
            where = 1; bits = kind ? 0x08A17F74u : 0x08A17F6Eu; back = kind ? 0x08863CD4u : 0x08863CB8u;
        } else {
            where = 0; bits = 0x08A17F68u; back = 0x08863D00u;
        }
        c->r[18] = where;
        if (!(where == 0 && kind == 0)) {
            call2(c, (uint32_t)(int32_t)S8(o + 338) & 0xFF, bits, f_08863D24, back);
            if (c->r[2] == 0) goto out;
        }
        *B(o + 193) = (uint8_t)kind; *B(o + 192) = (uint8_t)where;
        done = 1;
    }
out:
    c->r[2] = done;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a cpu fighter's counter: +482 = 5, +483 = 1, state 1 / 0 / 8 / 0 facing the opponent,
   +693 / +696 / +123 cleared, the move +490 into +236, and by the strength +491 the timings +662 / +664
   (15 / 5, 48 / 16, 81 / 27 with +548 = 4); +270 set. No frame. */
SFA_FUNC(0887BE00, cpu_counter_start) {
    gaddr o = ARG(0);
    *B(o + 482) = 5; *B(o + 483) = 1; *B(o + 4) = 1; *B(o + 6) = 8;
    int8_t facing = S8(o + 697);
    *B(o + 7) = 0; *B(o + 5) = 0; *B(o + 11) = (uint8_t)facing; *B(o + 693) = 0;
    int8_t move = S8(o + 490);
    *B(o + 696) = 0; *B(o + 123) = 0; *B(o + 236) = (uint8_t)move;
    int32_t k = S8(o + 491);
    *B(o + 492) = 0;
    if (k == 0) { *B(o + 548) = 0; *H(o + 662) = 0xF; *H(o + 664) = 5; }
    else if (k == 1) { *B(o + 548) = 0; *H(o + 662) = 0x30; *H(o + 664) = 0x10; }
    else if (k == 2) { *B(o + 548) = 4; *H(o + 662) = 0x51; *H(o + 664) = 0x1B; }
    *B(o + 270) = 1;
}

/* (fighter): back into a jump: +266 cleared, +192 = 2 before and after, 0x08863114, then a player's
   jump by the character (+416's +68); a cpu's counter when +693, its taunt when +688, else the
   same jump. A frame keeping s0, s1. */
SFA_FUNC(08861F68, jump_again) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = 2;
    *B(o + 266) = 0;
    *W(c->r[29] + 24) = ra;
    *B(o + 192) = 2;
    c->r[31] = 0x08861F8Cu; f_08863114(c);
    if (S8(o + 373) != 0 && S8(o + 693) != 0) call1(c, o, f_0887BE00, 0x08861FC0u);
    else if (S8(o + 373) != 0 && S8(o + 688) != 0) call1(c, o, f_08863D4C, 0x08861FD8u);
    else {
        uint32_t back = S8(o + 373) != 0 ? 0x08861FECu : 0x08861FA8u;
        gaddr jump = *W(*W(o + 416) + 68);
        c->r[5] = jump; ARG(0) = o; c->r[31] = back;
        rt_call_indirect(c, jump);
    }
    *B(o + 192) = (uint8_t)c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (): the fight's memory before a first fight: BATTLE's and GAME's flags and counts cleared (GAME +
   72 and BATTLE + 48 = -1, GAME + 43 = 1, BATTLE + 72 = 9, BATTLE + 20 = 8), the two players' fighters
   reset (0x088BE4E0), then 0x08823BEC, 0x088BE69C and the line-up's handicaps. A frame keeping s0,
   s1. */
SFA_FUNC(088BE39C, fight_memory_reset) {
    static const uint8_t first[] = { 79, 59, 66, 60, 85, 104, 63, 96, 37, 58, 35, 61, 30, 29, 28, 84, 36, 44, 42 };
    static const uint8_t game[] = { 53, 56, 71, 69, 59, 58, 57, 39, 38 };
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = BATTLE;
    for (unsigned i = 0; i < sizeof first; i++) *B(BATTLE + first[i]) = 0;
    *W(c->r[29] + 20) = s1; c->r[17] = GAME;
    *B(BATTLE + 15) = 0; *B(GAME + 125) = 0;
    *B(BATTLE + 11) = 0; *B(BATTLE + 13) = 0; *B(BATTLE + 7) = 0; *B(BATTLE + 5) = 0; *B(BATTLE + 22) = 0;
    *B(BATTLE + 45) = 0; *B(GAME + 41) = 0; *B(BATTLE + 4) = 0;
    for (unsigned i = 0; i < sizeof game; i++) *B(GAME + game[i]) = 0;
    *B(GAME + 72) = 0xFF; *B(GAME + 43) = 1; *B(BATTLE + 48) = 0xFF;
    *B(GAME + 44) = 0;
    *B(BATTLE + 31) = (uint8_t)S8(GAME + 44);
    *H(BATTLE + 72) = 9; *H(BATTLE + 20) = 8;
    *W(c->r[29] + 24) = ra;
    c->r[6] = 0; call2(c, FIGHTER_1, 0, f_088BE4E0, 0x088BE498u);
    c->r[6] = 0; call2(c, FIGHTER_2, 1, f_088BE4E0, 0x088BE4ACu);
    call2(c, GAME, BATTLE, f_08823BEC, 0x088BE4B8u);
    call2(c, GAME, BATTLE, f_088BE69C, 0x088BE4C4u);
    c->r[31] = 0x088BE4CCu; f_088365A0(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a recovery step. The cpu: at the animation's end (0x0887A300 first when +689, with +482
   / +481 cleared) standing, else ticked. A player: the stun's end while BATTLE + 13, else 0x088632C4
   then 0x08863504, else 0x08862B84 then the stun recovery; after that the ways out in turn: taunt,
   0x088639C0 / 0x088639CC, the stand-up choice, an attack (0x08862484 / 0x088628EC); at the
   animation's end 0x088623BC / 0x088623E4 or standing; else ticked. */
SFA_FUNC(0885AD6C, recover_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t cpu = S8(o + 373);
    enter(c, o, sv);
    if (cpu != 0) {
        if (S8(o + 75) & 0x80) {
            if (S8(o + 689) != 0) {
                *B(o + 482) = 0; *B(o + 481) = 0;
                call1(c, o, f_0887A300, 0x0885ADB0u);
            }
            call1(c, o, f_0886469C, 0x0885ADB8u);
        } else call1(c, o, f_08861848, 0x0885ADC8u);
    } else {
        if (S8(BATTLE + 13) != 0) call1(c, o, f_088629CC, 0x0885ADECu);
        else if (!try_then(c, o, f_088632C4, f_08863504, 0x0885ADFCu, 0x0885AE0Cu))
            try_then(c, o, f_08862B84, f_08862C48, 0x0885AE1Cu, 0x0885AE2Cu);
        if (try_then(c, o, f_08863B40, f_08863D4C, 0x0885AE34u, 0x0885AE44u)) { }
        else if (try_then(c, o, f_088639C0, f_088639CC, 0x0885AE54u, 0x0885AE64u)) { }
        else if (try_then(c, o, f_088622CC, f_088622E8, 0x0885AE74u, 0x0885AE84u)) { }
        else if (try_then(c, o, f_08862484, f_088628EC, 0x0885AE94u, 0x0885AEA4u)) { }
        else if (S8(o + 75) & 0x80) {
            if (!try_then(c, o, f_088623BC, f_088623E4, 0x0885AEC0u, 0x0885AED0u))
                call1(c, o, f_0886469C, 0x0885AEE0u);
        } else call1(c, o, f_08861848, 0x0885AEF0u);
    }
    leave(c, sv);
}

/* (fighter): a fall's step: landed (0x08862004, moving down): on the ground at +132, +235 / +773
   cleared, 0x0887C614, and a player's attack if one is due, else step 2 with the cue 0x11. In the air:
   at the animation's end with +251, step 1; with +327 a player may jump again (0x0886BB1C; +567 =
   -1); ticked, and character 11's +77 is cleared. */
SFA_FUNC(0885AF00, fall_step) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x0885AF14u; f_08862004(c);
    if (c->r[2] != 0 && (int32_t)*W(o + 100) < 0) {
        int32_t ground = S16(o + 132);
        *B(o + 73) = 0;
        *W(o + 20) = (uint32_t)ground << 16;
        *B(o + 235) = 0; *B(o + 773) = 0;
        call1(c, o, f_0887C614, 0x0885AF48u);
        if (S8(o + 373) == 0 && try_then(c, o, f_08862484, f_088628EC, 0x0885AF5Cu, 0x0885AF6Cu)) { }
        else {
            *B(o + 7) = 2;
            call2(c, o, 0x11, f_0886163C, 0x0885AF8Cu);
        }
        leave(c, sv);
        return;
    }
    if ((S8(o + 75) & 0x80) && S8(o + 251) != 0) { *B(o + 7) = 1; *B(o + 235) = 0; }
    if (S8(o + 327) != 0 && S8(o + 373) == 0) {
        call1(c, o, f_0886BB1C, 0x0885AFDCu);
        if (c->r[2] != 0) {
            call1(c, o, f_08861F68, 0x0885AFECu);
            *B(o + 567) = 0xFF;
        }
    }
    call1(c, o, f_08861848, 0x0885AFFCu);
    if (S8(o + 338) == 0xB && S8(o + 77) != 0) *B(o + 77) = 0;
    leave(c, sv);
}

/* (fighter): a move's end: 0x0886D29C, +543 = 5 (2 after +595), the move's flags cleared, facing the
   opponent, 0x08864494 and 0x0886731C; then a player (+566 = 2) taunts, 0x088639C0 / 0x088639CC,
   attacks, or goes on crouching or standing by the stick; a cpu (its plan cleared, 0x0887A300 with
   +486) blocks when there is something to (0x08876BE4 / 0x0887709C), else stands or crouches by
   +696. */
SFA_FUNC(0885E7FC, move_end) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x0885E810u; f_0886D29C(c);
    *B(o + 543) = S8(o + 595) != 0 ? 2 : 5;
    *B(o + 539) = 0; *B(o + 538) = 0; *B(o + 595) = 0; *B(o + 588) = 0; *B(o + 575) = 0; *B(o + 625) = 0;
    *B(o + 624) = 0; *B(o + 626) = 0; *B(o + 127) = 0;
    *W(o + 244) = 0; *H(o + 778) = 0;
    int8_t facing = S8(o + 697);
    *B(o + 126) = 0; *B(o + 125) = 0; *B(o + 11) = (uint8_t)facing; *B(o + 656) = 0;
    call1(c, o, f_08864494, 0x0885E87Cu);
    call1(c, o, f_0886731C, 0x0885E884u);
    *B(o + 773) = 0; *B(o + 270) = 0; *B(o + 770) = 0;
    int32_t cpu = S8(o + 373);
    *B(o + 592) = 0;
    if (cpu == 0) {
        *B(o + 566) = 2;
        if (try_then(c, o, f_08863B40, f_08863D4C, 0x0885E8ACu, 0x0885E8BCu)) { }
        else if (try_then(c, o, f_088639C0, f_088639CC, 0x0885E8CCu, 0x0885E8DCu)) { }
        else if (try_then(c, o, f_08862484, f_088628EC, 0x0885E8ECu, 0x0885E8FCu)) { }
        else if (*H(o + 752) & 4) call1(c, o, f_08864740, 0x0885E918u);
        else call1(c, o, f_0886469C, 0x0885E928u);
    } else {
        *B(o + 539) = 0; *B(o + 483) = 0; *B(o + 482) = 0; *B(o + 481) = 0;
        int16_t plan = S16(o + 486);
        *B(o + 270) = 0;
        if (plan != 0) call1(c, o, f_0887A300, 0x0885E954u);
        if (try_then(c, o, f_08876BE4, f_0887709C, 0x0885E95Cu, 0x0885E96Cu)) { }
        else if (S8(o + 696) == 0) call1(c, o, f_0886469C, 0x0885E984u);
        else call1(c, o, f_08864740, 0x0885E994u);
    }
    leave(c, sv);
}

/* (fighter): characters 17 and 19 change into each other: at first (+7 = 0) the other's number, +627
   cleared, the character's own set-up (+416's +140's first entry) and the cue 0x30; then at the
   animation's end standing; else a player's ways out (taunt, 0x088639C0, attack), the stand-up choice,
   or ticked. */
SFA_FUNC(08864364, change_character_step) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    int32_t who = S8(o + 338);
    *W(c->r[29] + 20) = sv[1];
    if (who == 0x11 || who == 0x13) {
        uint32_t step = *B(o + 7);
        if (step == 0) {
            *B(o + 7) = 1;
            *B(o + 627) = 0;
            *B(o + 338) = (uint8_t)(who ^ 2);
            gaddr list = *W(o + 416);
            c->r[5] = (uint32_t)(int32_t)S8(o + 338);
            gaddr set = *W(*W(list + 140));
            c->r[6] = set; ARG(0) = o; c->r[31] = 0x088643C0u;
            rt_call_indirect(c, set);
            call2(c, o, 0x30, f_0886163C, 0x088643CCu);
        } else if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x088643ECu);
        else if (S8(o + 373) == 0 &&
                 (try_then(c, o, f_08863B40, f_08863D4C, 0x08864404u, 0x08864414u) ||
                  try_then(c, o, f_088639C0, f_088639CC, 0x08864424u, 0x08864434u) ||
                  try_then(c, o, f_08862484, f_088628EC, 0x08864444u, 0x08864454u))) { }
        else if (!try_then(c, o, f_088622CC, f_088622E8, 0x08864464u, 0x08864474u))
            call1(c, o, f_08861848, 0x08864484u);
    }
    leave(c, sv);
}

/* (fighter): onto the ground: 0x08864494, state 1 / 0 / 3 / 2, 0x0886731C and 0x0887C614; then a
   player's ways out (taunt, 0x088639C0, attack), the stand-up choice, or the cue 0x11. */
SFA_FUNC(08864888, ground_start) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x0886489Cu; f_08864494(c);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3; *B(o + 7) = 2;
    call1(c, o, f_0886731C, 0x088648C0u);
    call1(c, o, f_0887C614, 0x088648C8u);
    if (S8(o + 373) == 0) {
        if (try_then(c, o, f_08863B40, f_08863D4C, 0x088648DCu, 0x088648ECu)) { }
        else if (try_then(c, o, f_088639C0, f_088639CC, 0x088648FCu, 0x0886490Cu)) { }
        else if (try_then(c, o, f_08862484, f_088628EC, 0x0886491Cu, 0x0886492Cu)) { }
        else if (try_then(c, o, f_088622CC, f_088622E8, 0x0886493Cu, 0x0886494Cu)) { }
        else call2(c, o, 0x11, f_0886163C, 0x0886495Cu);
    } else if (!try_then(c, o, f_088622CC, f_088622E8, 0x0886496Cu, 0x0886497Cu))
        call2(c, o, 0x11, f_0886163C, 0x0886498Cu);
    leave(c, sv);
}

void f_08864140(Cpu *c); void f_08864204(Cpu *c); void f_0886425C(Cpu *c); void f_08876AA8(Cpu *c);
void f_08877148(Cpu *c); void f_088771DC(Cpu *c); void f_08861E94(Cpu *c); void f_08861EDC(Cpu *c);
void f_08862430(Cpu *c);

/* What a landing does to the fighter: on the ground at +132, out of the air. */
static void set_down(gaddr o) {
    int32_t ground = S16(o + 132);
    *B(o + 73) = 0;
    *W(o + 20) = (uint32_t)ground << 16;
    *B(o + 235) = 0;
}

/* The step on (+7) with +768 cleared and the landing cue 0x11. */
static void land_cue(Cpu *c, gaddr o, uint32_t back) {
    uint8_t step = *B(o + 7);
    *B(o + 768) = 0;
    *B(o + 7) = (uint8_t)(step + 1);
    call2(c, o, 0x11, f_0886163C, back);
}

/* (fighter): a jump's step. Landed (0x08862004, moving down): set down, +768 / +773 / +270 cleared,
   0x0887C614; a cpu lands; a player: character 14's own landing (+416's +140: the first entry asks,
   the second does it), else taunt, 0x088639C0, an attack, the stand-up choice, or lands. In the air a
   player taunts, jumps again (0x088639C0), 0x08864140 / 0x08864204, attacks in the air (0x0886425C), or
   is ticked; a cpu jumps again (0x08876AA8), 0x08877148 / 0x08864204, and unless its choice is -1
   attacks on 0x088771DC, else ticked. A frame keeping s0, s1. */
SFA_FUNC(0885AAC4, jump_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = 0x0885AAE0u; f_08862004(c);
    if (c->r[2] != 0 && (int32_t)*W(o + 100) < 0) {
        set_down(o);
        *B(o + 768) = 0; *B(o + 773) = 0; *B(o + 270) = 0;
        call1(c, o, f_0887C614, 0x0885AB1Cu);
        if (S8(o + 373) != 0) land_cue(c, o, 0x0885AC34u);
        else {
            if (S8(o + 338) == 0xE) {
                gaddr ask = *W(*W(*W(o + 416) + 140));
                c->r[5] = ask; ARG(0) = o; c->r[31] = 0x0885AB4Cu;
                rt_call_indirect(c, ask);
                if (c->r[2] != 0) c->r[17] = 1;
            }
            if (c->r[17] != 0) {
                gaddr own = *W(*W(*W(o + 416) + 140) + 4);
                c->r[5] = own; ARG(0) = o; c->r[31] = 0x0885AB74u;
                rt_call_indirect(c, own);
            }
            else if (try_then(c, o, f_08863B40, f_08863D4C, 0x0885AB84u, 0x0885AB94u)) { }
            else if (try_then(c, o, f_088639C0, f_088639CC, 0x0885ABA4u, 0x0885ABB4u)) { }
            else if (try_then(c, o, f_08862484, f_088628EC, 0x0885ABC4u, 0x0885ABD4u)) { }
            else if (try_then(c, o, f_088622CC, f_088622E8, 0x0885ABE4u, 0x0885ABF4u)) { }
            else land_cue(c, o, 0x0885AC14u);
        }
    } else if (S8(o + 373) == 0) {
        if (try_then(c, o, f_08863B40, f_08863D4C, 0x0885AC4Cu, 0x0885AC5Cu)) { }
        else if (try_then(c, o, f_088639C0, f_08861F68, 0x0885AC6Cu, 0x0885AC7Cu)) { }
        else if (try_then(c, o, f_08864140, f_08864204, 0x0885AC8Cu, 0x0885AC9Cu)) { }
        else if (try_then(c, o, f_08862484, f_0886425C, 0x0885ACACu, 0x0885ACBCu)) { }
        else call1(c, o, f_08861848, 0x0885ACCCu);
    } else {
        if (try_then(c, o, f_08876AA8, f_08861F68, 0x0885ACDCu, 0x0885ACECu)) { }
        else if (try_then(c, o, f_08877148, f_08864204, 0x0885ACFCu, 0x0885AD0Cu)) { }
        else if (S8(o + 386) == -1) call1(c, o, f_08861848, 0x0885AD28u);
        else if (!try_then(c, o, f_088771DC, f_0886425C, 0x0885AD38u, 0x0885AD48u))
            call1(c, o, f_08861848, 0x0885AD58u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a jump's first part. Landed: set down, +773 cleared, 0x0887C614, a player's attack (+270
   cleared), else step 2 with the cue 0x11. In the air a player taunts, jumps again, or is ticked: with
   an attack due as it is, else into step 1 (+266 / +267 cleared when not facing the opponent); a cpu
   jumps again (0x08876AA8), or step 1 with nothing to block; ticked. */
SFA_FUNC(0885B02C, jump_rise_step) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x0885B040u; f_08862004(c);
    if (c->r[2] != 0 && (int32_t)*W(o + 100) < 0) {
        set_down(o);
        *B(o + 773) = 0;
        call1(c, o, f_0887C614, 0x0885B074u);
        int attack = 0;
        if (S8(o + 373) == 0) {
            call1(c, o, f_08862484, 0x0885B088u);
            attack = c->r[2] != 0;
        }
        if (attack) {
            *B(o + 270) = 0;
            call1(c, o, f_088628EC, 0x0885B0B4u);
        } else {
            *B(o + 7) = 2;
            call2(c, o, 0x11, f_0886163C, 0x0885B0A4u);
        }
    } else if (S8(o + 373) == 0) {
        if (try_then(c, o, f_08863B40, f_08863D4C, 0x0885B0D4u, 0x0885B0E4u)) { }
        else if (try_then(c, o, f_088639C0, f_08861F68, 0x0885B0F4u, 0x0885B104u)) { }
        else {
            call1(c, o, f_08862484, 0x0885B114u);
            if (c->r[2] != 0) call1(c, o, f_08861848, 0x0885B124u);
            else {
                if ((uint32_t)(int32_t)S8(o + 697) != *B(o + 11)) { *B(o + 266) = 0; *B(o + 267) = 0; }
                *B(o + 7) = 1;
                call1(c, o, f_08861848, 0x0885B14Cu);
            }
        }
    } else if (!try_then(c, o, f_08876AA8, f_08861F68, 0x0885B15Cu, 0x0885B16Cu)) {
        call1(c, o, f_08876BE4, 0x0885B17Cu);
        if (c->r[2] == 0) *B(o + 7) = 1;
        call1(c, o, f_08861848, 0x0885B190u);
    }
    leave(c, sv);
}

/* What the two block steps start with: the stun's end while BATTLE + 13, 0x088632C4 / 0x08863504,
   0x08862B84 / the stun recovery; then for a player the taunt, 0x088639C0 / 0x088639CC and 0x08861E94
   / 0x08861EDC. 1 when one of them took the frame. `back` lists the return addresses in order. */
static int block_head(Cpu *c, gaddr o, const uint32_t back[11]) {
    if (S8(BATTLE + 13) != 0) { call1(c, o, f_088629CC, back[0]); return 1; }
    if (try_then(c, o, f_088632C4, f_08863504, back[1], back[2])) return 1;
    if (try_then(c, o, f_08862B84, f_08862C48, back[3], back[4])) return 1;
    if (S8(o + 373) != 0) return 0;
    if (try_then(c, o, f_08863B40, f_08863D4C, back[5], back[6])) return 1;
    if (try_then(c, o, f_088639C0, f_088639CC, back[7], back[8])) return 1;
    return try_then(c, o, f_08861E94, f_08861EDC, back[9], back[10]);
}

/* The block's end for a player with no attack coming: the step on, turned to the opponent (+266 / +267
   cleared when it was not), and the cue. */
static void block_leave(Cpu *c, gaddr o, uint32_t cue, uint32_t back) {
    uint8_t step = *B(o + 7);
    uint32_t facing = *B(o + 11);
    *B(o + 7) = (uint8_t)(step + 1);
    int32_t to = S8(o + 697);
    if (facing != (uint32_t)to) { *B(o + 266) = 0; *B(o + 267) = 0; *B(o + 11) = (uint8_t)to; }
    call2(c, o, cue, f_0886163C, back);
}

/* (fighter): the standing block's hold: while an attack comes (a player by 0x08862484, a cpu by
   0x08876BE4, which else stands) it stays, facing the opponent and ticked; a player holding down
   (0x08862430), or a cpu whose opponent's +310 is set or who crouches on the ground, changes to the
   crouching block: step 2, +696 set, the cue 0x19. A player with none coming leaves (cue 0x17). */
SFA_FUNC(0885B85C, block_stand_hold) {
    static const uint32_t b[11] = { 0x0885B884u, 0x0885B894u, 0x0885B8A4u, 0x0885B8B4u, 0x0885B8C4u, 0x0885B8DCu,
                                    0x0885B8ECu, 0x0885B8FCu, 0x0885B90Cu, 0x0885B91Cu, 0x0885B92Cu };
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (!block_head(c, o, b)) {
        int low;
        uint32_t back_low, back_stay;
        if (S8(o + 373) == 0) {
            call1(c, o, f_08862484, 0x0885B93Cu);
            if (c->r[2] == 0) { block_leave(c, o, 0x17, 0x0885B9BCu); leave(c, sv); return; }
            call1(c, o, f_08862430, 0x0885B94Cu);
            low = c->r[2] != 0; back_low = 0x0885B974u; back_stay = 0x0885B988u;
        } else {
            call1(c, o, f_08876BE4, 0x0885B9CCu);
            if (c->r[2] == 0) { call1(c, o, f_0886469C, 0x0885B9DCu); leave(c, sv); return; }
            gaddr other = *W(o + 92);
            low = S8(other + 310) != 0 || (S8(other + 73) == 0 && S8(other + 696) != 0);
            back_low = 0x0885BA28u; back_stay = 0x0885BA38u;
        }
        int8_t to = S8(o + 697);
        if (low) {
            *B(o + 7) = 2; *B(o + 696) = 1; *B(o + 11) = (uint8_t)to;
            call2(c, o, 0x19, f_0886163C, back_low);
        } else {
            *B(o + 11) = (uint8_t)to;
            call1(c, o, f_08861848, back_stay);
        }
    }
    leave(c, sv);
}

/* (fighter): the crouching block's hold, the other way round: a player still holding down
   (0x088623BC), or a cpu whose opponent's +310 is set or who crouches on the ground, stays, facing
   the opponent and ticked; else up into the standing block: step 0, +696 cleared, the cue 0x16. A
   cpu with nothing to block crouches (0x08864740); a player with none coming leaves (cue 0x1A). */
SFA_FUNC(0885BB80, block_crouch_hold) {
    static const uint32_t b[11] = { 0x0885BBA8u, 0x0885BBB8u, 0x0885BBC8u, 0x0885BBD8u, 0x0885BBE8u, 0x0885BC00u,
                                    0x0885BC10u, 0x0885BC20u, 0x0885BC30u, 0x0885BC40u, 0x0885BC50u };
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (!block_head(c, o, b)) {
        int stay;
        uint32_t back_up, back_stay;
        if (S8(o + 373) == 0) {
            call1(c, o, f_08862484, 0x0885BC60u);
            if (c->r[2] == 0) { block_leave(c, o, 0x1A, 0x0885BCD8u); leave(c, sv); return; }
            call1(c, o, f_088623BC, 0x0885BC70u);
            stay = c->r[2] != 0; back_up = 0x0885BC90u; back_stay = 0x0885BCA4u;
        } else {
            call1(c, o, f_08876BE4, 0x0885BCE8u);
            if (c->r[2] == 0) { call1(c, o, f_08864740, 0x0885BCF8u); leave(c, sv); return; }
            gaddr other = *W(o + 92);
            if (S8(other + 310) != 0) { stay = 1; back_stay = 0x0885BD18u; }
            else { stay = S8(other + 696) != 0 && S8(other + 73) == 0; back_stay = 0x0885BD60u; }
            back_up = 0x0885BD50u;
        }
        int8_t to = S8(o + 697);
        if (stay) {
            *B(o + 11) = (uint8_t)to;
            call1(c, o, f_08861848, back_stay);
        } else {
            *B(o + 7) = 0; *B(o + 696) = 0; *B(o + 11) = (uint8_t)to;
            call2(c, o, 0x16, f_0886163C, back_up);
        }
    }
    leave(c, sv);
}
