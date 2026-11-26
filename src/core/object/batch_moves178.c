/* Move steps of 20 - 39 instructions: the stand-or-carry-on check (0x0885F79C, 10,878 calls) that picks
   a step from +771 unless the match is over, a part character's landing with a bank cue, jumps with
   their speeds by strength, a rise that counts a charge (0x088F8678) and a hold placed beside another
   object (0x08906FD8). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0893207C(Cpu *c);
void f_089320AC(Cpu *c); void f_08931D14(Cpu *c); void f_088D31EC(Cpu *c); void f_088D35D0(Cpu *c);
void f_088D36D0(Cpu *c); void f_088D2DBC(Cpu *c); void f_088D3960(Cpu *c); void f_088D2E04(Cpu *c);
void f_088D3EC0(Cpu *c); void f_088FABB8(Cpu *c); void f_088FE670(Cpu *c); void f_08821330(Cpu *c);

#define GAME  0x08E75910u
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

/* (fighter): 0x089320AC; above the floor 0x0893207C, else the landing 0x08931D14. */
SFA_FUNC(0893202C, move_air_or_land_1d14) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089320AC, 0x08932040u);
    if (S16(o + 132) < S16(o + 22)) call1(c, o, f_0893207C, 0x0893205Cu);
    else call1(c, o, f_08931D14, 0x0893206Cu);
    leave(c, sv);
}

/* (fighter): frame flag +76 starts a jump: +7 on, +73 / +77 cleared, the four speeds by strength
   (0x08A9D5A0: x, x brake toward the facing, y, fall); ticked. */
SFA_FUNC(089A3D3C, move_flag76_jump) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 76) != 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 0; *B(o + 77) = 0;
        gaddr e = 0x08A9D5A0u + ((uint32_t)s << 4);
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(e); *W(o + 104) = 0u - *W(e + 4); }
        else { *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); }
        *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    call1(c, o, f_08861848, 0x089A3DC8u);
    frame_out(c, ra);
}

/* (character): 0x088D31EC; when 0x088D35D0 gives 1 +7 steps on, 0x088D36D0, +73 cleared, cue 0x22,
   0x088D3960 (0, 0), move 9 + strength, bank cue 0x15 and +82 = 6; else the part ticked. */
SFA_FUNC(089B5C78, part_land_9) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D31EC, 0x089B5C8Cu);
    call1(c, o, f_088D35D0, 0x089B5C94u);
    if (c->r[2] == 1) {
        bump(o + 7);
        call1(c, o, f_088D36D0, 0x089B5CB4u);
        *B(o + 73) = 0;
        call1(c, o, f_088D2DBC, 0x089B5CC0u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089B5CD0u; f_088D3960(c);
        call2(c, o, (uint32_t)(S8(o + 194) + 9), f_088616EC, 0x089B5CE0u);
        call2(c, o, 0x15, f_088D2E04, 0x089B5CECu);
        *H(o + 82) = 6;
    } else call1(c, o, f_088D3EC0, 0x089B5D00u);
    leave(c, sv);
}

/* (fighter): standing once the match is decided (MATCH +14 clear with +79 below 7, or the game's +137
   set with +79 clear); else the step from 0x08A193A0 by +771. */
SFA_FUNC(0885F79C, fighter_stand_or_step) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    int stand;
    if (S8(MATCH + 14) == 0 && S8(MATCH + 79) < 7) stand = 1;
    else stand = S8(GAME + 137) != 0 && S8(MATCH + 79) == 0;
    if (stand) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        call2(c, o, 0, f_0886163C, 0x0885F804u);
    } else {
        gaddr base = 0x08A193A0u;
        uint32_t fn = *W(base + ((uint32_t)(int32_t)S8(o + 771) << 2));
        ARG(1) = fn; c->r[6] = base;
        c->r[31] = 0x0885F82Cu; rt_call_indirect(c, fn);
    }
    frame_out(c, ra);
}

/* (fighter): flies (+16 by +96, +20 by +100 by +108); once falling step 2, the charge (+0x338) set to 2 x
   strength (at least 1) and move 0xE + strength; else ticked. */
SFA_FUNC(088F8678, move_rise_charge_e) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    uint32_t vy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + vy;
    uint32_t nv = vy + *W(o + 108);
    *W(o + 100) = nv;
    if ((int32_t)nv < 0) {
        int32_t s = S8(o + 194);
        *B(o + 7) = 2;
        gaddr ch = o + 0x338;
        *B(ch) = (uint8_t)(s << 1);
        if ((int8_t)(s << 1) == 0) *B(ch) = (uint8_t)(S8(ch) + 1);
        call2(c, o, (uint32_t)(S8(o + 194) + 0xE) & 0xFFFF, f_088616EC, 0x088F86F4u);
    } else call1(c, o, f_08861848, 0x088F8708u);
    frame_out(c, ra);
}

/* (fighter): slides by +96 toward its facing (+96 by +104); once that turns +7 steps on, +262 cleared,
   +74 = 1 and move 0x35; else ticked until a frame flag, then 0x088FABB8. */
SFA_FUNC(088FAB1C, move_slide_35) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    *W(o + 16) = *B(o + 11) != 0 ? x + v : x - v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if ((int32_t)nv < 0) {
        bump(o + 7);
        *B(o + 262) = 0; *B(o + 74) = 1;
        call2(c, o, 0x35, f_088616EC, 0x088FAB78u);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x088FAB98u);
    else call1(c, o, f_088FABB8, 0x088FABACu);
    frame_out(c, ra);
}

/* (fighter): ticked; ended: +7 steps on with the four speeds by strength (0x08A4BAB0: x, x brake, y, fall)
   and move 0x1E + strength; else 0x088FE670 until a frame flag. */
SFA_FUNC(088FE380, move_end_jump_1e) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088FE394u);
    if (S8(o + 75) < 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        gaddr e = 0x08A4BAB0u + ((uint32_t)s << 4);
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
        call2(c, o, (uint32_t)(s + 0x1E) & 0xFFFF, f_088616EC, 0x088FE40Cu);
    } else if (S8(o + 77) == 0) call1(c, o, f_088FE670, 0x088FE3B4u);
    leave(c, sv);
}

/* (fighter): held at the x of 0x08821330's object (384 ahead when facing left); without a frame flag +7
   steps on, +78 = 1, x / y speeds by strength (0x08A539D0) with no brake or fall; ticked. */
SFA_FUNC(08906FD8, move_hold_beside) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08821330, 0x08906FECu);
    *W(o + 16) = *W(c->r[2] + 16);
    if (*B(o + 11) == 0) *W(o + 16) = *W(o + 16) + 0x1800000u;
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 78) = 1; *W(o + 108) = 0; *W(o + 104) = 0;
        gaddr e = 0x08A539D0u + ((uint32_t)s << 3);
        *W(o + 96) = *W(e); *W(o + 100) = *W(e + 4);
    }
    call1(c, o, f_08861848, 0x08907064u);
    leave(c, sv);
}
