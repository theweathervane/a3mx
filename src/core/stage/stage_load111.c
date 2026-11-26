/* The fight's stage load (308 instructions): the old stage textures freed, the stage number picked
   (hidden-boss and special-character cases), its buffers handed over, its music and palette lines
   faded in. Offsets stay numbers until something shows what they are.
 */
#include "../object/step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define PICS   0x08E75AD8u
#define TILES  0x092D9D40u
#define PALS   0x092D9E00u

void f_088054F0(Cpu *c); void f_08808424(Cpu *c); void f_08812F90(Cpu *c); void f_088D7EA0(Cpu *c);
void f_089E0C24(Cpu *c); void f_089E1234(Cpu *c); void f_089F3430(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void sound_slot_clear(void) {
    gaddr e = 216u * *B(0x08E76084u) + 0x08E76088u;
    *B(e + 202) = 0; *B(e + 203) = 0;
    *H(e + 208) = 0;
    *B(e + 204) = 0;
}

/* Between 0x088D7EA0 (1) and (0): 10 bytes from 0x089FE020 to the stack (0x089F3430); the old stage's
   textures 0xA0 - 0xC3 (unless 0x08E75C50 = 0x8080), 0x90 - 0x93 (with 0x08E75A28) and 0xD0 - 0xDA (unless
   0x08E75C52 = 0x8080, then set) and 0x70 / 0x71 freed (0x089E0C24); the stage number (0x09358BE0) from
   BATTLE +50, with the hidden-boss flag (BATTLE +69) changing stage 10 / 0x24 cases, and stages 18 / 20 /
   21 / 23 changing a fighter's character (BATTLE +112 / +116 +338; 18 -> 4, 19 -> 0x11, 20 -> 6, 36 -> 2,
   37 -> 0x15); a changed fighter reloads (0x08812F90) and waits for 0x08E76068 +4; the kept buffers' +100
   handed to PALS / PICS / TILES, 0x08A055DC / +64 from 0x08A07918 / +12, 0x08A053D0 = the stage's music
   byte (0x08A051EC), 0x08A05074 = 1; sounds 8 and the stage's (0x120 + stage, 0x08808424); 0x08E75C50 =
   stage + 0x30; palette lines 0xA0 - 0xB6 faded (0x089E1234, 0x20 - 0x25); 0x08E75C48 - 4C = -1. */
SFA_FUNC(088162FC, stage_load) {
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 52) = sv[6]; *W(sp + 56) = sv[7];
    c->r[23] = 0; c->r[22] = 0xA;
    for (int i = 0; i < 6; i++) *W(sp + 28 + 4 * i) = sv[i];
    *W(sp + 60) = sv[8]; *W(sp + 64) = ra;
    ARG(0) = sp + 0x10; ARG(1) = 0x089FE020u; ARG(2) = 0xA;
    c->r[31] = 0x08816344u; f_089F3430(c);
    const gaddr stage = 0x09358BE0u;
    c->r[17] = stage; c->r[18] = BATTLE; c->r[30] = 1;
    call_on(c, 1, 0x08816360u, f_088D7EA0);
    c->r[19] = 0x08E75A18u; c->r[20] = 0x8080; c->r[21] = 0x08E70000u;
    if (*H(0x08E75C50u) != 0x8080)
        for (uint32_t i = 0; i < 36; i++) { c->r[16] = i; call_on(c, i + 0xA0, 0x08816388u, f_089E0C24); }
    if (*W(0x08E75A18u + 16) != 0) {
        for (uint32_t i = 0; i < 4; i++) { c->r[16] = i; call_on(c, i + 0x90, 0x088163ACu, f_089E0C24); }
        *W(0x08E75A18u + 16) = 0;
    }
    if (*H(0x08E75C52u) != 0x8080) {
        for (uint32_t i = 0; i < 11; i++) { c->r[16] = i; call_on(c, i + 0xD0, 0x088163D4u, f_089E0C24); }
        *H(0x08E75C52u) = 0x8080;
    }
    call_on(c, 0x70, 0x088163F0u, f_089E0C24);
    call_on(c, 0x71, 0x088163F8u, f_089E0C24);
    *H(stage) = (uint16_t)(int32_t)S8(BATTLE + 50);
    int changed = 0;
    gaddr who = 0;
    if (S8(BATTLE + 69) == 1) {
        int32_t k = S16(stage);
        if (k == 10) {
            who = *W(BATTLE + 116);
            int32_t ch = S8(who + 338);
            if (ch == 0 || ch == 0x17) *B(BATTLE + 69) = 0;
        } else if (k == 0x24) {
            who = *W(BATTLE + 116);
            if (S8(who + 338) == 0x17) {
                *H(stage) = 10;
                who = *W(BATTLE + 112);
                changed = 1;
                *B(who + 338) = 10;
                *H(stage) = 10;
                *B(BATTLE + 69) = 0;
            }
        }
    }
    switch (S16(stage)) {
    case 18: *H(stage) = 4; who = *W(BATTLE + 112); changed = 1; *B(who + 338) = 4; *B(who + 386) = 0; break;
    case 19: *H(stage) = 0x11; break;
    case 20: *H(stage) = 6; who = *W(BATTLE + 112); changed = 1; *B(who + 338) = 6; *B(who + 386) = 0; break;
    case 21: who = *W(BATTLE + 112); *B(who + 338) = 0x25; changed = 1; break;
    case 23: who = *W(BATTLE + 116); changed = 1; *B(who + 338) = 0xA; break;
    case 36: *H(stage) = 2; break;
    case 37: *H(stage) = 0x15; break;
    default: break;
    }
    c->r[23] = (uint32_t)changed;
    if (changed) {
        call_on(c, who, 0x08816528u, f_08812F90);
        c->r[16] = 0x08E76068u;
        while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x08816544u, f_088054F0);
    }
    *W(PALS + 100) = *W(0x08A0577Cu + 100);
    *W(PICS + 100) = *W(0x08A0562Cu + 100);
    *W(TILES + 100) = *W(0x08A056D4u + 100);
    *W(0x08A055DCu) = *W(0x08A07918u);
    *W(0x08A055DCu + 64) = *W(0x08A07918u + 12);
    *W(0x08A053CCu + 4) = *B(0x08A051BCu + (uint32_t)S16(stage) + 48);
    c->r[16] = 0x08E70000u;
    *W(0x08A05070u + 4) = 1;
    c->r[18] = 0x08E76088u;
    sound_slot_clear();
    call_on(c, 8, 0x08816624u, f_08808424);
    sound_slot_clear();
    call_on(c, (uint32_t)S16(stage) + 0x120, 0x08816658u, f_08808424);
    *H(0x08E75C50u) = (uint16_t)(S16(stage) + 0x30);
    static const uint8_t line[23] = { 0xA6, 0xA7, 0xA8, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA9, 0xAA, 0xAB,
        0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6 };
    static const uint8_t fade[23] = { 0x20, 0x21, 0x21, 0x22, 0x23, 0x24, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25,
        0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25 };
    for (int i = 0; i < 23; i++) call2(c, line[i], fade[i], 0x08816674u + 12 * (uint32_t)i, f_089E1234);
    *H(0x08E75C48u + 4) = 0xFFFF;
    *H(0x08E75C48u + 2) = 0xFFFF;
    *H(0x08E75C48u) = 0xFFFF;
    call_on(c, 0, 0x0881679Cu, f_088D7EA0);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x50;
}
