/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 22 - 23
   instructions, ready once batch_tiny11.c was done: hit marks chosen by whether the fighter is
   guarding, steps that run only while a condition holds, two-button checks on pressed or released,
   the fighter's flash palettes, moves by facing. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_088D3C88(Cpu *c); void f_08822F54(Cpu *c); void f_088D3B50(Cpu *c); void f_089DA8C4(Cpu *c);
void f_08870140(Cpu *c); void f_088717E8(Cpu *c); void f_08871808(Cpu *c); void f_08871A84(Cpu *c);
void f_088988E0(Cpu *c); void f_088D59A0(Cpu *c); void f_08979D40(Cpu *c); void f_088768F0(Cpu *c);

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

/* ---------------------------------------------------------------- steps with a condition */
/* (object): its step by +4 (0x08AB604C) while its +139 is its owner's; otherwise 0x088D3C88. */
SFA_FUNC(089CB91C, step_while_owner_139_c) {
    gaddr o = ARG(0);
    int8_t theirs = S8(*W(o + 88) + 139), mine = S8(o + 139);
    if (theirs == mine) { run_step(c, *B(o + 4), 0x08AB604Cu, 5, 0x089CB968u); return; }
    ARG(1) = (uint32_t)(int32_t)theirs; ARG(2) = (uint32_t)(int32_t)mine;
    WRAP20(0x089CB940u, f_088D3C88);
}
/* (object): +24 from its owner (+88); then, unless BATTLE +37 is set, its step by +4. */
static void step_with_owner_24(Cpu *c, gaddr table, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0); WITH_S0(o);
    uint16_t v = *H(*W(o + 88) + 24);
    *H(o + 24) = v;
    ARG(0) = v;
    c->r[31] = back1; f_088D3B50(c);
    if (c->r[2] == 0) {
        gaddr h = *W(table + (uint32_t)*B(o + 4) * 4);
        ARG(0) = o; ARG(1) = h;
        c->r[31] = back2;
        rt_call_indirect(c, h);
    }
    END_S0();
}
SFA_FUNC(089CC8A0, step_with_owner_24_a) { step_with_owner_24(c, 0x08AB6104u, 0x089CC8C0u, 0x089CC8E8u); }
SFA_FUNC(089CC8F8, step_with_owner_24_b) { step_with_owner_24(c, 0x08AB6114u, 0x089CC918u, 0x089CC940u); }
/* (object): while GAME +40 is clear, its step by +4 (0x08A2A088); otherwise 0x088988E0. */
SFA_FUNC(08898374, step_unless_game_40_e) {
    int8_t v = S8(GAME + 40);
    if (v != 0) { ARG(1) = (uint32_t)(int32_t)v; WRAP20(0x088983C4u, f_088988E0); return; }
    run_step(c, *B(ARG(0) + 4), 0x08A2A088u, 5, 0x088983B0u);
}
/* (object): its step by its signed +73 (0x08A903B8) while +3 is its owner's character; otherwise
   0x08979D40 (+4 on, its routine). */
SFA_FUNC(08978E14, step_by_73_while_owner_character) {
    gaddr o = ARG(0);
    int32_t ch = S8(*W(o + 88) + 338);
    if ((int32_t)*B(o + 3) != ch) { ARG(1) = (uint32_t)ch; ARG(2) = *B(o + 3); WRAP20(0x08978E64u, f_08979D40); return; }
    run_step(c, (uint32_t)S8(o + 73), 0x08A903B8u, 5, 0x08978E50u);
}
/* (object): +4 on while BATTLE +42 is set; otherwise its step by +3 (0x08A928D0). */
SFA_FUNC(08980FCC, step_by_3_or_step_on) {
    if (S8(BATTLE + 42) != 0) { ARG(1) = inc8(ARG(0) + 4); return; }
    run_step(c, *B(ARG(0) + 3), 0x08A928D0u, 5, 0x08981008u);
}
/* (object): its step by +5 (0x08A928EC) while +127 is its owner's character; otherwise +4 on. */
SFA_FUNC(089810C4, step_by_5_while_127) {
    gaddr o = ARG(0);
    int8_t ch = S8(*W(o + 88) + 338);
    if (ch != S8(o + 127)) { ARG(1) = inc8(o + 4); return; }
    run_step(c, *B(o + 5), 0x08A928ECu, 5, 0x08981100u);
}

/* ---------------------------------------------------------------- objects */
/* (owner): a new object (0x08822F54: list 0x37, 0) with the owner at +88, +139 and +14 the owner's,
   +0 = 1, +3 = 0; the owner's +40 = it. */
SFA_FUNC(089CC74C, new_object_37_linked) {
    gaddr owner = ARG(0); WITH_S0(owner);
    ARG(0) = 0x37; ARG(1) = 0;
    c->r[31] = 0x089CC768u; f_08822F54(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = owner;
        *B(n + 139) = (uint8_t)S8(owner + 139);
        ARG(1) = 1;
        *B(n) = 1;
        *B(n + 3) = 0;
        ARG(0) = *B(owner + 14);
        *W(owner + 40) = n;
        *B(n + 14) = (uint8_t)ARG(0);
    }
    END_S0();
}
/* (object, record): unless the record's +2 is below +120, +5 on and 0x089DA8C4 (object, record). */
SFA_FUNC(089DA86C, step_on_5_by_120) {
    gaddr o = ARG(0), r = ARG(1);
    int32_t lim = *B(o + 120);
    ARG(0) = r;
    if (S16(r + 2) < lim) return;
    inc8(o + 5);
    ARG(0) = o; ARG(1) = r; ARG(2) = r;
    WRAP20(0x089DA8B8u, f_089DA8C4);
}
/* (object): x moved by its speed (+11 set: back, clear: forward / the other way round), changed by
   +104; y before or after. */
static void move_x(gaddr o, int plus_when_facing) {
    uint32_t v = *W(o + 96);
    int plus = (*B(o + 11) != 0) == plus_when_facing;
    *W(o + 16) += plus ? v : 0u - v;
    *W(o + 96) = v + *W(o + 104);
}
static void move_y(gaddr o) {
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    *W(o + 100) = vy + *W(o + 108);
}
SFA_FUNC(0893F1D0, move_y_then_x_reversed) { move_y(ARG(0)); move_x(ARG(0), 0); }
SFA_FUNC(0896A34C, move_y_then_x_facing_b) { move_y(ARG(0)); move_x(ARG(0), 1); }
/* (object): x the way it faces, stopped once the speed goes negative; then y. */
SFA_FUNC(0899FC90, move_x_stop_then_y_b) {
    gaddr o = ARG(0);
    move_x(o, 1);
    if ((int32_t)*W(o + 96) < 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
    move_y(o);
}
/* (object): y moved; below 40, step 3 (+4 = 3, +5 - +7 cleared) and y cleared; then 0x088768F0. */
SFA_FUNC(08980BC8, fall_to_40) {
    gaddr o = ARG(0);
    move_y(o);
    ARG(1) = 3;
    if (S16(o + 22) < 40) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; *W(o + 20) = 0; }
    WRAP20(0x08980C18u, f_088768F0);
}
/* (fighter): +0x358, +0x360 and +0x370 cleared. */
SFA_FUNC(088FDA68, clear_358_360_370) {
    gaddr f = ARG(0);
    *W(f + 0x358) = 0; *W(f + 0x360) = 0; *W(f + 0x370) = 0;
}

/* ---------------------------------------------------------------- fighters */
/* (fighter): pressed or released (+760 | +762): light kick with heavy punch, or light punch with heavy
   kick / medium kick with heavy punch, or medium punch with heavy kick. */
SFA_FUNC(08869FEC, pressed_or_released_lk_hp_or_lp_hk) {
    gaddr f = ARG(0);
    uint32_t k = (*H(f + 760) | *H(f + 762)) & 0xFFFF;
    RETURN(((k & 0x80) && (k & 0x40)) || ((k & 0x10) && (k & 0x200)));
}
SFA_FUNC(0886A048, pressed_or_released_mk_hp_or_mp_hk) {
    gaddr f = ARG(0);
    uint32_t k = (*H(f + 760) | *H(f + 762)) & 0xFFFF;
    RETURN(((k & 0x100) && (k & 0x40)) || ((k & 0x20) && (k & 0x200)));
}
/* (fighter, n): unless +657 is n, its palette slot 0 flashed white (0x08870140 (fighter, 0, 0xFFFF));
   and slot 1 too when +593 is 2 or more. */
SFA_FUNC(0886F194, flash_white) {
    gaddr f = ARG(0);
    int8_t n = (int8_t)ARG(1);
    WITH_S0(f);
    if (S8(f + 657) != n) { ARG(0) = f; ARG(1) = 0; ARG(2) = 0xFFFF; c->r[31] = 0x0886F1C4u; f_08870140(c); }
    ARG(2) = 0xFFFF;
    ARG(0) = S8(f + 593) < 2;
    if (!ARG(0)) { ARG(0) = f; ARG(1) = 1; c->r[31] = 0x0886F1E0u; f_08870140(c); }
    END_S0();
}
/* (fighter, x): while guarding (+696), 0x088717E8 / 0x08871808 (+117 = 9 / 10); otherwise +117 = the
   given mark and 0x08871A84 (fighter, x, x). */
static void mark_by_guard(Cpu *c, void (*guarding)(Cpu *), uint32_t back_g, uint8_t mark, uint32_t back) {
    gaddr f = ARG(0);
    uint32_t x = ARG(1);
    ARG(2) = x;
    if (S8(f + 696) != 0) { ARG(0) = f; ARG(1) = x; WRAP20(back_g, guarding); return; }
    *B(f + 117) = mark;
    ARG(0) = f; ARG(1) = x;
    WRAP20(back, f_08871A84);
}
SFA_FUNC(08871508, mark_3_or_guard_9) { mark_by_guard(c, f_088717E8, 0x08871534u, 3, 0x08871558u); }
SFA_FUNC(08871564, mark_4_or_guard_10) { mark_by_guard(c, f_08871808, 0x08871590u, 4, 0x088715B4u); }
SFA_FUNC(08871678, mark_6_or_guard_9) { mark_by_guard(c, f_088717E8, 0x088716A4u, 6, 0x088716C8u); }
SFA_FUNC(088716D4, mark_7_or_guard_10) { mark_by_guard(c, f_08871808, 0x08871700u, 7, 0x08871724u); }

/* ---------------------------------------------------------------- network */
/* (flag): the record at 0x095236C0's +12: 1 with the flag clear; else 3 when 0x088D59A0 (0x09523980)
   says so, 2 when not. */
SFA_FUNC(088E0C6C, net_state_12) {
    uint32_t flag = ARG(0) & 0xFF;
    gaddr r = 0x095236C0u;
    WITH_S0(r);
    if (flag == 0) { ARG(0) = 1; *B(r + 12) = 1; }
    else {
        ARG(0) = 0x09523980u;
        c->r[31] = 0x088E0C94u; f_088D59A0(c);
        ARG(0) = c->r[2] ? 3 : 2;
        *B(r + 12) = (uint8_t)ARG(0);
    }
    END_S0();
}
