/* A menu on the grid cursor (0x0883A490): the pad copied, a pick or cancel counted with its cue, or the
   cursor at 0x08B01F58 moved; its column goes to +10 and 0x0883A6E0 draws it. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C4CC(Cpu *c); void f_08821B6C(Cpu *c); void f_0883A6E0(Cpu *c); void f_0883A490(Cpu *c);

static void cue(Cpu *c, uint32_t n, uint32_t back) { ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c); }

/* (menu, a1): the pad by +53 bit 0 (0x08E75BA0, else 0x08E75BCC) copied to the frame; a pick (+12 bits
   7 / 15) +6 on and the cue 0x39; a cancel (bit 8) +6 on, +18 = 1 and the cue 0x2F; otherwise the grid
   cursor 0x08B01F58 moved by the pad. +10 = the cursor's column, then 0x0883A6E0 (a1). A 0x50 frame
   keeping s0..s2. */
SFA_FUNC(0883A490, grid_menu) {
    gaddr o = ARG(0), a1 = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 64) = s1; *W(sp + 68) = s2; *W(sp + 60) = s0; *W(sp + 72) = ra;
    const gaddr cur = 0x08B01F58u;
    c->r[17] = o; c->r[18] = cur; c->r[16] = a1;
    gaddr pad = (S8(o + 53) & 1) ? 0x08E75BA0u : 0x08E75BCCu;
    for (int i = 0; i < 11; i++) *W(sp + 16 + 4 * i) = *W(pad + 4 * i);
    uint32_t pr = *W(sp + 28);
    if (pr & 0x8080) { *H(o + 6) = (uint16_t)(*H(o + 6) + 1); cue(c, 0x39, 0x0883A550u); }
    else if (pr & 0x100) {
        *H(o + 6) = (uint16_t)(*H(o + 6) + 1); *H(o + 18) = 1;
        cue(c, 0x2F, 0x0883A58Cu);
    } else { ARG(0) = cur; ARG(1) = sp + 16; c->r[31] = 0x0883A5A0u; f_08821B6C(c); }
    uint32_t col = *B(cur + 1);
    c->r[18] = col;
    *H(o + 10) = (uint16_t)col;
    ARG(0) = o; ARG(1) = a1; c->r[31] = 0x0883A5B8u; f_0883A6E0(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x50;
}


void f_0882E0D0(Cpu *c); void f_08823338(Cpu *c);

/* (menu, a1): the grid menu's start: a1's +22 from 0x08A1223C by +124 and +118 (when set), +18 / +8 / +10
   cleared, +6 on, the grid cursor at 0x08B01F58 reset (column 0 of 3, no row), 0x0882E0D0, 0x08823338
   (0x11, 0 / 1 / 3) and its first frame. */
SFA_FUNC(0883A3CC, grid_menu_start) {
    gaddr o = ARG(0), a1 = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra; *W(c->r[29] + 16) = s0;
    c->r[17] = o; c->r[16] = a1;
    int32_t k = S8(o + 118);
    if (k != 0) *B(a1 + 22) = *B(0x08A1223Cu + ((*B(o + 124) + (uint32_t)k - 1) & 0xFF));
    *H(o + 18) = 0;
    *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
    *H(o + 8) = 0; *H(o + 10) = 0;
    const gaddr cur = 0x08B01F58u;
    *B(cur + 1) = 0; *B(cur + 2) = 3; *B(cur) = 0xFF; *B(cur + 3) = 0;
    ARG(0) = 0x08B00000u; ARG(1) = cur; ARG(2) = 3; ARG(3) = 0xFF; c->r[31] = 0x0883A44Cu; f_0882E0D0(c);
    ARG(0) = 0x11; ARG(1) = 0; c->r[31] = 0x0883A458u; f_08823338(c);
    ARG(0) = 0x11; ARG(1) = 1; c->r[31] = 0x0883A464u; f_08823338(c);
    ARG(0) = 0x11; ARG(1) = 3; c->r[31] = 0x0883A470u; f_08823338(c);
    ARG(0) = o; ARG(1) = a1; c->r[31] = 0x0883A47Cu; f_0883A490(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
