/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 20 - 23
   instructions: the select screen big portrait's steps (each moves on and then keeps the portrait to
   its fighter, 0x0889D074). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0889CE64(Cpu *c); void f_0889D074(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

#define WITH_S01(o, f) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 24) = ra_; \
    c->r[17] = (o); c->r[16] = (f)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)

/* (portrait, fighter): when 0x0889CE64 says so, on a step. */
SFA_FUNC(0889AAC0, select_portrait_step) {
    gaddr o = ARG(0), f = ARG(1);
    WITH_S01(o, f);
    c->r[31] = 0x0889AADCu; f_0889CE64(c);
    if (c->r[2] != 0) *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
    call2(c, o, f, 0x0889AAFCu, f_0889D074);
    END_S01();
}

/* (portrait, fighter): on a step (+114 = +112, +96 cleared) once the fighter's +78 bit 0 is set. */
SFA_FUNC(0889AA6C, select_portrait_wait) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(f + 78) & 1) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 114) = (uint16_t)S16(o + 112);
        *W(o + 96) = 0;
    }
    call2(c, o, f, 0x0889AAB4u, f_0889D074);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (portrait, fighter): when 0x0889CE64 says so, on a step with x (+16, whole part) from +114. */
SFA_FUNC(0889AA10, select_portrait_step_in) {
    gaddr o = ARG(0), f = ARG(1);
    WITH_S01(o, f);
    c->r[31] = 0x0889AA2Cu; f_0889CE64(c);
    if (c->r[2] != 0) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *W(o + 16) = (uint32_t)(int32_t)S16(o + 114) << 16;
    }
    call2(c, o, f, 0x0889AA58u, f_0889D074);
    END_S01();
}
