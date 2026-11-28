/* The sound engine: a voice given a new note's settings. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define VOICES_READY 0x08AD81ECu
#define VOICES       0x08E74534u /* 0x68 bytes a voice: +1 its state, +16 its note's settings */
#define VOICE_COUNT  0x08E74508u

void f_089F1898(Cpu *c); void f_089F2928(Cpu *c); void f_089F1910(Cpu *c); void f_089F1974(Cpu *c);

/* The lists the voices are kept in by state: how many each holds, and where it is. */
#define LIST_FREE   0x08E74510u, 0x08E752B4u
#define LIST_NEW    0x08E74514u, 0x08E75334u
#define LIST_3      0x08E74518u, 0x08E753B4u
#define LIST_4      0x08E7451Cu, 0x08E75434u
#define LIST_5      0x08E74520u, 0x08E754B4u
#define LIST_AGAIN  0x08E74524u, 0x08E75534u
#define LIST_7      0x08E74528u, 0x08E755B4u
#define LIST_8      0x08E7452Cu, 0x08E75634u
#define LIST_9      0x08E74530u, 0x08E756B4u

/* (voice, settings): the voice takes the note's settings (0x58 bytes, to its +16). A free voice,
   or one that has ended (states 5 and 9), goes to the list of new notes in state 2; one still
   sounding (3, 4, 7, 8) to the list of notes begun again in state 6; one already waiting in
   either (2, 6) only has its settings replaced. 0x80450001 before the voices are ready,
   0x80540011 for a voice that is not there, 0x80540012 for one in state 0, else 0. A 0x10
   frame keeping s0, which the ends close after the last two words are copied. */
SFA_FUNC(089F1808, voice_note_set) {
    uint32_t voice = ARG(0);
    c->r[7] = *W(VOICES_READY);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    c->r[8] = voice;
    *W(sp) = c->r[16];
    c->r[6] = 0x80450001u;
    gaddr from = ARG(1);
    c->r[16] = from;
    *W(sp + 4) = c->r[31];
    if (c->r[7] == 0xFFFFFFFFu) { f_089F1898(c); return; }
    c->r[6] = 0x80540011u;
    if (voice >= *W(VOICE_COUNT)) { f_089F1898(c); return; }
    int64_t m = (int64_t)(int32_t)voice * 0x68;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    uint32_t state = (uint32_t)S8(c->lo + VOICES + 1);
    c->r[2] = state;
    c->r[6] = 0;
    if (state >= 10) { f_089F1898(c); return; }
    static const struct { uint32_t from_n, from_list, to_n, to_list, back; uint8_t state; } MOVES[10] = {
        { 0 },
        { LIST_FREE, LIST_NEW, 0x089F18D0u, 2 },
        { 0 },
        { LIST_3, LIST_AGAIN, 0x089F19B0u, 6 },
        { LIST_4, LIST_AGAIN, 0x089F1A1Cu, 6 },
        { LIST_5, LIST_NEW, 0x089F1A88u, 2 },
        { 0 },
        { LIST_7, LIST_AGAIN, 0x089F1B48u, 6 },
        { LIST_8, LIST_AGAIN, 0x089F1BB4u, 6 },
        { LIST_9, LIST_NEW, 0x089F1C20u, 2 },
    };
    if (state == 0) { c->r[6] = 0x80540012u; f_089F1898(c); return; }
    gaddr to;
    if (state == 2 || state == 6) {
        to = c->lo + VOICES + 0x10;
    } else {
        ARG(0) = MOVES[state].from_n; ARG(1) = MOVES[state].from_list;
        ARG(2) = MOVES[state].to_n; ARG(3) = MOVES[state].to_list;
        c->r[31] = MOVES[state].back; f_089F2928(c);
        *B(c->r[2] + 1) = MOVES[state].state;
        to = c->r[2] + 0x10;
    }
    for (uint32_t i = 0; i < 0x50; i += 4) *W(to + i) = *W(from + i);
    c->r[6] = from + 0x50;
    if (state == 2 || state == 6) {
        c->r[8] = to + 0x50;
        f_089F1974(c);
    } else {
        c->r[7] = to + 0x50;
        c->r[16] = *W(from + 0x4C);
        f_089F1910(c);
    }
}
