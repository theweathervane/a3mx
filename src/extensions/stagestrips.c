/* WIDE FORCED's stage extensions: new art for the 48 columns at each side of the screen that the
 * stage's own art leaves black at the camera limits (docs/STAGE_EXTENSIONS.md).
 *
 * A stage has up to 8 layers (0x092E8800, 0xA0 bytes each; layer 1 is the fighters' layer), each
 * scrolling at its own rate. Each layer can have a strip at each side, joined to that layer: drawn
 * from that layer's own drawer call, placed with its scroll (+18 x, +98 y), and just behind it (one
 * less than its depth: the game's 2D library draws a larger depth in front), so a nearer layer's art
 * and strips stay in front of a farther one's.
 *
 * Files, in the stage's folder under assets/extensions/ (the stage archive's folder as index.json
 * names it: st0c is stages/0c_dan):
 *   widescreen_left.png, widescreen_right.png                 layer 1, the fighters' layer
 *   widescreen_layer<n>_left.png, widescreen_layer<n>_right.png  any other layer n
 *   widescreen.json   where each strip goes: "layer<n>_left" / "layer<n>_right", the layer x of the
 *                     strip's left column (written by tools/measure_stage_layers.py). Without it, or
 *                     without a layer's entry, layer 1's are 208 and 1024 and other layers draw none.
 *                     "layer<n>_<side>_front": 1 draws that strip just in front of its layer (one depth
 *                     step nearer) instead of behind it, so it can paint over the layer's own art
 *                     at the edge (Charlie's stage: the bright green filler columns of layer 0).
 * Each strip is 48 x 288, row r at the layer's y r: with the camera at rest (the layer's y scroll 16)
 * the screen's top row shows row 16, and rows 0 - 15 when the camera rises. The 2D library's 224 rows
 * are stretched to the screen's 272, so rows 0 - 239 are ever seen. Rounded to 15-bit colour; alpha
 * below 128 is transparent.
 * A missing or unreadable file is skipped (the log says why).
 *
 * Drawn only with --custom-assets (the launcher's Custom Assets, off by default), while WIDE FORCED is
 * on and a fight is running (the fight's per-frame step 0x08823674,
 * src/core/task/fight.c, wraps a timestamp), around both tile drawers (0x0881AB28, 0x0881B268,
 * src/core/stage/tiles.c: SFA_FUNC_WRAPPED), as textured quads through the game's 2D library like
 * padicons.c: our 16-bit texture on page STRIP_PAGE only while drawing.
 *
 * The strips change only what is drawn (and the extension's own memory): the game's state is the same
 * with or without them, so the fighters' parity with WIDE FORCED on and off is untouched.
 *
 * Measuring (tools/measure_stage_layers.py): A3MX_STRIP_MEASURE="F0-F1:MASK,..." turns the strips off
 * for the run and, in frames F0 - F1, draws only the stage layers in MASK (bit n = layer n); bit 8
 * also draws a magenta probe over the whole screen behind everything (depth 0), so the pixels it shows
 * through are the ones nothing covers: where a strip would be seen. */
#include "rt.h"
#include "asset_pac.h"
#include "png_decode.h"
#include <string.h>

extern int rt_widescreen;
#include "ext_features.h"                 /* stage_extensions (the launcher's Stage Extensions; --custom-assets before 0.60.0): off, no strips are loaded */
extern char rt_assets_root[1024];
uint64_t rt_vblank_count(void);
void f_089E0CF4(Cpu *c);   /* selects a texture list (a0) */
void f_089E1618(Cpu *c);   /* queues a quad list: a0 its descriptor, a1 vertex count, a2 flags, f12 depth */

#define BATTLE          0x092E1228u   /* s16: the stage being fought on */
#define LAYERS          0x092E8800u   /* the stage's layers: +2 camera mode, +3 kind, +18 x / +98 y scroll, +122 depth column (32-pixel layers) */
#define LAYER_SIZE      0xA0u
#define NLAYERS         8
#define FIGHTERS        1             /* the fighters' layer */
#define FIGHTERS_LAYER  (LAYERS + FIGHTERS * LAYER_SIZE)   /* +2 camera mode: 1 once a stage is set up */
#define DEPTHS          0x08A03B10u   /* u16: the 2D library's depth by (row + 2 * column); the stage uses row 1 */
#define REC_KIND        94            /* 0x0881B268's record: +94 its layer's kind, +60 its depth column */
#define REC_DEPTH       60
#define SCALE_X         0x08E759E8u
#define SCALE_Y         0x08E759ECu
#define OFFSET_X        0x08E75A04u
#define OFFSET_Y        0x08E75A0Cu
#define PAGES           0x08B0C334u   /* texture pages, 0x18 bytes each (+0x14 its descriptor) */
#define TEXT_TEXTURES   0x08A05080u
#define TEX_STATE       0x08B23334u   /* the selected texture list (8 bytes) and its first entry (12) */
#define PAL_SELECTED    0x08ACCD80u   /* u8: the palette 0x089E0EA4 last selected */

#define STRIP_W         48
#define STRIP_H         288
#define FIGHTERS_LEFT_X 208           /* layer 1's strips without widescreen.json */
#define FIGHTERS_RIGHT_X 1024
/* The screen in the 2D library's units (WIDE FORCED: 480 across from -48, and 224 rows from 8, stretched
   to the screen's 272) */
#define CLIP_X0         -48
#define CLIP_X1         432
#define CLIP_Y0         8
#define CLIP_Y1         232
#define TEX_W           512           /* up to 8 strips side by side, 64 texels apart */
#define TEX_H           512
#define SLOTS           8
#define STRIP_PAGE      0x3E

typedef struct { int layer, side, x, slot, front; } Strip;   /* front: drawn just in front of its layer, over its art */
static Strip strips[SLOTS];
static int nstrips;
static uint32_t tex, desc, list, prim;
static int loaded = -2;               /* the stage whose strips are in tex; -2 none tried */
static uint64_t drawn[NLAYERS];       /* the frame each layer's strips were last drawn */
static uint64_t fight_frame = ~0ull;

/* ---- measuring: A3MX_STRIP_MEASURE ---- */
typedef struct { long from, to; unsigned mask; } Window;
static Window windows[16];
static int nwindows, measure_read, measuring;
static void read_measure(void) {
    if (measure_read) return;
    measure_read = 1;
    const char *s = getenv("A3MX_STRIP_MEASURE");
    if (!s || !*s) return;
    measuring = 1;
    while (*s && nwindows < 16) {
        Window w;
        if (sscanf(s, "%ld-%ld:%x", &w.from, &w.to, &w.mask) != 3) break;
        windows[nwindows++] = w;
        s = strchr(s, ',');
        if (!s) break;
        s++;
    }
    rt_log("stage strips: measuring, %d windows; strips off", nwindows);
}

static uint16_t c5551(const uint8_t *p) {
    return (uint16_t)((p[3] >= 128 ? 0x8000 : 0) | (p[0] >> 3) | ((p[1] >> 3) << 5) | ((p[2] >> 3) << 10));
}

/* The numbers you can't pick load another stage's archive (STAGES.md), and so use its strips. */
static int archive_stage(int stage) {
    switch (stage) { case 0x12: return 0x04; case 0x13: return 0x11; case 0x14: return 0x06; case 0x1F: return 0x1E; }
    return stage;
}

/* assets/extensions/<the stage's folder>, from rt_assets_root (assets/<region>); 0 if unknown */
static int strip_folder(int stage, char *out, size_t n) {
    char stem[16], folder[1200];
    snprintf(stem, sizeof stem, "st%02x", archive_stage(stage));
    if (asset_index_folder(rt_assets_root, stem, folder, sizeof folder) != 0) return 0;
    size_t root = strlen(rt_assets_root);
    if (strncmp(folder, rt_assets_root, root) != 0 || folder[root] != '/') return 0;
    char base[1024]; snprintf(base, sizeof base, "%s", rt_assets_root);
    char *slash = strrchr(base, '/');
    if (!slash) return 0;
    *slash = 0;                                            /* assets/ */
    return snprintf(out, n, "%s/extensions/%s", base, folder + root + 1) < (int)n;
}

static int alloc_texture(void) {
    if (tex) return 1;
    uint32_t m = rt_alloc(TEX_W * TEX_H * 2 + 0x100, 1, 256, "stage strips");
    if (!m) return 0;
    tex = m;
    desc = m + TEX_W * TEX_H * 2;
    list = desc + 0x48; prim = list + 0x18;
    memcpy(HOST(desc), HOST(RW(PAGES + 0x14)), 0x44);    /* like page 0's (16-bit 5551) */
    WW(desc, STRIP_PAGE); WW(desc + 4, 0); WW(desc + 0x18, 0x01000000); WW(desc + 0x24, TEX_W); WW(desc + 0x28, TEX_H);
    uint32_t d0 = RW(TEXT_TEXTURES);
    WW(list + 8, RW(d0)); WW(list + 12, RW(d0 + 4)); WW(list + 16, STRIP_PAGE);
    WW(list, list + 8); WW(list + 4, 1);
    return 1;
}

static void load(int stage) {
    if (stage == loaded) return;
    loaded = stage; nstrips = 0;
    char dir[1200];
    if (!FEATURE(STAGE_EXT)) return;                                           /* Stage Extensions off (the default) */
    if (!rt_assets_root[0] || !strip_folder(stage, dir, sizeof dir)) return;   /* no assets/ (--no-assets, the tests) */
    char geo[1300]; snprintf(geo, sizeof geo, "%s/widescreen.json", dir);
    static const char *const sides[2] = { "left", "right" };
    for (int layer = 0; layer < NLAYERS; layer++) for (int side = 0; side < 2; side++) {
        char key[32], path[1400], err[128];
        double x;
        snprintf(key, sizeof key, "layer%d_%s", layer, sides[side]);
        if (json_file_number(geo, key, &x) != 0) {
            if (layer != FIGHTERS) continue;
            x = side ? FIGHTERS_RIGHT_X : FIGHTERS_LEFT_X;
        }
        if (layer == FIGHTERS) snprintf(path, sizeof path, "%s/widescreen_%s.png", dir, sides[side]);
        else snprintf(path, sizeof path, "%s/widescreen_layer%d_%s.png", dir, layer, sides[side]);
        PngImage img;
        if (png_decode_file(path, &img, err, sizeof err) != 0) {
            if (layer == FIGHTERS) rt_log("stage strips: %s: %s", path, err);
            continue;
        }
        if (img.w != STRIP_W || img.h != STRIP_H) {
            rt_log("stage strips: %s is %d x %d, not %d x %d: not drawn", path, img.w, img.h, STRIP_W, STRIP_H);
            png_free(&img); continue;
        }
        if (nstrips == SLOTS || !alloc_texture()) {
            rt_log("stage strips: %s: no room (%d strips at most)", path, SLOTS);
            png_free(&img); continue;
        }
        Strip *s = &strips[nstrips];
        s->layer = layer; s->side = side; s->x = (int)x; s->slot = nstrips;
        double front = 0;
        snprintf(key, sizeof key, "layer%d_%s_front", layer, sides[side]);
        s->front = json_file_number(geo, key, &front) == 0 && front != 0;
        for (int y = 0; y < STRIP_H; y++) for (int i = 0; i < STRIP_W; i++)
            WH(tex + (uint32_t)((y * TEX_W + s->slot * 64 + i) * 2), c5551(img.rgba + (y * STRIP_W + i) * 4));
        png_free(&img);
        nstrips++;
    }
    rt_log("stage strips: stage 0x%02X: %d strips", stage, nstrips);
}

static float ffrom(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

/* A layer position on screen, as the tile drawers place a layer's tiles. */
static int to_screen(int v, int scroll, uint32_t scale, uint32_t offset) {
    float f = (float)(v - scroll);
    f = f * ffrom(RW(scale));
    float o = ffrom(RW(offset)) + 0.5f;
    return (int)f + (int)o;
}

/* Layer n's strips, just behind it (depth: the layer's own, less one). */
static void draw(Cpu *c, int layer, int depth) {
    int any = 0;
    for (int i = 0; i < nstrips; i++) any |= strips[i].layer == layer;
    if (!any) return;
    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint8_t keep_tex[20], keep_pal = (uint8_t)RB(PAL_SELECTED);
    memcpy(keep_tex, HOST(TEX_STATE), sizeof keep_tex);
    uint32_t e = PAGES + 0x18u * STRIP_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(e + 4u * (uint32_t)i);
    WW(e, tex); WW(e + 4, tex); WW(e + 8, 0); WW(e + 0xC, 0); WW(e + 0x10, 0x80000000u); WW(e + 0x14, desc);
    c->r[4] = list; c->r[31] = 0x0881AB28; f_089E0CF4(c);

    uint32_t l = LAYERS + (uint32_t)layer * LAYER_SIZE;
    int sx = (int16_t)RH(l + 18), sy = (int16_t)RH(l + 98);
    int y0 = to_screen(0, sy, SCALE_Y, OFFSET_Y), y1 = to_screen(STRIP_H, sy, SCALE_Y, OFFSET_Y);
    float behind = (float)(depth > 0 ? depth - 1 : 0), over = (float)(depth + 1);
    for (int i = 0; i < nstrips; i++) {
        const Strip *s = &strips[i];
        if (s->layer != layer) continue;
        float z = s->front ? over : behind;
        int x0 = to_screen(s->x, sx, SCALE_X, OFFSET_X), x1 = to_screen(s->x + STRIP_W, sx, SCALE_X, OFFSET_X);
        /* Clipped to the screen here, texels with it: the library clips a quad's corners without moving
           its texels, which would squeeze a strip that runs off the screen (tiles are too small to). */
        int xa = x0 > CLIP_X0 ? x0 : CLIP_X0, xb = x1 < CLIP_X1 ? x1 : CLIP_X1;
        int ya = y0 > CLIP_Y0 ? y0 : CLIP_Y0, yb = y1 < CLIP_Y1 ? y1 : CLIP_Y1;
        if (xb <= xa || yb <= ya) continue;                    /* off the screen */
        int base = s->slot * 64;
        int u0 = base + (xa - x0) * STRIP_W / (x1 - x0), u1 = base + (xb - x0) * STRIP_W / (x1 - x0);
        int v0 = (ya - y0) * STRIP_H / (y1 - y0), v1 = (yb - y0) * STRIP_H / (y1 - y0);
        uint32_t xy = prim + 0x10, col = xy + 0x10, uv = col + 0x10;
        WH(xy + 0, x0); WH(xy + 2, ya); WH(xy + 4, x1); WH(xy + 6, ya); WH(xy + 8, x1); WH(xy + 0xA, yb); WH(xy + 0xC, x0); WH(xy + 0xE, yb);
        for (int k = 0; k < 4; k++) WW(col + 4u * (uint32_t)k, 0xFFFFFFFFu);
        WH(uv + 0, u0); WH(uv + 2, v0); WH(uv + 4, u1); WH(uv + 6, v0); WH(uv + 8, u1); WH(uv + 0xA, v1); WH(uv + 0xC, u0); WH(uv + 0xE, v1);
        WW(prim, xy); WW(prim + 4, col); WW(prim + 8, uv); WW(prim + 0xC, 4);
        c->r[4] = prim; c->r[5] = 4; c->r[6] = 0x80000020u; c->f[12] = fbits(z);
        c->r[31] = 0x0881AB28; f_089E1618(c);
    }

    for (int i = 0; i < 6; i++) WW(e + 4u * (uint32_t)i, keep_page[i]);
    memcpy(HOST(TEX_STATE), keep_tex, sizeof keep_tex);
    WB(PAL_SELECTED, keep_pal);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
}

#define PROBE_U  (SLOTS * 64 - 8)       /* 8 x 8 texels of magenta at the texture's top right */
static void draw_probe(Cpu *c) {
    if (!alloc_texture()) return;
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) WH(tex + (uint32_t)((y * TEX_W + PROBE_U + x) * 2), 0x801Fu | 0x1Fu << 10);
    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint8_t keep_tex[20], keep_pal = (uint8_t)RB(PAL_SELECTED);
    memcpy(keep_tex, HOST(TEX_STATE), sizeof keep_tex);
    uint32_t e = PAGES + 0x18u * STRIP_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(e + 4u * (uint32_t)i);
    WW(e, tex); WW(e + 4, tex); WW(e + 8, 0); WW(e + 0xC, 0); WW(e + 0x10, 0x80000000u); WW(e + 0x14, desc);
    c->r[4] = list; c->r[31] = 0x0881AB28; f_089E0CF4(c);
    uint32_t xy = prim + 0x10, col = xy + 0x10, uv = col + 0x10;
    WH(xy + 0, -64); WH(xy + 2, -64); WH(xy + 4, 544); WH(xy + 6, -64); WH(xy + 8, 544); WH(xy + 0xA, 336); WH(xy + 0xC, -64); WH(xy + 0xE, 336);
    for (int k = 0; k < 4; k++) WW(col + 4u * (uint32_t)k, 0xFFFFFFFFu);
    WH(uv + 0, PROBE_U + 2); WH(uv + 2, 2); WH(uv + 4, PROBE_U + 6); WH(uv + 6, 2); WH(uv + 8, PROBE_U + 6); WH(uv + 0xA, 6); WH(uv + 0xC, PROBE_U + 2); WH(uv + 0xE, 6);
    WW(prim, xy); WW(prim + 4, col); WW(prim + 8, uv); WW(prim + 0xC, 4);
    c->r[4] = prim; c->r[5] = 4; c->r[6] = 0x80000020u; c->f[12] = fbits(0.0f);
    c->r[31] = 0x0881AB28; f_089E1618(c);
    for (int i = 0; i < 6; i++) WW(e + 4u * (uint32_t)i, keep_page[i]);
    memcpy(HOST(TEX_STATE), keep_tex, sizeof keep_tex);
    WB(PAL_SELECTED, keep_pal);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
}
static uint64_t probed = ~0ull;

/* around the fight's per-frame step (0x08823674, src/core/task/fight.c): the strips are drawn only
   while it runs, so never on a menu or select screen that uses the same tile drawers */
int stagestrips_fight_frame(Cpu *c, uint32_t a0, int after) { (void)c; (void)a0; if (!after) fight_frame = rt_vblank_count(); return 0; }

/* Before a stage layer is drawn: its strips (or, measuring, whether to draw it at all). */
static int before_layer(Cpu *c, int layer, uint32_t column) {
    uint64_t now = rt_vblank_count();
    if (fight_frame == ~0ull || now - fight_frame > 2) return 0;          /* not in a fight */
    if (RB(FIGHTERS_LAYER + 2) != 1) return 0;                           /* no stage set up */
    read_measure();
    if (measuring) {
        static int logged[NLAYERS];
        if (!logged[layer]) {
            logged[layer] = 1;
            rt_log("stage strips: layer %d depth %d", layer, column < 128 ? (int)RH(DEPTHS + 2u * (1u + 2u * column)) : 0);
        }
        for (int i = 0; i < nwindows; i++)
            if ((long)now >= windows[i].from && (long)now <= windows[i].to) {
                if ((windows[i].mask & 0x100) && probed != now) { probed = now; draw_probe(c); }
                return !((windows[i].mask >> layer) & 1);
            }
        return 0;
    }
    if (!rt_widescreen || drawn[layer] == now) return 0;
    drawn[layer] = now;
    load((int16_t)RH(BATTLE));
    draw(c, layer, column < 128 ? (int)RH(DEPTHS + 2u * (1u + 2u * column)) : 0);
    return 0;
}

/* around 0x0881AB28 (a0 the layer) */
int stagestrips_layer_32(Cpu *c, uint32_t a0, int after) {
    if (after || a0 < LAYERS || a0 >= LAYERS + NLAYERS * LAYER_SIZE || (a0 - LAYERS) % LAYER_SIZE) return 0;
    return before_layer(c, (int)((a0 - LAYERS) / LAYER_SIZE), RB(a0 + 122));
}

/* around 0x0881B268 (a0 the layer's record: its kind names the layer) */
int stagestrips_layer_16(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    int kind = RH(a0 + REC_KIND) & 0xFF;
    for (int n = 0; n < NLAYERS; n++)
        if (RB(LAYERS + (uint32_t)n * LAYER_SIZE + 3) == (uint32_t)kind)
            return before_layer(c, n, (uint32_t)(int16_t)RH(a0 + REC_DEPTH));
    return 0;
}
