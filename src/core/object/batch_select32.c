/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 34 - 374
   instructions: three fighters' select slots with their pictures put up, the fight HUD's super gauge
   (its cells, level digits and flashing palette). Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u
#define P1B    0x092E47D0u   /* the partners */
#define P2B    0x092E4C68u
#define PALS   0x092D9E00u
#define HUD    0x092E51C0u   /* the HUD's 64-byte cells; a cell's picture at +60 */

void f_08812FCC(Cpu *c); void f_08815F9C(Cpu *c); void f_0880AEA8(Cpu *c); void f_08821368(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* (a, b, c): the characters at 0x08E75C48 copied to 0x092E89DC, the three fighters' select slots
   (0x08815F9C) and each one's pictures put up (0x08812FCC). */
SFA_FUNC(088131E0, fighters_select_slots_3_show) {
    gaddr fa = ARG(0), fb = ARG(1), fc = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0;
    c->r[18] = fa; c->r[17] = fb; c->r[16] = fc;
    *H(0x092E89DCu) = (uint16_t)S16(0x08E75C48u);
    *H(0x092E89DCu + 2) = (uint16_t)S16(0x08E75C48u + 2);
    *H(0x092E89DCu + 4) = (uint16_t)S16(0x08E75C48u + 4);
    *W(c->r[29] + 28) = ra;
    call3(c, fa, fb, fc, 0x08813238u, f_08815F9C);
    call_on(c, fa, 0x08813240u, f_08812FCC);
    call_on(c, fb, 0x08813248u, f_08812FCC);
    call_on(c, fc, 0x08813250u, f_08812FCC);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

static uint16_t *cell(uint32_t off) { return H(HUD + off + 60); }

/* C's % on the gauge value (negative values give negative remainders, as the original works them out). */
static int32_t mod32(int32_t v) { return v % 32; }

/* (-, fighter, side): the super gauge of one side each frame (the fighter's gauge at +366, 0 - 144):
   the bar's cells (side 0 from cell 0x474, side 1 from 0xAB4), the MAX cell (pictures 0x157 / 0x158
   blinking with 0x092E51C6 bit 6, hidden under 144 or while the gauge is held (+251) or maxed (+237) -
   for line-ups the partners' or both fighters' flags together), the level marks 0x322 / 0x323, the
   level cell 0x324 + the gauge's position in its 32, and the level digits (gauge x 100 / 0x5A0, then
   x 100 / 0x90 mod 10, the last one at random) or 0x317 / 0x317 / 0x321 at the top; then its flashing
   palette (a 32-byte step of PALS +64, 0x13 - 0x17 by 0x092E51C9) into slot 0xF or 0x11. */
SFA_FUNC(08891354, hud_super_gauge) {
    gaddr f = ARG(1);
    uint32_t side = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x40;
    *W(c->r[29] + 32) = s0;
    c->r[16] = side;
    *W(c->r[29] + 40) = s2; *W(c->r[29] + 36) = s1; *W(c->r[29] + 44) = s3; *W(c->r[29] + 48) = ra;
    c->r[18] = PALS;
    int32_t v0 = S16(f + 366);
    int32_t m = S16(BATTLE + 56);
    int human1 = (S8(GAME + 53) & 1) != 0;
    int32_t maxed, held;
    gaddr x = 0, y = 0;
    if (side == 0) {
        if (m == 2) { x = P1B; y = P2B; }
        else if (m == 3 && human1) { x = P1; y = P2; }
    } else {
        if (m == 1) { x = P1B; y = P2B; }
        else if (m == 3 && !human1) { x = P1; y = P2; }
    }
    if (x != 0) { maxed = S8(x + 237) | S8(y + 237); held = S8(x + 251) | S8(y + 251); }
    else { maxed = S8(f + 237); held = S8(f + 251); }

    uint32_t maxcell;
    if (side == 0) { *cell(0x474) = (uint16_t)(v0 + 0x1EC); maxcell = 0x4B4; }
    else { *cell(0xAB4) = (uint16_t)(v0 + 0x27D); maxcell = 0xAF4; }
    if (S16(f + 366) < 144) *cell(maxcell) = 0x8000;
    else {
        *cell(maxcell) = (*H(HUD + 6) & 0x40) ? 0x157 : 0x158;
        if ((held | maxed) != 0) *cell(maxcell) = 0x8000;
    }
    uint32_t step = 0x13;
    if (!(S16(f + 366) < 72) || held != 0) {
        uint32_t t = *B(HUD + 9) & 7;
        step = t < 4 ? (t + 0x13) & 0xFFFF : (0x1B - t) & 0xFFFF;
    }
    if (maxed != 0) step = (*B(HUD + 9) & 1) ? 0x13 : 0x17;
    c->r[19] = step << 5;

    uint32_t marks = side ? 0xB34 : 0x4F4;
    *cell(marks) = 0x8000;
    if (!(S16(f + 366) < 72)) {
        *cell(marks) = 0x322;
        if (!(S16(f + 366) < 144)) *cell(marks) = 0x323;
    }
    if (held != 0) *cell(marks) = 0x8000;

    int32_t v = S16(f + 366);
    uint32_t lvl;
    if (side == 0) { lvl = 0x5F4; *H(HUD + lvl + 6) = (uint16_t)(0xB1 - v); }
    else { lvl = 0xC34; *H(HUD + lvl + 6) = (uint16_t)(v + 0xD0); }
    *cell(lvl) = 0x324;
    uint32_t d0 = side ? 0xB74 : 0x534, d1 = side ? 0xBB4 : 0x574, d2 = side ? 0xBF4 : 0x5B4;
    if (held == 0) {
        *cell(d0) = 0x8000; *cell(d1) = 0x8000; *cell(d2) = 0x8000;
    } else {
        v = S16(f + 366);
        if (!(mod32(v) < 24)) { *cell(lvl) = (uint16_t)(*cell(lvl) - 1); v = S16(f + 366); }
        if (!(mod32(v) < 16)) { *cell(lvl) = (uint16_t)(*cell(lvl) + 1); v = S16(f + 366); }
        if (!(mod32(v) < 8)) { *cell(lvl) = (uint16_t)(*cell(lvl) + 1); v = S16(f + 366); }
        DIV(c, (uint32_t)(v * 100), 0x5A0);
        int32_t q = (int32_t)c->lo;
        c->r[17] = HUD + d0;
        if (!(q < 10)) {
            *cell(d0) = 0x317; *cell(d1) = 0x317; *cell(d2) = 0x321;
        } else {
            *cell(d0) = (uint16_t)(q + 0x30E);
            DIV(c, (uint32_t)(S16(f + 366) * 100), 0x90);
            DIV(c, c->lo, 10);
            c->r[17] = HUD + d2;
            *cell(d1) = (uint16_t)(c->hi + 0x30E);
            c->r[31] = side ? 0x08891868u : 0x088917B8u; f_08821368(c);
            uint32_t r = c->r[2] & 0xF;
            if (!((int32_t)r < 10)) r -= 10;
            *cell(d2) = (uint16_t)(r + 0x318);
        }
    }
    ARG(0) = 0; ARG(1) = *W(PALS + 64) + c->r[19]; ARG(2) = 0; ARG(3) = side ? 0x11 : 0xF; c->r[8] = 1;
    c->r[31] = side ? 0x08891910u : 0x088918F4u; f_0880AEA8(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x40;
}
