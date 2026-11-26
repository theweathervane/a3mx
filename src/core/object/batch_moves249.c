/* Special-move steps that leap once their animation ends (setting +573 through 0x0886E294) and call the
   effect spawner 0x0887FD2C on a frame flag. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861848(Cpu *c); void f_0886E294(Cpu *c); void f_0887FD2C(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void fx(Cpu *c, gaddr o, int32_t a, int32_t b, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)a; ARG(2) = (uint32_t)b; c->r[31] = back; f_0887FD2C(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void face81(gaddr o) { *B(o + 237) = S8(o + 81) != 0 ? 1 : 0xFF; }

/* (fighter): ticked; once ended +7 on, in the air, +237 cleared, 0x0886E294, +573 = 0x16 (0 with +81) and
   a jump (x 10.5 with +193, else 4.0; y 12.0, fall -1.125) toward its facing; until then on +77 it clears,
   +237 toward +81 and the effect at (-0x10, 0x3C). */
SFA_FUNC(08915F58, fx_then_jump) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x08915F6Cu);
    if (S8(o + 75) & 0x80) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 73) = 1; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x08915FD4u);
        *B(o + 573) = S8(o + 81) != 0 ? 0 : 0x16;
        *W(o + 96) = S8(o + 193) != 0 ? 0xA8000u : 0x40000u;
        *W(o + 104) = 0; *W(o + 100) = 0xC0000; *W(o + 108) = 0xFFFEE000u;
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    } else if (S8(o + 77) != 0) {
        *B(o + 77) = 0; face81(o);
        fx(c, o, -0x10, 0x3C, 0x08915FB0u);
    }
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +7 on, +237 cleared, 0x0886E294, +573 from 0x08A708B0 by its
   result (by strength without +81), x by strength from 0x08A708A4 toward its facing and move strength +
   0x35; on a flag other than 2 it becomes 2, +237 toward +81 and the effect at (-0x1D, 5). */
SFA_FUNC(08936EE8, fx_or_dash_35) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x08936EFCu);
    int32_t k = S8(o + 77);
    if (k == 0) {
        *B(o + 77) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x08936F24u);
        int32_t s = S8(o + 194);
        int32_t i = S8(o + 81) != 0 ? (int16_t)c->r[2] : s;
        *B(o + 573) = (uint8_t)*H(0x08A708B0u + (uint32_t)(i + i));
        uint32_t v = *W(0x08A708A4u + ((uint32_t)s << 2));
        *W(o + 96) = *B(o + 11) != 0 ? v : 0u - v;
        ARG(0) = o; ARG(1) = (uint32_t)(s + 0x35) & 0xFFFF; c->r[31] = 0x08936F94u; f_088616EC(c);
    } else if (k != 2) {
        *B(o + 77) = 2; face81(o);
        fx(c, o, -0x1D, 5, 0x08936FD8u);
    }
    leave(c, sv);
}
