/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 27 - 34
   instructions: the select / VS screens' objects arriving (GAME +20 bits), the palettes uploaded each
   frame, the pad records reset, a name panel's set-up, the CPU's script count. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_088718DC(Cpu *c); void f_0887B6B8(Cpu *c); void f_0887B7CC(Cpu *c); void f_0887B1A8(Cpu *c);
void f_089E11C8(Cpu *c); void f_089E1254(Cpu *c); void f_08888EE4(Cpu *c); void f_0880AEA8(Cpu *c);
void f_08804EA4(Cpu *c); void f_08804EB0(Cpu *c); void f_0889EAE0(Cpu *c); void f_0889CEA4(Cpu *c);
void f_0889EF00(Cpu *c); void f_0889CE64(Cpu *c); void f_0889F7F4(Cpu *c); void f_088A0594(Cpu *c);
void f_088A053C(Cpu *c);

static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WRAP20(back, fn) wrap(c, 0x20, 16, back, fn)
static uint8_t inc8(gaddr a) { uint8_t v = (uint8_t)(*B(a) + 1); *B(a) = v; return v; }
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
#define WITH_S01(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; c->r[16] = (o)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- screens' objects arriving */
/* (object): with BATTLE +81 set (objects arrive at once), `at_once`; otherwise when `arrived` says so:
   +5 on, a place from +114, and its bit (1 << +3 or +10) set in GAME +20. */
static void arrive(Cpu *c, void (*at_once)(Cpu *), uint32_t back1, void (*arrived)(Cpu *), uint32_t back2,
                   int kind) {
    gaddr o = ARG(0); WITH_S0(o);
    if (S8(BATTLE + 81) != 0) call_on(c, o, back1, at_once);
    else {
        call_on(c, o, back2, arrived);
        if (c->r[2] != 0) {
            inc8(o + 5);
            int16_t v = S16(o + 114);
            unsigned bit;
            if (kind == 0) { *H(o + 22) = (uint16_t)v; *H(o + 20) = 0; bit = *B(o + 3); }
            else if (kind == 1) { *H(o + 18) = (uint16_t)v; *H(o + 16) = 0; bit = *B(o + 10); }
            else { *W(o + 16) = (uint32_t)v << 16; bit = *B(o + 3); }
            *H(GAME + 20) = (uint16_t)((uint32_t)(int32_t)S16(GAME + 20) | (1u << (bit & 31)));
        }
    }
    END_S0();
}
SFA_FUNC(0889EB40, arrive_y) { arrive(c, f_0889EAE0, 0x0889EB68u, f_0889CEA4, 0x0889EB78u, 0); }
SFA_FUNC(0889EF94, arrive_x_10) { arrive(c, f_0889EF00, 0x0889EFBCu, f_0889CE64, 0x0889EFCCu, 1); }
SFA_FUNC(0889F76C, arrive_x) { arrive(c, f_0889F7F4, 0x0889F794u, f_0889CE64, 0x0889F7A4u, 2); }

/* (object): 0x08888EE4 in GAME mode 2 when the record at 0x0950FF40 is 1; and again unless its +10 is 3. */
SFA_FUNC(08888E70, run_by_mode_2) {
    gaddr o = ARG(0), r = 0x0950FF40u;
    WITH_S01(o);
    c->r[17] = 0x09510000u;
    if (S8(GAME + 114) == 2 && *B(r) == 1) call_on(c, o, 0x08888EB4u, f_08888EE4);
    if (*B(r + 10) != 3) call_on(c, o, 0x08888ED0u, f_08888EE4);
    END_S01();
}
/* (object, from): set up from `from` (+14, +15, +24, +38; +86 = 8, +11 / +36 / +148 cleared), +4 on;
   its speed (0x088A0594) and +36 = 0x18 + its fighter's (from +92) side (0x088A053C). */
SFA_FUNC(088A02EC, name_panel_setup) {
    gaddr o = ARG(0), src = ARG(1);
    WITH_S01(o);
    inc8(o + 4);
    *W(o + 148) = 0;
    *H(o + 86) = 8;
    c->r[17] = *W(src + 92);
    *B(o + 11) = 0;
    *B(o + 14) = *B(src + 14);
    *B(o + 15) = *B(src + 15);
    *H(o + 24) = *H(src + 24);
    int16_t b38 = S16(src + 38);
    *H(o + 36) = 0;
    *H(o + 38) = (uint16_t)b38;
    ARG(1) = (uint32_t)(int32_t)b38;
    call_on(c, o, 0x088A034Cu, f_088A0594);
    ARG(1) = (uint32_t)(S8(c->r[17] + 337) + 0x18) & 0xFFFF;
    call_on(c, o, 0x088A0360u, f_088A053C);
    END_S01();
}

/* ---------------------------------------------------------------- palettes and pads */
/* Each frame: when the palettes changed (0x092E0980), the 0x800 colours at 0x092DF980 copied to
   0x092E12E0 and marked (0x092E12D8); when marked, uploaded (0x089E1254 (0, 0x800, 0x092E12E0)). */
SFA_FUNC(0880B74C, palettes_upload) {
    WITH_S0(0x092E0000u);
    if (*W(0x092E0980u) != 0) {
        *W(0x092E0980u) = 0;
        *W(0x092E12D8u) = 1;
        ARG(0) = 0x092E12E0u; ARG(1) = 0x092DF980u; ARG(2) = 0x800;
        c->r[31] = 0x0880B78Cu; f_089E11C8(c);
    }
    if (*W(0x092E12D8u) != 0) {
        *W(0x092E12D8u) = 0;
        ARG(0) = 0; ARG(1) = 0x800; ARG(2) = 0x092E12E0u;
        c->r[31] = 0x0880B7ACu; f_089E1254(c);
    } else ARG(2) = 0x092E0000u;
    END_S0();
}
/* The three pad records at 0x08E75BA0 (0x2C apart) reset (0x08804EB0) with +40 / +41 = 0x18 / 6, and
   the word at 0x08AD8FE0 cleared. */
SFA_FUNC(08804ECC, pads_reset) {
    WITH_S01(0x08E75BA0u);
    static const uint32_t backs[3][2] = { { 0x08804EECu, 0x08804EFCu }, { 0x08804F08u, 0x08804F18u }, { 0x08804F24u, 0x08804F34u } };
    for (int k = 0; k < 3; k++) {
        gaddr r = 0x08E75BA0u + 0x2C * (uint32_t)k;
        if (k == 1) c->r[17] = r;
        if (k == 2) c->r[16] = r;
        call_on(c, r, backs[k][0], f_08804EB0);
        ARG(0) = r; ARG(1) = 0x18; ARG(2) = 6;
        c->r[31] = backs[k][1]; f_08804EA4(c);
    }
    ARG(0) = 0x08AE0000u;
    *W(0x08AD8FE0u) = 0;
    END_S01();
}
/* With GAME +17 set: six palettes into slots 0 on, from 0x08A317D8 while 0x09525DE3 is set, else from
   0x08A34C58. */
SFA_FUNC(088B1B00, palettes_by_game_17) {
    if (((uint32_t)(int32_t)S16(GAME + 16) & 0xFF00) == 0) { ARG(0) = 0x09520000u; return; }
    int8_t v = S8(0x09525DE3u);
    ARG(0) = 1; ARG(2) = 0; ARG(3) = 0; c->r[8] = 6;
    if (v != 0) { ARG(1) = 0x08A317D8u; wrap(c, 0x30, 32, 0x088B1B50u, f_0880AEA8); }
    else { ARG(1) = 0x08A34C58u; wrap(c, 0x30, 32, 0x088B1B74u, f_0880AEA8); }
}

/* ---------------------------------------------------------------- fighters */
/* (fighter, other): +117 = 0xC unless +698, +702, the other's +19 or +588 is set; then 0x088718DC
   (fighter, other) instead. */
SFA_FUNC(08871848, mark_C_or_088718DC) {
    gaddr f = ARG(0), x = ARG(1);
    ARG(0) = x;
    if (S8(f + 698) != 0 || S8(f + 702) != 0 || *B(x + 19) != 0 || S8(f + 588) != 0) {
        ARG(0) = f; ARG(1) = x; ARG(2) = x;
        WRAP20(0x08871898u, f_088718DC);
        return;
    }
    ARG(0) = 0xC;
    *B(f + 117) = 0xC;
}
/* (fighter): 0x0887B7CC when 0x0887B6B8 says so; else the script count +506 down, its first script
   and command cleared at 0. */
SFA_FUNC(0887A468, cpu_script_count) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887A47Cu, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887A48Cu, f_0887B7CC);
    else {
        int16_t v = S16(f + 506);
        if (v == 0) call_on(c, f, 0x0887A4A4u, f_0887B1A8);
        else {
            *H(f + 506) = (uint16_t)(v - 1);
            if (S16(f + 506) == 0) call_on(c, f, 0x0887A4C4u, f_0887B1A8);
        }
    }
    END_S0();
}
