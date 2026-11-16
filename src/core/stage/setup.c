/* A stage's own setup, when it is loaded: the animators (animators.c) that cycle its palettes or rewrite
 * its tile maps, and which of its palettes go in which slots (0x089E1234).
 *
 * 0x08819A20 (from the fight's setup and the screens that show a stage) runs 0x08815818, then the stage's
 * own routine from the table 0x08A08A14 if it has one (23 of the 42 numbers). Most make palette
 * animators: from 0x08823074, with a script (+8), its palette lists (+12), the source palettes (+16,
 * the stage's at 0x092EEA00) and a step routine (+20), set going by 0x0881F1FC; and map animators: from
 * 0x088230B4, with the tile size (+1), the script (+12), the place in the map (+8 / +10) and a step,
 * written once by 0x08819A68.
 */
#include "../decomp.h"

void f_08823074(Cpu *c);   /* a new palette animator, or 0 */
void f_088230B4(Cpu *c);   /* a new map animator, or 0 */
void f_08815818(Cpu *c);
void f_08813818(Cpu *c);
void f_089E11C8(Cpu *c);   /* copies (dst, src, halfwords) */
void f_089E1234(Cpu *c);   /* sets palette slot a0 from palette a1 */
void f_08821368(Cpu *c);   /* game_random (fight.c) */
void f_0881F1FC(Cpu *c); void f_08819A68(Cpu *c); void f_0881F308(Cpu *c); void f_0881F3E4(Cpu *c);
void f_0881F5D4(Cpu *c);
void rt_call_indirect(Cpu *c, uint32_t addr);

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t   S8(gaddr a) { return (int8_t)*B(a); }
static inline int32_t   S16(gaddr a) { return (int16_t)*H(a); }

#define BATTLE      0x092E1228u
#define LAYER_2     0x092E8940u
#define SOURCE      0x092EEA00u   /* the stage's palettes (its maps + 0x6000) */
#define ROUTINES    0x08A08A14u   /* u32[42]: each stage's setup, or 0 */
#define CYCLE       0x0881EEF4u   /* the palette animators' steps (animators.c) */
#define CYCLE_RANDOM 0x0881EF38u
#define SLOTS_CYCLE 0x0881EFB4u
#define CYCLE_WAIT  0x0881F01C
#define CYCLE_FRAME 0x0881F060u
#define FOLLOW_LAYER 0x0881F0D0u
#define PAIRS_CYCLE 0x0881F114u
#define AFTER_START 0x0881F148u
#define MAP_CYCLE   0x08819C2Cu   /* the map animators' steps */
#define MAP_WATCH   0x08819C74u
#define MAP_NONE    0x08820768u

typedef struct { uint32_t make, init; } Back;   /* the original's return addresses */

static uint32_t make(Cpu *c, GuestFunc f, uint32_t back) { c->r[31] = back; f(c); return c->r[2]; }

/* A palette animator: script, lists, step; set going. */
static gaddr palette_animator(Cpu *c, uint32_t script, uint32_t lists, uint32_t source, uint32_t step, Back b) {
    gaddr a = make(c, f_08823074, b.make);
    if (a == 0) return 0;
    *W(a + 8) = script; *W(a + 12) = lists; *W(a + 16) = source; *W(a + 20) = step;
    c->r[4] = a; c->r[31] = b.init; f_0881F1FC(c);
    return a;
}

/* A map animator: tile size, script, place, step; written once. */
static gaddr map_animator(Cpu *c, int big, uint32_t script, int32_t x, int32_t y, uint32_t step, Back b) {
    gaddr a = make(c, f_088230B4, b.make);
    if (a == 0) return 0;
    *B(a + 1) = (uint8_t)big;
    *W(a + 12) = script;
    *H(a + 8) = (uint16_t)x; *H(a + 10) = (uint16_t)y;
    *W(a + 16) = step;
    c->r[4] = a; c->r[31] = b.init; f_08819A68(c);
    return a;
}

static void pal(Cpu *c, uint32_t slot, uint32_t from, uint32_t back) {
    c->r[4] = slot; c->r[5] = from; c->r[31] = back; f_089E1234(c);
}

/* slots first.. get palettes from[0..] (a table of pairs) */
static void pals(Cpu *c, const uint8_t (*p)[2], int n, uint32_t back) {
    for (int i = 0; i < n; i++) pal(c, p[i][0], p[i][1], back);
}

/* ---- 0x08819A20: the stage's setup ---- */
SFA_FUNC(08819A20, stage_setup) {
    c->r[29] -= 0x10;
    c->r[31] = 0x08819A30; f_08815818(c);
    uint32_t fn = *W(ROUTINES + (uint32_t)S16(BATTLE) * 4u);
    if (fn != 0) { c->r[31] = 0x08819A5C; rt_call_indirect(c, fn); }
    c->r[29] += 0x10;
}

/* ---- 0x08819984: (a0): 0x08813818 with the stage (0 if the stage's 0x08A089E4 byte is 0) ---- */
SFA_FUNC(08819984, stage_load_with) {
    c->r[29] -= 0x20;
    if (*B(0x08A089E4u + (uint32_t)S16(BATTLE)) == 0) *H(BATTLE) = 0;
    c->r[5] = ARG(0);
    c->r[4] = (uint32_t)S16(BATTLE);
    c->r[31] = 0x088199C0; f_08813818(c);
    c->r[29] += 0x20;
}

/* ---- the stages' own ---- */

SFA_FUNC(0881F820, stage_setup_ryu) {                                  /* 0x00 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A094B0u), 0x08A094C4u, SOURCE, CYCLE, (Back){ 0x0881F830, 0x0881F870 });
    pal(c, 0xA5, 0x20, 0x0881F87C); pal(c, 0xA6, 0x21, 0x0881F888);
    c->r[29] += 0x20;
}

SFA_FUNC(0881F894, stage_setup_ken) {                                  /* 0x01: the crowd watches */
    c->r[29] -= 0x20;
    map_animator(c, 0, 0x08A094E8u, 0x160, 0x90, MAP_WATCH, (Back){ 0x0881F8A4, 0x0881F8DC });
    c->r[29] += 0x20;
}

SFA_FUNC(0881F8E8, stage_setup_akuma) {                                /* 0x02 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A09508u), 0x08A09520u, SOURCE, CYCLE, (Back){ 0x0881F8F8, 0x0881F938 });
    pal(c, 0xA4, 0x21, 0x0881F944); pal(c, 0xA5, 0x20, 0x0881F950);
    c->r[29] += 0x20;
}

SFA_FUNC(0881F95C, stage_setup_chunli) {                               /* 0x04: six map animators */
    c->r[29] -= 0x30;
    for (int i = 0; i < 6; i++)
        map_animator(c, 1, *W(0x08A0A170u), 0x100 + 0x80 * i, 0x80, MAP_CYCLE, (Back){ 0x0881F9A0, 0x0881F9C8 });
    c->r[29] += 0x30;
}

SFA_FUNC(0881F9FC, stage_setup_sodom) {                                /* 0x06 */
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 8; i++)
        palette_animator(c, *W(0x08A0A234u + 4 * i), *W(0x08A0A390u + 4 * i), SOURCE, CYCLE, (Back){ 0x0881FA48, 0x0881FA70 });
    for (uint32_t i = 0; i < 22; i++) pal(c, 0xA5 + i, 0x20 + i, 0x0881FA94);
    pal(c, 0xBB, 0x20, 0x0881FAB0);
    map_animator(c, 0, *W(0x08A0A470u), 0x170, 0x50, MAP_CYCLE, (Back){ 0x0881FAB8, 0x0881FAF4 });
    c->r[29] += 0x30;
}

SFA_FUNC(0881FB14, stage_setup_guy) {                                  /* 0x07 */
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 2; i++)
        palette_animator(c, *W(0x08A0A4B0u + 4 * i), 0x08A0A4C0u, SOURCE, CYCLE, (Back){ 0x0881FB60, 0x0881FB84 });
    c->r[29] += 0x30;
}

SFA_FUNC(0881FBB4, stage_setup_birdie) {                               /* 0x08 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0A4F4u), 0x08A0A504u, SOURCE, CYCLE_RANDOM, (Back){ 0x0881FBC4, 0x0881FC04 });
    c->r[29] += 0x20;
}

SFA_FUNC(0881FC10, stage_setup_rose) {                                 /* 0x09 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0A528u), 0x08A0A53Cu, SOURCE, CYCLE, (Back){ 0x0881FC20, 0x0881FC60 });
    pal(c, 0xA5, 0x20, 0x0881FC6C); pal(c, 0xA6, 0x21, 0x0881FC78);
    c->r[29] += 0x20;
}

SFA_FUNC(0881FC84, stage_setup_bison) {                                /* 0x0A, 0x1E */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0A580u), 0x08A0A59Cu, SOURCE + 0x200, CYCLE_FRAME, (Back){ 0x0881FC94, 0x0881FCD4 });
    pal(c, 0xA6, 0x20, 0x0881FCE0); pal(c, 0xA7, 0x21, 0x0881FCEC); pal(c, 0xA8, 0x21, 0x0881FCF8);
    for (uint32_t s = 0xA9; s <= 0xB9; s++) pal(c, s, 0x22, 0x0881FD04 + (s - 0xA9) * 12);
    c->r[29] += 0x20;
}

SFA_FUNC(0881FDD0, stage_setup_dan) {                                  /* 0x0D */
    c->r[29] -= 0x20;
    map_animator(c, 0, *W(0x08A0A668u), 0x1F0, 0x80, MAP_CYCLE, (Back){ 0x0881FDE0, 0x0881FE1C });
    c->r[29] += 0x20;
}

SFA_FUNC(0881FE28, stage_setup_rolento) {                              /* 0x0E */
    c->r[29] -= 0x20;
    c->r[4] = 0x092DF980u + 0x400; c->r[5] = SOURCE; c->r[6] = 0xC0; c->r[31] = 0x0881FE58;
    f_089E11C8(c);
    gaddr a = make(c, f_08823074, 0x0881FE60);
    if (a != 0) {                                     /* palette slots rotating (0x0881F5D4) */
        *W(a + 8) = 0x08A0A670u;
        *B(a + 2) = (uint8_t)S8(0x08A0A670u);
        *B(a + 4) = *B(*W(a + 8) + 2);
        *B(a + 5) = (uint8_t)S8(*W(a + 8) + 3);
        *W(a + 20) = SLOTS_CYCLE;
        c->r[4] = *B(a + 4); c->r[31] = 0x0881FEAC; f_0881F5D4(c);
    }
    palette_animator(c, 0x08A0A6D8u, 0x08A0A6F0u, SOURCE, CYCLE_WAIT, (Back){ 0x0881FEB4, 0x0881FEE8 });
    pal(c, 0xBD, 0x3A, 0x0881FEF4);
    c->r[29] += 0x20;
}

SFA_FUNC(0881FF04, stage_setup_dhalsim) {                              /* 0x0F */
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 4; i++)
        map_animator(c, 0, *W(0x08A0A8ACu + 4 * i), S16(0x08A0A8BCu + 4 * i), S16(0x08A0A8BEu + 4 * i), MAP_CYCLE,
                     (Back){ 0x0881FF40, 0x0881FF74 });
    c->r[29] += 0x30;
}

SFA_FUNC(0881FFA4, stage_setup_zangief) {                              /* 0x10 */
    static const uint8_t P[][2] = {
        {0xA3,0x20},{0xA4,0x21},{0xA5,0x22},{0xA6,0x23},{0xA7,0x24},{0xA8,0x25},{0xA9,0x26},{0xAA,0x26},
        {0xAB,0x27},{0xAC,0x28},{0xAD,0x29},{0xAE,0x2A},{0xAF,0x2B},{0xB0,0x2B},{0xB1,0x2C},{0xB2,0x2C},
        {0xB3,0x2C},{0xB4,0x2C},{0xB5,0x2D},{0xB6,0x2D},{0xB7,0x2D},{0xB8,0x2D},{0xB9,0x2E},{0xBA,0x2F},
        {0xBB,0x30},{0xBC,0x31},{0xBD,0x32},{0xBE,0x33},{0xBF,0x34},{0xC0,0x35},{0xC1,0x36},
    };
    c->r[29] -= 0x30;
    gaddr a = make(c, f_08823074, 0x0881FFC4);
    if (a != 0) {                                     /* palettes by layer 2's frame */
        *W(a + 8) = *W(0x08A0A8E0u); *W(a + 12) = *W(0x08A0A90Cu);
        *W(a + 16) = SOURCE; *W(a + 20) = FOLLOW_LAYER;
        *B(a + 6) = (uint8_t)S8(LAYER_2 + 124);
        *B(a + 7) = (uint8_t)S8(a + 6);
        c->r[4] = a; c->r[31] = 0x0882002C; f_0881F308(c);
    }
    for (uint32_t i = 0; i < 2; i++) {               /* two pair scripts */
        gaddr p = make(c, f_08823074, 0x08820044);
        if (p == 0) continue;
        *W(p + 16) = 0x092E8A00u + 0x66C0;
        *W(p + 8) = *W(0x08A0AC50u + 4 * i);
        *W(p + 20) = PAIRS_CYCLE;
        *B(p + 2) = (uint8_t)*H(*W(p + 8));
        c->r[4] = p; c->r[31] = 0x08820074; f_0881F3E4(c);
    }
    pals(c, P, (int)(sizeof P / sizeof P[0]), 0x08820090);
    map_animator(c, 1, *W(0x08A0ABF0u), 0x1E0, 0xA0, MAP_CYCLE, (Back){ 0x08820200, 0x0882023C });
    c->r[29] += 0x30;
}

SFA_FUNC(08820258, stage_setup_gen) {                                  /* 0x11: 31 in 32 fights */
    c->r[29] -= 0x20;
    c->r[31] = 0x08820268; f_08821368(c);
    if ((c->r[2] & 0x1F) != 0)
        map_animator(c, 0, *W(0x08A0AE40u), 0x320, 0x80, MAP_CYCLE, (Back){ 0x0882027C, 0x088202B8 });
    c->r[29] += 0x20;
}

SFA_FUNC(088202C4, stage_setup_balrog) {                               /* 0x15 */
    static const uint8_t P[][2] = {
        {0xA3,0x20},{0xA4,0x21},{0xA5,0x22},{0xA6,0x23},{0xA7,0x24},{0xA8,0x25},{0xA9,0x26},{0xAA,0x27},
        {0xAB,0x28},{0xAC,0x29},{0xAD,0x2A},{0xAE,0x2B},{0xAF,0x2C},{0xB0,0x2D},{0xB1,0x2E},{0xB2,0x2F},
        {0xB3,0x30},{0xB4,0x31},{0xB5,0x32},{0xB6,0x33},{0xB7,0x34},{0xB8,0x35},{0xB9,0x36},{0xBA,0x37},
        {0xBB,0x37},{0xBC,0x38},{0xBD,0x39},{0xBE,0x3A},{0xBF,0x3B},{0xC0,0x3C},{0xC1,0x3D},{0xC2,0x3D},
        {0xC3,0x3E},
    };
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 11; i++)
        palette_animator(c, *W(0x08A0AEF0u + 4 * i), *W(0x08A0B06Cu + 4 * i), SOURCE, CYCLE, (Back){ 0x08820310, 0x08820338 });
    pals(c, P, (int)(sizeof P / sizeof P[0]), 0x08820358);
    c->r[29] += 0x30;
}

SFA_FUNC(088204F8, stage_setup_cammy) {                                /* 0x16 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0B0B0u), 0x08A0B0E4u, SOURCE, CYCLE, (Back){ 0x0882050C, 0x08820550 });
    for (uint32_t i = 0; i < 4; i++) pal(c, 0xA7 + i, 0x20 + i, 0x08820560);
    map_animator(c, 0, *W(0x08A0D3DCu), 0x230, 0, MAP_CYCLE, (Back){ 0x08820578, 0x088205B0 });
    c->r[29] += 0x20;
}

SFA_FUNC(088205C0, stage_setup_ryu_2) {                                /* 0x17 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0D3E8u), 0x08A0D400u, SOURCE, CYCLE, (Back){ 0x088205D0, 0x08820610 });
    pal(c, 0xA4, 0x21, 0x0882061C); pal(c, 0xA5, 0x20, 0x08820628);
    c->r[29] += 0x20;
}

SFA_FUNC(08820634, stage_setup_honda) {                                /* 0x18 */
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 2; i++) {               /* they start with the round (0x0881F148) */
        gaddr a = make(c, f_08823074, 0x08820680);
        if (a == 0) continue;
        *B(a + 6) = 0;
        *W(a + 12) = 0x08A0D4F8u; *W(a + 8) = *W(0x08A0D46Cu + 4 * i);
        *W(a + 16) = SOURCE; *W(a + 20) = AFTER_START;
        c->r[4] = a; c->r[31] = 0x088206A8; f_0881F1FC(c);
    }
    pal(c, 0xA3, 0x20, 0x088206C4);
    for (uint32_t i = 0; i < 8; i++) pal(c, 0xA4 + i, 0x20 + i, 0x088206D4);
    gaddr m = map_animator(c, 0, *W(0x08A0D800u), 0x220, 0x50, MAP_NONE, (Back){ 0x088206EC, 0x0882072C });
    if (m != 0) *W(m + 12) += (uint32_t)(S8(m + 3) * 12);   /* ready for its next look */
    c->r[29] += 0x30;
}

SFA_FUNC(08820768, stage_map_nothing) { (void)c; }

SFA_FUNC(08820770, stage_setup_mika) {                                 /* 0x1A */
    c->r[29] -= 0x20;
    map_animator(c, 0, 0x08A0D838u, 0x1E0, 0x80, MAP_CYCLE, (Back){ 0x08820780, 0x088207B8 });
    c->r[29] += 0x20;
}

SFA_FUNC(08820820, stage_setup_deejay) {                               /* 0x22 */
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 4; i++)
        palette_animator(c, *W(0x08A0D90Cu + 4 * i), *W(0x08A0D9CCu + 4 * i), SOURCE, CYCLE, (Back){ 0x08820878, 0x088208A0 });
    for (uint32_t i = 0; i < 7; i++) pal(c, 0xA5 + i, 0x20 + i, 0x088208C4);
    for (uint32_t i = 0; i < 2; i++)
        map_animator(c, 0, *W(0x08A0DDB4u + 4 * i), 0x220 + 0x30 * (int32_t)i, 0x80, MAP_CYCLE, (Back){ 0x088208F4, 0x0882091C });
    c->r[29] += 0x30;
}

SFA_FUNC(08820954, stage_setup_akuma_2) {                              /* 0x24 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0DDC8u), 0x08A0DDE0u, SOURCE, CYCLE, (Back){ 0x08820964, 0x088209A4 });
    pal(c, 0xA4, 0x21, 0x088209B0); pal(c, 0xA5, 0x20, 0x088209BC);
    c->r[29] += 0x20;
}

SFA_FUNC(088209C8, stage_setup_karin_2) {                              /* 0x29 */
    c->r[29] -= 0x20;
    palette_animator(c, *W(0x08A0DDF0u), 0x08A0DDF8u, SOURCE, CYCLE, (Back){ 0x088209D8, 0x08820A18 });
    map_animator(c, 0, *W(0x08A0DEB8u), 0x1F0, 0x80, MAP_CYCLE, (Back){ 0x08820A20, 0x08820A5C });
    c->r[29] += 0x20;
}

/* ---- 0x0881F6A0: the fighters' palettes for the layout (BATTLE + 56: which of the four fighters
   play), each by 0x0881F794 ---- */
void f_0880AEA8(Cpu *c);   /* loads palettes: (0, source, 0, first slot, count) */
#define FIGHTER(n) (0x092E3EA0u + 0x498u * (n))
#define FIGHTER_PALETTES 0x092D9E00u   /* u32[]: each character's palettes */

SFA_FUNC(0881F794, fighter_palettes) {
    gaddr f = ARG(0);
    c->r[29] -= 0x30;
    gaddr base = *W(FIGHTER_PALETTES + (uint32_t)(S8(f + 337) * 4));
    int32_t colour = S8(f + 376);
    c->r[4] = 0; c->r[6] = 0; c->r[7] = *B(f + 13);
    if (S8(f + 338) == 0x29) { c->r[5] = base + (uint32_t)(colour << 8); c->r[8] = 8; c->r[31] = 0x0881F7E4; }
    else { c->r[5] = base + (uint32_t)((colour << 7) + (colour << 5)); c->r[8] = 5; c->r[31] = 0x0881F814; }
    f_0880AEA8(c);
    c->r[29] += 0x30;
}

SFA_FUNC(0881F6A0, fighters_palettes) {
    static const uint8_t PLAYING[4][3] = { {0, 1, 0xFF}, {0, 2, 3}, {1, 2, 3}, {0, 1, 2} };
    static const uint32_t BACK[4][3] = {
        {0x0881F6DC, 0x0881F6E8, 0}, {0x0881F714, 0x0881F720, 0x0881F72C},
        {0x0881F744, 0x0881F750, 0x0881F75C}, {0x0881F770, 0x0881F77C, 0x0881F788},
    };
    int32_t layout = S16(BATTLE + 56);
    if (layout < 0 || layout > 3) return;
    c->r[29] -= 0x20;
    for (int i = 0; i < 3 && PLAYING[layout][i] != 0xFF; i++) {
        c->r[4] = FIGHTER(PLAYING[layout][i]); c->r[31] = BACK[layout][i];
        f_0881F794(c);
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881CEF4 / 0x0881CE4C: a screen's backdrop palettes (the select and VS screens): slots
   0xE0 - 0xE5, and two from 0x08A08D08 by (a, b) into 0x092E0140 / 0x092E0160 ---- */
#define LAYER_1 0x092E88A0u
SFA_FUNC(0881CEF4, screen_backdrop_palettes) {
    uint32_t a = ARG(0), b = ARG(1);
    c->r[29] -= 0x20;
    *B(LAYER_1 + 123) = 6;
    static const uint8_t P[][2] = { {0xE0,0x3E},{0xE1,0x3E},{0xE2,0x3E},{0xE3,0x3E},{0xE4,0x3F},{0xE5,0x3F} };
    for (int i = 0; i < 6; i++) pal(c, P[i][0], P[i][1], 0x0881CF2C + (uint32_t)i * 12);
    uint32_t off = ((a << 5) + (b << 4)) * 2;
    c->r[4] = 0x092DF980u + 0x7C0; c->r[5] = off + 0x08A08D08u; c->r[6] = 0x10; c->r[31] = 0x0881CF98; f_089E11C8(c);
    c->r[4] = 0x092DF980u + 0x7E0; c->r[5] = off + 0x08A08D08u + 0x160; c->r[6] = 0x10; c->r[31] = 0x0881CFAC; f_089E11C8(c);
    *W(0x092E0980u) = 1;
    c->r[29] += 0x20;
}

SFA_FUNC(0881CE4C, screen_backdrop) {
    c->r[29] -= 0x20;
    *B(LAYER_1 + 123) = 6; *B(LAYER_1 + 122) = 2; *B(LAYER_2 + 122) = 1;
    *B(LAYER_1 + 155) = 0; *B(LAYER_2 + 155) = 0;
    *W(LAYER_2 + 16) = 0x400000; *W(LAYER_1 + 16) = 0x400000;
    *W(LAYER_2 + 20) = 0xFFF00000u; *W(LAYER_1 + 20) = 0xFFF00000u;
    c->r[31] = 0x0881CEA0; f_0881CEF4(c);                 /* a0 / a1 passed on */
    static const uint8_t P[][2] = { {0xE0,0x3E},{0xE1,0x3E},{0xE2,0x3E},{0xE3,0x3E},{0xE4,0x3F},{0xE5,0x3F} };
    for (int i = 0; i < 6; i++) pal(c, P[i][0], P[i][1], 0x0881CEAC + (uint32_t)i * 12);
    c->r[29] += 0x20;
}
