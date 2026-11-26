/* Network functions the link scenarios reach, 13 - 38 instructions: the link joined from the menus and
   the game, left when every peer is counted or on demand, and the network shut down. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088E336C(Cpu *c); void f_088E3604(Cpu *c); void f_0880ABD4(Cpu *c); void f_088E2F80(Cpu *c);
void f_088DD3DC(Cpu *c); void f_088E1EC4(Cpu *c); void f_088DCD40(Cpu *c); void f_088DC42C(Cpu *c);
void f_088DC0E0(Cpu *c); void f_08814134(Cpu *c); void f_08814574(Cpu *c); void f_08814320(Cpu *c);
void f_08813770(Cpu *c); void f_0880A6CC(Cpu *c);

#define GAME 0x08E75910u
#define LINK 0x09523920u

/* (task): the link joined for it (0x088E336C with delay 1) and its step (+6) moved on. */
SFA_FUNC(08824404, link_join_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    ARG(1) = 1; c->r[31] = 0x0882441Cu; f_088E336C(c);
    ARG(0) = (uint32_t)*H(o + 6) + 1;
    *H(o + 6) = (uint16_t)ARG(0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (task): once 0x088E3604 says so, the game joins the link (delay 1), the task's step (+6) moves on with
   +8 cleared, and 0x0880ABD4. */
SFA_FUNC(08893DF4, link_join_when_ready) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08893E08u; f_088E3604(c);
    int ready = c->r[2] != 0;
    ARG(0) = 0x08E70000u;
    if (ready) {
        ARG(0) = GAME; ARG(1) = 1;
        c->r[31] = 0x08893E1Cu; f_088E336C(c);
        uint32_t k = *H(o + 6);
        *H(o + 8) = 0;
        ARG(0) = k + 1;
        *H(o + 6) = (uint16_t)(k + 1);
        c->r[31] = 0x08893E30u; f_0880ABD4(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The game leaves the link (0x088E2F80 of 0x09523920), +37 and the game's +89 / +145 cleared. */
SFA_FUNC(088E35B8, link_leave_game) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = LINK;
    *W(c->r[29] + 20) = s1;
    c->r[17] = GAME;
    *W(c->r[29] + 24) = ra;
    ARG(0) = LINK; ARG(1) = GAME;
    c->r[31] = 0x088E35E4u; f_088E2F80(c);
    *B(LINK + 37) = 0;
    *B(GAME + 89) = 0; *B(GAME + 145) = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (link, peer): the peer's goodbye counted (+212) and the link's peer count (+42) up; once every peer
   (0x088DD3DC) is counted the game leaves the link (0x088E2F80). Returns 1. */
SFA_FUNC(088E3014, link_peer_left) {
    gaddr o = ARG(0), peer = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    uint32_t h = *H(peer + 212);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *H(peer + 212) = (uint16_t)(h + 1);
    *B(o + 42) = (uint8_t)(*B(o + 42) + 1);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 42);
    c->r[17] = n;
    ARG(0) = n;
    c->r[31] = 0x088E3048u; f_088DD3DC(c);
    ARG(1) = 0x08E70000u;
    if (n == c->r[2]) {
        ARG(0) = o; ARG(1) = GAME;
        c->r[31] = 0x088E305Cu; f_088E2F80(c);
    }
    c->r[2] = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (net state): the link shut down (worker, session, peers), 0x088DC0E0 and the adhoc layers closed
   (0x08814134, 0x08814574, 0x08814320, 0x08813770); with a block at +8 it is released (0x0880A6CC
   0x08A0104C, 2, 0x98, 4, 9); +0 / +2 / +47 cleared and the generation (+24) counted. */
SFA_FUNC(088D78BC, net_shutdown) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    c->r[31] = 0x088D78D0u; f_088E1EC4(c);
    c->r[31] = 0x088D78D8u; f_088DCD40(c);
    c->r[31] = 0x088D78E0u; f_088DC42C(c);
    c->r[31] = 0x088D78E8u; f_088DC0E0(c);
    c->r[31] = 0x088D78F0u; f_08814134(c);
    c->r[31] = 0x088D78F8u; f_08814574(c);
    c->r[31] = 0x088D7900u; f_08814320(c);
    c->r[31] = 0x088D7908u; f_08813770(c);
    int has = *W(o + 8) != 0;
    ARG(0) = 0x08A00000u;
    if (has) {
        ARG(0) = 0x08A0104Cu; ARG(1) = 2; ARG(2) = 0x98; ARG(3) = 4; c->r[8] = 9;
        c->r[31] = 0x088D792Cu; f_0880A6CC(c);
    }
    uint32_t g = *H(o + 24);
    *H(o + 2) = 0;
    ARG(0) = g + 1;
    *H(o + 24) = (uint16_t)(g + 1);
    *H(o) = 0;
    *B(o + 47) = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
