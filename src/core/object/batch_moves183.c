/* Move steps of 43 - 44 instructions: the flight that lands crouching into move 0x10 (0x0892B5C0, 2,548
   calls), landings into a move with a cue, a held opponent's puppet spawned with a longer hold, dash
   starts, a creep that steps forward on landing, a turn-or-timer dash (0x08996848) and projectile
   spawns with an extra one at full meter (0x08903570). Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0887C614(Cpu *c); void f_08931038(Cpu *c);
void f_08822D2C(Cpu *c); void f_089426E4(Cpu *c); void f_08837098(Cpu *c); void f_0887CE20(Cpu *c);
void f_08964F18(Cpu *c); void f_08993064(Cpu *c); void f_08999408(Cpu *c); void f_08903620(Cpu *c);
void f_08904A20(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): flies (+16 by +96 by +104, +20 by +100 by +108); at or below the floor step 3, at the floor,
   +262 / +73 cleared, cue 0x22 / 0x23, crouched (+696 = 1) and move 0x10; else ticked. */
SFA_FUNC(0892B5C0, move_fly_land_crouch_10) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t vx = *W(o + 96), vy = *W(o + 100);
    *W(o + 16) = *W(o + 16) + vx;
    *W(o + 20) = *W(o + 20) + vy;
    *W(o + 96) = vx + *W(o + 104);
    *W(o + 100) = vy + *W(o + 108);
    int32_t floor = S16(o + 132);
    if (!(floor < S16(o + 22))) {
        *B(o + 7) = 3; *H(o + 22) = (uint16_t)floor; *B(o + 262) = 0; *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x0892B638u);
        *B(o + 696) = 1;
        call2(c, o, 0x10, f_088616EC, 0x0892B64Cu);
    } else call1(c, o, f_08861848, 0x0892B65Cu);
    leave(c, sv);
}

/* (fighter): 0x08931038; in the air it moves by +96 toward its facing (+96 by +104) and ticks; at or
   below the floor +7 steps on, at the floor, +262 / +73 / +20 cleared, cue 0x22 / 0x23 and move 0xD. */
SFA_FUNC(08930F38, move_air_slide_land_d) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08931038, 0x08930F4Cu);
    int32_t floor = S16(o + 132);
    if (floor < S16(o + 22)) {
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + toward(o, v);
        *W(o + 96) = v + *W(o + 104);
        call1(c, o, f_08861848, 0x08930F98u);
    } else {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 262) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x08930FC8u);
        call2(c, o, 0xD, f_088616EC, 0x08930FD4u);
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag steps +7 on and spawns the held opponent's puppet (0x08822D2C 0x19 / 1,
   set up by 0x089426E4): its +346 by 3, +255 = 5, +347 / +112 cleared; +551 = 1, kept at +228,
   counted (+536) and +83 = 3. */
SFA_FUNC(0893E5D8, move_flag_puppet_b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x0893E5F0u);
    if (S8(o + 77) != 0) {
        bump(o + 7);
        ARG(0) = o; ARG(1) = 0x19; ARG(2) = 1; c->r[31] = 0x0893E618u; f_08822D2C(c);
        gaddr e = c->r[2];
        c->r[17] = e;
        if (e != 0) {
            ARG(0) = e; ARG(1) = o; c->r[31] = 0x0893E62Cu; f_089426E4(c);
            *B(e + 346) = (uint8_t)(S8(e + 346) + 3);
            *B(e + 255) = 5; *B(e + 347) = 0; *H(e + 112) = 0;
            uint32_t n = *B(o + 536);
            *B(o + 551) = 1;
            *W(o + 228) = e;
            *B(o + 536) = (uint8_t)(n + 1);
            *B(o + 0x53) = 3;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a dash: +262 = 1, step 1; locked (+373) +235 cleared and the guard (+78) from the facing
   and +497, else +644 = 2; 0x08837098, x 8.0 (11.5 without +193) / -0.1875, +549 = -1 (0 with +80),
   move `n`, 0x0887CE20 and `post`. */
static void dash_start(Cpu *c, uint32_t n, void (*post)(Cpu *), const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2];
    *B(o + 262) = 1;
    int32_t lock = S8(o + 373);
    *B(o + 7) = 1;
    enter(c, o, sv);
    if (lock != 0) {
        uint32_t f = *B(o + 11);
        int32_t g = S8(o + 497);
        *B(o + 235) = 0;
        *B(o + 78) = (uint8_t)(f ^ (uint32_t)g);
    } else *B(o + 644) = 2;
    call1(c, o, f_08837098, b[0]);
    *W(o + 104) = 0xFFFFD000u;
    *W(o + 96) = S8(o + 193) != 0 ? 0x80000u : 0xB8000u;
    int32_t h = S8(o + 80);
    *B(o + 549) = 0xFF;
    if (h != 0) *B(o + 549) = 0;
    call2(c, o, n, f_088616EC, b[1]);
    call1(c, o, f_0887CE20, b[2]);
    call1(c, o, post, b[3]);
    leave(c, sv);
}
SFA_FUNC(08964D08, move_dash_start_31) {
    static const uint32_t b[4] = { 0x08964D54u, 0x08964D94u, 0x08964D9Cu, 0x08964DA4u };
    dash_start(c, 0x31, f_08964F18, b);
}

/* (fighter): ticked until a frame flag; when 0x08993064 says so at or below the floor +7 steps on, at
   the floor, +73 and speeds cleared, 24 toward its facing, +74 = 1 and ticked. */
SFA_FUNC(0898FB48, move_creep_land_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) call1(c, o, f_08861848, 0x0898FB68u);
    call1(c, o, f_08993064, 0x0898FB70u);
    int32_t floor = S16(o + 132);
    if (c->r[2] != 0 && !(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 73) = 0; *W(o + 96) = 0; *W(o + 104) = 0; *W(o + 100) = 0;
        uint32_t f = *B(o + 11);
        *W(o + 108) = 0;
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(f != 0 ? x + 0x18 : x - 0x18);
        *B(o + 74) = 1;
        call1(c, o, f_08861848, 0x0898FBE4u);
    }
    leave(c, sv);
}

/* (fighter): 0x08999408 until a frame flag; once its facing matches +238 - 1 or the +82 timer runs out,
   +7 steps on with x 3.0 / +0.1875 (both negated facing right) into move 0x59; else ticked. */
SFA_FUNC(08996848, move_turn_or_timer_59) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) call1(c, o, f_08999408, 0x08996868u);
    uint32_t f = *B(o + 11);
    int go = (uint32_t)(S8(o + 238) - 1) == f;
    if (!go) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        go = (int16_t)t < 0;
    }
    if (go) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        if (f == 0) { *W(o + 96) = 0xFFFD0000u; *W(o + 104) = 0xFFFFD000u; }
        else { *W(o + 96) = 0x30000; *W(o + 104) = 0x3000; }
        call2(c, o, 0x59, f_088616EC, 0x089968E4u);
    } else call1(c, o, f_08861848, 0x089968A0u);
    leave(c, sv);
}

/* (fighter): ticked; a frame flag sets step 2 and +82 = 5 and spawns projectile 4 (set up by
   0x08903620); with +251 and the meter (+286) at +662 or more a second one, its +280 = +662. */
SFA_FUNC(08903570, move_flag_projectile_pair) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x08903588u);
    if (S8(o + 77) != 0) {
        *B(o + 7) = 2; *H(o + 82) = 5;
        ARG(0) = o; ARG(1) = 4; ARG(2) = 0; c->r[31] = 0x089035B0u; f_08822D2C(c);
        gaddr e = c->r[2];
        c->r[17] = e;
        if (e != 0) {
            ARG(0) = o; ARG(1) = e; c->r[31] = 0x089035C4u; f_08903620(c);
            if (S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662))) {
                ARG(0) = o; ARG(1) = 4; ARG(2) = 0; c->r[31] = 0x089035F0u; f_08822D2C(c);
                gaddr e2 = c->r[2];
                c->r[17] = e2;
                if (e2 != 0) {
                    ARG(0) = o; ARG(1) = e2; c->r[31] = 0x08903604u; f_08903620(c);
                    *B(e2 + 280) = (uint8_t)S16(o + 662);
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): when 0x08904A20 is negative and the move ended, the fall is -0.375 and at or below the floor
   it lands (step 5, at the floor, +235 / +73 / +20 cleared, cue 0x22 / 0x23) into move 0x1D + 2 x
   strength; else ticked. */
SFA_FUNC(08903DA0, move_end_fall_land_1d) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08904A20, 0x08903DB4u);
    if ((int32_t)c->r[2] < 0 && S8(o + 75) < 0) {
        int32_t floor = S16(o + 132);
        *W(o + 108) = 0xFFFFA000u;
        if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x08903DECu);
        else {
            *B(o + 7) = 5; *H(o + 22) = (uint16_t)floor; *B(o + 235) = 0; *B(o + 73) = 0; *H(o + 20) = 0;
            call1(c, o, f_0887C614, 0x08903E18u);
            call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0x1D) & 0xFFFF, f_088616EC, 0x08903E30u);
        }
    } else call1(c, o, f_08861848, 0x08903E40u);
    leave(c, sv);
}
