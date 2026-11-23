/* WIDE NATIVE, the scaled endings: a scene cut from a stage gets the stage's own art past its sides.
 *
 * Some endings' scenes are a stage's art cut to the 24 columns of the 4:3 picture and kept in the
 * ending's own maps and textures (endNN_u.pac): Ryu's stage behind Ryu, Sagat and Dan. Shown wide
 * (scaled_endings.c: a scene whose layers have art past the sides is carried on, not scaled), the
 * columns past the cut are bare, black or the sky behind. The stage's archive (stNN.pac) has them.
 *
 * tools/ending_stages.py finds those scenes (bands of rows of the ending's layer 1, 0x08819300) by
 * comparing their pixels with every stage's, and writes scaled_endings_stages.h: the band, a check of
 * its map words, the stage, and which of the stage's layers, each at its own place in its map, drawn in
 * which order, make up the ending's tiles there (Ryu's: layer 2's 32-pixel tiles under layer 1's).
 *
 * Here, after the ending's layer 1 is drawn (0x08819300, wrapped in core/stage/background.c), in a
 * scene drawn as it is (tiles.c's tiles_ending_mode 0): for a band of the table found in the layer's
 * map, the stage's archive is read (rt_archive_read: assets/ first, as the game's own loads), STRIP
 * columns each side of the band's art are drawn from the stage's layers into a texture of our own
 * (5551, the 4-bit textures in the colours tools/data/palette_pairs.json gives them), and that is drawn
 * at the layer's depth, placed by its scroll, on the columns past the band's art.
 *
 * Only the drawing changes; the game's memory is not touched outside our texture.
 */
#include "rt.h"
#include "ext_features.h"
#include "scaled_endings_stages.h"
#include <string.h>

int scaled_endings_on(void);                 /* scaled_endings.c */
extern int tiles_ending_mode, tiles_counting; /* core/stage/tiles.c */
void f_089E0CF4(Cpu *c);   /* selects a texture list (a0) */
void f_089E1618(Cpu *c);   /* queues a quad list: a0 its descriptor, a1 vertex count, a2 flags, f12 depth */
void f_0880A3C4(Cpu *c);   /* a draw depth: a0 the depth table's row, a1 its column */

#define LAYER_1        0x092E88A0u   /* the ending's layer 1: +18 x scroll, +98 y scroll, +122 depth column */
#define SCALE_X        0x08E759E8u
#define SCALE_Y        0x08E759ECu
#define OFFSET_X       0x08E75A04u
#define OFFSET_Y       0x08E75A0Cu
#define PAGES          0x08B0C334u   /* texture pages, 0x18 bytes each (+0x14 its descriptor) */
#define TEXT_TEXTURES  0x08A05080u
#define TEX_STATE      0x08B23334u   /* the selected texture list (8 bytes) and its first entry (12) */
#define PAL_SELECTED   0x08ACCD80u   /* u8: the palette 0x089E0EA4 last selected */
#define OUR_PAGE       0x3C          /* set only while drawing */
#define STRIP          8             /* columns drawn from the stage each side of the band (128 pixels) */
#define TEX_W          256           /* the left strip at u 0, the right at 128 */
#define TEX_H          256
#define CLIP_X0        -48
#define CLIP_X1        432
#define CLIP_Y0        8
#define CLIP_Y1        232
#define MAP_COLS       64            /* the ending's layer 1 map: 64 words a row, 32 rows */
#define MAP_ROWS       32

static uint32_t tex, desc, list, prim;
static int ready = -1;               /* the band whose strips are in tex; -1 none */
static int failed[sizeof ENDING_STAGE_BANDS / sizeof ENDING_STAGE_BANDS[0]];
static uint32_t map_arg;

static uint32_t crc32_of(const uint8_t *p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(c & 1));
    }
    return ~c;
}

/* the band of the table this map holds, or -1: its map words, column by column, match its check */
static int band_in(uint32_t map) {
    for (int b = 0; b < (int)(sizeof ENDING_STAGE_BANDS / sizeof ENDING_STAGE_BANDS[0]); b++) {
        const EndingStageBand *e = &ENDING_STAGE_BANDS[b];
        if (e->layer != 1 || failed[b]) continue;
        uint8_t words[MAP_COLS * MAP_ROWS * 4];
        size_t n = 0;
        for (int col = 0; col < MAP_COLS; col++)
            for (int row = e->row0; row <= e->row1 && row < MAP_ROWS; row++) {
                uint32_t w = RW(map + (uint32_t)(row * MAP_COLS + col) * 4u);
                if (w & 0x80000000u) { memcpy(words + n, &w, 4); n += 4; }
            }
        if (n && crc32_of(words, n) == e->crc) return b;
    }
    return -1;
}

/* ---- the stage's archive ---- */
typedef struct { const uint8_t *px; int w, bits, pf; } Page;
typedef struct { const uint8_t *data; uint32_t type, size; } Entry;

static int pac_entries(const uint8_t *f, size_t n, Entry *out, int max) {
    if (n < 0x800) return 0;
    uint32_t count, size, off = 0x800;
    memcpy(&count, f, 4);
    if (count > 254) return 0;
    int k = 0;
    for (uint32_t i = 0; i < count && k < max; i++) {
        uint32_t t; memcpy(&t, f + 8 + 8 * i, 4); memcpy(&size, f + 12 + 8 * i, 4);
        if ((size_t)off + size > n) return k;
        out[k].data = f + off; out[k].type = t; out[k].size = size; k++;
        off += (size + 0x7FFu) & ~0x7FFu;
    }
    return k;
}

/* page 0xC's texture of slot `slot`: its pixels after the PowerVR header */
static int find_page(const Entry *es, int n, int slot, Page *pg) {
    for (int i = 0; i < n; i++) {
        uint32_t t = es[i].type;
        if (((t >> 16) != 0x1000 && (t >> 16) != 0x3000) || ((t >> 8) & 0xFF) != 0x0C || (int)(t & 0xFF) != slot) continue;
        const uint8_t *e = es[i].data;
        for (uint32_t p = 0; p + 16 <= 32 && p + 16 <= es[i].size; p++) {
            if (memcmp(e + p, "PVRT", 4)) continue;
            uint8_t pf = e[p + 8], df = e[p + 9];
            uint16_t w; memcpy(&w, e + p + 12, 2);
            pg->px = e + p + 16; pg->w = w; pg->pf = pf;
            pg->bits = df == 1 ? 16 : df == 5 ? 4 : 0;
            return pg->bits != 0;
        }
    }
    return 0;
}

/* a 16-bit colour as 5551 (alpha set when opaque); 0 for transparent */
static uint16_t to5551(uint16_t v, int pf) {
    uint32_t r, g, b, a;
    if (pf == 1) { r = v & 31; g = (v >> 6) & 31; b = (v >> 11) & 31; a = 1; }
    else if (pf == 2) { r = (v & 15) << 1; g = ((v >> 4) & 15) << 1; b = ((v >> 8) & 15) << 1; a = (v >> 12) >= 8; }
    else { r = v & 31; g = (v >> 5) & 31; b = (v >> 10) & 31; a = v >> 15; }
    return a ? (uint16_t)(0x8000u | r | g << 5 | b << 10) : 0;
}

/* the stage's texel for the ending's layer pixel (x, y), from source s (its map moved by its dx, dy) */
static uint16_t stage_texel(const EndingStageBand *e, const uint8_t *maps, size_t maps_n, const Entry *es, int n,
                            const uint16_t cluts[][16], const int *clut_slot, int nclut, StageSource s, int x, int y) {
    int t = s.tile, cols = t == 16 ? 64 : 32;
    x += s.dx * 16; y += s.dy * 16;
    int mx = x / t, my = y / t;
    mx = ((mx % cols) + cols) % cols;
    uint32_t off = 0x2000u * s.layer + (uint32_t)(my * cols + mx) * 4u;
    if (my < 0 || off + 4 > maps_n || off >= 0x2000u * (s.layer + 1u)) return 0;
    uint32_t w; memcpy(&w, maps + off, 4);
    if (!(w & 0x80000000u)) return 0;
    int tx = ((x % t) + t) % t, ty = ((y % t) + t) % t;
    if (w & 0x10000000u) tx = t - 1 - tx;
    if (w & 0x20000000u) ty = t - 1 - ty;
    int slot = (int)(w >> 8) & 0xFF, cell = (int)w & 0xFF;
    Page pg;
    if (!find_page(es, n, slot, &pg)) return 0;
    int u = (cell & 0xF) * 16 + tx, v = (cell & 0xF0) + ty;
    if (u >= pg.w || v >= pg.w) return 0;
    if (pg.bits == 16) { uint16_t c; memcpy(&c, pg.px + ((size_t)v * pg.w + u) * 2, 2); return to5551(c, pg.pf); }
    uint8_t b = pg.px[((size_t)v * pg.w + u) / 2];
    int i = (u & 1) ? b >> 4 : b & 15;
    if (i == 0) return 0;
    for (int k = 0; k < nclut; k++) if (clut_slot[k] == slot) return to5551(cluts[k][i], 0);
    (void)e;
    return 0;
}

static int alloc_texture(void) {
    if (tex) return 1;
    uint32_t m = rt_alloc(TEX_W * TEX_H * 2 + 0x100, 1, 256, "ending stages");
    if (!m) return 0;
    tex = m;
    desc = m + TEX_W * TEX_H * 2;
    list = desc + 0x48; prim = list + 0x18;
    memcpy(HOST(desc), HOST(RW(PAGES + 0x14)), 0x44);    /* like page 0's (16-bit 5551) */
    WW(desc, OUR_PAGE); WW(desc + 4, 0); WW(desc + 0x18, 0x01000000); WW(desc + 0x24, TEX_W); WW(desc + 0x28, TEX_H);
    uint32_t d0 = RW(TEXT_TEXTURES);
    WW(list + 8, RW(d0)); WW(list + 12, RW(d0 + 4)); WW(list + 16, OUR_PAGE);
    WW(list, list + 8); WW(list + 4, 1);
    return 1;
}

/* the band's two strips drawn from the stage into tex: STRIP columns left of col0 at u 0, right of col1 at 128 */
static int build(int b) {
    const EndingStageBand *e = &ENDING_STAGE_BANDS[b];
    if ((e->row1 - e->row0 + 1) * 16 > TEX_H || !alloc_texture()) return 0;
    size_t n = 0;
    uint8_t *f = rt_archive_read(e->stage, &n);
    if (!f) { rt_log("scaled endings: %s could not be read, ending %02X's scene stays as it is", e->stage, e->ending); return 0; }
    Entry es[64];
    int ne = pac_entries(f, n, es, 64);
    if (ne < 1) { free(f); return 0; }
    uint16_t cluts[4][16]; int clut_slot[4], nclut = 0;
    for (int k = 0; k < e->nclut && k < 4; k++) {
        int ent = e->cluts[k].entry;
        if (ent >= ne || e->cluts[k].offset + 32u > es[ent].size) continue;
        memcpy(cluts[nclut], es[ent].data + e->cluts[k].offset, 32);
        clut_slot[nclut++] = e->cluts[k].slot;
    }
    uint16_t *out = (uint16_t *)HOST(tex);
    memset(out, 0, TEX_W * TEX_H * 2);
    for (int side = 0; side < 2; side++) {
        int c0 = side ? e->col1 + 1 : e->col0 - STRIP;
        for (int y = 0; y < (e->row1 - e->row0 + 1) * 16; y++)
            for (int x = 0; x < STRIP * 16; x++) {
                int sx = c0 * 16 + x, sy = e->row0 * 16 + y;
                uint16_t px = 0;
                for (int k = 0; k < e->nsources; k++) {
                    uint16_t t = stage_texel(e, es[0].data, es[0].size, es, ne, (const uint16_t (*)[16])cluts, clut_slot, nclut, e->sources[k], sx, sy);
                    if (t) px = t;
                }
                out[y * TEX_W + side * 128 + x] = px;
            }
    }
    free(f);
    rt_log("scaled endings: ending %02X's scene drawn on past its sides from %s", e->ending, e->stage);
    return 1;
}

static float ffrom(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static int to_screen(int v, uint32_t scale, uint32_t offset) {
    float f = (float)v;
    f = f * ffrom(RW(scale));
    float o = ffrom(RW(offset)) + 0.5f;
    return (int)f + (int)o;
}
/* v moved by a whole number of periods to be nearest `near` */
static int nearest(int v, int near, int period) {
    int d = (v - near) % period;
    if (d < 0) d += period;
    if (d > period / 2) d -= period;
    return near + d;
}

static void draw(Cpu *c, int b, uint32_t layer) {
    const EndingStageBand *e = &ENDING_STAGE_BANDS[b];
    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint8_t keep_tex[20], keep_pal = (uint8_t)RB(PAL_SELECTED);
    memcpy(keep_tex, HOST(TEX_STATE), sizeof keep_tex);
    uint32_t pe = PAGES + 0x18u * OUR_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(pe + 4u * (uint32_t)i);
    WW(pe, tex); WW(pe + 4, tex); WW(pe + 8, 0); WW(pe + 0xC, 0); WW(pe + 0x10, 0x80000000u); WW(pe + 0x14, desc);
    c->r[4] = list; c->r[31] = 0x08819300; f_089E0CF4(c);

    int h = (int16_t)RH(layer + 18) & 0x3FF, s = (int16_t)RH(layer + 98) & 0x3FF;
    c->r[4] = 1; c->r[5] = RB(layer + 122); c->r[31] = 0x08819300; f_0880A3C4(c);
    float z = (float)(int32_t)c->r[2];
    int top = nearest(e->row0 * 16 - s, 0, 1024), height = (e->row1 - e->row0 + 1) * 16;
    int y0 = to_screen(top, SCALE_Y, OFFSET_Y), y1 = to_screen(top + height, SCALE_Y, OFFSET_Y);
    for (int side = 0; side < 2; side++) {
        int c0 = side ? e->col1 + 1 : e->col0 - STRIP;
        int left = nearest(c0 * 16 - h, side ? 384 : -128, 1024);
        int x0 = to_screen(left, SCALE_X, OFFSET_X), x1 = to_screen(left + STRIP * 16, SCALE_X, OFFSET_X);
        int xa = x0 > CLIP_X0 ? x0 : CLIP_X0, xb = x1 < CLIP_X1 ? x1 : CLIP_X1;
        int ya = y0 > CLIP_Y0 ? y0 : CLIP_Y0, yb = y1 < CLIP_Y1 ? y1 : CLIP_Y1;
        if (xb <= xa || yb <= ya) continue;
        int base = side * 128;
        int u0 = base + (xa - x0) * STRIP * 16 / (x1 - x0), u1 = base + (xb - x0) * STRIP * 16 / (x1 - x0);
        int v0 = (ya - y0) * height / (y1 - y0), v1 = (yb - y0) * height / (y1 - y0);
        uint32_t xy = prim + 0x10, col = xy + 0x10, uv = col + 0x10;
        WH(xy + 0, xa); WH(xy + 2, ya); WH(xy + 4, xb); WH(xy + 6, ya); WH(xy + 8, xb); WH(xy + 0xA, yb); WH(xy + 0xC, xa); WH(xy + 0xE, yb);
        uint32_t grey = RW(0x08B014FCu);   /* the layers' colour this frame (the fade's brightness) */
        for (int k = 0; k < 4; k++) WW(col + 4u * (uint32_t)k, grey);
        WH(uv + 0, u0); WH(uv + 2, v0); WH(uv + 4, u1); WH(uv + 6, v0); WH(uv + 8, u1); WH(uv + 0xA, v1); WH(uv + 0xC, u0); WH(uv + 0xE, v1);
        WW(prim, xy); WW(prim + 4, col); WW(prim + 8, uv); WW(prim + 0xC, 4);
        c->r[4] = prim; c->r[5] = 4; c->r[6] = 0x80000020u; c->f[12] = fbits(z);
        c->r[31] = 0x08819300; f_089E1618(c);
    }

    for (int i = 0; i < 6; i++) WW(pe + 4u * (uint32_t)i, keep_page[i]);
    memcpy(HOST(TEX_STATE), keep_tex, sizeof keep_tex);
    WB(PAL_SELECTED, keep_pal);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
}

/* wraps 0x08819300 (a0 the layer, a1 its map): after the layer, its band's stage strips */
int scaled_endings_stage_layer(Cpu *c, uint32_t a0, int after) {
    if (!after) { map_arg = c->r[5]; return 0; }
    if (a0 != LAYER_1 || tiles_counting || tiles_ending_mode != 0 || !scaled_endings_on()) return 0;
    int b = band_in(map_arg);
    if (b < 0) return 0;
    if (ready != b) {
        if (!build(b)) { failed[b] = 1; return 0; }
        ready = b;
    }
    draw(c, b, a0);
    return 0;
}
