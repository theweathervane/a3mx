/* Small functions the scenarios reach, 61 - 62 instructions: a fighter's picture table and list-0xA
   helper, a projectile's start, a fighter's step dispatch by its two flags, two screens' seven
   palettes loaded. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c);
void f_08940A94(Cpu *c); void f_0880AEA8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A66B90, 0x0886EB48 (0xA,
   0x08A66C2C), +86 = its side, +164 = 1 and a list-0xA helper (0x08822F54) of its character linked
   both ways (its +88, the fighter's +40) with its place, +11 / +14 / character / side / +164, +78 =
   -1. */
SFA_FUNC(0891F6E8, fighter_table_copy_a) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0xFFFFFFFFu;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 24) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A66B90u + 4 * n);
    ARG(0) = f; ARG(1) = 0xA; ARG(2) = 0x08A66C2Cu;
    c->r[31] = 0x0891F748u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    ARG(0) = 0xA; ARG(1) = *B(f + 338);
    c->r[31] = 0x0891F770u; f_08822F54(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *W(f + 40) = n;
        *W(n + 16) = *W(f + 16);
        *B(n + 78) = 0xFF;
        *W(n + 20) = *W(f + 20);
        *B(n + 11) = *B(f + 11);
        *B(n + 14) = *B(f + 14);
        *B(n + 3) = (uint8_t)S8(f + 338);
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (projectile): on (+4, +9 / +73 / y step cleared), +352 / +356 = 0x08A734E4 / 0x08A73564, its
   pictures 0x08A70F38 / 0x08A70D80 (+128 / +200, 0x08876998), its strength's (+346) speed
   (0x08A7460C) its way, 0x53 ahead and 0x39 up, +344 from 0x08A74618, 0x0887684C (+344) and
   0x08940A94. */
SFA_FUNC(089409A0, projectile_start_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 9) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 352) = 0x08A734E4u;
    *W(o + 356) = 0x08A73564u;
    *W(o + 128) = 0x08A70F38u;
    *W(o + 200) = 0x08A70D80u;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x089409F8u, f_08876998);
    int32_t k = S8(o + 346);
    *B(o + 73) = 0;
    *W(o + 100) = 0;
    uint32_t sp = *W(0x08A7460Cu + ((uint32_t)k << 2));
    if (*B(o + 11) != 0) { *W(o + 96) = sp; *H(o + 18) = (uint16_t)(S16(o + 18) + 0x53); }
    else { *W(o + 96) = (uint32_t)-(int32_t)sp; *H(o + 18) = (uint16_t)(S16(o + 18) - 0x53); }
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x39);
    *B(o + 344) = (uint8_t)S8(0x08A74618u + (uint32_t)k);
    c->r[6] = (uint32_t)k;
    ARG(0) = o; ARG(1) = (uint32_t)S8(o + 344) & 0xFFFF;
    c->r[31] = 0x08940A7Cu; f_0887684C(c);
    call_on(c, o, 0x08940A84u, f_08940A94);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its +7 step from the table picked by +192 (0, 1 or other) and +193: 0x08AB14DC /
   0x08AB14FC, 0x08AB1518 / 0x08AB1520, 0x08AB152C; +696 = +192 is 1. */
SFA_FUNC(089BF93C, fighter_step_by_flags_3) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t i = (uint32_t)*B(f + 7) << 2;
    int32_t a = S8(f + 192);
    *W(c->r[29] + 16) = ra;
    gaddr tab; uint32_t back;
    if (a == 0) {
        int b = S8(f + 193) != 0;
        *B(f + 696) = 0;
        tab = b ? 0x08AB14FCu : 0x08AB14DCu; back = b ? 0x089BF99Cu : 0x089BF978u;
    } else if (a == 1) {
        int b = S8(f + 193) != 0;
        c->r[7] = (uint32_t)S8(f + 193);
        *B(f + 696) = 1;
        tab = b ? 0x08AB1520u : 0x08AB1518u; back = b ? 0x089BFA00u : 0x089BF9DCu;
    } else {
        c->r[7] = (uint32_t)a;
        *B(f + 696) = 0;
        tab = 0x08AB152Cu; back = 0x089BFA24u;
    }
    c->r[6] = tab;
    uint32_t h = *W(tab + i);
    c->r[5] = h;
    c->r[31] = back; rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen): counted (+3), then seven palettes loaded (0x0880AEA8 (2, from, 0, slot, 1)). */
static void screen_palettes(Cpu *c, const uint32_t back[7]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
    c->r[16] = 0x08A35330u;
    *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    static const uint32_t from[7] = { 0x08A35390u, 0x08A36430u, 0x08A36AF0u, 0x08A36B70u, 0x08A36C90u, 0x08A35410u, 0x08A35490u };
    static const uint32_t slot[7] = { 1, 0, 0, 1, 0, 2, 3 };
    for (int k = 0; k < 7; k++) {
        if (k == 2) c->r[17] = 0x08A364D0u;
        ARG(0) = 2; ARG(1) = from[k]; ARG(2) = 0; ARG(3) = slot[k]; c->r[8] = 1;
        c->r[31] = back[k]; f_0880AEA8(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
SFA_FUNC(088B33A4, screen_palettes_1) {
    static const uint32_t back[7] = { 0x088B33E0u, 0x088B3400u, 0x088B3420u, 0x088B3438u, 0x088B3458u, 0x088B3470u, 0x088B3488u };
    screen_palettes(c, back);
}
SFA_FUNC(088B37C8, screen_palettes_2) {
    static const uint32_t back[7] = { 0x088B3804u, 0x088B3824u, 0x088B3844u, 0x088B385Cu, 0x088B387Cu, 0x088B3894u, 0x088B38ACu };
    screen_palettes(c, back);
}
