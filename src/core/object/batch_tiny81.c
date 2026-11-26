/* Small functions the scenarios reach, 96 - 99 instructions: a mode screen's start with its
   sounds and five list-0x9A objects, a fighter's palette for its colour mode. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08814260(Cpu *c); void f_08822EAC(Cpu *c); void f_08823338(Cpu *c); void f_0880AEA8(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (screen): with a record picked (0x08A05894 not negative) its 0x354 bytes (0x09519588 + 0x354 each)
   copied to 0x0951CAD0; +4 = 2, 0x08814260, sounds 6 / 0x10 - 0x14, 6 / 0x19 and 0x10 / 4, five list-0x9A
   objects (0x08822EAC (0x9A, 0 / 1 / 1 / 1 / 2), +10 = 0 / 0 / 1 / 2 / 0), 0x09524A28 - 2B = -1 / 0 / 3 / 0,
   0x09524A30 / +14 / +193 cleared. */
SFA_FUNC(08832C5C, mode_screen_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    int32_t sel = (int32_t)*W(0x08A05894u);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[17] = 2;
    ARG(0) = (uint32_t)sel;
    if (sel >= 0) {
        gaddr src = 0x09519588u + (uint32_t)sel * 0x354u, dst = 0x0951CAD0u;
        for (uint32_t i = 0; i < 0x354; i += 4) *W(dst + i) = *W(src + i);
        ARG(0) = dst + 0x350;
    }
    *H(o + 4) = 2;
    c->r[31] = 0x08832CECu; f_08814260(c);
    static const uint32_t snd[7][2] = { {6, 0x10}, {6, 0x11}, {6, 0x12}, {6, 0x13}, {6, 0x14}, {6, 0x19}, {0x10, 4} };
    static const uint32_t sback[7] = { 0x08832CF8u, 0x08832D04u, 0x08832D10u, 0x08832D1Cu, 0x08832D28u, 0x08832D34u, 0x08832D40u };
    for (int i = 0; i < 7; i++) call2(c, snd[i][0], snd[i][1], sback[i], f_08823338);
    static const uint32_t kind[5] = { 0, 1, 1, 1, 2 }, ten[5] = { 0, 0, 1, 2, 0 };
    static const uint32_t oback[5] = { 0x08832D4Cu, 0x08832D60u, 0x08832D70u, 0x08832D80u, 0x08832D90u };
    for (int i = 0; i < 5; i++) {
        call2(c, 0x9A, kind[i], oback[i], f_08822EAC);
        if (i == 0) c->r[18] = 1;
        *B(c->r[2] + 10) = (uint8_t)ten[i];
    }
    *B(0x09524A28u) = 0xFF;
    *B(0x09524A2Bu) = 0;
    *B(0x09524A29u) = 0;
    *B(0x09524A2Au) = 3;
    *H(0x09524A30u) = 0;
    *H(o + 14) = 0;
    *B(o + 193) = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

static void pal0(Cpu *c, uint32_t from, uint32_t slot, uint32_t back) {
    ARG(0) = 0; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* (fighter, colour): its picture table's +128 bank, row colour + 0xC (32 bytes each), loaded by its
   colour mode (+593, 0 - 5) into +13: mode 0 the row, 1 the row + 0xE0, 2 / 3 the row and + 0x1A0 (into
   +13 + 1), 4 the row twice (+13, +13 + 1), 5 the row and + 0x300 (into +13 + 1). */
SFA_FUNC(0886F238, fighter_colour_mode_palette) {
    gaddr f = ARG(0);
    int32_t colour = (int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = f;
    *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = s2;
    gaddr bank = *W(*W(f + 416) + 128);
    c->r[18] = bank;
    int32_t row = (int8_t)(colour + 0xC);
    c->r[17] = (uint32_t)row;
    uint32_t m = (uint32_t)S8(f + 593);
    *W(c->r[29] + 44) = ra;
    if (m < 6) {
        gaddr base = bank + ((uint32_t)row << 5);
        c->r[17] = (uint32_t)row << 5;
        uint32_t c13 = *B(f + 13);
        switch (m) {
        case 1: pal0(c, base + 0xE0, c13, 0x0886F2ECu); break;
        case 2: case 3:
            pal0(c, base, c13, 0x0886F30Cu);
            pal0(c, base + 0x1A0, *B(f + 13) + 1u, 0x0886F32Cu);
            break;
        case 4:
            c->r[17] = base;
            pal0(c, base, c13, 0x0886F350u);
            pal0(c, base, *B(f + 13) + 1u, 0x0886F36Cu);
            break;
        case 5:
            pal0(c, base, c13, 0x0886F38Cu);
            pal0(c, base + 0x300, *B(f + 13) + 1u, 0x0886F3ACu);
            break;
        default: pal0(c, base, c13, 0x0886F2C8u); break;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}
