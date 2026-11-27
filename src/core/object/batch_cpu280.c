/* The cpu's counter-move choice: which of its twelve answers (tables in the pairing block at 0x09358468 /
   0x093584C8, eight bytes each: +192 / +193 / +194 / +290 to copy, +4 reach) fit the opponent's attack
   timing and distance. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define PAIRING 0x09358410u

/* (fighter, hit, frames): the answers that fit: bit i when the frames left reach answer i's +0; then,
   by the opponent's own answer slot (crouch / jump / strength, mirrored by its facing), each answer
   whose reach (+4, mirrored by the fighter's facing) falls on the wrong side of it is dropped. The
   second pass tests bit 0 each round and walks the reach list only while bit 0 is set (as the original
   does). */
SFA_FUNC(0886D150, cpu_answers_fit) {
    gaddr o = ARG(0);
    int32_t frames = (int32_t)c->r[6];
    gaddr p = *W(o + 92);
    int32_t mine = S8(o + 337), theirs = S8(p + 337);
    int32_t c193 = S8(p + 193), c192 = S8(p + 192), str = S8(p + 194);
    uint32_t pface = *B(p + 11);
    int32_t px = S16(p + 18);
    uint32_t face = *B(o + 11);
    c->r[24] = PAIRING;
    int32_t ox = S16(o + 18);
    uint32_t r = 0;
    gaddr t = PAIRING + (mine != 0 ? 0xB8u : 0x58u);
    for (uint32_t i = 0; i < 12; i++) if (!(frames < S16(t + i * 8))) r |= 1u << i;
    gaddr their = PAIRING + (theirs != 0 ? 0xB8u : 0x58u) + 96;
    int32_t slot = c193 == 0 ? 0 : 6;
    if (c192 != 0) slot += 3;
    int32_t v = S16(their + (uint32_t)((slot + str) * 8) + 4);
    if (pface == 0) v = -v;
    int32_t target = v + px;
    gaddr e = t;
    for (uint32_t i = 0; i < 12; i++) {
        if ((r & 1) == 0) continue;
        int32_t reach = S16(e + 4);
        e += 8;
        if (face != 0) reach = -reach;
        reach += ox;
        if (face == 0 ? target < reach : reach < target) r ^= 1u << i;
    }
    c->r[2] = r;
}

void f_0886D150(Cpu *c);

/* (fighter, hit, frames): the first answer (in the order at 0x08A1BFBC, or 0x08A1BFC8 against a
   crouching opponent) that fits (0x0886D150): its stance, jump and strength go to +192 / +193 / +194 and
   its buttons to +290: 1, else 0. A frame keeping s0, s1. */
SFA_FUNC(0886D054, cpu_answer_pick) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92); c->r[17] = p;
    c->r[31] = 0x0886D070u; f_0886D150(c);
    uint32_t mask = c->r[2];
    gaddr order = S8(p + 192) != 0 ? 0x08A1BFC8u : 0x08A1BFBCu;
    uint32_t r = 0;
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = *B(order + i);
        if (mask & (1u << k)) {
            gaddr a = PAIRING + (S8(o + 337) != 0 ? 0xB8u : 0x58u) + k * 8;
            *B(o + 192) = (uint8_t)S16(a);
            *B(o + 193) = (uint8_t)S16(a + 2);
            *B(o + 194) = (uint8_t)S16(a + 4);
            *H(o + 290) = (uint16_t)S16(a + 6);
            r = 1;
            break;
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): against an opponent in the middle of an attack (state 1 / 0 / 5), the frames to its next
   hit are added up along its animation script (+28, frames of 10 bytes from +416's list) until a frame
   with a hit (+1), then the counter-move choice (0x0886D054) with that hit and the frames; 0 when the
   script ends first. */
SFA_FUNC(0886CF90, cpu_counter) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 92);
    *W(c->r[29] + 16) = ra;
    uint32_t r = 0;
    if (*B(p + 4) == 1 && *B(p + 5) == 0 && *B(p + 6) == 5) {
        gaddr s = *W(o + 28);
        gaddr tab = *W(*W(o + 416) + 16);
        gaddr e = tab + (uint32_t)*H(s + 8) * 12;
        uint32_t acc = 0;
        for (;;) {
            uint32_t n = *B(e + 1);
            if (n != 0) {
                ARG(1) = *W(o + 224) + (n << 5); c->r[6] = acc;
                c->r[31] = 0x0886D048u; f_0886D054(c);
                r = c->r[2];
                break;
            }
            acc += *B(s);
            int32_t f = S8(s + 1);
            s += 10;
            if (f & 0x80) break;
            e = tab + (uint32_t)*H(s + 8) * 12;
        }
    }
    c->r[2] = r;
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_0886CF90(Cpu *c); void f_0886DC3C(Cpu *c); void f_0887A10C(Cpu *c); void f_0887B6B8(Cpu *c);
void f_0887B7CC(Cpu *c); void f_0887B1A8(Cpu *c);

/* The cpu's counter move begins: +482 = 3, +483 cleared, state 1 / 0 / 5 / 0, +235 = 1, +498 cleared,
   facing the opponent, then 0x0886DC3C. */
static void counter_start(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 482) = 3; *B(o + 483) = 0;
    *B(o + 4) = 1; *B(o + 6) = 5; *B(o + 7) = 0; *B(o + 5) = 0;
    c->r[4] = 1; c->r[5] = (uint32_t)S8(o + 697);
    *B(o + 235) = 1; *B(o + 498) = 0; *B(o + 11) = (uint8_t)c->r[5];
    ARG(0) = o; c->r[31] = back; f_0886DC3C(c);
}

/* (fighter, script): a cpu script step: the counter move when one fits (0x0886CF90), returning the
   script unchanged; else the next step 0x0887A10C. A frame keeping s0, s1. */
SFA_FUNC(088777E8, cpu_step_counter) {
    gaddr o = ARG(0), x = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = o;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = x;
    c->r[31] = 0x08877804u; f_0886CF90(c);
    if (c->r[2] == 0) { ARG(0) = o; ARG(1) = x; c->r[31] = 0x08877818u; f_0887A10C(c); }
    else { counter_start(c, o, 0x08877868u); c->r[2] = x; }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, script): a cpu wait: +482 cleared, +483 = 6, state 1 / 0 / 0 / 0 and +506 = the script's
   frames; the counter move when one fits; returns the script past its word. A frame keeping s0..s2. */
SFA_FUNC(08877880, cpu_step_wait) {
    gaddr o = ARG(0), sc = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *B(o + 482) = 0;
    *W(c->r[29] + 24) = s2; c->r[18] = 1;
    *B(o + 483) = 6;
    *B(o + 4) = 1; *B(o + 7) = 0; *B(o + 6) = 0; *B(o + 5) = 0;
    int32_t n = S16(sc);
    *W(c->r[29] + 20) = s1; c->r[17] = sc + 2;
    *H(o + 506) = (uint16_t)n;
    *W(c->r[29] + 28) = ra;
    ARG(0) = o; c->r[31] = 0x088778CCu; f_0886CF90(c);
    if (c->r[2] != 0) counter_start(c, o, 0x0887790Cu);
    c->r[2] = sc + 2;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the cpu's waiting frame: 0x0887B6B8 hands over to 0x0887B7CC; else +506 counts down to
   the next script step (0x0887B1A8), taking the counter move when one fits. */
SFA_FUNC(0887A694, cpu_wait_frame) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    ARG(0) = o; c->r[31] = 0x0887A6A8u; f_0887B6B8(c);
    if (c->r[2] != 0) { ARG(0) = o; c->r[31] = 0x0887A6B8u; f_0887B7CC(c); }
    else {
        int32_t t = S16(o + 506);
        if (t == 0) { ARG(0) = o; c->r[31] = 0x0887A6D0u; f_0887B1A8(c); }
        else {
            *H(o + 506) = (uint16_t)(t - 1);
            if (S16(o + 506) == 0) { ARG(0) = o; c->r[31] = 0x0887A6F0u; f_0887B1A8(c); }
            else {
                ARG(0) = o; c->r[31] = 0x0887A700u; f_0886CF90(c);
                if (c->r[2] != 0) counter_start(c, o, 0x0887A744u);
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
