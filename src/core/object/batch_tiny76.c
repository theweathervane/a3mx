/* Small functions the scenarios reach, 84 - 85 instructions: a menu's state reset with its
   palettes, a select screen page's buffers and palette, a fighter's picture table with two
   list-0x15 helpers on a button pair. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_088AC3A8(Cpu *c); void f_088054F0(Cpu *c);
void f_088075F8(Cpu *c); void f_088152B0(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E04EC(Cpu *c);
void f_089E0540(Cpu *c); void f_089E11C8(Cpu *c); void f_089E1234(Cpu *c); void f_08822F54(Cpu *c);
void f_0886EB48(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void pal1(Cpu *c, uint32_t from, uint32_t slot, uint32_t back) {
    ARG(0) = 1; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* (menu): counted (+8), 0x088AC3A8, its three cursors (0x08B027B0 / 0x08B027E0 / 0x08B02810: +0 /
   +4 / +16 / +18) and +9 cleared, GAME +10 cleared and GAME +22's low byte = 0xC0; with 0x095176F3
   at 2 or more a list-0x34 (8) and a list-0x36 (7, +78 = 1) object, both on; then three palettes
   (0x08A2C128 / 0x08A2C148 into 0, 0x08A30378 into 0x1F). */
SFA_FUNC(088AB29C, menu_reset) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    *W(c->r[29] + 36) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 40) = ra;
    *B(o + 8) = (uint8_t)(*B(o + 8) + 1);
    call_on(c, o, 0x088AB2C4u, f_088AC3A8);
    static const uint32_t cur[3] = { 0x08B02810u, 0x08B027E0u, 0x08B027B0u };
    for (int i = 0; i < 3; i++) { *H(cur[i] + 18) = 0; *H(cur[i] + 16) = 0; }
    *B(o + 9) = 0;
    for (int i = 0; i < 3; i++) { *W(cur[i] + 4) = 0; *W(cur[i]) = 0; }
    *H(GAME + 10) = 0;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 0xC0);
    if (S8(0x095176F3u) >= 2) {
        call2(c, 0x34, 8, 0x088AB35Cu, f_08822EAC);
        c->r[16] = 1;
        *B(c->r[2]) = 1;
        call2(c, 0x36, 7, 0x088AB370u, f_08822EAC);
        *B(c->r[2]) = 1;
        *B(c->r[2] + 78) = 1;
    }
    pal1(c, 0x08A2C128u, 0, 0x088AB39Cu);
    pal1(c, 0x08A2C148u, 0, 0x088AB3B8u);
    pal1(c, 0x08A30378u, 0x1F, 0x088AB3D8u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* A select screen page's start (between 0x088D7EA0 (1) and (0)): list 34's tile list (0x08A050B0 +136)
   handed to 0x08E75A18 +136, 0x088152B0, its 0x200-byte colours buffer (0x08A0577C +136) and a
   0x1800-byte buffer (0x08A05834 +20) made once (0x089E04EC), the sound slot's flags cleared,
   0x088075F8 (0x08A03140 +96, 0, 0), a wait (0x088054F0 (1)) for 0x08E76068 +4, palette line 0x19
   faded (0x60), the colours copied into line 0x60 (0x089E11C8) and marked changed, the buffer let
   go (0x089E0540). */
SFA_FUNC(0881515C, select_page_buffers) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; *W(sp + 24) = s1; *W(sp + 28) = ra;
    call_on(c, 1, 0x08815174u, f_088D7EA0);
    *W(0x08E75A18u + 136) = *W(0x08A050B0u + 136);
    c->r[31] = 0x08815190u; f_088152B0(c);
    gaddr k = 0x08A0577Cu, b = 0x08A05834u;
    c->r[16] = k;
    c->r[17] = 0x08E76068u;
    if (*W(k + 136) == 0) {
        *W(sp + 16) = b;
        call_on(c, 0x200, 0x088151BCu, f_089E04EC);
        *W(k + 136) = c->r[2];
    }
    if (*W(b + 20) == 0) {
        *W(sp + 16) = b;
        call_on(c, 0x1800, 0x088151DCu, f_089E04EC);
        *W(b + 20) = c->r[2];
    }
    uint32_t i = *B(0x08E76084u);
    gaddr e = 216u * i + 0x08E76088u;
    *B(e + 202) = 0; *B(e + 203) = 0;
    *H(e + 208) = 0;
    *B(e + 204) = 0;
    ARG(0) = *W(0x08A03140u + 96); ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x08815230u; f_088075F8(c);
    while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08815244u, f_088054F0);
    call2(c, 0x19, 0x60, 0x0881525Cu, f_089E1234);
    ARG(0) = 0x092E0580u; ARG(1) = *W(k + 136); ARG(2) = 0x10;
    c->r[31] = 0x08815274u; f_089E11C8(c);
    *W(0x092E0980u) = 1;
    if (*W(k + 136) != 0) {
        call_on(c, *W(k + 136), 0x08815290u, f_089E0540);
        *W(k + 136) = 0;
    }
    call_on(c, 0, 0x0881529Cu, f_088D7EA0);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08AA3ADC, 0x0886EB48
   (0x25, 0x08AA3B78), +86 = its side, +164 = 1; a player (+373 clear) in a plain battle (BATTLE +56)
   holding 0xC0 (+362) gets +0x3D8 = 0x08AA2F88 and two list-0x15 helpers (0x08822F54) as its +40 /
   +44 (+78 = 0 / 1) pointing back at it (+88) with its side and +164; +0x3DC = 0x08AA2F24. */
SFA_FUNC(089AFDB0, fighter_table_copy_25) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = s1;
    gaddr q = f + 0x3D8;
    c->r[17] = q;
    *W(c->r[29] + 24) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08AA3ADCu + 4 * n);
    ARG(0) = f; ARG(1) = 0x25; ARG(2) = 0x08AA3B78u;
    c->r[31] = 0x089AFE18u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    if (S8(f + 373) == 0 && S16(BATTLE + 56) == 0 && (*H(f + 362) & 0xC0) == 0xC0) {
        *W(q) = 0x08AA2F88u;
        static const uint32_t back[2] = { 0x089AFE78u, 0x089AFEB4u };
        for (uint32_t k = 0; k < 2; k++) {
            call2(c, 0x15, 0, back[k], f_08822F54);
            gaddr n = c->r[2];
            if (n == 0) break;
            *B(n + 78) = (uint8_t)k;
            *W(n + 88) = f;
            *W(f + (k ? 44 : 40)) = n;
            *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
            *W(n + 164) = *W(f + 164);
        }
    }
    *W(q + 4) = 0x08AA2F24u;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
