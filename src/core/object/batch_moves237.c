/* Special moves with their later steps: a start by strength into one of three handler tables, a
   three-step special, a leap toward the opponent (0x089212A0), a slide that slows to a stop and a held
   special that ends on the button. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886D468(Cpu *c); void f_088370D8(Cpu *c); void f_0887D144(Cpu *c);
void f_08837098(Cpu *c); void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_0886CA2C(Cpu *c);
void f_0886DB2C(Cpu *c); void f_089A0194(Cpu *c); void f_089A0308(Cpu *c); void f_089A0454(Cpu *c);
void f_089A04A4(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void by_step(Cpu *c, gaddr o, gaddr tab, uint32_t back) {
    uint32_t fn = *W(tab + ((uint32_t)*B(o + 7) << 2));
    ARG(0) = o; ARG(1) = fn; c->r[31] = back; rt_call_indirect(c, fn);
}

/* (fighter): the first time (+196) +123 cleared, the gain by distance, 0x088370D8, 0x0887D144 without
   +540 and move strength + 0x1D; after that the step's handler from the strength's table (0x08A64478,
   0x08A64480, 0x08A64494). */
SFA_FUNC(0891AE64, start_by_strength_tables) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    int32_t k = S8(o + 196);
    if (k == 0) {
        *B(o + 196) = 1; *B(o + 123) = 0;
        call1(c, o, f_0886D468, 0x0891AE94u);
        call1(c, o, f_088370D8, 0x0891AE9Cu);
        if (S8(o + 540) == 0) call1(c, o, f_0887D144, 0x0891AEB0u);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x1D) & 0xFFFF, f_088616EC, 0x0891AEC4u);
    } else if (k == 1) {
        int32_t s = S8(o + 194);
        if (s == 0) by_step(c, o, 0x08A64478u, 0x0891AF04u);
        else if (s == 1) by_step(c, o, 0x08A64480u, 0x0891AF34u);
        else if (s == 2) by_step(c, o, 0x08A64494u, 0x0891AF64u);
    }
    leave(c, sv);
}

/* (fighter): step 0: step 1, +696 / +235 cleared, +262 = 1, +644 = 3, the meter by 4, move 0x1D and +74
   by strength from 0x08A58448; step 1: on a frame flag (+77) cleared and +74 from 0x08A5844C, ticked
   until it ends, then step 2 and move strength + 0x1E; step 2: ticked, once ended +262 cleared and
   standing. */
SFA_FUNC(0890C200, special_three_step) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 696) = 0; *B(o + 235) = 0; *B(o + 262) = 1; *B(o + 644) = 3;
        call2(c, o, 4, f_0886CA2C, 0x0890C244u);
        call2(c, o, 0x1D, f_088616EC, 0x0890C250u);
        *B(o + 74) = *B(0x08A58448u + (uint32_t)S8(o + 194));
    } else if (st == 1) {
        if (S8(o + 77) != 0) {
            int32_t s = S8(o + 194);
            *B(o + 77) = 0;
            *B(o + 74) = *B(0x08A5844Cu + (uint32_t)s);
        }
        if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x0890C2B0u);
        else {
            int32_t s = S8(o + 194);
            *B(o + 7) = 2;
            call2(c, o, (uint32_t)(s + 0x1E) & 0xFFFF, f_088616EC, 0x0890C2D0u);
        }
    } else if (st == 2) {
        if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x0890C2F4u);
        else { *B(o + 262) = 0; call1(c, o, f_0886469C, 0x0890C304u); }
    }
    leave(c, sv);
}

/* (fighter): +656 / +123 cleared, +7 on, +262 = 1, the meter by 6, 0x08837098, x a 32nd of the way to the
   opponent (+92) plus 0x0C0000 either way, a jump (y 8.0, fall -0.5625) with +928 by the opponent's +338
   from 0x08A66A6C (in the air), 0x08A66ABC (with +696) or 0x08A66B0C, y raised by a 32nd of the height
   gap plus that (capped to 0x900000), and move 0xA. */
SFA_FUNC(089212A0, leap_to_opponent) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; c->r[16] = o;
    *B(o + 656) = 0;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 123) = 0; *B(o + 262) = 1;
    *W(sp + 24) = ra;
    call2(c, o, 6, f_0886CA2C, 0x089212D8u);
    call1(c, o, f_08837098, 0x089212E0u);
    gaddr p = *W(o + 92);
    int32_t dx = (int32_t)(*W(p + 16) - *W(o + 16));
    *W(o + 96) = (uint32_t)((dx >= 0 ? dx + 0xC0000 : dx - 0xC0000) >> 5);
    *W(o + 104) = 0; *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFF7000u;
    gaddr tab = 0x08A66A6Cu;
    if (S8(p + 73) == 0) tab = S8(p + 696) != 0 ? 0x08A66ABCu : 0x08A66B0Cu;
    gaddr y = o + 0x3A0;
    *H(y) = (uint16_t)S16(tab + (uint32_t)(S8(p + 338) * 2));
    *W(sp + 16) = *W(p + 20) - *W(o + 20);
    *H(sp + 18) = (uint16_t)(S16(sp + 18) + S16(y));
    if (*W(sp + 16) > 0x900000u) *W(sp + 16) = 0x900000u;
    *W(o + 100) = *W(o + 100) + (uint32_t)((int32_t)*W(sp + 16) >> 5);
    call2(c, o, 0xA, f_088616EC, 0x089213BCu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the first time (+196) +696 / +235 = 1, y still and x's brake by strength from 0x08A98868, the
   meter by 3; then slid toward its facing as x slows, stopped (+196 = 2, +74 = 1) once it would reverse,
   ticked unless a frame flag; then ticked with 0x0886DB2C until it ends, when the opponent's +543 = 5
   and standing. */
SFA_FUNC(08995DC0, slide_to_stop) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    gaddr p = *W(o + 92);
    int32_t k = S8(o + 196);
    if (k == 0) {
        int32_t s = S8(o + 194);
        *B(o + 196) = 1; *B(o + 696) = 1; *B(o + 235) = 1;
        uint32_t v = *W(0x08A98868u + ((uint32_t)s << 2));
        *W(o + 100) = 0; *W(o + 104) = v; *W(o + 108) = 0;
        call2(c, o, 3, f_0886CA2C, 0x08995E20u);
    } else if (k == 1) {
        uint32_t v = *W(o + 96);
        if (*B(o + 11) != 0) *W(o + 16) = *W(o + 16) + v; else *W(o + 16) = *W(o + 16) - v;
        int32_t v2 = (int32_t)(v + *W(o + 104));
        *W(o + 96) = (uint32_t)v2;
        if (v2 < 0) {
            *W(o + 96) = 0; *W(o + 104) = 0; *B(o + 196) = 2; *B(o + 74) = 1;
            call1(c, o, f_08861848, 0x08995E80u);
        } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08995E9Cu);
    } else if (k == 2) {
        if (S8(o + 75) < 0) { *B(p + 543) = 5; call1(c, o, f_0886469C, 0x08995EC4u); }
        else { call1(c, o, f_0886DB2C, 0x08995ED4u); call1(c, o, f_08861848, 0x08995EDCu); }
    }
    leave(c, sv);
}

/* (fighter): with a frame flag (+77) set 0x089A0194, clear 0x089A0308; with it below zero: 0x089A0308
   once ended, 0x089A0194 without +824, else +7 on, the meter by strength + 7, 0x08837098, +824 cleared,
   +904..911 copied to +888..895, +895 = 1, 0x089A0454, 0x089A04A4 and move strength + 0x12. */
SFA_FUNC(089A0068, held_release_12) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int32_t f = S8(o + 77);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (f >= 0) {
        if (f == 0) call1(c, o, f_089A0308, 0x089A0180u);
        else call1(c, o, f_089A0194, 0x089A0170u);
    } else {
        gaddr q = o + 0x334;
        c->r[17] = q;
        if (S8(o + 75) < 0) call1(c, o, f_089A0308, 0x089A009Cu);
        else if (S8(q + 4) == 0) call1(c, o, f_089A0194, 0x089A00B8u);
        else {
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, (uint32_t)(s + 7), f_0886CA2C, 0x089A00D8u);
            call1(c, o, f_08837098, 0x089A00E0u);
            *B(q + 4) = 0;
            for (int i = 0; i < 8; i++) *B(o + 0x378 + i) = *B(o + 0x388 + i);
            *B(o + 895) = 1;
            call1(c, o, f_089A0454, 0x089A013Cu);
            call1(c, o, f_089A04A4, 0x089A0144u);
            call2(c, o, (uint32_t)(S8(o + 194) + 0x12) & 0xFFFF, f_088616EC, 0x089A0158u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
