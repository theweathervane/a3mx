/* Seven more of the library: the sound's voices set up and gathered by what they play, the sound
   requests carried out, and the drawing buffer chosen. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GE      0x08E69300u /* the drawing state */
#define VOICES  0x08E74534u /* the sound's voices, 0x68 bytes each */
#define VOICE_N 0x08E74508u /* how many there are */
#define VOICE_UP 0x08AD81ECu /* -1 until they are set up */

void f_089ECADC(Cpu *c); void f_089ECCAC(Cpu *c); void f_089ECBF8(Cpu *c); void f_089ECC5C(Cpu *c);
void f_089ECDC0(Cpu *c); void f_089ECD54(Cpu *c); void f_089ECAA0(Cpu *c); void f_089F3470(Cpu *c);
void f_089EAB30(Cpu *c); void f_089F2898(Cpu *c); void f_089F22D4(Cpu *c); void f_089F23A4(Cpu *c);
void f_089F2460(Cpu *c); void f_089F2534(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* The 32 sound requests of the first callback table (0x1C bytes each at 0x09525E10) are carried
   out and cleared: kind 1 starts a voice (+4) with its samples (+8, +12), volumes (+16, +20) and
   pitch (+24) and a fixed envelope; kind 2 stops one. A 0x40 frame keeping s0..s3. */
SFA_FUNC(089E5838, sound_requests_run) {
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 4; i++) *W(sp + 32 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 48) = sv[4];
    c->r[17] = 0x7FFFFFFFu; c->r[16] = 0x40000000u;
    gaddr e = 0x09525E10u;
    for (uint32_t i = 0; i < 32; i++, e += 0x1C) {
        c->r[19] = i; c->r[18] = e;
        int32_t kind = (int32_t)*W(e);
        if (kind == 1) {
            call1(c, *W(e + 4), f_089ECADC, 0x089E5888u);
            ARG(3) = 1;
            call3(c, *W(e + 4), *W(e + 8), *W(e + 12), f_089ECCAC, 0x089E589Cu);
            ARG(3) = 0; c->r[8] = 0;
            call3(c, *W(e + 4), *W(e + 16), *W(e + 20), f_089ECBF8, 0x089E58B4u);
            ARG(1) = *W(e + 24);
            call1(c, *W(e + 4), f_089ECC5C, 0x089E58C0u);
            ARG(3) = 5; c->r[8] = 5; c->r[9] = 1;
            call3(c, *W(e + 4), 0xF, 0, f_089ECDC0, 0x089E58DCu);
            ARG(3) = 0x40000000u; c->r[8] = 0x40000000u; c->r[9] = 0x7FFFFFFFu;
            call3(c, *W(e + 4), 0xF, 0x7FFFFFFFu, f_089ECD54, 0x089E58F8u);
            call1(c, *W(e + 4), f_089ECAA0, 0x089E5900u);
            call3(c, e, 0, 0x1C, f_089F3470, 0x089E5910u);
        } else if (kind == 2) {
            call1(c, *W(e + 4), f_089ECADC, 0x089E592Cu);
            call3(c, e, 0, 0x1C, f_089F3470, 0x089E593Cu);
        }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}

/* (format, address, width): the buffer to draw into: three commands on the list in use (0xD2 with
   the format, 0x9C and 0x9D with the address and the width), the five lists and the drawing state
   take the format, and the state keeps the address (+44) and width (+40); its +52 (when clear,
   and the height at +64 is known) becomes the address past height times width words, and its +56
   (when clear) the width. A 0x10 frame keeping s0..s2. */
SFA_FUNC(089E73F0, draw_buffer_set) {
    uint32_t format = ARG(0), addr = ARG(1), width = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 8) = s2; *W(sp + 4) = s1; *W(sp) = s0; *W(sp + 12) = ra;
    c->r[18] = width; c->r[17] = addr; c->r[16] = format;
    c->r[31] = 0x089E7414u; f_089EAB30(c);
    gaddr list = *W(GE + 72);
    gaddr p = *W(list + 8);
    c->r[13] = list; c->r[24] = p;
    *W(p) = format | 0xD2000000u;
    *W(list + 8) = p + 12;
    *W(p + 4) = (addr & 0x00FFFFFFu) | 0x9C000000u;
    *W(p + 8) = (((addr >> 24) & 0xF) << 16) | width | 0x9D000000u;
    gaddr e = GE + 0x4C;
    for (int i = 0; i < 5; i++, e += 0xFC) {
        *W(e + 28) = *W(GE + 60);
        *W(e + 32) = *W(GE + 64);
        *W(e + 24) = format;
    }
    c->r[3] = GE;
    uint32_t depth = *W(GE + 52);
    *W(GE + 36) = format;
    *W(GE + 44) = addr;
    *W(GE + 40) = width;
    if (depth == 0) {
        uint32_t h = *W(GE + 64);
        if (h != 0) {
            int64_t m = (int64_t)(int32_t)h * (int32_t)width;
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            *W(GE + 52) = addr + (c->lo << 2);
        }
    }
    c->r[7] = GE;
    c->r[9] = *W(GE + 56);
    if (c->r[9] == 0) *W(GE + 56) = width;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x10;
}

/* (n): the sound's voices are set up, n of them (1 to 32, else 0x8054000A): each is reset with
   its number in its first byte, 1 at +1 and its place in the list at 0x08E752B4; the count goes
   to 0x08E74508 and 0x08E74510 and the word at 0x08AD81EC to 0. Returns 0. A 0x20 frame keeping
   s0..s4. */
SFA_FUNC(089F0E28, voices_setup) {
    uint32_t n = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[5];
    for (int i = 4; i >= 0; i--) *W(sp + 4 * (uint32_t)i) = sv[i];
    uint32_t answer = 0x8054000Au;
    if (n - 1 < 0x20) {
        c->r[19] = 0x08E70000u;
        *W(VOICE_N) = n;
        c->r[20] = 1;
        gaddr v = VOICES, slot = 0x08E752B4u;
        uint32_t i = 0;
        do {
            c->r[16] = v; c->r[17] = i; c->r[18] = slot;
            *B(v + 1) = 1;
            *W(v + 24) = 0;
            *W(v + 4) = 0;
            call1(c, v, f_089F2898, 0x089F0E90u);
            *B(v) = (uint8_t)i;
            i++;
            *W(slot) = v;
            v += 0x68; slot += 4;
        } while (i < *W(VOICE_N));
        *W(0x08E74510u) = *W(VOICE_N);
        *W(VOICE_UP) = 0;
        answer = 0;
    }
    c->r[3] = answer;
    c->r[2] = answer;
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x20;
}

/* Whether the voices are set up. The four loops below hand the states 7, 8 and 9 to the
   loop's own routine as the caller's three. */
static int voices_ready(Cpu *c) {
    (void)c;
    if (*W(VOICE_UP) == 0xFFFFFFFFu) return 0;
    return 1;
}

/* (pair, owner): the bits of the voices in a taken state that play the two bytes at `pair` for
   the owner. 0 before the voices are set up or without a pair. */
SFA_FUNC(089F224C, voices_playing_pair) {
    c->r[11] = 0;
    ARG(2) = 0;
    if (!voices_ready(c) || ARG(0) == 0 || *W(VOICE_N) == 0) { c->r[2] = 0; return; }
    c->r[10] = *W(VOICE_N);
    c->r[7] = VOICES - 0x68;
    c->r[8] = 0; c->r[9] = 1;
    c->r[12] = 7; c->r[13] = 8; c->r[14] = 9; c->r[15] = 1;
    f_089F22D4(c);
}

/* (owner): the bits of the voices in a taken state that play for the owner. */
SFA_FUNC(089F2324, voices_playing_for) {
    c->r[10] = 0;
    ARG(1) = 0;
    if (!voices_ready(c) || *W(VOICE_N) == 0) { c->r[2] = 0; return; }
    c->r[9] = *W(VOICE_N);
    ARG(2) = VOICES - 0x68;
    c->r[8] = 0; ARG(3) = 1;
    c->r[11] = 7; c->r[12] = 8; c->r[14] = 9; c->r[13] = 1;
    f_089F23A4(c);
}

/* (channel, owner): the bits of the voices in a taken state that play on the channel (below 16)
   for the owner. */
SFA_FUNC(089F23D4, voices_playing_on) {
    c->r[11] = 0;
    ARG(2) = 0;
    if (!voices_ready(c) || !(ARG(0) < 0x10) || *W(VOICE_N) == 0) { c->r[2] = 0; return; }
    c->r[10] = *W(VOICE_N);
    c->r[7] = VOICES - 0x68;
    c->r[9] = 0; c->r[8] = 1;
    c->r[12] = 7; c->r[13] = 8; c->r[14] = 9; c->r[15] = 1;
    f_089F2460(c);
}

/* (channel, owner): the same with the state 5 taken as well. */
SFA_FUNC(089F249C, voices_playing_on_5) {
    c->r[11] = 0;
    ARG(2) = 0;
    if (!voices_ready(c) || !(ARG(0) < 0x10) || *W(VOICE_N) == 0) { c->r[2] = 0; return; }
    c->r[10] = *W(VOICE_N);
    c->r[7] = VOICES - 0x68;
    c->r[9] = 0; c->r[8] = 1;
    c->r[12] = 7; c->r[13] = 8; c->r[14] = 9; c->r[15] = 5; c->r[24] = 1;
    f_089F2534(c);
}
