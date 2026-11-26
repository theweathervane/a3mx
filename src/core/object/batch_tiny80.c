/* Small functions the scenarios reach, 95 - 98 instructions: two falling-object starts sharing
   one picture set. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08821330(Cpu *c); void f_08821368(Cpu *c); void f_08822EAC(Cpu *c); void f_0887684C(Cpu *c);
void f_08876998(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* The start the two share: on (+4, +9 cleared), +344 = -1, its owner's +24, +352 / +356 = 0x08A8C208 /
   0x08A8C250, pictures 0x08A8BC00 / 0x08A8BA68 (+128 / +200, 0x08876998). */
static void falling_start(Cpu *c, gaddr o, gaddr p, uint32_t back) {
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(p + 24);
    *W(o + 352) = 0x08A8C208u;
    *W(o + 356) = 0x08A8C250u;
    *W(o + 128) = 0x08A8BC00u;
    *W(o + 200) = 0x08A8BA68u;
    c->r[5] = 0x08A90000u;
    call_on(c, o, back, f_08876998);
}

/* (object): the falling start, its owner's (+88) +12 - +14 / +38 / +121 / +122, +347 cleared, +73 = 1,
   +112 = 3, +92 = itself; at the screen edge (0x08821330's x) + 0x40 + 0 - 255 (0x08821368), 0x10 below
   the owner's ground (+132) and 0xC0 above that, falling (+108 = -0x4800) for 0x3C frames (+83), +0 =
   1, a list-0x27 shadow (0x08822EAC, 0xF) pointing at it, 0x0887684C (strength + 3). */
SFA_FUNC(0896FA6C, falling_object_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    falling_start(c, o, p, 0x0896FAE0u);
    *B(o + 122) = (uint8_t)S8(p + 122);
    *B(o + 347) = 0;
    *B(o + 121) = (uint8_t)S8(p + 121);
    *B(o + 14) = *B(p + 14);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *B(o + 73) = 1;
    *H(o + 112) = 3;
    *W(o + 92) = o;
    call_on(c, p, 0x0896FB30u, f_08821330);
    int32_t edge = S16(c->r[2] + 18);
    c->r[18] = (uint32_t)edge;
    c->r[31] = 0x0896FB38u; f_08821368(c);
    *H(o + 18) = (uint16_t)(edge + (int32_t)(c->r[2] & 0xFF) + 0x40);
    *H(o + 132) = (uint16_t)(S16(p + 132) + 0x10);
    *H(o + 22) = (uint16_t)(S16(o + 132) + 0xC0);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 104) = 0;
    *W(o + 164) = *W(p + 164);
    *W(o + 100) = 0; *W(o + 96) = 0;
    *W(o + 108) = 0xFFFFB800u;
    *B(o + 0x53) = 0x3C;
    *B(o) = 1;
    call2(c, 0x27, 0xF, 0x0896FBB0u, f_08822EAC);
    if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    call2(c, o, ((uint32_t)S8(o + 346) + 3) & 0xFFFF, 0x0896FBD0u, f_0887684C);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): the falling start, its owner's (+88) +11 - +14 / +38 / +121 / +122 / side / +164, over the
   owner and 0xC8 above its ground (+132), falling (+108 = -0x4800), x moved by its +73's offset
   (0x08A8E03C, its way), +82 = 6 for odd +73s, then 0x0887684C (strength + a random 0x08A8E02C
   entry). */
SFA_FUNC(0896F73C, falling_object_start_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 24) = ra;
    falling_start(c, o, p, 0x0896F7ACu);
    *B(o + 122) = (uint8_t)S8(p + 122);
    *B(o + 121) = (uint8_t)S8(p + 121);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *H(o + 112) = 0;
    *H(o + 38) = (uint16_t)S16(p + 38);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 132) = (uint16_t)S16(p + 132);
    *W(o + 104) = 0;
    *H(o + 22) = (uint16_t)(S16(p + 132) + 0xC8);
    *W(o + 100) = 0; *W(o + 96) = 0;
    *W(o + 108) = 0xFFFFB800u;
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    int32_t a = S8(o + 73);
    int32_t dx = S16(0x08A8E03Cu + 2 * (uint32_t)a);
    *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? S16(o + 18) + dx : S16(o + 18) - dx);
    *H(o + 82) = (a & 1) ? 6 : 0;
    ARG(0) = (a & 1) ? 6u : 0u;
    c->r[31] = 0x0896F888u; f_08821368(c);
    uint32_t arg = ((uint32_t)(S8(0x08A8E02Cu + (c->r[2] & 0xF)) + S8(o + 346))) & 0xFFFF;
    call2(c, o, arg, 0x0896F8B0u, f_0887684C);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
