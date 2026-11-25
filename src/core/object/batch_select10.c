/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 71 - 73
   instructions: the VS portrait for the second fighter, the ending of a vs panel by its row of speeds,
   a ticker picture with its palette, and the two black bands drawn each frame beside the picture.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0889CFE8(Cpu *c); void f_088988FC(Cpu *c); void f_0889CEE4(Cpu *c); void f_0889EF00(Cpu *c);
void f_0880AEA8(Cpu *c); void f_088AD2D8(Cpu *c); void f_0882196C(Cpu *c); void f_088B01A0(Cpu *c);
void f_089E10D8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): the VS portrait's other half: on its first frame its set-up (0x0889CFE8, its picture by
   side, 0x0889CEE4 (0x15 + side)), for side 1's +10 = 1 a start off the right edge (0x1F0, 0x80) sliding
   in, and its step by +10 (0x08A2B158); then at once with BATTLE +81 (0x0889EF00), or moved on at
   GAME +20 high byte 2. */
SFA_FUNC(0889EC8C, vs_portrait_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(o + 148) = 0;
        *H(o + 86) = 8;
        call2(c, o, 0, 0x0889ECC8u, f_0889CFE8);
        call_on(c, *W(o + 92), 0x0889ECD0u, f_088988FC);
        c->r[17] = c->r[2] & 0xFF;
        call2(c, o, (c->r[17] + 0x15) & 0xFFFF, 0x0889ECE4u, f_0889CEE4);
        uint8_t k = *B(o + 10);
        if (c->r[17] != 0 && k == 1) {
            *H(o + 112) = 0x1F0; *H(o + 18) = 0x1F0;
            *H(o + 114) = 0x80;
            *W(o + 96) = 0xFFD4B500u;
            *W(o + 104) = 0x2B400u;
        }
        gaddr h = *W(0x08A2B158u + (uint32_t)k * 4);
        ARG(0) = o; ARG(1) = h;
        c->r[31] = 0x0889ED40u;
        rt_call_indirect(c, h);
    } else if (S8(BATTLE + 81) != 0) call_on(c, o, 0x0889ED64u, f_0889EF00);
    else if (((uint32_t)(int32_t)S16(GAME + 20) & 0xFF00) == 0x200) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): speeds from the 16-byte row by +3 x 2 + +78 of 0x08A2CB78 (each word's low half the
   speed's fraction kept from the row, high half its whole part) and the falls from 0x08A2CBD8 by +3;
   x from the row, y = +100; two palettes from 0x08A316D8 into slot 1; then 0x088AD2D8. */
SFA_FUNC(088ACEEC, vs_panel_speeds) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    uint32_t b3x2 = (uint32_t)*B(o + 3) << 1;
    *B(o + 10) = 0;
    gaddr e = 0x08A2CB78u + ((b3x2 + (uint32_t)(int32_t)S8(o + 78)) << 4);
    uint32_t v96 = (((*W(o + 96) & 0xFFFF0000u) | *W(e)) & 0xFFFF) | (*W(e + 8) << 16);
    uint32_t v104 = (((*W(o + 104) & 0xFFFF0000u) | *W(e + 4)) & 0xFFFF) | (*W(e + 12) << 16);
    *W(o + 96) = v96;
    *W(o + 104) = v104;
    *H(o + 18) = (uint16_t)v96;
    gaddr t = 0x08A2CBD8u + (b3x2 << 1);
    uint32_t v100 = (*W(o + 100) | ((uint32_t)(int32_t)S16(t) << 16)) & 0xFFFF0000u;
    *W(o + 108) = *W(o + 108) | ((uint32_t)(int32_t)S16(t + 2) << 16);
    *W(o + 100) = v100;
    *W(o + 20) = v100;
    ARG(0) = 1; ARG(1) = 0x08A316D8u; ARG(2) = 0; ARG(3) = 1; c->r[8] = 2;
    c->r[31] = 0x088ACFF4u; f_0880AEA8(c);
    call_on(c, o, 0x088ACFFCu, f_088AD2D8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): with GAME +10 set, the object at 0x092E8940 at y -0xB20 and +100 still 0: picture and
   eight palettes by +96 (from 0x08A2EDDC's index: 0x08A35100 / 0x08A2EBA0 while 0x09525DE3 is set,
   else 0x08A35180 / 0x08A2ED04) into slot 0x20, +100 on; then 0x088B01A0. */
SFA_FUNC(088AFB30, ticker_picture) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    if (*H(GAME + 10) != 0 && S16(0x092E8940u + 22) == -0xB20 && *W(o + 100) == 0) {
        uint32_t k = (uint32_t)(int32_t)S8(0x08A2EDDCu + *W(o + 96)) << 2;
        int alt = S8(0x09525DE3u) != 0;
        ARG(1) = *W((alt ? 0x08A35100u : 0x08A35180u) + k);
        ARG(0) = 1; ARG(2) = 0; ARG(3) = 0x20; c->r[8] = 8;
        c->r[31] = alt ? 0x088AFBBCu : 0x088AFC0Cu; f_0880AEA8(c);
        ARG(1) = *W((alt ? 0x08A2EBA0u : 0x08A2ED04u) + (*W(o + 96) << 2));
        call_on(c, o, alt ? 0x088AFBDCu : 0x088AFC2Cu, f_0882196C);
        *W(o + 100) = *W(o + 100) + 1;
    }
    call_on(c, o, 0x088AFC40u, f_088B01A0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* Each frame: two black bands (four flat vertices at 0x08AD92F0, 12 bytes each, colour 0xFF000000,
   depth 0x15), y 8 - 0xE8: from x -0x30 to the float at 0x08E75A04, and from that + 384 x the float at
   0x08E759E8 to x 0x1B0 (0x089E10D8). */
SFA_FUNC(0880A930, side_bands) {
    uint32_t ra = c->r[31], s[5];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    WW(sp + 16, c->f[20]);
    for (int k = 0; k < 5; k++) { s[k] = c->r[16 + k]; *W(sp + 20 + 4 * (uint32_t)k) = s[k]; }
    *W(sp + 40) = ra;
    gaddr v = 0x08AD92F0u;
    c->r[16] = 0x08AE0000u; c->r[17] = v;
    c->f[20] = RW(0x08E75A04u);
    *W(v + 44) = 0xFF000000u; *W(v + 32) = 0xFF000000u; *W(v + 20) = 0xFF000000u; *W(v + 8) = 0xFF000000u;
    int32_t t = CVT_TRUNC(c, FS(c, 20));
    c->f[12] = (uint32_t)t;
    *H(v + 12) = 0xFFD0; *H(v) = 0xFFD0;
    *H(v + 36) = (uint16_t)t; *H(v + 24) = (uint16_t)t;
    c->r[18] = 8; c->r[19] = 0xE8; c->r[20] = 0x15;
    *H(v + 26) = 8; *H(v + 2) = 8;
    *H(v + 38) = 0xE8; *H(v + 14) = 0xE8;
    *H(v + 40) = 0x15; *H(v + 28) = 0x15; *H(v + 16) = 0x15; *H(v + 4) = 0x15;
    ARG(0) = v; ARG(1) = 4; ARG(2) = 0;
    c->r[31] = 0x0880A9CCu; f_089E10D8(c);
    c->f[12] = RW(0x08E759E8u);
    c->f[13] = 0x43C00000u;
    float w = FS(c, 12);
    w = w * FS(c, 13);
    FSET(c, 12, w);
    float x = FS(c, 20);
    x = x + FS(c, 12);
    FSET(c, 12, x);
    *H(v + 36) = 0x1B0; *H(v + 24) = 0x1B0;
    *H(v + 26) = 8;
    int32_t t2 = CVT_TRUNC(c, FS(c, 12));
    c->f[12] = (uint32_t)t2;
    *H(v + 2) = 8;
    *H(v + 38) = 0xE8; *H(v + 14) = 0xE8;
    *H(v + 40) = 0x15;
    *H(v + 12) = (uint16_t)t2; *H(v) = (uint16_t)t2;
    *H(v + 28) = 0x15; *H(v + 16) = 0x15; *H(v + 4) = 0x15;
    ARG(0) = v; ARG(1) = 4; ARG(2) = 0;
    c->r[31] = 0x0880AA30u; f_089E10D8(c);
    c->f[20] = RW(sp + 16);
    for (int k = 0; k < 5; k++) c->r[16 + k] = s[k];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
