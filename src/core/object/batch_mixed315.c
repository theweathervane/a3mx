/* Middle-sized ones whose callees are all done: who leads on vitality with a handicap, the combo
   count's text beside a fighter, how long a block's push lasts, a round's winner in the fights
   through a list of opponents, a result's place in a ranking, and three sets of objects. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_0880A6CC(Cpu *c); void f_08821368(Cpu *c); void f_08861724(Cpu *c); void f_08822EAC(Cpu *c);
void f_08825BE0(Cpu *c); void f_088212F4(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* 0x0880A6CC: (text, 1, x, 0, e). */
static void text_at(Cpu *c, uint32_t text, uint32_t x, uint32_t e, uint32_t back) {
    ARG(0) = text; ARG(1) = 1; ARG(2) = x; ARG(3) = 0; c->r[8] = e;
    c->r[31] = back; f_0880A6CC(c);
}

/* 1 when the left is ahead, 2 when the right is, 3 when level. */
static uint8_t lead(int32_t left, int32_t right) { return left == right ? 3 : right < left ? 1 : 2; }

/* (-, y): who leads on vitality, into y's +12, by the kind of fight (y + 56). 0: fighter 1 against
   2; in mode 7 (y + 79) with y's +109 set a draw goes to whichever of them is the cpu's opponent
   (+373), and a lead the cpu has is turned over. 1 and 2: twice fighter 1 (or 2) less fighter 3
   (in mode 7 with +109 set less 0x90 instead) against fighter 4. 3: fighters 1 and 2 less fighter
   3 against fighter 3. Others: nothing. */
SFA_FUNC(0882AC40, vitality_lead_handicap) {
    gaddr y = ARG(1);
    int32_t kind = S16(y + 56);
    if (kind < 2) c->r[2] = (uint32_t)kind;
    if (kind < 0 || kind > 3) return;
    if (kind == 3) {
        int32_t hp3 = S16(FIGHTER_3 + 112);
        int32_t rest = (int16_t)(S16(FIGHTER_1 + 112) + S16(FIGHTER_2 + 112) - hp3);
        *B(y + 12) = lead(rest, hp3);
        return;
    }
    int32_t mode = S8(y + 79);
    if (kind == 0) {
        int32_t hp1 = S16(FIGHTER_1 + 112), hp2 = S16(FIGHTER_2 + 112);
        uint8_t who = lead(hp1, hp2);
        *B(y + 12) = who;
        if (mode != 7 || S8(y + 109) == 0) return;
        if (who == 3) *B(y + 12) = S8(FIGHTER_1 + 373) != 0 ? 1 : 2;
        else if (who == 1) { if (S8(FIGHTER_1 + 373) == 0) *B(y + 12) = 2; }
        else if (S8(FIGHTER_2 + 373) == 0) *B(y + 12) = 1;
        return;
    }
    int32_t mine = (int16_t)(S16((kind == 1 ? FIGHTER_1 : FIGHTER_2) + 112) << 1);
    int32_t hp4 = S16(FIGHTER_4 + 112);
    if (mode == 7 && S8(y + 109) != 0) mine = (int16_t)(mine - 0x90);
    else mine = (int16_t)(mine - S16(FIGHTER_3 + 112));
    *B(y + 12) = lead(mine, hp4);
}

/* (fighter, x, y): the combo count's text for a fighter whose +412 is set, from its side's record
   (0x09517A28, or 0x09517A50 for the second side): while the record's place (+1) has reached its
   length (+0), or in the first quarter of each 256 frames, the plain text 0x089FE2A4 at y; else the
   text 0x089FE2AC at x (moved for the first side by a byte from 0x08A0F7C0) and the entry's own
   text from the table 0x08A0F718 0x24 further on. A 0x30 frame keeping s0, s1. */
SFA_FUNC(08832728, combo_text) {
    gaddr f = ARG(0);
    int32_t x = (int16_t)ARG(1), y = (int16_t)ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0; c->r[16] = (uint32_t)x;
    *W(sp + 36) = s1; *W(sp + 40) = ra;
    if (S8(f + 412) != 0) {
        gaddr rec;
        if (S8(f + 337) == 0) {
            rec = 0x09517A28u;
            int32_t at = (int16_t)(S8(rec + 1) + 1);
            x = (int16_t)(x + S8(0x08A0F7C0u + (uint32_t)(int32_t)S8(rec + (uint32_t)at + 3)));
            c->r[16] = (uint32_t)x;
        } else rec = 0x09517A50u;
        int32_t length = S8(rec) + 1;
        int32_t at = (int16_t)(S8(rec + 1) + 1);
        c->r[17] = (uint32_t)at;
        if (!(at < length)) text_at(c, 0x089FE2A4u, (uint32_t)y, 4, 0x088327E8u);
        else {
            int32_t entry = S8(rec + (uint32_t)at + 3);
            c->r[17] = (uint32_t)entry;
            if (*B(GAME + 32) < 64) text_at(c, 0x089FE2A4u, (uint32_t)y, 4, 0x08832834u);
            else {
                text_at(c, 0x089FE2ACu, (uint32_t)x, 5, 0x08832854u);
                text_at(c, *W(0x08A0F718u + (uint32_t)(entry << 2)), (uint32_t)(int32_t)(int16_t)(x + 0x24), 1,
                        0x08832884u);
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): a block lands. +556 is kept at +646, +699, +700, +555 and +557 are cleared and the
   count +649 goes up. A score is made of the character's byte (0x08A191F0 by its class from
   0x08A213F4), the byte from 0x08A191F6 by +650 (8 from 16 on), less 1, 2 or 4 for being one, two
   or more blocks ahead of the opponent, and a random byte of the 32 at 0x08A19206. By the score
   the push +816 lasts 0x3C (below 3), 0x42 (3), 0x48 (4 to 6), 0x4E (7) or 0x54 (8 on, or below
   0), twice that when the opponent's +790 is 3; and outside mode 2 (GAME + 114) +556 comes down by
   4, 8, 0xC, 0x10 or 0x14 and is 16 at least. Then the block's animation: 0x12 in the air, 0xF
   crouching, 0xC standing. A frame keeping s0..s2. */
SFA_FUNC(0885CA48, block_lands) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1;
    gaddr other = *W(o + 92);
    c->r[17] = other;
    *B(o + 646) = (uint8_t)S8(o + 556);
    int32_t count = S8(o + 649);
    *B(o + 699) = 0; *B(o + 700) = 0; *B(o + 555) = 0; *B(o + 557) = 0;
    *B(o + 649) = (uint8_t)(count + 1);
    int32_t score = S8(0x08A191F0u + (uint32_t)(int32_t)S8(0x08A213F4u + (uint32_t)(int32_t)S8(o + 338)));
    int32_t k = S8(o + 650);
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    score += k < 16 ? S8(0x08A191F6u + (uint32_t)k) : 8;
    int32_t ahead = S8(o + 649) - S8(other + 649);
    int32_t less = ahead <= 0 ? 0 : !((uint32_t)ahead < 3u) ? -4 : (uint32_t)ahead < 2u ? -1 : -2;
    c->r[18] = (uint32_t)(score + less);
    ARG(0) = (uint32_t)score;
    c->r[31] = 0x0885CB08u; f_08821368(c);
    int32_t v = score + less + S8(0x08A19206u + (c->r[2] & 0x1F));
    int32_t cost;
    uint32_t push;
    if (v < 0 || !(v < 8)) { cost = 0x14; push = 0x54; }
    else if (!(v < 7)) { cost = 0x10; push = 0x4E; }
    else if (!(v < 4)) { cost = 0xC; push = 0x48; }
    else if (!(v < 3)) { cost = 8; push = 0x42; }
    else { cost = 4; push = 0x3C; }
    *W(o + 816) = push;
    if (S8(other + 790) == 3) *W(o + 816) = *W(o + 816) << 1;
    if (S8(GAME + 114) != 2) {
        *B(o + 556) = (uint8_t)(S8(o + 556) - cost);
        if (S8(o + 556) < 16) *B(o + 556) = 0x10;
    }
    uint32_t anim = S8(o + 73) != 0 ? 0x12 : S8(o + 696) == 0 ? 0xC : 0xF;
    call2(c, o, anim, f_08861724, 0x0885CC04u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (owner): four objects of kind 0x15 / 3 (0x08822EAC) for the owner, with its +337 and +164 and
   the times 0x10, 0xC, 8 and 4. A frame keeping s0..s2. */
SFA_FUNC(0890E558, four_timed_objects) {
    static const uint32_t back[4] = { 0x0890E584u, 0x0890E5D0u, 0x0890E61Cu, 0x0890E668u };
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; *W(sp + 24) = s2;
    c->r[18] = 0x15; c->r[17] = 3;
    *W(sp + 28) = ra;
    for (uint32_t i = 0; i < 4; i++) {
        call2(c, 0x15, 3, f_08822EAC, back[i]);
        gaddr n = c->r[2];
        if (n == 0) continue;
        *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x15; *B(n + 3) = 3;
        *W(n + 88) = o;
        *H(n + 82) = (uint16_t)(0x10 - 4 * i);
        *H(n + 86) = (uint16_t)S8(o + 337);
        *W(n + 164) = *W(o + 164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (owner): seven objects of kind 0x15 (0x08822EAC with 8, 9, 4, 5, 6, 7 and 10) for the owner, with
   its +337 and +164, +78 = -1 and their own +9, +10 and facing. A frame keeping s0, s1. */
SFA_FUNC(089140AC, seven_objects_15) {
    static const struct { uint8_t sub, b9, b10, facing; uint32_t back; } t[7] = {
        { 8, 1, 3, 1, 0x089140D0u }, { 9, 1, 4, 0, 0x08914128u }, { 4, 1, 2, 1, 0x08914180u },
        { 5, 1, 1, 1, 0x089141D8u }, { 6, 2, 2, 0, 0x0891422Cu }, { 7, 2, 3, 0, 0x08914284u },
        { 10, 2, 5, 0, 0x089142E0u },
    };
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; c->r[17] = 0x15;
    *W(sp + 24) = ra;
    for (uint32_t i = 0; i < 7; i++) {
        call2(c, 0x15, t[i].sub, f_08822EAC, t[i].back);
        gaddr n = c->r[2];
        if (n == 0) continue;
        *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x15;
        *B(n + 9) = t[i].b9; *B(n + 10) = t[i].b10;
        *B(n + 78) = 0xFF;
        *B(n + 11) = t[i].facing;
        *W(n + 88) = o;
        *H(n + 86) = (uint16_t)S8(o + 337);
        *W(n + 164) = *W(o + 164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (owner): five objects of kind 0x72 (0x08822EAC with 0x12, 0xE, 0xF, 0x10 and 0x11) for the owner,
   facing right, with +164 = 1, the side of the owner's opponent at +86, and their own +9, +10 and
   +78. A frame keeping s0..s2. */
SFA_FUNC(08982B1C, five_objects_72) {
    static const struct { uint8_t sub, b9, b10, b78; uint32_t back; } t[5] = {
        { 0x12, 2, 2, 3, 0x08982B48u }, { 0xE, 2, 3, 4, 0x08982B9Cu }, { 0xF, 2, 6, 8, 0x08982BF4u },
        { 0x10, 3, 1, 1, 0x08982C4Cu }, { 0x11, 3, 4, 5, 0x08982CA0u },
    };
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; *W(sp + 24) = s2;
    gaddr other = *W(o + 92);
    c->r[18] = other; c->r[17] = 0x72;
    *W(sp + 28) = ra;
    for (uint32_t i = 0; i < 5; i++) {
        call2(c, 0x72, t[i].sub, f_08822EAC, t[i].back);
        gaddr n = c->r[2];
        if (n == 0) continue;
        *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x72;
        *B(n + 9) = t[i].b9; *B(n + 10) = t[i].b10; *B(n + 78) = t[i].b78;
        *B(n + 11) = 1;
        *W(n + 88) = o;
        int32_t side = S8(other + 337);
        *W(n + 164) = 1;
        *H(n + 86) = (uint16_t)side;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* The round's winner and loser are kept (y + 112, + 116, their characters at + 50 and + 51) and
   marked 1 and -1 at +1172. */
static void keep_result(gaddr y, gaddr winner, gaddr loser) {
    *W(y + 112) = winner; *W(y + 116) = loser;
    *B(y + 50) = (uint8_t)S8(winner + 338);
    *B(y + 51) = (uint8_t)S8(loser + 338);
    *B(winner + 1172) = 1;
    *B(loser + 1172) = 0xFF;
}

/* (-, y): a round's result (y + 12: 1 or 2 the winner, 3 a draw) in the fights that run through a
   list of opponents on each side (the records 0x09517A28 and 0x09517A50: the length, the place,
   and the rounds at +2). y's +83 is cleared.
   A win: the result is kept, y's +83 is -1 for a winner on full vitality (0x90), the winner's
   rounds (+374) and its record's go up, and when that record's rounds pass the other's length the
   fight is over (y's +5 = -1, +32 cleared).
   A draw: a side whose place has reached its length is out. With one out the other (as fighter 2
   and 1 go) wins: its wins (+426, 99 at most) are kept at +451 and the loser's cleared; with both
   out both are cleared; either way the fight is over and the result kept. Then both fighters'
   rounds go up and are shown (0x08825BE0), and both records' rounds. A frame keeping s0..s2. */
SFA_FUNC(088259E8, list_fight_round) {
    const gaddr rec1 = 0x09517A28u, rec2 = 0x09517A50u;
    gaddr y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    int32_t win = S8(y + 12);
    *W(sp + 20) = s1; *W(sp + 24) = s2;
    *B(y + 83) = 0;
    *W(sp + 28) = ra;
    gaddr a = FIGHTER_1, b = FIGHTER_2;
    c->r[16] = FIGHTER_2; c->r[17] = rec1; c->r[18] = rec2;
    if (win == 3) {
        uint32_t out = 0;
        if (!(S8(rec1 + 1) < S8(rec1))) out = 1;
        if (!(S8(rec2 + 1) < S8(rec2))) out |= 2;
        if (out != 0) {
            *B(y + 5) = 0xFF;
            if (out == 3) { *H(FIGHTER_2 + 426) = 0; *H(FIGHTER_1 + 426) = 0; }
            else {
                if (out & 1) { a = FIGHTER_2; b = FIGHTER_1; }
                int32_t wins = S16(a + 426);
                if (wins < 99) {
                    *H(a + 426) = (uint16_t)(wins + 1);
                    wins = S16(a + 426);
                }
                *B(a + 451) = (uint8_t)wins;
                *H(b + 426) = 0;
            }
            keep_result(y, a, b);
        }
        c->r[16] = b;
        *B(a + 374) = (uint8_t)(S8(a + 374) + 1);
        *B(b + 374) = (uint8_t)(S8(b + 374) + 1);
        call2(c, a, (uint32_t)S8(a + 374) & 0xFF, f_08825BE0, 0x08825B00u);
        call2(c, b, (uint32_t)S8(b + 374) & 0xFF, f_08825BE0, 0x08825B10u);
        *B(rec1 + 2) = (uint8_t)(S8(rec1 + 2) + 1);
        *B(rec2 + 2) = (uint8_t)(S8(rec2 + 2) + 1);
    } else {
        if ((win & 1) == 0) { a = FIGHTER_2; b = FIGHTER_1; }
        keep_result(y, a, b);
        if (S16(a + 112) == 0x90) *B(y + 83) = 0xFF;
        int32_t side = S8(a + 337);
        *B(a + 374) = (uint8_t)(S8(a + 374) + 1);
        gaddr mine = side == 0 ? rec1 : rec2, theirs = side == 0 ? rec2 : rec1;
        *B(mine + 2) = (uint8_t)(S8(mine + 2) + 1);
        if (!(S8(mine + 2) < S8(theirs) + 1)) { *B(y + 5) = 0xFF; *B(y + 32) = 0; }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* Is this the character, or the other form of the same one (4 and 0x12, 6 and 0x14)? */
static int same_character(int32_t a, int32_t b) {
    return a == b || (a == 4 && b == 0x12) || (a == 0x12 && b == 4) || (a == 6 && b == 0x14) || (a == 0x14 && b == 6);
}

/* (x): x's result goes into a ranking of 38 entries of 20 bytes (the score at +12, the character
   at +8): the table for x's +450 (760 bytes each from 0x08A06688), or the one at 0x08A06F70 when
   the byte x's +784 points at is set. The character's own entry (the lowest one, if it has one) is
   where the search starts; a score (0x088212F4 of x's +451) below that entry's starts from below
   the table instead and leaves the place 0x27. Entries the score is not below move down one, and
   the new one is written: 0x088212F4 of x's +452, the letters 0x43, 0x41, 0x50, x's +448..+450 and
   the score. The place, two more than where it stopped, goes to x's +461. A 0x30 frame keeping
   s0..s4. */
SFA_FUNC(088932A4, ranking_insert) {
    gaddr x = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = x;
    gaddr table = 0x08A05AA8u + 0xBE0 + (uint32_t)(S8(x + 450) * 760);
    *W(sp + 28) = sv[3];
    int32_t ch = S8(x + 448);
    gaddr flag = *W(x + 784);
    *W(sp + 24) = sv[2]; *W(sp + 20) = sv[1]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    uint32_t raw = (uint32_t)(int32_t)S8(x + 451);
    if (flag != 0 && S8(flag) != 0) table = 0x08A05AA8u + 0x14C8;
    gaddr at = table + 0x2E4;
    int32_t place = 0x25;
    for (;;) {
        if (same_character(ch, S8(at + 8))) break;
        int32_t was = place;
        place = (int16_t)(place - 1);
        at -= 20;
        if (was == 0) { place = 0x26; break; }
    }
    c->r[17] = raw; c->r[18] = (uint32_t)place; c->r[19] = table; c->r[20] = at;
    c->r[6] = 4; c->r[7] = 0x12; c->r[8] = 6; c->r[9] = 0x14;
    call2(c, raw, (uint32_t)(int32_t)S8(at + 8), f_088212F4, 0x0889339Cu);
    int32_t score = (int32_t)c->r[2];
    c->r[17] = (uint32_t)score;
    if (score < S16(at + 12)) { place = 0x26; at = table + 0x2F8; }
    place = (int16_t)(place - 1);
    if (place == 0x25) { *B(x + 461) = 0x27; goto out; }
    if (!(place < 0)) {
        gaddr up = at - 20;
        for (;;) {
            if (score < S16(up + 12)) break;
            *W(at) = *W(up);
            *B(at + 4) = *B(up + 4); *B(at + 5) = *B(up + 5); *B(at + 6) = *B(up + 6); *B(at + 7) = *B(up + 7);
            *B(at + 8) = *B(up + 8);
            *B(at + 15) = *B(up + 15);
            *B(at + 9) = *B(up + 9);
            *B(at + 10) = *B(up + 10);
            *H(at + 12) = *H(up + 12);
            int32_t was = place;
            at = up;
            place = (int16_t)(place - 1);
            up = at - 20;
            if (was == 0) break;
        }
    }
    c->r[18] = (uint32_t)place; c->r[20] = at;
    ARG(0) = *W(x + 452);
    c->r[31] = 0x0889345Cu; f_088212F4(c);
    *W(at) = c->r[2];
    *B(at + 4) = 0x43; *B(at + 5) = 0x41; *B(at + 6) = 0x50;
    *B(at + 8) = (uint8_t)S8(x + 448);
    *B(at + 9) = (uint8_t)S8(x + 449);
    *H(at + 12) = (uint16_t)score;
    *B(at + 10) = (uint8_t)S8(x + 450);
    *B(x + 461) = (uint8_t)(place + 2);
out:
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}
