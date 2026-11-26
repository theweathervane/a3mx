/* A fighter's fall: its start (0x0885D9A0), each frame (0x0885DA04) and the knockdown-recovery start
   (0x08866D4C): the state reset, then whether it hops back or stays down, chosen by the player's held
   direction or, for the computer, by its level and the distance (with 0x08821368's random bits).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886D29C(Cpu *c); void f_08821368(Cpu *c); void f_0887BC44(Cpu *c); void f_0887C408(Cpu *c);
void f_08861724(Cpu *c); void f_08866A84(Cpu *c); void f_08862004(Cpu *c); void f_0887C614(Cpu *c);
void f_08861848(Cpu *c); void f_08863130(Cpu *c); void f_0885D0F0(Cpu *c);

static void rnd(Cpu *c, uint32_t back) { c->r[31] = back; f_08821368(c); }

/* (fighter): +4..+7 = 1 / 0 / 3 / 8, in the air, +78 = -1, facing the opponent, 0x0886D29C and its state
   cleared (+669 = 4, +670 = 7, +689 = 1). Then a choice (r20): 0 stays down, 1 a short hop back (x 1.0, y
   4.0), 2 a long one (x 3.0, y 6.0), both with fall -0.375 and x away from where it was going.
   With no choice made: a player's held direction (+752 bits 0 / 1) picks 2 / 1; the computer in mode 2
   whose bit for +337 is clear picks by its level (0x0950FF68): 1 for level 1, none for 2, 2 for 3, and
   for levels 0 or 4 and up by distance: under 32 2, under 48 1 or 2, else 0..2 at random. +573 = 0xC / 0xA
   / 0xE. With a choice made: 0x0887BC44 and a random 0..3: 1 short, 3 long, else none; +573 = 6 or 8.
   +543 / +82 take +573's value, the cue 0x0887C408 (0x230, 0x232 with +414), +656 = 4 for +338 9 with
   +540, and 0x08861724 (0x18). A 0x30 frame keeping s0..s4. */
SFA_FUNC(08866D4C, knockdown_recover) {
    gaddr o = ARG(0);
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 5; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 36) = ra;
    c->r[16] = o; c->r[17] = 1;
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3; *B(o + 7) = 8;
    *B(o + 73) = 1; *B(o + 78) = 0xFF; *B(o + 11) = *B(o + 697);
    ARG(0) = o; c->r[31] = 0x08866DA8u; f_0886D29C(c);
    static const uint16_t clr[] = { 587, 586, 604, 539, 538, 671, 596, 770, 592, 270, 688, 591, 595, 588,
                                    562, 561, 560, 625, 624, 626, 127, 126, 125, 656, 543 };
    *W(o + 104) = 0;
    for (unsigned i = 0; i < sizeof clr / sizeof clr[0]; i++) *B(o + clr[i]) = 0;
    *W(o + 244) = 0; *H(o + 778) = 0;
    c->r[18] = 4;
    *B(o + 669) = 4; *B(o + 670) = 7; *B(o + 689) = 1;
    int32_t id = S8(o + 337);
    int32_t how;           /* 0 none, 1 short hop, 2 long hop */
    uint32_t mark;         /* +573 */
    if (S8(o + 373) == 0) {
        const gaddr g = 0x08E75910u;
        c->r[19] = 0xC;
        int cpu = S8(g + 114) == 2 && (S8(g + 53) & (1 << (id & 31))) == 0;
        if (!cpu) {
            uint32_t h = *H(o + 752);
            how = (h & 3) == 0 ? 0 : (h & 2) ? 1 : 2;
            c->r[20] = (uint32_t)how;
        } else {
            int32_t lvl = *B(0x0950FF68u);
            int by_dist = 0;
            if (lvl < 2) { how = 1; if (lvl <= 0) by_dist = 1; }
            else if (lvl < 3) how = 0;
            else { how = 2; if (lvl >= 4) by_dist = 1; }
            c->r[20] = (uint32_t)how;
            if (by_dist) {
                int32_t d = S16(o + 22) - S16(o + 132);
                if (d >= 48) {
                    ARG(0) = (uint32_t)id; ARG(1) = 0; ARG(2) = 0;
                    rnd(c, 0x08866ED4u);
                    how = (int32_t)(c->r[2] & 3);
                    if (how != 0) how--;
                } else if (d >= 32) {
                    c->r[20] = 2;
                    ARG(0) = (uint32_t)id; ARG(1) = 0; ARG(2) = 1;
                    rnd(c, 0x08866EFCu);
                    how = (c->r[2] & 1) ? 1 : 2;
                } else how = 2;
                c->r[20] = (uint32_t)how;
            }
        }
        mark = how == 0 ? 0xC : how == 1 ? 0xA : 0xE;
    } else {
        ARG(0) = o; c->r[31] = 0x08866F8Cu; f_0887BC44(c);
        c->r[19] = 8;
        rnd(c, 0x08866F94u);
        uint32_t r = c->r[2] & 3;
        how = (r & 1) == 0 ? 0 : (r & 2) ? 2 : 1;
        mark = how == 0 ? 8 : 6;
    }
    c->r[19] = mark;
    if (how != 0) {
        uint32_t x = how == 1 ? 0x10000u : 0x30000u;
        if ((int32_t)*W(o + 96) < 0) x = 0u - x;
        *W(o + 96) = x; *W(o + 100) = how == 1 ? 0x40000u : 0x60000u; *W(o + 108) = 0xFFFFA000u;
    }
    *B(o + 573) = (uint8_t)mark;
    *B(o + 543) = (uint8_t)mark; *H(o + 82) = (uint16_t)mark;
    ARG(0) = o; ARG(1) = (uint32_t)S8(o + 337);
    if (S8(o + 414) != 0) { ARG(2) = 0x232; c->r[31] = 0x08867014u; }
    else { ARG(2) = 0x230; c->r[31] = 0x08866FFCu; }
    f_0887C408(c);
    if (S8(o + 338) == 9 && S8(o + 540) != 0) *B(o + 656) = 4;
    ARG(0) = o; ARG(1) = 0x18; c->r[31] = 0x08867040u; f_08861724(c);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): rising, the recovery starts once 0x08866A84 allows it; otherwise 0x08862004 says whether it
   has landed: falling and landed it lies down (+4..+7 = 1 / 1 / 2 / 0, +698 / +541 / +539 cleared, on the
   floor and cue 0x0887C614); falling in the air +270 = 1, +543 = 2, +586 / +587 cleared and ticked. */
SFA_FUNC(0885DA04, fall_or_recover) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if ((int32_t)*W(o + 100) >= 0) {
        ARG(0) = o; c->r[31] = 0x0885DA24u; f_08866A84(c);
        if (c->r[2] != 0) { ARG(0) = o; c->r[31] = 0x0885DA34u; f_08866D4C(c); goto out; }
    }
    ARG(0) = o; c->r[31] = 0x0885DA44u; f_08862004(c);
    if ((int32_t)*W(o + 100) < 0) {
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 2; *B(o + 7) = 0;
            *B(o + 698) = 0; *B(o + 541) = 0; *B(o + 539) = 0; *B(o + 73) = 0;
            *W(o + 20) = (uint32_t)S16(o + 132) << 16;
            ARG(0) = o; c->r[31] = 0x0885DA8Cu; f_0887C614(c);
        } else {
            *B(o + 270) = 1; *B(o + 543) = 2; *B(o + 586) = 0; *B(o + 587) = 0;
            ARG(0) = o; c->r[31] = 0x0885DAB8u; f_08861848(c);
        }
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): +7 on, +235 / +696 / +541 / +604 cleared, 0x08863130, +73 = -1, +74 = 1, 0x0885D0F0 and the
   fall's first frame. */
SFA_FUNC(0885D9A0, fall_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    *B(o + 235) = 0;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 696) = 0; *B(o + 541) = 0; *B(o + 604) = 0;
    ARG(0) = o; c->r[31] = 0x0885D9D4u; f_08863130(c);
    *B(o + 73) = 0xFF; *B(o + 74) = 1;
    ARG(0) = o; c->r[31] = 0x0885D9ECu; f_0885D0F0(c);
    ARG(0) = o; c->r[31] = 0x0885D9F4u; f_0885DA04(c);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
