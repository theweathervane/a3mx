/* WIDE NATIVE's stage extensions: new art for the 48 columns at each side of the screen that the
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
 * Each strip is 48 - 64 x 288 (the placeholders are 64: 8 columns under the layer's own art and 8 past
 * the screen's edge, so neither rounding nor a layer's scroll wavering at the wall leaves a gap), row r at the layer's y r: with the camera at rest (the layer's y scroll 16)
 * the screen's top row shows row 16, and rows 0 - 15 when the camera rises. The 2D library's 224 rows
 * are stretched to the screen's 272, so rows 0 - 239 are ever seen. Rounded to 15-bit colour; alpha
 * below 128 is transparent.
 * A missing or unreadable file is skipped (the log says why).
 *
 * Drawn only with --custom-assets (the launcher's Custom Assets, off by default), while WIDE NATIVE is
 * on and a fight is running (the fight's per-frame step 0x08823674, src/core/task/fight.c, wraps a
 * timestamp, and the battle task is at its fight screen: GAME + 2 = 2, + 4 = 3), around both tile drawers (0x0881AB28, 0x0881B268,
 * src/core/stage/tiles.c: SFA_FUNC_WRAPPED) and the two picture drawers (0x08819498, 0x0881C0C8,
 * src/core/stage/background.c), as textured quads through the game's 2D library like
 * padicons.c: our 16-bit texture on page STRIP_PAGE only while drawing.
 *
 * The strips change only what is drawn (and the extension's own memory): the game's state is the same
 * with or without them, so the fighters' parity with WIDE NATIVE on and off is untouched.
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
#define GAME            0x08E75910u   /* +2 the battle task's screen, +4 its step: 2 / 3 is the fight itself. The
                                         select screen (1 / *), the world map, profile and VS screens (2 / 1 - 2)
                                         run inside the same per-frame fight step and reuse the stage's layers
                                         for their own backdrops, so the timestamp alone let the next stage's
                                         strips show on them (C29). Arcade, Free Battle and Training checked. */
#define SCALE_Y         0x08E759ECu
#define SHAKE_REC       0x092E5F78u   /* +112: the screen's shake / scroll (16.16), less which layer 1's +16 is its travel */
#define OFFSET_X        0x08E75A04u
#define OFFSET_Y        0x08E75A0Cu
#define PAGES           0x08B0C334u   /* texture pages, 0x18 bytes each (+0x14 its descriptor) */
#define TEXT_TEXTURES   0x08A05080u
#define TEX_STATE       0x08B23334u   /* the selected texture list (8 bytes) and its first entry (12) */
#define PAL_SELECTED    0x08ACCD80u   /* u8: the palette 0x089E0EA4 last selected */

#define STRIP_W         48            /* the columns WIDE NATIVE adds at each side */
#define STRIP_W_MAX     64            /* a strip can be wider, reaching under its layer's own art (an overlap, so no gap shows) */
#define STRIP_H         288
#define FIGHTERS_LEFT_X 208           /* layer 1's strips without widescreen.json */
#define FIGHTERS_RIGHT_X 1024
/* The screen in the 2D library's units (WIDE NATIVE: 480 across from -48, and 224 rows from 8, stretched
   to the screen's 272) */
#define CLIP_X0         -48
#define CLIP_X1         432
#define CLIP_Y0         8
#define CLIP_Y1         232
#define TEX_W           512           /* up to 8 strips side by side, 64 texels apart */
#define TEX_H           512
#define SLOTS           8
#define STRIP_PAGE      0x3E

typedef struct { int layer, side, x, x_top, w, slot, front; } Strip;   /* x_top: the top band's (see Band) */   /* front: drawn just in front of its layer, over its art */
static Strip strips[SLOTS];
static int nstrips;
static uint32_t tex, desc, list, prim;
static int loaded = -2;               /* the stage whose strips are in tex; -2 none tried */
static uint64_t drawn[NLAYERS][2];    /* the frame each layer's strips were last drawn, by band */
static uint64_t called[NLAYERS];      /* the frame of the layer's last 0x0881B268 call, and how many that frame */
static int ncalls[NLAYERS];
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

/* ---- the foreground objects' pieces ----
   A stage object drawn in front of the fighters (sprite layer 0, depth index 7: STAGES.md) is a sprite,
   not a layer, so its art stops at the 4:3 edge however the layers are extended. A piece is new art
   pinned to such an object: drawn right after it, at its depth, placed from the object's own position as
   the sprite drawer works it out (sprite.c's anchor), so it moves exactly with it. Files, in the stage's
   folder: widescreen_object_<kind>_<type>_<side>.png (side: left or right, up to 224 x 216), placed by
   widescreen.json's "object_<kind>_<type>_<side>": x and "..._y": y, the piece's top left relative to
   the object's position in the 2D library's units. Objects of the same kind and type share them. */
#define OBJ_ROW0     296            /* object pieces sit in the strips' texture below the strips */
#define MAX_OBJS     16
typedef struct { int kind, type, side, dx, dy, w, h, u, v; } Piece;
static Piece pieces[MAX_OBJS];
static int nobjs;

static void load_objects(const char *dir, const char *geo) {
    nobjs = 0;
    int pen_x = 0, seen[16][2], nseen = 0;
    for (uint32_t slot = 0; slot < 16; slot++) {            /* the kinds and types on this stage */
        uint32_t o = 0x09514B70u + slot * 0xC0u;
        if (RB(o) == 0 || RB(o + 9) != 0 || RH(o + 24) != 7) continue;
        int dup = 0;
        for (int i = 0; i < nseen; i++) dup |= seen[i][0] == (int)RB(o + 2) && seen[i][1] == (int)RB(o + 3);
        if (!dup) { seen[nseen][0] = RB(o + 2); seen[nseen][1] = RB(o + 3); nseen++; }
    }
    for (int i = 0; i < nseen; i++) for (int side = 0; side < 2; side++) {
        int kind = seen[i][0], type = seen[i][1];
        static const char *const sides[2] = { "left", "right" };
        char key[64], path[1400], err[128];
        double x, y;
        snprintf(key, sizeof key, "object_%d_%d_%s", kind, type, sides[side]);
        if (json_file_number(geo, key, &x) != 0) continue;
        snprintf(key, sizeof key, "object_%d_%d_%s_y", kind, type, sides[side]);
        if (json_file_number(geo, key, &y) != 0) continue;
        snprintf(path, sizeof path, "%s/widescreen_object_%d_%d_%s.png", dir, kind, type, sides[side]);
        PngImage img;
        if (png_decode_file(path, &img, err, sizeof err) != 0) { rt_log("stage strips: %s: %s", path, err); continue; }
        if (img.w > 224 || img.h > TEX_H - OBJ_ROW0 || pen_x + img.w > TEX_W || nobjs == MAX_OBJS || !alloc_texture()) {
            rt_log("stage strips: %s (%d x %d): no room", path, img.w, img.h);
            png_free(&img); continue;
        }
        Piece *p = &pieces[nobjs++];
        *p = (Piece){ kind, type, side, (int)x, (int)y, img.w, img.h, pen_x, OBJ_ROW0 };
        for (int yy = 0; yy < img.h; yy++) for (int xx = 0; xx < img.w; xx++)
            WH(tex + (uint32_t)(((OBJ_ROW0 + yy) * TEX_W + pen_x + xx) * 2), c5551(img.rgba + (yy * img.w + xx) * 4));
        pen_x += img.w;
        png_free(&img);
    }
}

static void load(int stage) {
    if (stage == loaded) return;
    loaded = stage; nstrips = 0; nobjs = 0;
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
        if (img.w < STRIP_W || img.w > STRIP_W_MAX || img.h != STRIP_H) {
            rt_log("stage strips: %s is %d x %d, not %d - %d x %d: not drawn", path, img.w, img.h, STRIP_W, STRIP_W_MAX, STRIP_H);
            png_free(&img); continue;
        }
        if (nstrips == SLOTS || !alloc_texture()) {
            rt_log("stage strips: %s: no room (%d strips at most)", path, SLOTS);
            png_free(&img); continue;
        }
        Strip *s = &strips[nstrips];
        s->layer = layer; s->side = side; s->x = (int)x; s->w = img.w; s->slot = nstrips;
        double xt;
        snprintf(key, sizeof key, "layer%d_%s_top", layer, sides[side]);
        s->x_top = json_file_number(geo, key, &xt) == 0 ? (int)xt : s->x;
        double front = 0;
        snprintf(key, sizeof key, "layer%d_%s_front", layer, sides[side]);
        s->front = json_file_number(geo, key, &front) == 0 && front != 0;
        for (int y = 0; y < STRIP_H; y++) for (int i = 0; i < img.w; i++)
            WH(tex + (uint32_t)((y * TEX_W + s->slot * 64 + i) * 2), c5551(img.rgba + (y * img.w + i) * 4));
        png_free(&img);
        nstrips++;
    }
    load_objects(dir, geo);
    rt_log("stage strips: stage 0x%02X: %d strips, %d object pieces", stage, nstrips, nobjs);
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

/* Layer n's strips, just behind it (depth: the layer's own, less one), placed by the horizontal scroll
   the layer's drawer used (scroll_x: not always the layer's +18) and its +98; band 1 (a layer's top rows
   that move on their own) uses the strips' x_top. Rows outside clip_y0 - clip_y1 (screen, the 2D
   library's units) are left out: one band's rows only. */
static void draw(Cpu *c, int layer, int depth, int scroll_x, int band, int clip_y0, int clip_y1) {
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
    int sx = scroll_x, sy = (int16_t)RH(l + 98);
    int y0 = to_screen(0, sy, SCALE_Y, OFFSET_Y), y1 = to_screen(STRIP_H, sy, SCALE_Y, OFFSET_Y);
    float behind = (float)(depth > 0 ? depth - 1 : 0), over = (float)(depth + 1);
    for (int i = 0; i < nstrips; i++) {
        const Strip *s = &strips[i];
        if (s->layer != layer) continue;
        float z = s->front ? over : behind;
        int at = band ? s->x_top : s->x;
        int x0 = to_screen(at, sx, SCALE_X, OFFSET_X), x1 = to_screen(at + s->w, sx, SCALE_X, OFFSET_X);
        /* Clipped to the screen here, texels with it: the library clips a quad's corners without moving
           its texels, which would squeeze a strip that runs off the screen (tiles are too small to). */
        int xa = x0 > CLIP_X0 ? x0 : CLIP_X0, xb = x1 < CLIP_X1 ? x1 : CLIP_X1;
        int ya = y0 > CLIP_Y0 ? y0 : CLIP_Y0, yb = y1 < CLIP_Y1 ? y1 : CLIP_Y1;
        if (ya < clip_y0) ya = clip_y0;
        if (yb > clip_y1) yb = clip_y1;
        if (xb <= xa || yb <= ya) continue;                    /* off the screen */
        int base = s->slot * 64;
        int u0 = base + (xa - x0) * s->w / (x1 - x0), u1 = base + (xb - x0) * s->w / (x1 - x0);
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

/* Before a stage layer (or one band of it) is drawn: its strips (or, measuring, whether to draw it at
   all). scroll_x: the horizontal scroll the drawer uses; band 0 the layer, 1 its separately moving top
   rows; clip_y0 - clip_y1 the band's screen rows. */
static int before_layer(Cpu *c, int layer, uint32_t column, int scroll_x, int band, int clip_y0, int clip_y1) {
    uint64_t now = rt_vblank_count();
    if (fight_frame == ~0ull || now - fight_frame > 2) return 0;          /* not in a fight */
    if (RH(GAME + 2) != 2 || RH(GAME + 4) != 3) return 0;               /* not the fight itself (C29) */
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
        /* outside the windows: the scroll each layer's strips are placed by, for the measuring tool */
        rt_log("stage strips: frame %llu layer %d band %d scroll %d", (unsigned long long)now, layer, band, scroll_x);
        return 0;
    }
    if (!rt_widescreen || drawn[layer][band] == now) return 0;
    drawn[layer][band] = now;
    load((int16_t)RH(BATTLE));
    draw(c, layer, column < 128 ? (int)RH(DEPTHS + 2u * (1u + 2u * column)) : 0, scroll_x, band, clip_y0, clip_y1);
    return 0;
}

/* around 0x0881AB28 (a0 the layer): it scrolls by the layer's +18 */
int stagestrips_layer_32(Cpu *c, uint32_t a0, int after) {
    if (after || a0 < LAYERS || a0 >= LAYERS + NLAYERS * LAYER_SIZE || (a0 - LAYERS) % LAYER_SIZE) return 0;
    return before_layer(c, (int)((a0 - LAYERS) / LAYER_SIZE), RB(a0 + 122), (int16_t)RH(a0 + 18), 0, -10000, 10000);
}

/* v nearest to near among v + k * period */
static int unwrap(int v, int near, int period) {
    int d = (v - near) % period;
    if (d < 0) d += period;
    if (d > period / 2) d -= period;
    return near + d;
}

/* around 0x0881B268 (a0 the layer's record: its kind names the layer). The record's builder
   (src/core/stage/background.c) chose the scroll: the layer's +18, the fighters' layer's travel, or for
   RYU's and CHUN-LI's layer 1 (kinds 0xA0 / 0xA4) a lagging one for the top rows, drawn by a first call
   with fewer than 16 rows, then the rest by a second. The column (+54, in 16-pixel tiles) and the part of
   a tile scrolled past (+32, on screen) give it back. */
int stagestrips_layer_16(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    int kind = RH(a0 + REC_KIND) & 0xFF;
    for (int n = 0; n < NLAYERS; n++) {
        uint32_t l = LAYERS + (uint32_t)n * LAYER_SIZE;
        if (RB(l + 3) != (uint32_t)kind) continue;
        uint64_t now = rt_vblank_count();
        if (called[n] != now) { called[n] = now; ncalls[n] = 0; }
        int call = ncalls[n]++;
        int x18 = (int16_t)RH(l + 18), scroll = x18;
        if (kind != 0x90) {                                   /* (0x90, GUILE's strips: +18 itself) */
            float sx = ffrom(RW(SCALE_X));
            int x0 = (int16_t)RH(a0 + 32), part = 0;
            for (int v = 0; v < 16; v++) { float f = (float)v; f = f * sx; f = -f; if ((int)f == x0) { part = v; break; } }
            scroll = unwrap(((int16_t)RH(a0 + 54) & 0x3F) * 16 + part, x18, 1024);
        }
        int rows = (int16_t)RH(a0 + 62);
        int split = (kind == 0xA0 || kind == 0xA4) && rows != 16;
        int band = split && call == 0 ? 1 : 0;
        int y0 = -10000, y1 = 10000;
        if (split) {
            int top = (int16_t)RH(a0 + 12) + (int16_t)RH(a0 + 30);
            y0 = top; y1 = top + rows * (int16_t)RH(a0 + 26);
            if (!band) y1 = 10000;                            /* the rest: down to the bottom */
            else y0 = -10000;                                 /* the top rows: from the top */
        }
        return before_layer(c, n, (uint32_t)(int16_t)RH(a0 + REC_DEPTH), scroll, band, y0, y1);
    }
    return 0;
}

/* around the picture layers 0x08819498 / 0x0881C0C8 (src/core/stage/background.c; BISON's mountains and
   sky): a0 the layer. The mountains scroll by the fighters' layer's travel (+16 less the shake record's
   +112), the sky by half of +8 less +16. */
int stagestrips_picture(Cpu *c, uint32_t a0, int after) {
    if (after || a0 < LAYERS || a0 >= LAYERS + NLAYERS * LAYER_SIZE || (a0 - LAYERS) % LAYER_SIZE) return 0;
    int kind = RB(a0 + 3), scroll;
    if (kind == 0xAA || kind == 0xBE) scroll = -(int)(((int32_t)(RW(a0 + 8) - RW(a0 + 16))) >> 17);
    else scroll = (int16_t)(((int32_t)(RW(a0 + 16) - RW(SHAKE_REC + 112))) >> 16);
    return before_layer(c, (int)((a0 - LAYERS) / LAYER_SIZE), RB(a0 + 122), scroll, 0, -10000, 10000);
}

/* ---- around the sprite drawer 0x0880C308 (src/core/object/sprite.c): a foreground stage object's
   pieces, after the object ---- */
#define POOL_5      0x09514B70u   /* the stage objects: 16 of 0xC0 */
#define ANCHORS     0x08A04014u   /* u32[]: anchor records (+18 x, +22 y) */
static int is_foreground(uint32_t sp) {
    return sp >= POOL_5 && sp < POOL_5 + 16u * 0xC0u && (sp - POOL_5) % 0xC0u == 0
        && RB(sp + 9) == 0 && RH(sp + 24) == 7;
}

/* The object's position as the sprite drawer works it out (sprite.c: draw_sprite, sprite_anchor). */
static void object_position(uint32_t sp, int *x, int *y) {
    int px = (int16_t)RH(sp + 18), py = (int16_t)(0xF0 - (int16_t)RH(sp + 22));
    uint8_t anchor = RB(sp + 14);
    if (anchor != 0) {
        if (anchor == 2) {
            int share = (int16_t)RH(sp + 38);
            if (share != 0) px = (int16_t)(px + ((int32_t)(RW(SHAKE_REC + 28) * (uint32_t)share) >> 16));
        }
        uint32_t rec = RW(ANCHORS + anchor * 4u);
        px = (int16_t)(px - (int16_t)RH(rec + 18));
        py = (int16_t)(py + (int16_t)RH(rec + 22));
    }
    if (RB(sp + 15) != 0) py = (int16_t)(py - (int16_t)RH(BATTLE + 74));
    *x = px; *y = py;
}

/* measuring (tools/measure_stage_objects.py): A3MX_OBJ_MEASURE="F0-F1:KIND.TYPE" hides that object in
   frames F0 - F1 (KIND.TYPE -1.-1: every foreground object), and every foreground object's position is
   logged each frame */
static int obj_measure_read, obj_measuring, hide_kind = -2, hide_type = -2;
static long hide_from, hide_to;
static int in_fight(void) {
    uint64_t now = rt_vblank_count();
    return fight_frame != ~0ull && now - fight_frame <= 2 && RH(GAME + 2) == 2 && RH(GAME + 4) == 3 && RB(FIGHTERS_LAYER + 2) == 1;
}

int stagestrips_object(Cpu *c, uint32_t a0, int after) {
    if (!is_foreground(a0) || !in_fight()) return 0;
    if (!obj_measure_read) {
        obj_measure_read = 1;
        const char *e = getenv("A3MX_OBJ_MEASURE");
        if (e && *e) {
            obj_measuring = 1;
            if (sscanf(e, "%ld-%ld:%d.%d", &hide_from, &hide_to, &hide_kind, &hide_type) != 4) hide_kind = -2;
        }
    }
    int kind = RB(a0 + 2), type = RB(a0 + 3);
    if (obj_measuring) {
        long now = (long)rt_vblank_count();
        if (!after) {
            int x, y;
            object_position(a0, &x, &y);
            rt_log("stage objects: frame %ld object %d.%d at %d %d scale %g %g offset %g %g", now, kind, type, x, y,
                   ffrom(RW(SCALE_X)), ffrom(RW(SCALE_Y)), ffrom(RW(OFFSET_X)), ffrom(RW(OFFSET_Y)));
            if (now >= hide_from && now <= hide_to && (hide_kind == -1 || (hide_kind == kind && hide_type == type))) return 1;
        }
        return 0;
    }
    if (!after || !rt_widescreen) return 0;
    load((int16_t)RH(BATTLE));
    int any = 0;
    for (int i = 0; i < nobjs; i++) any |= pieces[i].kind == kind && pieces[i].type == type;
    if (!any) return 0;
    int ox, oy;
    object_position(a0, &ox, &oy);

    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint8_t keep_tex[20], keep_pal = (uint8_t)RB(PAL_SELECTED);
    memcpy(keep_tex, HOST(TEX_STATE), sizeof keep_tex);
    uint32_t e = PAGES + 0x18u * STRIP_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(e + 4u * (uint32_t)i);
    WW(e, tex); WW(e + 4, tex); WW(e + 8, 0); WW(e + 0xC, 0); WW(e + 0x10, 0x80000000u); WW(e + 0x14, desc);
    c->r[4] = list; c->r[31] = 0x0880C308; f_089E0CF4(c);
    int depth = (int)RH(0x08AE03B8u + 2u * RH(a0 + 24));   /* the object's own: 16 */
    for (int i = 0; i < nobjs; i++) {
        const Piece *p = &pieces[i];
        if (p->kind != kind || p->type != type) continue;
        int x0 = to_screen(ox + p->dx, 0, SCALE_X, OFFSET_X), x1 = to_screen(ox + p->dx + p->w, 0, SCALE_X, OFFSET_X);
        int y0 = to_screen(oy + p->dy, 0, SCALE_Y, OFFSET_Y), y1 = to_screen(oy + p->dy + p->h, 0, SCALE_Y, OFFSET_Y);
        int xa = x0 > CLIP_X0 ? x0 : CLIP_X0, xb = x1 < CLIP_X1 ? x1 : CLIP_X1;
        int ya = y0 > CLIP_Y0 ? y0 : CLIP_Y0, yb = y1 < CLIP_Y1 ? y1 : CLIP_Y1;
        if (xb <= xa || yb <= ya) continue;
        int u0 = p->u + (xa - x0) * p->w / (x1 - x0), u1 = p->u + (xb - x0) * p->w / (x1 - x0);
        int v0 = p->v + (ya - y0) * p->h / (y1 - y0), v1 = p->v + (yb - y0) * p->h / (y1 - y0);
        uint32_t xy = prim + 0x10, col = xy + 0x10, uv = col + 0x10;
        WH(xy + 0, xa); WH(xy + 2, ya); WH(xy + 4, xb); WH(xy + 6, ya); WH(xy + 8, xb); WH(xy + 0xA, yb); WH(xy + 0xC, xa); WH(xy + 0xE, yb);
        for (int k = 0; k < 4; k++) WW(col + 4u * (uint32_t)k, 0xFFFFFFFFu);
        WH(uv + 0, u0); WH(uv + 2, v0); WH(uv + 4, u1); WH(uv + 6, v0); WH(uv + 8, u1); WH(uv + 0xA, v1); WH(uv + 0xC, u0); WH(uv + 0xE, v1);
        WW(prim, xy); WW(prim + 4, col); WW(prim + 8, uv); WW(prim + 0xC, 4);
        c->r[4] = prim; c->r[5] = 4; c->r[6] = 0x80000020u; c->f[12] = fbits((float)depth);
        c->r[31] = 0x0880C308; f_089E1618(c);
    }
    for (int i = 0; i < 6; i++) WW(e + 4u * (uint32_t)i, keep_page[i]);
    memcpy(HOST(TEX_STATE), keep_tex, sizeof keep_tex);
    WB(PAL_SELECTED, keep_pal);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
    return 0;
}
