/* Special-move starts on the meter gain by distance (0x0886D468): two that start once (+196) and then
   step through a table of handlers, and two that set a rise or part flags. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886D468(Cpu *c); void f_088370D8(Cpu *c); void f_0887D144(Cpu *c);
void f_0886DDCC(Cpu *c); void f_0887D020(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }
static void move(Cpu *c, gaddr o, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; f_088616EC(c);
}
/* the handler for the current step from `tab` */
static void by_step(Cpu *c, gaddr o, gaddr tab, uint32_t back) {
    uint32_t fn = *W(tab + ((uint32_t)*B(o + 7) << 2));
    ARG(0) = o; ARG(1) = fn; c->r[31] = back; rt_call_indirect(c, fn);
}

/* (fighter): the first time (+196) 0x0887D144, +826 cleared, the gain, 0x088370D8 and move 0x23; after
   that the step's handler from 0x08A5FF94 and 0x0886DDCC (the opponent held in place). */
SFA_FUNC(08915ECC, held_start_23) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 196) == 0) {
        *B(o + 196) = 1;
        call1(c, o, f_0887D144, 0x08915EF4u);
        *B(o + 0x334 + 6) = 0;
        call1(c, o, f_0886D468, 0x08915F04u);
        call1(c, o, f_088370D8, 0x08915F0Cu);
        move(c, o, 0x23, 0x08915F18u);
    } else {
        by_step(c, o, 0x08A5FF94u, 0x08915F40u);
        call1(c, o, f_0886DDCC, 0x08915F48u);
    }
    leave(c, sv);
}

/* (fighter): the first time (+196) 0x0887D144, +824 cleared, the gain, 0x088370D8 and move strength +
   0x2B; after that the step's handler from 0x08A5FFC0. */
SFA_FUNC(08916398, once_start_2b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 196) == 0) {
        *B(o + 196) = 1;
        call1(c, o, f_0887D144, 0x089163C0u);
        *B(o + 0x334 + 4) = 0;
        call1(c, o, f_0886D468, 0x089163D0u);
        call1(c, o, f_088370D8, 0x089163D8u);
        move(c, o, (uint32_t)(S8(o + 194) + 0x2B) & 0xFFFF, 0x089163ECu);
    } else by_step(c, o, 0x08A5FFC0u, 0x08916414u);
    leave(c, sv);
}

/* (fighter): +7 on, +696 / +260 cleared, +543 = 2, +83 = 0xC, 0x0887D144, the gain, 0x088370D8, y's low
   half (strength + 1) x 256 and move strength + 0x25. */
SFA_FUNC(089B1DE4, start_rise_25) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 696) = 0; *B(o + 543) = 2; *B(o + 260) = 0; *B(o + 0x53) = 0xC;
    call1(c, o, f_0887D144, 0x089B1E28u);
    call1(c, o, f_0886D468, 0x089B1E30u);
    call1(c, o, f_088370D8, 0x089B1E38u);
    *H(o + 0x64) = (uint16_t)((S8(o + 194) + 1) << 8);
    move(c, o, (uint32_t)(S8(o + 194) + 0x25) & 0xFFFF, 0x089B1E60u);
    leave(c, sv);
}

/* (fighter): +656 cleared, +7 on, the gain, 0x088370D8, +644 = 2, +654 = -1, +78 cleared, +937 and +83
   by strength from 0x08A66DDC, +502 = 1, +935 / +936 / +237 cleared, 0x0887D020 and move strength + 0x26. */
SFA_FUNC(08921E80, start_parts_26) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 656) = 0; step_on(o);
    call1(c, o, f_0886D468, 0x08921EA8u);
    call1(c, o, f_088370D8, 0x08921EB0u);
    const gaddr tab = 0x08A66DDCu;
    int32_t s = S8(o + 194);
    *B(o + 644) = 2; *B(o + 654) = 0xFF; *B(o + 78) = 0;
    *B(o + 0x3A0 + 9) = *B(tab + (uint32_t)(s << 1));
    *B(o + 0x53) = *B(tab + (uint32_t)(S8(o + 194) << 1) + 1);
    *B(o + 502) = 1;
    *B(o + 0x3A0 + 7) = 0; *B(o + 0x3A0 + 8) = 0; *B(o + 237) = 0;
    call1(c, o, f_0887D020, 0x08921F1Cu);
    move(c, o, (uint32_t)(S8(o + 194) + 0x26) & 0xFFFF, 0x08921F30u);
    leave(c, sv);
}
