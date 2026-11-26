/* Special moves with their later steps: a dash that stops once it would reverse, a dash by strength that
   waits on frame flags, and a start by button with a nudge (0x08972AAC). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_08837098(Cpu *c); void f_08861848(Cpu *c);
void f_0886469C(Cpu *c); void f_0886CA2C(Cpu *c); void f_08863114(Cpu *c); void f_0886A530(Cpu *c);
void f_0886DB2C(Cpu *c); void f_0890CC04(Cpu *c); void f_0898B0F0(Cpu *c); void f_0898C640(Cpu *c);

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
static void enter3(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (fighter): step 0: step 1, +262 / +73 = 1, 0x0898C640, the meter by 3 (1 with +836), 0x08837098, +840
   cleared and move strength + 0x1B; later 0x0898B0F0, stopped (+262 cleared) once x would run backward,
   +73 cleared on +76, +262 on a frame flag, standing once ended, else ticked. */
SFA_FUNC(0898B4B0, dash_until_reverse) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (*B(o + 7) == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1; *B(o + 73) = 1;
        call1(c, o, f_0898C640, 0x0898B4E4u);
        if (*W(o + 836) != 0) call2(c, o, 1, f_0886CA2C, 0x0898B50Cu);
        else call2(c, o, 3, f_0886CA2C, 0x0898B4FCu);
        call1(c, o, f_08837098, 0x0898B514u);
        int32_t s = S8(o + 194);
        *W(o + 840) = 0;
        call2(c, o, (uint32_t)(s + 0x1B) & 0xFFFF, f_088616EC, 0x0898B52Cu);
    } else {
        call1(c, o, f_0898B0F0, 0x0898B53Cu);
        int32_t v = (int32_t)*W(o + 96);
        if (*B(o + 11) == 0) v = -v;
        if (v < 0) { *B(o + 262) = 0; *W(o + 104) = 0; *W(o + 96) = 0; }
        if (S8(o + 76) != 0) *B(o + 73) = 0;
        if (S8(o + 77) != 0) *B(o + 262) = 0;
        if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x0898B598u);
        else call1(c, o, f_08861848, 0x0898B5A8u);
    }
    leave(c, sv);
}

/* (fighter): step 0: step 1, +262 = 1, the meter by 7, 0x08837098, +502 = 1, x / brake by strength from
   0x08A58388 and move strength + 2; step 1: ticked, step 2 on a frame flag; step 2: once ended 0x0886A530
   and standing, else a frame flag clears +262, 0x0890CC04 while x is not backward, 0x0886DB2C and ticked. */
SFA_FUNC(0890AB34, dash_by_strength_flags) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    uint32_t st = *B(o + 7);
    c->r[17] = 1;
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1;
        call2(c, o, 7, f_0886CA2C, 0x0890AB6Cu);
        call1(c, o, f_08837098, 0x0890AB74u);
        int32_t s = S8(o + 194);
        gaddr e = 0x08A58388u + ((uint32_t)(s << 1) << 2);
        *B(o + 502) = 1;
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
        call2(c, o, (uint32_t)(s + 2) & 0xFFFF, f_088616EC, 0x0890ABB0u);
    } else if (st == 1) {
        if (S8(o + 77) != 0) { *B(o + 77) = 0; *B(o + 7) = 2; }
        call1(c, o, f_08861848, 0x0890ABDCu);
    } else if (st == 2) {
        if (S8(o + 75) < 0) {
            call1(c, o, f_0886A530, 0x0890AC00u);
            call1(c, o, f_0886469C, 0x0890AC08u);
        } else {
            if (S8(o + 77) != 0) { *B(o + 262) = 0; *B(o + 77) = 0; }
            if ((int32_t)*W(o + 96) >= 0) call1(c, o, f_0890CC04, 0x0890AC34u);
            call1(c, o, f_0886DB2C, 0x0890AC3Cu);
            call1(c, o, f_08861848, 0x0890AC44u);
        }
    }
    leave3(c, sv);
}

/* (fighter): step strength + 1, facing the opponent, +235 = 1, 0x08863114, the meter by strength; at
   strength 1 with +290 bit 1 x 2.0 and move 0x22, with bit 0 and no choice made x -1.0 and move 0x22;
   otherwise x cleared there and move 2 x strength + 6 (+1 unlocked with no choice made without +290 bit
   0, else beyond the distance from 0x08A8FFAE) of its list. */
SFA_FUNC(08972AAC, special_by_button_nudge) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    *B(o + 7) = (uint8_t)(S8(o + 194) + 1);
    c->r[17] = 1;
    int32_t face = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)face;
    call1(c, o, f_08863114, 0x08972AE4u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08972AF0u);
    int32_t s = S8(o + 194);
    uint32_t n = (uint32_t)((s << 1) + 6) & 0xFFFF;
    if (s == 1) {
        uint32_t h = *H(o + 290);
        *W(o + 96) = 0;
        if (h & 3) {
            *W(o + 96) = 0x20000;
            if (h & 2) { call2(c, o, 0x22, f_08861674, 0x08972B48u); leave3(c, sv); return; }
            int32_t k = S8(o + 386);
            *W(o + 96) = 0;
            if (k == -1) {
                *W(o + 96) = 0xFFFF0000u;
                call2(c, o, 0x22, f_08861674, 0x08972B48u); leave3(c, sv); return;
            }
        }
    }
    if (S8(o + 373) == 0 && S8(o + 386) == 1) {
        if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF;
        call2(c, o, n, f_08861674, 0x08972B98u);
    } else {
        if (S16(0x08A8FFAEu + (uint32_t)(s + s)) < S16(o + 508)) n = (n + 1) & 0xFFFF;
        call2(c, o, n, f_08861674, 0x08972BD4u);
    }
    leave3(c, sv);
}
