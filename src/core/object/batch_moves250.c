/* Special-move steps that set +573 through 0x0886E294 once their effect is done: a dash with a part
   reset, a throw of object 0x16 and a counted special that ends in a hop. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861848(Cpu *c); void f_0886E294(Cpu *c); void f_0887FD2C(Cpu *c);
void f_08936DD8(Cpu *c); void f_08822D2C(Cpu *c); void f_0891EE94(Cpu *c); void f_0886469C(Cpu *c);
void f_0890AB14(Cpu *c); void f_0890CC04(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void fx(Cpu *c, gaddr o, int32_t a, int32_t b, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)a; ARG(2) = (uint32_t)b; c->r[31] = back; f_0887FD2C(c);
}
static void move(Cpu *c, gaddr o, uint32_t n, uint32_t back) { ARG(0) = o; ARG(1) = n; c->r[31] = back; f_088616EC(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void face81(gaddr o) { *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF; }

/* (fighter): ticked; without a frame flag +7 on, y still, +237 cleared, 0x0886E294, +573 from 0x08A70888
   by its result (by strength without +81), x 5.0 away from its facing, +83 = strength + 1, +82 cleared,
   0x08936DD8 and move 0x30; on a flag other than 2 it becomes 2, +237 toward +81 and the effect at
   (-0x25, 0x3A). */
SFA_FUNC(08936C08, fx_or_back_30) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x08936C1Cu);
    int32_t k = S8(o + 77);
    if (k == 0) {
        *W(o + 108) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 100) = 0; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x08936C48u);
        int32_t s = S8(o + 194);
        int32_t i = S8(o + 81) != 0 ? (int16_t)c->r[2] : s;
        *B(o + 573) = (uint8_t)*H(0x08A70888u + (uint32_t)(i + i));
        *W(o + 96) = *B(o + 11) != 0 ? 0xFFFB0000u : 0x50000u;
        *W(o + 104) = 0;
        *B(o + 0x53) = (uint8_t)(s + 1); *B(o + 0x52) = 0;
        call1(c, o, f_08936DD8, 0x08936CB8u);
        move(c, o, 0x30, 0x08936CC4u);
    } else if (k != 2) {
        *B(o + 77) = 2; face81(o);
        fx(c, o, -0x25, 0x3A, 0x08936D08u);
    }
    leave(c, sv);
}

/* (fighter): ticked; with +77 below zero +237 / +573 cleared, +7 on, 0x0886E294, +573 = 0xE without +81,
   object 0x16 thrown (owner's +121 / +122 / +14 / +38, +346 / +347 / +336 = 1, +255 = 4, +338 = 8) and
   0x0891EE94; on any other flag but 2 it becomes 2, +237 toward +81 and the effect at (0, 0x54). */
SFA_FUNC(0891B1E4, fx_or_throw_16) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x0891B1F8u);
    int32_t k = S8(o + 77);
    if (k < 0) {
        *B(o + 237) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 573) = 0;
        call1(c, o, f_0886E294, 0x0891B220u);
        if (S8(o + 81) == 0) *B(o + 573) = 0xE;
        ARG(0) = o; ARG(1) = 0x16; ARG(2) = 0; c->r[31] = 0x0891B244u; f_08822D2C(c);
        gaddr p = c->r[2];
        if (p != 0) {
            *B(p + 122) = *B(o + 122); *B(p + 121) = *B(o + 121);
            *B(p + 346) = 1; *B(p + 255) = 4; *B(p + 347) = 1;
            *B(p + 14) = *B(o + 14); *B(p + 336) = 1; *H(p + 38) = *H(o + 38);
            *B(p + 337) = 0; *B(p + 338) = 8; *B(p + 339) = 0;
            *W(p + 88) = o;
        }
        call1(c, o, f_0891EE94, 0x0891B2A8u);
    } else if (k != 2) {
        *B(o + 77) = 2; face81(o);
        fx(c, o, 0, 0x54, 0x0891B2E8u);
    }
    leave(c, sv);
}

/* (fighter): the count +823: at 0x2F +237 toward +81 and the effect at (-2, 0x5C); down a frame, and at
   its end +237 cleared, 0x0886E294 and +573 from 0x08A58378 (by strength, 3 with +81). Then once ended at
   strength 2 +7 on and a hop (x 4.0 toward its facing with brake -0.0195, y 11.0, fall -0.875), +82
   cleared and move 0x2C, other strengths standing; until then a frame flag clears and 0x0890AB14,
   0x0890CC04 while x is not backward, ticked. */
SFA_FUNC(0890A84C, fx_count_hop_2c) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr q = o + 0x334;
    c->r[17] = q;
    *W(c->r[29] + 24) = ra;
    int32_t v = S8(q + 3);
    if (v == 0x2F) { face81(o); fx(c, o, -2, 0x5C, 0x0890A8A0u); v = S8(q + 3); }
    if (v != 0) {
        int32_t n = v - 1;
        *B(q + 3) = (uint8_t)n;
        if ((int8_t)n != 0) goto out;
        *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x0890A8CCu);
        int32_t i = S8(o + 81) != 0 ? 3 : S8(o + 194);
        *B(o + 573) = *B(0x08A58378u + (uint32_t)i);
    }
    if (S8(o + 75) < 0) {
        if (S8(o + 194) == 2) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 73) = 1;
            *W(o + 100) = 0xB0000; *W(o + 108) = 0xFFFF2000u;
            if (*B(o + 11) != 0) { *W(o + 96) = 0x40000; *W(o + 104) = 0xFFFFFB00u; }
            else { *W(o + 96) = 0xFFFC0000u; *W(o + 104) = 0x500; }
            *H(o + 82) = 0;
            move(c, o, 0x2C, 0x0890A9B4u);
        } else call1(c, o, f_0886469C, 0x0890A94Cu);
    } else {
        if (S8(o + 77) != 0) { *B(o + 77) = 0; call1(c, o, f_0890AB14, 0x0890A914u); }
        if ((int32_t)*W(o + 96) >= 0) call1(c, o, f_0890CC04, 0x0890A928u);
        call1(c, o, f_08861848, 0x0890A930u);
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
