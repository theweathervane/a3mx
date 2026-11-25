/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 11 - 17
   instructions, ready once batch_tiny5.c was done: palettes loaded through 0x0880AEA8, objects moved by
   the way they face, objects set up from their owner or fighter, owners' links and counts, and a
   fighter nudged by its held direction. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0896F5F8(Cpu *c); void f_0896F658(Cpu *c); void f_0880AEA8(Cpu *c); void f_0887DD30(Cpu *c);
void f_0887E7AC(Cpu *c); void f_08912FBC(Cpu *c); void f_088219B0(Cpu *c); void f_0882324C(Cpu *c);
void f_088230F4(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_08821424(Cpu *c);

/* One call inside a frame of `frame` bytes with the return address kept at `slot`, as the originals do. */
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

/* Two calls on the same object, kept in s0 (saved at +16 of a 0x20 frame, the return address at +20). */
static void both(Cpu *c, uint32_t back, void (*first)(Cpu *), void (*second)(Cpu *)) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = back; first(c);
    ARG(0) = o; c->r[31] = back + 8; second(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- palettes (0x0880AEA8: a1 the colours, a3 the slot, t0 how many x 16) */
static void palette(Cpu *c, uint32_t a0, uint32_t from, uint32_t a2, uint32_t slot, uint32_t n16, uint32_t back) {
    ARG(0) = a0; ARG(1) = from; ARG(2) = a2; ARG(3) = slot; c->r[8] = n16;
    wrap(c, 0x30, 32, back, f_0880AEA8);
}
/* 6 x 16 colours from 0x08A03C00 into slots 0x15 on. */
SFA_FUNC(0880AEE4, palette_08A03C00) { palette(c, 0, 0x08A03C00u, 0, 0x15, 6, 0x0880AF08u); }
/* 16 colours from 0x08A328B8 into slot 0x20. */
SFA_FUNC(088AA8B4, palette_08A328B8) { palette(c, 1, 0x08A321F8u + 0x6C0, 0, 0x20, 1, 0x088AA8DCu); }
/* (palette n, slot, count): the fighters' palettes (0x092D9E00's first entry) from palette n. */
SFA_FUNC(088969BC, palette_fighter_0) {
    uint32_t n = ARG(0), slot = ARG(1), count = ARG(2);
    palette(c, 0, *W(0x092D9E00u + 32) + n * 32, 0, slot, count, 0x088969F0u);
}
/* (-, colour): colour choice n (8 x 16 colours) of the palettes at 0x092D9E00's +52, into slot 0. */
SFA_FUNC(0889DB48, palette_colour_52) { palette(c, 0, *W(0x092D9E00u + 52) + (ARG(1) << 8), 0, 0, 8, 0x0889DB80u); }

/* ---------------------------------------------------------------- moves */
/* (object): x moved by its speed the way it faces (+11 set: forward), the speed changed by +104;
   the new speed back. */
static uint32_t move_x_facing(gaddr o) {
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) != 0 ? v : 0u - v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    return nv;
}
SFA_FUNC(088D3140, move_x_facing_c) { move_x_facing(ARG(0)); }
SFA_FUNC(089B298C, move_x_facing_d) { move_x_facing(ARG(0)); }
SFA_FUNC(089613E8, move_x_facing_answer_a) { RETURN(move_x_facing(ARG(0))); }
SFA_FUNC(0899CF0C, move_x_facing_answer_b) { RETURN(move_x_facing(ARG(0))); }
/* (object): the same, stopped (speed and change 0) once the speed goes negative. */
SFA_FUNC(089452F8, move_x_facing_stop) {
    gaddr o = ARG(0);
    if ((int32_t)move_x_facing(o) < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
}
/* (object): moved on both axes, the y speed changed by +108; whether +132 is not above +22 /
   +132 - +22 as an s16. */
static void move_xy_y_only(gaddr o) {
    uint32_t vy = *W(o + 100), vx = *W(o + 96);
    *W(o + 20) += vy;
    *W(o + 16) += vx;
    *W(o + 100) = vy + *W(o + 108);
}
SFA_FUNC(08963F34, move_not_below_132) { gaddr o = ARG(0); move_xy_y_only(o); RETURN(!(S16(o + 132) < S16(o + 22))); }
SFA_FUNC(088FF65C, move_132_less_22) { gaddr o = ARG(0); move_xy_y_only(o); RETURN((int32_t)(int16_t)(S16(o + 132) - S16(o + 22))); }
/* (object): 0x0896F5F8 then 0x0896F658 (x, then y). */
SFA_FUNC(0896F62C, move_x_facing_then_y) { both(c, 0x0896F640u, f_0896F5F8, f_0896F658); }
/* (object): 0x08912FBC, then its x speed and change cleared if the speed is negative. */
SFA_FUNC(08911550, call_08912FBC_stop_back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08911564u; f_08912FBC(c);
    if (*W(o + 96) & 0x80000000u) { *W(o + 96) = 0; *W(o + 104) = 0; }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (object): +4 on while +75 is negative, then animate and 0x08821424. */
SFA_FUNC(089A23F0, step_if_75_animate_run) {
    gaddr o = ARG(0);
    if (S8(o + 75) < 0) inc8(o + 4);
    both(c, 0x089A241Cu, f_088219B0, f_08821424);
}
/* (fighter, d): x (+18) nudged by d against the held direction (+362 bits 0 / 1), for a human (+373 clear). */
SFA_FUNC(089363D0, nudge_by_held) {
    gaddr f = ARG(0);
    int32_t d = (int16_t)ARG(1);
    if (S8(f + 373) != 0) return;
    uint32_t k = *H(f + 362);
    if ((k & 3) == 0) return;
    int16_t x = S16(f + 18);
    *H(f + 18) = (uint16_t)((k & 2) ? x - d : x + d);
}

/* ---------------------------------------------------------------- objects and owners */
/* (fighter): +196 on, +0x53 = ' ', then 0x0887DD30 (fighter, record 0x42). */
SFA_FUNC(08937290, count_196_record_42) {
    gaddr f = ARG(0);
    *B(f + 196) = (uint8_t)(S8(f + 196) + 1);
    *B(f + 0x53) = 0x20;
    ARG(2) = f + 0x53; ARG(1) = 0x42;
    WRAP20(0x089372BCu, f_0887DD30);
}
/* (-, a, b): the record at 0x08B026AC / 0x08B026BC set (+0 = 0, +1 = 1, +2 = 0x30 / 0x16, +4 = a,
   +6 = b), then 0x0887E7AC (-, the record). */
static void record_then_0887E7AC(Cpu *c, gaddr r, uint32_t kind, uint32_t back) {
    *B(r) = 0; *B(r + 1) = 1;
    *H(r + 4) = (uint16_t)ARG(1);
    *B(r + 2) = (uint8_t)kind;
    *H(r + 6) = (uint16_t)ARG(2);
    ARG(1) = r; ARG(3) = kind; c->r[8] = r; c->r[9] = 1;
    WRAP20(back, f_0887E7AC);
}
SFA_FUNC(0887E460, record_08B026AC) { record_then_0887E7AC(c, 0x08B026ACu, 0x30, 0x0887E494u); }
SFA_FUNC(0887E4E0, record_08B026BC) { record_then_0887E7AC(c, 0x08B026BCu, 0x16, 0x0887E514u); }
/* (object): step 2 (+4) when +78 equals its owner's (+88), else animate. */
SFA_FUNC(08924500, step_2_or_animate) {
    gaddr o = ARG(0);
    int8_t mine = S8(o + 78), theirs = S8(*W(o + 88) + 78);
    ARG(1) = 2;
    if (mine == theirs) { *B(o + 4) = 2; return; }
    WRAP20(0x08924524u, f_088219B0);
}
/* (object): +6 on and +0x53 = 2 once the count +82 goes below 0. */
SFA_FUNC(0896F97C, count_82_then_step_6) {
    gaddr o = ARG(0);
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n >= 0) return;
    inc8(o + 6);
    *B(o + 0x53) = 2;
}
/* (object): set up from its owner (+88): +72 = 1, +11, +24, +12 - +14, +38. */
SFA_FUNC(08971DA8, from_owner) {
    gaddr o = ARG(0), w = *W(o + 88);
    uint8_t face = *B(w + 11);
    *B(o + 72) = 1;
    *B(o + 11) = face;
    *H(o + 24) = *H(w + 24);
    *B(o + 12) = (uint8_t)S8(w + 12);
    *B(o + 13) = *B(w + 13);
    *B(o + 14) = *B(w + 14);
    *H(o + 38) = (uint16_t)S16(w + 38);
}
/* (object, fighter): set up from the fighter (+24, +3 character, +12 - +14; +78 or +127 cleared, +82 = 0). */
static void from_fighter(gaddr o, gaddr f, unsigned cleared) {
    *H(o + 24) = *H(f + 24);
    *B(o + 3) = (uint8_t)S8(f + 338);
    *B(o + 12) = (uint8_t)S8(f + 12);
    *B(o + 13) = *B(f + 13);
    uint8_t b14 = *B(f + 14);
    *B(o + cleared) = 0;
    *B(o + 14) = b14;
    *H(o + 82) = 0;
}
SFA_FUNC(08981B58, step_on_4_from_fighter_127) { inc8(ARG(0) + 4); from_fighter(ARG(0), ARG(1), 127); }
SFA_FUNC(0893B4C4, step_on_4_from_owner) { gaddr o = ARG(0), w = *W(o + 88); inc8(o + 4); from_fighter(o, w, 78); }
/* (object, owner): the owner's +40 / +44 / +48 cleared where they are this object, then freed. */
SFA_FUNC(0899D26C, unlink_40_48_then_free) {
    gaddr o = ARG(0), w = ARG(1);
    uint32_t last = 0;
    for (unsigned k = 40; k <= 48; k += 4) { last = *W(w + k); if (last == o) *W(w + k) = 0; }
    ARG(2) = last;   /* +48 as it was */
    WRAP20(0x0899D2A0u, f_0882324C);
}
/* (object): its owner's (+88) count +536 down (+228 cleared at 0), +502 cleared when +3 is 0, then
   0x088230F4 (object, owner, +3). */
static void owner_536_502(Cpu *c, uint32_t back) {
    gaddr o = ARG(0), w = *W(o + 88);
    *B(w + 536) = (uint8_t)(*B(w + 536) - 1);
    if (*B(w + 536) == 0) *W(w + 228) = 0;
    uint8_t b3 = *B(o + 3);
    if (b3 == 0) *B(w + 502) = 0;
    ARG(1) = w; ARG(2) = b3;
    WRAP20(back, f_088230F4);
}
SFA_FUNC(08900BBC, owner_536_502_a) { owner_536_502(c, 0x08900BF4u); }
SFA_FUNC(0899D63C, owner_536_502_b) { owner_536_502(c, 0x0899D674u); }
/* (object): +0x153 cycles 0 - 7; +13 = the byte at 0x08A64630 by it, plus its owner's (+88) +13. */
SFA_FUNC(0891D3D8, cycle_colour_13) {
    gaddr o = ARG(0), p = o + 0x150, w = *W(o + 88);
    *B(p + 3) = (uint8_t)((S8(p + 3) + 1) & 7);
    *B(o + 13) = *B(0x08A64630u + (uint32_t)(int32_t)S8(p + 3));
    *B(o + 13) = (uint8_t)(*B(o + 13) + *B(w + 13));
}
/* (fighter): for the CPU (+373), the routine at 0x08A70A40 by the held directions (+290, low 4 bits). */
SFA_FUNC(08939BF4, cpu_by_held) {
    gaddr f = ARG(0);
    int8_t cpu = S8(f + 373);
    ARG(1) = (uint32_t)(int32_t)cpu;
    if (cpu == 0) return;
    run_step(c, *H(f + 290) & 0xF, 0x08A70A40u, 5, 0x08939C2Cu);
}
/* (object): +5 on, then 0x0887684C (object, 7 when +3 is set, else 6). */
SFA_FUNC(089410F8, step_on_5_0887684C_by_3) {
    gaddr o = ARG(0);
    uint8_t b3 = *B(o + 3);
    inc8(o + 5);
    ARG(2) = b3;
    if (b3 != 0) { ARG(1) = 7; WRAP20(0x0894111Cu, f_0887684C); }
    else { ARG(1) = 6; WRAP20(0x08941130u, f_0887684C); }
}
/* (fighter): +128 = the value for its health (+112) in its character's (+346) table (0x08A8AA9C), then
   0x08876998 with it. */
SFA_FUNC(08965F58, value_by_health) {
    gaddr f = ARG(0);
    gaddr t = *W(0x08A8AAA8u + (uint32_t)(int32_t)S8(f + 346) * 4 - 12);
    uint32_t h4 = (uint32_t)(int32_t)S16(f + 112) << 2;
    uint32_t v = *W(t + h4);
    ARG(2) = h4; ARG(1) = v;
    *W(f + 128) = v;
    WRAP20(0x08965F90u, f_08876998);
}
