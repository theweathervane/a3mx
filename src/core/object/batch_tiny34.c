/* Small functions the scenarios reach, 17 - 35 instructions: effects placed by their timer or owner,
   fighter objects' starts, an effect tied to its owner's place. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887E664(Cpu *c); void f_088219B0(Cpu *c); void f_0887FBE8(Cpu *c); void f_0888038C(Cpu *c);
void f_088804E0(Cpu *c); void f_08918D20(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DBE8(Cpu *c);
void f_0882324C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (owner): two list-5 effects (0x0887E664 (owner, 0, 1, 0x1E) and (owner, 0, 2, 0x1F)). */
SFA_FUNC(0887E560, effects_make_two_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    ARG(0) = o; ARG(1) = 0; ARG(2) = 1; ARG(3) = 0x1E;
    c->r[31] = 0x0887E580u; f_0887E664(c);
    ARG(0) = o; ARG(1) = 0; ARG(2) = 2; ARG(3) = 0x1F;
    c->r[31] = 0x0887E594u; f_0887E664(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +5 on, +82 = 5, placed by its timer (0x0887FBE8), 0x088219B0. */
SFA_FUNC(0887F75C, effect_timer_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *B(o + 0x52) = 5;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x0887F78Cu, f_0887FBE8);
    call_on(c, o, 0x0887F794u, f_088219B0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4, +5 cleared) once +75 goes negative; until then placed by its timer and 0x088219B0. */
SFA_FUNC(0887F894, effect_timer_run) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (S8(o + 75) < 0) { *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1); }
    else {
        call_on(c, o, 0x0887F8C8u, f_0887FBE8);
        call_on(c, o, 0x0887F8D0u, f_088219B0);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4 = 1, +24 = 7, +86 = 5), placed at its owner (0x088804E0), then 0x0888038C. */
SFA_FUNC(08880340, effect_place_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = 1; *H(o + 24) = 7; *H(o + 86) = 5;
    *W(c->r[29] + 20) = ra;
    call2(c, o, *W(o + 88), 0x08880374u, f_088804E0);
    call_on(c, o, 0x0888037Cu, f_0888038C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): its start (0x08918D20) when it has no effects (+536), or under two while +251 is set;
   else 0. */
SFA_FUNC(08918CCC, object_start_if_few) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[2] = 0;
    *W(c->r[29] + 16) = ra;
    uint32_t n = *B(o + 536);
    ARG(1) = n;
    if (S8(o + 251) == 0) { if (n == 0) { c->r[31] = 0x08918CF4u; f_08918D20(c); } }
    else if ((int32_t)n < 2) { c->r[31] = 0x08918D14u; f_08918D20(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 = 1 and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 0xC; +11
   = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(08983F74, object_start_386_is_1) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    c->r[18] = 0; c->r[17] = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    if (S8(o + 386) == 1) {
        c->r[31] = 0x08983FA4u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1; *B(o + 236) = 0xC;
            c->r[18] = 1;
            call_on(c, o, 0x08983FE0u, f_0886DBE8);
        }
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): gone (0x0882324C) once its owner (+88) is; else +1 = 1, at the owner's place (+16 / +20,
   +11, +24) with +84 = its owner's picture (+28's +4) - 0x2B3. */
SFA_FUNC(089CCF94, effect_on_owner_place) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 16) = ra;
    if (*B(p) == 0) { c->r[31] = 0x089CCFFCu; f_0882324C(c); }
    else {
        *B(o + 1) = 1;
        *H(o + 24) = *H(p + 24);
        *W(o + 16) = *W(p + 16);
        *W(o + 20) = *W(p + 20);
        *B(o + 11) = *B(p + 11);
        gaddr q = *W(p + 28);
        if (q != 0) *H(o + 84) = (uint16_t)(*H(q + 4) - 0x2B3);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 not negative and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1; +5 / +7
   / +236 cleared, +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(089063D0, object_start_386_0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) >= 0) {
        c->r[31] = 0x089063F4u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0;
            c->r[17] = 1;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, 0x0890642Cu, f_0886DBE8);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
