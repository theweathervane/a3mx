/* The fight's HUD (0x0880DB78 in frame.c, the text pass's calls on layers 0 and 1): a block of 0x40-byte
 * elements at 0x092E51C0 (laid out as the third object list's objects: +6 x, +10 y, +12 / +14 left and
 * width, +24 / +26 top and height, +36 / +42 / +48 the four corners' red, green, blue, +54 the picture
 * list, +55 the depth index, +60 the picture (bit 15: hidden)), drawn in a fixed order by four parts for
 * layer 1 and one for layer 0. An element is drawn either as a picture (0x0880E038, through the picture
 * drawer 0x0880E150) or as a plain shaded rectangle (0x08810B74: the bars' fills).
 *
 *   0x0880DC08  the two health bars' frames and fills (elements 8 - 10, 30 - 34) and element 0
 *   0x0880DC88  layer 0: the super / ISM bars at the bottom, their levels and fills (shown as rectangles
 *               or pictures as the overlay's +16 / +36 say), the names, the guard gauges ...
 *   0x0880DE2C  the guard gauges' marks, by each fighter's state (+313, +792), and the stun bar's fill
 *   0x0880DF98  elements 1, 2
 *   0x0880DFC8  ten elements (the timer, the score, the win marks ...)
 *
 * WIDE NATIVE: the HUD stays on the 4:3 picture, so what its pictures and rectangles have past the 4:3
 * picture's sides (the super bars' ends: art longer than the 4:3 screen, hidden there by its sides) is
 * cut off at x 0 and 384 (sprite_grid.c's x clip for pictures; the rectangles here). Only the drawing
 * changes.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */
extern int sprite_grid_clip_x;                     /* sprite_grid.c */
extern float sprite_grid_clip_lo, sprite_grid_clip_hi;

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

void f_089E0CF4(Cpu *c);   /* selects a texture page (a0 its descriptor) */
void f_089E1618(Cpu *c);   /* draws shaded vertices: a0 the list, a1 count, a2 flags; f12 the depth */
void f_0880E150(Cpu *c);   /* a picture drawn as cells (a0 draw state, a1 sprite, a2 picture, a3 list, t0) */

#define DRAW_STATE   0x08AD9320u   /* +0 / +2 x, y; +44 depth index; +48 colour; +56 / +60 flips; +85 - +87 */
#define RECT_LIST    0x08AD9378u   /* the shaded rectangle's list: +16 x, y (4 corners), +32 colours */
#define RECT_XY      0x08AD9388u
#define RECT_COLOUR  0x08AD9398u
#define DEPTHS       0x08AE03B8u   /* u16[]: depth by index */
#define PICTURES     0x08E75AD8u   /* u32[]: picture lists */
#define TEXTURES     0x08E75A18u   /* u32[]: texture page descriptors */
#define SCALE_X      0x08E759E8u
#define SCALE_Y      0x08E759ECu
#define OFFSET_X     0x08E75A04u
#define OFFSET_Y     0x08E75A0Cu
#define BATTLE       0x092E1228u   /* +56: s16, nonzero hides the guard marks */
#define FIGHTER_1    0x092E3EA0u   /* +313 s8, +792 u32: the fighter's state */
#define FIGHTER_2    0x092E4338u
#define GUARD_STATES 0x00820010u

enum { E_X = 6, E_Y = 10, E_LEFT = 12, E_WIDTH = 14, E_TOP = 24, E_HEIGHT = 26, E_RED = 36, E_GREEN = 42,
       E_BLUE = 48, E_LIST = 54, E_DEPTH = 55, E_PICTURE = 60 };
#define PICTURE_HIDDEN 0x8000u
#define LIST_SHARED 9               /* a list drawn with its own texture page (TEXTURES[9]), no sprite */

/* WIDE NATIVE: the 4:3 picture's sides on the screen */
static void hud_sides(float *lo, float *hi) {
    float sx = F(SCALE_X), ox = F(OFFSET_X);
    float a = 0.0f * sx; a = a + ox;
    float b = 384.0f * sx; b = b + ox;
    *lo = a; *hi = b;
}

/* 0x0880E038: an element drawn as its picture */
static void draw_element(Cpu *c, gaddr e) {
    if (*H(e + E_PICTURE) & PICTURE_HIDDEN) return;
    gaddr ds = DRAW_STATE;
    *H(ds + 0) = (uint16_t)S16(e + E_X);
    *H(ds + 2) = (uint16_t)S16(e + E_Y);
    uint32_t list_no = *B(e + E_LIST);
    gaddr list = *W(PICTURES + list_no * 4u);
    gaddr pic = list + (uint32_t)*H(e + E_PICTURE) * 12u;
    *B(ds + 85) = 0;
    *W(ds + 60) = 0;
    *W(ds + 56) = 0;
    *W(ds + 44) = *B(e + E_DEPTH);
    *W(ds + 48) = 0xFFFFFFFFu;
    *B(ds + 86) = 0;
    *B(ds + 87) = 0;
    if (rt_widescreen) { sprite_grid_clip_x = 1; hud_sides(&sprite_grid_clip_lo, &sprite_grid_clip_hi); }
    c->r[29] -= 0xF0;
    if (list_no == LIST_SHARED) {
        CALL1(f_089E0CF4, *W(TEXTURES + 36));
        c->r[4] = ds; c->r[5] = 0; c->r[6] = pic; c->r[7] = list; c->r[8] = 1;
        f_0880E150(c);
    } else {
        gaddr sp = c->r[29] + 0x20;                    /* a sprite of its own, on the stack */
        *B(sp + 12) = (uint8_t)(int8_t)*B(e + 1);
        *B(sp + 13) = *B(e + 2);
        *H(sp + 86) = (uint16_t)list_no;
        *W(sp + 164) = 0;
        c->r[4] = ds; c->r[5] = sp; c->r[6] = pic; c->r[7] = list; c->r[8] = 0;
        f_0880E150(c);
    }
    c->r[29] += 0xF0;
    sprite_grid_clip_x = 0;
}

/* 0x08810B74: an element drawn as a shaded rectangle, at `alpha` (0xFF: solid) */
static uint32_t corner(gaddr e, int i, uint32_t alpha) {
    return alpha << 24 | (uint32_t)*B(e + E_RED + i) << 16 | (uint32_t)*B(e + E_GREEN + i) << 8 | *B(e + E_BLUE + i);
}
static uint32_t mix(uint32_t a, uint32_t b, float t) {
    uint32_t out = 0;
    for (int s = 0; s < 32; s += 8) {
        float ca = (float)((a >> s) & 0xFF), cb = (float)((b >> s) & 0xFF);
        float v = ca + (cb - ca) * t;
        out |= (uint32_t)(int32_t)(v + 0.5f) << s;
    }
    return out;
}
static void draw_rect(Cpu *c, gaddr e, uint32_t alpha) {
    alpha &= 0xFF;
    int16_t y0 = (int16_t)(0xF0 - S16(e + E_Y));
    int16_t x0 = (int16_t)(S16(e + E_X) + S16(e + E_LEFT));
    int16_t top = (int16_t)(y0 + S16(e + E_TOP));
    int16_t x1 = (int16_t)(x0 + S16(e + E_WIDTH));
    int16_t bottom = (int16_t)(top + S16(e + E_HEIGHT));
    float sx = F(SCALE_X), sy = F(SCALE_Y), ox = F(OFFSET_X), oy = F(OFFSET_Y);
    float fx0 = (float)x0; fx0 = fx0 * sx; fx0 = fx0 + ox;
    float fx1 = (float)x1; fx1 = fx1 * sx; fx1 = fx1 + ox;
    float fy0 = (float)top; fy0 = fy0 * sy; fy0 = fy0 + oy;
    float fy1 = (float)bottom; fy1 = fy1 * sy; fy1 = fy1 + oy;
    int32_t X0 = CVT_TRUNC(NULL, fx0), X1 = CVT_TRUNC(NULL, fx1), Y0 = CVT_TRUNC(NULL, fy0), Y1 = CVT_TRUNC(NULL, fy1);
    uint32_t col[4] = { corner(e, 0, alpha), corner(e, 1, alpha), corner(e, 2, alpha), corner(e, 3, alpha) };
    if (rt_widescreen) {   /* WIDE NATIVE: cut at the 4:3 picture's sides, the shading with it */
        float lo, hi; hud_sides(&lo, &hi);
        int32_t L = (int32_t)lo, R = (int32_t)hi;
        if (X1 > X0 && (X0 < L || X1 > R)) {
            int32_t a = X0 < L ? L : X0, b = X1 > R ? R : X1;
            if (a >= b) return;
            float ta = (float)(a - X0) / (float)(X1 - X0), tb = (float)(b - X0) / (float)(X1 - X0);
            /* corners 0 top-left, 1 top-right, 2 bottom-right, 3 bottom-left */
            uint32_t c0 = mix(col[0], col[1], ta), c1 = mix(col[0], col[1], tb);
            uint32_t c3 = mix(col[3], col[2], ta), c2 = mix(col[3], col[2], tb);
            col[0] = c0; col[1] = c1; col[2] = c2; col[3] = c3;
            X0 = a; X1 = b;
        }
    }
    gaddr xy = RECT_XY;
    *H(xy + 0) = (uint16_t)X0;  *H(xy + 2) = (uint16_t)Y0;
    *H(xy + 4) = (uint16_t)X1;  *H(xy + 6) = (uint16_t)Y0;
    *H(xy + 8) = (uint16_t)X1;  *H(xy + 10) = (uint16_t)Y1;
    *H(xy + 12) = (uint16_t)X0; *H(xy + 14) = (uint16_t)Y1;
    for (int i = 0; i < 4; i++) *W(RECT_COLOUR + 4u * (uint32_t)i) = col[i];
    float z = (float)(int32_t)*H(DEPTHS + *W(DRAW_STATE + 44) * 2u);
    memcpy(&c->f[12], &z, 4);
    CALL3(f_089E1618, RECT_LIST, 4, alpha == 0xFF ? 0x20 : 0x60);
}

/* 0x0880DC08: the health bars */
static void part_bars(Cpu *c, gaddr o) {
    draw_element(c, o + 0x234);
    draw_rect(c, o + 0x274, 0x80); draw_rect(c, o + 0x1F4, 0xFF); draw_rect(c, o + 0x1B4, 0xFF);
    draw_element(c, o + 0x8B4);
    draw_rect(c, o + 0x874, 0x80); draw_rect(c, o + 0x834, 0xFF); draw_rect(c, o + 0x7F4, 0xFF);
    draw_element(c, o + 0x34);
}

/* 0x0880DC88: layer 0, the bottom of the screen: each side's super bar and its levels (+16 / +36: 0
   the fill as a rectangle, 1 three rectangles, else three pictures) */
static void part_bottom(Cpu *c, gaddr o) {
    draw_element(c, o + 0x434); draw_element(c, o + 0x474); draw_element(c, o + 0x5F4);
    if ((int8_t)*B(o + 16) == 0) draw_rect(c, o + 0x3F4, 0xFF);
    if ((int8_t)*B(o + 16) == 1) { draw_rect(c, o + 0x5B4, 0xFF); draw_rect(c, o + 0x574, 0xFF); draw_rect(c, o + 0x534, 0xFF); }
    else { draw_element(c, o + 0x5B4); draw_element(c, o + 0x574); draw_element(c, o + 0x534); }
    draw_element(c, o + 0x4B4); draw_element(c, o + 0x4F4); draw_element(c, o + 0x134); draw_element(c, o + 0x174);
    draw_element(c, o + 0xA74); draw_element(c, o + 0xAB4); draw_element(c, o + 0xC34);
    if ((int8_t)*B(o + 36) == 0) draw_rect(c, o + 0xA34, 0xFF);
    if ((int8_t)*B(o + 36) == 1) { draw_rect(c, o + 0xBF4, 0xFF); draw_rect(c, o + 0xBB4, 0xFF); draw_rect(c, o + 0xB74, 0xFF); }
    else { draw_element(c, o + 0xBF4); draw_element(c, o + 0xBB4); draw_element(c, o + 0xB74); }
    draw_element(c, o + 0xB34); draw_element(c, o + 0xAF4); draw_element(c, o + 0x774); draw_element(c, o + 0x7B4);
}

/* 0x0880DE2C: each side's guard marks, four pictures when it can guard (the battle's +56 clear, the
   fighter's +313 clear and none of its states 0x820010), else one; then the stun bar's fill */
static int can_guard(gaddr f) {
    return (int8_t)*B(f + 313) == 0 && !(*W(f + 792) & GUARD_STATES);
}
static void part_guard(Cpu *c, gaddr o) {
    int any = 0;
    if (S16(BATTLE + 56) == 0 && can_guard(FIGHTER_1)) {
        draw_element(c, o + 0x6F4); draw_element(c, o + 0x6B4); draw_element(c, o + 0x634); draw_element(c, o + 0x734);
        any = 1;
    } else draw_element(c, o + 0x674);
    if (S16(BATTLE + 56) == 0 && can_guard(FIGHTER_2)) {
        draw_element(c, o + 0xD34); draw_element(c, o + 0xCF4); draw_element(c, o + 0xC74); draw_element(c, o + 0xD74);
        any = 1;
    } else draw_element(c, o + 0xCB4);
    if ((int8_t)*B(FIGHTER_1 + 313) != 0 && (int8_t)*B(FIGHTER_2 + 313) != 0) return;
    if ((*W(FIGHTER_1 + 792) & GUARD_STATES) && (*W(FIGHTER_2 + 792) & GUARD_STATES)) return;
    if (any) draw_rect(c, o + 0xF4, 0xFF);
}

/* 0x0880DF98, 0x0880DFC8: elements drawn as they are */
static void part_two(Cpu *c, gaddr o) { draw_element(c, o + 0x74); draw_element(c, o + 0xB4); }
static void part_ten(Cpu *c, gaddr o) {
    static const uint16_t at[] = { 0x3B4, 0x374, 0x334, 0x2F4, 0x2B4, 0x9F4, 0x9B4, 0x974, 0x934, 0x8F4 };
    for (unsigned i = 0; i < sizeof at / sizeof at[0]; i++) draw_element(c, o + at[i]);
}

SFA_FUNC(0880E038, hud_draw_element) { draw_element(c, ARG(0)); }
SFA_FUNC(08810B74, hud_draw_rect)    { c->r[29] -= 0x20; draw_rect(c, ARG(0), ARG(1)); c->r[29] += 0x20; }
SFA_FUNC(0880DC08, hud_bars)         { c->r[29] -= 0x20; part_bars(c, ARG(0)); c->r[29] += 0x20; }
SFA_FUNC(0880DC88, hud_bottom)       { c->r[29] -= 0x40; part_bottom(c, ARG(0)); c->r[29] += 0x40; }
SFA_FUNC(0880DE2C, hud_guard)        { c->r[29] -= 0x30; part_guard(c, ARG(0)); c->r[29] += 0x30; }
SFA_FUNC(0880DF98, hud_two)          { c->r[29] -= 0x20; part_two(c, ARG(0)); c->r[29] += 0x20; }
SFA_FUNC(0880DFC8, hud_ten)          { c->r[29] -= 0x20; part_ten(c, ARG(0)); c->r[29] += 0x20; }
