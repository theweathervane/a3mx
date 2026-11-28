/* Thirteen of the link's small pieces: a menu's steps while a room is joined, lists guarded by a
   semaphore, a name drawn again, a port opened, and a packet's record. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0885A728(Cpu *c); void f_088DC3D4(Cpu *c); void f_088D5680(Cpu *c); void f_088E3ED8(Cpu *c);
void f_088DCD40(Cpu *c); void f_088DEB60(Cpu *c); void f_088DEC74(Cpu *c); void f_088E3F44(Cpu *c);
void f_088DC328(Cpu *c); void f_0880A6CC(Cpu *c); void f_088D68C8(Cpu *c); void f_088DC48C(Cpu *c);
void f_089F3470(Cpu *c); void f_089E4F28(Cpu *c); void f_088DD22C(Cpu *c); void f_088DD2A8(Cpu *c);
void f_088DF6D4(Cpu *c); void f_088E1BC8(Cpu *c); void f_088DC2C4(Cpu *c); void f_088E066C(Cpu *c);
void f_088DC18C(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);
void hle_sceNetAdhocMatchingCancelTarget(Cpu *c); void hle_sceNetAdhocPdpCreate(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
/* The semaphore whose handle is at `at`, when there is one, taken or given back. */
static void sema_wait(Cpu *c, gaddr at, uint32_t back) {
    uint32_t s = *W(at);
    ARG(0) = s;
    if (s != 0) call3(c, s, 1, 0, hle_sceKernelWaitSema, back);
}
static void sema_signal(Cpu *c, gaddr at, uint32_t back) {
    uint32_t s = *W(at);
    ARG(0) = s;
    if (s != 0) call2(c, s, 1, hle_sceKernelSignalSema, back);
}

/* (fighter): its +696 is cleared, its +7 moves on, and a walk starts. */
SFA_FUNC(0885A700, walk_start_step_on) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(f + 696) = 0;
    c->r[5] = (uint32_t)*B(f + 7) + 1;
    *W(c->r[29] + 16) = ra;
    *B(f + 7) = (uint8_t)c->r[5];
    call0(c, f_0885A728, 0x0885A71Cu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (menu): the menu leaves for the screen at 0x08A3EB14, its counters cleared. */
SFA_FUNC(088D56C4, menu_leave_eb14) {
    gaddr p = ARG(0);
    c->r[5] = 0x08A3EB14u;
    *W(p + 20) = 0x08A3EB14u;
    *H(p + 24) = 0;
    *H(p + 2) = 0;
    *H(p) = 0;
    *B(p + 47) = 0;
}

/* (menu): a room is joined, a step (+0) at a time: 0 the link starts (0x088DC3D4; then the byte
   at 0x08E75B98, 0x088E3ED8 with 3 and 0x088DCD40) and goes straight on to 1, which asks for the
   room the choices name (0x088DEB60 with a word from 0x08A3E93C and one from 0x08A3EB38); 2 waits
   for it (0x088DEC74); 3 waits for 0x088E3F44; 4 ends it (0x088DC328) and counts +24 on. A
   failure anywhere goes to 0x088D5680; past 4 the menu leaves. Every frame the choices' line of
   text and the link's own line are drawn. A 0x30 frame keeping s0. */
SFA_FUNC(088D6F68, room_join_steps) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = p;
    uint32_t step = *H(p);
    *W(c->r[29] + 36) = ra;
    ARG(0) = step;
    if (!(step < 5)) {
        *W(p + 20) = 0x08A3EB14u;
        *H(p + 24) = 0; *H(p + 2) = 0; *H(p) = 0;
        *B(p + 47) = 0;
    } else if (step == 0 || step == 1) {
        int ask = 1;
        if (step == 0) {
            call0(c, f_088DC3D4, 0x088D6FB4u);
            if ((int32_t)c->r[2] < 0) { call1(c, p, f_088D5680, 0x088D6FC8u); ask = 0; }
            else {
                *B(0x08E75B98u) = 1;
                call1(c, 3, f_088E3ED8, 0x088D6FE4u);
                call0(c, f_088DCD40, 0x088D6FECu);
                *H(p) = (uint16_t)(*H(p) + 1);
            }
        }
        if (ask) {
            *H(p) = (uint16_t)(*H(p) + 1);
            uint32_t a = *W(0x08A3E93Cu + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2));
            uint32_t b = *W(0x08A3EB38u + ((uint32_t)*B(p + 44) << 2));
            call2(c, a, b, f_088DEB60, 0x088D705Cu);
            if (c->r[2] != 0) call1(c, p, f_088D5680, 0x088D7070u);
        }
    } else if (step == 2) {
        call0(c, f_088DEC74, 0x088D7080u);
        int32_t r = (int32_t)c->r[2];
        if (r == 0) *H(p) = (uint16_t)(*H(p) + 1);
        else if (r < 0) call1(c, p, f_088D5680, 0x088D70ACu);
    } else if (step == 3) {
        call0(c, f_088E3F44, 0x088D70BCu);
        if (c->r[2] != 0) *H(p) = (uint16_t)(*H(p) + 1);
    } else {
        call0(c, f_088DC328, 0x088D70DCu);
        *H(p + 2) = 0;
        *H(p + 24) = (uint16_t)(*H(p + 24) + 1);
        *H(p) = 0;
        *B(p + 47) = 0;
    }
    ARG(3) = 4; c->r[8] = 9;
    call3(c, *W(0x08A3EA04u + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2)), 2, 0x30, f_0880A6CC, 0x088D7144u);
    call1(c, p, f_088D68C8, 0x088D714Cu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (x): x is taken out of the list at 0x09523720 (0x088DC48C with the one before it), under the
   list's semaphore (+8). A frame keeping s0..s2. */
SFA_FUNC(088DC7E8, list_3720_remove) {
    gaddr x = ARG(0);
    const gaddr list = 0x09523720u;
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2; *W(sp + 16) = s0;
    c->r[16] = x; c->r[18] = list;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    c->r[17] = 0;
    sema_wait(c, list + 8, 0x088DC820u);
    gaddr prev = 0;
    for (gaddr cur = *W(list); cur != 0; cur = *W(prev)) {
        if (cur == x) { call3(c, list, x, prev, f_088DC48C, 0x088DC844u); break; }
        prev = cur;
        c->r[17] = prev;
    }
    sema_signal(c, list + 8, 0x088DC870u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* The place 0x09523900. */
SFA_FUNC(088DD370, place_9523900) {
    ARG(0) = 0x09520000u;
    c->r[2] = 0x09523900u;
}

/* (record): a name's record with a text (+8) has its buffer (+4) cleared and the name drawn
   again, and its +14 cleared. */
SFA_FUNC(088DDA8C, name_record_redraw) {
    gaddr rec = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = rec;
    if (rec != 0 && *W(rec + 8) != 0) {
        gaddr buf = *W(rec + 4);
        if (buf != 0) {
            call3(c, buf, 0, 0x1020, f_089F3470, 0x088DDAC0u);
            call2(c, (*W(rec + 4) & 0xFFFFFFE0u) + 0x20, *W(rec + 8), f_089E4F28, 0x088DDAD8u);
        }
        *B(rec + 14) = 0;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (target): the link's matching with the target is called off (the handle at 0x09523748): its
   +140 goes to 4 unless that fails; the answer 0x8041080C counts as 0, and another failure is
   kept at 0x09523744 when nothing is there yet. A frame keeping s0, s1. */
SFA_FUNC(088DF208, match_cancel_target) {
    gaddr t = ARG(0);
    const gaddr st = 0x09523740u;
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; *W(sp + 16) = s0;
    c->r[16] = t; c->r[17] = st;
    *W(sp + 24) = ra;
    call2(c, *W(st + 8), t, hle_sceNetAdhocMatchingCancelTarget, 0x088DF230u);
    if (c->r[2] == 0x8041080Cu) { c->r[2] = 0; *W(t + 140) = 4; }
    else if ((int32_t)c->r[2] >= 0) *W(t + 140) = 4;
    else if (*W(st + 4) == 0) *W(st + 4) = c->r[2];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (x): the link's port for x (its record from 0x088DD2A8, the address from 0x088DD22C) is opened:
   port 0x402, 0x1000 of buffer. Returns 1 with the port's number kept; 0 after a failure that
   may be tried again (the count at +16, 15 at most, with 0x088DF6D4); and the error, also kept at
   +4, after the last or without a record (0x80410707). A frame keeping s0, s1. */
SFA_FUNC(088DF828, port_open) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = x;
    call0(c, f_088DD22C, 0x088DF840u);
    gaddr addr = c->r[2];
    c->r[17] = addr;
    call1(c, x, f_088DD2A8, 0x088DF84Cu);
    gaddr rec = c->r[2];
    c->r[16] = rec;
    uint32_t answer;
    int failed = 0;
    uint32_t error = 0;
    if (rec == 0) {
        error = 0x80410707u;
        *B(rec + 16) = 0;
        failed = 1;
    } else {
        *W(rec + 4) = 0;
        *W(rec + 8) = 2;
        *H(addr + 6) = 0x402;
        ARG(3) = 0;
        call3(c, addr, *H(addr + 6), 0x1000, hle_sceNetAdhocPdpCreate, 0x088DF8ACu);
        error = c->r[2];
        c->r[17] = error;
        if ((int32_t)error >= 0) {
            *W(rec) = error;
            *B(rec + 16) = 0;
        } else {
            uint32_t tries = *B(rec + 16);
            if (!((int32_t)tries < 15)) { *B(rec + 16) = 0; failed = 1; }
            else {
                c->r[17] = tries;
                call1(c, rec, f_088DF6D4, 0x088DF8D4u);
                *B(rec + 16) = (uint8_t)(tries + 1);
            }
        }
    }
    if (failed) {
        c->r[17] = error;
        *W(rec + 4) = error;
        call1(c, rec, f_088DF6D4, 0x088DF870u);
        answer = error;
    } else answer = (int32_t)error >= 0 ? 1 : 0;
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* Once: the 0x2C bytes at 0x095236E0 are cleared, the byte at 0x095236CE is set and 0x088E1BC8
   runs with 0x095236C0. */
SFA_FUNC(088E135C, link_state_once) {
    const gaddr st = 0x095236C0u;
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = st;
    *W(c->r[29] + 20) = ra;
    if (S8(st + 14) == 0) {
        call3(c, 0x095236E0u, 0, 0x2C, f_089F3470, 0x088E138Cu);
        *B(st + 14) = 1;
        call1(c, st, f_088E1BC8, 0x088E139Cu);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (x): x is handed to 0x088E066C with the state at 0x095236E0, under the semaphore at
   0x08A3EB88, and its answer returned; while the word at 0x095236C0 is set x is given back
   (0x088DC2C4) and 0x81001000 returned. A frame keeping s0..s2. */
SFA_FUNC(088E1468, link_hand_over) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2;
    uint32_t busy = *W(0x095236C0u);
    c->r[18] = 0x095236E0u;
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 28) = ra;
    c->r[16] = x;
    if (busy != 0) {
        call1(c, x, f_088DC2C4, 0x088E14B4u);
        c->r[2] = 0x81001000u;
    } else {
        c->r[17] = 0x08A40000u;
        sema_wait(c, 0x08A3EB88u, 0x088E14DCu);
        call2(c, 0x095236E0u, x, f_088E066C, 0x088E14E8u);
        uint32_t answer = c->r[2];
        c->r[16] = answer;
        sema_signal(c, 0x08A3EB88u, 0x088E14FCu);
        c->r[2] = answer;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, n): x waits to be sent, under the semaphore at 0x08A3EB88: the first in the state at
   0x095236E0 (+24, with n at +34 and the counts at +32 and +36 cleared; a second one waiting is
   given back), a later one as the second (+28, n at +38), in place of the one that was there. A
   0x30 frame keeping s0..s3. */
SFA_FUNC(088E187C, link_queue_two) {
    gaddr x = ARG(0);
    uint32_t n = ARG(1);
    const gaddr st = 0x095236E0u;
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 4; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 32) = sv[4];
    c->r[17] = x; c->r[18] = 0x08A40000u; c->r[19] = st; c->r[16] = n;
    ARG(1) = n;
    sema_wait(c, 0x08A3EB88u, 0x088E18BCu);
    uint32_t second = *W(st + 28);
    if (*W(st + 24) == 0) {
        *W(st + 24) = x;
        *H(st + 32) = 0;
        *H(st + 36) = 0;
        *H(st + 34) = (uint16_t)n;
        if (second != 0) {
            call1(c, second, f_088DC2C4, 0x088E18E4u);
            *W(st + 28) = 0;
        }
    } else {
        if (second != 0) {
            call1(c, second, f_088DC2C4, 0x088E18FCu);
            *W(st + 28) = 0;
        }
        *H(st + 38) = (uint16_t)n;
        *W(st + 28) = x;
    }
    sema_signal(c, 0x08A3EB88u, 0x088E191Cu);
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* The byte at 0x09523978 is cleared. */
SFA_FUNC(088E3FC0, clear_byte_3978) {
    ARG(0) = 0x09523960u;
    *B(0x09523960u + 24) = 0;
}

/* (kind, v): a packet's record: the size the kind's low four bits name in 0x08A3EC50 and twelve
   bytes of header, cleared, with v at +4, 0xAC at +8 and the kind at +9. Returns the place past
   the header (12 when there is no room). A 0x30 frame keeping s0..s3. */
SFA_FUNC(088E4324, packet_record_new) {
    uint32_t kind = ARG(0) & 0xFF, v = ARG(1);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 4; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 32) = sv[4];
    uint32_t size = *W(0x08A3EC50u + ((kind & 0xF) << 2)) + 0xC;
    c->r[19] = kind; c->r[18] = size; c->r[16] = v;
    call1(c, size, f_088DC18C, 0x088E4368u);
    gaddr rec = c->r[2];
    c->r[17] = rec;
    if (rec != 0) {
        call3(c, rec, 0, size, f_089F3470, 0x088E4380u);
        *W(rec + 4) = v;
        *B(rec + 9) = (uint8_t)kind;
        *B(rec + 8) = 0xAC;
    }
    c->r[2] = rec + 0xC;
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}
