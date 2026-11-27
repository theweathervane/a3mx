/* The hit loops: one fighter against another, and each live projectile against the four fighters (the
   object lists at 0x09517700 / 0x09517760, counts at 0x095177C0 / 0x095177C1, fighters at 0x08A17F88).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088741C8(Cpu *c); void f_08870918(Cpu *c); void f_088740E8(Cpu *c); void f_08873570(Cpu *c);

#define PAIRING 0x09358410u

/* (attacker, defender): the pairing set to both; the attacker's +195 cleared and, when it is live and
   neither +135 nor +234 hold it back, its hit tried on a hittable defender (0x088741C8, 0x08870918); the
   areas it touched (+195) kept in 0x08B0267E by both sides. A frame keeping s0, s1. */
SFA_FUNC(08870874, fighter_hits) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(PAIRING + 24) = a; *W(PAIRING + 16) = a; *W(PAIRING + 28) = b; *W(PAIRING + 20) = b;
    c->r[6] = *B(a);
    *W(c->r[29] + 20) = s1;
    *B(a + 195) = 0;
    c->r[17] = a;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = b;
    if (c->r[6] != 0 && S8(a + 135) == 0 && S8(a + 234) == 0) {
        ARG(0) = b; c->r[31] = 0x088708D0u; f_088741C8(c);
        if (c->r[2] != 0) { ARG(0) = a; ARG(1) = b; c->r[31] = 0x088708E0u; f_08870918(c); }
    }
    c->r[5] = *B(a + 195);
    c->r[6] = 0x08B0267Eu;
    *B(0x08B0267Eu + (uint32_t)(S8(a + 337) * 4 + S8(b + 337))) = (uint8_t)c->r[5];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): b's current hit box against a's current one (both by +296's +1): 1 when they overlap
   (0x08873570), 0 when b has none. */
SFA_FUNC(08874C14, hit_vs_hit) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t nb = *B(*W(b + 296) + 1);
    c->r[6] = nb; c->r[5] = a;
    *W(c->r[29] + 16) = ra;
    c->r[4] = b;
    uint32_t r = 0;
    if (nb != 0) {
        c->r[8] = b;
        uint32_t na = *B(*W(a + 296) + 1);
        gaddr ha = *W(a + 224) + (na << 5);
        gaddr hb = *W(b + 224) + (nb << 5);
        ARG(0) = a; ARG(1) = b; ARG(2) = ha; ARG(3) = hb; c->r[10] = ha;
        c->r[31] = 0x08874C68u; f_08873570(c);
        r = c->r[2] != 0;
    }
    c->r[2] = r;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The projectile loop over list / count: each live projectile (+0 = 1, no +234) against the fighters at
   0x08A17F88 on the given side (+121), hittable by 0x088741C8: the pairing set (projectile, its owner
   +88, the fighter), +195 cleared, its hit tried (0x08870918) and landed (0x088740E8). A 0x40 frame
   keeping s0..s7 and fp. */
static void projectile_hits(Cpu *c, gaddr list, gaddr count, int32_t side, const uint32_t back[3]) {
    uint32_t sv[8], fp = c->r[30], ra = c->r[31];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 44) = sv[7];
    uint32_t n = *B(count); c->r[23] = n;
    *W(sp + 48) = fp; c->r[30] = PAIRING;
    for (int i = 0; i < 7; i++) *W(sp + 16 + 4 * (uint32_t)i) = sv[i];
    *W(sp + 52) = ra;
    if (n != 0) {
        const gaddr fighters = 0x08A17F88u;
        c->r[22] = list; c->r[18] = fighters; c->r[20] = 1;
        for (;;) {
            gaddr x = *W(c->r[22]); c->r[21] = x;
            if (*B(x) == 1) {
                c->r[19] = 4;
                if (S8(x + 234) == 0) {
                    c->r[17] = fighters + 0x10;
                    for (;;) {
                        gaddr f = *W(c->r[17] - 4); c->r[16] = f;
                        int go = S8(f + 121) == side;
                        if (go) {
                            ARG(0) = f; c->r[31] = back[0]; f_088741C8(c);
                            go = c->r[2] != 0;
                        }
                        if (go) {
                            *W(PAIRING + 16) = x; *W(PAIRING + 24) = *W(x + 88);
                            *W(PAIRING + 28) = f; *W(PAIRING + 20) = f;
                            if (*B(*W(x + 296) + 1) != 0) {
                                *B(x + 195) = 0;
                                ARG(0) = x; ARG(1) = f; c->r[31] = back[1]; f_08870918(c);
                                if (c->r[2] != 0) { ARG(0) = x; ARG(1) = f; c->r[31] = back[2]; f_088740E8(c); }
                            }
                        }
                        c->r[19] -= 1;
                        int more = (int32_t)c->r[19] > 0;
                        c->r[17] -= 4;
                        if (!more) break;
                    }
                }
            }
            c->r[23] -= 1;
            int more = c->r[23] != 0;
            c->r[22] += 4;
            if (!more) break;
        }
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = fp; c->r[31] = ra; c->r[29] += 0x40;
}

/* The first projectile list against the fighters on side 1. */
SFA_FUNC(08873EA8, projectile_hits_1) {
    static const uint32_t b[3] = { 0x08873F34u, 0x08873F70u, 0x08873F80u };
    projectile_hits(c, 0x09517700u, 0x095177C0u, 1, b);
}

/* The second projectile list against the fighters on side 0. */
SFA_FUNC(08873FC8, projectile_hits_0) {
    static const uint32_t b[3] = { 0x08874054u, 0x08874090u, 0x088740A0u };
    projectile_hits(c, 0x09517760u, 0x095177C1u, 0, b);
}


void f_0887530C(Cpu *c);

/* (f0, f1, f2, f3): every fighter's hits against the others it targets (+263 bits 0..3 by fighter,
   re-read after each try), then the clashes by the side count at 0x092E125E: 0 f0 / f1; 1 f0 / f2 and
   f0 / f3; 2 f1 / f2 and f1 / f3; 3 f0 / f2 and f1 / f2 (each with the hit records +272). A 0x30 frame
   keeping s0..s3. */
SFA_FUNC(08874E6C, fighters_hits) {
    gaddr f[4] = { ARG(0), ARG(1), ARG(2), ARG(3) };
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = s3; *W(sp + 24) = s2; *W(sp + 20) = s1; *W(sp + 16) = s0; *W(sp + 32) = ra;
    c->r[19] = f[0]; c->r[18] = f[1]; c->r[17] = f[2]; c->r[16] = f[3];
    static const struct { int a, b, bit; uint32_t back; } tries[10] = {
        { 0, 1, 2, 0x08874EACu }, { 0, 2, 4, 0x08874EC8u }, { 0, 3, 8, 0x08874EE0u },
        { 1, 0, 1, 0x08874EFCu }, { 1, 2, 4, 0x08874F18u }, { 1, 3, 8, 0x08874F30u },
        { 2, 0, 1, 0x08874F48u }, { 2, 1, 2, 0x08874F60u },
        { 3, 0, 1, 0x08874F78u }, { 3, 1, 2, 0x08874F94u },
    };
    for (int i = 0; i < 10; i++) {
        if (S8(f[tries[i].a] + 263) & tries[i].bit) {
            ARG(0) = f[tries[i].a]; ARG(1) = f[tries[i].b]; c->r[31] = tries[i].back; f_08870874(c);
        }
    }
    int32_t side = S16(0x092E1228u + 56);
    int pairs[2][2]; uint32_t backs[2]; int np = 0;
    if (side == 0) { pairs[0][0] = 0; pairs[0][1] = 1; backs[0] = 0x08874FD0u; np = 1; }
    else if (side == 1) {
        pairs[0][0] = 0; pairs[0][1] = 2; backs[0] = 0x08875004u;
        pairs[1][0] = 0; pairs[1][1] = 3; backs[1] = 0x08875018u; np = 2;
    } else if (side == 2) {
        pairs[0][0] = 1; pairs[0][1] = 2; backs[0] = 0x08875034u;
        pairs[1][0] = 1; pairs[1][1] = 3; backs[1] = 0x08875048u; np = 2;
    } else if (side == 3) {
        pairs[0][0] = 0; pairs[0][1] = 2; backs[0] = 0x08875060u;
        pairs[1][0] = 1; pairs[1][1] = 2; backs[1] = 0x08875074u; np = 2;
    }
    for (int i = 0; i < np; i++) {
        gaddr a = f[pairs[i][0]], b = f[pairs[i][1]];
        ARG(0) = a; ARG(1) = b; ARG(2) = *W(a + 272); ARG(3) = *W(b + 272);
        c->r[31] = backs[i]; f_0887530C(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}


/* (projectile): does its current hit meet a fighter's hit? Against the other side's fighters by the
   side count (0: 1 or 0; 1: 2 and 3, or 0; 2: 2 and 3, or 1; 3: 2, or 0 and 1; by its +121), any of
   them (0x08874C14). A frame keeping s0. */
SFA_FUNC(08874AA0, projectile_vs_hits) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[2] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = x;
    if (*B(*W(x + 296) + 1) != 0) {
        int32_t side = S16(0x092E1228u + 56);
        int s = S8(x + 121) != 0;
        int t[2] = { -1, -1 }; uint32_t bk[2] = { 0, 0 };
        if (side == 0) { if (!s) { t[0] = 1; bk[0] = 0x08874AF0u; } else { t[0] = 0; bk[0] = 0x08874B04u; } }
        else if (side == 1) {
            if (!s) { t[0] = 2; bk[0] = 0x08874B30u; t[1] = 3; bk[1] = 0x08874B44u; }
            else { t[0] = 0; bk[0] = 0x08874B58u; }
        } else if (side == 2) {
            if (!s) { t[0] = 2; bk[0] = 0x08874B84u; t[1] = 3; bk[1] = 0x08874B98u; }
            else { t[0] = 1; bk[0] = 0x08874BACu; }
        } else if (side == 3) {
            if (!s) { t[0] = 2; bk[0] = 0x08874BD8u; }
            else { t[0] = 0; bk[0] = 0x08874BECu; t[1] = 1; bk[1] = 0x08874C00u; }
        }
        if (t[0] >= 0) {
            ARG(0) = x; ARG(1) = 0x092E3EA0u + (uint32_t)t[0] * 0x498u; c->r[31] = bk[0]; f_08874C14(c);
            if (t[1] >= 0) {
                uint32_t r = c->r[2]; c->r[16] = r;
                ARG(0) = x; ARG(1) = 0x092E3EA0u + (uint32_t)t[1] * 0x498u; c->r[31] = bk[1]; f_08874C14(c);
                c->r[2] |= r;
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08873D30(Cpu *c); void f_08874224(Cpu *c); void f_08872950(Cpu *c);

/* The frame's hit pass: unless the result block's +78, each fighter's +272 / +276 / +661 cleared; unless
   paused (+14 / +7 bit 7) or a fighter's +237 is set, 0x08873D30 on each fighter, 0x08874224, all the
   fighters' hits (0x08874E6C), both projectile loops and 0x08872950. A 0x30 frame keeping s0..s3. */
SFA_FUNC(08870750, hit_pass) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    const gaddr res = 0x092E1228u;
    c->r[6] = res;
    c->r[4] = (uint32_t)S8(res + 78);
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = s3; *W(sp + 32) = ra;
    int go = c->r[4] == 0;
    c->r[4] = 0x08A10000u;
    if (go) {
        for (uint32_t i = 0; i < 4; i++) {
            gaddr f = *W(0x08A17F88u + 4 * i);
            *W(f + 272) = 0; *W(f + 276) = 0; *B(f + 661) = 0;
        }
        c->r[4] = 0x08A17F98u; c->r[5] = 4; c->r[7] = 0;
        if (!(S8(res + 14) & 0x80) && !(S8(res + 7) & 0x80)) {
            const gaddr f0 = 0x092E3EA0u, f1 = 0x092E4338u, f2 = 0x092E47D0u, f3 = 0x092E4C68u;
            c->r[19] = f2; c->r[18] = f3; c->r[17] = f0; c->r[16] = f1;
            if ((S8(f2 + 237) | S8(f3 + 237) | S8(f0 + 237) | S8(f1 + 237)) == 0) {
                ARG(0) = f0; c->r[31] = 0x0887080Cu; f_08873D30(c);
                ARG(0) = f1; c->r[31] = 0x08870814u; f_08873D30(c);
                ARG(0) = f2; c->r[31] = 0x0887081Cu; f_08873D30(c);
                ARG(0) = f3; c->r[31] = 0x08870824u; f_08873D30(c);
                c->r[31] = 0x0887082Cu; f_08874224(c);
                ARG(0) = f0; ARG(1) = f1; ARG(2) = f2; ARG(3) = f3; c->r[31] = 0x08870840u; f_08874E6C(c);
                c->r[31] = 0x08870848u; f_08873EA8(c);
                c->r[31] = 0x08870850u; f_08873FC8(c);
                c->r[31] = 0x08870858u; f_08872950(c);
            }
        } else c->r[19] = 0x092E0000u;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}


void f_088768F0(Cpu *c); void f_08821384(Cpu *c); void f_08821424(Cpu *c);

/* (projectile): a projectile's frame: unless paused (the result block's +37 / +78), one that meets a
   fighter's hit goes to state 2, else it flies (position by speed, speed by brake) and goes to state 2
   below the ground; then 0x088768F0 and 0x08821384; 0x08821424 every frame. */
SFA_FUNC(08980964, projectile_fly) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    const gaddr res = 0x092E1228u;
    int32_t paused = S8(res + 37) | S8(res + 78);
    *W(c->r[29] + 16) = s0; c->r[16] = x;
    *W(c->r[29] + 20) = ra;
    if (paused == 0) {
        ARG(0) = x; c->r[31] = 0x08980998u; f_08874AA0(c);
        if (c->r[2] != 0) {
            *B(x + 4) = 2;
            ARG(0) = x; c->r[31] = 0x08980A14u; f_088768F0(c);
            ARG(0) = x; c->r[31] = 0x08980A1Cu; f_08821384(c);
        } else {
            uint32_t vx = *W(x + 96), vy = *W(x + 100);
            *W(x + 16) += vx;
            *W(x + 20) += vy;
            *W(x + 96) = vx + *W(x + 104);
            *W(x + 100) = vy + *W(x + 108);
            if (S16(x + 22) < S16(x + 132)) *B(x + 4) = 2;
            ARG(0) = x; c->r[31] = 0x089809F8u; f_088768F0(c);
            ARG(0) = x; c->r[31] = 0x08980A00u; f_08821384(c);
        }
    }
    ARG(0) = x; c->r[31] = 0x08980A24u; f_08821424(c);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
