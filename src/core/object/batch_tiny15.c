/* Small functions the scenarios reach, 14 - 26 instructions: step dispatchers of some characters'
   effects, a fighter object's start, the pads and a counter block reset. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08804ECC(Cpu *c); void f_0896F97C(Cpu *c); void f_0896F9BC(Cpu *c); void f_08814320(Cpu *c);
void f_088325DC(Cpu *c); void f_0893D900(Cpu *c); void f_08821424(Cpu *c); void f_089885CC(Cpu *c);
void f_08822F54(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): the step in `table` (by the byte at +`by`), called as step (object) with a1 = the step. */
static void step_from(Cpu *c, gaddr o, gaddr table, uint32_t by, uint32_t back) {
    gaddr h = *W(table + ((uint32_t)*B(o + by) << 2));
    ARG(0) = o; ARG(1) = h;
    c->r[31] = back;
    rt_call_indirect(c, h);
}

/* The pads reset (0x08804ECC), then 0x09523920 / 24 / 28 / 2C and 0x0952394E cleared. */
SFA_FUNC(088E2304, pads_and_counters_reset) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x088E2314u; f_08804ECC(c);
    *W(0x09523920u) = 0; *W(0x09523920u + 12) = 0; *W(0x09523920u + 8) = 0; *W(0x09523920u + 4) = 0;
    *B(0x09523920u + 46) = 0;
    c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (object): 0x0896F97C while +6 is clear, else 0x0896F9BC. */
SFA_FUNC(0896F940, effect_by_flag) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 6) == 0) { c->r[31] = 0x0896F95Cu; f_0896F97C(c); }
    else { c->r[31] = 0x0896F970u; f_0896F9BC(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (a, b): 0x08E75C44 = 0xFF, the select portraits freed (0x08814320), then 0x088325DC (a, b). */
SFA_FUNC(089DE244, portraits_free_then) {
    uint32_t a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = b; c->r[17] = a;
    *W(c->r[29] + 24) = ra;
    *H(0x08E75C44u) = 0xFF;
    ARG(1) = 0xFF;
    c->r[31] = 0x089DE26Cu; f_08814320(c);
    call2(c, a, b, 0x089DE278u, f_088325DC);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): on (+4 = 1, +6 = 7; +5 / +7 / +236 and +0x3BC cleared), +11 / +78 = +697, then 0x0893D900. */
SFA_FUNC(0893D7C0, object_start_697) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = 1; *B(o + 5) = 0;
    *B(o + 11) = (uint8_t)S8(o + 697);
    *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0;
    *B(o + 78) = *B(o + 11);
    *W(c->r[29] + 16) = ra;
    *B(o + 0x3B8 + 4) = 0;
    ARG(1) = 7; ARG(2) = o + 0x3B8;
    c->r[31] = 0x0893D800u; f_0893D900(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its step by +6, from 0x08A70640 when its fighter (+92) is character 0x27, else 0x08A70630. */
SFA_FUNC(089338D0, effect_step_char27) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(*W(o + 92) + 338) == 0x27) step_from(c, o, 0x08A70640u, 6, 0x08933908u);
    else step_from(c, o, 0x08A70630u, 6, 0x0893392Cu);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x089885CC unless its kind (+3) is its owner's (+88) character; then its step by +4
   (0x08A94840) and drawn (0x08821424). */
SFA_FUNC(08988934, effect_step_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if ((uint32_t)*B(o + 3) != (uint32_t)(int32_t)S8(*W(o + 88) + 338)) call_on(c, o, 0x0898895Cu, f_089885CC);
    else {
        step_from(c, o, 0x08A94840u, 4, 0x08988984u);
        call_on(c, o, 0x0898898Cu, f_08821424);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its step by +7, from 0x08AAA000 when its fighter (+92) is character 0xE, else 0x08AA9FF4. */
SFA_FUNC(089B8454, effect_step_char0e) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(*W(o + 92) + 338) == 0xE) step_from(c, o, 0x08AAA000u, 7, 0x089B848Cu);
    else step_from(c, o, 0x08AA9FF4u, 7, 0x089B84B0u);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, kind): a list-0x2F object (0x08822F54) of this kind for the owner: +3 = kind, +88 = owner,
   +139 / +14 the owner's, owner +44 = it, +0 = 1. */
SFA_FUNC(089BE80C, effect_make_2f) {
    gaddr o = ARG(0);
    uint32_t kind = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = o; c->r[17] = kind;
    *W(c->r[29] + 24) = ra;
    call2(c, 0x2F, 0, 0x089BE834u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 3) = (uint8_t)kind;
        *W(n + 88) = o;
        *B(n + 139) = (uint8_t)S8(o + 139);
        *B(n + 14) = *B(o + 14);
        *W(o + 44) = n;
        *B(n) = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
