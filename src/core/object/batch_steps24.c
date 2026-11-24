/* Steps (0.153.0): the 1 function of 9 - 200 instructions made ready by the batch before and the next
   of 201 - 400. A ranking row's start, and two fighters' bodies pushed apart. Offsets stay numbers
   until something shows what they are. What the original leaves in a0 - t1 at a call is set as the
   original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define RANK_TABLE 0x09525DE8u   /* u32: the ranking table shown (38 entries of 20 bytes) */

void f_08821330(Cpu *c);   /* object_routine_by_14 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_088BD4A0(Cpu *c);   /* ranking_row_cells */
void f_0882196C(Cpu *c);   /* object_set_animation */

/* (row): a ranking row's start: lit (+1, +12 1, +13 0xB, +14 1, +24 7, layer 0), the entry's new flag
   (+15) taken into +121, placed from its layer's camera (+0xE0, +0x104) sliding left (-0x80000); the
   face (kind 0x23 type 1, its ISM plate kind 0xB / (+14 mod 3) + 1 linked both ways), the profile box
   (kind 0x23 type 9), its cells; +86 0x16, +164 1, the cells' list 0x08A3848C at +156, +148 6, animation
   from 0x08A39234 by row; a new first-place row (type 0xA) 0x180 further right. */
SFA_FUNC(088BCDD8, ranking_row_start) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x30;
    c->r[16] = o; c->r[19] = 1;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1; *B(o + 9) = 0; *B(o + 14) = 1; *H(o + 24) = 7; *B(o + 11) = 0;
    *B(o + 12) = 1; *B(o + 13) = 0xB;
    gaddr e = *W(RANK_TABLE) + *B(o + 10) * 20;
    c->r[18] = e;
    *B(o + 121) = (uint8_t)S8(e + 15);
    *B(e + 15) = 0;
    c->r[4] = o; f_08821330(c);
    gaddr cam = c->r[2];
    *H(o + 18) = (uint16_t)(S16(cam + 18) + 0xE0);
    *H(o + 22) = (uint16_t)(S16(cam + 22) + 0x104);
    *W(o + 96) = 0xFFF80000u;
    c->r[4] = 0x23; c->r[5] = 1; f_08822EAC(c);
    gaddr face = c->r[2];
    c->r[17] = face;
    if (face != 0) {
        *W(face + 88) = o;
        *B(face + 10) = *B(o + 10);
        DIV(c, (uint32_t)(int32_t)S8(e + 14), 3);
        c->r[4] = 0xB; c->r[5] = (c->hi + 1) & 0xFF; f_08823338(c);
        gaddr plate = c->r[2];
        c->r[18] = plate;
        if (plate != 0) { *W(plate + 124) = face; *W(face + 92) = plate; }
        c->r[4] = 0x23; c->r[5] = 9; f_08822EAC(c);
        c->r[17] = c->r[2];
        if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    }
    c->r[4] = o; f_088BD4A0(c);
    *H(o + 86) = 0x16;
    *W(o + 164) = 1;
    *W(o + 156) = 0x08A3848Cu;
    *W(o + 148) = 6;
    c->r[4] = o; c->r[5] = *W(0x08A39234u + *B(o + 10) * 4); f_0882196C(c);
    if (*B(o + 3) == 0xA && S8(o + 121) != 0) *H(o + 18) = (uint16_t)(S16(o + 18) + 0x180);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* Move a fighter's x (+16, 16.16) by `d`. */
static void move_x(gaddr f, uint32_t d) { *W(f + 16) = *W(f + 16) + d; }

/* Both free to move: each half the push, away from the other (by which side of it b is, and their
   x when b is on the left). */
static void push_both(gaddr a, gaddr b, uint32_t push, int b_left) {
    uint32_t half = (uint32_t)((int32_t)push >> 1), ma = 0u - half, mb = half;
    if (b_left && !(S16(a + 18) < S16(b + 18))) { ma = half; mb = 0u - half; }
    move_x(a, ma);
    move_x(b, mb);
}

/* (a, b): two fighters' bodies (box +220 by the first frame byte +256 +3, 8 bytes: x, y, half width,
   half height) pushed apart when they overlap and neither has +135, negative health or +282: both
   +648 on; a fighter at a wall (+238 1 left, 2 right) stays and the other takes the whole overlap;
   else one in the air (+712 / +73) is pushed alone, or both by half. */
SFA_FUNC(08875FB8, fighters_push_apart) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ka = *B(*W(a + 256) + 3), kb;
    if (ka == 0 || (kb = *B(*W(b + 256) + 3)) == 0) return;
    if (S8(a + 135) != 0 || S8(b + 135) != 0) return;
    if (S16(a + 112) < 0 || S16(b + 112) < 0) return;
    gaddr ba = *W(a + 220) + ka * 8, bb = *W(b + 220) + kb * 8;
    int32_t dy = (S16(bb + 2) + S16(b + 22)) - (S16(ba + 2) + S16(a + 22));
    if (dy < 0) dy = -dy;
    if (dy - (S16(ba + 6) + S16(bb + 6)) >= 0) return;
    int32_t xa = *B(a + 11) ? S16(a + 18) - S16(ba) : S16(ba) + S16(a + 18);
    int32_t xb = *B(b + 11) ? S16(b + 18) - S16(bb) : S16(bb) + S16(b + 18);
    int32_t d = xb - xa;
    int b_left = 0;
    if (d < 0) { d = -d; b_left = 1; }
    int32_t over = d - (S16(ba + 4) + S16(bb + 4));
    if (over > 0) return;
    *B(a + 648) = (uint8_t)(S8(a + 648) + 1);
    *B(b + 648) = (uint8_t)(S8(b + 648) + 1);
    if (S8(a + 282) != 0 || S8(b + 282) != 0) return;
    uint32_t push = (uint32_t)(-over) << 16;
    int32_t ea = S8(a + 238), eb = S8(b + 238);
    int a_air = S8(a + 712) != 0 || S8(a + 73) != 0;
    int b_air = S8(b + 712) != 0 || S8(b + 73) != 0;
    if (ea != 0) {
        if (eb == 0) { move_x(b, ea == 1 ? push : 0u - push); return; }   /* a delay slot sets the 1 */
        if (a_air) { move_x(a, eb == 1 ? push : 0u - push); return; }
        if (b_air) { move_x(b, ea == 1 ? push : 0u - push); return; }
        move_x(a, eb == 1 ? push : 0u - push);
        return;
    }
    if (eb != 0) { move_x(a, eb == 1 ? push : 0u - push); return; }
    if (a_air) {
        if (b_air) push_both(a, b, push, b_left);
        else move_x(a, b_left ? push : 0u - push);
    } else {
        if (b_air) move_x(b, b_left ? 0u - push : push);
        else push_both(a, b, push, b_left);
    }
}
