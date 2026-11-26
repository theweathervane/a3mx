/* A menu item's frame (178 instructions): its palette flash, its slide, when it is drawn by the
   menu's state (GAME +16 / +22), where the option screen's arrow sits, and when it moves on.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08821424(Cpu *c); void f_088219B0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

static uint32_t g22(void) { return (uint32_t)S16(GAME + 22) & 0xFF; }
static uint32_t g16hi(void) { return (uint32_t)((int32_t)((uint32_t)S16(GAME + 16) & 0xFF00) >> 8); }

/* (item, kind +3): off (+1); kinds 4 / 5 flash the palette 0x08A34F38 (6 lines into 0x20) while no
   press (GAME +16's high byte); it slides by its y step (+100) and animates (0x088219B0) unless kind 8
   in menu state 8; kinds 0 - 5 are drawn (0x08821424) when not pressed, kinds 0 - 4 outside state 0x20
   and kind 5 in it; kinds 6 / 7 / 9 / 8 are drawn in states with bit 1 / 4 / 2 / any (bit 2 moves the
   arrow: +77 = 1 to (0xC0, 0x80), 2 to (0xD8, 0xF0)); kind 6 in state 3, kind 9 in state 7 and any
   item whose press differs from +32 move on (+4). */
SFA_FUNC(088AEECC, menu_item_frame) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0]; *W(sp + 40) = sv[2];
    c->r[16] = o;
    c->r[18] = GAME;
    *B(o + 1) = 0;
    *W(sp + 44) = sv[3];
    uint32_t k = *B(o + 3);
    c->r[19] = 8;
    *W(sp + 36) = sv[1]; *W(sp + 48) = ra;
    c->r[17] = 7;
    if (g16hi() == 0 && (k == 4 || k == 5)) {
        ARG(0) = 1; ARG(1) = 0x08A34F38u; ARG(2) = 0; ARG(3) = 0x20; c->r[8] = 6;
        c->r[31] = 0x088AEF44u; f_0880AEA8(c);
        k = *B(o + 3);
    }
    if (!(k == 8 && g22() == 8)) {
        *W(o + 20) += *W(o + 100) << 8;
        call_on(c, o, 0x088AEF7Cu, f_088219B0);
        k = *B(o + 3);
    }
    if ((int32_t)k < 6) {
        int hide;
        if (k == 4) hide = g22() == 0x20;
        else if (k == 5) hide = g22() != 0x20;
        else hide = (int32_t)k < 4 && g22() == 0x20;
        if (!hide && g16hi() == 0) { call_on(c, o, 0x088AF008u, f_08821424); k = *B(o + 3); }
    } else {
        int shown;
        if (k == 6 && (g22() & 1)) shown = 1;
        else if (k == 7 && (g22() & 4)) shown = 1;
        else if (k == 9 && (g22() & 2)) shown = 1;
        else shown = k == 8;
        if (shown) {
            if (g22() & 2) {
                int32_t a = S8(o + 77);
                if (a == 1) { *H(o + 86) = 0xC; *W(o + 164) = 0; *H(o + 18) = 0xC0; *H(o + 22) = 0x80; }
                if (a == 2) { *H(o + 86) = 0x14; *W(o + 164) = 1; *H(o + 18) = 0xD8; *H(o + 22) = 0xF0; }
            }
            if (g16hi() == 0) call_on(c, o, 0x088AF0FCu, f_08821424);
        }
    }
    if (g22() == 3 && *B(o + 3) == 6) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    if (g22() == 7 && *B(o + 3) == 9) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    if (((uint32_t)S16(GAME + 16) & 0xFF) != (uint32_t)S16(o + 32)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
