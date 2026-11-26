/* More special-move starts with their meter change (0x0886CA2C / 0x088D34F4), one that spends meter
   (0x088F096C). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_088370D8(Cpu *c); void f_0887D144(Cpu *c); void f_088D34F4(Cpu *c);

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
static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static uint32_t by_str(gaddr o, uint32_t base) { return (uint32_t)(S8(o + 194) + (int32_t)base) & 0xFFFF; }

/* (fighter): +7 on, 144 meter spent, 0x088370D8, 0x0887D144 and move 0x2A. */
SFA_FUNC(088F096C, super_start_2a) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    bump(o + 7);
    call2(c, o, 0xFFFFFF70u, f_0886CA2C, 0x088F0990u);
    call1(c, o, f_088370D8, 0x088F0998u);
    call1(c, o, f_0887D144, 0x088F09A0u);
    call2(c, o, 0x2A, f_088616EC, 0x088F09ACu);
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, +696 cleared, the meter by 1, 0x08837098 and move 0x2E. */
SFA_FUNC(088FA4D4, special_start_2e) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 7) = 1; *B(o + 696) = 0;
    call2(c, o, 1, f_0886CA2C, 0x088FA500u);
    call1(c, o, f_08837098, 0x088FA508u);
    call2(c, o, 0x2E, f_088616EC, 0x088FA514u);
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, the meter by 4, 0x08837098 and move `base` + strength. */
static void special_step1(Cpu *c, uint32_t base, uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, 4, f_0886CA2C, b0);
    call1(c, o, f_08837098, b1);
    call2(c, o, by_str(o, base), f_088616EC, b2);
    leave(c, sv);
}
SFA_FUNC(088F37A4, special_step1_14) { special_step1(c, 0x14, 0x088F37CCu, 0x088F37D4u, 0x088F37E8u); }
SFA_FUNC(088F837C, special_step1_15) { special_step1(c, 0x15, 0x088F83A4u, 0x088F83ACu, 0x088F83C0u); }

/* (fighter): +7 on, facing the opponent, +235 = 1, the meter by 1 and move 8 of its list. */
SFA_FUNC(089259A4, special_face_8) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call2(c, o, 1, f_0886CA2C, 0x089259DCu);
    call2(c, o, 8, f_08861674, 0x089259E8u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 5, 0x088370D8 and move 0. */
SFA_FUNC(089A6CF8, special_start_0) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    bump(o + 7); *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x089A6D28u);
    call1(c, o, f_088370D8, 0x089A6D30u);
    call2(c, o, 0, f_088616EC, 0x089A6D3Cu);
    leave(c, sv);
}

/* (fighter): step 1, +262 = 1, the meter by strength + 4 and move 8 + strength. */
SFA_FUNC(089A712C, special_start_8) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1; *B(o + 262) = 1;
    call2(c, o, (uint32_t)(S8(o + 194) + 4), f_0886CA2C, 0x089A715Cu);
    call2(c, o, by_str(o, 8), f_088616EC, 0x089A7170u);
    leave(c, sv);
}

/* (character): +7 on, the meter by 4, 0x08837098, move `base` + strength and +262 = 1. */
static void part_special(Cpu *c, int32_t base, uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    bump(o + 7);
    call2(c, o, 4, f_088D34F4, b0);
    call1(c, o, f_08837098, b1);
    call2(c, o, (uint32_t)(S8(o + 194) + base), f_088616EC, b2);
    *B(o + 262) = 1;
    leave(c, sv);
}
SFA_FUNC(089B5B28, part_special_6) { part_special(c, 6, 0x089B5B4Cu, 0x089B5B54u, 0x089B5B64u); }
SFA_FUNC(089C7984, part_special_4) { part_special(c, 4, 0x089C79A8u, 0x089C79B0u, 0x089C79C0u); }
