/* A grid cursor (0x08821B6C): a column (+1 of +2) and a row (+0 of +3), each moved by a pad's press with
   the cue 0x38 or by holding it with auto-repeat (+4 / +6), wrapping round; 0xFF marks an axis that does
   not move. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C4CC(Cpu *c);

static void cue(Cpu *c, uint32_t back) { ARG(0) = 0; ARG(1) = 0x38; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c); }
static void step_back(gaddr pos, gaddr n) {
    int32_t v = (int32_t)*B(pos) - 1;
    *B(pos) = (uint8_t)(v >= 0 ? v : (int32_t)*B(n) - 1);
}
static void step_on(gaddr pos, gaddr n) {
    int32_t v = (int32_t)*B(pos) + 1;
    *B(pos) = (uint8_t)(v < (int32_t)*B(n) ? (v & 0xFF) : 0);
}

/* one axis: `pos` of `n` with its repeat count `rep`; `back` / `on` are the pad bits; b[0..3] the cue
   return addresses (press back, press on, held back, held on) */
static void axis(Cpu *c, gaddr pos, gaddr n, gaddr rep, uint32_t pressed, uint32_t held, uint32_t back,
                 uint32_t on, const uint32_t b[4]) {
    if (pressed & back) {
        if (*B(n) < 2) return;
        cue(c, b[0]); *H(rep) = 0x20; step_back(pos, n);
    } else if (pressed & on) {
        if (*B(n) < 2) return;
        cue(c, b[1]); *H(rep) = 0x20; step_on(pos, n);
    } else if (held & (back | on)) {
        if (*B(n) < 2) return;
        int32_t t = S16(rep) - 1;
        *H(rep) = (uint16_t)t;
        if ((int16_t)t >= 0) return;
        if (held & back) { cue(c, b[2]); *H(rep) = 8; step_back(pos, n); }
        else { cue(c, b[3]); *H(rep) = 8; step_on(pos, n); }
    } else *H(rep) = 0x20;
}

/* (cursor, pad): the pad's 44 bytes copied to the frame; the column by the pad's bits 3 / 2, the row by
   bits 1 / 0, pressed (+12) or held (+4). A 0x80 frame keeping s0 / s1. */
SFA_FUNC(08821B6C, grid_cursor) {
    gaddr o = ARG(0), pad = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x80;
    gaddr sp = c->r[29];
    for (int i = 0; i < 11; i++) *W(sp + 60 + 4 * i) = *W(pad + 4 * i);
    *W(sp + 108) = s1; *W(sp + 104) = s0; *W(sp + 112) = ra;
    c->r[17] = 0xFF; c->r[16] = o;
    if (*B(o + 1) != 0xFF) {
        static const uint32_t b[4] = { 0x08821C10u, 0x08821C60u, 0x08821CD0u, 0x08821D3Cu };
        axis(c, o + 1, o + 2, o + 4, *W(sp + 72), *W(sp + 64), 8, 4, b);
    }
    if (*B(o) != 0xFF) {
        static const uint32_t b[4] = { 0x08821DA4u, 0x08821DF4u, 0x08821E64u, 0x08821ED0u };
        axis(c, o, o + 3, o + 6, *W(sp + 72), *W(sp + 64), 2, 1, b);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x80;
}
