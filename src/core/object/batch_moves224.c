/* Special-move starts with their meter change and launch speeds. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_08861848(Cpu *c); void f_0886469C(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_08821368(Cpu *c); void f_0898C640(Cpu *c);
void f_0892B188(Cpu *c); void f_08863A50(Cpu *c); void f_0887C408(Cpu *c);

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
/* s0 / s1 / ra frame with s1 = `v`. */
static void enter3(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (fighter): +7 on, +262 = 1, +82 a random 0 - 15, 0x0898C640, the meter by 3 (1 with +836), 0x08837098,
   +828 cleared and move 0x2A + strength. */
SFA_FUNC(0898C008, special_random_2a) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    c->r[31] = 0x0898C030u; f_08821368(c);
    *H(o + 82) = (uint16_t)(c->r[2] & 0xF);
    call1(c, o, f_0898C640, 0x0898C040u);
    if (*W(o + 836) == 0) call2(c, o, 3, f_0886CA2C, 0x0898C058u);
    else call2(c, o, 1, f_0886CA2C, 0x0898C068u);
    call1(c, o, f_08837098, 0x0898C070u);
    int32_t s = S8(o + 194);
    *W(o + 828) = 0;
    call2(c, o, (uint32_t)(s + 0x2A) & 0xFFFF, f_088616EC, 0x0898C088u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by strength + 7, 0x08837098, the four speeds by strength
   (0x08A9BF80: x, y, x brake, fall) and move `strength`. */
SFA_FUNC(0899F988, special_speeds_s) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 7), f_0886CA2C, 0x0899F9BCu);
    call1(c, o, f_08837098, 0x0899F9C4u);
    int32_t s2 = S8(o + 194);
    gaddr e = 0x08A9BF80u + ((uint32_t)s2 << 4);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4); *W(o + 104) = *W(e + 8); *W(o + 108) = *W(e + 12);
    call2(c, o, (uint32_t)s2 & 0xFFFF, f_088616EC, 0x0899FA08u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by strength + 7, 0x08837098, brake -0.5, fall -0.375, x / y by
   strength from `tab` and move 4 + strength. */
static void special_jump4(Cpu *c, gaddr tab, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 7), f_0886CA2C, b[0]);
    call1(c, o, f_08837098, b[1]);
    int32_t s2 = S8(o + 194);
    *W(o + 104) = 0xFFFF8000u; *W(o + 108) = 0xFFFFA000u;
    gaddr e = tab + ((uint32_t)s2 << 3);
    *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    call2(c, o, (uint32_t)(s2 + 4) & 0xFFFF, f_088616EC, b[2]);
    leave(c, sv);
}
SFA_FUNC(088EF520, special_jump4_a) {
    static const uint32_t b[3] = { 0x088EF554u, 0x088EF55Cu, 0x088EF5A4u };
    special_jump4(c, 0x08A43AB0u, b);
}
SFA_FUNC(0896368C, special_jump4_b) {
    static const uint32_t b[3] = { 0x089636C0u, 0x089636C8u, 0x08963710u };
    special_jump4(c, 0x08A8A8D0u, b);
}

/* (fighter): step 0: step 1, +262 = 1, facing the opponent, the meter by 1, 0x08837098 and move 0x36;
   step 1: standing once ended, else ticked. */
SFA_FUNC(0890C5A0, special_36) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st == 0) {
        int32_t face = S8(o + 697);
        *B(o + 7) = 1; *B(o + 262) = 1; *B(o + 11) = (uint8_t)face;
        call2(c, o, 1, f_0886CA2C, 0x0890C5DCu);
        call1(c, o, f_08837098, 0x0890C5E4u);
        call2(c, o, 0x36, f_088616EC, 0x0890C5F0u);
    } else if (st == 1) {
        if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x0890C624u);
        else call1(c, o, f_08861848, 0x0890C614u);
    }
    leave(c, sv);
}

/* (fighter): step 3, 0x0892B188, +235 = 1, 0x08863A50, the meter by strength and move 0x12 (0x18 with a
   guard) + 3 with +193, + strength of its list. */
SFA_FUNC(0892A300, rush_by_guard_c) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    *B(o + 7) = 3;
    call1(c, o, f_0892B188, 0x0892A324u);
    *B(o + 235) = 1;
    call1(c, o, f_08863A50, 0x0892A334u);
    uint32_t s = (uint32_t)S8(o + 194) & 0xFFFF;
    c->r[17] = s;
    call2(c, o, s, f_0886CA2C, 0x0892A350u);
    uint32_t n = S8(o + 78) != 0 ? 0x18 : 0x12;
    if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
    call2(c, o, (n + s) & 0xFFFF, f_08861674, 0x0892A384u);
    leave3(c, sv);
}

/* (fighter): +7 on, +696 / +10 / +123 cleared, +262 = 1, the meter by 6, 0x08837098, voice 0x212 and in
   the air move 0x31 + strength (+10 = 1), on the ground 0xD + strength. */
SFA_FUNC(089951CC, special_voice_air_or_ground) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    step_on(o);
    *B(o + 696) = 0; *B(o + 10) = 0; *B(o + 123) = 0; *B(o + 262) = 1;
    c->r[17] = 0xD;
    call2(c, o, 6, f_0886CA2C, 0x08995210u);
    call1(c, o, f_08837098, 0x08995218u);
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x212; c->r[31] = 0x08995228u; f_0887C408(c);
    if (S8(o + 73) != 0) { c->r[17] = 0x31; *B(o + 10) = 1; }
    call2(c, o, (c->r[17] + (uint32_t)S8(o + 194)) & 0xFFFF, f_088616EC, 0x08995250u);
    leave3(c, sv);
}

/* (fighter): +7 on, +196 - +199 / +696 / +235 cleared, +262 = 1 (+502 = 1 with +80), +820 from
   0x08A987D8 by the opponent's character, the meter by 2, 0x08837098 and move 0x17. */
SFA_FUNC(08995600, special_start_17_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    gaddr opp = *W(o + 92);
    step_on(o);
    *B(o + 199) = 0; *B(o + 198) = 0; *B(o + 197) = 0; *B(o + 196) = 0;
    *B(o + 696) = 0; *B(o + 235) = 0;
    int32_t k = S8(o + 80);
    *B(o + 262) = 1;
    if (k != 0) *B(o + 502) = 1;
    *W(o + 820) = (uint32_t)S16(0x08A987D8u + (uint32_t)(S8(opp + 338) * 2));
    call2(c, o, 2, f_0886CA2C, 0x08995674u);
    call1(c, o, f_08837098, 0x0899567Cu);
    call2(c, o, 0x17, f_088616EC, 0x08995688u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 6, 0x08837098, a hop (x from 0x08A4BB8C by +82 / 2 toward
   the facing, y 3.0 / -0.25) and move 0x2B. */
SFA_FUNC(088FF3C0, special_hop_2b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x088FF3F0u);
    call1(c, o, f_08837098, 0x088FF3F8u);
    int32_t t = S16(o + 82);
    *W(o + 104) = 0; *W(o + 108) = 0xFFFFC000u; *W(o + 100) = 0x30000;
    uint32_t v = *W(0x08A4BB8Cu + ((uint32_t)(t >> 1) << 2));
    uint32_t f = *B(o + 11);
    *W(o + 96) = v;
    if (f == 0) *W(o + 96) = 0u - *W(o + 96);
    call2(c, o, 0x2B, f_088616EC, 0x088FF44Cu);
    leave(c, sv);
}
