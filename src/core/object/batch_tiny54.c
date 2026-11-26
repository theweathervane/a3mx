/* Small functions the scenarios reach, 47 - 48 instructions: fighter objects' starts, a menu's step
   with its sound and dispatch, effects counting down, an effect following its owner's level. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886C8BC(Cpu *c); void f_0886DBE8(Cpu *c); void f_0886DB94(Cpu *c);
void f_08902314(Cpu *c); void f_088AC488(Cpu *c); void f_0880AEA8(Cpu *c); void f_08823338(Cpu *c);
void f_08821424(Cpu *c); void f_088768F0(Cpu *c); void f_0891C6C4(Cpu *c); void f_0891CCEC(Cpu *c);
void f_088219B0(Cpu *c); void f_0891E940(Cpu *c); void f_0882196C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): with +386 negative 0x0886C8BC must allow it, else 0x0886BF84; then 0x08902314 and on
   (+4 = 1, +6 = 7, +235 = 1, +236 = 5; +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(08901DF4, object_start_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 386) >= 0) call_on(c, o, 0x08901E90u, f_0886BF84);
    else call_on(c, o, 0x08901E1Cu, f_0886C8BC);
    if (c->r[2] != 0) {
        c->r[17] = 1;
        call_on(c, o, 0x08901E30u, f_08902314);
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = 5;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x08901E68u, f_0886DBE8);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with few effects (none, or under two with +251) and 0x0886BF84 allowing it, on (+4 = 1, +6 =
   7, +235 = 1, +236 = k; +11 = +697) and its start (0x0886DB94 / 0x0886DBE8); 1 then, else 0. */
static void start_few(Cpu *c, uint8_t k, void (*fn)(Cpu *), uint32_t back0, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int ok = 0;
    if (S8(o + 251) != 0) {
        if ((int32_t)n < 2) { c->r[31] = back1; f_0886BF84(c); ok = c->r[2] != 0; }
    } else if (n == 0) { c->r[31] = back0; f_0886BF84(c); ok = c->r[2] != 0; }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = k;
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[17] = 1;
        *B(o + 235) = 1;
        call_on(c, o, back2, fn);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08926250, object_start_few_2) { start_few(c, 2, f_0886DBE8, 0x08926280u, 0x089262ECu, 0x089262C0u); }
SFA_FUNC(08962FE0, object_start_few_a2) { start_few(c, 0xA, f_0886DB94, 0x08963010u, 0x0896307Cu, 0x08963050u); }

/* (menu): with a press (GAME +16's high byte), 0x088AC488 (0, 4, 0, 1) and 0x0880AEA8 (2, 0x08A34D18,
   0, 0x20, 0x10), and a pending +96 = +95 move needs sound 7 / 0xD (0x08823338) to go on (+95 on);
   without one +96 catches up with +95. Then its +17 step (the table at 0x08A2C730). */
SFA_FUNC(088AB6BC, menu_step_sound) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    *W(c->r[29] + 36) = ra;
    int32_t press = (int32_t)((uint32_t)S16(GAME + 16) & 0xFF00) >> 8;
    int go = 1;
    if (press == 0) *B(o + 96) = *B(o + 95);
    else {
        ARG(0) = 0; ARG(1) = 4; ARG(2) = 0; ARG(3) = 1;
        c->r[31] = 0x088AB6F8u; f_088AC488(c);
        ARG(0) = 2; ARG(1) = 0x08A34D18u; ARG(2) = 0; ARG(3) = 0x20; c->r[8] = 0x10;
        c->r[31] = 0x088AB714u; f_0880AEA8(c);
        if (*B(o + 96) == *B(o + 95)) {
            call2(c, 7, 0xD, 0x088AB72Cu, f_08823338);
            if (c->r[2] == 0) go = 0;
            else *B(o + 95) = (uint8_t)(*B(o + 95) + 1);
        }
    }
    if (go) {
        uint32_t h = *W(0x08A2C730u + 4 * *B(o + 17));
        ARG(0) = o; c->r[31] = 0x088AB76Cu; rt_call_indirect(c, h);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (effect): unless halted (BATTLE +37 / +78): its first frame sets a 2-frame count (+82) and
   0x0891C6C4; then when the count runs out it is restarted (+0 / +4 / +6 = 1, 0x0891CCEC), and
   0x088768F0 each frame. Always drawn. */
SFA_FUNC(0891C918, effect_countdown_restart) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) {
        uint32_t st = *B(o + 6);
        if (st == 0) {
            *B(o + 6) = 1;
            *B(o + 82) = 2;
            ARG(1) = o + 0x52;
            call_on(c, o, 0x0891C970u, f_0891C6C4);
        } else if (st == 1) {
            uint32_t v = (uint32_t)(S8(o + 82) - 1);
            *B(o + 82) = (uint8_t)v;
            ARG(1) = v;
            if ((int8_t)v == 0) {
                *B(o) = 1; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 1; *B(o + 7) = 0;
                call_on(c, o, 0x0891C9B8u, f_0891CCEC);
            }
            call_on(c, o, 0x0891C9C0u, f_088768F0);
        }
    }
    call_on(c, o, 0x0891C9C8u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): step 0 counts +82 down to its next step (count 8) with 0x0891E940 each frame; step 1
   counts it down to on (+4), flickering (+123 ^ 1) on odd counts; 0x088219B0 in both. */
SFA_FUNC(0891E71C, effect_count_flicker) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    uint32_t st = *B(o + 6);
    if (st == 0) {
        int16_t h = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)h;
        if (h < 0) { *B(o + 6) = 1; *H(o + 82) = 8; }
        call_on(c, o, 0x0891E768u, f_0891E940);
        call_on(c, o, 0x0891E770u, f_088219B0);
    } else if (st == 1) {
        int16_t h = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)h;
        if (h < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        if (S16(o + 82) & 1) *B(o + 123) = (uint8_t)(S8(o + 123) ^ 1);
        call_on(c, o, 0x0891E7CCu, f_088219B0);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): off (+1) unless its owner (+88) is its character (+338 = +3) with a level (+296's +10):
   then at the owner's place and side and on; a new level (+78) takes its animation from 0x08A6F05C,
   else 0x088219B0. No level clears +78. */
SFA_FUNC(0893B508, effect_owner_level) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = ra;
    if ((uint32_t)S8(p + 338) == *B(o + 3)) {
        uint32_t lv = *B(*W(p + 296) + 10);
        if (lv == 0) *B(o + 78) = 0;
        else {
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 11) = *B(p + 11);
            *B(o + 12) = (uint8_t)S8(p + 12);
            *B(o + 1) = 1;
            *B(o + 13) = *B(p + 13);
            lv = *B(*W(p + 296) + 10);
            if ((uint32_t)S8(o + 78) != lv) {
                *B(o + 78) = (uint8_t)lv;
                call2(c, o, *W(0x08A6F05Cu + 4 * lv), 0x0893B5B0u, f_0882196C);
            } else call_on(c, o, 0x0893B584u, f_088219B0);
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
