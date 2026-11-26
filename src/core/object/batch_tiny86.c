/* Small functions the scenarios reach, 119 - 120 instructions: an effect sliding in from the screen
   edge and pushing its owner, a fighter's two gauge helpers. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08821330(Cpu *c); void f_08821424(Cpu *c); void f_0882196C(Cpu *c); void f_08979D40(Cpu *c);
void f_08822D2C(Cpu *c); void f_0887CAD0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): step 0 sets it up from its owner (+88: +86 / +148 / +164), +9 = 1, 0x10 higher, at the
   screen edge (0x08821330) 0x1E0 right (facing left, x step -10 at +0x62) or 0x60 left (facing
   right, +10) by the owner's side, animation 0x08A8E920 +76; step 1 slides 0x10 frames moving its
   owner too, waits 0x3C, then slides back twice as fast for 4 frames, then goes on (+4 = 2); drawn.
   Later steps 0x08979D40. */
SFA_FUNC(08979F0C, effect_slide_push_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 4);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    if (st == 0) {
        *B(o + 4) = 1;
        c->r[18] = p;
        *H(o + 86) = *H(p + 86);
        c->r[17] = 1;
        *W(o + 164) = *W(p + 164);
        *W(o + 148) = *W(p + 148);
        *B(o + 9) = 1;
        *H(o + 22) = (uint16_t)(S16(o + 22) - 0x10);
        *H(o + 82) = 0x10;
        call_on(c, o, 0x08979F78u, f_08821330);
        *H(o + 18) = (uint16_t)S16(c->r[2] + 18);
        if (S8(p + 337) != 0) {
            *B(o + 11) = 0;
            *H(o + 18) = (uint16_t)(S16(o + 18) + 0x1E0);
            *H(o + 0x62) = (uint16_t)-10;
        } else {
            *B(o + 11) = 1;
            *H(o + 18) = (uint16_t)(S16(o + 18) - 0x60);
            *H(o + 0x62) = 10;
        }
        call2(c, o, *W(0x08A8E920u + 76), 0x08979FD8u, f_0882196C);
        call_on(c, o, 0x08979FE0u, f_08821424);
    } else if (st == 1) {
        uint32_t h = (uint32_t)S16(o + 82) - 1;
        uint32_t sub = *B(o + 5);
        *H(o + 82) = (uint16_t)h;
        if (sub == 0) {
            if ((int16_t)h == 0) { *B(o + 5) = 1; *H(o + 82) = 0x3C; }
            *H(o + 18) = (uint16_t)(S16(o + 18) + S16(o + 0x62));
            *H(p + 18) = (uint16_t)(S16(p + 18) + S16(o + 0x62));
        } else if (sub == 1) {
            if ((int16_t)h == 0) {
                *B(o + 5) = 2;
                *H(o + 82) = 4;
                *H(o + 0x62) = (uint16_t)-(S16(o + 0x62) << 1);
            }
        } else {
            if ((int16_t)h == 0) { *B(o + 4) = 2; *B(o + 5) = 0; }
            *H(o + 18) = (uint16_t)(S16(o + 18) + S16(o + 0x62));
        }
        call_on(c, o, 0x0897A0C0u, f_08821424);
    } else call_on(c, o, 0x0897A0D0u, f_08979D40);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* One gauge helper's set-up from its fighter. */
static void gauge_helper(gaddr n, gaddr f, uint32_t which, uint32_t v, uint8_t t52, uint8_t up, uint16_t h150) {
    *B(n + 3) = (uint8_t)which;
    *B(n + 255) = 5;
    *B(n + 347) = 1;
    *B(n + 346) = (uint8_t)S8(f + 194);
    *B(n + 14) = *B(f + 14);
    *B(n + 78) = (uint8_t)S8(f + 337);
    *B(n + 122) = (uint8_t)S8(f + 122);
    *B(n + 121) = (uint8_t)S8(f + 121);
    *B(n + 80) = (uint8_t)S8(f + 80);
    *W(n + 96) = v;
    *B(n + 81) = (uint8_t)S8(f + 81);
    *B(n + 0x52) = t52;
    *B(n + 0x53) = (uint8_t)which;
    *W(n + 88) = f;
    *B(n + 12) = (uint8_t)S8(f + 12);
    *B(n + 13) = *B(f + 13);
    *B(n + 13) = (uint8_t)(*B(n + 13) + up);
    *H(n + 0x150) = h150;
    *H(n + 86) = *H(f + 86);
    *W(n + 164) = *W(f + 164);
}

/* (fighter): with 2 or more of its side's gauge stocks (0x095176F0 / F1 by +121), +540 = 2, +656 = 4
   and two list-0x17 helpers (0x08822D2C (0x17, 0 / 1)), the later one first: +3 = 0 / 1, 0x0887CAD0's
   value for its side (+96), +0x52 = 8 / 0x10, +13 + 3 / + 4, +0x150 = 0x16 / 8, and the fighter's
   +12 / +14 / +80 / +81 / +121 / +122 / +194 / side / +86 / +164, +255 = 5, +347 = 1. */
SFA_FUNC(0891F508, fighter_gauge_helpers) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    ARG(0) = (uint32_t)S8(f + 337);
    c->r[31] = 0x0891F528u; f_0887CAD0(c);
    uint32_t v = c->r[2];
    c->r[17] = v;
    int32_t stock = S8(f + 121) != 0 ? S8(0x095176F1u) : S8(0x095176F0u);
    if (stock >= 2) {
        *B(f + 540) = 2;
        *B(f + 656) = 4;
        ARG(0) = f; ARG(1) = 0x17; ARG(2) = 0;
        c->r[31] = 0x0891F570u; f_08822D2C(c);
        gaddr a = c->r[2];
        ARG(0) = f; ARG(1) = 0x17; ARG(2) = 1;
        c->r[31] = 0x0891F584u; f_08822D2C(c);
        gaddr b = c->r[2];
        gaddr hi = b, lo = a;
        if (b < a) { hi = a; lo = b; }
        c->r[18] = lo;
        gauge_helper(hi, f, 0, v, 8, 3, 0x16);
        gauge_helper(lo, f, 1, v, 0x10, 4, 8);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
