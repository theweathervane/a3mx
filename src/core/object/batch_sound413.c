/* The game's queue of sound requests: one request carried out, and the queue served each frame.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define QUEUE    0x092D9EF0u /* 64 requests of 8 bytes: +0 what to do, +2 the sound, +4 and +6
                                what goes with it */
#define QUEUE_RD 0x092D9EECu /* the next one to serve */
#define QUEUE_WR 0x092D9EEDu /* where the next one goes */
#define SOUNDS   0x092DA0F0u

void f_08809EF0(Cpu *c); void f_089E2174(Cpu *c); void f_089E22E8(Cpu *c); void f_089E2340(Cpu *c);
void f_089E23A0(Cpu *c); void f_089E23E8(Cpu *c); void f_08809D78(Cpu *c); void f_089E203C(Cpu *c);

/* The request's sound looked up (0 for none), in r2 and r4. */
static uint32_t sound_of(Cpu *c, gaddr req, uint32_t back) {
    ARG(0) = (uint32_t)S16(req + 2);
    c->r[31] = back; f_08809EF0(c);
    ARG(0) = c->r[2];
    return c->r[2];
}

/* (request): one request of the first kind carried out, by the low byte of its first halfword:
   1 plays the sound (with the two bytes of +4 and the byte of +6), 2 stops it, 8 and 9 set one
   of two of its values (the byte of +4), 10 its pitch (+4). The sound engine's answer is
   returned; when it is 0 (done, or nothing to do) the queue moves on to the next request. Any
   other number, and 1, 8, 9 or 10 for a sound that is not there, answers 0x0A000001 and leaves
   the request where it is. A 0x20 frame keeping s0, s1. */
SFA_FUNC(08809C00, sound_request_do) {
    gaddr req = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = req;
    uint32_t what = (uint32_t)S16(req) & 0xFF;
    *W(sp + 20) = s1;
    c->r[17] = 0x0A000001u;
    *W(sp + 24) = ra;
    if (what == 1) {
        if (sound_of(c, req, 0x08809C50u) != 0) {
            int32_t pair = S16(req + 4);
            ARG(1) = (uint32_t)(int8_t)(pair >> 8);
            ARG(2) = (uint32_t)(int8_t)(pair & 0xFF);
            ARG(3) = (uint32_t)(int8_t)S16(req + 6);
            c->r[31] = 0x08809C88u; f_089E2174(c);
            c->r[17] = c->r[2];
        }
    } else if (what == 2) {
        c->r[17] = 0;
        if (sound_of(c, req, 0x08809C9Cu) != 0) {
            c->r[31] = 0x08809CB0u; f_089E22E8(c);
            c->r[17] = c->r[2];
        }
    } else if (what == 8) {
        if (sound_of(c, req, 0x08809CC4u) != 0) {
            ARG(1) = (uint32_t)(int8_t)S16(req + 4); ARG(2) = 0;
            c->r[31] = 0x08809CE4u; f_089E2340(c);
            c->r[17] = c->r[2];
        }
    } else if (what == 9) {
        if (sound_of(c, req, 0x08809CF4u) != 0) {
            ARG(1) = (uint32_t)(int8_t)S16(req + 4); ARG(2) = 0;
            c->r[31] = 0x08809D14u; f_089E23A0(c);
            c->r[17] = c->r[2];
        }
    } else if (what == 10) {
        if (sound_of(c, req, 0x08809D24u) != 0) {
            ARG(1) = (uint32_t)S16(req + 4); ARG(2) = 0;
            c->r[31] = 0x08809D3Cu; f_089E23E8(c);
            c->r[17] = c->r[2];
        }
    }
    ARG(0) = 0x092E0000u;
    if (c->r[17] == 0) {
        *B(QUEUE_RD) = (uint8_t)(*B(QUEUE_RD) + 1);
        *B(QUEUE_RD) = (uint8_t)(*B(QUEUE_RD) & 0x3F);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The queue is served: requests are carried out in turn until one cannot be yet or the queue is
   empty. By the kind in the first halfword's bits 8 to 11: 0 the first kind, 0x100 the second;
   0x400 is turned into the first kind where it stands (the sound 8 further on, and +4 moved by
   0x200 when its high byte is set, else by 0x100, 0x1EB or 0x1D9 for a low byte under 21,
   under 39, or above); a request of -1 ends the frame's serving and is passed over. Then the
   sound engine's state for the sounds' own number (their +28) is read, and one word of it kept
   at their +124. A 0x50 frame keeping s0..s6, with that state at +16. */
SFA_FUNC(08809A4C, sound_queue_serve) {
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 44) = sv[0];
    c->r[16] = 0x092E0000u;
    *W(sp + 48) = sv[1]; *W(sp + 52) = sv[2]; *W(sp + 68) = sv[6];
    c->r[17] = 0x092E0000u;
    *W(sp + 56) = sv[3]; *W(sp + 60) = sv[4]; *W(sp + 64) = sv[5];
    c->r[19] = 0x100; c->r[20] = 0x400; c->r[21] = 0xFFFFFFFFu;
    c->r[18] = QUEUE;
    c->r[22] = SOUNDS;
    *W(sp + 72) = ra;
    for (;;) {
        uint32_t rd = *B(QUEUE_RD);
        if (rd == *B(QUEUE_WR)) break;
        gaddr req = QUEUE + (rd << 3);
        ARG(1) = req;
        int32_t first = S16(req);
        c->r[6] = (uint32_t)first;
        uint32_t kind = (uint32_t)first & 0xF00;
        c->r[7] = kind;
        if (kind == 0) {
            ARG(0) = req; c->r[31] = 0x08809AC0u; f_08809C00(c);
            if (c->r[2] != 0) break;
        } else if (kind == 0x100) {
            ARG(0) = req; c->r[31] = 0x08809AE4u; f_08809D78(c);
            if (c->r[2] != 0) break;
        } else if (kind == 0x400) {
            *H(req) = (uint16_t)((uint32_t)first & 0xF0FF);
            *H(req + 2) = (uint16_t)(S16(req + 2) + 8);
            int32_t with = S16(req + 4);
            if ((with & 0xFF00) != 0) *H(req + 4) = (uint16_t)(with + 0x200);
            else if ((with & 0xFF) < 21) *H(req + 4) = (uint16_t)(with + 0x100);
            else if ((with & 0xFF) < 39) *H(req + 4) = (uint16_t)(with + 0x1EB);
            else *H(req + 4) = (uint16_t)(with + 0x1D9);
        } else if (first == -1) {
            *B(QUEUE_RD) = (uint8_t)(rd + 1);
            *B(QUEUE_RD) = (uint8_t)(*B(QUEUE_RD) & 0x3F);
            break;
        }
    }
    c->r[16] = *W(SOUNDS + 28);
    ARG(1) = sp + 0x10; ARG(0) = c->r[16];
    c->r[31] = 0x08809B90u; f_089E203C(c);
    if (c->r[2] == 0) {
        ARG(0) = *W(sp + 40);
        *W(SOUNDS + 124) = ARG(0);
    }
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x50;
}
