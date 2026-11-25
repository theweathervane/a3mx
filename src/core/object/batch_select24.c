/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 28 - 123
   instructions: leaving the select screen (two ways), the fighters put in drawing order, the VS
   screen's name plate's steps. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_08805564(Cpu *c); void f_0880A39C(Cpu *c); void f_08814320(Cpu *c); void f_08821F9C(Cpu *c);
void f_0886D860(Cpu *c); void f_088215C4(Cpu *c); void f_0889693C(Cpu *c); void f_088983D0(Cpu *c);
void f_0889844C(Cpu *c); void f_0889858C(Cpu *c); void f_088D5E44(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void timers_clear(gaddr o) {
    *H(o + 22) = 0; *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 16) = 0;
    *H(o + 10) = 0; *H(o + 8) = 0; *H(o + 6) = 0; *H(o + 4) = 0;
}

/* (screen): leaving the select screen for good, unless +53 is set: +2 = 6, its timers cleared,
   0x08E75C44 = 0xFF, 0x08805564 (2), 0x08821F9C, 0x0880A39C and the portraits freed (0x08814320). */
SFA_FUNC(0882B824, select_quit) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 53) == 0) {
        *H(o + 2) = 6;
        timers_clear(o);
        *H(0x08E75C44u) = 0xFF;
        call_on(c, 2, 0x0882B870u, f_08805564);
        c->r[31] = 0x0882B878u; f_08821F9C(c);
        c->r[31] = 0x0882B880u; f_0880A39C(c);
        c->r[31] = 0x0882B888u; f_08814320(c);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen): on to the fight once 0x092E510F bit 7 is set: 0x08821F9C, its timers cleared, +43 = +54 =
   1, +72 = -1, the four fighters' +1 cleared and the portraits freed (0x08814320). */
SFA_FUNC(088280A0, select_to_fight) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    if (*B(0x092E5100u + 15) & 0x80) {
        call_on(c, 0x80, 0x088280D0u, f_08821F9C);
        timers_clear(o);
        *B(o + 54) = 1; *B(o + 43) = 1;
        *B(o + 72) = 0xFF;
        *B(0x092E3EA0u + 1) = 0; *B(0x092E4338u + 1) = 0; *B(0x092E47D0u + 1) = 0; *B(0x092E4C68u + 1) = 0;
        call_on(c, 0x092E4C68u, 0x08828138u, f_08814320);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (a, b, c): three fighters put in drawing order at 0x092E22E8 (0x0886D860 swaps two of the stack
   words): by +135 (one ahead, one behind) and +261 when a has +135 set, else by the +7 byte of each
   one's current 12-byte frame row (+28's +8 into +416's +16), smallest first. */
SFA_FUNC(0886D944, fighters_draw_order) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 16) = ARG(0); *W(sp + 20) = ARG(1); *W(sp + 36) = s2; *W(sp + 24) = ARG(2);
    *W(sp + 28) = s0; *W(sp + 32) = s1; *W(sp + 40) = s3; *W(sp + 44) = s4; *W(sp + 48) = ra;
#define SWAP(x, y, back) call2(c, sp + (x), sp + (y), (back), f_0886D860)
    gaddr a = *W(sp + 16), b = *W(sp + 20);
    c->r[18] = a; c->r[17] = b;
    int32_t k = S8(a + 135);
    if (k > 0) {
        if (S8(b + 135) == 0) { SWAP(0x14, 0x18, 0x0886D998u); c->r[17] = *W(sp + 20); }
        if (S8(c->r[17] + 261) == 0) SWAP(0x10, 0x14, 0x0886D9B0u);
    } else if (k < 0) {
        if (S8(b + 135) == 0) { SWAP(0x14, 0x18, 0x0886D9C8u); c->r[18] = *W(sp + 16); }
        if (S8(c->r[18] + 261) != 0) SWAP(0x10, 0x14, 0x0886D9E0u);
    } else {
        gaddr d = *W(sp + 24);
        gaddr ra_ = *W(*W(a + 416) + 16) + 12u * *H(*W(a + 28) + 8);
        gaddr rb = *W(*W(b + 416) + 16) + 12u * *H(*W(b + 28) + 8);
        gaddr rd = *W(*W(d + 416) + 16) + 12u * *H(*W(d + 28) + 8);
        c->r[18] = ra_; c->r[17] = rb; c->r[16] = rd; c->r[20] = ra_;
        int32_t A = *B(ra_ + 7), Bv = *B(rb + 7);
        if (!(Bv < A)) {
            c->r[18] = sp + 0x10; c->r[19] = sp + 0x14;
            SWAP(0x10, 0x14, 0x0886DA74u);
            if (!((int32_t)*B(rd + 7) < (int32_t)*B(ra_ + 7))) {
                SWAP(0x14, 0x18, 0x0886DA90u);
                if (!((int32_t)*B(rd + 7) < (int32_t)*B(rb + 7))) SWAP(0x10, 0x14, 0x0886DAACu);
            }
        } else {
            c->r[19] = sp + 0x14;
            if (!((int32_t)*B(rd + 7) < Bv)) {
                SWAP(0x14, 0x18, 0x0886DAD0u);
                if (!((int32_t)*B(rd + 7) < (int32_t)*B(ra_ + 7))) SWAP(0x10, 0x14, 0x0886DAECu);
            }
        }
    }
#undef SWAP
    *W(0x092E22E8u) = *W(sp + 16);
    *W(0x092E22E8u + 4) = *W(sp + 20);
    *W(0x092E22E8u + 8) = *W(sp + 24);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x40;
}

/* (plate): the VS screen's name plate (its fighter at +92) by its step (+5): 0 slides in (x by +96 for
   +82 frames, then to +112, +104 = BATTLE +18); 1 follows BATTLE +18 (the selection: 0 moves on with
   +82 = 0xB; on a change for this side (bit 15 against the fighter's +373) the plate's character and
   pictures are set again: the fighter's, or the one picked through 0x08A2A0EC with bit 14 (also while
   0x088D5E44 / GAME +146), or the partner's (0x092E4C68) with bit 14); 2 slides out (y by +100 for +82
   frames, then +4 = 2). Then drawn (0x088215C4). */
SFA_FUNC(088986F4, vs_name_plate_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20];
    c->r[29] -= 0x30;
    gaddr f = *W(o + 92);
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = s4; *W(c->r[29] + 36) = ra;
    c->r[18] = f; c->r[16] = o;
    uint32_t st = *B(o + 5);
    if (st == 0) {
        *H(o + 18) = (uint16_t)(S16(o + 18) + *W(o + 96));
        int16_t t = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)t;
        if (t < 0) {
            *B(o + 5) = 1;
            *H(o + 18) = (uint16_t)S16(o + 112);
            *W(o + 104) = (uint32_t)(int32_t)S16(BATTLE + 18);
        }
    } else if (st == 1) {
        c->r[17] = BATTLE;
        uint32_t v = (uint32_t)(int32_t)S16(BATTLE + 18);
        if (v == 0) { *B(o + 5) = 2; *H(o + 82) = 0xB; }
        else if (v != *W(o + 104) && (int32_t)((v & 0x8000) >> 15) == S8(f + 373)) {
            ARG(0) = v;
            c->r[31] = 0x088987C0u; f_088D5E44(c);
            uint32_t ch = (uint32_t)(int32_t)S8(f + 338), shade;
            int picked = c->r[2] == 0 ? (S16(BATTLE + 18) & 0x4000) != 0 : S8(GAME + 146) != 0;
            if (picked) { ch = (uint32_t)(int32_t)S8(0x08A2A0ECu + ch); shade = 2; }
            else if (c->r[2] != 0 && (S16(BATTLE + 18) & 0x4000) != 0) {
                ch = (uint32_t)(int32_t)S8(0x092E4C68u + 338);
                shade = (uint32_t)(int32_t)S8(0x092E4C68u + 376);
            } else shade = (uint32_t)(int32_t)S8(f + 376);
            c->r[19] = ch;
            *B(o + 78) = (uint8_t)ch;
            call2(c, f, shade, 0x0889883Cu, f_088983D0);
            c->r[20] = c->r[2];
            call2(c, f, ch, 0x0889884Cu, f_0889844C);
            uint32_t pics = c->r[2];
            ARG(0) = pics; ARG(1) = (uint32_t)(int32_t)S8(f + 386); ARG(2) = c->r[20]; ARG(3) = *B(o + 13);
            c->r[18] = pics;
            c->r[31] = 0x08898864u; f_0889693C(c);
            call2(c, o, (uint32_t)(int32_t)(int16_t)pics, 0x08898874u, f_0889858C);
            *W(o + 104) = (uint32_t)(int32_t)S16(BATTLE + 18);
        } else *W(o + 104) = v;
    } else if (st == 2) {
        *H(o + 22) = (uint16_t)(S16(o + 22) + *W(o + 100));
        int16_t t = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)t;
        if (t < 0) *B(o + 4) = 2;
    }
    call_on(c, o, 0x088988C0u, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x30;
}
