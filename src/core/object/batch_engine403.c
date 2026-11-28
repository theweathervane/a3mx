/* The fighters' frame, and a fight's frame that runs it. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME     0x08E75910u
#define BATTLE   0x092E1228u
#define F1       0x092E3EA0u
#define F2       0x092E4338u
#define F3       0x092E47D0u
#define F4       0x092E4C68u
#define FIGHTERS 0x08A17F88u /* the four fighters' addresses */

void f_08858D54(Cpu *c); void f_08864DFC(Cpu *c); void f_0884A4B4(Cpu *c); void f_08859418(Cpu *c);
void f_088592E4(Cpu *c); void f_088590E4(Cpu *c); void f_08859750(Cpu *c); void f_088591F4(Cpu *c);
void f_0885911C(Cpu *c); void f_0885971C(Cpu *c); void f_0886B540(Cpu *c); void f_088368E8(Cpu *c);
void f_08854C10(Cpu *c); void f_0882E28C(Cpu *c); void f_0886D874(Cpu *c); void f_0882D5D8(Cpu *c);
void f_0882D998(Cpu *c); void f_0881DF10(Cpu *c); void f_088377A8(Cpu *c); void f_08873D5C(Cpu *c);
void f_08837690(Cpu *c); void f_0883771C(Cpu *c); void f_08886E18(Cpu *c); void f_088379B8(Cpu *c);
void f_08887AFC(Cpu *c); void f_08870750(Cpu *c); void f_0880B6CC(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* The fighters' frame, unless the game's +39 or the battle's +78 holds it: each fighter in use
   has its first routine run; each one's +73 is copied to its +712; the buttons are read (in the
   practice mode, game mode 2, for the player's fighter only, the other being run by the
   practice mode's frame); three routines for all of them; then the fighters take their turns in
   the order the list at 0x09358410 gives, skipping one that is not in use or has bit 7 of +135,
   and, while any fighter's +237 is set, all but those with it set: a fighter's frame, once more
   when its +135 is 1 (which becomes 2), and its partner's (+92) when +135 is set. Four routines
   for all of them close the frame. A 0x40 frame keeping s0..s8. */
SFA_FUNC(08858E80, fighters_frame) {
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = GAME;
    for (int i = 1; i < 8; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 48) = sv[8]; *W(sp + 52) = sv[9];
    if (S8(GAME + 39) == 0 && S8(BATTLE + 78) == 0) {
        c->r[17] = FIGHTERS;
        c->r[23] = 1;
        c->r[19] = F1; c->r[20] = F2; c->r[21] = F3; c->r[22] = F4;
        c->r[18] = FIGHTERS;
        for (c->r[30] = 0; (int32_t)c->r[30] < 4; c->r[30]++, c->r[18] += 4) {
            ARG(0) = *W(c->r[18]);
            ARG(1) = *B(ARG(0));
            if (ARG(1) != 0) call0(c, f_08858D54, 0x08858F18u);
        }
        c->r[30] = F1;
        *B(F1 + 712) = *B(F1 + 73);
        c->r[18] = F2;
        *B(F2 + 712) = *B(F2 + 73);
        *B(F3 + 712) = *B(F3 + 73);
        *B(F4 + 712) = *B(F4 + 73);
        if (S8(GAME + 114) == 2) {
            if (S8(GAME + 57) == 1) {
                call1(c, F1, f_08864DFC, 0x08858F74u);
                call2(c, F2, F1, f_0884A4B4, 0x08858F80u);
            } else {
                call1(c, F2, f_08864DFC, 0x08858F90u);
                call2(c, F1, F2, f_0884A4B4, 0x08858F9Cu);
            }
        } else {
            c->r[16] = FIGHTERS;
            for (c->r[18] = 0; (int32_t)c->r[18] < 4; c->r[18]++, c->r[16] += 4) {
                ARG(0) = *W(c->r[16]);
                ARG(1) = *B(ARG(0));
                if (ARG(1) != 0) call0(c, f_08864DFC, 0x08858FC0u);
            }
        }
        call0(c, f_08859418, 0x08858FD8u);
        call0(c, f_088592E4, 0x08858FE0u);
        call0(c, f_088590E4, 0x08858FE8u);
        c->r[30] = 0x09360000u;
        for (c->r[16] = 0; (int32_t)c->r[16] < 4; c->r[16]++) {
            gaddr f = *W(FIGHTERS + (uint32_t)*B(*W(0x09358410u) + c->r[16]) * 4);
            c->r[18] = f;
            if ((S8(f + 135) & 0x80) != 0 || *B(f) == 0) continue;
            if ((S8(F3 + 237) | S8(F4 + 237) | S8(F1 + 237) | S8(F2 + 237)) != 0 && S8(f + 237) == 0) continue;
            call1(c, f, f_08859750, 0x0885905Cu);
            int32_t again = S8(f + 135);
            if (again == 0) continue;
            if (again == 1) {
                *B(f + 135) = 2;
                call1(c, f, f_08859750, 0x0885907Cu);
            }
            call1(c, *W(f + 92), f_08859750, 0x08859084u);
        }
        call0(c, f_088591F4, 0x0885909Cu);
        call0(c, f_0885911C, 0x088590A4u);
        call0(c, f_0885971C, 0x088590ACu);
        call0(c, f_0886B540, 0x088590B4u);
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9];
    c->r[29] += 0x40;
}

/* (a, b): a fight's frame, unless b's +15 holds it. While a's +89 is clear the fight itself
   runs: b's slow motion (+28 frames of it left, each lasting the count at +29, begun again from
   +30) says in +78 whether this frame is held; a's count at +59 goes up unless b's +52 is set;
   then the fight's routines, the fighters' frame among them. The drawing's routines run either
   way. A 0x20 frame keeping s0, s1. */
SFA_FUNC(088BE574, fight_frame_b) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = a;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = b;
    if (S8(b + 15) == 0) {
        if (S8(a + 89) == 0) {
            uint32_t held = 0;
            int32_t slow = S8(b + 28);
            if (slow != 0) {
                int32_t left = S8(b + 29) - 1;
                *B(b + 29) = (uint8_t)left;
                if ((int8_t)left != 0) held = 1;
                else {
                    *B(b + 28) = (uint8_t)(slow - 1);
                    *B(b + 29) = *B(b + 30);
                }
            }
            *B(b + 78) = (uint8_t)held;
            if (S8(b + 52) == 0) *B(a + 59) = (uint8_t)(S8(a + 59) + 1);
            call0(c, f_088368E8, 0x088BE600u);
            call0(c, f_08854C10, 0x088BE608u);
            call2(c, a, b, f_0882E28C, 0x088BE614u);
            call0(c, f_08858E80, 0x088BE61Cu);
            call0(c, f_0886D874, 0x088BE624u);
            call0(c, f_0882D5D8, 0x088BE62Cu);
            call2(c, a, b, f_0882D998, 0x088BE638u);
        }
        call0(c, f_0881DF10, 0x088BE640u);
        call0(c, f_088377A8, 0x088BE648u);
        call0(c, f_08873D5C, 0x088BE650u);
        call0(c, f_08837690, 0x088BE658u);
        call0(c, f_0883771C, 0x088BE660u);
        call0(c, f_08886E18, 0x088BE668u);
        call0(c, f_088379B8, 0x088BE670u);
        call0(c, f_08887AFC, 0x088BE678u);
        call0(c, f_08870750, 0x088BE680u);
        call0(c, f_0880B6CC, 0x088BE688u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
