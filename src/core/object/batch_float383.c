/* The library's own arithmetic, first pieces: a float taken apart, a double put together from
   its parts, and with them a float widened to a double. Offsets stay numbers until something
   shows what they are.

   A number taken apart is six words: its kind (0 and 1 not a number, 2 zero, 3 a number, 4
   infinity), its sign, its exponent, and its fraction (one word for a float at +12, two for a
   double at +16 and +20) with the leading one at bit 30 of the float's word, or bit 28 of the
   double's high one.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089FBDEC(Cpu *c); void f_089FBCF8(Cpu *c); void f_089FBCFC(Cpu *c); void f_089FBEF8(Cpu *c);
void f_089FBC3C(Cpu *c); void f_089E4978(Cpu *c);

/* The end of a routine that stores a float's fraction: r7 to +12 of the place in r5. */
SFA_FUNC(089FBC34, tail_fraction_at_r5) {
    *W(c->r[5] + 12) = c->r[7];
}

/* (bits, out): the float the first points at, taken apart to the second. */
SFA_FUNC(089FBBC8, float_unpack) {
    gaddr out = ARG(1);
    uint32_t bits = *W(ARG(0));
    c->r[2] = bits;
    c->r[3] = 0x007FFFFFu;
    uint32_t exp = (bits >> 23) & 0xFF;
    c->r[6] = exp;
    ARG(0) = bits >> 31;
    *W(out + 4) = ARG(0);
    uint32_t frac = bits & 0x007FFFFFu;
    c->r[7] = frac;
    if (exp == 0) {
        c->r[2] = 2;
        if (frac == 0) { f_089FBC3C(c); return; }
        frac <<= 7;
        c->r[7] = frac;
        int32_t e = -126;
        c->r[3] = (uint32_t)e;
        ARG(0) = 3;
        c->r[2] = 0x3FFFFFFFu < frac;
        *W(out + 8) = (uint32_t)e;
        *W(out) = 3;
        if (c->r[2] == 0) {
            ARG(0) = 0x3FFFFFFFu;
            do {
                frac <<= 1;
                c->r[2] = 0x3FFFFFFFu < frac;
                e--;
            } while (c->r[2] == 0);
            c->r[7] = frac;
            c->r[3] = (uint32_t)e;
            *W(out + 8) = (uint32_t)e;
        }
        f_089FBC34(c);
        return;
    }
    c->r[2] = frac << 7;
    if (exp == 0xFF) {
        if (frac == 0) { c->r[2] = 4; f_089FBC3C(c); return; }
        c->r[2] = frac & 0x00100000u;
        if (c->r[2] == 0) *W(out) = 0;
        else { c->r[2] = 1; *W(out) = 1; }
        f_089FBC34(c);
        return;
    }
    c->r[3] = 0x40000000u;
    c->r[2] |= 0x40000000u;
    ARG(0) = exp - 127;
    *W(out + 12) = c->r[2];
    c->r[2] = 3;
    *W(out + 8) = ARG(0);
    f_089FBC3C(c);
}

/* A part of the routine that builds a double: the carry in r2 is added to the fraction's high
   part (r11); past 28 bits the exponent (r9) becomes 1; the low part comes down eight bits with
   the high part's low byte above it. */
SFA_FUNC(089FBDC8, tail_double_round_small) {
    c->r[11] = c->r[11] + c->r[2];
    c->r[2] = 0x0FFFFFFFu < c->r[11];
    c->r[3] = 1;
    c->r[10] = c->r[10] >> 8;
    ARG(0) = c->r[11] << 24;
    if (c->r[2] != 0) c->r[9] = 1;
    c->r[10] |= ARG(0);
    f_089FBDEC(c);
}

/* A part of the routine that builds a double, for a number too small for a whole exponent: its
   fraction (r10, r11) is rounded to the nearest at bit 8, a half going to the even side. */
SFA_FUNC(089FBDA8, tail_double_round_denormal) {
    c->r[2] = c->r[10] & 0xFF;
    ARG(0) = 0x80;
    c->r[3] = 0;
    if (c->r[2] == 0x80) {
        c->r[2] = c->r[10] & 0x100;
        if (c->r[2] == 0) {
            c->r[2] = 0x0FFFFFFFu < c->r[11];
            c->r[3] = 1;
            c->r[10] = c->r[10] >> 8;
            ARG(0) = c->r[11] << 24;
            if (c->r[2] != 0) c->r[9] = 1;
            c->r[10] |= ARG(0);
            f_089FBDEC(c);
            return;
        }
        c->r[10] += 0x80;
        c->r[2] = c->r[10] < 0x80;
    } else {
        c->r[10] += 0x7F;
        c->r[2] = c->r[10] < 0x7F;
    }
    f_089FBDC8(c);
}

/* (parts): a double put together from its parts, returned in r2 (low) and r3 (high). A 0x10
   frame the ends close. */
SFA_FUNC(089FBCC4, double_pack) {
    gaddr parts = ARG(0);
    uint32_t kind = *W(parts);
    c->r[3] = kind;
    c->r[29] -= 0x10;
    c->r[10] = *W(parts + 16);
    c->r[11] = *W(parts + 20);
    c->r[2] = kind < 2;
    c->r[12] = *W(parts + 4);
    c->r[9] = 0;
    if (kind < 2) {
        c->r[3] = 0x00080000u;
        c->r[2] = 0;
        c->r[11] |= 0x00080000u;
        c->r[9] = 0x7FF;
        f_089FBCF8(c);
        return;
    }
    int infinite = kind == 4;
    c->r[2] = kind ^ 4;
    if (!infinite && kind != 2) {
        c->r[2] = c->r[10] | c->r[11];
        if (c->r[2] == 0) { c->r[2] = *W(c->r[29] + 4); f_089FBCFC(c); return; }
        int32_t exp = (int32_t)*W(parts + 8);
        ARG(0) = (uint32_t)exp;
        if (exp < -1022) {
            uint32_t n = (uint32_t)(-1022 - exp);
            c->r[13] = n;
            c->r[3] = (int32_t)n < 57;
            if ((int32_t)n >= 57) {
                c->r[10] = 0; c->r[11] = 0;
                f_089FBDA8(c);
                return;
            }
            /* The fraction shifted down n places, with anything lost kept as its lowest bit. */
            uint64_t frac = ((uint64_t)c->r[11] << 32) | c->r[10];
            uint64_t lost = frac & (((uint64_t)1 << n) - 1);
            frac >>= n;
            ARG(1) = 0; ARG(0) = 1;
            c->r[2] = lost != 0;
            c->r[10] = (uint32_t)frac | c->r[2];
            c->r[11] = (uint32_t)(frac >> 32);
            f_089FBDA8(c);
            return;
        }
        c->r[2] = exp < 1024;
        c->r[9] = 0x7FF;
        if (exp < 1024) {
            c->r[3] = 0;
            c->r[6] = c->r[10] & 0xFF;
            c->r[7] = 0;
            c->r[9] = (uint32_t)exp + 0x3FF;
            if (c->r[6] == 0x80) {
                c->r[2] = c->r[10] & 0x100;
                if (c->r[2] == 0) {
                    c->r[2] = 0x1FFFFFFFu < c->r[11];
                    if (c->r[2] != 0) {
                        c->r[2] = c->r[11] << 31;
                        c->r[10] = (c->r[10] >> 1) | c->r[2];
                        c->r[11] = c->r[11] >> 1;
                        c->r[9] = c->r[9] + 1;
                    }
                    c->r[10] = c->r[10] >> 8;
                    c->r[2] = c->r[11] << 24;
                    c->r[10] |= c->r[2];
                    f_089FBDEC(c);
                    return;
                }
                c->r[10] += 0x80;
                c->r[2] = c->r[10] < 0x80;
            } else {
                c->r[10] += 0x7F;
                c->r[2] = c->r[10] < 0x7F;
            }
            f_089FBEF8(c);
            return;
        }
        infinite = 1;
    }
    if (infinite) c->r[9] = 0x7FF;
    c->r[10] = 0; c->r[11] = 0;
    c->r[2] = *W(c->r[29] + 4);
    f_089FBCFC(c);
}

/* (kind, sign, exponent, -, fraction low, fraction high in r8 and r9): a double from parts given
   in registers. A 0x30 frame holding them. */
SFA_FUNC(089FBB1C, double_from_parts) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    c->r[2] = ARG(0);
    *W(sp + 32) = ra;
    *W(sp) = ARG(0); *W(sp + 4) = ARG(1); *W(sp + 8) = ARG(2);
    *W(sp + 16) = c->r[8]; *W(sp + 20) = c->r[9];
    ARG(0) = sp;
    c->r[31] = 0x089FBB44u; f_089FBCC4(c);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (f12): the float as a double, in r2 (low) and r3 (high). A 0x30 frame with the parts at +0
   and the float at +16. */
SFA_FUNC(089FAEC0, float_to_double) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    ARG(0) = sp + 0x10; ARG(1) = sp;
    *W(sp + 32) = ra;
    *W(sp + 16) = c->f[12];
    c->r[31] = 0x089FAED8u; f_089FBBC8(c);
    uint32_t frac = *W(sp + 12);
    ARG(0) = *W(sp); ARG(1) = *W(sp + 4); ARG(2) = *W(sp + 8);
    c->r[2] = frac >> 2;
    c->r[9] = frac >> 2;
    c->r[8] = frac << 30;
    c->r[31] = 0x089FAF00u; f_089FBB1C(c);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (f12): the screen's fade, by way of the float widened to a double (which the fade does not
   read). A 0x20 frame. */
SFA_FUNC(089E0BA8, fade_alpha_set_wide) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x089E0BB8u; f_089FAEC0(c);
    ARG(1) = c->r[3];
    ARG(0) = c->r[2];
    c->r[31] = 0x089E0BC4u; f_089E4978(c);
    c->r[31] = ra; c->r[29] += 0x20;
}
