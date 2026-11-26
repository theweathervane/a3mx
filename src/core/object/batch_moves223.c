/* Special-move starts with their meter change and their launch speeds, and a super whose strength is
   picked by the distance to the opponent when locked (0x089C8CEC). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_08863114(Cpu *c);

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

/* (fighter): +7 on, +262 = 1, the meter by 6, 0x08837098, +123 / +551 / +20 cleared, a hop back (x -5.0
   / +0.1875, y 3.0 / -0.4375) and move 0x15 + 2 x strength. */
SFA_FUNC(0896DED0, special_hop_back_15) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x0896DF00u);
    call1(c, o, f_08837098, 0x0896DF08u);
    *B(o + 123) = 0; *B(o + 551) = 0; *H(o + 20) = 0;
    *W(o + 96) = 0xFFFB0000u;
    int32_t s = S8(o + 194);
    *W(o + 104) = 0x3000; *W(o + 100) = 0x30000; *W(o + 108) = 0xFFFF9000u;
    call2(c, o, (uint32_t)(s + s + 0x15) & 0xFFFF, f_088616EC, 0x0896DF4Cu);
    leave(c, sv);
}

/* (fighter): +7 on, facing the opponent, +235 = 1, 0x08863114, the meter by strength and move 6 + 2 x
   strength of its list (at strength 2 with +290 bit 1 step 4 and move 0x1E). */
SFA_FUNC(08983758, special_face_2k_6) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, 0x0898378Cu);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08983798u);
    int32_t s = S8(o + 194);
    uint32_t n = (uint32_t)((s << 1) + 6);
    if (s == 2 && (*H(o + 290) & 2)) { *B(o + 7) = 4; n = 0x1E; }
    call2(c, o, n, f_08861674, 0x089837D4u);
    leave(c, sv);
}

/* (fighter): a super: +7 on, +552 = 2, 144 meter spent, locked its strength by the distance (+508: below
   88 0, below 168 1, else 2), and move 0x31. */
SFA_FUNC(089C8CEC, super_by_distance_31) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint32_t st = *B(o + 7);
    *W(c->r[29] + 20) = s1; c->r[17] = 2;
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 552) = 2;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 0xFFFFFF70u, f_0886CA2C, 0x089C8D20u);
    if (S8(o + 373) != 0) {
        int32_t d = S16(o + 508);
        *B(o + 194) = d < 88 ? 0 : d < 168 ? 1 : 2;
    }
    call2(c, o, 0x31, f_088616EC, 0x089C8D64u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +262 / +73 = 1, step 1, the meter by 6, 0x08837098, x by strength from `tab` toward the
   facing, +20 cleared, y 4.0 / -0.375 and move `base` + strength. */
static void air_special(Cpu *c, gaddr tab, uint32_t base, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 73) = 1; *B(o + 7) = 1;
    call2(c, o, 6, f_0886CA2C, b[0]);
    call1(c, o, f_08837098, b[1]);
    int32_t s = S8(o + 194);
    *H(o + 20) = 0;
    uint32_t v = *W(tab + ((uint32_t)s << 2));
    if (*B(o + 11) == 0) v = 0u - v;
    *W(o + 96) = v; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA000u;
    call2(c, o, (uint32_t)(s + (int32_t)base) & 0xFFFF, f_088616EC, b[2]);
    leave(c, sv);
}
SFA_FUNC(088F3A10, air_special_19) {
    static const uint32_t b[3] = { 0x088F3A3Cu, 0x088F3A44u, 0x088F3A90u };
    air_special(c, 0x08A454D4u, 0x19, b);
}
SFA_FUNC(088F85E8, air_special_a) {
    static const uint32_t b[3] = { 0x088F8614u, 0x088F861Cu, 0x088F8668u };
    air_special(c, 0x08A49710u, 0xA, b);
}

/* (fighter): step 1, +196 cleared, +262 = 1, the meter by 9, 0x08837098, x / y by strength (0x08A496D8),
   brake -0.5, fall -0.375 and move 7 + strength. */
SFA_FUNC(088F8094, special_jump_7) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1; *B(o + 196) = 0; *B(o + 262) = 1;
    call2(c, o, 9, f_0886CA2C, 0x088F80C4u);
    call1(c, o, f_08837098, 0x088F80CCu);
    int32_t s = S8(o + 194);
    gaddr e = 0x08A496D8u + ((uint32_t)s << 3);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
    call2(c, o, (uint32_t)(s + 7) & 0xFFFF, f_088616EC, 0x088F8114u);
    leave(c, sv);
}

/* (fighter): +7 on, the four speeds by strength from `tab` (x, x brake, y, fall), +262 = 1, the meter by
   `gain`, 0x08837098 and move `base` + strength. */
static void speeds_special(Cpu *c, gaddr tab, uint32_t gain, int32_t base, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t s = S8(o + 194);
    *B(o + 7) = (uint8_t)(st + 1);
    gaddr e = tab + ((uint32_t)s << 4);
    *B(o + 262) = 1;
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    call2(c, o, gain, f_0886CA2C, b[0]);
    call1(c, o, f_08837098, b[1]);
    call2(c, o, (uint32_t)(S8(o + 194) + base), f_088616EC, b[2]);
    leave(c, sv);
}
SFA_FUNC(08906944, speeds_special_2) {
    static const uint32_t b[3] = { 0x089069ACu, 0x089069B4u, 0x089069C4u };
    speeds_special(c, 0x08A53940u, 9, 2, b);
}
SFA_FUNC(08906B60, speeds_special_c) {
    static const uint32_t b[3] = { 0x08906BC8u, 0x08906BD0u, 0x08906BE0u };
    speeds_special(c, 0x08A53988u, 7, 0xC, b);
}

/* (fighter): +7 on, +262 = 1, the meter by 5, 0x08837098, x 8.0 still otherwise, +82 = 48, +0x34C =
   -0.5 and move 0xC (0x32 with no choice made). */
SFA_FUNC(08910C54, special_dash_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x08910C84u);
    call1(c, o, f_08837098, 0x08910C8Cu);
    *W(o + 96) = 0x80000; *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 108) = 0;
    *H(o + 82) = 0x30;
    *W(o + 0x34C) = 0xFFFF8000u;
    uint32_t mv = (S8(o + 386) & 0x80) ? 0x32 : 0xC;
    call2(c, o, mv, f_088616EC, 0x08910CD4u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 2, 0x08837098, x by strength (0x08A5AEA8), guard its facing,
   turned around and move 0x21 + strength. */
SFA_FUNC(08911F7C, special_turn_21) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 2, f_0886CA2C, 0x08911FACu);
    call1(c, o, f_08837098, 0x08911FB4u);
    int32_t s = S8(o + 194);
    gaddr e = 0x08A5AEA8u + ((uint32_t)s << 3);
    uint32_t f = *B(o + 11);
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
    *B(o + 78) = (uint8_t)f; *B(o + 11) = (uint8_t)(f ^ 1);
    call2(c, o, (uint32_t)(s + 0x21) & 0xFFFF, f_088616EC, 0x08911FFCu);
    leave(c, sv);
}
