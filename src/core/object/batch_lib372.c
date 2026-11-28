/* Five more of the library, the starts of the loops over the sound's voices: every voice's
   volumes and pitch kept up to date, voices let go or held by what they play, and a sequence
   loaded for a sound bank. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define VOICES 0x08E74534u /* the sound's voices, 0x68 bytes each */

void f_089F2CA8(Cpu *c); void f_089F224C(Cpu *c); void f_089ED778(Cpu *c); void f_089ED7D4(Cpu *c);
void f_089F23D4(Cpu *c); void f_089ED860(Cpu *c); void f_089ED8C0(Cpu *c); void f_089F2324(Cpu *c);
void f_089ED95C(Cpu *c); void f_089EF238(Cpu *c); void f_089EFED8(Cpu *c);

/* Every voice in use has its volumes and pitch brought up to date (the loop's own routine does
   each; it is entered the way the original falls into it, a voice before the first). A 0x30
   frame keeping s0..s3. */
SFA_FUNC(089F2BC8, voices_update) {
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = c->r[19];
    c->r[19] = 0x08E70000u;
    uint32_t n = *W(0x08E74508u);
    *W(sp + 20) = c->r[17];
    c->r[17] = 0;
    *W(sp + 32) = c->r[31];
    *W(sp + 24) = c->r[18];
    *W(sp + 16) = c->r[16];
    if (n == 0) {
        c->r[31] = *W(sp + 32);
        c->r[19] = *W(sp + 28);
        c->r[18] = *W(sp + 24);
        c->r[17] = *W(sp + 20);
        c->r[16] = *W(sp + 16);
        c->r[29] += 0x30;
        return;
    }
    c->r[18] = 0x51EB851Fu;
    c->r[16] = VOICES - 0x68;
    c->r[17] = 0xFFFFFFFFu;
    ARG(0) = *W(VOICES - 0x68 + 100);
    f_089F2CA8(c);
}

/* (channel, note, owner): the voices playing the note on the channel for the owner are let go;
   the bits of those that were come back. 0 before the sound is started, for a channel past 15 or
   a note past 0x7F. A 0x30 frame keeping s0..s4, with the two bytes at its start. */
SFA_FUNC(089ED71C, note_off) {
    uint32_t ch = ARG(0), note = ARG(1), owner = ARG(2);
    c->r[9] = *W(0x08AD80D0u);
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    c->r[8] = note; c->r[7] = ch;
    ARG(1) = owner;
    *W(sp + 24) = c->r[18];
    c->r[18] = 0;
    *W(sp + 36) = c->r[31];
    ARG(0) = 0;
    *W(sp + 32) = c->r[20]; *W(sp + 28) = c->r[19]; *W(sp + 20) = c->r[17]; *W(sp + 16) = c->r[16];
    if (c->r[9] == 0xFFFFFFFFu || !(ch < 0x10) || !(note < 0x80)) { f_089ED778(c); return; }
    *B(sp) = (uint8_t)ch;
    *B(sp + 1) = (uint8_t)note;
    ARG(0) = sp;
    c->r[31] = 0x089ED7A8u; f_089F224C(c);
    c->r[17] = c->r[2];
    if (c->r[2] != 0) {
        c->r[19] = 0x08E70000u;
        if (*W(0x08E74080u) != 0) {
            c->r[20] = 1;
            c->r[16] = 0xFFFFFFFFu;
            f_089ED7D4(c);
            return;
        }
        c->r[16] = 0;
    }
    ARG(0) = c->r[18];
    f_089ED778(c);
}

/* (channel, mode, owner): the hold of the voices playing on the channel for the owner is set to
   the mode (0 or 1); the bits of those it was set for come back. A 0x20 frame keeping s0..s5. */
SFA_FUNC(089ED808, channel_hold) {
    uint32_t ch = ARG(0), mode = ARG(1), owner = ARG(2);
    c->r[9] = *W(0x08AD80D0u);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 8) = c->r[18];
    c->r[18] = 0;
    *W(sp + 4) = c->r[17];
    c->r[17] = mode;
    ARG(1) = owner;
    *W(sp + 24) = c->r[31];
    ARG(2) = 0;
    *W(sp + 20) = c->r[21]; *W(sp + 16) = c->r[20]; *W(sp + 12) = c->r[19]; *W(sp) = c->r[16];
    if (c->r[9] == 0xFFFFFFFFu || !(ch < 0x10) || !(mode < 2)) { f_089ED860(c); return; }
    c->r[31] = 0x089ED890u; f_089F23D4(c);
    c->r[19] = c->r[2];
    if (c->r[2] != 0) {
        c->r[20] = 0x08E70000u;
        if (*W(0x08E74080u) != 0) {
            c->r[21] = 1;
            c->r[16] = 0xFFFFFFFFu;
            f_089ED8C0(c);
            return;
        }
        c->r[16] = 0;
    }
    ARG(2) = c->r[18];
    f_089ED860(c);
}

/* (owner): every voice playing for the owner is let go; the bits of those that were come back.
   A 0x20 frame keeping s0..s4. */
SFA_FUNC(089ED8F4, owner_all_off) {
    c->r[7] = *W(0x08AD80D0u);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 8) = c->r[18];
    ARG(1) = 0;
    c->r[18] = 0;
    *W(sp + 20) = c->r[31];
    *W(sp + 16) = c->r[20]; *W(sp + 12) = c->r[19]; *W(sp + 4) = c->r[17]; *W(sp) = c->r[16];
    if (c->r[7] != 0xFFFFFFFFu) {
        c->r[31] = 0x089ED930u; f_089F2324(c);
        c->r[17] = c->r[2];
        if (c->r[2] != 0) {
            c->r[19] = 0x08E70000u;
            if (*W(0x08E74080u) != 0) {
                c->r[20] = 1;
                c->r[16] = 0xFFFFFFFFu;
                f_089ED95C(c);
                return;
            }
        }
    }
    c->r[31] = *W(sp + 20);
    c->r[20] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = 0;
    c->r[29] += 0x20;
}

/* (state, data, n): a sequence is loaded for the n-th sound bank (it must be entered; else
   0x80450002) and tied to it. Returns the sequence's number, or the loader's error; 0x80450001
   before the sound is started. A 0x10 frame keeping s0, s1. */
SFA_FUNC(089EDAC0, sequence_load_for_bank) {
    uint32_t n = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[9] = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = n;
    *W(sp + 8) = ra;
    *W(sp) = s0;
    uint32_t answer = 0x80450001u;
    if (c->r[9] != 0xFFFFFFFFu) {
        answer = 0x80450002u;
        if (n < 0x81 && *W(0x08E73E80u + (n << 2)) != 0) {
            c->r[31] = 0x089EDB24u; f_089EF238(c);
            answer = c->r[2];
            c->r[16] = answer;
            if ((int32_t)answer >= 0) {
                ARG(0) = answer; ARG(1) = n;
                c->r[31] = 0x089EDB3Cu; f_089EFED8(c);
            }
        }
    }
    c->r[7] = answer;
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}

void f_089F2928(Cpu *c); void f_089F2064(Cpu *c); void f_089EEAD0(Cpu *c); void f_089EEC08(Cpu *c);
void f_089EEA60(Cpu *c);

/* (voice, on): a voice is taken or given back. Taking (1) one that is free moves it from the
   free list (0x08E75234) to the list of the state 1 and marks it so; giving back one in a state
   1 to 9 moves it from that state's list (counts from 0x08E7450C, 0x80 bytes of words each from
   0x08E75234) to the free list. Returns 0; 0x80450001 before the voices are set up, 0x80540011
   past them, and 0x8054000E for a voice that is not free (taking) or is (giving back). A 0x10
   frame keeping s0. */
SFA_FUNC(089F1FB8, voice_take_or_free) {
    uint32_t v = ARG(0), on = ARG(1);
    c->r[7] = *W(0x08AD81ECu);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    c->r[8] = v;
    *W(sp) = c->r[16];
    c->r[16] = on;
    *W(sp + 4) = c->r[31];
    uint32_t answer;
    if (c->r[7] == 0xFFFFFFFFu) answer = 0x80450001u;
    else if (!(v < *W(0x08E74508u))) answer = 0x80540011u;
    else {
        int64_t m = (int64_t)(int32_t)v * 0x68;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        gaddr voice = VOICES + c->lo;
        int32_t state = S8(voice + 1);
        if (on == 1) {
            if (state != 0) answer = 0x8054000Eu;
            else {
                ARG(0) = 0x08E7450Cu; ARG(1) = 0x08E75234u; ARG(2) = 0x08E74510u; ARG(3) = 0x08E752B4u;
                c->r[31] = 0x089F2180u; f_089F2928(c);
                *B(c->r[2] + 1) = (uint8_t)c->r[16];
                answer = 0;
            }
        } else {
            c->r[16] = voice;
            if (state == 0) answer = 0x8054000Eu;
            else if (!((uint32_t)state < 10)) answer = 0;
            else {
                ARG(0) = 0x08E7450Cu + 4 * (uint32_t)state;
                ARG(1) = 0x08E75234u + 0x80 * (uint32_t)state;
                f_089F2064(c);
                return;
            }
        }
    }
    c->r[31] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = answer;
    c->r[29] += 0x10;
}

/* (bank, i, a, key, pitch, mode, d, pair, and a record on the stack): a note is made ready for a
   voice: the bank's i-th instrument is reported into the record's +12 on, a and the key are kept
   at its +76 and +77, and its pitch is worked out: the one given (plus the pair's +4) for the key
   0xFF, else the instrument's as at a note's start. The record's end fills the rest. 0x80450001
   before the sound is started, 0x8054000A for a key past 0x7F or a mode past 1, 0x80450002 for a
   bank that is not entered, or the report's error. A 0x30 frame keeping s0..s7. */
SFA_FUNC(089EE8E4, note_ready) {
    uint32_t n = ARG(0), key = ARG(3);
    c->r[13] = *W(0x08AD80D0u);
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 8) = c->r[18]; *W(sp + 28) = c->r[23]; *W(sp + 24) = c->r[22]; *W(sp + 20) = c->r[21];
    *W(sp + 16) = c->r[20]; *W(sp + 12) = c->r[19]; *W(sp + 4) = c->r[17]; *W(sp) = c->r[16];
    *W(sp + 32) = c->r[31];
    c->r[18] = n; c->r[23] = c->r[10]; c->r[22] = c->r[9]; c->r[21] = c->r[8];
    c->r[20] = ARG(2); c->r[19] = c->r[11]; c->r[16] = key;
    gaddr rec = *W(sp + 48);
    c->r[17] = rec;
    uint32_t error = 0;
    if (c->r[13] == 0xFFFFFFFFu) error = 0x80450001u;
    else if (!(key < 0x80) && key != 0xFF) error = 0x8054000Au;
    else if (!(c->r[22] < 2)) error = 0x8054000Au;
    else if (!(n < 0x81) || *W(0x08E73E80u + (n << 2)) == 0) error = 0x80450002u;
    else {
        ARG(2) = rec + 0xC;
        ARG(0) = n;
        c->r[31] = 0x089EE9ACu; f_089EEAD0(c);
        error = c->r[2];
    }
    if (error != 0) {
        c->r[31] = *W(sp + 32);
        for (int i = 7; i >= 0; i--) c->r[16 + i] = *W(sp + 4 * (uint32_t)i);
        c->r[2] = error;
        c->r[29] += 0x30;
        return;
    }
    c->r[9] = key & 0xFF;
    *B(rec + 76) = (uint8_t)c->r[20];
    *B(rec + 77) = (uint8_t)key;
    int32_t pitch;
    if (c->r[9] == 0xFF) pitch = (int32_t)(c->r[21] + *W(c->r[19] + 4));
    else {
        int32_t a = S8(rec + 60) * 127, b = S8(rec + 62) * 127;
        c->r[21] = 0x51EB851Fu;
        int64_t ma = (int64_t)a * 0x51EB851F, mb = (int64_t)b * 0x51EB851F;
        c->lo = (uint32_t)mb; c->hi = (uint32_t)(mb >> 32);
        ARG(0) = *B(rec + 61);
        ARG(2) = c->r[9];
        ARG(3) = (uint32_t)(int32_t)(int16_t)(((int32_t)(ma >> 32) >> 5) - (a >> 31));
        ARG(1) = (uint32_t)(int32_t)(int16_t)(((int32_t)(mb >> 32) >> 5) - (b >> 31));
        c->r[31] = 0x089EEA24u; f_089EEC08(c);
        int64_t m = (int64_t)(int32_t)*W(rec + 28) * (int32_t)c->r[2];
        uint64_t u = (uint64_t)(uint32_t)m * 0xBE37C63Bu;
        c->lo = (uint32_t)u; c->hi = (uint32_t)(u >> 32);
        pitch = (int32_t)((c->hi >> 15) + *W(c->r[19] + 4));
    }
    pitch = MIN(pitch, 0x4000);
    c->r[8] = (uint32_t)MAX(pitch, 1);
    f_089EEA60(c);
}
