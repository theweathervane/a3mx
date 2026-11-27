/* More of what the new leaves set free: a reaching grab's step, and an entrance that walks in when
   the opponent is a certain character. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08937790(Cpu *c); void f_08821330(Cpu *c); void f_0893B674(Cpu *c);
void f_0886E3CC(Cpu *c); void f_0887DD30(Cpu *c); void f_089371B8(Cpu *c); void f_0886175C(Cpu *c);
void f_08937820(Cpu *c); void f_0886E17C(Cpu *c); void f_0886163C(Cpu *c); void f_0891EA80(Cpu *c);
void f_0891F038(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (fighter): a reaching grab's step: ticked while +77 is 0x80; else the reach (+0x3B8) grows by the
   speed and 0x08937790 runs; at the animation's end, or reaching more than 0x180 past what 0x08821330
   returns, the miss (0x089371B8). Else 0x0893B674 (0x3E + strength, the reach's place) says: 0 goes
   on (0x0886E3CC 7 / 4, ticked); below 0 the cue 0x41 and the miss; above 0 the catch: the next step,
   +196..+199 cleared, +573 = -1, the opponent's +579 cleared, the box list 0x08A6E710 and the
   animation frame (0x08A6E544, 10 bytes each) by an eighth of the distance less 16 (44 at most), then
   0x0886175C, 0x08937820 and 0x0886E17C. A frame keeping s0, s1. */
SFA_FUNC(08936FE8, reach_grab_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t busy = (uint32_t)(int32_t)S8(o + 77) & 0xFF;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (busy == 0x80) { call1(c, o, f_08861848, 0x089371A4u); goto out; }
    c->r[17] = o + 0x3A0;
    {
        uint32_t reach = *W(o + 0x3A0 + 24) + *W(o + 96);
        c->r[6] = *W(o + 96);
        *W(o + 0x3A0 + 24) = reach;
        call2(c, o, reach, f_08937790, 0x08937028u);
    }
    int miss = S8(o + 75) < 0;
    if (!miss) {
        call1(c, o, f_08821330, 0x0893703Cu);
        uint32_t at = *W(o + 0x3A0 + 24) + *W(o + 16);
        c->r[17] = at;
        if (0x1800000u < at - *W(c->r[2] + 16)) miss = 1;
        else {
            call3(c, o, (uint32_t)(S8(o + 194) + 0x3E), at, f_0893B674, 0x08937074u);
            int32_t got = (int32_t)c->r[2];
            c->r[17] = (uint32_t)got;
            if (got == 0) {
                call3(c, o, 7, 4, f_0886E3CC, 0x0893708Cu);
                call1(c, o, f_08861848, 0x08937094u);
                goto out;
            }
            if (got < 0) { call2(c, o, 0x41, f_0887DD30, 0x089370B8u); miss = 1; }
            else {
                uint8_t step = *B(o + 7);
                *B(o + 196) = 0;
                *B(o + 7) = (uint8_t)(step + 1);
                *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
                gaddr other = *W(o + 92);
                *B(o + 573) = 0xFF;
                *B(other + 579) = 0;
                int32_t d = (int16_t)(S16(other + 18) - S16(o + 18));
                if (d < 0) d = (int16_t)-d;
                d = (int16_t)(d - 16);
                d = (int16_t)(d >> 3);
                if (!((int32_t)((uint32_t)d & 0xFFFF) < 45)) d = 0x2C;
                *W(*W(o + 416) + 16) = 0x08A6E710u;
                *W(o + 28) = 0x08A6E544u + (uint32_t)d * 10;
                call1(c, o, f_0886175C, 0x08937178u);
                call1(c, o, f_08937820, 0x08937180u);
                call1(c, o, f_0886E17C, 0x08937188u);
                goto out;
            }
        }
    }
    if (miss) call1(c, o, f_089371B8, 0x089370C0u);
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): an entrance, by its step +6. 0: shown; against character 10 with +430 set it walks in:
   step 2, 63 frames, from 0x40 behind where it stands, at 1.0, with the cue 0x34; else step 1 in
   the air with 0x0891EA80 and the cue 0x22. 1: ticked to the animation's end, then set down in state
   1 / 0 / 0 / 0 with the cue 0. 2: moved by hand until the time runs out, then step 3 with the cue
   0x35. 3: on +77 (cleared) the object 0x0891F038, the cue 0x44 and 0x0886E3CC (7, 4); at the
   animation's end state 1 / 0 / 0 / 0 with the cue 0, else ticked. */
SFA_FUNC(089180E0, entrance_walk_in) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint32_t k = *B(o + 6);
    *W(c->r[29] + 20) = ra;
    if (k == 0) {
        gaddr other = *W(o + 92);
        *B(o + 1) = 1;
        if (S8(other + 338) == 0xA && S8(other + 430) != 0) {
            *B(o + 6) = 2;
            uint8_t facing = *B(o + 11);
            *H(o + 82) = 0x3F;
            int32_t x = S16(o + 18);
            if (facing != 0) { *H(o + 18) = (uint16_t)(x + 0x40); *W(o + 96) = 0xFFFF0000u; }
            else { *H(o + 18) = (uint16_t)(x - 0x40); *W(o + 96) = 0x10000; }
            call2(c, o, 0x34, f_0886163C, 0x08918174u);
        } else {
            *B(o + 6) = 1; *B(o + 73) = 1;
            call1(c, o, f_0891EA80, 0x08918190u);
            call2(c, o, 0x22, f_0886163C, 0x0891819Cu);
        }
    } else if (k == 1) {
        if (S8(o + 75) < 0) {
            *B(o + 4) = 1; *B(o + 5) = 0;
            int16_t ground = S16(o + 132);
            *B(o + 6) = 0; *B(o + 7) = 0;
            *H(o + 22) = (uint16_t)ground;
            *B(o + 73) = 0;
            *H(o + 20) = 0;
            call2(c, o, 0, f_0886163C, 0x089181FCu);
        } else call1(c, o, f_08861848, 0x089181C0u);
    } else if (k == 2) {
        uint32_t x = *W(o + 16), v = *W(o + 96);
        int32_t t = S16(o + 82) - 1;
        *W(o + 16) = x + v;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) {
            *B(o + 6) = (uint8_t)(k + 1);
            call2(c, o, 0x35, f_0886163C, 0x08918258u);
        } else call1(c, o, f_08861848, 0x0891823Cu);
    } else if (k == 3) {
        if (S8(o + 77) != 0) {
            *B(o + 77) = 0;
            call1(c, o, f_0891F038, 0x08918280u);
            call2(c, o, 0x44, f_0887DD30, 0x0891828Cu);
            call3(c, o, 7, 4, f_0886E3CC, 0x0891829Cu);
        }
        if (S8(o + 75) < 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            call2(c, o, 0, f_0886163C, 0x089182D4u);
        } else call1(c, o, f_08861848, 0x089182B0u);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
