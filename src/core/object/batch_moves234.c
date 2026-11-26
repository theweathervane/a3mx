/* Special-move starts on the meter gain by distance (0x0886D468) that set their speeds from tables or
   by facing. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886D468(Cpu *c); void f_088370D8(Cpu *c); void f_0887D144(Cpu *c);
void f_0892B188(Cpu *c); void f_0890AB14(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

enum { K_FIRST = 1, K_LAST = 0 };
static void gain3(Cpu *c, gaddr o, int order, const uint32_t b[3]) {
    if (order == K_FIRST) {
        call1(c, o, f_0887D144, b[0]); call1(c, o, f_0886D468, b[1]); call1(c, o, f_088370D8, b[2]);
    } else {
        call1(c, o, f_0886D468, b[0]); call1(c, o, f_088370D8, b[1]); call1(c, o, f_0887D144, b[2]);
    }
}
static void move(Cpu *c, gaddr o, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; f_088616EC(c);
}
#define BACKS(...) static const uint32_t b[] = { __VA_ARGS__ }

/* +7 on, +696 = 1, +543 = 2, 0x0887D144 first and move 2 x strength + 0x36. */
SFA_FUNC(08991394, start_696_2s_36b) {
    BACKS(0x089913C8u, 0x089913D0u, 0x089913D8u, 0x089913F0u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 696) = 1; *B(o + 543) = 2;
    gain3(c, o, K_FIRST, b);
    move(c, o, (uint32_t)(S8(o + 194) * 2 + 0x36) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* step 1, 0x0887D144 last, 24 toward its facing and move 2 x strength + 0x1C. */
SFA_FUNC(08903BFC, start_nudge_2s_1c) {
    BACKS(0x08903C1Cu, 0x08903C24u, 0x08903C2Cu, 0x08903C60u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 7) = 1;
    gain3(c, o, K_LAST, b);
    int32_t d = *B(o + 11) != 0 ? 0x18 : -0x18;
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
    move(c, o, (uint32_t)((S8(o + 194) << 1) + 0x1C) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, +823 = 0x32, 0x0887D144 last, the brake -0.375, 0x0890AB14 and move strength + 0x29. */
SFA_FUNC(0890A7D8, start_ab14_29) {
    BACKS(0x0890A808u, 0x0890A810u, 0x0890A818u, 0x0890A83Cu);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 0x334 + 3) = 0x32;
    gain3(c, o, K_LAST, b);
    *W(o + 104) = 0xFFFFA000u;
    call1(c, o, f_0890AB14, 0x0890A828u);
    move(c, o, (uint32_t)(S8(o + 194) + 0x29) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, +237 = 1 with +81 (else -1), +823 = 0x32, 0x0887D144 first and move 9. */
SFA_FUNC(0890ACA0, start_237_9) {
    BACKS(0x0890ACECu, 0x0890ACF4u, 0x0890ACFCu, 0x0890AD08u);
    gaddr o = ARG(0); uint32_t sv[2];
    step_on(o);
    int32_t k = S8(o + 81);
    enter(c, o, sv);
    *B(o + 237) = k != 0 ? 1 : 0xFF;
    *B(o + 0x334 + 3) = 0x32;
    gain3(c, o, K_FIRST, b);
    move(c, o, 9, b[3]);
    leave(c, sv);
}

/* step 1, 0x0892B188, 0x0887D144 first, x by strength from 0x08A6BB24 with brake -0.5 and move
   strength + 0xD. */
SFA_FUNC(0892B9D0, start_b188_tab_d) {
    BACKS(0x0892B9F8u, 0x0892BA00u, 0x0892BA08u, 0x0892BA3Cu);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 7) = 1;
    call1(c, o, f_0892B188, 0x0892B9F0u);
    gain3(c, o, K_FIRST, b);
    int32_t s = S8(o + 194);
    *W(o + 96) = *W(0x08A6BB24u + ((uint32_t)s << 2));
    *W(o + 104) = 0xFFFF8000u;
    move(c, o, (uint32_t)(s + 0xD) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, 0x0887D144 last, a jump (x 5.0, y 8.5, fall -0.5625), +824 = -0.125 and move strength + 0xF. */
SFA_FUNC(0890723C, start_jump_f) {
    BACKS(0x0890725Cu, 0x08907264u, 0x0890726Cu, 0x089072ACu);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o);
    gain3(c, o, K_LAST, b);
    *W(o + 96) = 0x50000; *W(o + 104) = 0;
    *W(o + 100) = 0x88000; *W(o + 108) = 0xFFFF7000u;
    *W(o + 0x334 + 4) = 0xFFFFE000u;
    move(c, o, (uint32_t)(S8(o + 194) + 0xF), b[3]);
    leave(c, sv);
}

/* +7 on, +502 = 1, 0x0887D144 last, x / brake by strength from 0x08A53AC0 and move 2 x strength + 0x12. */
SFA_FUNC(08907558, start_502_tab_12) {
    BACKS(0x08907584u, 0x0890758Cu, 0x08907594u, 0x089075C8u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 502) = 1;
    gain3(c, o, K_LAST, b);
    uint32_t s2 = (uint32_t)(S8(o + 194) << 1);
    gaddr e = 0x08A53AC0u + (s2 << 2);
    *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
    move(c, o, s2 + 0x12, b[3]);
    leave(c, sv);
}

/* +7 on, a hop (x 8.0 toward its facing, y 2.0, fall -0.1875), 0x0887D144 last and move strength + 0x20. */
SFA_FUNC(08927400, start_hop_20) {
    BACKS(0x0892744Cu, 0x08927454u, 0x0892745Cu, 0x08927470u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t f = *B(o + 11);
    step_on(o);
    *W(o + 96) = f != 0 ? 0x80000u : 0xFFF80000u;
    *W(o + 100) = 0x20000; *W(o + 108) = 0xFFFFD000u; *W(o + 104) = 0;
    gain3(c, o, K_LAST, b);
    move(c, o, (uint32_t)(S8(o + 194) + 0x20) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, 0x0887D144 first, x 6.0 with brake -0.5, +282 = 1, +82 by strength from 0x08A900BC and move 0x20. */
SFA_FUNC(0897427C, start_dash_82_20) {
    BACKS(0x0897429Cu, 0x089742A4u, 0x089742ACu, 0x089742ECu);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o);
    gain3(c, o, K_FIRST, b);
    *W(o + 96) = 0x60000;
    int32_t s = S8(o + 194);
    *W(o + 104) = 0xFFFF8000u;
    *B(o + 282) = 1;
    *H(o + 82) = *H(0x08A900BCu + (uint32_t)(s + s));
    move(c, o, 0x20, b[3]);
    leave(c, sv);
}

/* +7 on, +820 = 1, +824 cleared, the gain, 0x088370D8; in the air (+73) +828 = 1 and move 0x26, else
   +828 cleared and move 3. */
SFA_FUNC(089BBCDC, start_air_or_3) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *W(c->r[29] + 24) = ra;
    step_on(o); *W(o + 820) = 1; *W(o + 824) = 0;
    call1(c, o, f_0886D468, 0x089BBD10u);
    call1(c, o, f_088370D8, 0x089BBD18u);
    if (S8(o + 73) != 0) { *W(o + 828) = 1; move(c, o, 0x26, 0x089BBD48u); }
    else { *W(o + 828) = 0; move(c, o, 3, 0x089BBD34u); }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
