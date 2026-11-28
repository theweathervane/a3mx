/* More of the library's own arithmetic: one double divided by another, and a double with its sign
   turned. The parts of a number are described in batch_float383.c. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089FBF64(Cpu *c); void f_089FBCC4(Cpu *c); void f_089FB740(Cpu *c);

/* The end of the division: the quotient (r14, r15) becomes the fraction of the parts r13 points
   at, and the double is built from them. */
SFA_FUNC(089FB844, tail_divide_store) {
    *W(c->r[13] + 16) = c->r[14];
    *W(c->r[13] + 20) = c->r[15];
    ARG(0) = c->r[13];
    f_089FB740(c);
}

/* The division's loop: the quotient (r14, r15) is built a bit at a time (the bit in r8, r9) from
   the high one down, the rest (r4, r5) doubling each time and the divisor (r10, r11) coming off
   it whenever it fits; r12 says whether the rest's high word is below the divisor's. With
   `subtract_first` the loop is entered where the divisor comes off, the borrow in r2. Then the
   quotient is rounded to the nearest at bit 8, a half going up only if bit 8 is set or anything
   is left over. */
static void divide_loop(Cpu *c, int subtract_first) {
    for (;;) {
        int subtract = subtract_first;
        subtract_first = 0;
        if (!subtract) {
            c->r[2] = c->r[9] << 31;
            if (c->r[12] == 0) {
                c->r[2] = c->r[4] < c->r[10];
                if (c->r[11] == c->r[5] && c->r[2] != 0) c->r[2] = c->r[9] << 31;
                else subtract = 1;
            }
        }
        if (subtract) {
            c->r[5] = c->r[5] - c->r[11];
            c->r[4] = c->r[4] - c->r[10];
            c->r[5] = c->r[5] - c->r[2];
            c->r[14] |= c->r[8];
            c->r[15] |= c->r[9];
            c->r[2] = c->r[9] << 31;
        }
        c->r[8] = (c->r[8] >> 1) | c->r[2];
        c->r[3] = c->r[4] >> 31;
        c->r[7] = (c->r[5] << 1) | c->r[3];
        c->r[9] = c->r[9] >> 1;
        c->r[6] = c->r[4] << 1;
        c->r[2] = c->r[8] | c->r[9];
        c->r[4] = c->r[6];
        c->r[5] = c->r[7];
        c->r[12] = c->r[7] < c->r[11];
        if (c->r[2] == 0) break;
    }
    c->r[2] = c->r[14] & 0xFF;
    ARG(0) = 0x80;
    c->r[3] = 0;
    if (c->r[2] == 0x80) {
        c->r[2] = c->r[14] & 0x100;
        int up = c->r[2] != 0;
        if (!up) {
            c->r[2] = c->r[6] | c->r[7];
            up = c->r[2] != 0;
        }
        if (up) {
            c->r[14] += 0x80;
            c->r[2] = c->r[14] < 0x80;
            c->r[15] += c->r[2];
        }
    }
    f_089FB844(c);
}

/* The division's loop from where the divisor comes off the rest. */
SFA_FUNC(089FB7E4, tail_divide_subtract) {
    divide_loop(c, 1);
}

/* The division's loop from its start: the first bit is bit 28 of the high word. */
SFA_FUNC(089FB7C0, tail_divide_loop) {
    c->r[9] = 0x10000000u;
    c->r[8] = 0;
    c->r[14] = 0; c->r[15] = 0;
    divide_loop(c, 0);
}

/* (a low, a high, b low, b high): a divided by b, in r2 (low) and r3 (high). What is not a number
   stays so; infinity by infinity and zero by zero give the constant at 0x08A02DB0; a number by
   infinity gives zero, and by zero infinity. A 0x70 frame keeping s0, with a's parts at +0, b's
   at +32 and the two doubles at +64 and +80; the ends close it. */
SFA_FUNC(089FB6A8, double_divide) {
    c->r[29] -= 0x70;
    gaddr sp = c->r[29];
    c->r[2] = ARG(0); c->r[3] = ARG(1);
    *W(sp + 96) = c->r[16];
    *W(sp + 64) = ARG(0);
    *W(sp + 68) = ARG(1);
    *W(sp + 100) = c->r[31];
    *W(sp + 80) = ARG(2);
    *W(sp + 84) = ARG(3);
    c->r[16] = sp + 0x20;
    ARG(0) = sp + 0x40; ARG(1) = sp;
    c->r[31] = 0x089FB6DCu; f_089FBF64(c);
    ARG(0) = sp + 0x50; ARG(1) = c->r[16];
    c->r[31] = 0x089FB6E8u; f_089FBF64(c);
    uint32_t ka = *W(sp);
    ARG(1) = ka;
    c->r[13] = sp;
    c->r[2] = ka < 2;
    ARG(0) = sp;
    if (ka < 2) { f_089FB740(c); return; }
    uint32_t kb = *W(sp + 32);
    c->r[6] = kb;
    c->r[2] = kb < 2;
    ARG(0) = c->r[16];
    if (kb < 2) { f_089FB740(c); return; }
    ARG(0) = *W(sp + 36);
    c->r[3] = *W(sp + 4) ^ ARG(0);
    c->r[2] = ka ^ 4;
    *W(sp + 4) = c->r[3];
    if (ka == 4 || ka == 2) {
        if (ka != 4) c->r[2] = kb ^ 4;
        c->r[2] = 0x08A00000u;
        ARG(0) = 0x08A02DB0u;
        if (ka != kb) ARG(0) = sp;
        f_089FB740(c);
        return;
    }
    c->r[2] = kb ^ 2;
    if (kb == 4) {
        c->r[2] = 0; c->r[3] = 0;
        ARG(0) = sp;
        *W(sp + 16) = 0;
        *W(sp + 20) = 0;
        *W(sp + 8) = 0;
        f_089FB740(c);
        return;
    }
    c->r[3] = *W(sp + 8);
    if (kb == 2) {
        c->r[2] = 4;
        ARG(0) = sp;
        *W(sp) = 4;
        f_089FB740(c);
        return;
    }
    ARG(0) = *W(sp + 16);
    ARG(1) = *W(sp + 20);
    c->r[10] = *W(sp + 48);
    c->r[11] = *W(sp + 52);
    c->r[2] = *W(sp + 40);
    c->r[12] = ARG(1) < c->r[11];
    c->r[3] = c->r[3] - c->r[2];
    *W(sp + 8) = c->r[3];
    int below = c->r[12] != 0;
    if (!below) {
        c->r[2] = ARG(0) < c->r[10];
        below = c->r[11] == ARG(1) && c->r[2] != 0;
    }
    if (below) {
        /* a's fraction is below b's: it is doubled, and the exponent goes down one. */
        c->r[2] = c->r[3] - 1;
        c->r[3] = ARG(0) >> 31;
        ARG(1) = (ARG(1) << 1) | c->r[3];
        *W(sp + 8) = c->r[2];
        ARG(0) = ARG(0) << 1;
        c->r[12] = ARG(1) < c->r[11];
    }
    f_089FB7C0(c);
}

/* (low, high): the double with its sign turned. A 0x40 frame with the double at +32 and its
   parts at +0. */
SFA_FUNC(089FBAD8, double_negate) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    c->r[2] = ARG(0); c->r[3] = ARG(1);
    *W(sp + 48) = ra;
    *W(sp + 32) = ARG(0);
    *W(sp + 36) = ARG(1);
    ARG(0) = sp + 0x20; ARG(1) = sp;
    c->r[31] = 0x089FBAFCu; f_089FBF64(c);
    ARG(0) = sp;
    c->r[2] = *W(sp + 4) < 1;
    *W(sp + 4) = c->r[2];
    c->r[31] = 0x089FBB10u; f_089FBCC4(c);
    c->r[31] = ra; c->r[29] += 0x40;
}
