/* Small functions the scenarios reach, 37 - 66 instructions: two command-input steps for a held
   direction, two thrown objects knocked away by a hit. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08867C84(Cpu *c); void f_088681F8(Cpu *c); void f_08868208(Cpu *c); void f_08821424(Cpu *c);
void f_0887DD30(Cpu *c); void f_08970008(Cpu *c); void f_0896FF9C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* Whether the directions the entry wants (at x) are all held (+764), else 0x08867C84 (fighter, arg). */
static int dir_held(Cpu *c, gaddr f, gaddr x, uint32_t arg, uint32_t back) {
    uint32_t want = *H(x);
    uint32_t got = (*H(f + 764) & 0x3F0 & want) & 0xFFFF;
    if (got == want) return 1;
    call2(c, f, arg, back, f_08867C84);
    return c->r[2] != 0;
}

/* (fighter, step, entry, arg): the direction held, the step moves on (+0) with a 0x20-frame window (+4)
   and +2 cleared (0x08868208); else it lapses (0x088681F8). */
SFA_FUNC(0886954C, command_dir_step) {
    gaddr f = ARG(0), t = ARG(1), x = ARG(2);
    uint32_t arg = ARG(3);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = t;
    if (dir_held(c, f, x, arg, 0x0886958Cu)) {
        *B(t) = (uint8_t)(*B(t) + 1);
        *B(t + 4) = 0x20;
        *H(t + 2) = 0;
        call_on(c, t, 0x088695D0u, f_08868208);
    } else call_on(c, t, 0x0886959Cu, f_088681F8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, step, entry, arg): the direction held, +2 counts up and +4 down, moving on (+0) at 0 (both
   0x08868208); else it lapses (0x088681F8). */
SFA_FUNC(088695E0, command_dir_hold) {
    gaddr f = ARG(0), t = ARG(1), x = ARG(2);
    uint32_t arg = ARG(3);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = t;
    if (dir_held(c, f, x, arg, 0x08869620u)) {
        *H(t + 2) = (uint16_t)(*H(t + 2) + 1);
        uint32_t n = (uint32_t)*B(t + 4) - 1;
        *B(t + 4) = (uint8_t)n;
        if ((n & 0xFF) == 0) {
            *B(t) = (uint8_t)(*B(t) + 1);
            call_on(c, t, 0x08869690u, f_08868208);
        } else call_on(c, t, 0x08869658u, f_08868208);
    } else call_on(c, t, 0x08869670u, f_088681F8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): hit by a fighter (0x08970008) it is taken (+0 = 1, +6 on, +88 / +121 from the hitter, +73 = 1,
   facing away from it), knocked by the hitter's strength (+194) row of 0x08A8E0A0 (x step, y step,
   y change), +0x52 = 0x10, 0x0887DD30 (4); its life (+0x53) runs out to step 2/2; drawn. */
SFA_FUNC(0896FD94, object_knocked) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call_on(c, o, 0x0896FDACu, f_08970008);
    gaddr h = c->r[2];
    c->r[17] = o + 0x52;
    if (h != 0) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *B(o) = 1;
        *B(o + 121) = (uint8_t)S8(h + 121);
        *W(o + 88) = h;
        *B(o + 73) = 1;
        *B(o + 11) = (S16(o + 18) - S16(h + 18) >= 0) ? 1 : 0;
        *B(o + 0x52) = 0x10;
        gaddr e = 0x08A8E0A0u + 12u * (uint32_t)S8(h + 194);
        *W(o + 104) = 0;
        *W(o + 96) = *W(e);
        *W(o + 100) = *W(e + 4);
        *W(o + 108) = *W(e + 8);
        call2(c, o, 4, 0x0896FE48u, f_0887DD30);
    }
    int8_t life = (int8_t)(S8(o + 0x53) - 1);
    *B(o + 0x53) = (uint8_t)life;
    if (life == 0) { *B(o + 4) = 2; *B(o + 5) = 2; *B(o + 6) = 0; *B(o + 7) = 0; }
    call_on(c, o, 0x0896FE80u, f_08821424);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): once its hit pause (+0x52) is over, a fighter's hit (0x08970008) takes it (+0 = 1, +6 on,
   +88 / +121 from the hitter) and knocks it by the hitter's strength (+194) row of 0x08A8E0D0 (the air
   rows for an airborne hitter, further on when falling), 0x0887DD30 (4); then its frame (0x0896FF9C). */
SFA_FUNC(0896FE94, object_knocked_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    gaddr t = o + 0x52;
    int waiting = 0;
    if (S8(t) != 0) {
        int8_t v = (int8_t)(S8(t) - 1);
        *B(t) = (uint8_t)v;
        waiting = v != 0;
    }
    if (waiting) call_on(c, o, 0x0896FEFCu, f_0896FF9C);
    else {
        call_on(c, o, 0x0896FEE0u, f_08970008);
        gaddr h = c->r[2];
        if (h != 0) {
            *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
            *B(o) = 1;
            *W(o + 88) = h;
            *B(o + 121) = (uint8_t)S8(h + 121);
            uint32_t k = 3u * (uint32_t)S8(h + 194);
            if (S8(h + 73) != 0) {
                k += 0xC;
                if ((int32_t)*W(h + 100) < 0) k += 0xC;
            }
            gaddr e = 0x08A8E0D0u + (k << 2);
            *W(o + 104) = 0;
            *W(o + 96) = *W(e);
            *W(o + 100) = *W(e + 4);
            *W(o + 108) = *W(e + 8);
            call2(c, o, 4, 0x0896FF84u, f_0887DD30);
        }
        call_on(c, o, 0x0896FF8Cu, f_0896FF9C);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
