/* Eighteen more of the library: a sound bank's three chunks found by their tags, the scissor and
   texture commands of the drawing list, loops over the sound's voices that the original enters in
   the middle, and more ends of longer routines. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LIST 0x08E69348u /* points at the drawing list being written; its +8 is the next place */

void f_089F2E78(Cpu *c); void f_089F035C(Cpu *c); void f_089FBCFC(Cpu *c); void f_089EA524(Cpu *c);
void f_089ECF44(Cpu *c); void f_089EDA4C(Cpu *c); void f_089EA460(Cpu *c); void f_089F295C(Cpu *c);
void f_089EF08C(Cpu *c); void f_089EF110(Cpu *c); void f_089EF194(Cpu *c);

static uint32_t word_at(gaddr a) {
    return (uint32_t)*B(a) | ((uint32_t)*B(a + 1) << 8) | ((uint32_t)*B(a + 2) << 16) | ((uint32_t)*B(a + 3) << 24);
}

/* (v): the word at 0x08AD7CE0. */
SFA_FUNC(089E49F0, word_8ad7ce0_set) {
    c->r[5] = 0x08AD0000u;
    *W(0x08AD7CE0u) = ARG(0);
}

/* (on): the command 0x50 on the drawing list. A 0x10 frame. */
SFA_FUNC(089E85E4, list_cmd_50_here) {
    uint32_t ra = c->r[31];
    c->r[3] = 0x08E70000u;
    ARG(1) = ARG(0);
    ARG(0) = *W(LIST);
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x089E8600u; f_089EA524(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (a, b, d, e): the three texture commands (0xC2, 0xC3, 0xCB) on the drawing list. A 0x10
   frame. */
SFA_FUNC(089E8740, list_cmd_c2_c3_cb_here) {
    uint32_t a = ARG(0), b = ARG(1), d = ARG(2), e = ARG(3);
    uint32_t ra = c->r[31];
    c->r[3] = a; c->r[2] = b; c->r[10] = d;
    c->r[29] -= 0x10;
    c->r[8] = e;
    ARG(0) = *W(LIST); ARG(1) = a; ARG(2) = b;
    *W(c->r[29]) = ra;
    ARG(3) = d;
    c->r[31] = 0x089E8770u; f_089EA460(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (list, level, w, h, width, address): a texture's level: its address in two commands (0xA0 and
   0xA8 past the level, with the buffer's width), its size as two powers of two (0xB8 past the
   level), and 0xCB. */
SFA_FUNC(089E9A30, list_texture_level) {
    gaddr list = ARG(0);
    uint32_t level = ARG(1), w = ARG(2), h = ARG(3), width = c->r[8], addr = c->r[9];
    c->r[14] = *W(list + 8);
    c->r[10] = 0x1F;
    uint32_t first = ((level + 0xA0) << 24) | (addr & 0x00FFFFFFu);
    uint32_t second = ((level + 0xA8) << 24) | (((addr >> 24) & 0xF) << 16) | width;
    c->r[12] = CLZ(w);
    c->r[11] = 0x1F - c->r[12];
    uint32_t third = ((level + 0xB8) << 24) | ((0x1F - CLZ(h)) << 8) | c->r[11];
    c->r[24] = (0x1F - CLZ(h)) << 8;
    c->r[15] = (level + 0xB8) << 24;
    c->r[25] = (level + 0xA8) << 24;
    c->r[13] = c->r[25] | (((addr >> 24) & 0xF) << 16);
    c->r[2] = c->r[15] | c->r[24];
    *W(c->r[14]) = first;
    ARG(2) = c->r[14] + 4;
    c->r[9] = second;
    ARG(3) = third;
    c->r[8] = ARG(2) + 0xC;
    c->r[3] = 0xCB000000u;
    *W(list + 8) = c->r[8];
    *W(ARG(2)) = second;
    *W(ARG(2) + 4) = third;
    *W(ARG(2) + 8) = 0xCB000000u;
}

/* (list, x, y, w, h): the scissor: the rectangle is kept at the list's +40 to +52, and while the
   list's +36 is set the commands 0xD4, 0xD5, 0x15 and 0x16 put it on the list. */
SFA_FUNC(089EA4A4, list_scissor) {
    gaddr list = ARG(0);
    uint32_t x = ARG(1), y = ARG(2), w = ARG(3), h = c->r[8];
    c->r[13] = 0xD4000000u;
    c->r[25] = (y << 10) | x;
    c->r[2] = y + h;
    c->r[9] = y + h - 1;
    c->r[12] = c->r[25] | 0xD4000000u;
    c->r[24] = x + w;
    c->r[13] = *W(list + 36);
    c->r[14] = x + w - 1;
    c->r[10] = list;
    c->r[15] = (c->r[9] << 10) | c->r[14];
    c->r[3] = 0xD5000000u;
    c->r[11] = c->r[15] | 0x16000000u;
    *W(list + 40) = x;
    ARG(0) = c->r[15] | 0xD5000000u;
    *W(list + 44) = y;
    *W(list + 48) = c->r[14];
    *W(list + 52) = c->r[9];
    if (c->r[13] != 0) {
        c->r[8] = *W(list + 8);
        ARG(2) = 0x15000000u;
        ARG(1) = c->r[8] + 4;
        ARG(3) = ARG(1) + 0xC;
        *W(c->r[8]) = c->r[12];
        *W(list + 8) = ARG(3);
        *W(ARG(1)) = ARG(0);
        *W(ARG(1) + 4) = 0x15000000u;
        *W(ARG(1) + 8) = c->r[11];
    }
}

/* The end of a routine: with bit 8 of r3 set the reverb's switches take the record's +24 and
   +28 (s0), and the answer is theirs when they fail; 0 otherwise. */
SFA_FUNC(089EDA3C, tail_reverb_switches) {
    c->r[8] = c->r[3] & 8;
    if (c->r[8] != 0) {
        ARG(1) = *W(c->r[16] + 28);
        ARG(0) = *W(c->r[16] + 24);
        c->r[31] = 0x089EDA68u; f_089ECF44(c);
        ARG(1) = c->r[2];
        if (c->r[2] == 0) ARG(1) = 0;
    } else ARG(1) = 0;
    f_089EDA4C(c);
}

/* The end of a routine that reports a voice: thirteen fields of the voice in r7 go to the report
   in s0, and 0 is returned from a 0x20 frame that kept s0, s1 at +16. */
SFA_FUNC(089EEB70, tail_voice_report) {
    gaddr v = c->r[7], out = c->r[16];
    gaddr sp = c->r[29];
    c->r[17] = *W(v + 32);
    ARG(0) = 0;
    *W(out + 24) = c->r[17];
    c->r[15] = *B(v + 36); *B(out + 28) = (uint8_t)c->r[15];
    c->r[14] = *W(v + 40); *W(out + 32) = c->r[14];
    c->r[13] = *B(v + 44); *B(out + 36) = (uint8_t)c->r[13];
    c->r[12] = *W(v + 80); *W(out + 40) = c->r[12];
    c->r[11] = *W(v + 84); *W(out + 44) = c->r[11];
    c->r[10] = *B(v + 56); *B(out + 48) = (uint8_t)c->r[10];
    ARG(2) = *B(v + 48); *B(out + 49) = (uint8_t)ARG(2);
    ARG(1) = *B(v + 52); *B(out + 50) = (uint8_t)ARG(1);
    c->r[9] = *W(v + 8); *W(out + 56) = c->r[9];
    c->r[8] = *W(v + 4); *W(out + 60) = c->r[8];
    c->r[3] = *B(v + 64); *B(out + 51) = (uint8_t)c->r[3];
    c->r[2] = *B(v + 68); *B(out + 52) = (uint8_t)c->r[2];
    c->r[31] = *W(sp + 24);
    c->r[17] = *W(sp + 20);
    c->r[16] = *W(sp + 16);
    c->r[2] = 0;
    c->r[29] += 0x20;
}

/* (bank, out): one chunk of a sound bank. The bank must start with the tag at 0x08A028C4; the
   chunk's place is the bank plus the word at `off`, unless that is -1, and goes to out. r7 is 0
   when the chunk starts with the tag at `tag`, else -1; the frame's end (`tail`) returns it. */
static void bank_chunk(Cpu *c, gaddr tag, uint32_t off, void (*tail)(Cpu *)) {
    gaddr bank = ARG(0), out = ARG(1);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[9] = tag;
    c->r[3] = word_at(tag);
    *W(sp) = c->r[3];
    c->r[7] = 0xFFFFFFFFu;
    if (bank != 0) {
        c->r[9] = 0x08A028C4u;
        c->r[3] = word_at(0x08A028C4u);
        *W(sp + 16) = c->r[3];
        c->r[2] = *W(bank);
        ARG(2) = *W(sp + 16);
        if (c->r[2] == ARG(2)) {
            ARG(2) = 0xFFFFFFFFu;
            if (out != 0) {
                c->r[8] = *W(bank + off);
                ARG(0) = bank + c->r[8];
                if (c->r[8] != 0xFFFFFFFFu) {
                    *W(out) = ARG(0);
                    c->r[10] = *W(ARG(0));
                    ARG(1) = *W(sp);
                    ARG(0) = c->r[10] ^ ARG(1);
                    if (ARG(0) == 0) c->r[7] = 0;
                }
            }
        }
    }
    tail(c);
}

/* (bank, out): the bank's chunk at +24, with the tag at 0x08A028C0. */
SFA_FUNC(089EF040, bank_chunk_24) {
    bank_chunk(c, 0x08A028C0u, 24, f_089EF08C);
}

/* (bank, out): the bank's chunk at +20, with the tag at 0x08A028C8. */
SFA_FUNC(089EF0C4, bank_chunk_20) {
    bank_chunk(c, 0x08A028C8u, 20, f_089EF110);
}

/* (bank, out): the bank's chunk at +16, with the tag at 0x08A028CC. */
SFA_FUNC(089EF148, bank_chunk_16) {
    bank_chunk(c, 0x08A028CCu, 16, f_089EF194);
}

/* The end of a routine: r2 goes to +28 of the track in s0, then the event's end. */
SFA_FUNC(089F0358, tail_f0358) {
    *W(c->r[16] + 28) = c->r[2];
    f_089F035C(c);
}

/* Whether a voice's state (its +1) is one the loops below take: 2, 3 or 6, or one of the three
   (four) the caller keeps in registers. */
static int state_taken(uint32_t state, uint32_t a, uint32_t b, uint32_t d) {
    return state - 2 < 2 || state == 6 || state == a || state == b || state == d;
}

/* The rest of a loop over the voices (0x68 bytes each, r7 the one before the next, r8 of r10
   done, r9 saying whether to go on): the bits (r15 shifted by the voice's number) of those in a
   taken state whose +92 and +93 are the two bytes at r4 and whose +16 is r5 are gathered in r11
   and returned. */
SFA_FUNC(089F22D4, voices_bits_by_pair) {
    for (;;) {
        int more = c->r[9] != 0;
        c->r[7] += 0x68;
        if (!more) break;
        gaddr v = c->r[7];
        ARG(2) = (uint32_t)(int32_t)S8(v + 1);
        c->r[8] = c->r[8] + 1;
        c->r[2] = ARG(2) - 2;
        c->r[24] = ARG(2) ^ 6;
        c->r[25] = c->r[2] < 2;
        c->r[2] = c->r[24] < 1;
        c->r[24] = c->r[25] | c->r[2];
        c->r[9] = c->r[8] < c->r[10];
        if (!state_taken(ARG(2), c->r[12], c->r[13], c->r[14])) continue;
        c->r[25] = *B(v + 92);
        ARG(2) = *B(ARG(0));
        if (c->r[25] != ARG(2)) continue;
        c->r[2] = *B(v + 93);
        c->r[24] = *B(ARG(0) + 1);
        if (c->r[2] != c->r[24]) continue;
        c->r[3] = *W(v + 16);
        if (c->r[3] != ARG(1)) continue;
        c->r[25] = *B(v);
        ARG(2) = c->r[15] << (c->r[25] & 31);
        c->r[11] |= ARG(2);
    }
    ARG(2) = c->r[11];
    c->r[2] = c->r[11];
}

/* The same over voices at r6 (r8 of r9 done, r7 saying whether to go on): the bits (r13 shifted)
   of those in a taken state whose +16 is r4 are gathered in r10 and returned. */
SFA_FUNC(089F23A4, voices_bits_by_owner) {
    for (;;) {
        int more = c->r[7] != 0;
        ARG(2) += 0x68;
        if (!more) break;
        gaddr v = ARG(2);
        ARG(1) = (uint32_t)(int32_t)S8(v + 1);
        c->r[8] = c->r[8] + 1;
        c->r[15] = ARG(1) ^ 6;
        c->r[2] = ARG(1) - 2;
        c->r[24] = c->r[2] < 2;
        c->r[25] = c->r[15] < 1;
        c->r[15] = c->r[24] | c->r[25];
        ARG(3) = c->r[8] < c->r[9];
        if (!state_taken(ARG(1), c->r[11], c->r[12], c->r[14])) continue;
        c->r[3] = *W(v + 16);
        if (c->r[3] != ARG(0)) continue;
        c->r[24] = *B(v);
        ARG(1) = c->r[13] << (c->r[24] & 31);
        c->r[10] |= ARG(1);
    }
    ARG(1) = c->r[10];
    c->r[2] = c->r[10];
}

/* The same over voices at r7 (r9 of r10 done, r8 saying whether to go on): those whose +92 is r4
   and +16 is r5, gathered in r11 with r15 shifted. */
SFA_FUNC(089F2460, voices_bits_by_byte) {
    for (;;) {
        int more = c->r[8] != 0;
        c->r[7] += 0x68;
        if (!more) break;
        gaddr v = c->r[7];
        ARG(2) = (uint32_t)(int32_t)S8(v + 1);
        c->r[9] = c->r[9] + 1;
        c->r[2] = ARG(2) - 2;
        c->r[24] = ARG(2) ^ 6;
        c->r[25] = c->r[2] < 2;
        c->r[2] = c->r[24] < 1;
        c->r[24] = c->r[25] | c->r[2];
        c->r[8] = c->r[9] < c->r[10];
        if (!state_taken(ARG(2), c->r[12], c->r[13], c->r[14])) continue;
        c->r[3] = *B(v + 92);
        if (c->r[3] != ARG(0)) continue;
        ARG(2) = *W(v + 16);
        if (ARG(2) != ARG(1)) continue;
        c->r[24] = *B(v);
        c->r[25] = c->r[15] << (c->r[24] & 31);
        c->r[11] |= c->r[25];
    }
    ARG(2) = c->r[11];
    c->r[2] = c->r[11];
}

/* The same with a fourth state of the caller's (r15) taken as well, and r24 shifted. */
SFA_FUNC(089F2534, voices_bits_by_byte_4) {
    for (;;) {
        int more = c->r[8] != 0;
        c->r[7] += 0x68;
        if (!more) break;
        gaddr v = c->r[7];
        ARG(2) = (uint32_t)(int32_t)S8(v + 1);
        c->r[9] = c->r[9] + 1;
        c->r[2] = ARG(2) - 2;
        c->r[25] = ARG(2) ^ 6;
        c->r[3] = c->r[2] < 2;
        c->r[2] = c->r[25] < 1;
        c->r[25] = c->r[3] | c->r[2];
        c->r[8] = c->r[9] < c->r[10];
        if (!state_taken(ARG(2), c->r[12], c->r[13], c->r[14]) && ARG(2) != c->r[15]) continue;
        c->r[3] = *B(v + 92);
        if (c->r[3] != ARG(0)) continue;
        ARG(2) = *W(v + 16);
        if (ARG(2) != ARG(1)) continue;
        c->r[3] = *B(v);
        c->r[25] = c->r[24] << (c->r[3] & 31);
        c->r[11] |= c->r[25];
    }
    ARG(2) = c->r[11];
    c->r[2] = c->r[11];
}

/* (count place, list, .., .., key): the first of the list's entries (words pointing at records)
   whose record starts with the byte `key` is looked for, and the list's end takes it out (r3 its
   number, r12 the record, 0 when there is none). */
SFA_FUNC(089F2928, list_find_by_byte) {
    c->r[11] = *W(ARG(0));
    c->r[12] = 0;
    c->r[3] = 0;
    if (c->r[11] != 0) {
        c->r[9] = ARG(1);
        c->r[10] = *W(c->r[9]);
        for (;;) {
            c->r[13] = *B(c->r[10]);
            c->r[9] += 4;
            if (c->r[13] == c->r[8]) { c->r[12] = c->r[10]; break; }
            c->r[3] = c->r[3] + 1;
            c->r[10] = c->r[3] < c->r[11];
            if (c->r[10] == 0) break;
            c->r[10] = *W(c->r[9]);
        }
    }
    f_089F295C(c);
}

/* The end of a routine: r4 goes to the place in r9, then both volumes are scaled. */
SFA_FUNC(089F2F0C, tail_f2f0c) {
    *W(c->r[9]) = ARG(0);
    f_089F2E78(c);
}

/* The end of a routine that builds a double, with its top word taken from the frame's +4. */
SFA_FUNC(089FBCF8, tail_fbcf8) {
    c->r[2] = *W(c->r[29] + 4);
    f_089FBCFC(c);
}
