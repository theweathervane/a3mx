/* Step dispatchers: the commonest small function in the game.
 *
 * Objects, fighters, effects, tasks and screens all keep their progress in a few "step" numbers (a
 * routine number, then steps inside it, usually the bytes at +4 to +7), and each level is run by a
 * function that does nothing but pick a handler from a table by that number and call it with the same
 * arguments it was given. The program has over a thousand of them; steps.c lists the ones the tests
 * run.
 */
#pragma once
#include "../decomp.h"

void rt_call_indirect(Cpu *c, uint32_t addr);

/* Calls table[step] with the caller's arguments untouched. The original keeps a 0x20-byte frame with
   its return address at +16 and leaves the handler and the table in two neighbouring registers
   (`reg`, `reg` + 1: a1 / a2 or a2 / a3), which handlers written with more parameters than they are
   given go on to read; all of that is kept. `back` is where the original's call returns to. */
static inline void run_step(Cpu *c, uint32_t step, gaddr table, int reg, uint32_t back) {
    gaddr handler = *(uint32_t *)guest_ptr(table + step * 4u);
    c->r[29] -= 0x20;
    *(uint32_t *)guest_ptr(c->r[29] + 16) = c->r[31];
    c->r[reg] = handler; c->r[reg + 1] = table;
    c->r[31] = back;
    rt_call_indirect(c, handler);
    c->r[31] = *(uint32_t *)guest_ptr(c->r[29] + 16);
    c->r[29] += 0x20;
}

/* The same with the object's owner (a pointer the object holds, usually at +88) as the handler's second
   argument; the original leaves it in a2 as well, and the handler in a3. */
static inline void run_step_with(Cpu *c, uint32_t step, gaddr table, uint32_t owner, uint32_t back) {
    gaddr handler = *(uint32_t *)guest_ptr(table + step * 4u);
    c->r[29] -= 0x20;
    *(uint32_t *)guest_ptr(c->r[29] + 16) = c->r[31];
    c->r[5] = owner; c->r[6] = owner; c->r[7] = handler;
    c->r[31] = back;
    rt_call_indirect(c, handler);
    c->r[31] = *(uint32_t *)guest_ptr(c->r[29] + 16);
    c->r[29] += 0x20;
}

/* A step, then one or two fixed routines on the same object (its animation, its drawing): the shape of
   an object's whole frame. The original keeps the object in s0 across the calls, with s0 and its
   return address saved at +16 and +20 of the frame; `back` is where the handler's call returns to, and
   each later call returns 8 further on. */
static inline void run_step_then(Cpu *c, gaddr obj, uint32_t step, gaddr table, uint32_t back,
                                 GuestFunc then, GuestFunc last) {
    gaddr handler = *(uint32_t *)guest_ptr(table + step * 4u);
    c->r[29] -= 0x20;
    *(uint32_t *)guest_ptr(c->r[29] + 16) = c->r[16];
    *(uint32_t *)guest_ptr(c->r[29] + 20) = c->r[31];
    c->r[16] = obj;
    c->r[4] = obj; c->r[5] = handler;
    c->r[31] = back;
    rt_call_indirect(c, handler);
    c->r[4] = c->r[16]; c->r[31] = back + 8;
    then(c);
    if (last) {
        c->r[4] = c->r[16]; c->r[31] = back + 16;
        last(c);
    }
    c->r[16] = *(uint32_t *)guest_ptr(c->r[29] + 16);
    c->r[31] = *(uint32_t *)guest_ptr(c->r[29] + 20);
    c->r[29] += 0x20;
}
