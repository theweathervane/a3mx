/* Library: a float's square root, a float scaled by a power of 2, one float's sign given to
   another, and a few one-line routines. No scenario runs these; the differential test checks
   the float routines against the originals.
 */
#include "step.h"
#include "../guest_mem.h"

#pragma STDC FP_CONTRACT OFF

/* Nothing. */
SFA_FUNC(089FAEB8, lib_nothing) {
    (void)c;
}

/* The third argument, in r2: the shared end of the doubles' addition, which returns the record
   it was working on. */
SFA_FUNC(089FAF60, tail_third_arg) {
    c->r[2] = ARG(2);
}

/* The first argument, in r2. */
SFA_FUNC(089FC0A4, lib_first_arg) {
    c->r[2] = ARG(0);
}

/* Where the error number is kept: the library's state (its first word), in r2. */
SFA_FUNC(089FC348, errno_place) {
    ARG(0) = 0x08AE0000u;
    c->r[2] = *W(0x08AD84DCu);
}

/* (f12 magnitude, f13 sign): the first float with the second's sign, in f0. */
SFA_FUNC(089FAE88, float_copy_sign) {
    c->r[6] = c->f[13];
    ARG(1) = c->f[13] & 0x80000000u;
    ARG(0) = (c->f[12] & 0x7FFFFFFFu) | ARG(1);
    c->f[0] = ARG(0);
}

/* The square root's answer for a negative number, 0 / 0, and its 0x10 frame given back. */
SFA_FUNC(089FABAC, tail_sqrt_of_negative) {
    c->f[0] = 0;
    FSET(c, 0, FS(c, 0) / FS(c, 0));
    c->r[29] += 0x10;
}

/* (f12): the square root, in f0, worked out a bit at a time: infinity and NaN give x * x + x,
   0 of either sign itself, a negative number 0 / 0. A 0x10 frame. */
SFA_FUNC(089FAB48, float_sqrt) {
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->f[12];
    int32_t ix = (int32_t)c->f[12];
    if ((ix & 0x7F800000) == 0x7F800000) {
        FSET(c, 0, FS(c, 12) * FS(c, 12));
        FSET(c, 0, FS(c, 0) + FS(c, 12));
        c->r[29] += 0x10;
        return;
    }
    if (ix <= 0) {
        if ((ix & 0x7FFFFFFF) == 0) {
            c->f[0] = c->f[12];
            c->r[29] += 0x10;
            return;
        }
        f_089FABAC(c);
        return;
    }
    int32_t m = ix >> 23;
    if (m == 0) {
        /* A number too small for the usual form: shifted up until it has one. */
        int32_t shifts = 0;
        do { ix <<= 1; shifts++; } while ((ix & 0x00800000) == 0);
        m = m - shifts + 1;
    }
    m -= 127;
    ix = (ix & 0x007FFFFF) | 0x00800000;
    ix += ix;
    if (m & 1) ix += ix;
    c->r[9] = (uint32_t)(m >> 1);
    int32_t s = 0, q = 0;
    for (uint32_t r = 0x01000000u; r != 0; r >>= 1) {
        int32_t t = s + (int32_t)r;
        if (ix >= t) {
            s = t + (int32_t)r;
            ix -= t;
            q += (int32_t)r;
        }
        ix += ix;
    }
    if (ix != 0) q += q & 1;
    uint32_t out = (uint32_t)(q >> 1) + 0x3F000000u + (c->r[9] << 23);
    ARG(0) = out;
    *W(sp + 4) = out;
    c->f[0] = out;
    c->r[29] += 0x10;
}

/* (f12 x, n): x times 2 to the power n, in f0, by changing the exponent: 0, infinity and NaN
   stay as they are, a result too large or too small is the largest or smallest float squared
   with x's sign, and one below the usual form is made 25 places up and brought down by a
   multiplication. A 0x30 frame keeping f20. */
SFA_FUNC(089FACCC, float_scale) {
    int32_t n = (int32_t)ARG(0);
    uint32_t f20 = c->f[20], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = c->f[12];
    uint32_t ix = c->f[12];
    int32_t k = (int32_t)(ix & 0x7F800000u) >> 23;
    *W(sp + 32) = f20; *W(sp + 36) = ra;
    c->f[0] = c->f[12];
    const uint32_t huge = 0x7149F2CAu, tiny = 0x0DA24260u;
    uint32_t with_sign = 0;
    if (k == 0) {
        if ((ix & 0x7FFFFFFFu) == 0) goto done;
        c->f[12] = 0x4C000000u;
        FSET(c, 0, FS(c, 0) * FS(c, 12));
        *W(sp + 20) = c->f[0];
        ix = c->f[0];
        k = ((int32_t)(ix & 0x7F800000u) >> 23) - 25;
        if (n < -50000) {
            c->f[12] = tiny;
            FSET(c, 0, FS(c, 0) * FS(c, 12));
            goto done;
        }
    }
    if (k == 0xFF) {
        FSET(c, 0, FS(c, 0) + FS(c, 0));
        goto done;
    }
    k += n;
    if (k >= 255) with_sign = huge;
    else if (k > 0) {
        *W(sp + 24) = (ix & 0x807FFFFFu) | ((uint32_t)k << 23);
        c->f[0] = *W(sp + 24);
        goto done;
    } else if (k < -24) with_sign = n > 50000 ? huge : tiny;
    else {
        *W(sp + 28) = (ix & 0x807FFFFFu) | ((uint32_t)(k + 25) << 23);
        c->f[0] = *W(sp + 28);
        c->f[12] = 0x33000000u;
        FSET(c, 0, FS(c, 0) * FS(c, 12));
        goto done;
    }
    c->f[13] = c->f[0];
    c->f[20] = with_sign;
    c->f[12] = with_sign;
    c->r[31] = with_sign == tiny ? 0x089FAE74u : k >= 255 ? 0x089FAD64u : 0x089FADF4u;
    f_089FAE88(c);
    FSET(c, 0, FS(c, 0) * FS(c, 20));
done:
    c->f[20] = f20; c->r[31] = ra; c->r[29] += 0x30;
}
