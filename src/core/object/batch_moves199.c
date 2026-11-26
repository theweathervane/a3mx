/* Steps of 69 - 71 instructions: the network session's next-free-slot search under its semaphore
   (0x088DC948), a throw start by the opponent's character (0x088EDC14), and projectile and
   afterimage spawns that copy the fighter (0x08921A74, 0x089441EC, 0x088F8FE8). Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c);
void f_088D55F8(Cpu *c); void f_08821330(Cpu *c); void f_08822D2C(Cpu *c); void f_089247DC(Cpu *c);
void f_08944300(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static gaddr spawn(Cpu *c, gaddr o, uint32_t kind, uint32_t arg, uint32_t back) {
    ARG(0) = o; ARG(1) = kind; ARG(2) = arg; c->r[31] = back; f_08822D2C(c);
    return c->r[2];
}

/* (count): under the session semaphore (0x09523720 +8) the list (+0) is walked for the last live entry
   (+152 > 0, +144 = 1) up to the current slot (+12) + count; with a positive count and none found by
   then, the first live entry after that point keeps the walk's index. A found slot goes to 0x088D55F8
   and +12. */
SFA_FUNC(088DC948, net_next_slot) {
    uint32_t n = ARG(0);
    uint32_t ra = c->r[31], s[5];
    for (int i = 0; i < 5; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr ses = 0x09523720u;
    *W(c->r[29] + 32) = s[4]; c->r[20] = ses;
    *W(c->r[29] + 16) = s[0]; *W(c->r[29] + 24) = s[2];
    c->r[16] = n;
    int32_t lim = (int32_t)((uint32_t)S16(ses + 12) + n);
    c->r[18] = (uint32_t)lim;
    uint32_t sema = *W(ses + 8);
    *W(c->r[29] + 28) = s[3];
    int32_t found = -1;
    *W(c->r[29] + 20) = s[1]; *W(c->r[29] + 36) = ra;
    c->r[17] = 1;
    if (sema != 0) {
        ARG(0) = sema; ARG(1) = 1; ARG(2) = 0; c->r[31] = 0x088DC994u; hle_sceKernelWaitSema(c);
        sema = *W(ses + 8);
    }
    gaddr p = *W(ses);
    int32_t i = 0;
    while (p != 0 && !(lim < i)) {
        if (S16(p + 152) > 0 && *W(p + 144) == 1) found = i;
        p = *W(p);
        i++;
    }
    if ((int32_t)n > 0 && found < lim && p != 0) {
        found = -1;
        for (;;) {
            if (S16(p + 152) > 0 && *W(p + 144) == 1) { found = i; break; }
            p = *W(p);
            if (p == 0) break;
        }
    } else if ((int32_t)n > 0 && found < lim) found = -1;
    c->r[19] = (uint32_t)found; c->r[5] = (uint32_t)i; c->r[6] = p;
    ARG(0) = sema;
    if (sema != 0) { ARG(1) = 1; c->r[31] = 0x088DCA28u; hle_sceKernelSignalSema(c); }
    if (found >= 0) {
        c->r[31] = 0x088DCA38u; f_088D55F8(c);
        *H(ses + 12) = (uint16_t)found;
    }
    for (int k = 0; k < 5; k++) c->r[16 + k] = s[k];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter): a throw start, +6 counts, +1 = 1, by the opponent's character: 0x17 a lift (+6 = 5, +234 =
   1, +82 = 19, x kept in +100, x 10.0 / -0.375, move 0x2A); 0xB move 0 when it is locked; 1 the edge
   version (+6 = 2, +82 = 2, beside 0x08821330's object by its side, move 0x30); else move 0x22. */
SFA_FUNC(088EDC14, throw_start_by_char) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    gaddr opp = *W(o + 92);
    uint32_t n = *B(o + 6);
    *W(c->r[29] + 20) = s1;
    c->r[17] = 1;
    *B(o + 6) = (uint8_t)(n + 1); *B(o + 1) = 1;
    int32_t st = S8(opp + 338);
    *W(c->r[29] + 24) = ra;
    if (st == 0x17) {
        *B(o + 6) = 5; *B(o + 234) = 1;
        uint32_t x = *W(o + 16);
        *H(o + 82) = 0x13; *W(o + 100) = x;
        *W(o + 96) = 0xA0000; *W(o + 104) = 0xFFFFA000u;
        call2(c, o, 0x2A, f_088616EC, 0x088EDCA0u);
    } else {
        uint32_t mv;
        if (st == 0xB && S8(opp + 373) != 0) mv = 0;
        else if (st == 1) {
            *B(o + 6) = 2; *H(o + 82) = 2;
            call1(c, o, f_08821330, 0x088EDCE0u);
            int32_t side = S8(o + 337);
            int32_t ex = S16(c->r[2] + 18);
            if (side == 0) { *H(o + 18) = (uint16_t)(ex + 0x60); *B(o + 11) = 1; }
            else { *H(o + 18) = (uint16_t)(ex + 0x120); *B(o + 11) = 0; }
            mv = 0x30;
        } else mv = 0x22;
        call2(c, o, mv, f_0886163C, 0x088EDD14u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a frame flag (cleared) steps +7 on and spawns projectile 0xA (set up by 0x089247DC: +255 /
   +347 / +112 cleared, +311 = 2, +312 = 8, +260 = strength + 10); with +251 and the meter (+286) at
   +662 or more a second one, its +280 = +662; ticked. */
SFA_FUNC(08921A74, move_flag_projectiles_a) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[4];
    int32_t k = S8(o + 77);
    c->r[29] -= 0x30;
    for (int i = 0; i < 4; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    if (k != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1);
        gaddr e = spawn(c, o, 0xA, 0, 0x08921AB8u);
        c->r[19] = e;
        if (e != 0) {
            ARG(0) = o; ARG(1) = e; c->r[31] = 0x08921ACCu; f_089247DC(c);
            *B(e + 255) = 0; *B(e + 347) = 0;
            c->r[18] = 2; *H(e + 112) = 0; c->r[17] = 8;
            *B(e + 311) = 2; *B(e + 312) = 8;
            *B(e + 260) = (uint8_t)(S8(o + 194) + 0xA);
            if (S8(o + 251) != 0 && !((S16(o + 286) & 0xFFFF) < (S16(o + 662) & 0xFFFF))) {
                gaddr e2 = spawn(c, o, 0xA, 0, 0x08921B28u);
                c->r[19] = e2;
                if (e2 != 0) {
                    ARG(0) = o; ARG(1) = e2; c->r[31] = 0x08921B3Cu; f_089247DC(c);
                    *B(e2 + 255) = 0; *B(e2 + 347) = 0; *H(e2 + 112) = 0;
                    uint32_t m = (uint32_t)S16(o + 662);
                    *B(e2 + 311) = 2; *B(e2 + 280) = (uint8_t)m; *B(e2 + 312) = 8;
                    *B(e2 + 260) = (uint8_t)(S8(o + 194) + 0xA);
                }
            }
        }
    }
    call1(c, o, f_08861848, 0x08921B6Cu);
    for (int i = 0; i < 4; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter): ticked; any frame flag runs 0x08944300; flag 3 also sets step 2 and spawns an afterimage
   (0x08822D2C 0x10 / 0) with the fighter's picture, place, facing and side (+255 = 5, +346 = strength,
   +138 = 1, tied to it). */
SFA_FUNC(089441EC, move_flag_afterimage_3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08944200u);
    int32_t k = S8(o + 77);
    if (k != 0) {
        if (k != 3) call1(c, o, f_08944300, 0x08944250u);
        else {
            call1(c, o, f_08944300, 0x0894421Cu);
            *B(o + 7) = 2;
            gaddr e = spawn(c, o, 0x10, 0, 0x08944234u);
            if (e != 0) {
                *B(e + 122) = *B(o + 122); *B(e + 121) = *B(o + 121);
                int32_t s = S8(o + 194);
                *B(e + 255) = 5; *B(e + 346) = (uint8_t)s; *B(e + 347) = 0;
                *B(e + 14) = *B(o + 14);
                *H(e + 86) = (uint16_t)S8(o + 337);
                *W(e + 164) = *W(o + 164);
                *B(e + 11) = *B(o + 11);
                *H(e + 38) = *H(o + 38);
                *H(e + 16) = *H(o + 16); *H(e + 18) = *H(o + 18);
                *H(e + 20) = *H(o + 20); *H(e + 22) = *H(o + 22);
                *H(e + 112) = 0;
                *W(e + 88) = o; *B(e + 138) = 1;
            }
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; ended: standing. A frame flag (cleared) spawns a copy (0x08822D2C 2 / 3) with the
   fighter's picture, palette, place and facing: +346 = strength + 9, +112 = 2 x strength + 3, +255 = 3,
   +347 = 1, +260 from 0x08A49770 by strength, +311 = 2, +312 = 8; counted (+536). */
SFA_FUNC(088F8FE8, move_flag_copy) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088F8FFCu);
    if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x088F9010u);
    else if (S8(o + 77) != 0) {
        *B(o + 77) = 0;
        gaddr e = spawn(c, o, 2, 3, 0x088F9034u);
        if (e != 0) {
            *B(e + 122) = *B(o + 122); *B(e + 121) = *B(o + 121); *B(e + 81) = *B(o + 81);
            *B(e + 346) = (uint8_t)(S8(o + 194) + 9);
            *H(e + 112) = (uint16_t)((S8(o + 194) << 1) + 3);
            *B(e + 255) = 3; *B(e + 347) = 1;
            *B(e + 14) = *B(o + 14); *B(e + 11) = *B(o + 11);
            *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13);
            *H(e + 38) = *H(o + 38);
            *W(e + 16) = *W(o + 16);
            uint32_t y = *W(o + 20);
            *W(e + 88) = o; *W(e + 20) = y;
            uint32_t n = *B(o + 536);
            int32_t s = S8(o + 194);
            *B(o + 536) = (uint8_t)(n + 1);
            *B(e + 260) = (uint8_t)S8(0x08A49770u + (uint32_t)s);
            *B(e + 311) = 2; *B(e + 312) = 8;
        }
    }
    leave(c, sv);
}
