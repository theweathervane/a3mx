/* The ending (the task 0x089DE0E8 starts after Arcade's last fight, fight.c): its steps and the layers
 * its scenes are drawn from.
 *
 *   0x089DE2C0  the ending's step, once a frame: the frame count (+6), the scene script (0x089DF82C) and
 *               what it asked for (0x089DF624), the layers' places (0x089DE444), the timed sounds and
 *               the fade-out (0x089DE7C0), the timed lines (0x089DE8A4), the object lists, the camera
 *               layers (0x089DE52C), the screen and the layers drawn (0x0881A2C8); a button press moves
 *               the task on (with a sound unless +53)
 *   0x089DE390  a step that only draws, until GAME + 93 is cleared (0x0880AA74 first)
 *   0x089DE444  the three layers' places and scrolls set from the ending's records at +0xAA, +0xBC and
 *               +0xCE (when their +0 is set) into the camera layers 0x0951F950 + 0, 0x98, 0x130
 *   0x089DE4B4  one of them (0x089DE444)
 *   0x089DE7C0  three things once the frame count passes them: an object of kind 0 made (+50, a
 *               fade-out over the screen, 0x180 x 0x30), 0x0887C708 (+54) and the sound 0xB4 (+56)
 *   0x0881A2C8  the layers drawn: the fight's three layer records (0x092E8800, 0x092E88A0, 0x092E8940),
 *               each turned on (+7) the first frame and drawn while its camera layer (0x0951F950 +0,
 *               +0x98, +0x130) is set, by the ending's kind (0x09358BE0 + 308): 0 a stage's (a picture,
 *               the rippling sky and a 16-pixel layer, a 32-pixel layer), 1 (two 16-pixel layers, a
 *               32-pixel layer), 2 the 24-tile-wide map of the endings' scenes (0x0881B7C8, layer 2
 *               only); maps from 0x08A07918 (12 bytes by kind: layer 1's, layer 0's, layer 2's)
 *
 * The ending object (a2 of 0x089DE2C0; offsets used here): +0 a kind, +6 the frame count, +49 / +52 / +53
 * done flags and +50 / +54 / +56 their frames, +0xAA / +0xBC / +0xCE the layer place records (18 bytes:
 * +0 set, +1 with scrolls, +2 x, +4 y, +6, +8, +10 - +16 the scroll parts).
 *
 * WIDE NATIVE: extensions/widescreen.c is told each frame an ending draws (widescreen_ending_frame,
 * wrapping 0x089DE2C0 and 0x089DE390), which the drawers ask (widescreen_in_ending).
 */
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }

#define GAME          0x08E75910u
#define BATTLE        0x092E1228u
#define CAMERA_LAYERS 0x0951F950u   /* three records of 0x98: +0 set (the layer is drawn) */
#define ENDING_KIND   (0x09358BE0u + 308)   /* u32: 0, 1 or 2 */
#define LAYER_MAPS    0x08A07918u   /* 12 bytes by kind: layer 1's map, layer 0's, layer 2's */
#define LAYER_0       0x092E8800u
#define LAYER_1       0x092E88A0u
#define LAYER_2       0x092E8940u
#define SCROLL        0x092E5F78u

void f_089DF82C(Cpu *c);   /* the scene script's step (small_steps.c) */
void f_089DF624(Cpu *c);   /* what the script asked for (still translated) */
void f_089DE8A4(Cpu *c);   /* timed lines (batch_mid8.c) */
void f_08837690(Cpu *c); void f_088379B8(Cpu *c); void f_0883771C(Cpu *c);   /* object lists (lists.c) */
void f_08886E18(Cpu *c); void f_08887AFC(Cpu *c); void f_0880B6CC(Cpu *c);
void f_089DE52C(Cpu *c);   /* camera layers (batch_mid11.c) */
void f_089E04B0(Cpu *c);   /* a button pressed */
void f_08809BE4(Cpu *c);   /* a sound */
void f_08822F8C(Cpu *c);   /* an object made */
void f_0887C708(Cpu *c);
void f_0880AA74(Cpu *c);
void f_088054F0(Cpu *c);   /* task.c: wait for frames */
void f_089E0CF4(Cpu *c);   /* a texture list made the one in use */
void f_0881C0C8(Cpu *c);   /* the rippling sky (background.c) */
void f_0881A8E0(Cpu *c);   /* a 16-pixel layer (background.c) */
void f_08819300(Cpu *c);   /* a 16-pixel layer, 64 rows (background.c) */
void f_08819498(Cpu *c);   /* a picture layer (background.c) */
void f_0881AB28(Cpu *c);   /* a 32-pixel layer (tiles.c) */
void f_0881B7C8(Cpu *c);   /* the endings' 24-tile-wide map (tiles.c) */
void f_089DE444(Cpu *c);
void f_089DE4B4(Cpu *c);
void f_089DE7C0(Cpu *c);
void f_0881A2C8(Cpu *c);

/* ---- 0x089DE2C0: the ending's step (a0 the task, a2 the ending object) ---- */
static void step(Cpu *c, gaddr task, gaddr o) {
    *H(o + 6) = (uint16_t)(S16(o + 6) + 1);
    CALL1(f_089DF82C, o);
    CALL1(f_089DF624, o);
    CALL1(f_089DE444, o);
    CALL1(f_089DE7C0, o);
    CALL1(f_089DE8A4, o);
    f_08837690(c);
    f_088379B8(c);
    f_0883771C(c);
    f_08886E18(c);
    f_08887AFC(c);
    CALL1(f_089DE52C, o);
    f_0880B6CC(c);
    f_0881A2C8(c);
    if (CALL1(f_089E04B0, task) == 0) return;
    if (S8(o + 53) == 0) CALL1(f_08809BE4, 0x3C);
    *H(task + 2) = (uint16_t)(*H(task + 2) + 1);
}
SFA_FUNC_WRAPPED(089DE2C0, ending_step, widescreen_ending_frame) {
    gaddr task = ARG(0), o = ARG(2), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = task;
    step(c, task, o);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---- 0x089DE390: draws until GAME + 93 is cleared ----
   Its loop runs many frames in one call (the fade out after a button press), so WIDE NATIVE is told of
   each of them (widescreen_ending_more), not only of the call. */
void widescreen_ending_more(void);   /* extensions/widescreen.c */
SFA_FUNC_WRAPPED(089DE390, ending_draw_wait, widescreen_ending_frame) {
    gaddr task = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    *H(task + 2) = (uint16_t)(*H(task + 2) + 1);
    c->r[16] = GAME;
    *B(GAME + 93) = 0;
    f_0880AA74(c);
    do {
        widescreen_ending_more();
        f_08837690(c);
        f_0880B6CC(c);
        f_0881A2C8(c);
        CALL1(f_088054F0, 1);
    } while (S8(GAME + 93) != 0);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---- 0x089DE444 / 0x089DE4B4: the layers' places ---- */
static void layer_place(gaddr rec, gaddr l) {
    *H(l + 10) = *H(rec + 2);
    if (S8(rec + 1) == 0) {
        *H(l + 12) = 0;
        *H(l + 14) = *H(rec + 4);
        *H(l + 8) = 0;
        return;
    }
    *H(l + 14) = *H(rec + 4);
    *H(l + 8) = *H(rec + 6);
    *H(l + 12) = *H(rec + 8);
    *W(l + 16) = (uint32_t)S16(rec + 10) << 16;
    *W(l + 20) = (uint32_t)S16(rec + 12) << 16;
    *W(l + 16) = ((uint32_t)S16(rec + 10) << 16) | (uint32_t)(int32_t)S16(rec + 14);
    *W(l + 20) = ((uint32_t)S16(rec + 12) << 16) | (uint32_t)(int32_t)S16(rec + 16);
}
SFA_FUNC(089DE4B4, ending_layer_place) { layer_place(ARG(0), ARG(1)); }

SFA_FUNC(089DE444, ending_layers_place) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    static const uint32_t rec[3] = { 0xAA, 0xBC, 0xCE }, layer[3] = { 0, 0x98, 0x130 };
    for (int i = 0; i < 3; i++) {
        c->r[4] = o + rec[i];
        c->r[5] = (uint32_t)(int32_t)S8(o + rec[i]);
        if (c->r[5] == 0) { c->r[5] = 0x09520000u; continue; }
        c->r[5] = CAMERA_LAYERS + layer[i];
        f_089DE4B4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---- 0x089DE7C0: the timed fade-out object, 0x0887C708 and a sound ---- */
SFA_FUNC(089DE7C0, ending_timed) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(o + 49) == 0 && S16(o + 50) < S16(o + 6)) {
        gaddr ob = CALL2(f_08822F8C, 0, 0);
        *H(ob + 26) = 0x180;
        *H(ob + 30) = 0x30;
        *W(ob + 20) = 0xA000;   /* the same for every kind (the original tests for 2 and 0x11 both ways) */
        *B(o + 49) = 0xFF;
    }
    if (S8(o + 52) == 0 && S16(o + 54) < S16(o + 6)) {
        f_0887C708(c);
        *B(o + 52) = 0xFF;
    }
    if (S8(o + 53) == 0 && S16(o + 56) < S16(o + 6)) {
        CALL1(f_08809BE4, 0xB4);
        *B(o + 53) = 0xFF;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---- 0x0881A2C8: the ending's layers drawn ----
   WIDE NATIVE, an ending's frame (extensions/widescreen.c): with the scaled endings off its layers are
   cut at the 4:3 picture's sides (tiles.c, tiles_ending_mode 1). With them on (scaled_endings.c) the
   scene is drawn 1.25 times larger (mode 2, its pictures too) when a layer stands still on one of its
   map's 4:3 pictures (the ones beside it are other scenes'), or when no layer has art of its own past
   both 4:3 sides (a tile in at least half the columns past each, counted first: tiles_counting); a scene
   that scrolls across art wider than the screen is drawn as it is, wider. */
extern int tiles_ending_mode, tiles_counting;   /* tiles.c */
extern int32_t tiles_count_in, tiles_count_in_art, tiles_count_out[2], tiles_count_out_art[2];
int widescreen_in_ending(void);                 /* extensions/widescreen.c */
int scaled_endings_on(void);                    /* extensions/scaled_endings.c */
void scaled_endings_scene(int scaled);
extern int rt_widescreen;
void inner_f_0881AB28(Cpu *c);

/* a tile layer with art of its own past both 4:3 sides: at least half the columns past each have a tile */
static int layer_wide(Cpu *c, GuestFunc draw, gaddr layer, gaddr map) {
    tiles_count_in = tiles_count_in_art = 0;
    for (int i = 0; i < 2; i++) tiles_count_out[i] = tiles_count_out_art[i] = 0;
    tiles_counting = 1;
    CALL2(draw, layer, map);
    tiles_counting = 0;
    for (int i = 0; i < 2; i++) if (tiles_count_out[i] == 0 || tiles_count_out_art[i] * 2 < tiles_count_out[i]) return 0;
    return 1;
}
/* a layer standing still on a picture's place: its scroll across a multiple of 384 (a picture's width:
   the ending's maps keep their 4:3 pictures side by side), as the frame before or just cut to */
static int32_t last_h[3] = { -10000, -10000, -10000 };
static int still(gaddr layer, int i) {
    int32_t h = S16(layer + 18), dh = h - last_h[i];
    return (h % 384 + 384) % 384 == 0 && (dh == 0 || dh > 64 || dh < -64);
}
static int scene_scaled(Cpu *c, uint32_t kind, gaddr maps) {
    int l0 = *B(LAYER_0 + 7) && *B(CAMERA_LAYERS), l1 = *B(LAYER_1 + 7) && *B(CAMERA_LAYERS + 0x98);
    int l2 = *B(LAYER_2 + 7) && *B(CAMERA_LAYERS + 0x130);
    if ((l2 && still(LAYER_2, 2)) || (l1 && kind != 0 && still(LAYER_1, 1)) || (l0 && kind != 0 && still(LAYER_0, 0))) return 1;
    if (l2 && layer_wide(c, kind == 2 ? f_0881B7C8 : inner_f_0881AB28, LAYER_2, *W(maps + 8))) return 0;
    if (l1 && layer_wide(c, kind != 0 ? f_08819300 : f_0881A8E0, LAYER_1, *W(maps + 4))) return 0;
    if (l0 && kind != 0 && layer_wide(c, f_0881A8E0, LAYER_0, *W(maps))) return 0;
    return 1;
}

static void layer_scroll_word(gaddr l) { *W(l + 96) = (uint32_t)(-(int32_t)*W(l + 20)) - ((uint32_t)*H(l + 90) << 16); }

static void ending_layers(Cpu *c) {
    if (S8(BATTLE + 23) != 0) return;
    *W(0x08B014DCu) = 0x08B014ECu;
    *W(0x08B014E0u) = 0x08B014FCu;
    *W(0x08B014E4u) = 0x08B0150Cu;
    *W(0x08B014E8u) = 4;
    int32_t g = S16(0x08E75C44u);
    uint32_t grey = (uint32_t)(g + (g << 8) + (g << 16)) - 0x1000000u;
    *W(0x08B01508u) = grey; *W(0x08B01504u) = grey; *W(0x08B01500u) = grey; *W(0x08B014FCu) = grey;
    CALL1(f_089E0CF4, 0x08A053CCu);
    layer_scroll_word(LAYER_0);
    layer_scroll_word(LAYER_1);
    layer_scroll_word(LAYER_2);
    *W(SCROLL + 112) = 0;
    uint32_t kind = *W(ENDING_KIND);
    gaddr maps = LAYER_MAPS + kind * 12u;
    if (rt_widescreen && widescreen_in_ending()) {
        int scaled = scaled_endings_on() && scene_scaled(c, kind, maps);
        tiles_ending_mode = !scaled_endings_on() ? 1 : scaled ? 2 : 0;
        scaled_endings_scene(scaled);
        last_h[0] = S16(LAYER_0 + 18); last_h[1] = S16(LAYER_1 + 18); last_h[2] = S16(LAYER_2 + 18);
    }

    if (*B(LAYER_1 + 7) == 0) {
        *W(LAYER_1 + 60) = 0;
        *B(LAYER_1 + 7) = 1;
    } else if (*B(CAMERA_LAYERS + 0x98) != 0) {
        if (kind == 0) {
            *B(LAYER_1 + 3) = 0xAA;
            *W(LAYER_1 + 60) = (*W(LAYER_1 + 60) + 0x4000) & 0xFFFFFFu;
            CALL1(f_0881C0C8, LAYER_1);
            CALL2(f_0881A8E0, LAYER_1, *W(maps + 4));
        } else {
            *B(LAYER_1 + 3) = 0xFF;
            CALL2(f_08819300, LAYER_1, *W(maps + 4));
        }
    }

    if (*B(LAYER_0 + 7) == 0) {
        *W(LAYER_0 + 60) = 8;
        *B(LAYER_0 + 131) = 0;
        *B(LAYER_0 + 7) = 1;
    } else if (*B(CAMERA_LAYERS) != 0) {
        if (kind == 0) {
            *B(LAYER_0 + 3) = 0x7A;
            CALL1(f_08819498, LAYER_0);
        } else {
            *B(LAYER_0 + 3) = 0xFF;
            CALL2(f_0881A8E0, LAYER_0, *W(maps));
        }
    }

    if (*B(LAYER_2 + 7) == 0) {
        *W(LAYER_2 + 60) = 6;
        *B(LAYER_2 + 131) = 0;
        *B(LAYER_2 + 7) = 1;
        return;
    }
    if (*B(CAMERA_LAYERS + 0x130) == 0) return;
    if (kind == 2) {
        *B(LAYER_2 + 3) = 0xFF;
        CALL2(f_0881B7C8, LAYER_2, *W(maps + 8));
        return;
    }
    *B(LAYER_2 + 3) = kind == 0 ? 0xDA : 0xFF;
    CALL2(f_0881AB28, LAYER_2, *W(maps + 8));
}
SFA_FUNC(0881A2C8, ending_layers_draw) {
    uint32_t s[5]; for (int i = 0; i < 5; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    ending_layers(c);
    tiles_ending_mode = 0;
    c->r[29] += 0x30;
    for (int i = 0; i < 5; i++) c->r[16 + i] = s[i];
}
