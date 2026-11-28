/* Two more of the routines the fighters' frame leans on that no test scenario runs: a record of
   a fighter's numbers kept up to date, and the shortcut buttons turned into the buttons they
   stand for. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089F3430(Cpu *c);

/* (a, f, g): g's +112 is set to 0x90. The record a follows the fighter f: while f's +112 is not
   0x90 a count of 0x78 frames is started (a's +25 and +26); it is started again while f's +272
   or +135 is set, and when it runs out f's +112 goes back to 0x90 and its +557 and +555 are
   cleared. Then four of a's numbers take f's when f's differ and are not 0: +30 from f's +126,
   +32 from +782, +34 from +778; and +36 from f's +701, only upwards while f's +702 is set. */
SFA_FUNC(08849F44, numbers_follow) {
    gaddr a = ARG(0), f = ARG(1), g = ARG(2);
    *H(g + 112) = 0x90;
    uint32_t counting = *B(a + 25);
    c->r[11] = 0x90;
    int32_t n30 = S16(a + 30);
    if (counting == 0) {
        c->r[2] = (uint32_t)S16(f + 112);
        if (c->r[2] != 0x90) {
            c->r[11] = 0x78;
            *B(a + 25) = 1;
            *B(a + 26) = 0x78;
        }
    } else {
        uint32_t left = (*B(a + 26) - 1u) & 0xFF;
        *B(a + 26) = (uint8_t)left;
        if (left == 0) {
            *B(a + 25) = 0;
            *H(f + 112) = 0x90;
            *B(f + 557) = 0;
            *B(f + 555) = 0;
            n30 = S16(a + 30);
        } else {
            if (*W(f + 272) != 0) *B(a + 26) = 0x78;
            if (S8(f + 135) != 0) *B(a + 26) = 0x78;
        }
    }
    int32_t n32 = S16(a + 32), n34 = S16(a + 34), n36 = S16(a + 36);
    int32_t v = S8(f + 126);
    if (n30 != v && v != 0) *H(a + 30) = (uint16_t)v;
    c->r[9] = (uint32_t)n32;
    v = S16(f + 782);
    if (n32 != v && v != 0) *H(a + 32) = (uint16_t)v;
    c->r[8] = (uint32_t)n34;
    v = S16(f + 778);
    if (n34 != v && v != 0) *H(a + 34) = (uint16_t)v;
    c->r[7] = (uint32_t)S8(f + 702);
    uint32_t top = *B(f + 701);
    ARG(1) = top;
    if (c->r[7] != 0) {
        c->r[6] = n36 < (int32_t)top;
        if (n36 < (int32_t)top) *H(a + 36) = (uint16_t)top;
    } else {
        c->r[6] = (uint32_t)n36;
        if ((uint32_t)n36 != top) *H(a + 36) = (uint16_t)top;
    }
}

/* (f, buttons): the buttons a fighter is given this frame, with the eight shortcut buttons (bits
   4 to 9, 14 and 15) each replaced by what the player has set for it (the eight halfwords at
   0x09525D88): below 15 a fixed set of buttons (the table at 0x08A13870, copied into the frame),
   from 15 one of the character's 24 or fewer moves (8 bytes each in the list at 0x08A1B3F8 for
   the character, as many as 0x08A1B4A0 says), from 39 one of its second list (0x08A1B4F4,
   0x08A1B59C). What each shortcut is set to is kept in the fighter from +1150. The result, with
   the four directions as given, goes to the fighter's +752 and +764. A 0x40 frame keeping s0,
   s1. */
SFA_FUNC(0884A2D0, shortcut_buttons) {
    gaddr f = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 48) = s0; *W(sp + 52) = s1;
    uint32_t buttons = ARG(1) & 0xFFFF;
    c->r[17] = buttons;
    c->r[16] = f;
    *W(sp + 56) = ra;
    ARG(0) = sp + 0x10; ARG(1) = 0x08A13870u; ARG(2) = 0x1E;
    c->r[31] = 0x0884A2FCu; f_089F3430(c);
    uint32_t ch = (uint32_t)S8(f + 338);
    gaddr moves = *W(0x08A1B3F8u + ch * 4);
    int32_t n_moves = *H(0x08A1B4A0u + ch * 2);
    gaddr moves2 = *W(0x08A1B4F4u + ch * 4);
    int32_t n_moves2 = *H(0x08A1B59Cu + ch * 2);
    ARG(0) = moves; ARG(1) = (uint32_t)n_moves; ARG(2) = moves2; ARG(3) = (uint32_t)n_moves2;
    static const uint16_t from[8] = { 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x4000, 0x8000 };
    uint32_t pressed = 0;
    for (int i = 0; i < 8; i++) if (buttons & from[i]) pressed |= 1u << i;
    c->r[9] = pressed;
    uint32_t out = 0;
    for (uint32_t i = 0; i < 8; i++) {
        if ((pressed & (1u << i)) == 0) continue;
        uint32_t set = *H(0x09525D88u + 2 * i);
        if ((int32_t)set < 15) {
            out |= *H(sp + 16 + 2 * set);
        } else {
            uint32_t m = (set - 15) & 0xFFFF;
            if ((int32_t)m < 24) {
                if ((int32_t)m < n_moves) out |= *H(moves + m * 8);
            } else {
                m = (m - 24) & 0xFFFF;
                if ((int32_t)m < n_moves2) out |= *H(moves2 + m * 8);
            }
        }
        out &= 0xFFFF;
        *H(f + 2 * i + 1150) = (uint16_t)set;
    }
    c->r[8] = out;
    ARG(0) = (buttons & 0xF) | out;
    *H(f + 752) = (uint16_t)ARG(0);
    *H(f + 764) = *H(f + 752);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x40;
}
