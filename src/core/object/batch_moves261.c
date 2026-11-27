/* Normal-attack starts that hand over to the shared attack step (0x0886D678): step 0 faces the opponent,
   gains meter by strength and picks the move, later steps are the attack step. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886D678(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c);

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

/* (fighter): the attack step; once its frame flag is clear +7 on, +82 = 0xB and x 7.0 toward its facing
   with brake 0.5. */
SFA_FUNC(089A2D74, attack_then_dash) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_0886D678, 0x089A2D88u);
    if (S8(o + 77) == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *H(o + 82) = 0xB;
        if (*B(o + 11) != 0) { *W(o + 96) = 0x70000; *W(o + 104) = 0xFFFF8000u; }
        else { *W(o + 96) = 0xFFF90000u; *W(o + 104) = 0x8000; }
    }
    leave(c, sv);
}

/* (fighter): step 0: step 1, +235 = 1, facing the opponent, 0x08863114, the meter by strength and move
   strength + 0xC (kept to 16 bits with `mask`); step 1 the attack step. */
static void start_c(Cpu *c, int mask, const uint32_t b[4]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st == 0) {
        *B(o + 7) = 1;
        *B(o + 235) = 1; *B(o + 11) = *B(o + 697);
        call1(c, o, f_08863114, b[0]);
        call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, b[1]);
        uint32_t n = (uint32_t)(S8(o + 194) + 0xC);
        call2(c, o, mask ? n & 0xFFFF : n, f_08861674, b[2]);
    } else if (st == 1) call1(c, o, f_0886D678, b[3]);
    leave(c, sv);
}
SFA_FUNC(08983A38, attack_start_c) {
    static const uint32_t b[4] = { 0x08983A70u, 0x08983A7Cu, 0x08983A8Cu, 0x08983AA4u };
    start_c(c, 0, b);
}
SFA_FUNC(089093FC, attack_start_c16) {
    static const uint32_t b[4] = { 0x08909434u, 0x08909440u, 0x08909454u, 0x0890946Cu };
    start_c(c, 1, b);
}

/* (fighter): step 0: step 1, +235 = 1, facing, 0x08863114, the meter by strength and move strength + 0xC
   (+3 with +193); later the attack step. */
SFA_FUNC(088F6EBC, attack_start_c_193) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (*B(o + 7) == 0) {
        *B(o + 235) = 1; *B(o + 7) = 1; *B(o + 11) = *B(o + 697);
        call1(c, o, f_08863114, 0x088F6EF4u);
        int32_t s = S8(o + 194);
        c->r[17] = (uint32_t)s;
        call2(c, o, (uint32_t)s, f_0886CA2C, 0x088F6F04u);
        if (S8(o + 193) != 0) { s += 3; c->r[17] = (uint32_t)s; }
        call2(c, o, (uint32_t)(s + 0xC) & 0xFFFF, f_08861674, 0x088F6F20u);
    } else call1(c, o, f_0886D678, 0x088F6F30u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

void f_088616EC(Cpu *c); void f_08861EDC(Cpu *c); void f_088EE894(Cpu *c); void f_088EE90C(Cpu *c);
void f_0892B188(Cpu *c);

/* (fighter): without a frame flag +7 on and the attack step; with one, a fighter with no choice made
   and no lock holding button bit 8 (+760) takes +7 on and move 0x1F, everyone else the attack step. */
SFA_FUNC(089259F8, attack_or_1f) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 77) == 0) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); c->r[31] = 0x08925A1Cu; f_0886D678(c); }
    else if (S8(o + 373) == 0 && S8(o + 386) != -1 && (*H(o + 760) & 0x100)) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        ARG(1) = 0x1F; c->r[31] = 0x08925A64u; f_088616EC(c);
    } else { c->r[31] = 0x08925A78u; f_0886D678(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the attack step; on a hit (+123) the opponent's +586 / +587 cleared from the second hit (+668),
   and on the first counted hit (+828 1) its pending flag let go (+592 -> +270, with +773) and +125
   cleared, the fighter in the air, +668 on and 0x08861EDC. */
SFA_FUNC(089C02F8, attack_hit_count) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92);
    c->r[17] = p;
    call1(c, o, f_0886D678, 0x089C0314u);
    if (S8(o + 123) != 0) {
        if (S8(o + 668) >= 2) { *B(p + 586) = 0; *B(p + 587) = 0; }
        if (*W(o + 828) == 1) {
            if (S8(p + 773) != 0 && S8(p + 592) != 0) { *B(p + 270) = 1; *B(p + 592) = 0; }
            *B(p + 125) = 0;
            int32_t n = S8(o + 668);
            *B(o + 73) = 1;
            *B(o + 668) = (uint8_t)(n + 1);
            call1(c, o, f_08861EDC, 0x089C0380u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): standing (+192 clear): +696 cleared and the step's handler from 0x08A43A0C (0x08A43A1C with
   +193); crouching: +696 = 1, step 0 is 0x088EE894, later 0x088EE90C. */
SFA_FUNC(088EE2CC, attack_by_stance) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t st = *B(o + 7);
    if (S8(o + 192) == 0) {
        *B(o + 696) = 0;
        gaddr tab = S8(o + 193) != 0 ? 0x08A43A1Cu : 0x08A43A0Cu;
        uint32_t fn = *W(tab + (st << 2));
        ARG(1) = fn; ARG(2) = tab;
        c->r[31] = S8(o + 193) != 0 ? 0x088EE32Cu : 0x088EE308u;
        rt_call_indirect(c, fn);
    } else {
        *B(o + 696) = 1; ARG(2) = 1;
        if (st == 0) { c->r[31] = 0x088EE34Cu; f_088EE894(c); }
        else { c->r[31] = 0x088EE360u; f_088EE90C(c); }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): +696 = 1; step 0: step 1, facing, +235 = 1, 0x0892B188, 0x08863114, the meter by strength
   and move strength + 0xC (0xF with +193); later the attack step. */
SFA_FUNC(0892A258, attack_start_b188) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *B(o + 696) = 1;
    *W(c->r[29] + 24) = ra; *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    if (*B(o + 7) != 0) call1(c, o, f_0886D678, 0x0892A284u);
    else {
        *B(o + 7) = 1; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
        call1(c, o, f_0892B188, 0x0892A2A8u);
        call1(c, o, f_08863114, 0x0892A2B0u);
        uint32_t s = (uint32_t)S8(o + 194) & 0xFFFF;
        c->r[17] = s;
        call2(c, o, s, f_0886CA2C, 0x0892A2CCu);
        uint32_t base = S8(o + 193) != 0 ? 0xF : 0xC;
        call2(c, o, (base + s) & 0xFFFF, f_08861674, 0x0892A2ECu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 0: step 1, facing, +235 = 1, 0x08863114 and move 7 (unlocked with no choice made
   without +290 bit 0), else 6 under 49 away and 7 beyond; later the attack step. */
SFA_FUNC(08966C94, attack_start_6_7) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t st = *B(o + 7);
    c->r[17] = st; c->r[16] = o;
    if (st != 0) call1(c, o, f_0886D678, 0x08966D30u);
    else {
        *B(o + 7) = 1;
        c->r[17] = 1;
        *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
        call1(c, o, f_08863114, 0x08966CD0u);
        uint32_t n;
        if (S8(o + 373) == 0 && S8(o + 386) == 1 && (*H(o + 290) & 1) == 0) n = 7;
        else n = S16(o + 508) < 49 ? 6 : 7;
        call2(c, o, n, f_08861674, 0x08966D20u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}