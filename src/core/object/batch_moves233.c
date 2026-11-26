/* More special-move starts on the meter gain by distance (0x0886D468), with their launch speeds and
   flags set before or after it. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886D468(Cpu *c); void f_088370D8(Cpu *c); void f_0887D144(Cpu *c);
void f_0892B188(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* 0x0887D144 first (K_FIRST) or last around the meter by distance and 0x088370D8; b[0..2] are their
   return addresses */
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
static uint32_t s1(gaddr o, int32_t base) { return (uint32_t)(S8(o + 194) + base) & 0xFFFF; }
static uint32_t s2(gaddr o, int32_t base) { return (uint32_t)(S8(o + 194) * 2 + base) & 0xFFFF; }

#define BACKS(...) static const uint32_t b[] = { __VA_ARGS__ }

/* +7 on, +543 = 2, then 0x0887D144 first and move 0x1D. */
SFA_FUNC(08969710, start_543_1d) {
    BACKS(0x0896973Cu, 0x08969744u, 0x0896974Cu, 0x08969758u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 543) = 2;
    gain3(c, o, K_FIRST, b); move(c, o, 0x1D, b[3]);
    leave(c, sv);
}

/* step 1, 0x0887D144 first and move 2 x strength + 0x19. */
SFA_FUNC(089A7AEC, start_2s_19) {
    BACKS(0x089A7B0Cu, 0x089A7B14u, 0x089A7B1Cu, 0x089A7B34u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 7) = 1;
    gain3(c, o, K_FIRST, b); move(c, o, s2(o, 0x19), b[3]);
    leave(c, sv);
}

/* step 1, 0x0892B188, 0x0887D144 last and move strength + 0x15. */
SFA_FUNC(0892C26C, start_b188_15) {
    BACKS(0x0892C294u, 0x0892C29Cu, 0x0892C2A4u, 0x0892C2B8u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 7) = 1;
    call1(c, o, f_0892B188, 0x0892C28Cu);
    gain3(c, o, K_LAST, b); move(c, o, s1(o, 0x15), b[3]);
    leave(c, sv);
}

/* +310 / +696 = 1, step 1, 0x0887D144 first and move strength + 0x21. */
SFA_FUNC(089318B8, start_310_21) {
    BACKS(0x089318E0u, 0x089318E8u, 0x089318F0u, 0x08931904u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 310) = 1; *B(o + 696) = 1; *B(o + 7) = 1;
    gain3(c, o, K_FIRST, b); move(c, o, s1(o, 0x21), b[3]);
    leave(c, sv);
}

/* +7 on, +952 / +932 cleared, 0x0887D144 last and move 0x32. */
SFA_FUNC(08936E8C, start_clear_32) {
    BACKS(0x08936EBCu, 0x08936EC4u, 0x08936ECCu, 0x08936ED8u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *W(o + 0x3A0 + 24) = 0; *W(o + 0x3A0 + 4) = 0;
    gain3(c, o, K_LAST, b); move(c, o, 0x32, b[3]);
    leave(c, sv);
}

/* +7 on, 0x0887D144 last, +82 = 4 and move 2 x strength + 0x1B. */
SFA_FUNC(089111F0, start_82_2s_1b) {
    BACKS(0x08911210u, 0x08911218u, 0x08911220u, 0x08911240u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o);
    gain3(c, o, K_LAST, b);
    int32_t s = S8(o + 194);
    *H(o + 82) = 4;
    move(c, o, (uint32_t)((s << 1) + 0x1B) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, +83 = 0x32, 0x0887D144 first and move 0x1E. */
SFA_FUNC(0893EF48, start_83_1e) {
    BACKS(0x0893EF7Cu, 0x0893EF84u, 0x0893EF8Cu, 0x0893EF98u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 0x52 + 1) = 0x32;
    gain3(c, o, K_FIRST, b); move(c, o, 0x1E, b[3]);
    leave(c, sv);
}

/* +7 on, a rise (y 4.0, fall -0.375, x still), 0x0887D144 last and move 0x1C. */
SFA_FUNC(088F0250, start_rise_1c) {
    BACKS(0x088F0288u, 0x088F0290u, 0x088F0298u, 0x088F02A4u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA000u; *W(o + 96) = 0;
    gain3(c, o, K_LAST, b); move(c, o, 0x1C, b[3]);
    leave(c, sv);
}

/* +7 on, 0x0887D144 first, x 6.0 with brake -0.5 and move strength + 0x1B. */
SFA_FUNC(088FE224, start_dash_1b) {
    BACKS(0x088FE244u, 0x088FE24Cu, 0x088FE254u, 0x088FE278u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o);
    gain3(c, o, K_FIRST, b);
    int32_t s = S8(o + 194);
    *W(o + 96) = 0x60000; *W(o + 104) = 0xFFFF8000u;
    move(c, o, (uint32_t)(s + 0x1B) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, +696 cleared, +543 = 2, 0x0887D144 first and move strength + 0x21. */
static void start_696_543_21(Cpu *c, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 696) = 0; *B(o + 543) = 2;
    gain3(c, o, K_FIRST, b); move(c, o, s1(o, 0x21), b[3]);
    leave(c, sv);
}
SFA_FUNC(0895E6A0, start_543_21) { BACKS(0x0895E6D0u, 0x0895E6D8u, 0x0895E6E0u, 0x0895E6F4u); start_696_543_21(c, b); }
SFA_FUNC(0898FC84, start_543_21b) { BACKS(0x0898FCB4u, 0x0898FCBCu, 0x0898FCC4u, 0x0898FCD8u); start_696_543_21(c, b); }

/* step 1, +543 = 2, x 8.0 with brake -0.5, 0x0887D144 first and move strength + `base`. */
static void start_dash8(Cpu *c, int32_t base, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 7) = 1; *B(o + 543) = 2;
    *W(o + 96) = 0x80000; *W(o + 104) = 0xFFFF8000u;
    gain3(c, o, K_FIRST, b); move(c, o, s1(o, base), b[3]);
    leave(c, sv);
}
SFA_FUNC(089314D8, start_dash8_1e) { BACKS(0x08931510u, 0x08931518u, 0x08931520u, 0x08931534u); start_dash8(c, 0x1E, b); }
SFA_FUNC(08965410, start_dash8_34) { BACKS(0x08965448u, 0x08965450u, 0x08965458u, 0x0896546Cu); start_dash8(c, 0x34, b); }

/* +7 on, +696 = 1, +543 = 2, 0x0887D144 first and move 2 x strength + 0x36. */
SFA_FUNC(0895EC98, start_696_2s_36) {
    BACKS(0x0895ECCCu, 0x0895ECD4u, 0x0895ECDCu, 0x0895ECF4u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 696) = 1; *B(o + 543) = 2;
    gain3(c, o, K_FIRST, b); move(c, o, s2(o, 0x36), b[3]);
    leave(c, sv);
}

/* +7 on, +543 = 2, 0x0887D144 first, +20 / +123 / +73 cleared and move strength + 0x19. */
SFA_FUNC(0896816C, start_543_19) {
    BACKS(0x08968198u, 0x089681A0u, 0x089681A8u, 0x089681C8u);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 543) = 2;
    gain3(c, o, K_FIRST, b);
    int32_t s = S8(o + 194);
    *H(o + 20) = 0; *B(o + 123) = 0; *B(o + 73) = 0;
    move(c, o, (uint32_t)(s + 0x19) & 0xFFFF, b[3]);
    leave(c, sv);
}

/* +7 on, +543 = 2, +696 / +123 cleared, 0x0887D144 first and move 2 x strength + 0x33. */
SFA_FUNC(0896D8D0, start_543_2s_33) {
    BACKS(0x0896D904u, 0x0896D90Cu, 0x0896D914u, 0x0896D92Cu);
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    step_on(o); *B(o + 543) = 2; *B(o + 696) = 0; *B(o + 123) = 0;
    gain3(c, o, K_FIRST, b); move(c, o, s2(o, 0x33), b[3]);
    leave(c, sv);
}
