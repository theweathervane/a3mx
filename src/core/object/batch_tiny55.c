/* Small functions the scenarios reach, 48 - 49 instructions: a falling effect's landing, two
   fighters' step dispatches by their two flags, a fighter object's start, an effect following its
   owner's step, a helper copied from its fighter, a fighter's button-strength pick. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0896F658(Cpu *c); void f_08876998(Cpu *c); void f_0887DD30(Cpu *c); void f_088768F0(Cpu *c);
void f_08821424(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08924210(Cpu *c);
void f_088215C4(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (effect): 0x0896F658, and once its y (+22) reaches the ground (+132) it lands there: next step
   (+6), steps cleared, +73 cleared, +0 = 2, +127 = 2, its pictures 0x08A8BC14 / 0x08A8BA68
   (+128 / +200), 0x08876998, +83 = -1 and 0x0887DD30 (5). Then 0x088768F0 and drawn. */
SFA_FUNC(0896FCD4, effect_fall_land) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call_on(c, o, 0x0896FCE8u, f_0896F658);
    uint16_t ground = *H(o + 132);
    if (!(ground < *H(o + 22))) {
        *H(o + 22) = ground;
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *W(o + 108) = 0; *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 96) = 0;
        *B(o + 73) = 0;
        *B(o) = 2;
        *B(o + 127) = 2;
        *W(o + 128) = 0x08A8BC14u;
        *W(o + 200) = 0x08A8BA68u;
        call_on(c, o, 0x0896FD58u, f_08876998);
        *B(o + 0x53) = 0xFF;
        ARG(1) = 5;
        call_on(c, o, 0x0896FD74u, f_0887DD30);
    }
    call_on(c, o, 0x0896FD7Cu, f_088768F0);
    call_on(c, o, 0x0896FD84u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its +7 step from one of four tables picked by +192 / +193 (+696 = +192 set). */
static void step_by_flags(Cpu *c, const uint32_t tab[4], const uint32_t back[4]) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t a = S8(f + 192), b = S8(f + 193);
    uint32_t i = (uint32_t)*B(f + 7) << 2;
    *W(c->r[29] + 16) = ra;
    int k = (a != 0 ? 2 : 0) + (b != 0 ? 1 : 0);
    *B(f + 696) = a != 0 ? 1 : 0;
    c->r[7] = a != 0 ? 1u : (uint32_t)a;
    c->r[6] = tab[k];
    uint32_t h = *W(tab[k] + i);
    c->r[5] = h;
    c->r[31] = back[k]; rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08972834, fighter_step_by_flags_1) {
    static const uint32_t tab[4] = { 0x08A8FF4Cu, 0x08A8FF58u, 0x08A8FF6Cu, 0x08A8FF88u };
    static const uint32_t back[4] = { 0x08972870u, 0x08972894u, 0x089728C4u, 0x089728E8u };
    step_by_flags(c, tab, back);
}
SFA_FUNC(089B86E8, fighter_step_by_flags_2) {
    static const uint32_t tab[4] = { 0x08AAA018u, 0x08AAA034u, 0x08AAA040u, 0x08AAA048u };
    static const uint32_t back[4] = { 0x089B8724u, 0x089B8748u, 0x089B8778u, 0x089B879Cu };
    step_by_flags(c, tab, back);
}

/* (object): with few effects (none, or under two with +251) and 0x0886BF84 allowing it, on (+4 = 1, +6 =
   7, +235 = 1, +236 = 7; +11 = +697, +196 cleared) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(0897C11C, object_start_few_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int ok = 0;
    if (S8(o + 251) != 0) {
        if ((int32_t)n < 2) { c->r[31] = 0x0897C1BCu; f_0886BF84(c); ok = c->r[2] != 0; }
    } else if (n == 0) { c->r[31] = 0x0897C14Cu; f_0886BF84(c); ok = c->r[2] != 0; }
    if (ok) {
        c->r[17] = 1;
        *B(o + 236) = 7;
        *B(o + 235) = 1;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)S8(o + 697);
        *B(o + 196) = 0;
        call_on(c, o, 0x0897C190u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): every +82 frames (+82 counted down, +32 counting up between) it takes +34 as +32 and,
   while its owner (+88) is still in the step it was made for (+139 / +140 / +141 = the owner's
   +4 / +5 / +6, +142 clear), its frame palette (0x08924210) and, with the owner's +1, drawn;
   otherwise off (+4 = 2). */
SFA_FUNC(0892414C, effect_follow_owner_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    int16_t h = (int16_t)(S16(o + 82) - 1);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *H(o + 82) = (uint16_t)h;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (h >= 0) *H(o + 32) = (uint16_t)(S16(o + 32) + 1);
    else {
        *H(o + 82) = 0;
        gaddr p = *W(o + 88);
        c->r[17] = p;
        *H(o + 32) = (uint16_t)S16(o + 34);
        if ((uint32_t)S8(o + 139) != *B(p + 4) || (uint32_t)S8(o + 140) != *B(p + 5) ||
            (uint32_t)S8(o + 141) != *B(p + 6) || S8(o + 142) != 0)
            *B(o + 4) = 2;
        else {
            call_on(c, o, 0x089241D0u, f_08924210);
            if (*B(p + 1) != 0) call_on(c, o, 0x089241E4u, f_088215C4);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, helper): the helper takes the fighter's +121 / +122 / +80 / +14 / side (+337 as +86) /
   +164 / +11 - +13 / +38 / place, +194 as +346, +255 / +347 / +112 cleared, pointing back at it
   (+88); +260 = 0xE with the fighter in step 10, else 0xC; +311 = 2, +312 = 8. */
SFA_FUNC(08963B64, helper_copy_fighter) {
    gaddr f = ARG(0), n = ARG(1);
    *B(n + 122) = (uint8_t)S8(f + 122);
    *B(n + 121) = (uint8_t)S8(f + 121);
    *B(n + 346) = (uint8_t)S8(f + 194);
    *B(n + 255) = 0;
    *B(n + 80) = (uint8_t)S8(f + 80);
    *B(n + 347) = 0;
    *B(n + 14) = *B(f + 14);
    *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(n + 164) = *W(f + 164);
    *B(n + 11) = *B(f + 11);
    *B(n + 12) = (uint8_t)S8(f + 12);
    *B(n + 13) = *B(f + 13);
    *H(n + 38) = (uint16_t)S16(f + 38);
    *H(n + 18) = (uint16_t)S16(f + 18);
    *H(n + 112) = 0;
    *H(n + 22) = (uint16_t)S16(f + 22);
    *W(n + 88) = f;
    *B(n + 260) = *B(f + 7) == 10 ? 0xE : 0xC;
    *B(n + 311) = 2; *B(n + 312) = 8;
    c->r[4] = 2; c->r[6] = 8;
}

/* (fighter, out): the strength it attacks with (5 / 6 / 7 into out; 7 also sets +282): a player's
   from the buttons held (+760: 0x10, 0x20, else 0x40), the CPU's from its distance (+508, under
   0x70) and +696 (under 0x50 for 6 with it set, else 5 or 7 by the distance's lowest bit); 1 with
   one picked, else 0. */
SFA_FUNC(08974620, fighter_pick_strength) {
    gaddr f = ARG(0), out = ARG(1);
    gaddr m = *W(f + 92);
    c->r[7] = m;
    uint32_t r = 0;
    if (S8(f + 373) == 0) {
        if (*H(f + 760) & 0x70) {
            *B(out) = 5;
            r = 1;
            if (!(*H(f + 760) & 0x10)) {
                *B(out) = 6;
                if (!(*H(f + 760) & 0x20)) { *B(out) = 7; *B(f + 282) = 1; }
            }
        }
    } else if (S8(m + 73) == 0 && S16(f + 508) < 112) {
        int32_t d = S16(f + 508);
        if (S8(m + 696) == 0) {
            *B(out) = 5;
            r = 1;
            if (S16(f + 508) & 1) { *B(out) = 7; *B(f + 282) = 1; }
        } else if (d < 80) { *B(out) = 6; r = 1; }
    }
    c->r[2] = r;
}
