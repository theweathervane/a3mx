/* Special-move follow-ups that call the effect spawner 0x0887FD2C on a frame flag: a step on, +237 set
   toward +81 (or -1) and the effect at an offset. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0887FD2C(Cpu *c); void f_0887D144(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void cue(Cpu *c, gaddr o, int32_t a, int32_t b, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)a; ARG(2) = (uint32_t)b; c->r[31] = back; f_0887FD2C(c);
}

/* how a follow-up runs: the frame flag it waits on (+77 or +76; F_NEG waits for it below zero), the step
   it sets (0 = one on), +237 = -1 or toward +81, and whether it ticks after the check, before it or not */
enum { F_NEG = 0x100 };
enum { T_AFTER, T_BEFORE };
typedef struct { uint16_t flag; uint8_t step, by81, tick; int16_t a, b; uint32_t bc, bt; } CueStep;

static void cue_step(Cpu *c, const CueStep *k) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (k->tick == T_BEFORE) call1(c, o, f_08861848, k->bt);
    int32_t f = S8(o + (k->flag & 0xFF));
    if ((k->flag & F_NEG) ? f < 0 : f != 0) {
        *B(o + 7) = k->step ? k->step : (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = !k->by81 || S8(o + 81) == 0 ? 0xFF : 1;
        cue(c, o, k->a, k->b, k->bc);
    }
    if (k->tick == T_AFTER) call1(c, o, f_08861848, k->bt);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

SFA_FUNC(088F09BC, fx_step_13) {
    static const CueStep k = { 77, 0, 0, T_AFTER, 0x13, 0x3C, 0x088F09F8u, 0x088F0A00u }; cue_step(c, &k);
}
SFA_FUNC(089121D0, fx_step_a) {
    static const CueStep k = { 77, 0, 0, T_AFTER, 0xA, 0x58, 0x0891220Cu, 0x08912214u }; cue_step(c, &k);
}
SFA_FUNC(089A7B44, fx_step_m30) {
    static const CueStep k = { 76, 0, 0, T_AFTER, -0x30, 0x48, 0x089A7B80u, 0x089A7B88u }; cue_step(c, &k);
}
SFA_FUNC(08903C70, fx_step2_32) {
    static const CueStep k = { 76, 2, 1, T_BEFORE, 0x32, 0x2F, 0x08903CB8u, 0x08903C84u }; cue_step(c, &k);
}
SFA_FUNC(08903F20, fx_step2_m1a) {
    static const CueStep k = { 77, 2, 1, T_BEFORE, -0x1A, 0x33, 0x08903F68u, 0x08903F34u }; cue_step(c, &k);
}
SFA_FUNC(088F4D54, fx_step2_1a) {
    static const CueStep k = { 77, 2, 1, T_AFTER, 0x1A, 0x1A, 0x088F4D98u, 0x088F4DA0u }; cue_step(c, &k);
}
SFA_FUNC(0892BA4C, fx_step2_m10) {
    static const CueStep k = { 77, 2, 1, T_AFTER, -0x10, 0x2E, 0x0892BA90u, 0x0892BA98u }; cue_step(c, &k);
}
SFA_FUNC(0892C2C8, fx_step2_3) {
    static const CueStep k = { 77, 2, 1, T_AFTER, 3, 0x55, 0x0892C30Cu, 0x0892C314u }; cue_step(c, &k);
}
SFA_FUNC(089075D8, fx_step_m10) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x10, 0x43, 0x08907620u, 0x08907628u }; cue_step(c, &k);
}
SFA_FUNC(08931914, fx_step_m1a) {
    static const CueStep k = { 77, 0, 1, T_BEFORE, -0x1A, 0x44, 0x08931964u, 0x08931928u }; cue_step(c, &k);
}
SFA_FUNC(089742FC, fx_step_m1a_48) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x1A, 0x48, 0x08974344u, 0x0897434Cu }; cue_step(c, &k);
}
SFA_FUNC(08945CDC, fx_step2_m13) {
    static const CueStep k = { 76, 2, 1, T_AFTER, -0x13, 0x53, 0x08945D28u, 0x08945D30u }; cue_step(c, &k);
}
SFA_FUNC(08927480, fx_step_m28) {
    static const CueStep k = { 76, 0, 1, T_AFTER, -0x28, 0x62, 0x089274D0u, 0x089274D8u }; cue_step(c, &k);
}
SFA_FUNC(0895ED04, fx_step_neg_me) {
    static const CueStep k = { 77 | F_NEG, 0, 1, T_BEFORE, -0xE, 0x3C, 0x0895ED5Cu, 0x0895ED18u }; cue_step(c, &k);
}
SFA_FUNC(089681D8, fx_step_m26) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x26, 0x35, 0x08968228u, 0x08968230u }; cue_step(c, &k);
}
SFA_FUNC(08969768, fx_step_m48) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x48, 0x42, 0x089697B8u, 0x089697C0u }; cue_step(c, &k);
}
SFA_FUNC(089A44BC, fx_step_m19) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x19, 0x37, 0x089A450Cu, 0x089A4514u }; cue_step(c, &k);
}
SFA_FUNC(089A47F0, fx_step_m24) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x24, 0x1F, 0x089A4840u, 0x089A4848u }; cue_step(c, &k);
}
SFA_FUNC(089A4B44, fx_step_m19b) {
    static const CueStep k = { 77, 0, 1, T_AFTER, -0x19, 0x37, 0x089A4B94u, 0x089A4B9Cu }; cue_step(c, &k);
}

/* (fighter): +543 = 2, then a follow-up like the rest. */
static void cue_step_543(Cpu *c, const CueStep *k) { *B(ARG(0) + 543) = 2; cue_step(c, k); }
SFA_FUNC(08931544, fx_543_step2_f) {
    static const CueStep k = { 77, 2, 1, T_AFTER, 0xF, 0x44, 0x08931590u, 0x08931598u }; cue_step_543(c, &k);
}
SFA_FUNC(0896547C, fx_543_step2_m12) {
    static const CueStep k = { 77, 2, 1, T_AFTER, -0x12, 0x30, 0x089654C8u, 0x089654D0u }; cue_step_543(c, &k);
}

/* (fighter, a, b): 0x0887D144 and the effects at (a, b). */
SFA_FUNC(088D3558, fx_after_d144) {
    gaddr o = ARG(0);
    int32_t a = (int16_t)ARG(1), b = (int16_t)ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o; c->r[17] = (uint32_t)a; c->r[18] = (uint32_t)b;
    call1(c, o, f_0887D144, 0x088D3588u);
    cue(c, o, a, b, 0x088D3598u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): on a frame flag +7 on, +237 toward +81 and the effects at (-0x10, 0x43); then ticked, on
   whatever object a0 holds after the spawner (the original keeps no copy of the fighter). */
SFA_FUNC(089072BC, fx_step_m10_a0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF;
        cue(c, o, -0x10, 0x43, 0x089072FCu);
    }
    c->r[31] = 0x08907310u; f_08861848(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): +82 counts down; once out +7 on, +82 = 0x38, +237 toward +81 and the effects at (-0x28,
   0x3C). */
SFA_FUNC(08911250, fx_after_count) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int32_t t = S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *H(o + 82) = 0x38;
        *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF;
        cue(c, o, -0x28, 0x3C, 0x089112ACu);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
