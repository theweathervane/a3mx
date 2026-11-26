/* Animation timers and menu confirms, 29 - 34 instructions: a step's frame count running out moves the
   animation on (or ends it), a fighter's move picked with its cancel data, menu items confirmed.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088215C4(Cpu *c); void f_0889C640(Cpu *c); void f_0889D01C(Cpu *c); void f_0886175C(Cpu *c);
void f_08899880(Cpu *c); void f_08899A30(Cpu *c); void f_0889CE64(Cpu *c); void f_0889C954(Cpu *c);
void f_088D3CD8(Cpu *c); void f_08861C70(Cpu *c);

static void enter(Cpu *c, gaddr m, gaddr x, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = sv[1];
    c->r[17] = m;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = x;
}
static void leave(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2];
    c->r[29] += 0x20;
}

/* (menu, item): with the item's +78 bit 1 the step (+6) moves on, +114 = +112, +96's high half
   cleared and the cursor cue; then the choice (0x0889C640) and 0x088215C4. */
SFA_FUNC(0889C538, menu_choice_confirm) {
    gaddr m = ARG(0), it = ARG(1);
    uint32_t sv[3];
    int32_t f = S8(it + 78);
    enter(c, m, it, sv);
    if (f & 2) {
        int32_t k = S16(m + 112);
        uint32_t w = *W(m + 96);
        *B(m + 6) = (uint8_t)(*B(m + 6) + 1);
        *H(m + 114) = (uint16_t)k;
        *W(m + 96) = w & 0xFFFF;
        ARG(0) = m; c->r[31] = 0x0889C584u; f_0889D01C(c);
    }
    ARG(0) = m; ARG(1) = it; c->r[31] = 0x0889C590u; f_0889C640(c);
    ARG(0) = m; c->r[31] = 0x0889C598u; f_088215C4(c);
    leave(c, sv);
}

/* (fighter, n): move n (+84) picked: unless +308, its two cancel bytes (the character's +28 list for the
   fighter's +386, two bytes per move) to +542 / +643; its animation (+12 list) made current and
   applied. */
SFA_FUNC(08861674, fighter_move_pick) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1) & 0xFFFF;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr tab = *W(o + 416);
    *W(c->r[29] + 16) = ra;
    if (S8(o + 308) == 0) {
        gaddr p = *W(*W(tab + 28) + ((uint32_t)S8(o + 386) << 2) + 4) + (n << 1);
        *B(o + 542) = *B(p);
        *B(o + 643) = *B(p + 1);
    }
    *H(o + 84) = (uint16_t)n;
    *W(o + 28) = *W(*W(tab + 12) + (n << 2));
    ARG(0) = o; c->r[31] = 0x088616E0u; f_0886175C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu, item): with the item's +78 bit 0 the menu's +5 moves on with the cursor cue; then the item
   drawn (0x08899880), 0x08899A30 with +82's high byte, and 0x088215C4. */
SFA_FUNC(08899744, menu_item_select) {
    gaddr m = ARG(0), it = ARG(1);
    uint32_t sv[3];
    int32_t f = S8(it + 78);
    enter(c, m, it, sv);
    if (f & 1) {
        *B(m + 5) = (uint8_t)(*B(m + 5) + 1);
        ARG(0) = m; c->r[31] = 0x0889977Cu; f_0889D01C(c);
    }
    ARG(0) = m; ARG(1) = it; c->r[31] = 0x08899788u; f_08899880(c);
    ARG(0) = m; ARG(1) = it; ARG(2) = (uint32_t)(S16(m + 82) >> 8) & 0xFFFF;
    c->r[31] = 0x088997A0u; f_08899A30(c);
    ARG(0) = m; c->r[31] = 0x088997A8u; f_088215C4(c);
    leave(c, sv);
}

/* (menu, side): when 0x0889CE64 says so the step (+6) moves on, +16 = +114 << 16 and the menu's x (+18)
   nudged 4 toward the side (+337); then the cursor (0x0889C954). */
SFA_FUNC(0889C828, menu_cursor_nudge) {
    gaddr m = ARG(0), x = ARG(1);
    uint32_t sv[3];
    enter(c, m, x, sv);
    c->r[31] = 0x0889C844u; f_0889CE64(c);
    if (c->r[2] != 0) {
        int32_t k = S16(m + 114);
        *B(m + 6) = (uint8_t)(*B(m + 6) + 1);
        *W(m + 16) = (uint32_t)k << 16;
        *H(m + 18) = (uint16_t)(S16(m + 18) + (S8(x + 337) != 0 ? -4 : 4));
    }
    ARG(0) = m; ARG(1) = x; c->r[31] = 0x0889C88Cu; f_0889C954(c);
    leave(c, sv);
}

/* The next step: +1 bit 6 ends the animation (+75 = -1); else it jumps +6 steps (10 bytes each). */
static int next_step(gaddr o, gaddr st, gaddr *to, int *back_hi) {
    int32_t f = S8(st + 1);
    if (f & 0x40) { *B(o + 75) = 0xFF; return 0; }
    *to = st + (uint32_t)(S16(st + 6) * 10);
    *back_hi = (f & 0x80) != 0;
    return 1;
}

/* (part): an ended step (+1 bit 6) ends the animation; else its frame count (+74) runs down and at 0
   the next step is applied (0x088D3CD8) with +152 = 1. */
SFA_FUNC(088D3DD8, part_anim_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr st = *W(o + 28);
    *W(c->r[29] + 20) = ra;
    if (S8(st + 1) & 0x40) *B(o + 75) = 0xFF;
    else {
        int8_t t = (int8_t)(S8(o + 74) - 1);
        *B(o + 74) = (uint8_t)t;
        gaddr to; int hi;
        if (t == 0 && next_step(o, st, &to, &hi)) {
            *W(o + 28) = to;
            ARG(0) = o; c->r[31] = hi ? 0x088D3E50u : 0x088D3E7Cu; f_088D3CD8(c);
            *W(o + 152) = 1;
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): the step's frame count (+74) runs down; at 0 the next step is set (0x08861C70). */
SFA_FUNC(08861BE8, fighter_anim_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int8_t t = (int8_t)(S8(o + 74) - 1);
    gaddr st = *W(o + 28);
    *W(c->r[29] + 16) = ra;
    *B(o + 74) = (uint8_t)t;
    gaddr to; int hi;
    if (t == 0 && next_step(o, st, &to, &hi)) {
        *W(o + 28) = to;
        ARG(0) = o; ARG(1) = to;
        c->r[31] = hi ? 0x08861C50u : 0x08861C64u; f_08861C70(c);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
