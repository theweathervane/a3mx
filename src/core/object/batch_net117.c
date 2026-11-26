/* Small network functions the link scenarios reach, 9 - 43 instructions: a socket closed / flushed by
   id, the peer list shut down, a peer's state by id, the handshake sent. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DD2A8(Cpu *c); void f_088DF6D4(Cpu *c); void f_088DFB68(Cpu *c); void f_088DC328(Cpu *c);
void f_088DD124(Cpu *c); void f_088DD314(Cpu *c); void f_088E0EF4(Cpu *c); void f_088E0C6C(Cpu *c);
void f_088E0CC8(Cpu *c); void f_088E2D78(Cpu *c);
void hle_sceKernelDeleteSema(Cpu *c);

/* (id): its socket (0x088DD2A8) closed (0x088DF6D4). */
SFA_FUNC(088DF754, link_socket_close_id) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088DF764u; f_088DD2A8(c);
    ARG(0) = c->r[2]; c->r[31] = 0x088DF76Cu; f_088DF6D4(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (id): its socket (0x088DD2A8) flushed (0x088DFB68). */
SFA_FUNC(088DFC10, link_socket_flush_id) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088DFC20u; f_088DD2A8(c);
    ARG(0) = c->r[2]; c->r[31] = 0x088DFC28u; f_088DFB68(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The peer list (0x09523720) emptied (0x088DC328) and its semaphore deleted. */
SFA_FUNC(088DC42C, link_peers_shutdown) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    *W(c->r[29] + 20) = ra;
    c->r[16] = 0x09523720u;
    c->r[31] = 0x088DC444u; f_088DC328(c);
    if (*W(0x09523720u + 8) != 0) {
        ARG(0) = *W(0x09523720u + 8);
        c->r[31] = 0x088DC458u; hle_sceKernelDeleteSema(c);
        *W(0x09523720u + 8) = 0;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (id): the link's state (0x095236C0) when set or id is ours (0x088DD124), else that peer's (0x088DD314)
   state (0x088E0EF4). */
SFA_FUNC(088E0F98, link_peer_state_id) {
    uint32_t id = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0x095236C0u;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = id;
    uint32_t r;
    if (*W(0x095236C0u) != 0) r = *W(0x095236C0u);
    else {
        c->r[31] = 0x088E0FC4u; f_088DD124(c);
        if (c->r[2] == id) r = *W(0x095236C0u);
        else {
            ARG(0) = id; c->r[31] = 0x088E0FD4u; f_088DD314(c);
            ARG(0) = id; ARG(1) = c->r[2];
            c->r[31] = 0x088E0FE0u; f_088E0EF4(c);
            r = c->r[2];
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (session, link): 0x088E0C6C (0), the handshake word 0x1CE14355 sent on the link's queue (+0xAC,
   0x088E0CC8 with its +228); once sent, +43 = 1, the link's counts (+0xD4: +2 on, +4 / +6 / +8 cleared)
   and its wait (0x088E2D78) answered; else 0. */
SFA_FUNC(088E2E9C, link_handshake_send) {
    gaddr o = ARG(0), l = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0;
    c->r[16] = l;
    *W(sp + 24) = s1; *W(sp + 28) = s2;
    c->r[17] = o;
    gaddr st = l + 0xD4;
    c->r[18] = st;
    *W(sp + 32) = ra;
    ARG(0) = 0; c->r[31] = 0x088E2EC4u; f_088E0C6C(c);
    *W(sp + 16) = 0x1CE14355u;
    ARG(0) = l + 0xAC; ARG(1) = sp + 0x10; ARG(2) = 0; ARG(3) = *W(l + 228);
    c->r[31] = 0x088E2EE4u; f_088E0CC8(c);
    if ((int32_t)c->r[2] <= 0) c->r[2] = 0;
    else {
        *B(o + 43) = 1;
        *H(st + 8) = 0;
        *H(st + 2) = (uint16_t)(*H(st + 2) + 1);
        *H(st + 6) = 0; *H(st + 4) = 0;
        ARG(0) = o; ARG(1) = l;
        c->r[31] = 0x088E2F14u; f_088E2D78(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}
