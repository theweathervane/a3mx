/* Two of the link's screens: a room made, a step at a time, and the list of the players in it.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DF778(Cpu *c); void f_088DD23C(Cpu *c); void f_088DF2F4(Cpu *c); void f_088DEEC4(Cpu *c);
void f_088D5680(Cpu *c); void f_088DEFD8(Cpu *c); void f_088E3F44(Cpu *c); void f_0880A6CC(Cpu *c);
void f_088D68C8(Cpu *c); void f_088DD124(Cpu *c); void f_088E40D8(Cpu *c); void f_088E4B5C(Cpu *c);
void f_088DD114(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (menu): a room is made, a step (+0) at a time: 0 starts (0x088DF778, a wait of 0x168 frames at
   +4) and goes straight on to 1, which makes the room for the player's own name (0x088DD23C,
   0x088DF2F4, 0x088DEEC4); 2 waits for it (0x088DEFD8); 3 lets the wait run out when +44 is 1 and
   goes on to 4, which waits for 0x088E3F44; 5 counts +24 on and starts over. A failure goes to
   0x088D5680; past 5 the menu leaves for the screen at 0x08A3EB14. Every frame the choices' line
   of text and the link's own line are drawn. A 0x40 frame keeping s0, s1, with the room's name
   at sp + 32. */
SFA_FUNC(088D715C, room_make_steps) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 44) = s1;
    uint32_t step = *H(p);
    c->r[17] = step;
    *W(sp + 40) = s0;
    c->r[16] = p;
    *W(sp + 48) = ra;
    ARG(0) = step < 6;
    if (!(step < 6)) {
        *W(p + 20) = 0x08A3EB14u;
        *H(p + 24) = 0; *H(p + 2) = 0; *H(p) = 0;
        *B(p + 47) = 0;
    } else if (step == 0 || step == 1) {
        uint32_t next = step + 1;
        if (step == 0) {
            call0(c, f_088DF778, 0x088D71B4u);
            *H(p) = (uint16_t)(*H(p) + 1);
            *H(p + 4) = 0x168;
            next = (uint32_t)*H(p) + 1;
        }
        c->r[17] = sp + 0x20;
        *H(p) = (uint16_t)next;
        call1(c, 0, f_088DD23C, 0x088D71ECu);
        call2(c, sp + 0x20, c->r[2], f_088DF2F4, 0x088D71F8u);
        call2(c, sp + 0x20, 0, f_088DEEC4, 0x088D7204u);
        if (c->r[2] != 0) call2(c, p, c->r[2], f_088D5680, 0x088D7218u);
    } else if (step == 2) {
        call0(c, f_088DEFD8, 0x088D7228u);
        int32_t r = (int32_t)c->r[2];
        if (r == 0) *H(p) = (uint16_t)(*H(p) + 1);
        else if (r < 0) call2(c, p, (uint32_t)r, f_088D5680, 0x088D7254u);
    } else if (step == 3 || step == 4) {
        int wait = 1;
        if (step == 3) {
            int32_t t = *B(p + 44) == 1 ? S16(p + 4) : 0;
            if (t != 0) { *H(p + 4) = (uint16_t)(t - 1); wait = 0; }
            else *H(p) = (uint16_t)(step + 1);
        }
        if (wait) {
            call0(c, f_088E3F44, 0x088D7290u);
            if (c->r[2] != 0) *H(p) = (uint16_t)(*H(p) + 1);
        }
    } else {
        *H(p + 2) = 0;
        *H(p + 24) = (uint16_t)(*H(p + 24) + 1);
        *H(p) = 0;
        *B(p + 47) = 0;
    }
    ARG(3) = 4; c->r[8] = 9;
    call3(c, *W(0x08A3EA04u + (((uint32_t)*B(p + 42) + *B(p + 43)) << 2)), 2, 0x30, f_0880A6CC, 0x088D7310u);
    call1(c, p, f_088D68C8, 0x088D7318u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x40;
}

/* A label of the list: drawn by 0x0880A6CC while the list is chosen and by 0x088E40D8 while it is
   not, each with its own place to return to. */
static void label(Cpu *c, uint32_t sel, uint32_t text, uint32_t a, uint32_t x, uint32_t y, uint32_t e, uint32_t back, uint32_t back2) {
    ARG(3) = y; c->r[8] = e;
    if (sel != 0) call3(c, text, a, x, f_0880A6CC, back);
    else call3(c, text, a, x, f_088E40D8, back2);
}

/* (menu, sel): the list of a room's players: two labels with their lines (0x088E4B5C; the one
   0x088DD124 names is marked with 0xA), a third label, and in the rooms of three (+42 at 2 or 3)
   with two players or more a fourth label and a third line. The menu's +56 takes sel and its +57
   the lines' colour (0x10 while chosen, 0x13 while not). A 0x50 frame keeping s0..s7 and fp. */
SFA_FUNC(088D6950, room_players_list) {
    gaddr p = ARG(0);
    uint32_t sel = ARG(1);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    for (int i = 0; i < 8; i++) *W(sp + 40 + 4 * i) = sv[i];
    *W(sp + 72) = sv[8]; *W(sp + 76) = sv[9];
    c->r[19] = sel; c->r[20] = p;
    call0(c, f_088DD124, 0x088D6988u);
    uint32_t mine = c->r[2];
    uint32_t y = sel + 0x5E;
    uint32_t colour = sel != 0 ? 0x10 : 0x13;
    *W(sp + 32) = mine;
    c->r[21] = y; c->r[17] = y - 0x10; c->r[22] = y + 5; c->r[23] = y + 3;
    c->r[18] = 0x08A01038u; c->r[30] = 0x08A01040u;
    *W(sp + 36) = colour;
    label(c, sel, 0x08A01030u, 1, 0x3C, y - 0x10, 4, 0x088D69E0u, 0x088D6A00u);
    c->r[16] = *W(sp + 32) == 0 ? 0xA : 0;
    ARG(3) = y + 5; c->r[8] = *W(sp + 36); c->r[9] = c->r[16];
    call3(c, p, 0, 8, f_088E4B5C, 0x088D6A2Cu);
    c->r[16] = 0xD8;
    label(c, sel, 0x08A01038u, 1, 0x10C, y - 0x10, 4, 0x088D6A4Cu, 0x088D6A6Cu);
    c->r[9] = *W(sp + 32) == 1 ? 0xA : 0;
    c->r[18] = 2;
    c->r[17] = *W(sp + 36);
    ARG(3) = y + 5; c->r[8] = c->r[17];
    call3(c, p, 1, 0xD8, f_088E4B5C, 0x088D6AA4u);
    label(c, sel, 0x08A01040u, 2, 0xB0, y + 3, 9, 0x088D6AC4u, 0x088D6AE0u);
    uint32_t kind = *B(p + 42);
    c->r[30] = kind;
    if (kind == 3 || kind == 2) {
        ARG(0) = 3;
        call0(c, f_088DD114, 0x088D6B00u);
        y += 0x29;
        c->r[21] = y;
        if (!((int32_t)c->r[2] < 2)) {
            c->r[16] = 0x70;
            c->r[21] = y + 5;
            label(c, sel, 0x08A01044u, 1, 0xA4, y - 0x10, 4, 0x088D6B3Cu, 0x088D6B5Cu);
            c->r[9] = *W(sp + 32) == 2 ? 0xA : 0;
            ARG(3) = y + 5; c->r[8] = 0x13;
            call3(c, p, 2, 0x70, f_088E4B5C, 0x088D6B88u);
        }
    }
    *B(p + 56) = (uint8_t)sel;
    *B(p + 57) = (uint8_t)c->r[17];
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x50;
}
