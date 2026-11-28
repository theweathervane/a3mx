/* The link play's lobby: its screen where a player is picked. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define PADS 0x08E75BA0u /* three pads' records, 44 bytes each; +12 the buttons just pressed */

void f_088DD22C(Cpu *c); void f_088DCB00(Cpu *c); void f_0887C4CC(Cpu *c); void f_088DC888(Cpu *c);
void f_088DF188(Cpu *c); void f_088DD114(Cpu *c); void f_0880A6CC(Cpu *c); void f_0880A7B0(Cpu *c);
void f_088DC948(Cpu *c); void f_088E3ED8(Cpu *c); void f_088E3F98(Cpu *c); void f_088DD02C(Cpu *c);
void f_088E3F44(Cpu *c); void f_088DDDF0(Cpu *c); void f_088D68C8(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void sound(Cpu *c, uint32_t which, uint32_t back) {
    ARG(0) = 0; ARG(1) = which; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c);
}
static uint32_t pressed(void) { return *W(PADS + 12) | *W(PADS + 56) | *W(PADS + 100); }

/* The screen's script goes on to its next part: the place in it (+24) up by one, the part's own
   step and counters cleared. */
static void script_on(gaddr o) {
    *H(o + 2) = 0;
    *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
    *H(o) = 0;
    *B(o + 47) = 0;
}

/* (o): the screen's step (o's +0):
     0, 1: its fields cleared, the list refreshed, and on to 2.
     2: when a request has come from another player it is taken up (kept at +28) and the script
        goes on. Else, by the buttons: bit 7 picks the player under the cursor (with +44 at 0
        or 2; the pick kept at +28 with +32 set, and the script goes on if the request can be
        sent) or, with +44 at 1 and the routine at 0x088DD114 saying so, goes to step 3 after
        0x1E frames; bit 8 leaves by the script at 0x08A3EB0C; otherwise up and down move the
        cursor, and with +44 at 1 a hint is drawn.
     3: the lobby's message 0x10 is put up for 0xB4 frames.
     4: the wait: at its end, or on bit 8 (with +6 set to 1), or on bit 7 in its last 150 frames
        (which runs the routine at 0x088DD02C), on to 5.
     5: once the message has gone, back to step 0 when +6 is set, else on to 6.
     6: the script goes on two parts.
   Any other step sets the script at 0x08A3EB14. Then the list is drawn, with the title for the
   screen's +42 and +43, and the screen's own drawing. A 0x30 frame keeping s0, s1. */
SFA_FUNC(088D93B0, lobby_pick_screen) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s1;
    uint32_t step = *H(o);
    c->r[17] = step;
    *W(sp + 32) = s0;
    c->r[16] = o;
    *W(sp + 40) = ra;
    if (step >= 7) {
        *W(o + 20) = 0x08A3EB14u;
        *H(o + 24) = 0;
        *H(o + 2) = 0;
        *H(o) = 0;
        *B(o + 47) = 0;
    } else if (step <= 2) {
        if (step == 0) {
            *B(o + 36) = 0;
            *W(o + 12) = 0;
            *W(o + 16) = 0;
            *W(o + 28) = 0;
            *B(o + 32) = 0;
            *H(o) = 1;
        }
        if (step <= 1) {
            c->r[17] = PADS;
            call0(c, f_088DD22C, 0x088D9424u);
            *H(o) = (uint16_t)(*H(o) + 1);
        }
        uint32_t buttons = pressed();
        c->r[17] = buttons;
        call0(c, f_088DCB00, 0x088D946Cu);
        if (c->r[2] != 0) {
            *W(o + 28) = c->r[2];
            *B(o + 32) = 0;
            sound(c, 0x39, 0x088D9490u);
            script_on(o);
        } else if ((buttons & 0x80) != 0) {
            uint32_t kind = *B(o + 44);
            if (kind == 0 || kind == 2) {
                call0(c, f_088DC888, 0x088D94E8u);
                if (c->r[2] != 0) {
                    *W(o + 28) = c->r[2];
                    *B(o + 32) = 1;
                    call1(c, c->r[2] + 4, f_088DF188, 0x088D9504u);
                    if ((int32_t)c->r[2] >= 0) {
                        sound(c, 0x39, 0x088D9518u);
                        script_on(o);
                    }
                }
            } else if (kind == 1) {
                call0(c, f_088DD114, 0x088D953Cu);
                if (c->r[2] != 0) {
                    sound(c, 0x39, 0x088D9550u);
                    *H(o + 4) = 0x1E;
                    *H(o) = (uint16_t)(*H(o) + 1);
                }
            }
        } else if ((buttons & 0x100) != 0) {
            sound(c, 0x2F, 0x088D957Cu);
            *W(o + 20) = 0x08A3EB0Cu;
            *H(o + 24) = 0;
            *H(o + 2) = 0;
            *H(o) = 0;
            *B(o + 47) = 0;
        } else {
            int hinted = 0;
            if (*B(o + 44) == 1) {
                call0(c, f_088DD114, 0x088D95B4u);
                if (c->r[2] != 0) {
                    ARG(0) = 0x08A01084u; ARG(1) = 1; ARG(2) = 0xC0; ARG(3) = 0xCC; c->r[8] = 0xB;
                    c->r[31] = 0x088D95D8u; f_0880A6CC(c);
                    ARG(0) = 3; ARG(1) = 0xEC; ARG(2) = 0xCD;
                    c->r[31] = 0x088D95E8u; f_0880A7B0(c);
                    hinted = 1;
                }
            }
            if (!hinted) {
                if ((buttons & 8) != 0) call1(c, 0xFFFFFFFFu, f_088DC948, 0x088D9600u);
                else if ((buttons & 4) != 0) call1(c, 1, f_088DC948, 0x088D9618u);
            }
        }
    } else if (step == 3) {
        call1(c, 0x10, f_088E3ED8, 0x088D9628u);
        *H(o + 4) = 0xB4;
        *H(o + 6) = 0;
        *H(o) = (uint16_t)(*H(o) + 1);
    } else if (step == 4) {
        uint32_t buttons = pressed();
        c->r[17] = buttons;
        call1(c, 1, f_088E3F98, 0x088D9668u);
        int32_t wait = S16(o + 4) - 1;
        *H(o + 4) = (uint16_t)wait;
        if ((int16_t)wait == 0) {
            *H(o + 6) = 0;
            *H(o) = (uint16_t)(*H(o) + 1);
        } else if ((buttons & 0x100) != 0) {
            sound(c, 0x2F, 0x088D96ACu);
            *H(o + 6) = 1;
            *H(o) = (uint16_t)(*H(o) + 1);
        } else if (S16(o + 4) < 150 && (buttons & 0x80) != 0) {
            sound(c, 0x39, 0x088D96E8u);
            call0(c, f_088DD02C, 0x088D96F0u);
            *H(o + 6) = 0;
            *H(o) = (uint16_t)(*H(o) + 1);
        }
    } else if (step == 5) {
        call0(c, f_088E3F44, 0x088D970Cu);
        if (c->r[2] != 0) {
            if (S16(o + 6) != 0) {
                *H(o + 6) = 0;
                *H(o + 4) = 0;
                *H(o) = 0;
            } else {
                *H(o) = (uint16_t)(*H(o) + 1);
            }
        }
    } else {
        script_on(o);
        *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
    }
    call0(c, f_088DDDF0, 0x088D9784u);
    ARG(0) = *W(0x08A3EA04u + (((uint32_t)*B(o + 42) + *B(o + 43)) << 2));
    ARG(1) = 2; ARG(2) = 0x30; ARG(3) = 4; c->r[8] = 9;
    c->r[31] = 0x088D97B8u; f_0880A6CC(c);
    call1(c, o, f_088D68C8, 0x088D97C0u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}
