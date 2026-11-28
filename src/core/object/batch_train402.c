/* The practice mode's frame for a fighter, which no test scenario runs: what the second fighter
   does by the menu's choice, with the recording and the playing back of a player's moves.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME     0x08E75910u
#define BATTLE   0x092E1228u
#define PRACTICE 0x0950FF60u /* the practice mode's state: +5 what the fighter does, +7 a second
                                choice, +40 the recording's step, +42 its frames left, +44 its
                                place, +46 whether one is kept, +48 a choice kept in the
                                fighter's +810, +52 a wait */
#define TAPE     0x09525888u /* the recorded buttons, a halfword a frame; 0x2000 ends them */
#define SHOWN    0x08E75C58u /* bit 3: the blinking text is on this frame */

void f_08849E9C(Cpu *c); void f_08849DB4(Cpu *c); void f_0884AB58(Cpu *c); void f_08849E54(Cpu *c);
void f_08864DFC(Cpu *c); void f_08848728(Cpu *c); void f_0880A6CC(Cpu *c); void f_08828C04(Cpu *c);
void f_0884A2D0(Cpu *c); void f_08849F44(Cpu *c); void f_088487E8(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call4(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t e, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; ARG(3) = e; c->r[31] = back; fn(c);
}
static void buttons_clear(gaddr f) { *H(f + 362) = 0; *H(f + 752) = 0; *H(f + 764) = 0; }
static void text(Cpu *c, gaddr s, uint32_t x, uint32_t back) {
    c->r[8] = 9;
    call4(c, s, 0, x, 0x5A, f_0880A6CC, back);
}

/* (f, g): the practice mode's frame for the fighter f, with the other fighter g.
     0: nothing.  4: f's +373 is set and both gauges are held at their level.
     1 to 3: f's buttons come from the menu's choice alone, with its marks and gauges kept up
       (not at all while the second choice is 5 and f's +127 is set).
     5, recording: once the fight is under way (the battle's +44 at 6) 0x258 frames are taken:
       g stands still while f is driven by g's buttons, which go on the tape with the shortcut
       buttons' settings; the seconds left are shown; then a wait of 0x3C frames, and the end.
     6, playing back: the tape drives f (left and right turned when f faces the other way from
       when it was recorded), with ten frames between one run and the next.
   Last, the record of f's numbers is kept up and shown. A 0x40 frame keeping s0..s4. */
SFA_FUNC(0884A4B4, practice_fighter_frame) {
    gaddr f = ARG(0), g = ARG(1);
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0]; *W(sp + 40) = sv[2];
    c->r[16] = f;
    c->r[18] = PRACTICE;
    *B(f + 373) = 0;
    int32_t choice = S8(PRACTICE + 48);
    *W(sp + 36) = sv[1]; *W(sp + 44) = sv[3]; *W(sp + 48) = sv[4]; *W(sp + 52) = ra;
    c->r[17] = g;
    if (choice == 0) *B(f + 810) = 7;
    else if (choice == 1) *B(f + 810) = 0;
    else if (choice == 2) *B(f + 810) = 8;
    uint32_t mode = *B(PRACTICE + 5);
    if (mode == 4) {
        *B(f + 373) = 1;
        call2(c, PRACTICE, g, f_08849E9C, 0x0884A568u);
        call2(c, PRACTICE, f, f_08849E9C, 0x0884A574u);
    } else if (mode >= 1 && mode <= 3) {
        *H(f + 362) = 0;
        call4(c, PRACTICE, f, g, 1, f_08849DB4, 0x0884A594u);
        int held = *B(PRACTICE + 7) == 5;
        if (!(held && S8(f + 127) != 0)) {
            uint32_t back = held ? 0x0884A5CCu : 0x0884A614u;
            *B(PRACTICE + 17) = 0;
            *B(PRACTICE + 11) = 0;
            call4(c, PRACTICE, f, g, 1, f_0884AB58, back);
            call2(c, PRACTICE, f, f_08849E54, back + 0xC);
            call2(c, PRACTICE, g, f_08849E9C, back + 0x18);
            call2(c, PRACTICE, f, f_08849E9C, back + 0x24);
        }
        ARG(0) = *H(f + 362) & 0x83FFu;
        *H(f + 764) = (uint16_t)ARG(0);
        *H(f + 752) = (uint16_t)ARG(0);
    } else if (mode == 5) {
        int32_t step = S8(PRACTICE + 40);
        if (step == 0) {
            if (S8(BATTLE + 44) == 6) {
                buttons_clear(g);
                *B(PRACTICE + 40) = (uint8_t)(S8(PRACTICE + 40) + 1);
                *H(PRACTICE + 42) = 0x258;
                *H(PRACTICE + 44) = 0;
                *H(PRACTICE + 46) = 1;
                gaddr from = 0x08A058A0u + ((S8(GAME + 53) & 1) != 0 ? 16 : 36);
                *B(0x09525D98u) = *B(from);
                *B(0x09525D99u) = *B(from + 1);
                *B(0x09525D9Au) = *B(from + 2);
            }
        } else if (step == 1) {
            *H(f + 362) = *H(g + 362);
            buttons_clear(g);
            c->r[19] = *B(f + 2);
            *B(f + 2) = *B(g + 2);
            ARG(0) = f; c->r[31] = 0x0884A758u; f_08864DFC(c);
            *B(f + 2) = (uint8_t)c->r[19];
            uint32_t at = (uint32_t)S16(PRACTICE + 44);
            *H(PRACTICE + 44) = (uint16_t)(at + 1);
            *H(TAPE + at * 2) = *H(f + 362);
            c->r[20] = 0x08A1388Eu;
            c->r[19] = 0x3C;
            for (uint32_t i = 0; i < 8; i++) *H(0x09525D88u + 2 * i) = *H(f + 2 * i + 1150);
            *H(PRACTICE + 42) = (uint16_t)(S16(PRACTICE + 42) - 1);
            if (S16(PRACTICE + 42) <= 0) {
                *H(TAPE + (uint32_t)S16(PRACTICE + 44) * 2) = 0x2000;
                *H(PRACTICE + 46) = 0;
                *B(PRACTICE + 40) = (uint8_t)(S8(PRACTICE + 40) + 1);
                *W(PRACTICE + 52) = 0x3C;
            }
            call4(c, PRACTICE, g, f, 0, f_08849DB4, 0x0884A818u);
            ARG(0) = *H(g + 362) & 0x83FFu;
            *H(g + 764) = (uint16_t)ARG(0);
            *H(g + 752) = (uint16_t)ARG(0);
            *B(PRACTICE + 17) = 0;
            call4(c, PRACTICE, g, f, 0, f_0884AB58, 0x0884A840u);
            call2(c, PRACTICE, g, f_08849E9C, 0x0884A84Cu);
            call2(c, PRACTICE, f, f_08849E9C, 0x0884A858u);
            DIV(c, (uint32_t)S16(PRACTICE + 42), c->r[19]);
            call2(c, c->r[20] + 0xF, c->lo, f_08848728, 0x0884A86Cu);
            if ((*W(SHOWN) & 8) != 0) text(c, c->r[20], 0x74, 0x0884A894u);
        } else if (step == 2) {
            buttons_clear(g);
            buttons_clear(f);
            int32_t wait = (int32_t)*W(PRACTICE + 52) - 1;
            *W(PRACTICE + 52) = (uint32_t)wait;
            if (wait < 0) {
                *W(PRACTICE + 52) = 0;
                *B(PRACTICE + 40) = (uint8_t)(S8(PRACTICE + 40) + 1);
            }
            if ((*W(SHOWN) & 8) != 0) text(c, 0x089FEDF8u, 0x78, 0x0884A900u);
        } else if (step == 3) {
            buttons_clear(g);
            buttons_clear(f);
            if ((*W(SHOWN) & 8) != 0) text(c, 0x089FEDF8u, 0x78, 0x0884A94Cu);
        }
        if (S8(PRACTICE + 40) < 2) {
            ARG(0) = g; c->r[31] = 0x0884A964u; f_08828C04(c);
        }
    } else if (mode == 6) {
        int32_t step = S8(PRACTICE + 40);
        if (step == 0) {
            if (S8(BATTLE + 44) == 6) {
                *B(PRACTICE + 40) = (uint8_t)(step + 1);
                if (S16(PRACTICE + 46) != 0) {
                    *H(TAPE + (uint32_t)S16(PRACTICE + 44) * 2) = 0x2000;
                    *H(PRACTICE + 46) = 0;
                }
                *H(PRACTICE + 42) = 0;
                *H(PRACTICE + 44) = 0;
            }
        } else if (step == 1) {
            *H(f + 362) = 0;
            uint32_t at = (uint32_t)S16(PRACTICE + 44);
            *H(PRACTICE + 44) = (uint16_t)(at + 1);
            *H(f + 362) = *H(TAPE + at * 2);
            if ((*H(f + 362) & 0x2000) != 0) {
                *H(f + 362) = 0;
                *H(PRACTICE + 44) = 0;
                *H(PRACTICE + 42) = 0xA;
                *B(PRACTICE + 40) = 2;
            } else if ((*B(f + 11) != 0) == (S8(f + 337) != 0)) {
                uint32_t v = *H(f + 362);
                *H(f + 362) = (uint16_t)(v & 0xFFFFFFFCu);
                uint32_t turned = ((v & 2) >> 1) | ((v & 1) << 1);
                *H(f + 362) = (uint16_t)(turned | *H(f + 362));
            }
            call2(c, f, *H(f + 362), f_0884A2D0, 0x0884AABCu);
        } else if (step == 2) {
            int32_t left = S16(PRACTICE + 42) - 1;
            *H(PRACTICE + 42) = (uint16_t)left;
            if ((int16_t)left < 0) {
                *H(PRACTICE + 42) = 0;
                *B(PRACTICE + 40) = 1;
            }
            *H(f + 362) = 0;
        }
        call4(c, PRACTICE, f, g, 1, f_0884AB58, 0x0884AB00u);
        call2(c, PRACTICE, g, f_08849E9C, 0x0884AB0Cu);
        call2(c, PRACTICE, f, f_08849E9C, 0x0884AB18u);
    }
    ARG(2) = g;
    call2(c, PRACTICE, f, f_08849F44, 0x0884AB28u);
    ARG(2) = g;
    call2(c, PRACTICE, f, f_088487E8, 0x0884AB38u);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}
