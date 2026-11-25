/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in): textures
   freed when a screen is left (the fighters' picture lists, the select screen's portraits), a VS
   portrait's set-up, the CPU's script choices. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_089E157C(Cpu *c); void f_089E0C24(Cpu *c); void f_0887B6B8(Cpu *c); void f_0887B7CC(Cpu *c);
void f_0887B1A8(Cpu *c); void f_0889CFE8(Cpu *c); void f_0889CD8C(Cpu *c); void f_0889CEE4(Cpu *c);
void f_0889CDC8(Cpu *c);

#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- textures freed */
/* The texture sets at 0x08A054C0 and 0x08A05048 freed. */
SFA_FUNC(08813744, free_texture_sets_a) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20; *W(c->r[29] + 16) = ra;
    call_on(c, 0x08A054C0u, 0x08813758u, f_089E157C);
    call_on(c, 0x08A05048u, 0x08813764u, f_089E157C);
    c->r[31] = ra; c->r[29] += 0x20;
}
/* When 0x092E22E4 is set (cleared), the texture set at 0x09358408 freed; 0x092E22E0 cleared. */
SFA_FUNC(088AC618, free_texture_set_09358408) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20; *W(c->r[29] + 16) = ra;
    if (*W(0x092E22E4u) != 0) {
        *W(0x092E22E4u) = 0;
        call_on(c, 0x09358408u, 0x088AC640u, f_089E157C);
    }
    ARG(0) = 0x092E0000u;
    *W(0x092E22E0u) = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}
/* The textures 0x60 - 0x62 freed where the flags at 0x08A05604 / 08 / 0C say they are held, with the
   slot-30 entries of the picture lists (0x08E75AD8), tile sets (0x092D9D40) and palettes (0x092D9E00)
   and of 0x08E75A18 cleared. */
SFA_FUNC(0881369C, free_textures_60_62) {
    gaddr flags = 0x08A055DCu;
    WITH_S0(flags);
    static const uint32_t lists[3] = { 0x08E75AD8u + 120, 0x092D9D40u + 124, 0x092D9E00u + 128 };
    static const uint32_t backs[3] = { 0x088136D0u, 0x088136FCu, 0x08813728u };
    for (int k = 0; k < 3; k++) {
        if (*W(flags + 40 + 4 * (uint32_t)k) == 0) continue;
        *W(lists[k]) = 0;
        *W(flags + 40 + 4 * (uint32_t)k) = 0;
        call_on(c, 0x60u + (uint32_t)k, backs[k], f_089E0C24);
        *W(0x08E75A18u + 120 + 4 * (uint32_t)k) = 0;
    }
    END_S0();
}
/* The select screen's textures freed: 0xA0 - 0xC3 and 0x90 - 0x93, 0x08E75A28 cleared; and 0xD0 -
   0xDA unless 0x08E75C52 is already 0x8080; 0x08E75C50 / 52 = 0x8080. */
SFA_FUNC(08813770, free_select_textures) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[18] = 0x8080;
    *H(0x08E75C50u) = 0x8080;
    c->r[16] = 0x08E70000u;
    for (c->r[17] = 0; c->r[17] < 0x24; c->r[17]++) call_on(c, c->r[17] + 0xA0, 0x088137A0u, f_089E0C24);
    for (c->r[17] = 0; c->r[17] < 4; c->r[17]++) call_on(c, c->r[17] + 0x90, 0x088137BCu, f_089E0C24);
    *W(0x08E75A18u + 16) = 0;
    c->r[17] = 0;
    if (*H(0x08E75C52u) != 0x8080) {
        for (; c->r[17] < 0xB; c->r[17]++) call_on(c, c->r[17] + 0xD0, 0x088137ECu, f_089E0C24);
        *H(0x08E75C52u) = 0x8080;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- VS portraits */
/* (portrait, fighter): with the fighter's +424 bit 3, +4 on and +5 cleared; otherwise +6 on, +82 =
   0x600, +148 cleared, 0x0889CFE8 (portrait, 1), +13 = the fighter's side (+12 = -1 for side 1), its
   picture by GAME +20 (0x0889CD8C), and two parts by side (0x0889CEE4 with 0xC + side, 0x0889CDC8
   with 0x18 + side). */
SFA_FUNC(0889CAF8, vs_portrait_setup) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[17] = o; c->r[16] = f;
    if ((uint32_t)(int32_t)S16(f + 424) & 8) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *B(o + 5) = 0;
    } else {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 82) = 0x600;
        *W(o + 148) = 0;
        call2(c, o, 1, 0x0889CB58u, f_0889CFE8);
        int8_t side = S8(f + 337);
        *B(o + 13) = (uint8_t)side;
        if ((uint8_t)side != 0) *B(o + 12) = 0xFF;
        call2(c, o, (uint32_t)(int32_t)S16(GAME + 20) & 0xFF, 0x0889CB8Cu, f_0889CD8C);
        c->r[16] = (uint32_t)(int32_t)S8(f + 337);
        call2(c, o, (c->r[16] + 0xC) & 0xFFFF, 0x0889CBA0u, f_0889CEE4);
        call2(c, o, (c->r[16] + 0x18) & 0xFFFF, 0x0889CBB0u, f_0889CDC8);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- the CPU's script choices */
/* (fighter): as 0x0887ABC4 with the count +506 counted down first. */
SFA_FUNC(0887AE5C, cpu_script_count_near_508) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887AE70u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887AE80u, f_0887B7CC);
    else if (*B(f + 6) == 0) call_on(c, f, 0x0887AE98u, f_0887B1A8);
    else {
        int16_t v = S16(f + 506);
        if (v == 0) call_on(c, f, 0x0887AEB0u, f_0887B1A8);
        else {
            *H(f + 506) = (uint16_t)(v - 1);
            if (S8(f + 238) != 0) call_on(c, f, 0x0887AECCu, f_0887B1A8);
            else {
                int32_t a = S16(f + 490), b = S16(f + 508);
                if ((int32_t)(((uint32_t)(a - b) + 3) & 0xFFFF) < 6) call_on(c, f, 0x0887AEFCu, f_0887B1A8);
                else {
                    *B(f + 497) = 0;
                    if (!(a < b)) *B(f + 497) = (uint8_t)(S8(f + 497) + 1);
                }
            }
        }
    }
    END_S0();
}
