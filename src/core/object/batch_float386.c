/* More of the arithmetic in the library: a float put together from its parts, and with it a
   double narrowed to a float. The parts of a number are described in batch_float383.c.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089FBF64(Cpu *c);

/* The end of a routine that builds a float: its fraction (r5), exponent (r7) and sign (r8) go
   together in r6 and are returned in f0. */
SFA_FUNC(089FC208, tail_float_pack) {
    c->r[3] = c->r[5] & 0x007FFFFFu;
    c->r[6] = (c->r[6] & 0xFF800000u) | c->r[3];
    ARG(1) = 0x807FFFFFu;
    ARG(0) = (c->r[7] & 0xFF) << 23;
    c->r[6] = (c->r[6] & 0x807FFFFFu) | ARG(0);
    c->r[2] = 0x7FFFFFFFu;
    c->r[3] = c->r[8] << 31;
    c->r[6] = (c->r[6] & 0x7FFFFFFFu) | c->r[3];
    c->f[0] = c->r[6];
}

/* The same after the fraction comes down seven bits. */
SFA_FUNC(089FC2C8, tail_float_pack_shift) {
    ARG(1) = ARG(1) >> 7;
    f_089FC208(c);
}

/* The same for a number too small for a whole exponent: when the rounded fraction has reached
   bit 30 the exponent (r7) becomes 1. r2 holds 0x3FFF0000. */
SFA_FUNC(089FC2B8, tail_float_small_exponent) {
    c->r[2] |= 0xFFFF;
    c->r[2] = c->r[2] < ARG(1);
    c->r[3] = 1;
    if (c->r[2] != 0) c->r[7] = 1;
    f_089FC2C8(c);
}

/* The same from the rounding: to the nearest at bit 7, a half going to the even side. */
SFA_FUNC(089FC2A0, tail_float_small_round) {
    c->r[3] = ARG(1) & 0x7F;
    if (c->r[3] == 0x40) {
        c->r[2] = ARG(1) & 0x80;
        if (c->r[2] != 0) ARG(1) += 0x40;
    } else {
        ARG(1) += 0x3F;
    }
    c->r[2] = 0x3FFF0000u;
    f_089FC2B8(c);
}

/* The end for a whole number after the rounding: a fraction that has reached bit 31 comes down
   one more and the exponent goes up. */
SFA_FUNC(089FC320, tail_float_carry) {
    if ((int32_t)ARG(1) >= 0) {
        ARG(1) = ARG(1) >> 7;
        f_089FC208(c);
        return;
    }
    ARG(1) = ARG(1) >> 1;
    c->r[7] = c->r[7] + 1;
    f_089FC2C8(c);
}

/* (parts): a float put together from its parts, returned in f0. */
SFA_FUNC(089FC1E4, float_pack) {
    gaddr parts = ARG(0);
    uint32_t kind = *W(parts);
    c->r[3] = kind;
    uint32_t frac = *W(parts + 12);
    ARG(1) = frac;
    c->r[8] = *W(parts + 4);
    c->r[2] = kind < 2;
    c->r[7] = 0;
    if (kind < 2) {
        c->r[2] = 0x00100000u;
        ARG(1) = frac | 0x00100000u;
        c->r[7] = 0xFF;
        f_089FC208(c);
        return;
    }
    c->r[2] = kind ^ 4;
    if (kind == 4) { c->r[7] = 0xFF; ARG(1) = 0; f_089FC208(c); return; }
    c->r[2] = kind ^ 2;
    if (kind == 2) { ARG(1) = 0; f_089FC208(c); return; }
    c->r[3] = 0x007F0000u;
    if (frac == 0) { f_089FC208(c); return; }
    int32_t exp = (int32_t)*W(parts + 8);
    ARG(0) = (uint32_t)exp;
    if (exp < -126) {
        uint32_t n = (uint32_t)(-126 - exp);
        ARG(0) = n;
        c->r[3] = (int32_t)n < 26;
        c->r[2] = 1;
        if ((int32_t)n < 26) {
            c->r[2] = frac & ((1u << (n & 31)) - 1);
            c->r[3] = frac >> (n & 31);
            c->r[2] = c->r[2] != 0;
            ARG(1) = c->r[3] | c->r[2];
        } else {
            ARG(1) = 0;
        }
        f_089FC2A0(c);
        return;
    }
    c->r[2] = exp < 128;
    if (exp >= 128) { c->r[7] = 0xFF; ARG(1) = 0; f_089FC208(c); return; }
    c->r[3] = frac & 0x7F;
    c->r[2] = 0x40;
    c->r[7] = (uint32_t)exp + 0x7F;
    if (c->r[3] == 0x40) {
        c->r[2] = frac & 0x80;
        if (c->r[2] != 0) ARG(1) = frac + 0x40;
    } else {
        ARG(1) = frac + 0x3F;
    }
    f_089FC320(c);
}

/* (kind, sign, exponent, fraction): a float from parts given in registers. A 0x20 frame holding
   them. */
SFA_FUNC(089FBC94, float_from_parts) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[2] = ARG(0);
    *W(sp + 16) = ra;
    *W(sp) = ARG(0); *W(sp + 4) = ARG(1); *W(sp + 8) = ARG(2); *W(sp + 12) = ARG(3);
    ARG(0) = sp;
    c->r[31] = 0x089FBCB8u; f_089FC1E4(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (low, high): the double as a float, in f0. The fraction comes down 30 bits, anything lost kept
   as its lowest bit. A 0x40 frame with the double at +32 and its parts at +0. */
SFA_FUNC(089FBB50, double_to_float) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    c->r[2] = ARG(0); c->r[3] = ARG(1);
    *W(sp + 48) = ra;
    *W(sp + 32) = ARG(0);
    *W(sp + 36) = ARG(1);
    ARG(0) = sp + 0x20; ARG(1) = sp;
    c->r[31] = 0x089FBB74u; f_089FBF64(c);
    uint32_t lo = *W(sp + 16), hi = *W(sp + 20);
    c->r[9] = hi;
    ARG(0) = *W(sp); ARG(1) = *W(sp + 4);
    c->r[3] = 0;
    c->r[2] = lo & 0x3FFFFFFFu;
    c->r[8] = (lo >> 30) | (hi << 2);
    c->r[2] = c->r[2] != 0;
    ARG(2) = *W(sp + 8);
    c->r[2] |= c->r[8];
    ARG(3) = c->r[2];
    c->r[31] = 0x089FBBBCu; f_089FBC94(c);
    c->r[31] = ra; c->r[29] += 0x40;
}
