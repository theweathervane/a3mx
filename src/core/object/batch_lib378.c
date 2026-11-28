/* Six more of the library: a record's quad drawn, the screen's fade and its border drawn each
   frame, the music's requests carried out, the sound core's start, and one more end of the
   routine that begins a drawing list. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E8C8C(Cpu *c); void f_089E7E8C(Cpu *c); void f_089E7E34(Cpu *c); void f_089E8010(Cpu *c);
void f_089E87A8(Cpu *c); void f_089E8BC0(Cpu *c); void f_089E877C(Cpu *c); void f_089E8740(Cpu *c);
void f_089E8914(Cpu *c); void f_089EE300(Cpu *c); void f_089EE24C(Cpu *c); void f_089F3470(Cpu *c);
void f_089E6DAC(Cpu *c); void f_089E75A4(Cpu *c); void f_089E7774(Cpu *c); void f_089E77A0(Cpu *c);
void f_089E77C8(Cpu *c); void f_089E77F4(Cpu *c); void f_089EC9A0(Cpu *c);
void hle_sceKernelDcacheWritebackRange(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
/* The blend the plain things are drawn with: (0, 2, 3, 0, 0). */
static void blend_plain(Cpu *c, uint32_t back) {
    ARG(3) = 0; c->r[8] = 0;
    call3(c, 0, 2, 3, f_089E8C8C, back);
}
static void enter(Cpu *c, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = sv[0]; *W(c->r[29] + 36) = sv[1];
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x30; }

/* (record): a record of the frame's list drawn without a texture: plain blending, the state 9
   off and the state 4 on or off by its +8, then its vertices (+0, +4 of them) as a strip of the
   vertex type 0x80011C. A 0x30 frame keeping s0. */
SFA_FUNC(089E4370, record_plain_draw) {
    gaddr rec = ARG(0);
    uint32_t sv[2];
    enter(c, sv);
    c->r[16] = rec;
    blend_plain(c, 0x089E4398u);
    call1(c, 9, f_089E7E8C, 0x089E43A0u);
    if (*W(rec + 8) != 0) call1(c, 4, f_089E7E34, 0x089E43B4u);
    else call1(c, 4, f_089E7E8C, 0x089E43C8u);
    uint32_t count = *W(rec + 4);
    c->r[16] = *W(rec);
    ARG(3) = 0; c->r[8] = c->r[16];
    call3(c, 4, 0x0080011Cu, count, f_089E8010, 0x089E43ECu);
    leave(c, sv);
}

/* The screen's fade: the colour at 0x08AD7CE0, with its red and blue changed over, in the two
   corners of a rectangle of 480 by 224 at 0x08E67128; drawn with plain blending as a sprite. A
   0x30 frame keeping s0. */
SFA_FUNC(089E4590, fade_draw) {
    const gaddr v = 0x08E67128u;
    uint32_t sv[2];
    sv[0] = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = sv[0];
    c->r[16] = v;
    *H(v + 4) = 0;
    *H(v + 6) = 0;
    uint32_t colour = *W(0x08AD7CE0u);
    uint32_t turned = ((colour & 0xFF) << 16) | (colour & 0xFF00) | ((colour & 0x00FF0000u) >> 16)
                      | (colour & 0xFF000000u);
    *W(v) = turned;
    *H(v + 12 + 4) = 0x1E0;
    *H(v + 12 + 6) = 0xE0;
    *W(v + 12) = turned;
    sv[1] = c->r[31];
    *W(c->r[29] + 36) = sv[1];
    call2(c, v, 0x18, hle_sceKernelDcacheWritebackRange, 0x089E4608u);
    call2(c, 0, 1, f_089E87A8, 0x089E4614u);
    call1(c, 9, f_089E7E8C, 0x089E461Cu);
    call1(c, 4, f_089E7E34, 0x089E4624u);
    blend_plain(c, 0x089E463Cu);
    ARG(3) = 0; c->r[8] = v;
    call3(c, 6, 0x0080011Cu, 2, f_089E8010, 0x089E4658u);
    leave(c, sv);
}

/* The screen itself: the picture the game drew (a texture of 512 by 512, 256 wide in memory, at
   the address in 0x08D65C08) put on the display as one sprite of 480 by 272, or with the byte at
   0x08E65C65 set as the middle 384 of it, leaving bars at the sides. The two corners are at
   0x08E67140. A 0x30 frame keeping s0. */
SFA_FUNC(089E4668, screen_draw) {
    const gaddr v = 0x08E67140u;
    uint32_t sv[2];
    sv[0] = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = sv[0];
    c->r[16] = v;
    uint32_t bars = *B(0x08E65C65u);
    sv[1] = c->r[31];
    *W(c->r[29] + 36) = sv[1];
    *H(v + 8) = 0;
    *H(v + 10) = 0;
    *W(v + 4) = 0xFFFFFFFFu;
    *H(v) = bars != 0 ? 0x30 : 0;
    *H(v + 2) = 0;
    *H(v + 16 + 8) = 0x1E0;
    *H(v + 16 + 10) = 0x110;
    *W(v + 16 + 4) = 0xFFFFFFFFu;
    *H(v + 16) = bars != 0 ? 0x1B0 : 0x1E0;
    *H(v + 16 + 2) = 0xE0;
    call2(c, v, 0x20, hle_sceKernelDcacheWritebackRange, 0x089E470Cu);
    ARG(3) = 0x110;
    call3(c, 0, 0, 0x1E0, f_089E8BC0, 0x089E4720u);
    call1(c, 0, f_089E7E8C, 0x089E4728u);
    call2(c, 0, 0, f_089E87A8, 0x089E4734u);
    call2(c, 7, 1, f_089E877C, 0x089E4740u);
    ARG(3) = 0;
    call3(c, 1, 0, 0, f_089E8740, 0x089E4754u);
    c->r[8] = *W(0x08D65C08u);
    ARG(3) = 0x200;
    call3(c, 0, 0x200, 0x100, f_089E8914, 0x089E4770u);
    call1(c, 9, f_089E7E34, 0x089E4778u);
    call1(c, 4, f_089E7E8C, 0x089E4780u);
    blend_plain(c, 0x089E4798u);
    ARG(3) = 0; c->r[8] = v;
    call3(c, 6, 0x0080011Eu, 2, f_089E8010, 0x089E47B4u);
    leave(c, sv);
}

/* The 32 music requests of the second callback table (0x14 bytes each at 0x09526190) are carried
   out and cleared: kind 1 stops the sequence (+8) and starts it again with its +12 and +16 (the
   volumes 0 and 0x40, once); kind 2 only stops it. A 0x30 frame with the place and the count kept
   on the stack. */
SFA_FUNC(089E5A2C, music_requests_run) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 40) = ra;
    gaddr e = 0x09526190u;
    for (uint32_t i = 0; i < 32; i++, e += 0x14) {
        int32_t kind = (int32_t)*W(e);
        if (kind != 1 && kind != 2) continue;
        *W(sp + 36) = i;
        *W(sp + 32) = e;
        if (kind == 1) {
            call1(c, *W(e + 8), f_089EE300, 0x089E5A68u);
            ARG(3) = 0; c->r[8] = 0x40; c->r[9] = 1;
            call3(c, *W(e + 4), *W(e + 12), *W(e + 16), f_089EE24C, 0x089E5A88u);
            call3(c, e, 0, 0x14, f_089F3470, 0x089E5A98u);
        } else {
            call1(c, *W(e + 8), f_089EE300, 0x089E5ABCu);
            call3(c, e, 0, 0x14, f_089F3470, 0x089E5ACCu);
        }
    }
    c->r[31] = ra; c->r[29] += 0x30;
}

/* One more end of the routine that begins a drawing list (the page of the drawing state in s4):
   the first list ever (the state's +4 clear) also sets the dither matrix at 0x08A02480, the
   command 0x36 (16, 16), the command 0x53 (7), and the three float commands with the constant at
   0x08A02DC8, and marks the state. The list's own beginning follows. */
SFA_FUNC(089E6D9C, tail_list_begin_first) {
    c->r[16] = c->r[20] + 0xFFFF9300u;
    gaddr st = c->r[16];
    c->r[20] = *W(st + 4);
    c->r[2] = 0x08A00000u;
    if (c->r[20] == 0) {
        call1(c, 0x08A02480u, f_089E75A4, 0x089E6E70u);
        call2(c, 0x10, 0x10, f_089E7774, 0x089E6E7Cu);
        call1(c, 7, f_089E77A0, 0x089E6E84u);
        ARG(1) = 0x08A00000u;
        c->f[20] = *W(0x08A02DC8u);
        c->f[12] = c->f[20];
        c->r[31] = 0x089E6E94u; f_089E77C8(c);
        c->f[12] = c->f[20];
        c->f[13] = c->f[20];
        c->r[31] = 0x089E6EA0u; f_089E77F4(c);
        ARG(2) = 1;
        *W(st + 4) = 1;
    }
    f_089E6DAC(c);
}

/* The sound core's start with 0x100. A 0x10 frame. */
SFA_FUNC(089ED064, sound_core_start_100) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    call1(c, 0x100, f_089EC9A0, 0x089ED074u);
    c->r[31] = ra; c->r[29] += 0x10;
}

void f_089E89EC(Cpu *c); void f_089E8A28(Cpu *c);

/* (record): a record of the frame's list drawn with its texture: the blend by its +26 (0 plain,
   1 the one with (0xA, 0xA, -1, -1)), the state 9 on, the texture (+8, its size at +20 and +22)
   and its format (+24, through the table 0x08AD7EA8); for the formats 0 and 1 the palette (+4,
   with the table 0x08AD7EB8 by +25) is sent when it is not the one last sent (kept at
   0x08AD7EA0); the state 4 on or off by its +12; then its vertices (+0, +16 of them) as a strip
   of the vertex type 0x80011E. A 0x30 frame keeping s0. */
SFA_FUNC(089E50D4, record_textured_draw) {
    gaddr rec = ARG(0);
    uint32_t sv[2];
    enter(c, sv);
    c->r[16] = rec;
    uint32_t blend = *B(rec + 26);
    if (blend == 0) blend_plain(c, 0x089E510Cu);
    else if (blend == 1) {
        ARG(3) = 0xFFFFFFFFu; c->r[8] = 0xFFFFFFFFu;
        call3(c, 0, 0xA, 0xA, f_089E8C8C, 0x089E5130u);
    }
    call1(c, 9, f_089E7E34, 0x089E5138u);
    ARG(3) = *H(rec + 20); c->r[8] = *W(rec + 8);
    call3(c, 0, *H(rec + 20), *H(rec + 22), f_089E8914, 0x089E5150u);
    ARG(3) = 0;
    call3(c, *W(0x08AD7EA8u + ((uint32_t)*B(rec + 24) << 2)), 0, 0, f_089E8740, 0x089E5178u);
    uint32_t format = *B(rec + 24);
    if (format < 2 && !(*B(0x08AD7EA4u) == format && *W(0x08AD7EA0u) == *W(rec + 4))) {
        *W(0x08AD7EA0u) = *W(rec + 4);
        *B(0x08AD7EA4u) = *B(rec + 24);
        ARG(3) = 0;
        call3(c, *W(0x08AD7EB8u + ((uint32_t)*B(rec + 25) << 2)), 0, 0xFF, f_089E89EC,
              format == 0 ? 0x089E5200u : 0x089E526Cu);
        call2(c, format == 0 ? 1 : 0x10, *W(rec + 4), f_089E8A28, format == 0 ? 0x089E520Cu : 0x089E5278u);
    }
    if (*W(rec + 12) != 0) call1(c, 4, f_089E7E34, 0x089E528Cu);
    else call1(c, 4, f_089E7E8C, 0x089E52A0u);
    uint32_t count = *W(rec + 16);
    c->r[16] = *W(rec);
    ARG(3) = 0; c->r[8] = c->r[16];
    call3(c, 4, 0x0080011Eu, count, f_089E8010, 0x089E52C4u);
    leave(c, sv);
}

/* A colour with its red and blue changed over. */
static uint32_t turned(uint32_t colour) {
    return ((colour & 0xFF) << 16) | (colour & 0xFF00) | ((colour & 0x00FF0000u) >> 16) | (colour & 0xFF000000u);
}

/* The screen's background: a black rectangle of 480 by 224 and, over it, the middle 384 with the
   four corner colours at 0x08E65C54; eight vertices of twelve bytes at 0x08E67164, drawn as two
   strips with plain blending and the states 4 and 9 off. A 0x30 frame keeping s0..s2. */
SFA_FUNC(089E5570, background_draw) {
    const gaddr v = 0x08E67164u;
    static const uint16_t xy[8][2] = { { 0, 0 }, { 0x1E0, 0 }, { 0, 0xE0 }, { 0x1E0, 0xE0 },
                                       { 0x30, 0 }, { 0x1B0, 0 }, { 0x30, 0xE0 }, { 0x1B0, 0xE0 } };
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0; *W(sp + 36) = s1; *W(sp + 40) = s2; *W(sp + 44) = ra;
    c->r[16] = v; c->r[17] = v + 0x30;
    for (uint32_t i = 0; i < 8; i++) {
        gaddr p = v + 12 * i;
        *H(p + 4) = xy[i][0];
        *H(p + 6) = xy[i][1];
        *W(p) = i < 4 ? 0xFF000000u : turned(*W(0x08E65C54u + 4 * (i - 4)));
    }
    call2(c, v, 0x60, hle_sceKernelDcacheWritebackRange, 0x089E56D8u);
    call2(c, 0, 0, f_089E87A8, 0x089E56E4u);
    call1(c, 9, f_089E7E8C, 0x089E56ECu);
    call1(c, 4, f_089E7E8C, 0x089E56F4u);
    blend_plain(c, 0x089E570Cu);
    c->r[18] = 0x0080011Cu;
    ARG(3) = 0; c->r[8] = v;
    call3(c, 4, 0x0080011Cu, 4, f_089E8010, 0x089E572Cu);
    ARG(3) = 0; c->r[8] = v + 0x30;
    call3(c, 4, 0x0080011Cu, 4, f_089E8010, 0x089E5744u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}

/* (record): a record of the frame's list drawn by its flags (+12): the same blend; with the top
   bit the texture and palette as above, else the state 9 off; the state 4 by the bit 0x40; and
   its vertices as a fan (bit 0x20) or as lines. A record with no flags at all is drawn as lines
   with both states off. A 0x30 frame keeping s0, s1. */
SFA_FUNC(089E52D4, record_flagged_draw) {
    gaddr rec = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0;
    c->r[16] = rec;
    uint32_t blend = *B(rec + 26);
    *W(sp + 36) = s1;
    c->r[17] = 0x0080011Eu;
    *W(sp + 40) = ra;
    if (blend == 0) blend_plain(c, 0x089E5314u);
    else if (blend == 1) {
        ARG(3) = 0xFFFFFFFFu; c->r[8] = 0xFFFFFFFFu;
        call3(c, 0, 0xA, 0xA, f_089E8C8C, 0x089E533Cu);
    }
    uint32_t flags = *W(rec + 12);
    if (flags == 0) {
        call1(c, 9, f_089E7E8C, 0x089E553Cu);
        call1(c, 4, f_089E7E8C, 0x089E5544u);
        ARG(3) = 0; c->r[8] = *W(rec);
        call3(c, 2, 0x0080011Eu, *W(rec + 16), f_089E8010, 0x089E555Cu);
    } else {
        if (flags & 0x80000000u) {
            call1(c, 9, f_089E7E34, 0x089E535Cu);
            ARG(3) = *H(rec + 20); c->r[8] = *W(rec + 8);
            call3(c, 0, *H(rec + 20), *H(rec + 22), f_089E8914, 0x089E5374u);
            ARG(3) = 0;
            call3(c, *W(0x08AD7EA8u + ((uint32_t)*B(rec + 24) << 2)), 0, 0, f_089E8740, 0x089E539Cu);
            uint32_t format = *B(rec + 24);
            if (format < 2 && !(*B(0x08AD7EA4u) == format && *W(0x08AD7EA0u) == *W(rec + 4))) {
                *W(0x08AD7EA0u) = *W(rec + 4);
                *B(0x08AD7EA4u) = *B(rec + 24);
                ARG(3) = 0;
                call3(c, *W(0x08AD7EB8u + ((uint32_t)*B(rec + 25) << 2)), 0, 0xFF, f_089E89EC,
                      format == 0 ? 0x089E5424u : 0x089E5490u);
                call2(c, format == 0 ? 1 : 0x10, *W(rec + 4), f_089E8A28, format == 0 ? 0x089E5430u : 0x089E549Cu);
            }
        } else call1(c, 9, f_089E7E8C, 0x089E54ACu);
        if (*W(rec + 12) & 0x40) call1(c, 4, f_089E7E34, 0x089E54C4u);
        else call1(c, 4, f_089E7E8C, 0x089E54E0u);
        uint32_t fan = *W(rec + 12) & 0x20;
        uint32_t count = *W(rec + 16);
        c->r[16] = *W(rec);
        ARG(3) = 0; c->r[8] = c->r[16];
        if (fan != 0) call3(c, 5, 0x0080011Eu, count, f_089E8010, 0x089E5510u);
        else call3(c, 2, 0x0080011Eu, count, f_089E8010, 0x089E552Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}
