/* Three more: the heap's growing, and two objects' starts that pick the link play's course.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08804660(Cpu *c); void f_08828C40(Cpu *c); void f_088386B0(Cpu *c); void f_08837B5C(Cpu *c);
void f_088D42BC(Cpu *c);

/* (n): n more bytes of the heap: the place they start at, or -1 when the heap (its start at
   0x08AD8FC4, next free place at 0x08AD8FC0, end at 0x08AD8FC8) does not reach. The first call
   has the system's block asked for by the routine's other part: of the size the word at
   0x08A02DE0 gives in units of 1024 bytes, or 0x10000 when that is 0. A 0x10 frame keeping
   s0..s2. */
SFA_FUNC(08804618, heap_grow) {
    uint32_t n = ARG(0);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->r[16];
    c->r[16] = 0x08AE0000u;
    c->r[3] = *W(0x08AD8FC4u);
    *W(sp + 8) = c->r[18];
    c->r[18] = n;
    *W(sp + 12) = c->r[31];
    *W(sp + 4) = c->r[17];
    if (c->r[3] == 0) {
        c->r[3] = 0x08A00000u;
        ARG(0) = 0x08A02DE0u;
        c->r[2] = *W(0x08A02DE0u);
        c->r[17] = c->r[2] << 10;
        if (c->r[2] == 0) c->r[17] = 0x10000;
        if (c->r[17] != 0) {
            c->r[2] = 0x08A00000u;
            f_08804660(c);
            return;
        }
    }
    uint32_t start = *W(0x08AD8FC4u);
    c->r[2] = 0xFFFFFFFFu;
    if (start != 0) {
        c->r[6] = 0x08AE0000u;
        uint32_t next = *W(0x08AD8FC0u);
        ARG(1) = next;
        uint32_t after = next + n;
        ARG(0) = after;
        c->r[8] = after < start;
        if (after >= start) {
            c->r[11] = 0x08AE0000u;
            c->r[10] = *W(0x08AD8FC8u);
            c->r[9] = c->r[10] < after;
            if (c->r[9] == 0) {
                c->r[2] = next;
                *W(0x08AD8FC0u) = after;
            }
        }
    }
    c->r[31] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[29] += 0x10;
}

/* (o, a): an object's first step sets it up and picks the link play's course; its later steps go
   to `rest`. A 0x20 frame keeping s0, s1. */
static void start_or_rest(Cpu *c, void (*rest)(Cpu *), uint32_t back_setup, uint32_t back_pick, uint32_t back_rest) {
    gaddr o = ARG(0);
    uint32_t a = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[6] = *H(o);
    *W(sp + 20) = s1;
    c->r[17] = o;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = a;
    if (c->r[6] == 0) {
        ARG(0) = o; ARG(1) = a; c->r[31] = back_setup; f_08828C40(c);
        ARG(0) = o; ARG(1) = a; c->r[31] = back_pick; f_088386B0(c);
    } else {
        ARG(0) = o; ARG(1) = a; c->r[31] = back_rest; rest(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

SFA_FUNC(08837AFC, course_object_step_a) {
    start_or_rest(c, f_08837B5C, 0x08837B28u, 0x08837B34u, 0x08837B48u);
}

SFA_FUNC(088D425C, course_object_step_b) {
    start_or_rest(c, f_088D42BC, 0x088D4288u, 0x088D4294u, 0x088D42A8u);
}
