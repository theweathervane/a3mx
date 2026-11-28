/* Another round of the library: the frame's pools turned over, commands of the drawing list, the
   sound core's voices and output behind their guards, a sequence's variable-length number, and
   more ends of longer routines. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LIST     0x08E69348u /* points at the drawing list being written; its +8 is the next place */
#define GE       0x08E69300u /* the drawing state the list's pointer belongs to (LIST is its +72) */
#define SAS      0x08E73040u /* the sound core the voices belong to */
#define SAS_UP   0x08E73000u /* set once the sound core is started */
#define SAS_DOWN 0x80420100u /* the answer while it is not */

void f_089EA740(Cpu *c); void f_089EECF8(Cpu *c);
void hle_sceGeListUpdateStallAddr(Cpu *c); void hle_sceAudioOutputPannedBlocking(Cpu *c);
void hle___sceSasCore(Cpu *c); void hle___sceSasSetVoice(Cpu *c); void hle___sceSasSetSimpleADSR(Cpu *c);

/* The frame's pools turn over: the byte at 0x08E65C64 flips, the used counts at 0x08E65C40 and
   0x08E65C44 are cleared, and 0x08E65C48 points at the new frame's 0x18000 bytes from
   0x08D65C40. */
SFA_FUNC(089E4544, frame_pools_flip) {
    *B(0x08E65C64u) = (uint8_t)(*B(0x08E65C64u) ^ 1);
    uint32_t frame = *B(0x08E65C64u);
    c->r[6] = 0x20000;
    int64_t m = (int64_t)(int32_t)frame * 0x18000;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    *W(0x08E65C44u) = 0;
    *H(0x08E65C40u) = 0;
    ARG(0) = c->lo + 0x08D65C40u;
    ARG(1) = 0x08E60000u;
    *W(0x08E65C48u) = ARG(0);
}

/* (a, b): the drawing list's commands 0x9E with the low 24 bits of a, and 0x9F with b and four
   more bits of a; both are kept at the drawing state's +52 and +56. */
SFA_FUNC(089E7DD8, list_cmd_9e_9f) {
    uint32_t a = ARG(0), b = ARG(1);
    c->r[10] = GE;
    c->r[13] = *W(GE + 72);
    c->r[12] = a >> 24;
    c->r[14] = *W(c->r[13] + 8);
    c->r[3] = 0x00FFFFFFu;
    c->r[24] = c->r[12] & 0xF;
    c->r[25] = a & 0x00FFFFFFu;
    c->r[7] = c->r[24] << 16;
    c->r[15] = c->r[25] | 0x9E000000u;
    c->r[6] = c->r[7] | b;
    c->r[11] = c->r[14] + 4;
    c->r[9] = 0x9F000000u;
    c->r[12] = c->r[6] | 0x9F000000u;
    c->r[2] = c->r[11] + 4;
    *W(c->r[14]) = c->r[15];
    *W(c->r[13] + 8) = c->r[2];
    *W(c->r[11]) = c->r[12];
    *W(GE + 56) = b;
    *W(GE + 52) = a;
}

/* (a, b, d): the drawing list's commands 0xC0 with two bits of a under the list's +108, and 0xC1
   with b under d; the two bits are kept at the list's +112. */
SFA_FUNC(089E886C, list_cmd_c0_c1) {
    c->r[9] = *W(LIST);
    c->r[12] = ARG(0) & 3;
    c->r[2] = ARG(2) << 8;
    c->r[24] = *W(c->r[9] + 108);
    c->r[8] = *W(c->r[9] + 8);
    c->r[10] = c->r[2] | ARG(1);
    c->r[15] = c->r[24] << 8;
    c->r[3] = c->r[8] + 4;
    c->r[13] = c->r[15] | c->r[12];
    c->r[14] = 0xC0000000u;
    c->r[11] = 0xC1000000u;
    c->r[7] = c->r[13] | 0xC0000000u;
    ARG(0) = c->r[10] | 0xC1000000u;
    ARG(1) = c->r[3] + 4;
    *W(c->r[9] + 112) = c->r[12];
    *W(c->r[9] + 8) = ARG(1);
    *W(c->r[8]) = c->r[7];
    *W(c->r[3]) = ARG(0);
}

/* (a, b): the drawing list's commands 0xB0 and 0xB1 with the address b in two parts, and 0xC4
   with a. */
SFA_FUNC(089E8A28, list_cmd_b0_b1_c4) {
    uint32_t a = ARG(0), b = ARG(1);
    c->r[11] = *W(LIST);
    c->r[25] = 0x00FFFFFFu;
    c->r[13] = *W(c->r[11] + 8);
    c->r[15] = b & 0x00FFFFFFu;
    c->r[14] = b >> 24;
    c->r[24] = 0xB0000000u;
    c->r[6] = c->r[14] & 0xF;
    *W(c->r[13]) = c->r[15] | 0xB0000000u;
    c->r[7] = c->r[6] << 16;
    c->r[8] = c->r[13] + 4;
    c->r[12] = 0xB1000000u;
    c->r[2] = 0xC4000000u;
    c->r[10] = c->r[7] | 0xB1000000u;
    c->r[9] = a | 0xC4000000u;
    c->r[3] = c->r[8] + 8;
    *W(c->r[11] + 8) = c->r[3];
    *W(c->r[8]) = c->r[10];
    *W(c->r[8] + 4) = c->r[9];
}

/* The drawing hardware is told how far the list has got (the list's +8, for the handle at the
   drawing state's +24), unless the state's +12 or the list's +116 is set. Returns 0, or the
   error. A 0x10 frame. */
SFA_FUNC(089EA70C, list_stall_update) {
    c->r[29] -= 0x10;
    *W(c->r[29]) = c->r[31];
    ARG(0) = GE;
    c->r[2] = *W(GE + 12);
    c->r[3] = 0;
    if (c->r[2] == 0) {
        gaddr list = *W(GE + 72);
        c->r[3] = list;
        ARG(1) = *W(list + 116);
        if (ARG(1) == 0) {
            ARG(1) = *W(list + 8);
            ARG(0) = *W(GE + 24);
            c->r[31] = 0x089EA754u; hle_sceGeListUpdateStallAddr(c);
            c->r[3] = c->r[2];
            if ((int32_t)c->r[2] >= 0) c->r[3] = 0;
        } else c->r[3] = 0;
    }
    c->r[31] = *W(c->r[29]);
    f_089EA740(c);
}

/* (i, left, right, samples): the i-th of the four sound channels plays the samples at the two
   volumes. 0x80440010 past the channels, 0x8044000A for a volume above 0x8000. A 0x10 frame. */
SFA_FUNC(089EB528, channel_output) {
    uint32_t i = ARG(0), left = ARG(1), right = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    uint32_t answer = 0x80440010u;
    c->r[10] = 0x8000; c->r[11] = i < 4; c->r[9] = 0x8000u < left;
    if (i < 4) {
        c->r[11] = 0x80440000u;
        c->r[10] = 0x8000u < right;
        answer = 0x8044000Au;
        if (!(0x8000u < left)) {
            c->r[12] = 0x08E70000u;
            c->r[9] = 0x08E72FB8u;
            c->r[3] = (i << 2) + 0x08E72FB8u;
            ARG(0) = 0x80440000u;
            if (!(0x8000u < right)) {
                ARG(0) = *W(c->r[3]);
                c->r[31] = 0x089EB57Cu; hle_sceAudioOutputPannedBlocking(c);
                answer = c->r[2];
            }
        }
    }
    c->r[8] = answer;
    c->r[2] = answer;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (buffer): the sound core fills the buffer; 0x80420005 without one or for one that is not on a
   boundary of 64 bytes. A 0x10 frame. */
SFA_FUNC(089EC8E4, sound_core_run) {
    uint32_t buf = ARG(0);
    uint32_t ra = c->r[31];
    ARG(1) = *W(SAS_UP);
    c->r[29] -= 0x10;
    c->r[3] = 0x80420000u;
    *W(c->r[29]) = ra;
    c->r[2] = SAS_DOWN;
    if (ARG(1) != 0) {
        c->r[3] = buf & 0x3F;
        c->r[2] = 0x80420005u;
        ARG(1) = 0x80420000u;
        if (buf != 0) {
            c->r[7] = 0x80420000u;
            ARG(1) = buf; ARG(0) = SAS;
            if ((buf & 0x3F) == 0) { c->r[31] = 0x089EC930u; hle___sceSasCore(c); }
        }
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (voice, data, size, loop): the voice's samples. A 0x10 frame. */
SFA_FUNC(089ECCAC, voice_data) {
    uint32_t voice = ARG(0), data = ARG(1), size = ARG(2), loop = ARG(3);
    uint32_t ra = c->r[31];
    c->r[12] = *W(SAS_UP);
    c->r[3] = size;
    c->r[29] -= 0x10;
    c->r[10] = 0x80420000u; c->r[9] = 0x08E70000u;
    *W(c->r[29]) = ra;
    c->r[8] = loop;
    ARG(0) = SAS; ARG(1) = voice; ARG(2) = data; ARG(3) = size;
    c->r[11] = SAS_DOWN;
    if (c->r[12] != 0) {
        c->r[31] = 0x089ECCF4u; hle___sceSasSetVoice(c);
        c->r[11] = c->r[2];
    }
    c->r[2] = c->r[11];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (voice, a, b): the voice's envelope in its short form. A 0x10 frame. */
SFA_FUNC(089ECE78, voice_simple_adsr) {
    uint32_t voice = ARG(0), a = ARG(1), b = ARG(2);
    uint32_t ra = c->r[31];
    c->r[10] = *W(SAS_UP);
    c->r[3] = voice;
    c->r[29] -= 0x10;
    c->r[8] = 0x80420000u;
    *W(c->r[29]) = ra;
    ARG(0) = SAS; ARG(1) = voice; ARG(2) = a; ARG(3) = b;
    c->r[9] = SAS_DOWN;
    if (c->r[10] != 0) {
        c->r[31] = 0x089ECEB8u; hle___sceSasSetSimpleADSR(c);
        c->r[9] = c->r[2];
    }
    c->r[2] = c->r[9];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The end of a routine that fills a record (s1) from a pair of words (s3) and what it kept in
   s2, s6, s7 and r8, and returns 0 from a 0x30 frame that kept s0..s7. */
SFA_FUNC(089EEA60, tail_record_fill) {
    gaddr rec = c->r[17], pair = c->r[19];
    gaddr sp = c->r[29];
    *W(rec + 32) = c->r[8];
    c->r[12] = 0;
    c->r[25] = *W(pair);
    *W(rec + 80) = c->r[25];
    c->r[24] = *W(pair + 4);
    *W(rec + 8) = c->r[22];
    *W(rec + 84) = c->r[24];
    *W(rec) = c->r[23];
    *W(rec + 4) = c->r[18];
    c->r[31] = *W(sp + 32);
    for (int i = 7; i >= 0; i--) c->r[16 + i] = *W(sp + 4 * (uint32_t)i);
    c->r[2] = 0;
    c->r[29] += 0x30;
}

/* The end of a routine that scales a volume: the two table entries (sixteen bits each, at
   0x08AD80D4 by r6 and 0x08AD80EC by r7) are multiplied and the high half kept; with r8 negative
   it is rounded (1 << r10) and shifted down by r11. The answer is cut to sixteen bits. */
SFA_FUNC(089EECB8, tail_volume_scale) {
    c->r[3] = 0x08AE0000u; c->r[14] = 0x08AE0000u;
    c->r[15] = c->r[6] << 1;
    c->r[9] = c->r[7] << 1;
    c->r[24] = 1;
    ARG(0) = 0x08AD80D4u; ARG(1) = 0x08AD80ECu;
    c->r[2] = c->r[9] + 0x08AD80ECu;
    c->r[6] = 1u << (c->r[10] & 31);
    c->r[10] = c->r[15] + 0x08AD80D4u;
    c->r[25] = *H(c->r[2]);
    c->r[12] = *H(c->r[10]);
    int64_t m = (int64_t)(int32_t)c->r[12] * (int32_t)c->r[25];
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    c->r[7] = c->lo;
    c->r[2] = (uint32_t)((int32_t)c->r[7] >> 16);
    if ((int32_t)c->r[8] < 0) {
        c->r[8] = c->r[2] + c->r[6];
        c->r[2] = c->r[8] >> (c->r[11] & 31);
    }
    f_089EECF8(c);
}

/* (track): the track's next number, in the sequence's variable-length form (seven bits a byte,
   the top bit saying another follows), read from its place (+28) into +60. Returns 0. */
SFA_FUNC(089F0504, track_number_read) {
    gaddr t = ARG(0);
    c->r[6] = t;
    gaddr at = *W(t + 28);
    uint32_t v = *B(at);
    ARG(0) = at;
    c->r[3] = at + 1;
    *W(t + 28) = at + 1;
    if (v & 0x80) {
        v &= 0x7F;
        uint32_t b;
        do {
            gaddr q = *W(t + 28);
            c->r[11] = q;
            c->r[7] = v << 7;
            b = *B(q);
            c->r[9] = b;
            c->r[10] = q + 1;
            *W(t + 28) = q + 1;
            c->r[8] = b & 0x7F;
            v = (v << 7) + (b & 0x7F);
        } while ((int8_t)b < 0);
    }
    ARG(1) = v;
    c->r[2] = 0;
    *W(t + 60) = v;
}

/* The end of a routine that takes an entry out of one list of words and adds it to another: the
   words past the place r3 of the first (r5, r11 of them) move down one, its count (r4) drops,
   and r12 goes to the end of the second (r7, its count at r6). Returns r12. */
SFA_FUNC(089F295C, tail_list_move) {
    c->r[9] = c->r[11] - 1;
    c->r[8] = c->r[3];
    if (c->r[3] != c->r[9]) {
        c->r[3] = c->r[3] < c->r[9];
        c->r[13] = c->r[8] << 2;
        if (c->r[3] != 0) {
            c->r[5] = c->r[13] + c->r[5];
            do {
                c->r[15] = *W(c->r[5] + 4);
                c->r[8] = c->r[8] + 1;
                c->r[14] = c->r[8] < c->r[9];
                *W(c->r[5]) = c->r[15];
                c->r[5] += 4;
            } while (c->r[14] != 0);
        }
    }
    c->r[24] = c->r[11] - 1;
    *W(c->r[4]) = c->r[24];
    c->r[2] = c->r[12];
    c->r[8] = *W(c->r[6]);
    c->r[9] = c->r[8] << 2;
    c->r[5] = c->r[9] + c->r[7];
    c->r[7] = c->r[8] + 1;
    *W(c->r[6]) = c->r[7];
    *W(c->r[5]) = c->r[12];
}

/* One word scaled by r5 over 127, the way the original divides (a multiplication by 0x02040811
   and two shifts). */
static uint32_t over_127(Cpu *c, uint32_t v, uint32_t *product, uint32_t *high) {
    int64_t m = (int64_t)(int32_t)v * (int32_t)c->r[5];
    *product = (uint32_t)m;
    uint64_t u = (uint64_t)*product * 0x02040811u;
    c->lo = (uint32_t)u; c->hi = (uint32_t)(u >> 32);
    *high = c->hi;
    return (*high + ((*product - *high) >> 1)) >> 6;
}

/* The end of a routine that scales two volumes: the words at the places r8 and r9 are each
   multiplied by r5 and divided by 127. */
SFA_FUNC(089F2E78, tail_two_over_127) {
    gaddr first = c->r[8], second = c->r[9];
    uint32_t product, high;
    c->r[13] = *W(first);
    c->r[2] = 0x02040000u;
    c->r[24] = 0x02040811u;
    c->r[7] = over_127(c, c->r[13], &product, &high);
    c->r[12] = product; c->r[6] = high;
    c->r[11] = product - high;
    c->r[3] = c->r[11] >> 1;
    c->r[10] = high + c->r[3];
    *W(first) = c->r[7];
    c->r[25] = *W(second);
    uint32_t scaled = over_127(c, c->r[25], &product, &high);
    c->r[15] = product; c->r[13] = high;
    c->r[14] = product - high;
    ARG(0) = c->r[14] >> 1;
    c->r[8] = high + ARG(0);
    ARG(1) = scaled;
    *W(second) = scaled;
}
