/* Throw steps of 22 - 49 instructions: throw holds that launch on a frame flag, a part character's
   throw jumps and grab (0x089BA358, which takes a new opponent from 0x088D4144), the opponent released
   with a hop (0x088EDE54) and the menu cursor's entry with its countdown voice (0x08855384). Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_08861A14(Cpu *c); void f_0886469C(Cpu *c);
void f_0886DDCC(Cpu *c); void f_0886DEC8(Cpu *c); void f_08978890(Cpu *c); void f_088D3EC0(Cpu *c);
void f_088D2E54(Cpu *c); void f_088D2E04(Cpu *c); void f_08855DBC(Cpu *c); void f_08862004(Cpu *c);
void f_088D35D0(Cpu *c); void f_088D36D0(Cpu *c); void f_088D2DBC(Cpu *c); void f_088D3960(Cpu *c);
void f_088D3968(Cpu *c); void f_088D2D54(Cpu *c); void f_088D2D74(Cpu *c); void f_088D4144(Cpu *c);
void f_0886AAE8(Cpu *c); void f_088D31AC(Cpu *c);

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

/* (fighter): without a frame flag 0x08978890; with one +7 steps on, +74 by 2 x strength - 4, and the
   opponent placed (0x0886DDCC). */
SFA_FUNC(08976B1C, throw_flag_step_74) {
    gaddr o = ARG(0); uint32_t ra;
    int32_t k = S8(o + 77);
    frame_in(c, &ra);
    if (k == 0) call1(c, o, f_08978890, 0x08976B38u);
    else {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194), v = S8(o + 74);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 74) = (uint8_t)(v + (s << 1) - 4);
        call1(c, o, f_0886DDCC, 0x08976B68u);
    }
    frame_out(c, ra);
}

/* (character): the part ticked; without frame flag +76 +7 steps on with a hop (+73 = 1, x 3.0 toward
   the facing, y 6.0 / -0.375) and the opponent placed; with it the fighter placed. */
SFA_FUNC(089BA50C, part_throw_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BA520u);
    if (S8(o + 76) == 0) {
        bump(o + 7);
        *B(o + 73) = 1;
        *W(o + 96) = 0x30000; *W(o + 104) = 0; *W(o + 100) = 0x60000;
        uint32_t f = *B(o + 11);
        *W(o + 108) = 0xFFFFA000u;
        if (f == 0) *W(o + 96) = 0u - *W(o + 96);
        call1(c, o, f_0886DDCC, 0x089BA578u);
    } else call1(c, o, f_0886DEC8, 0x089BA588u);
    leave(c, sv);
}

/* (character): the part ticked; without a frame flag +7 steps on with a flight (x 6.0 toward the facing,
   y 10.0, +82 = 24) and 0x088D2E54 (0x54); the opponent placed. */
SFA_FUNC(089BC3C8, part_throw_fly) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BC3DCu);
    if (S8(o + 77) == 0) {
        bump(o + 7);
        *W(o + 96) = 0x60000; *W(o + 104) = 0; *W(o + 100) = 0xA0000; *W(o + 108) = 0;
        uint32_t f = *B(o + 11);
        *H(o + 82) = 0x18;
        if (f == 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
        call2(c, o, 0x54, f_088D2E54, 0x089BC440u);
    }
    call1(c, o, f_0886DDCC, 0x089BC448u);
    leave(c, sv);
}

/* (menu cursor): in menu 5 a fresh one (+123 = -1) starts at x 112 with +82 = 16 and +5 = 1; its step
   from 0x08A16DB8 by +5; at +79 = 4 parked (y 0xFFF, +1 cleared); the countdown voice. */
SFA_FUNC(08855384, menu_cursor_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t m = S8(MATCH + 44);
    enter(c, o, sv);
    if (m == 5 && S8(o + 123) == 0) {
        *B(o + 123) = 0xFF; *W(o + 16) = 0x700000; *H(o + 82) = 0x10; *B(o + 5) = 1;
    }
    uint32_t fn = *W(0x08A16DB8u + ((uint32_t)*B(o + 5) << 2));
    ARG(0) = o; ARG(1) = fn;
    c->r[31] = 0x088553F0u; rt_call_indirect(c, fn);
    if (*B(o + 79) == 4) { *W(o + 20) = 0xFFF0000u; *B(o + 1) = 0; }
    call1(c, o, f_08855DBC, 0x08855410u);
    leave(c, sv);
}

/* (character): the part ticked; without a frame flag +7 steps on with a dash (x 12.0 / y 4.0 / fall
   -0.25, guard 2 forward, 1 facing right with x negated), bank cue 0x14 (strength 2 without +820) or
   0x12 and 0x088D2E54 (0x54); the opponent placed. */
SFA_FUNC(089BC1AC, part_throw_dash) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089BC1C0u);
    if (S8(o + 77) == 0) {
        bump(o + 7);
        *W(o + 96) = 0xC0000; *W(o + 104) = 0; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFC000u;
        uint32_t f = *B(o + 11);
        *B(o + 78) = 2;
        if (f != 0) {
            *B(o + 78) = 1;
            *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104);
        }
        if (S8(o + 194) == 2 && *W(o + 820) == 0) call2(c, o, 0x14, f_088D2E04, 0x089BC248u);
        else call2(c, o, 0x12, f_088D2E04, 0x089BC25Cu);
        call2(c, o, 0x54, f_088D2E54, 0x089BC268u);
    }
    call1(c, o, f_0886DDCC, 0x089BC270u);
    leave(c, sv);
}

/* (fighter): ticked with the opponent placed until a frame flag; then the hold ends (+135 cleared on
   both, the opponent turned): +6 steps on, +82 = 7, a hop back (x 2.0 away, y 4.0 / -0.25) and move
   0x18 of list 2. */
SFA_FUNC(088EDE54, throw_release_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t k = S8(o + 77);
    gaddr opp = *W(o + 92);
    if (k == 0) {
        call1(c, o, f_08861848, 0x088EDE78u);
        call1(c, o, f_0886DDCC, 0x088EDE80u);
    } else {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 82) = 7;
        *B(opp + 11) = (uint8_t)(*B(opp + 11) ^ 1);
        *B(o + 135) = 0; *B(opp + 135) = 0;
        *W(o + 96) = *B(o + 11) != 0 ? 0xFFFE0000u : 0x20000u;
        *W(o + 104) = 0; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFC000u;
        ARG(0) = o; ARG(1) = 0x18; ARG(2) = 2; c->r[31] = 0x088EDEF0u; f_08861A14(c);
    }
    leave(c, sv);
}

/* (character): the part ticked, 0x08862004; when 0x088D35D0 gives 1, +73 cleared, +7 on, 0x088D36D0,
   cue 0x22, move 0x14, 0x088D3960 (0, 0), 0x088D3968 (-28, 0), 0x088D2D54 and 0x088D2D74 on the
   opponent; the opponent placed. */
SFA_FUNC(089BA598, part_throw_land) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr opp = *W(o + 92);
    c->r[17] = opp;
    call1(c, o, f_088D3EC0, 0x089BA5B4u);
    call1(c, o, f_08862004, 0x089BA5BCu);
    call1(c, o, f_088D35D0, 0x089BA5C4u);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_088D36D0, 0x089BA5E8u);
        call1(c, o, f_088D2DBC, 0x089BA5F0u);
        call2(c, o, 0x14, f_088616EC, 0x089BA5FCu);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089BA60Cu; f_088D3960(c);
        ARG(0) = o; ARG(1) = 0xFFFFFFE4u; ARG(2) = 0; c->r[31] = 0x089BA61Cu; f_088D3968(c);
        call1(c, o, f_088D2D54, 0x089BA624u);
        call1(c, opp, f_088D2D74, 0x089BA62Cu);
    }
    call1(c, o, f_0886DDCC, 0x089BA634u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (character): ended: standing. A pending grab (+123 = 1) takes a free opponent from 0x088D4144 (+92):
   step 5, move 0x13, 0x0886AAE8 (its routine +224 +0x540) and the fighter placed; otherwise +123 is
   cleared and, as on other frames, a frame flag runs 0x088D31AC before the part ticks. */
SFA_FUNC(089BA358, part_grab) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (e < 0) { call1(c, o, f_0886469C, 0x089BA378u); leave(c, sv); return; }
    if (S8(o + 123) == 1) {
        call1(c, o, f_088D4144, 0x089BA398u);
        gaddr t = c->r[2];
        if (t != 0 && S8(t + 135) == 0) {
            *W(o + 92) = t; *B(o + 7) = 5;
            call2(c, o, 0x13, f_088616EC, 0x089BA3CCu);
            ARG(0) = o; ARG(1) = *W(o + 92); ARG(2) = *W(o + 224) + 0x540;
            c->r[31] = 0x089BA3E0u; f_0886AAE8(c);
            call1(c, o, f_0886DEC8, 0x089BA3E8u);
            leave(c, sv);
            return;
        }
        *B(o + 123) = 0;
    }
    if (S8(o + 77) != 0) call1(c, o, f_088D31AC, 0x089BA404u);
    call1(c, o, f_088D3EC0, 0x089BA40Cu);
    leave(c, sv);
}
