/* Four more of the library: a sound effect asked for, a drawing list finished, the dither matrix,
   and a sequence's header read. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LIST 0x08E69348u /* points at the drawing list being written; its +8 is the next place */
#define GE   0x08E69300u /* the drawing state */

void f_089E57D0(Cpu *c); void f_089EA70C(Cpu *c); void f_089E7008(Cpu *c);

/* (entry, bank, n, priority): one of the sixteen sound entries at 0x08AD3AF8 plays the n-th effect
   of a bank (below 6; 0x1204 bytes each from 0x08ACCEE0, the count at +4096 and the effects from
   +4100): a request of kind 1 goes to the first callback table with the entry's number, the
   effect's samples (in the bank's data, listed at 0x08ACCEB0) and size, the entry's volume (+4)
   split left and right by its pan (+8, -0x7F to 0x7F), and the effect's pitch scaled by the
   entry's (+12) over 44100. The entry keeps the priority. Returns 0. A 0x30 frame keeping s0,
   s1. */
SFA_FUNC(089E2174, entry_effect_play) {
    gaddr entry = ARG(0);
    int32_t bank = (int8_t)ARG(1), n = (int8_t)ARG(2), pr = (int8_t)ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s1; *W(sp + 32) = s0; *W(sp + 40) = ra;
    c->r[17] = (uint32_t)pr;
    gaddr e = 0x08AD3AF8u;
    uint32_t idx = 0;
    while (idx < 16 && e != entry) { idx++; e += 0x14; }
    c->r[16] = e;
    if (idx != 16 && bank < 6) {
        gaddr b = 0x08ACCEE0u + (uint32_t)bank * 0x1204;
        if (n < (int32_t)*W(b + 4096)) {
            gaddr effect = *W(b + ((uint32_t)n << 2) + 4100);
            uint32_t place = *W(effect);
            if (place != 0xFFFFFFFFu) {
                int32_t pan = (int32_t)*W(e + 8), volume = (int32_t)*W(e + 4);
                uint32_t left_n = (uint32_t)(volume * (0x7F - pan));
                uint32_t right_n = (uint32_t)(volume * (pan + 0x7F));
                uint32_t pitch_n = *W(effect + 4) * *W(e + 12);
                uint32_t data = *W(0x08ACCEB0u + ((uint32_t)bank << 3)) + place;
                uint32_t size = *W(effect + 8);
                DIV(c, left_n, 0x7F);
                uint32_t left = c->lo;
                DIV(c, right_n, 0x7F);
                uint32_t right = c->lo;
                DIVU(c, pitch_n, 0xAC44);
                ARG(0) = idx; ARG(1) = data; ARG(2) = size; ARG(3) = left;
                c->r[8] = right; c->r[9] = c->lo;
                c->r[31] = 0x089E22CCu; f_089E57D0(c);
                *W(e + 16) = (uint32_t)pr;
            }
        }
    }
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* (arg): the drawing list in use is finished, by its kind (the drawing state's +12): the first
   ends with the commands 0x0F (with arg) and 0x0C and tells the hardware; a called one ends with
   0x0B, or with 0x0E 0x12 and 0x0C when the state's +16 is 1; the others with 0x0F and 0x0C. The
   list's own end follows. 0x80000021 for a list that is already ended (+116) and 0x80000107 for
   a kind past 4. A 0x10 frame keeping s0, s1. */
SFA_FUNC(089E6F70, list_finish) {
    uint32_t arg = ARG(0) & 0xFFFF;
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->r[16];
    c->r[16] = 0x08E70000u;
    *W(sp + 8) = c->r[31];
    c->r[7] = arg;
    *W(sp + 4) = c->r[17];
    gaddr list = *W(GE + 72);
    uint32_t error = 0;
    uint32_t kind = *W(GE + 12);
    if (*W(list + 116) != 0) error = 0x80000021u;
    else if (!(kind < 5)) error = 0x80000107u;
    else if (kind == 1 || kind == 3) {
        gaddr p = *W(list + 8);
        if (*W(GE + 16) == 1) {
            *W(p) = 0x0E120000u;
            *W(list + 8) = p + 8;
            *W(p + 4) = 0x0C000000u;
        } else {
            *W(p) = 0x0B000000u;
            *W(list + 8) = p + 4;
        }
    } else {
        gaddr p = *W(list + 8);
        c->r[17] = p;
        *W(p) = arg | 0x0F000000u;
        *W(list + 8) = p + 8;
        *W(p + 4) = 0x0C000000u;
        if (kind == 0) {
            c->r[31] = 0x089E7000u; f_089EA70C(c);
            if ((int32_t)c->r[2] < 0) error = c->r[2];
        }
    }
    if (error != 0) {
        c->r[31] = *W(sp + 8);
        c->r[17] = *W(sp + 4);
        c->r[16] = *W(sp);
        c->r[2] = error;
        c->r[29] += 0x10;
        return;
    }
    f_089E7008(c);
}

/* (matrix): the dither matrix: sixteen words, four bits each, in the four commands 0xE2 to 0xE5
   of the drawing list. */
SFA_FUNC(089E75A4, list_dither_matrix) {
    gaddr m = ARG(0);
    gaddr list = *W(LIST);
    gaddr p = *W(list + 8);
    uint32_t cmd = 0;
    c->r[11] = list;
    for (uint32_t k = 0; k < 4; k++) {
        gaddr row = m + 16 * k;
        cmd = ((*W(row + 12) & 0xF) << 12) | ((*W(row + 8) & 0xF) << 8) | ((*W(row + 4) & 0xF) << 4)
              | (*W(row) & 0xF) | ((0xE2 + k) << 24);
        if (k == 3) *W(list + 8) = p + 16;
        *W(p + 4 * k) = cmd;
    }
    c->r[10] = p + 4;
    c->r[9] = p + 16;
    ARG(0) = cmd;
}

/* The two or four bytes at a, most significant first. */
static uint32_t be16(gaddr a) { return ((uint32_t)*B(a) << 8) | *B(a + 1); }
static uint32_t be32(gaddr a) { return (be16(a) << 16) | be16(a + 2); }
/* The four bytes at a as they stand, for comparing with a tag. */
static uint32_t le32(gaddr a) {
    return (uint32_t)*B(a) | ((uint32_t)*B(a + 1) << 8) | ((uint32_t)*B(a + 2) << 16) | ((uint32_t)*B(a + 3) << 24);
}

/* (data, track): a standard sequence file of one track is taken in: "MThd" with the format 0, its
   time division to the track's +10, then "MTrk" with its length to +24 and the place of its first
   event (0x16 past the start) to +28; the file itself is kept at +4. Returns 0, or 0x80540005
   for anything else. A 0x40 frame the bytes are turned round in. */
SFA_FUNC(089F0554, sequence_file_open) {
    gaddr data = ARG(0), t = ARG(1);
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    uint32_t answer = 0x80540005u;
    c->r[9] = t; c->r[7] = data;
    *W(sp + 48) = le32(data);
    if (*W(sp + 48) == *W(0x08A028D0u)) {
        *H(sp) = (uint16_t)be16(data + 8);
        if (*H(sp) == 0) {
            *H(sp + 16) = (uint16_t)be16(data + 12);
            *W(sp + 48) = le32(data + 14);
            if (*W(sp + 48) == *W(0x08A028D8u)) {
                *W(sp + 32) = be32(data + 18);
                *W(t + 28) = data + 0x16;
                *H(t + 10) = *H(sp + 16);
                *W(t + 24) = *W(sp + 32);
                *W(t + 4) = data;
                answer = 0;
            }
        }
    }
    c->r[8] = answer;
    c->r[2] = answer;
    c->r[29] += 0x40;
}

#define VOICES   0x08E74534u /* the sound's voices, 0x68 bytes each */
#define VOICE_N  0x08E74508u /* how many there are */
#define VOICE_UP 0x08AD81ECu /* -1 until they are set up */

void f_089F2928(Cpu *c); void f_089F1D3C(Cpu *c); void f_089F1D20(Cpu *c); void f_089F1CE0(Cpu *c);

/* A voice moves from the list (count, words) to another and takes the state given. */
static void voice_move(Cpu *c, gaddr count, gaddr words, gaddr to_count, gaddr to_words, uint32_t state, uint32_t back) {
    ARG(0) = count; ARG(1) = words; ARG(2) = to_count; ARG(3) = to_words;
    c->r[31] = back; f_089F2928(c);
    c->r[3] = c->r[2];
    c->r[2] = state;
    f_089F1D3C(c);
}

/* (voice): a voice is let go, by its state (+1) and whether it is held (+24): 2 goes to the state
   1 (9 when held), 3 to 4 (7 when held), 6 to 4 (8 when held), each moving to that state's list;
   the others stay. Returns 0; 0x80450001 before the voices are set up, 0x80540011 past them, and
   0x80540012 for one not in use. A 0x10 frame. */
SFA_FUNC(089F1C68, voice_let_go) {
    uint32_t v = ARG(0);
    c->r[6] = *W(VOICE_UP);
    c->r[29] -= 0x10;
    c->r[8] = v;
    *W(c->r[29]) = c->r[31];
    uint32_t error = 0;
    int32_t state = 0;
    gaddr voice = 0;
    if (c->r[6] == 0xFFFFFFFFu) error = 0x80450001u;
    else if (!(v < *W(VOICE_N))) error = 0x80540011u;
    else {
        int64_t m = (int64_t)(int32_t)v * 0x68;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        voice = VOICES + c->lo;
        state = S8(voice + 1);
        if (state == 0) error = 0x80540012u;
    }
    if (error != 0 || (state != 2 && state != 3 && state != 6 && !(state < 3))) {
        c->r[31] = *W(c->r[29]);
        c->r[2] = error;
        c->r[29] += 0x10;
        return;
    }
    uint32_t held = state == 2 || state == 3 || state == 6 ? *W(voice + 24) : 0;
    if (state == 2) {
        if (held == 0) voice_move(c, 0x08E74514u, 0x08E75334u, 0x08E74510u, 0x08E752B4u, 1, 0x089F1DECu);
        else voice_move(c, 0x08E74514u, 0x08E75334u, 0x08E74530u, 0x08E756B4u, 9, 0x089F1E18u);
    } else if (state == 3) {
        if (held == 0) { ARG(0) = 0x08E74518u; ARG(1) = 0x08E753B4u; f_089F1D20(c); }
        else voice_move(c, 0x08E74518u, 0x08E753B4u, 0x08E74528u, 0x08E755B4u, 7, 0x089F1DB0u);
    } else if (state == 6) {
        if (held == 0) { ARG(0) = 0x08E74524u; ARG(1) = 0x08E75534u; f_089F1D20(c); }
        else voice_move(c, 0x08E74524u, 0x08E75534u, 0x08E7452Cu, 0x08E75634u, 8, 0x089F1D64u);
    } else f_089F1CE0(c);
}
