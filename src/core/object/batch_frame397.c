/* The game's start and each frame's drawing, and two of the link play's main loops. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_089E6CEC(Cpu *c); void f_089E7E34(Cpu *c); void f_089E8BC0(Cpu *c); void f_089E73F0(Cpu *c);
void f_089E877C(Cpu *c); void f_089E5570(Cpu *c); void f_089E87A8(Cpu *c); void f_089E4408(Cpu *c);
void f_089E4590(Cpu *c); void f_089E7E8C(Cpu *c); void f_089E4668(Cpu *c); void f_089E7894(Cpu *c);
void f_089E4110(Cpu *c); void f_089E4C0C(Cpu *c); void f_089E0B64(Cpu *c); void f_089E0B80(Cpu *c);
void f_0880986C(Cpu *c); void f_089E0B90(Cpu *c); void f_089E0B98(Cpu *c); void f_088167CC(Cpu *c);
void f_0880AA54(Cpu *c); void f_0882134C(Cpu *c); void f_088073E8(Cpu *c); void f_08804ECC(Cpu *c);
void f_08804A74(Cpu *c); void f_0880A39C(Cpu *c); void f_08838688(Cpu *c); void f_08805564(Cpu *c);
void f_088054F0(Cpu *c); void f_088235C0(Cpu *c); void f_08820C94(Cpu *c); void f_088859C8(Cpu *c);
void f_08837AFC(Cpu *c); void f_08821FEC(Cpu *c); void f_088D425C(Cpu *c); void f_0882362C(Cpu *c);
void hle_sceCtrlSetSamplingMode(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* The frame is drawn: the frame's count is kept (0x08E65C40 to 0x08E65C42), a list begun at the
   frame's buffer (0x08E65C48), states 0 and 2 switched on, the scissor set to 480 by 224, the
   back screen made the target, the background drawn, the frame's records run by layer and the
   fade drawn; then the target set to the frame's other buffer (0x08E65C4C) with state 2 off for
   the screen's own drawing. The list is finished, and its size as a part of 0x18000 bytes, out
   of 240, is kept at 0x08E67160. A 0x20 frame. */
SFA_FUNC(089E47C4, frame_draw) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    ARG(1) = *W(0x08E65C48u);
    *H(0x08E65C42u) = *H(0x08E65C40u);
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0; ARG(2) = 4; c->r[31] = 0x089E47F0u; f_089E6CEC(c);
    call1(c, 0, f_089E7E34, 0x089E47F8u);
    call1(c, 2, f_089E7E34, 0x089E4800u);
    ARG(3) = 0xE0;
    call3(c, 0, 0, 0x1E0, f_089E8BC0, 0x089E4814u);
    call3(c, 1, *W(0x08D65C08u), 0x200, f_089E73F0, 0x089E4828u);
    call2(c, 4, 0, f_089E877C, 0x089E4834u);
    c->r[31] = 0x089E483Cu; f_089E5570(c);
    call2(c, 0, 1, f_089E87A8, 0x089E4848u);
    c->r[31] = 0x089E4850u; f_089E4408(c);
    c->r[31] = 0x089E4858u; f_089E4590(c);
    call3(c, 1, *W(0x08E65C4Cu), 0x200, f_089E73F0, 0x089E486Cu);
    call1(c, 2, f_089E7E8C, 0x089E4874u);
    c->r[31] = 0x089E487Cu; f_089E4668(c);
    c->r[31] = 0x089E4884u; f_089E7894(c);
    ARG(0) = (c->r[2] << 8) - (c->r[2] << 4);
    ARG(1) = 0x18000;
    DIVU(c, ARG(0), ARG(1));
    ARG(0) = 0x08E60000u;
    ARG(1) = c->lo;
    *H(0x08E67160u) = (uint16_t)c->lo;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The game is started: the picture's scale and place set (1, 1, 1, 0.5; 0, 640, 240), the
   buttons read as an analogue pad, the display set up, the font opened, the fade's colours set,
   the sound started, the frame's two routines given (0x088059E8 at its start and 0x08804E68 for
   its fade), the save system readied, four more floats (0, 0, 8, 8), the two lists of 48 words
   at 0x08E75A18 and 0x08E75AD8 cleared, and the first routines of the game run. A 0x20 frame. */
SFA_FUNC(08804CFC, game_start) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    *W(0x08E759E8u) = 0x3F800000u;
    *W(0x08E759ECu) = 0x3F800000u;
    *W(0x08E759F0u) = 0x3F800000u;
    c->f[12] = 0x3F000000u;
    *W(0x08E759F4u) = 0x3F000000u;
    *H(0x08E759F8u) = 0;
    *H(0x08E759FAu) = 0x280;
    *H(0x08E759FCu) = 0xF0;
    ARG(1) = 0x08E70000u;
    call1(c, 1, hle_sceCtrlSetSamplingMode, 0x08804D5Cu);
    c->r[31] = 0x08804D64u; f_089E4110(c);
    c->r[31] = 0x08804D6Cu; f_089E4C0C(c);
    call3(c, 0xFF000000u, 0xFF000000u, 0xFF000000u, f_089E0B64, 0x08804D7Cu);
    call1(c, 0, f_089E0B80, 0x08804D84u);
    c->r[31] = 0x08804D8Cu; f_0880986C(c);
    call1(c, 0x088059E8u, f_089E0B90, 0x08804D98u);
    call1(c, 0x08804E68u, f_089E0B98, 0x08804DA4u);
    c->r[31] = 0x08804DACu; f_088167CC(c);
    *W(0x08E75A00u) = 0;
    *W(0x08E75A04u) = 0;
    c->f[12] = 0x41000000u;
    *W(0x08E75A08u) = 0x41000000u;
    *W(0x08E75A0Cu) = 0x41000000u;
    call2(c, 0, 0, f_0880AA54, 0x08804DE4u);
    *W(0x08E75A10u) = 0;
    ARG(1) = 0x08E70000u;
    *W(0x08E75A14u) = 1;
    call1(c, 0x1C3, f_0882134C, 0x08804E00u);
    for (uint32_t i = 0; i < 0x30; i++) {
        *W(0x08E75A18u + 4 * i) = 0;
        *W(0x08E75AD8u + 4 * i) = 0;
    }
    c->r[31] = 0x08804E38u; f_088073E8(c);
    c->r[31] = 0x08804E40u; f_08804ECC(c);
    ARG(1) = 1;
    *B(0x08E75B98u) = 1;
    call1(c, 0, f_08804A74, 0x08804E54u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* What both of the link play's loops clear before they begin. */
static void loop_begin(Cpu *c, uint32_t s0, uint32_t s1, uint32_t ra, uint32_t back) {
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = GAME;
    *B(GAME + 89) = 0;
    *B(GAME + 138) = 0;
    *W(sp + 16) = s0;
    *B(GAME + 135) = 0;
    *W(sp + 24) = ra;
    c->r[16] = BATTLE;
    c->r[31] = back; f_0880A39C(c);
}

/* One of the link play's main loops (it never returns): after the set-up, each frame the game's
   count (+32) goes up and the frame's routines run with the course's object, again in the same
   frame for as long as the battle's +23 is set; then the frame ends. A 0x20 frame. */
SFA_FUNC(08837A44, link_loop_a) {
    loop_begin(c, c->r[16], c->r[17], c->r[31], 0x08837A74u);
    c->r[31] = 0x08837A7Cu; f_08838688(c);
    *B(GAME + 82) = 0; *B(GAME + 81) = 0; *B(GAME + 93) = 0; *B(GAME + 92) = 0;
    call1(c, 3, f_08805564, 0x08837A94u);
    ARG(0) = 0xFFFFFFFFu;
    *B(GAME + 54) = 0xFF;
    call1(c, 1, f_088054F0, 0x08837AA4u);
    for (;;) {
        *B(GAME + 36) = 0xFF;
        do {
            *B(GAME + 32) = (uint8_t)(*B(GAME + 32) + 1);
            call2(c, GAME, BATTLE, f_088235C0, 0x08837AC4u);
            c->r[31] = 0x08837ACCu; f_08820C94(c);
            c->r[31] = 0x08837AD4u; f_088859C8(c);
            call2(c, GAME, BATTLE, f_08837AFC, 0x08837AE0u);
        } while (S8(BATTLE + 23) != 0);
        call1(c, 1, f_088054F0, 0x08837AF4u);
    }
}

/* The other (it never returns either): the same set-up with one more routine; each turn the
   game's count goes up and the frame's routines run, the last of them (0x0882362C) in place of
   the frame's end. A 0x20 frame. */
SFA_FUNC(088D41B0, link_loop_b) {
    loop_begin(c, c->r[16], c->r[17], c->r[31], 0x088D41E0u);
    *B(GAME + 82) = 0; *B(GAME + 81) = 0; *B(GAME + 93) = 0; *B(GAME + 92) = 0;
    call1(c, 3, f_08805564, 0x088D41F8u);
    ARG(0) = 0xFFFFFFFFu;
    *B(GAME + 54) = 0xFF;
    c->r[31] = 0x088D4204u; f_08821FEC(c);
    call1(c, 1, f_088054F0, 0x088D420Cu);
    for (;;) {
        ARG(1) = 0xFFFFFFFFu;
        *B(GAME + 36) = 0xFF;
        *B(GAME + 32) = (uint8_t)(*B(GAME + 32) + 1);
        call2(c, GAME, BATTLE, f_088235C0, 0x088D422Cu);
        c->r[31] = 0x088D4234u; f_08820C94(c);
        c->r[31] = 0x088D423Cu; f_088859C8(c);
        call2(c, GAME, BATTLE, f_088D425C, 0x088D4248u);
        call2(c, GAME, BATTLE, f_0882362C, 0x088D4254u);
    }
}
