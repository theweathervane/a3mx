/* Small functions the scenarios reach, 134 - 139 instructions: a menu's reset with eight list-0x37
   objects and six palettes, a select portrait's start with its character's palette. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_0881CD20(Cpu *c); void f_08822EAC(Cpu *c); void f_08823338(Cpu *c);
void f_088AC3A8(Cpu *c); void f_0882196C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void pal(Cpu *c, uint32_t list, uint32_t from, uint32_t slot, uint32_t n, uint32_t back) {
    ARG(0) = list; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = n;
    c->r[31] = back; f_0880AEA8(c);
}

/* (menu): counted (+8), 0x088AC3A8, the cursors' +16 / +18, +9 / +78 - +80 and GAME +10 cleared,
   0x0881CD20 (1); with 0x095176F3 at 8 or more eight list-0x37 objects (0 - 7: on, +10 = 1, +78 = 0);
   six palettes (0x08A34FF8 into 1, 0x08A31058 +0xE0 / +0x160 / +0x1E0 / +0x260 into 2 / 3 / 4 / 8, list 0
   0x08A35038 into 9), sound 7 / 0x10. */
SFA_FUNC(088ACBB4, menu_reset_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    *W(c->r[29] + 36) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 40) = ra;
    *B(o + 8) = (uint8_t)(*B(o + 8) + 1);
    call_on(c, o, 0x088ACBDCu, f_088AC3A8);
    static const uint32_t cur[3] = { 0x08B02810u, 0x08B027E0u, 0x08B027B0u };
    for (int i = 0; i < 3; i++) { *H(cur[i] + 18) = 0; *H(cur[i] + 16) = 0; }
    *B(o + 9) = 0;
    *H(GAME + 10) = 0;
    *B(o + 80) = 0; *B(o + 79) = 0; *B(o + 78) = 0;
    call_on(c, 1, 0x088ACC34u, f_0881CD20);
    if (S8(0x095176F3u) >= 8) {
        static const uint32_t back[8] = { 0x088ACC54u, 0x088ACC70u, 0x088ACC88u, 0x088ACCA0u,
            0x088ACCB8u, 0x088ACCD0u, 0x088ACCE8u, 0x088ACD00u };
        c->r[16] = 1;
        for (uint32_t i = 0; i < 8; i++) {
            call2(c, 0x37, i, back[i], f_08822EAC);
            gaddr n = c->r[2];
            *B(n) = 1; *B(n + 10) = 1; *B(n + 78) = 0;
        }
    }
    pal(c, 2, 0x08A34FF8u, 1, 1, 0x088ACD28u);
    c->r[16] = 0x08A31058u;
    pal(c, 2, 0x08A31138u, 2, 1, 0x088ACD48u);
    pal(c, 2, 0x08A311B8u, 3, 1, 0x088ACD60u);
    pal(c, 2, 0x08A31238u, 4, 1, 0x088ACD78u);
    pal(c, 2, 0x08A312B8u, 8, 1, 0x088ACD90u);
    pal(c, 0, 0x08A35038u, 9, 1, 0x088ACDACu);
    call2(c, 7, 0x10, 0x088ACDB8u, f_08823338);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (portrait): once GAME +10's low byte is 2, on (+4), +32 = GAME +16's low byte, +148 cleared, +86 =
   0x14, +164 = 1, +9 / +14 / +15 / +38 cleared, +12 = -1, +13 = 0x20, +24 / +26 = 7 / 1; x and side
   from 0x08A2E1A0 by GAME +22's high byte, y 0x70, its animation (0x08A2DCF0 / 0x08A2DD50 +32 by
   0x09525DE3) and its character's (0x08A2E1C0) palette (0x08A35058 by character, or for the other
   table a few named ones) loaded into 0x20 (4 lines). */
SFA_FUNC(088AE73C, portrait_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = GAME;
    *W(c->r[29] + 36) = ra;
    if ((*H(GAME + 10) & 0xFF) == 2) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *H(o + 32) = (uint16_t)((uint32_t)S16(GAME + 16) & 0xFF);
        *W(o + 148) = 0;
        *H(o + 86) = 0x14;
        *W(o + 164) = 1;
        *B(o + 15) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
        *B(o + 12) = 0xFF; *B(o + 13) = 0x20; *B(o + 14) = 0;
        uint32_t h = (uint32_t)((int32_t)((uint32_t)S16(GAME + 22) & 0xFF00) >> 8);
        gaddr e = 0x08A2E1A0u + 4 * h;
        *H(o + 18) = (uint16_t)S16(e);
        *H(o + 22) = 0x70;
        *B(o + 11) = (uint8_t)S16(e + 2);
        *H(o + 24) = 7;
        *H(o + 26) = 1;
        *H(o + 38) = 0;
        uint32_t h2 = (uint32_t)((int32_t)((uint32_t)S16(GAME + 22) & 0xFF00) >> 8);
        int32_t k = S8(0x08A2E1C0u + h2);
        uint32_t from;
        if (S8(0x09525DE3u) != 0) {
            from = *W(0x08A35058u + ((uint32_t)k << 2));
            c->r[16] = from;
            call2(c, o, *W(0x08A2DCF0u + (h2 << 2) + 32), 0x088AE850u, f_0882196C);
        } else {
            uint32_t j = (uint32_t)(k - 1);
            if (j < 0x1D) {
                static const int16_t named[29] = { 164, -1, -1, -1, -1, 112, -1, 104, -1, -1, 48, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, 128, -1, 136, 132, 156, 152, 140, 160 };
                from = named[j] >= 0 ? *W(0x08A35058u + (uint32_t)named[j]) : j << 2;
            } else from = j;
            c->r[16] = from;
            call2(c, o, *W(0x08A2DD50u + (h2 << 2) + 32), 0x088AE940u, f_0882196C);
        }
        pal(c, 1, from, 0x20, 4, 0x088AE958u);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
