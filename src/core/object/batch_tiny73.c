/* Small functions the scenarios reach, 78 - 81 instructions: a fighter's picture table with a
   list-0x46 helper on a button pair, an effect shown while its owner has a level, the three pads'
   inputs fed on a network beat. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_08822EAC(Cpu *c); void f_0886EB48(Cpu *c); void f_0887DF88(Cpu *c); void f_088D3B88(Cpu *c);
void f_08804F50(Cpu *c); void f_088D59A0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A74290, 0x0886EB48 (0xF,
   0x08A7432C), +86 = its side, +164 = 1; outside mode 2 (GAME +114) in a plain battle (BATTLE +59 /
   +56 clear), with BATTLE +0 = 0xF or a player (+373 clear) holding 0x210 (+362), a list-0x46 helper
   (0x08822EAC) pointing back at it (+88) with its side (as +122 / +86), +121 and +164. */
SFA_FUNC(0893BA20, fighter_table_copy_f) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0xF;
    *W(c->r[29] + 24) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A74290u + 4 * n);
    ARG(0) = f; ARG(1) = 0xF; ARG(2) = 0x08A7432Cu;
    c->r[31] = 0x0893BA88u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    if (S8(GAME + 114) != 2 && S8(BATTLE + 59) == 0 && S16(BATTLE + 56) == 0 &&
        (S16(BATTLE) == 0xF || (S8(f + 373) == 0 && (*H(f + 362) & 0x210) == 0x210))) {
        call2(c, 0x46, 0, 0x0893BB0Cu, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *W(n + 88) = f;
            *B(n + 122) = (uint8_t)S8(f + 337);
            *B(n + 121) = (uint8_t)S8(f + 121);
            *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
            *W(n + 164) = *W(f + 164);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): first (+5 = 0), unless its owner (+88) has +539, it waits for a level (+296's +10 or
   +11), then shows on the owner (on, its place and side, 0x20 ahead or 0x38 for +11 = 1) via
   0x0887DF88 (0x47); later it runs (0x088D3B88) until +75 goes negative and the level is gone, then
   off (+1 / +5 cleared). */
SFA_FUNC(089C440C, effect_owner_level_show) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 5);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    if (st == 0) {
        if (S8(p + 539) != 0) *B(o + 5) = 0;
        else {
            gaddr q = *W(p + 296);
            if (*B(q + 10) != 0 || *B(q + 11) != 0) {
                *B(o + 5) = (uint8_t)(st + 1);
                *B(o + 1) = 1;
                *W(o + 16) = *W(p + 16);
                *W(o + 20) = *W(p + 20);
                *B(o + 11) = *B(p + 11);
                int32_t off = *B(*W(p + 296) + 11) == 1 ? 0x38 : 0x20;
                int32_t x = S16(o + 18);
                c->r[17] = (uint32_t)x;
                *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + off : x - off);
                call2(c, o, 0x47, 0x089C44ECu, f_0887DF88);
            }
        }
    } else {
        if (S8(o + 75) >= 0) call_on(c, o, 0x089C4508u, f_088D3B88);
        if (S8(o + 75) < 0) {
            gaddr q = *W(p + 296);
            if (*B(q + 10) == 0 && *B(q + 11) == 0) { *B(o + 5) = 0; *B(o + 1) = 0; }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (link): with the link up (0x088D59A0 on 0x09523980) on its beat (+35 a multiple of 0x08A3EBA8, its
   +32 not above 0), the three pads' inputs (+4 / +8 / +12) fed in (0x08804F50 on 0x08E75BA0 /
   +0x2C / +0x58 with 8-byte records at 0x08E75C28: pad, 1, input) and the beat counted; 1 then,
   else 0. */
SFA_FUNC(088E29A0, link_feed_pads) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = o;
    *W(sp + 20) = sv[1];
    uint32_t beat = *B(o + 35);
    c->r[17] = beat;
    for (int i = 2; i < 6; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 40) = ra;
    call_on(c, 0x09523980u, 0x088E29D8u, f_088D59A0);
    uint32_t ok = 0;
    if (c->r[2] != 0) {
        uint32_t m = *B(0x08A3EBA8u);
        if (m != 0 && S8(o + 32) <= 0) {
            DIV(c, beat, m);
            ok = c->hi < 1u;
        }
    }
    uint32_t r = 0;
    if (ok) {
        uint32_t in[3] = { *W(o + 4), *W(o + 8), *W(o + 12) };
        c->r[17] = in[1]; c->r[18] = in[2];
        gaddr rec = 0x08E75C28u;
        c->r[19] = rec; c->r[20] = 1; c->r[21] = 0x08E75BA0u;
        static const uint32_t back[3] = { 0x088E2A54u, 0x088E2A6Cu, 0x088E2A88u };
        for (uint32_t k = 0; k < 3; k++) {
            gaddr e = rec + 8 * k;
            *H(e) = (uint16_t)k; *H(e + 2) = 1; *W(e + 4) = in[k];
            call2(c, 0x08E75BA0u + 0x2C * k, e, back[k], f_08804F50);
        }
        *B(o + 35) = (uint8_t)(*B(o + 35) + 1);
        r = 1;
    }
    c->r[2] = r;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
