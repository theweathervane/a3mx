/* Small functions the scenarios reach, 35 instructions: fighter objects' starts, an effect copying its
   owner, effects' steps and landing. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_088FDA68(Cpu *c);
void f_08822EAC(Cpu *c); void f_0886C6EC(Cpu *c); void f_088215C4(Cpu *c); void f_088219B0(Cpu *c);
void f_08981AE4(Cpu *c); void f_089879C4(Cpu *c); void f_0898C5DC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

#define S01_BEGIN() \
    gaddr o = ARG(0); \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; \
    *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; \
    c->r[16] = o
#define S01_END() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)

/* (object): when 0x0886BF84 allows it, 0x088FDA68, on (+4 = 1, +6 = 7, +235 = 1; +5 / +7 / +236
   cleared, +11 = +697) and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(088FD448, object_start_da68) {
    S01_BEGIN();
    c->r[31] = 0x088FD460u; f_0886BF84(c);
    if (c->r[2] != 0) {
        call_on(c, o, 0x088FD470u, f_088FDA68);
        *B(o + 4) = 1; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 5) = 0; *B(o + 236) = 0;
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 235) = 1;
        call_on(c, o, 0x088FD4A4u, f_0886DBE8);
        c->r[2] = 1;
    } else c->r[2] = 0;
    S01_END();
}

/* (owner): a list-0x29 type-8 effect (0x08822EAC) copying the owner's place and looks (+11 - +14, +18 /
   +22, +24, +38, +86, +164) and pointing back at it (+88). */
SFA_FUNC(0891EE94, effect_make_29_copy) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x29, 8, 0x0891EEB0u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 14) = *B(o + 14);
        *B(n + 12) = (uint8_t)S8(o + 12);
        *B(n + 13) = *B(o + 13);
        *H(n + 38) = (uint16_t)S16(o + 38);
        *W(n + 88) = o;
        *H(n + 24) = *H(o + 24);
        *H(n + 18) = (uint16_t)S16(o + 18);
        *H(n + 22) = (uint16_t)S16(o + 22);
        *B(n + 11) = *B(o + 11);
        *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 1; +5 / +7 / +193
   cleared, +11 / +78 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0893D9C4, object_start_1_78) {
    S01_BEGIN();
    c->r[31] = 0x0893D9DCu; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7;
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[17] = 1;
        *B(o + 7) = 0; *B(o + 236) = 1;
        *B(o + 193) = 0;
        *B(o + 78) = *B(o + 11);
        *B(o + 235) = 1;
        call_on(c, o, 0x0893DA20u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    S01_END();
}

/* (object): with +386 clear, a full gauge (+366 from 144) and 0x0886C6EC allowing it, on (+4 = 1, +6 =
   8, +235 = 1, +236 = 0xA, +194 / +552 = 2; +5 / +7 cleared); 1 then, else 0. */
SFA_FUNC(0895CB48, object_start_full_gauge) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) == 0 && !(S16(o + 366) < 144)) {
        c->r[31] = 0x0895CB78u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 0xA; *B(o + 235) = 1;
            *B(o + 552) = 2; *B(o + 194) = 2;
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x08981AE4; under its fighter's (+92) floor (+132) it lands there (step 3); 0x088219B0, and
   on alternate frames (GAME +59 bit 0) once more and drawn. */
SFA_FUNC(08981250, effect_land_on_floor) {
    S01_BEGIN();
    gaddr p = *W(o + 92);
    c->r[17] = p;
    c->r[31] = 0x0898126Cu; f_08981AE4(c);
    if (S16(o + 22) < S16(p + 132)) {
        *W(o + 20) = (uint32_t)(int32_t)S16(p + 132) << 16;
        *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
    }
    call_on(c, o, 0x089812A0u, f_088219B0);
    if (S8(GAME + 59) & 1) {
        call_on(c, o, 0x089812C0u, f_088219B0);
        call_on(c, o, 0x089812C8u, f_088215C4);
    }
    S01_END();
}

/* (effect): its step by +5 (0x08A928FC) while its kind (+127) is its fighter's (+92) character;
   otherwise BATTLE +58 one less and +4 on. */
SFA_FUNC(089812DC, effect_step_28fc) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 127) == S8(*W(o + 92) + 338)) {
        gaddr h = *W(0x08A928FCu + ((uint32_t)*B(o + 5) << 2));
        ARG(1) = h;
        c->r[31] = 0x08981318u; rt_call_indirect(c, h);
    } else {
        *B(BATTLE + 58) = (uint8_t)(S8(BATTLE + 58) - 1);
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x088219B0 while its kind (+3) is its fighter's (+92) character, the fight is not over
   (BATTLE +7) and its +0x60 object is clear; else 0x089879C4. */
SFA_FUNC(08987938, effect_while_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if ((uint32_t)(int32_t)S8(*W(o + 92) + 338) != *B(o + 3)) { c->r[31] = 0x0898795Cu; f_089879C4(c); }
    else if (S8(BATTLE + 7) != 0) { c->r[31] = 0x08987980u; f_089879C4(c); }
    else if (S8(*W(o + 0x60)) != 0) { c->r[31] = 0x089879A4u; f_089879C4(c); }
    else { c->r[31] = 0x089879B8u; f_088219B0(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+836 = 1, +4 = 1, +6 = 7, +235 = 1, +236 = 1; +5 / +7
   cleared), 0x0898C5DC, +11 = +697 and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0898A4DC, object_start_c5dc) {
    S01_BEGIN();
    c->r[31] = 0x0898A4F4u; f_0886BF84(c);
    c->r[17] = 1;
    if (c->r[2] != 0) {
        *W(o + 836) = 1;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 1;
        call_on(c, o, 0x0898A524u, f_0898C5DC);
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x0898A538u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    S01_END();
}
