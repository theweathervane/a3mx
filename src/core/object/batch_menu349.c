/* The link's menu of three questions, a step at a time. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define NETP 0x09523980u

void f_0880A6CC(Cpu *c); void f_088DCD40(Cpu *c); void f_088E4960(Cpu *c); void f_088DD8F4(Cpu *c);
void f_088DC0E0(Cpu *c); void f_08823338(Cpu *c); void f_088DD5D0(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* A question of `n` answers on the menu; its answer: below 0 is going back, above 0 a choice. */
static int32_t ask(Cpu *c, gaddr p, uint32_t n, uint32_t back) {
    ARG(2) = 3;
    call2(c, p, n, f_088DD5D0, back);
    return (int32_t)c->r[2];
}
/* The menu is left for the screen at `next`, with its counters cleared. */
static void leave_for(gaddr p, gaddr next) {
    *H(p + 24) = 0;
    *W(p + 20) = next;
    *H(p + 2) = 0;
    *H(p) = 0;
    *B(p + 47) = 0;
}

/* (menu): one frame of the link's menu. A line of text first: the one at 0x08A01054 in the first
   three steps, afterwards the one the choices so far (+42 and +43) name in the table 0x08A3EA04.
   Then the step (+0):
   0 the menu is set up (0x088DCD40, 0x088E4960, 0x088DD8F4, 0x088DC0E0, six of 0x08823338);
   1 waits for GAME's +93 to clear;
   2 the first question, of four (going back counts as the last): the first two answers are kept
     at +42, the next two also set +43 to 5 and +44, skipping a step, and the last leaves for the
     screen at 0x08A3EB00;
   3 the second, of three: going back returns to the first with its answer; the answers give
     +43 as 0 (skipping two steps), 2 or 4;
   4 the third, of two: going back returns to the question it came from; the first answer goes on
     unless NETP's +43 is 2 or 4, the second skips a step with +44 at 2;
   5 and 6 move on with +47 cleared; 7 counts +24 on and starts over.
   Past 7 the menu leaves for the screen at 0x08A3EB14. Each choice moves the cursor's sound
   position (+50) on by +55. A 0x30 frame keeping s0..s2. */
SFA_FUNC(088D8458, link_menu_frame) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    uint32_t first = *H(p);
    *W(sp + 36) = s1; *W(sp + 40) = s2;
    c->r[17] = 2; c->r[18] = 4;
    *W(sp + 32) = s0; *W(sp + 44) = ra;
    c->r[16] = p;
    ARG(3) = 4; c->r[8] = 9;
    if ((int32_t)first < 3) {
        ARG(2) = 0x78;
        call2(c, 0x08A01054u, 2, f_0880A6CC, 0x088D84DCu);
    } else {
        ARG(2) = 0x30;
        call2(c, *W(0x08A3EA04u + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2)), 2, f_0880A6CC, 0x088D84B8u);
    }
    uint32_t step = *H(p);
    if (!(step < 8)) leave_for(p, 0x08A3EB14u);
    else if (step == 0) {
        *B(p + 43) = 0;
        *B(p + 46) = *B(p + 42);
        *B(p + 44) = 0;
        *B(p + 45) = 0;
        *B(p + 37) = 0;
        *B(p + 47) = 1;
        ARG(0) = 1;
        call0(c, f_088DCD40, 0x088D8528u);
        call0(c, f_088E4960, 0x088D8530u);
        call0(c, f_088DD8F4, 0x088D8538u);
        call0(c, f_088DC0E0, 0x088D8540u);
        for (uint32_t k = 0; k < 6; k++) call2(c, 0x20, k, f_08823338, 0x088D854Cu + 12 * k);
        *H(p) = (uint16_t)(*H(p) + 1);
    } else if (step == 1) {
        if (S8(GAME + 93) == 0) *H(p) = (uint16_t)(step + 1);
    } else if (step == 2) {
        int32_t r = ask(c, p, 4, 0x088D85C4u);
        if (r < 0) { *B(p + 46) = 4; r = 1; }
        if (r > 0) {
            *H(p) = (uint16_t)(*H(p) + 1);
            int32_t sel = S8(p + 46);
            uint32_t pos = (uint32_t)(int32_t)S16(p + 50) + *B(p + 55);
            if (sel == 0 || sel == 1) {
                *B(p + 42) = (uint8_t)sel;
                *B(p + 43) = 0;
                *B(p + 44) = 0;
            } else if (sel == 2 || sel == 3) {
                *B(p + 42) = (uint8_t)sel;
                *B(p + 43) = 5;
                *B(p + 44) = 1;
                *H(p) = (uint16_t)(*H(p) + 1);
            } else {
                leave_for(p, 0x08A3EB00u);
                *H(p + 24) = (uint16_t)(*H(p + 24) + 1);
            }
            *H(p + 50) = (uint16_t)pos;
            *B(p + 46) = 0;
        }
    } else if (step == 3) {
        int32_t r = ask(c, p, 3, 0x088D86D8u);
        if (r < 0) {
            *B(p + 46) = *B(p + 42);
            *B(p + 42) = 0;
            *H(p) = (uint16_t)(*H(p) - 1);
        } else if (r > 0) {
            int32_t d = S8(p + 46) * 2;
            uint32_t pos = (uint32_t)(int32_t)S16(p + 50) + *B(p + 55);
            uint32_t at = *H(p);
            if (d == 4 || d == 2) {
                *B(p + 43) = (uint8_t)d;
                *B(p + 44) = 1;
            } else if (d == 0) {
                *B(p + 43) = 0;
                *B(p + 44) = 0;
                *H(p) = (uint16_t)(at + 2);
                at = *H(p);
            }
            *H(p + 50) = (uint16_t)pos;
            *B(p + 46) = 0;
            *H(p) = (uint16_t)(at + 1);
        }
    } else if (step == 4) {
        int32_t r = ask(c, p, 2, 0x088D8790u);
        if (r < 0) {
            uint32_t v = *B(p + 43);
            uint32_t at = *H(p);
            if (v == 5) {
                *B(p + 46) = *B(p + 42);
                *B(p + 42) = 0;
                *H(p) = (uint16_t)(at - 2);
            } else {
                *B(p + 46) = (uint8_t)(v >> 1);
                *B(p + 43) = 0;
                *H(p) = (uint16_t)(at - 1);
            }
        } else if (r > 0) {
            int32_t sel = S8(p + 46) + 1;
            uint32_t at = (uint32_t)*H(p) + 1;
            uint32_t pos = (uint32_t)(int32_t)S16(p + 50) + *B(p + 55);
            if (sel == 1) {
                *B(p + 44) = 1;
                uint32_t m = *B(NETP + 43);
                if (m != 2 && m != 4) {
                    *H(p) = (uint16_t)at;
                    at = (uint32_t)*H(p) + 1;
                }
            } else if (sel == 2) {
                *H(p) = (uint16_t)at;
                at = (uint32_t)*H(p) + 1;
                *B(p + 44) = 2;
            }
            *H(p + 50) = (uint16_t)pos;
            *B(p + 46) = 0;
            *H(p) = (uint16_t)at;
        }
    } else if (step == 5) {
        *H(p) = (uint16_t)(step + 1);
        uint32_t at = (uint32_t)*H(p) + 1;
        *B(p + 47) = 0;
        *H(p) = (uint16_t)at;
    } else if (step == 6) {
        *B(p + 47) = 0;
        *H(p) = (uint16_t)(step + 1);
    } else {
        *H(p + 2) = 0;
        *H(p + 24) = (uint16_t)(*H(p + 24) + 1);
        *H(p) = 0;
        *B(p + 47) = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}
