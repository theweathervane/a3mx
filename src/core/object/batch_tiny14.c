/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 15 - 26
   instructions, ready once batch_tiny13.c was done: a move started when 0x0886BF84 allows it, objects
   set up from their fighter, wobbling speeds, a character's range check, steps held by BATTLE +42.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088D3B88(Cpu *c); void f_0890DB44(Cpu *c); void f_0890DAA8(Cpu *c); void f_08869FEC(Cpu *c);
void f_0886BF84(Cpu *c); void f_089439DC(Cpu *c); void f_0896F658(Cpu *c); void f_088768F0(Cpu *c);
void f_0880AEA8(Cpu *c); void f_0884FED0(Cpu *c); void f_0882196C(Cpu *c); void f_089CCEC0(Cpu *c);
void f_08821424(Cpu *c); void f_0886DB94(Cpu *c);

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
/* s0 and s1 kept at +16 / +20 of a 0x20 frame, the return address at +24 */
#define WITH_S01(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; c->r[16] = (o)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- moves started when allowed */
/* (fighter): when 0x0886BF84 allows it, step 1.0.7.0 and 0x089439DC (fighter, 4 / 5); 1 if started. */
static void start_7_with(Cpu *c, uint32_t kind, uint32_t back1, uint32_t back2) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, back1, f_0886BF84);
    ARG(0) = 1;
    uint32_t started = c->r[2] != 0;
    if (started) {
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
        ARG(1) = kind;
        call_on(c, f, back2, f_089439DC);
    }
    RETURN(started);
    END_S0();
}
SFA_FUNC(0894371C, start_7_kind_4) { start_7_with(c, 4, 0x08943730u, 0x08943758u); }
SFA_FUNC(08943780, start_7_kind_5) { start_7_with(c, 5, 0x08943794u, 0x089437BCu); }
/* (fighter): when 0x0886BF84 allows it, step 1.0.7.0 facing the opponent (+697), +235 / +236 cleared;
   1 if started. */
SFA_FUNC(089842B8, start_7_facing) {
    gaddr f = ARG(0); WITH_S01(f);
    c->r[17] = 0;
    call_on(c, f, 0x089842D4u, f_0886BF84);
    ARG(0) = 1;
    if (c->r[2] != 0) {
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
        *B(f + 11) = (uint8_t)S8(f + 697);
        *B(f + 235) = 0; *B(f + 236) = 0;
        c->r[17] = 1;
    }
    RETURN(c->r[17]);
    END_S01();
}
/* (fighter): the same with +236 cleared, then the punch's strength (0x0886DB94); 1 if started. */
SFA_FUNC(08909820, start_7_punch) {
    gaddr f = ARG(0); WITH_S01(f);
    c->r[17] = 0;
    call_on(c, f, 0x0890983Cu, f_0886BF84);
    ARG(0) = 1;
    if (c->r[2] != 0) {
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
        *B(f + 236) = 0;
        *B(f + 11) = (uint8_t)S8(f + 697);
        c->r[17] = 1;
        call_on(c, f, 0x08909870u, f_0886DB94);
    }
    RETURN(c->r[17]);
    END_S01();
}
/* (fighter): strength (+194) and +235 cleared; unless all three punches were pressed or released and
   unless 0x08869FEC says so, +0x3BC on. */
SFA_FUNC(0893D900, punch_reset_3BC) {
    gaddr f = ARG(0);
    uint32_t k = (uint32_t)(int32_t)(int16_t)(*H(f + 760) | *H(f + 762)) & 0x70;
    *B(f + 194) = 0; *B(f + 235) = 0;
    WITH_S0(f);
    if (k != 0x70) {
        call_on(c, f, 0x0893D940u, f_08869FEC);
        ARG(0) = f + 0x3B8;
        if (c->r[2] == 0) *B(f + 0x3BC) = (uint8_t)(S8(f + 0x3BC) + 1);
    }
    END_S0();
}

/* ---------------------------------------------------------------- objects */
/* (object): 0x088D3B88 (animate or hold), then +4 on while +75 is negative. */
static void animate_then_step(Cpu *c, uint32_t back) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, back, f_088D3B88);
    if (S8(o + 75) < 0) inc8(o + 4);
    END_S0();
}
SFA_FUNC(089BE9C4, animate_then_step_a) { animate_then_step(c, 0x089BE9D8u); }
SFA_FUNC(089CB480, animate_then_step_b) { animate_then_step(c, 0x089CB494u); }
/* (object): +132 up 16, +82 = 0x18, then its speeds (0x0890DB44) and animation (0x0890DAA8). */
SFA_FUNC(0890D694, start_speeds_animation) {
    gaddr o = ARG(0); WITH_S0(o);
    *H(o + 132) = (uint16_t)(S16(o + 132) + 0x10);
    *H(o + 82) = 0x18;
    ARG(1) = 0x18;
    call_on(c, o, 0x0890D6C0u, f_0890DB44);
    call_on(c, o, 0x0890D6C8u, f_0890DAA8);
    END_S0();
}
/* (object): +0 = 2, +136 = 1; with its record (+296) +1 clear, +136 = -1 and +116 cleared; otherwise,
   when its +116 is no longer its list's (+224) +14: +0 = 1, and if +123 was set, +0 = 2, +123 cleared,
   +116 = it. */
SFA_FUNC(0891D374, follow_list_14) {
    gaddr o = ARG(0);
    *B(o) = 2;
    *B(o + 136) = 1;
    if (*B(*W(o + 296) + 1) == 0) { *B(o + 136) = 0xFF; *B(o + 116) = 0; return; }
    gaddr l = *W(o + 224);
    if (*B(l + 14) == *B(o + 116)) return;
    *B(o) = 1;
    if (S8(o + 123) == 0) return;
    *B(o) = 2;
    *B(o + 123) = 0;
    *B(o + 116) = *B(l + 14);
}
/* (object): the counts +0x5E / +0x5F down; each time one goes below 0 it restarts at 0x1F and the x /
   y change (+104 / +108) turns round (a wobble). */
SFA_FUNC(0891E97C, wobble) {
    gaddr o = ARG(0), p = o + 0x5C;
    int8_t a = (int8_t)(S8(p + 2) - 1);
    *B(p + 2) = (uint8_t)a;
    if (a < 0) { *B(p + 2) = 0x1F; *W(o + 104) = 0u - *W(o + 104); }
    int8_t b = (int8_t)(S8(p + 3) - 1);
    *B(p + 3) = (uint8_t)b;
    if (b < 0) { *B(p + 3) = 0x1F; *W(o + 108) = 0u - *W(o + 108); }
}
/* (object): 0x0896F658 (moved); on the ground (+132 not above +22): step 2, y = +132, +127 = 2;
   then 0x088768F0. */
SFA_FUNC(0896F9BC, move_land_step_2) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, 0x0896F9D0u, f_0896F658);
    int16_t g = S16(o + 132);
    ARG(1) = 2;
    if (!((int32_t)(uint16_t)g < (int32_t)(uint16_t)S16(o + 22))) {
        *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        *H(o + 22) = (uint16_t)g;
        *B(o + 127) = 2;
    }
    call_on(c, o, 0x0896FA10u, f_088768F0);
    END_S0();
}
/* (object): step 2 (+4 = 2, +5 - +7 cleared) while BATTLE +42 is set; otherwise its step by +5
   (0x08A9481C). */
SFA_FUNC(0898825C, step_by_5_unless_battle_42) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 42) != 0) { ARG(1) = 2; *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; return; }
    run_step(c, *B(o + 5), 0x08A9481Cu, 5, 0x089882B4u);
}
/* (object): its step by +3 (0x08A66F18) then 0x08821424 while +73 is its owner's character;
   otherwise step 2. */
SFA_FUNC(08924498, step_by_3_while_73) {
    gaddr o = ARG(0);
    if (S8(*W(o + 88) + 338) != S8(o + 73)) { *B(o + 4) = 2; return; }
    run_step_then(c, o, *B(o + 3), 0x08A66F18u, 0x089244D8u, f_08821424, NULL);
}
/* (object): with kind +3 = 2, the palette at 0x08ACCD34 into slot 0xF first; then its step by +4
   (0x08A133E0). */
SFA_FUNC(08847C90, palette_then_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    if (*B(o + 3) == 2) {
        ARG(0) = 2; ARG(1) = 0x08ACCD34u; ARG(2) = 0; ARG(3) = 0xF; c->r[8] = 1;
        c->r[31] = 0x08847CC8u; f_0880AEA8(c);
    }
    gaddr h = *W(0x08A133E0u + (uint32_t)*B(o + 4) * 4);
    ARG(0) = o; ARG(1) = h;
    c->r[31] = 0x08847CE8u;
    rt_call_indirect(c, h);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}
/* (object): step 1 (+5), at (0x300.0, 0xF0.0) with speeds (-0.625, -0.0703125); 0x0884FED0, then the
   animation at 0x08A153AC. */
SFA_FUNC(0884FD5C, setup_at_300_F0) {
    gaddr o = ARG(0); WITH_S0(o);
    *B(o + 5) = 1;
    *W(o + 16) = 0x3000000u; *W(o + 20) = 0xF00000u;
    *W(o + 96) = 0xFFFF6000u; *W(o + 100) = 0xFFFFEE00u;
    ARG(1) = 0xFFFFEE00u;
    call_on(c, o, 0x0884FDA0u, f_0884FED0);
    ARG(1) = *W(0x08A15388u + 36);
    call_on(c, o, 0x0884FDB4u, f_0882196C);
    END_S0();
}
/* (fighter, object, kind): the object set up from the fighter (+0 = 1, +3 kind, owner +88, side +337
   at +78, +12, +13, +86, +164, +148 cleared); for INGRID (0x29), 0x089CCEC0 (its linked object). */
SFA_FUNC(0887CC00, setup_from_fighter_kind) {
    gaddr f = ARG(0), o = ARG(1);
    *B(o) = 1;
    *B(o + 3) = (uint8_t)ARG(2);
    *W(o + 88) = f;
    *B(o + 78) = (uint8_t)S8(f + 337);
    *B(o + 12) = (uint8_t)S8(f + 12);
    *B(o + 13) = *B(f + 13);
    *H(o + 86) = *H(f + 86);
    uint32_t w164 = *W(f + 164);
    *W(o + 148) = 0;
    *W(o + 164) = w164;
    int8_t ch = S8(f + 338);
    ARG(2) = (uint32_t)(int32_t)ch; ARG(3) = 0x29;
    ARG(0) = o;
    if (ch == 0x29) WRAP20(0x0887CC5Cu, f_089CCEC0);
}
/* (object): with +5 set: step 1, +0 = 1, +112 = 0xFF; otherwise +4 on once its owner's record
   (+88's +296) +10 is clear; then 0x08821424. */
SFA_FUNC(08904F84, restart_or_step_on) {
    gaddr o = ARG(0), w = *W(o + 88);
    ARG(1) = w;
    if (*B(o + 5) != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        *B(o) = 1;
        *H(o + 112) = 0xFF;
        ARG(1) = 1; ARG(2) = 0xFF;
    } else {
        ARG(1) = *B(*W(w + 296) + 10);
        if (ARG(1) == 0) ARG(1) = inc8(o + 4);
    }
    WRAP20(0x08904FE0u, f_08821424);
}

/* ---------------------------------------------------------------- leaves */
/* (a, fighter): for a living (+0) character 0x1B, its +0x33C = -1 while a is within 0x28 to the
   left / 0x28 to the right of it (else 0); then 1 if its record (+296) +11 is set. */
SFA_FUNC(0898155C, near_character_1B) {
    gaddr a = ARG(0), f = ARG(1);
    if (*B(f) == 0 || S8(f + 338) != 0x1B) { RETURN(0); return; }
    gaddr p = f + 0x334;
    *B(p + 8) = 0;
    uint32_t d = ((((uint32_t)(S16(a + 18) - S16(f + 18))) & 0xFFFF) + 0x28) & 0xFFFF;
    if (!((int32_t)d < 81)) { RETURN(0); return; }
    *B(p + 8) = 0xFF;
    RETURN(*B(*W(f + 296) + 11) != 0);
}
/* (fighter): +0x350 - +0x35C, +0x368 - +0x37C and +0x388 - +0x38C cleared (twelve words). */
SFA_FUNC(0898C5DC, clear_350_to_38C) {
    gaddr f = ARG(0);
    static const uint16_t at[12] = { 0x350, 0x354, 0x368, 0x36C, 0x358, 0x35C, 0x370, 0x374, 0x378, 0x37C, 0x388, 0x38C };
    for (int i = 0; i < 12; i++) *W(f + at[i]) = 0;
}
/* (object): moved on both axes; 1 when falling (new y speed negative) below +132. */
SFA_FUNC(08921850, move_xy_fell_below_132) {
    gaddr o = ARG(0);
    uint32_t vx = *W(o + 96), vy = *W(o + 100);
    *W(o + 16) += vx;
    *W(o + 96) = vx + *W(o + 104);
    *W(o + 20) += vy;
    uint32_t nvy = vy + *W(o + 108);
    *W(o + 100) = nvy;
    if ((int32_t)nvy >= 0) { RETURN(0); return; }
    RETURN((int32_t)(uint16_t)S16(o + 22) < (int32_t)(uint16_t)S16(o + 132));
}
