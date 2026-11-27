/* Normal-attack starts that pick their move by button or distance and then hand over to the shared attack
   step (0x0886D678). Offsets stay numbers until something shows what they are.
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
/* step 0's opening: step 1, facing the opponent, +235 = 1 and 0x08863114 */
static void open_face(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 7) = 1; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
    call1(c, o, f_08863114, back);
}

/* (fighter): step 0: the opening, the meter by strength and move 2 x strength + 6 (+1 for a choice above 0
   without +290 bit 0, else beyond the distance from 0x08A58274); step 1 the attack step. */
SFA_FUNC(0890920C, attack_start_2s6) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st == 0) {
        open_face(c, o, 0x08909244u);
        call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x08909250u);
        int32_t s = S8(o + 194);
        uint32_t n = (uint32_t)((s << 1) + 6);
        if (S8(o + 373) == 0 && S8(o + 386) > 0) { if ((*H(o + 290) & 1) == 0) n++; }
        else if (*H(0x08A58274u + (uint32_t)(s + s)) < S16(o + 508)) n++;
        call2(c, o, n & 0xFFFF, f_08861674, 0x089092B8u);
    } else if (st == 1) call1(c, o, f_0886D678, 0x089092D0u);
    leave(c, sv);
}

/* (fighter): step 0: the opening, the meter by 2; with +290 bit 1 move 0x1F and +310 = 1, else +310
   cleared and move 0xB (unlocked with no choice made and without +290 bit 0), otherwise 0xA under 49
   away and 0xB beyond; later the attack step. */
SFA_FUNC(08966E24, attack_start_a_b) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t st = *B(o + 7);
    c->r[17] = st; c->r[16] = o;
    if (st != 0) call1(c, o, f_0886D678, 0x08966EE4u);
    else {
        c->r[17] = 1;
        open_face(c, o, 0x08966E60u);
        call2(c, o, 2, f_0886CA2C, 0x08966E6Cu);
        uint32_t h = *H(o + 290);
        if (h & 2) { call2(c, o, 0x1F, f_08861674, 0x08966E88u); *B(o + 310) = 1; }
        else {
            *B(o + 310) = 0;
            uint32_t n;
            if (S8(o + 373) == 0 && S8(o + 386) == 1 && (h & 1) == 0) n = 0xB;
            else n = S16(o + 508) < 49 ? 0xA : 0xB;
            call2(c, o, n, f_08861674, 0x08966ED4u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 0: the opening, the meter by strength and move 2 x strength (+1 for a choice above 0
   without +290 bit 0, else beyond the distance from 0x08A944B4; with no choice made 2 becomes 0x1F); later
   the attack step. */
SFA_FUNC(08983604, attack_start_2s_1f) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (*B(o + 7) == 0) {
        open_face(c, o, 0x0898363Cu);
        call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x08983648u);
        int32_t s = S8(o + 194), k = S8(o + 386);
        uint32_t n = (uint32_t)(s << 1);
        if (S8(o + 373) == 0 && k > 0) { if ((*H(o + 290) & 1) == 0) n++; }
        else if (*H(0x08A944B4u + (uint32_t)(s + s)) < S16(o + 508)) n++;
        if (k == -1 && n == 2) n = 0x1F;
        call2(c, o, n, f_08861674, 0x089836C0u);
    } else call1(c, o, f_0886D678, 0x089836D0u);
    leave(c, sv);
}

/* (fighter): step 0: the opening, the meter by 1; with +290 bit 1 move 0x1E (asked twice), else 9
   (unlocked with no choice made without +290 bit 0), otherwise 8 under 49 away and 9 beyond; later the
   attack step. A frame keeping s0..s2. */
SFA_FUNC(08966D44, attack_start_8_9) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2;
    uint32_t st = *B(o + 7);
    c->r[18] = st;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra; *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    if (st != 0) call1(c, o, f_0886D678, 0x08966E0Cu);
    else {
        c->r[18] = 1;
        open_face(c, o, 0x08966D84u);
        call2(c, o, 1, f_0886CA2C, 0x08966D90u);
        uint32_t h = *H(o + 290);
        c->r[17] = 0x1E;
        if (h & 2) call2(c, o, 0x1E, f_08861674, 0x08966DACu);
        else {
            uint32_t n;
            if (S8(o + 373) == 0 && S8(o + 386) == 1 && (h & 1) == 0) n = 9;
            else n = S16(o + 508) < 49 ? 8 : 9;
            c->r[17] = n;
        }
        call2(c, o, c->r[17], f_08861674, 0x08966DFCu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): standing: +696 cleared and the step's handler from 0x08A9BE80 (0x08A9BE88 with +193);
   crouching: +696 = 1, step 0 the opening, the meter by strength and move strength + 0xC (0xF with +193),
   later the attack step. */
SFA_FUNC(0899E38C, attack_by_stance_b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (S8(o + 192) == 0) {
        *B(o + 696) = 0;
        int k = S8(o + 193) != 0;
        gaddr tab = k ? 0x08A9BE88u : 0x08A9BE80u;
        uint32_t fn = *W(tab + (st << 2));
        ARG(0) = o; ARG(1) = fn; c->r[31] = k ? 0x0899E3F0u : 0x0899E3D0u;
        rt_call_indirect(c, fn);
    } else {
        *B(o + 696) = 1;
        if (st != 0) call1(c, o, f_0886D678, 0x0899E468u);
        else {
            open_face(c, o, 0x0899E420u);
            call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x0899E42Cu);
            uint32_t n = (uint32_t)(S8(o + 194) + (S8(o + 193) != 0 ? 0xF : 0xC)) & 0xFFFF;
            call2(c, o, n, f_08861674, 0x0899E458u);
        }
    }
    leave(c, sv);
}

/* (fighter): step 0: the opening, the meter by strength and move 2 x strength (+1 unlocked with no choice
   made without +290 bit 0, else beyond the distance from 0x08A8B77C); move 5 with no choice made sets
   +310 = 1; later the attack step. */
SFA_FUNC(08966B54, attack_start_2s_310) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    uint32_t st = *B(o + 7);
    c->r[17] = st;
    *W(c->r[29] + 24) = ra; *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    if (st != 0) call1(c, o, f_0886D678, 0x08966C4Cu);
    else {
        c->r[17] = 1;
        open_face(c, o, 0x08966B90u);
        call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x08966B9Cu);
        int32_t s = S8(o + 194);
        int32_t n = (int16_t)(s << 1);
        if (S8(o + 373) == 0 && S8(o + 386) == 1) { if ((*H(o + 290) & 1) == 0) n = (int16_t)(n + 1); }
        else if (S16(0x08A8B77Cu + (uint32_t)(s + s)) < S16(o + 508)) n = (int16_t)(n + 1);
        if (n == 5 && S8(o + 386) == -1) *B(o + 310) = 1;
        call2(c, o, (uint32_t)n & 0xFFFF, f_08861674, 0x08966C3Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 0: the opening, the meter by strength and move 2 x strength (+6 with +193; +1 unlocked
   with no choice made without +290 bit 0, else beyond the distance from 0x08A85976 / 0x08A8597C); later
   the attack step. */
SFA_FUNC(0895C18C, attack_start_2s_193) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    uint32_t st = *B(o + 7);
    c->r[17] = st;
    *W(c->r[29] + 24) = ra; *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    if (st != 0) call1(c, o, f_0886D678, 0x0895C288u);
    else {
        c->r[17] = 1;
        open_face(c, o, 0x0895C1C8u);
        call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x0895C1D4u);
        int32_t s = S8(o + 194);
        uint32_t n; int32_t lim;
        if (S8(o + 193) != 0) { n = (uint32_t)((s << 1) + 6) & 0xFFFF; lim = *H(0x08A8597Cu + (uint32_t)(s + s)); }
        else { n = (uint32_t)(s << 1) & 0xFFFF; lim = *H(0x08A85976u + (uint32_t)(s + s)); }
        if (S8(o + 373) == 0 && S8(o + 386) == 1) { if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF; }
        else if (lim < S16(o + 508)) n = (n + 1) & 0xFFFF;
        call2(c, o, n, f_08861674, 0x0895C278u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}