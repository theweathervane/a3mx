/* Special-move starts with their meter change and launch speeds, and rush starts by guard and button.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_08863A50(Cpu *c); void f_0890FB68(Cpu *c); void f_089A5AEC(Cpu *c); void f_088D34F4(Cpu *c);

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
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* (fighter): move 2 x strength (+1 past the distance from 0x08A9E6F0, 0x089A5AEC) of its list. */
SFA_FUNC(089A5A3C, special_by_distance) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t s = S8(o + 194);
    uint32_t lim = (uint32_t)S16(0x08A9E6F0u + (uint32_t)(s + s));
    ARG(0) = o; ARG(1) = (uint32_t)(s << 1) & 0xFFFF; ARG(2) = lim; c->r[31] = 0x089A5A78u; f_089A5AEC(c);
    call2(c, o, c->r[2] & 0xFFFF, f_08861674, 0x089A5A84u);
    leave(c, sv);
}

/* (fighter): step 3, +235 = 1, +192 = 2, 0x08863A50, the meter by strength; at strength 1 without +193
   and with +290 bit 2 x halved and move 0x21 of its list, else 0x0890FB68. */
SFA_FUNC(0890FAB0, rush_or_fb68) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *B(o + 7) = 3; *B(o + 235) = 1; *B(o + 192) = 2;
    *W(c->r[29] + 24) = ra;
    call1(c, o, f_08863A50, 0x0890FAE4u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0890FAF0u);
    if (S8(o + 193) != 0) call1(c, o, f_0890FB68, 0x0890FB04u);
    else if (S8(o + 194) != 1) call1(c, o, f_0890FB68, 0x0890FB1Cu);
    else if ((*H(o + 290) & 4) == 0) call1(c, o, f_0890FB68, 0x0890FB38u);
    else {
        *W(o + 96) = (uint32_t)((int32_t)*W(o + 96) >> 1);
        call2(c, o, 0x21, f_08861674, 0x0890FB54u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 on, +262 = 1 and `flags`, the meter by 6, 0x08837098, the four speeds by strength from
   `tab` (x and x brake toward the facing, y, fall) and move `base` + strength. */
enum { F_696 = 1, F_502 = 2, F_73 = 4 };
static void special_speeds_f(Cpu *c, int flags, gaddr tab, uint32_t base, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    if (flags & F_696) *B(o + 696) = 0;
    if (flags & F_502) *B(o + 502) = 1;
    if (flags & F_73) *B(o + 73) = 1;
    call2(c, o, 6, f_0886CA2C, b[0]);
    call1(c, o, f_08837098, b[1]);
    int32_t s = S8(o + 194);
    gaddr e = tab + ((uint32_t)s << 4);
    if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(e); *W(o + 104) = 0u - *W(e + 4); }
    else { *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); }
    *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    call2(c, o, (uint32_t)(s + (int32_t)base) & 0xFFFF, f_088616EC, b[2]);
    leave(c, sv);
}
SFA_FUNC(089908B0, special_speeds_5a) {
    static const uint32_t b[3] = { 0x089908E4u, 0x089908ECu, 0x08990958u };
    special_speeds_f(c, F_696, 0x08A96768u, 0x5A, b);
}
SFA_FUNC(089A41D8, special_speeds_c_502) {
    static const uint32_t b[3] = { 0x089A420Cu, 0x089A4214u, 0x089A4280u };
    special_speeds_f(c, F_502, 0x08A9D630u, 0xC, b);
}
SFA_FUNC(089A3C0C, special_speeds_9_air) {
    static const uint32_t b[3] = { 0x089A3C44u, 0x089A3C4Cu, 0x089A3CB8u };
    special_speeds_f(c, F_502 | F_73, 0x08A9D570u, 9, b);
}

/* (character): +7 on, a dash by strength (4.0 / 6.0 / 8.0 with brakes -0.25 / -0.375 / -0.5), +82
   cleared, the meter by 4, 0x08837098, move 5 + strength and +262 = 1. */
SFA_FUNC(089B9E3C, part_dash_5) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t s = S8(o + 194);
    *B(o + 7) = (uint8_t)(st + 1);
    if (s == 0) { *W(o + 96) = 0x40000; *W(o + 104) = 0xFFFFC000u; }
    else if (s == 1) { *W(o + 96) = 0x60000; *W(o + 104) = 0xFFFFA000u; }
    else if (s == 2) { *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF8000u; }
    *H(o + 82) = 0;
    call2(c, o, 4, f_088D34F4, 0x089B9EC4u);
    call1(c, o, f_08837098, 0x089B9ECCu);
    call2(c, o, (uint32_t)(S8(o + 194) + 5), f_088616EC, 0x089B9EDCu);
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): step 3, +235 = 1, 0x08863A50, the meter by strength; with a guard at strength 0 with +193
   and +290 bit 2 move 0x1E, else 0x12 (0x18 with a guard) + 3 with +193, + strength, of its list. */
SFA_FUNC(089A2EA8, rush_by_guard_e) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 3; *B(o + 235) = 1;
    call1(c, o, f_08863A50, 0x089A2ED0u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089A2EDCu);
    int32_t g = S8(o + 78), s = S8(o + 194), k = S8(o + 193);
    if (g != 0 && s == 0 && k != 0 && (*H(o + 290) & 4)) call2(c, o, 0x1E, f_08861674, 0x089A2F18u);
    else {
        uint32_t n = g != 0 ? 0x18 : 0x12;
        if (k != 0) n = (n + 3) & 0xFFFF;
        call2(c, o, (n + (uint32_t)s) & 0xFFFF, f_08861674, 0x089A2F54u);
    }
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, the meter by strength + 7, 0x08837098, 24 toward its facing, +123
   cleared, the four speeds by strength (0x08A515A0: x, x brake, y, fall), +82 = strength + 1 and move
   0x13 + 2 x strength. */
SFA_FUNC(0890375C, special_speeds_13_2k) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t s0 = S8(o + 194);
    *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, (uint32_t)(s0 + 7), f_0886CA2C, 0x08903788u);
    call1(c, o, f_08837098, 0x08903790u);
    int32_t d = *B(o + 11) == 0 ? -0x18 : 0x18;
    int32_t s = S8(o + 194);
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
    int32_t i = (int16_t)(s << 2);
    gaddr e = 0x08A515A0u + ((uint32_t)i << 2);
    *B(o + 123) = 0;
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    *H(o + 82) = (uint16_t)(s + 1);
    call2(c, o, (uint32_t)((s << 1) + 0x13) & 0xFFFF, f_088616EC, 0x0890380Cu);
    leave(c, sv);
}
