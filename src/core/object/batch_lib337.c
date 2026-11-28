/* More small pieces of the library and the three of the game that waited for them: the screen's
   fade colour, the sound thread's flags, a table of blocks given back, and a callback run by hand.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define SOUND     0x08E69200u /* the sound driver's state */

void f_089E495C(Cpu *c); void f_089E0B64(Cpu *c); void f_0882E264(Cpu *c); void f_089E49FC(Cpu *c);
void f_089E08E4(Cpu *c); void f_089E0A58(Cpu *c); void f_089E5B30(Cpu *c); void f_089E5B4C(Cpu *c);

/* (a, b, d): the four words at 0x08E65C54 take a, b, d and d. */
SFA_FUNC(089E495C, four_words_set) {
    c->r[7] = 0x08E60000u;
    c->r[8] = 0x08E65C54u;
    *W(0x08E65C54u) = ARG(0);
    *W(0x08E65C54u + 4) = ARG(1);
    *W(0x08E65C54u + 8) = ARG(2);
    *W(0x08E65C54u + 12) = ARG(2);
}

/* The same behind a frame. */
SFA_FUNC(089E0B64, four_words_set_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x089E0B74u; f_089E495C(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (colour): the same with the colour's top byte set, three times over. */
SFA_FUNC(0882E264, colour_set_opaque) {
    uint32_t colour = ARG(0) | 0xFF000000u;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = colour; ARG(1) = colour; ARG(2) = colour;
    c->r[31] = 0x0882E280u; f_089E0B64(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* BATTLE's +36 goes to -1 and +43 to 0xB, the four fighters' +1 are cleared and the colour is
   white. */
SFA_FUNC(0882E138, fight_off_white) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(BATTLE + 36) = 0xFF;
    c->r[5] = BATTLE; c->r[6] = 0xB;
    *B(BATTLE + 43) = 0xB;
    *B(FIGHTER_1 + 1) = 0;
    *B(FIGHTER_2 + 1) = 0;
    *B(FIGHTER_3 + 1) = 0;
    *B(FIGHTER_4 + 1) = 0;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0x00FFFFFFu; c->r[31] = 0x0882E194u; f_0882E264(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* BATTLE's +36 goes from -1 to 0, the four fighters' +1 are set and the colour is black. */
SFA_FUNC(0882E1A0, fight_on_black) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t was = S8(BATTLE + 36);
    *W(c->r[29] + 16) = ra;
    if (was == -1) *B(BATTLE + 36) = 0;
    c->r[6] = (uint32_t)was;
    *B(FIGHTER_1 + 1) = 1;
    *B(FIGHTER_2 + 1) = 1;
    *B(FIGHTER_3 + 1) = 1;
    *B(FIGHTER_4 + 1) = 1;
    c->r[5] = FIGHTER_4;
    ARG(0) = 0; c->r[31] = 0x0882E1FCu; f_0882E264(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The word at 0x08D65C0C. */
SFA_FUNC(089E49FC, word_at_8d65c0c) {
    ARG(0) = 0x08D60000u;
    c->r[2] = *W(0x08D65C0Cu);
}

/* The 1024 entries of 0x18 bytes at 0x08B0C334 that are marked (bit 0 of +8) go back to their kept
   word (+4 to +0, both others cleared); the two words at 0x08B23354 take the word above and the
   place 0x133400 past it. A 0x10 frame. */
SFA_FUNC(089E08E4, entries_give_back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    gaddr e = 0x08B0C334u;
    for (uint32_t i = 0; i < 1024; i++, e += 0x18) {
        if (*W(e + 8) & 1) {
            *W(e) = *W(e + 4);
            *W(e + 4) = 0;
            *W(e + 8) = 0;
        }
    }
    c->r[31] = 0x089E0930u; f_089E49FC(c);
    *W(0x08B20000u + 13140) = c->r[2];
    *W(0x08B20000u + 13144) = c->r[2] + 0x133400u;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (list, first): the entries are given back when first is set; then each of the list's records
   (+4 of them, 0xC bytes each from +0) has its +8 passed to 0x089E0A58. A frame keeping s0..s2. */
SFA_FUNC(089E0A74, list_records_run) {
    gaddr list = ARG(0);
    uint32_t first = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = list;
    if (first != 0) { c->r[31] = 0x089E0A98u; f_089E08E4(c); }
    for (uint32_t i = 0; i < *W(list + 4); i++) {
        c->r[18] = i; c->r[17] = i * 0xC;
        ARG(0) = *W(*W(list) + i * 0xC + 8); c->r[31] = 0x089E0ABCu; f_089E0A58(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (.., .., .., fn): fn runs when there is one. Returns 0. A 0x10 frame. */
SFA_FUNC(089E3A40, run_fourth) {
    uint32_t fn = ARG(3);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    ARG(0) = fn;
    if (fn != 0) { c->r[31] = 0x089E3A58u; rt_call_indirect(c, fn); }
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The sound thread's flag gets its bit 8 set and the byte at SOUND + 166 goes to 0xFF. */
SFA_FUNC(089E6484, sound_flag_8) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = SOUND;
    *W(c->r[29] + 20) = ra;
    ARG(0) = *W(SOUND + 148); ARG(1) = 8; c->r[31] = 0x089E64A4u; f_089E5B30(c);
    *B(SOUND + 166) = 0xFF;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* The sound thread's flag gets its bit 0x10 set. */
SFA_FUNC(089E6500, sound_flag_10_set) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = *W(SOUND + 148); ARG(1) = 0x10; c->r[31] = 0x089E651Cu; f_089E5B30(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The sound thread's flag gets its bit 0x10 cleared. */
SFA_FUNC(089E6528, sound_flag_10_clear) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = *W(SOUND + 148); ARG(1) = 0x10; c->r[31] = 0x089E6544u; f_089E5B4C(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (n): the tempo's other way: the flag's bit 0x20 set and 0x40 cleared, 0 at SOUND + 156 and
   0x8000 divided by a quarter of n at +160. A frame keeping s0, s1. */
SFA_FUNC(089E6550, sound_tempo_other) {
    uint32_t n = ARG(0) & 0xFFFF;
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = n;
    *W(sp + 20) = s1; c->r[17] = SOUND;
    uint32_t flag = *W(SOUND + 148);
    *W(sp + 24) = ra;
    ARG(0) = flag; ARG(1) = 0x20; c->r[31] = 0x089E6578u; f_089E5B30(c);
    ARG(0) = *W(SOUND + 148); ARG(1) = 0x40; c->r[31] = 0x089E6584u; f_089E5B4C(c);
    uint32_t quarter = (uint32_t)((int32_t)(n + ((uint32_t)((int32_t)n >> 2) >> 30)) >> 2);
    DIV(c, 0x8000, quarter);
    *W(SOUND + 156) = 0;
    *W(SOUND + 160) = c->lo;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
