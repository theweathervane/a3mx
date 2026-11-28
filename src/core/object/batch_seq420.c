/* The sound engine's sequences: a note on, a pitch bend, and the pitch a bend gives a voice.
   Offsets stay numbers until something shows what they are.

   A sequence's record has, from +40, four bytes of its own levels (two volumes, each followed by
   its pan), at +44 a bit for each channel in use, and from +64 sixteen bytes for each channel:
   +0 its program, +1 its volume, +2 its expression, +3 its pan, +4 one more setting, +8 the
   group its notes belong to, +12 its pitch bend (0x2000 is none).
 */
#include "step.h"
#include "../guest_mem.h"

void f_089F27EC(Cpu *c); void f_089ED0F8(Cpu *c); void f_089EEC08(Cpu *c); void f_089ED71C(Cpu *c);
void f_089F06CC(Cpu *c); void f_089ED598(Cpu *c); void f_089EDC04(Cpu *c); void f_089EDF58(Cpu *c);
void f_089EDE14(Cpu *c);

/* The divisions the original does by multiplying: a signed number by 100, and unsigned ones by
   8191, 127 and 44100. */
static int32_t div100(int32_t n) {
    return (int32_t)(((int64_t)n * 0x51EB851F) >> 32 >> 5) - (n >> 31);
}
static uint32_t div8191(uint32_t n) {
    uint32_t hi = (uint32_t)(((uint64_t)n * 0x00080041u) >> 32);
    return (hi + ((n - hi) >> 1)) >> 12;
}
static uint32_t div127(uint32_t n) {
    uint32_t hi = (uint32_t)(((uint64_t)n * 0x02040811u) >> 32);
    return (hi + ((n - hi) >> 1)) >> 6;
}
static uint32_t div44100(uint32_t n) {
    return (uint32_t)(((uint64_t)n * 0xBE37C63Bu) >> 32) >> 15;
}

/* The pitch of a note with a fine tuning, from the routine at 0x089EEC08. */
static uint32_t pitch_of(Cpu *c, uint32_t a, int32_t fine_a, uint32_t note, int32_t fine, uint32_t back) {
    ARG(0) = a; ARG(1) = (uint32_t)fine_a; ARG(2) = note; ARG(3) = (uint32_t)fine;
    c->r[31] = back; f_089EEC08(c);
    return c->r[2];
}

/* (voice, bend): how far the bend moves the voice's pitch: the difference between its pitch as
   it is and with the bend, times the voice's rate (+44), over 44100. A bend above 0x2000 is
   scaled by the voice's range upward (+80) and one below by its range downward (+79); none, or
   a range of 0, gives 0. The voice's settings are read into the bottom of a 0x30 frame, which
   the end closes; keeps s0..s2. */
SFA_FUNC(089ED0A8, voice_bend_pitch) {
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 40) = c->r[18];
    uint32_t bend = ARG(1);
    c->r[18] = bend;
    ARG(2) = sp + 4; ARG(3) = sp + 8; ARG(1) = sp;
    c->r[8] = sp + 0xC; c->r[9] = sp + 0x10; c->r[10] = sp + 0x14; c->r[11] = sp + 0x18;
    *W(sp + 44) = c->r[31];
    *W(sp + 36) = c->r[17];
    *W(sp + 32) = c->r[16];
    c->r[31] = 0x089ED0E0u; f_089F27EC(c);
    int32_t fine = div100((int32_t)(*W(sp + 12) * 127));
    int32_t fine_a = div100((int32_t)(*W(sp + 20) * 127));
    ARG(0) = 0;
    if (bend >= 0x2001) {
        uint32_t range = *W(sp);
        if (range == 0) { f_089ED0F8(c); return; }
        uint32_t moved = div8191(((bend - 0x2000) * range) << 7);
        c->r[16] = 0x51EB851Fu;
        c->r[17] = moved + (uint32_t)fine;
        c->r[18] = moved;
        uint32_t plain = pitch_of(c, *H(sp + 16), (int16_t)fine_a, *H(sp + 8), (int16_t)fine, 0x089ED1A4u);
        c->r[18] = plain;
        uint32_t bent = pitch_of(c, *H(sp + 16), (int16_t)div100((int32_t)(*W(sp + 20) * 127)), *H(sp + 8),
                                 (int16_t)c->r[17], 0x089ED1E4u);
        ARG(0) = div44100((bent - plain) * *W(sp + 24));
        f_089ED0F8(c);
        return;
    }
    if (bend >= 0x2000) { f_089ED0F8(c); return; }
    uint32_t range = *W(sp + 4);
    if (range == 0) { f_089ED0F8(c); return; }
    c->r[17] = 0x51EB851Fu;
    uint32_t moved = (((0x2000 - bend) * range) << 7) >> 13;
    c->r[18] = moved;
    int32_t lowered = (int32_t)(moved + (uint32_t)fine);
    c->r[16] = (uint32_t)lowered;
    uint32_t plain = pitch_of(c, *H(sp + 16), (int16_t)fine_a, *H(sp + 8), (int16_t)fine, 0x089ED298u);
    c->r[18] = plain;
    int32_t whole = (int32_t)((uint32_t)lowered + ((uint32_t)(lowered >> 31) >> 25)) >> 7;
    uint32_t bent = pitch_of(c, *H(sp + 16), (int16_t)div100((int32_t)(*W(sp + 20) * 127)),
                             (*H(sp + 8) - (uint32_t)whole) & 0xFFFF,
                             (int16_t)((uint32_t)whole * 128 - (uint32_t)lowered), 0x089ED2F4u);
    ARG(0) = 0 - div44100((plain - bent) * *W(sp + 24));
    f_089ED0F8(c);
}

/* The voices in the mask (s1) from the one in s0 on: each has `level` set by the routine at
   0x089EDC04 and, when the channel (s3) has a bend (other than s4, 0x2000), the bend's pitch. */
static void note_voices_from(Cpu *c) {
    for (; c->r[16] < 0x20; c->r[16]++) {
        c->r[15] = c->r[17] >> (c->r[16] & 31);
        c->r[14] = c->r[15] & 1;
        ARG(0) = c->r[16];
        ARG(1) = c->r[18];
        if (c->r[14] == 0) continue;
        c->r[31] = 0x089F0884u; f_089EDC04(c);
        c->r[24] = *W(c->r[19] + 76);
        ARG(0) = c->r[16];
        ARG(1) = c->r[24];
        if (c->r[24] == c->r[20]) continue;
        c->r[31] = 0x089F089Cu; f_089ED0A8(c);
        ARG(1) = c->r[2];
        ARG(0) = c->r[16];
        c->r[31] = 0x089F08A8u; f_089EDF58(c);
    }
    c->r[31] = *W(c->r[29] + 36);
    f_089F06CC(c);
}

/* The rest of a note on, entered where its loop over the voices goes on. */
SFA_FUNC(089F0868, note_on_voices_rest) {
    note_voices_from(c);
}

/* (channel, note, velocity, sequence): a note on. Velocity 0 is a note off for the channel's
   group. A channel not in use does nothing. Else the two sides' volumes (the sequence's level
   times the channel's volume over 127, times its expression over 127, times the velocity over
   127) and pans (the sequence's and the channel's, less 0x40, kept in 0 to 127) are put at the
   bottom of the 0x30 frame and the note is played on the channel's program; each voice it
   started then gets its level and the channel's bend. Keeps s0..s4; the end closes the frame. */
SFA_FUNC(089F0684, sequence_note_on) {
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = c->r[18];
    uint32_t velocity = ARG(2) & 0xFF;
    c->r[18] = velocity;
    gaddr seq = ARG(3);
    c->r[14] = seq;
    *W(sp + 20) = c->r[17];
    uint32_t channel = ARG(0) & 0xF;
    c->r[15] = channel;
    uint32_t note = ARG(1) & 0xFF;
    c->r[17] = note;
    *W(sp + 36) = c->r[31];
    *W(sp + 32) = c->r[20];
    *W(sp + 28) = c->r[19];
    *W(sp + 16) = c->r[16];
    gaddr ch = seq + (channel << 4);
    if (velocity == 0) {
        ARG(2) = *W(ch + 72);
        ARG(0) = channel; ARG(1) = note;
        c->r[31] = 0x089F08C8u; f_089ED71C(c);
        c->r[31] = *W(sp + 36);
        f_089F06CC(c);
        return;
    }
    c->r[19] = channel << 4;
    if (((*W(seq + 44) >> channel) & 1) == 0) {
        c->r[31] = *W(sp + 36);
        f_089F06CC(c);
        return;
    }
    c->r[16] = ch;
    c->r[19] = ch + 0x40;
    uint32_t volume = *B(ch + 65), expression = *B(ch + 66), pan = *B(ch + 67);
    uint32_t side_a = (uint32_t)((int32_t)(*B(seq + 40) * volume) / 127) * expression;
    uint32_t side_b = (uint32_t)((int32_t)(*B(seq + 42) * volume) / 127) * expression;
    *B(sp + 3) = (uint8_t)MIN(MAX((int32_t)(*B(seq + 43) + pan) - 0x40, 0), 0x7F);
    *B(sp + 1) = (uint8_t)MIN(MAX((int32_t)(*B(seq + 41) + pan) - 0x40, 0), 0x7F);
    *W(sp + 4) = 0;
    *B(sp) = (uint8_t)div127(div127(side_a) * velocity);
    *B(sp + 2) = (uint8_t)div127(div127(side_b) * velocity);
    ARG(0) = (uint32_t)S8(seq + 9);
    ARG(1) = channel;
    ARG(2) = *B(ch + 64);
    ARG(3) = note;
    c->r[8] = *B(ch + 68);
    c->r[9] = sp;
    c->r[10] = *W(ch + 72);
    c->r[31] = 0x089F083Cu; f_089ED598(c);
    c->r[17] = c->r[2];
    if (c->r[2] == 0) {
        c->r[31] = *W(sp + 36);
        f_089F06CC(c);
        return;
    }
    c->r[19] = c->r[16];
    c->r[20] = 0x2000;
    c->r[16] = 0;
    note_voices_from(c);
}

/* The voices in the mask (s2) from the one in s0 on take the bend's pitch (s1 the bend); then
   0 is returned from the 0x10 frame that kept s0..s2. */
static void bend_voices_from(Cpu *c) {
    for (; c->r[16] < 0x20; c->r[16]++) {
        c->r[10] = c->r[18] >> (c->r[16] & 31);
        c->r[6] = c->r[10] & 1;
        ARG(0) = c->r[16];
        ARG(1) = c->r[17];
        if (c->r[6] == 0) continue;
        c->r[31] = 0x089F0E14u; f_089ED0A8(c);
        ARG(1) = c->r[2];
        ARG(0) = c->r[16];
        c->r[31] = 0x089F0E20u; f_089EDF58(c);
    }
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = 0;
    c->r[29] += 0x10;
}

SFA_FUNC(089F0DE4, pitch_bend_voices_rest) {
    bend_voices_from(c);
}

/* (channel, low, high, sequence): a pitch bend: the two seven-bit halves make the bend (kept to
   0x3FFF), which is kept for the channel and given to every voice sounding for its group.
   Returns 0. A 0x10 frame keeping s0..s2. */
SFA_FUNC(089F0D78, sequence_pitch_bend) {
    c->r[9] = ARG(2) & 0xFF;
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    c->r[8] = ARG(1) & 0xFF;
    c->r[3] = c->r[9] << 7;
    ARG(0) &= 0xF;
    *W(sp + 8) = c->r[18];
    gaddr ch = ARG(3) + (ARG(0) << 4);
    c->r[6] = ch;
    *W(sp + 4) = c->r[17];
    c->r[17] = c->r[3] | c->r[8];
    c->r[7] = 0x3FFF;
    if (c->r[17] >= 0x4000) c->r[17] = 0x3FFF;
    *W(sp + 12) = c->r[31];
    *W(sp) = c->r[16];
    *W(ch + 76) = c->r[17];
    ARG(1) = *W(ch + 72);
    c->r[31] = 0x089F0DC0u; f_089EDE14(c);
    c->r[18] = c->r[2];
    c->r[16] = 0;
    if (c->r[2] == 0) {
        c->r[31] = *W(sp + 12);
        c->r[18] = *W(sp + 8);
        c->r[17] = *W(sp + 4);
        c->r[16] = *W(sp);
        c->r[2] = 0;
        c->r[29] += 0x10;
        return;
    }
    bend_voices_from(c);
}
