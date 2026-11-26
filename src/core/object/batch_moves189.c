/* Move steps of 18 - 52 instructions: a creep into the guard dash, an air slide that lands into move
   0x18, the bounce off the ground after a hit (0x0885E228), a landing that slows the match and hops back
   (0x0896B568), a lunge that stops at its mark or near the opponent (0x08916B68), timed jumps with a
   guard pick, and a part character's jump by strength. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c);
void f_088F965C(Cpu *c); void f_0887C614(Cpu *c); void f_08862004(Cpu *c); void f_0896F62C(Cpu *c);
void f_0886E3CC(Cpu *c); void f_0896F498(Cpu *c); void f_0896F568(Cpu *c); void f_0887C408(Cpu *c);
void f_0886E294(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D31AC(Cpu *c);

#define MATCH 0x092E1228u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): ended: step 1 and the guard dash (0x088F965C); else x by the frame flag and ticked. */
SFA_FUNC(088F9614, move_creep_then_guard_dash_b) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) { *B(o + 7) = 1; call1(c, o, f_088F965C, 0x088F9630u); }
    else {
        *H(o + 18) = (uint16_t)(S16(o + 18) + S8(o + 77));
        call1(c, o, f_08861848, 0x088F9650u);
    }
    frame_out(c, ra);
}

/* (fighter): it falls (+20 by +100 by +108); in the air it slides by +96 toward its facing (+96 by
   +104) and ticks; at or below the floor +7 steps on, at the floor, +262 / +73 / +20 cleared, cue 0x22 /
   0x23 and move 0x18. */
SFA_FUNC(08931168, move_fall_slide_land_18) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132);
    *W(o + 100) = vy + *W(o + 108);
    if (floor < S16(o + 22)) {
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + toward(o, v);
        *W(o + 96) = v + *W(o + 104);
        call1(c, o, f_08861848, 0x089311E0u);
    } else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x08931210u);
        call2(c, o, 0x18, f_088616EC, 0x0893121Cu);
    }
    leave(c, sv);
}

/* (fighter): after a hit, when 0x08862004 lands it falling it bounces: +7 on, +73 cleared, at the floor,
   +82 = 4, +239 / +270 = 1, hit records (+560 - +562, +586 / +587, +656) cleared, facing and x 6.0 by
   the way it was moving, move 0x1F of list 2; still in the air and falling ticked. */
SFA_FUNC(0885E228, fighter_hit_bounce) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x0885E23Cu);
    int32_t vy = (int32_t)*W(o + 100);
    if (c->r[2] != 0) {
        if (vy < 0) {
            uint32_t st = *B(o + 7);
            int32_t floor = S16(o + 132);
            *B(o + 7) = (uint8_t)(st + 1);
            *B(o + 73) = 0;
            *W(o + 20) = (uint32_t)floor << 16;
            *H(o + 82) = 4; *B(o + 239) = 1; *B(o + 270) = 1;
            *B(o + 560) = 0; *B(o + 561) = 0; *B(o + 562) = 0; *B(o + 586) = 0; *B(o + 587) = 0;
            int32_t v = (int32_t)*W(o + 96);
            *B(o + 656) = 0;
            if (v < 0) { *B(o + 11) = 1; *W(o + 96) = 0xFFFA0000u; }
            else { *B(o + 11) = 0; *W(o + 96) = 0x60000; }
            call2(c, o, 0x1F, f_08861724, 0x0885E2C8u);
        }
    } else if (vy < 0) call1(c, o, f_08861848, 0x0885E2E0u);
    leave(c, sv);
}

/* (fighter): 0x0896F62C; at or below the floor 0x0886E3CC (8, 12), +7 on, at the floor, a hop back
   (x -1.17, y 10.0 / -0.5), the match slowed (+35 = 32), 0x0896F498, 0x0896F568, cue 0x27 from its
   side and move 0x1F; else ticked. */
SFA_FUNC(0896B568, move_land_slow_hop_1f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896F62C, 0x0896B57Cu);
    if ((S16(o + 132) & 0xFFFF) < (S16(o + 22) & 0xFFFF)) call1(c, o, f_08861848, 0x0896B624u);
    else {
        ARG(0) = o; ARG(1) = 8; ARG(2) = 0xC; c->r[31] = 0x0896B5A4u; f_0886E3CC(c);
        uint32_t st = *B(o + 7);
        int32_t floor = S16(o + 132);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor;
        *W(o + 96) = 0xFFFED44Bu; *W(o + 104) = 0; *W(o + 100) = 0xA0000; *W(o + 108) = 0xFFFF8000u;
        *B(MATCH + 35) = 0x20;
        call1(c, o, f_0896F498, 0x0896B5F0u);
        call1(c, o, f_0896F568, 0x0896B5F8u);
        ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x27; c->r[31] = 0x0896B608u; f_0887C408(c);
        call2(c, o, 0x1F, f_0886163C, 0x0896B614u);
    }
    leave(c, sv);
}

/* (fighter): moves by +96; it steps on into move 0x3F once past its mark (+0x334), when +238 asks for
   its way of moving (1 back, 2 forward), or within 80 of the opponent's x (32 ahead); else ticked. */
SFA_FUNC(08916B68, move_lunge_to_mark_3f) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t x = *W(o + 16);
    gaddr opp = *W(o + 92);
    uint32_t v = *W(o + 96);
    *W(o + 16) = x + v;
    uint32_t f = *B(o + 11);
    int32_t px = S16(o + 18), m = S16(o + 0x334);
    frame_in(c, &ra);
    int16_t d = (int16_t)(f != 0 ? m - px : px - m);
    int go = 1;
    if (d >= 0 && S8(o + 238) != ((int32_t)v < 0 ? 1 : 2)) {
        uint32_t off = f == 0 ? 0xFFE0u : 0x20u;
        int32_t r = (int32_t)((off + (uint32_t)px - (uint32_t)S16(opp + 18) + 0x50) & 0xFFFF);
        if (0xA0 < r) go = 0;
    }
    if (go) { bump(o + 7); call2(c, o, 0x3F, f_088616EC, 0x08916C2Cu); }
    else call1(c, o, f_08861848, 0x08916C0Cu);
    frame_out(c, ra);
}

/* (fighter): the +83 wait runs down; past 0 +7 steps on, +237 / +573 cleared, +83 from 0x08A8DFC4 (by
   strength, or by 0x0886E294 with +81), +82 = 2 without strength, +0x392 cleared, +74 = 1; ticked
   unless a negative frame flag. */
SFA_FUNC(0896E710, move_wait_guard_pick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    gaddr w = o + 0x52;
    *W(c->r[29] + 24) = s2; c->r[18] = w;
    *W(c->r[29] + 20) = s1; c->r[17] = w + 1;
    int8_t t = (int8_t)(S8(w + 1) - 1);
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *B(w + 1) = (uint8_t)t;
    *W(c->r[29] + 28) = ra;
    if (t < 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x0896E764u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = 0;
        *B(w + 1) = (uint8_t)S8(0x08A8DFC4u + i);
        *B(w) = S8(o + 194) != 0 ? 0 : 2;
        *B(o + 0x392) = 0;
        *B(o + 74) = 1;
    }
    if (S8(o + 77) >= 0) call1(c, o, f_08861848, 0x0896E7C8u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1, +237 cleared, +573 from 0x08A9D710 (by
   strength, or by 0x0886E294 with +81) and the four speeds by strength (0x08A9D6E0: x, x brake toward
   the facing, y, fall). */
SFA_FUNC(089A4858, move_tick_jump_573_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089A486Cu);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x089A4898u);
        int32_t s = S8(o + 194);
        gaddr e = 0x08A9D6E0u + ((uint32_t)s << 4);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)s : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08A9D710u + i);
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(e); *W(o + 104) = 0u - *W(e + 4); }
        else { *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); }
        *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    leave(c, sv);
}

/* (character): the part ticked; frame flag 1 (cleared) runs 0x088D31AC; frame flag 2 (cleared) steps +7
   on with x stopped, +73 = 1 and a jump by strength (y 4.0 / 6.0 / 8.0, fall -0.375). */
SFA_FUNC(089BBB5C, part_flag_jump) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_088D3EC0, 0x089BBB74u);
    c->r[17] = 1;
    if (S8(o + 77) == 1) { *B(o + 77) = 0; call1(c, o, f_088D31AC, 0x089BBB90u); }
    if (S8(o + 77) == 2) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 96) = 0; *W(o + 104) = 0;
        int32_t s = S8(o + 194);
        uint32_t vy = s == 0 ? 0x40000u : s == 1 ? 0x60000u : s == 2 ? 0x80000u : 0;
        if (vy != 0) { *W(o + 100) = vy; *W(o + 108) = 0xFFFFA000u; }
        *B(o + 73) = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
