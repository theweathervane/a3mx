/* Special moves with their later steps: a three-step special with a held count, and a timed special
   (0x0892BD04) whose start picks its move by choice, unlock and button. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08837098(Cpu *c); void f_08861848(Cpu *c); void f_0886469C(Cpu *c);
void f_0886CA2C(Cpu *c); void f_0886DB2C(Cpu *c); void f_0891F0C4(Cpu *c); void f_08864740(Cpu *c);
void f_0886E3CC(Cpu *c); void f_0892B188(Cpu *c);

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

/* (fighter): step 0: step 1, +262 = 1, +502 = 1 with +540 (else cleared and the meter by 4), 0x08837098
   and move strength + 0xD; step 1: on a frame flag step 2, +77 = 1, 0x0891F0C4, +824 cleared and +82 by
   strength from 0x08A64380, ticked; step 2: standing once ended, else +82 counts down (+262 cleared when
   it runs out) with 0x0886DB2C once out, ticked. */
SFA_FUNC(08919640, special_held_count) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1;
        int32_t k = S8(o + 540);
        *B(o + 502) = 1;
        if (k == 0) { *B(o + 502) = 0; call2(c, o, 4, f_0886CA2C, 0x08919684u); }
        call1(c, o, f_08837098, 0x0891968Cu);
        call2(c, o, (uint32_t)(S8(o + 194) + 0xD) & 0xFFFF, f_088616EC, 0x089196A0u);
    } else if (st == 1) {
        if (S8(o + 77) != 0) {
            *B(o + 7) = 2; *B(o + 77) = 1;
            call1(c, o, f_0891F0C4, 0x089196CCu);
            *B(o + 0x334 + 4) = 0;
            *H(o + 82) = *B(0x08A64380u + (uint32_t)S8(o + 194));
        }
        call1(c, o, f_08861848, 0x089196F8u);
    } else if (st == 2) {
        if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x0891971Cu);
        else {
            int32_t t = S16(o + 82);
            if (t != 0) {
                *H(o + 82) = (uint16_t)(t - 1);
                if ((int16_t)(t - 1) != 0) { call1(c, o, f_08861848, 0x08919750u); leave(c, sv); return; }
                *B(o + 262) = 0;
            }
            call1(c, o, f_0886DB2C, 0x08919768u);
            call1(c, o, f_08861848, 0x08919770u);
        }
    }
    leave(c, sv);
}

/* (fighter): step 0: step 1, 0x0892B188, +262 = 1, +82 = 0x48, +696 cleared; move 0x1B (0x23 with +235 = 1
   with no choice made), or 0x1C (0x24 with no choice made) with +696 = 1 when unlocked with +193 or
   locked with +290 bit 2; the meter by 4 (3 with +696), 0x08837098 and 0x0886E3CC (6, 0x38); later +82
   counts down, ticked until it runs out, then 0x08864740 with +696, else standing. */
SFA_FUNC(0892BD04, timed_special_48) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t st = *B(o + 7);
    enter(c, o, sv);
    if (st != 0) {
        int32_t t = S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t != 0) call1(c, o, f_08861848, 0x0892BD6Cu);
        else if (S8(o + 696) != 0) call1(c, o, f_08864740, 0x0892BD4Cu);
        else call1(c, o, f_0886469C, 0x0892BD5Cu);
        leave(c, sv); return;
    }
    *B(o + 7) = 1;
    call1(c, o, f_0892B188, 0x0892BD84u);
    *B(o + 262) = 1; *H(o + 82) = 0x48;
    int32_t k = S8(o + 386);
    uint32_t n = 0x1B;
    int32_t u = S8(o + 373);
    if (k == -1) { n = 0x23; *B(o + 235) = 1; }
    *B(o + 696) = 0;
    int alt = u != 0 ? S8(o + 193) != 0 : (*H(o + 290) & 4) != 0;
    if (alt) {
        n = k == -1 ? 0x24 : 0x1C;
        *B(o + 696) = (uint8_t)(S8(o + 696) + 1);
    }
    call2(c, o, n & 0xFFFF, f_088616EC, 0x0892BE00u);
    call2(c, o, S8(o + 696) == 0 ? 4 : 3, f_0886CA2C, 0x0892BE1Cu);
    call1(c, o, f_08837098, 0x0892BE24u);
    ARG(0) = o; ARG(1) = 6; ARG(2) = 0x38; c->r[31] = 0x0892BE34u; f_0886E3CC(c);
    leave(c, sv);
}
