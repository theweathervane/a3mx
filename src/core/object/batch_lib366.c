/* Five more of the library: the frame's list of things to draw run layer by layer, a primitive on
   the drawing list, a sequence started and its state reset, and a note's pitch split into its
   octave and step. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E43FC(Cpu *c); void f_089EECB8(Cpu *c);

/* The frame's list (8 bytes an entry, 0x8000 a frame from 0x08E55C40: a record, a kind and a
   layer) is run once for each of the 24 layers in turn: every entry of the layer has its record
   handed to the routine its kind names in the table 0x08AD7EC4. The count is the word at
   0x08E65C42. A 0x30 frame keeping s0..s6. */
SFA_FUNC(089E4408, frame_list_run) {
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 7; i++) *W(sp + 16 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 44) = sv[7];
    c->r[31] = 0x089E4434u; f_089E43FC(c);
    c->r[16] = 0x08AD7EC4u; c->r[17] = 0x08E60000u; c->r[20] = 0x08E55C40u; c->r[21] = 0x08E60000u;
    int32_t count = *H(0x08E65C42u);
    for (uint32_t layer = 0; layer < 24; layer++) {
        c->r[22] = layer;
        gaddr e = ((uint32_t)*B(0x08E65C64u) << 15) + 0x08E55C40u;
        for (uint32_t i = 0; (int32_t)i < count; i = (i + 1) & 0xFFFF, e += 8) {
            c->r[18] = i; c->r[19] = e;
            if (*H(e + 6) == layer) {
                uint32_t fn = *W(0x08AD7EC4u + ((uint32_t)*H(e + 4) << 2));
                ARG(2) = fn;
                ARG(0) = *W(e); c->r[31] = 0x089E4498u;
                rt_call_indirect(c, fn);
            }
            count = *H(0x08E65C42u);
        }
    }
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x30;
}

/* (list, kind, type, count, indices, vertices): a primitive on the list given: its vertex type
   (0x12) when there is one, the two addresses when there are any (0x10 with each one's top bits,
   0x02 for the indices and 0x01 for the vertices), and the command 0x04 with the kind and the
   count. */
SFA_FUNC(089E8EE8, list_primitive) {
    gaddr list = ARG(0);
    uint32_t kind = ARG(1), type = ARG(2), count = ARG(3), indices = c->r[8], vertices = c->r[9];
    c->r[14] = (type & 0x00FFFFFFu) | 0x12000000u;
    c->r[12] = (((indices >> 24) & 0xF) << 16) | 0x10000000u;
    c->r[10] = (indices & 0x00FFFFFFu) | 0x02000000u;
    c->r[13] = (((vertices >> 24) & 0xF) << 16) | 0x10000000u;
    c->r[11] = (vertices & 0x00FFFFFFu) | 0x01000000u;
    uint32_t draw = ((kind & 7) << 16) | count | 0x04000000u;
    if (type != 0) {
        gaddr p = *W(list + 8);
        *W(p) = c->r[14];
        *W(list + 8) = p + 4;
    }
    if (indices != 0) {
        gaddr p = *W(list + 8);
        *W(p) = c->r[12];
        *W(list + 8) = p + 8;
        *W(p + 4) = c->r[10];
    }
    if (vertices != 0) {
        gaddr p = *W(list + 8);
        *W(p) = c->r[13];
        *W(list + 8) = p + 8;
        *W(p + 4) = c->r[11];
    }
    c->r[12] = *W(list + 8);
    *W(c->r[12]) = draw;
    ARG(1) = c->r[12] + 4;
    *W(list + 8) = ARG(1);
}

/* (pitch, a, key, b): the end of a routine that turns a pitch into what the volume table wants:
   the two fine values (a and b, 128 to a step) are added and carried into the key, the key less
   the pitch's base is split into an octave (twelve steps) and a step, and the three go on to the
   scaling in r6, r7 and r8 with the rounding in r10 and r11. */
SFA_FUNC(089EEC08, tail_pitch_split) {
    c->r[24] = (uint32_t)(int32_t)(int16_t)ARG(1);
    c->r[15] = (uint32_t)(int32_t)(int16_t)ARG(3);
    c->r[13] = c->r[15] + c->r[24];
    c->r[14] = (uint32_t)((int32_t)c->r[13] >> 31);
    c->r[9] = c->r[14] >> 25;
    c->r[11] = c->r[13] + c->r[9];
    c->r[9] = (uint32_t)((int32_t)c->r[11] >> 7);
    c->r[10] = ARG(2) & 0xFFFF;
    c->r[8] = ARG(0) & 0xFFFF;
    ARG(1) = c->r[9] + c->r[10];
    c->r[3] = ARG(1) - c->r[8];
    c->r[2] = 0x2AAA0000u;
    c->r[10] = (uint32_t)(int32_t)(int16_t)c->r[3];
    c->r[12] = 0x2AAAAAABu;
    int64_t m = (int64_t)(int32_t)c->r[10] * (int32_t)c->r[12];
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    c->r[25] = (uint32_t)((int32_t)c->r[10] >> 31);
    ARG(0) = c->hi;
    c->r[24] = (uint32_t)((int32_t)ARG(0) >> 1);
    ARG(1) = c->r[24] - c->r[25];
    c->r[15] = ARG(1) << 1;
    c->r[14] = c->r[15] + ARG(1);
    c->r[11] = c->r[14] << 2;
    c->r[8] = c->r[10] - c->r[11];
    c->r[11] = c->r[9] << 7;
    ARG(2) = (uint32_t)(int32_t)(int16_t)c->r[8];
    c->r[2] = ARG(1) + 0xFFFFFFFEu;
    ARG(3) = c->r[13] - c->r[11];
    ARG(0) = ARG(2) < 1;
    c->r[8] = (uint32_t)(int32_t)(int16_t)c->r[2];
    c->r[12] = ARG(3) >> 31;
    ARG(1) = ARG(0) & c->r[12];
    c->r[10] = ARG(2) + 0xC;
    c->r[3] = c->r[8] + 0xFFFFFFFFu;
    ARG(0) = ARG(3) + c->r[11];
    int lower = (int32_t)ARG(2) < 0;
    if (!lower) {
        c->r[11] = 0 - c->r[8];
        if (ARG(1) != 0) lower = 1;
    }
    if (lower) {
        ARG(2) = (uint32_t)(int32_t)(int16_t)c->r[10];
        c->r[8] = (uint32_t)(int32_t)(int16_t)c->r[3];
        c->r[11] = 0 - c->r[8];
    }
    c->r[13] = ARG(2) + c->r[9];
    c->r[10] = c->r[11] + 0xFFFFFFFFu;
    c->r[2] = c->r[13] + 0xFFFFFFFFu;
    if ((int32_t)ARG(3) < 0) {
        ARG(2) = (uint32_t)(int32_t)(int16_t)c->r[2];
        ARG(3) = ARG(0) + 0x80;
    }
    f_089EECB8(c);
}

/* (i, a, b, d, e, n): the i-th of the sequences listed at 0x08E74308 is started: its four bytes
   at +40 to +43 (each below 0x80), n repeats at +52 and +56, and 1 in its first byte. Returns 0;
   0x80450001 before the list exists, 0x80540007 without the sequence, 0x80540009 while it runs,
   0x80540008 for no repeats, and 0x8054000A for a byte out of range. */
SFA_FUNC(089EF4E4, sequence_start) {
    uint32_t i = ARG(0), a = ARG(1), b = ARG(2), d = ARG(3), e = c->r[8], n = c->r[9];
    uint32_t answer;
    if (*W(0x08E74304u) == 0) answer = 0x80450001u;
    else {
        gaddr slot = 0x08E74308u + (i << 2);
        ARG(0) = slot;
        gaddr p = i < 0x81 ? *W(slot) : 0;
        if (p == 0) answer = 0x80540007u;
        else if (*B(p) != 0) answer = 0x80540009u;
        else if (n == 0) answer = 0x80540008u;
        else if (!(a < 0x80) || !(b < 0x80) || !(d < 0x80) || !(e < 0x80)) answer = 0x8054000Au;
        else {
            *B(p + 40) = (uint8_t)a;
            *B(*W(slot) + 41) = (uint8_t)b;
            *B(*W(slot) + 42) = (uint8_t)d;
            *B(*W(slot) + 43) = (uint8_t)e;
            gaddr q = *W(slot);
            ARG(1) = q;
            *B(q) = 1;
            *W(q + 56) = n;
            *W(q + 52) = n;
            answer = 0;
        }
    }
    c->r[10] = answer;
    c->r[2] = answer;
}

/* (sequence, k): a sequence's state back to its start: stopped (4), a tempo of 1000000, its
   volumes at 0x7F and 0x40, and each of its sixteen channels (16 bytes each from +64) with the
   volume 0x7F, the pan 0x40, the bend 0x2000 and the word 0xFF000000 with k and its number.
   Returns 0. */
SFA_FUNC(089F0104, sequence_reset) {
    gaddr p = ARG(0);
    uint32_t k = ARG(1) << 8;
    *B(p + 12) = 1;
    *B(p + 42) = 0x7F;
    *B(p + 43) = 0x40;
    *B(p + 37) = 0xFF;
    *W(p + 44) = 0xFFFFFFFFu;
    *W(p + 4) = 0; *W(p + 28) = 0; *W(p + 20) = 0;
    *B(p + 9) = 0xFF;
    *W(p + 60) = 0; *W(p + 48) = 0;
    *B(p + 8) = 0xFF;
    *W(p + 52) = 0; *W(p + 56) = 0;
    *B(p + 40) = 0x7F;
    *B(p + 41) = 0x40;
    *W(p + 32) = 0;
    *B(p + 36) = 0;
    *B(p) = 4;
    *W(p + 16) = 0x000F4240u;
    ARG(1) = k;
    c->r[11] = 0x7F; c->r[14] = 0x40; c->r[13] = 0x2000; c->r[12] = 0xFF000000u;
    gaddr ch = p;
    for (uint32_t i = 0; i < 0x10; i++, ch += 0x10) {
        *W(ch + 72) = k | i | 0xFF000000u;
        *B(ch + 64) = 0;
        *B(ch + 65) = 0x7F;
        *B(ch + 66) = 0x7F;
        *B(ch + 67) = 0x40;
        *W(ch + 76) = 0x2000;
        *B(ch + 68) = 0;
    }
    c->r[9] = ch; c->r[10] = 0x10;
    c->r[2] = 0;
}
