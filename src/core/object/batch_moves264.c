/* Normal attacks with steps of their own around the shared attack step (0x0886D678). Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886D678(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c);
void f_08987440(Cpu *c); void f_08864740(Cpu *c); void f_08861848(Cpu *c);

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

/* (fighter): step 0: step 1, +235 = 1, facing, 0x08863114; at strength 2 step 2 with x 14.0 and brake
   -1.0; the meter by strength and move strength + 0xF (0x20 at strength 0 with no choice made). Steps 1 and
   2 are the attack step (2 moves on with a frame flag); step 3 0x08987440 and on once x runs backward, then
   the attack step; step 4 crouches once ended, else ticked. */
SFA_FUNC(08983AB4, attack_rush_steps) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 235) = 1; *B(o + 11) = *B(o + 697);
        call1(c, o, f_08863114, 0x08983AECu);
        int32_t s = S8(o + 194);
        if (s == 2) {
            *B(o + 7) = 2;
            *W(o + 96) = 0xE0000; *W(o + 100) = 0; *W(o + 104) = 0xFFFF0000u; *W(o + 108) = 0;
        }
        call2(c, o, (uint32_t)s, f_0886CA2C, 0x08983B28u);
        uint32_t n = (uint32_t)(S8(o + 194) + 0xF);
        if (S8(o + 386) == -1 && n == 0xF) n = 0x20;
        call2(c, o, n, f_08861674, 0x08983B58u);
    } else if (st == 1) call1(c, o, f_0886D678, 0x08983B70u);
    else if (st == 2) {
        if (S8(o + 77) != 0) *B(o + 7) = 3;
        call1(c, o, f_0886D678, 0x08983B98u);
    } else if (st == 3) {
        call1(c, o, f_08987440, 0x08983BB0u);
        if ((int32_t)*W(o + 96) < 0) *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call1(c, o, f_0886D678, 0x08983BD0u);
    } else if (st == 4) {
        if (S8(o + 75) < 0) call1(c, o, f_08864740, 0x08983BF4u);
        else call1(c, o, f_08861848, 0x08983C04u);
    }
    leave(c, sv);
}
/* (fighter): standing: +696 cleared and the step's handler from 0x08A9E6E0 (0x08A9E6E8 with +193).
   Crouching: +696 = 1; step 0: +235 = 1, step 1 (2 for +193 at strength 2), facing, 0x08863114, the meter
   by strength and move strength + 0xC (+3 with +193); step 1 the attack step; later crouched once ended,
   else ticked and moved by its frame push (+76, cleared) toward its facing. */
SFA_FUNC(089A58D0, crouch_attack_push) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (S8(o + 192) == 0) {
        *B(o + 696) = 0;
        int k = S8(o + 193) != 0;
        gaddr tab = k ? 0x08A9E6E8u : 0x08A9E6E0u;
        uint32_t fn = *W(tab + (st << 2));
        ARG(0) = o; ARG(1) = fn; c->r[31] = k ? 0x089A5934u : 0x089A5914u;
        rt_call_indirect(c, fn);
    } else {
        *B(o + 696) = 1;
        if (st == 0) {
            *B(o + 235) = 1; *B(o + 7) = 1;
            int k = S8(o + 193);
            *B(o + 11) = *B(o + 697);
            if (k != 0 && S8(o + 194) == 2) *B(o + 7) = 2;
            call1(c, o, f_08863114, 0x089A597Cu);
            call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x089A5988u);
            int32_t n = S8(o + 194);
            if (S8(o + 193) != 0) n = (int16_t)(n + 3);
            n = (int16_t)(n + 0xC);
            call2(c, o, (uint32_t)n & 0xFFFF, f_08861674, 0x089A59C0u);
        } else if (st == 1) call1(c, o, f_0886D678, 0x089A59D8u);
        else if (S8(o + 75) < 0) call1(c, o, f_08864740, 0x089A5A2Cu);
        else {
            call1(c, o, f_08861848, 0x089A59F0u);
            int32_t d = S8(o + 76);
            *B(o + 76) = 0;
            *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) != 0 ? d : -d));
        }
    }
    leave(c, sv);
}