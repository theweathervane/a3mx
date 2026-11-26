/* Small functions the scenarios reach, 54 - 56 instructions: effects following their owners' level
   or step, a level object handing a pointer over, two screen objects set up from tables, a
   list-0x29 effect jittered round its owner. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define PALS   0x092D9E00u

void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_08821424(Cpu *c); void f_088D3B64(Cpu *c);
void f_088D3B88(Cpu *c); void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect, owner): off (+1) and gone (+4 on) once the owner is no longer its character (+338 =
   +3); with a level (+296's +10) at the owner's place and side and on: a new one (+78) takes its
   animation from 0x08A9B060, else 0x088219B0. No level clears +78. */
SFA_FUNC(089A20D8, effect_owner_level_4) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 3) != (uint32_t)S8(p + 338)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t lv = *B(*W(p + 296) + 10);
        if (lv == 0) *B(o + 78) = 0;
        else {
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 1) = 1;
            *B(o + 11) = *B(p + 11);
            lv = *B(*W(p + 296) + 10);
            if ((uint32_t)S8(o + 78) != lv) {
                *B(o + 78) = (uint8_t)lv;
                c->r[6] = *W(0x08A9B060u + 4 * *B(*W(p + 296) + 10));
                call2(c, o, c->r[6], 0x089A2190u, f_0882196C);
            } else call_on(c, o, 0x089A21A4u, f_088219B0);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): halted it is only drawn; while its owner (+88) is in step 1/0/7 with +77 set it takes
   that as its +0 and the owner's place, drawn; otherwise gone (+4 on, +1 cleared, +0 = 2). */
SFA_FUNC(089C59D8, effect_owner_step_place) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr p = *W(o + 88);
    int draw = 1;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
        if (*B(p + 4) == 1 && *B(p + 5) == 0 && *B(p + 6) == 7 && S8(p + 77) != 0) {
            *B(o) = (uint8_t)S8(p + 77);
            *W(o + 16) = *W(p + 16);
            *W(o + 20) = *W(p + 20);
        } else {
            *B(o + 1) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            *B(o) = 2;
            draw = 0;
        }
    }
    if (draw) { c->r[31] = 0x089C5A64u; f_08821424(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): follows its owner's (+88) +296's +11: with none, off (+1, +78 cleared); with one, on;
   a new one (+78) is set up (0x088D3B64 with 0x08AB26B8) and hands the owner's +48 object's +48
   over: 13 / 18 take it (the owner's +40 object's +48 = this), others give it back; an unchanged
   one 0x088D3B88. */
SFA_FUNC(089CCDE8, object_follow_level_hand) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 24) = ra;
    if (*B(*W(p + 296) + 11) == 0) { *B(o + 1) = 0; *B(o + 78) = 0; }
    else {
        *B(o + 1) = 1;
        uint32_t lv = *B(*W(p + 296) + 11);
        if ((uint32_t)S8(o + 78) == lv) call_on(c, o, 0x089CCE38u, f_088D3B88);
        else {
            *B(o + 78) = (uint8_t)lv;
            ARG(0) = o; ARG(1) = 0x08AB26B8u; ARG(2) = *B(*W(p + 296) + 11);
            c->r[31] = 0x089CCE54u; f_088D3B64(c);
            gaddr q = *W(p + 48);
            int32_t v = S8(o + 78);
            gaddr cur = *W(q + 48);
            if (v == 13 || v == 18) {
                if (cur != 0) { *W(q + 48) = 0; *W(*W(p + 40) + 48) = o; }
            } else if (cur == 0) { *W(q + 48) = o; *W(*W(p + 40) + 48) = 0; }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* Shared tail of the two screen objects below: +9 / +11 - +14 cleared, +24 = 7, +86 = 0x1C, +148 =
   3, +164 cleared, then 0x0880AEA8 (0, PALS +112, 0, 0, 5). */
static void screen_obj_tail(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 12) = 0; *B(o + 14) = 0; *B(o + 9) = 0;
    *H(o + 24) = 7;
    *B(o + 11) = 0;
    *H(o + 86) = 0x1C;
    *W(o + 148) = 3;
    *W(o + 164) = 0;
    ARG(0) = 0; ARG(1) = *W(PALS + 112); ARG(2) = 0; ARG(3) = 0; c->r[8] = 5;
    c->r[31] = back; f_0880AEA8(c);
}

/* (object): on (+4), at (0xC0, 0x78 / 0xB8 with +10 set), picture +84 by +10 (8, 0x25, 0x2C,
   then 0x25), and the screen object set-up. */
SFA_FUNC(088455D0, screen_object_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 18) = 0xC0;
    uint32_t v = *B(o + 10);
    *W(c->r[29] + 32) = ra;
    *H(o + 22) = v != 0 ? 0xB8 : 0x78;
    *H(o + 84) = v == 0 ? 8 : v == 2 ? 0x2C : 0x25;
    *B(o + 13) = 0;
    screen_obj_tail(c, o, 0x088456A0u);
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): its kind (+3) picks its step (+4, 0x08A07898), place (0x08A07860) and picture (+84,
   0x08A078A6); the screen object set-up; then its step (0x08A078B4). */
SFA_FUNC(08818D28, screen_object_from_tables) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    uint32_t k = *B(o + 3);
    *B(o + 4) = *B(0x08A07898u + k);
    *H(o + 18) = *H(0x08A07860u + 4 * k);
    *H(o + 22) = *H(0x08A07860u + 4 * k + 2);
    *B(o + 9) = 0;
    *H(o + 84) = *B(0x08A078A6u + k);
    *B(o + 13) = 0;
    *W(c->r[29] + 36) = ra;
    screen_obj_tail(c, o, 0x08818DD8u);
    uint32_t h = *W(0x08A078B4u + 4 * *B(o + 4));
    c->r[5] = h;
    ARG(0) = o; c->r[31] = 0x08818DF8u; rt_call_indirect(c, h);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (owner): a list-0x29 effect (0x08822EAC, 4) with its +11 - +14 / +24 / +38 / +86 / +164, pointing
   back at it (+88), placed by the next of the 8 offset pairs at 0x08A64914 (its owner's owner
   +0x339 steps by 2, x turned for +11). */
SFA_FUNC(0891ED28, effect_jitter_29) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x29, 4, 0x0891ED44u, f_08822EAC);
    gaddr n = c->r[2];
    ARG(0) = n;
    if (n != 0) {
        *B(n + 14) = *B(o + 14);
        *B(n + 12) = (uint8_t)S8(o + 12);
        *B(n + 13) = *B(o + 13);
        *H(n + 38) = (uint16_t)S16(o + 38);
        *H(n + 24) = *H(o + 24);
        *W(n + 88) = o;
        *B(n + 11) = *B(o + 11);
        *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
        gaddr q = *W(o + 88) + 0x334;
        uint32_t k = (uint32_t)(*B(q + 5) + 2) & 0xE;
        *B(q + 5) = (uint8_t)k;
        gaddr e = 0x08A64914u + k;
        *H(n + 22) = (uint16_t)(S16(o + 22) + S8(e + 1));
        int32_t dx = S8(e);
        if (*B(n + 11) != 0) dx = (int16_t)-dx;
        *H(n + 18) = (uint16_t)(S16(o + 18) + dx);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
