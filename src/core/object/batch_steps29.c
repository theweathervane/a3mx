/* Steps (0.158.0): the last ready function of 201 - 400 instructions: the fight HUD's pop-up cells for
   both sides (three a side, set off by a fighter's +378 / +379 / +380, each held by a timer that then
   colours it). Offsets stay numbers until something shows what they are.
 */
#include "step.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define IDLE      0xF3          /* a cell waiting */
#define VALUES    0x08A26DA0u   /* the pop-up's value by kind & 0xF */

/* A side's timer (HUD +slot: cell number, count, value): when it runs out the cell (HUD + 0x34 + 64 n)
   takes the value (with colour 0x83 first on side 2), coloured 0x82 for 205 - 207 and 0x81 for 207 -
   212. */
static void timer(gaddr o, uint32_t slot) {
    uint8_t n = *B(o + slot + 1);
    if (n == 0) return;
    *B(o + slot + 1) = (uint8_t)(n - 1);
    if (*B(o + slot + 1) != 0) return;
    gaddr cell = o + (uint32_t)*B(o + slot) * 64 + 0x34;
    *H(cell + 60) = *H(o + slot + 2);
    if (slot == 48) *B(cell + 2) = 0x83;
    int32_t v = *H(cell + 60);
    if (v >= 205 && v < 208) { *B(cell + 2) = 0x82; v = *H(cell + 60); }
    if (!(v < 207) && v < 213) *B(cell + 2) = 0x81;
}

/* A waiting cell set off by kind k: colour 0x83, the timer (kind number, 2 frames, value from 0x08A26DA0
   plus 10 for bit 6 and 20 for bit 5), shown as 0xF5 with bits 6 / 7, else 0xF4. */
static void pop(gaddr o, gaddr cell, int32_t k, uint32_t slot, uint8_t number) {
    if (*H(cell + 60) != IDLE || k == 0) return;
    *B(cell + 2) = 0x83;
    *B(o + slot) = number; *B(o + slot + 1) = 2;
    *H(o + slot + 2) = *H(VALUES + (uint32_t)(k & 0xF) * 2);
    if (k & 0x40) *H(o + slot + 2) = (uint16_t)(*H(o + slot + 2) + 0xA);
    if (k & 0x20) *H(o + slot + 2) = (uint16_t)(*H(o + slot + 2) + 0x14);
    *H(cell + 60) = (k & 0xC0) ? 0xF5 : 0xF4;
}

/* A side's three cells (HUD + base, + 0x40, + 0x80) by its fighter's +378 / +379 / +380; while the other
   fighter has +309 only the first, by the last of them that is set (+378 when not 4). */
static void side(gaddr o, gaddr f, gaddr other, uint32_t base, uint32_t slot, uint8_t number) {
    int32_t k = S8(f + 378);
    if (S8(other + 309) != 0) {
        if ((k & 0xF) == 4) k = 0;
        if (S8(f + 379) != 0) k = S8(f + 379);
        if (S8(f + 380) != 0) k = S8(f + 380);
    }
    pop(o, o + base, k, slot, number);
    if (S8(other + 309) != 0) return;
    pop(o, o + base + 0x40, S8(f + 379), slot, (uint8_t)(number + 1));
    pop(o, o + base + 0x80, S8(f + 380), slot, (uint8_t)(number + 2));
}

/* (hud): outside mode 2, both sides' timers run and their cells set off: side 1 (fighter +12, cells
   +0x334, kinds 0xC - 0xE, timer +28) against fighter +32, side 2 (fighter +32, cells +0x974, kinds 0x25 -
   0x27, timer +48) against fighter +12. */
SFA_FUNC(0888F9D4, hud_pop_cells) {
    gaddr o = ARG(0);
    if (S8(GAME + 114) == 2) return;
    gaddr f1 = *W(o + 12), f2 = *W(o + 32);
    timer(o, 28);
    timer(o, 48);
    side(o, f1, f2, 0x334, 28, 0xC);
    side(o, f2, f1, 0x974, 48, 0x25);
}
