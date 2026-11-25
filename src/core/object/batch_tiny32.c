/* Small functions the scenarios reach, 33 instructions: fighter objects' starts, a fading effect's
   timer, a fighter object's hold timer. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_08821424(Cpu *c);
void f_0890DA50(Cpu *c); void f_0886E294(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

static void on_7(gaddr o, uint32_t k236) {
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
    *B(o + 236) = (uint8_t)k236; *B(o + 235) = 1;
    *B(o + 11) = (uint8_t)S8(o + 697);
}

/* (effect): on when 0x0890DA50 says so; +82 counted down, drawn while it lasts and then on alternate
   frames (GAME +59 bit 0). */
SFA_FUNC(0890D8C8, effect_fade_timer) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x0890D8DCu; f_0890DA50(c);
    if (c->r[2] != 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    int16_t t = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)t;
    if (t >= 0) call_on(c, o, 0x0890D93Cu, f_08821424);
    else {
        *H(o + 82) = 0;
        if (S8(GAME + 59) & 1) call_on(c, o, 0x0890D92Cu, f_08821424);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k236; +11 = +697) and
   `then`; 1 then, else 0. */
static void start_s1(Cpu *c, uint32_t k236, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o, k236);
        c->r[17] = 1;
        call_on(c, o, back2, then);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0890FF18, object_start_s1_3) { start_s1(c, 3, f_0886DBE8, 0x0890FF30u, 0x0890FF6Cu); }
SFA_FUNC(089101D4, object_start_s1_9) { start_s1(c, 9, f_0886DB94, 0x089101ECu, 0x08910228u); }

/* As start_s1, with s1 0 from the start (it is the answer) and +196 - +199 cleared. */
static void start_196(Cpu *c, uint32_t k236, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o, k236);
        *B(o + 199) = 0; *B(o + 198) = 0; *B(o + 197) = 0; *B(o + 196) = 0;
        c->r[17] = 1;
        call_on(c, o, back2, then);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08918E20, object_start_196_2) { start_196(c, 2, f_0886DB94, 0x08918E3Cu, 0x08918E8Cu); }
SFA_FUNC(08918EA4, object_start_196_a) { start_196(c, 0xA, f_0886DBE8, 0x08918EC0u, 0x08918F10u); }

/* (object): +82 counted down; at 0 the next step (+7 on, +237 and x steps cleared, 0x0886E294) with
   +573 from 0x08A5AE10 by +194 (0 with +81 set). */
SFA_FUNC(089112B8, object_hold_timer) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    int16_t t = (int16_t)(S16(o + 82) - 1);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *H(o + 82) = (uint16_t)t;
    *W(c->r[29] + 20) = ra;
    if (t == 0) {
        *B(o + 237) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 96) = 0; *W(o + 104) = 0;
        call_on(c, o, 0x08911304u, f_0886E294);
        if (S8(o + 81) != 0) *B(o + 573) = 0;
        else *B(o + 573) = (uint8_t)S8(0x08A5AE10u + (uint32_t)(int32_t)S8(o + 194));
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
