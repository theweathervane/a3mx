/* Small functions the scenarios reach, 62 - 63 instructions: a projectile's start, an effect
   following its owner's level, a fighter's shadow-like copy set up, a fighter's picture table and
   list-0xE helper, an effect on its owner during its move. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0887684C(Cpu *c); void f_0891CCEC(Cpu *c); void f_0891CD58(Cpu *c); void f_0882196C(Cpu *c);
void f_088219B0(Cpu *c); void f_08876998(Cpu *c); void f_0887CA88(Cpu *c); void f_0887CAF4(Cpu *c);
void f_0891D41C(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c); void f_088215C4(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (projectile): on (+4, +9 / +3 / y step / +83 cleared), its owner's (+88) +24, pictures 0x08A612E0
   (+200), +352 / +356 = 0x08A60564 / 0x08A60594, +0x6C - +0x6F = 5 / 0 / 0xB / 0; speed (0x08A64520)
   by its strength (+346; +347 set: the fourth, +344 = 1, else +344 = 3) its way; 0x0887684C (2),
   0x0891CCEC and 0x0891CD58. */
SFA_FUNC(0891C5CC, projectile_start_3) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *H(o + 24) = *H(p + 24);
    *W(o + 200) = 0x08A612E0u;
    *W(o + 352) = 0x08A60564u;
    *W(o + 356) = 0x08A60594u;
    *W(o + 100) = 0;
    *B(o + 0x53) = 0;
    *B(o + 3) = 0;
    *B(o + 0x6C) = 5; *B(o + 0x6E) = 0xB; *B(o + 0x6D) = 0; *B(o + 0x6F) = 0;
    *W(c->r[29] + 20) = ra;
    uint32_t sp;
    if (S8(o + 347) != 0) { *B(o + 344) = 1; sp = *W(0x08A64520u + 0xC); }
    else { *B(o + 344) = 3; sp = *W(0x08A64520u + ((uint32_t)S8(o + 346) << 2)); }
    *W(o + 96) = *B(o + 11) != 0 ? sp : (uint32_t)-(int32_t)sp;
    call2(c, o, 2, 0x0891C6A4u, f_0887684C);
    call_on(c, o, 0x0891C6ACu, f_0891CCEC);
    call_on(c, o, 0x0891C6B4u, f_0891CD58);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): off (+1) and gone (+4 on) once its owner (+88) is no longer its character (+338 = +3);
   with a level (+296's +10, or +11 with +83 set) at the owner's place and side and on, a new one
   (+78) taking its animation from 0x08A8C90C, and 0x088219B0. No level clears +78. */
SFA_FUNC(08971E7C, effect_owner_level_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 1) = 0;
    *W(c->r[29] + 20) = ra;
    if ((uint32_t)S8(p + 338) != *B(o + 3)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t lv = S8(o + 0x53) != 0 ? *B(*W(p + 296) + 11) : *B(*W(p + 296) + 10);
        if (lv == 0) *B(o + 78) = 0;
        else {
            *B(o + 11) = *B(p + 11);
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 1) = 1;
            if ((uint32_t)S8(o + 78) != lv) {
                *B(o + 78) = (uint8_t)lv;
                call2(c, o, *W(0x08A8C90Cu + (lv << 2)), 0x08971F48u, f_0882196C);
            }
            call_on(c, o, 0x08971F50u, f_088219B0);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (copy): on (+4, +9 = 1), +112 = 0xFFF, +116 / +123 / +127 / +0x153 cleared, +0x152 = 0x20, +348 =
   its +13, +20 = 0xFF, +344 = -1, its owner's (+88) +24 (0x0891D41C), the owner's picture table's
   first and fifth words (+352 / +356), pictures 0x08A6256C / 0x08A619C8 (+128 / +200, 0x08876998),
   0x0887CA88 -> 0x0887CAF4, and +296 = +356's entry for its frame (+28's +8, 12 bytes each). */
SFA_FUNC(0891D080, fighter_copy_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1;
    *H(o + 112) = 0xFFF;
    *B(o + 123) = 0; *B(o + 116) = 0;
    *B(o + 0x153) = 0; *B(o + 127) = 0;
    *B(o + 0x152) = 0x20;
    *B(o + 348) = *B(o + 13);
    *H(o + 20) = 0xFF;
    *B(o + 344) = 0xFF;
    uint32_t h24 = *H(p + 24);
    *W(c->r[29] + 24) = ra;
    *H(o + 24) = (uint16_t)h24;
    call2(c, o, h24, 0x0891D0F8u, f_0891D41C);
    *W(o + 352) = *W(*W(p + 416));
    *W(o + 356) = *W(*W(p + 416) + 16);
    *W(o + 128) = 0x08A6256Cu;
    *W(o + 200) = 0x08A619C8u;
    call_on(c, o, 0x0891D130u, f_08876998);
    call_on(c, o, 0x0891D138u, f_0887CA88);
    call2(c, o, c->r[2], 0x0891D144u, f_0887CAF4);
    *W(o + 296) = *W(o + 356) + 12u * *H(*W(o + 28) + 8);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A7057C, 0x0886EB48 (0xE,
   0x08A70618), +86 = its side, +164 = 1 and a list-0xE helper (0x08822F54) of its character linked
   both ways (its +88, the fighter's +40) with its place, +11 - +14, character, side and +164. */
SFA_FUNC(089337D4, fighter_table_copy_e) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A7057Cu + 4 * n);
    ARG(0) = f; ARG(1) = 0xE; ARG(2) = 0x08A70618u;
    c->r[31] = 0x08933834u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    call2(c, 0xE, *B(f + 338), 0x0893385Cu, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *W(f + 40) = n;
        *W(n + 16) = *W(f + 16);
        *W(n + 20) = *W(f + 20);
        *B(n + 11) = *B(f + 11);
        *B(n + 14) = *B(f + 14);
        *B(n + 12) = (uint8_t)S8(f + 12);
        *B(n + 13) = *B(f + 13);
        *B(n + 3) = (uint8_t)S8(f + 338);
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): halted it is only drawn; while its owner (+88) is in step 1/0/8 (+76 not negative) for
   +82 frames it sits on the owner, 0x088219B0, +9 = +77 / 2, drawn; otherwise gone (+4 on). */
SFA_FUNC(089B3AF8, effect_on_owner_timed) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) != 0) {
        *B(o + 9) = (uint8_t)(S8(o + 77) >> 1);
        call_on(c, o, 0x089B3BE0u, f_088215C4);
    } else {
        gaddr p = *W(o + 88);
        c->r[17] = p;
        int on = 0;
        if (S8(p + 76) >= 0 && *B(p + 4) == 1 && *B(p + 5) == 0 && *B(p + 6) == 8) {
            int8_t t = (int8_t)(S8(o + 0x52) - 1);
            *B(o + 0x52) = (uint8_t)t;
            on = t >= 0;
        }
        if (!on) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        else {
            call_on(c, o, 0x089B3B98u, f_088219B0);
            *H(o + 18) = (uint16_t)S16(p + 18);
            *H(o + 22) = (uint16_t)S16(p + 22);
            *B(o + 9) = (uint8_t)(S8(o + 77) >> 1);
            call_on(c, o, 0x089B3BBCu, f_088215C4);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
