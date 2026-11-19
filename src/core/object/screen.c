/* The screen's setup in the text pass (object/frame.c), the parts WIDE NATIVE changes.
 *
 * WIDE NATIVE: after Rickow's 16:9 patch (sfa3-PSP-widescreen, MIT), chosen by rt_widescreen
 * (docs/WIDESCREEN.md).
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA) and round
 * differently from the PSP.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE (extensions/widescreen.c; extensions_off.c: 0) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

#define SCALE_X     0x08E759E8u   /* floats: screen = position * scale + offset */
#define SCALE_Y     0x08E759ECu
#define OFFSET_X    0x08E75A04u
#define DRAW_STATE  0x08AD9320u   /* the sprite drawers' state (object/sprite_grid.c) */
#define DEPTHS      0x08AE03B8u   /* u16[9]: depth by index */
#define TEXT_EXTRA  0x092E22E0u   /* u32: a further text layer to draw; +4 its picture */
#define OFFSET_Y    0x08E75A0Cu
#define TEXTURES    0x08E75A18u   /* u32[]: texture page descriptors */
#define GAME        0x08E75910u   /* +189: s8, 1 = no wallpaper bars on this screen */
#define OPTIONS     0x08A058A0u   /* +0x1FE: SCREEN SETTING > WALLPAPER (0 NONE, 1 - 10) */
#define SCREEN_W    384.0f        /* WIDE NATIVE: 480 */
#define SCREEN_W_WIDE 480.0f

static int16_t scaled(float scale, float n) { float f = scale; f = f * n; return (int16_t)CVT_TRUNC(NULL, f); }

/* The sprite cells' sizes on screen (16 and 32 pixels, through the scale), the right edge they are
   drawn to (the screen's width through the scale and offset), the depth table, and the extra text
   layer's picture cleared (0xE000 words of 0x80008000) if there is one. */
static void screen_setup(void) {
    *H(DRAW_STATE + 12) = (uint16_t)scaled(F(SCALE_X), 16.0f);
    *H(DRAW_STATE + 14) = (uint16_t)scaled(F(SCALE_Y), 16.0f);
    *H(DRAW_STATE + 16) = (uint16_t)scaled(F(SCALE_X), 32.0f);
    *H(DRAW_STATE + 18) = (uint16_t)scaled(F(SCALE_Y), 32.0f);
    float right = F(SCALE_X);
    right = right * (rt_widescreen ? SCREEN_W_WIDE : SCREEN_W);
    right = right + F(OFFSET_X);
    *H(DRAW_STATE + 20) = (uint16_t)CVT_TRUNC(NULL, right);
    static const uint16_t depth[9] = { 2, 4, 6, 8, 0xA, 0xC, 0xE, 0x10, 0x11 };
    for (int i = 0; i < 9; i++) *H(DEPTHS + (uint32_t)i * 2u) = depth[i];
    if (*W(TEXT_EXTRA) != 0) {
        gaddr p = *W(TEXT_EXTRA + 4);
        for (uint32_t n = 0xE000; n != 0; n--, p += 4) *W(p) = 0x80008000u;
    }
}

void f_089E14D8(Cpu *c);   /* a texture from a picture: a0 its palette, a1 the picture, a2 flags, a3 */
void f_089E0CF4(Cpu *c);   /* selects a texture page (a0 its descriptor) */
void f_089E0EA4(Cpu *c);   /* selects a palette (a0) */
void f_089E1438(Cpu *c);   /* returns the current texture's draw flags */
void f_089E1950(Cpu *c);   /* draws textured vertices: a0, a1 count, a2 flags, a3 blend, t0 effect */
void f_0880A930(Cpu *c);   /* the screen's sides without a wallpaper */

/* A quad's 4 vertices on the frame, 16 bytes each (+0 x, +2 y, +4 depth, +6 u, +8 v, +12 colour), in
   the order top-left, bottom-left, top-right, bottom-right */
static void quad(gaddr v, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t z, int16_t u1, int16_t v1) {
    const int16_t xy[4][2] = { { x0, y0 }, { x0, y1 }, { x1, y0 }, { x1, y1 } };
    const int16_t uv[4][2] = { { 0, 0 }, { 0, v1 }, { u1, 0 }, { u1, v1 } };
    for (int i = 0; i < 4; i++) {
        *H(v + i * 16 + 0) = (uint16_t)xy[i][0]; *H(v + i * 16 + 2) = (uint16_t)xy[i][1];
        *H(v + i * 16 + 4) = z;
        *H(v + i * 16 + 6) = (uint16_t)uv[i][0]; *H(v + i * 16 + 8) = (uint16_t)uv[i][1];
        *W(v + i * 16 + 12) = 0xFFFFFFFFu;
    }
}

/* The extra text layer (0x092E22E0, cleared here): its picture over the whole screen, 384 texels wide
   (WIDE NATIVE: 480), 224 high, to the right edge the sprites are drawn to. */
static void text_layer(Cpu *c, gaddr frame) {
    *W(TEXT_EXTRA) = 0;
    CALL4(f_089E14D8, 0xFF, *W(TEXT_EXTRA + 4), 0x40000000u, 0);
    int16_t x0 = (int16_t)CVT_TRUNC(NULL, F(OFFSET_X)), y0 = (int16_t)CVT_TRUNC(NULL, F(OFFSET_Y));
    float bottom = F(SCALE_Y);
    bottom = bottom * 224.0f;
    bottom = bottom + F(OFFSET_Y);
    int16_t y1 = (int16_t)CVT_TRUNC(NULL, bottom), x1 = (int16_t)*H(DRAW_STATE + 20);
    gaddr v = frame + 32;
    quad(v, x0, y0, x1, y1, *H(DEPTHS + 14), rt_widescreen ? 0x1E0 : 0x180, 0xE0);
    CALL5(f_089E1950, v, 4, 0xFF, 1, 0);
}

/* The wallpaper (SCREEN SETTING > WALLPAPER) in the bars either side of the 4:3 picture: x -48 - 0 and
   384 - 432, y 8 - 232. With NONE, or on screens without the bars (GAME + 189 = 1), 0x0880A930.
   WIDE NATIVE: nothing (the picture reaches the sides). */
static void pillarbox(Cpu *c, gaddr frame) {
    uint8_t wallpaper = *B(OPTIONS + 0x1FE);
    if (wallpaper == 0) { CALL1(f_0880A930, 0); return; }
    if ((int8_t)*B(GAME + 189) == 1) { CALL1(f_0880A930, 0x08E70000u); return; }
    CALL1(f_089E0CF4, *W(TEXTURES + 156));
    CALL1(f_089E0EA4, (uint32_t)wallpaper - 1u);
    gaddr v = frame + 32;
    quad(v, -48, 8, 0, 0xE8, 0x15, 0x30, 0xE0);
    f_089E1438(c);
    CALL5(f_089E1950, v, 4, c->r[2], 0, 0);
    quad(v, 0x180, 8, 0x1B0, 0xE8, 0x15, 0x30, 0xE0);
    f_089E1438(c);
    CALL5(f_089E1950, v, 4, c->r[2], 0, 0);
}

SFA_FUNC(0880B928, game_screen_setup) { screen_setup(); }
SFA_FUNC(0880F160, game_text_layer) { c->r[29] -= 0x70; text_layer(c, c->r[29]); c->r[29] += 0x70; }
SFA_FUNC(0880F390, game_pillarbox) {
    if (rt_widescreen) return;
    c->r[29] -= 0x70; pillarbox(c, c->r[29]); c->r[29] += 0x70;
}
