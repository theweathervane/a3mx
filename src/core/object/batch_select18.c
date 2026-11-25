/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 90 - 91
   instructions: a fighter's start on the fight (its entry step), the main menu's set-up and its
   flashing title, the select screen's background loaded. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08871358(Cpu *c); void f_08872080(Cpu *c); void f_08872174(Cpu *c); void f_0887335C(Cpu *c);
void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_088AC3A8(Cpu *c); void f_08821368(Cpu *c);
void f_0882196C(Cpu *c); void f_088B01A0(Cpu *c); void f_088054F0(Cpu *c); void f_088075F8(Cpu *c);
void f_088139CC(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E04EC(Cpu *c); void f_089E11C8(Cpu *c);
void f_089E1234(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t set, uint32_t from, uint32_t slot, uint32_t count, uint32_t back) {
    ARG(0) = set; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = count;
    c->r[31] = back; f_0880AEA8(c);
}

/* (battle object, fighter, player): a fighter's start: 0x0887335C, +539 = 1, its entry wait (+586:
   0xC; with the battle object's +80 set, 0x18 for +637, else by the player's +19 0xC / 0x12 / 0x18;
   the CPU (+373) adds +375 / 2 (low 4 bits)) around 0x08872174 (all three), 0x08872080 (fighter,
   player), +592 2 -> 3; then with health (+112) below 0 0x08871358, else its entry step by the player's
   +13 (0x08A21D0C with +702 or +73 set, else 0x08A21C4C). */
SFA_FUNC(088711F0, fighter_start) {
    gaddr o = ARG(0), f = ARG(1), p = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[17] = f; c->r[18] = o; c->r[16] = p;
    call_on(c, f, 0x08871218u, f_0887335C);
    *B(f + 539) = 1;
    uint8_t wait;
    if (S8(o + 80) == 0) wait = 0xC;
    else if (S8(f + 637) != 0) wait = 0x18;
    else {
        uint8_t q = *B(p + 19);
        wait = q == 0 ? 0xC : q == 1 ? 0x12 : 0x18;
    }
    *B(f + 586) = wait;
    call3(c, o, f, p, 0x08871288u, f_08872174);
    if (S8(f + 373) != 0) *B(f + 586) = (uint8_t)(S8(f + 586) + ((S8(f + 375) >> 1) & 0xF));
    call2(c, f, p, 0x088712B8u, f_08872080);
    if (S8(f + 592) == 2) *B(f + 592) = 3;
    if (S16(f + 112) < 0) call2(c, f, p, 0x088712E0u, f_08871358);
    else {
        uint32_t k = (uint32_t)*B(p + 13) << 2;
        int alt = S8(f + 702) != 0 || S8(f + 73) != 0;
        gaddr h = *W((alt ? 0x08A21D0Cu : 0x08A21C4Cu) + k);
        ARG(0) = f; ARG(1) = p; ARG(2) = h;
        c->r[31] = alt ? 0x08871320u : 0x08871340u;
        rt_call_indirect(c, h);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu): the main menu's set-up: +8 on, +84 / +88 cleared, 0x088AC3A8 (the new +8), the three tickers
   (0x08B027B0 / E0 / 0x08B02810) reset (0x08B02814 = 0xF4E00000), +9 / +12 / +78 / +79 and GAME +10
   cleared, +80 = 1, 0x092E89BA = 2; from two players (0x095176F3) a list-0x31 and a list-0x36 object
   (+0 = 1, the second's +78 = 0x0951CE52's high byte); +13 = that byte; the palettes 0x08A31758 into
   slot 1 (two) and 0x08A329D8 into slot 0x1F. */
SFA_FUNC(088A9BF0, main_menu_setup) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = s2; *W(c->r[29] + 44) = ra;
    c->r[16] = o; c->r[17] = 0;
    uint32_t n8 = *B(o + 8) + 1u;
    *B(o + 8) = (uint8_t)n8;
    *W(o + 88) = 0; *W(o + 84) = 0;
    call_on(c, n8, 0x088A9C24u, f_088AC3A8);   /* (a0 is the new +8, as the original leaves it) */
    *H(0x08B02810u + 18) = 0; *H(0x08B02810u + 16) = 0;
    *H(0x08B027E0u + 18) = 0; *H(0x08B027E0u + 16) = 0;
    *H(0x08B027B0u + 18) = 0; *H(0x08B027B0u + 16) = 0;
    *W(0x08B02814u) = 0; *W(0x08B02810u) = 0;
    *B(o + 80) = 0;
    *W(0x08B027E4u) = 0; *W(0x08B027E0u) = 0;
    *B(o + 79) = 0; *B(o + 78) = 0; *B(o + 9) = 0;
    *H(GAME + 10) = 0;
    c->r[18] = 1;
    *B(o + 80) = 1;
    *B(0x092E8940u + 122) = 2;
    *W(0x08B02810u) = 0;
    *W(0x08B02814u) = 0xF4E00000u;
    *B(o + 12) = 0;
    c->r[17] = 0x09520000u;
    if (!(S8(0x095176F3u) < 2)) {
        call2(c, 0x31, 0, 0x088A9CD4u, f_08822EAC); *B(c->r[2]) = 1;
        call2(c, 0x36, 0, 0x088A9CE4u, f_08822EAC); *B(c->r[2]) = 1;
        *B(c->r[2] + 78) = (uint8_t)(*H(0x0951CE52u) >> 8);
    }
    *B(o + 13) = (uint8_t)(*H(0x0951CE52u) >> 8);
    palette(c, 1, 0x08A31758u, 1, 2, 0x088A9D20u);
    palette(c, 1, 0x08A329D8u, 0x1F, 1, 0x088A9D40u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (title): the main menu's title: +82 counts frames; y (+22) by GAME +10's low byte (0xC8 / 0x80 /
   0x38 for 0 / 1 / 2); by GAME +22's high byte: 0 flashes its palette (0x08A321F8, 4 steps) into slot
   0x20, 2 sets the 32 palettes from 0x08A34158 and its animation (+72 of 0x08A2EBA0, or 0x08A2ED04
   with 0x09525DE3 clear, by the byte + 0x08821368's bit 1); then 0x088B01A0, and from frame 24 +4 on. */
SFA_FUNC(088AFD14, main_menu_title) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o; c->r[17] = GAME;
    *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
    uint32_t m = *H(GAME + 10) & 0xFF;
    if (m == 0) *H(o + 22) = 0xC8;
    else if (m == 1) *H(o + 22) = 0x80;
    else if (m == 2) *H(o + 22) = 0x38;
    int32_t hi = ((int32_t)S16(GAME + 22) & 0xFF00) >> 8;
    if (hi == 0) palette(c, 1, 0x08A321F8u + (((uint32_t)(int32_t)S16(o + 82) & 3) << 5), 0x20, 1, 0x088AFDB8u);
    else if (hi == 2) {
        palette(c, 1, 0x08A34158u, 0x20, 0x20, 0x088AFDE0u);
        c->r[31] = 0x088AFDE8u; f_08821368(c);
        uint32_t k = (uint32_t)((((int32_t)S16(GAME + 22) & 0xFF00) >> 8) + (int32_t)(c->r[2] & 2)) << 2;
        c->r[17] = k;
        if (S8(0x09525DE3u) != 0) call2(c, o, *W(0x08A2EBA0u + k + 72), 0x088AFE24u, f_0882196C);
        else call2(c, o, *W(0x08A2ED04u + k + 72), 0x088AFE44u, f_0882196C);
    }
    call_on(c, o, 0x088AFE4Cu, f_088B01A0);
    if (!(S16(o + 82) < 24)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* The select screen's background, under the loader's lock (0x088D7EA0 (1) / (0)): 0x088139CC (0), list
   40's tile set put back (0x08A050B0), a 0x1400-byte buffer for list 40's palettes (0x089E04EC, kept at
   0x08A0581C), 0x089E1234 (0x18, 0x40), slot 10's lists put back, the current 216-byte record's +202 -
   +204 / +208 cleared, 0x088075F8 (0x08A0318C's word, 0, 0), a wait (0x088054F0 (1) a frame) until
   0x08E7606C is set, 0x08E75C48 / 4A / 4C = -1, and 256 colours from the buffer into the palettes at
   0x092E0180 (marked changed). */
SFA_FUNC(08813D04, select_background_load) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    call_on(c, 1, 0x08813D1Cu, f_088D7EA0);
    call_on(c, 0, 0x08813D24u, f_088139CC);
    *W(0x08E75A18u + 160) = *W(0x08A050B0u + 160);
    c->r[17] = 0x08A0577Cu; c->r[16] = 0x08E76068u;
    if (*W(0x08A0577Cu + 160) == 0) {
        call_on(c, 0x1400, 0x08813D5Cu, f_089E04EC);
        *W(0x08A0577Cu + 160) = c->r[2];
    }
    call2(c, 0x18, 0x40, 0x08813D6Cu, f_089E1234);
    *W(0x092D9E00u + 40) = *W(0x08A0577Cu + 40);
    gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
    *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
    uint32_t arc = *W(0x08A03140u + 76);
    *W(0x08E75AD8u + 40) = *W(0x08A0562Cu + 40);
    *W(0x092D9D40u + 40) = *W(0x08A056D4u + 40);
    call3(c, arc, 0, 0, 0x08813DF8u, f_088075F8);
    while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08813E0Cu, f_088054F0);
    *H(0x08E75C48u + 4) = 0xFFFF; *H(0x08E75C48u + 2) = 0xFFFF; *H(0x08E75C48u) = 0xFFFF;
    call3(c, 0x092DF980u + 0x800, *W(0x08A0577Cu + 160), 0x100, 0x08813E48u, f_089E11C8);
    ARG(0) = 1;
    *W(0x092E0980u) = 1;
    call_on(c, 0, 0x08813E5Cu, f_088D7EA0);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
