/* The link play's game: the start of its screen before a fight. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define NETP 0x09523980u

void f_08821F9C(Cpu *c); void f_08813770(Cpu *c); void f_08814ED8(Cpu *c); void f_0887C6E4(Cpu *c);
void f_089E11C8(Cpu *c); void f_088969BC(Cpu *c); void f_08823338(Cpu *c); void f_088D5E7C(Cpu *c);
void f_08822EAC(Cpu *c); void f_088157A0(Cpu *c); void f_088E3604(Cpu *c); void f_0882235C(Cpu *c);
void f_08823304(Cpu *c); void f_08822934(Cpu *c); void f_0880AA74(Cpu *c); void f_088E336C(Cpu *c);
void f_0882DD5C(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (a, b): the screen that shows the two sides before a link fight is made: the screens before
   are put away, the music 0x132 started, a block of 0x200 bytes copied to 0x092DF980, a voice
   asked for, and its objects made: six of kind 6, then, when the routine at 0x088D5E7C says
   so, a wait of 0x1E0 frames (a's +20, with +22 at 2), else two of kind 0x10. Then for the two
   sides' fighters (b's +112 and +116): one object of kind 0x98 by the first's +337 (or the
   fifth kind when neither has +426 set), noting which at +41 of the link's record, and for each
   fighter three objects numbered 0 to 2 and one more, each pointing at the fighter. A 0x30
   frame keeping s0..s4. */
SFA_FUNC(088E97A4, link_versus_screen_start) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[16] = b;
    c->r[18] = a;
    *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = ra;
    c->r[17] = NETP;
    call0(c, f_08821F9C, 0x088E97D4u);
    call0(c, f_08813770, 0x088E97DCu);
    call0(c, f_08814ED8, 0x088E97E4u);
    ARG(0) = 0x132; call0(c, f_0887C6E4, 0x088E97ECu);
    *H(a + 6) = 0;
    *H(b + 102) = 0;
    ARG(2) = 0x200;
    call2(c, 0x092DF980u, *W(0x092D9E20u), f_089E11C8, 0x088E9810u);
    ARG(2) = 4;
    call2(c, 0x68D, 0, f_088969BC, 0x088E9820u);
    static const uint8_t kinds[6] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x19 };
    for (uint32_t i = 0; i < 6; i++) call2(c, 6, kinds[i], f_08823338, 0x088E982Cu + 12 * i);
    call0(c, f_088D5E7C, 0x088E9870u);
    if (c->r[2] != 0) {
        ARG(0) = 0x1E0; ARG(1) = 2;
        *H(a + 20) = 0x1E0;
        *H(a + 22) = 2;
    } else {
        call2(c, 0x10, 0, f_08823338, 0x088E989Cu);
        call2(c, 0x10, 1, f_08823338, 0x088E98A8u);
    }
    gaddr first = *W(b + 112), second = *W(b + 116);
    c->r[19] = first;
    c->r[18] = second;
    if (S16(first + 426) == 0 && S16(second + 426) == 0) {
        call2(c, 0x98, 5, f_08822EAC, 0x088E98D0u);
        ARG(0) = 3;
        *B(NETP + 41) = 3;
    } else {
        call2(c, 0x98, (uint32_t)S8(first + 337) + 3, f_08822EAC, 0x088E98ECu);
        ARG(0) = (uint32_t)S8(first + 337) + 1;
        *B(NETP + 41) = (uint8_t)ARG(0);
    }
    for (int side = 0; side < 2; side++) {
        gaddr fighter = side == 0 ? first : second;
        uint32_t base = side == 0 ? 0x088E9908u : 0x088E993Cu;
        for (c->r[20] = 0; (int32_t)c->r[20] < 3;) {
            call2(c, 0x98, 2, f_08822EAC, base);
            *B(c->r[2] + 10) = (uint8_t)c->r[20];
            c->r[20]++;
            *W(c->r[2] + 92) = fighter;
        }
        call2(c, 0x98, 6, f_08822EAC, base + 0x20);
        *W(c->r[2] + 92) = fighter;
    }
    ARG(0) = (uint32_t)S16(NETP + 52);
    *H(b + 56) = (uint16_t)ARG(0);
    call0(c, f_088157A0, 0x088E996Cu);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (a, b): the link game's step before a fight, once the routine at 0x088E3604 says the two
   machines are ready: the screens are cleared and b's +56 set from the link's record. Outside
   kind 6 (a's +114), when the routine at 0x088D5E7C says so, the step goes on two with +22 at
   2. Else a wait of 0x4B0 frames is set (+20, with +22 at 1) and the step goes on one: kind 6
   makes the screen of the two sides, the others make two objects of kind 1 (one from each
   side of the screen). A 0x20 frame keeping s0..s2. */
SFA_FUNC(088E998C, link_before_fight_step) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = b;
    *W(sp + 16) = s0; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = a;
    call0(c, f_088E3604, 0x088E99ACu);
    if (c->r[2] != 0) {
        call0(c, f_0882235C, 0x088E99BCu);
        call0(c, f_08823304, 0x088E99C4u);
        c->r[18] = 6;
        *H(b + 56) = (uint16_t)S16(NETP + 52);
        int skip = 0;
        ARG(0) = (uint32_t)S8(a + 114);
        if (S8(a + 114) != 6) {
            call0(c, f_088D5E7C, 0x088E99ECu);
            if (c->r[2] != 0) {
                ARG(0) = a; call0(c, f_08822934, 0x088E99FCu);
                call0(c, f_0880AA74, 0x088E9A04u);
                *H(a + 22) = 2;
                *H(a + 4) = (uint16_t)(*H(a + 4) + 2);
                skip = 1;
            }
        }
        if (!skip) {
            call2(c, a, 0, f_088E336C, 0x088E9A24u);
            *H(a + 20) = 0x4B0;
            *H(a + 22) = 1;
            if (S8(a + 114) == 6) {
                call2(c, a, b, f_088E97A4, 0x088E9A48u);
            } else {
                call2(c, 1, 0, f_08823338, 0x088E9A60u);
                gaddr made = c->r[2];
                if (made != 0) {
                    *W(made + 8) = 0x00F00000u;
                    *W(made + 108) = 0;
                    *W(made + 116) = 0x00F00000u;
                    *H(made + 26) = 0x38;
                }
                call2(c, 1, 0, f_08823338, 0x088E9A8Cu);
                made = c->r[2];
                if (made != 0) {
                    *W(made + 8) = 0xFFF00000u;
                    *W(made + 108) = 0x30000;
                    *W(made + 116) = 0x180000;
                    *H(made + 26) = 0x30;
                }
                call0(c, f_0882DD5C, 0x088E9ABCu);
            }
            c->r[17] = *H(a + 4) + 1u;
            *H(a + 4) = (uint16_t)c->r[17];
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
