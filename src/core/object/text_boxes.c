/* The text boxes of the text pass (0x0880C1AC, frame.c): three records of 0x30 bytes at 0x092E3E08,
 * each drawn by 0x088108D8 while in use (+0) and shown (+1). They carry the lines spoken before a rival
 * or final fight (the two portraits' dialogue).
 *
 * A record (offsets used here): +10 the depth index, +14 the font's first glyph, +26 x, +30 y (16.0
 * screen units before the scale), +35 the characters to draw, +36 the string. The string starts with a
 * line's x offset (s8), then its characters; a 0 starts a new line 16 lower, its own offset next. Each
 * character is an 8 by 16 glyph from texture rows 0x50 on (32 to a row), drawn as one quad of four
 * vertices through 0x089E1950. A new line's 0 is not counted.
 *
 * WIDE FORCED: in a fight's dialogue the lines move with the portrait they belong to, the one in their
 * half of the screen, which the sprite drawer moves out to the 16:9 edge (widescreen_text_shift in
 * extensions/widescreen.c). Only the drawing changes.
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA).
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE FORCED: extensions/widescreen.c (extensions_off.c: 0) */
int widescreen_text_shift(gaddr rec);

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline float     F(gaddr a) { float f; memcpy(&f, guest_ptr(a), 4); return f; }

void f_089E1950(Cpu *c);   /* draws textured vertices: a0, a1 count, a2 flags, a3 blend, t0 effect */
void f_088108D8(Cpu *c);   /* this file: one text box */

#define TEXT_BOXES  0x092E3E08u
#define DRAW_STATE  0x08AD9320u   /* +0 x, +2 y, +8 / +10 the glyph's texels, +44 depth index, +76 flags */
#define DEPTHS      0x08AE03B8u
#define SCALE_X     0x08E759E8u
#define SCALE_Y     0x08E759ECu
#define OFFSET_X    0x08E75A04u
#define OFFSET_Y    0x08E75A0Cu
#define GLYPH_FLAGS 0x8C
enum { TB_DEPTH = 10, TB_FONT = 14, TB_X = 26, TB_Y = 30, TB_COUNT = 35, TB_TEXT = 36 };

static int16_t to_screen(int32_t v, float scale, float offset) {
    float f = (float)v;
    f = f * scale;
    f = f + offset;
    return (int16_t)CVT_TRUNC(NULL, f);
}

static void draw_glyph(Cpu *c, gaddr ds, gaddr rec, uint32_t ch, gaddr v) {
    float sx = F(SCALE_X), ox = F(OFFSET_X), sy = F(SCALE_Y), oy = F(OFFSET_Y);
    int32_t x = S16(ds), y = S16(ds + 2);
    int16_t x0 = to_screen(x, sx, ox), x1 = to_screen(x + 8, sx, ox);
    int16_t y0 = to_screen(y, sy, oy), y1 = to_screen(y + 16, sy, oy);
    uint32_t glyph = ((ch + *B(rec + TB_FONT)) & 0xFFFF) - 1;
    glyph &= 0xFFFF;
    int16_t u = (int16_t)((glyph & 0x1F) << 3);
    int16_t t = (int16_t)((((int32_t)glyph >> 5) << 4) + 0x50);
    uint16_t z = *H(DEPTHS + *W(ds + 44) * 2u);
    *H(ds + 8) = (uint16_t)u;
    *H(ds + 10) = (uint16_t)t;
    /* top left, bottom left, top right, bottom right: x, y, depth, u, v, colour */
    const int16_t xy[4][2] = { { x0, y0 }, { x0, y1 }, { x1, y0 }, { x1, y1 } };
    const int16_t uv[4][2] = { { u, t }, { u, (int16_t)(t + 16) }, { (int16_t)(u + 8), t }, { (int16_t)(u + 8), (int16_t)(t + 16) } };
    for (int i = 0; i < 4; i++) {
        gaddr p = v + 16u * (uint32_t)i;
        *H(p + 0) = (uint16_t)xy[i][0]; *H(p + 2) = (uint16_t)xy[i][1]; *H(p + 4) = z;
        *H(p + 6) = (uint16_t)uv[i][0]; *H(p + 8) = (uint16_t)uv[i][1]; *W(p + 12) = 0xFFFFFFFFu;
    }
    *W(ds + 76) = GLYPH_FLAGS;
    CALL5(f_089E1950, v, 4, GLYPH_FLAGS, 1, 0);
    *H(ds) = (uint16_t)(S16(ds) + 8);
}

static void draw_box(Cpu *c, gaddr rec, gaddr frame) {
    gaddr ds = DRAW_STATE, s = *W(rec + TB_TEXT);
    int32_t shift = rt_widescreen ? widescreen_text_shift(rec) : 0;
    *H(ds + 2) = *H(rec + TB_Y);
    *H(ds) = (uint16_t)(S16(rec + TB_X) + (int8_t)*B(s) + shift);
    *W(ds + 44) = *B(rec + TB_DEPTH);
    s++;
    for (int32_t i = 0; i < (int32_t)*B(rec + TB_COUNT);) {
        uint32_t ch = (uint32_t)(int8_t)*B(s) & 0xFFFF;
        s++;
        if (ch == 0) {   /* a new line, its offset next */
            *H(ds + 2) = (uint16_t)(S16(ds + 2) + 16);
            *H(ds) = (uint16_t)(S16(rec + TB_X) + (int8_t)*B(s) + shift);
            s++;
            continue;
        }
        draw_glyph(c, ds, rec, ch, frame + 32);
        i++;
    }
}

static void draw_boxes(Cpu *c) {
    for (gaddr rec = TEXT_BOXES; rec < TEXT_BOXES + 3 * 0x30; rec += 0x30)
        if (*B(rec) != 0 && *B(rec + 1) != 0) CALL1(f_088108D8, rec);
}

SFA_FUNC(0880C1AC, text_boxes_draw) { c->r[29] -= 0x20; draw_boxes(c); c->r[29] += 0x20; }
SFA_FUNC(088108D8, text_box_draw) { gaddr rec = ARG(0); c->r[29] -= 0x90; draw_box(c, rec, c->r[29]); c->r[29] += 0x90; }
