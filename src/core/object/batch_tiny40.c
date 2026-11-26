/* Small functions the scenarios reach, 13 - 37 instructions: fighter objects' starts (super moves with
   a full gauge among them), a range check, an effect launched from a row, an effect's flight.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D3BEC(Cpu *c); void f_0886C6EC(Cpu *c); void f_0891E914(Cpu *c); void f_08821424(Cpu *c);
void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter): 0x088D3BEC's answer (0 for none). */
SFA_FUNC(088D4110, fighter_other_in_range_2) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088D4120u; f_088D3BEC(c);
    if (c->r[2] == 0) c->r[2] = 0;
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* A super move's start: on (+4 = 1, +6 = 8, +235 = 1, +194 / +552 = 2, +236 = k236; +5 / +7 cleared,
   +11 = +697). */
static void super_on(gaddr o, uint32_t k236) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
    *B(o + 236) = (uint8_t)k236;
    *B(o + 11) = (uint8_t)S8(o + 697);
    *B(o + 235) = 1;
    *B(o + 194) = 2; *B(o + 552) = 2;
}

/* (object): with +386 clear, a full gauge (+366 from 144) and 0x0886C6EC allowing it, a super move
   0xB; 0x0886C6EC's answer then, else 0. */
SFA_FUNC(088EF33C, object_super_b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int ok = 0;
    if (S8(o + 386) == 0 && !(S16(o + 366) < 144)) {
        c->r[31] = 0x088EF36Cu; f_0886C6EC(c);
        if (c->r[2] != 0) { super_on(o, 0xB); ok = 1; }
    }
    if (!ok) c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* As 0x088EF33C, answering 1 / 0. */
SFA_FUNC(088F3194, object_super_b_1) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 386) == 0 && !(S16(o + 366) < 144)) {
        c->r[31] = 0x088F31CCu; f_0886C6EC(c);
        if (c->r[2] != 0) { super_on(o, 0xB); c->r[17] = 1; }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): launched: +9 = 2, +0x5C / +0x5D = 2 / 0, its steps (+96 / +100 / +104 / +108) from the
   16-byte row of 0x08A648E4 by kind +3 (x turned for +11 clear), then 0x0891E914 (2). */
SFA_FUNC(0891DFE0, effect_launch_row) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 9) = 2;
    *B(o + 0x5C) = 2; *B(o + 0x5D) = 0;
    gaddr row = 0x08A648E4u + ((uint32_t)*B(o + 3) << 4);
    *W(o + 96) = *W(row);
    *W(o + 100) = *W(row + 4);
    *W(o + 104) = *W(row + 8);
    *W(c->r[29] + 16) = ra;
    *W(o + 108) = *W(row + 12);
    if (*B(o + 11) == 0) {
        *W(o + 96) = 0u - *W(o + 96);
        *W(o + 104) = 0u - *W(o + 104);
    }
    call2(c, o, 2, 0x0891E068u, f_0891E914);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): flies by its steps (x +96, y +100 falling by +108) for +82 frames, then on (+4); in its
   last 17 frames drawn on even ones only. */
SFA_FUNC(08941BF0, effect_flight) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(o + 16) = *W(o + 16) + *W(o + 96);
    uint32_t dy = *W(o + 100);
    *W(o + 20) = *W(o + 20) + dy;
    *W(o + 100) = dy + *W(o + 108);
    int16_t t = (int16_t)(S16(o + 82) - 1);
    *W(c->r[29] + 16) = ra;
    *H(o + 82) = (uint16_t)t;
    if (t == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        int32_t v = S16(o + 82);
        if (!((int32_t)(v & 0xFFFF) < 18) || (v & 1) == 0) { c->r[31] = 0x08941C78u; f_08821424(c); }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter object): for character 0x13 when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 =
   3; +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(089494D4, object_start_char13) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 338) == 0x13) {
        c->r[31] = 0x089494FCu; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 3;
            c->r[17] = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, 0x08949538u, f_0886DBE8);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 clear, the gauge exactly 144 (+366) and 0x0886C6EC allowing it, a super move 9;
   1 then, else 0. */
SFA_FUNC(089674D8, object_super_9) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int ok = 0;
    if (S8(o + 386) == 0 && S16(o + 366) == 0x90) {
        c->r[31] = 0x08967508u; f_0886C6EC(c);
        if (c->r[2] != 0) { super_on(o, 9); ok = 1; }
    }
    c->r[2] = ok ? 1 : 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object, k): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k; +5 / +7 cleared, +11
   = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0896C4F8, object_start_k) {
    gaddr o = ARG(0);
    uint32_t k = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[17] = k; c->r[16] = o;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[31] = 0x0896C51Cu; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[18] = 1;
        *B(o + 235) = 1;
        *B(o + 236) = (uint8_t)k;
        call_on(c, o, 0x0896C554u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
