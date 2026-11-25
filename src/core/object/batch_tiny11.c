/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 9 - 22
   instructions, ready once batch_tiny10.c was done: steps that run only while a condition holds (else
   another routine), a fighter's single colour into a palette slot, moves with stops, nudges by held
   directions, a network record's restart. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_089BE6C4(Cpu *c); void f_089C4760(Cpu *c); void f_089DA8E8(Cpu *c); void f_088E3F44(Cpu *c);
void f_089BE2DC(Cpu *c); void f_089BE330(Cpu *c); void f_0880AEA8(Cpu *c); void f_088219B0(Cpu *c);
void f_0887DF88(Cpu *c); void f_0889DB2C(Cpu *c); void f_088215C4(Cpu *c); void f_089885CC(Cpu *c);
void f_088D3C88(Cpu *c);

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

/* ---------------------------------------------------------------- steps with a condition */
/* (object): while GAME +40 is clear, its step by +4 from the table; otherwise 0x0889DB2C (GAME +40). */
static void step_unless_game_40(Cpu *c, gaddr table, uint32_t back_other, uint32_t back_step) {
    int8_t v = S8(GAME + 40);
    if (v != 0) { ARG(1) = (uint32_t)(int32_t)v; WRAP20(back_other, f_0889DB2C); return; }
    run_step(c, *B(ARG(0) + 4), table, 5, back_step);
}
SFA_FUNC(0889D3D8, step_unless_game_40_a) { step_unless_game_40(c, 0x08A2ABF4u, 0x0889D3FCu, 0x0889D424u); }
SFA_FUNC(0889D63C, step_unless_game_40_b) { step_unless_game_40(c, 0x08A2AC4Cu, 0x0889D660u, 0x0889D688u); }
SFA_FUNC(0889D760, step_unless_game_40_c) { step_unless_game_40(c, 0x08A2AC5Cu, 0x0889D784u, 0x0889D7ACu); }
SFA_FUNC(0889D96C, step_unless_game_40_d) { step_unless_game_40(c, 0x08A2ACD0u, 0x0889D990u, 0x0889D9B8u); }
/* (object): step 2 (+4) while BATTLE +42 is set; otherwise its step by +3 (0x08A584F4). */
SFA_FUNC(0890D718, step_by_3_unless_battle_42) {
    if (S8(BATTLE + 42) != 0) { ARG(1) = 2; *B(ARG(0) + 4) = 2; return; }
    run_step(c, *B(ARG(0) + 3), 0x08A584F4u, 5, 0x0890D764u);
}
/* (object): its step by +4 (0x08A9480C) while its +3 is its owner's (+88) character (+338);
   otherwise 0x089885CC (+4 on, its routine). */
SFA_FUNC(08988120, step_while_owner_character) {
    gaddr o = ARG(0);
    int8_t ch = S8(*W(o + 88) + 338);
    uint8_t mine = *B(o + 3);
    if (mine == (uint8_t)ch && (int32_t)mine == ch) { run_step(c, *B(o + 4), 0x08A9480Cu, 5, 0x0898816Cu); return; }
    ARG(1) = (uint32_t)(int32_t)ch; ARG(2) = mine;
    WRAP20(0x08988144u, f_089885CC);
}
/* (object): its step by +4 while its +139 is its owner's (+88); otherwise 0x088D3C88 (the owner's count). */
static void step_while_owner_139(Cpu *c, gaddr table, uint32_t back_other, uint32_t back_step) {
    gaddr o = ARG(0);
    int8_t theirs = S8(*W(o + 88) + 139), mine = S8(o + 139);
    if (theirs == mine) { run_step(c, *B(o + 4), table, 5, back_step); return; }
    ARG(1) = (uint32_t)(int32_t)theirs; ARG(2) = (uint32_t)(int32_t)mine;
    WRAP20(back_other, f_088D3C88);
}
SFA_FUNC(089C584C, step_while_owner_139_a) { step_while_owner_139(c, 0x08AB1888u, 0x089C5870u, 0x089C5898u); }
SFA_FUNC(089CB86C, step_while_owner_139_b) { step_while_owner_139(c, 0x08AB602Cu, 0x089CB890u, 0x089CB8B8u); }

/* ---------------------------------------------------------------- wrappers */
/* (object): +4 / +5 on, then 0x089BE6C4 / 0x089C4760 (unlinked and freed) / 0x089DA8E8. */
SFA_FUNC(089BE6A0, step_on_4_unlink_a) { ARG(1) = inc8(ARG(0) + 4); WRAP20(0x089BE6B8u, f_089BE6C4); }
SFA_FUNC(089C473C, step_on_4_unlink_b) { ARG(1) = inc8(ARG(0) + 4); WRAP20(0x089C4754u, f_089C4760); }
SFA_FUNC(089DA8C4, step_on_5_call_089DA8E8) { ARG(2) = inc8(ARG(0) + 5); WRAP20(0x089DA8DCu, f_089DA8E8); }
/* (owner): two new linked objects (0x089BE2DC, 0x089BE330). */
SFA_FUNC(089BE2B0, new_objects_30_31) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x089BE2C4u, f_089BE2DC);
    call_on(c, o, 0x089BE2CCu, f_089BE330);
    END_S0();
}
/* (object): +0x52 counted down; below 0, +5 on and 0x0887DF88 (object, 0x14); otherwise animate. */
SFA_FUNC(0887F448, count_52_then_0887DF88) {
    gaddr o = ARG(0), p = o + 0x52;
    int32_t n = S8(p) - 1;
    *B(p) = (uint8_t)n;
    ARG(1) = (uint32_t)n & 0x80;
    if (n & 0x80) { ARG(2) = inc8(o + 5); ARG(1) = 0x14; WRAP20(0x0887F480u, f_0887DF88); return; }
    WRAP20(0x0887F494u, f_088219B0);
}
/* (object): +82 counted down; at 0, +140 flips, +82 = 3 and +13 = +139 + +140 (blinking); then animate. */
SFA_FUNC(08924784, blink_13_then_animate) {
    gaddr o = ARG(0);
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) {
        int8_t b139 = S8(o + 139);
        *B(o + 140) = (uint8_t)(S8(o + 140) ^ 1);
        *H(o + 82) = 3;
        *B(o + 13) = (uint8_t)(b139 + S8(o + 140));
    }
    WRAP20(0x089247D0u, f_088219B0);
}
/* (object): at its owner's (+88) x; step 2 when the owner's +0 is 0, else drawn unless BATTLE +36. */
SFA_FUNC(0897193C, follow_owner_x_draw) {
    gaddr o = ARG(0), w = *W(o + 88);
    *H(o + 18) = (uint16_t)S16(w + 18);
    if (*B(w) == 0) { ARG(1) = 2; *B(o + 4) = 2; return; }
    if (S8(BATTLE + 36) != 0) return;
    WRAP20(0x08971978u, f_088215C4);
}

/* ---------------------------------------------------------------- palettes and moves */
/* (fighter, slot, colour): one palette of colour 0 transparent and 15 times `colour`, built on the
   stack, into palette slot +13 + slot (0x0880AEA8). */
SFA_FUNC(08870140, palette_one_colour) {
    gaddr f = ARG(0);
    uint32_t slot = ARG(1) & 0xFF, colour = ARG(2) & 0xFFFF;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *H(sp + 32) = 0;
    for (int i = 1; i < 16; i++) *H(sp + 32 + 2 * i) = (uint16_t)colour;
    *W(sp + 64) = ra;
    c->r[9] = 0;
    ARG(3) = *B(f + 13) + slot;
    ARG(1) = sp + 0x20;
    ARG(0) = 0; ARG(2) = 0; c->r[8] = 1;
    c->r[31] = 0x0887018Cu; f_0880AEA8(c);
    c->r[31] = ra;
    c->r[29] += 0x50;
}
/* (object): x the way it faces (stopped once the speed goes negative), then y; the new y speed back. */
SFA_FUNC(08904A20, move_x_stop_then_y) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) += *B(o + 11) == 0 ? 0u - v : v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if ((int32_t)nv < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    uint32_t nvy = vy + *W(o + 108);
    *W(o + 100) = nvy;
    RETURN(nvy);
}
/* (object): moved on both axes; 1 while +132 is not above +22. */
SFA_FUNC(0890DA50, move_xy_not_below_132) {
    gaddr o = ARG(0);
    uint32_t vx = *W(o + 96), vy = *W(o + 100);
    *W(o + 16) += vx;
    *W(o + 20) += vy;
    *W(o + 96) = vx + *W(o + 104);
    *W(o + 100) = vy + *W(o + 108);
    RETURN(!(S16(o + 132) < S16(o + 22)));
}
/* (fighter): for a human (+373 clear) holding left / right (+290 bits 0 / 1), x nudged by 3 that way
   (mirrored by facing). */
SFA_FUNC(08921988, nudge_3_by_held) {
    gaddr f = ARG(0);
    if (S8(f + 373) != 0) return;
    uint32_t k = *H(f + 290);
    if ((k & 3) == 0) return;
    int32_t d = (k & 2) ? -3 : 3;
    if (*B(f + 11) != 0) d = (int16_t)-d;
    *H(f + 18) = (uint16_t)(S16(f + 18) + d);
}
/* (object, fighter, which): the fighter's +311 = 2, +312 = 8, +260 from 0x08A6A63C (or 0x08A6A642
   when `which` is set) by the object's +194. */
SFA_FUNC(089291EC, set_260_by_194_which) {
    gaddr o = ARG(0), f = ARG(1);
    *B(f + 311) = 2; *B(f + 312) = 8;
    uint32_t i = (uint32_t)(int32_t)S8(o + 194) * 2;
    *B(f + 260) = (uint8_t)S16(((int8_t)ARG(2) != 0 ? 0x08A6A642u : 0x08A6A63Cu) + i);
}
/* (fighter): for the CPU, the low byte of +100 cleared and 1 if its partner's (+92) +73 is set; for
   a human, 1 when a kick is held (+752 bits 7 - 9) and no punch (bits 4 - 6). */
SFA_FUNC(089B2188, kick_held_or_partner_73) {
    gaddr f = ARG(0);
    if (S8(f + 373) != 0) {
        *W(f + 100) &= 0xFFFFFF00u;
        RETURN(S8(*W(f + 92) + 73) != 0);
        return;
    }
    uint32_t k = *H(f + 752);
    RETURN(!(k & 0x70) && (k & 0x380));
}

/* ---------------------------------------------------------------- network */
/* (record): its count +0: 1 sets it going again (+20 = 0x08A3EABC), 2 or more restart it at 0x08A3EB14,
   0 waits for 0x088E3F44 and counts on. Restarting clears +0, +2, +24 and +47. */
SFA_FUNC(088D6E74, net_record_count) {
    gaddr r = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = r;
    uint16_t v = *H(r);
    if (v == 0) {
        c->r[31] = 0x088D6ED4u; f_088E3F44(c);
        if (c->r[2] != 0) *H(r) = (uint16_t)(*H(r) + 1);
    } else {
        *W(r + 20) = v == 1 ? 0x08A3EABCu : 0x08A3EB14u;
        *H(r + 24) = 0; *H(r + 2) = 0; *H(r) = 0; *B(r + 47) = 0;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}
