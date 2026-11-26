/* Special-move follow-ups on a frame flag that call the effect spawner 0x0887FD2C, with a count, a
   push or a second effect (0x0887FC70). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0887FD2C(Cpu *c); void f_0887FC70(Cpu *c); void f_0886D468(Cpu *c);
void f_088370D8(Cpu *c); void f_0887D144(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void fx(Cpu *c, void (*fn)(Cpu *), gaddr o, int32_t a, int32_t b, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)a; ARG(2) = (uint32_t)b; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }
static void face81(gaddr o) { *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF; }
/* +7 on, +237 toward +81 and the effect at (a, b) */
static void go(Cpu *c, gaddr o, int32_t a, int32_t b, uint32_t back) {
    step_on(o); face81(o); fx(c, f_0887FD2C, o, a, b, back);
}

/* (fighter): on either frame flag (+76 / +77) the effect at (a, b); ticked. */
static void either_flag(Cpu *c, int32_t a, int32_t b, uint32_t bc, uint32_t bt) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if ((S8(o + 76) | S8(o + 77)) != 0) go(c, o, a, b, bc);
    call1(c, o, f_08861848, bt);
    leave(c, sv);
}
SFA_FUNC(088FE288, fx_either_m18) { either_flag(c, -0x18, 0x28, 0x088FE2E4u, 0x088FE2ECu); }
SFA_FUNC(0899BDF0, fx_either_ma) { either_flag(c, -0xA, 0x67, 0x0899BE4Cu, 0x0899BE54u); }
SFA_FUNC(0899C254, fx_either_30) { either_flag(c, 0x30, 0x3B, 0x0899C2B0u, 0x0899C2B8u); }

/* (fighter): +543 = 2; with +77 below zero the effect at (-6, 0x16); ticked. */
SFA_FUNC(0898FCE8, fx543_neg_m6b) {
    gaddr o = ARG(0); uint32_t sv[2];
    *B(o + 543) = 2;
    int32_t f = S8(o + 77);
    enter(c, o, sv);
    if (f & 0x80) go(c, o, -6, 0x16, 0x0898FD44u);
    call1(c, o, f_08861848, 0x0898FD4Cu);
    leave(c, sv);
}

/* (fighter): +82 down; on +77 step 2, +237 toward +81, the effect at (0x1E, 0x30) and 0x0887FC70 at
   (-0x2C, 0x30); ticked. */
SFA_FUNC(088F8EF0, fx_two_1e) {
    gaddr o = ARG(0); uint32_t sv[2];
    *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
    int32_t f = S8(o + 77);
    enter(c, o, sv);
    if (f != 0) {
        *B(o + 7) = 2; face81(o);
        fx(c, f_0887FD2C, o, 0x1E, 0x30, 0x088F8F40u);
        fx(c, f_0887FC70, o, -0x2C, 0x30, 0x088F8F50u);
    }
    call1(c, o, f_08861848, 0x088F8F58u);
    leave(c, sv);
}

/* (fighter): ticked; on +77 first the effect at (0xF, 0x4E). */
SFA_FUNC(088FE018, fx_f_4e) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x088FE038u);
    else { go(c, o, 0xF, 0x4E, 0x088FE078u); call1(c, o, f_08861848, 0x088FE080u); }
    leave(c, sv);
}

/* (fighter): +83 down; on +77 it clears and the effect at (0, 0x61); ticked. */
SFA_FUNC(0893EFA8, fx_83_0_61) {
    gaddr o = ARG(0); uint32_t sv[2];
    *B(o + 0x53) = (uint8_t)(S8(o + 0x53) - 1);
    int32_t f = S8(o + 77);
    enter(c, o, sv);
    if (f != 0) {
        step_on(o); *B(o + 77) = 0; face81(o);
        fx(c, f_0887FD2C, o, 0, 0x61, 0x0893F014u);
    }
    call1(c, o, f_08861848, 0x0893F01Cu);
    leave(c, sv);
}

/* (fighter): +7 on, the gain by distance, 0x088370D8, 0x0887D144, +83 = 0x3C, +237 toward +81, the effect
   at (0, 0x28) and ticked. */
SFA_FUNC(0896E68C, fx_start_0_28) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o);
    call1(c, o, f_0886D468, 0x0896E6ACu);
    call1(c, o, f_088370D8, 0x0896E6B4u);
    call1(c, o, f_0887D144, 0x0896E6BCu);
    *B(o + 0x53) = 0x3C;
    face81(o);
    fx(c, f_0887FD2C, o, 0, 0x28, 0x0896E6F8u);
    call1(c, o, f_08861848, 0x0896E700u);
    leave(c, sv);
}

/* (fighter): +82 down; on +77 +7 on, +237 toward +81, x 4.5 toward its facing (`pre` first sets +821 =
   0x30) and the effect at (-0x10, 0x58); ticked. */
static void fx_push(Cpu *c, int pre, uint32_t bc, uint32_t bt) {
    gaddr o = ARG(0); uint32_t sv[2];
    *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
    int32_t f = S8(o + 77);
    enter(c, o, sv);
    if (f != 0) {
        step_on(o); face81(o);
        if (pre) *B(o + 0x334 + 5) = 0x30;
        *W(o + 96) = *B(o + 11) != 0 ? 0x48000u : 0xFFFB8000u;
        fx(c, f_0887FD2C, o, -0x10, 0x58, bc);
    }
    call1(c, o, f_08861848, bt);
    leave(c, sv);
}
SFA_FUNC(08964FE4, fx_push_m10) { fx_push(c, 0, 0x08965054u, 0x0896505Cu); }
SFA_FUNC(088F9F38, fx_push_m10_30) { fx_push(c, 1, 0x088F9FB4u, 0x088F9FBCu); }
