/* Sound-cue callers, 17 - 25 instructions: a fighter's super-gauge "ready" cue, menu steps that play a
   cue as they move on, and a random announcer cue. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C408(Cpu *c); void f_0887C4CC(Cpu *c); void f_0887C5B4(Cpu *c); void f_08821368(Cpu *c);

#define GAME 0x08E75910u

static void cue(Cpu *c, uint32_t n, uint32_t back) {
    ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0;
    c->r[31] = back; f_0887C4CC(c);
}

/* (attacker, fighter, gauge): unless held off (the attacker's +137, or a fighter in state 7 / 4 / 5 whose
   gauge must wait: +373, 0x09517A00, side bit 2 of +337, the fighter's +20), a live gauge (+0, +313
   clear) fills by 2 (+366); on reaching 144 it plays cue 0x31 from its side, and is held at 144. */
SFA_FUNC(08825908, gauge_fill) {
    gaddr a = ARG(0), f = ARG(1), g = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = g;
    int go = 1;
    if (S8(a + 137) == 0) {
        int32_t k = S8(f + 79);
        if (k == 7) {
            if (S8(g + 373) == 0 && *W(0x09517A00u) != 0) go = 0;
        } else if (k == 4 || k == 5) {
            if ((S8(g + 337) & 2) == 0 && S16(f + 20) != 0) go = 0;
        }
    }
    if (go && *B(g) != 0 && S8(g + 313) == 0) {
        int32_t v = (int16_t)(S16(g + 366) + 2);
        *H(g + 366) = (uint16_t)v;
        if (v >= 144) {
            if (S16(g + 366) != 0x90) *H(g + 366) = 0x90;
            else {
                ARG(1) = (uint32_t)(int32_t)S8(g + 337); ARG(2) = 0x31;
                c->r[31] = 0x088259D0u; f_0887C408(c);
            }
        }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (task): +5 steps on, +15 / +18 set, cue 0x3D. */
SFA_FUNC(088860D4, task_step_cue_3d) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *B(o + 15) = 1; *B(o + 18) = 1;
    *W(c->r[29] + 16) = ra;
    cue(c, 0x3D, 0x0888610Cu);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): cue `n`, or the next one when 0x0887C5B4 says so, from the object's side (+337). */
static void obj_cue_alt(Cpu *c, uint32_t n, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = n;
    c->r[31] = back1; f_0887C5B4(c);
    if (c->r[2] != 0) c->r[17] = n + 1;
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = c->r[17];
    c->r[31] = back2; f_0887C408(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0887C56C, obj_cue_24) { obj_cue_alt(c, 0x24, 0x0887C588u, 0x0887C5A0u); }
SFA_FUNC(0887C5CC, obj_cue_25) { obj_cue_alt(c, 0x25, 0x0887C5E8u, 0x0887C600u); }
SFA_FUNC(0887C614, obj_cue_22) { obj_cue_alt(c, 0x22, 0x0887C630u, 0x0887C648u); }

/* (task): the +16 timer counts down; when it runs out the step (+4) moves on, the timer restarts at 0x1F
   and cue 0x3D plays. */
static void timer_cue(Cpu *c, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t = (uint32_t)S16(o + 16) - 1;
    *W(c->r[29] + 16) = ra;
    *H(o + 16) = (uint16_t)t;
    if ((int16_t)t < 0) {
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        *H(o + 16) = 0x1F;
        cue(c, 0x3D, back);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0882B6C8, timer_cue_3d) { timer_cue(c, 0x0882B70Cu); }
SFA_FUNC(089DF008, timer_cue_3d_b) { timer_cue(c, 0x089DF04Cu); }

/* (task, item): as above, with the item's +81 set to -1 and cue 0x131. */
static void timer_cue_item(Cpu *c, uint32_t back) {
    gaddr o = ARG(0), it = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t = (uint32_t)S16(o + 16) - 1;
    *H(o + 16) = (uint16_t)t;
    *W(c->r[29] + 16) = ra;
    ARG(0) = it; ARG(1) = o;
    if ((int16_t)t < 0) {
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        *B(it + 81) = 0xFF;
        *H(o + 16) = 0x1F;
        cue(c, 0x131, back);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0882B718, timer_cue_131) { timer_cue_item(c, 0x0882B770u); }
SFA_FUNC(089DF058, timer_cue_131_b) { timer_cue_item(c, 0x089DF0B0u); }

/* (task): unless the game's +93 is set, cue 0x121, +12 = 0x1B0 and the step (+3) moves on. */
static void step_cue_121(Cpu *c, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (S8(GAME + 93) == 0) {
        cue(c, 0x121, back);
        *W(o + 12) = 0x1B0;
        *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088B35D4, cue_121_step) { step_cue_121(c, 0x088B3604u); }
SFA_FUNC(088B3B70, cue_121_step_b) { step_cue_121(c, 0x088B3BA0u); }

/* (task): the +12 count runs down; at 0, cue 0x121, +84 = 0x100 and the step (+9) moves on. */
SFA_FUNC(088AC298, count_cue_121) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    uint32_t n = (uint32_t)*B(o + 12) - 1;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 12) = (uint8_t)n;
    *W(c->r[29] + 20) = ra;
    ARG(0) = 0;
    if ((n & 0xFF) == 0) {
        cue(c, 0x121, 0x088AC2CCu);
        *W(o + 84) = 0x100;
        *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* With nothing pending (the game's +130), a random announcer cue 0x140 - 0x14F (0x08821368) that is not
   the last one (+132). */
SFA_FUNC(0887C680, announcer_cue_random) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = GAME;
    *W(c->r[29] + 20) = ra;
    if (S16(GAME + 130) == 0) {
        uint32_t n;
        do {
            c->r[31] = 0x0887C6A8u; f_08821368(c);
            n = (c->r[2] & 0xF) + 0x140;
            *H(GAME + 130) = 0;
        } while (*H(GAME + 132) == (n & 0xFFFF));
        *H(GAME + 132) = (uint16_t)n;
        cue(c, n & 0xFFFF, 0x0887C6D4u);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
