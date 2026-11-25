/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 9 - 18
   instructions, ready once batch_tiny6.c was done: whole-frame step dispatchers, palette loads,
   objects set up from their fighter, a new object linked to its owner, steps chosen by a field.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088969BC(Cpu *c); void f_0899CF0C(Cpu *c); void f_0899CF4C(Cpu *c); void f_0880AEE4(Cpu *c);
void f_0887D9D0(Cpu *c); void f_0887D990(Cpu *c); void f_088219B0(Cpu *c); void f_088215C4(Cpu *c);
void f_08821424(Cpu *c); void f_089E11C8(Cpu *c); void f_088768F0(Cpu *c); void f_08940E38(Cpu *c);
void f_08940E68(Cpu *c); void f_089410F8(Cpu *c); void f_089613E8(Cpu *c); void f_08822F54(Cpu *c);

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

/* The object kept in s0 (saved at +16 of a 0x20 frame, the return address at +20) around `body`. */
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- whole-frame step dispatchers (step.h) */
SFA_FUNC(0887F714, step_0887F714) { run_step_then(c, ARG(0), *B(ARG(0) + 5), 0x08A245D8u, 0x0887F744u, f_088215C4, NULL); }
SFA_FUNC(08904CCC, step_08904CCC) { run_step_then(c, ARG(0), *B(ARG(0) + 5), 0x08A516D8u, 0x08904CFCu, f_08821424, NULL); }
SFA_FUNC(08947E4C, step_08947E4C) { run_step_then(c, ARG(0), *B(ARG(0) + 5), 0x08A766F4u, 0x08947E7Cu, f_08821424, NULL); }
SFA_FUNC(089B2BC8, step_089B2BC8) { run_step_then(c, ARG(0), *B(ARG(0) + 5), 0x08AA3D68u, 0x089B2BF8u, f_08821424, NULL); }

/* ---------------------------------------------------------------- palettes (0x088969BC: palette n, slot, count) */
SFA_FUNC(08896808, palette_101) { ARG(0) = 0x101; ARG(1) = 0x14; ARG(2) = 0xB; WRAP20(0x08896820u, f_088969BC); }
/* (a, -, b, slot): fighter palette a x 24 + b x 4 + 0x148 into the slot, 4 of them. */
static void palette_by(Cpu *c, uint32_t back) {
    uint32_t a = ARG(0) & 0xFFFF, b = ARG(2) & 0xFFFF;
    c->r[8] = a * 16;
    ARG(0) = a * 24 + b * 4 + 0x148;
    ARG(1) = ARG(3) & 0xFFFF;
    ARG(2) = 4;
    WRAP20(back, f_088969BC);
}
SFA_FUNC(0889693C, palette_by_a) { palette_by(c, 0x08896970u); }
SFA_FUNC(0889697C, palette_by_b) { palette_by(c, 0x088969B0u); }
/* (record, object): the record's +8 on, +10 / +22 cleared; the object's +18, +66, +82 cleared; then
   the palettes at 0x08A03C00 loaded (0x0880AEE4). */
SFA_FUNC(08832024, reset_then_palette_08A03C00) {
    gaddr r = ARG(0), o = ARG(1);
    *H(r + 8) = (uint16_t)(*H(r + 8) + 1);
    *B(o + 82) = 0; *H(o + 18) = 0; *B(o + 66) = 0;
    *H(r + 22) = 0; *H(r + 10) = 0;
    ARG(1) = 0; ARG(2) = 0; ARG(3) = 0;
    wrap(c, 0x10, 0, 0x08832060u, f_0880AEE4);
}
/* (object): +82 / +32 cleared, +4 on, and 16 colours copied from 0x092EEDC0 to 0x092E0160. */
SFA_FUNC(088AD784, step_on_4_copy_colours) {
    gaddr o = ARG(0);
    *H(o + 82) = 0;
    inc8(o + 4);
    *H(o + 32) = 0;
    ARG(0) = 0x092E0160u; ARG(1) = 0x092EEDC0u; ARG(2) = 0x10;
    WRAP20(0x088AD7C0u, f_089E11C8);
}

/* ---------------------------------------------------------------- objects */
/* (object): 0x0899CF0C then 0x0899CF4C (x the way it faces, then y). */
SFA_FUNC(0899CF6C, move_x_facing_then_y_b) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x0899CF80u, f_0899CF0C);
    call_on(c, o, 0x0899CF88u, f_0899CF4C);
    END_S0();
}
/* (fighter): +9 cleared, 0x0887D9D0; +12 = -1 and +13 = 2 unless +121 is set; then 0x0887D990. */
SFA_FUNC(0887D948, reset_12_13_unless_121) {
    gaddr f = ARG(0); WITH_S0(f);
    *B(f + 9) = 0;
    call_on(c, f, 0x0887D960u, f_0887D9D0);
    if (S8(f + 121) == 0) { ARG(1) = 2; *B(f + 12) = 0xFF; *B(f + 13) = 2; }
    call_on(c, f, 0x0887D980u, f_0887D990);
    END_S0();
}
/* (object): while +75 is negative, +5 cleared and +4 on; otherwise animate. */
SFA_FUNC(0887F4A0, step_on_4_or_animate) {
    gaddr o = ARG(0);
    ARG(1) = (uint32_t)(int32_t)S8(o + 75) & 0x80;
    if (ARG(1)) { *B(o + 5) = 0; inc8(o + 4); return; }
    WRAP20(0x0887F4DCu, f_088219B0);
}
/* (object, fighter): step 1 (+4), set up from the fighter (+24, +3 character, +12 - +14, +86, +164),
   +78 cleared. */
static void start_from_fighter(gaddr o, gaddr f) {
    *B(o + 4) = 1;
    *H(o + 24) = *H(f + 24);
    *B(o + 3) = (uint8_t)S8(f + 338);
    *B(o + 12) = (uint8_t)S8(f + 12);
    *B(o + 13) = *B(f + 13);
    uint8_t b14 = *B(f + 14);
    *B(o + 78) = 0;
    *B(o + 14) = b14;
    *H(o + 86) = *H(f + 86);
    *W(o + 164) = *W(f + 164);
}
SFA_FUNC(08917994, start_from_fighter_a) { start_from_fighter(ARG(0), ARG(1)); }
SFA_FUNC(08928A68, start_from_fighter_b) { start_from_fighter(ARG(0), ARG(1)); }
/* (object): while +75 is negative, +0 = 2 and step 2 (+4 = 2, +5 - +7 cleared); otherwise 0x088768F0. */
SFA_FUNC(0893A688, step_2_or_088768F0) {
    gaddr o = ARG(0);
    int8_t v = S8(o + 75);
    ARG(1) = 2;
    if (v < 0) { *B(o) = 2; *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; return; }
    WRAP20(0x0893A6C4u, f_088768F0);
}
/* (object): by +5, 0x08940E38 / 0x089410F8 (0) or 0x08940E68 (set), then 0x08821424. */
static void by_5_then_run(Cpu *c, void (*zero)(Cpu *), uint32_t back0, uint32_t back1, uint32_t back_run) {
    gaddr o = ARG(0); WITH_S0(o);
    if (*B(o + 5) == 0) call_on(c, o, back0, zero);
    else call_on(c, o, back1, f_08940E68);
    call_on(c, o, back_run, f_08821424);
    END_S0();
}
SFA_FUNC(08940DF0, by_5_a) { by_5_then_run(c, f_08940E38, 0x08940E10u, 0x08940E20u, 0x08940E28u); }
SFA_FUNC(089410B0, by_5_b) { by_5_then_run(c, f_089410F8, 0x089410D0u, 0x089410E0u, 0x089410E8u); }
/* (object): 0x089613E8 (x the way it faces); once the speed is negative: speed and change cleared,
   +7 on, +262 cleared. */
SFA_FUNC(0895D6A0, move_then_stop_step_7) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x0895D6B4u, f_089613E8);
    if ((int32_t)*W(o + 96) < 0) {
        *W(o + 96) = 0;
        inc8(o + 7);
        *W(o + 104) = 0;
        *B(o + 262) = 0;
    }
    END_S0();
}
/* (object): step 2 (+4) while +75 is negative; otherwise animate and draw. */
SFA_FUNC(08971848, step_2_or_animate_draw) {
    gaddr o = ARG(0); WITH_S0(o);
    if (S8(o + 75) < 0) *B(o + 4) = 2;
    else { call_on(c, o, 0x08971868u, f_088219B0); call_on(c, o, 0x08971870u, f_088215C4); }
    END_S0();
}
/* (object): x moved the way it faces, the speed changed by +104; 1 once the speed is negative. */
SFA_FUNC(08993090, move_x_facing_until_back) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) != 0 ? v : 0u - v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    RETURN((int32_t)nv < 0);
}
/* (object, x): thrown towards x: speed (x - its x - 640.0) / 256 x 12 (rounding to zero), change 0;
   up 3.0, falling by 0.125. */
SFA_FUNC(089BB148, throw_towards) {
    gaddr o = ARG(0);
    int32_t d = (int32_t)(ARG(1) - *W(o + 16) - 0x2800000u);
    int32_t q = (int32_t)((uint32_t)d + ((uint32_t)(d >> 8) >> 24)) >> 8;
    *W(o + 96) = (uint32_t)q * 12u;
    *W(o + 104) = 0;
    *W(o + 100) = 0x30000u;
    *W(o + 108) = 0xFFFFE000u;
}
/* (owner): a new object (0x08822F54: list 0x3A, 0) with the owner at +88, +0 = 1, +14 the owner's;
   the owner's +48 = it. */
SFA_FUNC(089CCEC0, new_object_3A_linked) {
    gaddr owner = ARG(0); WITH_S0(owner);
    ARG(0) = 0x3A; ARG(1) = 0;
    c->r[31] = 0x089CCEDCu; f_08822F54(c);
    gaddr n = c->r[2];
    ARG(0) = 1;
    if (n != 0) {
        *W(n + 88) = owner;
        *B(n) = 1;
        ARG(0) = *B(owner + 14);
        *W(owner + 48) = n;
        *B(n + 14) = (uint8_t)ARG(0);
    }
    END_S0();
}
