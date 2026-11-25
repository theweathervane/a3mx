/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 134 - 143
   instructions: the arcade route's ten opponents chosen, the VS screen's objects made, the fighters'
   select-slot pictures loaded. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u
#define KEPT   0x08A055DCu

void f_0882134C(Cpu *c); void f_0882EEE8(Cpu *c); void f_0882EF74(Cpu *c); void f_08822EAC(Cpu *c);
void f_08823338(Cpu *c); void f_088969BC(Cpu *c); void f_0889DB48(Cpu *c); void f_08812FCC(Cpu *c);
void f_08812FD4(Cpu *c); void f_08813538(Cpu *c); void f_088158A4(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* (-, route, fighter): the arcade route for the fighter: 0x0882134C (0x1C3 + route +33, side), then
   for stage i = 0 - 9 its kind (row +34 low nibble of 0x08A0ED68, 10 bytes) noted at 0x095179B0 + i and
   its opponent at 0x09517988 + i: stage 4 the fighter's rival (0x08A0E7D8 by character), 8 and 9 the
   bosses (0x08A0E784 / 0x08A0E7AE), the others a CPU pick (0x0882EF74's rows, else 0x0882EEE8 at
   random). After opponent 0x29 the next one is noted at GAME +192; on stages of kind 0xFF 4 becomes
   0x12 and 6 becomes 0x14. */
SFA_FUNC(0882ECD0, arcade_route_make) {
    gaddr s = ARG(1), f = ARG(2);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x60;
    gaddr sp = c->r[29];
    for (int k = 0; k < 9; k++) *W(sp + 44 + 4u * (uint32_t)k) = sv[k];
    *W(sp + 80) = ra;
    c->r[22] = s; c->r[19] = f;
    *B(s + 48) = (uint8_t)S8(f + 337);
    call2(c, ((uint32_t)(int32_t)S8(s + 33) + 0x1C3) & 0xFFFF, (uint32_t)(int32_t)S8(f + 337), 0x0882ED1Cu, f_0882134C);
    const gaddr kinds = 0x08A0ED68u, noted = 0x095179B0u, taken = 0x09517988u;
    *W(sp + 40) = kinds; *W(sp + 36) = noted; *W(sp + 32) = 0x08A0E7D8u; *W(sp + 28) = taken;
    *W(sp + 24) = 0x08A0E784u; *W(sp + 20) = 0x08A0E7AEu; *W(sp + 16) = GAME;
    c->r[21] = 4; c->r[20] = 0xFF; c->r[30] = 6; c->r[23] = 0x14;
    for (uint32_t i = 0; (int32_t)i < 10; i = (i + 1) & 0xFF) {
        c->r[18] = i;
        uint32_t g = (uint32_t)S8(s + 34) & 0xF;
        uint32_t kind = *B(kinds + ((10u * g) & 0xFF) + i);
        gaddr out = taken + i;
        *B(noted + i) = (uint8_t)kind;
        c->r[16] = noted + i; c->r[17] = out;
        uint32_t ch = (uint32_t)(int32_t)S8(f + 338);
        if (i == 4) *B(out) = *B(0x08A0E7D8u + ch);
        else if (i == 8) *B(out) = *B(0x08A0E784u + ch);
        else if (i == 9) *B(out) = *B(0x08A0E7AEu + ch);
        else {
            call2(c, f, i, 0x0882EE34u, f_0882EF74);
            if (c->r[2] == 0xFF) call2(c, f, i, 0x0882EE4Cu, f_0882EEE8);
            *B(out) = (uint8_t)c->r[2];
        }
        kind = *B(noted + i);
        c->r[16] = kind;
        if ((int32_t)i > 0 && *B(out - 1) == 0x29) *B(GAME + 192) = *B(out);
        if (kind == 0xFF) {
            if (*B(out) == 4) *B(out) = 0x12;
            if (*B(out) == 6) *B(out) = 0x14;
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[21] = sv[5]; c->r[22] = sv[6]; c->r[23] = sv[7]; c->r[30] = sv[8]; c->r[31] = ra;
    c->r[29] += 0x60;
}

/* The VS screen's objects: 0x092E3E98 = 0x08A29768; the fighter whose turn it is (BATTLE +64: 1
   player 2, 2 / 3 the partners, else BATTLE +112's or the human side's); from 7 players three list-0x12
   objects (types from 0x08A2AD10) pointing at it; three list-1 objects placed (type 1 at x 0x20; two
   type 0 at x 0x110 / -0x10 moving left / right); list-0x12 type 3 (unless GAME +69) and, with
   BATTLE +61, type 2 pointing at the fighter, else type 6; then 0x0889DB48 (its character, colour) and
   the stage's palettes (0x68D + 4 x BATTLE +102, slot 0x1B, 4). */
SFA_FUNC(0889DBE4, vs_objects_make) {
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[2];
    *W(0x092E3E98u) = 0x08A29768u;
    *W(sp + 20) = sv[1]; *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    *W(sp + 40) = ra;
    c->r[18] = BATTLE; c->r[17] = GAME; c->r[16] = 1;
    int32_t k = S8(BATTLE + 64);
    gaddr f;
    if (k == 1) f = P2;
    else if (k == 2) f = 0x092E47D0u;
    else if (k == 3) f = 0x092E4C68u;
    else { f = *W(BATTLE + 112); if (f == 0) f = (S8(GAME + 53) & 1) ? P1 : P2; }
    c->r[19] = f;
    if (!(S8(0x095176F3u) < 7)) {
        c->r[20] = 0x08A2AD10u;
        for (c->r[21] = 3; c->r[21] != 0; c->r[21]--) {
            uint32_t type = *B(c->r[20]);
            c->r[20]++;
            call2(c, 0x12, type, 0x0889DCC8u, f_08822EAC);
            *B(c->r[2]) = 1;
            *W(c->r[2] + 88) = f;
        }
    }
    call2(c, 1, 1, 0x0889DCE4u, f_08823338);
    if (c->r[2] != 0) { gaddr n = c->r[2]; *W(n + 8) = 0x200000; *W(n + 108) = 0; *H(n + 26) = 0x20; }
    int made = 0;
    if (S8(GAME + 69) == 0) {
        call2(c, 0x12, 3, 0x0889DD18u, f_08822EAC);
        *B(c->r[2]) = 1;
        if (S8(BATTLE + 61) != 0) {
            call2(c, 0x12, 2, 0x0889DD34u, f_08822EAC);
            *B(c->r[2]) = 1;
            *W(c->r[2] + 88) = f;
            made = 1;
        }
    }
    if (!made) { call2(c, 0x12, 6, 0x0889DD48u, f_08822EAC); *B(c->r[2]) = 1; }
    call2(c, 1, 0, 0x0889DD58u, f_08823338);
    if (c->r[2] != 0) {
        gaddr n = c->r[2];
        *W(n + 8) = 0x1100000; *W(n + 108) = 0xFFFD0000u; *W(n + 116) = 0xF00000; *H(n + 26) = 0x38;
    }
    call2(c, 1, 0, 0x0889DD8Cu, f_08823338);
    if (c->r[2] != 0) {
        gaddr n = c->r[2];
        *W(n + 8) = 0xFFF00000u; *W(n + 108) = 0x30000; *W(n + 116) = 0x180000; *H(n + 26) = 0x30;
    }
    call2(c, (uint32_t)(int32_t)S8(f + 338), (uint32_t)(int32_t)S8(f + 376), 0x0889DDC0u, f_0889DB48);
    call3(c, ((uint32_t)(int32_t)S16(BATTLE + 102) << 2) + 0x68D, 0x1B, 4, 0x0889DDD8u, f_088969BC);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* The select slot by 0x088158A4's answer bits: 0 with none in 0xF0, 1 with none in 0xF00, 2 with none
   in 0xF000, else as it was. */
static uint32_t slot_by(uint32_t r, uint32_t was) {
    if ((r & 0xF0) == 0) return 0;
    if ((r & 0xF00) == 0) return 1;
    if ((r & 0xF000) == 0) return 2;
    return was;
}

/* One fighter's select pictures (+416's +192 / +196 = a slot's buffer and 0x208 past it). */
static void fighter_slot(gaddr f, uint32_t slot) {
    uint32_t v = *W(KEPT + 68 + (slot << 2));
    *W(*W(f + 416) + 192) = v;
    *W(*W(f + 416) + 196) = v + 0x208;
}

/* The two fighters' select-slot pictures: the characters at 0x08E75C48 copied to 0x092E89DC; for each
   fighter (0x088158A4's bit 0 for player 1, bit 1 for player 2) the slot already used (0x09358410
   +1984 / +1988), or a free one found and loaded (0x08812FD4's character, 0x08813538); then +86 = the
   fighter's side and 0x08812FCC on both. */
SFA_FUNC(08815D60, fighters_select_slots) {
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4];
    const gaddr chars = 0x092E89DCu;
    *H(chars) = (uint16_t)S16(0x08E75C48u);
    *H(chars + 2) = (uint16_t)S16(0x08E75C48u + 2);
    *H(chars + 4) = (uint16_t)S16(0x08E75C48u + 4);
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 36) = ra;
    c->r[20] = chars;
    call_on(c, chars, 0x08815DACu, f_088158A4);
    uint32_t r = c->r[2];
    c->r[18] = P1; c->r[17] = KEPT; c->r[16] = P2;
    if (r & 1) {
        uint32_t used = *W(0x09358410u + 1984);
        *W(*W(P1 + 416) + 192) = *W(KEPT + 68 + (used << 2));
        *W(*W(P1 + 416) + 196) = *W(KEPT + 68 + (*W(0x09358410u + 1984) << 2)) + 0x208;
    } else {
        c->r[19] = slot_by(r, c->r[19]);
        call_on(c, P1, 0x08815E08u, f_08812FD4);
        call3(c, 0, c->r[2] & 0xFFFF, c->r[19], 0x08815E18u, f_08813538);
        fighter_slot(P1, c->r[19]);
    }
    call_on(c, chars, 0x08815E80u, f_088158A4);
    c->r[20] = c->r[2];
    if (c->r[20] & 2) {
        uint32_t used = *W(0x09358410u + 1988);
        *W(*W(P2 + 416) + 192) = *W(KEPT + 68 + (used << 2));
        *W(*W(P2 + 416) + 196) = *W(KEPT + 68 + (*W(0x09358410u + 1988) << 2)) + 0x208;
    } else {
        c->r[19] = slot_by(c->r[20], c->r[19]);
        call_on(c, P2, 0x08815EC8u, f_08812FD4);
        call3(c, 1, c->r[2] & 0xFFFF, c->r[19], 0x08815ED8u, f_08813538);
        fighter_slot(P2, c->r[19]);
    }
    uint32_t a = (uint32_t)(int32_t)S8(P1 + 337);
    c->r[19] = (uint32_t)(int32_t)S8(P2 + 337);
    *H(P1 + 86) = (uint16_t)a;
    *H(P2 + 86) = (uint16_t)c->r[19];
    call_on(c, P1, 0x08815F74u, f_08812FCC);
    call_on(c, P2, 0x08815F7Cu, f_08812FCC);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[31] = ra;
    c->r[29] += 0x30;
}
