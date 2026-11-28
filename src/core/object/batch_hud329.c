/* The run's own line on screen in the modes with a list of opponents: the time, and either a row
   of marks for the fights done and still to come or the number left. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define PAIR 0x095179E0u

void f_0880A6CC(Cpu *c); void f_0883296C(Cpu *c);

/* 0x0880A6CC: (text, a, x, y, e). */
static void text_at(Cpu *c, uint32_t text, uint32_t a, uint32_t x, uint32_t y, uint32_t e, uint32_t back) {
    ARG(0) = text; ARG(1) = a; ARG(2) = x; ARG(3) = y; c->r[8] = e;
    c->r[31] = back; f_0880A6CC(c);
}

/* (g, y, fighter, x, tx): nothing unless the fighter's +412 is set. With g's +137 clear the label
   0x089FE2C4 at tx and the run's time (PAIR + 12..14, written out at 0x08A0F82C) at x / 0xB8.
   From mode 4 on (y + 79), a row of marks is built at 0x08A0F837 and drawn at 0xE0 / 0x20 for the
   first side and 0x50 / 0x20 for the second: one for each fight of the run (PAIR + 4), 0x23 for
   those done (y + 108) and 0x22 for the rest, filled from the right for the first side and padded
   to five with spaces on the left for the second. But with g's +137 set and six or more at
   0x09517A78 + 910, a number is drawn instead (at 0x108, or 0x44 for the second side): how many
   are left, or in mode 0xE (+946) how many are done, as three digits (the hundreds and, without
   hundreds, the tens from the table 0x08A0F708, the others from 0x08A0F6F8), and the drawn text's
   +3 = 7 and +12 = 1. A 0x40 frame keeping s0..s5. */
SFA_FUNC(0883323C, run_line) {
    const gaddr marks = 0x08A0F837u;
    gaddr g = ARG(0), y = ARG(1), f = ARG(2);
    int32_t x = (int16_t)ARG(3), tx = (int16_t)c->r[8];
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0]; *W(sp + 44) = sv[3];
    c->r[16] = g;
    *W(sp + 48) = sv[4];
    int32_t shown = S8(f + 412);
    *W(sp + 36) = sv[1];
    c->r[19] = (uint32_t)x; c->r[17] = y; c->r[20] = PAIR;
    *W(sp + 40) = sv[2]; *W(sp + 52) = sv[5]; *W(sp + 56) = sv[6];
    c->r[18] = f;
    if (shown == 0) goto out;
    if (S8(g + 137) == 0) {
        text_at(c, 0x089FE2C4u, 1, (uint32_t)tx, 0, 4, 0x088332ACu);
        const gaddr time = 0x08A0F82Cu;
        c->r[21] = time;
        ARG(1) = (uint32_t)(int32_t)S8(PAIR + 12);
        ARG(2) = (uint32_t)(int32_t)S8(PAIR + 13);
        ARG(3) = (uint32_t)(int32_t)S8(PAIR + 14);
        ARG(0) = time; c->r[31] = 0x088332C8u; f_0883296C(c);
        text_at(c, time, 1, (uint32_t)x, 0xB8, 3, 0x088332E0u);
    }
    if (S8(y + 79) < 4) goto out;
    {
        int32_t paused = S8(g + 137);
        int32_t side = S8(f + 337);
        c->r[16] = (uint32_t)side;
        if (paused != 0 && !(S8(0x09517A78u + 910) < 6)) {
            const gaddr blank_zero = 0x08A0F708u, digits = 0x08A0F6F8u;
            int32_t done = S8(y + 108);
            int32_t n;
            gaddr text, at;
            if (S16(0x09517A78u + 946) == 0xE) { text = 0x08A0F846u; at = text; n = (int8_t)done; }
            else { text = 0x08A0F83Eu; at = text + 4; n = (int16_t)(*W(PAIR + 4) - (uint32_t)done); }
            int32_t hundreds = n / 100;
            *B(at) = (uint8_t)S8(blank_zero + (uint32_t)hundreds);
            n = (int16_t)(n % 100);
            c->r[17] = (uint32_t)n;
            *B(at + 1) = (uint8_t)S8((hundreds != 0 ? digits : blank_zero) + (uint32_t)(n / 10));
            DIV(c, (uint32_t)n, 10);
            *B(at + 2) = (uint8_t)S8(digits + c->hi);
            c->r[16] = 1;
            text_at(c, text, 1, side != 0 ? 0x44 : 0x108, 0, 1, 0x08833588u);
            *B(c->r[2] + 3) = 7;
            *B(c->r[2] + 12) = 1;
            goto out;
        }
        uint32_t total = *W(PAIR + 4);
        uint32_t col;
        if (side == 0) {
            *B(marks + total) = 0;
            int32_t at = (int16_t)(*W(PAIR + 4) - 1);
            int32_t stop = (int16_t)(at - S8(y + 108));
            c->r[17] = (uint32_t)stop;
            while (stop < at) {
                *B(marks + (uint32_t)at) = 0x23;
                at = (int16_t)(at - 1);
            }
            while (!(at < 0)) {
                *B(marks + (uint32_t)at) = 0x22;
                at = (int16_t)(at - 1);
            }
            c->r[20] = (uint32_t)at;
            col = 0xE0;
        } else {
            gaddr p = marks;
            int32_t pad = (int16_t)(5 - total);
            for (int32_t i = 0; i < pad; i = (int16_t)(i + 1)) *B(p++) = 0x20;
            for (int32_t i = 0; i < S8(y + 108); i = (int16_t)(i + 1)) *B(p++) = 0x23;
            int32_t left = (int16_t)(*W(PAIR + 4) - (uint32_t)(int32_t)S8(y + 108));
            int32_t i = 0;
            for (; i < left; i = (int16_t)(i + 1)) *B(p++) = 0x22;
            c->r[20] = (uint32_t)i;
            *B(p) = 0;
            col = 0x50;
        }
        text_at(c, marks, 2, col, 0x20, 3, 0x08833474u);
    }
out:
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x40;
}
