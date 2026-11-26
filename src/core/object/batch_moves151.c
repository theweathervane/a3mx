/* Move steps of 19 - 26 instructions: a part-animation character's teleport (0x089BF27C), recoveries and
   slides, landings with a cue, and jumps with their speeds. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EC0(Cpu *c);
void f_088D31EC(Cpu *c); void f_088D35D0(Cpu *c); void f_088D36D0(Cpu *c); void f_089BF140(Cpu *c);
void f_089C3B0C(Cpu *c); void f_0886E294(Cpu *c); void f_0886E3CC(Cpu *c); void f_08862004(Cpu *c);
void f_0887C614(Cpu *c); void f_088D2DDC(Cpu *c); void f_088D3848(Cpu *c); void f_0886DB2C(Cpu *c);
void f_088F501C(Cpu *c); void f_088F504C(Cpu *c); void f_08929118(Cpu *c); void f_0893207C(Cpu *c);
void f_0892F82C(Cpu *c);

#define BATTLE     0x092E1228u
#define SIDE_X     0x08AB147Cu   /* a start x per side */

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, gaddr o, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = a; ARG(2) = b; c->r[31] = back; fn(c);
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

/* (character): the teleport. Before it (+824 clear) the +820 wait runs down, then 0x088D31EC and, when
   0x088D35D0 says it is time, it appears at its side's start x (0x08AB147C) 24 in front, +824 counted,
   0x088D36D0; otherwise the part ticks (frame flag 2: 0x089C3B0C (3, 0)). After it, once the animation
   ends it turns round, back at its start x, and stands (0x089BF140). */
SFA_FUNC(089BF27C, part_teleport) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (*W(o + 824) != 0) {
        call1(c, o, f_088D3EC0, 0x089BF380u);
        if (S8(o + 75) < 0) {
            int32_t side = S8(o + 337);
            *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
            *B(o + 591) = 0; *B(o + 671) = 0;
            *W(o + 16) = *W(SIDE_X + ((uint32_t)side << 2));
            call1(c, o, f_089BF140, 0x089BF3C4u);
        }
    } else {
        int32_t t = (int32_t)*W(o + 820);
        if (t > 0) *W(o + 820) = (uint32_t)(t - 1);
        else {
            call1(c, o, f_088D31EC, 0x089BF2ACu);
            call1(c, o, f_088D35D0, 0x089BF2B4u);
            c->r[17] = 1;
            if (c->r[2] == 1) {
                uint32_t n = *W(o + 824);
                *W(o + 820) = 0;
                *W(o + 824) = n + 1;
                call1(c, o, f_088D36D0, 0x089BF2F8u);
                int32_t side = S8(o + 337);
                *B(o + 73) = 0;
                *W(o + 16) = *W(SIDE_X + ((uint32_t)side << 2));
                *H(o + 18) = (uint16_t)(S16(o + 18) + (side == 0 ? 0x18 : -0x18));
                *B(o + 74) = 1;
                call1(c, o, f_088D3EC0, 0x089BF33Cu);
            } else {
                call1(c, o, f_088D3EC0, 0x089BF34Cu);
                if (S8(o + 77) == 2) { *B(o + 77) = 0; call3(c, o, 3, 0, f_089C3B0C, 0x089BF370u); }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; without a frame flag step 3, +237 cleared and +573 from 0x08A6BB34 by its +194 (with
   +81: by 0x0886E294). */
SFA_FUNC(0892C324, move_tick_573_c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0892C338u);
    if (S8(o + 77) == 0) {
        *B(o + 7) = 3; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x0892C354u);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)S8(o + 194) : c->r[2];
        *B(o + 573) = (uint8_t)S8(0x08A6BB34u + i);
    }
    leave(c, sv);
}

/* (fighter): moves by +96 against its facing (+11); at x 640 (+18) step 2 with move 0x30, else ticked. */
SFA_FUNC(0892F644, move_walk_to_640) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t v = *W(o + 96);
    frame_in(c, &ra);
    if (*B(o + 11) != 0) v = 0u - v;
    *W(o + 16) = *W(o + 16) + v;
    if (S16(o + 18) == 0x280) { *B(o + 7) = 2; call2(c, o, 0x30, f_0886163C, 0x0892F69Cu); }
    else call1(c, o, f_08861848, 0x0892F684u);
    frame_out(c, ra);
}

/* (fighter): without a held opponent (+228) +262 cleared and +7 steps on with move 0x11; else ticked;
   then 0x0886E3CC (8, 4). */
SFA_FUNC(0893E49C, move_hold_or_11) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    gaddr h = *W(o + 228);
    enter(c, o, sv);
    if (h == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 262) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call2(c, o, 0x11, f_088616EC, 0x0893E4D0u);
    } else call1(c, o, f_08861848, 0x0893E4E0u);
    call3(c, o, 8, 4, f_0886E3CC, 0x0893E4F0u);
    leave(c, sv);
}

/* (fighter): 0x08862004; on landing step 9, at the floor, +73 cleared, cue 0x22 / 0x23 and move 0x17;
   else ticked. */
SFA_FUNC(089462B0, move_land_17) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089462C4u);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        *B(o + 7) = 9; *W(o + 20) = (uint32_t)y << 16; *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x089462F8u);
        call2(c, o, 0x17, f_088616EC, 0x08946304u);
    } else call1(c, o, f_08861848, 0x089462D4u);
    leave(c, sv);
}

/* (fighter): step 4 (8 unless +236 is 2), hop speeds (x -0.75, y 10.0 / -0.5), move 9 + its +194. */
static void hop_9(Cpu *c, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    *B(o + 7) = S8(o + 236) == 2 ? 4 : 8;
    *W(o + 96) = 0xFFFF4000u; *W(o + 104) = 0; *W(o + 100) = 0xA0000;
    int32_t k = S8(o + 194);
    *W(o + 108) = 0xFFFF8000u;
    call2(c, o, (uint32_t)(k + 9) & 0xFFFF, f_088616EC, back);
    frame_out(c, ra);
}
SFA_FUNC(0898F9DC, move_hop_9) { hop_9(c, 0x0898FA34u); }
SFA_FUNC(08995134, move_hop_9_b) { hop_9(c, 0x0899518Cu); }

/* (fighter): a negative frame flag sets +82 to 0x22xx less 0x200 * its +194 (keeping the low byte) and
   steps +7 on; ticked. */
SFA_FUNC(0899C33C, move_flag_82) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) < 0) {
        uint32_t st = *B(o + 7);
        *H(o + 82) = (uint16_t)(S16(o + 82) & 0xFF);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 82) = (uint16_t)(S16(o + 82) | 0x2200);
        *H(o + 82) = (uint16_t)(S16(o + 82) - (S8(o + 194) << 9));
        *B(o + 77) = 0;
    }
    call1(c, o, f_08861848, 0x0899C394u);
    frame_out(c, ra);
}

/* (fighter): step 3, +310 / +696 = 1, +594 cleared, a push (+96 8.0 / -0.25, away from the facing), move 0xD. */
SFA_FUNC(089BAAA8, move_push_d) {
    gaddr o = ARG(0);
    uint32_t ra;
    *B(o + 7) = 3; *B(o + 310) = 1; *B(o + 696) = 1; *B(o + 594) = 0;
    *W(o + 96) = 0x80000;
    uint32_t f = *B(o + 11);
    frame_in(c, &ra);
    *W(o + 104) = 0xFFFFC000u;
    if (f == 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    call2(c, o, 0xD, f_088616EC, 0x089BAB00u);
    frame_out(c, ra);
}

/* (character): the part ticked; a frame flag steps +7 on with +73 = 1, 0x088D2DDC and 0x088D3848;
   otherwise 0x0886DB2C. */
SFA_FUNC(089C8164, part_tick_flag_or_db2c) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C8178u);
    if (S8(o + 77) != 0) {
        bump(o + 7); *B(o + 73) = 1;
        call1(c, o, f_088D2DDC, 0x089C81A0u);
        call1(c, o, f_088D3848, 0x089C81A8u);
    } else call1(c, o, f_0886DB2C, 0x089C81B8u);
    leave(c, sv);
}

/* (fighter): without a frame flag +237 / +123 / +82 cleared, +7 steps on, +260 = 6, a push (8.0 / -0.5)
   and 0x0886E294; ticked. */
SFA_FUNC(088F0A10, move_flag_push_tick) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 123) = 0; *H(o + 82) = 0; *B(o + 260) = 6;
        *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF8000u;
        call1(c, o, f_0886E294, 0x088F0A60u);
    }
    call1(c, o, f_08861848, 0x088F0A68u);
    leave(c, sv);
}

/* (fighter): once the animation has ended a count (+`cnt` of the mark at +0x334) runs down; out, step 3
   with move `n`; otherwise it moves on by +96 and ticks. */
static void count_then(Cpu *c, uint32_t cnt, int at_zero, uint32_t n, uint32_t b_pick, uint32_t b_tick) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        gaddr a = o + 0x334 + cnt;
        int8_t t = (int8_t)(S8(a) - 1);
        *B(a) = (uint8_t)t;
        if (at_zero ? t == 0 : t < 0) {
            *B(o + 7) = 3;
            call2(c, o, n, f_088616EC, b_pick);
            frame_out(c, ra);
            return;
        }
    }
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    call1(c, o, f_08861848, b_tick);
    frame_out(c, ra);
}
SFA_FUNC(088F3B20, move_count_then_9) { count_then(c, 0, 0, 9, 0x088F3B5Cu, 0x088F3B7Cu); }
SFA_FUNC(088F8714, move_count_then_d) { count_then(c, 4, 1, 0xD, 0x088F8750u, 0x088F8770u); }

/* (fighter): 0x088F501C; when the animation ends 0x088F504C, step 6, the mark's +0 = 4 and move 0x32;
   else ticked. */
SFA_FUNC(088F4E9C, move_end_then_32) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088F501C, 0x088F4EB0u);
    if (S8(o + 75) < 0) {
        call1(c, o, f_088F504C, 0x088F4EC4u);
        *B(o + 7) = 6; *B(o + 0x334) = 4;
        call2(c, o, 0x32, f_088616EC, 0x088F4EE4u);
    } else call1(c, o, f_08861848, 0x088F4EF4u);
    leave(c, sv);
}

/* (fighter): when the animation ends back to standing; else a frame flag sets the battle's +35 = 24, and
   ticked. */
SFA_FUNC(088F6160, move_end_or_shake) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        call2(c, o, 0, f_0886163C, 0x088F618Cu);
    } else {
        if (S8(o + 77) != 0) { *B(o + 77) = 0; *B(BATTLE + 35) = 0x18; }
        call1(c, o, f_08861848, 0x088F61BCu);
    }
    frame_out(c, ra);
}

/* (fighter): +7 steps on, +262 = 1, a lunge forward (x 8.5 / -0.25 toward the facing), move 0x2F. */
SFA_FUNC(088FF280, move_lunge_2f) {
    gaddr o = ARG(0);
    uint32_t ra;
    bump(o + 7);
    *B(o + 262) = 1;
    uint32_t f = *B(o + 11);
    frame_in(c, &ra);
    *W(o + 104) = 0;
    if (f != 0) { *W(o + 96) = 0x88000; *W(o + 104) = 0xFFFFC000u; }
    else { *W(o + 96) = 0xFFF78000u; *W(o + 104) = 0x4000; }
    call2(c, o, 0x2F, f_088616EC, 0x088FF2DCu);
    frame_out(c, ra);
}

/* (fighter): ticked; when the frame byte `flag` is clear, step 3, +237 cleared and +573 from `tab` by its
   +194 (with +81: by 0x0886E294). */
static void tick_573_tab(Cpu *c, uint32_t flag, gaddr tab, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, b1);
    if (S8(o + flag) == 0) {
        *B(o + 7) = 3;
        uint32_t i;
        if (S8(o + 81) != 0) { call1(c, o, f_0886E294, b2); i = c->r[2]; }
        else i = (uint32_t)S8(o + 194);
        int8_t v = S8(tab + i);
        *B(o + 237) = 0;
        *B(o + 573) = (uint8_t)v;
    }
    leave(c, sv);
}
SFA_FUNC(08903CC8, move_tick_573_76) { tick_573_tab(c, 76, 0x08A5162Cu, 0x08903CDCu, 0x08903CFCu); }
SFA_FUNC(08903F78, move_tick_573_77) { tick_573_tab(c, 77, 0x08A51678u, 0x08903F8Cu, 0x08903FACu); }

/* (fighter): 0x08862004; on landing +7 steps on, +73 cleared, at the floor, cue 0x22 / 0x23 and move
   0x11; else ticked. */
SFA_FUNC(089162F0, move_land_11) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x08916304u);
    if (c->r[2] != 0) {
        int32_t y = S16(o + 132);
        bump(o + 7); *B(o + 73) = 0; *W(o + 20) = (uint32_t)y << 16;
        call1(c, o, f_0887C614, 0x0891633Cu);
        call2(c, o, 0x11, f_0886163C, 0x08916348u);
    } else call1(c, o, f_08861848, 0x08916314u);
    leave(c, sv);
}

/* (fighter): ticked; at frame flag 2 +7 steps on, +73 = 1 and it steps 40 forward. */
SFA_FUNC(08927560, move_tick_flag2_step40) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08927574u);
    if (S8(o + 77) == 2) {
        bump(o + 7);
        uint32_t f = *B(o + 11);
        *B(o + 73) = 1;
        *H(o + 18) = (uint16_t)(S16(o + 18) + (f != 0 ? 0x28 : -0x28));
    }
    leave(c, sv);
}

/* (fighter): 0x08929118; once falling +7 steps on, +96 cleared, +108 = -0.5 and move 0x23 + its +194;
   else ticked. */
SFA_FUNC(089275C8, move_fall_23) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08929118, 0x089275DCu);
    if ((int32_t)*W(o + 100) < 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        *W(o + 96) = 0; *W(o + 108) = 0xFFFF8000u;
        call2(c, o, (uint32_t)(k + 0x23) & 0xFFFF, f_088616EC, 0x08927620u);
    } else call1(c, o, f_08861848, 0x089275F0u);
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag and with 0x0893207C not negative, +7 steps on with move 0x2A;
   else 0x0892F82C. */
SFA_FUNC(08931EE4, move_tick_check_2a) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931EF8u);
    int go = 0;
    if (S8(o + 77) == 0) { call1(c, o, f_0893207C, 0x08931F0Cu); go = (int32_t)c->r[2] >= 0; }
    if (go) { bump(o + 7); call2(c, o, 0x2A, f_088616EC, 0x08931F2Cu); }
    else call1(c, o, f_0892F82C, 0x08931F3Cu);
    leave(c, sv);
}
