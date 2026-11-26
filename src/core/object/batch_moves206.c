/* Steps of 85 - 89 instructions: win-pose starts by the opponent's character, one with a coin-toss
   effect (0x0895BF64) and one that throws six sparkles in a late round (0x08947614), and a copy spawned
   with two trailing glows kept at +44 / +48 (0x0899C3A0). Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08821368(Cpu *c);
void f_08822EAC(Cpu *c); void f_08822D2C(Cpu *c); void f_08822F54(Cpu *c); void f_0899C504(Cpu *c);

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
static gaddr effect(Cpu *c, uint32_t kind, uint32_t arg, uint32_t back) {
    ARG(0) = kind; ARG(1) = arg; c->r[31] = back; f_08822EAC(c);
    return c->r[2];
}
/* Effect 0x78 takes the fighter's side, picture and frame and is tied to it. */
static void tie_78(gaddr e, gaddr o) {
    *H(e + 86) = *H(o + 86); *W(e + 164) = *W(o + 164);
    uint32_t f = *W(o + 148);
    *W(e + 88) = o; *W(e + 148) = f;
}

/* (fighter): a win pose: +6 = 0: +6 = 1, +1 = 1; against character 0xA move 0x22 (with effect 0x78
   when it is locked) or 0x32; against others a coin toss (0x08821368) gives effect 0x78 with move 0x22
   or move 0 of its own list. Later standing once ended, else ticked. */
SFA_FUNC(0895BF64, win_pose_coin) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t n = *B(o + 6);
    gaddr opp = *W(o + 92);
    if (n == 0) {
        *B(o + 6) = 1; *B(o + 1) = 1;
        int32_t st = S8(opp + 338);
        if (st == 0xA) {
            if (S8(opp + 373) != 0) {
                gaddr e = effect(c, 0x78, 0, 0x0895BFB8u);
                if (e != 0) tie_78(e, o);
                call2(c, o, 0x22, f_0886163C, 0x0895BFECu);
            } else call2(c, o, 0x32, f_0886163C, 0x0895BFFCu);
        } else {
            ARG(0) = (uint32_t)st; c->r[31] = 0x0895C00Cu; f_08821368(c);
            if (c->r[2] & 1) {
                gaddr e = effect(c, 0x78, 0, 0x0895C024u);
                if (e != 0) tie_78(e, o);
                call2(c, o, 0x22, f_0886163C, 0x0895C058u);
            } else call2(c, o, 0, f_088616EC, 0x0895C068u);
        }
    } else if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        call2(c, o, 0, f_0886163C, 0x0895C0A8u);
    } else call1(c, o, f_08861848, 0x0895C084u);
    leave(c, sv);
}

/* (fighter): a win pose, +1 = 1: against character 0x1A +6 = 2, from round 6 (0x095176F3) six sparkles
   (0x08822EAC 0x2D, indexed by +78: three / 0, three / 1), x kept in +108, 192 toward its side and a
   dash of 7.0 into move 0x21; against 0x26 +6 = 5; else +6 = 1 and move 0x22. */
SFA_FUNC(08947614, win_pose_sparkles) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    gaddr opp = *W(o + 92);
    *B(o + 1) = 1;
    int32_t st = S8(opp + 338);
    if (st == 0x1A) {
        *B(o + 6) = 2;
        if (!(S8(0x095176F3u) < 6)) {
            static const uint32_t back[6] = { 0x08947664u, 0x0894767Cu, 0x08947698u,
                                              0x089476B4u, 0x089476D0u, 0x089476ECu };
            for (uint32_t i = 0; i < 6; i++) {
                gaddr e = effect(c, 0x2D, i < 3 ? 0 : 1, back[i]);
                if (e != 0) *B(e + 78) = (uint8_t)i;
            }
        }
        int32_t side = S8(o + 337);
        uint32_t d = side != 0 ? 0xC0u : 0xFFFFFF40u, vx = side != 0 ? 0xFFF90000u : 0x70000u;
        uint32_t x = *W(o + 16);
        int32_t px = S16(o + 18);
        *W(o + 108) = x;
        *H(o + 18) = (uint16_t)((uint32_t)px + d);
        *W(o + 96) = vx;
        call2(c, o, 0x21, f_088616EC, 0x08947738u);
    } else if (st == 0x26) *B(o + 6) = 5;
    else { *B(o + 6) = 1; call2(c, o, 0x22, f_0886163C, 0x08947760u); }
    leave(c, sv);
}

/* Glow `g` (0x08822F54 0x16) tied to the fighter with its side and picture, kept at `slot`. */
static void tie_glow(gaddr g, gaddr o, uint32_t slot) {
    *W(g + 88) = o;
    int32_t side = S8(o + 337);
    *W(o + slot) = g;
    *H(g + 86) = (uint16_t)side;
    *W(g + 164) = *W(o + 164);
}

/* (fighter): a frame flag steps +7 on and spawns a copy (0x08822D2C 7 / 1: the fighter's picture,
   palette, place, side, +346 = strength, +112 = strength + 1, +255 cleared, +347 = 1) kept at +228 and
   counted (+536), with glows 0x16 / 1 and 0x16 / 2 kept at +44 / +48; then 0x0899C504. */
SFA_FUNC(0899C3A0, move_flag_copy_glows) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        ARG(0) = o; ARG(1) = 7; ARG(2) = 1; c->r[31] = 0x0899C3D4u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 122) = *B(o + 122); *B(e + 121) = *B(o + 121);
            *B(e + 346) = *B(o + 194); *B(e + 80) = *B(o + 80);
            *B(e + 14) = *B(o + 14); *B(e + 11) = *B(o + 11);
            *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13);
            *H(e + 38) = *H(o + 38); *H(e + 18) = *H(o + 18);
            uint32_t y = *H(o + 22);
            *B(e + 255) = 0; *H(e + 22) = (uint16_t)y; *B(e + 347) = 1;
            *H(e + 112) = (uint16_t)(S8(o + 194) + 1);
            *W(o + 228) = e; *W(e + 88) = o;
            uint32_t n = *B(o + 536);
            int32_t side = S8(o + 337);
            *B(o + 536) = (uint8_t)(n + 1);
            *H(e + 86) = (uint16_t)side;
            *W(e + 164) = *W(o + 164);
            ARG(0) = 0x16; ARG(1) = 1; c->r[31] = 0x0899C488u; f_08822F54(c);
            if (c->r[2] != 0) tie_glow(c->r[2], o, 44);
            ARG(0) = 0x16; ARG(1) = 2; c->r[31] = 0x0899C4C0u; f_08822F54(c);
            if (c->r[2] != 0) tie_glow(c->r[2], o, 48);
        }
    }
    call1(c, o, f_0899C504, 0x0899C4F4u);
    leave(c, sv);
}
