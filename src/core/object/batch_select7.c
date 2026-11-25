/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 53 - 60
   instructions: the VS screen's name and portrait objects arriving, the select screen's textures
   freed on the way in, the ticker's palette cycle, an intro banner, a hit's reaction chosen by kind.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0880AEA8(Cpu *c); void f_088B1D10(Cpu *c); void f_088969BC(Cpu *c); void f_08855D80(Cpu *c);
void f_0882196C(Cpu *c); void f_088215C4(Cpu *c); void f_0889FC84(Cpu *c); void f_08871508(Cpu *c);
void f_08871564(Cpu *c); void f_088715C0(Cpu *c); void f_08871678(Cpu *c); void f_088716D4(Cpu *c);
void f_08871730(Cpu *c); void f_0889CFE8(Cpu *c); void f_088988FC(Cpu *c); void f_0889CF90(Cpu *c);
void f_0889FBF8(Cpu *c); void f_0889EAE0(Cpu *c); void f_089E0C24(Cpu *c);

#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void game_bit(unsigned bit) { *H(GAME + 20) = (uint16_t)((uint32_t)(int32_t)S16(GAME + 20) | (1u << (bit & 31))); }

/* ---------------------------------------------------------------- VS screen */
/* (object): the VS name: step 0 waits +82 frames (none with BATTLE +81) then sets its GAME +20 bit
   (1 << +10); step 1 waits for GAME +20 bit 15, then +4 on (bit 2 set for +10 = 0). Every frame its two
   palette lists (+0x60, +0x68) step on and it is drawn. */
SFA_FUNC(0889E8E4, vs_name) {
    gaddr o = ARG(0); WITH_S0(o);
    uint8_t step = *B(o + 5);
    if (step == 0) {
        if (S8(BATTLE + 81) != 0) *B(o + 5) = 1;
        else {
            int16_t n = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)n;
            if (n == 0) { *B(o + 5) = (uint8_t)(step + 1); game_bit(*B(o + 10)); }
        }
    } else if (S16(GAME + 20) < 0) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        if (*B(o + 10) == 0) *H(GAME + 20) = (uint16_t)((uint32_t)(int32_t)S16(GAME + 20) | 4);
    }
    call2(c, o, o + 0x60, 0x0889E9A0u, f_0889FC84);
    call2(c, o, o + 0x68, 0x0889E9ACu, f_0889FC84);
    call_on(c, o, 0x0889E9B4u, f_088215C4);
    END_S0();
}
/* (object): the VS portrait: on its first frame its set-up (0x0889CFE8, x 0x78 or 0x108 by its fighter's
   side (0x088988FC), 0x0889CF90 (0x14), its pictures by the stage (0x0889FBF8)); then at once with
   BATTLE +81 (0x0889EAE0), or step 2 of the screen (GAME +20 high byte 2) moves it on. */
SFA_FUNC(0889E9F8, vs_portrait) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[18] = *B(o + 5); c->r[16] = o; c->r[17] = 0x092E0000u;
    if (c->r[18] == 0) {
        *B(o + 5) = 1;
        *W(o + 148) = 0;
        *H(o + 86) = 8;
        call2(c, o, 0, 0x0889EA3Cu, f_0889CFE8);
        call_on(c, *W(o + 92), 0x0889EA44u, f_088988FC);
        c->r[18] = c->r[2];
        *H(o + 18) = c->r[18] != 0 ? 0x108 : 0x78;
        call2(c, o, 0x14, 0x0889EA64u, f_0889CF90);
        ARG(2) = *H(BATTLE);
        call2(c, o, c->r[18] & 0xFFFF, 0x0889EA78u, f_0889FBF8);
    } else if (S8(BATTLE + 81) != 0) call_on(c, o, 0x0889EA98u, f_0889EAE0);
    else if (((uint32_t)(int32_t)S16(GAME + 20) & 0xFF00) == 0x200) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (banner): the intro banner: the stage's palette (0x63C + the byte at 0x08A16B34 by the stage) into
   slot 0x1E, 0x08855D80 (banner, 1, 0xC0, -0x3E), set up (+148 = 0xB, +156 = 0x08A16962, +7 = 0x1E,
   +4 on), its animation from 0x08A16A90 (2, or 0x1A with BATTLE +63), and drawn. */
SFA_FUNC(08854D28, intro_banner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o; c->r[17] = 0x092E0000u; c->r[18] = 0x1E;
    ARG(0) = *B(0x08A16B34u + (uint32_t)(int32_t)S16(BATTLE)) + 0x63Cu; ARG(1) = 0x1E; ARG(2) = 1;
    c->r[31] = 0x08854D6Cu; f_088969BC(c);
    ARG(0) = o; ARG(1) = 1; ARG(2) = 0xC0; ARG(3) = 0xFFFFFFC2u;
    c->r[31] = 0x08854D80u; f_08855D80(c);
    *W(o + 148) = 0xB;
    *W(o + 156) = 0x08A16962u;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 82) = 0;
    *B(o + 11) = 0;
    *B(o + 7) = 0x1E;
    *B(o + 79) = 0;
    uint32_t k = S8(BATTLE + 63) != 0 ? 0x1A : 2;
    ARG(1) = *W(0x08A16A90u + k * 4);
    call_on(c, o, 0x08854DE0u, f_0882196C);
    call_on(c, o, 0x08854DE8u, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
/* (object): the ticker's palette cycle: with GAME +17 set, palette 0x08A30718 / 0x08A30A98 (by
   0x09525DE3) + 32 x +10 into slot 0x20, +10 cycling 0 - 27; from GAME +16's low byte 24 on, +4 on;
   then 0x088B1D10. */
SFA_FUNC(088B1C3C, ticker_palette) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 36) = s1; *W(c->r[29] + 32) = s0; *W(c->r[29] + 40) = ra;
    c->r[17] = GAME; c->r[16] = o;
    if ((((uint32_t)(int32_t)S16(GAME + 16) & 0xFF00) >> 8) != 0) {
        uint32_t idx = (uint32_t)*B(o + 10) << 5;
        ARG(1) = (S8(0x09525DE3u) != 0 ? 0x08A30718u : 0x08A30A98u) + idx;
        ARG(0) = 1; ARG(2) = 0; ARG(3) = 0x20; c->r[8] = 1;
        c->r[31] = 0x088B1CB4u; f_0880AEA8(c);
        uint32_t n = *B(o + 10) + 1u;
        *B(o + 10) = (uint8_t)n;
        if (!((int32_t)(n & 0xFF) < 28)) *B(o + 10) = 0;
    }
    if (!(((uint32_t)(int32_t)S16(GAME + 16) & 0xFF) < 24)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    call_on(c, o, 0x088B1CFCu, f_088B1D10);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* ---------------------------------------------------------------- select screen textures */
/* (x): unless already freed (0x08E75C50 = 0x8080), the select screen's textures 0xA0 - 0xC3 (and
   0x90 - 0x93 while 0x08E75A28 is set) freed; 0xD0 - 0xDA unless 0x08E75C52 is 0x8080; then 0x08E75C52
   = x and 0x08A055DC = 0x092E8A00. */
SFA_FUNC(088139CC, select_textures_in) {
    uint32_t x = ARG(0);
    uint32_t ra = c->r[31], s[6];
    c->r[29] -= 0x30;
    for (int k = 0; k < 6; k++) { s[k] = c->r[16 + k]; *W(c->r[29] + 16 + 4 * (uint32_t)k) = s[k]; }
    *W(c->r[29] + 40) = ra;
    c->r[21] = 0x08E70000u; c->r[20] = 0x8080; c->r[16] = x; c->r[17] = 0x08E70000u;
    if (*H(0x08E75C50u) != 0x8080) {
        c->r[18] = 0x08E75A18u;
        for (c->r[19] = 0; (int32_t)c->r[19] < 36; c->r[19]++) call_on(c, c->r[19] + 0xA0, 0x08813A18u, f_089E0C24);
        if (*W(0x08E75A28u) != 0) {
            for (c->r[19] = 0; (int32_t)c->r[19] < 4; c->r[19]++) call_on(c, c->r[19] + 0x90, 0x08813A40u, f_089E0C24);
            *W(0x08E75A28u) = 0;
        }
        *H(0x08E75C50u) = 0x8080;
    }
    if (*H(0x08E75C52u) != 0x8080) {
        for (c->r[19] = 0; (int32_t)c->r[19] < 11; c->r[19]++) call_on(c, c->r[19] + 0xD0, 0x08813A74u, f_089E0C24);
    }
    *H(0x08E75C52u) = (uint16_t)x;
    ARG(0) = 0x092E8A00u;
    *W(0x08A055DCu) = 0x092E8A00u;
    for (int k = 0; k < 6; k++) c->r[16 + k] = s[k];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* ---------------------------------------------------------------- hits */
/* (fighter, hit): the reaction by the hit's kind (+13: 0, 1, else) and whether +722 is below +724. */
SFA_FUNC(08871420, hit_reaction) {
    gaddr f = ARG(0), x = ARG(1);
    int lower = (int32_t)*H(f + 722) < (int32_t)*H(f + 724);
    uint8_t kind = *B(x + 13);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20; *W(c->r[29] + 16) = ra;
    ARG(0) = f; ARG(1) = x; ARG(2) = x;
    if (lower) {
        if (kind == 0) { c->r[31] = 0x08871460u; f_08871508(c); }
        else if (kind == 1) { c->r[31] = 0x08871484u; f_08871564(c); }
        else { c->r[31] = 0x0887149Cu; f_088715C0(c); }
    } else {
        if (kind == 0) { c->r[31] = 0x088714C0u; f_08871678(c); }
        else if (kind == 1) { c->r[31] = 0x088714E4u; f_088716D4(c); }
        else { c->r[31] = 0x088714FCu; f_08871730(c); }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
