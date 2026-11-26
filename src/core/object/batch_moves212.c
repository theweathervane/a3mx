/* Steps of 67 - 111 instructions: an options page that steps two values with left / right and leaves
   on back or confirm (0x088CC798), and the throw check against one opponent by its hit box
   (0x0893B674). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088CD120(Cpu *c); void f_0887C4CC(Cpu *c); void f_0880A39C(Cpu *c); void f_0880AA74(Cpu *c);
void f_08873AD8(Cpu *c); void f_0886AAE8(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void sound(Cpu *c, uint32_t n, uint32_t back) { ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c); }

/* (arg): once the game's +4 is set, the page step from 0x08A3C684 by +6 (with the argument); else both
   players' pads (0x08E75BA0, +0x2C) step +6 through 0 - 4; back (0x8080) leaves (sound 0x39,
   0x0880A39C, +4 = 1, repeat delay 20), confirm (0x100) 0x0880AA74, sound 0x2F, 0x0880A39C and +2
   counts. */
SFA_FUNC(088CC798, options_page_two) {
    uint32_t arg = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = GAME;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (*H(GAME + 4) != 0) {
        uint32_t fn = *W(0x08A3C684u + ((uint32_t)*H(GAME + 6) << 2));
        ARG(0) = arg; c->r[6] = fn;
        c->r[31] = 0x088CC890u; rt_call_indirect(c, fn);
    } else {
        gaddr pad = 0x08E75BA0u;
        c->r[17] = pad;
        ARG(0) = pad; ARG(1) = *H(GAME + 6); ARG(2) = 4; c->r[31] = 0x088CC7D8u; f_088CD120(c);
        *H(GAME + 6) = (uint16_t)c->r[2];
        ARG(0) = pad + 0x2C; ARG(1) = *H(GAME + 6); ARG(2) = 4; c->r[31] = 0x088CC7ECu; f_088CD120(c);
        uint32_t b = *W(pad + 12) | *W(pad + 56);
        *H(GAME + 6) = (uint16_t)c->r[2];
        if (b & 0x8080) {
            sound(c, 0x39, 0x088CC814u);
            c->r[31] = 0x088CC81Cu; f_0880A39C(c);
            *H(GAME + 4) = 1;
            *B(0x08B02E76u) = 0x14;
        } else if (b & 0x100) {
            ARG(0) = 0x100; c->r[31] = 0x088CC84Cu; f_0880AA74(c);
            sound(c, 0x2F, 0x088CC85Cu);
            c->r[31] = 0x088CC864u; f_0880A39C(c);
            *H(GAME + 2) = (uint16_t)(*H(GAME + 2) + 1);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, box index, extra): against an opponent that is not blocking, is in state 1 / 2 or 1 / 0
   (or +816, or 1 / 1 hit with +116 = 6), while the match runs, standing and not held, with a live hit
   box (+256 bytes 0 - 2): 0x08873AD8 checks our box (+224 + 32 x index) against its (+220 + 8 x +3);
   on a hit, 0x0886AAE8 takes it (1), unless it is crouched on a frame of kind 1 - 3 or 6 (-1). Else 0. */
SFA_FUNC(0893B674, throw_check_box) {
    gaddr o = ARG(0);
    uint32_t idx = ARG(1), extra = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 36) = s1;
    gaddr opp = *W(o + 92);
    c->r[17] = opp;
    *W(c->r[29] + 32) = s0; c->r[16] = o;
    *W(c->r[29] + 40) = s2; c->r[18] = idx;
    *W(c->r[29] + 44) = ra;
    int32_t ret = 0;
    int ok = (S8(opp + 543) | S8(opp + 573)) == 0;
    if (ok) {
        uint32_t a = *B(opp + 4), b = *B(opp + 5);
        ok = (a == 1 && b == 2) || (a == 1 && b == 0) || *W(opp + 816) != 0
             || (a == 1 && b == 1 && *B(opp + 116) == 6);
    }
    if (ok) ok = (S8(MATCH + 14) | S8(opp + 73) | S8(opp + 135)) == 0;
    gaddr p = *W(opp + 256);
    if (ok) ok = *B(p) != 0 || *B(p + 1) != 0 || *B(p + 2) != 0;
    if (ok) {
        gaddr mine = *W(o + 224) + (idx << 5);
        c->r[18] = mine;
        gaddr theirs = *W(opp + 220) + ((uint32_t)*B(p + 3) << 3);
        ARG(0) = o; ARG(1) = opp; ARG(2) = mine; c->r[7] = theirs; c->r[8] = extra;
        c->r[31] = 0x0893B78Cu; f_08873AD8(c);
        if (c->r[2] != 0) {
            uint32_t f = *B(*W(opp + 296) + 5);
            int take = f == 0 || S8(opp + 696) == 0 || !((int32_t)f < 4 || f == 6);
            if (take) {
                ARG(0) = o; ARG(1) = opp; ARG(2) = mine; c->r[31] = 0x0893B7F8u; f_0886AAE8(c);
                ret = 1;
            } else ret = -1;
        }
    }
    c->r[2] = (uint32_t)ret;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}
