/* A held special with a countdown and a charge counter (0x089A3F8C), and two special-move starts that
   pick their move by button, distance and choice. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088616EC(Cpu *c); void f_08861674(Cpu *c); void f_08861848(Cpu *c); void f_08863114(Cpu *c);
void f_0886CA2C(Cpu *c); void f_089A3ED8(Cpu *c); void f_089A3F24(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter3(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
static void face_start(Cpu *c, gaddr o, uint32_t back) {
    uint32_t st = *B(o + 7);
    int32_t face = S8(o + 697);
    *B(o + 7) = (uint8_t)(st + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)face; *B(o + 235) = 1;
    call1(c, o, f_08863114, back);
}

/* (fighter): the ending: +7 on and move 0x15 + 4 x the charge (+96). */
static void held_end(Cpu *c, gaddr o, uint32_t back) {
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    call2(c, o, (uint32_t)((*W(o + 96) << 2) + 0x15) & 0xFFFF, f_088616EC, back);
}

/* (fighter): ended at once while 0x092E1228 +13 / +14 hold; else +82 counts down (when it runs out with
   no choice made and +760 bits 4..6, +78 down and +82 = 9), ticked while +76; at the end of +820: with
   +78 and +82 left +820 = 3, else ended, or with +78 run out the charge (+100) down: once more charged
   (+82 = 9, +820 = 7, +78 = 4, 0x089A3F24, +96 up, the meter by 2, 0x089A3ED8), else ended; ticked unless
   0x092E1228 +52. */
SFA_FUNC(089A3F8C, held_charge) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    const gaddr g = 0x092E1228u;
    if ((S8(g + 14) | S8(g + 13)) != 0) { held_end(c, o, 0x089A3FF4u); goto out; }
    int32_t t = S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if (S8(o + 373) == 0 && (int16_t)t >= 0 && (*H(o + 760) & 0x70)) {
        *B(o + 78) = (uint8_t)(S8(o + 78) - 1);
        *H(o + 82) = 9;
    }
    if (S8(o + 76) != 0) { call1(c, o, f_08861848, 0x089A4054u); goto out; }
    int32_t n = (int32_t)*W(o + 820) - 1;
    *W(o + 820) = (uint32_t)n;
    if (n < 0) {
        if (S8(o + 78) >= 0) {
            if (S16(o + 82) < 0) { held_end(c, o, 0x089A40A4u); goto out; }
            *W(o + 820) = *W(o + 820) + 4;
        } else {
            int32_t k = (int32_t)*W(o + 100) - 1;
            *W(o + 100) = (uint32_t)k;
            if (k < 0) { held_end(c, o, 0x089A40E8u); goto out; }
            *H(o + 82) = 9; *W(o + 820) = 7; *B(o + 78) = 4;
            call1(c, o, f_089A3F24, 0x089A4110u);
            *W(o + 96) = *W(o + 96) + 1;
            call2(c, o, 2, f_0886CA2C, 0x089A4128u);
            call1(c, o, f_089A3ED8, 0x089A4130u);
            goto out;
        }
    }
    if (S8(g + 52) == 0) call1(c, o, f_08861848, 0x089A414Cu);
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): face_start; with +290 bit 1 at strength 1 step 2, the meter by 1 and move 0x1E, at strength
   2 with a choice made step 3, the meter by 2 and move 0x21; otherwise the meter by strength and move 2 x
   strength (+1 unlocked with no choice made without +290 bit 0, else beyond the distance from 0x08A43A2C;
   with no choice made 2 becomes 0x22 and 5 0x23) of its list. */
SFA_FUNC(088EE36C, special_by_button_d) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    face_start(c, o, 0x088EE3A4u);
    int32_t s = S8(o + 194);
    if (*H(o + 290) & 2) {
        if (s == 1) {
            *B(o + 7) = 2;
            call2(c, o, 1, f_0886CA2C, 0x088EE3E4u);
            call2(c, o, 0x1E, f_08861674, 0x088EE3F0u);
            leave3(c, sv); return;
        }
        if (S8(o + 386) != 0 && s == 2) {
            *B(o + 7) = 3;
            call2(c, o, 2, f_0886CA2C, 0x088EE44Cu);
            call2(c, o, 0x21, f_08861674, 0x088EE458u);
            leave3(c, sv); return;
        }
    }
    call2(c, o, (uint32_t)s, f_0886CA2C, 0x088EE418u);
    s = S8(o + 194);
    int32_t k = S8(o + 386);
    uint32_t n = (uint32_t)(s << 1) & 0xFFFF;
    if (S8(o + 373) == 0 && k == 1) { if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF; }
    else if (S16(0x08A43A2Cu + (uint32_t)(s + s)) < S16(o + 508)) n = (n + 1) & 0xFFFF;
    if (k == -1) { if (n == 2) n = 0x22; else if (n == 5) n = 0x23; }
    call2(c, o, n, f_08861674, 0x088EE4E4u);
    leave3(c, sv);
}

/* (fighter): face_start, the meter by strength; with +290 bit 1 at strength 1 step 2 and move 0x1E (0x1F
   with no choice made), at strength 2 step 7 and move 0x20; with +290 bit 0 at strength 1 without a choice
   step 3 and the same 0x1E / 0x1F; otherwise move 2 x strength + 6 (+1 unlocked with no choice made
   without +290 bit 0, else beyond the distance from 0x08A9BEB6) of its list. */
SFA_FUNC(0899E560, special_by_button_e) {
    gaddr o = ARG(0); uint32_t sv[3];
    enter3(c, o, sv);
    face_start(c, o, 0x0899E598u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0899E5A4u);
    int32_t s = S8(o + 194);
    uint32_t n = (uint32_t)((s << 1) + 6) & 0xFFFF;
    if (s != 0) {
        uint32_t h = *H(o + 290);
        int32_t k = S8(o + 386);
        if (h & 2) {
            if (s != 1) { *B(o + 7) = 7; n = 0x20; }
            else { *B(o + 7) = 2; n = k == -1 ? 0x1F : 0x1E; }
            goto call;
        }
        if (k != 1 && (h & 1) && s == 1) {
            *B(o + 7) = 3; n = k == -1 ? 0x1F : 0x1E;
            goto call;
        }
    }
    if (S8(o + 373) == 0 && S8(o + 386) == 1) { if ((*H(o + 290) & 1) == 0) n = (n + 1) & 0xFFFF; }
    else if (S16(0x08A9BEB6u + (uint32_t)(s + s)) < S16(o + 508)) n = (n + 1) & 0xFFFF;
call:
    call2(c, o, n, f_08861674, 0x0899E6A4u);
    leave3(c, sv);
}
