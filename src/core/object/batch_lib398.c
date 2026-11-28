/* Two of the library's own: memory asked for, and a routine noted to be run when the program
   ends. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define REENT 0x08AD84DCu /* points at the library's state: +328 the newest block of routines to
                             run at the end (+0 the block before, +4 how many, +8.. the routines),
                             the first block inside the state at +0x14C */

void f_089F6CDC(Cpu *c);

/* (n): n bytes of memory, from the library's state. A 0x20 frame. */
SFA_FUNC(089F339C, lib_malloc) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    ARG(1) = ARG(0);
    *W(c->r[29] + 16) = ra;
    ARG(0) = *W(REENT);
    c->r[31] = 0x089F33B4u; f_089F6CDC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (routine): the routine is noted to be run at the program's end: 0, or -1 when a new block of
   32 was needed and could not be had. A 0x20 frame. */
SFA_FUNC(089F3224, lib_atexit) {
    uint32_t routine = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[7] = 0x08AE0000u;
    gaddr state = *W(REENT);
    gaddr block = *W(state + 328);
    *W(sp + 20) = ra;
    if (block == 0) {
        block = state + 0x14C;
        *W(state + 328) = block;
    }
    ARG(1) = block;
    c->r[6] = block + 4;
    if ((int32_t)*W(block + 4) >= 32) {
        *W(sp + 16) = routine;
        ARG(0) = 0x88; c->r[31] = 0x089F3264u; f_089F339C(c);
        block = c->r[2];
        ARG(1) = block;
        ARG(0) = *W(sp + 16);
        c->r[7] = 0x08AE0000u;
        if (block == 0) {
            c->r[2] = 0xFFFFFFFFu;
            c->r[31] = ra; c->r[29] += 0x20;
            return;
        }
        *W(block + 4) = 0;
        *W(block) = *W(*W(REENT) + 328);
        *W(*W(REENT) + 328) = block;
        c->r[6] = block + 4;
    }
    uint32_t n = *W(block + 4);
    c->r[7] = n + 1;
    c->r[8] = n << 2;
    *W(block + 4) = n + 1;
    ARG(1) = block + (n << 2);
    *W(block + (n << 2) + 8) = routine;
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}
