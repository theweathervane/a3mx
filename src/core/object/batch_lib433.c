/* Library: a float raised to a float power (the C library's powf, in an early form), and the
   sound engine's one use of it. Offsets stay numbers until something shows what they are.

   The arithmetic is written as expressions; each keeps the original's order of operations, since
   floats give other answers in another order. The float registers the original leaves behind
   (all scratch) are not reproduced, only the answer in f0.
 */
#include <string.h>
#include "step.h"
#include "../guest_mem.h"

#pragma STDC FP_CONTRACT OFF

#define LIB_VERSION 0x08AD8E58u /* how errors are reported: -1 not at all, 0, 1, or 2 by errno only */
#define D_ONE       0x08AD8E30u /* doubles: 1 */
#define D_ZERO      0x08AD8E38u /* 0 */
#define D_HUGE      0x08AD8E40u /* the largest float */
#define D_HALF      0x08AD8E48u /* 0.5 */
#define D_NEG_HUGE  0x08AD8E50u /* minus the largest float */
#define D_INFINITY  0x08AD8F00u /* infinity */
#define ONE         0x3F800000u

void f_089FACA0(Cpu *c); void f_089F9F64(Cpu *c); void f_089FAEC0(Cpu *c); void f_089FC348(Cpu *c);
void f_089FBB50(Cpu *c); void f_089FBAD8(Cpu *c); void f_089FB6A8(Cpu *c); void f_089FB330(Cpu *c);
void f_089F9CC0(Cpu *c); void f_089FB8CC(Cpu *c); void f_089FAB48(Cpu *c); void f_089FAC88(Cpu *c);
void f_089FACCC(Cpu *c);

static float fb(uint32_t bits) { float f; memcpy(&f, &bits, 4); return f; }
static uint32_t bf(float f) { uint32_t bits; memcpy(&bits, &f, 4); return bits; }
/* A float with its low twelve bits cleared, noted in two words of the frame on the way. */
static float chopped(gaddr at, float f) {
    *W(at) = bf(f);
    *W(at + 4) = bf(f) & 0xFFFFF000u;
    return fb(bf(f) & 0xFFFFF000u);
}

/* (record): the program's own handling of a math error: there is none, so 0. */
SFA_FUNC(089F9CB8, math_error_hook) {
    c->r[2] = 0;
}

/* Infinity, in f0. */
SFA_FUNC(089FACC0, float_infinity) {
    ARG(0) = 0x7F800000u;
    c->f[0] = 0x7F800000u;
}

/* x to the power y, as bits. */
static uint32_t power_bits(Cpu *c, gaddr sp, uint32_t xb, uint32_t yb) {
    int32_t hx = (int32_t)xb, hy = (int32_t)yb;
    int32_t ix = hx & 0x7FFFFFFF, iy = hy & 0x7FFFFFFF;
    float x = fb(xb), y = fb(yb);
    if (iy == 0) return ONE;
    c->r[21] = 0x7F800000u;
    if (ix > 0x7F800000 || iy > 0x7F800000) return bf(x + y);
    /* Whether y is a whole number, for a negative x: 0 no, 1 odd, 2 even. */
    int32_t whole = 0;
    c->r[23] = ONE;
    if (hx < 0) {
        if (iy >= 0x4B800000) whole = 2;
        else if (iy >= 0x3F800000) {
            int32_t k = (iy >> 23) - 0x7F;
            int32_t j = iy >> ((23 - k) & 31);
            if ((int32_t)((uint32_t)j << ((23 - k) & 31)) == iy) whole = 2 - (j & 1);
        }
    }
    c->r[22] = (uint32_t)whole;
    if (iy == 0x7F800000) {
        if (ix == 0x3F800000) return 0;
        if (ix > 0x3F800000) return hy < 0 ? 0 : yb;
        return hy >= 0 ? 0 : yb ^ 0x80000000u;
    }
    if (iy == 0x3F800000) return hy >= 0 ? xb : bf(1.0f / x);
    if (hy == 0x40000000) return bf(x * x);
    c->r[30] = 0x3F000000u;
    if (hy == 0x3F000000 && hx >= 0) {
        c->f[12] = xb; c->r[31] = 0x089FA21Cu; f_089FAB48(c);
        return c->f[0];
    }
    c->f[12] = xb; c->r[31] = 0x089FA204u; f_089FAC88(c);
    uint32_t axb = c->f[0];
    float ax = fb(axb);
    if (ix == 0x7F800000 || ix == 0 || ix == 0x3F800000) {
        /* x is 0, 1 or infinity, of either sign. */
        uint32_t zb = axb;
        if (hy < 0) {
            if (FCMP(ax, 0.0f, 2)) {
                c->f[12] = 0; c->r[31] = 0x089FA298u; f_089FACC0(c);
                return c->f[0];
            }
            zb = bf(1.0f / ax);
        }
        if (hx < 0) {
            if (((ix - 0x3F800000) | whole) == 0) zb = 0;
            else if (whole == 1) zb ^= 0x80000000u;
        }
        return zb;
    }
    int32_t positive = (int32_t)(((uint32_t)hx >> 31) - 1); /* -1 for a positive x, 0 for a negative */
    c->r[16] = (uint32_t)positive;
    if ((positive | whole) == 0) return 0;

    /* The logarithm to base 2 of |x|, as t1 + t2, and t2 * y. */
    float t1, t2y;
    if (iy > 0x4D000000) {
        /* |y| is huge: anything not very near 1 overflows or underflows. */
        if (ix < 0x3F7FFFF8) return hy < 0 ? ONE : 0;
        if (ix > 0x3F800007) return hy > 0 ? ONE : 0;
        float t = x - 1.0f;
        float w = (t * t) * (0.5f - t * (fb(0x3EAAAAABu) - t * 0.25f));
        float u = t * fb(0x3FB8AA00u);
        float v = t * fb(0x36ECA570u) - w * fb(0x3FB8AA3Bu);
        t1 = chopped(sp + 24, u + v);
        t2y = (v - (t1 - u)) * y;
    } else {
        int32_t n = 0;
        if (ix < 0x00800000) {
            *W(sp + 32) = bf(ax * fb(0x4B800000u));
            n = -24;
            ix = (int32_t)*W(sp + 32);
        }
        n += (ix >> 23) - 0x7F;
        int32_t j = ix & 0x007FFFFF;
        ix = j | 0x3F800000;
        /* The interval |x| is in: below the square root of 3/2, below that of 3, or above. */
        int k = 0;
        if (j > 0x1CC471) {
            if (j < 0x5DB3D7) k = 1;
            else { n += 1; ix -= 0x00800000; }
        }
        float bp = fb(*W(0x08AD8E70u + 4 * (uint32_t)k));
        float dp_l = fb(*W(0x08AD8E80u + 4 * (uint32_t)k));
        float dp_h = fb(*W(0x08AD8E78u + 4 * (uint32_t)k));
        float tn = (float)n;
        *W(sp + 36) = (uint32_t)ix;
        float a = fb((uint32_t)ix);
        /* s = (a - bp) / (a + bp), as s_h + s_l. */
        float v = 1.0f / (a + bp);
        *W(sp + 48) = (uint32_t)(((ix >> 1) | 0x20000000) + 0x00040000 + (k << 21));
        float t_h = fb(*W(sp + 48));
        float u = a - bp;
        float s = u * v;
        float t_l = a - (t_h - bp);
        float s2 = s * s;
        float s_h = chopped(sp + 40, s);
        float s_l = v * ((u - s_h * t_h) - s_h * t_l);
        /* The logarithm of a. */
        float r = (s2 * s2) * (fb(0x3F19999Au) + s2 * (fb(0x3EDB6DB7u) + s2 * (fb(0x3EAAAAABu)
                  + s2 * (fb(0x3E8BA305u) + s2 * (fb(0x3E6C3255u) + s2 * fb(0x3E53F142u))))));
        r += s_l * (s + s_h);
        s2 = s_h * s_h;
        t_h = chopped(sp + 52, (s2 + 3.0f) + r);
        t_l = r - ((t_h - 3.0f) - s2);
        u = s_h * t_h;
        v = s_l * t_h + s * t_l;
        /* Times 2 / (3 log 2). */
        float p_h = chopped(sp + 60, u + v);
        float p_l = v - (p_h - u);
        float z_h = p_h * fb(0x3F763800u);
        float z_l = (p_h * fb(0x369DC3A0u) + p_l * fb(0x3F76384Fu)) + dp_l;
        t1 = chopped(sp + 68, ((z_h + z_l) + dp_h) + tn);
        t2y = (z_l - (((t1 - tn) - dp_h) - z_h)) * y;
    }
    c->r[22] = (uint32_t)(positive | (whole - 1));
    float sign = c->r[22] != 0 ? 1.0f : -1.0f; /* minus for a negative x to an odd power */

    /* y times the logarithm, as p_h + p_l. */
    *W(sp + 76) = yb;
    *W(sp + 80) = yb & 0xFFFFF000u;
    float y1 = fb(yb & 0xFFFFF000u);
    float p_l = (y - y1) * t1 + t2y;
    float p_h = t1 * y1;
    float z = p_h + p_l;
    *W(sp + 84) = bf(z);
    int32_t j = (int32_t)bf(z);
    int32_t i = j & 0x7FFFFFFF;
    const float tiny = fb(0x0DA24260u);
    if (j > 0x43000000) return 0;
    if (j == 0x43000000) {
        if (!FCMP(p_l + fb(0x3338AA3Cu), z - p_h, 14)) return 0;
    } else if (i > 0x43160000) {
        return bf((sign * tiny) * tiny);
    } else if (j == (int32_t)0xC3160000) {
        if (FCMP(p_l, z - p_h, 14)) return bf((sign * tiny) * tiny);
    }

    /* 2 to the power p_h + p_l: its whole part n, and the rest by a series. */
    int32_t k = (i >> 23) - 0x7F;
    int32_t n = 0;
    if (i > 0x3F000000) {
        n = j + (0x00800000 >> ((k + 1) & 31));
        k = ((n & 0x7FFFFFFF) >> 23) - 0x7F;
        *W(sp + 88) = (uint32_t)n & ~(uint32_t)(0x007FFFFF >> (k & 31));
        float whole_part = fb(*W(sp + 88));
        n = ((n & 0x007FFFFF) | 0x00800000) >> ((23 - k) & 31);
        if (j < 0) n = -n;
        p_h = p_h - whole_part;
    }
    float t = chopped(sp + 92, p_h + p_l);
    float u = t * fb(0x3F317200u);
    float v = (p_l - (t - p_h)) * fb(0x3F317218u) + t * fb(0x35BFBE8Cu);
    float e = u + v;
    float w = v - (e - u);
    float e2 = e * e;
    float e1 = e - e2 * (fb(0x3E2AAAABu) + e2 * (fb(0xBB360B61u) + e2 * (fb(0x388AB355u)
               + e2 * (fb(0xB5DDEA0Eu) + e2 * fb(0x3331BB4Cu)))));
    float r = (e1 * e) / (e1 - 2.0f) - (w + w * e);
    float out = 1.0f - (r - e);
    *W(sp + 100) = bf(out);
    int32_t scaled = (int32_t)(bf(out) + ((uint32_t)n << 23));
    if ((scaled >> 23) > 0) {
        *W(sp + 104) = (uint32_t)scaled;
        return bf(sign * fb((uint32_t)scaled));
    }
    c->f[12] = bf(out);
    ARG(0) = (uint32_t)n;
    c->r[31] = 0x089FAB00u; f_089FACCC(c);
    return bf(sign * fb(c->f[0]));
}

/* (f12 x, f13 y): x to the power y, in f0, with no report of errors: the special values of x
   and y are answered at once (this early form gives 0 where later ones give a NaN, and 0 or 1
   where they overflow or underflow), and the rest is 2 to the power y * log2(x), the logarithm
   kept in two parts for exactness. A 0xA0 frame keeping f20, f22, s0..s7 and fp. */
SFA_FUNC(089F9F84, powf_core) {
    uint32_t sv[8], fp = c->r[30], ra = c->r[31], f20 = c->f[20], f22 = c->f[22];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0xA0;
    gaddr sp = c->r[29];
    uint32_t xb = c->f[12], yb = c->f[13];
    *W(sp + 16) = xb; *W(sp + 20) = yb;
    *W(sp + 108) = f20; *W(sp + 112) = f22;
    for (int i = 0; i < 8; i++) *W(sp + 116 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 148) = fp; *W(sp + 152) = ra;
    c->r[16] = xb; c->r[17] = yb;
    c->r[18] = 0x7FFFFFFFu;
    c->f[20] = xb; c->f[22] = yb;
    c->r[20] = yb & 0x7FFFFFFFu;
    c->r[19] = xb & 0x7FFFFFFFu;
    uint32_t out = power_bits(c, sp, xb, yb);
    c->f[0] = out;
    c->f[20] = f20; c->f[22] = f22;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = fp; c->r[31] = ra; c->r[29] += 0xA0;
}

static int is_nan(Cpu *c, uint32_t bits, uint32_t back) {
    c->f[12] = bits; c->r[31] = back; f_089FACA0(c);
    return c->r[2] != 0;
}
static int is_finite(Cpu *c, uint32_t bits, uint32_t back) {
    c->f[12] = bits; c->r[31] = back; f_089F9F64(c);
    return c->r[2] != 0;
}
/* A float made a double, into two words. */
static void widen(Cpu *c, uint32_t bits, uint32_t back, gaddr to) {
    c->f[12] = bits; c->r[31] = back; f_089FAEC0(c);
    *W(to) = c->r[2];
    *W(to + 4) = c->r[3];
}
static void answer_set(gaddr sp, gaddr from) {
    *W(sp + 44) = *W(from + 4);
    *W(sp + 40) = *W(from);
}
/* The error's record at the frame's +16: its kind, the routine's name, x and y as doubles, the
   answer to give (+24 on) and an error number (+32 on), 0 for none. */
static void record(Cpu *c, gaddr sp, uint32_t kind, uint32_t xb, uint32_t yb, uint32_t back1, uint32_t back2) {
    *W(sp + 16) = kind;
    *W(sp + 20) = 0x08A02DA8u;
    *W(sp + 48) = 0;
    widen(c, xb, back1, sp + 24);
    widen(c, yb, back2, sp + 32);
}
static void errno_set(Cpu *c, uint32_t code, uint32_t back) {
    c->r[16] = code;
    c->r[31] = back; f_089FC348(c);
    *W(c->r[2]) = code;
}
/* The error is reported: by errno alone, or through the program's hook and, if that says 0, by
   errno. */
static void report(Cpu *c, gaddr sp, int by_errno, uint32_t code, uint32_t back_direct, uint32_t back_hook,
                   uint32_t back_errno) {
    if (by_errno) { errno_set(c, code, back_direct); return; }
    ARG(0) = sp + 0x10;
    c->r[31] = back_hook; f_089F9CB8(c);
    if (c->r[2] == 0) errno_set(c, code, back_errno);
}
/* The record's own error number, if any, goes to errno, and its answer is given as a float. */
static void answer_give(Cpu *c, gaddr sp, uint32_t back_errno, uint32_t back_narrow) {
    if (*W(sp + 48) != 0) {
        *W(sp + 56) = *W(sp + 48);
        c->r[31] = back_errno; f_089FC348(c);
        *W(c->r[2]) = *W(sp + 56);
    }
    ARG(0) = *W(sp + 40); ARG(1) = *W(sp + 44);
    c->r[31] = back_narrow; f_089FBB50(c);
}

/* The errors of x to the power y = z, when they are to be reported; whether one was, with its
   answer then in f0. */
static int power_errors(Cpu *c, gaddr sp, uint32_t xb, uint32_t yb, uint32_t zb) {
    float x = fb(xb), y = fb(yb), z = fb(zb);
    int32_t version;
    if (is_nan(c, yb, 0x089F9528u)) return 0;
    int x_nan = is_nan(c, xb, 0x089F9538u);
    c->f[26] = 0;
    if (x_nan) {
        /* A NaN to the power 0. */
        if (!FCMP(y, 0.0f, 2)) return 0;
        record(c, sp, 1, xb, yb, 0x089F956Cu, 0x089F9584u);
        c->r[18] = *W(sp + 24); c->r[19] = *W(sp + 28);
        version = (int32_t)*W(LIB_VERSION);
        answer_set(sp, sp + 24);
        if (version == -1 || version == 2) answer_set(sp, D_ONE);
        else report(c, sp, 0, 0x21, 0, 0x089F95C8u, 0x089F95D8u);
        answer_give(c, sp, 0x089F95F0u, 0x089F9604u);
        return 1;
    }
    if (FCMP(x, 0.0f, 2)) {
        if (FCMP(y, 0.0f, 2)) {
            /* 0 to the power 0. */
            record(c, sp, 1, xb, yb, 0x089F9678u, 0x089F9688u);
            answer_set(sp, D_ZERO);
            if (*W(LIB_VERSION) != 0) answer_set(sp, D_ONE);
            else report(c, sp, 0, 0x21, 0, 0x089F96CCu, 0x089F96DCu);
            answer_give(c, sp, 0x089F96F4u, 0x089F9708u);
            return 1;
        }
        if (!is_finite(c, yb, 0x089F9748u)) return 0;
        if (!FCMP(y, 0.0f, 12)) return 0;
        /* 0 to a negative power. */
        record(c, sp, 1, xb, yb, 0x089F977Cu, 0x089F978Cu);
        version = (int32_t)*W(LIB_VERSION);
        c->r[17] = (uint32_t)version;
        if (version == 0) answer_set(sp, D_ZERO);
        else {
            ARG(1) = *W(D_INFINITY + 4); ARG(0) = *W(D_INFINITY);
            c->r[31] = 0x089F97C8u; f_089FBAD8(c);
            *W(sp + 44) = c->r[3]; *W(sp + 40) = c->r[2];
        }
        report(c, sp, version == 2, 0x21, 0x089F97E0u, 0x089F97F0u, 0x089F9800u);
        answer_give(c, sp, 0x089F9818u, 0x089F982Cu);
        return 1;
    }
    if (!is_finite(c, zb, 0x089F986Cu) && is_finite(c, xb, 0x089F987Cu) && is_finite(c, yb, 0x089F988Cu)) {
        int z_nan = is_nan(c, zb, 0x089F989Cu);
        c->r[22] = (uint32_t)z_nan;
        version = (int32_t)*W(LIB_VERSION);
        c->r[17] = (uint32_t)version;
        c->r[18] = 0x08A02DA8u;
        c->r[19] = 2;
        uint32_t xd[2], yd[2];
        c->f[12] = xb; c->r[31] = 0x089F98B4u; f_089FAEC0(c);
        xd[0] = c->r[2]; xd[1] = c->r[3];
        c->r[20] = xd[0]; c->r[21] = xd[1];
        c->f[12] = yb; c->r[31] = 0x089F98C4u; f_089FAEC0(c);
        yd[0] = c->r[2]; yd[1] = c->r[3];
        *W(sp + 16) = z_nan ? 1 : 3;
        *W(sp + 20) = 0x08A02DA8u;
        *W(sp + 48) = 0;
        *W(sp + 28) = xd[1]; *W(sp + 24) = xd[0];
        *W(sp + 36) = yd[1]; *W(sp + 32) = yd[0];
        if (z_nan) {
            /* A negative number to a power that is not whole. */
            if (version == 0) answer_set(sp, D_ZERO);
            else {
                ARG(0) = *W(D_ZERO); ARG(1) = *W(D_ZERO + 4); ARG(2) = *W(D_ZERO); ARG(3) = *W(D_ZERO + 4);
                c->r[31] = 0x089F991Cu; f_089FB6A8(c);
                *W(sp + 44) = c->r[3]; *W(sp + 40) = c->r[2];
            }
            report(c, sp, version == 2, 0x21, 0x089F9934u, 0x089F9944u, 0x089F9954u);
            answer_give(c, sp, 0x089F996Cu, 0x089F9980u);
            return 1;
        }
        /* Too large: the largest float, or infinity, with a minus for a negative x to an odd
           power (told by whether half of y is whole). */
        ARG(0) = yd[0]; ARG(1) = yd[1]; ARG(2) = *W(D_HALF); ARG(3) = *W(D_HALF + 4);
        c->r[31] = 0x089F99E8u; f_089FB330(c);
        ARG(1) = c->r[3]; ARG(0) = c->r[2];
        c->r[31] = 0x089F99F4u; f_089FBB50(c);
        c->f[12] = c->f[0];
        answer_set(sp, version == 0 ? D_HUGE : D_INFINITY);
        if (FCMP(x, 0.0f, 12)) {
            c->r[31] = version == 0 ? 0x089F9A20u : 0x089F9A8Cu; f_089FAEC0(c);
            c->r[21] = c->r[3]; c->r[20] = c->r[2];
            ARG(1) = c->r[21]; ARG(0) = c->r[20];
            c->r[31] = version == 0 ? 0x089F9A34u : 0x089F9AA0u; f_089F9CC0(c);
            int was_zero = version == 0;
            version = (int32_t)*W(LIB_VERSION);
            c->r[17] = (uint32_t)version;
            ARG(1) = c->r[3]; ARG(3) = c->r[21]; ARG(2) = c->r[20]; ARG(0) = c->r[2];
            c->r[31] = was_zero ? 0x089F9A4Cu : 0x089F9AB8u; f_089FB8CC(c);
            if (c->r[2] != 0) {
                if (was_zero) answer_set(sp, D_NEG_HUGE);
                else {
                    ARG(1) = *W(D_INFINITY + 4); ARG(0) = *W(D_INFINITY);
                    c->r[31] = 0x089F9ACCu; f_089FBAD8(c);
                    *W(sp + 44) = c->r[3]; *W(sp + 40) = c->r[2];
                }
            }
        }
        report(c, sp, version == 2, 0x22, 0x089F9AE4u, 0x089F9AF4u, 0x089F9B04u);
        answer_give(c, sp, 0x089F9B1Cu, 0x089F9B30u);
        return 1;
    }
    /* Too small. */
    if (!FCMP(z, 0.0f, 2)) return 0;
    if (!is_finite(c, xb, 0x089F9B80u)) return 0;
    if (!is_finite(c, yb, 0x089F9B90u)) return 0;
    record(c, sp, 4, xb, yb, 0x089F9BB4u, 0x089F9BC4u);
    answer_set(sp, D_ZERO);
    report(c, sp, *W(LIB_VERSION) == 2, 0x22, 0x089F9BF8u, 0x089F9C08u, 0x089F9C18u);
    answer_give(c, sp, 0x089F9C30u, 0x089F9C44u);
    return 1;
}

/* (f12 x, f13 y): x to the power y, in f0. With the library set to report errors (it is not:
   the word at 0x08AD8E58 is -1), the cases the plain routine answers quietly are reported
   first. A 0x70 frame keeping f20, f22, f24, f26 and s0..s6, with the error's record at
   its +16. */
SFA_FUNC(089F94CC, lib_powf) {
    uint32_t sv[7], ra = c->r[31];
    uint32_t fv[4] = { c->f[20], c->f[22], c->f[24], c->f[26] };
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x70;
    gaddr sp = c->r[29];
    uint32_t xb = c->f[12], yb = c->f[13];
    *W(sp + 64) = fv[1];
    c->f[22] = yb;
    *W(sp + 60) = fv[0]; *W(sp + 68) = fv[2]; *W(sp + 72) = fv[3];
    for (int i = 0; i < 7; i++) *W(sp + 76 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 104) = ra;
    c->f[20] = xb;
    c->r[31] = 0x089F950Cu; f_089F9F84(c);
    uint32_t zb = c->f[0];
    c->r[16] = 0x08AE0000u;
    ARG(0) = *W(LIB_VERSION);
    c->r[17] = 0xFFFFFFFFu;
    c->f[24] = zb;
    if (ARG(0) == 0xFFFFFFFFu || !power_errors(c, sp, xb, yb, zb)) c->f[0] = zb;
    c->f[20] = fv[0]; c->f[22] = fv[1]; c->f[24] = fv[2]; c->f[26] = fv[3];
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x70;
}

/* (voice, bend): the voice among the sixteen at 0x08AD3AF8 (0x14 bytes each) has its pitch
   factor (+12) set from a bend: 2 to the power (bend + 49152) / 4096, as a whole number.
   Another address does nothing. A 0x20 frame keeping s0. */
SFA_FUNC(089E23E8, voice_bend_factor_set) {
    gaddr voice = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[7] = 0x08AD3AF8u;
    *W(sp + 16) = s0;
    c->r[6] = (uint32_t)(int16_t)ARG(1);
    ARG(1) = 0;
    c->r[16] = 0x08AD3AF8u;
    *W(sp + 20) = ra;
    while (c->r[16] != voice) {
        ARG(1)++;
        c->r[7] = (int32_t)ARG(1) < 16;
        c->r[16] += 0x14;
        if (c->r[7] == 0) break;
    }
    ARG(0) = 0x10;
    if (ARG(1) != 0x10) {
        c->f[12] = c->r[6];
        FSET(c, 13, (float)(int32_t)c->f[12]);
        c->f[14] = 0x47400000u;
        c->f[15] = 0x39800000u;
        FSET(c, 13, FS(c, 13) + FS(c, 14));
        ARG(0) = 0x40000000u;
        c->f[12] = 0x40000000u;
        FSET(c, 13, FS(c, 13) * FS(c, 15));
        c->r[31] = 0x089E2460u; f_089F94CC(c);
        c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 0));
        ARG(0) = c->f[12];
        *W(c->r[16] + 12) = ARG(0);
    }
    c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
