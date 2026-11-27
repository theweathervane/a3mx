/* Command steps for held-button and direction elements (the command state: +0 step, +1 element, +2 / +4
   timers), and a hit handler picked by the defender's reaction. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08868208(Cpu *c); void f_0886806C(Cpu *c); void f_088681F8(Cpu *c); void f_08868214(Cpu *c);

static void call_s(Cpu *c, gaddr s, void (*fn)(Cpu *), uint32_t back) { ARG(0) = s; c->r[31] = back; fn(c); }

/* (fighter, state, table): a held-button element's timer: +4 counts down, and each time it runs out +2
   drops by 2 (0 ends it, 0x088681F8, else +4 = 2). With no buttons, or none the element names, the
   command fails (0x08868208); the light / medium / heavy pairs need +2 under 2 / 4 / 6 to fail, else it
   holds (0x08868214). */
SFA_FUNC(088693B0, cmd_hold_buttons) {
    gaddr o = ARG(0), s = ARG(1), t = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t4 = (uint32_t)(*B(s + 4) - 1);
    c->r[7] = t4 & 0xFF; c->r[8] = t4;
    *W(c->r[29] + 16) = ra;
    *B(s + 4) = (uint8_t)t4;
    if ((t4 & 0xFF) == 0) {
        uint32_t c2 = (uint32_t)(*B(s + 2) - 2) & 0xFF;
        *B(s + 2) = (uint8_t)c2;
        if (c2 == 0) {
            call_s(c, s, f_088681F8, 0x088693F4u);
            c->r[31] = ra; c->r[29] += 0x20; return;
        }
        *B(s + 4) = 2;
    }
    uint32_t idx = *B(s + 1);
    uint32_t btn = (*H(o + 760) | *H(o + 762)) & 0x3F0;
    uint32_t e = *H(t + idx * 2);
    c->r[6] = e;
    uint32_t back;
    void (*fn)(Cpu *) = f_08868208;
    if (btn == 0) back = 0x08869468u;
    else {
        uint32_t m = btn & e & 0xFFFF;
        c->r[6] = m;
        if (m == 0) back = 0x08869454u;
        else {
            int lim = (m & 0x90) ? 2 : (m & 0x120) ? 4 : (m & 0x240) ? 6 : 0;
            if (lim == 0) back = 0x088694A4u;
            else if (*B(s + 2) < (uint32_t)lim) back = 0x088694C0u;
            else { fn = f_08868214; back = 0x088694D4u; }
        }
    }
    call_s(c, s, fn, back);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, state, table): a command element: with bit 13 it needs the stick (+764 low bits) to share a
   direction with it, without it to match exactly (an empty element is the wait 0x0886806C); a mismatch
   fails (0x08868208). A matched element with bit 10 moves on (+0 on, next element, +4 = 0xA, +2 = 6)
   into the held-button timer, otherwise the wait. */
SFA_FUNC(088692C4, cmd_element) {
    gaddr o = ARG(0), s = ARG(1), t = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t idx = *B(s + 1);
    uint32_t e = *H(t + idx * 2);
    uint32_t st = *H(o + 764) & 0xF, d = e & 0xF;
    c->r[2] = e & 0x2000;
    *W(c->r[29] + 16) = ra;
    int ok;
    if (e & 0x2000) {
        if ((st & d) == 0) { call_s(c, s, f_08868208, 0x08869358u); c->r[31] = ra; c->r[29] += 0x20; return; }
        ok = 1;
    } else if (e == 0) {
        ARG(0) = o; ARG(1) = s; c->r[6] = o; c->r[31] = 0x08869374u; f_0886806C(c);
        c->r[31] = ra; c->r[29] += 0x20; return;
    } else if (st != d) {
        call_s(c, s, f_08868208, 0x08869390u); c->r[31] = ra; c->r[29] += 0x20; return;
    } else ok = 1;
    (void)ok;
    c->r[7] = t;
    if (e & 0x400) {
        *B(s) = (uint8_t)(*B(s) + 1);
        *B(s + 1) = (uint8_t)(idx + 1);
        *B(s + 4) = 0xA; *B(s + 2) = 6;
        c->r[8] = 6;
        ARG(0) = o; ARG(1) = s; ARG(2) = t; c->r[31] = 0x08869344u; f_088693B0(c);
    } else {
        ARG(0) = o; ARG(1) = s; c->r[6] = o; c->r[31] = 0x088693A4u; f_0886806C(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, defender, c): the handler for the defender's reaction (+296's +5) from 0x08A21C10. */
SFA_FUNC(08870AF8, hit_by_reaction) {
    gaddr b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[8] = 0x08A21C20u;
    uint32_t fn = *W(0x08A21C20u + ((uint32_t)*B(*W(b + 296) + 5) << 2) - 16);
    c->r[7] = fn;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x08870B24u; rt_call_indirect(c, fn);
    c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter, state, table): a command element's timer +4 counts down: out of time it ends (0x088681F8),
   else the element check (0x088692C4). */
SFA_FUNC(0886926C, cmd_element_timed) {
    gaddr o = ARG(0), s = ARG(1), t = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t4 = (uint32_t)(*B(s + 4) - 1);
    c->r[8] = t;
    *B(s + 4) = (uint8_t)t4;
    c->r[7] = t4 & 0xFF; c->r[6] = o;
    *W(c->r[29] + 16) = ra;
    if ((t4 & 0xFF) == 0) { call_s(c, s, f_088681F8, 0x0886929Cu); }
    else { c->r[7] = t; ARG(0) = o; ARG(1) = s; ARG(2) = t; c->r[31] = 0x088692B8u; f_088692C4(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, state, table): a release element: the stick must leave the first entry's direction (with
   bit 13 share none of it, else differ), or the command fails (0x08868208); then +0 on, the wait
   (0x0886806C) and the element check (0x088692C4). A frame keeping s0..s2. */
SFA_FUNC(088691A0, cmd_release) {
    gaddr o = ARG(0), s = ARG(1), t = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; c->r[18] = o;
    uint32_t e = *H(t + 2);
    *W(c->r[29] + 20) = s1; c->r[17] = s;
    uint32_t st = *H(o + 764) & 0xF, d = e & 0xF;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = t;
    int fail;
    if (e & 0x2000) fail = ((st & d) & 0xFFFF) != 0;
    else fail = st == d;
    if (fail) call_s(c, s, f_08868208, (e & 0x2000) ? 0x088691F0u : 0x08869218u);
    else {
        *B(s) = (uint8_t)(*B(s) + 1);
        ARG(0) = o; ARG(1) = s; c->r[31] = 0x08869244u; f_0886806C(c);
        ARG(0) = o; ARG(1) = s; ARG(2) = t; c->r[31] = 0x08869254u; f_088692C4(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0886766C(Cpu *c); void f_08821368(Cpu *c);

/* (fighter, state): the alternate setting (0x0886766C), +1 on, and +4 = a random delay (0x08A1B650 by
   0x08821368's low five bits) plus the setting's from 0x08A1B670, then the step 0x08868208. A frame
   keeping s0, s1. */
SFA_FUNC(08868184, cmd_random_wait) {
    gaddr s = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = s;
    c->r[31] = 0x0886819Cu; f_0886766C(c);
    uint32_t k = c->r[2]; c->r[17] = k;
    c->r[4] = (uint32_t)*B(s + 1) + 1;
    *B(s + 1) = (uint8_t)c->r[4];
    c->r[31] = 0x088681B0u; f_08821368(c);
    uint32_t a = *B(0x08A1B650u + (c->r[2] & 0x1F));
    c->r[6] = a;
    *B(s + 4) = (uint8_t)(a + *B(0x08A1B670u + k));
    call_s(c, s, f_08868208, 0x088681E4u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
