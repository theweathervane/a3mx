/* Fifteen more of the library: the drawing list's commands for the list in use, a sound bank's
   entries looked up, and more ends of longer routines. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LIST 0x08E69348u /* points at the drawing list being written */
#define GE   0x08E69300u /* the drawing state */

void f_089E49F0(Cpu *c); void f_089E732C(Cpu *c); void f_089E8EE8(Cpu *c); void f_089EA70C(Cpu *c);
void f_089EA1E8(Cpu *c); void f_089E9A30(Cpu *c); void f_089EA4A4(Cpu *c); void f_089ECF90(Cpu *c);
void f_089EDA3C(Cpu *c); void f_089EDA4C(Cpu *c); void f_089EF4E4(Cpu *c); void f_089EF040(Cpu *c);
void f_089EF0C4(Cpu *c); void f_089EEF3C(Cpu *c); void f_089EF148(Cpu *c); void f_089F2928(Cpu *c);
void f_089F1D3C(Cpu *c); void f_089F2E78(Cpu *c); void f_089F2F0C(Cpu *c); void f_089FBCF8(Cpu *c);

/* (four or five words): the same call with the list in use put first. A 0x10 frame. */
static void with_list_5(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t a = ARG(0), b = ARG(1), d = ARG(2), e = ARG(3);
    uint32_t ra = c->r[31];
    c->r[10] = *W(LIST);
    c->r[11] = b; c->r[12] = d; c->r[13] = e; c->r[2] = a;
    c->r[29] -= 0x10;
    c->r[9] = c->r[8];
    ARG(0) = c->r[10]; ARG(1) = a; ARG(2) = b; ARG(3) = d;
    *W(c->r[29]) = ra;
    c->r[8] = e;
    c->r[31] = back; fn(c);
}

/* (v): the word at 0x08AD7CE0, behind a frame. */
SFA_FUNC(089E0BD8, word_8ad7ce0_set_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x089E0BE8u; f_089E49F0(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (w, h, offset, width): the display's mode with the format the drawing state has (+36); the five
   drawing lists (252 bytes each from the state's +76) then take the format and the size. A 0x10
   frame keeping s0. */
SFA_FUNC(089E7BD8, display_size_set) {
    uint32_t w = ARG(0), h = ARG(1), offset = ARG(2), width = ARG(3);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0;
    c->r[16] = GE;
    c->r[2] = w; c->r[3] = h; c->r[10] = offset;
    c->r[8] = width;
    *W(c->r[29] + 4) = ra;
    ARG(0) = *W(GE + 36); ARG(1) = w; ARG(2) = h; ARG(3) = offset;
    c->r[31] = 0x089E7C10u; f_089E732C(c);
    gaddr e = GE + 0x4C;
    for (int i = 0; i < 5; i++, e += 0xFC) {
        *W(e + 24) = *W(GE + 36);
        *W(e + 28) = *W(GE + 60);
        *W(e + 32) = *W(GE + 64);
    }
    ARG(1) = e; ARG(2) = 0xFFFFFFFFu;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* (kind, type, count, indices, vertices): a primitive on the list in use, and the hardware told
   how far the list has got. A 0x10 frame. */
SFA_FUNC(089E8010, list_primitive_here) {
    uint32_t ra = c->r[31];
    with_list_5(c, f_089E8EE8, 0x089E804Cu);
    c->r[31] = 0x089E8054u; f_089EA70C(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (colour): the colour in all three places (0x55 and 0x58, 0x56, 0x57) on the list in use. A
   0x10 frame. */
SFA_FUNC(089E8510, list_colour_all) {
    uint32_t ra = c->r[31];
    c->r[3] = 0x08E70000u;
    c->r[29] -= 0x10;
    ARG(2) = ARG(0);
    ARG(0) = *W(LIST);
    *W(c->r[29]) = ra;
    ARG(1) = 7;
    c->r[31] = 0x089E8530u; f_089EA1E8(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (level, w, h, width, address): a texture's level on the list in use. A 0x10 frame. */
SFA_FUNC(089E8914, list_texture_level_here) {
    uint32_t ra = c->r[31];
    with_list_5(c, f_089E9A30, 0x089E8950u);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (x, y, w, h): the scissor on the list in use. A 0x10 frame. */
SFA_FUNC(089E8BC0, list_scissor_here) {
    uint32_t x = ARG(0), y = ARG(1), w = ARG(2), h = ARG(3);
    uint32_t ra = c->r[31];
    c->r[3] = x; c->r[2] = y; c->r[10] = w;
    c->r[29] -= 0x10;
    c->r[8] = h;
    ARG(0) = *W(LIST); ARG(1) = x; ARG(2) = y;
    *W(c->r[29]) = ra;
    ARG(3) = w;
    c->r[31] = 0x089E8BF0u; f_089EA4A4(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The end of a routine: with bit 4 of r3 set the reverb's volumes take the record's +16 and +20
   (s0); a failure is the answer, and otherwise the record's first word says whether the switches
   follow. */
SFA_FUNC(089EDA30, tail_reverb_volumes) {
    c->r[7] = c->r[3] & 4;
    if (c->r[7] != 0) {
        ARG(1) = *W(c->r[16] + 20);
        ARG(0) = *W(c->r[16] + 16);
        c->r[31] = 0x089EDA80u; f_089ECF90(c);
        ARG(1) = c->r[2];
        if (c->r[2] != 0) { f_089EDA4C(c); return; }
        c->r[3] = *W(c->r[16]);
    }
    f_089EDA3C(c);
}

/* (i and five words): a sequence started, or 0x80450001 before the sound is. A 0x10 frame. */
SFA_FUNC(089EE24C, sequence_start_checked) {
    uint32_t ra = c->r[31];
    c->r[12] = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    c->r[11] = 0xFFFFFFFFu;
    *W(c->r[29]) = ra;
    c->r[10] = 0x80450001u;
    if (c->r[12] != 0xFFFFFFFFu) {
        c->r[31] = 0x089EE274u; f_089EF4E4(c);
        c->r[10] = c->r[2];
    }
    c->r[2] = c->r[10];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (bank, i, out): the i-th entry (16 bytes each from +32 of the chunk at the bank's +24, numbered
   from the chunk's +16 to its +20) goes to out. Returns 0, or -1 when it is not there or its
   first word is -1. A frame keeping s0, s1, with the chunk's place at its start. */
SFA_FUNC(089EEE44, bank_entry_16) {
    uint32_t i = ARG(1);
    gaddr out = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = i;
    ARG(1) = sp;
    *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[17] = out;
    c->r[31] = 0x089EEE64u; f_089EF040(c);
    uint32_t answer = 0xFFFFFFFFu;
    if ((int32_t)c->r[2] >= 0) {
        gaddr chunk = *W(sp);
        uint32_t first = *W(chunk + 16);
        c->r[8] = i - first;
        if (!(i < first)) {
            c->r[7] = *W(chunk + 20);
            c->r[2] = chunk + ((i - first) << 4);
            ARG(0) = c->r[2] + 0x20;
            if (!(c->r[7] < i)) {
                c->r[9] = *W(c->r[2] + 32);
                *W(out) = ARG(0);
                ARG(0) = ~c->r[9];
                if (ARG(0) != 0) answer = 0;
            }
        }
    }
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (bank, i, out): the same for the chunk at the bank's +20 with entries of 96 bytes: -1 when its
   first word and its +12 are both -1. */
SFA_FUNC(089EEEC4, bank_entry_96) {
    uint32_t i = ARG(1);
    gaddr out = ARG(2);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = c->r[16];
    c->r[16] = i;
    ARG(1) = sp;
    *W(sp + 20) = c->r[17]; *W(sp + 24) = c->r[31];
    c->r[17] = out;
    c->r[31] = 0x089EEEE4u; f_089EF0C4(c);
    ARG(2) = 0xFFFFFFFFu;
    if ((int32_t)c->r[2] >= 0) {
        gaddr chunk = *W(sp);
        uint32_t first = *W(chunk + 16);
        if (!(i < first)) {
            c->r[7] = *W(chunk + 20);
            c->r[2] = chunk + (i - first) * 96;
            ARG(0) = c->r[2] + 0x20;
            if (!(c->r[7] < i)) {
                c->r[3] = *W(c->r[2] + 32);
                c->r[10] = 0xFFFFFFFFu;
                *W(out) = ARG(0);
                if (c->r[3] != 0xFFFFFFFFu) ARG(2) = 0;
                else {
                    c->r[11] = *W(ARG(0) + 12);
                    if (c->r[11] != c->r[3]) ARG(2) = 0;
                }
            }
        }
    }
    c->r[31] = *W(sp + 24);
    f_089EEF3C(c);
}

/* (bank, i): 0 when the i-th word (i below 0x80) from +32 of the chunk at the bank's +16 is not
   -1; -1 otherwise. A frame keeping s0, with the chunk's place at its start. */
SFA_FUNC(089EEFE0, bank_word_there) {
    uint32_t i = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = i;
    *W(sp + 20) = ra;
    ARG(1) = sp;
    uint32_t answer = 0xFFFFFFFFu;
    *W(sp) = 0;
    if (i < 0x80) {
        c->r[31] = 0x089EF00Cu; f_089EF148(c);
        if ((int32_t)c->r[2] >= 0) {
            if (~*W((i << 2) + *W(sp) + 32) != 0) answer = 0;
        }
    }
    c->r[2] = answer;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* The end of a routine: the record with the key in r8 is moved from the list counted at r4 to
   the one at 0x08E75434 (its count at 0x08E7451C), gets 4 at its +1, and 0 is returned. */
SFA_FUNC(089F1D20, tail_list_move_a) {
    c->r[11] = 0x08E70000u; c->r[10] = 0x08E70000u;
    ARG(2) = 0x08E7451Cu; ARG(3) = 0x08E75434u;
    c->r[31] = 0x089F1D34u; f_089F2928(c);
    c->r[3] = c->r[2];
    c->r[2] = 4;
    f_089F1D3C(c);
}

/* The end of a routine: the same to the list at 0x08E75234 (its count at 0x08E7450C), the record
   getting 0 at its +1; 0 is returned from a 0x10 frame that kept s0. */
SFA_FUNC(089F2064, tail_list_move_b) {
    c->r[9] = 0x08E70000u; c->r[16] = 0x08E70000u;
    ARG(2) = 0x08E7450Cu; ARG(3) = 0x08E75234u;
    c->r[31] = 0x089F2078u; f_089F2928(c);
    *B(c->r[2] + 1) = 0;
    ARG(2) = 0;
    c->r[31] = *W(c->r[29] + 4);
    c->r[16] = *W(c->r[29]);
    c->r[2] = 0;
    c->r[29] += 0x10;
}

/* The end of a routine that splits a volume (r4) between left (the place in r8) and right (r9)
   by a pan: r6 + r7 - 0x40, kept between 0 and 0x7F. 0 is all left; below 64 the right gets the
   pan's share in 64ths; 64 is both in full; above it the left gets what is left of 0x7F. Both
   are then scaled. */
SFA_FUNC(089F2E50, tail_volume_pan) {
    uint32_t volume = ARG(0);
    c->r[2] = ARG(2) + ARG(3);
    int32_t pan = (int32_t)(c->r[2] - 0x40);
    c->r[10] = 0;
    if (pan < 0) pan = 0;
    c->r[3] = (uint32_t)pan;
    ARG(3) = 0x7F;
    if (pan > 0x7F) pan = 0x7F;
    ARG(2) = (uint32_t)pan;
    c->r[2] = pan < 64;
    if (pan == 0) {
        *W(c->r[8]) = volume;
        *W(c->r[9]) = 0;
        f_089F2E78(c);
    } else if (pan < 64) {
        int64_t m = (int64_t)(int32_t)volume * pan;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        *W(c->r[8]) = volume;
        ARG(2) = c->lo;
        c->r[3] = c->lo >> 6;
        *W(c->r[9]) = c->r[3];
        f_089F2E78(c);
    } else {
        c->r[3] = pan < 65;
        ARG(2) = 0x7F - (uint32_t)pan;
        if (pan < 65) *W(c->r[8]) = volume;
        else {
            int64_t m = (int64_t)(int32_t)volume * (int32_t)ARG(2);
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            c->r[12] = c->lo;
            c->r[11] = c->lo >> 6;
            *W(c->r[8]) = c->r[11];
        }
        f_089F2F0C(c);
    }
}

/* The end of a routine that builds a double: its fraction's high part comes down eight bits
   first. */
SFA_FUNC(089FBDEC, tail_fbdec) {
    c->r[11] = c->r[11] >> 8;
    f_089FBCF8(c);
}
