/* Seven small routines the fighters' frame leans on, none of which a test scenario runs: a
   block of numbers written as text and its lines drawn, and five checks of a fighter's settings.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0880A6CC(Cpu *c); void f_0886CA2C(Cpu *c);

/* (list, n): each of the n lines (12 bytes: +0 the text, 0 for none, +4 and +6 its place, +8
   and +10 two bytes more) is drawn. A 0x30 frame keeping s0..s2. */
SFA_FUNC(08848654, text_lines_draw) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 40) = s2;
    c->r[18] = 0;
    *W(sp + 36) = s1;
    c->r[6] = 0 < (int32_t)ARG(1);
    c->r[17] = ARG(0);
    *W(sp + 32) = s0; *W(sp + 44) = ra;
    c->r[16] = ARG(1);
    while ((int32_t)c->r[18] < (int32_t)c->r[16]) {
        gaddr line = c->r[17];
        ARG(0) = *W(line);
        if (ARG(0) != 0) {
            ARG(1) = *H(line + 8) & 0xFF;
            ARG(2) = (uint32_t)S16(line + 4);
            ARG(3) = (uint32_t)S16(line + 6);
            c->r[8] = *H(line + 10) & 0xFF;
            c->r[31] = 0x088486A4u; f_0880A6CC(c);
        }
        c->r[18]++;
        c->r[17] += 0xC;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}

/* A digit as the letter the text uses: '0' to '9', then 'A' on. */
static uint32_t digit_char(uint32_t d) {
    d = (d + 0x30) & 0xFF;
    if ((int32_t)d >= 58) d = (d + 7) & 0xFF;
    return d;
}

/* (to, value): the byte as three letters at `to`: hundreds (a space for none), tens, ones. */
SFA_FUNC(08848728, byte_to_text) {
    gaddr to = ARG(0);
    uint32_t v = ARG(1) & 0xFF;
    c->r[9] = v;
    c->r[8] = 100;
    DIV(c, v, 100);
    uint32_t d = digit_char(c->lo);
    c->r[10] = 0x30;
    ARG(1) = 10;
    *B(to) = d == 0x30 ? 0x20 : (uint8_t)d;
    c->r[6] = to + 2;
    DIV(c, v, 100);
    uint32_t rest = c->hi & 0xFF;
    c->r[9] = rest;
    DIV(c, rest, 10);
    d = digit_char(c->lo);
    c->r[8] = (int32_t)((c->lo + 0x30) & 0xFF) < 58;
    DIV(c, rest, 10);
    *B(to + 1) = (uint8_t)d;
    uint32_t ones = c->hi & 0xFF;
    d = digit_char(ones);
    ARG(1) = (int32_t)((ones + 0x30) & 0xFF) < 58;
    c->r[7] = d;
    ARG(0) = to + 2;
    *B(to + 2) = (uint8_t)d;
}

/* (a, b, d): when a's +20 has bit 0, seven of the fighters' numbers are written into the six
   lines of text listed at 0x08A13754 (each 13 letters in, one more 9 letters into the last) and
   the lines drawn. A 0x30 frame keeping s0..s4. */
SFA_FUNC(088487E8, numbers_show) {
    gaddr a = ARG(0), b = ARG(1), d = ARG(2);
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    c->r[19] = *W(d + 92);
    c->r[7] = *B(a + 20) & 1;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[18] = a; c->r[17] = b;
    *W(sp + 16) = sv[0]; *W(sp + 32) = sv[4]; *W(sp + 36) = ra;
    c->r[16] = d;
    if (c->r[7] != 0) {
        gaddr lines = 0x08A13754u;
        c->r[20] = 0x08A10000u;
        ARG(0) = *W(lines) + 0xD; ARG(1) = (uint32_t)S16(a + 32) & 0xFF;
        c->r[31] = 0x08848838u; f_08848728(c);
        c->r[20] = lines;
        ARG(0) = *W(lines + 12) + 0xD; ARG(1) = (uint32_t)S16(a + 34) & 0xFF;
        c->r[31] = 0x08848850u; f_08848728(c);
        ARG(0) = *W(lines + 24) + 0xD; ARG(1) = (uint32_t)S16(b + 780) & 0xFF;
        c->r[31] = 0x08848864u; f_08848728(c);
        ARG(0) = *W(lines + 36) + 0xD; ARG(1) = (uint32_t)S16(a + 30) & 0xFF;
        c->r[31] = 0x08848878u; f_08848728(c);
        ARG(0) = *W(lines + 48) + 0xD; ARG(1) = (uint32_t)S8(d + 357) & 0xFF;
        c->r[31] = 0x0884888Cu; f_08848728(c);
        ARG(0) = *W(lines + 60) + 9; ARG(1) = *B(c->r[19] + 700);
        c->r[31] = 0x0884889Cu; f_08848728(c);
        ARG(0) = *W(lines + 60) + 0xD; ARG(1) = (uint32_t)S16(a + 36) & 0xFF;
        c->r[31] = 0x088488B0u; f_08848728(c);
        ARG(0) = lines; ARG(1) = 6;
        c->r[31] = 0x088488BCu; f_08848654(c);
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (a, f, -, n): by the byte at a's +4 for the side n: 2 sets bit 2 of the fighter's +362, 3
   sets bit 3. */
SFA_FUNC(08849DB4, setting_to_flags) {
    gaddr f = ARG(1);
    c->r[6] = ARG(3) & 0xFF;
    ARG(0) = *B(ARG(0) + c->r[6] + 4);
    c->r[6] = 2;
    if (ARG(0) == 2) {
        ARG(0) = *H(f + 362) | 4u;
        *H(f + 362) = (uint16_t)ARG(0);
        return;
    }
    c->r[6] = 3;
    if (ARG(0) == 3) {
        ARG(0) = *H(f + 362) | 8u;
        *H(f + 362) = (uint16_t)ARG(0);
    }
}

/* (-, -, f): 1 when the fighter is character 0xE in state 1, step 8, with +7 set and +236 at
   6. */
SFA_FUNC(08849DF8, fighter_is_in_e_hold) {
    gaddr f = ARG(2);
    ARG(0) = (uint32_t)S8(f + 338);
    ARG(1) = 0xE;
    if (ARG(0) == 0xE) {
        ARG(0) = *B(f + 4);
        c->r[2] = 1;
        if (ARG(0) == 1) {
            ARG(0) = *B(f + 5);
            ARG(1) = 8;
            if (ARG(0) == 8) {
                ARG(0) = *B(f + 7);
                if ((int32_t)ARG(0) > 0) {
                    ARG(0) = (uint32_t)S8(f + 236);
                    ARG(1) = 6;
                    if (ARG(0) == 6) return;
                }
            }
        }
    }
    c->r[2] = 0;
}

/* (a, f): when a's +10 is set and the fighter is at step 3 with +82 above 0, the fighter's bit
   (by its side, +337) is set in a's +11. */
SFA_FUNC(08849E54, side_bit_set) {
    gaddr a = ARG(0), f = ARG(1);
    c->r[6] = *B(a + 10);
    if (c->r[6] == 0) return;
    c->r[6] = *B(f + 5);
    c->r[7] = 3;
    if (c->r[6] != 3) return;
    c->r[6] = (uint32_t)S16(f + 82);
    if ((int32_t)c->r[6] <= 0) return;
    ARG(1) = (uint32_t)S8(f + 337);
    c->r[6] = *B(a + 11);
    c->r[7] = 1;
    ARG(1) = c->r[6] | (1u << (ARG(1) & 31));
    *B(a + 11) = (uint8_t)ARG(1);
}

/* (a, f): the fighter's gauge (+366) held at the level a's setting asks for (its +12, or +13
   when the fighter's side is not among the game's +53; through the table for the fighter's
   style at 0x08A1385C; below 0 for no limit): a gauge at or above it is cut to it, and one
   below it is filled by the routine at 0x0886CA2C unless the battle's +14 is set. A 0x20
   frame. */
SFA_FUNC(08849E9C, gauge_hold_level) {
    gaddr a = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = 1u << ((uint32_t)S8(f + 337) & 31);
    c->r[9] = (uint32_t)S8(f + 386) << 2;
    c->r[8] = (uint32_t)S8(GAME + 53) & c->r[6];
    gaddr table = *W(0x08A13858u + c->r[9] + 4);
    c->r[6] = table;
    c->r[7] = c->r[8] != 0 ? 0 : 1;
    *W(c->r[29] + 16) = ra;
    ARG(0) = (uint32_t)*B(a + c->r[7] + 12) * 2;
    int32_t level = S16(table + ARG(0));
    c->r[7] = (uint32_t)level;
    if (level >= 0) {
        ARG(0) = S16(f + 366) < level;
        if (ARG(0) == 0) {
            ARG(0) = 0x092E0000u;
            *H(f + 366) = (uint16_t)level;
        } else {
            ARG(0) = (uint32_t)S8(BATTLE + 14);
            if (ARG(0) == 0) {
                ARG(0) = f; ARG(1) = 1;
                c->r[31] = 0x08849F28u; f_0886CA2C(c);
            }
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
