/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 17 - 38
   instructions: the VS screen portrait's steps (each moves on and then keeps the portrait to its
   fighter, 0x0889B3B4) and a fighter's twelve figures for the select / VS screens. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088231C8(Cpu *c); void f_0889B3B4(Cpu *c); void f_0889CE64(Cpu *c); void f_0886CE24(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

#define WITH_S01(o, f) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 24) = ra_; \
    c->r[17] = (o); c->r[16] = (f)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)

/* (portrait, fighter): when 0x0889CE64 says so, gone (0x088231C8) with +10 bit 7, else on a step. */
SFA_FUNC(0889B344, vs_portrait_step_end) {
    gaddr o = ARG(0), f = ARG(1);
    WITH_S01(o, f);
    c->r[31] = 0x0889B360u; f_0889CE64(c);
    if (c->r[2] != 0) {
        if (*B(o + 10) & 0x80) call_on(c, o, 0x0889B380u, f_088231C8);
        else *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
    }
    call2(c, o, f, 0x0889B3A0u, f_0889B3B4);
    END_S01();
}

/* (portrait, fighter): when 0x0889CE64 says so, on a step with x (+18) from +114. */
SFA_FUNC(0889B254, vs_portrait_step_in) {
    gaddr o = ARG(0), f = ARG(1);
    WITH_S01(o, f);
    c->r[31] = 0x0889B270u; f_0889CE64(c);
    if (c->r[2] != 0) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 18) = (uint16_t)S16(o + 114);
    }
    call2(c, o, f, 0x0889B298u, f_0889B3B4);
    END_S01();
}

/* (portrait, fighter): on a step (+114 = +112, +96 cleared) once the fighter is ready: with +10 bit 7
   when the other side's fighter has +78 low nibble 0xF, else when this fighter's +78 bit 0 is set. */
SFA_FUNC(0889B2AC, vs_portrait_wait) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int go;
    if (*B(o + 10) & 0x80) {
        gaddr other = S8(f + 337) != 0 ? 0x092E3EA0u : 0x092E4338u;
        go = (S8(other + 78) & 0xF) == 0xF;
    } else go = (S8(f + 78) & 1) != 0;
    if (go) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 114) = (uint16_t)S16(o + 112);
        *W(o + 96) = 0;
    }
    call2(c, o, f, 0x0889B338u, f_0889B3B4);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its twelve figures (0x0886CE24 for n = 11 down to 0): each walks a chain of 10-byte steps
   (from the list at +416's +12, +0x2C back) adding up their times (+0) until one whose 12-byte row
   (+8 into +416's +16) has +1 set; a step with +1 bit 7 ends the chain with no figure. */
SFA_FUNC(0886CD5C, fighter_all_figures) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = f;
    c->r[17] = *W(*W(f + 416) + 12) + 0x2C;
    for (c->r[18] = 11; (int32_t)c->r[18] >= 0; c->r[18]--, c->r[17] -= 4) {
        gaddr step = *W(c->r[17]);
        gaddr rows = *W(*W(f + 416) + 16);
        gaddr row = rows + 12u * *H(step + 8);
        uint32_t time = 0;
        for (;;) {
            if (*B(row + 1) != 0) {
                ARG(0) = f; ARG(1) = row; ARG(2) = c->r[18]; ARG(3) = time;
                c->r[31] = 0x0886CDC8u; f_0886CE24(c);
                break;
            }
            time += *B(step);
            int last = (S8(step + 1) & 0x80) != 0;
            step += 10;
            if (last) break;
            row = rows + 12u * *H(step + 8);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
