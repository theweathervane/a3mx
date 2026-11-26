/* More special-move follow-ups on a frame flag that call an effect: the spawner 0x0887FD2C, or 0x088D3558
   (0x0887D144 then the spawner), ticked by 0x08861848 or 0x088D3EC0. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0887FD2C(Cpu *c); void f_088D3558(Cpu *c); void f_088D3EC0(Cpu *c);
void f_088370D8(Cpu *c); void f_088F1DBC(Cpu *c);

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

/* the frame flag: +77 or +76, set (F_SET), below zero (F_NEG) or exactly 1 (F_ONE) */
enum { F_SET = 0, F_NEG = 0x100, F_ONE = 0x200 };
/* +237: -1, toward +81 (1 / -1) or toward +81 but 0 (0 / -1) */
enum { D_NEG, D_81, D_81_0 };
enum { T_AFTER, T_BEFORE };
/* the effect (0x0887FD2C or 0x088D3558) and the tick (0x08861848 or 0x088D3EC0) */
enum { X_FD2C = 0, X_3558 = 1 };
typedef struct {
    uint16_t flag; uint8_t dir, tick, kind, pre543; int16_t a, b; uint32_t bc, bt;
} CueStep;

static int flag_on(gaddr o, uint16_t flag) {
    int32_t f = S8(o + (flag & 0xFF));
    if (flag & F_NEG) return f < 0;
    if (flag & F_ONE) return f == 1;
    return f != 0;
}
static uint8_t dir237(gaddr o, uint8_t dir) {
    if (dir == D_NEG) return 0xFF;
    if (S8(o + 81) == 0) return 0xFF;
    return dir == D_81 ? 1 : 0;
}
static void cue_step(Cpu *c, const CueStep *k) {
    gaddr o = ARG(0); uint32_t sv[2];
    void (*tick)(Cpu *) = k->kind == X_3558 ? f_088D3EC0 : f_08861848;
    if (k->pre543) *B(o + 543) = 2;
    enter(c, o, sv);
    if (k->tick == T_BEFORE) call1(c, o, tick, k->bt);
    if (flag_on(o, k->flag)) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = dir237(o, k->dir);
        fx(c, k->kind == X_3558 ? f_088D3558 : f_0887FD2C, o, k->a, k->b, k->bc);
    }
    if (k->tick == T_AFTER) call1(c, o, tick, k->bt);
    leave(c, sv);
}

SFA_FUNC(089BBAA0, fx3558_20) {
    static const CueStep k = { 77, D_NEG, T_AFTER, X_3558, 0, 0x20, 0x50, 0x089BBADCu, 0x089BBAE4u }; cue_step(c, &k);
}
SFA_FUNC(089C1F34, fx3558_m14) {
    static const CueStep k = { 77, D_NEG, T_AFTER, X_3558, 0, -0x14, 0x44, 0x089C1F70u, 0x089C1F78u }; cue_step(c, &k);
}
SFA_FUNC(089C21E0, fx3558_m10) {
    static const CueStep k = { 77, D_NEG, T_AFTER, X_3558, 0, -0x10, 0x48, 0x089C221Cu, 0x089C2224u }; cue_step(c, &k);
}
SFA_FUNC(089C88DC, fx3558_18) {
    static const CueStep k = { 76, D_NEG, T_BEFORE, X_3558, 0, 0x18, 0xC, 0x089C8920u, 0x089C88F0u }; cue_step(c, &k);
}
SFA_FUNC(089B68EC, fx3558_one_c) {
    static const CueStep k = { 76 | F_ONE, D_NEG, T_BEFORE, X_3558, 0, 0xC, 0x44, 0x089B6934u, 0x089B6900u };
    cue_step(c, &k);
}
SFA_FUNC(08991400, fx_neg_me_b) {
    static const CueStep k = { 77 | F_NEG, D_81, T_BEFORE, X_FD2C, 0, -0xE, 0x3C, 0x0899145Cu, 0x08991414u };
    cue_step(c, &k);
}
SFA_FUNC(089B1E70, fx543_neg_m15) {
    static const CueStep k = { 77 | F_NEG, D_81_0, T_AFTER, X_FD2C, 1, -0x15, 0x58, 0x089B1EC4u, 0x089B1ECCu };
    cue_step(c, &k);
}
SFA_FUNC(089B2294, fx543_neg_23) {
    static const CueStep k = { 77 | F_NEG, D_81_0, T_AFTER, X_FD2C, 1, 0x23, 0x44, 0x089B22E8u, 0x089B22F0u };
    cue_step(c, &k);
}
SFA_FUNC(0895E704, fx543_neg_m6) {
    static const CueStep k = { 77 | F_NEG, D_81, T_AFTER, X_FD2C, 1, -6, 0x16, 0x0895E75Cu, 0x0895E764u };
    cue_step(c, &k);
}
SFA_FUNC(0896D93C, fx543_neg_m2a) {
    static const CueStep k = { 77 | F_NEG, D_81, T_AFTER, X_FD2C, 1, -0x2A, 0x2B, 0x0896D994u, 0x0896D99Cu };
    cue_step(c, &k);
}

/* (fighter): on +76 +7 on, +237 = -1, +840 = the strength, strength 2, 0x088370D8 and the effect at
   (-0x12, 0x4D); ticked by 0x088D3EC0. */
SFA_FUNC(089C8D78, fx3558_s2_m12) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 76) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        int32_t s = S8(o + 194);
        *B(o + 237) = 0xFF;
        *W(o + 840) = (uint32_t)s; *B(o + 194) = 2;
        call1(c, o, f_088370D8, 0x089C8DBCu);
        fx(c, f_088D3558, o, -0x12, 0x4D, 0x089C8DCCu);
    }
    call1(c, o, f_088D3EC0, 0x089C8DD4u);
    leave(c, sv);
}

/* (fighter): on +77 +7 on, +237 toward +81, 0x088F1DBC and the effect at (0x28, 0x3C); ticked. */
SFA_FUNC(088F02B4, fx_1dbc_28) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF;
        call1(c, o, f_088F1DBC, 0x088F02FCu);
        fx(c, f_0887FD2C, o, 0x28, 0x3C, 0x088F030Cu);
    }
    call1(c, o, f_08861848, 0x088F0314u);
    leave(c, sv);
}

/* (fighter): +82 down; on +77 it clears, +7 on, +237 toward +81 and the effect at (0x11, 0x40); ticked. */
SFA_FUNC(089312B8, fx_count_11) {
    gaddr o = ARG(0); uint32_t sv[2];
    *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
    int32_t f = S8(o + 77);
    enter(c, o, sv);
    if (f != 0) {
        *B(o + 77) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF;
        fx(c, f_0887FD2C, o, 0x11, 0x40, 0x08931310u);
    }
    call1(c, o, f_08861848, 0x08931318u);
    leave(c, sv);
}

/* (fighter): ticked; on +77 first +7 on, +237 toward +81 and the effect at (-0x24, 0x3E). */
SFA_FUNC(089770CC, fx_m24_3e) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089770ECu);
    else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF;
        fx(c, f_0887FD2C, o, -0x24, 0x3E, 0x08977124u);
        call1(c, o, f_08861848, 0x0897712Cu);
    }
    leave(c, sv);
}
