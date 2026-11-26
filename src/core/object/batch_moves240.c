/* A chained special with a follow-up count (0x08930690) and a special-move start by button with a hop.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_08861848(Cpu *c); void f_08863114(Cpu *c);
void f_0886CA2C(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
/* 0x20 frame with s0..s2 at 16..24 and ra at 28 */
static void enter4(Cpu *c, gaddr o, uint32_t sv[4]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[18]; sv[3] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2]; *W(c->r[29] + 28) = sv[3];
    c->r[16] = o;
}
static void leave4(Cpu *c, const uint32_t sv[4]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = sv[3]; c->r[29] += 0x20;
}

/* (fighter): ticked; once ended with a follow-up asked (+823) and fewer than 2 taken (+824), the next one
   (strength down a step when unlocked, the meter by 1, move 0xF + its entry from 0x08A6DB0A); without,
   +7 on, strength from that entry, +502 = 1 and move strength + 0x12. While it runs a follow-up is asked
   by +760 against the strength's mask from 0x08A6DB04 (locked), or any strength but 0 (unlocked). */
SFA_FUNC(08930690, chained_special) {
    gaddr o = ARG(0); uint32_t sv[4];
    enter4(c, o, sv);
    call1(c, o, f_08861848, 0x089306ACu);
    gaddr q = o + 0x334;
    c->r[18] = q;
    if (S8(o + 75) < 0) {
        const gaddr tab = 0x08A6DB0Au;
        c->r[17] = tab;
        if (S8(q + 3) != 0) {
            int32_t i = S8(q + 4);
            *B(q + 3) = 0;
            if (i != 2) {
                *B(q + 4) = (uint8_t)(i + 1);
                if (S8(o + 373) != 0) *B(o + 194) = (uint8_t)(S8(o + 194) - 1);
                call2(c, o, 1, f_0886CA2C, 0x08930704u);
                uint32_t e = *H(tab + (uint32_t)(S8(q + 4) * 2));
                call2(c, o, (e + 0xF) & 0xFFFF, f_088616EC, 0x08930724u);
                leave4(c, sv); return;
            }
        }
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 194) = (uint8_t)*H(tab + (uint32_t)(S8(q + 4) * 2));
        *B(o + 502) = 1;
        call2(c, o, (uint32_t)(S8(o + 194) + 0x12) & 0xFFFF, f_088616EC, 0x08930778u);
    } else {
        int32_t s = S8(o + 194);
        if (S8(o + 373) == 0) { if (*H(o + 760) & *H(0x08A6DB04u + (uint32_t)(s + s))) *B(q + 3) = 1; }
        else if (s != 0) *B(q + 3) = 1;
    }
    leave4(c, sv);
}

/* (fighter): +7 on, facing the opponent, +235 = 1, 0x08863114; at strength 1 with +290 bit 1 and a choice
   made step 2, the meter by 1, a hop (x 3.375 toward its facing, y 4.0, fall -0.375), +123 cleared and
   move 0x1F; otherwise the meter by strength and move 2 x strength + 6 (+1 unlocked with no choice made
   without +290 bit 0, else beyond the distance from 0x08A43A32; 0xA becomes 0x24 with no choice made). */
SFA_FUNC(088EE5E8, special_by_button_hop) {
    gaddr o = ARG(0); uint32_t sv[4];
    enter4(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[18] = 1;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, 0x088EE624u);
    c->r[17] = 0xFFFFFFFFu;
    int32_t k = S8(o + 386), s = S8(o + 194);
    if (k != -1 && s == 1 && (*H(o + 290) & 2)) {
        *B(o + 7) = 2;
        call2(c, o, 1, f_0886CA2C, 0x088EE660u);
        *W(o + 96) = *B(o + 11) != 0 ? 0x36000u : 0xFFFCA000u;
        *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA000u; *B(o + 123) = 0;
        call2(c, o, 0x1F, f_08861674, 0x088EE6A4u);
        leave4(c, sv); return;
    }
    call2(c, o, (uint32_t)s, f_0886CA2C, 0x088EE6B8u);
    s = S8(o + 194);
    k = S8(o + 386);
    uint32_t n = (uint32_t)((s << 1) + 6) & 0xFFFF;
    if (S8(o + 373) == 0 && k == 1) { if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF; }
    else if (S16(0x08A43A32u + (uint32_t)(s + s)) < S16(o + 508)) n = (n + 1) & 0xFFFF;
    if (k == -1 && n == 0xA) n = 0x24;
    call2(c, o, n, f_08861674, 0x088EE740u);
    leave4(c, sv);
}
