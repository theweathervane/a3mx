/* Special-move starts with their meter change (0x0886CA2C / 0x088D34F4), a throw count step with meter
   (0x0895E460) and a part character's two-way special (0x089C1B18). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_088D34F4(Cpu *c);
void f_089C4CFC(Cpu *c); void f_0892B188(Cpu *c);

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

/* (fighter): +262 = 1, step 1, the meter by strength + 3, 0x08837098 and move 0x10 + strength. */
SFA_FUNC(08903518, special_start_10) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    int32_t s = S8(o + 194);
    *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, (uint32_t)(s + 3), f_0886CA2C, 0x08903544u);
    call1(c, o, f_08837098, 0x0890354Cu);
    call2(c, o, by_str(o, 0x10), f_088616EC, 0x08903560u);
    leave(c, sv);
}

/* (fighter): a throw count (+196 on), +696 / +235 = 1, y still with the x brake by strength
   (0x08A85B58), and the meter by 3. */
SFA_FUNC(0895E460, throw_count_meter) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t n = S8(o + 196), s = S8(o + 194);
    *B(o + 196) = (uint8_t)(n + 1);
    *B(o + 696) = 1; *B(o + 235) = 1;
    uint32_t v = *W(0x08A85B58u + ((uint32_t)s << 2));
    *W(o + 100) = 0; *W(o + 104) = v; *W(o + 108) = 0;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 3; c->r[31] = 0x0895E4ACu; f_0886CA2C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +7 on, the throw mark (+0x337) cleared, +262 = 1, the meter by 5, 0x08837098 and move 5. */
SFA_FUNC(0890A140, special_start_5) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 7) = (uint8_t)(st + 1);
    *B(o + 0x337) = 0; *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x0890A178u);
    call1(c, o, f_08837098, 0x0890A180u);
    call2(c, o, 5, f_088616EC, 0x0890A18Cu);
    leave(c, sv);
}

/* (fighter): +696 cleared, +7 on, +262 cleared, the meter by 7, 0x08837098 and move 0x14 + strength. */
SFA_FUNC(0895D8D4, special_start_14_m7) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 696) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 262) = 0;
    call2(c, o, 7, f_0886CA2C, 0x0895D904u);
    call1(c, o, f_08837098, 0x0895D90Cu);
    call2(c, o, by_str(o, 0x14), f_088616EC, 0x0895D920u);
    leave(c, sv);
}

/* (character): +7 on, x brake cleared, the meter by 4, 0x08837098, move `base` + strength, +262 = 1. */
static void part_special_b(Cpu *c, int32_t base, uint32_t b0, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *W(o + 104) = 0; *B(o + 7) = (uint8_t)(st + 1);
    call2(c, o, 4, f_088D34F4, b0);
    call1(c, o, f_08837098, b1);
    call2(c, o, (uint32_t)(S8(o + 194) + base), f_088616EC, b2);
    *B(o + 262) = 1;
    leave(c, sv);
}
SFA_FUNC(089C1308, part_special_f) { part_special_b(c, 0xF, 0x089C1334u, 0x089C133Cu, 0x089C134Cu); }
SFA_FUNC(089C1474, part_special_3) { part_special_b(c, 3, 0x089C14A0u, 0x089C14A8u, 0x089C14B8u); }

/* (character): +7 on, 0x089C4CFC (2); without +820 the meter by 4, 0x08837098 and move 0x12 + strength,
   with it move 0x15 and +644 = 2; +262 = 1, +82 = -1. */
SFA_FUNC(089C1B18, part_special_two_way) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    call2(c, o, 2, f_089C4CFC, 0x089C1B3Cu);
    if (*W(o + 820) == 0) {
        call2(c, o, 4, f_088D34F4, 0x089C1B54u);
        call1(c, o, f_08837098, 0x089C1B5Cu);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x12), f_088616EC, 0x089C1B6Cu);
    } else {
        call2(c, o, 0x15, f_088616EC, 0x089C1B80u);
        *B(o + 644) = 2;
    }
    *B(o + 262) = 1; *H(o + 82) = 0xFFFF;
    leave(c, sv);
}

/* (fighter): +7 on, +502 / +262 = 1, the meter by 2, 0x08837098 and move 8 + strength. */
SFA_FUNC(088FDE34, special_start_8_502) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 7) = (uint8_t)(st + 1); *B(o + 502) = 1; *B(o + 262) = 1;
    call2(c, o, 2, f_0886CA2C, 0x088FDE68u);
    call1(c, o, f_08837098, 0x088FDE70u);
    call2(c, o, by_str(o, 8), f_088616EC, 0x088FDE84u);
    leave(c, sv);
}

/* (fighter): +656 cleared, +7 on, +262 = 1, the meter by 5, 0x08837098 and move 2 + strength. */
SFA_FUNC(08921A14, special_start_2_656) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t st = *B(o + 7);
    *B(o + 656) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 262) = 1;
    call2(c, o, 5, f_0886CA2C, 0x08921A48u);
    call1(c, o, f_08837098, 0x08921A50u);
    call2(c, o, by_str(o, 2), f_088616EC, 0x08921A64u);
    leave(c, sv);
}

/* (fighter): step 1, 0x0892B188, +262 = 1, the meter by `gain`, 0x08837098 and move `base` + strength. */
static void special_b188(Cpu *c, uint32_t gain, uint32_t base, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1;
    call1(c, o, f_0892B188, b[0]);
    *B(o + 262) = 1;
    call2(c, o, gain, f_0886CA2C, b[1]);
    call1(c, o, f_08837098, b[2]);
    call2(c, o, by_str(o, base), f_088616EC, b[3]);
    leave(c, sv);
}
SFA_FUNC(0892B20C, special_b188_7) {
    static const uint32_t b[4] = { 0x0892B22Cu, 0x0892B240u, 0x0892B248u, 0x0892B25Cu };
    special_b188(c, 5, 7, b);
}
SFA_FUNC(0892B4CC, special_b188_a) {
    static const uint32_t b[4] = { 0x0892B4ECu, 0x0892B500u, 0x0892B508u, 0x0892B51Cu };
    special_b188(c, 6, 0xA, b);
}
