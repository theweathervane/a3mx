/* Two more fight frames: the usual one, and a wait with a message that runs one. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define F1 0x092E3EA0u
#define F2 0x092E4338u

void f_088368E8(Cpu *c); void f_08854C10(Cpu *c); void f_0882E28C(Cpu *c); void f_08858E80(Cpu *c);
void f_0886D874(Cpu *c); void f_0882D5D8(Cpu *c); void f_0882D998(Cpu *c); void f_0881DF10(Cpu *c);
void f_088377A8(Cpu *c); void f_08873D5C(Cpu *c); void f_08837690(Cpu *c); void f_0883771C(Cpu *c);
void f_08886E18(Cpu *c); void f_0888D64C(Cpu *c); void f_088379B8(Cpu *c); void f_08887AFC(Cpu *c);
void f_08870750(Cpu *c); void f_0880B6CC(Cpu *c); void f_0880A6CC(Cpu *c); void f_088BE574(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (a, b): a fight's frame, unless b's +15 holds it. While a's +145 is clear the fight itself
   runs: b's slow motion (+28 frames of it left, each lasting the count at +29, begun again from
   +30) says in +78 whether this frame is held; a's count at +59 goes up unless b's +52 is set;
   then the fight's routines, the fighters' frame among them. Of the drawing's routines only the
   first and last run while a's +145 is set. A 0x20 frame keeping s0, s1. */
SFA_FUNC(0882485C, fight_frame_a) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = a;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = b;
    if (S8(b + 15) == 0) {
        if (S8(a + 145) == 0) {
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
            call0(c, f_088368E8, 0x088248E8u);
            call0(c, f_08854C10, 0x088248F0u);
            call2(c, a, b, f_0882E28C, 0x088248FCu);
            call0(c, f_08858E80, 0x08824904u);
            call0(c, f_0886D874, 0x0882490Cu);
            call0(c, f_0882D5D8, 0x08824914u);
            call2(c, a, b, f_0882D998, 0x08824920u);
        }
        call0(c, f_0881DF10, 0x08824928u);
        if (S8(a + 145) == 0) {
            call0(c, f_088377A8, 0x0882493Cu);
            call0(c, f_08873D5C, 0x08824944u);
            call0(c, f_08837690, 0x0882494Cu);
            call0(c, f_0883771C, 0x08824954u);
            call0(c, f_08886E18, 0x0882495Cu);
            call0(c, f_0888D64C, 0x08824964u);
            call0(c, f_088379B8, 0x0882496Cu);
            call0(c, f_08887AFC, 0x08824974u);
            call0(c, f_08870750, 0x0882497Cu);
            call0(c, f_0888D64C, 0x08824984u);
        }
        call0(c, f_0880B6CC, 0x0882498Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): a wait of a's +16 frames: while it lasts both first fighters' +112 and +114 are held
   at 0x90, the message at 0x089FFDA0 is drawn and the fight's frame runs; when it is over a's
   step (+4) goes on, both fighters' +1 are cleared and b's +14 set. A 0x30 frame keeping s0,
   s1. */
SFA_FUNC(088BE018, fight_wait_with_message) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s1;
    c->r[17] = a;
    *W(sp + 32) = s0;
    c->r[16] = b;
    int32_t left = S16(a + 16) - 1;
    *H(a + 16) = (uint16_t)left;
    *W(sp + 40) = ra;
    if ((int16_t)left < 0) {
        c->r[7] = 1;
        *H(a + 4) = (uint16_t)(*H(a + 4) + 1);
        *B(F2 + 1) = 0;
        *B(F1 + 1) = 0;
        *B(b + 14) = 1;
    } else {
        *H(F2 + 114) = 0x90;
        *H(F1 + 114) = 0x90;
        *H(F2 + 112) = 0x90;
        *H(F1 + 112) = 0x90;
        ARG(0) = 0x089FFDA0u; ARG(1) = 1; ARG(2) = 0x68; ARG(3) = 0xC8; c->r[8] = 4;
        c->r[31] = 0x088BE088u; f_0880A6CC(c);
        call2(c, a, b, f_088BE574, 0x088BE094u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}
