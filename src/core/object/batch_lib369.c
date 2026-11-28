/* Eight more of the library: the drawing state and its five lists back to their start, a sound
   bank's entries reported, a voice's hold set, and more ends of longer routines. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GE       0x08E69300u /* the drawing state */
#define VOICES   0x08E74534u /* the sound's voices, 0x68 bytes each */
#define VOICE_N  0x08E74508u /* how many there are */
#define VOICE_UP 0x08AD81ECu /* -1 until they are set up */

void f_089ED018(Cpu *c); void f_089EDA4C(Cpu *c); void f_089EDA30(Cpu *c); void f_089F249C(Cpu *c);
void f_089EDE3C(Cpu *c); void f_089EEEC4(Cpu *c); void f_089EEE44(Cpu *c); void f_089EEB70(Cpu *c);
void f_089EEFE0(Cpu *c); void f_089EF148(Cpu *c); void f_089F2928(Cpu *c); void f_089F1F44(Cpu *c);
void f_089F1F48(Cpu *c); void f_089ECC5C(Cpu *c); void f_089F2E50(Cpu *c); void f_089ECBF8(Cpu *c);
void f_089ECE78(Cpu *c); void f_089EDB58(Cpu *c); void f_089ECAA0(Cpu *c); void f_089FBDEC(Cpu *c);

/* The drawing state back to its start: a screen of 480 by 272 in the format 1, no list in use
   (-1 at +12), and each of the five lists (252 bytes from +76) empty with the same size, 0x64 at
   +16, 0xFFFF at +80 and -1 in its last word; the two words at 0x08E692D0 are cleared. */
SFA_FUNC(089EA600, draw_state_reset) {
    *W(GE + 60) = 0x1E0;
    *W(GE + 12) = 0xFFFFFFFFu;
    *W(GE + 4) = 0;
    *W(GE + 40) = 0x1E0;
    *W(GE + 44) = 0; *W(GE + 48) = 0; *W(GE + 52) = 0; *W(GE + 56) = 0;
    *W(GE + 8) = 0; *W(GE + 16) = 0; *W(GE + 72) = 0; *W(GE + 68) = 0;
    *W(GE + 36) = 1;
    *W(GE + 64) = 0x110;
    gaddr e = GE + 0x4C;
    for (int i = 0; i < 5; i++, e += 0xFC) {
        *W(e) = 4;
        *W(e + 116) = 0;
        *W(e + 16) = 0x64;
        *W(e + 20) = 0;
        *W(e + 24) = 1;
        *W(e + 28) = 0x1E0;
        *W(e + 32) = 0x110;
        for (uint32_t k = 36; k <= 52; k += 4) *W(e + k) = 0;
        *W(e + 84) = 0;
        *W(e + 88) = 1;
        *W(e + 92) = 0;
        for (uint32_t k = 56; k <= 76; k += 4) *W(e + k) = 0;
        *W(e + 80) = 0xFFFF;
        *W(e + 96) = 0; *W(e + 104) = 0; *W(e + 100) = 0; *W(e + 108) = 0; *W(e + 112) = 0;
        *W(e + 0xF8) = 0xFFFFFFFFu;
    }
    ARG(1) = e; ARG(2) = e + 0xF8;
    c->r[3] = 0xFFFFFFFFu;
    c->r[14] = GE;
    c->r[8] = 0x08E70000u; c->r[7] = 0x08E692D0u;
    *W(0x08E692D0u) = 0;
    *W(0x08E692D4u) = 0;
}

/* The end of a routine: with bit 2 of r3 set the reverb's delay and feedback take the record's +8
   and +12 (s0); a failure is the answer, and otherwise the record's first word says whether the
   volumes follow. */
SFA_FUNC(089EDA24, tail_reverb_params) {
    ARG(1) = c->r[3] & 2;
    if (ARG(1) != 0) {
        ARG(1) = *W(c->r[16] + 12);
        ARG(0) = *W(c->r[16] + 8);
        c->r[31] = 0x089EDA98u; f_089ED018(c);
        ARG(1) = c->r[2];
        if (c->r[2] != 0) { f_089EDA4C(c); return; }
        c->r[3] = *W(c->r[16]);
    }
    f_089EDA30(c);
}

/* (channel, owner): the voices playing on the channel (below 16), with the state 5 taken as
   well; 0 before the sound is started. A 0x10 frame. */
SFA_FUNC(089EDE14, voices_on_channel) {
    c->r[9] = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    c->r[8] = 0xFFFFFFFFu;
    *W(c->r[29]) = c->r[31];
    c->r[3] = ARG(0) < 0x10;
    c->r[7] = 0;
    if (c->r[9] != 0xFFFFFFFFu && c->r[3] != 0) {
        c->r[31] = 0x089EDE54u; f_089F249C(c);
        c->r[7] = c->r[2];
    }
    f_089EDE3C(c);
}

/* (n, i, out): the i-th instrument of the n-th sound bank (listed at 0x08E73E80) is reported at
   out. One without a sample (its first word -1) reports 1 and its +12; the others report 0, the
   sample's place in the bank's data (+4 of the bank's record), its size and its +4; the voice's
   fields follow from the instrument. 0x80540005 when the bank has no such entry. A frame keeping
   s0, s1, with the two places found at its start. */
SFA_FUNC(089EEAD0, bank_instrument_report) {
    uint32_t n = ARG(0);
    gaddr out = ARG(2);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = c->r[17];
    gaddr slot = 0x08E73E80u + (n << 2);
    *W(sp + 16) = c->r[16];
    c->r[17] = slot;
    *W(sp + 24) = c->r[31];
    c->r[16] = out;
    ARG(2) = sp;
    ARG(0) = *W(*W(slot));
    c->r[31] = 0x089EEB04u; f_089EEEC4(c);
    int failed = (int32_t)c->r[2] < 0;
    if (!failed) {
        gaddr inst = *W(sp);
        c->r[7] = inst;
        uint32_t sample = *W(inst);
        c->r[9] = sample;
        if (sample == 0xFFFFFFFFu) {
            *W(out) = 1;
            *B(out + 12) = *B(inst + 12);
            f_089EEB70(c);
            return;
        }
        ARG(1) = sample; ARG(2) = sp + 4;
        ARG(0) = *W(*W(slot));
        c->r[31] = 0x089EEB34u; f_089EEE44(c);
        failed = (int32_t)c->r[2] < 0;
        if (!failed) {
            *W(out) = 0;
            gaddr entry = *W(sp + 4);
            c->r[14] = entry;
            c->r[25] = *W(slot);
            c->r[7] = *W(sp);
            c->r[24] = *W(entry + 8);
            c->r[17] = *W(entry);
            c->r[15] = *W(c->r[25] + 4);
            *W(out + 8) = c->r[24];
            c->r[13] = c->r[15] + c->r[17];
            c->r[12] = *W(entry + 4);
            *W(out + 4) = c->r[13];
            *W(out + 16) = c->r[12];
            f_089EEB70(c);
            return;
        }
    }
    c->r[31] = *W(sp + 24);
    c->r[17] = *W(sp + 20);
    c->r[16] = *W(sp + 16);
    c->r[2] = 0x80540005u;
    c->r[29] += 0x20;
}

/* (bank, i, out): the place of the bank's i-th word (0x089EEFE0 says whether it is there): the
   bank plus the word at +32 of the chunk at its +16. Returns 0, or -1. A frame keeping s0..s2. */
SFA_FUNC(089EEF64, bank_word_place) {
    gaddr bank = ARG(0);
    uint32_t i = ARG(1);
    gaddr out = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2; *W(sp + 20) = s1; *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[18] = out; c->r[17] = bank; c->r[16] = i;
    c->r[31] = 0x089EEF88u; f_089EEFE0(c);
    uint32_t answer = 0xFFFFFFFFu;
    if ((int32_t)c->r[2] >= 0) {
        ARG(0) = bank; ARG(1) = sp;
        c->r[31] = 0x089EEFA0u; f_089EF148(c);
        if ((int32_t)c->r[2] >= 0) {
            answer = 0;
            *W(out) = bank + *W((i << 2) + *W(sp) + 32);
        }
    }
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (voice, mode): a voice's hold (+24). Mode 0 lets go: one in the state 7 moves to the list at
   0x08E75434 with the state 4, one in the state 2, 3 or 6 has its hold cleared. Mode 1 sets the
   hold of one in the state 2, 3 or 6. Returns 0; 0x80450001 before the voices are set up,
   0x80540011 past them, 0x8054000A for another mode, and 0x80540012 for a voice not in use. */
SFA_FUNC(089F1E98, voice_hold) {
    uint32_t v = ARG(0), mode = ARG(1);
    c->r[7] = *W(VOICE_UP);
    c->r[29] -= 0x10;
    c->r[8] = v;
    *W(c->r[29]) = c->r[31];
    ARG(2) = 0x80450001u;
    if (c->r[7] == 0xFFFFFFFFu) { f_089F1F48(c); return; }
    ARG(2) = 0x80540011u;
    if (!(v < *W(VOICE_N))) { f_089F1F48(c); return; }
    ARG(2) = 0x8054000Au;
    if (!(mode < 2)) { f_089F1F48(c); return; }
    int64_t m = (int64_t)(int32_t)v * 0x68;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr voice = VOICES + c->lo;
    c->r[7] = voice;
    int32_t state = S8(voice + 1);
    ARG(0) = (uint32_t)state;
    ARG(2) = 0x80540012u;
    if (state == 0) { f_089F1F48(c); return; }
    if (mode != 0) {
        if (state == 3 || state == 2 || state == 6) { *W(voice + 24) = mode; f_089F1F44(c); }
        else { ARG(2) = 0; f_089F1F48(c); }
    } else if (state == 7) {
        ARG(0) = 0x08E74528u; ARG(1) = 0x08E755B4u; ARG(2) = 0x08E7451Cu; ARG(3) = 0x08E75434u;
        c->r[31] = 0x089F1F7Cu; f_089F2928(c);
        *B(c->r[2] + 1) = 4;
        *W(c->r[2] + 24) = 0;
        f_089F1F44(c);
    } else if (state == 3 || state == 2 || state == 6) {
        *W(voice + 24) = 0;
        f_089F1F44(c);
    } else { ARG(2) = 0; f_089F1F48(c); }
}

/* The end of a routine that starts a note on a voice (s0, number s2): its pitch is set, the
   voice's own hold copied (+100 to +12), its two pairs of volumes split by their pans and set,
   its envelope and the four volumes recorded, the four pan bytes kept, and the voice started.
   Returns 0, or -1 when the start fails, from a 0x30 frame that kept s0..s3 at +16. */
SFA_FUNC(089F2ABC, tail_note_start) {
    gaddr sp = c->r[29];
    gaddr t = c->r[16];
    c->r[31] = 0x089F2AC4u; f_089ECC5C(c);
    int64_t m = (int64_t)(int32_t)c->r[18] * 0x68;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr voice = c->lo + c->r[19] + 0x4534;
    c->r[11] = voice;
    c->r[12] = *W(voice + 100);
    *W(voice + 12) = c->r[12];
    ARG(0) = *W(t + 52); ARG(1) = *B(t + 96); ARG(2) = *B(t + 56); ARG(3) = *B(t + 97);
    c->r[8] = sp; c->r[9] = sp + 4;
    c->r[31] = 0x089F2AFCu; f_089F2E50(c);
    ARG(0) = *W(t + 60); ARG(1) = *B(t + 98); ARG(2) = *B(t + 64); ARG(3) = *B(t + 99);
    c->r[9] = sp + 0xC; c->r[8] = sp + 8;
    c->r[31] = 0x089F2B18u; f_089F2E50(c);
    ARG(3) = *W(sp + 8); c->r[8] = *W(sp + 12);
    ARG(0) = *B(t); ARG(1) = *W(sp); ARG(2) = *W(sp + 4);
    c->r[31] = 0x089F2B30u; f_089ECBF8(c);
    ARG(0) = *B(t); ARG(1) = *W(t + 68); ARG(2) = *W(t + 72);
    c->r[31] = 0x089F2B40u; f_089ECE78(c);
    ARG(0) = *B(t); ARG(1) = *W(sp); ARG(2) = *W(sp + 4); ARG(3) = *W(sp + 8); c->r[8] = *W(sp + 12);
    c->r[31] = 0x089F2B58u; f_089EDB58(c);
    c->r[3] = *B(t + 96);
    c->r[10] = *B(t + 97);
    c->r[19] = *B(t + 98);
    c->r[18] = *B(t + 99);
    ARG(0) = *B(t);
    *B(t + 8) = (uint8_t)c->r[3];
    *B(t + 9) = (uint8_t)c->r[10];
    *B(t + 10) = (uint8_t)c->r[19];
    *B(t + 11) = (uint8_t)c->r[18];
    c->r[31] = 0x089F2B80u; f_089ECAA0(c);
    c->r[2] = (int32_t)c->r[2] < 0 ? 0xFFFFFFFFu : 0;
    c->r[31] = *W(sp + 32);
    c->r[19] = *W(sp + 28);
    c->r[18] = *W(sp + 24);
    c->r[17] = *W(sp + 20);
    c->r[16] = *W(sp + 16);
    c->r[29] += 0x30;
}

/* The end of a routine that rounds a double's fraction: the carry in r2 is added to the high
   part (r11); past 29 bits both parts shift down one and the exponent (r9) goes up; the low part
   then comes down eight bits with the high part's low byte above it. */
SFA_FUNC(089FBEF8, tail_double_round) {
    c->r[11] = c->r[11] + c->r[2];
    if (0x1FFFFFFFu < c->r[11]) {
        c->r[10] = (c->r[10] >> 1) | (c->r[11] << 31);
        c->r[11] = c->r[11] >> 1;
        c->r[9] = c->r[9] + 1;
    }
    c->r[10] = c->r[10] >> 8;
    c->r[2] = c->r[11] << 24;
    c->r[10] = c->r[10] | c->r[2];
    f_089FBDEC(c);
}
