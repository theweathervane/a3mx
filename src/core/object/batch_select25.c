/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 23 - 129
   instructions: the fighters' drawing order by the match's line-up, the fight's common pictures
   loaded, the select screen's portrait buffers made, a menu's set-up. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define PICS   0x08E75AD8u
#define TILES  0x092D9D40u
#define PALS   0x092D9E00u
#define KEPT   0x08A055DCu

void f_0886D764(Cpu *c); void f_0886D944(Cpu *c); void f_08808424(Cpu *c); void f_088D7EA0(Cpu *c);
void f_089E1234(Cpu *c); void f_08814320(Cpu *c); void f_089E04EC(Cpu *c); void f_089E0C24(Cpu *c);
void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_088AA84C(Cpu *c); void f_088AC3A8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t from, uint32_t slot, uint32_t count, uint32_t back) {
    ARG(0) = 1; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = count;
    c->r[31] = back; f_0880AEA8(c);
}
static void record_clear(void) {
    gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
    *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
}

/* The fighters' drawing order (0x0886D944) by the match's line-up (BATTLE +56): 0 two fighters
   (0x0886D764), 1 player 1 and two partners, 2 player 2 and two, 3 the partners then players 1 / 2. */
SFA_FUNC(0886D874, fighters_draw_order_by_lineup) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int32_t m = S16(BATTLE + 56);
    if (m == 0) { c->r[31] = 0x0886D8ACu; f_0886D764(c); }
    else if (m == 1) call3(c, 0x092E3EA0u, 0x092E47D0u, 0x092E4C68u, 0x0886D8ECu, f_0886D944);
    else if (m == 2) call3(c, 0x092E4338u, 0x092E47D0u, 0x092E4C68u, 0x0886D914u, f_0886D944);
    else if (m == 3) call3(c, 0x092E47D0u, 0x092E3EA0u, 0x092E4338u, 0x0886D938u, f_0886D944);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* List k's tile and picture lists put back from the copies (0x08A050B0 / 0x08A0562C) when it has none,
   then its archive queued (0x08808424). Returns whether it did. */
static int list_back(Cpu *c, uint32_t k, uint32_t archive, uint32_t back) {
    uint32_t off = k << 2;
    if (*W(0x08E75A18u + off) != 0) return 0;
    *W(0x08E75A18u + off) = *W(0x08A050B0u + off);
    *W(PICS + off) = *W(0x08A0562Cu + off);
    if (k == 17) *W(PALS + off) = *W(0x08A0577Cu + off);
    call_on(c, archive, back, f_08808424);
    return 1;
}

/* The fight's common pictures, under the loader's lock: the current record cleared; lists 9, 7, 17 and
   24 put back and their archives queued (0x15, 1, 2, 0x1A) where missing; with list 17 also textures
   0x20 - 0x2B made (0x089E1234, 0x3F each, 0x62 for 0x2A). */
SFA_FUNC(08813F44, fight_common_load) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    call_on(c, 1, 0x08813F58u, f_088D7EA0);
    record_clear();
    c->r[16] = 0x08E75A18u;
    list_back(c, 9, 0x15, 0x08813FCCu);
    list_back(c, 7, 1, 0x08814004u);
    if (list_back(c, 17, 2, 0x08814054u)) {
        for (uint32_t t = 0; t < 12; t++)
            call2(c, 0x20 + t, t == 10 ? 0x62 : 0x3F, 0x08814060u + 12u * t, f_089E1234);
    }
    list_back(c, 24, 0x1A, 0x0881411Cu);
    call_on(c, 0, 0x08814124u, f_088D7EA0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The select screen's portrait buffers, under the loader's lock: the old portraits freed (0x08814320),
   textures 0x70 / 0x71 freed, 0x08A05074 = 2, three buffers made (0x400, 0x5400, 0x800) as list 13's
   pictures, tiles and palettes (kept at 0x08A055E0 - E8), the current record cleared, the portrait
   archive 0xC0 + character queued (0x13 as 0x11, 0x25 as 0x15) and 0x08E75C4C = -1. The character is
   the one whose turn it is: by BATTLE +64 (1 player 2, 2 / 3 the partners) or BATTLE +112's, else the
   human side's (GAME +53 bit 0: player 1); in mode GAME +114 = 3 always the human side's. */
SFA_FUNC(08814384, select_portrait_buffers) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; *W(sp + 24) = s1; *W(sp + 28) = ra;
    call_on(c, 1, 0x0881439Cu, f_088D7EA0);
    c->r[31] = 0x088143A4u; f_08814320(c);
    gaddr human = (S8(GAME + 53) & 1) ? 0x092E3EA0u : 0x092E4338u;
    gaddr f = 0;
    if (S8(GAME + 114) == 3) f = human;
    else {
        int32_t k = S8(BATTLE + 64);
        if (k == 1) f = 0x092E4338u;
        else if (k == 2) f = 0x092E47D0u;
        else if (k == 3) f = 0x092E4C68u;
        else { f = *W(BATTLE + 112); if (f == 0) f = human; }
    }
    uint32_t ch = (uint32_t)(int32_t)S8(f + 338) & 0xFFFF;
    if (ch == 0x25) ch = 0x15;
    else if (ch == 0x13) ch = 0x11;
    *H(sp + 16) = (uint16_t)ch;
    call_on(c, 0x70, 0x08814498u, f_089E0C24);
    call_on(c, 0x71, 0x088144A0u, f_089E0C24);
    *W(0x08A05070u + 4) = 2;
    call_on(c, 0x400, 0x088144B8u, f_089E04EC); c->r[16] = c->r[2];
    call_on(c, 0x5400, 0x088144C4u, f_089E04EC); c->r[17] = c->r[2];
    call_on(c, 0x800, 0x088144D0u, f_089E04EC);
    *W(KEPT + 4) = c->r[16]; *W(PICS + 52) = c->r[16];
    *W(KEPT + 8) = c->r[17]; *W(TILES + 52) = c->r[17];
    *W(KEPT + 12) = c->r[2]; *W(PALS + 52) = c->r[2];
    record_clear();
    call_on(c, (uint32_t)*H(sp + 16) + 0xC0, 0x08814548u, f_08808424);
    *H(0x08E75C48u + 4) = 0xFFFF;
    call_on(c, 0, 0x08814560u, f_088D7EA0);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu): a menu's set-up: +8 on, 0x088AC3A8 (the new +8), the three tickers reset, +9 and GAME +10
   cleared, GAME +22 = +94 with bits 1 - 2 of 0x0951CE52's high byte as its high byte; from 7 players
   (0x095176F3) six list-0x33 objects (types 0 - 2, +78 = 0 then 1) and a list-0x36 one (type 3);
   0x088AA84C; the palettes 0x08A31898 (two, slot 0), 0x08A32BD8 (slot 2) and 0x08A32A18 (slot 0x1F). */
SFA_FUNC(088AA4AC, menu_setup) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o; c->r[17] = 0;
    uint32_t n8 = *B(o + 8) + 1u;
    *B(o + 8) = (uint8_t)n8;
    call_on(c, n8, 0x088AA4D4u, f_088AC3A8);   /* (a0 is the new +8, as the original leaves it) */
    *H(0x08B02810u + 18) = 0; *H(0x08B02810u + 16) = 0;
    *H(0x08B027E0u + 18) = 0; *H(0x08B027E0u + 16) = 0;
    *H(0x08B027B0u + 18) = 0; *H(0x08B027B0u + 16) = 0;
    *W(0x08B02814u) = 0; *B(o + 9) = 0; *W(0x08B02810u) = 0;
    *W(0x08B027E4u) = 0; *W(0x08B027E0u) = 0;
    *W(0x08B027B4u) = 0; *W(0x08B027B0u) = 0;
    *H(GAME + 10) = 0;
    int32_t hb = (int8_t)((*H(0x0951CE52u) & 0xFF00) >> 8);
    *H(GAME + 22) = (uint16_t)((uint32_t)*B(o + 94) | ((uint32_t)((hb & 6) >> 1) << 8));
    if (!(S8(0x095176F3u) < 7)) {
        static const uint32_t backs[7] = { 0x088AA5B0u, 0x088AA5C8u, 0x088AA5DCu, 0x088AA5F0u, 0x088AA608u, 0x088AA61Cu, 0x088AA630u };
        c->r[16] = 1;
        for (uint32_t n = 0; n < 7; n++) {
            if (n == 3) c->r[17] = 1;
            call2(c, n == 6 ? 0x36 : 0x33, n == 6 ? 3 : n % 3, backs[n], f_08822EAC);
            *B(c->r[2]) = 1;
            *B(c->r[2] + 78) = n < 3 ? 0 : 1;
        }
    } else ARG(0) = 0x33;
    c->r[31] = 0x088AA640u; f_088AA84C(c);
    palette(c, 0x08A31898u, 0, 2, 0x088AA660u);
    palette(c, 0x08A32BD8u, 2, 1, 0x088AA67Cu);
    palette(c, 0x08A32A18u, 0x1F, 1, 0x088AA69Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
