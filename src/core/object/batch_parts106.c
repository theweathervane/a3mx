/* A fighter's picture parts set up for characters 0x11 / 0x13 and the shared ones (221
   instructions). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886E638(Cpu *c);

static void part(Cpu *c, gaddr t, uint32_t off, uint32_t id, uint32_t back) {
    ARG(0) = t + off; ARG(1) = *W(t + 196); ARG(2) = *W(t + 192); ARG(3) = id;
    c->r[31] = back; f_0886E638(c);
}

/* (fighter): its picture table's (+416) parts reset (0x0886E638 with the table's +196 / +192): for
   character 0x11 +0xC8 / +0x308 / +0x548 as 1 / 2 / 3, +0x788 as 4 (5 with +386 = -1), +0x9F0 as 0x14;
   for 0x13 as 6 / 7 / 8, 9 (0xA), 0x18; then for all +0x9D4 - +0x9EC as 0xC - 0x12, +0x9FC as 0x20,
   +0x10 / +0x18 / +0x20 / +0x28 / +0x2C as 0xB / 0x13 / 0x1C / 0x1D / 0x1E, +0x80 and +0x84 (twice) as 0x1F;
   and the table's part pointers (+0 / +4 / +8 / +12 / +20 / +28 / +36, +2504 - +2512). */
SFA_FUNC(0894E548, fighter_parts_set_11_13) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 8; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 48) = sv[8]; *W(sp + 52) = ra;
    c->r[16] = f;
    gaddr t = *W(f + 416);
    c->r[17] = t;
    c->r[18] = t + 0xC8; c->r[19] = t + 0x308; c->r[20] = t + 0x548; c->r[21] = t + 0x788;
    c->r[22] = t + 0x9F0; c->r[23] = t + 0x9FC;
    int32_t ch = S8(f + 338);
    if (ch == 0x11 || ch == 0x13) {
        int a = ch == 0x11;
        part(c, t, 0xC8, a ? 1 : 6, a ? 0x0894E5C0u : 0x0894E674u);
        part(c, t, 0x308, a ? 2 : 7, a ? 0x0894E5D4u : 0x0894E688u);
        part(c, t, 0x548, a ? 3 : 8, a ? 0x0894E5E8u : 0x0894E69Cu);
        if (S8(f + 386) == -1) part(c, t, 0x788, a ? 5 : 0xA, a ? 0x0894E610u : 0x0894E6C4u);
        else part(c, t, 0x788, a ? 4 : 9, a ? 0x0894E630u : 0x0894E6E4u);
        part(c, t, 0x9F0, a ? 0x14 : 0x18, a ? 0x0894E64Cu : 0x0894E700u);
    }
    c->r[16] = t + 0x9D4;
    static const uint32_t off[16] = { 0x9D4, 0x9D8, 0x9DC, 0x9E0, 0x9E4, 0x9E8, 0x9EC, 0x9FC,
        0x10, 0x18, 0x20, 0x28, 0x2C, 0x80, 0x84, 0x84 };
    static const uint32_t id[16] = { 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x20, 0xB, 0x13, 0x1C, 0x1D, 0x1E, 0x1F, 0x1F, 0x1F };
    static const uint32_t back[16] = { 0x0894E720u, 0x0894E734u, 0x0894E748u, 0x0894E75Cu, 0x0894E770u,
        0x0894E784u, 0x0894E798u, 0x0894E7ACu, 0x0894E7C0u, 0x0894E7D4u, 0x0894E7E8u, 0x0894E7FCu,
        0x0894E810u, 0x0894E824u, 0x0894E83Cu, 0x0894E850u };
    for (int i = 0; i < 16; i++) {
        if (i == 14) c->r[30] = t + 0x84;
        part(c, t, off[i], id[i], back[i]);
    }
    for (uint32_t i = 0; i < 3; i++) *W(t + 2504 + 4 * i) = t + 0x9D4;
    *W(t + 20) = t + 0x9C8;
    *W(t) = t + 0xC8;
    *W(t + 4) = t + 0x308;
    *W(t + 8) = t + 0x548;
    *W(t + 12) = t + 0x788;
    *W(t + 28) = t + 0x9F0;
    *W(t + 36) = t + 0x9FC;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
