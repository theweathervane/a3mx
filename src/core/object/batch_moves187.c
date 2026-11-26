/* Move steps of 18 - 47 instructions: a throw step table that keeps the opponent posed (0x08937248), a
   landing that picks a once-only follow-up, a throw slide placed by side (0x0892F6A8), a held
   opponent's puppet with its hit data (0x089313AC), landings into a move, and a jump with its guard
   pick. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886E17C(Cpu *c);
void f_08929118(Cpu *c); void f_0887C614(Cpu *c); void f_0892F82C(Cpu *c); void f_0886E3CC(Cpu *c);
void f_08822D2C(Cpu *c); void f_08930930(Cpu *c); void f_089376A8(Cpu *c); void f_08936178(Cpu *c);
void f_0896F62C(Cpu *c); void f_08821330(Cpu *c); void f_08862004(Cpu *c); void f_0886E294(Cpu *c);

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

/* (fighter): the throw's step from 0x08A708B8 by +196, then the opponent kept posed (0x0886E17C). */
SFA_FUNC(08937248, throw_step_pose) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t fn = *W(0x08A708B8u + ((uint32_t)(int32_t)S8(o + 196) << 2));
    ARG(0) = o; ARG(1) = fn;
    c->r[31] = 0x08937278u; rt_call_indirect(c, fn);
    call1(c, o, f_0886E17C, 0x08937280u);
    leave(c, sv);
}

/* (fighter): 0x08929118; in the air ticked; landed at strength 2 the first time (+828) step 6 into move
   0x35 (no choice made) or 0x29; else +7 steps on, at the floor, +73 cleared, cue 0x22 / 0x23 and move
   0x12. */
SFA_FUNC(089277F0, move_land_once_or_12) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08929118, 0x08927804u);
    if (c->r[2] != 0) call1(c, o, f_08861848, 0x08927814u);
    else if (S8(o + 194) == 2 && *W(o + 828) == 0) {
        *B(o + 7) = 6; *W(o + 828) = 1;
        if (S8(o + 386) < 0) call2(c, o, 0x35, f_088616EC, 0x08927854u);
        else call2(c, o, 0x29, f_088616EC, 0x08927868u);
    } else {
        uint32_t st = *B(o + 7);
        int32_t floor = S16(o + 132);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0;
        call1(c, o, f_0887C614, 0x08927890u);
        call2(c, o, 0x12, f_088616EC, 0x0892789Cu);
    }
    leave(c, sv);
}

/* (fighter): slides by +96 away from its facing (+96 by +104); once that turns step 3, x stopped and
   placed at 0x2E0 / 0x220 by its side, move 0x31; else ticked, 0x0892F82C and 0x0886E3CC (7, 4). */
SFA_FUNC(0892F6A8, move_throw_slide_place) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    *W(o + 16) = *B(o + 11) == 0 ? x + v : x - v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if ((int32_t)nv < 0) {
        *B(o + 7) = 3; *W(o + 104) = 0;
        int32_t side = S8(o + 337);
        *W(o + 96) = 0; *H(o + 16) = 0;
        *H(o + 18) = (side & 1) ? 0x2E0 : 0x220;
        call2(c, o, 0x31, f_0886163C, 0x0892F72Cu);
    } else {
        call1(c, o, f_08861848, 0x0892F73Cu);
        call1(c, o, f_0892F82C, 0x0892F744u);
        ARG(0) = o; ARG(1) = 7; ARG(2) = 4; c->r[31] = 0x0892F754u; f_0886E3CC(c);
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag steps +7 on and spawns the held opponent's puppet (0x08822D2C 0xD / 1,
   set up by 0x08930930): +346 by 3, +255 = 3, +347 = 1, +112 = strength + 2, +260 from 0x08A6DB84 by
   strength, +311 = 2, +312 = 8. */
SFA_FUNC(089313AC, move_flag_puppet_hit) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x089313C4u);
    if (S8(o + 77) != 0) {
        bump(o + 7);
        ARG(0) = o; ARG(1) = 0xD; ARG(2) = 1; c->r[31] = 0x089313ECu; f_08822D2C(c);
        gaddr e = c->r[2];
        c->r[17] = e;
        if (e != 0) {
            ARG(0) = o; ARG(1) = e; c->r[31] = 0x08931400u; f_08930930(c);
            *B(e + 346) = (uint8_t)(S8(e + 346) + 3);
            *B(e + 255) = 3; *B(e + 347) = 1;
            *H(e + 112) = (uint16_t)(S8(o + 194) + 2);
            int32_t s = S8(o + 194);
            *B(e + 260) = (uint8_t)S16(0x08A6DB84u + (uint32_t)(s + s));
            *B(e + 311) = 2; *B(e + 312) = 8;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 0x089376A8; below the floor 0x08936178; 48 or more above it facing the way +238 asks
   (2 facing left, 1 right) +7 steps on with the y speeds cleared, move 0x29 and cue 0x22 / 0x23;
   else ticked. */
SFA_FUNC(089360BC, move_air_turn_29) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089376A8, 0x089360D0u);
    int32_t floor = S16(o + 132), y = S16(o + 22);
    if (y < floor) call1(c, o, f_08936178, 0x08936168u);
    else {
        int32_t lim = (int16_t)(floor + 0x30);
        int go = 0;
        if (!((y & 0xFFFF) < (lim & 0xFFFF))) go = S8(o + 238) == (*B(o + 11) == 0 ? 2 : 1);
        if (go) {
            uint32_t st = *B(o + 7);
            *W(o + 108) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 100) = 0;
            call2(c, o, 0x29, f_088616EC, 0x08936148u);
            call1(c, o, f_0887C614, 0x08936150u);
        } else call1(c, o, f_08861848, 0x08936124u);
    }
    leave(c, sv);
}

/* (fighter): 0x0896F62C; falling at or below the floor +7 steps on, at the floor, +20 / +73 cleared,
   x set beside 0x08821330's object (96 or 288 by its side) and move 4; else ticked. */
SFA_FUNC(0896B634, move_land_beside_4) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_0896F62C, 0x0896B648u);
    int32_t floor = S16(o + 132);
    if ((int32_t)*W(o + 100) < 0 && !((floor & 0xFFFF) < (S16(o + 22) & 0xFFFF))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0; *B(o + 73) = 0;
        call1(c, o, f_08821330, 0x0896B690u);
        int32_t side = S8(o + 337);
        *W(c->r[29] + 16) = 0;
        uint32_t x = *W(c->r[2] + 16);
        *W(o + 16) = x + (side != 0 ? 0x1200000u : 0x600000u);
        call2(c, o, 4, f_0886163C, 0x0896B6C8u);
    } else call1(c, o, f_08861848, 0x0896B6E0u);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked on a frame flag; else in the air (+73 = 1) until 0x08862004 lands it: +7 on, +73 /
   +20 / +262 cleared, at the floor, cue 0x22 / 0x23, 0x0886E3CC (10, 10), move 0x33 + strength and
   +74 by strength - 1. */
SFA_FUNC(089766FC, move_air_land_33) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) call1(c, o, f_08861848, 0x0897671Cu);
    else {
        *B(o + 73) = 1;
        call1(c, o, f_08862004, 0x08976734u);
        if (c->r[2] == 0) call1(c, o, f_08861848, 0x08976744u);
        else {
            uint32_t st = *B(o + 7);
            int32_t floor = S16(o + 132);
            *B(o + 7) = (uint8_t)(st + 1);
            *B(o + 73) = 0; *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 262) = 0;
            call1(c, o, f_0887C614, 0x08976770u);
            ARG(0) = o; ARG(1) = 0xA; ARG(2) = 0xA; c->r[31] = 0x08976780u; f_0886E3CC(c);
            call2(c, o, (uint32_t)(S8(o + 194) + 0x33) & 0xFFFF, f_088616EC, 0x08976794u);
            *B(o + 74) = (uint8_t)(S8(o + 74) + S8(o + 194) - 1);
        }
    }
    leave(c, sv);
}

/* (fighter): +543 = 2, ticked; without a negative frame flag +7 steps on, +237 cleared, +573 from
   0x08A966BC (by strength, or by 0x0886E294 with +81), +73 = 1 and the four speeds by strength
   (0x08A966C0: x, x brake, y, fall). */
SFA_FUNC(0898FD5C, move_tick_jump_573) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 543) = 2;
    call1(c, o, f_08861848, 0x0898FD7Cu);
    if (!(S8(o + 77) & 0x80)) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, 0x0898FDA4u);
        int32_t s = S8(o + 194);
        uint32_t i = S8(o + 81) == 0 ? (uint32_t)s : c->r[2];
        *B(o + 573) = *B(0x08A966BCu + i);
        *B(o + 73) = 1;
        gaddr e = 0x08A966C0u + ((uint32_t)s << 4);
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    leave(c, sv);
}
