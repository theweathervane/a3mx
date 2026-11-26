/* A mashed special (0x0896D2AC) and its start (0x0896D22C): button presses (+760 bits 4..6) past a
   level from 0x08A8DE64 restart it at that strength, and it ends by 0x0896D594 when its count (+83) runs
   out, a pause / round flag is set or 0x088632C4 says so. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886E3CC(Cpu *c); void f_088632C4(Cpu *c); void f_0896D594(Cpu *c); void f_0886DC3C(Cpu *c);
void f_08837098(Cpu *c); void f_08861848(Cpu *c); void f_08861900(Cpu *c); void f_08877258(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
/* 0x0886DC3C, 0x08837098, +551 = -1 and ticked */
static void again(Cpu *c, gaddr o, const uint32_t b[3], uint32_t bt) {
    call1(c, o, f_0886DC3C, b[0]); call1(c, o, f_08837098, b[1]);
    *B(o + 551) = 0xFF;
    call1(c, o, f_08861848, bt);
}

/* (fighter): 0x0886E3CC (7, 4); ended while 0x092E1228 +13 / +14 or 0x08E75910 +38 hold or 0x088632C4
   says so. With no choice made: a press of strength 0 / 1 / 2 (+760 bits 4 / 5 / 6) with +83 above its
   level sets +83 = 0xF and restarts the move at that strength (a new strength takes the frame for move
   0xF + strength, +28 = its entry plus 10 x +76, and 0x08861900); without a press +83 counts down and ends
   it at 0. With a choice made: ticked until its animation ends, then restarted (+551 = -1 if clear) and
   with +193 ended once +83 is 0, else ended when 0x08877258 (0x08A8DAD0) says so. */
SFA_FUNC(0896D2AC, mashed_special) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    ARG(0) = o; ARG(1) = 7; ARG(2) = 4; c->r[31] = 0x0896D2C8u; f_0886E3CC(c);
    const gaddr g = 0x092E1228u;
    gaddr cnt = o + 0x53;
    if ((int8_t)(S8(g + 13) | S8(g + 14) | S8(0x08E75910u + 38)) != 0) goto end;
    call1(c, o, f_088632C4, 0x0896D304u);
    if (c->r[2] != 0) goto end;
    if (S8(o + 373) == 0) {
        uint32_t h = *H(o + 760);
        if (h & 0x70) {
            int32_t lv = (h & 0x10) ? 0 : (h & 0x20) ? 1 : 2;
            int32_t lim = *H(0x08A8DE64u + (uint32_t)(lv + lv));
            if (lim < (int32_t)((uint32_t)S8(cnt) & 0xFFFF)) {
                *B(cnt) = 0xF;
                if (S8(o + 194) == lv) {
                    static const uint32_t b[3] = { 0x0896D398u, 0x0896D3A0u };
                    again(c, o, b, 0x0896D3B0u);
                } else {
                    *B(o + 194) = (uint8_t)lv;
                    int32_t s = S8(o + 194);
                    *H(o + 84) = (uint16_t)(s + 0xF);
                    gaddr tbl = *W(*W(o + 416) + 4);
                    uint32_t v = *W(tbl + ((uint32_t)*H(o + 84) << 2));
                    int32_t f = S8(o + 76);
                    *W(o + 28) = v;
                    *W(o + 28) = v + (uint32_t)(f * 10);
                    call1(c, o, f_08861900, 0x0896D408u);
                }
                goto out;
            }
        }
        int32_t n = S8(cnt) - 1;
        *B(cnt) = (uint8_t)n;
        if ((int8_t)n == 0) goto end;
        static const uint32_t b2[3] = { 0x0896D43Cu, 0x0896D444u };
        again(c, o, b2, 0x0896D454u);
        goto out;
    }
    if (S8(o + 75) == 0) { call1(c, o, f_08861848, 0x0896D518u); goto out; }
    call1(c, o, f_0886DC3C, 0x0896D474u);
    call1(c, o, f_08837098, 0x0896D47Cu);
    if (S8(o + 551) == 0) *B(o + 551) = 0xFF;
    if (S8(o + 193) != 0) {
        if (S8(cnt) == 0) call1(c, o, f_0896D594, 0x0896D4D4u);
        else call1(c, o, f_08861848, 0x0896D4BCu);
    } else {
        ARG(0) = o; ARG(1) = 0x08A8DAD0u; c->r[31] = 0x0896D4F0u; f_08877258(c);
        if (c->r[2] != 0) call1(c, o, f_0896D594, 0x0896D500u);
        else call1(c, o, f_08861848, 0x0896D518u);
    }
    goto out;
end:
    call1(c, o, f_0896D594, 0x0896D530u);
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): on a frame flag +7 on, +83 = 0xF (with a choice made from 0x08A8DE54 by +193), +656 = 0x1D
   and the mashed special; else ticked. */
SFA_FUNC(0896D22C, mashed_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 0x53) = 0xF;
        *B(o + 656) = 0x1D;
        if (S8(o + 373) != 0) *B(o + 0x53) = *B(0x08A8DE54u + (uint32_t)S8(o + 193));
        c->r[31] = 0x0896D28Cu; f_0896D2AC(c);
    } else { c->r[31] = 0x0896D2A0u; f_08861848(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}
