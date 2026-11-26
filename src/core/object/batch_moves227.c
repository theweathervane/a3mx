/* Special-move starts with their meter change and launch speeds, a rush that may turn into a locked
   air version (0x08993E28) and the strength bump by distance or held button (0x089A5AEC). Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_08863A50(Cpu *c); void f_08994C34(Cpu *c); void f_0894A35C(Cpu *c); void f_08863114(Cpu *c);
void f_0887C408(Cpu *c);

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

/* (fighter): locked in the air with +491 bit 3 and a guard, high enough above the floor (64, 88 when
   falling) it switches to state 1 / 0 / 7 / 0 with stance 9 and +235 = 1 (0x08994C34); otherwise step
   3, +192 = 2, +235 = 1, 0x08863A50, the meter by strength and move 0x12 (0x18 with a guard) + 3 with
   +193, + strength, of its list. */
SFA_FUNC(08993E28, rush_or_air) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int air = 0;
    if (S8(o + 373) != 0 && (S8(o + 491) & 8) && S8(o + 73) != 0 && S8(o + 78) > 0) {
        int32_t th = (int32_t)*W(o + 100) >= 0 ? 0x40 : 0x58;
        air = S16(o + 132) + th < S16(o + 22);
    }
    uint32_t n;
    if (air) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 9; *B(o + 235) = 1;
        call1(c, o, f_08994C34, 0x08993EFCu);
        leave(c, sv);
        return;
    } else {
        *B(o + 7) = 3; *B(o + 192) = 2; *B(o + 235) = 1;
        call1(c, o, f_08863A50, 0x08993EB0u);
        call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08993EBCu);
        n = S8(o + 78) == 0 ? 0x12 : 0x18;
    }
    if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
    call2(c, o, (n + (uint32_t)S8(o + 194)) & 0xFFFF, f_08861674, 0x08993F38u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by strength + 7, 0x08837098, +123 / +199 cleared, a hop (x 4.0 /
   -0.5, y by strength from 0x08A790CC, fall -0.25), 16 toward its facing and 0x0894A35C. */
SFA_FUNC(08949F1C, special_hop_mash) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 7), f_0886CA2C, 0x08949F50u);
    call1(c, o, f_08837098, 0x08949F58u);
    *B(o + 123) = 0; *B(o + 199) = 0;
    *W(o + 96) = 0x40000;
    int32_t s2 = S8(o + 194);
    *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFC000u;
    uint32_t f = *B(o + 11);
    *W(o + 100) = *W(0x08A790CCu + ((uint32_t)s2 << 2));
    int32_t x = S16(o + 18);
    *H(o + 18) = (uint16_t)(f != 0 ? x + 0x10 : x - 0x10);
    call1(c, o, f_0894A35C, 0x08949FBCu);
    leave(c, sv);
}

/* (fighter): +7 on, +262 / +73 = 1, the meter by 6, 0x08837098, +551 / +123 / +20 and speeds cleared, x
   by strength (0x08A8DE80), +82 from 0x08A8DE8C and move 6 + 2 x strength. */
SFA_FUNC(0896D5FC, special_glide_6) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1; *B(o + 73) = 1;
    call2(c, o, 6, f_0886CA2C, 0x0896D630u);
    call1(c, o, f_08837098, 0x0896D638u);
    *B(o + 551) = 0; *B(o + 123) = 0;
    int32_t s = S8(o + 194);
    *H(o + 20) = 0; *W(o + 108) = 0; *W(o + 104) = 0; *W(o + 100) = 0;
    *W(o + 96) = *W(0x08A8DE80u + ((uint32_t)s << 2));
    *B(o + 0x52) = (uint8_t)S8(0x08A8DE8Cu + (uint32_t)s);
    call2(c, o, (uint32_t)((S8(o + 194) << 1) + 6) & 0xFFFF, f_088616EC, 0x0896D69Cu);
    leave(c, sv);
}

/* (fighter): +7 on, +262 / +73 = 1, the meter by 6, 0x08837098, +551 / +123 / +20 cleared, the four
   speeds by strength (0x08A8DF3C, the next three entries with no choice made) and move 0x1B + 2 x
   strength. */
SFA_FUNC(0896E308, special_glide_1b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1; *B(o + 73) = 1;
    call2(c, o, 6, f_0886CA2C, 0x0896E33Cu);
    call1(c, o, f_08837098, 0x0896E344u);
    *B(o + 551) = 0;
    int32_t s = S8(o + 194);
    *B(o + 123) = 0; *H(o + 20) = 0;
    uint32_t i = (uint32_t)s << 2;
    if (S8(o + 386) < 0) i += 0xC;
    gaddr e = 0x08A8DF3Cu + (i << 2);
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    call2(c, o, (uint32_t)(s + s + 0x1B) & 0xFFFF, f_088616EC, 0x0896E3A8u);
    leave(c, sv);
}

/* (fighter, base, distance): +235 = 1, step 1, facing the opponent, 0x08863114, the meter by strength;
   gives `base`, one more when locked or with no choice made and the opponent beyond `distance`
   (+508), or with a choice made without +290 bit 0. */
SFA_FUNC(089A5AEC, strength_bump) {
    gaddr o = ARG(0);
    uint32_t base = ARG(1), lim = ARG(2);
    uint32_t ra = c->r[31], s[3];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s[0] = c->r[16];
    *W(c->r[29] + 24) = s[2] = c->r[18];
    c->r[16] = o;
    uint32_t r = base & 0xFFFF;
    c->r[18] = r;
    int32_t face = S8(o + 697);
    *B(o + 235) = 1;
    *W(c->r[29] + 20) = s[1] = c->r[17];
    int32_t d = (int16_t)lim;
    c->r[17] = (uint32_t)d;
    *B(o + 7) = 1; *B(o + 11) = (uint8_t)face;
    *W(c->r[29] + 28) = ra;
    call1(c, o, f_08863114, 0x089A5B2Cu);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089A5B38u);
    if (S8(o + 373) == 0 && S8(o + 386) > 0) { if ((*H(o + 290) & 1) == 0) r = (r + 1) & 0xFFFF; }
    else if (d < S16(o + 508)) r = (r + 1) & 0xFFFF;
    c->r[2] = r;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 on, +262 / +235 = 1 (+713 = 1 with +251), the meter by strength + 5, 0x08837098, fall
   -0.5, no brake, x (toward the facing) / y by strength from `tab` and move `base` + strength. */
static void special_jump_tab(Cpu *c, gaddr tab, uint32_t base, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    int32_t k = S8(o + 251);
    *B(o + 235) = 1;
    int32_t s = S8(o + 194);
    if (k != 0) *B(o + 713) = 1;
    call2(c, o, (uint32_t)(s + 5), f_0886CA2C, b[0]);
    call1(c, o, f_08837098, b[1]);
    int32_t s2 = S8(o + 194);
    *W(o + 108) = 0xFFFF8000u; *W(o + 104) = 0;
    gaddr e = tab + ((uint32_t)s2 << 3);
    uint32_t f = *B(o + 11);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    if (f == 0) *W(o + 96) = 0u - *W(o + 96);
    call2(c, o, (uint32_t)(s2 + (int32_t)base) & 0xFFFF, f_088616EC, b[2]);
    leave(c, sv);
}
SFA_FUNC(08973DC0, special_jump_tab_d) {
    static const uint32_t b[3] = { 0x08973E04u, 0x08973E0Cu, 0x08973E64u };
    special_jump_tab(c, 0x08A90058u, 0xD, b);
}
SFA_FUNC(08976648, special_jump_tab_30) {
    static const uint32_t b[3] = { 0x0897668Cu, 0x08976694u, 0x089766ECu };
    special_jump_tab(c, 0x08A901ACu, 0x30, b);
}

/* (fighter): +7 on, +262 = 1, +696 cleared (+310 = 1 at strength 0), the meter by strength + 4,
   0x08837098, x by strength (0x08A92604 facing right, 0x08A92610 left), move 0x12 + strength and voice
   0x30. */
SFA_FUNC(0897CDEC, special_dash_voice_12) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    int32_t s = S8(o + 194);
    *B(o + 696) = 0;
    if (s == 0) *B(o + 310) = 1;
    call2(c, o, (uint32_t)(s + 4), f_0886CA2C, 0x0897CE2Cu);
    call1(c, o, f_08837098, 0x0897CE34u);
    int32_t s2 = S8(o + 194);
    gaddr tab = *B(o + 11) != 0 ? 0x08A92604u : 0x08A92610u;
    *W(o + 96) = *W(tab + ((uint32_t)s2 << 2));
    call2(c, o, (uint32_t)(s2 + 0x12) & 0xFFFF, f_088616EC, 0x0897CE80u);
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x30; c->r[31] = 0x0897CE90u; f_0887C408(c);
    leave(c, sv);
}
