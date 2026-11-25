/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 19 - 25
   instructions, ready once batch_tiny12.c was done: the fighters' extra pictures each frame, steps
   with two tables, the CPU's script choices, set-ups and moves. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0896A34C(Cpu *c); void f_08822F54(Cpu *c); void f_0880D68C(Cpu *c); void f_0880A39C(Cpu *c);
void f_08821FC8(Cpu *c); void f_08896DE8(Cpu *c); void f_088967C4(Cpu *c); void f_08896808(Cpu *c);
void f_08869FEC(Cpu *c); void f_088219B0(Cpu *c); void f_08821424(Cpu *c); void f_0884FED0(Cpu *c);
void f_0882196C(Cpu *c); void f_08858460(Cpu *c); void f_08858418(Cpu *c); void f_088215C4(Cpu *c);
void f_0880AEA8(Cpu *c); void f_0887B6B8(Cpu *c); void f_0887B7CC(Cpu *c); void f_0887B1A8(Cpu *c);

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

/* ---------------------------------------------------------------- frames */
/* Each frame, unless BATTLE +36 or the word at 0x08E7602C is set: the four fighters' extra pictures
   (0x0880D68C). */
SFA_FUNC(0880D62C, fighters_extra_pictures) {
    if ((S8(BATTLE + 36) | *W(0x08E7602Cu)) != 0) { ARG(0) = 0x092E0000u; return; }
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20; *W(c->r[29] + 16) = ra;
    static const uint32_t fighters[4] = { 0x092E3EA0u, 0x092E4338u, 0x092E47D0u, 0x092E4C68u };
    static const uint32_t backs[4] = { 0x0880D65Cu, 0x0880D668u, 0x0880D674u, 0x0880D680u };
    for (int k = 0; k < 4; k++) call_on(c, fighters[k], backs[k], f_0880D68C);
    c->r[31] = ra; c->r[29] += 0x20;
}
/* (record, x): its +6 on, then 0x0880A39C, 0x08821FC8, 0x08896DE8 for fighters 1 and 2, 0x088967C4
   and 0x08896808 (x). */
SFA_FUNC(08893D94, record_frame_calls) {
    gaddr r = ARG(0);
    uint32_t x = ARG(1);
    WITH_S0(x);
    ARG(1) = (uint16_t)(*H(r + 6) + 1);
    *H(r + 6) = (uint16_t)ARG(1);
    c->r[31] = 0x08893DB4u; f_0880A39C(c);
    c->r[31] = 0x08893DBCu; f_08821FC8(c);
    call_on(c, 0x092E3EA0u, 0x08893DC8u, f_08896DE8);
    call_on(c, 0x092E4338u, 0x08893DD4u, f_08896DE8);
    c->r[31] = 0x08893DDCu; f_088967C4(c);
    call_on(c, x, 0x08893DE4u, f_08896808);
    END_S0();
}

/* ---------------------------------------------------------------- steps from one of two tables */
SFA_FUNC(088F1BB0, step_5_by_3) {
    if (*B(ARG(0) + 3) != 0) run_step(c, *B(ARG(0) + 5), 0x08A43CA8u, 5, 0x088F1C04u);
    else run_step(c, *B(ARG(0) + 5), 0x08A43CA0u, 5, 0x088F1BE0u);
}
SFA_FUNC(08921E20, step_7_by_430) {
    if (S8(ARG(0) + 430) != 0) run_step(c, *B(ARG(0) + 7), 0x08A66DCCu, 5, 0x08921E74u);
    else run_step(c, *B(ARG(0) + 7), 0x08A66DBCu, 5, 0x08921E50u);
}
/* (fighter): its step by +236 from the CPU's table or the human's. */
SFA_FUNC(0899F354, step_236_by_cpu) {
    uint32_t n = (uint32_t)S8(ARG(0) + 236);
    if (S8(ARG(0) + 373) != 0) run_step(c, n, 0x08A9BF0Cu, 5, 0x0899F3A8u);
    else run_step(c, n, 0x08A9BEE4u, 5, 0x0899F384u);
}

/* ---------------------------------------------------------------- the CPU's scripts */
/* (fighter): 0x0887B7CC when 0x0887B6B8 says so; else its first script and command cleared when +6 is
   0 or the fighter is guarding (+696) / when +536 is 0 or +228 is clear. */
SFA_FUNC(0887A4D4, cpu_script_by_guard) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887A4E8u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887A4F8u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887A510u, f_0887B1A8);
    else if (S8(f + 696) != 0) call_on(c, f, 0x0887A528u, f_0887B1A8);
    END_S0();
}
SFA_FUNC(0887A84C, cpu_script_by_536) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887A860u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887A870u, f_0887B7CC);
    else if (*B(f + 536) == 0) call_on(c, f, 0x0887A888u, f_0887B1A8);
    else if (*W(f + 228) == 0) call_on(c, f, 0x0887A8A0u, f_0887B1A8);
    END_S0();
}
/* (fighter): the move's strength (+194) and +193 cleared; unless all three punches were pressed or
   released, +193 = 1 when 0x08869FEC says no. */
SFA_FUNC(089633BC, punch_strength_reset) {
    gaddr f = ARG(0);
    uint32_t k = (*H(f + 760) | *H(f + 762)) & 0xFFFF & 0x70;
    *B(f + 193) = 0; *B(f + 194) = 0;
    WITH_S0(f);
    if (k != 0x70) {
        call_on(c, f, 0x089633F8u, f_08869FEC);
        if (c->r[2] == 0) *B(f + 193) = (uint8_t)(S8(f + 193) + 1);
    }
    END_S0();
}

/* ---------------------------------------------------------------- objects */
/* (owner): a new object (0x08822F54: list 0x37, 0) with the owner at +88, +139 and +14 the owner's,
   +0 = 1, +3 = 2; the owner's +48 = it. */
SFA_FUNC(089CC810, new_object_37_kind_2) {
    gaddr owner = ARG(0); WITH_S0(owner);
    ARG(0) = 0x37; ARG(1) = 0;
    c->r[31] = 0x089CC82Cu; f_08822F54(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = owner;
        *B(n + 139) = (uint8_t)S8(owner + 139);
        ARG(1) = 1;
        *B(n) = 1;
        *B(n + 3) = 2;
        ARG(0) = *B(owner + 14);
        *W(owner + 48) = n;
        *B(n + 14) = (uint8_t)ARG(0);
    }
    END_S0();
}
/* (object): with an x speed, the speed changed; once it goes negative it stops (speed and change 0),
   otherwise 0x0896A34C (moved). */
SFA_FUNC(08969098, slow_then_move) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    if (v == 0) return;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if ((int32_t)nv < 0) { *W(o + 96) = 0; *W(o + 104) = 0; return; }
    ARG(1) = nv;
    WRAP20(0x089690C4u, f_0896A34C);
}
/* (object): speeds and their changes from the 16-byte row by +3 of 0x08A5853C, x mirrored by facing. */
SFA_FUNC(0890DB44, speeds_by_3) {
    gaddr o = ARG(0), e = 0x08A5853Cu + (uint32_t)*B(o + 3) * 16;
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
    *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    if (*B(o + 11) != 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
}
/* (object): with an x speed, x moved the way it faces (the speed stopping at 0); then y. */
SFA_FUNC(089376E4, move_x_if_speed_then_y) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    if (v != 0) {
        *W(o + 16) += *B(o + 11) != 0 ? v : 0u - v;
        uint32_t nv = v + *W(o + 104);
        *W(o + 96) = nv;
        if ((int32_t)nv < 0) *W(o + 96) = 0;
    }
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    *W(o + 100) = vy + *W(o + 108);
}
/* (object): x the way it faces (stopped when it goes negative), then y; 1 while rising (y speed negative). */
SFA_FUNC(0894CAF4, move_x_stop_y_rising) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) != 0 ? v : 0u - v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if ((int32_t)nv < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    uint32_t nvy = vy + *W(o + 108);
    *W(o + 100) = nvy;
    RETURN((int32_t)nvy < 0);
}
/* (object): at its owner's (+88) place, raised by 0x40 (0x80 with +3 set) and moved 0x6C (0x4B) back
   from the way it faces. */
SFA_FUNC(0894113C, at_owner_raised) {
    gaddr o = ARG(0), w = *W(o + 88);
    *W(o + 16) = *W(w + 16);
    *W(o + 20) = *W(w + 20);
    int32_t y = S16(o + 22), dx;
    if (*B(o + 3) != 0) { dx = -0x4B; y += 0x80; } else { dx = -0x6C; y += 0x40; }
    if (*B(o + 11) != 0) dx = (int16_t)-dx;
    *H(o + 22) = (uint16_t)y;
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
}
/* (object): +4 on, set up from its owner (+24, +3 character, +12 - +14, +11, side +337 at +86, +164),
   +78 cleared. */
SFA_FUNC(08971E1C, step_on_4_from_owner_all) {
    gaddr o = ARG(0), w = *W(o + 88);
    inc8(o + 4);
    *H(o + 24) = *H(w + 24);
    *B(o + 3) = (uint8_t)S8(w + 338);
    *B(o + 12) = (uint8_t)S8(w + 12);
    *B(o + 13) = *B(w + 13);
    uint8_t b14 = *B(w + 14);
    *B(o + 78) = 0;
    *B(o + 14) = b14;
    *B(o + 11) = *B(w + 11);
    *H(o + 86) = (uint16_t)(int16_t)S8(w + 337);
    *W(o + 164) = *W(w + 164);
}
/* (object): +4 on when +127 is its owner's character and +75 is negative; otherwise animate and 0x08821424. */
SFA_FUNC(089819FC, step_on_or_animate_run) {
    gaddr o = ARG(0);
    int8_t ch = S8(*W(o + 88) + 338);
    WITH_S0(o);
    if (S8(o + 127) == ch && (S8(o + 75) & 0x80)) inc8(o + 4);
    else { call_on(c, o, 0x08981A44u, f_088219B0); call_on(c, o, 0x08981A4Cu, f_08821424); }
    END_S0();
}
/* (object): step 3 (+5), at (0x40.0, 0xA1.0) with speeds (0.625, 0.0703125); 0x0884FED0 (nothing), then
   the animation at 0x08A153B0. */
SFA_FUNC(0884FE08, setup_at_40_A1) {
    gaddr o = ARG(0); WITH_S0(o);
    *B(o + 5) = 3;
    *W(o + 16) = 0x400000u; *W(o + 20) = 0xA10000u;
    *W(o + 96) = 0xA000u; *W(o + 100) = 0x1200u;
    ARG(1) = 0xA000u;
    call_on(c, o, 0x0884FE48u, f_0884FED0);
    ARG(1) = *W(0x08A15388u + 40);
    call_on(c, o, 0x0884FE5Cu, f_0882196C);
    END_S0();
}
/* (object): 0x08858460 (animate); when +75 is set, +7 on, +139 = 0x3C and 0x08858418 (object, +6); then draw. */
SFA_FUNC(08857DC0, animate_then_08858418) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x08857DD4u, f_08858460);
    if (S8(o + 75) != 0) {
        inc8(o + 7);
        ARG(1) = (uint32_t)(int32_t)(int16_t)*B(o + 6);
        ARG(2) = 0x3C;
        *B(o + 139) = 0x3C;
        call_on(c, o, 0x08857E0Cu, f_08858418);
    }
    call_on(c, o, 0x08857E14u, f_088215C4);
    END_S0();
}
/* (object): animate unless its frame (+28) holds (+1 bit 6: then +75 = -1); whether +75 is negative
   back (as -128 or 0). */
SFA_FUNC(088D3B88, animate_or_hold) {
    gaddr o = ARG(0); WITH_S0(o);
    if (S8(*W(o + 28) + 1) & 0x40) *B(o + 75) = 0xFF;
    else call_on(c, o, 0x088D3BB4u, f_088219B0);
    RETURN((int32_t)(int8_t)(S8(o + 75) & 0x80));
    END_S0();
}
/* (fighter, n): unless +657 is n, its palette (routine table +132) for n + 5 x its colour (+376) - 24
   into its slot (+13). */
SFA_FUNC(0886F8A0, palette_by_n_colour) {
    gaddr f = ARG(0);
    int32_t n = (int8_t)ARG(1);
    uint32_t base = *W(*W(f + 416) + 132);
    ARG(1) = base;
    if (S8(f + 657) == n) return;
    uint32_t i = (uint32_t)(n + S8(f + 376) * 5 - 24) & 0xFFFF;
    ARG(1) = base + (i << 5);
    ARG(3) = *B(f + 13);
    ARG(0) = 0; ARG(2) = 0; c->r[8] = 1;
    wrap(c, 0x30, 32, 0x0886F8F8u, f_0880AEA8);
}
