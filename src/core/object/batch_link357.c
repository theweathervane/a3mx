/* The link's request to another player, a step at a time. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define PADS 0x08E75BA0u /* three pads of 44 bytes; +12 is what was newly pressed */

void f_088E3ED8(Cpu *c); void f_088DCB00(Cpu *c); void f_088DF208(Cpu *c); void f_088D5680(Cpu *c);
void f_088DF188(Cpu *c); void f_0887C4CC(Cpu *c); void f_088DCE6C(Cpu *c); void f_088E3FD0(Cpu *c);
void f_0880E788(Cpu *c); void f_088DDC30(Cpu *c); void f_088E3F98(Cpu *c); void f_088E3F44(Cpu *c);
void f_088DC7E8(Cpu *c); void f_088DCD40(Cpu *c); void f_0880A6CC(Cpu *c); void f_088D68C8(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static uint32_t pressed(void) { return *W(PADS + 12) | *W(PADS + 56) | *W(PADS + 100); }
static void step_on(gaddr p) { *H(p) = (uint16_t)(*H(p) + 1); }
static void row_to(gaddr p, int by) {
    *H(p + 2) = 0;
    *H(p + 24) = (uint16_t)(*H(p + 24) + by);
    *H(p) = 0;
    *B(p + 47) = 0;
}

/* The request while it stands, by the other player's state (+144), once anyone else's request has
   been called off: gone (+152 clear) or state 4 ends it (the step 0x14 with 0x13 at the menu's
   +16); 0 and 1 fail with 0x81001003; 2 waits, and 3 too until the button 0x80 asks again
   (0x088DF188, giving up after four failures; the sound 0x39), the wait (+6) ending in the sound
   0x2F and the step 0x14, as the button 0x100 does; 5 accepts (0x088DCE6C, the other's +154 at 2)
   and moves on for the kinds 0 to 2 of +44. */
static void request_standing(Cpu *c, gaddr p, gaddr other) {
    call0(c, f_088DCB00, 0x088D9878u);
    gaddr who = c->r[2];
    c->r[18] = who;
    if (who != 0 && who != other) {
        call1(c, who + 4, f_088DF208, 0x088D9894u);
        *B(who + 154) = 0;
    }
    if (S16(other + 152) == 0) { *H(p) = 0x14; *W(p + 16) = 0x13; return; }
    uint32_t st = *W(other + 144);
    int32_t wait = 0;
    int waiting = 0;
    if (!(st < 6)) return;
    if (st == 0) { call2(c, p, 0x81001003u, f_088D5680, 0x088D9A7Cu); return; }
    if (st == 1) { call2(c, p, 0x81001003u, f_088D5680, 0x088D98FCu); return; }
    if (st == 4) { *H(p) = 0x14; *W(p + 16) = 0x13; return; }
    if (st == 5) {
        call1(c, other + 4, f_088DCE6C, 0x088D9A04u);
        *B(other + 154) = 2;
        if (*B(p + 44) < 3) step_on(p);
        return;
    }
    if (st == 2) { wait = S16(p + 6); waiting = 1; }
    else {
        c->r[18] = PADS;
        if (!(pressed() & 0x80) && !((int32_t)*H(p + 2) > 0)) { wait = S16(p + 6); waiting = 1; }
        else {
            call1(c, other + 4, f_088DF188, 0x088D9940u);
            if ((int32_t)c->r[2] < 0) {
                uint32_t n = (uint32_t)*H(p + 2) + 1;
                *H(p + 2) = (uint16_t)n;
                if (!((int32_t)(n & 0xFFFF) < 4)) { *H(p) = 0x14; *H(p + 2) = 0; }
            }
            if (pressed() & 0x80) call3(c, 0, 0x39, 0xC0, f_0887C4CC, 0x088D9998u);
        }
    }
    if (waiting) {
        *H(p + 4) = 0;
        if (wait > 0 && !(pressed() & 0x100)) *H(p + 6) = (uint16_t)(wait - 1);
        else {
            call3(c, 0, 0x2F, 0xC0, f_0887C4CC, 0x088D99ECu);
            *B(p + 32) = 0;
            *H(p) = 0x14;
        }
    }
}

/* (menu): a request to the player at +28. Steps 0 and 1 set it up (waits of 0x168 at +4 and +6,
   5 at +16, 0x088E3ED8 with 4 when +32 is set and 7 when not) and go straight on to 2, the
   request standing, with a box and the other's name while 0x088E3FD0 agrees; 3 waits for
   0x088E3F44 and goes back a row for the kind 1 of +44; 4 ends the row. The steps from 0x14 call
   it off: 0x14 cancels (and takes the other out of the list for the kind 1), 0x15 waits 0xB4
   frames or, in the last 150, for the button 0x100; 0x16 waits for 0x088E3F44; 0x17 goes back a
   row. Any other step leaves for the screen at 0x08A3EB14. The choices' line of text and the
   link's own line are drawn every frame. A 0x30 frame keeping s0..s2. */
SFA_FUNC(088D97D4, request_steps) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0;
    c->r[16] = p;
    uint32_t step = *H(p);
    *W(sp + 36) = s1; *W(sp + 40) = s2; *W(sp + 44) = ra;
    gaddr other = *W(p + 28);
    c->r[17] = other;
    if (step <= 2) {
        if (step != 2) {
            uint32_t next = step + 1;
            if (step == 0) {
                *H(p) = (uint16_t)(step + 1);
                *H(p + 4) = 0x168;
                *H(p + 6) = 0x168;
                next = (uint32_t)*H(p) + 1;
            }
            *H(p) = (uint16_t)next;
            *H(p + 2) = 0;
            *W(p + 16) = 5;
            if (*B(p + 32) != 0) call1(c, 4, f_088E3ED8, 0x088D9860u);
            else call1(c, 7, f_088E3ED8, 0x088D9870u);
        }
        request_standing(c, p, other);
        call0(c, f_088E3FD0, 0x088D9A84u);
        if (c->r[2] != 0) {
            ARG(3) = 0x92; c->r[8] = 0xA0800080u; c->r[9] = 0x13;
            call3(c, 0x6D, 0x7C, 0x113, f_0880E788, 0x088D9AA8u);
            ARG(3) = 0; c->r[8] = 0x13;
            call3(c, other + 4, 0x70, 0x84, f_088DDC30, 0x088D9AC0u);
            call1(c, 1, f_088E3F98, 0x088D9AC8u);
        }
    } else if (step == 3) {
        call0(c, f_088E3F44, 0x088D9AD8u);
        if (c->r[2] != 0) {
            if (*B(p + 44) == 1) row_to(p, -1);
            else step_on(p);
        }
    } else if (step == 4) row_to(p, 1);
    else if (step == 0x14) {
        call1(c, other + 4, f_088DF208, 0x088D9B3Cu);
        *B(other + 154) = 0;
        uint32_t kind = *B(p + 44);
        *W(p + 28) = 0;
        if (kind == 1) call1(c, other, f_088DC7E8, 0x088D9B58u);
        call0(c, f_088DCD40, 0x088D9B60u);
        *H(p + 4) = 0xB4;
        if (*B(p + 32) != 0) call1(c, *W(p + 16), f_088E3ED8, 0x088D9B78u);
        else step_on(p);
        uint32_t next = (uint32_t)*H(p) + 1;
        *W(p + 16) = 0;
        *H(p) = (uint16_t)next;
    } else if (step == 0x15) {
        call1(c, 1, f_088E3F98, 0x088D9BACu);
        int32_t t = S16(p + 4) - 1;
        *H(p + 4) = (uint16_t)t;
        if ((int16_t)t == 0) step_on(p);
        else if (S16(p + 4) < 150 && (pressed() & 0x100)) {
            call3(c, 0, 0x2F, 0xC0, f_0887C4CC, 0x088D9C14u);
            step_on(p);
        }
    } else if (step == 0x16) {
        call0(c, f_088E3F44, 0x088D9C2Cu);
        if (c->r[2] != 0) step_on(p);
    } else if (step == 0x17) row_to(p, -1);
    else {
        *W(p + 20) = 0x08A3EB14u;
        *H(p + 24) = 0; *H(p + 2) = 0; *H(p) = 0;
        *B(p + 47) = 0;
    }
    ARG(3) = 4; c->r[8] = 9;
    call3(c, *W(0x08A3EA04u + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2)), 2, 0x30, f_0880A6CC, 0x088D9CB0u);
    call1(c, p, f_088D68C8, 0x088D9CB8u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}
