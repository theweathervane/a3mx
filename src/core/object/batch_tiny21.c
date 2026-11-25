/* Small functions the scenarios reach, 13 - 29 instructions: effects made, followed and ended, a
   slowing move, a menu overlay's step, the smallest value over a list. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_089CC74C(Cpu *c); void f_089CC7A4(Cpu *c); void f_089CC810(Cpu *c); void f_08881F94(Cpu *c);
void f_08822F54(Cpu *c); void f_088681F8(Cpu *c); void f_08868208(Cpu *c); void f_08868214(Cpu *c);
void f_088688EC(Cpu *c); void f_088B118C(Cpu *c); void f_088BDB9C(Cpu *c); void f_088DD3DC(Cpu *c);
void f_088DD418(Cpu *c); void f_088E1074(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

/* (owner): 0x089CC810, 0x089CC74C and a list-0x37 effect (0x089CC7A4). */
SFA_FUNC(089CC718, effects_make_three) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call_on(c, o, 0x089CC72Cu, f_089CC810);
    call_on(c, o, 0x089CC734u, f_089CC74C);
    call_on(c, o, 0x089CC73Cu, f_089CC7A4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): x (+16) by its step (+96), the step slowed by +104 until it would turn (then both cleared);
   while still moving 0x08881F94 every 8th frame (GAME +59). */
SFA_FUNC(0898B0F0, object_slide_slowing) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t v = *W(o + 96);
    if (v != 0) {
        *W(o + 16) = *W(o + 16) + v;
        uint32_t slow = *W(o + 104);
        v += slow;
        *W(o + 96) = v;
        if ((int32_t)(v ^ slow) >= 0) { *W(o + 104) = 0; *W(o + 96) = 0; v = 0; }
        if (v != 0 && (S8(GAME + 59) & 7) == 0) { c->r[31] = 0x0898B154u; f_08881F94(c); }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, kind): a list-0x35 effect (0x08822F54) for the owner: +88 = owner, +139 / +14 its, +24 = 2,
   +3 = kind, +0 = 1; the owner's +44 = it. */
SFA_FUNC(089CB1E8, effect_make_35) {
    gaddr o = ARG(0);
    uint32_t kind = ARG(1) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    c->r[17] = kind;
    *W(c->r[29] + 24) = ra;
    call2(c, 0x35, 0, 0x089CB20Cu, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = o;
        *B(n + 139) = (uint8_t)S8(o + 139);
        *H(n + 24) = 2;
        *B(n + 3) = (uint8_t)kind;
        *B(n) = 1;
        *W(o + 44) = n;
        *B(n + 14) = *B(o + 14);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): ends (+4 on, +5 cleared; its owner's +536 count down and +228 let go) unless the owner
   (+88) is still in its move (+236 = 9, step 1 / 0 / 8). */
SFA_FUNC(089CC4D4, effect_end_with_owner) {
    gaddr o = ARG(0);
    gaddr p = *W(o + 88);
    if (S8(p + 236) == 9 && *B(p + 4) == 1 && *B(p + 5) == 0 && *B(p + 6) == 8) return;
    *B(o + 5) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(p + 536) = (uint8_t)(*B(p + 536) - 1);
    if (*W(p + 228) == o) *W(p + 228) = 0;
}

/* (-, timer): +4 counted down; at 0 0x088681F8, else 0x08868214 when 0x088688EC says so, else
   0x08868208. */
SFA_FUNC(08868878, timer_step) {
    gaddr t = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    uint8_t left = (uint8_t)(*B(t + 4) - 1);
    *B(t + 4) = left;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = t;
    if (left == 0) call_on(c, t, 0x088688C4u, f_088681F8);
    else {
        call0(c, 0x088688A4u, f_088688EC);
        if (c->r[2] != 0) call_on(c, t, 0x088688B4u, f_08868214);
        else call_on(c, t, 0x088688DCu, f_08868208);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (overlay): its step by +5 (0x08A2FBB8), +100 cleared (set first while GAME +16's high byte is 0),
   then 0x088B118C. */
SFA_FUNC(088B0C20, menu_overlay_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr h = *W(0x08A2FBB8u + ((uint32_t)*B(o + 5) << 2));
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; ARG(1) = h;
    c->r[31] = 0x088B0C50u; rt_call_indirect(c, h);
    if ((((int32_t)S16(GAME + 16) & 0xFF00) >> 8) == 0) *W(o + 100) = 1;
    if (*W(o + 100) != 0) *W(o + 100) = 0;
    call_on(c, o, 0x088B0C84u, f_088B118C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): its step by +128 (0x08A3AFD8) while GAME +4 is set and its parent (+124) points back at it
   (+92) and is on its way (+4); with no parent 0x088BDB9C. */
SFA_FUNC(088BDAB8, child_object_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr p = *W(o + 124);
    ARG(1) = p;
    if (*H(GAME + 4) == 0 || *W(p + 92) != o) { c->r[31] = 0x088BDB20u; f_088BDB9C(c); }
    else if (*B(p + 4) != 0) {
        gaddr h = *W(0x08A3AFD8u + ((uint32_t)*B(o + 128) << 2));
        ARG(1) = h;
        c->r[31] = 0x088BDB0Cu; rt_call_indirect(c, h);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The smallest of 0x088E1074 (entry + 0xAC, 0) over the list walked by 0x088DD3DC / 0x088DD418 (a
   negative value counts as none yet); 0x81001000 for an empty list. */
SFA_FUNC(088E114C, list_smallest) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s1;
    c->r[17] = 0x81001000u;
    *W(sp + 20) = s0; *W(sp + 28) = ra;
    call0(c, 0x088E1168u, f_088DD3DC);
    *W(sp + 16) = c->r[2];
    c->r[16] = sp + 0x10;
    for (;;) {
        call_on(c, sp + 0x10, 0x088E1178u, f_088DD418);
        gaddr e = c->r[2];
        if (e == 0) break;
        call2(c, e + 0xAC, 0, 0x088E118Cu, f_088E1074);
        int32_t v = (int32_t)c->r[2];
        if ((int32_t)c->r[17] < 0 || v < (int32_t)c->r[17]) c->r[17] = (uint32_t)v;
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
