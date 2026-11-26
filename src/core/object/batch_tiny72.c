/* Small functions the scenarios reach, 76 - 78 instructions: a projectile's start, a stage
   object's palette rows, a screen's all-items-pressed check. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define PALS 0x092D9E00u

void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_089330A8(Cpu *c); void f_08933350(Cpu *c);
void f_0880AEA8(Cpu *c); void f_08823338(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (projectile): on (+4 = 1, +9 / +73 / y step cleared), its owner's (+88) +24 / +86 / +164, pictures
   0x08A6D0DC / 0x08A6C81C (+128 / +200, 0x08876998), 0x08933350 for +3, its strength's (+346) speed
   (0x08A6DCFC) its way, 0x59 ahead and 0x36 up, +139 / +344 from 0x08A6D974 / 0x08A6DCF4, +352 /
   +356 = 0x08A6C720 / 0x08A6C75C, 0x0887684C (strength) and 0x089330A8. */
SFA_FUNC(08932F78, projectile_start_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = 1;
    gaddr p = *W(o + 88);
    *B(o + 9) = 0;
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 128) = 0x08A6D0DCu;
    *W(o + 200) = 0x08A6C81Cu;
    *W(c->r[29] + 20) = ra;
    c->r[5] = 0x08A6D0DCu; c->r[6] = 0x08A70000u;
    call_on(c, o, 0x08932FD0u, f_08876998);
    if (*B(o + 3) != 0) call_on(c, o, 0x08932FE4u, f_08933350);
    int32_t k = S8(o + 346);
    *W(o + 100) = 0;
    *B(o + 73) = 0;
    *W(o + 96) = *W(0x08A6DCFCu + ((uint32_t)k << 2));
    int32_t dx = 0x59;
    if (*B(o + 11) == 0) { dx = -0x59; *W(o + 96) = (uint32_t)-(int32_t)*W(o + 96); }
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x36);
    *B(o + 139) = (uint8_t)S8(0x08A6D974u + (uint32_t)k);
    *B(o + 344) = (uint8_t)S8(0x08A6DCF4u + (uint32_t)k);
    *W(o + 352) = 0x08A6C720u;
    *W(o + 356) = 0x08A6C75Cu;
    call2(c, o, (uint32_t)k & 0xFFFF, 0x08933090u, f_0887684C);
    call_on(c, o, 0x08933098u, f_089330A8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (stage object): +126 = 0x1C; with a palette set for its kind (+3, 0x08A24528) +135 = 1, +134 = 2,
   +132 = its row from 0x08A2450E (sets 0 - 7 by +78 / +73 via 0x08A24540, odd ones +135 = -1) loaded
   from PALS +68 into line 0x3F; kinds 7 / 8 also +114 = -1 and +112 = their second row into line
   0x14. */
SFA_FUNC(0887EDD8, stage_object_palettes) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    uint32_t kind = *B(o + 3);
    *B(o + 126) = 0x1C;
    int32_t v = S8(0x08A24528u + kind);
    *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = s2; *W(c->r[29] + 44) = ra;
    if (v >= 0) {
        c->r[18] = PALS;
        c->r[17] = 0x08A2450Eu;
        *B(o + 135) = 1;
        uint32_t idx = 2u * (uint32_t)v;
        if (v < 8) {
            int32_t w = 2 * S8(o + 73) + S8(0x08A24540u + idx + (uint32_t)S8(o + 78));
            if (w & 1) *B(o + 135) = 0xFF;
            idx = 2u * (uint32_t)w;
        }
        *B(o + 134) = 2;
        *H(o + 132) = *H(0x08A2450Eu + idx);
        ARG(0) = 0; ARG(1) = *W(PALS + 68) + ((uint32_t)S16(o + 132) << 5); ARG(2) = 0; ARG(3) = 0x3F; c->r[8] = 1;
        c->r[31] = 0x0887EEA4u; f_0880AEA8(c);
        uint32_t k = *B(o + 3);
        if (k == 7 || k == 8) {
            *H(o + 114) = 0xFFFF;
            *H(o + 112) = *H(0x08A2450Eu + 2 * k + 8);
            ARG(0) = 0; ARG(1) = *W(PALS + 68) + ((uint32_t)S16(o + 112) << 5); ARG(2) = 0; ARG(3) = 0x14; c->r[8] = 1;
            c->r[31] = 0x0887EEF8u; f_0880AEA8(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (screen): when all four items are pressed (GAME +10's high byte 0xF) it moves on (+3), GAME +22's
   low nibble set and three palettes loaded (0x08A35330 +0 / +0x1E0 / +0x260 into 1 / 4 / 8); else
   the fourth item's sound (8) clears GAME +10's low byte, a full GAME +22 high byte counts GAME +20
   on, and at 4 its +8 - +10 = 1 with sound 0x1D. */
SFA_FUNC(088B349C, screen_all_pressed) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 44) = sv[3];
    c->r[19] = GAME;
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    uint32_t h = *H(GAME + 10);
    *W(sp + 40) = sv[2];
    c->r[18] = 0xF00;
    *W(sp + 36) = sv[1]; *W(sp + 48) = ra;
    c->r[17] = 4;
    if ((h & 0xFF00) == 0xF00) {
        *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
        *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
        *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 0xF);
        c->r[16] = 0x08A35330u;
        static const uint32_t off[3] = { 0, 0x1E0, 0x260 }, slot[3] = { 1, 4, 8 };
        static const uint32_t back[3] = { 0x088B3518u, 0x088B3530u, 0x088B3548u };
        for (int i = 0; i < 3; i++) {
            ARG(0) = 2; ARG(1) = 0x08A35330u + off[i]; ARG(2) = 0; ARG(3) = slot[i]; c->r[8] = 1;
            c->r[31] = back[i]; f_0880AEA8(c);
        }
    } else {
        if ((h & 0xFF) == 0xF) {
            call2(c, 8, 0, 0x088B3568u, f_08823338);
            *H(GAME + 10) = (uint16_t)(*H(GAME + 10) & 0xFF00);
        }
        int32_t v = S16(GAME + 22);
        if (((uint32_t)v & 0xFF00) == 0xF00) {
            *H(GAME + 20) = (uint16_t)(S16(GAME + 20) + 1);
            *H(GAME + 22) = (uint16_t)(v & 0xFF);
        }
        if (S16(GAME + 20) == 4) {
            *B(o + 10) = 1; *B(o + 9) = 1; *B(o + 8) = 1;
            call2(c, 0x1D, 0, 0x088B35B8u, f_08823338);
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
