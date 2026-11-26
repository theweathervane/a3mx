/* Special-move starts that charge the super meter (0x0886CA2C): +7 steps on (or a set step) with +262 =
   1, the meter by a fixed amount, 0x08837098 and the move; plus the meter's 16-bit entry (0x088D34F4).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);

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

/* (fighter, amount): the meter gain with the amount as a 16-bit value. */
SFA_FUNC(088D34F4, meter_gain16) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(1) = (uint32_t)(int16_t)ARG(1);
    c->r[31] = 0x088D3508u; f_0886CA2C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 steps on, +262 = 1, the meter by `gain`, 0x08837098 and move `base` + strength. */
static void special_start(Cpu *c, uint32_t gain, uint32_t base, uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 262) = 1;
    call2(c, o, gain, f_0886CA2C, b0);
    call1(c, o, f_08837098, b1);
    call2(c, o, (uint32_t)(S8(o + 194) + (int32_t)base) & 0xFFFF, f_088616EC, b2);
    leave(c, sv);
}
SFA_FUNC(089115C4, special_start_15c4) { special_start(c, 9, 0x11, 0x089115F4u, 0x089115FCu, 0x08911610u); }
SFA_FUNC(08916F3C, special_start_6f3c) { special_start(c, 5, 0x1C, 0x08916F6Cu, 0x08916F74u, 0x08916F88u); }
SFA_FUNC(08926C10, special_start_6c10) { special_start(c, 4, 0x2, 0x08926C40u, 0x08926C48u, 0x08926C5Cu); }
SFA_FUNC(08926D48, special_start_6d48) { special_start(c, 4, 0x5, 0x08926D78u, 0x08926D80u, 0x08926D94u); }
SFA_FUNC(0893E11C, special_start_e11c) { special_start(c, 4, 0xB, 0x0893E14Cu, 0x0893E154u, 0x0893E168u); }
SFA_FUNC(0893E3B8, special_start_e3b8) { special_start(c, 6, 0xE, 0x0893E3E8u, 0x0893E3F0u, 0x0893E404u); }
SFA_FUNC(0893E57C, special_start_e57c) { special_start(c, 6, 0x12, 0x0893E5ACu, 0x0893E5B4u, 0x0893E5C8u); }

/* (fighter): move 0x15 + strength, the meter by 7 and 0x08837098. */
SFA_FUNC(08984CF8, special_start_15_m7) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call2(c, o, (uint32_t)(S8(o + 194) + 0x15), f_088616EC, 0x08984D14u);
    call2(c, o, 7, f_0886CA2C, 0x08984D20u);
    call1(c, o, f_08837098, 0x08984D28u);
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, the meter by 4, 0x08837098 and move 0x18. */
SFA_FUNC(088F9C14, special_start_18) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, 4, f_0886CA2C, 0x088F9C3Cu);
    call1(c, o, f_08837098, 0x088F9C44u);
    call2(c, o, 0x18, f_088616EC, 0x088F9C50u);
    leave(c, sv);
}

/* (fighter): +7 steps on, +262 = 1, the meter by 3 and move 3. */
SFA_FUNC(08969288, special_start_3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 262) = 1;
    call2(c, o, 3, f_0886CA2C, 0x089692B8u);
    call2(c, o, 3, f_088616EC, 0x089692C4u);
    leave(c, sv);
}

/* (fighter): +73 / +262 = 1, step 1, the meter by 6, 0x08837098, +696 cleared and move 0x15 +
   strength. */
SFA_FUNC(08931098, special_start_air_15) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 73) = 1; *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, 6, f_0886CA2C, 0x089310C4u);
    call1(c, o, f_08837098, 0x089310CCu);
    int32_t s = S8(o + 194);
    *B(o + 696) = 0;
    call2(c, o, (uint32_t)(s + 0x15) & 0xFFFF, f_088616EC, 0x089310E4u);
    leave(c, sv);
}
