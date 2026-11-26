/* Super starts that spend 144 meter at the per-character rate (0x0886D560) and more special-move starts
   with their meter change. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_088370D8(Cpu *c); void f_0886D560(Cpu *c); void f_0887CC68(Cpu *c); void f_08939BF4(Cpu *c);
void f_08863114(Cpu *c); void f_08821368(Cpu *c); void f_0898C640(Cpu *c);

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
static uint32_t by_str(gaddr o, uint32_t base) { return (uint32_t)(S8(o + 194) + (int32_t)base) & 0xFFFF; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* The super's tail: 0x0887CC68, 144 meter spent at the rate, 0x088370D8 and move `mv`. */
static void super_tail(Cpu *c, gaddr o, uint32_t mv, const uint32_t b[4]) {
    call1(c, o, f_0887CC68, b[0]);
    call2(c, o, 0xFFFFFF70u, f_0886D560, b[1]);
    call1(c, o, f_088370D8, b[2]);
    call2(c, o, mv, f_088616EC, b[3]);
}

/* (fighter): a super: +7 on, guard (+78) its facing, +235 cleared, then move 0x29. */
SFA_FUNC(08912170, super_start_29) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7), f = *B(o + 11);
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 78) = (uint8_t)f; *B(o + 235) = 0;
    static const uint32_t b[4] = { 0x089121A0u, 0x089121ACu, 0x089121B4u, 0x089121C0u };
    super_tail(c, o, 0x29, b);
    leave(c, sv);
}

/* (fighter): a super: +7 on, strength 2 (+194 / +552), +82 = 50, +235 cleared, then move 0x32. */
SFA_FUNC(08964F78, super_start_32) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    *B(o + 194) = 2; *B(o + 552) = 2; *H(o + 82) = 0x32; *B(o + 235) = 0;
    static const uint32_t b[4] = { 0x08964FB4u, 0x08964FC0u, 0x08964FC8u, 0x08964FD4u };
    super_tail(c, o, 0x32, b);
    leave(c, sv);
}

/* (fighter): a super: +7 on, strength 2 (+194 / +552), +82 = 50, guard its facing, +235 cleared, then
   move 0x29. */
SFA_FUNC(088F9EC4, super_start_29_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    *B(o + 194) = 2; *B(o + 552) = 2;
    uint32_t f = *B(o + 11);
    *H(o + 82) = 0x32; *B(o + 78) = (uint8_t)f; *B(o + 235) = 0;
    static const uint32_t b[4] = { 0x088F9F08u, 0x088F9F14u, 0x088F9F1Cu, 0x088F9F28u };
    super_tail(c, o, 0x29, b);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, +0x3C0 and y speeds cleared, the meter by 3, 0x08837098, 0x08939BF4 and
   move 0xC + strength. */
SFA_FUNC(08935A94, special_start_c_3c0) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    *B(o + 0x3C0) = 0; *W(o + 108) = 0; *W(o + 100) = 0;
    call2(c, o, 3, f_0886CA2C, 0x08935AD4u);
    call1(c, o, f_08837098, 0x08935ADCu);
    call1(c, o, f_08939BF4, 0x08935AE4u);
    call2(c, o, by_str(o, 0xC), f_088616EC, 0x08935AF8u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by strength + 7, 0x08837098, x 6.0 toward the facing and move
   0xB + strength. */
SFA_FUNC(089A6780, special_dash_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 7), f_0886CA2C, 0x089A67B4u);
    call1(c, o, f_08837098, 0x089A67BCu);
    uint32_t v = *B(o + 11) != 0 ? 0x60000u : 0xFFFA0000u;
    int32_t s2 = S8(o + 194);
    *W(o + 96) = v;
    call2(c, o, (uint32_t)(s2 + 0xB) & 0xFFFF, f_088616EC, 0x089A67E4u);
    leave(c, sv);
}

/* (fighter): +7 on, +235 = 1, facing the opponent, 0x08863114, the meter by strength and move 0xC +
   strength (+3 with +193) of its list. */
static void special_face_list(Cpu *c, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t face = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)face;
    call1(c, o, f_08863114, b[0]);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, b[1]);
    uint32_t n = by_str(o, 0xC);
    if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
    call2(c, o, n, f_08861674, b[2]);
    leave(c, sv);
}
SFA_FUNC(088EE894, special_face_list_a) {
    static const uint32_t b[3] = { 0x088EE8C8u, 0x088EE8D4u, 0x088EE8FCu };
    special_face_list(c, b);
}
SFA_FUNC(08967024, special_face_list_b) {
    static const uint32_t b[3] = { 0x08967058u, 0x08967064u, 0x0896708Cu };
    special_face_list(c, b);
}

/* (fighter): +7 on, +262 = 1, +840 cleared, the meter by 3, 0x08837098, +82 a random 0 - 15, 0x0898C640
   and move 0x12 + strength. */
SFA_FUNC(0898AB7C, special_start_random_12) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1; *W(o + 840) = 0;
    call2(c, o, 3, f_0886CA2C, 0x0898ABB0u);
    call1(c, o, f_08837098, 0x0898ABB8u);
    c->r[31] = 0x0898ABC0u; f_08821368(c);
    *H(o + 82) = (uint16_t)(c->r[2] & 0xF);
    call1(c, o, f_0898C640, 0x0898ABD0u);
    call2(c, o, by_str(o, 0x12), f_088616EC, 0x0898ABE4u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 3 (1 with +836), 0x08837098 and move 0x1E + strength. */
SFA_FUNC(0898B194, special_start_1e) {
    gaddr o = ARG(0); uint32_t sv[2];
    step_on(o);
    uint32_t k = *W(o + 836);
    *B(o + 262) = 1;
    enter(c, o, sv);
    if (k == 0) call2(c, o, 3, f_0886CA2C, 0x0898B1CCu);
    else call2(c, o, 1, f_0886CA2C, 0x0898B1E0u);
    call1(c, o, f_08837098, 0x0898B1E8u);
    call2(c, o, by_str(o, 0x1E), f_088616EC, 0x0898B1FCu);
    leave(c, sv);
}

/* (fighter): +7 on, +235 = 1, facing the opponent, 0x08863114, the meter by strength, step 2 at strength
   2, and move 0xF + strength of its list. */
SFA_FUNC(089A2CFC, special_face_list_f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t face = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)face;
    call1(c, o, f_08863114, 0x089A2D30u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089A2D3Cu);
    int32_t s = S8(o + 194);
    if (s == 2) *B(o + 7) = 2;
    call2(c, o, (uint32_t)(s + 0xF) & 0xFFFF, f_08861674, 0x089A2D64u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by strength + 3, 0x08837098 and move 0x10 (stance 1) or 0x11 +
   strength. */
static void special_stance(Cpu *c, const uint32_t b[3]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    step_on(o);
    int32_t s = S8(o + 194);
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *B(o + 262) = 1;
    *W(c->r[29] + 24) = ra;
    call2(c, o, (uint32_t)(s + 3), f_0886CA2C, b[0]);
    call1(c, o, f_08837098, b[1]);
    uint32_t base = S8(o + 236) == 1 ? 0x10 : 0x11;
    call2(c, o, (base + (uint32_t)S8(o + 194)) & 0xFFFF, f_088616EC, b[2]);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088EF84C, special_stance_a) {
    static const uint32_t b[3] = { 0x088EF884u, 0x088EF88Cu, 0x088EF8B4u };
    special_stance(c, b);
}
SFA_FUNC(089639D8, special_stance_b) {
    static const uint32_t b[3] = { 0x08963A10u, 0x08963A18u, 0x08963A40u };
    special_stance(c, b);
}
