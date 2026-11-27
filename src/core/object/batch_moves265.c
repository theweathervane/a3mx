/* More normal attacks with steps of their own around the shared attack step. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886D678(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c);
void f_08861848(Cpu *c); void f_0886469C(Cpu *c);

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
static void open_face(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 7) = 1; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
    call1(c, o, f_08863114, back);
}

/* (fighter): standing (+696 cleared): without +193 step 0 opens, gains meter and moves 2 x strength + 1,
   later the attack step; with +193 step 0 at strength 1 with +290 bit 1 is step 2 with the meter by 1 and
   move 0x1E, else it opens, gains meter and moves 2 x strength + 6; step 1 the attack step; step 2 ticked,
   standing once ended, else moved by its frame push (+76, cleared). Crouching (+696 = 1): step 0 opens,
   gains meter and moves strength + 0xC (+3 with +193), later the attack step. */
static void stance_steps(Cpu *c, uint32_t d) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (S8(o + 192) == 0) {
        *B(o + 696) = 0;
        if (S8(o + 193) == 0) {
            if (st == 0) {
                open_face(c, o, 0x08993AACu + d);
                call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x08993AB8u + d);
                call2(c, o, (uint32_t)((S8(o + 194) << 1) + 1) & 0xFFFF, f_08861674, 0x08993AD0u + d);
            } else call1(c, o, f_0886D678, 0x08993AE0u + d);
        } else if (st == 0) {
            *B(o + 7) = 1; *B(o + 11) = *B(o + 697);
            int32_t s = S8(o + 194);
            *B(o + 235) = 1;
            if (s == 1 && (*H(o + 290) & 2)) {
                *B(o + 7) = 2;
                call2(c, o, 1, f_0886CA2C, 0x08993B2Cu + d);
                call2(c, o, 0x1E, f_08861674, 0x08993B38u + d);
            } else {
                call1(c, o, f_08863114, 0x08993B48u + d);
                call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x08993B54u + d);
                call2(c, o, (uint32_t)((S8(o + 194) << 1) + 6) & 0xFFFF, f_08861674, 0x08993B6Cu + d);
            }
        } else if (st == 1) call1(c, o, f_0886D678, 0x08993B84u + d);
        else if (st == 2) {
            call1(c, o, f_08861848, 0x08993B9Cu + d);
            if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x08993BB0u + d);
            else {
                int32_t d = S8(o + 76);
                if (d != 0) {
                    *H(o + 18) = (uint16_t)(S16(o + 18) + (*B(o + 11) != 0 ? d : -d));
                    *B(o + 76) = 0;
                }
            }
        }
    } else {
        *B(o + 696) = 1;
        if (st == 0) {
            open_face(c, o, 0x08993C10u + d);
            call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x08993C1Cu + d);
            uint32_t n = (uint32_t)S8(o + 194) & 0xFFFF;
            if (S8(o + 193) != 0) n = (n + 3) & 0xFFFF;
            call2(c, o, (n + 0xC) & 0xFFFF, f_08861674, 0x08993C4Cu + d);
        } else call1(c, o, f_0886D678, 0x08993C5Cu + d);
    }
    leave(c, sv);
}

SFA_FUNC(08993A60, attack_stance_steps) { stance_steps(c, 0); }
/* the same at 0x0899A660, its addresses 0x6C00 on */
SFA_FUNC(0899A660, attack_stance_steps_b) { stance_steps(c, 0x6C00); }


void f_08983604(Cpu *c); void f_089836E0(Cpu *c); void f_08983A38(Cpu *c); void f_08983AB4(Cpu *c);

/* (fighter): the attack by stance and button set: standing (+696 cleared) 0x08983604 (0x089836E0 with
   +193); crouching (+696 = 1) 0x08983A38 (0x08983AB4 with +193). */
SFA_FUNC(0898358C, attack_by_stance_c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int32_t h = S8(o + 193);
    ARG(1) = (uint32_t)h;
    if (S8(o + 192) == 0) {
        *B(o + 696) = 0;
        if (h != 0) { c->r[31] = 0x089835C4u; f_089836E0(c); } else { c->r[31] = 0x089835B0u; f_08983604(c); }
    } else {
        ARG(2) = 1; *B(o + 696) = 1;
        if (h != 0) { c->r[31] = 0x089835F8u; f_08983AB4(c); } else { c->r[31] = 0x089835E4u; f_08983A38(c); }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_08867C30(Cpu *c); void f_0887C408(Cpu *c); void f_0893202C(Cpu *c);

/* (fighter): with charges left (+825) and a punch held (+760 bits 4..6) or pressed (entry 6), one charge
   used: +820 = -1, step 4 and the cue 0x0887C408 (+337, the count's entry from 0x08A6DC14); then
   0x0893202C. */
SFA_FUNC(08931C78, charge_use) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr q = o + 0x334;
    c->r[17] = q; c->r[16] = o;
    if (S8(q + 5) != 0) {
        int go = (*H(o + 760) & 0x70) != 0;
        if (!go) { ARG(0) = o; ARG(1) = 6; c->r[31] = 0x08931CB0u; f_08867C30(c); go = c->r[2] != 0; }
        if (go) {
            int32_t n = S8(q + 5);
            *B(q) = 0xFF;
            *B(q + 5) = (uint8_t)(n - 1);
            *B(o + 7) = 4;
            int32_t v = S16(0x08A6DC14u + (uint32_t)(S8(q + 5) * 2));
            ARG(0) = o; ARG(1) = (uint32_t)S8(o + 337); ARG(2) = (uint32_t)v;
            c->r[31] = 0x08931CF8u; f_0887C408(c);
        }
    }
    ARG(0) = o; c->r[31] = 0x08931D00u; f_0893202C(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
