/* Small functions the scenarios reach, 82 instructions: the select screen's buffers made and its
   palette lines faded in, a fighter's picture table with its list-0x1C / 0x1B helpers, an effect
   thrown up off its owner. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_088054F0(Cpu *c); void f_088075F8(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E04EC(Cpu *c);
void f_089E1234(Cpu *c); void f_08822EAC(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c);
void f_08821424(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* The select screen's start (between 0x088D7EA0 (1) and (0)): list 39's tile list (0x08A050B0 +156)
   handed to 0x08E75A18 +156, list 39's 0x1400-byte palette buffer made once (0x089E04EC), the
   current sound slot's (0x08E76084, 216 bytes each from 0x08E76088) +202 - +208 cleared, 0x088075F8
   (0x08A03140 +36, 0, 0), a wait (0x088054F0 (1)) until 0x08E76068 +4 is set, then palette lines 4 -
   0xC faded (0x089E1234 (line, 0x50)). */
SFA_FUNC(08815478, select_screen_buffers) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; *W(sp + 24) = ra;
    call_on(c, 1, 0x0881548Cu, f_088D7EA0);
    *W(0x08E75A18u + 156) = *W(0x08A050B0u + 156);
    gaddr k = 0x08A0577Cu;
    c->r[16] = 0x08E76068u;
    if (*W(k + 156) == 0) {
        *W(sp + 16) = k;
        call_on(c, 0x1400, 0x088154C8u, f_089E04EC);
        *W(k + 156) = c->r[2];
    }
    uint32_t i = *B(0x08E76084u);
    gaddr e = 216u * i + 0x08E76088u;
    *B(e + 202) = 0; *B(e + 203) = 0;
    *H(e + 208) = 0;
    *B(e + 204) = 0;
    ARG(0) = *W(0x08A03140u + 36); ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x0881551Cu; f_088075F8(c);
    while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08815530u, f_088054F0);
    for (uint32_t line = 4; line <= 0xC; line++)
        call2(c, line, 0x50, 0x08815548u + 12 * (line - 4), f_089E1234);
    call_on(c, 0, 0x088155B0u, f_088D7EA0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A9234C, 0x0886EB48
   (0x1B, 0x08A923E8), +164 = 1, +0x338 cleared; the battle's first one (BATTLE +58 counted) makes a
   list-0x1C helper (0x08822EAC (0x1C, 3): +2 = 0x1C, +3 = 3); then a list-0x1B helper (0x08822F54)
   as its +44 (+2 = 0x1B); both on and pointing back at it (+88) with its side and +164. */
SFA_FUNC(0897A0E8, fighter_table_copy_1b) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 24) = s2;
    c->r[18] = BATTLE;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 1;
    *W(c->r[29] + 28) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A9234Cu + 4 * n);
    ARG(0) = f; ARG(1) = 0x1B; ARG(2) = 0x08A923E8u;
    c->r[31] = 0x0897A15Cu; f_0886EB48(c);
    *W(f + 164) = 1;
    *B(f + 0x338) = 0;
    if (S8(BATTLE + 58) == 0) {
        call2(c, 0x1C, 3, 0x0897A180u, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x1C; *B(n + 3) = 3;
            *W(n + 88) = f;
            *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
            *W(n + 164) = *W(f + 164);
            *B(BATTLE + 58) = (uint8_t)(S8(BATTLE + 58) + 1);
        }
    }
    call2(c, 0x1B, 0, 0x0897A1D8u, f_08822F54);
    gaddr m = c->r[2];
    if (m != 0) {
        *B(m) = 1; *B(m + 1) = 0; *B(m + 2) = 0x1B; *B(m + 3) = 0;
        *W(m + 88) = f;
        *W(f + 44) = m;
        *H(m + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(m + 164) = *W(f + 164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): first (+5 = 0), once the owner's +77 is clear, thrown up from it (0x9E higher,
   0x3C ahead, steps 0x10000 / 0x8000 changing by 0x1000 / 0x200, its way) for 0x64 frames with the
   animation 0x08A9D968's +44 word; then it flies until the frames run out (on, +4); 0x088219B0 and
   drawn meanwhile. */
SFA_FUNC(089A8CE0, effect_thrown_off_owner) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (*B(o + 5) == 0) {
        if (S8(p + 77) == 0) {
            *B(o + 5) = 1;
            *H(o + 82) = 0x64;
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)(S16(p + 22) + 0x9E);
            *W(o + 100) = 0x8000;
            *W(o + 108) = 0x200;
            if (*B(o + 11) != 0) {
                *H(o + 18) = (uint16_t)(S16(o + 18) + 0x3C);
                *W(o + 96) = 0x10000; *W(o + 104) = 0x1000;
            } else {
                *H(o + 18) = (uint16_t)(S16(o + 18) - 0x3C);
                *W(o + 96) = 0xFFFF0000u; *W(o + 104) = 0xFFFFF000u;
            }
            call2(c, o, *W(0x08A9D968u + 44), 0x089A8D90u, f_0882196C);
        }
        call_on(c, o, 0x089A8D98u, f_088219B0);
        call_on(c, o, 0x089A8DA0u, f_08821424);
    } else {
        uint32_t vx = *W(o + 96), vy = *W(o + 100);
        *W(o + 16) += vx;
        *W(o + 96) = vx + *W(o + 104);
        *W(o + 20) += vy;
        *W(o + 100) = vy + *W(o + 108);
        int16_t h = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)h;
        if (h == 0) {
            *B(o + 5) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        } else {
            call_on(c, o, 0x089A8E10u, f_088219B0);
            call_on(c, o, 0x089A8E18u, f_08821424);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
