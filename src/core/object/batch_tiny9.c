/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, about 20
   instructions, ready once batch_tiny8.c was done: two-button checks, a fighter's state reset before
   a move, speeds and nudges from per-move tables mirrored by facing, palettes by colour, set-ups from
   an owner. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887B1A8(Cpu *c); void f_0887BD00(Cpu *c); void f_0887FB14(Cpu *c); void f_088215C4(Cpu *c);
void f_0887DF88(Cpu *c); void f_0880AEA8(Cpu *c); void f_088370D8(Cpu *c); void f_08837098(Cpu *c);
void f_088230F4(Cpu *c); void f_089134A0(Cpu *c); void f_089134DC(Cpu *c); void f_0882196C(Cpu *c);
void f_0896F62C(Cpu *c); void f_08821330(Cpu *c);

static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WRAP20(back, fn) wrap(c, 0x20, 16, back, fn)
static uint8_t inc8(gaddr a) { uint8_t v = (uint8_t)(*B(a) + 1); *B(a) = v; return v; }
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- buttons (+760: just pressed; bits 4 - 6
   punches, 7 - 9 kicks, light to heavy) */
/* (fighter): light kick with heavy punch, or light punch with heavy kick. */
SFA_FUNC(0886A0A4, pressed_lk_hp_or_lp_hk) {
    uint32_t k = *H(ARG(0) + 760);
    RETURN(((k & 0x80) && (k & 0x40)) || ((k & 0x10) && (k & 0x200)));
}
/* (fighter): medium kick with heavy punch, or medium punch with heavy kick. */
SFA_FUNC(0886A0F4, pressed_mk_hp_or_mp_hk) {
    uint32_t k = *H(ARG(0) + 760);
    RETURN(((k & 0x100) && (k & 0x40)) || ((k & 0x20) && (k & 0x200)));
}

/* ---------------------------------------------------------------- fighters */
/* (fighter): the CPU's first script and command cleared (0x0887B1A8) when +6 is 0, or when
   0x0887BD00 says so. */
SFA_FUNC(0887B064, cpu_clear_481_when) {
    gaddr f = ARG(0); WITH_S0(f);
    if (*B(f + 6) == 0) call_on(c, f, 0x0887B084u, f_0887B1A8);
    else {
        call_on(c, f, 0x0887B094u, f_0887BD00);
        if (c->r[2] != 0) call_on(c, f, 0x0887B0A4u, f_0887B1A8);
    }
    END_S0();
}
/* (fighter): step 1 of a move with +6 = n: +5 / +7 cleared, +543, +234, +123, +537, +696, +671
   cleared, +235 = 1; then the move's own set-up. */
static void move_start(Cpu *c, uint8_t n, uint32_t back, void (*setup)(Cpu *)) {
    gaddr f = ARG(0);
    *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = n; *B(f + 7) = 0;
    *B(f + 543) = 0; *B(f + 234) = 0; *B(f + 123) = 0; *B(f + 537) = 0;
    *B(f + 696) = 0; *B(f + 235) = 1; *B(f + 671) = 0;
    ARG(1) = 1;
    WRAP20(back, setup);
}
SFA_FUNC(088D3278, move_start_8) { move_start(c, 8, 0x088D32BCu, f_088370D8); }
SFA_FUNC(088D3488, move_start_7) { move_start(c, 7, 0x088D34CCu, f_08837098); }
/* (fighter): +0x380 / +0x384 cleared, and +0x358 / +0x35C unless its character (+338) is 4. */
SFA_FUNC(08902314, clear_380_358) {
    gaddr f = ARG(0);
    *W(f + 0x380) = 0; *W(f + 0x384) = 0;
    if (S8(f + 338) != 4) { *W(f + 0x358) = 0; *W(f + 0x35C) = 0; }
}
/* (a, b, c, d) as s8s: 16 x d colours from 0x08A2C7A8 + 32 b into slot c (0x0880AEA8). */
SFA_FUNC(088AC488, palette_08A2C7A8) {
    int32_t a = (int8_t)ARG(0), b = (int8_t)ARG(1), s = (int8_t)ARG(2), n = (int8_t)ARG(3);
    c->r[9] = (uint32_t)a;
    ARG(0) = 1; ARG(1) = 0x08A2C7A8u + (uint32_t)b * 32; ARG(2) = (uint32_t)a; ARG(3) = (uint32_t)s;
    c->r[8] = (uint32_t)n;
    wrap(c, 0x30, 32, 0x088AC4CCu, f_0880AEA8);
}
/* (object): x speed and change from the 8-byte row by its +194 of 0x08A95808 / 0x08A9C034, turned
   round when facing (+11) is clear. */
static void speeds_by_194(gaddr o, gaddr table) {
    gaddr e = table + (uint32_t)(int32_t)S8(o + 194) * 8;
    uint32_t v = *W(e);
    if (*B(o + 11) == 0) { *W(o + 96) = 0u - v; *W(o + 104) = 0u - *W(e + 4); }
    else { *W(o + 96) = v; *W(o + 104) = *W(e + 4); }
}
SFA_FUNC(0898C640, speeds_by_194_a) { speeds_by_194(ARG(0), 0x08A95808u); }
SFA_FUNC(089A0454, speeds_by_194_b) { speeds_by_194(ARG(0), 0x08A9C034u); }
/* (object): 1 when its target (+600) is a fighter of character 0x1F / 0x1E with health left, not
   +251, and its record (+296) +11 set. */
static int target_is(gaddr o, int ch) {
    gaddr t = *W(o + 600);
    if (!t || S8(t + 338) != ch || S16(t + 112) < 0 || S8(t + 251) != 0) return 0;
    return *B(*W(t + 296) + 11) != 0;
}
SFA_FUNC(0899311C, target_is_1F) { RETURN(target_is(ARG(0), 0x1F)); }
SFA_FUNC(08999428, target_is_1E) { RETURN(target_is(ARG(0), 0x1E)); }

/* ---------------------------------------------------------------- objects */
/* (object): whole-frame step by +5 (0x08A245B4), then 0x0887FB14 (at its owner's place) and draw. */
SFA_FUNC(0887F3C8, step_0887F3C8) { run_step_then(c, ARG(0), *B(ARG(0) + 5), 0x08A245B4u, 0x0887F3F8u, f_0887FB14, f_088215C4); }
/* (object): +9 = +11 = 1, then 0x0887DF88 (object, 2 x +3 + 0x18); from +78 >= 32, +9 = 2, +11
   cleared and one more. */
SFA_FUNC(0888038C, set_9_11_call_0887DF88) {
    gaddr o = ARG(0);
    *B(o + 9) = 1; *B(o + 11) = 1;
    uint32_t n = (uint32_t)*B(o + 3) * 2 + 0x18;
    ARG(2) = S8(o + 78) < 32;
    if (!ARG(2)) { ARG(2) = 2; *B(o + 9) = 2; *B(o + 11) = 0; n++; }
    ARG(1) = n;
    WRAP20(0x088803D0u, f_0887DF88);
}
/* (object): if it is its owner's (+88) +228, the owner's count +536 down (+228 cleared at 0); then
   0x088230F4. */
SFA_FUNC(088D3C88, owner_228_count_down) {
    gaddr o = ARG(0), w = *W(o + 88);
    uint32_t v = *W(w + 228);
    ARG(1) = v; ARG(2) = w;
    if (v != 0 && v == o) {
        uint8_t n = (uint8_t)(*B(w + 536) - 1);
        *B(w + 536) = n;
        ARG(1) = n;
        if (n == 0) *W(w + 228) = 0;
    }
    WRAP20(0x088D3CCCu, f_088230F4);
}
/* (object): x moved the way it faces, then y, each speed changed. */
SFA_FUNC(088F3720, move_x_facing_then_y) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) != 0 ? v : 0u - v;
    *W(o + 96) = v + *W(o + 104);
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    *W(o + 100) = vy + *W(o + 108);
}
/* (object): nudged by the s16 pair by its +3 in 0x08A58518 (x mirrored when facing, +11, is set). */
SFA_FUNC(0890DAF4, nudge_by_3) {
    gaddr o = ARG(0), e = 0x08A58518u + (uint32_t)*B(o + 3) * 4;
    int16_t dx = S16(e), dy = S16(e + 2);
    if (*B(o + 11) != 0) dx = (int16_t)-dx;
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(o + 22) + dy);
}
/* (object): x nudged by the s16 at 0x08A762C4 by +77 x 3 + +194 (mirrored when facing is clear); +77
   cleared. */
SFA_FUNC(08944300, nudge_by_77_194) {
    gaddr o = ARG(0);
    int32_t i = S8(o + 77) * 3 + S8(o + 194);
    int16_t d = S16(0x08A762C4u + (uint32_t)i * 2);
    if (*B(o + 11) == 0) d = (int16_t)-d;
    *B(o + 77) = 0;
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
}
/* (object): moved on both axes (0x089134A0); below y 4, held at 4 and +4 on; then 0x089134DC. */
SFA_FUNC(08913360, move_land_at_4) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x08913374u, f_089134A0);
    if (S16(o + 22) < 4) { *H(o + 22) = 4; inc8(o + 4); }
    call_on(c, o, 0x089133A0u, f_089134DC);
    END_S0();
}
/* (object): 0x0896F62C (moved); landed at +132 + 0x80 with +7 on and +20 cleared. */
SFA_FUNC(0896E63C, move_land_at_132) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x0896E650u, f_0896F62C);
    int16_t ground = (int16_t)(S16(o + 132) + 0x80);
    if (ground < S16(o + 22)) { *H(o + 22) = (uint16_t)ground; inc8(o + 7); *H(o + 20) = 0; }
    END_S0();
}
/* (object): +0x52 counted down; below 0: +5 on, +0x52 = 0x78, speeds cleared, falling by 0.34375. */
SFA_FUNC(08971750, count_52_then_fall) {
    gaddr o = ARG(0), p = o + 0x52;
    int8_t n = (int8_t)(S8(p) - 1);
    *B(p) = (uint8_t)n;
    if (n >= 0) return;
    inc8(o + 5);
    *B(p) = 0x78;
    *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 96) = 0;
    *W(o + 108) = 0xFFFFA800u;
}
/* (owner, object): the object set up from the owner (+122, +121, +14, +11 - +13, +38, x, y), owner at +88. */
SFA_FUNC(0897CAD4, setup_from_owner) {
    gaddr w = ARG(0), o = ARG(1);
    *B(o + 122) = (uint8_t)S8(w + 122);
    *B(o + 121) = (uint8_t)S8(w + 121);
    *B(o + 14) = *B(w + 14);
    *B(o + 11) = *B(w + 11);
    *B(o + 12) = (uint8_t)S8(w + 12);
    *B(o + 13) = *B(w + 13);
    *H(o + 38) = (uint16_t)S16(w + 38);
    *H(o + 18) = (uint16_t)S16(w + 18);
    int16_t y = S16(w + 22);
    *W(o + 88) = w;
    *H(o + 22) = (uint16_t)y;
}
/* (object): +9 = 1, +78 = 2, +86 = its owner's side (+337), +164 the owner's; then the animation at
   0x08A656F0. */
SFA_FUNC(08924394, setup_side_animation) {
    gaddr o = ARG(0), w = *W(o + 88);
    *B(o + 9) = 1; *B(o + 78) = 2;
    *H(o + 86) = (uint16_t)(int16_t)S8(w + 337);
    *W(o + 164) = *W(w + 164);
    ARG(2) = 0x08A60000u;
    ARG(1) = *W(0x08A6569Cu + 84);
    WRAP20(0x089243D8u, f_0882196C);
}
/* (object): +82 = 3, +13 = +139, +140 cleared; then the animation by +78 from 0x08A6569C. */
SFA_FUNC(08924734, setup_animation_by_78) {
    gaddr o = ARG(0);
    uint32_t n = (uint32_t)(int32_t)S8(o + 78) & 0xFFFF;
    int8_t b139 = S8(o + 139);
    *H(o + 82) = 3;
    *B(o + 13) = (uint8_t)b139;
    *B(o + 140) = 0;
    ARG(3) = (uint32_t)(int32_t)b139; ARG(2) = 0x08A6569Cu;
    ARG(1) = *W(0x08A6569Cu + n * 4);
    WRAP20(0x08924778u, f_0882196C);
}
/* (object): facing cleared, +72 cleared, its owner (+88) also at +92; at the camera's x + 0xC0 and the
   owner's +132; then the animation at 0x08A90F6C. */
SFA_FUNC(08980E90, setup_at_camera) {
    gaddr o = ARG(0), w = *W(o + 88);
    *B(o + 11) = 0; *B(o + 72) = 0;
    *W(o + 92) = w;
    *H(o + 18) = (uint16_t)(S16(0x092E88A0u + 10) + 0xC0);
    *H(o + 22) = (uint16_t)S16(w + 132);
    ARG(2) = 0x08A90000u;
    ARG(1) = *W(0x08A90CF0u + 636);
    WRAP20(0x08980ED4u, f_0882196C);
}
/* (object): facing cleared; 1 when it is more than 0xC0 right of what 0x08821330 answers. */
SFA_FUNC(0898820C, right_of_by_C0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[17] = o; c->r[16] = 0;
    c->r[31] = 0x08988228u; f_08821330(c);
    gaddr p = c->r[2];
    *B(o + 11) = 0;
    RETURN(S16(p + 18) + 0xC0 < S16(o + 18));
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
