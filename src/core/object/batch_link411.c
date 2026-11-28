/* The link play's lobby loop, and a fight's step that sets it up first. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define NETP   0x09523980u

void f_088BDE40(Cpu *c); void f_088BDFBC(Cpu *c); void f_0880B6CC(Cpu *c); void f_088D5704(Cpu *c);
void f_088E30FC(Cpu *c); void f_088E358C(Cpu *c); void f_08820C94(Cpu *c); void f_08837690(Cpu *c);
void f_08887AFC(Cpu *c); void f_088DDB78(Cpu *c); void f_088D6BC0(Cpu *c); void f_088D7FF4(Cpu *c);
void f_088E3FF0(Cpu *c); void f_088859C8(Cpu *c); void f_088054F0(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (a, b): a's first step (+6 at 0) sets the fight up, its later ones run the routine at
   0x088BDFBC; then the frame's last routine. A 0x20 frame. */
SFA_FUNC(088BDDE8, fight_b_step) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[7] = *H(a + 6);
    c->r[6] = b;
    *W(c->r[29] + 16) = ra;
    if (c->r[7] == 0) call2(c, a, b, f_088BDE40, 0x088BDE14u);
    else call2(c, a, b, f_088BDFBC, 0x088BDE2Cu);
    call0(c, f_0880B6CC, 0x088BDE34u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The link play's lobby (it never returns): after the set-up, each frame the link's count (+55)
   goes up and the link is served; when it has something, or the link's +8 is set, the game's
   count (+32) goes up, the link's +57 and +58 are cleared, and the lobby's own step (the
   link's script at +20, by its +24) runs with the drawing; then the frame's routines and its
   end. A 0x20 frame. */
SFA_FUNC(088D8128, lobby_loop) {
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = c->r[17]; *W(sp + 24) = c->r[18];
    c->r[18] = NETP;
    c->r[17] = GAME;
    *W(sp + 16) = c->r[16];
    c->r[16] = BATTLE;
    *W(sp + 28) = c->r[31];
    ARG(2) = BATTLE;
    call2(c, NETP, GAME, f_088D5704, 0x088D8164u);
    for (;;) {
        ARG(1) = *B(NETP + 55) + 1u;
        *B(NETP + 55) = (uint8_t)ARG(1);
        ARG(0) = GAME; c->r[31] = 0x088D8178u; f_088E30FC(c);
        call0(c, f_088E358C, 0x088D8180u);
        if (c->r[2] != 0 || *W(NETP + 8) != 0) {
            *B(GAME + 32) = (uint8_t)(*B(GAME + 32) + 1);
            *B(NETP + 58) = 0;
            *B(NETP + 57) = 0;
            call0(c, f_08820C94, 0x088D81ACu);
            uint32_t step = *W(*W(NETP + 20) + ((uint32_t)*H(NETP + 24) << 2));
            ARG(1) = step;
            ARG(0) = NETP;
            c->r[31] = 0x088D81C8u; rt_call_indirect(c, step);
            call0(c, f_08837690, 0x088D81D0u);
            call0(c, f_08887AFC, 0x088D81D8u);
            call0(c, f_088DDB78, 0x088D81E0u);
        }
        call1(c, NETP, f_088D6BC0, 0x088D81E8u);
        call2(c, GAME, BATTLE, f_088D7FF4, 0x088D81F4u);
        call1(c, BATTLE, f_088E3FF0, 0x088D81FCu);
        call0(c, f_088859C8, 0x088D8204u);
        call0(c, f_0880B6CC, 0x088D820Cu);
        call1(c, 1, f_088054F0, 0x088D8214u);
    }
}
