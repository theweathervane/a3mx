/* Small functions the scenarios reach, 108 - 115 instructions: two menus' state resets with their
   objects and palettes. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_088AA37C(Cpu *c); void f_088AC3A8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void pal(Cpu *c, uint32_t list, uint32_t from, uint32_t slot, uint32_t n, uint32_t back) {
    ARG(0) = list; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = n;
    c->r[31] = back; f_0880AEA8(c);
}

static const uint32_t CUR[3] = { 0x08B02810u, 0x08B027E0u, 0x08B027B0u };

/* (menu): counted (+8), 0x088AC3A8, +78 - +80 / +9 / the cursors' +16 / +18 / GAME +10 cleared,
   0x088AA37C; with 0x095176F3 at 8 or more eight list-0x32 objects (6, 7 / 7 with +78 = 0 / 1, four 5s
   with +78 = 0 - 3, 0), the 5s pointing (+88) at the last; then the palette 0x08A31798 into 7 (2). */
SFA_FUNC(088A9FD4, menu_reset_3) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    *W(sp + 36) = sv[1];
    c->r[17] = 0;
    *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3]; *W(sp + 48) = sv[4]; *W(sp + 52) = ra;
    *B(o + 8) = (uint8_t)(*B(o + 8) + 1);
    call_on(c, o, 0x088AA008u, f_088AC3A8);
    *B(o + 80) = 0; *B(o + 79) = 0; *B(o + 78) = 0;
    for (int i = 0; i < 3; i++) { *H(CUR[i] + 18) = 0; *H(CUR[i] + 16) = 0; }
    *B(o + 9) = 0;
    *H(GAME + 10) = 0;
    call_on(c, o, 0x088AA064u, f_088AA37C);
    if (S8(0x095176F3u) >= 8) {
        static const uint32_t kind[8] = { 6, 7, 7, 5, 5, 5, 5, 0 };
        static const int32_t b78[8] = { -1, 0, 1, 0, 1, 2, 3, -1 };
        static const uint32_t back[8] = { 0x088AA084u, 0x088AA098u, 0x088AA0ACu, 0x088AA0C4u,
            0x088AA0DCu, 0x088AA0F4u, 0x088AA110u, 0x088AA12Cu };
        gaddr fives[4];
        c->r[16] = 1;
        for (int i = 0; i < 8; i++) {
            call2(c, 0x32, kind[i], back[i], f_08822EAC);
            gaddr n = c->r[2];
            *B(n) = 1;
            if (b78[i] >= 0) *B(n + 78) = (uint8_t)b78[i];
            if (i >= 3 && i < 7) fives[i - 3] = n;
            if (i == 2) c->r[17] = 1;
            if (i == 7) {
                *W(n + 88) = n;
                for (int k = 0; k < 4; k++) *W(fives[k] + 88) = n;
            }
        }
        c->r[18] = fives[0]; c->r[19] = fives[1]; c->r[17] = fives[2]; c->r[20] = fives[3];
    }
    pal(c, 1, 0x08A31798u, 7, 2, 0x088AA164u);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* (menu): counted (+8), 0x088AC3A8, the cursors and +9 / GAME +10 cleared, GAME +22's low byte = 1;
   with 0x095176F3 at 6 or more list-0x34 objects 6 / 7 / 9 and list-0x36 objects 6 / 8 / 9 (+10 = 1 / 2
   / 3), all on; then five palettes (0x08A2C0E8 / 0x08A2C108 into 5 / 6, 0x08A30358 into 0xF, 0x08A2C148
   into 0, 0x08A30378 into 0x1F). */
SFA_FUNC(088AAED8, menu_reset_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    *W(c->r[29] + 36) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 40) = ra;
    *B(o + 8) = (uint8_t)(*B(o + 8) + 1);
    call_on(c, o, 0x088AAF00u, f_088AC3A8);
    for (int i = 0; i < 3; i++) { *H(CUR[i] + 18) = 0; *H(CUR[i] + 16) = 0; }
    *B(o + 9) = 0;
    for (int i = 1; i < 3; i++) { *W(CUR[i] + 4) = 0; *W(CUR[i]) = 0; }
    *H(GAME + 10) = 0;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
    *W(0x08B02810u) = 0;
    *W(0x08B02810u + 4) = 0;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 1);
    if (S8(0x095176F3u) >= 6) {
        static const uint32_t list[6] = { 0x34, 0x34, 0x34, 0x36, 0x36, 0x36 }, kind[6] = { 6, 7, 9, 6, 8, 9 };
        static const uint32_t back[6] = { 0x088AAF98u, 0x088AAFACu, 0x088AAFBCu, 0x088AAFCCu, 0x088AAFE0u, 0x088AAFF8u };
        c->r[16] = 1;
        for (int i = 0; i < 6; i++) {
            call2(c, list[i], kind[i], back[i], f_08822EAC);
            *B(c->r[2]) = 1;
            if (i >= 3) *B(c->r[2] + 10) = (uint8_t)(i - 2);
        }
    }
    c->r[16] = 0x08A2C008u;
    pal(c, 1, 0x08A2C0E8u, 5, 1, 0x088AB024u);
    pal(c, 1, 0x08A2C108u, 6, 1, 0x088AB03Cu);
    c->r[16] = 0x08A30358u;
    pal(c, 1, 0x08A30358u, 0xF, 1, 0x088AB05Cu);
    pal(c, 1, 0x08A2C148u, 0, 1, 0x088AB078u);
    pal(c, 1, 0x08A30378u, 0x1F, 1, 0x088AB090u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
