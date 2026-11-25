/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 39 - 147
   instructions: the VS screen's start with its sparkle, a mode screen's set-up by the chosen mode, a
   menu icon bouncing in its box. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08821368(Cpu *c); void f_08822F8C(Cpu *c); void f_0889DBE4(Cpu *c); void f_0880AEA8(Cpu *c);
void f_08822EAC(Cpu *c); void f_08823338(Cpu *c); void f_088A9AD8(Cpu *c); void f_088AC3A8(Cpu *c);
void f_08821424(Cpu *c); void f_0882196C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t set, uint32_t from, uint32_t slot, uint32_t count, uint32_t back) {
    ARG(0) = set; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = count;
    c->r[31] = back; f_0880AEA8(c);
}

/* (screen, battle): the VS screen's start: +6 on, its objects (0x0889DBE4), the battle object's +82
   cleared and a list-1 sparkle (0x08822F8C: +0 / +3 = 1, +14 cleared, +42 = 0xC, +26 = 0x39, +30 =
   0xB8, +32 = 0x1001, its picture one of eight 128-byte ones at 0x08A0583C's word, at random). */
SFA_FUNC(08827E38, vs_start_sparkle) {
    gaddr o = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = b;
    call_on(c, b, 0x08827E5Cu, f_0889DBE4);
    *B(b + 82) = 0;
    call2(c, 1, 0, 0x08827E6Cu, f_08822F8C);
    gaddr n = c->r[2];
    c->r[16] = n;
    if (n != 0) {
        *B(n) = 1; *B(n + 3) = 1; *B(n + 14) = 0; *B(n + 42) = 0xC;
        *H(n + 26) = 0x39; *H(n + 30) = 0xB8; *H(n + 32) = 0x1001;
        c->r[31] = 0x08827EA8u; f_08821368(c);
        *W(n + 44) = *W(0x08A05834u + 8) + ((c->r[2] & 7) << 7);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen): a mode screen's set-up: +8 on, 0x088AC3A8 (the new +8), the tickers reset (0x08B027E0 =
   0x400000, 0x08B02810 = 0x6800000), +9 and GAME +10 cleared, +76 cleared, +77 = the mode (0x0951CE52's
   high byte), 0x088A9AD8 (4, 4); from two players a list-0x36 type-2 object (+78 by the mode: 5 -> 7,
   6 -> 8, 8 -> 4, 11 -> 6, 14 -> 9, 17 -> 5), two list-7 objects (types 6, 7; none free ends the set-up
   here) and a list-0x36 type-5 one; then the mode's 8 palettes (0x08A35100 / 0x08A35180) into slot
   0x20, 0x08A31718 into slot 1 and 0x08A30398 into slot 0x1F (set 2). */
SFA_FUNC(088AC974, mode_screen_by_mode) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = s2; *W(c->r[29] + 44) = ra;
    c->r[16] = o; c->r[17] = 0;
    uint32_t n8 = *B(o + 8) + 1u;
    *B(o + 8) = (uint8_t)n8;
    call_on(c, n8, 0x088AC9A0u, f_088AC3A8);   /* (a0 is the new +8, as the original leaves it) */
    *H(0x08B02810u + 18) = 0; *H(0x08B02810u + 16) = 0;
    *H(0x08B027E0u + 18) = 0; *H(0x08B027E0u + 16) = 0;
    *H(0x08B027B0u + 18) = 0; *H(0x08B027B0u + 16) = 0;
    *B(o + 9) = 0;
    *W(0x08B02814u) = 0; *W(0x08B027E4u) = 0; *W(0x08B027B4u) = 0; *W(0x08B027B0u) = 0;
    *H(GAME + 10) = 0;
    *W(0x08B027E0u) = 0x400000;
    *W(0x08B02810u) = 0x6800000;
    *B(o + 77) = (uint8_t)((*H(0x0951CE52u) & 0xFF00) >> 8);
    *B(o + 76) = 0;
    call2(c, 4, 4, 0x088ACA2Cu, f_088A9AD8);
    if (!(S8(0x095176F3u) < 2)) {
        call2(c, 0x36, 2, 0x088ACA48u, f_08822EAC);
        gaddr n = c->r[2];
        c->r[17] = 1;
        *B(n) = 1;
        switch (*B(o + 77)) {
            case 5: *B(n + 78) = 7; break;
            case 6: *B(n + 78) = 8; break;
            case 8: *B(n + 78) = 4; break;
            case 11: *B(n + 78) = 6; break;
            case 14: *B(n + 78) = 9; break;
            case 17: *B(n + 78) = 5; break;
            default: break;
        }
        for (c->r[18] = 6;;) {
            call2(c, 7, c->r[18] & 0xFF, 0x088ACAD4u, f_08823338);
            c->r[18]++;
            if (c->r[2] == 0) goto done;
            if (!((int32_t)c->r[18] < 8)) break;
        }
        call2(c, 0x36, 5, 0x088ACAF4u, f_08822EAC);
        *B(c->r[2]) = 1;
    }
    {
        uint32_t k4 = (uint32_t)(*B(o + 77) & 0x1F) << 2;
        c->r[16] = k4;
        if (S8(0x09525DE3u) != 0) palette(c, 2, *W(0x08A35100u + k4), 0x20, 8, 0x088ACB34u);
        else palette(c, 2, *W(0x08A35180u + k4), 0x20, 8, 0x088ACB60u);
        palette(c, 2, 0x08A31718u, 1, 2, 0x088ACB80u);
        palette(c, 2, 0x08A30398u, 0x1F, 1, 0x088ACB9Cu);
    }
done:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (icon): a menu icon (+3 its place): while its place is the menu's cursor (GAME +10) it shades
   (+160 = 0xFF, else 0x68), jitters (x / y step -4 - 3 at random) inside its box (+3's 4-byte row of
   0x08A2E5C8, 6 either side, the step turned at the edges) and plays its animation (0x08A2E59C's list,
   8 bytes by +10); +12 / +13 = 1 / 0, 1 / 1 or 1 / 2 (by +78); +1 cleared; the palettes 0x08A31898 and
   0x08A32BD8; drawn (0x08821424) while GAME +16's high byte is 0, else +4 on; +82 counts frames and
   from 24 +4 on. */
SFA_FUNC(088AEAA4, menu_icon) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0]; *W(sp + 48) = sv[4];
    c->r[16] = o;
    *B(o + 160) = 0x68;
    c->r[20] = GAME;
    *W(sp + 36) = sv[1]; *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3]; *W(sp + 52) = sv[5]; *W(sp + 56) = ra;
    const gaddr boxes = 0x08A2E5C8u;
    c->r[21] = boxes;
    int32_t dx = 0, dy = 0;
    uint32_t shade = 0x101;
    c->r[17] = 0; c->r[18] = 0; c->r[19] = 0x101;
    if ((*H(GAME + 10) & 0xFF) == *B(o + 3)) {
        *B(o + 160) = 0xFF;
        c->r[31] = 0x088AEB08u; f_08821368(c);
        int32_t v = (int16_t)(c->r[2] & 0xFF);
        dx = (int16_t)((v & 7) - 4);
        dy = dx;
        c->r[17] = (uint32_t)dx; c->r[18] = (uint32_t)dy; c->r[19] = 0x100;
        call2(c, o, *W(0x08A2E550u + 0x4C) + ((uint32_t)*B(o + 10) << 3), 0x088AEB5Cu, f_0882196C);
        shade = 0x100;
    }
    gaddr box = boxes + ((uint32_t)*B(o + 3) << 2);
    int32_t x = S16(o + 18), y = S16(o + 22);
    if (S8(o + 78) != 0) shade = 0x102;
    *B(o + 12) = (uint8_t)((shade & 0xFF00) >> 8);
    *B(o + 13) = (uint8_t)(shade & 0xFF);
    int32_t left = *H(box), top = *H(box + 2);
    if (left + 6 < x && dx > 0) dx = (int16_t)~dx;
    if (x < left - 6 && dx < 0) dx = (int16_t)~dx;
    if (top + 6 < y && dy > 0) dy = (int16_t)~dy;
    if (y < top - 6 && dy < 0) dy = (int16_t)~dy;
    c->r[17] = (uint32_t)dx; c->r[18] = (uint32_t)dy; c->r[19] = (uint32_t)left;
    *H(o + 18) = (uint16_t)(x + dx);
    *H(o + 22) = (uint16_t)(y + dy);
    *B(o + 1) = 0;
    palette(c, 1, 0x08A31898u, 0, 2, 0x088AEC58u);
    palette(c, 1, 0x08A32BD8u, 2, 1, 0x088AEC74u);
    if ((((int32_t)S16(GAME + 16) & 0xFF00) >> 8) == 0) call_on(c, o, 0x088AEC90u, f_08821424);
    else *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
    if (!(S16(o + 82) < 24)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
