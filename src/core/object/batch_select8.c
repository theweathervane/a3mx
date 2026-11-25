/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 62 - 65
   instructions: the fighters' drawing order, the slot-29 picture list / tile set / palettes allocated
   and freed (the select screen's big portrait), a ticker object's start, a shaking intro object.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088B1AB4(Cpu *c); void f_0880AEA8(Cpu *c); void f_08821368(Cpu *c); void f_089E0540(Cpu *c);
void f_0886D860(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E04EC(Cpu *c); void f_08808424(Cpu *c);
void f_08858B34(Cpu *c); void f_088215C4(Cpu *c);

#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- fighters' drawing order */
/* Which of fighters 1 and 2 is drawn in front (0x092E22E8 / EC, first and second): by +135 (set: by the
   other's / its own +261), else by their frames' records' +7 (the one with the larger behind). */
static uint8_t frame_depth(gaddr f) {
    gaddr rec = *W(*W(f + 416) + 16);
    return *B(rec + (uint32_t)*H(*W(f + 28) + 8) * 12 + 7);
}
SFA_FUNC(0886D764, fighters_order) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = 0x092E3EA0u;
    *W(sp + 20) = 0x092E4338u;
    *W(sp + 24) = ra;
    gaddr f1 = *W(sp + 16);
    int8_t v = S8(f1 + 135);
    int swap;
    if (v == 0) swap = (int32_t)frame_depth(f1) < (int32_t)frame_depth(*W(sp + 20));
    else if (v < 0) swap = S8(f1 + 261) != 0;
    else swap = S8(*W(sp + 20) + 261) == 0;
    if (swap) {
        ARG(0) = sp + 16; ARG(1) = sp + 20;
        c->r[31] = v == 0 ? 0x0886D83Cu : v < 0 ? 0x0886D7D4u : 0x0886D7B8u;
        f_0886D860(c);
    }
    *W(0x092E22E8u) = *W(sp + 16);
    *W(0x092E22ECu) = *W(sp + 20);
    ARG(0) = 0x092E22E8u;
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- slot 29 (the big portrait) */
/* The pictures, tiles and palettes held for slots 29 (0x08A055EC - F4) and 13 (0x08A055E0 - E8)
   freed (0x089E0540) and their list entries cleared. */
SFA_FUNC(08814574, free_slots_29_13) {
    gaddr h = 0x08A055DCu;
    WITH_S0(h);
    static const uint8_t at[6] = { 16, 20, 24, 4, 8, 12 };
    static const uint32_t lists[6] = { 0x08E75AD8u + 116, 0x092D9D40u + 116, 0x092D9E00u + 116,
                                       0x08E75AD8u + 52, 0x092D9D40u + 52, 0x092D9E00u + 52 };
    static const uint32_t backs[6] = { 0x0881459Cu, 0x088145C0u, 0x088145E4u, 0x08814608u, 0x0881462Cu, 0x08814650u };
    for (int k = 0; k < 6; k++) {
        uint32_t p = *W(h + at[k]);
        if (p == 0) continue;
        call_on(c, p, backs[k], f_089E0540);
        *W(lists[k]) = 0;
        *W(h + at[k]) = 0;
    }
    END_S0();
}
/* With the loader's lock (0x088D7EA0 (1) / (0)): unless slot 29 has its picture list, 0x4400 / 0x2F400 /
   0x400 bytes allocated (0x089E04EC) for its pictures, tiles and palettes (held at 0x08A055EC - F4), the
   current 216-byte record's (0x08E76088 by 0x08E76084) +202 - +204 and +208 cleared and 0x08808424
   (0x10); then its 0x1D palettes into slot 0. */
SFA_FUNC(08814670, alloc_slot_29) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 40) = s0; *W(sp + 44) = s1; *W(sp + 48) = ra;
    call_on(c, 1, 0x08814688u, f_088D7EA0);
    gaddr pal = 0x092D9E00u;
    if (*W(0x08E75AD8u + 116) == 0) {
        *W(sp + 36) = 0x08E75AD8u;
        *W(sp + 32) = pal;
        call_on(c, 0x4400, 0x088146B0u, f_089E04EC);
        c->r[16] = c->r[2];
        call_on(c, 0x2F400, 0x088146C0u, f_089E04EC);
        c->r[17] = c->r[2];
        call_on(c, 0x400, 0x088146CCu, f_089E04EC);
        uint32_t p3 = c->r[2];
        gaddr h = 0x08A055DCu;
        *W(h + 16) = c->r[16]; *W(0x08E75AD8u + 116) = c->r[16];
        *W(h + 20) = c->r[17]; *W(0x092D9D40u + 116) = c->r[17];
        *W(h + 24) = p3; *W(pal + 116) = p3;
        gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
        *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
        call_on(c, 0x10, 0x08814738u, f_08808424);
    }
    ARG(1) = *W(pal + 116);
    ARG(0) = 1; ARG(2) = 0; ARG(3) = 0; c->r[8] = 0x1D;
    c->r[31] = 0x08814754u; f_0880AEA8(c);
    call_on(c, 0, 0x0881475Cu, f_088D7EA0);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x40;
}

/* ---------------------------------------------------------------- objects */
/* (object): a ticker object's start: +4 on, set up (+12 = 1, +24 = 5, +86 = 0xC, others cleared),
   0x088B1AB4, the palettes (as 0x088B1B00) and +82's high byte a random 1 - 32. */
SFA_FUNC(088B15F4, ticker_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 0xC;
    *W(o + 164) = 0;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *W(o + 100) = 0;
    *B(o + 12) = 1; *B(o + 13) = 0;
    *H(o + 24) = 5;
    *H(o + 36) = 0;
    ARG(1) = 1;
    call_on(c, o, 0x088B1660u, f_088B1AB4);
    ARG(0) = 1; ARG(2) = 0; ARG(3) = 0; c->r[8] = 6;
    if (S8(0x09525DE3u) != 0) { ARG(1) = 0x08A317D8u; c->r[31] = 0x088B1690u; }
    else { ARG(1) = 0x08A34C58u; c->r[31] = 0x088B16B0u; }
    f_0880AEA8(c);
    c->r[31] = 0x088B16B8u; f_08821368(c);
    uint32_t r = (c->r[2] & 0x1F) + 1;
    *H(o + 82) = (uint16_t)((uint32_t)(int32_t)S16(o + 82) & 0xFF);
    *H(o + 82) = (uint16_t)((uint32_t)(int32_t)S16(o + 82) | (r << 8));
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
/* (object): ends (+4 = 3) once its fighter's (+92) +769 is set; step 0 moves it by +96 and shakes its
   owner (+88) by the s16s at 0x08A1759C for +82 frames, then +5 on, +82 = 0x33 and 0x08858B34; step 1
   waits +82 frames and ends. Drawn while going. */
SFA_FUNC(088589FC, shake_owner) {
    gaddr o = ARG(0);
    if (S8(*W(o + 92) + 769) != 0) { ARG(0) = 3; *B(o + 4) = 3; return; }
    WITH_S0(o);
    if (*B(o + 5) == 0) {
        *W(o + 16) += *W(o + 96);
        int32_t i = S16(o + 82);
        if (S8(o + 78) != 0) i = ((int32_t)*B(o + 3) << 4) + i + 0x10;
        gaddr w = *W(o + 88);
        *H(w + 18) = (uint16_t)(S16(w + 18) + S16(0x08A1759Cu + (uint32_t)i * 2));
        int16_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        if (n < 0) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *H(o + 82) = 0x33;
            call_on(c, o, 0x08858AB8u, f_08858B34);
        }
        call_on(c, o, 0x08858AC0u, f_088215C4);
    } else {
        int16_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        if (n < 0) *B(o + 4) = 3;
        else call_on(c, o, 0x08858AF0u, f_088215C4);
    }
    END_S0();
}
