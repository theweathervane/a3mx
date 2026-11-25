/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 35 - 101
   instructions: the CPU's random picks of an opponent (from a list, or by chance from 11-byte rows),
   the VS screen's start (the fighters' colours by the match's line-up), a mode screen's panel set-up.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08821368(Cpu *c); void f_0882F024(Cpu *c); void f_0880AEA8(Cpu *c); void f_0881304C(Cpu *c);
void f_088158A4(Cpu *c); void f_08819144(Cpu *c); void f_08821F9C(Cpu *c); void f_088D7C1C(Cpu *c);
void f_0882196C(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* (fighter, n): a random character (0 - 41, 0x08821368 mod 42) that 0x0882F024 allows against the
   n taken and the fighter's list (0x08A0EA24 by its character +338). */
SFA_FUNC(0882EEE8, cpu_pick_random) {
    gaddr f = ARG(0);
    uint32_t n = ARG(1) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], s4 = c->r[20];
    c->r[29] -= 0x30;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = s3; *W(c->r[29] + 32) = s4;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 36) = ra;
    c->r[20] = n; c->r[19] = 0x2A; c->r[16] = f; c->r[17] = 0x08A0EA24u;
    do {
        c->r[31] = 0x0882EF20u; f_08821368(c);
        DIV(c, c->r[2], 0x2A);
        c->r[18] = c->hi & 0xFF;
        call3(c, c->r[18], n, *W(0x08A0EA24u + ((uint32_t)(int32_t)S8(f + 338) << 2)), 0x0882EF48u, f_0882F024);
    } while (c->r[2] == 0);
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[20] = s4; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter, n): the CPU's next opponent from the fighter's 11-byte rows (0x08A0ECC0 by its character,
   0xFF-ended): a row is taken when 0x08821368 comes under its chance (byte n + 1) and 0x0882F024
   allows its character (+0); the answer is that row's character, or 0xFF. */
SFA_FUNC(0882EF74, cpu_pick_rows) {
    gaddr f = ARG(0);
    uint32_t n = ARG(1) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    gaddr row = *W(0x08A0ECC0u + ((uint32_t)(int32_t)S8(f + 338) << 2));
    *B(sp + 16) = 0xFF;
    *W(sp + 28) = s2; *W(sp + 20) = s0; *W(sp + 24) = s1; *W(sp + 32) = s3; *W(sp + 36) = ra;
    c->r[19] = n; c->r[17] = 0xFF; c->r[16] = sp + 16;
    for (;;) {
        c->r[18] = row;
        if (*B(row) == 0xFF) break;
        c->r[31] = 0x0882EFCCu; f_08821368(c);
        if ((int32_t)c->r[2] < (int32_t)*B(row + n + 1)) {
            call3(c, *B(row), n, sp + 16, 0x0882EFF0u, f_0882F024);
            if (c->r[2] != 0) break;
        }
        row += 11;
    }
    c->r[2] = *B(row);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (screen, match): the VS screen's start, once 0x092E510F bit 7 and the loader (0x08E7606C) are set
   and 0x088D7C1C answers no (or yes without +145): +6 on, +8 / +10 / +128 cleared, 0x08821F9C, the
   characters at 0x092E89DC copied to 0x08E75C48, 0x088158A4, the fighters' colours (0x0881304C) by
   the match's line-up (+56: 0 one each, 1 player 1 with two partners, 2 player 2 with two, 3 players 1
   and 2 with one), the VS palettes (0x08A057B8's list) into slot 0x38, 0x08E75C50 from 0x092E89F8, y
   0x3C and +157 = -1. Otherwise 0x08819144. */
SFA_FUNC(088965E4, vs_start) {
    gaddr o = ARG(0), m = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o; c->r[17] = m;
    if (!(*B(0x092E5100u + 15) & 0x80) || *B(0x08E76068u + 4) == 0) goto wait;
    c->r[31] = 0x0889662Cu; f_088D7C1C(c);
    if (c->r[2] != 0 && S8(o + 145) != 0) { c->r[31] = 0x088966A8u; f_08819144(c); goto done; }
    {
        *H(o + 10) = 0;
        uint32_t n6 = *H(o + 6) + 1u;
        *H(o + 6) = (uint16_t)n6;
        *H(o + 8) = 0;
        *B(o + 128) = 0;
        ARG(0) = n6;
        c->r[31] = 0x0889665Cu; f_08821F9C(c);
        *H(0x08E75C48u) = (uint16_t)S16(0x092E89DCu);
        *H(0x08E75C48u + 2) = (uint16_t)S16(0x092E89DEu);
        *H(0x08E75C48u + 4) = (uint16_t)S16(0x092E89E0u);
        ARG(0) = 0x08E75C48u;
        c->r[31] = 0x08896688u; f_088158A4(c);
        int32_t lineup = S16(m + 56);
        c->r[17] = (uint32_t)lineup;
        if (lineup == 0) call3(c, 0x092E3EA0u, 0x092E4338u, 0, 0x088966E0u, f_0881304C);
        else if (lineup == 1) call3(c, 0x092E3EA0u, 0x092E47D0u, 0x092E4C68u, 0x0889671Cu, f_0881304C);
        else if (lineup == 2) call3(c, 0x092E4338u, 0x092E47D0u, 0x092E4C68u, 0x08896740u, f_0881304C);
        else if (lineup == 3) call3(c, 0x092E3EA0u, 0x092E4338u, 0x092E47D0u, 0x08896764u, f_0881304C);
        ARG(0) = 1; ARG(1) = *W(0x08A0577Cu + 60); ARG(2) = 0; ARG(3) = 0x38; c->r[8] = 6;
        c->r[31] = 0x08896784u; f_0880AEA8(c);
        *H(0x08E75C50u) = *H(0x092E89F8u);
        *H(o + 18) = 0x3C;
        *B(o + 157) = 0xFF;
        goto done;
    }
wait:
    c->r[31] = 0x088967B0u; f_08819144(c);
done:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (panel): a mode screen's panel set-up: +4 on, +32 = GAME +16's low byte, +148 cleared, +86 = 0x14,
   +164 = 1, +96 (its step) 0 for kind (+78) 0xC or 2 for 7, set up (+12 = -1, +13 = 0x20, +14 = 3, +24
   = 5, +26 = 2; +5 / +11 / +15 / +38 / +82 / +100 cleared), placed by 0x092E8940 (x + 0xC0, y - 0x30),
   its animation by the step (0x08A2EBA0, or 0x08A2ED04 with 0x09525DE3 clear) and its 8 palettes
   (0x08A35100 / 0x08A35180 by the step's byte of 0x08A2EDB0) into slot 0x20; then the step on. */
SFA_FUNC(088AF3E0, mode_panel_setup) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 32) = (uint16_t)((uint32_t)(int32_t)S16(GAME + 16) & 0xFF);
    *W(o + 148) = 0;
    *H(o + 86) = 0x14;
    *W(o + 164) = 1;
    int32_t kind = S8(o + 78);
    if (kind == 0xC) *W(o + 96) = 0;
    else if (kind == 7) *W(o + 96) = 2;
    *H(o + 38) = 0; *B(o + 15) = 0; *B(o + 11) = 0; *H(o + 82) = 0; *B(o + 5) = 0; *W(o + 100) = 0;
    *B(o + 12) = 0xFF; *B(o + 13) = 0x20; *H(o + 24) = 5; *H(o + 26) = 2; *B(o + 14) = 3;
    gaddr q = 0x092E8940u;
    *H(o + 18) = (uint16_t)(S16(q + 18) + 0xC0);
    *H(o + 22) = (uint16_t)(S16(q + 22) - 0x30);
    uint32_t step = *W(o + 96);
    uint32_t idx = (uint32_t)(int32_t)S8(0x08A2EDB0u + step) << 2;
    gaddr pal;
    if (S8(0x09525DE3u) != 0) {
        pal = *W(0x08A35100u + idx);
        c->r[17] = pal;
        call2(c, o, *W(0x08A2EBA0u + (step << 2)), 0x088AF50Cu, f_0882196C);
    } else {
        pal = *W(0x08A35180u + idx);
        c->r[17] = pal;
        call2(c, o, *W(0x08A2ED04u + (step << 2)), 0x088AF53Cu, f_0882196C);
    }
    ARG(0) = 1; ARG(1) = pal; ARG(2) = 0; ARG(3) = 0x20; c->r[8] = 8;
    c->r[31] = 0x088AF554u; f_0880AEA8(c);
    *W(o + 96) = *W(o + 96) + 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
