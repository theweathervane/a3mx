/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 11 - 19
   instructions, ready once batch_tiny7.c was done: moves (by facing, with stops), objects at their
   owner's place, palettes by a fighter's colour, the CPU's script count, steps that wait for BATTLE +37,
   new objects linked to their owner. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08993090(Cpu *c); void f_089930D8(Cpu *c); void f_089BB148(Cpu *c); void f_08832024(Cpu *c);
void f_0880AEA8(Cpu *c); void f_0887B1A8(Cpu *c); void f_088219B0(Cpu *c); void f_088DD124(Cpu *c);
void f_08876998(Cpu *c); void f_0887E278(Cpu *c); void f_0891B9E0(Cpu *c); void f_08877258(Cpu *c);
void f_08821424(Cpu *c); void f_089B2A64(Cpu *c); void f_08822F54(Cpu *c); void f_088D3B50(Cpu *c);

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

/* ---------------------------------------------------------------- moves */
/* (object): 0x08993090 then 0x089930D8 (x the way it faces, then y). */
SFA_FUNC(08993064, move_x_then_y_c) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x08993078u, f_08993090);
    call_on(c, o, 0x08993080u, f_089930D8);
    END_S0();
}
/* (object): y by its speed (changed by +108), then x the way it faces (+11 clear: back), changed by +104. */
SFA_FUNC(088F504C, move_y_then_x_facing) {
    gaddr o = ARG(0);
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    *W(o + 100) = vy + *W(o + 108);
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) == 0 ? 0u - v : v;
    *W(o + 96) = v + *W(o + 104);
}
/* (object): x moved by its speed (one's complement of it while +78 is clear), stopped once the speed
   goes negative. */
SFA_FUNC(089120F0, move_x_by_78_stop) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) += S8(o + 78) == 0 ? ~v : v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if (nv & 0x80000000u) { *W(o + 96) = 0; *W(o + 104) = 0; }
}
/* (object): its x speed and change cleared when the speed points back (negative facing right, or
   not negative facing left), then 0x0891B9E0 (moved on both axes). */
SFA_FUNC(0891B994, stop_backwards_then_move) {
    gaddr o = ARG(0);
    int32_t v = (int32_t)*W(o + 96);
    uint8_t face = *B(o + 11);
    ARG(1) = face;
    if ((v < 0) == (face != 0)) { *W(o + 104) = 0; *W(o + 96) = 0; }
    WRAP20(0x0891B9D4u, f_0891B9E0);
}
/* (fighter): its partner's (+92) x / y moved by +0x3A8 / +0x3AC, which change by +0x3B0 / +0x3B4. */
SFA_FUNC(08937744, move_partner) {
    gaddr o = ARG(0), w = *W(o + 92), p = o + 0x3A0;
    *W(w + 16) += *W(p + 8);
    uint32_t vy = *W(p + 12);
    *W(p + 8) += *W(p + 16);
    *W(w + 20) += vy;
    *W(p + 12) += *W(p + 20);
}
/* (object): speeds from the 12-byte row +824 of 0x08A76354: +100, +108 and +0x334; +96 / +104 cleared. */
SFA_FUNC(08946074, speeds_by_824) {
    gaddr o = ARG(0), e = 0x08A76354u + *W(o + 824) * 12;
    *W(o + 100) = *W(e);
    *W(o + 108) = *W(e + 4);
    *W(o + 0x334) = *W(e + 8);
    *W(o + 104) = 0; *W(o + 96) = 0;
}

/* ---------------------------------------------------------------- objects */
/* (object): thrown towards the camera's (0x092E88A0) x + 640.0 (0x089BB148). */
SFA_FUNC(089BB11C, throw_towards_camera) {
    ARG(2) = 0x2800000u;
    ARG(1) = *W(0x092E88A0u + 16) + 0x2800000u;
    WRAP20(0x089BB13Cu, f_089BB148);
}
/* (record, object): +22 counted down; below 0, 0x08832024 (the reset and the palettes at 0x08A03C00). */
SFA_FUNC(08831FBC, count_22_then_reset) {
    gaddr r = ARG(0);
    int16_t n = (int16_t)(S16(r + 22) - 1);
    *H(r + 22) = (uint16_t)n;
    ARG(2) = (uint32_t)(int32_t)n;
    if (n < 0) WRAP20(0x08831FE8u, f_08832024);
}
/* (object): at its owner's (+88) place moved by +112 / +114 (x mirrored with the owner's facing). */
SFA_FUNC(0887FB14, at_owner_offset) {
    gaddr o = ARG(0), w = *W(o + 88);
    uint8_t face = *B(w + 11);
    int16_t dx = S16(o + 112), dy = S16(o + 114);
    *B(o + 11) = face;
    int16_t x = S16(w + 18);
    *H(o + 18) = (uint16_t)(face != 0 ? x - dx : x + dx);
    *H(o + 22) = (uint16_t)(S16(w + 22) + dy);
}
/* (object): +0x52 counted down; when it goes below 0: +5 on and +0x52 = 0x17; then animate. */
SFA_FUNC(0887F7A4, count_52_then_animate) {
    gaddr o = ARG(0), p = o + 0x52;
    int32_t n = S8(p) - 1;
    *B(p) = (uint8_t)n;
    ARG(1) = p;
    if (n & 0x80) { inc8(o + 5); *B(p) = 0x17; }
    WRAP20(0x0887F7E4u, f_088219B0);
}
/* (object): +0x53 counted down; below 0, +6 on; then 0x08821424. */
SFA_FUNC(0896FC88, count_53_then_run) {
    gaddr o = ARG(0), p = o + 0x53;
    int8_t n = (int8_t)(S8(p) - 1);
    *B(p) = (uint8_t)n;
    ARG(1) = (uint32_t)(int32_t)n;
    if (n < 0) ARG(1) = inc8(o + 6);
    WRAP20(0x0896FCC8u, f_08821424);
}
/* (object): while +0x53 is set, counted down; every fourth, a new kind-4 object of its own (0x089B2A64). */
SFA_FUNC(089B2A18, count_53_spawn_4) {
    gaddr o = ARG(0), p = o + 0x53;
    ARG(1) = p;
    if (S8(p) == 0) return;
    int32_t n = S8(p) - 1;
    *B(p) = (uint8_t)n;
    ARG(1) = (uint32_t)n & 3;
    if ((n & 3) == 0) { ARG(1) = 4; WRAP20(0x089B2A58u, f_089B2A64); }
}
/* (object): a new object (0x0887E278); if one came, it is put 32 to the side it faces from. */
SFA_FUNC(08917870, new_object_beside) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x08917884u, f_0887E278);
    gaddr n = c->r[2];
    ARG(0) = n;
    if (n != 0) *H(n + 18) = (uint16_t)(S16(n + 18) + (*B(o + 11) == 0 ? 0x20 : -0x20));
    END_S0();
}
/* (owner): a new object (0x08822F54: list 0x33) with the owner at +88, +0 = 1, +139 and +14 the
   owner's; the owner's +40 = it. */
SFA_FUNC(089B78B0, new_object_33_linked) {
    gaddr owner = ARG(0); WITH_S0(owner);
    ARG(0) = 0x33;
    c->r[31] = 0x089B78C8u; f_08822F54(c);
    gaddr n = c->r[2];
    ARG(0) = 1;
    if (n != 0) {
        *B(n) = 1;
        *W(n + 88) = owner;
        *B(n + 139) = (uint8_t)S8(owner + 139);
        ARG(0) = *B(owner + 14);
        *B(n + 14) = (uint8_t)ARG(0);
        *W(owner + 40) = n;
    }
    END_S0();
}
/* (object): its step from a table by +4 / +3, unless BATTLE +37 is set (0x088D3B50). */
static void step_unless_37(Cpu *c, uint32_t at, gaddr table, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, back1, f_088D3B50);
    if (c->r[2] == 0) {
        gaddr h = *W(table + (uint32_t)*B(o + at) * 4);
        ARG(0) = o; ARG(1) = h;
        c->r[31] = back2;
        rt_call_indirect(c, h);
    }
    END_S0();
}
SFA_FUNC(089BE384, step_unless_37_a) { step_unless_37(c, 4, 0x08AAA320u, 0x089BE398u, 0x089BE3C0u); }
SFA_FUNC(089BE3D0, step_unless_37_b) { step_unless_37(c, 4, 0x08AAA330u, 0x089BE3E4u, 0x089BE40Cu); }
SFA_FUNC(089BE874, step_unless_37_c) { step_unless_37(c, 3, 0x08AAA340u, 0x089BE888u, 0x089BE8B0u); }

/* ---------------------------------------------------------------- fighters */
/* (fighter, n): unless +657 is n already, its palette for its colour (+376) into its slot (+13),
   from its routine table's (+416) +132. */
SFA_FUNC(0886F904, palette_by_657) {
    gaddr f = ARG(0);
    int8_t n = (int8_t)ARG(1);
    uint32_t base = *W(*W(f + 416) + 132);
    ARG(1) = base;
    if (S8(f + 657) == n) return;
    ARG(1) = base + ((uint32_t)(int32_t)S8(f + 376) << 5);
    ARG(3) = *B(f + 13);
    ARG(0) = 0; ARG(2) = 0; c->r[8] = 1;
    wrap(c, 0x30, 32, 0x0886F944u, f_0880AEA8);
}
/* (fighter): the CPU's script count +506 down; at 0 (or already 0) its first script and command are
   cleared (0x0887B1A8). */
SFA_FUNC(0887A800, cpu_count_506) {
    gaddr f = ARG(0);
    int16_t v = S16(f + 506);
    ARG(1) = (uint32_t)(int32_t)(v - 1);
    if (v == 0) { WRAP20(0x0887A81Cu, f_0887B1A8); return; }
    *H(f + 506) = (uint16_t)(v - 1);
    ARG(1) = (uint32_t)(int32_t)S16(f + 506);
    if (S16(f + 506) == 0) WRAP20(0x0887A840u, f_0887B1A8);
}
/* (fighter): +128 = the value for its health (+112) from 0x08A4732C / 0x08A6D0AC, by a start its
   character (+346) gives in 0x08A4993C / 0x08A6DD25; then 0x08876998 with it. */
static void value_by_health(Cpu *c, gaddr starts, gaddr values, uint32_t back) {
    gaddr f = ARG(0);
    uint32_t i = *B(starts + (uint32_t)(int32_t)S8(f + 346)) + (uint32_t)(int32_t)S16(f + 112);
    uint32_t v = *W(values + i * 4);
    ARG(2) = values; ARG(1) = v;
    *W(f + 128) = v;
    WRAP20(back, f_08876998);
}
SFA_FUNC(088FC444, value_by_health_a) { value_by_health(c, 0x08A4993Fu - 3, 0x08A4732Cu, 0x088FC484u); }
SFA_FUNC(08933350, value_by_health_b) { value_by_health(c, 0x08A6DD28u - 3, 0x08A6D0ACu, 0x08933390u); }
/* (fighter): for a human, whether +0x52 is negative; for the CPU, whether 0x08877258 (fighter,
   0x08A8DAD0) answers nonzero. */
SFA_FUNC(0896E990, human_52_or_cpu) {
    gaddr f = ARG(0);
    ARG(1) = 0x08A90000u;
    if (S8(f + 373) == 0) { RETURN(S8(f + 0x52) < 0); return; }
    ARG(1) = 0x08A8DAD0u;
    WRAP20(0x0896E9CCu, f_08877258);
    RETURN(c->r[2] != 0);
}
/* The session's +228 compared with 0x0952398C (when 0x095239AB is 2 or 4: 1 if not below it). */
SFA_FUNC(088D7E24, session_228_reached) {
    gaddr s = 0x09523980u;
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = s;
    c->r[31] = 0x088D7E3Cu; f_088DD124(c);
    uint8_t k = *B(s + 43);
    if (k == 2 || k == 4) c->r[2] = !((int32_t)c->r[2] < (int32_t)*W(s + 12));
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}
