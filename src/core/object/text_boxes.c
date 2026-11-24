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
 * WIDE NATIVE: in a fight's dialogue (widescreen_text_shift in extensions/widescreen.c says when) each
 * line is moved so that its middle is where WIDE shows it (1.25 times its place in the 4:3 picture):
 * centred as in WIDE, between the portraits moved out to the 16:9 edges (0.99.36; before, the lines went
 * 64 out with their portrait). Only the drawing changes.
 *
 * Every float multiply and add is its own statement, so the compiler can't fuse them (FMA).
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */
int widescreen_text_shift(gaddr rec);

#include "../guest_mem.h"
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

/* WIDE NATIVE: a dialogue line (offset at `s`, its characters up to the next 0) moved so its middle is
   where WIDE shows it: WIDE draws the 4:3 picture 1.25 times as wide, so a line whose middle is at x in
   the 4:3 picture has it at 1.25 x on the 16:9 screen (the line's own 8-pixel glyphs stay 8 wide here) */
#define WIDE_SCALE 1.25f
#define PICTURE_LEFT 48.0f   /* the 4:3 picture's left side on the 16:9 screen */
static int32_t line_shift(gaddr rec, gaddr s) {
    int32_t n = 0;
    while (*B(s + 1u + (uint32_t)n) != 0 && n < 255) n++;
    float sx = F(SCALE_X), ox = F(OFFSET_X);
    if (sx <= 0.0f) return 0;
    float mid = (float)(S16(rec + TB_X) + (int8_t)*B(s) + n * 4);   /* the line's middle, in units */
    float now = mid * sx; now = now + ox;                              /* on the screen */
    float want = now - PICTURE_LEFT; want = want * WIDE_SCALE;         /* where WIDE has it */
    float d = (want - now) / sx;
    return (int32_t)(d < 0.0f ? d - 0.5f : d + 0.5f);
}

/* WIDE NATIVE, an ending's lines (extensions/widescreen.c: widescreen_in_ending): the 4:3 screen keeps
   them in its left part (their middle near x 136), the right being the staff credits'. While the credits
   roll (widescreen_ending_credits) each line goes CREDITS_SHIFT further left, into the 16:9 screen's left
   margin, leaving the credits more room; otherwise there are no credits, and the lines are laid out again
   across the 16:9 screen: joined, wrapped at WRAP_CHARS and each centred on the screen's middle (x 192),
   from the first line's height. As the 4:3 screen does, only the first +35 characters are shown (a line's
   end is not counted; a space joining two lines shows with the character after it). */
int widescreen_in_ending(void);
int widescreen_ending_credits(void);
#define CREDITS_SHIFT (-40)
#define WRAP_CHARS    44
#define MIDDLE_X      192
static void draw_box_wide(Cpu *c, gaddr rec, gaddr s, gaddr frame) {
    char ch[512]; int16_t at[512]; int n = 0, shown = 0;   /* the characters joined, each one's count */
    int32_t count = (int32_t)*B(rec + TB_COUNT);
    s++;
    for (int guard = 0; guard < 1024 && n < 500; guard++) {
        uint8_t k = *B(s++);
        if (k == 0) {
            if (*B(s + 1) == 0 || shown >= 255) break;   /* the text's end: a line with nothing on it */
            s++;                                          /* a new line: its offset */
            if (n > 0 && ch[n - 1] != ' ') { ch[n] = ' '; at[n] = (int16_t)shown; n++; }
            continue;
        }
        if (k == ' ' && (n == 0 || ch[n - 1] == ' ')) { shown++; continue; }
        ch[n] = (char)k; at[n] = (int16_t)shown; n++; shown++;
    }
    while (n > 0 && ch[n - 1] == ' ') n--;
    gaddr ds = DRAW_STATE;
    int32_t y = S16(rec + TB_Y);
    *W(ds + 44) = *B(rec + TB_DEPTH);
    for (int start = 0; start < n;) {
        int end = start + WRAP_CHARS < n ? start + WRAP_CHARS : n;
        if (end < n) {                          /* break at the last space that fits */
            int b = end;
            while (b > start && ch[b] != ' ') b--;
            if (b > start) end = b;
        }
        int len = end - start;
        *H(ds + 2) = (uint16_t)y;
        *H(ds) = (uint16_t)(MIDDLE_X - len * 4);
        for (int i = start; i < end; i++) if (at[i] < count) draw_glyph(c, ds, rec, (uint32_t)(uint8_t)ch[i], frame + 32);
        y += 16;
        start = end;
        while (start < n && ch[start] == ' ') start++;
    }
}

static void draw_box(Cpu *c, gaddr rec, gaddr frame) {
    gaddr ds = DRAW_STATE, s = *W(rec + TB_TEXT);
    if (rt_widescreen && widescreen_in_ending() && !widescreen_ending_credits()) { draw_box_wide(c, rec, s, frame); return; }
    int32_t ending_shift = rt_widescreen && widescreen_in_ending() ? CREDITS_SHIFT : 0;
    int dialogue = rt_widescreen && widescreen_text_shift(rec) != 0;
    int32_t shift = dialogue ? line_shift(rec, s) : ending_shift;
    *H(ds + 2) = *H(rec + TB_Y);
    *H(ds) = (uint16_t)(S16(rec + TB_X) + (int8_t)*B(s) + shift);
    *W(ds + 44) = *B(rec + TB_DEPTH);
    s++;
    for (int32_t i = 0; i < (int32_t)*B(rec + TB_COUNT);) {
        uint32_t ch = (uint32_t)(int8_t)*B(s) & 0xFFFF;
        s++;
        if (ch == 0) {   /* a new line, its offset next */
            *H(ds + 2) = (uint16_t)(S16(ds + 2) + 16);
            if (dialogue) shift = line_shift(rec, s);
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
