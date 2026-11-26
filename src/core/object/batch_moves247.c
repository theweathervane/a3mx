/* Special-move steps that set +573 through 0x0886E294 and call the effect spawner 0x0887FD2C. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0887FD2C(Cpu *c); void f_0886E294(Cpu *c); void f_088616EC(Cpu *c);
void f_089164E8(Cpu *c);

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

/* (fighter): +656 cleared; on frame flag 2 +237 cleared, 0x0886E294 and +573 from 0x08A66DE2 by its
   result (by strength without +81); on frame flag 1 +237 toward +81, the flag cleared and the effect at
   (0x2F, 0x41). */
SFA_FUNC(08921FD8, fx_573_2f) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    int32_t k = S8(o + 77);
    *B(o + 656) = 0;
    if (k == 2) {
        *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x08922004u);
        uint32_t i = S8(o + 81) != 0 ? c->r[2] : (uint32_t)S8(o + 194);
        *B(o + 573) = *B(0x08A66DE2u + i);
    } else if (k == 1) {
        face81(o); *B(o + 77) = 0;
        fx(c, o, 0x2F, 0x41, 0x08922068u);
    }
    leave(c, sv);
}

/* (fighter): with the count +823 running: the effect at (-0x26, 0x62) at 0x31, down a frame, and at the
   end +237 cleared, 0x0886E294 and +573 cleared; else ticked until a frame flag starts it at 0xA with +7 on. */
SFA_FUNC(0890A1D8, fx_count_31) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr q = o + 0x334;
    c->r[17] = q;
    *W(c->r[29] + 24) = ra;
    int32_t v = S8(q + 3);
    if (v == 0) {
        if (S8(o + 77) != 0) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); *B(q + 3) = 0xA; }
        else call1(c, o, f_08861848, 0x0890A258u);
    } else {
        int32_t n = v - 1;
        if (v == 0x31) { fx(c, o, -0x26, 0x62, 0x0890A214u); n = S8(q + 3) - 1; }
        *B(q + 3) = (uint8_t)n;
        if ((int8_t)n == 0) {
            *B(o + 237) = 0;
            call1(c, o, f_0886E294, 0x0890A23Cu);
            *B(o + 573) = 0;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked; on +77 it clears, +237 toward +81 and the effect at (-0xA, 0x72); once ended +7 on,
   +237 cleared, 0x0886E294, +573 from 0x08A5FFE8 by strength (0 with +81), move strength + 0x40 and
   0x089164E8. */
SFA_FUNC(08916424, fx_end_40) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x08916438u);
    if (S8(o + 77) != 0) {
        *B(o + 77) = 0; face81(o);
        fx(c, o, -0xA, 0x72, 0x0891646Cu);
    }
    if (S8(o + 75) != 0) {
        *B(o + 237) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call1(c, o, f_0886E294, 0x08916490u);
        int32_t s = S8(o + 194);
        int32_t v = S8(o + 81) != 0 ? 0 : (int8_t)*H(0x08A5FFE8u + (uint32_t)(s + s));
        *B(o + 573) = (uint8_t)v;
        ARG(0) = o; ARG(1) = (uint32_t)(s + 0x40) & 0xFFFF; c->r[31] = 0x089164D0u; f_088616EC(c);
        call1(c, o, f_089164E8, 0x089164D8u);
    }
    leave(c, sv);
}
