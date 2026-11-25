/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 162 - 216
   instructions: a fighter's palette by its kind, three fighters' select-slot pictures. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define KEPT   0x08A055DCu

void f_0880AEA8(Cpu *c); void f_08812FD4(Cpu *c); void f_08813538(Cpu *c); void f_088158A4(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t from, uint32_t slot, uint32_t back) {
    ARG(0) = 0; ARG(1) = from; ARG(2) = 0; ARG(3) = slot; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* (fighter, n): the fighter's palette n (1 up, 64 bytes each from +416's +128) by its palette kind
   (+593, 0 - 5) into its slot (+13) and for kinds 2 - 5 also slot +13 + 1: the first or (GAME +59
   bit 0) second 32 bytes, and for kinds 1, 2, 3 and 5 the second slot (kind 1 the only one) from 0x300
   further. */
SFA_FUNC(088702C8, fighter_palette_by_kind) {
    gaddr f = ARG(0);
    int32_t n = (int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = f;
    int32_t k = (int8_t)((n - 1) << 1);
    gaddr base = *W(*W(f + 416) + 128);
    uint32_t kind = (uint32_t)(int32_t)S8(f + 593);
    *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    if (kind < 6) {
        int alt = (S8(GAME + 59) & 1) != 0;
        gaddr p = base + ((uint32_t)k << 5);
        uint32_t half = alt ? 0x20 : 0;
        uint32_t slot = *B(f + 13);
        switch (kind) {
            case 0:
                palette(c, p + half, slot, alt ? 0x0887038Cu : 0x08870370u);
                break;
            case 1:
                c->r[17] = p;
                palette(c, p + 0x300 + half, slot, alt ? 0x088703CCu : 0x088703B0u);
                break;
            case 2: case 3:
                c->r[17] = p;
                palette(c, p + half, slot, alt ? 0x08870428u : 0x088703F0u);
                palette(c, p + 0x300 + half, *B(f + 13) + 1u, alt ? 0x08870444u : 0x0887040Cu);
                break;
            case 4:
                c->r[17] = p + half;
                palette(c, p + half, slot, alt ? 0x088704A8u : 0x08870468u);
                palette(c, p + half, *B(f + 13) + 1u, alt ? 0x088704C4u : 0x08870484u);
                break;
            case 5:
                c->r[17] = p;
                palette(c, p + half, slot, alt ? 0x08870520u : 0x088704E8u);
                palette(c, p + 0x300 + half, *B(f + 13) + 1u, alt ? 0x0887053Cu : 0x08870504u);
                break;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

static uint32_t slot_by(uint32_t r, uint32_t was) {
    if ((r & 0xF0) == 0) return 0;
    if ((r & 0xF00) == 0) return 1;
    if ((r & 0xF000) == 0) return 2;
    return was;
}

/* One fighter's select pictures: with its bit of 0x088158A4's answer the slot already used (its row
   of 0x08A05874 by the line-up and side, into 0x09358410 +1984), else a free one loaded (0x08813538 with
   the side and 0x08812FD4's character); +416's +192 / +196 = the slot's buffer and 0x208 past it. */
static void one_fighter(Cpu *c, gaddr f, uint32_t bit, const uint32_t backs[3]) {
    call_on(c, 0x092E89DCu, backs[0], f_088158A4);
    uint32_t r = c->r[2];
    if (r & bit) {
        gaddr q = 0x09358410u + ((uint32_t)(int32_t)S8(0x08A05874u + ((uint32_t)(int32_t)S16(BATTLE + 56) << 2) + (uint32_t)(int32_t)S8(f + 337)) << 2);
        *W(*W(f + 416) + 192) = *W(KEPT + 68 + (*W(q + 1984) << 2));
        *W(*W(f + 416) + 196) = *W(KEPT + 68 + (*W(q + 1984) << 2)) + 0x208;
    } else {
        c->r[20] = slot_by(r, c->r[20]);
        call_on(c, f, backs[1], f_08812FD4);
        call3(c, (uint32_t)(int32_t)S8(f + 337), c->r[2] & 0xFFFF, c->r[20], backs[2], f_08813538);
        uint32_t v = *W(KEPT + 68 + (c->r[20] << 2));
        *W(*W(f + 416) + 192) = v;
        *W(*W(f + 416) + 196) = v + 0x208;
    }
    if (bit == 4) c->r[19] = r;
}

/* (a, b, c): three fighters' select-slot pictures (bits 0 - 2 of 0x088158A4's answer), then +86 = each
   one's side. */
SFA_FUNC(08815F9C, fighters_select_slots_3) {
    gaddr fa = ARG(0), fb = ARG(1), fc = ARG(2);
    uint32_t ra = c->r[31];
    uint32_t sv[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3]; *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5]; *W(sp + 40) = ra;
    c->r[16] = fa; c->r[19] = 0x092E89DCu; c->r[18] = fc; c->r[17] = fb; c->r[21] = KEPT;
    static const uint32_t ba[3] = { 0x08815FD8u, 0x08816024u, 0x08816034u };
    static const uint32_t bb[3] = { 0x088160CCu, 0x08816114u, 0x08816124u };
    static const uint32_t bc[3] = { 0x088161B8u, 0x08816200u, 0x08816210u };
    one_fighter(c, fa, 1, ba);
    one_fighter(c, fb, 2, bb);
    one_fighter(c, fc, 4, bc);
    c->r[20] = (uint32_t)(int32_t)S8(fa + 337);
    *H(fa + 86) = (uint16_t)c->r[20];
    *H(fb + 86) = (uint16_t)(int32_t)S8(fb + 337);
    *H(fc + 86) = (uint16_t)(int32_t)S8(fc + 337);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4]; c->r[21] = sv[5];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
