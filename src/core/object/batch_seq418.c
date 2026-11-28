/* The sound engine: a voice found for a new note, and the voices sounding for a group. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define VOICES_READY 0x08AD81ECu /* -1 until the voices are made ready */
#define VOICES       0x08E74534u /* a voice's record, 0x68 bytes each: +0 its number, +1 its
                                    state, +4 when it began, +84 its group, +88 its priority */
#define VOICE_COUNT  0x08E74508u

void f_089F1670(Cpu *c); void f_089F2E40(Cpu *c);

/* (list, count, priority): of the voices in the list whose priority (+88) is not above the one
   given, the one that began first (+4): its number, or -1 when there is none. They are gathered
   at the bottom of a 0x80 frame, which the end closes. */
SFA_FUNC(089F2DB4, voice_oldest_in_list) {
    gaddr list = ARG(0);
    uint32_t count = ARG(1), priority = ARG(2);
    c->r[29] -= 0x80;
    gaddr sp = c->r[29];
    uint32_t kept = 0, oldest = 0xFFFFFFFFu;
    c->r[12] = 0; c->r[11] = 0xFFFFFFFFu; c->r[8] = 0;
    c->r[2] = 0xFFFFFFFFu;
    if (count != 0) {
        for (uint32_t i = 0; i < count; i++) {
            gaddr v = *W(list + 4 * i);
            c->r[7] = v;
            if (priority < *W(v + 88)) continue;
            *W(sp + 4 * kept) = v;
            kept++;
            if (*W(v + 4) < oldest) oldest = *W(v + 4);
        }
        c->r[12] = kept; c->r[11] = oldest;
        for (uint32_t i = 0; i < kept; i++) {
            gaddr v = *W(sp + 4 * i);
            ARG(1) = v;
            if (*W(v + 4) == oldest) { c->r[2] = *B(v); break; }
        }
    }
    f_089F2E40(c);
}

/* The lists a voice is looked for in when none is free, in the order they are tried: how many
   each holds, and where it is. */
static const struct { uint32_t count, list, back; } TRIED[8] = {
    { 0x08E74520u, 0x08E754B4u, 0x089F17F8u }, { 0x08E7451Cu, 0x08E75434u, 0x089F17D4u },
    { 0x08E74528u, 0x08E755B4u, 0x089F17B0u }, { 0x08E74518u, 0x08E753B4u, 0x089F178Cu },
    { 0x08E7452Cu, 0x08E75634u, 0x089F1768u }, { 0x08E74530u, 0x08E756B4u, 0x089F1744u },
    { 0x08E74514u, 0x08E75334u, 0x089F1720u }, { 0x08E74524u, 0x08E75534u, 0x089F16F8u },
};

/* The lists from the one numbered `from` on are tried for a voice whose priority is not above
   the one in s1: the first found is returned, or 0x80540010. The end closes the 0x10 frame. */
static void voice_take_from(Cpu *c, int from) {
    for (int i = from; i < 8; i++) {
        ARG(1) = *W(TRIED[i].count);
        if (ARG(1) == 0) continue;
        ARG(0) = TRIED[i].list;
        ARG(2) = c->r[17];
        c->r[31] = TRIED[i].back; f_089F2DB4(c);
        ARG(1) = c->r[2];
        if (c->r[2] != 0xFFFFFFFFu) { f_089F1670(c); return; }
    }
    c->r[2] = 0x80540000u;
    ARG(1) = 0x80540010u;
    f_089F1670(c);
}

/* (priority): a voice for a new note: 0x80450001 before the voices are ready; the first free
   one (the list at 0x08E752B4, its count at 0x08E74510) when there is one; else one taken from
   a voice that is sounding. A 0x10 frame keeping s0, s1. */
SFA_FUNC(089F1628, voice_find) {
    c->r[6] = *W(VOICES_READY);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->r[16];
    c->r[16] = 0xFFFFFFFFu;
    *W(sp + 4) = c->r[17];
    ARG(1) = 0x80450001u;
    c->r[17] = ARG(0);
    *W(sp + 8) = c->r[31];
    if (c->r[6] == 0xFFFFFFFFu) { f_089F1670(c); return; }
    ARG(0) = *W(0x08E74510u);
    if (ARG(0) != 0) {
        c->r[6] = *W(0x08E752B4u);
        ARG(1) = *B(c->r[6]);
        f_089F1670(c);
        return;
    }
    voice_take_from(c, 0);
}

SFA_FUNC(089F1694, voice_find_from_1) { voice_take_from(c, 1); }
SFA_FUNC(089F16A0, voice_find_from_2) { voice_take_from(c, 2); }
SFA_FUNC(089F16AC, voice_find_from_3) { voice_take_from(c, 3); }
SFA_FUNC(089F16B8, voice_find_from_4) { voice_take_from(c, 4); }
SFA_FUNC(089F16C4, voice_find_from_5) { voice_take_from(c, 5); }
SFA_FUNC(089F16D0, voice_find_from_6) { voice_take_from(c, 6); }
SFA_FUNC(089F16DC, voice_find_from_7) { voice_take_from(c, 7); }

/* The voices sounding for a group, in two parts the original enters separately. (group, -): a
   mask of the voices in states 2, 3, 6, 7, 8 or 9 whose group (+84) is the one given (1 to
   127); 0 before the voices are ready. `from` 1 enters at the loop's end (0x089F221C), the
   registers holding the rest. */
static void group_voices(Cpu *c, int from) {
    if (from == 0) {
        uint32_t group = ARG(0);
        c->r[7] = *W(VOICES_READY);
        c->r[10] = 0;
        ARG(1) = 0;
        if (c->r[7] == 0xFFFFFFFFu || group == 0 || group >= 0x80) { c->r[2] = 0; return; }
        c->r[2] = *W(VOICE_COUNT);
        c->r[8] = 0;
        if (c->r[2] == 0) { ARG(1) = 0; c->r[2] = 0; return; }
        c->r[6] = VOICES;
        c->r[9] = c->r[2];
        c->r[11] = 7; c->r[12] = 8; c->r[14] = 9; c->r[13] = 1;
    } else {
        goto next;
    }
    for (;;) {
        int32_t state = S8(c->r[6] + 1);
        ARG(1) = (uint32_t)state;
        c->r[8]++;
        c->r[7] = c->r[8] < c->r[9];
        if (state == 6 || state == 2 || state == 3 || state == 7 || state == 8 || state == 9) {
            c->r[3] = *W(c->r[6] + 84);
            if (c->r[3] == ARG(0)) c->r[10] |= 1u << (*B(c->r[6]) & 31);
        }
next:
        if (c->r[7] == 0) break;
        c->r[6] += 0x68;
    }
    ARG(1) = c->r[10];
    c->r[2] = c->r[10];
}

SFA_FUNC(089F2188, group_voices_mask) { group_voices(c, 0); }
SFA_FUNC(089F221C, group_voices_next) { group_voices(c, 1); }
