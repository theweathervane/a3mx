/* Move steps of 36 - 37 instructions: landings into a move, a slide that stops into move 0xE, a part
   character's arrival, a step table run only in mode 7 (0x088EDA28) with a timer that marks its side in
   the game flags, and jumps with their speeds by strength. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0899FC90(Cpu *c); void f_089A2620(Cpu *c);
void f_08862004(Cpu *c); void f_088D35D0(Cpu *c); void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c);
void f_088D3EC0(Cpu *c); void f_0887C614(Cpu *c); void f_08912FEC(Cpu *c);

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

/* (fighter): 0x0899FC90; below the floor it lands (+7 on, +73 / +235 / +262 cleared, at the floor) into
   move 6 + strength; else a frame flag (cleared) runs 0x089A2620, and ticked. */
SFA_FUNC(0899FBA8, move_land_6_or_flag) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0899FC90, 0x0899FBBCu);
    int32_t floor = S16(o + 132);
    if (S16(o + 22) < floor) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1); *B(o + 73) = 0;
        *W(o + 20) = (uint32_t)floor << 16;
        *B(o + 235) = 0; *B(o + 262) = 0;
        call2(c, o, (uint32_t)(S8(o + 194) + 6) & 0xFFFF, f_088616EC, 0x0899FC04u);
    } else {
        if (S8(o + 77) != 0) { *B(o + 77) = 0; call1(c, o, f_089A2620, 0x0899FC20u); }
        call1(c, o, f_08861848, 0x0899FC28u);
    }
    leave(c, sv);
}

/* (fighter): rises (+20 by +100 by +108) and slides by +96 toward its facing (+96 by +104); once that
   turns, x stopped, +7 steps on and move 0xE; else ticked. */
SFA_FUNC(089A6894, move_arc_stop_e) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    *W(o + 100) = vy + *W(o + 108);
    uint32_t v = *W(o + 96);
    *W(o + 16) = *W(o + 16) + (*B(o + 11) == 0 ? 0u - v : v);
    uint32_t v2 = v + *W(o + 104);
    *W(o + 96) = v2;
    if ((int32_t)v2 < 0) {
        uint32_t st = *B(o + 7);
        *W(o + 104) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 96) = 0;
        call2(c, o, 0xE, f_088616EC, 0x089A6904u);
    } else call1(c, o, f_08861848, 0x089A6918u);
    frame_out(c, ra);
}

/* (character): 0x08862004; when 0x088D35D0 gives 1, +73 cleared, +7 steps on, 0x088D36D0, +235 cleared,
   0x088D3960 (0, 0), +74 = 1, the part ticked and +262 cleared; else the part ticked until a frame flag. */
SFA_FUNC(089C15D4, part_arrive_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08862004, 0x089C15ECu);
    call1(c, o, f_088D35D0, 0x089C15F4u);
    c->r[17] = 1;
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_088D36D0, 0x089C1618u);
        *B(o + 235) = 0;
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089C162Cu; f_088D3960(c);
        *B(o + 74) = 1;
        call1(c, o, f_088D3EC0, 0x089C1638u);
        *B(o + 262) = 0;
    } else if (S8(o + 77) == 0) call1(c, o, f_088D3EC0, 0x089C1650u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in mode 7 (+692) the step table 0x08A439E0 by +7; else the +82 timer (when set) runs down,
   and as it runs out the fighter's side bit is set in the game flags (0x092E1228 +11); ticked. */
SFA_FUNC(088EDA28, move_mode7_table_or_timer) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 692) == 7) {
        gaddr base = 0x08A439E0u;
        uint32_t fn = *W(base + ((uint32_t)*B(o + 7) << 2));
        ARG(1) = fn; c->r[6] = base;
        c->r[31] = 0x088EDA60u; rt_call_indirect(c, fn);
    } else {
        int32_t t = S16(o + 82);
        if (t != 0) {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) {
                gaddr g = 0x092E1228u;
                *B(g + 11) = (uint8_t)(S8(g + 11) | (1u << (S8(o + 337) & 31)));
            }
        }
        call1(c, o, f_08861848, 0x088EDAB0u);
    }
    frame_out(c, ra);
}

/* (fighter): it falls (+20 by +100 by +108); at or below the floor step 4, cue 0x22 / 0x23, +73 / +262 /
   +235 cleared, at the floor and ticked; in the air ticked until a frame flag. */
SFA_FUNC(088F81E0, move_fall_land_4) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    int32_t floor = S16(o + 132), y = S16(o + 22);
    *W(o + 100) = vy + *W(o + 108);
    enter(c, o, sv);
    if (floor < y) {
        if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x088F8230u);
    } else {
        *B(o + 7) = 4;
        call1(c, o, f_0887C614, 0x088F8248u);
        *B(o + 73) = 0; *B(o + 262) = 0; *B(o + 235) = 0;
        *H(o + 22) = (uint16_t)S16(o + 132);
        call1(c, o, f_08861848, 0x088F8264u);
    }
    leave(c, sv);
}

/* (fighter): 0x08912FEC; falling below the floor it lands (at the floor, +73 cleared, step 5) into move
   0x1C + 2 x strength; else ticked. */
SFA_FUNC(08911418, move_fall_land_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08912FEC, 0x0891142Cu);
    if (*W(o + 100) & 0x80000000u) {
        int32_t floor = S16(o + 132);
        if (S16(o + 22) < floor) {
            int32_t k = S8(o + 194);
            *W(o + 20) = (uint32_t)floor << 16; *B(o + 73) = 0; *B(o + 7) = 5;
            call2(c, o, (uint32_t)((k << 1) + 0x1C) & 0xFFFF, f_088616EC, 0x0891147Cu);
        } else call1(c, o, f_08861848, 0x0891148Cu);
    } else call1(c, o, f_08861848, 0x0891149Cu);
    leave(c, sv);
}

/* (fighter): on frame flag 6 a jump: +7 on, +73 = 1, speeds by 2 x (strength + +828) (0x08A6A4EC: x, y),
   x brake -0.5, fall -0.375 (-0.5 for entry 4); ticked. */
SFA_FUNC(089276D8, move_flag6_jump) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 6) {
        int32_t s = S8(o + 194);
        uint32_t st = *B(o + 7), m = *W(o + 828);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1;
        int32_t i = (int16_t)(((uint32_t)s + m) << 1);
        gaddr e = 0x08A6A4ECu + ((uint32_t)i << 2);
        *W(o + 96) = *W(e);
        *W(o + 104) = 0xFFFF8000u;
        *W(o + 100) = *W(e + 4);
        *W(o + 108) = i == 4 ? 0xFFFF8000u : 0xFFFFA000u;
    }
    call1(c, o, f_08861848, 0x08927760u);
    frame_out(c, ra);
}

/* (fighter): without a frame flag step 2, +73 = 1, +20 low half cleared, the four speeds by strength
   (0x08A6BAEC: x, y, x brake, fall; x ones toward the facing); ticked. */
SFA_FUNC(0892B52C, move_jump_4speeds) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        int32_t s = S8(o + 194);
        *B(o + 7) = 2;
        gaddr e = 0x08A6BAECu + ((uint32_t)s << 4);
        *B(o + 73) = 1;
        *H(o + 20) = 0;
        uint32_t vx = *W(e), bx = *W(e + 4);
        *W(o + 100) = *W(e + 8);
        *W(o + 108) = *W(e + 12);
        if (*B(o + 11) == 0) { vx = 0u - vx; bx = 0u - bx; }
        *W(o + 96) = vx; *W(o + 104) = bx;
    }
    call1(c, o, f_08861848, 0x0892B5B4u);
    frame_out(c, ra);
}
