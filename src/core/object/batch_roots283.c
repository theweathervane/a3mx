/* Functions many others wait on: the fighters' handicap levels, the combo counts shown to each side, the
   six queued pictures of the fight, the fight's tune, and the score and stage numbers of the round's
   display. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_0880C308(Cpu *c); void f_088D5E44(Cpu *c); void f_08821368(Cpu *c); void f_0880A6CC(Cpu *c);
void f_088D7CE0(Cpu *c); void f_088D5E7C(Cpu *c); void f_0888722C(Cpu *c);
void f_08837238(Cpu *c); void f_08836A3C(Cpu *c); void f_08836E90(Cpu *c); void f_088369AC(Cpu *c);
void f_08837688(Cpu *c); void f_0883680C(Cpu *c); void f_088366D8(Cpu *c); void f_08836AA8(Cpu *c);
void f_08836C80(Cpu *c); void f_08836C3C(Cpu *c); void f_088872FC(Cpu *c); void f_088873DC(Cpu *c);
void f_088874C8(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (): the hit count each side shows (+769) from the other side's count (+126), by the line-up (BATTLE +
   56): one against one each shows the other's; with a pair, the single fighter shows the pair's sum
   (99 at most, and +357 keeps the highest). No frame. */
SFA_FUNC(0886B540, hit_counts_show) {
    int32_t n = S16(BATTLE + 56), v;
    gaddr to;
    if (n < 0) { to = BATTLE; v = n; }
    else if (n == 0) {
        int8_t two = S8(FIGHTER_2 + 126), one = S8(FIGHTER_1 + 126);
        *B(FIGHTER_1 + 769) = (uint8_t)two; *B(FIGHTER_2 + 769) = (uint8_t)one;
        return;
    } else if (n == 1) {
        *B(*W(BATTLE + 140) + 769) = (uint8_t)S8(FIGHTER_1 + 126);
        to = *W(BATTLE + 136); v = S8(FIGHTER_3 + 126) + S8(FIGHTER_4 + 126);
    } else if (n == 2) {
        *B(*W(BATTLE + 136) + 769) = (uint8_t)S8(FIGHTER_2 + 126);
        to = *W(BATTLE + 140); v = S8(FIGHTER_3 + 126) + S8(FIGHTER_4 + 126);
    } else if (n == 3) {
        gaddr one = *W(BATTLE + 140);
        to = *W(BATTLE + 136);
        if (S8(GAME + 70) & 1) { gaddr t = one; one = to; to = t; }
        *B(one + 769) = (uint8_t)S8(FIGHTER_3 + 126);
        v = S8(FIGHTER_1 + 126) + S8(FIGHTER_2 + 126);
    } else { to = BATTLE; v = 0; }   /* the original falls here with its own comparison's result */
    if (!(v < 99)) v = 99;
    int32_t best = S8(to + 357);
    *B(to + 769) = (uint8_t)v;
    if (best < v) *B(to + 357) = (uint8_t)v;
}

/* (): draws the fight's six queued pictures, last first (0x0880C308): the flags at 0x092E29A8 and the
   pictures 8 further, or the second set 40 on while GAME + 145. A frame keeping s0..s2. */
SFA_FUNC(0880BEEC, queued_six_draw) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    int alt = S8(GAME + 145) != 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    gaddr flags = alt ? 0x092E29D0u : 0x092E29A8u;
    uint32_t back = alt ? 0x0880BF84u : 0x0880BF40u;
    c->r[18] = flags;
    for (int32_t i = 5; i >= 0; i--) {
        c->r[16] = (uint32_t)i; c->r[17] = flags + 8 + (uint32_t)i * 4;
        if (*B(flags + (uint32_t)i) != 0) { ARG(0) = *W(c->r[17]); c->r[31] = back; f_0880C308(c); }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the level a fighter in use with +373 set plays at (+375), 31 unless: while GAME + 191, 8
   for one neither +432 nor +337 bit 1; in mode 16 the table 0x08A0FB48 by GAME + 190 (8 for a pair's
   member without +337 bit 1); in mode 18 and over the network BATTLE + 46; in mode 2 the table by GAME
   + 45; else by the line-up and the partner's +314. A frame keeping s0..s4. */
SFA_FUNC(08836C80, fighter_level_set) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1; *W(sp) = s0; *W(sp + 8) = s2; *W(sp + 12) = s3; *W(sp + 16) = s4; *W(sp + 20) = ra;
    int32_t level = 0x1F;
    if (S8(o + 373) == 0 || *B(o) == 0) goto out;
    gaddr mate = *W(o + 92);
    if (S8(GAME + 191) != 0) {
        level = (S8(o + 432) != 0 || (S8(o + 337) & 2)) ? 0x1F : 8;
        goto store;
    }
    int32_t mode = S8(GAME + 114);
    if (mode == 0x10) {
        if (S16(BATTLE + 56) != 0 && !(S8(o + 337) & 2)) level = 8;
        else level = S16(0x08A0FB48u + 2 * (uint32_t)(int32_t)S8(GAME + 190));
        goto store;
    }
    if (mode == 0x12) { level = S16(BATTLE + 46); goto store; }
    if (mode == 2) { level = S16(0x08A0FB48u + 2 * (uint32_t)(int32_t)S8(GAME + 45)); goto store; }
    c->r[16] = o; c->r[17] = 0x1F; c->r[18] = mate; c->r[19] = GAME; c->r[20] = 2;
    ARG(0) = (uint32_t)mode; ARG(1) = 0x12;
    c->r[31] = 0x08836D90u; f_088D5E44(c);
    if (c->r[2] != 0) { level = S16(BATTLE + 46); goto store; }
    int32_t mate_set = S8(mate + 314), lineup = S16(BATTLE + 56);
    if (mate_set == 0 && lineup != 3) {
        level = S16(BATTLE + 46);
        if ((lineup == 1 || lineup == 2) && S8(GAME + 114) != 3 && S8(BATTLE + 79) < 4) level >>= 1;
        goto store;
    }
    if (lineup != 3) goto store;
    int pair = S8(o + 337) & 2;
    if (S8(GAME + 137) == 0) level = pair ? S16(BATTLE + 46) : 8;
    else if (!pair) level = 8;
    else if (mate_set == 0) level = S16(BATTLE + 46);
store:
    *B(o + 375) = (uint8_t)level;
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (): the four fighters' levels (0x08836C80). */
SFA_FUNC(08836C3C, fighter_levels_set) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = FIGHTER_1; call0(c, f_08836C80, 0x08836C50u);
    ARG(0) = FIGHTER_2; call0(c, f_08836C80, 0x08836C5Cu);
    ARG(0) = FIGHTER_3; call0(c, f_08836C80, 0x08836C68u);
    ARG(0) = FIGHTER_4; call0(c, f_08836C80, 0x08836C74u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (): the round's handicap set-up unless both players fight (GAME + 57 = 3): each of the first two
   takes the other's +432 into +314, the pair's two (when the line-up has them) their own; then
   0x08837238, 0x08836A3C, 0x08836E90, 0x088369AC and the levels; always 0x08837688 last. */
SFA_FUNC(088368E8, round_handicaps) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    int32_t fights = S8(GAME + 57);
    *W(c->r[29]) = ra;
    if (fights != 3) {
        gaddr a = (fights & 1) ? FIGHTER_1 : FIGHTER_2, b = (fights & 1) ? FIGHTER_2 : FIGHTER_1;
        int8_t av = S8(a + 432), bv = S8(b + 432);
        *B(b + 314) = (uint8_t)av; *B(a + 314) = (uint8_t)bv;
        if (S16(BATTLE + 56) != 0) {
            *B(FIGHTER_3 + 314) = (uint8_t)S8(FIGHTER_3 + 432);
            *B(FIGHTER_4 + 314) = (uint8_t)S8(FIGHTER_4 + 432);
        }
        call0(c, f_08837238, 0x08836978u);
        call0(c, f_08836A3C, 0x08836980u);
        call0(c, f_08836E90, 0x08836988u);
        call0(c, f_088369AC, 0x08836990u);
        call0(c, f_08836C3C, 0x08836998u);
    }
    call0(c, f_08837688, 0x088369A0u);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (): the handicap set-up by line-up: with a pair 0x0883680C; both players fighting 0x088366D8; else
   0x08836AA8, 0x08836E90, 0x088369AC and the levels. */
SFA_FUNC(088365A0, lineup_handicaps) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    if (S16(BATTLE + 56) != 0) call0(c, f_0883680C, 0x088365ECu);
    else if (S8(GAME + 57) == 3) call0(c, f_088366D8, 0x088365D8u);
    else {
        call0(c, f_08836AA8, 0x08836600u);
        call0(c, f_08836E90, 0x08836608u);
        call0(c, f_088369AC, 0x08836610u);
        call0(c, f_08836C3C, 0x08836618u);
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (cue): the fight's tune: a cue above 255 as it is; else the tune table 0x08A22D60 by a number picked
   from the mode (GAME + 114), the stage (BATTLE + 0) and the first fighter's character (+338; 41 takes
   the stage's): the stage's own tune with stages 37 - 40 as 44, 41 as 29 and stage 6 as 20 one time in
   two; the final fights' 42 (+430); two tunes that alternate (0x08A22DBC); 23 / 43 / 36 for the boss
   stages of the arcade's ninth and eighth fights. A 0x10 frame keeping s0. */
SFA_FUNC(0887C724, fight_tune) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    uint32_t cue = ARG(0) & 0xFFFF;
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    if (cue & 0xFF00) goto out;
    gaddr f = FIGHTER_3;
    int32_t mode = S8(GAME + 114);
    if (S16(BATTLE + 56) == 0) f = S8(BATTLE + 48) != 0 ? FIGHTER_1 : FIGHTER_2;
    int32_t who = S8(f + 338);
    uint32_t own = who == 0x29 ? (uint16_t)S16(BATTLE) : (uint16_t)who;
    cue = own;
    enum { STAGE, FIXED45, PRESENT, PLAIN, LOOKUP, SEVENTH, FINAL, NETWORK } kind;
    static const uint8_t kinds[18] = {
        STAGE, FIXED45, PRESENT, PLAIN, PLAIN, LOOKUP, SEVENTH, FINAL, PLAIN,
        PLAIN, PLAIN, NETWORK, STAGE, PLAIN, FINAL, STAGE, FINAL, STAGE,
    };
    kind = (uint32_t)(mode - 1) < 18u ? kinds[mode - 1] : PLAIN;
    if (kind == NETWORK) kind = S8(GAME + 146) != 0 ? STAGE : PLAIN;
    if (kind == STAGE) {
        cue = (uint16_t)S16(BATTLE);
        if (cue >= 37 && cue < 41) cue = 0x2C;
        else if (cue == 0x29) cue = 0x1D;
        else if (cue == 6) {
            c->r[16] = cue; ARG(0) = 6;
            c->r[31] = 0x0887C80Cu; f_08821368(c);
            if (c->r[2] & 1) cue = 0x14;
        }
    } else if (kind == FIXED45) cue = 0x2D;
    else if (kind == PRESENT) {
        gaddr p = (S8(GAME + 53) & 1) ? FIGHTER_2 : FIGHTER_1;
        if (S8(p + 430) != 0) cue = 0x2A;
    } else if (kind == FINAL || kind == SEVENTH) {
        int turn = kind == FINAL ? S8(f + 430) == 0 : S8(BATTLE + 79) == 7;
        if (turn) {
            uint32_t t = *W(0x08A22DBCu);
            cue = (t + 0x2B) & 0xFFFF;
            *W(0x08A22DBCu) = t ^ 1;
        } else if (S8(f + 430) != 0) cue = 0x2A;
    } else if (kind == PLAIN) {
        if (S8(BATTLE + 61) != 0) { }
        else if (S8(f + 430) != 0) cue = 0x2A;
        else if (mode != 3 && S8(BATTLE + 79) == 0) {
            int32_t fight = S16(BATTLE + 20);
            if (fight == 9 && own == 0) cue = 0x17;
            else if (fight == 8) {
                int32_t stage = S16(BATTLE);
                if (stage == 10) cue = 0x2B;
                else if (stage == 2) cue = 0x24;
            }
        }
    }
    cue = *H(0x08A22D60u + 2 * cue);
out:
    c->r[2] = cue;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* The text call both number displays end with: the string at (x, 0), kind 1 / 5, and the text object
   it returns gets +3 = 7 and +12 = 1. s0 holds 1 across the call. */
static void number_show(Cpu *c, gaddr text, int32_t x, uint32_t back) {
    c->r[16] = 1;
    ARG(0) = text; ARG(1) = 1; ARG(2) = (uint32_t)x; ARG(3) = 0; c->r[8] = 5;
    c->r[31] = back; f_0880A6CC(c);
    *B(c->r[2] + 3) = 7; *B(c->r[2] + 12) = 1;
}

/* (fighter, x): the fighter's score (+352) as seven digits, blanks before the first and a 0 for none,
   into the text at 0x08A25A58 and shown at x. A 0x30 frame keeping s0. */
SFA_FUNC(088872FC, score_show) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    int32_t x = (int16_t)ARG(1);
    uint32_t v = *W(ARG(0) + 352), div = 1000000;
    c->r[29] -= 0x30;
    gaddr at = *W(0x08A25A58u);
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    int any = 0;
    for (int i = 0; i < 7; i++) {
        uint32_t q = v / div;
        if (q | (uint32_t)any) { any = 1; *B(at) = (uint8_t)(q + '0'); }
        else *B(at) = ' ';
        at++; v %= div; div /= 10;
    }
    if (!any) *B(at - 1) = '0';
    number_show(c, *W(0x08A25A58u), x, 0x088873C0u);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter, x): the fight's number (BATTLE + 20, from 1) as three digits with blanks before the first,
   into the text at 0x08A25A68 from its sixth character, shown at x. A 0x30 frame keeping s0. */
SFA_FUNC(088873DC, fight_number_show) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    int32_t x = (int16_t)ARG(1);
    c->r[29] -= 0x30;
    uint32_t v = (uint32_t)(S16(BATTLE + 20) + 1) & 0xFFFF, div = 100;
    gaddr at = *W(0x08A25A68u) + 5;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    int any = 0;
    for (int i = 0; i < 3; i++) {
        if (!any && v / div == 0) *B(at) = ' ';
        else { any = 1; *B(at) = (uint8_t)(v / div + '0'); }
        at++; v = (v % div) & 0xFFFF; div = (div / 10) & 0xFFFF;
    }
    number_show(c, *W(0x08A25A68u), x, 0x088874ACu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter, x): the number under a fighter's bar, unless BATTLE + 36, 0x088D7CE0, 0x088D5E7C or mode
   16's BATTLE + 159 = 2 say none: with BATTLE + 61 the display 0x0888722C; nothing while BATTLE + 16 or
   the fighter's +373; the score while GAME + 137; in mode 18 the fight's number (the score with GAME +
   32 bit 6); else the score, or for a fighter with +426 or in mode 7 the display 0x0888722C unless
   that bit. A frame keeping s0..s2. */
SFA_FUNC(088874C8, bar_number_show) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    int32_t x = (int16_t)ARG(1);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[17] = BATTLE; c->r[18] = (uint32_t)x; c->r[16] = o;
    c->r[5] = (uint32_t)(int32_t)S8(BATTLE + 36);
    if (c->r[5] != 0) goto out;
    call0(c, f_088D7CE0, 0x08887500u);
    if (c->r[2] != 0) goto out;
    call0(c, f_088D5E7C, 0x08887510u);
    if (c->r[2] != 0) goto out;
    int32_t mode = S8(GAME + 114);
    if (mode == 0x10 && S8(BATTLE + 159) == 2) goto out;
    if (S8(BATTLE + 61) != 0) { call2(c, o, (uint32_t)x, f_0888722C, 0x08887554u); goto out; }
    if (S8(BATTLE + 16) != 0 || S8(o + 373) != 0) goto out;
    int scores = (*B(GAME + 32) & 0x40) != 0;
    if (S8(GAME + 137) != 0) call2(c, o, (uint32_t)x, f_088872FC, 0x08887588u);
    else if (mode == 0x12) {
        if (scores) call2(c, o, (uint32_t)x, f_088872FC, 0x088875C4u);
        else call2(c, o, (uint32_t)x, f_088873DC, 0x088875B4u);
    } else if (S16(o + 426) != 0 || mode == 7) {
        if (scores) call2(c, o, (uint32_t)x, f_088872FC, 0x08887608u);
        else call2(c, o, (uint32_t)x, f_0888722C, 0x088875F8u);
    } else call2(c, o, (uint32_t)x, f_088872FC, 0x08887618u);
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (): the numbers under both bars, at x 72 and 280: the two sides' fighters (BATTLE + 136 / 140); in
   mode 15, and mode 16 with BATTLE + 159 = 3, only for the line-up 3: the third fighter and its mate
   (+92), sides by GAME + 53. A frame keeping s0. */
SFA_FUNC(08887168, bar_numbers_show) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t mode = S8(GAME + 114);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = BATTLE;
    if (mode == 0xF || (mode == 0x10 && S8(BATTLE + 159) == 3)) {
        c->r[16] = 0x092E0000u;
        if (S16(BATTLE + 56) == 3) {
            int right = S8(GAME + 53) == 2;
            c->r[16] = FIGHTER_3;
            call2(c, *W(FIGHTER_3 + 92), right ? 0x118 : 0x48, f_088874C8, right ? 0x088871D8u : 0x088871F4u);
            call2(c, c->r[16], right ? 0x48 : 0x118, f_088874C8, right ? 0x088871E4u : 0x08887200u);
        }
    } else {
        call2(c, *W(BATTLE + 136), 0x48, f_088874C8, 0x08887210u);
        call2(c, *W(c->r[16] + 140), 0x118, f_088874C8, 0x0888721Cu);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

void f_08876F44(Cpu *c); void f_08830384(Cpu *c);

/* (list): moves six scrolling entries (16 bytes each, from +0x20) by their signed step (+10 the sign,
   +11 the size): the first three along +4, wrapped into 0..240, the next three along +2, wrapped down
   by 400 from 64; entries with +1 set are not wrapped; ten bits kept. No frame. */
SFA_FUNC(08886454, scroll_six) {
    gaddr p = ARG(0) + 0x20;
    for (int i = 0; i < 3; i++, p += 0x10) {
        int32_t d = S8(p + 10) >= 0 ? *B(p + 11) : (int16_t)-(int32_t)*B(p + 11);
        int32_t v = (int16_t)(d + S16(p + 4));
        if (*B(p + 1) == 0) {
            if (v < 0) v = (int16_t)(v + 0xF0);
            if (!(v < 241)) v = (int16_t)(v - 0xF0);
        }
        *H(p + 4) = (uint16_t)(v & 0x3FF);
    }
    for (int i = 0; i < 3; i++, p += 0x10) {
        int32_t d = S8(p + 10) >= 0 ? *B(p + 11) : (int16_t)-(int32_t)*B(p + 11);
        int32_t v = (int16_t)(d + S16(p + 2));
        if (*B(p + 1) == 0 && !(v < 64)) v = (int16_t)(v - 0x190);
        *H(p + 2) = (uint16_t)(v & 0x3FF);
    }
}

/* (object): two colours of the table 0x08A2B058 by the object's place (+18, +22, low four bits) into
   the list at 0x08A28324 / 0x08A2832A; and, when its count (+74) runs out, the next pair of colours
   (+77 counts them) from 0x08A2B080 into 0x08A2830C / 0x08A28312, with the next wait from 0x08A2B078.
   No frame. */
SFA_FUNC(0889E318, colours_by_place) {
    gaddr o = ARG(0);
    *H(0x08A28324u) = *H(0x08A2B058u + 2 * (uint32_t)(S16(o + 18) & 0xF));
    *H(0x08A2832Au) = *H(0x08A2B058u + 2 * (uint32_t)(S16(o + 22) & 0xF));
    int32_t t = S8(o + 74);
    if (t < 0) return;
    *B(o + 74) = (uint8_t)(t - 1);
    if ((int8_t)(t - 1) != 0) return;
    int32_t i = S8(o + 77);
    *B(o + 77) = (uint8_t)(i + 1);
    *B(o + 74) = (uint8_t)S8(0x08A2B078u + (uint32_t)i);
    int32_t k = (int16_t)(i + i);
    *H(0x08A2830Cu) = *H(0x08A2B080u + 2 * (uint32_t)k);
    *H(0x08A28312u) = *H(0x08A2B080u + 2 * (uint32_t)(int32_t)(int16_t)(k + 1));
}

/* (texture): the bytes a texture's pixels take: its width (+16) times height (+20), which (with the
   format at +4) are read from the picture's header (+0) when not yet known, a GBIX or a PVRT one; by
   the format's high half: 0x500 half a byte a pixel, 0x700 one, else two. No frame. */
SFA_FUNC(089E05A4, texture_bytes) {
    gaddr t = ARG(0), p = t + 4;
    uint32_t n = *W(p + 12) * *W(p + 16);
    if (n == 0) {
        gaddr head = *W(t);
        uint32_t magic = *W(head);
        if (magic == 0x58494247u || magic == 0x54525650u) {
            uint32_t at = magic == 0x58494247u ? 24 : 8;
            *W(p) = ((uint32_t)*H(head + at) << 16) + *H(head + at + 2);
            *W(p + 12) = *H(*W(t) + at + 4);
            uint32_t high = *H(*W(t) + at + 6);
            n = *W(p + 12) * high;
            *W(p + 16) = high;
        }
    }
    uint32_t format = *W(p) >> 16;
    if (format == 0x500) c->r[2] = (n << 2) >> 3;
    else if (format == 0x700) c->r[2] = (n << 3) >> 3;
    else c->r[2] = (n << 4) >> 3;
}

/* Three layers' places from three scroll records 0x30 apart (a 16.16 value's whole part at +0 / +4,
   plus an offset at +16 / +18) into the objects at 0x092E8800, 0x092E88A0 and 0x092E8940. */
static void layers_place(gaddr rec) {
    gaddr o = 0x092E8800u;
    *H(o + 18) = (uint16_t)((*W(rec) >> 16) + (uint32_t)S16(rec + 16));
    uint32_t y = *W(rec + 4) >> 16;
    *H(o + 22) = (uint16_t)y;
    *H(o + 98) = (uint16_t)(y + (uint32_t)S16(rec + 18));
    for (int i = 1; i < 3; i++) {
        rec += 0x30; o += 0xA0;
        uint32_t x = *W(rec) >> 16;
        *H(o + 18) = (uint16_t)x;
        *W(o + 16) = (x + (uint32_t)S16(rec + 16)) << 16;
        y = *W(rec + 4) >> 16;
        *H(o + 22) = (uint16_t)y;
        *W(o + 96) = (y + (uint32_t)S16(rec + 18)) << 16;
    }
}

/* (): the three layer objects placed from the scroll records at 0x08B027B0. No frame. */
SFA_FUNC(088A9794, layers_place_first) { (void)c; layers_place(0x08B027B0u); }

/* (): the same from the second set at 0x08B027C8. No frame. */
SFA_FUNC(088A9880, layers_place_second) { (void)c; layers_place(0x08B027C8u); }

/* (): the cpu fighters' (+373) shared draw (+499), 0x40 while either player has +432: else one of 32
   lots, picked at random, of which the first are 0 and the next 0x20 and the rest 0x40, the two counts
   from the table 0x08A0FB88 by GAME + 73, the round (BATTLE + 2, 3 at most) and the fight (BATTLE + 20:
   up to 7, by 0x08A0F9B8 with BATTLE + 79; mode 18 takes the table by the fight alone). A frame
   keeping s0..s4. */
SFA_FUNC(08837300, cpu_draw_lots) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp) = s0; *W(sp + 4) = s1; *W(sp + 12) = s3; *W(sp + 8) = s2; *W(sp + 16) = s4; *W(sp + 20) = ra;
    int32_t lot = 0x40;
    if (S8(FIGHTER_1 + 432) == 0 && S8(FIGHTER_2 + 432) == 0) {
        const gaddr lots = 0x09524A68u + 6;
        gaddr row = 0x08A0FB88u + ((uint32_t)(uint16_t)(int16_t)S8(GAME + 73) << 6);
        int32_t round = S16(BATTLE + 2);
        if (!(round < 3)) round = 3;
        row += (uint32_t)round << 4;
        int32_t fight = S16(BATTLE + 20);
        if (S8(GAME + 114) == 0x12) {
            row = 0x08A0FB88u + ((uint32_t)(fight / 4) << 4);
            fight %= 8;
        } else if (S8(BATTLE + 79) != 0) fight = S8(0x08A0F9B8u + (uint32_t)fight);
        else if (!(fight < 8)) fight = 7;
        row += (uint32_t)(fight * 2);
        gaddr at = lots;
        for (int i = 0; i < 0x20; i++) *B(lots + (uint32_t)i) = 0x40;
        int32_t n = S8(row);
        if (n != 0) { n--; do { *B(at++) = 0; n--; } while (n >= 0); }
        n = S8(row + 1);
        if (n != 0) { n--; do { *B(at++) = 0x20; n--; } while (n >= 0); }
        c->r[16] = FIGHTER_1; c->r[17] = FIGHTER_2; c->r[18] = 0x09524A68u; c->r[19] = FIGHTER_3;
        c->r[20] = FIGHTER_4;
        c->r[31] = 0x08837490u; f_08821368(c);
        lot = S8(0x09524A68u + (c->r[2] & 0x1F) + 6);
    }
    int8_t a = S8(FIGHTER_1 + 373), b = S8(FIGHTER_2 + 373), d = S8(FIGHTER_3 + 373), e = S8(FIGHTER_4 + 373);
    if (a != 0) *B(FIGHTER_1 + 499) = (uint8_t)lot;
    if (b != 0) *B(FIGHTER_2 + 499) = (uint8_t)lot;
    if (d != 0) *B(FIGHTER_3 + 499) = (uint8_t)lot;
    if (e != 0) *B(FIGHTER_4 + 499) = (uint8_t)lot;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, other): whether the other's coming attack would reach the fighter: from the other's
   animation frame (+28) on to the first frame with an attack box (the box list +416's +16, 12 bytes
   each, +1 the box number); a frame flagged 0xC0 (+1) before one, or a box (+224, 32 bytes each, its
   reach turned by +11) more than 96 away, hands over to 0x08876F44; within reach: whether the other's
   +551 is clear. */
SFA_FUNC(08876F78, attack_reach_check) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr frame = *W(b + 28), boxes = *W(*W(b + 416) + 16);
    *W(c->r[29] + 16) = ra;
    for (;;) {
        uint32_t box = *B(boxes + *H(frame + 8) * 12u + 1);
        if (box != 0) {
            int32_t reach = S16(*W(b + 224) + (box << 5));
            if (*B(b + 11) != 0) reach = -reach;
            uint32_t gap = (((uint32_t)reach + (uint32_t)S16(b + 18) - (uint32_t)S16(a + 18)) + 0x60) & 0xFFFF;
            if (gap < 193) c->r[2] = S8(b + 551) == 0;
            else { c->r[6] = b; ARG(0) = a; ARG(1) = b; c->r[31] = 0x08877020u; f_08876F44(c); }
            break;
        }
        if (S8(frame + 1) & 0xC0) {
            c->r[6] = b; ARG(0) = a; ARG(1) = b; c->r[31] = 0x08877090u; f_08876F44(c);
            break;
        }
        frame += (uint32_t)(S16(frame + 6) * 10);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (bar): the icons of a bar's row: for each of BATTLE + 72 + 1 entries of the list 0x09517988 the
   blank 0x1EF, or for those 0x08830384 passes for the player's fighter (the first while GAME + 57 bit
   0, else the second), up to the bar's +82 of them, the icon from 0x08A2B190, into the list at
   0x08A282C6 (+4, 6 bytes each); returns how many passed beyond those shown. A 0x40 frame keeping
   s0..s7 and fp, the bar at +20, the fighter at +16. */
SFA_FUNC(0889F35C, bar_icons_fill) {
    uint32_t sv[10], bar = ARG(0);
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 8; i++) *W(sp + 24 + 4 * i) = sv[i];
    *W(sp + 56) = sv[8]; *W(sp + 60) = sv[9];
    *W(sp + 20) = bar;
    *W(sp + 16) = (S8(GAME + 57) & 1) ? FIGHTER_1 : FIGHTER_2;
    uint32_t i = 0, passed = 0, shown = 0, left = (uint16_t)S16(BATTLE + 72), was;
    gaddr out = 0x08A282C6u;
    c->r[30] = 0x09517988u; c->r[23] = 0x08A2B190u;
    do {
        uint32_t kind = *B(0x09517988u + i), icon = 0x1EF;
        i = (i + 1) & 0xFFFF;
        c->r[16] = kind; c->r[17] = icon; c->r[18] = left; c->r[19] = out; c->r[20] = i; c->r[21] = shown;
        c->r[22] = passed;
        ARG(0) = kind; ARG(1) = *W(sp + 16);
        c->r[31] = 0x0889F410u; f_08830384(c);
        if (c->r[2] != 0) {
            passed = (passed + 1) & 0xFFFF;
            if ((int32_t)shown < S16(*W(sp + 20) + 82)) {
                icon = *H(0x08A2B190u + 2 * kind);
                shown = (shown + 1) & 0xFFFF;
            }
        }
        *H(out + 4) = (uint16_t)icon;
        was = left; left = (left - 1) & 0xFFFF; out += 6;
    } while (was != 0);
    c->r[2] = (passed - shown) & 0xFFFF;
    for (int k = 0; k < 8; k++) c->r[16 + k] = sv[k];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x40;
}
