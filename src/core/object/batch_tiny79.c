/* Small functions the scenarios reach, 90 - 95 instructions: a projectile's start from its
   strength's tables, a fighter copy stepping through pictures while its owner's move lasts. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_0899D448(Cpu *c); void f_088215C4(Cpu *c);
void f_088231C8(Cpu *c); void f_0887CA88(Cpu *c); void f_0887CAF4(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (projectile): on (+4, +9 cleared), +344 = -1, its owner's (+88) +24, pictures 0x08A99430 / 0x08A99278
   (+128 / +200, 0x08876998), +73 = 0x3C, y steps cleared; by its strength (+346) its offset (x
   0x08A9AF6C, y 0x08A9AF78; their +6 entries with +3) and speed (0x08A9AF48, or with +3 speed and
   change from 0x08A9AF54), turned for its side; +352 / +356 = 0x08A991B0 / 0x08A991D0, 0x0887684C
   (strength, + 3 with +3) and 0x0899D448. */
SFA_FUNC(0899D2E0, projectile_start_8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(*W(o + 88) + 24);
    *W(o + 128) = 0x08A99430u;
    *W(o + 200) = 0x08A99278u;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x0899D334u, f_08876998);
    int32_t k = S8(o + 346);
    *B(o + 73) = 0x3C;
    *W(o + 108) = 0;
    uint32_t k3 = *B(o + 3);
    *W(o + 100) = 0;
    gaddr tx = 0x08A9AF6Cu + 2 * (uint32_t)k, ty = 0x08A9AF78u + 2 * (uint32_t)k;
    int32_t y0 = S16(o + 22), dx, dy;
    uint32_t vx, ax;
    if (k3 != 0) {
        dx = S16(tx + 6); dy = S16(ty + 6);
        gaddr e = 0x08A9AF54u + 8 * (uint32_t)k;
        vx = *W(e); ax = *W(e + 4);
    } else {
        dx = S16(tx); dy = S16(ty);
        vx = *W(0x08A9AF48u + 4 * (uint32_t)k); ax = 0;
    }
    if (*B(o + 11) == 0) { dx = (int16_t)-dx; vx = (uint32_t)-(int32_t)vx; ax = (uint32_t)-(int32_t)ax; }
    *H(o + 22) = (uint16_t)(y0 + dy);
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *W(o + 96) = vx;
    *W(o + 104) = ax;
    uint32_t arg = k3 != 0 ? ((uint32_t)k + 3) & 0xFFFF : (uint32_t)k & 0xFFFF;
    *W(o + 352) = 0x08A991B0u;
    *W(o + 356) = 0x08A991D0u;
    call2(c, o, arg, 0x0899D430u, f_0887684C);
    call_on(c, o, 0x0899D438u, f_0899D448);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (copy): step 0 starts it (+4 = 1, +9 = 2, +12 = -1, its owner's (+88) +24, first picture via
   0x0887CA88 -> 0x0887CAF4); step 1 every +34 frames (+82 counted down, +32 counting up between, not
   while halted) takes the next picture while its owner's move lasts (kind 2: its +100 frame without
   bit 0x80; else the owner's +4 word 0x01000700 / 0x0107xxxx), otherwise off (+4 = 2); drawn with
   the owner (+1). Later steps free it (0x088231C8). */
SFA_FUNC(08966710, fighter_copy_pictures) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 4);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    if (st == 0) {
        *B(o + 4) = 1; *B(o + 9) = 2; *B(o + 12) = 0xFF;
        *H(o + 24) = *H(p + 24);
        call_on(c, o, 0x08966758u, f_0887CA88);
        call2(c, o, c->r[2], 0x08966764u, f_0887CAF4);
    } else if (st == 1) {
        int next = 0, off = 0;
        if (S8(BATTLE + 37) != 0) *H(o + 32) = (uint16_t)(S16(o + 32) + 1);
        else {
            int16_t h = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)h;
            if (h >= 0) *H(o + 32) = (uint16_t)(S16(o + 32) + 1);
            else {
                *H(o + 82) = 0;
                *H(o + 32) = (uint16_t)S16(o + 34);
                if (*B(o + 3) == 2) {
                    if (*B(*W(o + 100)) & 0x80) off = 1; else next = 1;
                } else {
                    uint32_t w = *W(p + 4);
                    if ((w & 0xFFFFFF00u) == 0x01000700u || (w & 0xFFFF0000u) == 0x01070000u) next = 1;
                    else off = 1;
                }
            }
        }
        if (off) *B(o + 4) = 2;
        else {
            if (next) {
                call_on(c, o, 0x089667DCu, f_0887CA88);
                call2(c, o, c->r[2], 0x089667E8u, f_0887CAF4);
            }
            uint32_t shown = *B(p + 1);
            c->r[17] = shown;
            if (shown != 0) call_on(c, o, 0x08966868u, f_088215C4);
        }
    } else call_on(c, o, 0x08966878u, f_088231C8);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
