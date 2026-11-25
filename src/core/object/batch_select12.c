/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 46 - 79
   instructions: leaving the select screen (slot 8 back, waiting for the loader), the VS screen's
   stage name, the intro's second banner, a mode screen's tickers reset. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_088054F0(Cpu *c); void f_088075F8(Cpu *c); void f_08814134(Cpu *c); void f_088D7EA0(Cpu *c);
void f_0889CFE8(Cpu *c); void f_0889FBF8(Cpu *c); void f_0889FC34(Cpu *c); void f_08821424(Cpu *c);
void f_0882196C(Cpu *c); void f_08823338(Cpu *c); void f_08855D80(Cpu *c); void f_088969BC(Cpu *c);
void f_0880AEA8(Cpu *c); void f_08822EAC(Cpu *c); void f_088AC3A8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* Leaving the select screen, under the loader's lock (0x088D7EA0 (1) / (0)): slot 8 put back
   (0x08814134), the current 216-byte record's +202 - +204 / +208 cleared, 0x088075F8 (0x08A03150's
   word, 0, 0), a wait (0x088054F0 (1) a frame) until 0x08E7606C is set, and 0x08E75C4C = -1. */
SFA_FUNC(08814260, select_leave) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    call_on(c, 1, 0x08814274u, f_088D7EA0);
    c->r[31] = 0x0881427Cu; f_08814134(c);
    gaddr rec = 0x08E76088u + (uint32_t)*B(0x08E76084u) * 216;
    *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
    call3(c, *W(0x08A03140u + 16), 0, 0, 0x088142C8u, f_088075F8);
    c->r[16] = 0x08E76068u;
    while (*B(0x08E76068u + 4) == 0) call_on(c, 1, 0x088142E4u, f_088054F0);
    *H(0x08E75C48u + 4) = 0xFFFF;
    call_on(c, 0, 0x08814308u, f_088D7EA0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (name): the VS screen's stage name: on its first frame its set-up (0x0889CFE8, x 0x100, +14 / +24 /
   y from the 6-byte row by +10 of 0x08A2B120), its pictures by the stage (0x0889FBF8 (+10 + 4)) and
   two palette lists (+10 at +0x60, +10 + 2 at +0x68); then at once with BATTLE +81 (+4 = +5 = 1), or
   at GAME +20 high byte 1: +4 on, +82 = 0x10. */
SFA_FUNC(0889E7B0, vs_stage_name) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o; c->r[17] = 0x092E0000u;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(o + 148) = 0;
        *H(o + 86) = 8;
        call2(c, o, 0, 0x0889E7F0u, f_0889CFE8);
        uint32_t k = *B(o + 10);
        *H(o + 18) = 0x100;
        gaddr e = 0x08A2B120u + k * 6;
        *B(o + 14) = (uint8_t)S16(e);
        *H(o + 24) = (uint16_t)S16(e + 2);
        *H(o + 22) = (uint16_t)S16(e + 4);
        call3(c, o, (k + 4) & 0xFFFF, *H(BATTLE), 0x0889E84Cu, f_0889FBF8);
        call3(c, o, (uint32_t)*B(o + 10) & 0xFFFF, o + 0x60, 0x0889E860u, f_0889FC34);
        call3(c, o, ((uint32_t)*B(o + 10) + 2) & 0xFFFF, o + 0x68, 0x0889E878u, f_0889FC34);
    } else if (S8(BATTLE + 81) != 0) { *B(o + 5) = 1; *B(o + 4) = 1; }
    else if (((uint32_t)(int32_t)S16(GAME + 20) & 0xFF00) == 0x100) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *B(o + 5) = 0;
        *H(o + 82) = 0x10;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (banner): the intro's second banner: 0x08855D80 (banner, 4, 0xC0, 0x80), the stage's palette
   (0x654 + the byte at 0x08A16B34) into slot 0x1E, BATTLE +44 cleared, set up (+4 on, +5 = 2, +82 =
   0xB4, +7 = 0x1E, +79 = 8, +148 = 0xB, +156 = 0x08A16962); with BATTLE +13 set its animation 0x13,
   else (+5 = 1, +79 = 7) with BATTLE +76 set 0x12, else a resting banner (+7 = 0xF, +9 = 7, +82 = 0x12,
   +79 = 2) with a list-5 type-0xA object pointing back at it (+124). */
SFA_FUNC(088555BC, intro_banner_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    ARG(0) = o; ARG(1) = 4; ARG(2) = 0xC0; ARG(3) = 0x80;
    c->r[31] = 0x088555E4u; f_08855D80(c);
    c->r[17] = 0x092E0000u; c->r[18] = 0x1E;
    call3(c, *B(0x08A16B34u + (uint32_t)(int32_t)S16(BATTLE)) + 0x654u, 0x1E, 1, 0x08855610u, f_088969BC);
    *B(BATTLE + 44) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 5) = 2;
    *H(o + 82) = 0xB4;
    *B(o + 7) = 0x1E;
    *B(o + 79) = 8;
    *W(o + 148) = 0xB;
    *W(o + 156) = 0x08A16962u;
    uint32_t k;
    if (S8(BATTLE + 13) != 0) k = 0x13;
    else {
        *B(o + 5) = 1; *B(o + 79) = 7;
        if (S8(BATTLE + 76) != 0) k = 0x12;
        else {
            *B(o + 5) = 0; *B(o + 7) = 0xF; *B(o + 9) = 7; *H(o + 82) = 0x12; *B(o + 79) = 2;
            call2(c, 5, 0xA, 0x088556A8u, f_08823338);
            gaddr n = c->r[2];
            ARG(0) = n;
            if (n != 0) *W(n + 124) = o;
            goto done;
        }
    }
    call2(c, o, *W(0x08A16A90u + k * 4), 0x088556D8u, f_0882196C);
    call_on(c, o, 0x088556E0u, f_08821424);
done:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen): +8 on, 0x088AC3A8 (the new +8), the three tickers (0x08B027B0 / E0 / 0x08B02810) reset (0x08B02814 =
   0x2000000), +9 and GAME +10 cleared; with 4 or more free objects in pool 4, four list-0x35 objects
   (types 3, 4, 5 and +2) with +0 = 1; then the palette 0x08A32C18 + 32 x +2 and 0x08A32C78 into slots
   0 and 0x1F. */
SFA_FUNC(088ABE98, mode_tickers_reset) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    c->r[16] = o; c->r[17] = 0;
    uint32_t n8 = *B(o + 8) + 1u;
    *B(o + 8) = (uint8_t)n8;
    call_on(c, n8, 0x088ABEC0u, f_088AC3A8);   /* (a0 is the new +8, as the original leaves it) */
    *H(0x08B02810u + 18) = 0; *H(0x08B02810u + 16) = 0;
    *H(0x08B027E0u + 18) = 0; *H(0x08B027E0u + 16) = 0;
    *H(0x08B027B0u + 18) = 0; *H(0x08B027B0u + 16) = 0;
    *W(0x08B02810u) = 0;
    *B(o + 9) = 0;
    *W(0x08B027E4u) = 0; *W(0x08B027E0u) = 0;
    *W(0x08B027B4u) = 0; *W(0x08B027B0u) = 0;
    *H(GAME + 10) = 0;
    *W(0x08B02814u) = 0x2000000u;
    if (!(S8(0x095176F3u) < 4)) {
        call2(c, 0x35, 3, 0x088ABF40u, f_08822EAC); c->r[17] = 1; *B(c->r[2]) = 1;
        call2(c, 0x35, 4, 0x088ABF54u, f_08822EAC); *B(c->r[2]) = 1;
        call2(c, 0x35, 5, 0x088ABF64u, f_08822EAC); *B(c->r[2]) = 1;
        call2(c, 0x35, *H(o + 2) & 0xFF, 0x088ABF78u, f_08822EAC); *B(c->r[2]) = 1;
    }
    c->r[16] = 0x08A32BD8u;
    ARG(0) = 1; ARG(1) = 0x08A32BD8u + ((uint32_t)*H(o + 2) << 5) + 0x40; ARG(2) = 0; ARG(3) = 0; c->r[8] = 1;
    c->r[31] = 0x088ABFA8u; f_0880AEA8(c);
    ARG(0) = 1; ARG(1) = 0x08A32BD8u + 0xA0; ARG(2) = 0; ARG(3) = 0x1F; c->r[8] = 1;
    c->r[31] = 0x088ABFC0u; f_0880AEA8(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}
