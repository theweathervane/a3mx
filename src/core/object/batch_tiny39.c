/* Small functions the scenarios reach, 36 - 37 instructions: an effect on its fighter's box flag,
   fighter objects' starts, an effect's tables set, three fighters' picture tables copied, a range
   check against the other fighter. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088215C4(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c);
void f_0886EB48(Cpu *c); void f_089B78B0(Cpu *c); void f_089BE2B0(Cpu *c); void f_089C5DD8(Cpu *c);
void f_08821330(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect, fighter): at the fighter's place (+16 / +20, +11); its animation (0x08A9D968) follows the
   fighter's box +11 flag (kept at +114), else 0x088219B0; then drawn. */
SFA_FUNC(089A8B74, effect_box_flag_anim) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20);
    *B(o + 11) = *B(p + 11);
    int32_t v = S8(*W(p + 296) + 11);
    *W(c->r[29] + 20) = ra;
    if (v == S16(o + 114)) call_on(c, o, 0x089A8BECu, f_088219B0);
    else {
        *H(o + 114) = (uint16_t)v;
        call2(c, o, *W(0x08A9D968u + ((uint32_t)v << 2)), 0x089A8BDCu, f_0882196C);
    }
    call_on(c, o, 0x089A8BF4u, f_088215C4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 not negative and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1, +236 =
   k236; +11 = +697) and `then`; 1 then, else 0. */
static void start_386(Cpu *c, uint32_t k236, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) >= 0) {
        c->r[31] = back1; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = (uint8_t)k236;
            c->r[17] = 1;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            call_on(c, o, back2, then);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089B0CD4, object_start_386_2) { start_386(c, 2, f_0886DB94, 0x089B0CF8u, 0x089B0D34u); }
SFA_FUNC(089B0D64, object_start_386_3) { start_386(c, 3, f_0886DBE8, 0x089B0D88u, 0x089B0DC4u); }

/* (effect): +4 on, +9 cleared, +344 = -1, +24 its owner's, its tables (+128 = 0x08AA2D94, +200 =
   0x08AA2D50, +352 = 0x08AA2DF4, +356 = 0x08AA2E00), 0x08876998, +73 / +112 cleared, 0x0887684C (0). */
SFA_FUNC(089B2B38, effect_tables_set) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(*W(o + 88) + 24);
    *W(o + 352) = 0x08AA2DF4u;
    *W(o + 356) = 0x08AA2E00u;
    *W(o + 128) = 0x08AA2D94u;
    *W(o + 200) = 0x08AA2D50u;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x089B2BA4u, f_08876998);
    *B(o + 73) = 0;
    *H(o + 112) = 0;
    call2(c, o, 0, 0x089B2BB8u, f_0887684C);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 = 1, +12 = -1, its 39-word picture table (+416) from `src`, 0x0886EB48 (k, `tab`),
   +86 = its side (+337), +164 = 1, then `then`. */
static void table_copy(Cpu *c, gaddr src, uint32_t k, gaddr tab, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 1;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(src + 4 * n);
    ARG(0) = f; ARG(1) = k; ARG(2) = tab;
    c->r[31] = back1; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    ARG(1) = 1;
    call_on(c, f, back2, then);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089B7660, fighter_table_copy_26) { table_copy(c, 0x08AA7198u, 0x26, 0x08AA7234u, f_089B78B0, 0x089B76C0u, 0x089B76E0u); }
SFA_FUNC(089BDE60, fighter_table_copy_27) { table_copy(c, 0x08AAA268u, 0x27, 0x08AAA304u, f_089BE2B0, 0x089BDEC0u, 0x089BDEE0u); }
SFA_FUNC(089CA194, fighter_table_copy_29) { table_copy(c, 0x08AB5EE0u, 0x29, 0x08AB5F7Cu, f_089C5DD8, 0x089CA1F4u, 0x089CA214u); }

/* (fighter): 1 when the other fighter (0x08821330) is 0 - 511 behind it plus 0x40 (x difference; for
   kind +14 = 2 less the other's +18 - +10 times its +38 / 128). */
SFA_FUNC(088D3BEC, fighter_other_in_range) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = f;
    call_on(c, f, 0x088D3C00u, f_08821330);
    gaddr g = c->r[2];
    int32_t d = (int16_t)(S16(f + 18) - S16(g + 18) + 0x40);
    if (*B(f + 14) == 2 && S16(f + 38) != 0) {
        int32_t prod = (S16(g + 18) - S16(g + 10)) * S16(f + 38);
        int32_t q = (prod + (int32_t)((uint32_t)(prod >> 7) >> 25)) >> 7;
        d = (int16_t)(d - q);
    }
    c->r[2] = (d < 512 && !(d < 0)) ? 1 : 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
