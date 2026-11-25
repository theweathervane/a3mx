/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 85 - 89
   instructions: a fighter's figures for the select / VS screens (the highest of three), the VS
   screen's portrait kept to its fighter, a select slot's pictures loaded, a mode screen's flashing
   palette. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0886CF78(Cpu *c); void f_088215C4(Cpu *c); void f_0882196C(Cpu *c); void f_0889697C(Cpu *c);
void f_088075F8(Cpu *c); void f_089E0C24(Cpu *c); void f_0880AEA8(Cpu *c); void f_088B01A0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter, entry, n, value): row n of the side's 8-byte table (0x09358468, or 0x093584C8 for side 1:
   +0 = value, +4 = the width of the entry's box (+1 into the 32-byte list at fighter +224), +2 = the
   highest of up to three figures (0x0886CF78 on the lists at fighter +208 / +212 / +216, picked by the
   three bytes of the entry's row (+0, 4 bytes, of fighter +200)). A missing third figure counts as the
   fighter's own address, as the original leaves it. */
SFA_FUNC(0886CE24, fighter_figures) {
    gaddr f = ARG(0), e = ARG(1);
    uint32_t n = ARG(2), value = ARG(3);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = s4; *W(c->r[29] + 36) = ra;
    c->r[16] = f;
    gaddr row = (S8(f + 337) != 0 ? 0x093584C8u : 0x09358468u) + 8u * n;
    c->r[17] = row;
    *H(row) = (uint16_t)value;
    gaddr box = *W(f + 224) + ((uint32_t)*B(e + 1) << 5);
    *H(row + 4) = (uint16_t)(S16(box + 4) - S16(box));
    gaddr pick = *W(f + 200) + ((uint32_t)*B(e) << 2);
    c->r[18] = pick;
    uint32_t a = *B(pick), b, d = f;
    if (a != 0) { call2(c, f, *W(f + 208) + 8u * a, 0x0886CEC8u, f_0886CF78); a = c->r[2]; }
    c->r[19] = a;
    b = *B(pick + 1);
    if (b != 0) { call2(c, f, *W(f + 212) + 8u * b, 0x0886CEF0u, f_0886CF78); b = c->r[2]; }
    c->r[20] = b;
    uint32_t k = *B(pick + 2);
    if (k != 0) { call2(c, f, *W(f + 216) + 8u * k, 0x0886CF18u, f_0886CF78); d = c->r[2]; }
    int32_t sa = (int32_t)a, sb = (int32_t)b, sd = (int32_t)d;
    uint32_t most;
    if (!(sa < sb) && !(sa < sd)) { most = a; ARG(0) = 0; }
    else {
        if (sb < sa) most = d;
        else most = sb < sd ? d : b;
        ARG(0) = sb < sd;
    }
    *H(row + 2) = (uint16_t)most;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (portrait, fighter): the VS screen's portrait kept to its fighter: +73 / +81 / +117 follow +78 / +80
   / +116; the character is the fighter's (+338), or with +10 bit 7 the other side's through 0x08A2AA2C
   (0x24 while GAME +191 is set). When the character or colour (+376) changed: +78 / +80 (+386) / +116
   set, the palette (0x0889697C, 0x25 as 0x15) and the animation (row +80 of 0x08A27A50, 8 bytes by the
   character); then drawn (0x088215C4). */
SFA_FUNC(0889B3B4, vs_portrait_follow) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    uint32_t was = (uint32_t)(int32_t)S8(o + 78);
    uint32_t col = (uint32_t)(int32_t)S8(o + 80) & 0xFFFF;
    uint32_t v116 = *B(o + 116);
    if (was != (uint32_t)(int32_t)S8(o + 73) || col != (uint32_t)(int32_t)S8(o + 81) || v116 != *B(o + 117)) {
        *B(o + 73) = (uint8_t)was; *B(o + 81) = (uint8_t)col; *B(o + 117) = (uint8_t)v116;
    }
    uint32_t ch = (uint32_t)(int32_t)S8(f + 338) & 0xFFFF;
    uint32_t pal = (uint32_t)(int32_t)S8(f + 386);
    uint32_t shade = (uint32_t)(int32_t)S8(f + 376) & 0xFFFF;
    if (*B(o + 10) & 0x80) {
        gaddr other = S8(f + 337) != 0 ? 0x092E3EA0u : 0x092E4338u;
        ch = S8(GAME + 191) != 0 ? 0x24 : *B(0x08A2AA2Cu + (uint32_t)(int32_t)S8(other + 338));
    }
    c->r[17] = pal; c->r[18] = ch;
    if (was != ch || *B(o + 117) != shade) {
        *B(o + 78) = (uint8_t)ch;
        *B(o + 80) = (uint8_t)pal;
        *B(o + 116) = (uint8_t)shade;
        if (ch == 0x25) ch = 0x15;
        c->r[18] = ch;
        ARG(0) = ch; ARG(1) = 0; ARG(2) = shade; ARG(3) = *B(o + 13);
        c->r[31] = 0x0889B4CCu; f_0889697C(c);
        call2(c, o, *W(0x08A27A4Cu + (pal << 2) + 4) + (ch << 3), 0x0889B4F0u, f_0882196C);
    }
    call_on(c, o, 0x0889B4F8u, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (n, character, slot): a select slot's pictures loaded: the slot's copies (0x08A0562C) and tile area
   (0x092DB240 + 2 KB x slot) noted in the words at 0x08A055DC + 4 x slot (+68 / +52), the character at
   0x092E89DC + 2 x slot, 0x08E75C48 + 2 x slot = -1, 0x092E89E8 + 4n = slot, the current 216-byte
   record's +200 = n and +201 - +204 / +208 = slot, 0x088075F8 (the character's archive from
   0x08A03680, 0, 0), texture 0x60 + slot freed, and list 0x1E + slot's picture list and tile set put
   back (from 0x08A0562C / 0x08A050B0; the picture list also noted at +40). */
SFA_FUNC(08813538, select_slot_load) {
    uint32_t n = ARG(0), ch = ARG(1), slot_arg = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = ra;
    uint32_t slot = slot_arg & 0xFFFF;
    c->r[16] = slot; c->r[17] = 0x08A0562Cu;
    gaddr note = 0x08A055DCu + 4u * slot;
    c->r[18] = note;
    *W(note + 68) = *W(0x08A0562Cu + 4u * slot + 144);
    *W(note + 52) = 0x092DB240u + (slot << 11);
    *H(0x092E89DCu + 2u * slot) = (uint16_t)ch;
    *H(0x08E75C48u + 2u * slot) = 0xFFFF;
    *W(0x092E89E8u + 4u * n) = slot;
    gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
    *B(rec + 203) = (uint8_t)slot_arg; *B(rec + 202) = (uint8_t)slot_arg;
    *H(rec + 208) = (uint16_t)slot_arg; *B(rec + 204) = (uint8_t)slot_arg;
    *B(rec + 200) = (uint8_t)n; *B(rec + 201) = (uint8_t)slot_arg;
    ARG(0) = *W(0x08A03140u + 1344 + ((ch & 0xFFFF) << 2)); ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x08813630u; f_088075F8(c);
    uint32_t list = (slot + 0x1E) & 0xFF;
    c->r[19] = list;
    call_on(c, slot + 0x60, 0x08813640u, f_089E0C24);
    uint32_t k = list << 2;
    uint32_t pics = *W(0x08A0562Cu + k);
    *W(note + 40) = pics;
    uint32_t tiles = *W(0x08A050B0u + k);
    *W(0x08E75A18u + k) = tiles;
    *W(0x08E75AD8u + k) = pics;
    ARG(0) = 0x08E75AD8u + k; ARG(1) = pics; ARG(2) = tiles; ARG(3) = 0x08E75A18u + k;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (screen): a mode screen's flashing palette (28 steps of 0x08A30718 by +10 into slot 0x1F): kind 7
   steps every 3 frames (+34); other kinds from frame 13 step every frame (into palette set 0 while
   GAME +93 is set). With +78 clear, from frame 13 +18 moves 2 a frame. Then 0x088B01A0. */
SFA_FUNC(088AFE7C, mode_flash) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    if (*B(o + 3) == 7) {
        ARG(0) = 1; ARG(1) = 0x08A30718u + ((uint32_t)*B(o + 10) << 5); ARG(2) = 0; ARG(3) = 0x1F; c->r[8] = 1;
        c->r[31] = 0x088AFEC4u; f_0880AEA8(c);
        if (!(*B(o + 10) < 28)) *B(o + 10) = 0;
        int16_t t = (int16_t)(S16(o + 34) + 1);
        *H(o + 34) = (uint16_t)t;
        if (t == 3) { *H(o + 34) = 0; *B(o + 10) = (uint8_t)(*B(o + 10) + 1); }
    } else if (!(S16(o + 34) < 13)) {
        ARG(0) = S8(GAME + 93) != 0 ? 0 : 1;
        ARG(1) = 0x08A30718u + ((uint32_t)*B(o + 10) << 5); ARG(2) = 0; ARG(3) = 0x1F; c->r[8] = 1;
        c->r[31] = S8(GAME + 93) != 0 ? 0x088AFF70u : 0x088AFF4Cu; f_0880AEA8(c);
        uint32_t v = *B(o + 10) + 1u;
        *B(o + 10) = (uint8_t)v;
        if (!((v & 0xFF) < 28)) *B(o + 10) = 0;
    }
    if (S8(o + 78) == 0) {
        int16_t t = (int16_t)(S16(o + 34) + 1);
        *H(o + 34) = (uint16_t)t;
        if (!(t < 13)) *H(o + 18) = (uint16_t)(S16(o + 18) + 2);
    }
    call_on(c, o, 0x088AFFD0u, f_088B01A0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
