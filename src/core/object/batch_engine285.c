/* The fighter's own frame: the top of a fighter object each frame (0x08859750), its round start
   (0x08865290) with the handicaps of the mode (0x08865540) and the side's numbers (0x0886499C), and the
   running step's bookkeeping: the timers that count down every frame (0x08865DDC) and the dizzy wait
   that ends in the fall (0x0886625C). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define EDIT      0x09517A78u   /* the edited fighter's settings */
#define TOUR      0x09358410u   /* the world tour's record */

void f_08859650(Cpu *c); void f_08865290(Cpu *c); void f_08859964(Cpu *c); void f_0886EDA8(Cpu *c);
void f_08863918(Cpu *c); void f_0885DFB0(Cpu *c); void f_0887B7FC(Cpu *c); void f_0887BB6C(Cpu *c);
void f_08865DDC(Cpu *c); void f_08859C10(Cpu *c); void f_0887C408(Cpu *c); void f_0887E884(Cpu *c);
void f_0884A264(Cpu *c); void f_088D5E44(Cpu *c); void f_08865BEC(Cpu *c); void f_08864BBC(Cpu *c);
void f_08864BB0(Cpu *c); void f_088597F0(Cpu *c); void f_08865540(Cpu *c); void f_08862D88(Cpu *c);
void f_0886499C(Cpu *c); void f_08864D70(Cpu *c); void f_08864BC8(Cpu *c); void f_088374EC(Cpu *c);
void f_08864D40(Cpu *c); void f_08864B98(Cpu *c); void f_0886CD5C(Cpu *c); void f_0886469C(Cpu *c);
void f_088597AC(Cpu *c); void f_088644F4(Cpu *c); void f_0886163C(Cpu *c); void f_0885F79C(Cpu *c);
void f_088598F0(Cpu *c); void f_0886625C(Cpu *c); void f_088599EC(Cpu *c); void f_08863D70(Cpu *c);
void f_08859AD8(Cpu *c); void f_0886CA2C(Cpu *c); void f_08859B44(Cpu *c); void f_0885C2B4(Cpu *c);
void f_088261E0(Cpu *c); void f_0887E868(Cpu *c);

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

/* The change a fighter's choice (+386) makes to its limit +556: 8 more for -1, else 8 less, and 24 less
   for character 27 with a choice above 0. */
static int32_t limit_change(gaddr o) {
    int32_t choice = S8(o + 386);
    if (choice == -1) return 8;
    if (choice != 0 && S8(o + 338) == 0x1B) return -24;
    return -8;
}

/* (fighter, halve): the fighter's limit (+556): the character's (table 0x08A213CA) changed by the
   choice, and halved when asked. No frame. */
SFA_FUNC(0884A264, fighter_limit_set) {
    gaddr o = ARG(0);
    *B(o + 556) = (uint8_t)S8(0x08A213CAu + (uint32_t)(int32_t)S8(o + 338));
    int32_t change = limit_change(o);
    *B(o + 556) = (uint8_t)(S8(o + 556) + change);
    if (ARG(1) != 0) *B(o + 556) = (uint8_t)(S8(o + 556) >> 1);
}

/* (fighter, record): a tenth of the record's +44 (ten more with the fighter's flag 0x8000) as that
   many counts: each one up in the three counters at +804, which one by the table 0x08A190E8. No
   frame. */
SFA_FUNC(088598F0, fighter_counts_add) {
    gaddr o = ARG(0);
    int32_t n = S16(ARG(1) + 44);
    if (n == 0) return;
    if (*W(o + 792) & 0x8000) n += 10;
    n /= 10;
    for (int32_t i = 0; i < n; i++) {
        gaddr at = o + (uint32_t)(S8(0x08A190E8u + (uint32_t)i) * 2) + 804;
        *H(at) = (uint16_t)(S16(at) + 1);
    }
}

/* (fighter): shares a pair member's +252 with its partner: the partner's +366 comes down to it. The
   pair is the third and fourth fighters in line-ups 1 and 2, the first two in line-up 3. No frame. */
SFA_FUNC(088599EC, pair_share_252) {
    gaddr o = ARG(0), to;
    int32_t lineup = S16(BATTLE + 56), side;
    if (lineup == 1 || lineup == 2) {
        side = S8(o + 337);
        if (!(side & 2)) return;
        to = side == 2 ? FIGHTER_4 : FIGHTER_3;
    } else if (lineup == 3) {
        side = S8(o + 337);
        if (side & 2) return;
        to = side != 0 ? FIGHTER_1 : FIGHTER_2;
    } else return;
    int32_t v = S16(o + 252);
    if (v < S16(to + 366)) *H(to + 366) = (uint16_t)v;
}

/* (fighter, other): the dizzy fighter falls: its waits (+628, +629, +639) cleared, state 1 / 1 / 0 /
   0 with +117 = 13, +698 set (and +702 unless +810 is 7), facing as the other faces (+11 into +134),
   the cue 3 (0x0887C408), and for character 38 with +820 and +236 = 4 the cue 0x217. */
SFA_FUNC(08859B44, dizzy_fall) {
    gaddr o = ARG(0), other = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 628) = 0; *B(o + 629) = 0; *B(o + 639) = 0;
    *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 117) = 0xD;
    int32_t kind = S8(o + 810);
    *W(c->r[29] + 16) = s0;
    *B(o + 698) = 1;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (kind != 7) *B(o + 702) = 1;
    *B(o + 699) = 0; *B(o + 700) = 0;
    *B(o + 134) = *B(other + 11);
    *B(o + 119) = 0; *H(o + 38) = 0;
    c->r[6] = 3;
    call2(c, o, (uint32_t)(int32_t)S8(o + 337), f_0887C408, 0x08859BC4u);
    if (S8(o + 338) == 0x26 && *W(o + 820) != 0 && S8(o + 236) == 4) {
        c->r[6] = 0x217;
        call2(c, o, (uint32_t)(int32_t)S8(o + 337), f_0887C408, 0x08859C00u);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (...): 0x0887E884 behind a frame of its own. */
SFA_FUNC(0887E868, call_0887E884) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x0887E878u; f_0887E884(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the side's numbers at a round's start: +121, +122 and +263 by line-up and place (tables
   0x08A195F2, 0x08A19602, 0x08A19612), the character's +701 (0x08A213A0, halved with +308), the limit
   +556 (as 0x0884A264, 8 less with the flag 0x200, halved with +308; 16 for a cpu fighter against an
   edited one of kind 3 while GAME + 137), +232 (0x08A2019C) and the two tables at +704 / +708 by the
   choice (+386); in mode 2, for a side not present when 0x0950FF69 is 2, the limit halved. A frame
   with only the return address. */
SFA_FUNC(0886499C, fighter_side_numbers) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t side = S8(o + 337);
    uint32_t at = (uint32_t)((S16(BATTLE + 56) << 2) + side);
    *B(o + 121) = (uint8_t)S8(0x08A195F2u + at);
    *B(o + 122) = (uint8_t)S8(0x08A19602u + at);
    int32_t who = S8(o + 338);
    *B(o + 263) = (uint8_t)S8(0x08A19612u + at);
    *B(o + 701) = (uint8_t)S8(0x08A213A0u + (uint32_t)who);
    int32_t half = S8(o + 308);
    *B(o + 556) = (uint8_t)S8(0x08A213CAu + (uint32_t)who);
    int32_t choice = S8(o + 386);
    *W(c->r[29] + 16) = ra;
    if (half != 0) *B(o + 701) = (uint8_t)(*B(o + 701) >> 1);
    int32_t change = limit_change(o);
    if (*W(o + 792) & 0x200) change -= 8;
    *B(o + 556) = (uint8_t)(S8(o + 556) + change);
    if (half != 0) *B(o + 556) = (uint8_t)(S8(o + 556) >> 1);
    if (S8(GAME + 137) != 0 && S8(EDIT + 913) == 3 && S8(o + 373) != 0) *B(o + 556) = 0x10;
    *H(o + 232) = (uint16_t)(int16_t)S8(0x08A2019Cu + (uint32_t)who);
    if (choice == 0) { *W(o + 704) = 0x08A201C6u; *W(o + 708) = 0x08A203BEu; }
    else if (choice == -1) { *W(o + 704) = 0x08A20316u; *W(o + 708) = 0x08A20E3Eu; }
    else { *W(o + 704) = 0x08A2026Eu; *W(o + 708) = 0x08A208FEu; }
    if (S8(GAME + 114) == 2 && !(S8(GAME + 53) & (1 << (side & 31))) && *B(0x0950FF69u) == 2)
        call2(c, o, 1, f_0884A264, 0x08864B8Cu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the handicaps of a round's start: the flags (+792), the four rates +796 to +802 (100
   each), the counters +804 to +808 and their kin cleared; then by mode. A player's own record
   (+784, in the tour's record by side) is found for the first two fighters. While GAME + 137 (an
   edited fighter): its side's settings from EDIT, the others only its kind (+790). Modes 13 and 19:
   the rates by the tour record's levels (tables 0x08A197AA / 0x08A197B6). Team modes (8, 15, the 7
   and 16 stages listed) give the pair's two the rate 150 at +800, the solo ones (17 and the others
   listed) give it to the single fighter. Then, for a player's fighter (or modes 1, 2, 6), a record
   in use gives the flags and the rates (half the record's, plus 75; ten more with flag 0x8000), its
   counts (0x088598F0), and with +2 = -1 and flag 0x10000000 the +812 = 3 and +366 = 0x90; over the
   network or with GAME + 186 the session's flags (GAME + 176) and rates (GAME + 180 / 182, percent);
   the record's flags set +327, +320, +325 and +326; in line-ups 1 and 2 the flag 0x2000 (unless
   0x800020, or mode 19) and, in mode 13, 0x800 for the single fighter. A 0x30 frame keeping s0..s6. */
SFA_FUNC(08865540, fighter_handicaps_set) {
    gaddr o = ARG(0);
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    *W(o + 792) = 0; *B(o + 789) = 0; *B(o + 790) = 0; *B(o + 810) = 0;
    *W(sp + 28) = sv[3];
    *B(o + 791) = 0;
    *H(o + 796) = 100; *H(o + 798) = 100; *H(o + 800) = 100; *H(o + 802) = 100;
    *H(o + 804) = 0; *H(o + 806) = 0; *H(o + 808) = 0;
    *B(o + 812) = 0; *B(o + 327) = 0; *B(o + 320) = 0;
    int32_t side = S8(o + 337);
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    *B(o + 325) = 0;
    *W(sp + 36) = sv[5];
    *B(o + 321) = 0;
    *W(sp + 32) = sv[4]; *W(sp + 40) = sv[6]; *W(sp + 44) = sv[7];
    int pair = side & 2;
    gaddr rec = sv[4];   /* a pair member goes on with whatever s4 held */
    if (!pair) { rec = TOUR + 0x118 + (uint32_t)(side * 0x354); *W(o + 784) = rec; }
    int32_t mode;
    if (S8(GAME + 137) != 0) {
        uint32_t bit = (uint32_t)(int32_t)S8(BATTLE + 156) & (1u << (side & 31));
        if (bit == 0) *B(o + 790) = (uint8_t)S8(EDIT + 913);
        else {
            c->r[16] = o; c->r[17] = BATTLE; c->r[18] = EDIT; c->r[19] = 100; c->r[20] = rec; c->r[21] = GAME;
            c->r[22] = 1;
            call2(c, bit, 1u << (side & 31), f_088D5E44, 0x0886562Cu);
            if (c->r[2] != 0) *W(o + 792) = *W(EDIT + 948);
            *H(o + 796) = (uint16_t)S16(0x08A197A0u + (uint32_t)(S8(EDIT + 914) * 2));
            *H(o + 800) = (uint16_t)S16(0x08A197A0u + (uint32_t)(S8(EDIT + 915) * 2));
            *B(o + 789) = (uint8_t)S8(EDIT + 916);
            if (S8(EDIT + 917) == 1) *W(o + 792) |= 8;
            if (S8(EDIT + 918) == 1) *W(o + 792) |= 0x4000;
            if (S8(EDIT + 919) == 1) *W(o + 792) |= 0x10;
            int32_t kind = S8(EDIT + 902);
            if (kind != 4 || S8(o + 337) == 3) *B(o + 810) = (uint8_t)kind;
        }
        goto players;
    }
    mode = S8(GAME + 114);
    if (mode == 0xD || mode == 0x13) {
        gaddr rates = mode == 0xD ? 0x08A197AAu : 0x08A197B6u;
        int32_t lineup = S16(BATTLE + 56);
        int32_t level = pair ? S16(TOUR + (uint32_t)((lineup & 1) * 2) + 84) : S16(TOUR + (uint32_t)(lineup * 2) + 82);
        int16_t rate = S16(rates + (uint32_t)(level * 2));
        *H(o + 800) = (uint16_t)rate; *H(o + 796) = (uint16_t)rate;
        if (pair) goto out;
        goto players;
    }
    if (mode == 1 || mode == 2 || mode == 6) goto record;
    if (mode == 0x10 && S8(BATTLE + 159) == 2) goto record;
    {
        int32_t stage = S8(BATTLE + 79), rule = S8(BATTLE + 159);
        int team = mode == 8 || mode == 0xF || (mode == 7 && (stage == 4 || stage == 5)) ||
                   (mode == 0x10 && (rule == 4 || rule == 3));
        int solo = !team && (mode == 0x11 || (mode == 7 && (stage == 6 || stage == 7)) || (mode == 0x10 && rule == 5));
        if (team) {
            if (!pair) goto players;
            *H(o + 800) = 0x96;
        } else if (solo) {
            if (!pair) { *H(o + 800) = 0x96; goto players; }
        } else {
            if (pair) goto out;
            goto players;
        }
        if (S8(GAME + 191) != 0) {
            uint32_t flags = *W(o + 792);
            *B(o + 789) = 2;
            *W(o + 792) = flags | 0x1000000u;
        }
        goto out;
    }
players:
    if (S8(o + 373) != 0) goto out;
record:
    if (S8(rec) != 0) {
        *W(o + 792) = *W(rec + 28);
        *H(o + 796) = (uint16_t)((S16(rec + 38) >> 1) + 0x4B);
        *H(o + 798) = (uint16_t)((S16(rec + 40) >> 1) + 0x4B);
        *H(o + 800) = (uint16_t)((S16(rec + 42) >> 1) + 0x4B);
        int16_t last = S16(rec + 46);
        uint32_t flags = *W(o + 792);
        *H(o + 802) = (uint16_t)((last >> 1) + 0x4B);
        if (flags & 0x8000) {
            for (uint32_t i = 0; i < 4; i++) *H(o + 796 + 2 * i) = (uint16_t)(S16(o + 796 + 2 * i) + 10);
        }
        call2(c, o, rec, f_088598F0, 0x08865A14u);
        if (S8(rec + 2) == -1 && (*W(o + 792) & 0x10000000u)) { *B(o + 812) = 3; *H(o + 366) = 0x90; }
    }
    if (S8(GAME + 146) != 0 || S8(GAME + 186) != 0) {
        uint32_t flags = *W(o + 792);
        int32_t a = S16(o + 796);
        *W(o + 792) = flags | *W(GAME + 176);
        int32_t v = (int32_t)((uint32_t)a * (uint32_t)(int32_t)S16(GAME + 180)) / 100;
        int32_t b = S16(o + 798), d = S16(o + 800), e = S16(o + 802);
        *H(o + 796) = (uint16_t)v;
        *H(o + 798) = (uint16_t)((int32_t)((uint32_t)b * (uint32_t)(int32_t)S16(GAME + 180)) / 100);
        *H(o + 800) = (uint16_t)((int32_t)((uint32_t)d * (uint32_t)(int32_t)S16(GAME + 182)) / 100);
        *H(o + 802) = (uint16_t)((int32_t)((uint32_t)e * (uint32_t)(int32_t)S16(GAME + 182)) / 100);
    }
    if (S8(rec) != 0) {
        uint32_t flags = *W(o + 792);
        if (flags & 0x08000000u) *B(o + 327) = 1;
        if (flags & 0x80000000u) { *B(o + 320) = 1; *B(o + 326) = 10; }
        if (flags & 0x20000000u) { *B(o + 320) = 1; *B(o + 325) = 1; *B(o + 326) = 10; }
    }
    {
        int32_t lineup = S16(BATTLE + 56);
        if (lineup == 1 || lineup == 2) {
            int32_t m = S8(GAME + 114);
            if (m != 0x13) {
                uint32_t flags = *W(o + 792);
                if (!(flags & 0x00800020u)) { *W(o + 792) = flags | 0x2000; m = S8(GAME + 114); }
            }
            if (m == 0xD && !(S8(o + 337) & 2)) *W(o + 792) |= 0x800;
        }
    }
out:
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x30;
}

/* (fighter): a fighter's round start: its round's fields cleared (+0xC0 to +0x140 and +0x1E0 to
   +0x470; +815 is kept over the network), in use, its place by the side's entry (GAME + 126 into
   0x08A17F98), +589 = 8 and +598 = 30; a player's options copied (+431, +428, +385 into +313, +309,
   +308); then the set-up calls in turn (the handicaps and the side's numbers among them); last the
   entrance: standing (0x0886469C, 0x088597AC) in line-up 3, modes 13 and 17 and the listed stages of
   7 and 16, else 0x088644F4 and, when that gave an entrance (+415), state 1 / 7 with the cue 0x22
   (unless +771) and 0x0885F79C. A 0x30 frame keeping s0..s6. */
SFA_FUNC(08865290, fighter_round_start) {
    gaddr o = ARG(0);
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1];
    c->r[16] = o; c->r[17] = 0;
    *B(o + 80) = 0; *B(o + 414) = 0; *B(o + 356) = 0; *B(o + 136) = 0; *B(o + 123) = 0; *B(o + 125) = 0;
    *H(o + 778) = 0; *B(o + 126) = 0; *B(o + 135) = 0;
    for (uint32_t i = 2; i < 7; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 44) = sv[7];
    *B(o + 73) = 0;
    call1(c, 0, f_088D5E44, 0x088652ECu);
    c->r[18] = GAME; c->r[21] = BATTLE; c->r[20] = 1; c->r[22] = 3;
    if (c->r[2] != 0 && S8(GAME + 146) == 0) c->r[19] = (uint32_t)(int32_t)S8(o + 815);
    for (uint32_t i = 0; i < 32; i++) *W(o + 0xC0 + 4 * i) = 0;
    for (uint32_t i = 0; i < 164; i++) *W(o + 0x1E0 + 4 * i) = 0;
    int32_t side = S8(o + 337);
    *B(o + 1) = 1;
    int16_t place = S16(0x08A17F98u + (uint32_t)*B(GAME + (uint32_t)side + 126) * 2);
    *H(o + 114) = (uint16_t)place; *H(o + 112) = (uint16_t)place;
    *B(o + 589) = 8; *B(o + 598) = 0x1E;
    call2(c, 0x1E, 8, f_088D5E44, 0x08865390u);
    int32_t cpu = S8(o + 373);
    if (c->r[2] != 0 && S8(GAME + 146) == 0) *B(o + 815) = (uint8_t)c->r[19];
    if (cpu == 0) {
        int8_t a = S8(o + 431), b = S8(o + 428);
        *B(o + 313) = (uint8_t)a;
        uint8_t d = *B(o + 385);
        *B(o + 309) = (uint8_t)b; *B(o + 308) = d;
    }
    call1(c, o, f_08865BEC, 0x088653D0u);
    call1(c, o, f_08864BBC, 0x088653D8u);
    call1(c, o, f_08864BB0, 0x088653E0u);
    call1(c, o, f_088597F0, 0x088653E8u);
    call1(c, o, f_08865540, 0x088653F0u);
    call1(c, o, f_08862D88, 0x088653F8u);
    call1(c, o, f_0886499C, 0x08865400u);
    call1(c, o, f_08864D70, 0x08865408u);
    call1(c, o, f_08864BC8, 0x08865410u);
    c->r[31] = 0x08865418u; f_088374EC(c);
    call1(c, o, f_08864D40, 0x08865420u);
    call1(c, o, f_08864B98, 0x08865428u);
    call1(c, o, f_0886CD5C, 0x08865430u);
    int stand = S16(BATTLE + 56) == 3;
    if (!stand) {
        int32_t mode = S8(GAME + 114), rule = S8(BATTLE + 159);
        c->r[18] = (uint32_t)mode;
        stand = mode == 0xD || (mode == 7 && !(S8(BATTLE + 79) < 4)) ||
                (mode == 0x10 && (rule == 4 || rule == 3 || rule == 5)) || mode == 0x11;
    }
    if (!stand) {
        call1(c, o, f_088644F4, 0x088654D8u);
        if (S8(o + 415) == 0) stand = 1;
    }
    if (stand) {
        call1(c, o, f_0886469C, 0x088654C0u);
        call1(c, o, f_088597AC, 0x088654C8u);
    } else {
        *B(o + 4) = 1; *B(o + 5) = 7; *B(o + 6) = 0;
        int32_t quiet = S8(o + 771);
        *B(o + 7) = 0;
        if (quiet == 0) call2(c, o, 0x22, f_0886163C, 0x08865508u);
        call1(c, o, f_088597AC, 0x08865510u);
        call1(c, o, f_0885F79C, 0x08865518u);
    }
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x30;
}

/* The dizzy wait's last part: the count of tries (+628) down; when it runs out the fall, else the
   wait (+629) starts again from +639. */
static void dizzy_try(Cpu *c, gaddr o, gaddr other, uint32_t back) {
    int32_t left = S8(o + 628) - 1;
    *B(o + 628) = (uint8_t)left;
    if ((int8_t)left >= 0) *B(o + 629) = (uint8_t)S8(o + 639);
    else call2(c, o, other, f_08859B44, back);
}

/* (fighter): the dizzy wait (+629), one frame: not while the fight is held (GAME + 38, BATTLE + 13 /
   14), the fighter is not in use, in state 1 / 2 or 3, or +237. While it runs: with no tries left
   (+628) and 24 frames gone, the fall. When it ends: in mode 2 for a side that fights, and against an
   edited kind other than 0 and 4, a try; else the vitality (+112) loses +632, and while some is left a
   try; with none left the knock-out: vitality 0 in mode 2, state 1 / 1 with +117 = 13 and +698,
   0x0885C2B4, the winner's mark (+630, bit 6 for a full bar) into the other side's +248, a fighter
   held (+135) let go into state 1 / 1 / 1, and when 0x088261E0 says the round is over, +630 less 10
   into the other's +552 and BATTLE + 68, with 0x0887E868. A 0x30 frame keeping s0..s5. */
SFA_FUNC(0886625C, dizzy_wait) {
    gaddr o = ARG(0);
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    gaddr other = *W(o + 92);
    c->r[17] = other; c->r[18] = GAME;
    int held = S8(GAME + 38) | (*B(o + 1) == 0);
    *W(sp + 28) = sv[3];
    c->r[19] = BATTLE;
    held |= S8(BATTLE + 14) | S8(BATTLE + 13);
    *W(sp + 16) = sv[0];
    c->r[16] = o;
    *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5]; *W(sp + 40) = sv[6];
    if (held != 0) goto out;
    c->r[20] = 1;
    if (*B(o + 4) == 1 && (*B(o + 5) == 3 || *B(o + 5) == 2)) goto out;
    int32_t wait = S8(o + 629);
    if (S8(o + 237) != 0 || wait == 0) goto out;
    wait--;
    *B(o + 629) = (uint8_t)wait;
    if ((int8_t)wait != 0) {
        if (S8(o + 628) == 0 && !(S8(o + 639) - S8(o + 629) < 24)) call2(c, o, other, f_08859B44, 0x08866574u);
        goto out;
    }
    if (S8(GAME + 114) == 2 && (S8(GAME + 57) & (1 << (S8(o + 337) & 31)))) {
        dizzy_try(c, o, other, 0x08866364u);
        goto out;
    }
    int32_t kind = S8(other + 790);
    if (kind != 0 && kind != 4) { dizzy_try(c, o, other, 0x088663ACu); goto out; }
    int32_t vital = S16(o + 112) - S16(o + 632);
    *H(o + 112) = (uint16_t)vital;
    if (!((int16_t)vital < 0)) { dizzy_try(c, o, other, 0x08866404u); goto out; }
    c->r[21] = 1;
    if (S8(GAME + 114) == 2) *H(o + 112) = 0;
    *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 117) = 0xD; *B(o + 698) = 1;
    call1(c, o, f_0885C2B4, 0x08866448u);
    int32_t mark = S8(o + 630);
    if (S16(other + 112) == 0x90) mark = (int8_t)(mark | 0x40);
    if (S16(BATTLE + 56) == 0) *B(other + 248) = (uint8_t)mark;
    else if (S8(o + 337) & 2) { *B(FIGHTER_1 + 248) = (uint8_t)mark; *B(FIGHTER_2 + 248) = (uint8_t)mark; }
    else { *B(FIGHTER_3 + 248) = (uint8_t)mark; *B(FIGHTER_4 + 248) = (uint8_t)mark; }
    if (S8(o + 135) != 0) {
        *B(other + 4) = 1; *B(other + 5) = 1; *B(other + 6) = 1; *B(other + 7) = 0;
        *B(other + 119) = 0; *B(other + 116) = 2; *B(other + 117) = 0xF;
        *B(o + 135) = 0;
        uint8_t facing = *B(other + 11);
        *B(other + 135) = 0; *B(other + 579) = 0;
        *B(other + 134) = (uint8_t)(facing ^ 1);
        *B(other + 538) = 0;
        *B(o + 698) = 1;
    }
    call2(c, GAME, BATTLE, f_088261E0, 0x08866518u);
    if (c->r[2] != 0) {
        *B(other + 552) = (uint8_t)(S8(o + 630) - 10);
        int32_t m = S8(o + 630);
        *B(BATTLE + 43) = 0;
        *B(BATTLE + 68) = (uint8_t)(m - 10);
        call1(c, o, f_0887E868, 0x08866544u);
    }
out:
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}

/* (fighter): the running fighter's timers, one frame each: the dizzy wait (unless in step 9); +573;
   +788 dropped on the ground; the block bar (+252) refilling every third frame (+284) while it is
   broken (+810 is not 1, the other's vitality is not negative): when full again +251, +284, +366 and
   +502 are cleared, else +366 is brought down to it and shared (0x088599EC); +634, +566 and +543;
   the store +816 (not while +127 or held): 0x08863D70 takes what it can, with +670 = 8 and +669 /
   +605 = 2; +699 (then +700 cleared); +555 (then 0x08859AD8) while alive; +266; +669 (then +670
   cleared, +657 = -1); +605 flashing +268. Then, while the fight runs (BATTLE + 13 / 14 / 7, GAME +
   38, alive): with the flags 0x1080400 the meter gain (0x0886CA2C) every 15 frames (8 with 0x1000000)
   by +776, unless +136 or +313; kinds 5 and 6 (+810) regain a point of vitality every 45 / 30 frames
   (+811, unless +539), 0x90 at most; flag 0x10000000 keeps +366 at 0x90 for +812 times, then 0; in
   modes 13 and 19 with the tour record's +79 by line-up, a single fighter on the ground (+6 is not 8)
   gains meter on GAME + 32 bit 0. The flag 0x4000000 sets +254 every frame. A 0x30 frame keeping
   s0..s4. */
SFA_FUNC(08865DDC, fighter_timers) {
    gaddr o = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1];
    gaddr other = *W(o + 92);
    *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    uint8_t step = *B(o + 5);
    *W(sp + 24) = sv[2];
    *B(o + 694) = 0;
    *W(sp + 16) = sv[0]; *W(sp + 36) = sv[5];
    c->r[16] = o; c->r[17] = other; c->r[18] = 1; c->r[19] = BATTLE; c->r[20] = GAME;
    if (step != 9) call1(c, o, f_0886625C, 0x08865E2Cu);
    int32_t t = S8(o + 573), kind = S8(o + 810);
    if (t != 0) *B(o + 573) = (uint8_t)(t - 1);
    if (*B(o + 788) != 0 && S8(o + 73) == 0) *B(o + 788) = 0;
    if (kind != 1 && S16(o + 252) != 0 && !(S16(other + 112) < 0)) {
        int32_t bar = S16(o + 252);
        int32_t a = S16(o + 286) - 1, b = S16(o + 284) - 1;
        *H(o + 286) = (uint16_t)a; *H(o + 284) = (uint16_t)b;
        int full = 0;
        if ((int16_t)b == 0) {
            *H(o + 284) = 3;
            bar--;
            *H(o + 252) = (uint16_t)bar;
            if ((int16_t)bar == 0) {
                *B(o + 251) = 0; *H(o + 284) = 0; *H(o + 366) = 0; *B(o + 502) = 0;
                full = 1;
            }
        }
        if (!full) {
            bar = S16(o + 252);
            if (bar < S16(o + 366)) {
                *H(o + 366) = (uint16_t)bar;
                call1(c, o, f_088599EC, 0x08865EF4u);
            }
        }
    }
    {
        int32_t a = S8(o + 634), b = S8(o + 566), d = S8(o + 543);
        if (a != 0) *B(o + 634) = (uint8_t)(a - 1);
        if (b != 0) *B(o + 566) = (uint8_t)(b - 1);
        if (d != 0) *B(o + 543) = (uint8_t)(d - 1);
    }
    if (S8(o + 127) == 0 && *W(o + 816) != 0) {
        uint32_t store = *W(o + 816) - 1;
        if (S8(o + 135) == 0) {
            *W(o + 816) = store;
            call1(c, o, f_08863D70, 0x08865F54u);
            uint32_t took = c->r[2];
            if ((took & 0xFFFF) != 0) {
                *B(o + 670) = 8;
                uint32_t now = *W(o + 816);
                *B(o + 669) = 2; *B(o + 605) = 2;
                now -= took;
                if ((int32_t)now < 0) now = 0;
                *W(o + 816) = now;
            }
        }
    }
    {
        uint32_t a = *B(o + 699);
        int32_t vital = S16(o + 112);
        if (a != 0) {
            a--;
            *B(o + 699) = (uint8_t)a;
            if ((a & 0xFF) == 0) *B(o + 700) = 0;
        }
        if (!(vital < 0)) {
            int32_t b = S8(o + 555);
            if (b != 0) {
                b--;
                *B(o + 555) = (uint8_t)b;
                if ((int8_t)b == 0) call1(c, o, f_08859AD8, 0x08865FDCu);
            }
        }
    }
    int32_t t266 = S8(o + 266), t669 = S8(o + 669), flash = S8(o + 605);
    uint32_t flags = *W(o + 792);
    if (t266 != 0) *B(o + 266) = (uint8_t)(t266 - 1);
    if (t669 != 0) {
        t669--;
        *B(o + 669) = (uint8_t)t669;
        if ((int8_t)t669 == 0) { *B(o + 670) = 0; *B(o + 657) = 0xFF; }
    }
    *H(o + 268) = 0;
    if (flash != 0) {
        *B(o + 605) = (uint8_t)(flash - 1);
        *H(o + 268) = (S8(o + 605) & 1) ? 1 : 0xFFFF;
    }
    if ((S8(BATTLE + 14) | S8(BATTLE + 13) | S8(GAME + 38)) != 0 || S16(o + 112) < 0 || S8(BATTLE + 7) != 0)
        goto last;
    c->r[17] = 5;
    if (S8(o + 251) == 0 && (flags & 0x01080400u)) {
        int32_t g = S8(o + 776) - 1;
        *B(o + 776) = (uint8_t)g;
        if (!((int8_t)g > 0)) {
            *B(o + 776) = (flags & 0x01000000u) ? 8 : 0xF;
            if (S8(o + 136) == 0 && S8(o + 313) == 0) {
                call2(c, o, 1, f_0886CA2C, 0x088660F4u);
                flags = *W(o + 792);
            }
        }
    }
    kind = S8(o + 810);
    if ((kind == 5 || kind == 6) && S8(o + 539) == 0) {
        int32_t g = S8(o + 811) - 1;
        *B(o + 811) = (uint8_t)g;
        if (!((int8_t)g > 0)) {
            int32_t vital = S16(o + 112);
            *B(o + 811) = kind == 5 ? 0x2D : 0x1E;
            vital++;
            *H(o + 112) = (uint16_t)vital;
            if (!((int16_t)vital < 145)) *H(o + 112) = 0x90;
        }
    }
    if (flags & 0x10000000u) {
        int32_t n = S8(o + 812);
        if (n == 0) *H(o + 366) = 0;
        else if (S16(o + 366) != 0x90) { *H(o + 366) = 0x90; *B(o + 812) = (uint8_t)(n - 1); }
    }
    {
        int32_t mode = S8(GAME + 114);
        if ((mode == 0xD || mode == 0x13) && S8(TOUR + (uint32_t)(int32_t)S16(BATTLE + 56) + 79) != 0 &&
            !(S8(o + 337) & 2) && S8(o + 251) == 0 && *B(o + 6) != 8 && (*B(GAME + 32) & 1)) {
            call2(c, o, 1, f_0886CA2C, 0x08866228u);
            flags = *W(o + 792);
        }
    }
last:
    if (flags & 0x04000000u) *B(o + 254) = 1;
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* (fighter): the running fighter's frame: a player's input (0x08863918, 0x0885DFB0) or the cpu's
   (0x0887B7FC, then 0x0887BB6C when it says so), the timers, 0x08859C10, and the step's handler
   (+5 into the table 0x08A17FA0). */
SFA_FUNC(08859964, fighter_running) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t cpu = S8(o + 373);
    enter(c, o, sv);
    if (cpu == 0) {
        call1(c, o, f_08863918, 0x08859984u);
        call1(c, o, f_0885DFB0, 0x0885998Cu);
    } else {
        call1(c, o, f_0887B7FC, 0x0885999Cu);
        if (c->r[2] != 0) call1(c, o, f_0887BB6C, 0x088599ACu);
    }
    call1(c, o, f_08865DDC, 0x088599B4u);
    call1(c, o, f_08859C10, 0x088599BCu);
    gaddr handler = *W(0x08A17FA0u + (uint32_t)*B(o + 5) * 4);
    c->r[5] = handler; ARG(0) = o;
    c->r[31] = 0x088599DCu;
    rt_call_indirect(c, handler);
    leave(c, sv);
}

/* (fighter): a fighter object's frame: a player's pad (0x08859650), then the round start while +4
   is 0, else the running frame; 0x0886EDA8 last. */
SFA_FUNC(08859750, fighter_frame) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t cpu = S8(o + 373);
    enter(c, o, sv);
    if (cpu == 0) call1(c, o, f_08859650, 0x08859770u);
    if (*B(o + 4) == 0) call1(c, o, f_08865290, 0x08859784u);
    else call1(c, o, f_08859964, 0x08859794u);
    call1(c, o, f_0886EDA8, 0x0885979Cu);
    leave(c, sv);
}
