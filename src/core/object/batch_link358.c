/* Three more of the link's pieces: two small packets that carry the player's two bytes, and the
   menu's frame while a room is shown. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DD370(Cpu *c); void f_088E4324(Cpu *c); void f_088E43FC(Cpu *c); void f_088E696C(Cpu *c);
void f_088D732C(Cpu *c); void f_0880A6CC(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (v): the player's +2 (at 0x09523902) becomes v, and a packet of the kind 0 with the two bytes
   (+3 above +2) waits to be sent with 0xF0. A frame keeping s0, s1. */
SFA_FUNC(088E448C, own_byte_2_send) {
    uint32_t v = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = v;
    call0(c, f_088DD370, 0x088E44A4u);
    gaddr own = c->r[2];
    c->r[17] = own;
    call2(c, 0, 0xFFFFFFFFu, f_088E4324, 0x088E44B4u);
    *B(own + 2) = (uint8_t)v;
    *W(c->r[2]) = ((uint32_t)*B(own + 3) << 16) + *B(own + 2);
    call2(c, c->r[2], 0xF0, f_088E43FC, 0x088E44D8u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The player's +3 counts on, and the same packet waits to be sent. A frame keeping s0. */
SFA_FUNC(088E450C, own_byte_3_next_send) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    call0(c, f_088DD370, 0x088E4520u);
    gaddr own = c->r[2];
    c->r[16] = own;
    call2(c, 0, 0xFFFFFFFFu, f_088E4324, 0x088E4530u);
    uint32_t next = (uint32_t)*B(own + 3) + 1;
    uint32_t low = *B(own + 2);
    *B(own + 3) = (uint8_t)next;
    *W(c->r[2]) = ((next & 0xFF) << 16) + low;
    call2(c, c->r[2], 0xF0, f_088E43FC, 0x088E455Cu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (menu): the menu's frame: its own row (0x088E696C) for the choices 2 and 4 of +43 and the room
   shown (0x088D732C) for the others; then the line of text the choices name in 0x08A3EA04. A
   0x30 frame keeping s0, s1. */
SFA_FUNC(088D7468, menu_frame_by_43) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0;
    c->r[16] = p;
    uint32_t pick = *B(p + 43);
    *W(sp + 36) = s1;
    c->r[17] = 0x08A3EA04u;
    *W(sp + 40) = ra;
    if (pick == 2 || pick == 4) call1(c, p, f_088E696C, 0x088D74B8u);
    else call1(c, p, f_088D732C, 0x088D74DCu);
    uint32_t text = *W(0x08A3EA04u + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2));
    c->r[16] = text;
    ARG(3) = 4; c->r[8] = 9;
    ARG(2) = 0x30;
    call2(c, text, 2, f_0880A6CC, 0x088D750Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}
