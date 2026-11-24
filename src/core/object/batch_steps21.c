/* Steps (0.150.0): the next 2 ready functions of 101 - 200 instructions. A round's results scored (the
   vital bonus by health, time and level, its number at 0x09523C88 - 0x09523C94) and a fighter's flag
   +772 by matchups. Offsets stay numbers until something shows what they are. What the original leaves
   in a0 - t0 at a call is set as the original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define RESULTS   0x09523C88u   /* the round's results: +0 time bonus, +4 vital, +8 level, +12 combo */

void f_088D5E44(Cpu *c);   /* game_network_battle */
void f_08821080(Cpu *c);   /* game_add_score_team */
void f_0882BD04(Cpu *c);   /* fighter_level_by_9 */
void f_088301F4(Cpu *c);   /* value_by_55 */
void f_08855BD8(Cpu *c);   /* intro_owner_by_stage */

static int32_t max8(gaddr a, gaddr b) { int32_t x = S8(a + 357), y = S8(b + 357); return x < y ? y : x; }

/* (game, record): the round's results: +8 = 4, +20 = 0xB4; unless the record's +16, the winner's
   (+112; layout 3 the pair's average, fighter 2 when the game's +57 bit 0 is clear) health as the
   base (0 below 0, 0x90 counts 300), with the game's +137 more for +108 (5 or 60 each by network
   battle, with +79) and 200 for a stage 1 - 6 (0x09517E09); 100 x base the vital bonus, times the
   level (fighter_level_by_9), plus the time bonus (value_by_55, +32) when +137 is clear, added to the
   winner's score (+392 = +352 - +396). The highest combo (+357, by layout and side) at +12, kept in
   the side's record (+784 +36) with +137; with +155 +8 = 2 again. Then the intro's owner. */
SFA_FUNC(0882A5B8, round_results_score) {
    gaddr g = ARG(0), rec = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21], s22 = c->r[22];
    c->r[29] -= 0x30;
    gaddr win = *W(rec + 112);
    *H(g + 8) = 4; *H(g + 20) = 0xB4;
    c->r[19] = 0xB4; c->r[21] = 0x09520000u; c->r[22] = RESULTS; c->r[16] = g; c->r[17] = rec;
    int32_t layout = S16(rec + 56);
    if (S8(rec + 16) == 0) {
        int32_t v = S16(win + 112);
        if (layout == 3) {
            v = (v + S16(FIGHTER1 + 112)) >> 1;
            if ((S8(g + 57) & 1) == 0) win = FIGHTER1;
        }
        if (v < 0) v = 0;
        else if (v == 0x90) v = 0x12C;
        if (S8(g + 137) != 0) {
            if (S8(rec + 79) != 0) {
                f_088D5E44(c);
                int32_t k = S8(rec + 108);
                v += c->r[2] != 0 ? k * 5 : k * 60;
            }
            if ((uint32_t)(S8(0x09517E09u) - 1) < 6u) v += 200;
        }
        *W(RESULTS + 4) = (uint32_t)(v * 100);
        c->r[18] = win; c->r[20] = (uint32_t)v;
        c->r[4] = g; c->r[5] = rec; f_0882BD04(c);
        *W(RESULTS + 8) = c->r[2];
        int64_t m = (int64_t)(int32_t)c->r[2] * v;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        v = (int32_t)c->lo;
        if (S8(g + 137) == 0) {
            c->r[4] = g; c->r[5] = rec; f_088301F4(c);
            int32_t t = (int32_t)c->r[2];
            *W(RESULTS) = (uint32_t)(t * 100);
            if (S8(rec + 32) != 0) v = t + v;
        }
        c->r[4] = (uint32_t)(v * 100); c->r[5] = win; f_08821080(c);
        *W(win + 392) = *W(win + 352) - *W(win + 396);
        layout = S16(rec + 56);
    }
    int32_t combo = S8(win + 357);
    if (layout != 0) {
        int side2 = (S8(win + 337) & 2) != 0;
        if (layout == 3) { if (!side2) combo = max8(FIGHTER0, FIGHTER1); }
        else if (side2) combo = max8(FIGHTER2, FIGHTER3);
    }
    *W(RESULTS + 12) = (uint32_t)combo;
    c->r[4] = (uint32_t)combo;
    if (S8(g + 137) != 0) {
        gaddr side = *W(((S8(g + 53) & 1) ? FIGHTER0 : FIGHTER1) + 784);
        if ((int32_t)*B(side + 36) < combo) *B(side + 36) = (uint8_t)combo;
        if (S8(rec + 155) != 0) { *H(g + 8) = 2; *H(g + 20) = 0xB4; }
        c->r[4] = 2;
    }
    f_08855BD8(c);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21; c->r[22] = s22;
}

/* (fighter): its +772 by matchups: outside the cases below, a human in routine 1 / 4 against a CPU, or
   a CPU in routine 1 / 5 or 1 / 6 against a human, looks up the pair in the attacker's list
   (0x08A21778 by character: opponent, value; ending at bit 7) and takes the value through 0x08A1988A
   (human) or 0x08A19890 (CPU). Cases: GAME +74, layout 3, BATTLE +79, no BATTLE +5, GAME +137, modes
   1 / 6 / 0xD / 0x13 / 0x11 / 0x12 or BATTLE +159: then +772 = 0 (3 when 0x09517E2A is 0, in a
   network battle without GAME +146), or 8 for side 2 in routine 1 / 6 with BATTLE +79 at 7 or 0xA. */
SFA_FUNC(08867060, fighter_flag_772) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[17] = GAME; c->r[16] = f;
    gaddr opp = *W(f + 92);
    int32_t m = S8(GAME + 114);
    int special = S8(GAME + 74) != 0 || S16(BATTLE + 56) == 3 || S8(BATTLE + 79) != 0 || S8(BATTLE + 5) == 0
               || S8(GAME + 137) != 0 || m == 1 || m == 0xD || m == 0x13 || m == 6 || S8(BATTLE + 159) != 0
               || m == 0x11 || m == 0x12;
    if (!special) {
        gaddr list; int32_t want; int cpu;
        if (S8(f + 373) == 0) {
            if (*B(f + 4) != 1 || *B(f + 5) != 4 || S8(opp + 373) == 0) goto done;
            list = *W(0x08A21778u + (uint32_t)(int32_t)S8(f + 338) * 4);
            want = S8(opp + 338); cpu = 0;
        } else {
            if (S8(opp + 373) != 0 || *B(f + 4) != 1) goto done;
            uint32_t b5 = *B(f + 5);
            if (b5 != 6 && b5 != 5) goto done;
            list = *W(0x08A21778u + (uint32_t)(int32_t)S8(opp + 338) * 4);
            want = S8(f + 338); cpu = 1;
        }
        for (gaddr p = list; !(S8(p) & 0x80); p += 2) {
            if (S8(p) != want) continue;
            int32_t v = S8(p + 1);
            *B(f + 772) = (uint8_t)S8((cpu ? 0x08A19890u : 0x08A1988Au) + (uint32_t)v);
            break;
        }
        goto done;
    }
    f_088D5E44(c);
    int b79 = S8(BATTLE + 79);
    if (c->r[2] != 0 && S8(GAME + 146) == 0) {
        *B(f + 772) = 0;
        if (S16(0x09517E2Au) == 0) { *B(f + 772) = 3; goto done; }
        if ((b79 == 7 || b79 == 0xA) && (S8(f + 337) & 2) && *B(f + 4) == 1 && *B(f + 5) == 6) *B(f + 772) = 8;
    } else if ((b79 == 7 || b79 == 0xA) && (S8(f + 337) & 2)) {
        *B(f + 772) = 0;
        if (*B(f + 4) == 1 && *B(f + 5) == 6) *B(f + 772) = 8;
    }
done:
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}
