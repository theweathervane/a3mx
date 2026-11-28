/* A game mode's start and its first screen's step, and the lobby's pick of the player under the
   cursor. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define F1   0x092E3EA0u
#define F2   0x092E4338u
#define LIST 0x09523720u /* the lobby's list: +0 its first entry, +8 its lock, +12 the cursor */

void f_0887C6E4(Cpu *c); void f_08896808(Cpu *c); void f_0880A39C(Cpu *c); void f_08821FC8(Cpu *c);
void f_088967C4(Cpu *c); void f_08896DE8(Cpu *c); void f_0889682C(Cpu *c); void f_08837690(Cpu *c);
void f_0883771C(Cpu *c); void f_0880B6CC(Cpu *c); void f_0882E264(Cpu *c); void f_08814320(Cpu *c);
void f_08814260(Cpu *c); void f_0881CE4C(Cpu *c); void f_089E11C8(Cpu *c); void f_088969BC(Cpu *c);
void f_0882E0D0(Cpu *c); void f_088D7C1C(Cpu *c); void f_08896B8C(Cpu *c); void f_088939DC(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }

/* (a, b): the mode's first screen: its first step (a's +6 at 0) starts the music 0x100, sets a
   wait of 0x10 frames (+16) and a's counters, and makes the screen with both first fighters;
   later steps count the wait down, and at its end a's step (+4) goes on. A 0x20 frame keeping
   s0, s1. */
SFA_FUNC(08893F38, mode_first_screen) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[6] = *H(a + 6);
    *W(sp + 20) = s1;
    c->r[17] = a;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = b;
    if (c->r[6] == 0) {
        call1(c, 0x100, f_0887C6E4, 0x08893F60u);
        *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
        *H(a + 16) = 0x10;
        *H(a + 10) = 0;
        *H(a + 8) = 0;
        *H(a + 18) = 0;
        *H(a + 20) = 1;
        *H(a + 22) = 0;
        *B(a + 72) = 0;
        *B(a + 43) = 0;
        *B(a + 39) = 0;
        call1(c, b, f_08896808, 0x08893FA0u);
        call0(c, f_0880A39C, 0x08893FA8u);
        call0(c, f_08821FC8, 0x08893FB0u);
        call0(c, f_088967C4, 0x08893FB8u);
        call1(c, F1, f_08896DE8, 0x08893FC4u);
        call1(c, F2, f_08896DE8, 0x08893FD0u);
        call0(c, f_0889682C, 0x08893FD8u);
        call0(c, f_08837690, 0x08893FE0u);
        call0(c, f_0883771C, 0x08893FE8u);
        call0(c, f_0880B6CC, 0x08893FF0u);
    } else {
        int32_t wait = S16(a + 16) - 1;
        *H(a + 16) = (uint16_t)wait;
        if ((int16_t)wait == 0) {
            *H(a + 18) = 0;
            *H(a + 4) = (uint16_t)(*H(a + 4) + 1);
            *H(a + 16) = 0;
            *H(a + 6) = 0;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): the mode's start: the screens before are put away, the stage for b's +102 loaded, a
   block of 0x200 bytes copied to 0x092DF980, a voice asked for, a table of 49 bytes at
   0x08A26F60 set from the one after it with six of its entries (listed at 0x08A26FC2) made -1,
   and twelve words of 0x4000 set in the records at 0x08A28418. Then one of two routines by
   the link's state; with a's +39 set the first screen runs at once, else a's counters are set
   and the music 0x100 started. A 0x20 frame keeping s0..s2. */
SFA_FUNC(08893BE8, mode_start) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = a;
    *W(sp + 20) = s1;
    c->r[17] = b;
    *B(a + 145) = 0;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    call1(c, 0, f_0882E264, 0x08893C10u);
    call0(c, f_08814320, 0x08893C18u);
    call0(c, f_08814260, 0x08893C20u);
    c->r[18] = 1;
    ARG(1) = 1;
    call1(c, (uint32_t)S16(b + 102), f_0881CE4C, 0x08893C30u);
    ARG(1) = *W(0x092D9E20u); ARG(2) = 0x200;
    call1(c, 0x092DF980u, f_089E11C8, 0x08893C4Cu);
    *W(0x092E0980u) = 1;
    ARG(1) = 0x24; ARG(2) = 8;
    call1(c, 0x6A1, f_088969BC, 0x08893C68u);
    *B(a + 56) = 1;
    *B(a + 134) = 0;
    call0(c, f_0882E0D0, 0x08893C74u);
    for (uint32_t i = 0; i < 49; i++) *B(0x08A26F60u + i) = *B(0x08A26F91u + i);
    for (uint32_t i = 0; i < 6; i++) {
        gaddr rec = 0x08A28418u + 6 * i;
        *H(rec + 4) = 0x4000;
        *B(0x08A26F60u + (uint32_t)S8(0x08A26FC2u + i)) = 0xFF;
        *H(rec + 286) = 0x4000;
    }
    call0(c, f_088D7C1C, 0x08893CF8u);
    if (c->r[2] != 0) call1(c, a, f_08896B8C, 0x08893D08u);
    else call1(c, a, f_088939DC, 0x08893D18u);
    if (S8(a + 39) != 0) {
        ARG(1) = *H(a + 4) + 1u;
        *H(a + 4) = (uint16_t)ARG(1);
        ARG(1) = b;
        call1(c, a, f_08893F38, 0x08893D3Cu);
    } else {
        *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
        *B(a + 72) = 0;
        *B(a + 43) = 0;
        *H(a + 10) = 0;
        *H(a + 8) = 0;
        *H(a + 18) = 0;
        *H(a + 16) = 0;
        *H(a + 20) = 1;
        *H(a + 22) = 0;
        call1(c, 0x100, f_0887C6E4, 0x08893D7Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* The lobby's entry under the cursor is picked, with the list's lock held: the cursor is walked
   down the list (and set back to the last entry's place when the list is shorter); the entry
   is returned with its +154 set to 1 when it is a player ready to be picked (+152 above 0 and
   +144 at 1), else 0. A 0x20 frame keeping s0, s1. */
SFA_FUNC(088DC888, lobby_cursor_pick) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = LIST;
    ARG(0) = *W(LIST + 8);
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    ARG(1) = 1;
    if (ARG(0) != 0) { ARG(2) = 0; c->r[31] = 0x088DC8B4u; hle_sceKernelWaitSema(c); }
    int32_t cursor = S16(LIST + 12);
    gaddr entry = *W(LIST);
    c->r[16] = entry;
    for (int32_t left = cursor; left > 0; left--) {
        if (entry == 0) { *H(LIST + 12) = (uint16_t)(cursor - left); break; }
        if (*W(entry) != 0) entry = *W(entry);
        c->r[16] = entry;
    }
    if (entry != 0 && S16(entry + 152) > 0 && *W(entry + 144) == 1) *B(entry + 154) = 1;
    else entry = 0;
    c->r[16] = entry;
    ARG(0) = *W(LIST + 8);
    if (ARG(0) != 0) { ARG(1) = 1; c->r[31] = 0x088DC930u; hle_sceKernelSignalSema(c); }
    c->r[2] = c->r[16];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
