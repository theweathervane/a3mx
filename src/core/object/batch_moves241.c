/* A bouncing special (0x089848E0): a jump that lands, bounces by its count and ends crouched; a held
   special that repeats on the button (0x08968E2C); and a special that throws one or two projectiles
   (0x089292B0). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08837098(Cpu *c); void f_08861848(Cpu *c); void f_08864740(Cpu *c);
void f_0886CA2C(Cpu *c); void f_0886DB2C(Cpu *c); void f_0887C614(Cpu *c); void f_08987440(Cpu *c);
void f_0886E3CC(Cpu *c); void f_08969098(Cpu *c); void f_088632C4(Cpu *c); void f_0896900C(Cpu *c);
void f_0886DC3C(Cpu *c); void f_08822D2C(Cpu *c); void f_089291EC(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter): step 0: step 1, +262 = 1, +696 cleared, the meter by 7, 0x08837098, +82 = strength (the
   bounces) and move strength + 0xE; step 1: until a frame flag step 2, in the air (x 5.0, y 8.0, fall
   -1.0), ticked; step 2: 0x08987440 and ticked while above the floor (+132) unless a frame flag, then
   one bounce used (step 3 and +262 cleared once out, else step 1), landed on the floor, cue 0x0887C614
   and ticked; step 3: crouched once ended, else 0x0886DB2C and ticked. */
SFA_FUNC(089848E0, bouncing_special) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1; *B(o + 696) = 0;
        call2(c, o, 7, f_0886CA2C, 0x08984918u);
        call1(c, o, f_08837098, 0x08984920u);
        int32_t s = S8(o + 194);
        *H(o + 82) = (uint16_t)s;
        call2(c, o, (uint32_t)(s + 0xE), f_088616EC, 0x0898493Cu);
    } else if (st == 1) {
        if (S8(o + 77) == 0) {
            *B(o + 7) = 2; *B(o + 73) = 1;
            *W(o + 96) = 0x50000; *W(o + 100) = 0x80000; *W(o + 104) = 0; *W(o + 108) = 0xFFFF0000u;
        }
        call1(c, o, f_08861848, 0x08984984u);
    } else if (st == 2) {
        if (S8(o + 77) == 0) {
            call1(c, o, f_08987440, 0x089849A8u);
            if (S16(o + 132) < S16(o + 22)) { call1(c, o, f_08861848, 0x089849F0u); goto out; }
        }
        int32_t t = S16(o + 82) - 1;
        *B(o + 7) = 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) { *B(o + 7) = 3; *B(o + 262) = 0; }
        *H(o + 22) = (uint16_t)S16(o + 132);
        *B(o + 73) = 0; *H(o + 20) = 0;
        call1(c, o, f_0887C614, 0x08984A28u);
        call1(c, o, f_08861848, 0x08984A30u);
    } else if (st == 3) {
        if (S8(o + 75) < 0) call1(c, o, f_08864740, 0x08984A54u);
        else { call1(c, o, f_0886DB2C, 0x08984A64u); call1(c, o, f_08861848, 0x08984A6Cu); }
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0x0886E3CC (8, 8), 0x08969098; ended by 0x0896900C while 0x092E1228 +13 / +14 or 0x08E75910
   +38 hold or 0x088632C4 says so. With no choice made +82 counts down and, on +760 bits 4..6, +824 counts
   presses (+832 = 1 at the fifth) and +82 = 4. Once it ends with +74 at 1 and +83 run out: +828 up, and
   with fewer than 3, +832 and +82 left again (0x0886DC3C, 0x08837098, +551 = 1, +832 / +824 cleared, +82
   = 4, +83 = 1 and move strength + 0x13), else 0x0896900C; ticked otherwise. */
SFA_FUNC(08968E2C, held_repeat_special) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    ARG(0) = o; ARG(1) = 8; ARG(2) = 8; c->r[31] = 0x08968E50u; f_0886E3CC(c);
    call1(c, o, f_08969098, 0x08968E58u);
    const gaddr g = 0x092E1228u;
    if (S8(g + 13) != 0 || S8(g + 14) != 0 || S8(0x08E75910u + 38) != 0) {
        call1(c, o, f_0896900C, 0x08968ED4u); goto out;
    }
    call1(c, o, f_088632C4, 0x08968E90u);
    if (c->r[2] != 0) { call1(c, o, f_0896900C, 0x08968EB4u); goto out; }
    gaddr q = o + 0x52;
    c->r[18] = q;
    if (S8(o + 373) == 0) {
        int32_t v = S8(q);
        *B(q) = (uint8_t)(v - 1);
        if (v != 0 && (*H(o + 760) & 0x70)) {
            uint32_t n = *W(o + 824) + 1;
            *W(o + 824) = n;
            if (n == 5) *W(o + 832) = 1;
            *B(q) = 4;
        }
    }
    if (S8(o + 75) < 0) {
        c->r[17] = 1;
        if (S8(o + 74) == 1) {
            int32_t h = S8(q + 1) - 1;
            *B(q + 1) = (uint8_t)h;
            if ((int8_t)h == 0) {
                uint32_t n = *W(o + 828) + 1;
                *W(o + 828) = n;
                if ((int32_t)n < 3 && *W(o + 832) != 0 && S8(q) != 0) {
                    call1(c, o, f_0886DC3C, 0x08968FB0u);
                    call1(c, o, f_08837098, 0x08968FB8u);
                    *B(o + 551) = 1; *W(o + 832) = 0; *W(o + 824) = 0;
                    *B(q) = 4; *B(q + 1) = 1;
                    call2(c, o, (uint32_t)(S8(o + 194) + 0x13) & 0xFFFF, f_088616EC, 0x08968FE4u);
                } else call1(c, o, f_0896900C, 0x08968FA0u);
                goto out;
            }
        }
    }
    call1(c, o, f_08861848, 0x08968FF4u);
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* the projectile's fields from its owner */
static void shot_from(gaddr p, gaddr o) {
    *B(p + 122) = *B(o + 122); *B(p + 121) = *B(o + 121); *B(p + 346) = *B(o + 194);
    *B(p + 255) = 0; *B(p + 80) = *B(o + 80); *B(p + 347) = 0;
    *B(p + 14) = *B(o + 14); *B(p + 11) = *B(o + 11); *B(p + 12) = *B(o + 12);
    *H(p + 86) = (uint16_t)S8(o + 337);
    *W(p + 164) = *W(o + 164); *H(p + 38) = *H(o + 38); *B(p + 13) = *B(o + 13);
    *H(p + 18) = *H(o + 18); *B(p + 139) = 0; *H(p + 22) = *H(o + 22); *H(p + 112) = 0;
    *W(p + 88) = o;
    *W(o + 228) = p;
    *B(o + 536) = (uint8_t)(*B(o + 536) + 1);
}
static gaddr spawn_shot(Cpu *c, gaddr o, uint32_t back) {
    ARG(0) = o; ARG(1) = 0xB; ARG(2) = 0; c->r[31] = back; f_08822D2C(c);
    return c->r[2];
}

/* (fighter): ticked; on frame flag 2 +7 on, +82 = 5 and a projectile (object 0xB) from it, 0x089291EC; with
   +251 and +286 at least +662 a second one carrying +662. */
SFA_FUNC(089292B0, special_two_shots) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x089292C4u);
    if (S8(o + 77) == 2) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *H(o + 82) = 5;
        gaddr p = spawn_shot(c, o, 0x089292F8u);
        if (p != 0) {
            shot_from(p, o);
            ARG(0) = o; ARG(1) = p; ARG(2) = 0; c->r[31] = 0x089293A4u; f_089291EC(c);
            if (S8(o + 251) != 0 && S16(o + 286) >= S16(o + 662)) {
                gaddr p2 = spawn_shot(c, o, 0x089293D0u);
                if (p2 != 0) {
                    shot_from(p2, o);
                    *B(p2 + 280) = (uint8_t)S16(o + 662);
                    ARG(0) = o; ARG(1) = p2; ARG(2) = 0; c->r[31] = 0x08929484u; f_089291EC(c);
                }
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
