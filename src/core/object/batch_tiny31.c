/* Small functions the scenarios reach, 32 - 33 instructions: fighter objects' starts, a blinking step,
   an effect's timer by kind. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_089A536C(Cpu *c);
void f_08821424(Cpu *c); void f_089DB8E8(Cpu *c); void f_0887DF88(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void on_7(gaddr o, uint32_t k236) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
    *B(o + 236) = (uint8_t)k236; *B(o + 235) = 1;
    *B(o + 11) = (uint8_t)S8(o + 697);
}

/* (object): when 0x0886BF84 allows it, 0x089A536C, on (+4 = 1, +6 = 7, +235 = 1, +236 = k236; +11 =
   +697) and `then`, whose answer it gives; else 0. */
static void start_536c(Cpu *c, uint32_t k236, void (*then)(Cpu *), const uint32_t backs[3]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = backs[0]; f_0886BF84(c);
    if (c->r[2] != 0) {
        call_on(c, o, backs[1], f_089A536C);
        on_7(o, k236);
        call_on(c, o, backs[2], then);
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089A32D8, object_start_536c_5) {
    static const uint32_t b[3] = { 0x089A32ECu, 0x089A32FCu, 0x089A3334u };
    start_536c(c, 5, f_0886DBE8, b);
}
SFA_FUNC(089A3358, object_start_536c_6) {
    static const uint32_t b[3] = { 0x089A336Cu, 0x089A337Cu, 0x089A33B4u };
    start_536c(c, 6, f_0886DB94, b);
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k236; +11 = +697) and
   `then`; 1 then, else 0. */
static void start_s1(Cpu *c, uint32_t k236, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o, k236);
        c->r[17] = 1;
        call_on(c, o, back2, then);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089B0BD4, object_start_s1_0) { start_s1(c, 0, f_0886DB94, 0x089B0BECu, 0x089B0C24u); }
SFA_FUNC(089B0C54, object_start_s1_1) { start_s1(c, 1, f_0886DBE8, 0x089B0C6Cu, 0x089B0CA4u); }
SFA_FUNC(0890645C, object_start_s1_2) { start_s1(c, 2, f_0886DBE8, 0x08906474u, 0x089064B0u); }

/* (object, row): on (+5, then 0x089DB8E8) once the row's +2 reaches its level (+80); until then +82
   blinks and it is drawn every other frame. */
SFA_FUNC(089DB868, object_wait_level) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S16(p + 2) < S8(o + 80)) {
        *H(o + 82) = (uint16_t)(S16(o + 82) ^ 1);
        if (S16(o + 82) == 0) { ARG(0) = o; c->r[31] = 0x089DB8DCu; f_08821424(c); }
    } else {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        call2(c, o, p, 0x089DB8B0u, f_089DB8E8);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +4 on, its timer (+83) from 0x08A24598 by kind +3 (2 bytes) and +78, 0x0887DF88 (0x13),
   then +139 = -1, or 0 while its owner (+88) has +237. */
SFA_FUNC(0887F2C4, effect_timer_by_kind) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 0x52 + 1) = (uint8_t)S8(0x08A24598u + ((uint32_t)*B(o + 3) << 1) + (uint32_t)(int32_t)S8(o + 78));
    *W(c->r[29] + 24) = ra;
    call2(c, o, 0x13, 0x0887F31Cu, f_0887DF88);
    *B(o + 139) = S8(p + 237) != 0 ? 0 : 0xFF;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
