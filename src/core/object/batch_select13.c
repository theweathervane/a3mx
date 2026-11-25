/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 80
   instructions: the menus' palettes set each frame and their screen steps (OPTION MODE and the mode
   screens), the select screen's chosen fighter's portrait. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_08808424(Cpu *c); void f_089E0C24(Cpu *c); void f_089E11C8(Cpu *c); void f_0880AEA8(Cpu *c);
void f_088215C4(Cpu *c); void f_088D5E44(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t from, uint32_t slot, uint32_t back) {
    ARG(0) = 2; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* A menu's eight palettes: five from `base` into slots 0, 1, 2, 0xD, 0xE and three from 0x08ACCCD4 into
   slots 0xF - 0x11 (one 0x0880AEA8 each). */
static void menu_palettes(Cpu *c, gaddr base, const uint32_t backs[8]) {
    static const uint8_t slots[5] = { 0, 1, 2, 0xD, 0xE };
    c->r[17] = base;
    for (int k = 0; k < 5; k++) palette(c, base + 0x20u * (uint32_t)k, slots[k], backs[k]);
    c->r[17] = 0x08ACCCB4u;
    for (int k = 0; k < 3; k++) palette(c, 0x08ACCCB4u + 0x20u * (uint32_t)(k + 1), 0xFu + (uint32_t)k, backs[5 + k]);
}

/* (screen): OPTION MODE's screen: its palettes (from 0x08A13128; kind 6 also 0x08ACCD34 into slot 0x12),
   then its step by +3 (0x08A1326C). */
SFA_FUNC(0884644C, option_screen) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o;
    static const uint32_t b[8] = { 0x08846480u, 0x08846498u, 0x088464B0u, 0x088464C8u, 0x088464E0u, 0x08846500u, 0x08846518u, 0x08846530u };
    menu_palettes(c, 0x08A13128u, b);
    if (*B(o + 3) == 6) palette(c, 0x08ACCD34u, 0x12, 0x08846558u);
    gaddr h = *W(0x08A1326Cu + (uint32_t)*B(o + 3) * 4);
    ARG(0) = o; ARG(1) = h;
    c->r[31] = 0x08846578u;
    rt_call_indirect(c, h);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
/* (screen): a mode screen: its palettes (from 0x08A3C6A8) for kinds under 19, 0x088D5E44, its step by
   +3 (0x08A3C8CC), and drawn while +1 is set. */
SFA_FUNC(088CDFE0, mode_screen) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o;
    if (*B(o + 3) < 19) {
        static const uint32_t b[8] = { 0x088CE020u, 0x088CE038u, 0x088CE050u, 0x088CE068u, 0x088CE080u, 0x088CE0A0u, 0x088CE0B8u, 0x088CE0D0u };
        menu_palettes(c, 0x08A3C6A8u, b);
    }
    c->r[31] = 0x088CE0D8u; f_088D5E44(c);
    gaddr h = *W(0x08A3C8CCu + (uint32_t)*B(o + 3) * 4);
    ARG(0) = o; ARG(1) = h;
    c->r[31] = 0x088CE0F8u;
    rt_call_indirect(c, h);
    if (*B(o + 1) != 0) call_on(c, o, 0x088CE10Cu, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (character, colour): the select screen's chosen fighter's portrait: textures 0x70 / 0x71 freed; unless
   the character is -1, slot 21's lists from the copies at 0x08A0562C / D4 / 0x08A0577C, the current
   216-byte record's +202 - +204 / +208 cleared, archive 0x90 + character (0x13 as 0x11, 0x25 as 0x15)
   queued (0x08808424), and the stage's 64 colours (0x0932DE00 + 128 x BATTLE +102) and the colour's 128
   (0x0932E080 + 256 x colour) copied into the palettes at 0x092DFF80 / 0x092E0000 (marked changed). */
SFA_FUNC(08813AC4, select_portrait) {
    int32_t ch = (int32_t)ARG(0);
    uint32_t colour = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = (uint32_t)ch; c->r[16] = colour;
    call_on(c, 0x70, 0x08813AE4u, f_089E0C24);
    call_on(c, 0x71, 0x08813AECu, f_089E0C24);
    if (ch != -1) {
        *W(0x08E75AD8u + 84) = *W(0x08A0562Cu + 84);
        *W(0x092D9D40u + 84) = *W(0x08A056D4u + 84);
        *W(0x092D9E00u + 84) = *W(0x08A0577Cu + 84);
        if (ch == 0x13) ch = 0x11;
        else if (ch == 0x25) ch = 0x15;
        gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
        *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
        call_on(c, (uint32_t)ch + 0x90, 0x08813B98u, f_08808424);
        c->r[17] = 0x092DF980u;
        ARG(0) = 0x092DF980u + 0x600; ARG(1) = 0x0932DE00u + ((uint32_t)(int32_t)S16(BATTLE + 102) << 7); ARG(2) = 0x40;
        c->r[31] = 0x08813BC8u; f_089E11C8(c);
        ARG(0) = 0x092DF980u + 0x680; ARG(1) = 0x0932E080u + (colour << 8); ARG(2) = 0x80;
        c->r[31] = 0x08813BE4u; f_089E11C8(c);
        ARG(0) = 1;
        *W(0x092E0980u) = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
