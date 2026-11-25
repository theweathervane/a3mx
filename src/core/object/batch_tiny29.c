/* Small functions the scenarios reach, 32 instructions: fighter objects' starts and trail effects made
   every few frames. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08822EAC(Cpu *c); void f_08920EB4(Cpu *c);
void f_089238A0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void on_7(gaddr o) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
    *B(o + 11) = (uint8_t)S8(o + 697);
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 1; +5 / +7 / +196 - +199
   cleared, +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(08918DA0, object_start_1_196) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = 0x08918DBCu; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 236) = 1; *B(o + 235) = 1;
        on_7(o);
        *B(o + 199) = 0; *B(o + 198) = 0; *B(o + 197) = 0; *B(o + 196) = 0;
        c->r[17] = 1;
        call_on(c, o, 0x08918E08u, f_0886DB94);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): on frames where GAME +59's low bits (`mask`) are 0, a list-0x28 trail effect of `type` at
   the owner (+88 = it; its +11 / +14 / x / y / +86 / +164). */
static void trail(Cpu *c, uint32_t mask, uint32_t type, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if ((S8(GAME + 59) & (int32_t)mask) == 0) {
        call2(c, 0x28, type, back, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *W(n + 88) = o;
            *B(n + 14) = *B(o + 14);
            *B(n + 11) = *B(o + 11);
            *H(n + 18) = (uint16_t)S16(o + 18);
            *H(n + 22) = (uint16_t)S16(o + 22);
            *H(n + 86) = *H(o + 86);
            *W(n + 164) = *W(o + 164);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0891DD70, trail_0_every4) { trail(c, 3, 0, 0x0891DDA0u); }
SFA_FUNC(0891DDF0, trail_1_every8) { trail(c, 7, 1, 0x0891DE20u); }
SFA_FUNC(0891DE70, trail_2_every4) { trail(c, 3, 2, 0x0891DEA0u); }

/* (object, n): with +386 not negative and 0x0886BF84 allowing it, +236 = 3, +0x3A6 = n, 0x089238A0
   and its start (0x08920EB4); 1 then, else 0. */
SFA_FUNC(08920B34, object_start_3a6) {
    gaddr o = ARG(0);
    uint32_t n = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = n;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) >= 0) {
        c->r[31] = 0x08920B60u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 236) = 3;
            *B(o + 0x3A0 + 6) = (uint8_t)n;
            call_on(c, o, 0x08920B7Cu, f_089238A0);
            call_on(c, o, 0x08920B84u, f_08920EB4);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1; +5 / +7 / +236 cleared, +11 =
   +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0892AD04, object_start_0_db94b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[31] = 0x0892AD1Cu; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o);
        *B(o + 236) = 0;
        c->r[17] = 1;
        *B(o + 235) = 1;
        call_on(c, o, 0x0892AD54u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 0xD; +5 / +7 / +196 - +199
   cleared, +11 = +697) answering 0; 1 when it is not allowed. */
SFA_FUNC(08935064, object_start_d_inv) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08935078u; f_0886BF84(c);
    if (c->r[2] == 0) c->r[2] = 1;
    else {
        *B(o + 236) = 0xD;
        on_7(o);
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
        *B(o + 235) = 1;
        c->r[2] = 0;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
