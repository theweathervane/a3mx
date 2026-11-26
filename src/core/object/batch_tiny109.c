/* Small functions the scenarios reach, 41 - 42 instructions: a fighter's picture table for
   characters 0x11 / 0x13 with a list-0x11 helper, a pulsing rectangle. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08822F54(Cpu *c); void f_0894CC44(Cpu *c); void f_0894D0E4(Cpu *c); void f_0880E788(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (fighter): +171 cleared, +12 = -1, 0x0894D0E4, its picture table's words 5 - 38 from 0x08A78F98, +164 =
   1, its character set (0x0894CC44) and a list-0x11 helper of it (0x08822F54) linked both ways (its +88,
   the fighter's +40); the fighter's +88 cleared. */
SFA_FUNC(089486C4, fighter_table_copy_11) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    call_on(c, f, 0x089486E8u, f_0894D0E4);
    gaddr dst = *W(f + 416) + 0x14;
    for (uint32_t n = 0; n < 0x27 - 5; n++) *W(dst + 4 * n) = *W(0x08A78F98u + 4 * n);
    *W(f + 164) = 1;
    call2(c, f, *B(f + 338), 0x08948730u, f_0894CC44);
    call2(c, 0x11, *B(f + 338), 0x08948740u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) { *W(n + 88) = f; *W(f + 40) = n; }
    *W(f + 88) = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (x, y, w, h, colour, depth): the rectangle (0x0880E788) with its alpha pulsing by the frame byte
   GAME +32 (its size as a signed byte) + 0x30. */
SFA_FUNC(088D6D74, rect_pulse) {
    int32_t x = (int32_t)ARG(0), y = (int32_t)ARG(1), w = (int32_t)ARG(2), h = (int32_t)ARG(3);
    uint32_t colour = c->r[8], depth = c->r[9];
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    int32_t a = (int8_t)*B(GAME + 32);
    if (a < 0) a = (int8_t)-a;
    *W(c->r[29] + 32) = ra;
    ARG(0) = (uint32_t)(int32_t)(int16_t)x;
    ARG(1) = (uint32_t)(int32_t)(int16_t)y;
    ARG(2) = (uint32_t)(int32_t)(int16_t)(w + x);
    ARG(3) = (uint32_t)(int32_t)(int16_t)(h + y);
    c->r[8] = ((uint32_t)a << 24) + (colour & 0x00FFFFFFu) + 0x30000000u;
    c->r[9] = depth;
    c->r[2] = (uint32_t)h;
    c->r[31] = 0x088D6E10u; f_0880E788(c);
    c->r[31] = ra;
    c->r[29] += 0x30;
}
