/* Special jumps that end in the common lift (0x0895D3E0) once +123 is set: a timer at +83 or a fall
   starts the next leg. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0895D3E0(Cpu *c); void f_089613E8(Cpu *c); void f_08961448(Cpu *c); void f_08861848(Cpu *c);
void f_088616EC(Cpu *c);

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
/* +83 counted down; 1 when it reaches 0 */
static int tick83(gaddr o) {
    int8_t t = (int8_t)(S8(o + 0x52 + 1) - 1);
    *B(o + 0x52 + 1) = (uint8_t)t;
    return t == 0;
}

/* (fighter): +123 hands over to the lift; else 0x089613E8 and once +83 runs out +7 on and a rise by
   strength from 0x08A85A20 (brake -0.375); ticked. */
SFA_FUNC(0895D17C, jump_rise_5a20) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[5] = (uint32_t)S8(o + 123);
    if (c->r[5] != 0) call1(c, o, f_0895D3E0, 0x0895D19Cu);
    else {
        call1(c, o, f_089613E8, 0x0895D1ACu);
        if (tick83(o)) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *W(o + 100) = *W(0x08A85A20u + ((uint32_t)S8(o + 194) << 2));
            *W(o + 108) = 0xFFFFA000u;
        }
        call1(c, o, f_08861848, 0x0895D204u);
    }
    leave(c, sv);
}

/* (fighter): +123 hands over to the lift; else 0x08961448 (a negative result stops it sideways) and once
   falling +7 on, x 0.125 and move strength + 6; ticked. */
SFA_FUNC(0895D214, jump_fall_6) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[5] = (uint32_t)S8(o + 123);
    if (c->r[5] != 0) call1(c, o, f_0895D3E0, 0x0895D234u);
    else {
        call1(c, o, f_08961448, 0x0895D244u);
        if ((int32_t)c->r[2] < 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
        if ((int32_t)*W(o + 100) < 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *W(o + 96) = 0x8000; *W(o + 104) = 0;
            call2(c, o, (uint32_t)(S8(o + 194) + 6) & 0xFFFF, f_088616EC, 0x0895D28Cu);
        }
        call1(c, o, f_08861848, 0x0895D294u);
    }
    leave(c, sv);
}

/* (fighter): 0x08961448; +77 with +123 hands over to the lift; else +123 cleared and once +83 runs out
   +7 on and a leap (x by strength from 0x08A85BA8, brake -0.1875, y 8.0, brake -0.375); ticked. */
SFA_FUNC(0895E9C0, jump_leap_5ba8) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08961448, 0x0895E9D4u);
    if (S8(o + 77) != 0 && S8(o + 123) != 0) call1(c, o, f_0895D3E0, 0x0895E9F4u);
    else {
        *B(o + 123) = 0;
        if (tick83(o)) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *W(o + 96) = *W(0x08A85BA8u + ((uint32_t)S8(o + 194) << 2));
            *W(o + 104) = 0xFFFFD000u; *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFFA000u;
        }
        call1(c, o, f_08861848, 0x0895EA64u);
    }
    leave(c, sv);
}

/* (fighter): +77 with +123 hands over to the lift; else +123 cleared (moving back stops it sideways) and
   once falling +7 on, +77 cleared, x 0.125 and move strength + 6; 0x08961448 and ticked. */
SFA_FUNC(0895EA74, jump_fall_6b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[5] = (uint32_t)S8(o + 77);
    if (c->r[5] != 0 && S8(o + 123) != 0) call1(c, o, f_0895D3E0, 0x0895EAA0u);
    else {
        int32_t v = (int32_t)*W(o + 96);
        *B(o + 123) = 0;
        if (v < 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
        if ((int32_t)*W(o + 100) < 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 77) = 0;
            *W(o + 96) = 0x8000; *W(o + 104) = 0;
            call2(c, o, (uint32_t)(S8(o + 194) + 6) & 0xFFFF, f_088616EC, 0x0895EAF8u);
        }
        call1(c, o, f_08961448, 0x0895EB00u);
        call1(c, o, f_08861848, 0x0895EB08u);
    }
    leave(c, sv);
}


void f_08935F04(Cpu *c); void f_08936470(Cpu *c); void f_08938D40(Cpu *c); void f_089376A8(Cpu *c);
void f_08935E6C(Cpu *c); void f_0887C614(Cpu *c);

/* (fighter): the part's stick read into +78 until the part's +32 is set; at +77 = 1 +7 on, +77 cleared,
   +73 = 1 and a jump straight up by strength from 0x08A707C8 (rise, brake 12 further on); ticked. A frame
   keeping s0, s1. */
SFA_FUNC(08935B08, part_jump_07c8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 0x3A0 + 32) == 0) call1(c, o, f_08935F04, 0x08935B34u);
    if (S8(o + 77) == 1) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 77) = 0; *B(o + 73) = 1;
        *W(o + 104) = 0; *W(o + 96) = 0;
        gaddr t = 0x08A707C8u + ((uint32_t)S8(o + 194) << 2);
        *W(o + 100) = *W(t); *W(o + 108) = *W(t + 12);
    }
    call1(c, o, f_08861848, 0x08935B88u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the part's stick until its +32 is set, 0x089376A8; landed 0x08935E6C; within 0x18 above
   the ground with the part's +32 +7 on and move strength + 0xF; otherwise ticked. A frame keeping s0,
   s1 (the part). */
SFA_FUNC(08935B9C, part_land_e6c) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr q = o + 0x3A0; c->r[17] = q;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(q + 32) == 0) call1(c, o, f_08935F04, 0x08935BC4u);
    call1(c, o, f_089376A8, 0x08935BCCu);
    int32_t a = S16(o + 132), b = S16(o + 22);
    if (!(a < b)) call1(c, o, f_08935E6C, 0x08935BE8u);
    else if ((int32_t)((uint32_t)(a + 0x18) & 0xFFFF) < (int32_t)((uint32_t)b & 0xFFFF) && S8(q + 32) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, (uint32_t)(S8(o + 194) + 0xF) & 0xFFFF, f_088616EC, 0x08935C48u);
    } else call1(c, o, f_08861848, 0x08935C24u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the part's stick into the strength until its +32 is set; once its animation ends +7 on, a
   leap (x 6.0 along the facing, y 9.0, brake -0.5) and move 0x2A, else ticked. */
SFA_FUNC(089361C8, part_leap_2a) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 0x3A0 + 32) == 0) call1(c, o, f_08936470, 0x089361ECu);
    if (S8(o + 75) < 0) {
        *W(o + 104) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 100) = 0x90000; *W(o + 108) = 0xFFFF8000u;
        *W(o + 96) = *B(o + 11) != 0 ? 0x60000u : 0xFFFA0000u;
        call2(c, o, 0x2A, f_088616EC, 0x08936240u);
    } else call1(c, o, f_08861848, 0x08936250u);
    leave(c, sv);
}

/* (fighter): without +77 the part's stick until its +32 is set; 0x089376A8; landed +7 on, set down (+22
   = +132, +73 / +20 / +123 cleared), 0x0887C614 and move 0x21; else ticked. */
SFA_FUNC(08938860, part_land_21) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 77) == 0 && S8(o + 0x3A0 + 32) == 0) call1(c, o, f_08938D40, 0x08938890u);
    call1(c, o, f_089376A8, 0x08938898u);
    if (S16(o + 22) < S16(o + 132)) {
        *H(o + 22) = *H(o + 132);
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 73) = 0; *H(o + 20) = 0; *B(o + 123) = 0;
        call1(c, o, f_0887C614, 0x089388D0u);
        call2(c, o, 0x21, f_088616EC, 0x089388DCu);
    } else call1(c, o, f_08861848, 0x089388ECu);
    leave(c, sv);
}
