/* Special-move starts with their meter change (0x0886CA2C / 0x088D34F4). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_088D34F4(Cpu *c); void f_08939BF4(Cpu *c); void f_08944300(Cpu *c); void f_08863114(Cpu *c);

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
static uint32_t by_str(gaddr o, uint32_t base) { return (uint32_t)(S8(o + 194) + (int32_t)base) & 0xFFFF; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* (fighter): +7 on, +262 = 1, the meter by 7, 0x08837098, +551 cleared and move 0xC + strength. */
SFA_FUNC(0896D1CC, special_start_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 7, f_0886CA2C, 0x0896D1FCu);
    call1(c, o, f_08837098, 0x0896D204u);
    int32_t s = S8(o + 194);
    *B(o + 551) = 0;
    call2(c, o, (uint32_t)(s + 0xC) & 0xFFFF, f_088616EC, 0x0896D21Cu);
    leave(c, sv);
}

/* (fighter): +7 on, +696 cleared, +262 = 1, the meter by 7, 0x08837098 and move 0x14 + strength. */
SFA_FUNC(0898FA74, special_start_14_696) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 696) = 0; *B(o + 262) = 1;
    call2(c, o, 7, f_0886CA2C, 0x0898FAA8u);
    call1(c, o, f_08837098, 0x0898FAB0u);
    call2(c, o, by_str(o, 0x14), f_088616EC, 0x0898FAC4u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 9, 0x08837098 and move 0xB + 2 x strength. */
SFA_FUNC(0899BAF0, special_start_b_2k) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 9, f_0886CA2C, 0x0899BB20u);
    call1(c, o, f_08837098, 0x0899BB28u);
    call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0xB) & 0xFFFF, f_088616EC, 0x0899BB40u);
    leave(c, sv);
}

/* (fighter): +7 on, +502 / +262 = 1, the meter by 2, 0x08837098 and move 8 + strength. */
SFA_FUNC(0899BCC0, special_start_8_502_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 502) = 1; *B(o + 262) = 1;
    call2(c, o, 2, f_0886CA2C, 0x0899BCF4u);
    call1(c, o, f_08837098, 0x0899BCFCu);
    call2(c, o, by_str(o, 8), f_088616EC, 0x0899BD10u);
    leave(c, sv);
}

/* (character): +696 cleared, +7 on, +820 cleared, the meter by 4, 0x08837098, move 0xF + strength and
   +262 = 1. */
SFA_FUNC(089B6040, part_special_f_820) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 696) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 820) = 0;
    call2(c, o, 4, f_088D34F4, 0x089B6070u);
    call1(c, o, f_08837098, 0x089B6078u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0xF), f_088616EC, 0x089B6088u);
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): step 1, +262 = 1, the meter by strength + 3, 0x08837098, +551 cleared and move 9 + 2 x
   strength. */
SFA_FUNC(08902590, special_start_9_2k) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1;
    int32_t s = S8(o + 194);
    *B(o + 262) = 1;
    call2(c, o, (uint32_t)(s + 3), f_0886CA2C, 0x089025C0u);
    call1(c, o, f_08837098, 0x089025C8u);
    int32_t s2 = S8(o + 194);
    *B(o + 551) = 0;
    call2(c, o, (uint32_t)((s2 << 1) + 9) & 0xFFFF, f_088616EC, 0x089025E4u);
    leave(c, sv);
}

/* (fighter): +7 on, +0x3C0 cleared, +262 = 1, the meter by 2, 0x08837098, 0x08939BF4 and move 0x27. */
SFA_FUNC(08935FBC, special_start_27) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 0x3C0) = 0; *B(o + 262) = 1;
    call2(c, o, 2, f_0886CA2C, 0x08935FF4u);
    call1(c, o, f_08837098, 0x08935FFCu);
    call1(c, o, f_08939BF4, 0x08936004u);
    call2(c, o, 0x27, f_088616EC, 0x08936010u);
    leave(c, sv);
}

/* (fighter): +262 = 1, step 1, +640 cleared, +502 = 1, the meter by 7, 0x08837098, move 0x31 + strength
   and 0x08944300. */
SFA_FUNC(08944188, special_start_31) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 262) = 1; *B(o + 7) = 1; *B(o + 640) = 0; *B(o + 502) = 1;
    call2(c, o, 7, f_0886CA2C, 0x089441B8u);
    call1(c, o, f_08837098, 0x089441C0u);
    call2(c, o, by_str(o, 0x31), f_088616EC, 0x089441D4u);
    call1(c, o, f_08944300, 0x089441DCu);
    leave(c, sv);
}

/* (fighter): +696 cleared, +7 on, +123 / +262 cleared, the meter by strength + 6, 0x08837098 and move 3
   + strength. */
SFA_FUNC(0895D0AC, special_start_3_m6) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 696) = 0; *B(o + 7) = (uint8_t)(st + 1);
    int32_t s = S8(o + 194);
    *B(o + 123) = 0; *B(o + 262) = 0;
    call2(c, o, (uint32_t)(s + 6), f_0886CA2C, 0x0895D0E4u);
    call1(c, o, f_08837098, 0x0895D0ECu);
    call2(c, o, by_str(o, 3), f_088616EC, 0x0895D100u);
    leave(c, sv);
}

/* (fighter): +7 on, +235 = 1, facing the opponent, 0x08863114, the meter by strength and move 0xC +
   strength of its list. */
SFA_FUNC(089A2C98, special_face_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    int32_t face = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)face;
    call1(c, o, f_08863114, 0x089A2CCCu);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089A2CD8u);
    call2(c, o, by_str(o, 0xC), f_08861674, 0x089A2CECu);
    leave(c, sv);
}

/* (fighter): +7 on, +0x33C cleared, +262 / +73 = 1, the meter by 5 unless +540, 0x08837098 and move
   7. */
SFA_FUNC(08919928, special_start_7_air) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t st = *B(o + 7);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 0x33C) = 0; *B(o + 262) = 1;
    int32_t k = S8(o + 540);
    *B(o + 73) = 1;
    enter(c, o, sv);
    if (k == 0) call2(c, o, 5, f_0886CA2C, 0x0891996Cu);
    call1(c, o, f_08837098, 0x08919974u);
    call2(c, o, 7, f_088616EC, 0x08919980u);
    leave(c, sv);
}
