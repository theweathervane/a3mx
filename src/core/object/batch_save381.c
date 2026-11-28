/* The save screen's waits: for a save or a load to finish, for the check of what is on the card,
   and for the free space, each choosing the message to show next. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

/* The save screen's record: +2 its step, +3 the slot, +4 the message to show, +5 a result, +8
   whether it saves, +9 a cursor, +10 what it is for. */

void f_08816FC4(Cpu *c); void f_08816EC4(Cpu *c); void f_089E32CC(Cpu *c); void f_089E3838(Cpu *c);
void f_08816B74(Cpu *c); void f_089E358C(Cpu *c); void f_08816B90(Cpu *c); void f_089E7AE4(Cpu *c);

#define NO_DATA 0xFFFFFF41u /* the answer when the file is not one of the game's */

/* (o): the wait for a save or a load. When the job answers, the step becomes 4 with the answer at
   +5, and the message follows: for a save 0xE on success (and the routine after a save runs),
   0x52 for the answer 0xFFFFFF0B, 0xF for any other; for a load 0x11 on success, 0x51 or 6 (by
   +10 being 1) when the file is not the game's, 0x12 for any other. A 0x10 frame keeping s0. */
SFA_FUNC(08817B50, save_screen_wait_job) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    c->r[31] = 0x08817B64u; f_08816FC4(c);
    uint32_t r = c->r[2];
    ARG(0) = r;
    ARG(1) = 0xFFFFFFFFu;
    if (r != 0xFFFFFFFFu) {
        *B(o + 2) = 4;
        *B(o + 5) = (uint8_t)r;
        int32_t saves = S8(o + 8);
        ARG(1) = (uint32_t)saves;
        uint32_t msg;
        if (r == 0) {
            if (saves == 0) msg = 0x11;
            else {
                msg = 0xE;
                *B(o + 4) = 0xE;
                ARG(0) = 0xE;
                c->r[31] = 0x08817B98u; f_08816EC4(c);
            }
        } else if (saves == 0) {
            ARG(1) = NO_DATA;
            if (r != NO_DATA) msg = 0x12;
            else {
                ARG(1) = 1;
                msg = S8(o + 10) != 1 ? 6 : 0x51;
            }
        } else {
            ARG(1) = 0xFFFFFF0Bu;
            msg = r != 0xFFFFFF0Bu ? 0xF : 0x52;
        }
        if (!(r == 0 && saves != 0)) { ARG(0) = msg; *B(o + 4) = (uint8_t)msg; }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* (o): the wait for the check of what is on the card. While the dialog is up, nothing. When data
   is there: step 2, cursor -1, and the message 0x31 for a save, else 0x18 (or 0x41 when +10 is
   neither 0 nor 4). When there is none (0xFFFFFF05) or it is not the game's: a save goes on to
   its routine for the slot and step 0xA; a load ends at step 4 with 5 at +5 and a message by
   +10. Any other answer: step 6, message 0x13. A 0x20 frame keeping s0, s1. */
SFA_FUNC(08817D68, save_screen_wait_check) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = o;
    c->r[31] = 0x08817D80u; f_089E32CC(c);
    c->r[17] = 0xFFFFFFFFu;
    if (c->r[2] != 0xFFFFFFFFu) {
        c->r[31] = 0x08817D94u; f_089E3838(c);
        uint32_t r = c->r[2];
        ARG(0) = r;
        ARG(1) = 0xFFFFFF05u;
        if (r == 0) {
            *B(o + 2) = 2;
            int32_t saves = S8(o + 8);
            *B(o + 9) = 0xFF;
            if (saves != 0) *B(o + 4) = 0x31;
            else {
                int32_t k = S8(o + 10);
                ARG(1) = (uint32_t)k;
                *B(o + 4) = (k == 0 || k == 4) ? 0x18 : 0x41;
            }
        } else if (r == 0xFFFFFF05u || r == NO_DATA) {
            ARG(1) = NO_DATA;
            c->r[6] = (uint32_t)S8(o + 8);
            int32_t k = S8(o + 10);
            ARG(1) = (uint32_t)k;
            if (c->r[6] != 0) {
                ARG(0) = (uint32_t)S8(o + 3);
                c->r[31] = 0x08817E10u; f_08816B74(c);
                ARG(0) = 0xA;
                *B(o + 2) = 0xA;
            } else {
                c->r[6] = 4;
                *B(o + 9) = 0;
                c->r[7] = 5;
                *B(o + 2) = 4;
                c->r[6] = (uint32_t)k < 5;
                *B(o + 5) = 5;
                if ((uint32_t)k < 5) {
                    if (k == 1) *B(o + 4) = r != NO_DATA ? 0x2F : 0x51;
                    else if (k == 2 || k == 3) *B(o + 4) = 0x2E;
                    else *B(o + 4) = r != NO_DATA ? 0x2D : 6;
                }
            }
        } else {
            ARG(0) = 6; ARG(1) = 0x13;
            *B(o + 2) = 6;
            *B(o + 4) = 0x13;
            *B(o + 9) = 0;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (o): the wait for the card's free space. While the dialog is up, nothing. An error: step 6,
   message 0x13. Enough space for what +10 needs: step 2, message 0x19, cursor -1. Too little:
   step 4 with 4 at +5 and the message 0x33 (none when +10 is set). A 0x20 frame keeping
   s0..s2. */
SFA_FUNC(08817EC4, save_screen_wait_space) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = o;
    c->r[31] = 0x08817EE0u; f_089E32CC(c);
    c->r[18] = 0xFFFFFFFFu;
    if (c->r[2] != 0xFFFFFFFFu) {
        c->r[31] = 0x08817EF4u; f_089E358C(c);
        int32_t space = (int32_t)c->r[2];
        c->r[17] = (uint32_t)space;
        if (space < 0) {
            ARG(0) = 6; ARG(1) = 0x13;
            *B(o + 2) = 6;
            *B(o + 4) = 0x13;
            *B(o + 9) = 0;
        } else {
            ARG(0) = (uint32_t)S8(o + 3);
            ARG(1) = (uint32_t)S8(o + 10);
            c->r[31] = 0x08817F0Cu; f_08816B90(c);
            if (space < (int32_t)c->r[2]) {
                *B(o + 9) = 0;
                ARG(0) = 4;
                ARG(1) = (uint32_t)S8(o + 10);
                *B(o + 2) = 4;
                if (ARG(1) != 0) *B(o + 4) = 0;
                else { ARG(1) = 0x33; *B(o + 4) = 0x33; }
                *B(o + 5) = 4;
            } else {
                ARG(0) = 0x19; ARG(1) = 2;
                *B(o + 4) = 0x19;
                *B(o + 2) = 2;
                *B(o + 9) = 0xFF;
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* All the drawing sent so far is waited for. A 0x20 frame. */
SFA_FUNC(089E48D8, draw_sync_all) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    ARG(0) = 0;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 0;
    c->r[31] = 0x089E48ECu; f_089E7AE4(c);
    c->r[31] = ra; c->r[29] += 0x20;
}
