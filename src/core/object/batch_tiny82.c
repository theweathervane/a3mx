/* Small functions the scenarios reach, 99 - 105 instructions: a menu screen's countdown with its
   five list-0x7D objects, two fighters' push boxes kept apart. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08822934(Cpu *c); void f_08822EAC(Cpu *c); void f_08823338(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (screen): step 0 counts +5 down: at 9 sound 8 / 1, at 5 GAME +10 cleared and 0x08822934, at 2 (with
   0x095176F3 at 5 or more) on to step 1 with four list-0x7D objects (0 - 3, +10 = 3) and a fifth (0xA),
   sound 0x1D; step 1 waits for a press (GAME +10's high byte) to move on (+3, GAME +20 = 4, +8 - +10
   = 1) and loads three palettes (0x08A35330 +0 / +0x1E0 / +0x260 into 1 / 4 / 8). */
SFA_FUNC(088B38F8, menu_screen_countdown) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 36) = sv[1];
    uint32_t st = *B(o + 4);
    c->r[17] = st;
    *W(sp + 32) = sv[0]; *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3]; *W(sp + 48) = ra;
    c->r[16] = o;
    if (st == 0) {
        int32_t v = S8(o + 5);
        if (v == 9) {
            call2(c, 8, 1, 0x088B3938u, f_08823338);
            v = S8(o + 5);
        } else if (v == 5) {
            *H(GAME + 10) = 0;
            ARG(0) = GAME; c->r[31] = 0x088B3958u; f_08822934(c);
            v = S8(o + 5);
        } else if (v == 2 && !(S8(0x095176F3u) < 5)) {
            *B(o + 4) = (uint8_t)(st + 1);
            c->r[17] = 1; c->r[18] = 3;
            for (uint32_t i = 0; i < 4; i++) {
                c->r[19] = i;
                call2(c, 0x7D, i & 0xFF, 0x088B3998u, f_08822EAC);
                gaddr n = c->r[2];
                *B(n) = 1; *B(n + 10) = 3; *B(n + 78) = 0;
            }
            c->r[19] = 4;
            call2(c, 0x7D, 0xA, 0x088B39BCu, f_08822EAC);
            *B(c->r[2]) = 1;
            call2(c, 0x1D, 0, 0x088B39CCu, f_08823338);
            v = S8(o + 5);
        }
        *B(o + 5) = (uint8_t)(v - 1);
    } else if ((*H(GAME + 10) & 0xFF00) != 0) {
        *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
        *H(GAME + 20) = 4;
        *B(o + 10) = 1; *B(o + 9) = 1; *B(o + 8) = 1;
        c->r[16] = 0x08A35330u;
        static const uint32_t off[3] = { 0, 0x1E0, 0x260 }, slot[3] = { 1, 4, 8 };
        static const uint32_t back[3] = { 0x088B3A38u, 0x088B3A50u, 0x088B3A68u };
        for (int i = 0; i < 3; i++) {
            ARG(0) = 2; ARG(1) = 0x08A35330u + off[i]; ARG(2) = 0; ARG(3) = slot[i]; c->r[8] = 1;
            c->r[31] = back[i]; f_0880AEA8(c);
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* (a, b): with b on the ground (+73 clear) and both push boxes (their +256 frame's +3 entry of +220, 8
   bytes: x, y, half w, half h) overlapping, b is pushed out along x by the overlap, or (b's +238 set)
   b's +92 object and a are pushed the other way. */
SFA_FUNC(08876304, push_boxes_apart) {
    gaddr a = ARG(0), b = ARG(1);
    if (S8(b + 73) != 0) return;
    uint32_t ka = *B(*W(a + 256) + 3);
    if (ka == 0) return;
    uint32_t kb = *B(*W(b + 256) + 3);
    if (kb == 0) return;
    gaddr ba = *W(a + 220) + 8 * ka, bb = *W(b + 220) + 8 * kb;
    int16_t dy = (int16_t)((int16_t)(S16(bb + 2) + S16(b + 22)) - (int16_t)(S16(ba + 2) + S16(a + 22)));
    if (dy < 0) dy = (int16_t)-dy;
    if ((int16_t)(dy - (int16_t)(S16(ba + 6) + S16(bb + 6))) >= 0) return;
    c->r[2] = *B(a + 11);
    int16_t xa = *B(a + 11) != 0 ? (int16_t)(S16(a + 18) - S16(ba)) : (int16_t)(S16(ba) + S16(a + 18));
    int32_t bx = S16(b + 18);
    int16_t xb = *B(b + 11) != 0 ? (int16_t)(bx - S16(bb)) : (int16_t)(S16(bb) + bx);
    int16_t d = (int16_t)(xb - xa);
    int neg = 0;
    if (d < 0) { d = (int16_t)-d; neg = 1; }
    int16_t ov = (int16_t)(d - (int16_t)(S16(ba + 4) + S16(bb + 4)));
    if (ov >= 0) return;
    int16_t push = neg ? ov : (int16_t)-ov;
    if (S8(b + 238) == 0) *H(b + 18) = (uint16_t)(bx + push);
    else {
        gaddr q = *W(b + 92);
        *H(q + 18) = (uint16_t)(S16(q + 18) - push);
        *H(a + 18) = (uint16_t)(S16(a + 18) - push);
    }
}
