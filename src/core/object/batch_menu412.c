/* The pieces of a menu screen: each one's start and step by its kind, with its places scaled by
   the constant the game keeps for that. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME  0x08E75910u
#define SCALE 0x08A268F0u /* the double every place is divided by */

void f_08889FC0(Cpu *c); void f_08889968(Cpu *c); void f_08889BA8(Cpu *c); void f_08889C88(Cpu *c);
void f_08889CC8(Cpu *c); void f_08889D64(Cpu *c); void f_08889E00(Cpu *c); void f_08889E80(Cpu *c);
void f_08889F7C(Cpu *c); void f_0888A224(Cpu *c); void f_0888A1B0(Cpu *c); void f_0888A1EC(Cpu *c);
void f_089FB928(Cpu *c); void f_089FB6A8(Cpu *c); void f_089FBA04(Cpu *c);

static const uint8_t SHADES[12] = { 51, 50, 49, 48, 45, 44, 43, 42, 39, 38, 37, 36 };

/* (o): a choice's look by whether it is the one the game's +194 names: full bright, a little
   larger and in state 1, or dimmer in state 2. */
SFA_FUNC(08889AD0, choice_look) {
    gaddr o = ARG(0);
    c->r[6] = (uint32_t)S8(GAME + 194);
    ARG(1) = *B(o + 92);
    if (c->r[6] == ARG(1)) {
        *H(o + 24) = 0xFFFC;
        *H(o + 12) = 0xFFFC;
        *H(o + 14) = 0xB8;
        c->r[6] = 0xFF;
        *H(o + 26) = 0x48;
        *B(o + 136) = 0xFF;
        for (int i = 0; i < 12; i++) *B(o + SHADES[i]) = 0xFF;
        ARG(1) = 1;
        *B(o + 56) = 1;
    } else {
        *H(o + 24) = 0;
        *H(o + 12) = 0;
        *H(o + 14) = 0xB0;
        *H(o + 26) = 0x40;
        c->r[6] = 0x80;
        *B(o + 136) = 0xC0;
        for (int i = 0; i < 12; i++) *B(o + SHADES[i]) = 0x80;
        ARG(1) = 2;
        *B(o + 56) = 2;
    }
}

/* (o): the long bar's start: its place and size (0x290 by 0x28 at 0, 9), its three colours, and
   its states. */
SFA_FUNC(08889EF8, bar_piece_start) {
    gaddr o = ARG(0);
    *B(o + 88) = 0;
    *B(o + 55) = 7;
    *H(o + 6) = 0;
    *H(o + 10) = 9;
    *H(o + 12) = 0;
    *H(o + 24) = 0x12;
    *H(o + 14) = 0x290;
    *H(o + 26) = 0x28;
    c->r[6] = 1;
    *B(o + 56) = 1;
    c->r[7] = 0x76;
    *B(o + 136) = 0xC0;
    for (int i = 36; i < 40; i++) *B(o + i) = 0x76;
    for (int i = 42; i < 46; i++) *B(o + i) = 0x76;
    ARG(1) = 0xBC;
    for (int i = 48; i < 52; i++) *B(o + i) = 0xBC;
    *B(o + 93) = 1;
}

/* (o): a choice's start (kinds 29 to 35): its picture from the table at 0x08A264C0, its place
   from the one at 0x08A26934 (16 higher), and its look. A 0x20 frame keeping s0, s1. */
SFA_FUNC(08889A28, choice_piece_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = o;
    *B(o + 54) = 0x12;
    *W(sp + 20) = s1;
    c->r[17] = 1;
    *B(o + 88) = 1;
    *B(o + 55) = 7;
    ARG(1) = 0x08A264C0u + ((uint32_t)*B(o + 92) << 4);
    *W(sp + 24) = ra;
    ARG(0) = o; c->r[31] = 0x08889A70u; f_08889F7C(c);
    gaddr place = 0x08A26934u + (((uint32_t)*B(o + 92) - 0x1D) << 2);
    *H(o + 6) = *H(place);
    c->r[6] = 2;
    *B(o + 56) = 2;
    *H(o + 10) = (uint16_t)(*H(place + 2) - 0x10);
    ARG(0) = o; c->r[31] = 0x08889AB0u; f_08889AD0(c);
    ARG(0) = 1;
    *B(o + 138) = 1;
    *B(o + 93) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The halfword at o's +at as a double, in r2 and r3. */
static void to_double(Cpu *c, gaddr o, uint32_t at, uint32_t back) {
    ARG(0) = (uint32_t)S16(o + at);
    c->r[31] = back; f_089FB928(c);
}
/* The double (lo, hi) divided by the scale kept in the registers lo_reg and hi_reg, as a whole
   number at o's +at. */
static void scaled_store(Cpu *c, gaddr o, uint32_t at, uint32_t lo, uint32_t hi, int lo_reg, int hi_reg, uint32_t back) {
    ARG(1) = hi; ARG(3) = c->r[hi_reg]; ARG(2) = c->r[lo_reg]; ARG(0) = lo;
    c->r[31] = back; f_089FB6A8(c);
    ARG(1) = c->r[3]; ARG(0) = c->r[2];
    c->r[31] = back + 0xC; f_089FBA04(c);
    *H(o + at) = (uint16_t)c->r[2];
}

/* (o): a piece's step: its +93 is set; the small ones (kinds below 15) and the choices (29 to
   35) have their routine or look run and their +12 and +14 scaled; kinds 17 to 20 and 21 to 24
   have their own routines. A 0x20 frame keeping s0, s2, s3. */
SFA_FUNC(0888A010, menu_piece_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = o;
    int32_t kind = *B(o + 92);
    *B(o + 93) = 1;
    *W(sp + 20) = s2; *W(sp + 24) = s3; *W(sp + 28) = ra;
    uint32_t back = 0;
    if (kind < 15) {
        ARG(0) = o; c->r[31] = 0x0888A048u; f_0888A224(c);
        back = 0x0888A050u;
    } else if (kind == 15 || kind == 0x1A || kind == 0x10) {
    } else if (kind >= 17 && kind < 21) {
        ARG(0) = o; c->r[31] = 0x0888A0E0u; f_0888A1B0(c);
    } else if (kind >= 21 && kind < 25) {
        ARG(0) = o; c->r[31] = 0x0888A104u; f_0888A1EC(c);
    } else if (kind == 0x19 || kind == 0x1B || kind == 0x1C) {
    } else if (kind < 36) {
        ARG(0) = o; c->r[31] = 0x0888A134u; f_08889AD0(c);
        back = 0x0888A13Cu;
    }
    if (back != 0) {
        to_double(c, o, 12, back);
        c->r[19] = *W(SCALE + 4);
        c->r[18] = *W(SCALE);
        scaled_store(c, o, 12, c->r[2], c->r[3], 18, 19, back + 0x20);
        to_double(c, o, 14, back + 0x38);
        scaled_store(c, o, 14, c->r[2], c->r[3], 18, 19, back + 0x4C);
    }
    c->r[16] = s0; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x20;
}

/* (o): a piece's frame: its count (+128) goes up, the routine at 0x08889FC0 runs, then the
   piece's own routine by its kind (36 and above have none, and their +93 is cleared); +12, +14
   and +6 are then scaled. A 0x40 frame keeping s0..s6, with the third number at +16. */
SFA_FUNC(088895C4, menu_piece_frame) {
    gaddr o = ARG(0);
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 48) = sv[6];
    c->r[22] = o;
    ARG(1) = *B(o + 128) + 1u;
    for (int i = 0; i < 6; i++) *W(sp + 24 + 4 * i) = sv[i];
    *W(sp + 52) = ra;
    *B(o + 128) = (uint8_t)ARG(1);
    c->r[31] = 0x088895FCu; f_08889FC0(c);
    int32_t kind = *B(o + 92);
    void (*own)(Cpu *) = 0;
    uint32_t back;
    if (kind < 15) { own = f_08889968; back = 0x08889614u; }
    else if (kind == 15 || kind == 0x1A) { own = f_08889BA8; back = 0x08889660u; }
    else if (kind == 0x10) { own = f_08889C88; back = 0x088896A4u; }
    else if (kind >= 17 && kind < 21) { own = f_08889CC8; back = 0x088896F0u; }
    else if (kind >= 21 && kind < 25) { own = f_08889D64; back = 0x08889740u; }
    else if (kind == 0x19) { own = f_08889E00; back = 0x08889784u; }
    else if (kind == 0x1B) { own = f_08889E80; back = 0x088897C8u; }
    else if (kind == 0x1C) { own = f_08889EF8; back = 0x0888980Cu; }
    else if (kind >= 29 && kind < 36) { own = f_08889A28; back = 0x08889858u; }
    else { back = 0x08889890u; *B(o + 93) = 0; }
    if (own != 0) { ARG(0) = o; c->r[31] = back; own(c); }
    to_double(c, o, 12, back + 8);
    c->r[21] = c->r[3]; c->r[20] = c->r[2];
    to_double(c, o, 14, back + 0x18);
    c->r[19] = c->r[3]; c->r[18] = c->r[2];
    to_double(c, o, 6, back + 0x28);
    c->r[17] = c->r[3]; c->r[16] = c->r[2];
    *W(sp + 20) = c->r[17];
    *W(sp + 16) = c->r[16];
    c->r[17] = *W(SCALE + 4);
    c->r[16] = *W(SCALE);
    scaled_store(c, o, 12, c->r[20], c->r[21], 16, 17, 0x088898E8u);
    scaled_store(c, o, 14, c->r[18], c->r[19], 16, 17, 0x0888990Cu);
    scaled_store(c, o, 6, *W(sp + 16), *W(sp + 20), 16, 17, 0x08889930u);
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}
