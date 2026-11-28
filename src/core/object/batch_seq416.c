/* The sound engine's small pieces: a voice's settings set and read, a slot let go, and ends of
   its larger routines. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SEQ_READY    0x08AD80D0u /* -1 until the sequences are made ready */
#define VOICES_READY 0x08AD81ECu /* -1 until the voices are */
#define VOICES       0x08E74534u /* a voice's record, 0x68 bytes each */
#define VOICE_COUNT  0x08E74508u

void f_089F1E98(Cpu *c); void f_089F0244(Cpu *c); void f_089F0358(Cpu *c); void f_089ED808(Cpu *c);
void f_089F0930(Cpu *c); void f_089F1898(Cpu *c);
void hle___sceSasSetNoise(Cpu *c);

/* (voice, noise): the voice is made a noise of that kind: 0x80420100 before the sound core is
   started, else the core's answer. A 0x10 frame. */
SFA_FUNC(089ECD08, voice_noise_set) {
    uint32_t ra = c->r[31];
    c->r[8] = *W(0x08E73000u);
    c->r[7] = ARG(0);
    c->r[29] -= 0x10;
    c->r[3] = 0x80420000u;
    c->r[2] = 0x08E70000u;
    c->r[6] = ARG(1);
    *W(c->r[29]) = ra;
    ARG(1) = c->r[7];
    ARG(0) = 0x08E73040u;
    c->r[7] = 0x80420100u;
    if (c->r[8] != 0) {
        c->r[31] = 0x089ECD40u; hle___sceSasSetNoise(c);
        c->r[7] = c->r[2];
    }
    c->r[2] = c->r[7];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The end of a routine: r4 is returned from a 0x30 frame that kept s0..s2. */
SFA_FUNC(089ED0F8, tail_ed0f8) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 44);
    c->r[18] = *W(sp + 40);
    c->r[17] = *W(sp + 36);
    c->r[16] = *W(sp + 32);
    c->r[2] = ARG(0);
    c->r[29] += 0x30;
}

/* One of the tables of slots (128 words) has the slot n let go: `not_ready` when `ready` says
   the engine is not started, `bad` for a number past `limit` or a slot that is empty, else 0. */
static void slot_release(Cpu *c, int ready, uint32_t not_ready, gaddr table, uint32_t limit, uint32_t bad) {
    uint32_t n = ARG(0);
    c->r[6] = n < limit;
    ARG(1) = not_ready;
    if (ready) {
        gaddr slot = table + (n << 2);
        c->r[3] = slot;
        ARG(1) = bad;
        if (n < limit && *W(slot) != 0) {
            *W(slot) = 0;
            ARG(1) = 0;
        }
    }
    c->r[2] = ARG(1);
}

/* (n): the bank in slot n (0 to 128) is let go. */
SFA_FUNC(089ED540, bank_slot_release) {
    slot_release(c, *W(SEQ_READY) != 0xFFFFFFFFu, 0x80450001u, 0x08E73E80u, 0x81, 0x80450002u);
}

/* (n): the sequence in slot n (0 to 127) is let go. */
SFA_FUNC(089EF304, sequence_slot_release) {
    slot_release(c, *W(0x08E74304u) != 0, 0x80450001u, 0x08E74308u, 0x80, 0x80540007u);
}

/* The routine at 0x089F1E98, or 0x80450001 before the sequences are ready. A 0x10 frame. */
SFA_FUNC(089ED9B0, voices_call_checked) {
    uint32_t ra = c->r[31];
    c->r[8] = *W(SEQ_READY);
    c->r[29] -= 0x10;
    c->r[7] = 0xFFFFFFFFu;
    *W(c->r[29]) = ra;
    c->r[6] = 0x80450001u;
    if (c->r[8] != 0xFFFFFFFFu) {
        c->r[31] = 0x089ED9D8u; f_089F1E98(c);
        c->r[6] = c->r[2];
    }
    c->r[2] = c->r[6];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (n): the word n of the table at 0x08E74084. */
SFA_FUNC(089EDC1C, channel_value_get) {
    ARG(1) = ARG(0) << 2;
    c->r[6] = 0x08E74084u;
    c->r[3] = ARG(1) + 0x08E74084u;
    c->r[2] = *W(c->r[3]);
}

/* Three ends of the sequence's event routine. */
SFA_FUNC(089F029C, tail_f029c) {
    *W(c->r[16] + 28) = c->r[2];
    f_089F0244(c);
}
SFA_FUNC(089F0354, tail_f0354) {
    c->r[2] = ARG(1) + ARG(0);
    f_089F0358(c);
}
SFA_FUNC(089F0CB0, tail_f0cb0) {
    c->r[31] = 0x089F0CB8u; f_089ED808(c);
    c->r[31] = *W(c->r[29] + 28);
    f_089F0930(c);
}

/* A part of a voice's start: two words copied from where r6 points to where r8 does. */
SFA_FUNC(089F1974, tail_f1974) {
    c->r[2] = *W(c->r[6]);
    c->r[3] = *W(c->r[6] + 4);
    *W(c->r[8]) = c->r[2];
    *W(c->r[8] + 4) = c->r[3];
    c->r[6] = 0;
    f_089F1898(c);
}

/* A voice's byte at +`at` set (the value below 0x80): 0x80450001 before the voices are ready,
   0x80540011 for a voice that is not there, 0x8054000A for a value too large. */
static void voice_byte_set(Cpu *c, uint32_t at) {
    uint32_t voice = ARG(0), value = ARG(1);
    c->r[8] = *W(VOICES_READY);
    c->r[6] = 0x80450001u;
    if (c->r[8] != 0xFFFFFFFFu) {
        c->r[9] = *W(VOICE_COUNT);
        c->r[2] = value < 0x80;
        c->r[7] = voice < c->r[9];
        c->r[6] = 0x80540011u;
        if (c->r[7] != 0) {
            c->r[6] = 0x8054000Au;
            if (value < 0x80) {
                int64_t m = (int64_t)(int32_t)voice * 0x68;
                c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
                c->r[6] = 0;
                ARG(0) = c->lo + VOICES;
                *B(ARG(0) + at) = (uint8_t)value;
            }
        }
    }
    c->r[2] = c->r[6];
}

SFA_FUNC(089F25F4, voice_byte96_set) { voice_byte_set(c, 96); }
SFA_FUNC(089F265C, voice_byte97_set) { voice_byte_set(c, 97); }
SFA_FUNC(089F26C4, voice_byte98_set) { voice_byte_set(c, 98); }
SFA_FUNC(089F272C, voice_byte99_set) { voice_byte_set(c, 99); }

/* (voice, value): a voice's word at +100 set, with the same two first answers. */
SFA_FUNC(089F2794, voice_word100_set) {
    uint32_t voice = ARG(0);
    c->r[8] = *W(VOICES_READY);
    c->r[6] = 0x80450001u;
    if (c->r[8] != 0xFFFFFFFFu) {
        c->r[9] = *W(VOICE_COUNT);
        c->r[7] = voice < c->r[9];
        c->r[6] = 0x80540011u;
        if (c->r[7] != 0) {
            int64_t m = (int64_t)(int32_t)voice * 0x68;
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            c->r[6] = 0;
            ARG(0) = c->lo + VOICES;
            *W(ARG(0) + 100) = ARG(1);
        }
    }
    c->r[2] = c->r[6];
}

/* (voice, seven places): seven of a voice's settings read out, each as a word: its +80, +79,
   +93, +76 (signed), +77, +78 (signed) and +44. */
SFA_FUNC(089F27EC, voice_settings_get) {
    int64_t m = (int64_t)(int32_t)ARG(0) * 0x68;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr v = c->lo + VOICES;
    c->r[13] = v;
    c->r[14] = v + 0x40;
    *W(ARG(1)) = *B(v + 80);
    *W(ARG(2)) = *B(v + 79);
    *W(ARG(3)) = *B(v + 93);
    *W(c->r[8]) = (uint32_t)S8(v + 76);
    *W(c->r[9]) = *B(v + 77);
    c->r[2] = (uint32_t)S8(v + 78);
    *W(c->r[10]) = c->r[2];
    c->r[3] = *W(v + 44);
    *W(c->r[11]) = c->r[3];
}

/* The end of a routine with a 0x80 frame. */
SFA_FUNC(089F2E40, tail_f2e40) {
    c->r[29] += 0x80;
}
