/* Special-move starts built on the meter gain by distance (0x0886D468): +7 on (or step 1), the gain,
   0x088370D8 and 0x0887D144 in one of two orders, and a fixed move or strength + base. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_0886D468(Cpu *c); void f_088370D8(Cpu *c); void f_0887D144(Cpu *c);
void f_089B7590(Cpu *c);

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
static void step_on(gaddr o) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }

/* the meter by distance, 0x088370D8 and 0x0887D144 */
static void gain(Cpu *c, gaddr o, uint32_t b) { call1(c, o, f_0886D468, b); }
static void a70(Cpu *c, gaddr o, uint32_t b) { call1(c, o, f_088370D8, b); }
static void k144(Cpu *c, gaddr o, uint32_t b) { call1(c, o, f_0887D144, b); }
static void move(Cpu *c, gaddr o, uint32_t n, uint32_t b) { call2(c, o, n, f_088616EC, b); }
/* strength + base, kept to 16 bits when `mask` */
static uint32_t by_s(gaddr o, int32_t base, int mask) {
    uint32_t n = (uint32_t)(S8(o + 194) + base);
    return mask ? n & 0xFFFF : n;
}

/* +7 on, the gain, 0x088370D8 and move strength + `base`. */
static void gain_by_s(Cpu *c, int32_t base, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    gain(c, o, b[0]); a70(c, o, b[1]);
    move(c, o, by_s(o, base, 0), b[2]);
    leave(c, sv);
}
SFA_FUNC(089BBA58, start_gain_31) { static const uint32_t b[3] = { 0x089BBA78u, 0x089BBA80u, 0x089BBA90u }; gain_by_s(c, 0x31, b); }
SFA_FUNC(089C1EEC, start_gain_16) { static const uint32_t b[3] = { 0x089C1F0Cu, 0x089C1F14u, 0x089C1F24u }; gain_by_s(c, 0x16, b); }
SFA_FUNC(089C8894, start_gain_24) { static const uint32_t b[3] = { 0x089C88B4u, 0x089C88BCu, 0x089C88CCu }; gain_by_s(c, 0x24, b); }

/* +7 on, 0x0887D144, the gain, 0x088370D8 and move strength + `base` (16 bits). */
static void k_gain_by_s(Cpu *c, int32_t base, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    k144(c, o, b[0]); gain(c, o, b[1]); a70(c, o, b[2]);
    move(c, o, by_s(o, base, 1), b[3]);
    leave(c, sv);
}
SFA_FUNC(088EFFBC, start_k_gain_27) {
    static const uint32_t b[4] = { 0x088EFFDCu, 0x088EFFE4u, 0x088EFFECu, 0x088F0000u }; k_gain_by_s(c, 0x27, b);
}
SFA_FUNC(088FDFC4, start_k_gain_f) {
    static const uint32_t b[4] = { 0x088FDFE4u, 0x088FDFECu, 0x088FDFF4u, 0x088FE008u }; k_gain_by_s(c, 0xF, b);
}
SFA_FUNC(08964330, start_k_gain_27b) {
    static const uint32_t b[4] = { 0x08964350u, 0x08964358u, 0x08964360u, 0x08964374u }; k_gain_by_s(c, 0x27, b);
}
SFA_FUNC(0899BD9C, start_k_gain_17) {
    static const uint32_t b[4] = { 0x0899BDBCu, 0x0899BDC4u, 0x0899BDCCu, 0x0899BDE0u }; k_gain_by_s(c, 0x17, b);
}
SFA_FUNC(0899C200, start_k_gain_14) {
    static const uint32_t b[4] = { 0x0899C220u, 0x0899C228u, 0x0899C230u, 0x0899C244u }; k_gain_by_s(c, 0x14, b);
}

/* +7 on, the gain, 0x088370D8, 0x0887D144 and move strength + 0xF (16 bits). */
SFA_FUNC(089A4468, start_gain_k_f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    gain(c, o, 0x089A4488u); a70(c, o, 0x089A4490u); k144(c, o, 0x089A4498u);
    move(c, o, by_s(o, 0xF, 1), 0x089A44ACu);
    leave(c, sv);
}

/* +7 on, 0x0887D144, the gain, 0x088370D8 and move `n`. */
static void k_gain_n(Cpu *c, uint32_t n, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    k144(c, o, b[0]); gain(c, o, b[1]); a70(c, o, b[2]);
    move(c, o, n, b[3]);
    leave(c, sv);
}
SFA_FUNC(08977080, start_k_gain_40) {
    static const uint32_t b[4] = { 0x089770A0u, 0x089770A8u, 0x089770B0u, 0x089770BCu }; k_gain_n(c, 0x40, b);
}

/* +7 on, the gain, 0x088370D8, 0x0887D144 and move `n`. */
static void gain_k_n(Cpu *c, uint32_t n, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    gain(c, o, b[0]); a70(c, o, b[1]); k144(c, o, b[2]);
    move(c, o, n, b[3]);
    leave(c, sv);
}
SFA_FUNC(089A47A4, start_gain_k_21) {
    static const uint32_t b[4] = { 0x089A47C4u, 0x089A47CCu, 0x089A47D4u, 0x089A47E0u }; gain_k_n(c, 0x21, b);
}

/* step 1, 0x0887D144, the gain, 0x088370D8 and move 0x30. */
SFA_FUNC(088F4D08, start_k_gain_30) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1;
    k144(c, o, 0x088F4D28u); gain(c, o, 0x088F4D30u); a70(c, o, 0x088F4D38u);
    move(c, o, 0x30, 0x088F4D44u);
    leave(c, sv);
}

/* +7 on, the gain, 0x088370D8, 0x0887D144 and move 0x2F. */
SFA_FUNC(08936BBC, start_gain_k_2f) {
    static const uint32_t b[4] = { 0x08936BDCu, 0x08936BE4u, 0x08936BECu, 0x08936BF8u }; gain_k_n(c, 0x2F, b);
}

/* step 1, +824 cleared, the gain, 0x088370D8, 0x0887D144 and move 0x11. */
SFA_FUNC(08945C8C, start_gain_k_11) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1; *W(o + 824) = 0;
    gain(c, o, 0x08945CB0u); a70(c, o, 0x08945CB8u); k144(c, o, 0x08945CC0u);
    move(c, o, 0x11, 0x08945CCCu);
    leave(c, sv);
}

/* +7 on, the gain, 0x088370D8, move strength + 0x23 and 0x089B7590. */
SFA_FUNC(089B689C, start_gain_23_7590) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o);
    gain(c, o, 0x089B68BCu); a70(c, o, 0x089B68C4u);
    move(c, o, by_s(o, 0x23, 0), 0x089B68D4u);
    call1(c, o, f_089B7590, 0x089B68DCu);
    leave(c, sv);
}

/* +7 on, strength 2, the gain, 0x088370D8 and move 0x1C. */
SFA_FUNC(089C2190, start_gain_s2_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *B(o + 194) = 2;
    gain(c, o, 0x089C21BCu); a70(c, o, 0x089C21C4u);
    move(c, o, 0x1C, 0x089C21D0u);
    leave(c, sv);
}

/* step 1, +82 = 0x36, 0x0887D144, the gain, 0x088370D8 and move 0x1F. */
SFA_FUNC(088F8E9C, start_k_gain_1f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1; *H(o + 82) = 0x36;
    k144(c, o, 0x088F8EC4u); gain(c, o, 0x088F8ECCu); a70(c, o, 0x088F8ED4u);
    move(c, o, 0x1F, 0x088F8EE0u);
    leave(c, sv);
}

/* step 1, the gain, 0x088370D8, 0x0887D144, +1146 = 1 and move 0x22. */
SFA_FUNC(08903ECC, start_gain_k_22) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 1;
    gain(c, o, 0x08903EECu); a70(c, o, 0x08903EF4u); k144(c, o, 0x08903EFCu);
    *B(o + 1146) = 1;
    move(c, o, 0x22, 0x08903F10u);
    leave(c, sv);
}

/* +7 on, +123 cleared, the gain, 0x088370D8, 0x0887D144 and move 0x26. */
SFA_FUNC(089A4AF0, start_gain_k_26) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 123) = 0; step_on(o);
    gain(c, o, 0x089A4B18u); a70(c, o, 0x089A4B20u); k144(c, o, 0x089A4B28u);
    move(c, o, 0x26, 0x089A4B34u);
    leave(c, sv);
}

/* +7 on, +82 = 0x32, 0x0887D144, the gain, 0x088370D8 and move 0x1C. */
SFA_FUNC(08931260, start_k_gain_1c) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    step_on(o); *H(o + 82) = 0x32;
    k144(c, o, 0x0893128Cu); gain(c, o, 0x08931294u); a70(c, o, 0x0893129Cu);
    move(c, o, 0x1C, 0x089312A8u);
    leave(c, sv);
}
