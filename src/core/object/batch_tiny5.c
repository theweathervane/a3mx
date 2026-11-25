/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 8 - 15
   instructions, ready once batch_tiny4.c and steps_fighters.c were done: fighters' fields set from
   tables, objects set up from their fighter, moves with a test, step dispatchers with a field set
   first, and the palette copy (0x0880AEA8) every colour change goes through. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0893AAD0(Cpu *c); void f_0887DD64(Cpu *c); void f_088219B0(Cpu *c); void f_08821424(Cpu *c);
void f_0886BF84(Cpu *c); void f_0887DF88(Cpu *c); void f_088768F0(Cpu *c); void f_08822EAC(Cpu *c);
void f_089E11C8(Cpu *c); void f_0893BCF8(Cpu *c); void f_0893C140(Cpu *c); void f_0896F708(Cpu *c);
void f_0896FA38(Cpu *c); void f_0882196C(Cpu *c);

/* One call inside a 0x20 frame with the return address kept at +16, as the originals do. */
static void wrap(Cpu *c, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
static uint8_t inc8(gaddr a) { uint8_t v = (uint8_t)(*B(a) + 1); *B(a) = v; return v; }

/* ---------------------------------------------------------------- wrappers */
/* (object): +72 = 1, then 0x0893AAD0 (+14 from the owner, its animation). */
SFA_FUNC(0893AAB0, set_72_then_0893AAD0) { ARG(1) = 1; *B(ARG(0) + 72) = 1; wrap(c, 0x0893AAC4u, f_0893AAD0); }
/* (fighter, n): 0x0887DD64 (fighter, opponent (+92), its record n of 32 bytes at +224, opponent). */
SFA_FUNC(0887DD30, call_0887DD64_record) {
    gaddr f = ARG(0);
    uint32_t opp = *W(f + 92);
    ARG(2) = *W(f + 224) + ((ARG(1) & 0xFF) << 5);
    ARG(3) = opp; ARG(1) = opp;
    wrap(c, 0x0887DD58u, f_0887DD64);
}
/* (object): +4 on while +75 is negative, then animate (0x088219B0). */
static void step_if_75_then_animate(Cpu *c, uint32_t back) {
    gaddr o = ARG(0);
    int8_t v = S8(o + 75);
    ARG(1) = (uint32_t)(int32_t)v;
    if (v < 0) ARG(1) = inc8(o + 4);
    wrap(c, back, f_088219B0);
}
SFA_FUNC(0891E634, step_if_75_animate_a) { step_if_75_then_animate(c, 0x0891E65Cu); }
SFA_FUNC(0893AD20, step_if_75_animate_b) { step_if_75_then_animate(c, 0x0893AD48u); }
/* (object): when its owner's (+88) +77 is set, +5 on and 0x08821424. */
SFA_FUNC(08941B48, owner_77_step_on_5) {
    gaddr o = ARG(0);
    int8_t v = S8(*W(o + 88) + 77);
    ARG(1) = (uint32_t)(int32_t)v;
    if (v != 0) { ARG(2) = inc8(o + 5); wrap(c, 0x08941B70u, f_08821424); }
}
/* (fighter): 0 unless +566 is set; then 0x0886BF84's answer. */
SFA_FUNC(0886C8BC, if_566_call_0886BF84) {
    int8_t v = S8(ARG(0) + 566);
    ARG(1) = (uint32_t)(int32_t)v;
    if (v == 0) { RETURN(0); return; }
    wrap(c, 0x0886C8D8u, f_0886BF84);
}
/* (object): when its owner's (+88) +73 is set, +5 on and 0x0887DF88 (object, 0xF, the new +5). */
SFA_FUNC(088F1C10, owner_73_step_on_5) {
    gaddr o = ARG(0);
    int8_t v = S8(*W(o + 88) + 73);
    ARG(1) = (uint32_t)(int32_t)v;
    if (v != 0) { ARG(2) = inc8(o + 5); ARG(1) = 0xF; wrap(c, 0x088F1C3Cu, f_0887DF88); }
}
/* (object): step 3 (+4 = 3, +5 - +7 cleared) while +75 is negative, then 0x088768F0 (object, 3). */
SFA_FUNC(08940E68, step_3_if_75) {
    gaddr o = ARG(0);
    if (S8(o + 75) < 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
    ARG(1) = 3;
    wrap(c, 0x08940E94u, f_088768F0);
}
/* (owner, kind): a new object (0x08822EAC: list 0x24, the kind's low byte) with this owner at +88; the
   object (or 0) back. Keeps s0 in its frame as the original does. */
SFA_FUNC(089B2A64, new_object_24_owned) {
    gaddr owner = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = owner;
    ARG(1) &= 0xFF; ARG(0) = 0x24;
    c->r[31] = 0x089B2A80u;
    f_08822EAC(c);
    gaddr o = c->r[2];
    ARG(0) = o;
    if (o != 0) *W(o + 88) = owner;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (-, from, -, slot, n): n colours (half-words) from `from` into palette slot `slot` (0x092DF980 + 32
   a slot, through 0x089E11C8), and the flag at 0x092E0980 that the palettes changed. */
SFA_FUNC(0880AEA8, palette_load) {
    ARG(0) = 0x092DF980u + (ARG(3) << 5);
    ARG(2) = c->r[8] << 4;
    wrap(c, 0x0880AECCu, f_089E11C8);
    ARG(0) = 1; ARG(1) = 0x092E0000u;
    *W(0x092E0980u) = 1;
}
/* (object): 0x08821424 on frames where GAME +32 plus GAME +66 is odd. */
SFA_FUNC(089134DC, every_other_frame_08821424) {
    uint32_t odd = (uint32_t)(*B(GAME + 32) + S8(GAME + 66)) & 1;
    ARG(1) = odd;
    if (odd) wrap(c, 0x0891350Cu, f_08821424);
}
/* (fighter): +696 cleared, then 0x0893C140 for a human's button (+193 set) or 0x0893BCF8. */
SFA_FUNC(0893BCBC, by_193) {
    gaddr f = ARG(0);
    int8_t v = S8(f + 193);
    ARG(1) = (uint32_t)(int32_t)v;
    *B(f + 696) = 0;
    if (v != 0) wrap(c, 0x0893BCECu, f_0893C140);
    else wrap(c, 0x0893BCD8u, f_0893BCF8);
}
/* (object): 0x0896FA38 when +3 is set, else 0x0896F708. */
SFA_FUNC(0896F6CC, by_3) {
    ARG(1) = *B(ARG(0) + 3);
    if (ARG(1) != 0) wrap(c, 0x0896F6FCu, f_0896FA38);
    else wrap(c, 0x0896F6E8u, f_0896F708);
}
/* (object): at its fighter's (+92) place, +72 cleared, then the animation at 0x08A90F98. */
SFA_FUNC(08980F28, at_fighter_animation) {
    gaddr o = ARG(0), f = *W(o + 92);
    *B(o + 72) = 0;
    *H(o + 18) = (uint16_t)S16(f + 18);
    *H(o + 22) = (uint16_t)S16(f + 22);
    ARG(2) = 0x08A90000u;
    ARG(1) = *W(0x08A90CF0u + 680);
    wrap(c, 0x08980F58u, f_0882196C);
}
/* step dispatchers with a field set first */
SFA_FUNC(0890017C, step_1_then_0890017C) { *B(ARG(0) + 1) = 1; run_step(c, *B(ARG(0) + 6), 0x08A4BC0Cu, 5, 0x089001ACu); }
SFA_FUNC(089C8858, step_6_8_then_089C8858) { *B(ARG(0) + 6) = 8; run_step(c, *B(ARG(0) + 7), 0x08AB5DE8u, 5, 0x089C8888u); }
SFA_FUNC(089C8CB0, step_6_8_then_089C8CB0) { *B(ARG(0) + 6) = 8; run_step(c, *B(ARG(0) + 7), 0x08AB5E10u, 5, 0x089C8CE0u); }

/* ---------------------------------------------------------------- leaves */
/* (fighter): its opponent's (+92) +127 = the s8 at 0x08A1BFD4 by +194 (3 while +308 or +81 is set). */
SFA_FUNC(0886E294, opponent_127_by_194) {
    gaddr f = ARG(0), opp = *W(f + 92);
    int32_t n = (S8(f + 308) | S8(f + 81)) == 0 ? S8(f + 194) : 3;
    *B(opp + 127) = (uint8_t)S8(0x08A1BFD4u + (uint32_t)n);
    RETURN(n);
}
/* (object): x (+16) moved by its speed (+96) the way it faces, the speed changed by +104. */
SFA_FUNC(0896F5F8, move_x_facing_b) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) != 0 ? v : 0u - v;
    *W(o + 96) = v + *W(o + 104);
}
/* (object): the same, only while the speed is not negative. */
SFA_FUNC(08931A94, move_x_facing_forward) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    if ((int32_t)v < 0) return;
    *W(o + 16) += *B(o + 11) == 0 ? 0u - v : v;
    *W(o + 96) = v + *W(o + 104);
}
/* (object): 1 while falling (+100 negative) and still 16 above +132 (y +22 below +132 + 16). */
SFA_FUNC(089958E4, falling_above_132) {
    gaddr o = ARG(0);
    if ((int32_t)*W(o + 100) >= 0) { RETURN(0); return; }
    RETURN(S16(o + 132) + 16 < S16(o + 22));
}
/* (fighter): +0x334 cleared, +0x336 = the s16 at 0x08A9C04C by +194 and +0x339. */
SFA_FUNC(089A04A4, set_336_by_194) {
    gaddr f = ARG(0), p = f + 0x334;
    *H(p) = 0;
    int32_t i = S8(f + 194) * 4 + S8(p + 5);
    *H(p + 2) = (uint16_t)S16(0x08A9C04Cu + (uint32_t)i * 2);
}
/* (fighter): 1 when the buttons just pressed (+760, bits 4 - 9) are exactly 0x30, 0x50, 0x60 or 0x70. */
SFA_FUNC(088D30A0, two_punches_pressed) {
    uint32_t k = *H(ARG(0) + 760) & 0x3F0;
    RETURN(k == 0x50 || k == 0x70 || k == 0x30 || k == 0x60);
}
/* (object): +4 on, +9 = +72 = +15 = 1, +24 = its owner's (+88), +86 = 5. */
SFA_FUNC(088F1B78, start_from_owner_24) {
    gaddr o = ARG(0), w = *W(o + 88);
    inc8(o + 4);
    *B(o + 9) = 1; *B(o + 72) = 1; *B(o + 15) = 1;
    *H(o + 24) = *H(w + 24);
    *H(o + 86) = 5;
}
/* (object): when +123 is set: cleared, +82 counted down, and +234 = 1 once it reaches 0. */
SFA_FUNC(08903954, count_82_on_123) {
    gaddr o = ARG(0);
    if (S8(o + 123) == 0) return;
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *B(o + 123) = 0;
    *H(o + 82) = (uint16_t)n;
    if (n == 0) *B(o + 234) = 1;
}
/* (object, fighter): +139 = -1, or by its +346 from 0x08A516D0 unless the fighter's ISM (+386) is negative. */
SFA_FUNC(08904C94, set_139_by_346) {
    gaddr o = ARG(0), f = ARG(1);
    *B(o + 139) = 0xFF;
    if (S8(f + 386) < 0) return;
    *B(o + 139) = (uint8_t)S16(0x08A516D0u + (uint32_t)(int32_t)S8(o + 346) * 2);
}
/* (fighter): when +77 changed (kept at +0x3BD), the move number +656 = it + 0x18 (0 if negative). */
SFA_FUNC(0893E0B0, move_656_from_77) {
    gaddr f = ARG(0), p = f + 0x3B8;
    int8_t v = S8(f + 77);
    if (v == S8(p + 5)) return;
    *B(p + 5) = (uint8_t)v;
    v = S8(f + 77);
    *B(f + 656) = v < 0 ? 0 : (uint8_t)(v + 0x18);
}
/* (object, fighter): set up from its fighter: +24, +3 (character, +338), +12, +13, +14; +78 cleared. */
static void from_fighter(gaddr o, gaddr f) {
    *H(o + 24) = *H(f + 24);
    *B(o + 3) = (uint8_t)S8(f + 338);
    *B(o + 12) = (uint8_t)S8(f + 12);
    *B(o + 13) = *B(f + 13);
    uint8_t b14 = *B(f + 14);
    *B(o + 78) = 0;
    *B(o + 14) = b14;
}
SFA_FUNC(08947C34, start_1_from_fighter) { *B(ARG(0) + 4) = 1; from_fighter(ARG(0), ARG(1)); }
SFA_FUNC(0890047C, step_on_4_from_fighter) { inc8(ARG(0) + 4); from_fighter(ARG(0), ARG(1)); }
/* (object): moved on both axes, the y speed changed by +108 (not x's); +22 - +132 back / whether
   +132 is above +22. */
static void move_xy_y_only(gaddr o) {
    uint32_t vy = *W(o + 100), vx = *W(o + 96);
    *W(o + 20) += vy;
    *W(o + 16) += vx;
    *W(o + 100) = vy + *W(o + 108);
}
SFA_FUNC(088F3FA4, move_height_a) { gaddr o = ARG(0); move_xy_y_only(o); RETURN(S16(o + 22) - S16(o + 132)); }
SFA_FUNC(088F8AF8, move_height_b) { gaddr o = ARG(0); move_xy_y_only(o); RETURN(S16(o + 22) - S16(o + 132)); }
SFA_FUNC(08929118, move_below_132) { gaddr o = ARG(0); move_xy_y_only(o); RETURN(S16(o + 132) < S16(o + 22)); }
