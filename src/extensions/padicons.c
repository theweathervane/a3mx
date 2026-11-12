/* The MODE MENU bar's two ends (0.49.0):
 *   - on the left, a numbered bubble for each gamepad plugged in: 1 - 4 in that player's colours, red,
 *     blue and green as the game's own 1P / 2P / 3P boxes, yellow for 4 (ours). Gamepad k is player k
 *     (host_sdl.c); one that is not plugged in has no bubble, and the others keep their places;
 *   - on the right, where 3P and 4P's pad icons used to be, the frame timing the player is actually
 *     getting (pacing.c, pacing_given): ORIGINAL in a red box, SMOOTH in a blue one, white capitals
 *     with a dark outline. With FRAME TIMING AUTO it says which AUTO picked, which it does when this
 *     menu opens (pacing_decide); SMOOTH or ORIGINAL chosen in SCREEN SETTING is always what it says.
 * Each has a light outline and a black shadow, like the game's boxes, so it stands out on the menu's
 * background.
 *
 * Drawn while the MODE MENU's running state (0x0888C5B0) runs, at the end of the text pass
 * (selectscreen.c calls padicons_draw), as textured quads through the game's 2D library like JOIN /
 * WAIT: our 16-bit texture on page 0x3D only while drawing, layer 22. */
#include "rt.h"
#include <string.h>
#include "padicon_art.h"                    /* the bubbles' pixels, shared with the launcher's Controllers tab */

int host_pad_connected(int i);
extern int rt_human[3];
int rt_pads_forced = -1;                    /* --pads: the gamepads taken as plugged in (bit k = pad k + 1), tests */
void f_089E0CF4(Cpu *c);
void f_089E1618(Cpu *c);

#define PAGES      0x08B0C334u
#define TEXT_TEXTURES 0x08A05080u
#define OUR_PAGE   0x3D
#define LAYER      22
#define TEX_W      128
#define TEX_H      32
#define BUB        16                       /* a bubble: 13 across, its outline and shadow (u = BUB * k, v 0) */
#define BADGE_W    50                       /* a timing badge: the box, its frame and shadow (v 16; u 0 ORIGINAL, 64 SMOOTH) */
#define BADGE_H    14
#define BADGE_V    16
/* Drawn on the game's own pixel grid, the text pass's 384 x 224 (stretched to the 480 x 272 screen),
 * one texel a screen pixel like the rest of the menu. */
#define EDGE       2                        /* from the bar's ends */
#define BAR_Y      13                       /* the menu bar is 12 - 28: centred in it */
#define STEP       17                       /* a pixel between two bubbles */

/* capitals, 7 high, for ORIGINAL and SMOOTH */
static const struct { char ch; const char *row[7]; } letters[] = {
    { 'O', { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
    { 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
    { 'I', { "###", ".#.", ".#.", ".#.", ".#.", ".#.", "###" } },
    { 'G', { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###." } },
    { 'N', { "#...#", "##..#", "#.#.#", "#.#.#", "#.#.#", "#..##", "#...#" } },
    { 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
    { 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
    { 'S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
    { 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
    { 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
    { 'H', { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
};
/* per timing (0 ORIGINAL, 1 SMOOTH): text outline, box */
static const uint32_t badge_colours[2][2] = {
    { 0x3A0000, 0xC01818 },                 /* ORIGINAL: red */
    { 0x00103A, 0x1840C0 },                 /* SMOOTH: blue */
};
#define LIGHT 0xF7F7E7                      /* the outline round each, like the boxes' frame */
#define WHITE 0xFFFFFF
#define SET   (1u << 24)                    /* a pixel that is drawn (black is SET alone) */

static uint16_t c5551(uint32_t rgb) { return (uint16_t)(0x8000 | ((rgb >> 19) & 31) | (((rgb >> 11) & 31) << 5) | (((rgb >> 3) & 31) << 10)); }

/* a W x H picture, built up in layers */
#define PW 64
#define PH 16
typedef struct { uint32_t px[PH][PW]; } Pic;
static int near_set(const Pic *p, int y, int x, int diagonals) {
    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
        if (!(dx || dy) || (!diagonals && dx && dy)) continue;
        int yy = y + dy, xx = x + dx;
        if (yy >= 0 && yy < PH && xx >= 0 && xx < PW && p->px[yy][xx]) return 1;
    }
    return 0;
}
/* ink (1 where set) at x, y in `fill`, with a one-pixel outline in `edge` all round */
static void ink(Pic *p, const uint8_t m[PH][PW], int x0, int y0, uint32_t fill, uint32_t edge) {
    Pic tmp; memset(&tmp, 0, sizeof tmp);
    for (int y = 0; y < PH; y++) for (int x = 0; x < PW; x++) if (m[y][x]) tmp.px[y][x] = 1;
    for (int y = 0; y < PH; y++) for (int x = 0; x < PW; x++) {
        int yy = y0 + y - 1, xx = x0 + x - 1;
        if (yy < 0 || yy >= PH || xx < 0 || xx >= PW) continue;
        if (tmp.px[y][x]) p->px[yy][xx] = fill | SET;
        else if (near_set(&tmp, y, x, 1)) p->px[yy][xx] = edge | SET;
    }
}
/* the light outline round what is drawn, then the black shadow down and right */
static void frame_and_shadow(Pic *p) {
    Pic out = *p;
    for (int y = 0; y < PH; y++) for (int x = 0; x < PW; x++) if (!p->px[y][x] && near_set(p, y, x, 0)) out.px[y][x] = LIGHT | SET;
    for (int y = PH - 1; y >= 1; y--) for (int x = PW - 1; x >= 1; x--)
        if (!out.px[y][x] && out.px[y - 1][x - 1] && out.px[y - 1][x - 1] != SET) out.px[y][x] = SET;
    *p = out;
}
static void put(uint32_t m, const Pic *p, int u, int v, int w, int h) {
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        WH(m + (uint32_t)(((v + y) * TEX_W + u + x) * 2), p->px[y][x] ? c5551(p->px[y][x] & 0xFFFFFF) : 0);
}
static int glyph(char ch) { for (size_t i = 0; i < sizeof letters / sizeof *letters; i++) if (letters[i].ch == ch) return (int)i; return -1; }
static int text_width(const char *t) {
    int w = 0;
    for (; *t; t++) { int g = glyph(*t); if (g >= 0) w += (int)strlen(letters[g].row[0]) + (t[1] ? 1 : 0); }
    return w;
}

static uint32_t tex, desc, list, prim;
static int build(void) {
    if (tex) return 1;
    uint32_t m = rt_alloc(TEX_W * TEX_H * 2 + 0x100, 1, 256, "pad icons");
    if (!m) return 0;
    memset(HOST(m), 0, TEX_W * TEX_H * 2);
    static uint8_t mask[PH][PW];
    for (int k = 0; k < 4; k++) {           /* the bubbles (padicon_art.h: the number one right of centre since 0.50.1, the maintainer's eye) */
        uint32_t b[PADICON_SIZE][PADICON_SIZE];
        padicon_bubble(k, b);
        Pic p; memset(&p, 0, sizeof p);
        for (int y = 0; y < BUB; y++) for (int x = 0; x < BUB; x++) p.px[y][x] = b[y][x];
        put(m, &p, BUB * k, 0, BUB, BUB);
    }
    for (int t = 0; t < 2; t++) {           /* the badges: a box 47 x 11 at 1, 1, the word centred in it */
        const char *word = t ? "SMOOTH" : "ORIGINAL";
        Pic p; memset(&p, 0, sizeof p);
        for (int y = 0; y < 11; y++) for (int x = 0; x < BADGE_W - 3; x++) p.px[y + 1][x + 1] = badge_colours[t][1] | SET;
        memset(mask, 0, sizeof mask);
        int x = 1;
        for (const char *ch = word; *ch; ch++) {
            int g = glyph(*ch); if (g < 0) continue;
            int w = (int)strlen(letters[g].row[0]);
            for (int y = 0; y < 7; y++) for (int i = 0; i < w; i++) mask[y + 1][x + i] = letters[g].row[y][i] == '#';
            x += w + 1;
        }
        int tx = 1 + (BADGE_W - 3 - text_width(word)) / 2;
        ink(&p, (const uint8_t (*)[PW])mask, tx, 3, WHITE, badge_colours[t][0]);
        frame_and_shadow(&p);
        put(m, &p, 64 * t, BADGE_V, BADGE_W, BADGE_H);
    }
    tex = m;
    desc = m + TEX_W * TEX_H * 2;
    list = desc + 0x48; prim = list + 0x18;
    memcpy(HOST(desc), HOST(RW(PAGES + 0x14)), 0x44);   /* like page 0's (16-bit 5551) */
    WW(desc, OUR_PAGE); WW(desc + 4, 0); WW(desc + 0x18, 0x01000000); WW(desc + 0x24, TEX_W); WW(desc + 0x28, TEX_H);
    uint32_t d0 = RW(TEXT_TEXTURES);
    WW(list + 8, RW(d0)); WW(list + 12, RW(d0 + 4)); WW(list + 16, OUR_PAGE);
    WW(list, list + 8); WW(list + 4, 1);
    return 1;
}

static int plugged(int k) {
    if (rt_pads_forced >= 0) return (rt_pads_forced >> k) & 1;
    if (k >= 1 && k <= 2 && rt_human[k] >= 0) return rt_human[k];
    return host_pad_connected(k);
}

static uint64_t menu_frame = ~0ull;
/* around the MODE MENU running state (0x0888C5B0, src/core/task/mode_menu.c: SFA_FUNC_WRAPPED) */
void pacing_decide(const char *where);      /* pacing.c: FRAME TIMING AUTO chooses only here and on leaving SCREEN SETTING */
int padicons_mode_menu(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    if (after) return 0;
    uint64_t now = rt_vblank_count();
    if (menu_frame == ~0ull || now - menu_frame > 2) pacing_decide("the MODE MENU");   /* it has just opened (the first time, or coming back) */
    menu_frame = now;
    return 0;
}

static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float ffrom(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }

static void quad(Cpu *c, int x0, int y0, int u0, int v0, int w, int h) {
    uint32_t xy = prim + 0x10, col = xy + 0x10, uv = col + 0x10;
    int x1 = x0 + w, y1 = y0 + h, u1 = u0 + w, v1 = v0 + h;
    WH(xy + 0, x0); WH(xy + 2, y0); WH(xy + 4, x1); WH(xy + 6, y0); WH(xy + 8, x1); WH(xy + 0xA, y1); WH(xy + 0xC, x0); WH(xy + 0xE, y1);
    for (int i = 0; i < 4; i++) WW(col + 4u * (uint32_t)i, 0xFFFFFFFFu);
    WH(uv + 0, u0); WH(uv + 2, v0); WH(uv + 4, u1); WH(uv + 6, v0); WH(uv + 8, u1); WH(uv + 0xA, v1); WH(uv + 0xC, u0); WH(uv + 0xE, v1);
    WW(prim, xy); WW(prim + 4, col); WW(prim + 8, uv); WW(prim + 0xC, 4);
    c->r[4] = prim; c->r[5] = 4; c->r[6] = 0x80000060u; c->f[12] = fbits(LAYER);
    c->r[31] = 0x0880B0E8; f_089E1618(c);
}

int pacing_given(void);                     /* pacing.c: 1 SMOOTH, 0 ORIGINAL */
void padicons_draw(Cpu *c) {
    uint64_t now = rt_vblank_count();
    if (menu_frame == ~0ull || now - menu_frame > 2) return;
    if (!build()) return;
    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint32_t e = PAGES + 0x18u * OUR_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(e + 4u * (uint32_t)i);
    WW(e, tex); WW(e + 4, tex); WW(e + 8, 0); WW(e + 0xC, 0); WW(e + 0x10, 0x80000000u); WW(e + 0x14, desc);
    c->r[4] = list; c->r[31] = 0x0880B0E8; f_089E0CF4(c);
    float sx = ffrom(RW(0x08E759E8)), sy = ffrom(RW(0x08E759EC)), ox = ffrom(RW(0x08E75A04)), oy = ffrom(RW(0x08E75A0C));
    float mid = BAR_Y * sy + oy - 0.5f;     /* the bar's middle, on screen (as the old 15-high pad icons) */
    /* 0.50.1: the bar fills screen rows 5 - 23 (middle 14); a bubble's disc (texture rows 1 - 13) and a
       badge's box (rows 1 - 11) are centred on it. Before, the discs were 2 rows low, the badges 1. */
    for (int k = 0; k < 4; k++)             /* the bubbles from the bar's left end, each in its own place */
        if (plugged(k)) quad(c, (int)(EDGE * sx + ox + 0.5f) + STEP * k, (int)(mid - 9.0f + 0.5f), BUB * k, 0, BUB, BUB);
    int t = pacing_given() ? 1 : 0;         /* the timing badge at the bar's right end */
    quad(c, (int)((384 - EDGE) * sx + ox + 0.5f) - BADGE_W, (int)(mid - 7.5f + 0.5f), 64 * t, BADGE_V, BADGE_W, BADGE_H);
    c->r[4] = TEXT_TEXTURES; c->r[31] = 0x0880B0E8; f_089E0CF4(c);
    for (int i = 0; i < 6; i++) WW(e + 4u * (uint32_t)i, keep_page[i]);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
}
