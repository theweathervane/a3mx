/* Small functions the scenarios reach, 46 instructions: an effect beside its owner during its move,
   two list-8 effects either side, a random launch, fighter objects' starts, a fighter's picture table
   and list-0x10 helper, an effect's two-stage end. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_08821424(Cpu *c); void f_088219B0(Cpu *c); void f_08822EAC(Cpu *c); void f_08821368(Cpu *c);
void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08822F54(Cpu *c); void f_0886EB48(Cpu *c);
void f_0882196C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): unless halted (BATTLE +37), while its owner (+88) is in its step 1 / 0 / 8, 0x088219B0
   and 0x20 to the owner's side (+78 picks which); once the owner leaves it, on (+4, +5 cleared) and
   not drawn this frame; otherwise drawn. */
SFA_FUNC(088F1C48, effect_beside_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int draw = 1;
    if (S8(BATTLE + 37) == 0) {
        if (*B(p + 4) == 1 && *B(p + 5) == 0 && *B(p + 6) == 8) {
            call_on(c, o, 0x088F1CA4u, f_088219B0);
            int32_t x = S16(p + 18);
            c->r[17] = (uint32_t)x;
            *H(o + 18) = (uint16_t)(S8(o + 78) != 0 ? x - 0x20 : x + 0x20);
        } else {
            *B(o + 5) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            draw = 0;
        }
    }
    if (draw) call_on(c, o, 0x088F1CECu, f_08821424);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): two list-8 effects (0x08822EAC) 0x20 either side of it (+11 / +78 = 0 / 1), +12 / +13
   cleared, its +14 and y, pointing back at it (+88). */
SFA_FUNC(088F1DBC, effects_make_two_8) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    for (int side = 0; side < 2; side++) {
        call2(c, 8, 0, side ? 0x088F1E20u : 0x088F1DD8u, f_08822EAC);
        gaddr n = c->r[2];
        if (n == 0) break;
        *B(n + 12) = 0; *B(n + 13) = 0;
        *B(n + 14) = *B(o + 14);
        *B(n + 11) = (uint8_t)side;
        *H(n + 18) = (uint16_t)(S16(o + 18) + (side ? -0x20 : 0x20));
        *B(n + 78) = (uint8_t)side;
        *H(n + 22) = (uint16_t)S16(o + 22);
        *W(n + 88) = o;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): one of 8 launches at random (0x08B03F9C = the pick; 16-byte rows of 0x08A5AFC8: x / y
   steps and their changes), x ones turned (as ~) for +11 set. */
SFA_FUNC(089133E8, object_random_launch) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    c->r[31] = 0x089133FCu; f_08821368(c);
    *H(0x08B03F9Cu) = (uint16_t)(c->r[2] & 7);
    gaddr row = 0x08A5AFC8u + ((uint32_t)(int32_t)S16(0x08B03F9Cu) << 4);
    *W(o + 96) = *W(row);
    *W(o + 104) = *W(row + 4);
    *W(o + 100) = *W(row + 8);
    *W(o + 108) = *W(row + 12);
    if (*B(o + 11) != 0) {
        *W(o + 96) = ~*W(o + 96);
        *W(o + 104) = ~*W(o + 104);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (object): with few effects (none, or under two with +251) and 0x0886BF84 allowing it, on (+4 = 1, +6 =
   7, +235 = 1, +236 = 1; +11 = +697) and 0x0886DB94; 1 then, else 0. */
static void start_few2(Cpu *c, uint32_t back0, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int ok = 0;
    if (S8(o + 251) != 0) {
        if ((int32_t)n < 2) { c->r[31] = back1; f_0886BF84(c); ok = c->r[2] != 0; }
    } else if (n == 0) { c->r[31] = back0; f_0886BF84(c); ok = c->r[2] != 0; }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        c->r[17] = 1;
        *B(o + 236) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 235) = 1;
        call_on(c, o, back2, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08926198, object_start_few_1c) { start_few2(c, 0x089261C8u, 0x08926230u, 0x08926204u); }
SFA_FUNC(08962F28, object_start_few_1d) { start_few2(c, 0x08962F58u, 0x08962FC0u, 0x08962F94u); }

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A763EC, 0x0886EB48 (0x10,
   0x08A76488), +164 = 1 and a list-0x10 helper (0x08822F54: its side, +164, +138 = 1) linked both ways
   (its +88, the fighter's +40). */
SFA_FUNC(08942764, fighter_table_copy_10) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A763ECu + 4 * n);
    ARG(0) = f; ARG(1) = 0x10; ARG(2) = 0x08A76488u;
    c->r[31] = 0x089427C4u; f_0886EB48(c);
    *W(f + 164) = 1;
    call2(c, 0x10, 0, 0x089427D8u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = f;
        *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(n + 164) = *W(f + 164);
        *B(n + 138) = 1;
        *W(f + 40) = n;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): when +75 goes negative: on its first time (+7 = 1) with +73 set 0x10 lower and the
   animation 0x08A9B2BC's word, else (and the second time) on (+4); until then 0x088219B0 and drawn. */
SFA_FUNC(089A2338, effect_two_stage_end) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    uint32_t st = *B(o + 7);
    int neg = S8(o + 75) < 0;
    if (!neg) {
        call_on(c, o, st != 0 ? 0x089A23D8u : 0x089A23A8u, f_088219B0);
        call_on(c, o, st != 0 ? 0x089A23E0u : 0x089A23B0u, f_08821424);
    } else if (st != 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        *B(o + 7) = 1;
        if (S8(o + 73) != 0) {
            *H(o + 22) = (uint16_t)(S16(o + 22) - 0x10);
            call2(c, o, *W(0x08A9B2B0u + 12), 0x089A2398u, f_0882196C);
        } else *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
