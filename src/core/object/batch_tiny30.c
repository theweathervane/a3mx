/* Small functions the scenarios reach, 32 instructions: effects made from or copying their owner,
   fighter objects' starts. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08822D2C(Cpu *c); void f_0893B604(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c);
void f_0886DBE8(Cpu *c); void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void on_7(gaddr o) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
    *B(o + 11) = (uint8_t)S8(o + 697);
}

/* (owner): an effect (0x08822D2C (owner, 0x18, +82)) copying the owner (0x0893B604) with its +81 and
   +194 (as +346), +255 = 5, +347 = 1; the owner's +82 on. */
SFA_FUNC(08936DD8, effect_make_copy) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 24) = s2;
    c->r[18] = o + 0x52;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    ARG(0) = o; ARG(1) = 0x18; ARG(2) = *B(o + 0x52);
    c->r[31] = 0x08936E00u; f_08822D2C(c);
    gaddr n = c->r[2];
    c->r[17] = n;
    ARG(0) = n;
    if (n != 0) {
        call2(c, n, o, 0x08936E14u, f_0893B604);
        *B(n + 81) = (uint8_t)S8(o + 81);
        *B(n + 346) = (uint8_t)S8(o + 194);
        *B(n + 255) = 5;
        *B(n + 347) = 1;
        *B(o + 0x52) = (uint8_t)(S8(o + 0x52) + 1);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): the effect takes the owner's +194 (as +346), +121 / +122 / +80 / +11 - +14 / +24 /
   +38 / x / y / +164 and side (+337, as +86), and points back at it (+88). */
SFA_FUNC(089426E4, effect_take_owner_3) {
    gaddr o = ARG(0), p = ARG(1);
    *B(o + 346) = (uint8_t)S8(p + 194);
    *B(o + 122) = (uint8_t)S8(p + 122);
    *B(o + 121) = (uint8_t)S8(p + 121);
    *B(o + 80) = (uint8_t)S8(p + 80);
    *H(o + 24) = *H(p + 24);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *W(o + 88) = p;
    *H(o + 22) = (uint16_t)S16(p + 22);
    uint32_t side = (uint32_t)(int32_t)S8(p + 337);
    *H(o + 86) = (uint16_t)side;
    *W(o + 164) = *W(p + 164);
    ARG(2) = side; ARG(1) = *W(p + 164);
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7; +5 / +7 cleared, +11 = +697; `k236` at
   +236, +235 = 1 or +696 cleared as `kind`) and 0x0886DB94; 1 then, else 0. */
static void start_db94(Cpu *c, uint32_t k236, int kind, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o);
        *B(o + 236) = (uint8_t)k236;
        c->r[17] = 1;
        if (kind == 0) *B(o + 235) = 1; else *B(o + 696) = 0;
        call_on(c, o, back2, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08962EA8, object_start_0_db94c) { start_db94(c, 0, 0, 0x08962EC0u, 0x08962EF8u); }
SFA_FUNC(089735A0, object_start_1_696)   { start_db94(c, 1, 1, 0x089735B8u, 0x089735F0u); }

/* (object): with +600 set, +251 clear and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7; +5 / +7 / +235
   / +236 cleared, +11 = +697); 1 then, else 0. */
SFA_FUNC(0898EAC4, object_start_600) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (*W(o + 600) != 0 && S8(o + 251) == 0) {
        c->r[31] = 0x0898EAF0u; f_0886BF84(c);
        if (c->r[2] != 0) {
            on_7(o);
            *B(o + 236) = 0; *B(o + 235) = 0;
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 set and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 5; +11 =
   +697) and 0x0886DBE8, whose answer it gives; else 0. */
SFA_FUNC(0899F0F8, object_start_386_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int done = 0;
    if (S8(o + 386) != 0) {
        c->r[31] = 0x0899F118u; f_0886BF84(c);
        if (c->r[2] != 0) {
            on_7(o);
            *B(o + 236) = 5; *B(o + 235) = 1;
            call_on(c, o, 0x0899F154u, f_0886DBE8);
            done = 1;
        }
    }
    if (!done) c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): a list-0x43 type-1 effect (0x08822EAC) above the owner (y + 0x58, 0x10 lower for level 1 of
   +194; +73 = level - 1), facing its way, pointing back at it (+88). */
SFA_FUNC(089A25A0, effect_make_43) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x43, 1, 0x089A25BCu, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 11) = *B(o + 11);
        *H(n + 18) = (uint16_t)S16(o + 18);
        *W(n + 88) = o;
        *H(n + 22) = (uint16_t)(S16(o + 22) + 0x58);
        int8_t k = (int8_t)(S8(o + 194) - 1);
        *B(n + 73) = (uint8_t)k;
        if (k == 0) *H(n + 22) = (uint16_t)(S16(n + 22) - 0x10);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
