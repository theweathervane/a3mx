/* Special-move starts with their meter change and launch speeds, one aimed at another object
   (0x08906DD8) and a part character's start that picks its strength by distance (0x089B87A8).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_08821330(Cpu *c);
void f_088D34F4(Cpu *c); void f_08863114(Cpu *c); void f_088D3A64(Cpu *c);

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
static void enter3(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }
static void speeds4(gaddr o, gaddr e) {
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
}

/* (fighter): +7 on, +262 = 1, the meter by 2, 0x08837098, a leap at 0x08821330's object (x 1/32 of the
   distance to it, 384 past it facing left), y 9.0 / -0.375 and move 5 + 2 x strength. */
SFA_FUNC(08906DD8, special_leap_at_object) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 2, f_0886CA2C, 0x08906E08u);
    call1(c, o, f_08837098, 0x08906E10u);
    call1(c, o, f_08821330, 0x08906E18u);
    uint32_t t = *W(c->r[2] + 16);
    if (*B(o + 11) == 0) t += 0x1800000u;
    uint32_t x = *W(o + 16);
    *W(o + 104) = 0;
    *W(o + 96) = (uint32_t)((int32_t)(t - x) >> 5);
    int32_t s = S8(o + 194);
    *W(o + 100) = 0x90000; *W(o + 108) = 0xFFFFA000u;
    call2(c, o, (uint32_t)((s << 1) + 5), f_088616EC, 0x08906E64u);
    leave(c, sv);
}

/* (fighter): step 8, +235 / +262 = 1, the meter by 1, 0x08837098, a hop (y 3.5 / -0.5, x by strength
   from 0x08A901E8 toward the facing) and move 0x36. */
SFA_FUNC(0897683C, special_hop_36) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 8; *B(o + 235) = 1; *B(o + 262) = 1;
    call2(c, o, 1, f_0886CA2C, 0x0897686Cu);
    call1(c, o, f_08837098, 0x08976874u);
    int32_t s = S8(o + 194);
    *W(o + 100) = 0x38000; *W(o + 108) = 0xFFFF8000u; *W(o + 104) = 0;
    uint32_t v = *W(0x08A901E8u + ((uint32_t)s << 2));
    uint32_t f = *B(o + 11);
    *W(o + 96) = v;
    if (f == 0) *W(o + 96) = 0u - *W(o + 96);
    call2(c, o, 0x36, f_088616EC, 0x089768C8u);
    leave(c, sv);
}

/* (fighter): +7 on, +73 / +262 = 1, +123 cleared, the four speeds by strength (0x08A98938), the meter by
   6, 0x08837098 and move 0x29 + strength. */
SFA_FUNC(089969E8, special_air_29) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t s = S8(o + 194);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 73) = 1; *B(o + 262) = 1; *B(o + 123) = 0;
    speeds4(o, 0x08A98938u + ((uint32_t)s << 4));
    call2(c, o, 6, f_0886CA2C, 0x08996A58u);
    call1(c, o, f_08837098, 0x08996A60u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0x29) & 0xFFFF, f_088616EC, 0x08996A74u);
    leave(c, sv);
}

/* (character): +7 on, facing the opponent, +235 = 1, 0x08863114, the meter by strength, and 0x088D3A64
   with a strength level: unlocked with no choice made 2 x strength (+1 without +290 bit 0; step 2 at
   strength 0); else by distance (+508): strength 0 step 2 and 0 / 1 at 32, strength 1 2 / 3 at 46,
   strength 2 4 / 5 at 58. */
SFA_FUNC(089B87A8, part_level_by_distance) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    step_on(o);
    c->r[17] = 1;
    int32_t face = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)face;
    call1(c, o, f_08863114, 0x089B87E0u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089B87ECu);
    int32_t s = S8(o + 194);
    uint32_t n;
    if (S8(o + 373) == 0 && S8(o + 386) == 1) {
        n = (uint32_t)(s + s);
        if ((*H(o + 290) & 1) == 0) n += 1;
        if (s == 0) *B(o + 7) = 2;
    } else if (s < 0) n = 1;
    else if (s == 0) { *B(o + 7) = 2; n = S16(o + 508) < 32 ? 0 : 1; }
    else if (s == 1) n = S16(o + 508) < 46 ? 2 : 3;
    else if (s == 2) n = S16(o + 508) < 58 ? 4 : 5;
    else n = 0;
    call2(c, o, n, f_088D3A64, 0x089B88C0u);
    leave3(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 9, 0x08837098, x 1.5 toward the facing, y by strength
   (0x08A4BA08), fall -0.47 and move 0xB + strength. */
SFA_FUNC(088FDC58, special_jump_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 9, f_0886CA2C, 0x088FDC88u);
    call1(c, o, f_08837098, 0x088FDC90u);
    *W(o + 96) = *B(o + 11) == 0 ? 0xFFFE8000u : 0x18000u;
    int32_t s = S8(o + 194);
    *W(o + 108) = 0xFFFF8800u;
    *W(o + 100) = *W(0x08A4BA08u + ((uint32_t)s << 2));
    call2(c, o, (uint32_t)(s + 0xB) & 0xFFFF, f_088616EC, 0x088FDCE8u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 5, 0x08837098, +502 = 1 with +80, the four speeds by strength
   (0x08A5AD9C) and move 7. */
SFA_FUNC(089105F0, special_speeds_7) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    uint32_t st = *B(o + 7);
    c->r[17] = 1;
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x08910624u);
    call1(c, o, f_08837098, 0x0891062Cu);
    if (S8(o + 80) != 0) *B(o + 502) = 1;
    speeds4(o, 0x08A5AD9Cu + ((uint32_t)S8(o + 194) << 4));
    call2(c, o, 7, f_088616EC, 0x0891067Cu);
    leave3(c, sv);
}

/* (character): +7 on, x 6.0 / 12.0 / 12.0 by strength (no brake), the meter by 4, 0x08837098, move 0xC +
   strength and +262 = 1. */
SFA_FUNC(089B5DC0, part_dash_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t s = S8(o + 194);
    *B(o + 7) = (uint8_t)(st + 1);
    if (s == 0) *W(o + 96) = 0x60000;
    else if (s == 1 || s == 2) *W(o + 96) = 0xC0000;
    *W(o + 104) = 0;
    call2(c, o, 4, f_088D34F4, 0x089B5E30u);
    call1(c, o, f_08837098, 0x089B5E38u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0xC), f_088616EC, 0x089B5E48u);
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by strength + 5, 0x08837098, +20 cleared, a jump (y 4.0 /
   -0.375, +73 = 1, x by strength from 0x08A43AE8 toward the facing) and move 7. */
SFA_FUNC(088EF9BC, special_jump_7_b) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    step_on(o);
    int32_t s = S8(o + 194);
    c->r[17] = 1;
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 5), f_0886CA2C, 0x088EF9F4u);
    call1(c, o, f_08837098, 0x088EF9FCu);
    *H(o + 20) = 0; *W(o + 100) = 0x40000;
    int32_t s2 = S8(o + 194);
    *B(o + 73) = 1; *W(o + 108) = 0xFFFFA000u;
    uint32_t v = *W(0x08A43AE8u + ((uint32_t)s2 << 2));
    *W(o + 96) = *B(o + 11) != 0 ? v : 0u - v;
    call2(c, o, 7, f_088616EC, 0x088EFA50u);
    leave3(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 6, 0x08837098, no brake, fall -1.625, x (toward the facing)
   and y by strength (0x08A6A488) and move 8 + strength. */
SFA_FUNC(08926E80, special_jump_8) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x08926EB0u);
    call1(c, o, f_08837098, 0x08926EB8u);
    int32_t s = S8(o + 194);
    *W(o + 104) = 0; *W(o + 108) = 0xFFFE6000u;
    gaddr e = 0x08A6A488u + ((uint32_t)s << 3);
    uint32_t v = *W(e);
    *W(o + 96) = *B(o + 11) != 0 ? v : 0u - v;
    *W(o + 100) = *W(e + 4);
    call2(c, o, (uint32_t)(s + 8) & 0xFFFF, f_088616EC, 0x08926F18u);
    leave(c, sv);
}
