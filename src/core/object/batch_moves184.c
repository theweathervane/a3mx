/* Move steps of 44 instructions: a slide that grabs a standing opponent (0x0890BAC8), a timed step with
   a frame-flag recovery, projectile spawns with an extra one at full meter, slides and hops with their
   speeds by strength, a dash that stops by its guard (0x08964DFC) and rises that mark a cancel.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0890CC04(Cpu *c);
void f_0891B9E0(Cpu *c); void f_0886DB2C(Cpu *c); void f_0891DE70(Cpu *c); void f_08822D2C(Cpu *c);
void f_0892B31C(Cpu *c); void f_0886E294(Cpu *c); void f_08993064(Cpu *c); void f_0898F9DC(Cpu *c);
void f_089930D8(Cpu *c); void f_08877258(Cpu *c);

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

/* (fighter): once +96 turns, +7 steps on with a brake of -0.375; else 0x0890CC04, and a pending +123
   (cleared) against an opponent standing and not immune (+117 = 0xFF): out of the fight (+112) the same
   step on, else step 7 with +235 = 1; ticked. */
SFA_FUNC(0890BAC8, move_slide_grab) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t v = (int32_t)*W(o + 96);
    enter(c, o, sv);
    if (v < 0) { bump(o + 7); *W(o + 104) = 0xFFFFA000u; }
    else {
        call1(c, o, f_0890CC04, 0x0890BB00u);
        if (S8(o + 123) != 0) {
            gaddr opp = *W(o + 92);
            *B(o + 123) = 0;
            if (S8(opp + 73) == 0 && *B(opp + 117) != 0xFF) {
                if (S16(opp + 112) < 0) { bump(o + 7); *W(o + 104) = 0xFFFFA000u; }
                else { *B(o + 7) = 7; *B(o + 235) = 1; }
            }
        }
    }
    call1(c, o, f_08861848, 0x0890BB68u);
    leave(c, sv);
}

/* (fighter): the +82 timer runs down; out, +7 steps on into move 0x13 + strength; else 0x0891B9E0 (x
   stopped once +96 is 0), on a frame flag 0x0886DB2C with +262 cleared, else 0x0891DE70; ticked. */
SFA_FUNC(0891B878, move_timer_13_recover) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    enter(c, o, sv);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, (uint32_t)(s + 0x13) & 0xFFFF, f_088616EC, 0x0891B8C4u);
    } else {
        call1(c, o, f_0891B9E0, 0x0891B8D4u);
        if (*W(o + 96) == 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
        if (S8(o + 77) != 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, 0x0891B900u); }
        else call1(c, o, f_0891DE70, 0x0891B910u);
        call1(c, o, f_08861848, 0x0891B918u);
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag sets step 2 and +82 = 5 and spawns projectile 0xC (set up by
   0x0892B31C); with +251 and the meter (+286) at +662 or more a second one, its +280 = +662. */
SFA_FUNC(0892B26C, move_flag_projectile_pair_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0892B280u);
    if (S8(o + 77) != 0) {
        *B(o + 7) = 2; *H(o + 82) = 5;
        ARG(0) = o; ARG(1) = 0xC; ARG(2) = 0; c->r[31] = 0x0892B2A8u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            ARG(0) = o; ARG(1) = e; c->r[31] = 0x0892B2BCu; f_0892B31C(c);
            if (S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662))) {
                ARG(0) = o; ARG(1) = 0xC; ARG(2) = 0; c->r[31] = 0x0892B2E8u; f_08822D2C(c);
                gaddr e2 = c->r[2];
                if (e2 != 0) {
                    uint32_t m = (uint32_t)S16(o + 662) & 0xFF;
                    *B(e2 + 280) = (uint8_t)m;
                    ARG(0) = o; ARG(1) = e2; ARG(2) = m;
                    c->r[31] = 0x0892B30Cu; f_0892B31C(c);
                }
            }
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag (and +237 set: cleared, +573 from 0x08A6BB30 by strength, or by
   0x0886E294 with +81) it slides by +96 toward its facing (+96 by +104; once that turns, x stopped and
   +7 steps on). */
SFA_FUNC(0892BAA8, move_tick_slide_573) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0892BABCu);
    if (S8(o + 77) == 0) {
        if (S8(o + 237) != 0) {
            *B(o + 237) = 0;
            call1(c, o, f_0886E294, 0x0892BAE0u);
            uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
            *B(o + 573) = *B(0x08A6BB30u + i);
        }
        uint32_t v = *W(o + 96);
        uint32_t d = *B(o + 11) == 0 ? 0u - v : v;
        *W(o + 16) = *W(o + 16) + d;
        uint32_t v2 = v + *W(o + 104);
        *W(o + 96) = v2;
        if ((int32_t)v2 < 0) {
            uint32_t st = *B(o + 7);
            *W(o + 104) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 96) = 0;
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1, +82 = 12 + 4 x strength, x by strength
   (0x08A85A48) / -1/16 toward its facing, fall -3/32 from rest. */
SFA_FUNC(0895D518, move_tick_hop_timer) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0895D52Cu);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1;
        *B(o + 0x52) = (uint8_t)((s << 2) + 0xC);
        *W(o + 96) = *W(0x08A85A48u + ((uint32_t)S8(o + 194) << 2));
        *W(o + 100) = 0; *W(o + 104) = 0xFFFFF000u; *W(o + 108) = 0xFFFFE800u;
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    }
    leave(c, sv);
}

/* (fighter): moves by +96 by its guard (+78: forward, else back; +96 by +104), faces the opponent
   (+697); when its guard bit is in +238 (+653 = -1) or the speed is down to 3.0, step 3, +82 = 12,
   +262 cleared and move 0; else ticked. */
SFA_FUNC(08964DFC, move_guard_dash_stop) {
    gaddr o = ARG(0); uint32_t ra;
    int32_t g = S8(o + 78);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    frame_in(c, &ra);
    *W(o + 16) = g != 0 ? x + v : x - v;
    int32_t face = S8(o + 697);
    *W(o + 96) = v + *W(o + 104);
    int32_t bits = S8(o + 238);
    *B(o + 11) = (uint8_t)face;
    int stop = 1;
    if (bits & (1 << (g & 31))) *B(o + 653) = 0xFF;
    else if (0x30000 < (int32_t)*W(o + 96)) stop = 0;
    if (stop) {
        *B(o + 7) = 3; *H(o + 82) = 0xC; *B(o + 262) = 0;
        call2(c, o, 0, f_0886163C, 0x08964EA0u);
    } else call1(c, o, f_08861848, 0x08964E78u);
    frame_out(c, ra);
}

/* (fighter): 0x08993064; a frame flag with +123 runs 0x0898F9DC; else +123 cleared and the +82 timer
   runs down: out, +7 steps on with a hop (x by strength from 0x08A966FC / -0.1875, y 8.0 / -0.375);
   ticked. */
SFA_FUNC(0898FF7C, move_timer_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08993064, 0x0898FF90u);
    if (S8(o + 77) != 0 && S8(o + 123) != 0) call1(c, o, f_0898F9DC, 0x0898FFB0u);
    else {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *B(o + 123) = 0;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) {
            uint32_t st = *B(o + 7);
            int32_t s = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 96) = *W(0x08A966FCu + ((uint32_t)s << 2));
            *W(o + 104) = 0xFFFFD000u; *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFFA000u;
        }
        call1(c, o, f_08861848, 0x0899001Cu);
    }
    leave(c, sv);
}

/* (fighter): 0x089930D8; with y speed in [0, 1.0) +7 steps on with fall -1/32, +234 = 1, +82 cleared;
   else the cancel mark (+74 = 1) by 0x08877258 (0x08A964D8) when locked (+373), by +760 when not;
   ticked. */
SFA_FUNC(08991544, move_rise_cancel_mark_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089930D8, 0x08991558u);
    if ((*W(o + 100) & 0xFFFF0000u) < 0x10000u) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 108) = 0xFFFFF800u; *B(o + 234) = 1; *H(o + 82) = 0;
        call1(c, o, f_08861848, 0x089915E4u);
    } else {
        if (S8(o + 373) != 0) {
            call2(c, o, 0x08A964D8u, f_08877258, 0x089915A4u);
            if (c->r[2] != 0) *B(o + 74) = 1;
        } else if (*H(o + 760) & 0x3FF) *B(o + 74) = 1;
        call1(c, o, f_08861848, 0x089915B8u);
    }
    leave(c, sv);
}
