/* More of the library's own arithmetic: a double taken apart, a whole number made a double, and a
   double made a whole number. The parts of a number are described in batch_float383.c. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089FBFA0(Cpu *c); void f_089FBCC4(Cpu *c); void f_089FB958(Cpu *c); void f_089FBA78(Cpu *c);

/* The end of a routine that stores a double's fraction: r8 and r9 to +16 and +20 of the place in
   r10. */
SFA_FUNC(089FC000, tail_fraction_at_r10) {
    *W(c->r[10] + 16) = c->r[8];
    *W(c->r[10] + 20) = c->r[9];
}

/* (bits, out): the double the first points at, taken apart to the second. */
SFA_FUNC(089FBF64, double_unpack) {
    gaddr out = ARG(1);
    uint32_t hi = *W(ARG(0) + 4);
    c->r[2] = hi;
    c->r[3] = 0x000FFFFFu;
    c->r[10] = out;
    uint32_t exp = (hi >> 20) & 0x7FF;
    c->r[7] = exp;
    ARG(1) = hi >> 31;
    uint32_t lo = *W(ARG(0));
    c->r[8] = lo;
    uint32_t frac = hi & 0x000FFFFFu;
    c->r[9] = frac;
    *W(out + 4) = ARG(1);
    if (exp == 0) {
        c->r[2] = lo | frac;
        if (c->r[2] == 0) { c->r[2] = 2; f_089FBFA0(c); return; }
        c->r[2] = lo >> 24;
        frac = (frac << 8) | (lo >> 24);
        int32_t e = -1022;
        c->r[2] = (uint32_t)e;
        c->r[3] = 3;
        ARG(0) = 0x0FFFFFFFu < frac;
        lo <<= 8;
        *W(out + 8) = (uint32_t)e;
        *W(out) = 3;
        if (ARG(0) == 0) {
            ARG(1) = 0x0FFFFFFFu;
            do {
                c->r[3] = lo >> 31;
                frac = (frac << 1) | (lo >> 31);
                c->r[2] = 0x0FFFFFFFu < frac;
                lo <<= 1;
                e--;
            } while (c->r[2] == 0);
            ARG(0) = (uint32_t)e;
            *W(out + 8) = (uint32_t)e;
        }
        c->r[8] = lo; c->r[9] = frac;
        f_089FC000(c);
        return;
    }
    c->r[2] = 0x7FF;
    c->r[6] = lo >> 24;
    if (exp == 0x7FF) {
        c->r[2] = lo | frac;
        if (c->r[2] == 0) { c->r[2] = 4; f_089FBFA0(c); return; }
        c->r[3] = frac & 0x00080000u;
        c->r[2] = c->r[3];
        if (c->r[2] == 0) *W(out) = 0;
        else { c->r[2] = 1; *W(out) = 1; }
        f_089FC000(c);
        return;
    }
    c->r[3] = (frac << 8) | (lo >> 24) | 0x10000000u;
    ARG(1) = 0x10000000u; ARG(0) = 0;
    c->r[2] = lo << 8;
    c->r[6] = exp - 1023;
    *W(out + 16) = c->r[2];
    *W(out + 20) = c->r[3];
    c->r[2] = 3;
    *W(out + 8) = c->r[6];
    f_089FBFA0(c);
}

/* The end of a routine that builds a double from the parts at the bottom of its 0x30 frame. */
SFA_FUNC(089FB94C, tail_pack_frame) {
    ARG(0) = c->r[29];
    c->r[31] = 0x089FB954u; f_089FBCC4(c);
    c->r[31] = *W(c->r[29] + 32);
    f_089FB958(c);
}

/* A part of the routine that makes a whole number a double: the size in r2 and r3 becomes the
   fraction, moved up (and the exponent at +8 down) until its leading one is at bit 28 of the
   high word. */
SFA_FUNC(089FB980, tail_normalize_frame) {
    gaddr sp = c->r[29];
    *W(sp + 16) = c->r[2];
    *W(sp + 20) = c->r[3];
    c->r[3] = *W(sp + 20);
    c->r[2] = 0x0FFFFFFFu < c->r[3];
    ARG(1) = *W(sp + 8);
    if (c->r[2] == 0) {
        c->r[6] = 0x0FFFFFFFu;
        do {
            c->r[2] = *W(sp + 16);
            c->r[3] = *W(sp + 20);
            ARG(0) = c->r[2] >> 31;
            c->r[3] = (c->r[3] << 1) | ARG(0);
            c->r[2] = c->r[2] << 1;
            *W(sp + 16) = c->r[2];
            *W(sp + 20) = c->r[3];
            ARG(0) = 0x0FFFFFFFu < *W(sp + 20);
            ARG(1) = ARG(1) - 1;
        } while (ARG(0) == 0);
        *W(sp + 8) = ARG(1);
    }
    f_089FB94C(c);
}

/* (n): the whole number as a double, in r2 (low) and r3 (high). The parts are built at the bottom
   of a 0x30 frame the ends close; the lowest number of all is answered from the constant at
   0x08A02DD8. */
SFA_FUNC(089FB928, int_to_double) {
    uint32_t n = ARG(0);
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    c->r[3] = n >> 31;
    c->r[2] = 3;
    *W(sp + 32) = c->r[31];
    *W(sp) = 3;
    *W(sp + 4) = c->r[3];
    if (n == 0) {
        c->r[2] = 2;
        *W(sp) = 2;
        f_089FB94C(c);
        return;
    }
    c->r[2] = 0x3C;
    *W(sp + 8) = 0x3C;
    if (c->r[3] == 0) {
        c->r[2] = n;
        c->r[3] = (uint32_t)((int32_t)n >> 31);
    } else {
        c->r[2] = 0x80000000u;
        if (n == 0x80000000u) {
            c->r[2] = *W(0x08A02DD8u);
            c->r[3] = *W(0x08A02DDCu);
            c->r[31] = *W(sp + 32);
            f_089FB958(c);
            return;
        }
        c->r[2] = 0 - n;
        c->r[3] = (uint32_t)((int32_t)c->r[2] >> 31);
    }
    f_089FB980(c);
}

/* (low, high): the double as a whole number, towards zero: 0 for zero and what is not a number,
   the nearest end of the range for infinity and anything of 2 to the 31 or more. A 0x40 frame
   with the double at +32 and its parts at +0; the end returns r5. */
SFA_FUNC(089FBA04, double_to_int) {
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    c->r[2] = ARG(0); c->r[3] = ARG(1);
    *W(sp + 32) = ARG(0);
    *W(sp + 36) = ARG(1);
    *W(sp + 48) = c->r[31];
    ARG(0) = sp + 0x20; ARG(1) = sp;
    c->r[31] = 0x089FBA28u; f_089FBF64(c);
    ARG(1) = 0;
    uint32_t kind = *W(sp);
    c->r[3] = kind;
    c->r[2] = kind ^ 2;
    ARG(0) = kind < 2;
    if (kind == 2) { f_089FBA78(c); return; }
    c->r[2] = kind ^ 4;
    if (kind < 2) { f_089FBA78(c); return; }
    c->r[3] = 0x7FFF0000u;
    ARG(1) = 0x80000000u;
    if (kind != 4) {
        int32_t exp = (int32_t)*W(sp + 8);
        c->r[2] = (uint32_t)exp;
        ARG(1) = 0;
        ARG(0) = exp < 31;
        if (exp < 0) { f_089FBA78(c); return; }
        c->r[3] = 0x7FFF0000u;
        ARG(1) = 0x80000000u;
        if (exp < 31) {
            uint32_t n = 0x3Cu - (uint32_t)exp;
            ARG(0) = n;
            c->r[2] = *W(sp + 16);
            c->r[3] = *W(sp + 20);
            c->r[8] = n << 26;
            if ((int32_t)c->r[8] < 0) {
                c->r[6] = c->r[3] >> (n & 31);
                c->r[7] = 0;
            } else {
                c->r[6] = c->r[2] >> (n & 31);
                if (c->r[8] != 0) {
                    c->r[8] = c->r[3] << ((0 - n) & 31);
                    c->r[6] |= c->r[8];
                }
                c->r[7] = c->r[3] >> (n & 31);
            }
            c->r[2] = *W(sp + 4);
            ARG(1) = 0 - c->r[6];
            if (c->r[2] == 0) ARG(1) = c->r[6];
            f_089FBA78(c);
            return;
        }
    }
    c->r[2] = *W(sp + 4);
    c->r[3] |= 0xFFFF;
    if (c->r[2] == 0) ARG(1) = c->r[3];
    f_089FBA78(c);
}

/* The end of a routine that builds a double from the parts r4 points at, from a 0x70 frame that
   kept s0. */
SFA_FUNC(089FB740, tail_pack_frame_70) {
    c->r[31] = 0x089FB748u; f_089FBCC4(c);
    c->r[31] = *W(c->r[29] + 100);
    c->r[16] = *W(c->r[29] + 96);
    c->r[29] += 0x70;
}
