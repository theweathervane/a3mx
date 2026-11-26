/* Special-move starts with their meter change: rush starts by guard and +193 (via 0x08863A50 /
   0x08863114), a super that spends 144 meter (0x089B2214) and part characters' starts. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_088370D8(Cpu *c); void f_088D34F4(Cpu *c); void f_089A3F24(Cpu *c); void f_089A3ED8(Cpu *c);
void f_08863A50(Cpu *c); void f_08863114(Cpu *c); void f_0887D144(Cpu *c); void f_089B2A64(Cpu *c);
void f_088D3180(Cpu *c);

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
static void meter_str(Cpu *c, gaddr o, uint32_t back) { call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, back); }

/* (fighter): +7 on, +262 = 1, the meter by 5, 0x08837098, +82 = 9, +820 = 11, guard 4, 0x089A3F24, x
   still, +100 = 2 and 0x089A3ED8. */
SFA_FUNC(089A3E5C, special_charge_start) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x089A3E8Cu);
    call1(c, o, f_08837098, 0x089A3E94u);
    *H(o + 82) = 9; *W(o + 820) = 0xB; *B(o + 78) = 4;
    call1(c, o, f_089A3F24, 0x089A3EB4u);
    *W(o + 96) = 0; *W(o + 100) = 2;
    call1(c, o, f_089A3ED8, 0x089A3EC8u);
    leave(c, sv);
}

/* (fighter): step 3, +192 = 2, +235 = 1, 0x08863A50, the meter by strength and move 0x12 (0x18 with a
   guard) + 3 with +193, + strength of its list. */
static void rush_by_guard(Cpu *c, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 3; *B(o + 192) = 2; *B(o + 235) = 1;
    call1(c, o, f_08863A50, b[0]);
    meter_str(c, o, b[1]);
    uint32_t n = S8(o + 78) != 0 ? 0x18 : 0x12;
    if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
    call2(c, o, (n + (uint32_t)S8(o + 194)) & 0xFFFF, f_08861674, b[2]);
    leave(c, sv);
}
SFA_FUNC(089095C4, rush_by_guard_a) {
    static const uint32_t b[3] = { 0x089095F4u, 0x08909600u, 0x08909634u };
    rush_by_guard(c, b);
}
SFA_FUNC(0895C29C, rush_by_guard_b) {
    static const uint32_t b[3] = { 0x0895C2CCu, 0x0895C2D8u, 0x0895C310u };
    rush_by_guard(c, b);
}

/* (fighter): step 3, +235 = 1, 0x08863A50, the meter by strength and move 0x12 (0x21 with no choice
   made) + 6 with a guard + 3 with +193, + strength of its list. */
SFA_FUNC(08983C14, rush_by_choice) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 3; *B(o + 235) = 1;
    call1(c, o, f_08863A50, 0x08983C3Cu);
    meter_str(c, o, 0x08983C48u);
    uint32_t n = S8(o + 386) == -1 ? 0x21 : 0x12;
    if (S8(o + 78) != 0) n += 6;
    if (S8(o + 193) != 0) n += 3;
    call2(c, o, n + (uint32_t)S8(o + 194), f_08861674, 0x08983C88u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 / +502 = 1, the meter by 2, 0x08837098 and move 0x29 (6 with a choice made) +
   strength. */
SFA_FUNC(089A3AEC, special_by_choice) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 262) = 1; *B(o + 502) = 1;
    call2(c, o, 2, f_0886CA2C, 0x089A3B20u);
    call1(c, o, f_08837098, 0x089A3B28u);
    if (S8(o + 386) == 0) call2(c, o, by_str(o, 0x29), f_088616EC, 0x089A3B44u);
    else call2(c, o, by_str(o, 6), f_088616EC, 0x089A3B5Cu);
    leave(c, sv);
}

/* (fighter): a super: +7 on, +696 / +260 cleared, +543 = 2, +83 = -1, 0x0887D144, 144 meter spent,
   0x088370D8, 0x089B2A64 (8) and move 0x31. */
SFA_FUNC(089B2214, super_start_31) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    *B(o + 696) = 0; *B(o + 543) = 2; *B(o + 260) = 0; *B(o + 0x53) = 0xFF;
    call1(c, o, f_0887D144, 0x089B2258u);
    call2(c, o, 0xFFFFFF70u, f_0886CA2C, 0x089B2264u);
    call1(c, o, f_088370D8, 0x089B226Cu);
    call2(c, o, 8, f_089B2A64, 0x089B2278u);
    call2(c, o, 0x31, f_088616EC, 0x089B2284u);
    leave(c, sv);
}

/* (character): +7 on, the meter by 4, 0x08837098, move 8 standing (stance 2, +696 cleared) or 9
   crouched (+696 = 1), +262 = 1. */
SFA_FUNC(089C7CCC, part_special_stance) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    step_on(o);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    call2(c, o, 4, f_088D34F4, 0x089C7CF4u);
    call1(c, o, f_08837098, 0x089C7CFCu);
    c->r[17] = 1;
    if (S8(o + 236) == 2) { *B(o + 696) = 0; call2(c, o, 8, f_088616EC, 0x089C7D1Cu); }
    else { *B(o + 696) = 1; call2(c, o, 9, f_088616EC, 0x089C7D34u); }
    *B(o + 262) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 on, facing the opponent, +235 = 1, 0x08863114, the meter by strength and move 0xC +
   strength (+3 with +193, kept to a byte) of its list. */
SFA_FUNC(08905E50, special_face_list_byte) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, 0x08905E84u);
    meter_str(c, o, 0x08905E90u);
    int32_t n = (int8_t)(S8(o + 194) + 0xC);
    if (S8(o + 193) != 0) n = (int8_t)(n + 3);
    call2(c, o, (uint32_t)n & 0xFFFF, f_08861674, 0x08905EC8u);
    leave(c, sv);
}

/* (fighter): +7 on, +262 = 1, the meter by 8, 0x08837098, a hop (x 4.0, y 2.5 / -0.625), +235 / +73 =
   1 and move 0x11. */
SFA_FUNC(0890B8D0, special_hop_11) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint32_t st = *B(o + 7);
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 262) = 1;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 8, f_0886CA2C, 0x0890B904u);
    call1(c, o, f_08837098, 0x0890B90Cu);
    *W(o + 96) = 0x40000; *W(o + 100) = 0x28000; *W(o + 104) = 0; *W(o + 108) = 0xFFFF6000u;
    *B(o + 235) = 1; *B(o + 73) = 1;
    call2(c, o, 0x11, f_088616EC, 0x0890B944u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 on, +196 / +696 / +235 cleared, +262 = 1 (+502 = 1 with +80), +820 from 0x08A85AC0 by the
   opponent's character, the meter by 2, 0x08837098 and move 0x17. */
SFA_FUNC(0895DAE0, special_start_17) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    gaddr opp = *W(o + 92);
    step_on(o);
    *B(o + 196) = 0; *B(o + 696) = 0; *B(o + 235) = 0;
    int32_t k = S8(o + 80);
    *B(o + 262) = 1;
    if (k != 0) *B(o + 502) = 1;
    *W(o + 820) = *B(0x08A85AC0u + (uint32_t)(int32_t)S8(opp + 338));
    call2(c, o, 2, f_0886CA2C, 0x0895DB44u);
    call1(c, o, f_08837098, 0x0895DB4Cu);
    call2(c, o, 0x17, f_088616EC, 0x0895DB58u);
    leave(c, sv);
}

/* (character): +7 on, the meter by 4, 0x08837098, a hop (x 5.0, y 4.0 / -0.375) at strengths 0 - 2,
   0x088D3180, move 0xC + strength and +262 = 1. */
SFA_FUNC(089C7F98, part_special_hop_c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    call2(c, o, 4, f_088D34F4, 0x089C7FBCu);
    call1(c, o, f_08837098, 0x089C7FC4u);
    int32_t s = S8(o + 194);
    if (s >= 0 && s < 3) { *W(o + 96) = 0x50000; *W(o + 100) = 0x40000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFA000u; }
    call1(c, o, f_088D3180, 0x089C7FF8u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0xC), f_088616EC, 0x089C8008u);
    *B(o + 262) = 1;
    leave(c, sv);
}

/* (fighter): +696 cleared, +7 on, +10 / +123 cleared, +262 = 1, the meter by 6, 0x08837098 and in the
   air move 0x31 + strength (+10 = 1), on the ground 0xD + strength. */
SFA_FUNC(0895D48C, special_air_or_ground) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 696) = 0; *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 10) = 0; *B(o + 123) = 0; *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x0895D4C8u);
    call1(c, o, f_08837098, 0x0895D4D0u);
    if (S8(o + 73) != 0) { call2(c, o, by_str(o, 0x31), f_088616EC, 0x0895D4ECu); *B(o + 10) = 1; }
    else call2(c, o, by_str(o, 0xD), f_088616EC, 0x0895D508u);
    leave(c, sv);
}
