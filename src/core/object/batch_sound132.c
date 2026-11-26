/* Sound-cue callers, 20 - 30 instructions: a fighter's idle taunt voice, menu cursor cues, hit voices
   by attack, the super gauge's change and its "ready" cue. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08821368(Cpu *c); void f_088D2E04(Cpu *c); void f_0889995C(Cpu *c); void f_0889D048(Cpu *c);
void f_0889C6A0(Cpu *c); void f_0887C408(Cpu *c); void f_0887C4CC(Cpu *c);

#define GAME       0x08E75910u
#define HIT_VOICES 0x08A2436Cu   /* s16 cues, four per attack kind (+119) by strength (+139) */

static void bank_cue(Cpu *c, gaddr o, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; f_088D2E04(c);
}
static void side_cue(Cpu *c, gaddr o, uint32_t side, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = side; ARG(2) = n; c->r[31] = back; f_0887C408(c);
}

/* (fighter): the +82 wait counts down; at 0 (+7 counted) a voice is picked from the fighter's held
   buttons (+362 bits 4 - 9, the first one set; random 0 - 5 without one or with +373): 1, 3 and 5 play
   bank voice 0x23 (+828 = 0x4E), the rest 0x22 (+828 = 0x3C). */
SFA_FUNC(089C613C, fighter_taunt_voice) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    int32_t t = S16(o + 82);
    *W(c->r[29] + 20) = ra;
    if (t != 0) *H(o + 82) = (uint16_t)(t - 1);
    else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        c->r[31] = 0x089C6168u; f_08821368(c);
        DIV(c, c->r[2], 6);
        int32_t k = (int16_t)c->hi;
        if (S8(o + 373) == 0) {
            uint32_t held = *H(o + 362) & 0x3F0;
            for (int i = 0; i < 6; i++)
                if (held & (0x10u << i)) { k = i; break; }
        }
        if (k == 5 || k == 3 || k == 1) {
            bank_cue(c, o, 0x23, 0x089C6250u);
            *W(o + 828) = 0x4E;
        } else {
            bank_cue(c, o, 0x22, 0x089C623Cu);
            *W(o + 828) = 0x3C;
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu, item): the item drawn (0x0889995C), first playing the cursor cue (0x0889D048) when the item's
   +78 bit 0 is clear and the cursor is on it (+139 / +140 against the item's +22 / +18) or it is a live
   (+81) plain (+429 clear) item outside the game's +32 bits 0 - 1. */
SFA_FUNC(08899880, menu_item_cue) {
    gaddr m = ARG(0), it = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = m;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = it;
    int check;   /* 0: draw only, 1: look at +78 */
    uint32_t back = 0x08899948u;
    if (S8(it + 81) != 0 && S8(it + 429) == 0) {
        if ((*B(GAME + 32) & 3) != 0) { check = 0; back = 0x088998E4u; }
        else check = 1;
    } else if (S8(m + 139) == S16(it + 22) && S8(m + 140) == S16(it + 18)) { check = 0; back = 0x08899920u; }
    else check = 1;
    if (check && (S8(it + 78) & 1) == 0) {
        ARG(0) = m; c->r[31] = 0x0889993Cu; f_0889D048(c);
    }
    ARG(0) = m; ARG(1) = it;
    c->r[31] = back; f_0889995C(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu, item): when the item's choice (+386) is not the menu's (+80), the cursor cue and 0x0889C6A0. */
SFA_FUNC(0889C640, menu_choice_change) {
    gaddr m = ARG(0), it = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2;
    int32_t n = S8(it + 386);
    c->r[18] = (uint32_t)n;
    *W(c->r[29] + 20) = s1;
    c->r[17] = m;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = it;
    if (n != S8(m + 80)) {
        ARG(0) = m; c->r[31] = 0x0889C670u; f_0889D048(c);
        ARG(0) = m; ARG(1) = it; ARG(2) = (uint32_t)n & 0xFFFF;
        c->r[31] = 0x0889C688u; f_0889C6A0(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in state 0 / 7 / 1 (+5 - +7) with +236 = 4 and +820 = 1, bank voice 0x17. */
SFA_FUNC(089B5104, fighter_voice_17) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 5) == 0 && *B(o + 6) == 7 && *B(o + 7) == 1 && S8(o + 236) == 4 && *W(o + 820) == 1)
        bank_cue(c, o, 0x17, 0x089B515Cu);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (attack, fighter): unless the attack's kind (+119) has bit 7, the hit voice: by kind and strength
   (+139) from 0x08A2436C, or for kind 0 cue 0x20 (0x21 for the fighter's states 6, 0x14, 0xE, and 0x1C
   with +828 bits 8 - 15), from the fighter's side. */
SFA_FUNC(0887DFB8, hit_voice) {
    gaddr a = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t kind = *B(a + 119);
    *W(c->r[29] + 16) = ra;
    if ((kind & 0x80) == 0) {
        uint32_t n;
        if (kind != 0) n = (uint32_t)(int32_t)S16(HIT_VOICES + 2 * ((kind << 2) + (uint32_t)S8(a + 139)));
        else {
            int32_t s = S8(f + 338);
            n = (s == 6 || s == 0x14 || s == 0xE || (s == 0x1C && (*W(f + 828) & 0xFF00) != 0)) ? 0x21 : 0x20;
        }
        side_cue(c, f, (uint32_t)(int32_t)S8(f + 337), n, 0x0887E054u);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (gauge, change): the super gauge (+366) changed, held to 0 .. 144; reaching 144 from below plays the
   "ready" cue 0x31 from its side. */
SFA_FUNC(0886CBFC, gauge_change) {
    gaddr g = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t old = S16(g + 366);
    uint32_t v = (uint32_t)old + ARG(1);
    *H(g + 366) = (uint16_t)v;
    *W(c->r[29] + 16) = ra;
    if ((int16_t)v < 0) *H(g + 366) = 0;
    else if (S16(g + 366) >= 144) *H(g + 366) = 0x90;
    int32_t now = S16(g + 366);
    if (old != now && now == 0x90) side_cue(c, g, (uint32_t)(int32_t)S8(g + 337), 0x31, 0x0886CC64u);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, side): bank cue 0x200 from `side`, cue 0x2E, and in state 0x26 with +820 and +236 = 4,
   bank cue 0x217. */
SFA_FUNC(0887C4F8, fighter_cues_200) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    side_cue(c, o, ARG(1), 0x200, 0x0887C510u);
    side_cue(c, o, (uint32_t)(int32_t)S8(o + 337), 0x2E, 0x0887C520u);
    if (S8(o + 338) == 0x26 && *W(o + 820) != 0 && S8(o + 236) == 4)
        side_cue(c, o, (uint32_t)(int32_t)S8(o + 337), 0x217, 0x0887C55Cu);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (task, item): the step (+6) moves on with +18 cleared and +8 = 3 (cue 0x10F in mode 0x12, +114), the
   timer +20 = 180, +10 / +22 cleared and the item's +44 = 7. */
SFA_FUNC(08826A64, task_step_cue_10f) {
    gaddr o = ARG(0), it = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
    *H(o + 18) = 0;
    int32_t mode = S8(o + 114);
    *H(o + 8) = 3;
    *W(c->r[29] + 20) = s1;
    c->r[17] = o;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = it;
    if (mode == 0x12) { ARG(0) = 0; ARG(1) = 0x10F; ARG(2) = 0xC0; c->r[31] = 0x08826AB0u; f_0887C4CC(c); }
    *H(o + 20) = 0xB4; *H(o + 10) = 0; *H(o + 22) = 0;
    *B(it + 44) = 7;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
