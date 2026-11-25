/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 200
   instructions: another mode screen's set-up by the chosen mode. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_08823338(Cpu *c); void f_088A9AD8(Cpu *c);
void f_088AC3A8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t from, uint32_t slot, uint32_t count, uint32_t back) {
    ARG(0) = 2; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = count;
    c->r[31] = back; f_0880AEA8(c);
}

/* +78 of the mode's list-0x36 object by the mode (jump table 0x089FF5F0). */
static void mode_kind(gaddr n, uint32_t mode) {
    switch (mode) {
        case 5: *B(n + 78) = 7; break;
        case 6: *B(n + 78) = 8; break;
        case 8: *B(n + 78) = 4; break;
        case 11: *B(n + 78) = 6; break;
        case 14: *B(n + 78) = 9; break;
        case 17: *B(n + 78) = 5; break;
        default: break;
    }
}

/* The mode's 4 palettes with 0x09525DE3 clear (jump table 0x089FF628): words of 0x08A35058 for the
   listed modes; any other mode leaves the source as the original does (1, or 0 past mode 29). */
static uint32_t mode_palettes(uint32_t mode) {
    static const struct { uint8_t mode, off; } rows[] = {
        { 1, 92 }, { 6, 112 }, { 8, 104 }, { 11, 48 }, { 22, 124 }, { 24, 132 }, { 25, 84 }, { 26, 140 },
        { 27, 136 }, { 28, 120 }, { 29, 128 },
    };
    if (!(mode - 1u < 0x1Du)) return 0;
    for (unsigned k = 0; k < sizeof rows / sizeof rows[0]; k++)
        if (rows[k].mode == mode) return *W(0x08A35058u + rows[k].off);
    return 1;
}

/* (screen): a mode screen's set-up: +8 on, 0x088AC3A8 (the new +8), the tickers reset (0x08B02814 =
   0xFD000000), +9 / +76 / +84 and GAME +10 cleared, +80 = 1, 0x092E89BA = 2, +77 = the mode (0x0951CE52's
   high byte), 0x088A9AD8 (0, 4); from two players list-0x36 objects of types 1 (+78 by the mode) and
   4; two list-7 objects (types 4, 5; none free ends the set-up here); the mode's 4 palettes into slot
   0x20 (0x08A35058 by the mode's low 5 bits, or the mode's own word with 0x09525DE3 clear), then
   0x08A316D8 into slot 1 and 0x08A30718 into slot 0x1F (set 2). */
SFA_FUNC(088AC654, mode_screen_by_mode_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o; c->r[17] = 0;
    uint32_t n8 = *B(o + 8) + 1u;
    *B(o + 8) = (uint8_t)n8;
    call_on(c, n8, 0x088AC67Cu, f_088AC3A8);   /* (a0 is the new +8, as the original leaves it) */
    *H(0x08B02810u + 18) = 0; *H(0x08B02810u + 16) = 0;
    *H(0x08B027E0u + 18) = 0; *H(0x08B027E0u + 16) = 0;
    *H(0x08B027B0u + 18) = 0; *H(0x08B027B0u + 16) = 0;
    *W(0x08B02810u) = 0; *W(0x08B02814u) = 0;
    *W(0x08B027E0u) = 0; *W(0x08B027E4u) = 0;
    *W(0x08B027B4u) = 0; *W(0x08B027B0u) = 0;
    *B(o + 9) = 0;
    *H(GAME + 10) = 0;
    c->r[17] = 1;
    *W(o + 84) = 0;
    *B(o + 80) = 1;
    *B(0x092E8940u + 122) = 2;
    *W(0x08B02810u) = 0;
    *W(0x08B02814u) = 0xFD000000u;
    *B(o + 77) = (uint8_t)((*H(0x0951CE52u) & 0xFF00) >> 8);
    *B(o + 76) = 0;
    call2(c, 0, 4, 0x088AC72Cu, f_088A9AD8);
    if (!(S8(0x095176F3u) < 2)) {
        call2(c, 0x36, 1, 0x088AC748u, f_08822EAC);
        *B(c->r[2]) = 1;
        mode_kind(c->r[2], *B(o + 77));
        call2(c, 0x36, 4, 0x088AC7CCu, f_08822EAC);
        *B(c->r[2]) = 1;
    }
    for (c->r[17] = 4;;) {
        call2(c, 7, c->r[17] & 0xFF, 0x088AC7E0u, f_08823338);
        c->r[17]++;
        if (c->r[2] == 0) goto done;
        if (!((int32_t)c->r[17] < 6)) break;
    }
    {
        uint32_t mode = *B(o + 77);
        c->r[16] = mode;
        if (S8(0x09525DE3u) != 0) palette(c, *W(0x08A35058u + ((mode & 0x1F) << 2)), 0x20, 4, 0x088AC830u);
        else {
            c->r[16] = mode - 1u < 0x1Du ? (mode - 1u) << 2 : mode - 1u;
            palette(c, mode_palettes(mode), 0x20, 4, 0x088AC924u);
        }
        palette(c, 0x08A316D8u, 1, 2, 0x088AC944u);
        palette(c, 0x08A30718u, 0x1F, 1, 0x088AC960u);
    }
done:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
