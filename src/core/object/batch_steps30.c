/* Steps (0.159.0): a leaf that sets a row object up by its kind (+92): the count +128, the flags and
   colours every kind starts from, then the place (+6, +10), size (+14, +26) and shades of the kind.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define ROW_X     0x08A3CA60u   /* kind 1's x by the game's +6 */

/* The three shade quads: +36 - +39, +42 - +45, +48 - +51. */
static void shades(gaddr o, uint8_t a, uint8_t b, uint8_t c) {
    for (int i = 0; i < 4; i++) {
        *B(o + 36 + i) = a;
        *B(o + 42 + i) = b;
        *B(o + 48 + i) = c;
    }
}

static void place(gaddr o, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    *H(o + 6) = x; *H(o + 10) = y; *H(o + 14) = w; *H(o + 26) = h;
}

/* (row object): +128 counts up; +93 = 1, +55 = 6, +56 / +1 / +3 / +12 / +24 and the shades cleared,
   +136 = 0xFF, +98 = 0xFF, and +94 cleared unless the kind is 0, 0xB or 0xE; then kinds 0 - 16 (the
   table at 0x08A000D0) each set their own. */
SFA_FUNC(088CFCCC, row_object_setup) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1; *B(o + 55) = 6; *B(o + 56) = 0; *B(o + 1) = 0; *B(o + 3) = 0;
    *B(o + 136) = 0xFF;
    *H(o + 12) = 0; *H(o + 24) = 0;
    shades(o, 0, 0, 0);
    uint32_t k = *B(o + 92);
    *H(o + 98) = 0xFF;
    if (k != 0 && k != 0xB && k != 0xE) *H(o + 94) = 0;

    switch (k) {
    case 0: case 7:   /* 0 also sets +3 = 2 */
        *B(o + 56) = 0; *B(o + 136) = 0x80;
        if (k == 0) *B(o + 3) = 2;
        *H(o + 12) = 0; *H(o + 24) = 0;
        *H(o + 14) = 0x60; *H(o + 26) = 0x10;
        return;
    case 1: case 9:   /* 1 then takes x from 0x08A3CA60 by the game's +6 */
        place(o, 0x10, 0x20, 0x40, 0x10);
        shades(o, 0xFF, 0, 0xFF);
        if (k == 1) *H(o + 6) = *H(ROW_X + (uint32_t)*H(GAME + 6) * 2);
        return;
    case 2: case 8:
        *B(o + 56) = 1; *B(o + 136) = 0x80;
        place(o, 0x16, 0xCE, 0xA6, 0x10);
        return;
    case 3: case 10:
        *B(o + 136) = 0x80;
        place(o, 0x2C, 0x86, 0x128, 0x28);
        shades(o, 0x80, 0, 0x80);
        return;
    case 4: case 6:
        *B(o + 55) = 7; *B(o + 56) = 7; *B(o + 136) = 0x80;
        place(o, 0x10, k == 4 ? 0x89 : 0x49, 0x160, 0x1B);
        return;
    case 5:
        *B(o + 56) = 1;
        place(o, 0x10, 0x69, 0x160, 0x1B);
        return;
    case 11:
        *B(o + 55) = 7; *B(o + 56) = 7; *B(o + 136) = 0x80;
        place(o, 0x16, 0xCE, 0xA6, 0x10);
        return;
    case 12:          /* y moves up 32 a step by +96 */
        *B(o + 56) = 0; *B(o + 1) = 0; *B(o + 3) = 0; *B(o + 136) = 0x80;
        place(o, 0x10, (uint16_t)(0xBA - S16(o + 96) * 32), 0x9C, 0x18);
        *H(o + 12) = 0; *H(o + 24) = 0;
        *B(o + 131) = 0;
        shades(o, 0xC0, 0xC0, 0xC0);
        return;
    case 13:
        *B(o + 136) = 0x80;
        place(o, 0xD0, 0x54, 0x94, 0x36);
        shades(o, 0x80, 0, 0x80);
        return;
    case 14:          /* two looks by +94 */
        *B(o + 93) = 0; *B(o + 56) = 0; *B(o + 1) = 0; *B(o + 3) = 0; *B(o + 136) = 0x80;
        *H(o + 12) = 0; *H(o + 24) = 0;
        if (S16(o + 94) == 0) {
            shades(o, 0x76, 0x76, 0xBC);
            place(o, 0xA8, 0x8C, 0x30, 0x10);
        } else {
            shades(o, 0x80, 0, 0x80);
            place(o, 0x98, 0x58, 0x50, 0x38);
        }
        return;
    case 15:
        *B(o + 93) = 0;
        place(o, 0x6A, 0x62, 0xAD, 0x10);
        shades(o, 0x80, 0, 0x80);
        return;
    case 16:          /* two looks by +96 */
        *B(o + 93) = 0; *B(o + 136) = 0x7F;
        if (S16(o + 96) == 0) {
            place(o, 0x66, 0x77, 0xB4, 0xC);
            shades(o, 0, 0, 0xFF);
        } else {
            place(o, 0x66, 0x80, 0xB4, 0x50);
            shades(o, 0, 0, 0);
        }
        return;
    default:
        return;
    }
}
