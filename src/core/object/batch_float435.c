/* The library's own arithmetic for doubles: addition and subtraction, and a double rounded to a
   whole number. A double taken apart is a record: +0 its class (0 and 1 a NaN, 2 zero, 3 a
   number, 4 infinity), +4 its sign, +8 its exponent, +16 its fraction (64 bits, the top bit of
   a number at bit 60). No scenario runs these; the differential test checks them against the
   originals.

   The addition is one routine in the original, with places inside it that others jump to; here
   each such place is a function of its own that the one before ends in, with the values it is
   handed in the registers the original keeps them in.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089FAF60(Cpu *c); void f_089FBF64(Cpu *c); void f_089FBCC4(Cpu *c);
void f_089FB08C(Cpu *c); void f_089FB090(Cpu *c); void f_089FB0D4(Cpu *c); void f_089FB16C(Cpu *c);

static uint64_t frac_get(gaddr rec) { return *W(rec + 16) | ((uint64_t)*W(rec + 20) << 32); }
static void frac_set(gaddr rec, uint64_t f) { *W(rec + 16) = (uint32_t)f; *W(rec + 20) = (uint32_t)(f >> 32); }

/* The sum (the record in r10, its fraction's top word in r2) is finished: its class is a
   number, and a fraction that has reached bit 61 is halved, keeping the bit that falls off, with
   the exponent one up. The record is the answer. */
SFA_FUNC(089FB16C, tail_sum_finish) {
    gaddr sum = c->r[10];
    *W(sum) = 3;
    if (c->r[2] > 0x1FFFFFFFu) {
        uint64_t f = frac_get(sum);
        frac_set(sum, (f & 1) | (f >> 1));
        *W(sum + 8) = *W(sum + 8) + 1;
    }
    ARG(2) = sum;
    f_089FAF60(c);
}

/* The difference (the record in r10) is brought up to the usual form: while its fraction is
   short of bit 60 it is doubled and the exponent taken one down. Then it is finished. */
SFA_FUNC(089FB0D4, tail_sum_normalize) {
    gaddr sum = c->r[10];
    uint64_t f = frac_get(sum);
    while (f - 1 < 0x0FFFFFFFFFFFFFFFull) {
        f <<= 1;
        *W(sum + 8) = *W(sum + 8) - 1;
        frac_set(sum, f);
    }
    c->r[8] = (uint32_t)f; c->r[9] = (uint32_t)(f >> 32);
    c->r[2] = *W(sum + 20);
    f_089FB16C(c);
}

/* The two fractions, brought to the same exponent (r11), are added or subtracted into the
   record in r10: r6 is the first's sign, r8 the second's record, r12/r13 and r14/r15 the
   fractions. With the same sign they are added; otherwise the smaller is taken from the larger
   and the sign is that of the larger. */
SFA_FUNC(089FB090, tail_sum_combine) {
    gaddr sum = c->r[10];
    uint32_t sign_a = c->r[6], sign_b = *W(c->r[8] + 4);
    uint64_t fa = c->r[12] | ((uint64_t)c->r[13] << 32), fb = c->r[14] | ((uint64_t)c->r[15] << 32);
    if (sign_a == sign_b) {
        *W(sum + 4) = sign_a;
        *W(sum + 8) = c->r[11];
        frac_set(sum, fa + fb);
        c->r[2] = *W(sum + 20);
        f_089FB16C(c);
        return;
    }
    int64_t d = (int64_t)(sign_a != 0 ? fb - fa : fa - fb);
    c->r[2] = 1;
    if (d < 0) {
        *W(sum + 4) = 1;
        *W(sum + 8) = c->r[11];
        frac_set(sum, (uint64_t)-d);
    } else {
        *W(sum + 8) = c->r[11];
        frac_set(sum, (uint64_t)d);
        *W(sum + 4) = 0;
    }
    f_089FB0D4(c);
}

/* The same, with the first's sign still to be read from its record (r7). */
SFA_FUNC(089FB08C, tail_sum_combine_signed) {
    c->r[6] = *W(c->r[7] + 4);
    f_089FB090(c);
}

/* A fraction moved down by `places`, one at a time, each bit that falls off kept in the lowest. */
static uint64_t shifted_down(uint64_t f, uint32_t places) {
    do f = (f & 1) | (f >> 1); while (--places != 0);
    return f;
}

/* (a, b, sum): two doubles taken apart are added; the answer is one of the three records, in
   r2. A NaN gives itself; infinity gives itself, unless the other is the opposite infinity (the
   NaN at 0x08A02DB0); zero gives the other, and two zeros a zero that is negative only when
   both are. Otherwise the one with the smaller exponent is moved down to the other's (to nothing
   when they are 64 or more apart) and the fractions are combined. */
SFA_FUNC(089FAF0C, double_parts_add) {
    gaddr a = ARG(0), b = ARG(1), sum = ARG(2);
    c->r[7] = a; c->r[10] = sum; c->r[8] = b;
    uint32_t class_a = *W(a);
    if (class_a < 2) { ARG(2) = a; f_089FAF60(c); return; }
    uint32_t class_b = *W(b);
    if (class_b < 2) { ARG(2) = b; f_089FAF60(c); return; }
    if (class_a == 4) {
        if (class_b == 4 && *W(a + 4) != *W(b + 4)) ARG(2) = 0x08A02DB0u;
        else ARG(2) = a;
        f_089FAF60(c);
        return;
    }
    if (class_b == 4) { ARG(2) = b; f_089FAF60(c); return; }
    if (class_b == 2) {
        if (class_a != 2) { ARG(2) = a; f_089FAF60(c); return; }
        *W(sum) = class_a;
        ARG(2) = sum;
        *W(sum + 4) = *W(a + 4);
        *W(sum + 8) = *W(a + 8);
        *W(sum + 12) = *W(a + 12);
        *W(sum + 16) = *W(a + 16);
        *W(sum + 20) = *W(a + 20);
        *W(sum + 4) = *W(a + 4) & *W(b + 4);
        f_089FAF60(c);
        return;
    }
    if (class_a == 2) { ARG(2) = b; f_089FAF60(c); return; }
    int32_t exp_a = (int32_t)*W(a + 8), exp_b = (int32_t)*W(b + 8);
    uint64_t fa = frac_get(a), fb = frac_get(b);
    int32_t apart = exp_a - exp_b;
    if (apart < 0) apart = -apart;
    int first_signed = 1; /* whether the first's sign is already in r6 */
    if (apart < 64) {
        if (exp_b < exp_a) { fb = shifted_down(fb, (uint32_t)(exp_a - exp_b)); exp_b = exp_a; }
        if (exp_a < exp_b) { fa = shifted_down(fa, (uint32_t)(exp_b - exp_a)); exp_a = exp_b; first_signed = 0; }
    } else if (exp_b < exp_a) {
        fb = 0;
    } else {
        fa = 0;
        exp_a = exp_b;
        first_signed = 0;
    }
    c->r[11] = (uint32_t)exp_a;
    c->r[9] = (uint32_t)exp_b;
    c->r[12] = (uint32_t)fa; c->r[13] = (uint32_t)(fa >> 32);
    c->r[14] = (uint32_t)fb; c->r[15] = (uint32_t)(fb >> 32);
    if (first_signed) { c->r[6] = *W(a + 4); f_089FB090(c); }
    else f_089FB08C(c);
}

/* Two doubles (a0/a1 and a2/a3) are taken apart into a 0x90 frame, the second's sign turned
   round for a subtraction, added, and the sum put together again in r2/r3. */
static void add_or_subtract(Cpu *c, int subtract, uint32_t back) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x90;
    gaddr sp = c->r[29];
    c->r[3] = ARG(1); c->r[2] = ARG(0);
    *W(sp + 128) = s0;
    c->r[16] = sp + 0x20;
    *W(sp + 132) = ra;
    *W(sp + 112) = ARG(2); *W(sp + 116) = ARG(3);
    *W(sp + 96) = c->r[2]; *W(sp + 100) = c->r[3];
    ARG(0) = sp + 0x60; ARG(1) = sp;
    c->r[31] = back + 0x34; f_089FBF64(c);
    ARG(0) = sp + 0x70; ARG(1) = sp + 0x20;
    c->r[31] = back + 0x40; f_089FBF64(c);
    if (subtract) {
        c->r[2] = *W(sp + 36) ^ 1;
        *W(sp + 36) = c->r[2];
    }
    ARG(1) = sp + 0x20; ARG(2) = sp + 0x40; ARG(0) = sp;
    c->r[31] = back + (subtract ? 0x5C : 0x50); f_089FAF0C(c);
    ARG(0) = c->r[2];
    c->r[31] = back + (subtract ? 0x64 : 0x58); f_089FBCC4(c);
    c->r[31] = ra; c->r[16] = s0; c->r[29] += 0x90;
}

/* (a, b as doubles): a + b, in r2/r3. */
SFA_FUNC(089FB254, double_add) {
    add_or_subtract(c, 0, 0x089FB254u);
}

/* (a, b as doubles): a - b, in r2/r3. */
SFA_FUNC(089FB2BC, double_subtract) {
    add_or_subtract(c, 1, 0x089FB2BCu);
}

/* The end of the doubles' multiplication: the record in a0 put together, and its 0xA0 frame's
   registers given back (s1 is not among them). */
SFA_FUNC(089FB3D8, tail_double_multiply_end) {
    gaddr sp = c->r[29];
    c->r[31] = 0x089FB3E0u; f_089FBCC4(c);
    c->r[31] = *W(sp + 148);
    c->r[21] = *W(sp + 144); c->r[20] = *W(sp + 140); c->r[19] = *W(sp + 136);
    c->r[18] = *W(sp + 132); c->r[16] = *W(sp + 128);
    c->r[29] += 0xA0;
}

/* (x as a double): x rounded to a whole number, in r2/r3, the way the processor rounds (to the
   nearest, a tie to the even one): 2 to the power 52, with x's sign, is added and taken away
   again, after the bits below a half are gathered into one so the first sum cannot round
   twice. Whole numbers, zeros and infinity come back as they are, and a NaN as x + x. A 0x40
   frame keeping s0..s3. */
SFA_FUNC(089F9CC0, double_round) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    uint32_t lo = ARG(0), hi = ARG(1);
    *W(sp + 4) = hi; *W(sp) = lo;
    uint32_t i0 = hi, i1 = lo;
    int32_t j0 = (int32_t)((hi >> 20) & 0x7FF) - 0x3FF;
    *W(sp + 40) = s0;
    uint32_t negative = hi >> 31;
    c->r[16] = negative;
    c->r[3] = hi; c->r[2] = lo;
    *W(sp + 44) = s1; *W(sp + 48) = s2; *W(sp + 52) = s3; *W(sp + 56) = ra;
    const gaddr two52 = 0x08AD8E60u + (negative << 3);
    if (j0 < 20) {
        if (j0 < 0) {
            c->r[17] = 0x7FFFFFFFu;
            if (((i0 & 0x7FFFFFFFu) | i1) == 0) goto done;
            i1 |= i0 & 0x000FFFFFu;
            i0 &= 0xFFFE0000u;
            i0 |= ((i1 | (0 - i1)) >> 12) & 0x00080000u;
            *W(sp + 12) = hi; *W(sp + 8) = lo; *W(sp + 12) = i0;
            c->r[19] = *W(two52 + 4); c->r[18] = *W(two52);
            ARG(3) = *W(sp + 12); ARG(2) = *W(sp + 8); ARG(1) = c->r[19]; ARG(0) = c->r[18];
            c->r[31] = 0x089F9D90u; f_089FB254(c);
            ARG(1) = c->r[3]; ARG(3) = c->r[19]; ARG(2) = c->r[18]; ARG(0) = c->r[2];
            c->r[31] = 0x089F9DA4u; f_089FB2BC(c);
            *W(sp + 20) = c->r[3]; *W(sp + 16) = c->r[2];
            *W(sp + 28) = c->r[3]; *W(sp + 24) = c->r[2];
            *W(sp + 28) = (*W(sp + 20) & 0x7FFFFFFFu) | (negative << 31);
            c->r[3] = *W(sp + 28); c->r[2] = *W(sp + 24);
            goto done;
        }
        uint32_t i = (uint32_t)(0x000FFFFF >> (j0 & 31));
        if (((i0 & i) | i1) == 0) goto done;
        i >>= 1;
        c->r[17] = *W(two52 + 4); c->r[16] = *W(two52);
        if (((i0 & i) | i1) != 0) {
            if (j0 == 19) i1 = 0x40000000u;
            else i0 = (i0 & ~i) | (uint32_t)(0x00020000 >> (j0 & 31));
        }
    } else {
        if (j0 >= 52) {
            if (j0 == 0x400) {
                ARG(1) = hi; ARG(3) = hi; ARG(0) = lo; ARG(2) = lo;
                c->r[31] = 0x089F9E80u; f_089FB254(c);
            }
            goto done;
        }
        uint32_t i = 0xFFFFFFFFu >> ((j0 - 20) & 31);
        if ((i1 & i) == 0) goto done;
        i >>= 1;
        c->r[17] = *W(two52 + 4); c->r[16] = *W(two52);
        if ((i1 & i) != 0) i1 = (i1 & ~i) | (0x40000000u >> ((j0 - 20) & 31));
    }
    *W(sp + 36) = i0; *W(sp + 32) = i1;
    ARG(3) = *W(sp + 36); ARG(2) = *W(sp + 32); ARG(1) = c->r[17]; ARG(0) = c->r[16];
    c->r[31] = 0x089F9F34u; f_089FB254(c);
    ARG(1) = c->r[3]; ARG(3) = c->r[17]; ARG(0) = c->r[2]; ARG(2) = c->r[16];
    c->r[31] = 0x089F9F48u; f_089FB2BC(c);
done:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x40;
}
