/* The fight's camera, once a frame: each camera layer follows the fighters at its own rate, and a few
 * stages move theirs on their own.
 *
 * 0x0881D1A8 (from the fight's frame, 0x0881DF10) runs, unless the fight is paused: the fighters' layer
 * 1 (0x0881D244), the screen's lean and shake (0x0881D348), layer 0 (0x0881D960), layer 2 (0x0881DC6C),
 * then 0x0881EE84 and 0x08819BBC; and always the background (0x08819CFC, background.c).
 *
 * A camera layer (0x092E8800 + 0xA0 n) here: +1 set up this frame, +2 its mode (0 / 1 / 2: which of the
 * three layers it places), +4 its step (0 the first frame), +5 / +6 a stage's own flags, +8 / +12 its x / y
 * start (16.16), +16 / +20 where it is drawn (+18 / +22 the whole pixels), +32 / +36 where it is (16.16;
 * +34 / +38 the pixels), +40 / +44 last frame's, +56 / +57 the tile alignment, +59 its draw flags,
 * +60 / +62 a stage's counters, +64 - +70 limits, +80 / +84 / +92 a stage's own drift (16.16), +82 / +86 /
 * +88 / +90 offsets, +124 / +125 / +131 a stage's frames, +132 a speed, +155 the 16-pixel drawer.
 */
#include "../decomp.h"

void f_0881E058(Cpu *c);   /* layer_follow_x (camera.c): (layer, rate) */
void f_0881E148(Cpu *c);   /* layer_follow_y (camera.c): (layer, rate) */
void f_0881E320(Cpu *c);   /* camera_follow_span (camera.c) */
void f_0881E538(Cpu *c);   /* camera_layer_follow_height (camera.c) */
void f_0881D488(Cpu *c);   /* the fighters' layer's first frame (runners.c) */
void f_0881EE84(Cpu *c);
void f_08819BBC(Cpu *c);
void f_08819CFC(Cpu *c);   /* stage_background (background.c) */
void f_08821368(Cpu *c);   /* game_random (fight.c) */
void f_0887C4CC(Cpu *c);   /* plays a sound: (0, sound, volume) */
void f_0881ECA4(Cpu *c); void f_0881E1B4(Cpu *c); void f_0881E0D8(Cpu *c);
void f_0881D244(Cpu *c); void f_0881D348(Cpu *c); void f_0881D960(Cpu *c); void f_0881DC6C(Cpu *c);
void f_0881D4D8(Cpu *c); void f_0881D588(Cpu *c); void f_0881D5F4(Cpu *c); void f_0881D6EC(Cpu *c);
void f_0881D8A8(Cpu *c);
void rt_call_indirect(Cpu *c, uint32_t addr);

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int32_t   S8(gaddr a) { return (int8_t)*B(a); }
static inline int32_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline void      SET16(gaddr a, int32_t v) { *H(a) = (uint16_t)v; }

#define GAME      0x08E75910u   /* +0 screen, +2 its step, +32 a frame count, +89 / +145 halts */
#define BATTLE    0x092E1228u   /* +0 the stage, +2 a variant, +15 paused, +35, +37 / +52 animation stopped, +74 the shake's height */
#define SHAKE     0x092E5F78u   /* the screen's lean: +4 step, +24, +28 strength (16.16), +104 / +108, +112 lean, +128 / +130 / +132 */
#define LAYER_0   0x092E8800u
#define LAYER_1   0x092E88A0u
#define LAYER_2   0x092E8940u
#define STARTS    0x08A08FC8u   /* s16[stage][4]: layers 0 and 2's start x, y */
#define RATES     0x08A09118u   /* s16[stage][4]: layers 0 and 2's follow rates x, y */
#define ALIGN     0x08A09468u   /* s16[mode]: the tile alignment mask */
#define LEAN_TIME 0x08A092BCu   /* u16[stage]: frames over which the lean builds (0xFFFF none) */
#define LEAN_TOP  0x08A09268u   /* u16[stage]: the lean's first height below 0xD8 */

static int anim_stopped(void) { return (S8(BATTLE + 37) | S8(BATTLE + 52)) != 0; }

static void call(Cpu *c, GuestFunc f, uint32_t a0, uint32_t back) { c->r[4] = a0; c->r[31] = back; f(c); }
static void call2(Cpu *c, GuestFunc f, uint32_t a0, uint32_t a1, uint32_t back) {
    c->r[4] = a0; c->r[5] = a1; c->r[31] = back; f(c);
}
static void call3(Cpu *c, GuestFunc f, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t back) {
    c->r[4] = a0; c->r[5] = a1; c->r[6] = a2; c->r[31] = back; f(c);
}

/* The vertical drawn place from the layer's y: -(y - the shake's height + offset), in pixels. */
static int32_t drawn_y(gaddr l) { return -(S16(l + 38) - S16(BATTLE + 74) + S16(l + 90)); }

/* ---- 0x0881ECA4: a camera layer placed at (x, y), its offsets and limits set by its mode ---- */
static void layer_place(gaddr l, int32_t x_in, int32_t y_in) {
    SET16(l + 88, 0); SET16(l + 90, -16); SET16(l + 20, 0);
    SET16(l + 16, 0); SET16(l + 12, 0); SET16(l + 8, 0);
    int32_t x = (int16_t)x_in, y = (int16_t)y_in;
    SET16(l + 10, x_in); SET16(l + 14, y_in);
    int32_t cx = (int16_t)(x + S16(l + 82)), cy = (int16_t)(y + S16(l + 86));
    SET16(l + 34, cx); SET16(l + 18, cx); SET16(l + 38, cy); SET16(l + 22, cy);
    uint8_t mode = *B(l + 2);
    if (mode == 0) {
        SET16(LAYER_0 + 34, cx); SET16(LAYER_0 + 102, cy); SET16(LAYER_0 + 18, cx);
        SET16(LAYER_0 + 98, cy - S16(l + 90));
        SET16(l + 68, -1); SET16(l + 70, -1); SET16(l + 66, -1); SET16(l + 64, -1);
    } else if (mode == 1) {
        *B(LAYER_1 + 123) = 0xF;
        *W(LAYER_1 + 16) = (uint32_t)cx << 16; *W(LAYER_1 + 32) = (uint32_t)cx << 16;
        *W(LAYER_1 + 96) = (uint32_t)cy << 16; *W(LAYER_1 + 100) = (uint32_t)cy << 16;
        SET16(l + 64, 0x10); SET16(l + 66, 0); SET16(l + 68, 0x100); SET16(l + 70, 0x280);
        SET16(l + 18, S16(l + 34)); SET16(l + 22, S16(l + 38));
        *W(LAYER_1 + 16) = (uint32_t)(S16(l + 34) + S16(l + 88)) << 16;
        *W(LAYER_1 + 96) = (uint32_t)drawn_y(l) << 16;
    } else if (mode == 2) {
        *W(LAYER_2 + 16) = (uint32_t)cx << 16; *W(LAYER_2 + 32) = (uint32_t)cx << 16;
        *W(LAYER_2 + 100) = (uint32_t)cy << 16;
        *W(LAYER_2 + 96) = (uint32_t)(cy - S16(l + 90)) << 16;
        SET16(l + 68, -1); SET16(l + 70, -1); SET16(l + 66, -1); SET16(l + 64, -1);
    }
    int32_t m = S16(ALIGN + (uint32_t)mode * 2u);
    *B(l + 56) = (uint8_t)(~(uint32_t)x & (uint32_t)m);
    *B(l + 57) = (uint8_t)(~(uint32_t)y & (uint32_t)m);
    *B(l + 1) = 1;
}

/* void layer_place(layer *a0, s16 x a1, s16 y a2) */
SFA_FUNC(0881ECA4, camera_layer_place) { layer_place(ARG(0), (int32_t)ARG(1), (int32_t)ARG(2)); }

/* ---- 0x0881E1B4: the fighters' layer follows the fighters (not on the select screen's last steps) ---- */
SFA_FUNC(0881E1B4, camera_fighters_follow) {
    gaddr l = ARG(0);
    c->r[29] -= 0x20;
    if (*B(l) != 0 && (*H(GAME) != 1 || (int32_t)*H(GAME + 2) < 3)) {
        call(c, f_0881E320, l, 0x0881E1F8);
        call(c, f_0881E538, l, 0x0881E200);
    }
    SET16(l + 18, S16(l + 34)); SET16(l + 22, S16(l + 38));
    *W(LAYER_1 + 16) = (uint32_t)(S16(l + 34) + S16(l + 88)) << 16;
    *W(LAYER_1 + 96) = (uint32_t)drawn_y(l) << 16;
    c->r[29] += 0x20;
}

/* ---- 0x0881E0D8: a layer that keeps to the fighters' layer's travel less the shake's strength × rate
   (CHUN-LI's layer 2, VEGA's layer 0), with its own drift (+80) ---- */
SFA_FUNC(0881E0D8, camera_layer_follow_lean) {
    gaddr l = ARG(0);
    int32_t rate = (int16_t)ARG(1);
    uint32_t lean = *W(SHAKE + 28) * (uint32_t)rate;
    uint32_t x = *W(LAYER_1 + 32) - *W(LAYER_1 + 8) - lean + *W(l + 8) + *W(l + 80);
    *B(l + 59) = (int32_t)x < (int32_t)*W(l + 40) ? 8 : 12;
    *W(l + 32) = x;
}

/* ---- 0x0881D244: the fighters' layer ---- */
SFA_FUNC(0881D244, camera_fighters_layer) {
    gaddr l = ARG(0);
    c->r[29] -= 0x20;
    int32_t stage = S16(BATTLE);
    if (*B(l + 4) == 0) {
        *B(l + 155) = 1; *W(l + 84) = 0; *W(l + 80) = 0; *B(l + 131) = 0;
        call3(c, f_0881ECA4, l, 0x1C0, 0, 0x0881D294);
        stage = S16(BATTLE);
        if (stage == 0x0A || stage == 0x1E) *W(l + 60) = 0;
        if (*B(l + 1) != 0) {
            *B(l + 1) = 0;
            *B(l + 4) = (uint8_t)(*B(l + 4) + 1);
            call(c, f_0881D488, l, 0x0881D2CC);
        }
    } else {
        if ((stage == 0x0A || stage == 0x1E) && !anim_stopped())   /* BISON: the sky's ripple */
            *W(l + 60) = (*W(l + 60) + 0x4000) & 0xFFFFFFu;
        call(c, f_0881E1B4, l, 0x0881D320);
    }
    SET16(l + 42, S16(l + 34));
    SET16(l + 46, S16(l + 38));
    c->r[29] += 0x20;
}

/* ---- 0x0881D348: the screen's lean (the floor's perspective): set up from the stage's tables, then
   each frame from how far the fighters' layer has moved from its start ---- */
SFA_FUNC(0881D348, camera_lean) {
    int32_t stage = S16(BATTLE);
    if (*B(SHAKE + 4) == 0) {
        uint32_t i = (uint32_t)stage * 2u;
        if (*H(LEAN_TIME + i) == 0xFFFF) {
            *W(SHAKE + 28) = 0; *W(SHAKE + 112) = 0; *B(SHAKE + 4) = 2;
            return;
        }
        SET16(SHAKE + 130, 0xD8 - (int32_t)*H(LEAN_TOP + i));
        SET16(SHAKE + 132, 0xD8);
        *H(SHAKE + 128) = *H(LEAN_TIME + (uint32_t)S16(BATTLE) * 2u);
        *B(SHAKE + 4) = 1;
        return;
    }
    if (*B(SHAKE + 4) != 1) return;
    *W(SHAKE + 104) = *W(LAYER_1 + 32);
    *W(SHAKE + 108) = *W(LAYER_1 + 36);
    DIV(c, *W(SHAKE + 104) - *W(LAYER_1 + 8), (uint32_t)S16(SHAKE + 128));
    uint32_t q = c->lo;
    int32_t span = S16(SHAKE + 132) - S16(SHAKE + 130);
    *W(SHAKE + 28) = q;
    SET16(SHAKE + 24, S16(LAYER_1 + 22) - S16(BATTLE + 74) + 0xC8);
    int64_t m = (int64_t)(int32_t)q * span;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    *W(SHAKE + 112) = (uint32_t)m;
    stage = S16(BATTLE);
    if (stage == 0x0B) *W(SHAKE + 112) += q << 4;        /* SAGAT */
    else if (stage == 0x1C) *W(SHAKE + 112) += q << 3;   /* VEGA */
}

/* The end of layer 0's and 2's frames: where they are drawn, from where they are. */
static void layer_drawn(gaddr l, gaddr to) {
    *W(l + 16) = *W(l + 32);
    *W(l + 20) = *W(l + 36);
    if (to == LAYER_0) SET16(LAYER_0 + 18, S16(l + 34) + S16(l + 88));
    else *W(to + 16) = (uint32_t)(S16(l + 34) + S16(l + 88)) << 16;
    *H(l + 16) = *H(l + 32);
    if (to == LAYER_0) SET16(LAYER_0 + 98, drawn_y(l));
    else *W(to + 96) = (uint32_t)drawn_y(l) << 16;
    *H(l + 20) = *H(l + 36);
}

/* ---- 0x0881D960: layer 0 ---- */
SFA_FUNC(0881D960, camera_layer_0) {
    gaddr l = ARG(0);
    c->r[29] -= 0x20;
    int32_t stage = S16(BATTLE);
    if (*B(l + 4) == 0) {
        *W(l + 84) = 0; *W(l + 80) = 0;
        gaddr e = STARTS + (uint32_t)stage * 8u;
        call3(c, f_0881ECA4, l, (uint32_t)(int32_t)(int16_t)*H(e), (uint32_t)(int32_t)(int16_t)*H(e + 2), 0x0881D9CC);
        stage = S16(BATTLE);
        uint32_t x = *W(l + 32), y = *W(l + 36);
        if (stage == 0x0A || stage == 0x1E || stage == 0x15) { *W(l + 60) = 8; *B(l + 131) = 0; }
        else if (stage == 0x11) SET16(l + 62, 0x14);
        else if (stage == 0x1C) { if (S16(BATTLE + 2) == 0) *W(l + 84) = 0xFF000000u; else *B(l + 5) = 1; }
        else if (stage == 0x20) {
            SET16(l + 62, 0xB0); SET16(l + 60, 0x46); *W(l + 132) = 0x1E000;
            *B(l + 124) = 0; *B(l + 125) = 0;
        }
        stage = S16(BATTLE);
        *B(l + 155) = (stage == 0x1C || stage == 0x0F) ? 0 : 1;
        if (*B(l + 1) != 0) {
            *W(l + 92) = 0;
            *B(l + 4) = (uint8_t)(*B(l + 4) + 1);
            *B(l + 1) = 0;
        }
        *W(l + 40) = x; *W(l + 44) = y;
        c->r[29] += 0x20;
        return;
    }
    if (stage == 0x0E) {                                           /* ROLENTO: the sky drifts */
        if (!anim_stopped()) *W(l + 80) = (*W(l + 80) - 0x20000) & 0x1FF0000u;
    } else if (stage == 0x11) call(c, f_0881D4D8, l, 0x0881DB2C);   /* GEN */
    else if (stage == 0x1C) call(c, f_0881D588, l, 0x0881DB44);     /* VEGA */
    else if (stage == 0x20) call(c, f_0881D5F4, l, 0x0881DB5C);     /* GUILE */
    else if (stage >= 37 && stage < 41) {                           /* BATTLE1 - 4 */
        if (!anim_stopped()) *W(l + 80) += 0xFFFFA000u;
    }
    stage = S16(BATTLE);
    gaddr e = RATES + (uint32_t)stage * 8u;
    int32_t rate_y = S16(e + 2);
    if (stage == 0x1C) call2(c, f_0881E0D8, l, 7, 0x0881DBD0);
    else call2(c, f_0881E058, l, (uint32_t)S16(e), 0x0881DBE4);
    call2(c, f_0881E148, l, (uint32_t)rate_y, 0x0881DBF0);
    layer_drawn(l, LAYER_0);
    *W(l + 40) = *W(l + 32); *W(l + 44) = *W(l + 36);
    c->r[29] += 0x20;
}

/* ---- 0x0881DC6C: layer 2 ---- */
SFA_FUNC(0881DC6C, camera_layer_2) {
    gaddr l = ARG(0);
    c->r[29] -= 0x30;
    int32_t stage = S16(BATTLE);
    if (*B(l + 4) == 0) {
        *W(l + 84) = 0; *W(l + 80) = 0;
        gaddr e = STARTS + (uint32_t)stage * 8u;
        call3(c, f_0881ECA4, l, (uint32_t)(int32_t)(int16_t)*H(e + 4), (uint32_t)(int32_t)(int16_t)*H(e + 6), 0x0881DCD8);
        stage = S16(BATTLE);
        uint32_t x = *W(l + 32), y = *W(l + 36);
        if (stage == 0x0A || stage == 0x1E) { *W(l + 60) = 6; *B(l + 131) = 0; }
        else if (stage == 0x10) { *B(l + 124) = 0; *W(l + 60) = 0xC; }
        else if (stage == 0x1D) { SET16(l + 62, 0x10); SET16(l + 60, 0); *B(l + 124) = 0; }
        stage = S16(BATTLE);
        *B(l + 155) = (stage == 1 || stage == 3 || stage == 0x0B) ? 1 : 0;
        if (*B(l + 1) != 0) {
            *B(l + 1) = 0;
            *B(l + 4) = (uint8_t)(*B(l + 4) + 1);
        }
        *W(l + 40) = x; *W(l + 44) = y;
        c->r[29] += 0x30;
        return;
    }
    gaddr e = RATES + (uint32_t)stage * 8u;
    int32_t rate_x = S16(e + 4), rate_y = S16(e + 6);
    if (stage == 0x10 && !anim_stopped()) {                          /* ZANGIEF: 4 frames of 12 */
        uint32_t n = *W(l + 60) - 1;
        *W(l + 60) = n;
        if (n == 0) {
            *B(l + 124) = (uint8_t)(S8(l + 124) + 1);
            *B(l + 124) = (uint8_t)(S8(l + 124) & 3);
            *W(l + 60) = 0xC;
        }
    }
    stage = S16(BATTLE);
    if (stage == 4) call2(c, f_0881E0D8, l, (uint32_t)rate_x, 0x0881DE10);   /* CHUN-LI */
    else {
        if (stage == 0x1D) call(c, f_0881D8A8, l, 0x0881DE2C);                /* KARIN 1 */
        else if (stage == 0x20 && !anim_stopped())                              /* GUILE: the clouds */
            *W(l + 80) = (*W(l + 80) + 0x30000) & 0x1FF0000u;
        call2(c, f_0881E058, l, (uint32_t)rate_x, 0x0881DE7C);
    }
    call2(c, f_0881E148, l, (uint32_t)rate_y, 0x0881DE88);
    layer_drawn(l, LAYER_2);
    *W(l + 40) = *W(l + 32); *W(l + 44) = *W(l + 36);
    c->r[29] += 0x30;
}

/* ---- 0x0881D4D8: GEN's layer 0: a figure (+86 / +62) picked at random from a table every few frames ---- */
#define GEN_FRAMES 0x08A0938Eu   /* s16[2][8]: +86, then +62 less a random 0 - 15 */
SFA_FUNC(0881D4D8, camera_gen_layer_0) {
    gaddr l = ARG(0);
    c->r[29] -= 0x10;
    if (!anim_stopped()) {
        int32_t n = (int16_t)(S16(l + 62) - 1);
        SET16(l + 62, n);
        if (n == 0) {
            *B(l + 124) = (uint8_t)(S8(l + 124) + 2);
            *B(l + 124) = (uint8_t)(S8(l + 124) & 0xF);
            c->r[31] = 0x0881D53C; f_08821368(c);
            uint32_t r = c->r[2];
            gaddr t = GEN_FRAMES + (uint32_t)(S8(l + 124) * 2);
            SET16(l + 86, S16(t));
            SET16(l + 62, S16(t + 2) + (int32_t)((r + *B(GAME + 32)) & 0xF));
        }
    }
    c->r[29] += 0x10;
}

/* ---- 0x0881D588: VEGA's layer 0 (the second variant): the cage falls, a sound when it lands ---- */
SFA_FUNC(0881D588, camera_vega_layer_0) {
    gaddr l = ARG(0);
    c->r[29] -= 0x20;
    if (*B(l + 5) == 0) {
        *W(l + 92) += 0x8000;
        uint32_t y = *W(l + 84) + *W(l + 92);
        *W(l + 84) = y;
        if ((int32_t)y >= 0) {
            *B(l + 5) = 1;
            *W(l + 84) = 0;
            *B(BATTLE + 35) = 0x40;
            call3(c, f_0887C4CC, 0, 0x37, 0xC0, 0x0881D5E8);
        }
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881D5F4: GUILE's layer 0: the train; by the layer's x it speeds up, slows, or runs a step
   of 0x0881D6EC's ---- */
SFA_FUNC(0881D5F4, camera_guile_layer_0) {
    gaddr l = ARG(0);
    c->r[29] -= 0x20;
    if (!anim_stopped()) {
        int32_t x = S16(l + 18);
        uint32_t v;
        if (x >= 4096 && x < 8064) {
            *W(l + 80) += 0x200000;
            v = *W(l + 80);
        } else if (x >= 8064 && x < 32352) {
            *W(l + 80) += 0x180000;
            v = *W(l + 80);
            if (*B(l + 5) == 0) {
                *B(l + 5) = 1; *B(l + 6) = 0;
                SET16(l + 62, 0xB0); SET16(l + 60, 0x46); *W(l + 132) = 0x1E000;
                *B(l + 124) = 0; *B(l + 125) = 0;
            }
        } else {
            call(c, f_0881D6EC, l, 0x0881D6C0);
            v = *W(l + 80);
            if (*B(l + 5) != 0) *B(l + 5) = 0;
        }
        *W(l + 80) = v & 0x7FFF0000u;
    }
    c->r[29] += 0x20;
}

/* ---- 0x0881D6EC: GUILE's train's step (table 0x08A093B0 by +6), then its stop-and-go timer ---- */
#define GUILE_STEPS 0x08A093B0u
SFA_FUNC(0881D6EC, camera_guile_train_step) {
    gaddr l = ARG(0);
    c->r[29] -= 0x20;
    c->r[4] = l; c->r[31] = 0x0881D71C;
    rt_call_indirect(c, *W(GUILE_STEPS + (uint32_t)*B(l + 6) * 4u));
    uint32_t n = *H(l + 60);
    if (n != 0) SET16(l + 60, (int32_t)n - 1);
    else if (*W(l + 84) != 0) { SET16(l + 60, 0x64); *W(l + 84) = 0; }
    else { SET16(l + 60, 2); *W(l + 84) = 0xFFFF0000u; }
    c->r[29] += 0x20;
}

/* ---- 0x0881D768: GUILE's train's first step: drift by +132; past x 3585 (before 4096) on to step 1 ---- */
SFA_FUNC(0881D768, camera_guile_train_drift) {
    gaddr l = ARG(0);
    int32_t x = S16(l + 18);
    if (x >= 3585 && x < 4096) { *B(l + 6) = 1; SET16(l + 62, 0); }
    *W(l + 80) += *W(l + 132);
}

/* ---- 0x0881D8A8: KARIN 1's layer 2: the boat's bob, a frame (+86) and its time (+62) from a table ---- */
#define KARIN_BOB 0x08A093F8u   /* s8[48]: pairs of +86, then +62 */
SFA_FUNC(0881D8A8, camera_karin_layer_2) {
    gaddr l = ARG(0);
    if (anim_stopped()) return;
    int32_t n = (int16_t)(S16(l + 62) - 1);
    SET16(l + 62, n);
    if (n != 0) return;
    int32_t i = S8(l + 124) + 2;
    if (i >= 48) *B(l + 124) = 0;
    else *B(l + 124) = (uint8_t)i;
    gaddr t = KARIN_BOB + (uint32_t)S8(l + 124);
    int32_t extra = (int32_t)*H(l + 60) + 2 < 48 ? 2 : 0;
    SET16(l + 86, S8(t));
    SET16(l + 62, S8(t + 1) + extra);
}

/* ---- 0x0881D1A8: the camera's frame ---- */
SFA_FUNC(0881D1A8, camera_frame) {
    c->r[29] -= 0x20;
    if ((S8(GAME + 89) | S8(BATTLE + 15) | S8(GAME + 145)) == 0) {
        call(c, f_0881D244, LAYER_1, 0x0881D200);
        c->r[31] = 0x0881D208; f_0881D348(c);
        call(c, f_0881D960, LAYER_0, 0x0881D210);
        call(c, f_0881DC6C, LAYER_2, 0x0881D218);
        c->r[31] = 0x0881D220; f_0881EE84(c);
        c->r[31] = 0x0881D228; f_08819BBC(c);
    }
    c->r[31] = 0x0881D230; f_08819CFC(c);
    c->r[29] += 0x20;
}
