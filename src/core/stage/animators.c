/* A stage's animators: small records (made by the stage's setup, setup.c) that, each frame, cycle some of
 * the stage's palettes or rewrite part of a layer's tile map.
 *
 * An animator (0x40-byte record from 0x08823074 / 0x088230B4): +1 the map's tile size (0: 16 pixels,
 * else 32), +2 frames left, +3 / +4 / +5 / +6 / +7 its state, +8 the current entry of its script (palette
 * animators: 4-byte entries {frames, +3, list index, step}; map animators: x / y at +8 / +10), +12 its lists
 * (palette: a list of {palette slot, source | 0x80 last} pairs per index; map: 12-byte entries {+2, +3, the
 * map, columns, rows, the words}), +16 the source (palettes, 32 bytes each), +20 its step routine.
 *
 * Palettes are copied into the game's palette table 0x092DF980 (32 bytes each) through 0x089E11C8, and
 * 0x092E0980 set so they are sent to the GE.
 */
#include "../decomp.h"

void f_089E11C8(Cpu *c);   /* copies (dst, src, halfwords) */
void f_089E1234(Cpu *c);   /* sets palette slot a0 from palette a1 */
void f_08821368(Cpu *c);   /* game_random (fight.c) */
void f_08819A68(Cpu *c); void f_0881F1FC(Cpu *c); void f_0881F308(Cpu *c); void f_0881F3E4(Cpu *c);
void f_0881F48C(Cpu *c); void f_0881F5D4(Cpu *c);

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t   S8(gaddr a) { return (int8_t)*B(a); }
static inline int32_t   S16(gaddr a) { return (int16_t)*H(a); }

#define PALETTES  0x092DF980u   /* the game's palettes, 32 bytes each */
#define PAL_DIRTY 0x092E0980u   /* u32: 1 = send the palettes */
#define BATTLE    0x092E1228u
#define LAYER_1   0x092E88A0u
#define LAYER_2   0x092E8940u
#define MAPS      0x08A08ABCu   /* u32[]: tile maps by number */
#define P1        0x092E3EA0u   /* the fighters (+18 x) */
#define P2        0x092E4338u

static int anim_stopped(void) { return (S8(BATTLE + 37) | S8(BATTLE + 52)) != 0; }

static void copy_palettes(Cpu *c, uint32_t dst, uint32_t src, uint32_t n, uint32_t back) {
    c->r[4] = dst; c->r[5] = src; c->r[6] = n; c->r[31] = back;
    f_089E11C8(c);
}
static void call(Cpu *c, GuestFunc f, uint32_t a0, uint32_t back) { c->r[4] = a0; c->r[31] = back; f(c); }

/* Copies a palette list: {slot, source | 0x80 on the last} pairs, 16 colours each. */
static void palette_list(Cpu *c, gaddr list, gaddr source, uint32_t back) {
    for (;;) {
        uint32_t dst = PALETTES + ((uint32_t)*B(list) << 5);
        uint32_t src = source + ((uint32_t)(*B(list + 1) & 0x7F) << 5);
        copy_palettes(c, dst, src, 0x10, back);
        if (*B(list + 1) & 0x80) break;
        list += 2;
    }
    *W(PAL_DIRTY) = 1;
}

/* ---- 0x08819A68: a map animator writes its current entry's words into the map ---- */
SFA_FUNC(08819A68, stage_map_write) {
    gaddr a = ARG(0);
    gaddr e = *W(a + 12);
    gaddr src = *W(e + 8);
    *B(a + 2) = (uint8_t)S8(e);
    *B(a + 3) = (uint8_t)S8(e + 1);
    int small = S8(a + 1) == 0;                   /* 16-pixel tiles: 64 words a row; else 32: 32 words */
    int shift = small ? 4 : 5, row_bytes = small ? 8 : 7;
    for (int32_t row = 0; row < S16(e + 6); row++) {
        gaddr dst = *W(MAPS + (uint32_t)S16(e + 2) * 4u)
                  + ((uint32_t)((S16(a + 10) >> shift) + row) << row_bytes)
                  + ((uint32_t)(S16(a + 8) >> shift) << 2);
        for (int32_t i = 0; i < S16(e + 4); i++) {
            *W(dst) = *W(src);
            src += 4; dst += 4;
        }
    }
}

/* ---- 0x08819C2C: a map animator's step: every +2 frames, on to the next entry ---- */
SFA_FUNC(08819C2C, stage_map_cycle) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    uint8_t n = (uint8_t)(*B(a + 2) - 1);
    *B(a + 2) = n;
    if (n == 0) {
        *W(a + 12) += (uint32_t)(S8(a + 3) * 12);
        call(c, f_08819A68, a, 0x08819C68);
    }
    c->r[29] += 0x20;
}

/* ---- 0x08819C74: a map animator that turns to face the nearer fighter within 48 pixels (KEN's crowd) ---- */
SFA_FUNC(08819C74, stage_map_watch) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    int32_t x1 = S16(P1 + 18), x2 = S16(P2 + 18), x = S16(a + 8);
    int32_t step = S8(a + 3);
    int32_t d = (x2 < x1 ? x2 : x1) - x;
    int32_t want = d < 48 ? -1 : 1;
    if (want != step) {
        *W(a + 12) += (uint32_t)(step * 12);
        call(c, f_08819A68, a, 0x08819CF0);
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881F1FC: a palette animator's entry: its fields, then its list copied ---- */
SFA_FUNC(0881F1FC, stage_palette_entry) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    gaddr e = *W(a + 8);
    *B(a + 3) = (uint8_t)S8(e + 1);
    *B(a + 4) = *B(e + 2);
    *B(a + 5) = (uint8_t)S8(e + 3);
    *B(a + 2) = (uint8_t)S8(e);
    gaddr list = *W(*W(a + 12) + (uint32_t)*B(a + 4) * 4u);
    palette_list(c, list, *W(a + 16), 0x0881F29C);
    c->r[29] += 0x20;
}

/* One frame of a palette script: every +2 frames, step +5 entries and copy (0 if not yet). */
static int script_tick(gaddr a) {
    int8_t n = (int8_t)(S8(a + 2) - 1);
    *B(a + 2) = (uint8_t)n;
    if (n != 0) return 0;
    *W(a + 8) += (uint32_t)(S8(a + 5) * 4);
    return 1;
}

/* ---- 0x0881EEF4: a palette animator's step ---- */
SFA_FUNC(0881EEF4, stage_palette_cycle) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    if (script_tick(a)) call(c, f_0881F1FC, a, 0x0881EF2C);
    c->r[29] += 0x20;
}

/* ---- 0x0881EF38: the same, held (+3) until a 1-in-128 chance releases it ---- */
SFA_FUNC(0881EF38, stage_palette_cycle_random) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    if (S8(a + 3) != 0) {
        c->r[31] = 0x0881EF58; f_08821368(c);
        if ((c->r[2] & 0x7F) == 0) *B(a + 3) = 0;
    } else if (script_tick(a)) call(c, f_0881F1FC, a, 0x0881EFA4);
    c->r[29] += 0x20;
}

/* ---- 0x0881EFB4: a script that sets palette slots 0xA3 on (0x0881F5D4) ---- */
SFA_FUNC(0881EFB4, stage_palette_slots_cycle) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    int8_t n = (int8_t)(S8(a + 2) - 1);
    *B(a + 2) = (uint8_t)n;
    if (n == 0) {
        gaddr e = *W(a + 8) + (uint32_t)(S8(a + 5) * 4);
        *W(a + 8) = e;
        *B(a + 2) = (uint8_t)S8(e);
        *B(a + 4) = *B(e + 2);
        *B(a + 5) = (uint8_t)S8(e + 3);
        c->r[4] = (uint32_t)S8(a + 4); c->r[31] = 0x0881F010; f_0881F5D4(c);
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881F01C: a script whose entries may wait a random time (0x0881F48C) ---- */
SFA_FUNC(0881F01C, stage_palette_cycle_wait) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    if (script_tick(a)) call(c, f_0881F48C, a, 0x0881F054);
    c->r[29] += 0x20;
}

/* ---- 0x0881F060: BISON's: unless held (+3), a palette script whose +4 is also the fighters' layer's frame ---- */
SFA_FUNC(0881F060, stage_palette_cycle_frame) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    if (S8(a + 3) == 0) {
        if (script_tick(a)) call(c, f_0881F1FC, a, 0x0881F0B0);
        *B(LAYER_1 + 131) = *B(a + 4);
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881F0D0: ZANGIEF's: the palettes follow layer 2's frame (+124) ---- */
SFA_FUNC(0881F0D0, stage_palette_follow_layer) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    if (S8(a + 6) != S8(a + 7)) call(c, f_0881F308, a, 0x0881F0F4);
    *B(a + 6) = (uint8_t)S8(LAYER_2 + 124);
    c->r[29] += 0x20;
}

/* ---- 0x0881F114: every +2 frames, the next pair of 0x0881F3E4's script ---- */
SFA_FUNC(0881F114, stage_palette_pairs_cycle) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    int8_t n = (int8_t)(S8(a + 2) - 1);
    *B(a + 2) = (uint8_t)n;
    if (n == 0) { c->r[31] = 0x0881F13C; f_0881F3E4(c); }
    c->r[29] += 0x20;
}

/* ---- 0x0881F148: HONDA's: starts (+6) once the round has begun (BATTLE + 7) and the clock isn't
   stopped (+28); then a palette script, and once its +3 is set, the map animator 0x095177C8 rewritten ---- */
SFA_FUNC(0881F148, stage_palette_after_start) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    if (S8(a + 6) == 0) {
        if (S8(BATTLE + 7) != 0 && S8(BATTLE + 28) == 0) *B(a + 6) = 1;
    } else if (!anim_stopped() && script_tick(a)) {
        call(c, f_0881F1FC, a, 0x0881F1D4);
        if (S8(a + 3) != 0) {
            *B(a + 3) = 0;
            call(c, f_08819A68, 0x095177C8u, 0x0881F1EC);
        }
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881F308: ZANGIEF's palettes for layer 2's frame +6: four lists, sizes 0x08A09470, and the
   fighters' layer's frame from 0x08A09478 ---- */
SFA_FUNC(0881F308, stage_palette_by_frame) {
    gaddr a = ARG(0);
    c->r[29] -= 0x30;
    int32_t i = S8(a + 6);
    *B(a + 7) = (uint8_t)i;
    gaddr list = *W(*W(a + 12) + (uint32_t)(i * 4));
    *B(LAYER_1 + 131) = (uint8_t)*H(0x08A09478u + (uint32_t)(i * 2));
    for (int j = 0; j < 4; j++, list += 2) {
        uint32_t dst = PALETTES + ((uint32_t)*B(list) << 5);
        uint32_t src = *W(a + 16) + ((uint32_t)(*B(list + 1) & 0x7F) << 5);
        copy_palettes(c, dst, src, *H(0x08A09470u + (uint32_t)j * 2u), 0x0881F3A4);
    }
    *W(PAL_DIRTY) = 1;
    c->r[29] += 0x30;
}

/* ---- 0x0881F3E4: a script of {frames, slot, sources...} with 0x80 ending the sources: the next one ---- */
SFA_FUNC(0881F3E4, stage_palette_pair) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    gaddr t = *W(a + 8);
    uint32_t k = *B(a + 4);
    *B(a + 2) = (uint8_t)*H(t);
    uint32_t slot = *H(t + 2), src = *H(t + 4 + k * 2);
    if (src & 0x80) {                                    /* the end: back to the first */
        *B(a + 4) = 0;
        *B(a + 2) = (uint8_t)*H(t);
        k = *B(a + 4);
        slot = *H(t + 2);
        src = *H(t + 4 + k * 2);
    }
    *B(a + 4) = (uint8_t)(k + 1);
    copy_palettes(c, PALETTES + (slot << 5), *W(a + 16) + (src << 5), 0x10, 0x0881F474);
    *W(PAL_DIRTY) = 1;
    c->r[29] += 0x20;
}

/* ---- 0x0881F48C: an entry that waits a random time (when its +1 is set) before its list ---- */
SFA_FUNC(0881F48C, stage_palette_entry_wait) {
    gaddr a = ARG(0);
    c->r[29] -= 0x20;
    gaddr e = *W(a + 8);
    *B(a + 3) = (uint8_t)S8(e + 1);
    *B(a + 4) = *B(e + 2);
    *B(a + 5) = (uint8_t)S8(e + 3);
    if (S8(a + 3) != 0) {
        c->r[31] = 0x0881F4D8; f_08821368(c);
        *B(a + 2) = (uint8_t)(c->r[2] + (uint32_t)S8(a + 3) + 0xFF);
    } else {
        *B(a + 2) = (uint8_t)S8(e);
    }
    gaddr list = *W(*W(a + 12) + (uint32_t)*B(a + 4) * 4u);
    palette_list(c, list, *W(a + 16), 0x0881F568);
    c->r[29] += 0x20;
}

/* ---- 0x0881F5D4: palette slots 0xA3 - 0xBC from 0x08A09480 (s8 + 0x20), rotated to start at n ---- */
SFA_FUNC(0881F5D4, stage_palette_slots) {
    int32_t n = (int8_t)ARG(0);
    c->r[29] -= 0x30;
    uint32_t j = 0;
    for (int32_t k = n; k < 26; k++, j++) {
        c->r[4] = (uint32_t)(k + 0xA3); c->r[5] = (uint32_t)(S8(0x08A09480u + j) + 0x20); c->r[31] = 0x0881F620;
        f_089E1234(c);
    }
    for (int32_t k = 0; k < n; k++, j++) {
        c->r[4] = (uint32_t)(k + 0xA3); c->r[5] = (uint32_t)(S8(0x08A09480u + j) + 0x20); c->r[31] = 0x0881F658;
        f_089E1234(c);
    }
    c->r[29] += 0x30;
}

/* ---- 0x0881EBA8: (?, object, out): out = the object's y ---- */
SFA_FUNC(0881EBA8, stage_object_y) {
    c->r[4] = (uint32_t)S16(ARG(1) + 22);
    *H(ARG(2)) = (uint16_t)c->r[4];
}
