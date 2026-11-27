/* Guard stun: the +127 counter, the push-back from 0x08A17FC8 and the crouch / stand choice while
   blocking. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861724(Cpu *c); void f_08861848(Cpu *c); void f_08881F70(Cpu *c); void f_0885CE18(Cpu *c);
void f_0885D164(Cpu *c); void f_088619EC(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter): while blocking (+116 = 1, +127 running, a reaction) switch the guard by the stick for a
   player (+752 bit 0 held: bit 2 crouching move 0xF, else standing 0xC; not held: ticked) or by the
   opponent for the cpu (its +310 or +696 crouch; its +73 stands), with +696 = 1 for crouching. */
SFA_FUNC(08864044, guard_stance) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = 1;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 116) == 1 && S8(o + 127) != 0 && *B(*W(o + 296) + 5) != 0) {
        if (S8(o + 373) == 0) {
            uint32_t h = *H(o + 752);
            c->r[6] = h & 4;
            if ((h & 1) == 0) { c->r[31] = 0x088640D0u; f_08861848(c); }
            else if ((h & 4) == 0) { *B(o + 696) = 0; call2(c, o, 0xC, f_08861724, 0x088640BCu); }
            else { *B(o + 696) = 1; call2(c, o, 0xF, f_08861724, 0x088640A8u); }
        } else {
            gaddr p = *W(o + 92);
            if (S8(p + 310) != 0) { *B(o + 696) = 1; call2(c, o, 0xF, f_08861724, 0x088640F4u); }
            else if (S8(p + 73) != 0) { *B(o + 696) = 0; call2(c, o, 0xC, f_08861724, 0x08864120u); }
            else if (S8(p + 696) != 0) { *B(o + 696) = 1; call2(c, o, 0xF, f_08861724, 0x08864134u); }
            else { *B(o + 696) = 0; call2(c, o, 0xC, f_08861724, 0x08864120u); }
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the guard stance (0x08864044); on odd frames of +82 a grounded, unlocked fighter gets
   0x08881F70 (with +541 cleared meanwhile; +541 = 1 after); it shakes along x: 1 back from the facing
   (+134), doubled with a free hit slot, then 3 by +11, flipped on odd +127. */
SFA_FUNC(08863F80, guard_shake) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call1(c, o, f_08864044, 0x08863F94u);
    *B(o + 541) = 0;
    if ((S16(o + 82) & 1) && S8(o + 238) == 0 && S8(o + 73) == 0) call1(c, o, f_08881F70, 0x08863FC4u);
    *B(o + 541) = 1;
    int16_t d = S8(o + 134) != 0 ? 1 : -1;
    if (S16(o + 112) < 0) d = (int16_t)(d * 2);
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
    int16_t e = *B(o + 11) != 0 ? 3 : -3;
    if (S8(o + 127) & 1) e = (int16_t)-e;
    c->r[5] = (uint32_t)(int32_t)e;
    *H(o + 18) = (uint16_t)(S16(o + 18) + e);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the guard stun counter +127: run out it moves to the next state (+6 on, +7 cleared) by
   0x0885CE18 for kind 0x21, else 0x0885D164; running, with the opponent's +251 the shake (0x08863F80),
   the push-back +268 from 0x08A17FC8 (negated by +134) and 0x088619EC. A frame keeping s0, s1. */
SFA_FUNC(0885CD4C, guard_stun) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    int8_t t = (int8_t)(S8(o + 127) - 1);
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *B(o + 127) = (uint8_t)t;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (t < 0) {
        *B(o + 7) = 0;
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        uint32_t k = *B(o + 117);
        *B(o + 127) = 0;
        if (k == 0x21) call1(c, o, f_0885CE18, 0x0885CDA4u);
        else call1(c, o, f_0885D164, 0x0885CDB4u);
    } else {
        const gaddr tab = 0x08A17FC8u; c->r[17] = tab;
        if (S8(*W(o + 92) + 251) != 0) call1(c, o, f_08863F80, 0x0885CDD4u);
        int32_t v = S16(tab + (uint32_t)(S8(o + 127) * 2));
        c->r[17] = (uint32_t)v;
        *H(o + 268) = (uint16_t)(S8(o + 134) != 0 ? -v : v);
        call1(c, o, f_088619EC, 0x0885CE04u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0885D080(Cpu *c); void f_08863850(Cpu *c); void f_0885CE18(Cpu *c); void f_08880C38(Cpu *c);
void f_08880C54(Cpu *c); void f_0887C408(Cpu *c); void f_08863130(Cpu *c); void f_0885D0F0(Cpu *c);
void f_08880568(Cpu *c); void f_0885DB78(Cpu *c); void f_0885E6B0(Cpu *c);

static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (fighter): a guard reaction's start: +116 / +631 / +587 cleared, facing the opponent, 0x0885D080, the
   reaction move by kind from 0x08A18F3C (one more with +588), 0x08863850, and the stun counter step
   once +127 is clear. */
SFA_FUNC(0885C2C4, guard_react) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 116) = 0; *B(o + 631) = 0; *B(o + 587) = 0;
    *B(o + 11) = (uint8_t)S8(o + 697);
    call1(c, o, f_0885D080, 0x0885C2F0u);
    uint32_t n = *B(0x08A18F3Cu + *B(o + 117));
    if (S8(o + 588) != 0) n = (n + 1) & 0xFFFF;
    call2(c, o, n, f_08861724, 0x0885C324u);
    call1(c, o, f_08863850, 0x0885C32Cu);
    if (S8(o + 127) == 0) call1(c, o, f_0885CD4C, 0x0885C340u);
    leave(c, sv);
}

/* A guard crush start (+656 = kind): +116 / +631 / +587 cleared, 0x0885D080, 0x0885CE18, the first
   time a counter (+561 / +560) and its cue, 0x0887C408 (+337, 0x2C) and the stun counter step once
   +127 is clear. */
static void guard_crush(Cpu *c, uint32_t kind, gaddr flag, void (*cue)(Cpu *), const uint32_t back[6]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 116) = 0; *B(o + 631) = 0; *B(o + 587) = 0; *B(o + 656) = (uint8_t)kind;
    call1(c, o, f_0885D080, back[0]);
    call1(c, o, f_0885CE18, back[1]);
    if (S8(o + flag) == 0) { *B(o + flag) = 1; call1(c, o, cue, back[2]); }
    ARG(0) = o; ARG(1) = (uint32_t)S8(o + 337); ARG(2) = 0x2C; c->r[31] = back[3]; f_0887C408(c);
    if (S8(o + 127) == 0) call1(c, o, f_0885CD4C, back[4]);
    leave(c, sv);
}

/* (fighter): a guard crush of kind 1 (+561, 0x08880C38). */
SFA_FUNC(0885C5E0, guard_crush_1) {
    static const uint32_t b[6] = { 0x0885C60Cu, 0x0885C614u, 0x0885C62Cu, 0x0885C63Cu, 0x0885C650u, 0 };
    guard_crush(c, 1, 561, f_08880C38, b);
}

/* (fighter): a guard crush of kind 2 (+560, 0x08880C54). */
SFA_FUNC(0885C6C4, guard_crush_2) {
    static const uint32_t b[6] = { 0x0885C6F0u, 0x0885C6F8u, 0x0885C710u, 0x0885C720u, 0x0885C734u, 0 };
    guard_crush(c, 2, 560, f_08880C54, b);
}

/* A knock-up start: +235 / +696 / +541 / +604 cleared, +7 on, +770 = 1 with +313, 0x08863130 and
   0x0885D0F0, +73 = -1, 0x08880568 with +702, then the fall's first frame. */
static void knock_up(Cpu *c, void (*fall)(Cpu *), const uint32_t back[4]) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    *B(o + 235) = 0;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 696) = 0; *B(o + 541) = 0;
    int32_t k = S8(o + 313);
    *B(o + 604) = 0;
    enter(c, o, sv);
    if (k != 0) *B(o + 770) = 1;
    call1(c, o, f_08863130, back[0]);
    call1(c, o, f_0885D0F0, back[1]);
    *B(o + 73) = 0xFF;
    if (S8(o + 702) != 0) call1(c, o, f_08880568, back[2]);
    call1(c, o, fall, back[3]);
    leave(c, sv);
}

/* (fighter): knocked up into the fall 0x0885DB78. */
SFA_FUNC(0885DAFC, knock_up_a) {
    static const uint32_t b[4] = { 0x0885DB40u, 0x0885DB48u, 0x0885DB60u, 0x0885DB68u };
    knock_up(c, f_0885DB78, b);
}

/* (fighter): knocked up into the fall 0x0885E6B0. */
SFA_FUNC(0885E634, knock_up_b) {
    static const uint32_t b[4] = { 0x0885E678u, 0x0885E680u, 0x0885E698u, 0x0885E6A0u };
    knock_up(c, f_0885E6B0, b);
}
