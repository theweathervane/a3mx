/* Steps of 93 - 95 instructions: a part character's throw fall with its bank cues by strength
   (0x089BC458), a part shot launched from its owner's hand with its life by flight time (0x089CB974)
   and a puppet with two trailing glows (0x0897D13C). Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0886DDCC(Cpu *c); void f_08862004(Cpu *c);
void f_088D2D74(Cpu *c); void f_088D2DBC(Cpu *c); void f_088D2E04(Cpu *c); void f_088D35D0(Cpu *c);
void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c); void f_088D3968(Cpu *c); void f_088D3EC0(Cpu *c);
void f_089BE80C(Cpu *c); void f_088D3D94(Cpu *c); void f_088D3E9C(Cpu *c); void f_08822D2C(Cpu *c);
void f_08822EAC(Cpu *c); void f_0897CAD4(Cpu *c); void f_0897D3E8(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, gaddr o, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = a; ARG(2) = b; c->r[31] = back; fn(c);
}

/* (character): the part ticked, 0x08862004; when the +82 wait runs out it drops (still, y -1.0 / -0.5,
   +591 cleared) with bank cue 0x15 (strength 0 / 1) or 0x17 (2), +824 marking 1 / 2; falling below 96
   with the mark, 0x089BE80C (0); when 0x088D35D0 gives 1, +73 cleared, +7 on, move 0x22, cue 0x22,
   0x088D36D0, 0x088D3960 (0, 0), 0x088D3968 (-28, 0) and 0x088D2D74 on the opponent; the opponent
   placed. */
SFA_FUNC(089BC458, part_throw_fall) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[3];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s[0] = c->r[16]; c->r[16] = o;
    *W(c->r[29] + 20) = s[1] = c->r[17];
    *W(c->r[29] + 24) = s[2] = c->r[18];
    *W(c->r[29] + 28) = ra;
    gaddr opp = *W(o + 92);
    c->r[18] = opp;
    call1(c, o, f_088D3EC0, 0x089BC478u);
    call1(c, o, f_08862004, 0x089BC480u);
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    c->r[17] = 1;
    if ((int16_t)t == 0) {
        *W(o + 96) = 0; *W(o + 104) = 0; *W(o + 100) = 0xFFFF0000u; *W(o + 108) = 0xFFFF8000u;
        int32_t sg = S8(o + 194);
        *B(o + 591) = 0;
        if (sg == 0) call2(c, o, 0x15, f_088D2E04, 0x089BC4D0u);
        else if (sg == 1) { call2(c, o, 0x15, f_088D2E04, 0x089BC500u); *W(o + 824) = 1; }
        else if (sg == 2) { call2(c, o, 0x17, f_088D2E04, 0x089BC510u); *W(o + 824) = 1; }
    }
    if ((int32_t)*W(o + 100) < 0 && *W(o + 824) == 1 && S16(o + 22) < 96) {
        call2(c, o, 0, f_089BE80C, 0x089BC544u);
        *W(o + 824) = 0;
    }
    call1(c, o, f_088D35D0, 0x089BC550u);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, 0x22, f_088616EC, 0x089BC574u);
        call1(c, o, f_088D2DBC, 0x089BC57Cu);
        call1(c, o, f_088D36D0, 0x089BC584u);
        call3(c, o, 0, 0, f_088D3960, 0x089BC594u);
        call3(c, o, 0xFFFFFFE4u, 0, f_088D3968, 0x089BC5A4u);
        call1(c, opp, f_088D2D74, 0x089BC5ACu);
    }
    call1(c, o, f_0886DDCC, 0x089BC5B4u);
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (part shot): its next state, +255 cleared, its tables (+356 0x08AB3024, +128 0x08AB3C4C, +200
   0x08AB3AC8), 0x088D3E9C (+3), the picture 0x08AB2FDC from strength + 1, the four speeds by strength
   (0x08AB5FE8; x toward the owner's facing) from 33 ahead / 65 up of its owner, the owner's palette
   (+13 shifted by +3) and facing, +344 = -1, +140 = -76, +1 = 1, hit data (+260 = 10, +311 = 2, +312 =
   8), +170 / +171 = 1, and its life (+143) the time to the top (y / fall). */
SFA_FUNC(089CB974, part_shot_launch) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr own = *W(o + 88);
    c->r[17] = own;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 255) = 0;
    *W(o + 356) = 0x08AB3024u; *W(o + 128) = 0x08AB3C4Cu;
    uint32_t k3 = *B(o + 3);
    *W(o + 200) = 0x08AB3AC8u;
    *W(c->r[29] + 24) = ra;
    call2(c, o, k3, f_088D3E9C, 0x089CB9CCu);
    call3(c, o, 0x08AB2FDCu, (uint32_t)(S8(o + 194) + 1), f_088D3D94, 0x089CB9E4u);
    gaddr e = 0x08AB5FE8u + ((uint32_t)S8(o + 194) << 4);
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    int32_t y;
    if (*B(own + 11) == 0) {
        *W(o + 96) = 0u - *W(o + 96);
        *H(o + 18) = (uint16_t)(S16(own + 18) - 0x21);
    } else *H(o + 18) = (uint16_t)(S16(own + 18) + 0x21);
    y = S16(own + 22) + 0x41;
    DIV(c, *W(o + 100), 0u - *W(o + 108));
    *H(o + 22) = (uint16_t)y;
    *B(o + 12) = *B(own + 12);
    *B(o + 13) = (uint8_t)(*B(own + 13) + *B(o + 3));
    uint32_t f = *B(own + 11);
    *B(o + 141) = 0; *B(o + 11) = (uint8_t)f;
    *B(o + 344) = 0xFF; *B(o + 140) = 0xB4; *B(o + 1) = 1;
    *B(o + 260) = 0xA; *B(o + 311) = 2; *B(o + 312) = 8;
    *B(o + 170) = 1; *B(o + 171) = 1;
    *B(o + 143) = (uint8_t)c->lo;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* A glow (0x08822EAC 0x1C) tied to the fighter and its puppet (+228) with its side and picture. */
static void tie_glow(gaddr g, gaddr o) {
    *W(g + 88) = o;
    *W(g + 92) = *W(o + 228);
    *H(g + 86) = (uint16_t)S8(o + 337);
    *W(g + 164) = *W(o + 164);
}

/* (fighter): a frame flag steps +7 on (+262 cleared) and spawns a puppet (0x08822D2C 0x21, set up by
   0x0897CAD4: +80 / +346 copied, +255 = 5, +347 / +112 cleared, floor, +10 = +82, +260 = 0xD, +311 =
   2, +312 = 8) kept at +228 and counted, with glows 0x1C / 1 and 0x1C / 2 (that one drawn as 0x1C /
   2), then 0x0897D3E8; ticked. */
SFA_FUNC(0897D13C, move_flag_puppet_glows) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[3];
    int32_t k = S8(o + 77);
    c->r[29] -= 0x20;
    for (int i = 0; i < 3; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    if (k != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 262) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call3(c, o, 0x21, 0, f_08822D2C, 0x0897D17Cu);
        gaddr e = c->r[2];
        c->r[17] = e; c->r[18] = e;
        if (e != 0) {
            c->r[17] = 1;
            *B(e) = 1; *B(e + 1) = 0;
            call2(c, o, e, f_0897CAD4, 0x0897D1A0u);
            *B(e + 80) = *B(o + 80);
            int32_t sg = S8(o + 194);
            *B(e + 255) = 5; *B(e + 346) = (uint8_t)sg; *B(e + 347) = 0;
            *H(e + 112) = 0;
            *H(e + 132) = *H(o + 132);
            *B(e + 10) = (uint8_t)S16(o + 82);
            uint32_t n = *B(o + 536);
            *W(o + 228) = e;
            *B(o + 536) = (uint8_t)(n + 1);
            *B(e + 260) = 0xD; *B(e + 311) = 2; *B(e + 312) = 8;
            ARG(0) = 0x1C; ARG(1) = 1; c->r[31] = 0x0897D204u; f_08822EAC(c);
            gaddr g = c->r[2];
            if (g != 0) {
                *B(g) = 1; *B(g + 1) = 0;
                tie_glow(g, o);
                ARG(0) = 0x1C; ARG(1) = 2; c->r[31] = 0x0897D248u; f_08822EAC(c);
                gaddr g2 = c->r[2];
                if (g2 != 0) {
                    *B(g2) = 1; *B(g2 + 1) = 0; *B(g2 + 2) = 0x1C; *B(g2 + 3) = 2;
                    tie_glow(g2, o);
                }
            }
            call1(c, o, f_0897D3E8, 0x0897D298u);
        }
    }
    call1(c, o, f_08861848, 0x0897D2A0u);
    for (int i = 0; i < 3; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x20;
}
