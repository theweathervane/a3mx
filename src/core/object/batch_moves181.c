/* Move steps of 41 - 42 instructions: a follow-up move picked from a stance (0x08904A78), dashes that
   mark where they started, a hop that may turn around, landings into a move, a part character's
   guard pick, and a shadow spawned in the fighter's place (0x0893BB94). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_08917870(Cpu *c); void f_0887C614(Cpu *c);
void f_0899CF0C(Cpu *c); void f_0899CF6C(Cpu *c); void f_0886E294(Cpu *c); void f_089A45C8(Cpu *c);
void f_08929118(Cpu *c); void f_08822EAC(Cpu *c); void f_0893BC3C(Cpu *c);

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

/* (fighter): in state 1 / 0 / 7, stance 1, a choice made (+386) and +251 clear, a pending +123 (cleared)
   picks move 0x2F when positive, else 0x2A + 2 x strength. */
SFA_FUNC(08904A78, move_stance_follow_up) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 386) >= 0 && *B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 7 && S8(o + 236) == 1
        && S8(o + 251) == 0) {
        int32_t f = S8(o + 123);
        if (f > 0) { *B(o + 123) = 0; call2(c, o, 0x2F, f_088616EC, 0x08904AF0u); }
        else if (f < 0) {
            *B(o + 123) = 0;
            call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0x2A) & 0xFFFF, f_088616EC, 0x08904B10u);
        }
    }
    frame_out(c, ra);
}

/* (fighter): ended: +7 on, +82 / +123 cleared, the mark (+0x334) 0x160 ahead of x, a dash (8.0 / -0.75)
   toward the facing and 0x08917870; ticked. */
SFA_FUNC(08916AC4, move_dash_mark_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e & 0x80) {
        uint32_t st = *B(o + 7);
        *H(o + 82) = 0; *B(o + 7) = (uint8_t)(st + 1);
        int32_t x = S16(o + 18);
        *B(o + 123) = 0;
        gaddr m = o + 0x334;
        *H(m) = (uint16_t)x;
        if (*B(o + 11) == 0) {
            *H(m) = (uint16_t)(S16(m) - 0x160);
            *W(o + 96) = 0xFFF80000u; *W(o + 104) = 0xC000;
        } else {
            *H(m) = (uint16_t)(S16(m) + 0x160);
            *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF4000u;
        }
        call1(c, o, f_08917870, 0x08916B50u);
    }
    call1(c, o, f_08861848, 0x08916B58u);
    leave(c, sv);
}

/* (fighter): ended: a hop (+7 on, y 6.0 / -0.3125), turning around with +290 bit 1, x 8.0 / -0.3125 when
   facing left, else -8.0 / +0.3125, and move 0xD; else ticked. */
SFA_FUNC(08921514, move_end_hop_d) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        bump(o + 7);
        *W(o + 96) = 0x60000; *W(o + 104) = 0xFFFFC000u; *W(o + 100) = 0x60000;
        uint32_t h = *H(o + 290);
        *W(o + 108) = 0xFFFFB000u;
        if (h & 2) *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
        if (*B(o + 11) == 0) { *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFFB000u; }
        else { *W(o + 96) = 0xFFF80000u; *W(o + 104) = 0x5000; }
        call2(c, o, 0xD, f_088616EC, 0x08921598u);
    } else call1(c, o, f_08861848, 0x089215ACu);
    frame_out(c, ra);
}

/* (fighter): falling the fall is -0.5; x stopped when 0x0899CF0C is negative; 0x0899CF6C; at or below
   the floor +7 steps on, at the floor, +20 / +73 cleared, cue 0x22 / 0x23 and move 0x1E + strength;
   else ticked. */
SFA_FUNC(0899C0E0, move_fall_land_1e) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t vy = (int32_t)*W(o + 100);
    enter(c, o, sv);
    if (vy < 0) *W(o + 108) = 0xFFFF8000u;
    call1(c, o, f_0899CF0C, 0x0899C108u);
    if ((int32_t)c->r[2] < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
    call1(c, o, f_0899CF6C, 0x0899C120u);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x0899C13Cu);
    else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x0899C160u);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x1E) & 0xFFFF, f_088616EC, 0x0899C174u);
    }
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1, +237 / +82 cleared, +573 from
   0x08A9D678 (by strength, or by 0x0886E294 with +81), the guard (+78) and +820 from 0x08A9D67C by
   strength, then 0x089A45C8. */
SFA_FUNC(089A4524, move_tick_guard_pick) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089A4538u);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x089A4564u);
        int32_t s = S8(o + 194);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)s : c->r[2];
        *B(o + 573) = *B(0x08A9D678u + i);
        gaddr p = 0x08A9D67Cu + ((uint32_t)s << 1);
        *B(o + 78) = *B(p);
        *H(o + 82) = 0;
        *W(o + 820) = *B(p);
        call1(c, o, f_089A45C8, 0x089A45B8u);
    }
    leave(c, sv);
}

/* (fighter): with the guard (+78) step 6 and a brake of 0.5 back from the facing, else +7 on, +82 = 60
   and x 8.0 toward the facing; move 0x3D and +262 = 1. */
SFA_FUNC(089BCDA4, move_guard_or_dash_3d) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t f = *B(o + 11);
    if (S8(o + 78) != 0) {
        *B(o + 7) = 6; *W(o + 104) = 0xFFFF8000u;
        if (f == 0) *W(o + 104) = 0u - *W(o + 104);
        call2(c, o, 0x3D, f_088616EC, 0x089BCDECu);
    } else {
        bump(o + 7);
        *H(o + 82) = 0x3C; *W(o + 96) = 0xFFF80000u; *W(o + 104) = 0;
        if (f == 0) *W(o + 96) = 0u - *W(o + 96);
        call2(c, o, 0x3D, f_088616EC, 0x089BCE30u);
    }
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): when 0x08929118 lands it +7 steps on, at the floor, +20 / +73 / +828 cleared, cue 0x22 /
   0x23, ticked and move 0x34 (no choice made) or 0x26 + strength; else ticked on frame flag 4. */
SFA_FUNC(08927630, move_land_34_or_26) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08929118, 0x08927644u);
    if (c->r[2] != 0) {
        if (S8(o + 77) == 4) call1(c, o, f_08861848, 0x08927664u);
    } else {
        uint32_t st = *B(o + 7);
        int32_t floor = S16(o + 132);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0; *W(o + 828) = 0;
        call1(c, o, f_0887C614, 0x08927690u);
        call1(c, o, f_08861848, 0x08927698u);
        if (S8(o + 386) < 0) call2(c, o, 0x34, f_088616EC, 0x089276ACu);
        else call2(c, o, (uint32_t)(S8(o + 194) + 0x26) & 0xFFFF, f_088616EC, 0x089276C8u);
    }
    leave(c, sv);
}

/* (fighter): +6 steps on and a shadow (0x21 / 0) is spawned in its place: tied to it, its +12 - +14,
   place, picture (+24 / +164), facing, +9 = 3 and +86 its side; then 0x0893BC3C. */
SFA_FUNC(0893BB94, move_spawn_shadow) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
    ARG(0) = 0x21; ARG(1) = 0; c->r[31] = 0x0893BBBCu; f_08822EAC(c);
    gaddr e = c->r[2];
    if (e != 0) {
        *W(e + 88) = o;
        *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13); *B(e + 14) = *B(o + 14);
        *H(e + 18) = *H(o + 18);
        *B(e + 9) = 3;
        *H(e + 22) = *H(o + 22); *H(e + 24) = *H(o + 24);
        *B(e + 11) = *B(o + 11);
        *H(e + 86) = (uint16_t)S8(o + 337);
        *W(e + 164) = *W(o + 164);
    }
    call1(c, o, f_0893BC3C, 0x0893BC2Cu);
    leave(c, sv);
}
