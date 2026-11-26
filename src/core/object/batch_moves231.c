/* Special-move starts that pick their move by distance, button and part level, and the meter gain
   for a strength picked by distance (0x0886D468). Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861674(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_088D3A64(Cpu *c);
void f_089429CC(Cpu *c); void f_08942A68(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter3(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
/* +7 on, facing the opponent, +235 = 1, 0x08863114 and the meter by strength. */
static void face_start(Cpu *c, gaddr o, uint32_t b1, uint32_t b2) {
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, b1);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, b2);
}
static int unlocked(gaddr o) { return S8(o + 373) == 0 && S8(o + 386) == 1; }

/* (fighter): face_start, then a part level for 0x088D3A64: unlocked with no choice made 2 x strength + 6
   (+1 without +290 bit 0; strength 2 sets step 3 / 4; strength 1 with +290 bit 1 0x1E with a rise, else
   step 6), otherwise by strength and the distance under 48 (6 / 7, 8 / 9, 0xA / 0xB; 0x1E at strength 1
   with +290 bit 1). */
SFA_FUNC(089C3474, part_level_by_distance) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    face_start(c, o, 0x089C34ACu, 0x089C34B8u);
    int32_t s = S8(o + 194);
    uint32_t n;
    if (unlocked(o)) {
        uint32_t h = *H(o + 290);
        n = (uint32_t)(s + s + 6);
        if ((h & 1) == 0) { n += 1; if (s == 2) *B(o + 7) = 3; }
        else if (s == 2) *B(o + 7) = 4;
        if (s == 1) {
            if (h & 2) { *B(o + 7) = 2; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFF8000u; n = 0x1E; }
            else { *B(o + 7) = 6; *W(o + 828) = 0; }
        }
    } else if (s < 0) n = 1;
    else if (s == 0) n = S16(o + 508) < 48 ? 6 : 7;
    else if (s == 1) {
        if (*H(o + 290) & 2) { *B(o + 7) = 2; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFF8000u; n = 0x1E; }
        else { *B(o + 7) = 6; n = S16(o + 508) < 48 ? 8 : 9; *W(o + 828) = 0; }
    } else if (s == 2) {
        if (S16(o + 508) < 48) { *B(o + 7) = 4; n = 0xA; } else { *B(o + 7) = 3; n = 0xB; }
    } else n = 0;
    call2(c, o, n, f_088D3A64, 0x089C3620u);
    leave3(c, sv);
}

/* (fighter): face_start, move 2 x strength (+6 with +193; +1 unlocked with no choice made without +290
   bit 0, else beyond the distance from 0x08A9D4E0 / 0x08A9D4E6) of its list. */
SFA_FUNC(089A2B88, special_by_distance_c) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    face_start(c, o, 0x089A2BC0u, 0x089A2BCCu);
    int32_t s = S8(o + 194);
    int32_t lim; uint32_t n;
    if (S8(o + 193) != 0) { lim = S16(0x08A9D4E6u + (uint32_t)(s + s)); n = (uint32_t)((s << 1) + 6) & 0xFFFF; }
    else { lim = S16(0x08A9D4E0u + (uint32_t)(s + s)); n = (uint32_t)(s << 1) & 0xFFFF; }
    if (unlocked(o)) { if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF; }
    else if (lim < S16(o + 508)) n = (n + 1) & 0xFFFF;
    call2(c, o, n, f_08861674, 0x089A2C68u);
    leave3(c, sv);
}

/* (fighter): the meter gain from 0x08A1BDF8: with no choice made by the kept +552, else by strength,
   which is kept when the distance (+366) is past its entry and otherwise picked by distance (0 under 96,
   1 under 144, else 2) into +194 / +552; with no choice made and +804 scaled by its tenths from
   0x08A1BDFE. */
SFA_FUNC(0886D468, meter_by_distance) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    const gaddr tab = 0x08A1BDF8u;
    int32_t amt;
    if (S8(o + 373) == 0) amt = S16(tab + (uint32_t)(S8(o + 552) * 2));
    else {
        int32_t s = S8(o + 194);
        int32_t v = S16(tab + (uint32_t)(s + s));
        int32_t d = S16(o + 366);
        if ((int16_t)(-v) < d) { amt = (int16_t)v; *B(o + 552) = (uint8_t)s; }
        else {
            int32_t k = d >= 144 ? 2 : d >= 96 ? 1 : 0;
            *B(o + 194) = (uint8_t)k; *B(o + 552) = (uint8_t)k;
            amt = S16(tab + (uint32_t)(k + k));
        }
    }
    if (S8(o + 386) == -1) {
        int32_t i = S16(o + 804);
        if (i != 0) {
            DIV(c, (uint32_t)(amt * (int32_t)*B(0x08A1BDFEu + (uint32_t)i)), 10);
            amt = (int16_t)c->lo;
        }
    }
    call2(c, o, (uint32_t)amt, f_0886CA2C, 0x0886D554u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): face_start, move strength x 2 + 1 (none with +290 bit 0 when unlocked with no choice made,
   else within 65 of the distance), +6 with +193, of its list. */
SFA_FUNC(089B0554, special_by_distance_65) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    face_start(c, o, 0x089B058Cu, 0x089B0598u);
    uint32_t s2 = (uint32_t)(S8(o + 194) * 2);
    int32_t k = S8(o + 193);
    if (unlocked(o)) {
        uint32_t b = (*H(o + 290) & 1) ? 0 : 1;
        uint32_t n = k != 0 ? (b + 6) & 0xFFFF : b;
        call2(c, o, (n + s2) & 0xFFFF, f_08861674, 0x089B05F0u);
    } else {
        uint32_t b = (int32_t)(*H(o + 508)) < 65 ? 0 : 1;
        uint32_t n = k != 0 ? (b + 6) & 0xFFFF : b;
        call2(c, o, (n + s2) & 0xFFFF, f_08861674, 0x089B0640u);
    }
    leave3(c, sv);
}

/* (fighter): step 1, facing the opponent, +235 = 1, the meter by strength, 0x08863114; with a choice
   made 0x089429CC; at strength 2 with +290 bit 1 step 2, a launch (x 18.0 toward its facing with brake
   0.0625, y 4.5, fall -0.375) and move 0x20, else 0x08942A68. */
SFA_FUNC(089428C8, special_launch_20) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    int32_t face = S8(o + 697);
    *B(o + 7) = 1;
    int32_t s = S8(o + 194);
    c->r[17] = (uint32_t)s;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call2(c, o, (uint32_t)s, f_0886CA2C, 0x08942904u);
    call1(c, o, f_08863114, 0x0894290Cu);
    if (S8(o + 386) != -1) call1(c, o, f_089429CC, 0x08942988u);
    else if (s != 2 || (*H(o + 290) & 2) == 0) call1(c, o, f_08942A68, 0x0894296Cu);
    else {
        *B(o + 7) = 2;
        *W(o + 100) = 0x48000; *W(o + 108) = 0xFFFFA000u;
        if (*B(o + 11) == 0) { *W(o + 96) = 0xFFEE0000u; *W(o + 104) = 0x1000; }
        else { *W(o + 96) = 0x120000; *W(o + 104) = 0xFFFFF000u; }
        call2(c, o, 0x20, f_08861674, 0x089429B8u);
    }
    leave3(c, sv);
}
