/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 41 - 61
   instructions: the select screen's steps after a pick (on to the next pick, back to the start) and
   the select screen's common lists loaded. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define PICS  0x08E75AD8u
#define TILES 0x092D9D40u
#define PALS  0x092D9E00u
#define P1    0x092E3EA0u
#define P2    0x092E4338u

void f_08808424(Cpu *c); void f_08814384(Cpu *c); void f_0881CE4C(Cpu *c); void f_08821F58(Cpu *c);
void f_0888D5B0(Cpu *c); void f_08892018(Cpu *c); void f_08813F44(Cpu *c); void f_088D7EA0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (screen, battle): after a pick: +4 on, +43 = 1, +72 = -1, the battle object's +37 = 1, 0x08821F58,
   with its +105 set 0x0888D5B0 and 0x0881CE4C (+102, 0); the next portrait's buffers (0x08814384),
   archive 4 queued, the screen's timers and the battle object's +15 / +81 cleared, 0x08892018. */
SFA_FUNC(0882B260, select_after_pick) {
    gaddr o = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[17] = o; c->r[16] = b;
    *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
    *B(o + 43) = 1;
    *B(o + 72) = 0xFF;
    *B(b + 37) = 1;
    ARG(0) = 0xFFFFFFFFu; ARG(1) = 1;
    c->r[31] = 0x0882B29Cu; f_08821F58(c);
    int32_t k = S8(b + 105);
    if (k != 0) {
        call_on(c, (uint32_t)k, 0x0882B2B0u, f_0888D5B0);
        ARG(0) = (uint32_t)(int32_t)S16(b + 102); ARG(1) = 0;
        c->r[31] = 0x0882B2BCu; f_0881CE4C(c);
    }
    c->r[31] = 0x0882B2C4u; f_08814384(c);
    call_on(c, 4, 0x0882B2CCu, f_08808424);
    *H(o + 22) = 0; *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 10) = 0; *H(o + 8) = 0; *H(o + 6) = 0;
    *B(b + 81) = 0;
    *B(b + 15) = 0;
    c->r[31] = 0x0882B2F0u; f_08892018(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The select screen's common lists, under the loader's lock: lists 5, 6, 16 and 28's pictures, tiles
   and palettes put back from the copies (0x08A0562C / D4 / 0x08A0577C), archives 0, 3 and 0x1C queued
   (0x08808424) and the fight's common pictures (0x08813F44). */
SFA_FUNC(08813E70, select_common_load) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    call_on(c, 1, 0x08813E80u, f_088D7EA0);
    static const uint32_t offs[4] = { 20, 24, 64, 112 };
    for (int n = 0; n < 4; n++) {
        *W(PICS + offs[n]) = *W(0x08A0562Cu + offs[n]);
        *W(TILES + offs[n]) = *W(0x08A056D4u + offs[n]);
        *W(PALS + offs[n]) = *W(0x08A0577Cu + offs[n]);
    }
    call_on(c, 0, 0x08813F18u, f_08808424);
    call_on(c, 3, 0x08813F20u, f_08808424);
    call_on(c, 0x1C, 0x08813F28u, f_08808424);
    c->r[31] = 0x08813F30u; f_08813F44(c);
    call_on(c, 0, 0x08813F38u, f_088D7EA0);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen, battle): back to the select screen's start: with +137 clear on to step 4 (timers cleared,
   the battle object's +15 = 1 and +55 cleared, both fighters marked CPU (+373), the next portrait's
   buffers); else step 3 (+2 = 3, all timers cleared, +43 = 1, +72 = -1, the battle object's +56 / +81
   cleared, +15 = -1, +37 = 1). Either way the four fighters' +1 cleared. */
SFA_FUNC(0882781C, select_back_to_start) {
    gaddr o = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = s1; *W(c->r[29] + 8) = ra;
    c->r[17] = P1; c->r[16] = P2;
    if (S8(o + 137) == 0) {
        *H(o + 4) = 4;
        *H(o + 22) = 0; *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 10) = 0; *H(o + 8) = 0; *H(o + 6) = 0;
        *B(b + 15) = 1;
        *B(b + 55) = 0;
        *B(P1 + 373) = 1; *B(P2 + 373) = 1;
        ARG(0) = b; ARG(1) = o; ARG(2) = 1; ARG(3) = 4;
        c->r[31] = 0x08827888u; f_08814384(c);
    } else {
        *H(o + 2) = 3;
        *H(b + 56) = 0;
        *B(b + 81) = 0;
        *H(o + 22) = 0; *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 16) = 0;
        *H(o + 10) = 0; *H(o + 8) = 0; *H(o + 6) = 0; *H(o + 4) = 0;
        *B(o + 43) = 1;
        *B(o + 72) = 0xFF;
        *B(b + 15) = 0xFF;
        *B(b + 37) = 1;
    }
    *B(0x092E4C68u + 1) = 0; *B(0x092E47D0u + 1) = 0; *B(P2 + 1) = 0; *B(P1 + 1) = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x10;
}
