/* SCREEN SETTING > FILTER (0.60.0): RetroArch's slang shaders over the finished picture, through
 * librashader (third_party/librashader: headers only; the library is loaded when the game starts, if it
 * is there). Part of the "shaders" feature (ext_features.h), on by default.
 *
 * What is needed, both outside this repository:
 *   - the library: librashader.dll / .so / .dylib. Looked for, in this order: --librashader FILE (or
 *     A3MX_LIBRASHADER), beside the game, .tools/librashader/ beside the work folder, then wherever the
 *     system finds libraries;
 *   - the shaders: libretro's slang-shaders, at the commit pinned in tools/data/pins.json
 *     (python tools/shaders.py fetches them into .tools/slang-shaders/). --shaders DIR (or
 *     A3MX_SHADERS_DIR) names another copy.
 * With either missing the picture is shown as it always was, and the log says which and where it looked.
 *
 * The filters (0.166.0, shader_list.c), by category:
 *   FAVOURITES  the table below, always: the picture's own look (OFF), five CRTs, four kinds of scanline
 *               and two pixel smoothers, each one preset from slang-shaders, some with parameters of ours;
 *   then        the catalogue: every preset the game could load and build, by folder (CRT, SCANLINES,
 *               HANDHELD, NTSC, EDGE SMOOTHING, ...), each named after its file. The repository's
 *               tools/data/shader_catalogue.tsv (0.171.0: made once, 2026-11-26, for the pinned
 *               slang-shaders commit, and shipped as data; nothing is tested on the player's machine),
 *               or an a3mx_catalogue.tsv beside the shaders (an older download made one), which wins;
 *   CUSTOM      the player's own *.slangp, in shaders/ beside a3mx.cfg (<configs>/shaders/, its
 *               subfolders too).
 * a3mx.cfg holds a favourite by its name ([screen setting] filter=CRT GUEST), any other by its preset's
 * path (filter=crt/crt-geom-deluxe.slangp; custom:NAME.slangp for one of CUSTOM's). */
#pragma once

typedef struct {
    const char *name;            /* SCREEN SETTING and a3mx.cfg: capitals, at most 11 letters (the green bar's width) */
    const char *preset;          /* under the shaders folder; NULL: no filter */
    const char *params;          /* "NAME=value;..." set on the chain after it is made, or NULL */
    const char *what;            /* for docs and the log */
} ShaderFilter;

static const ShaderFilter SHADER_FILTERS[] = {
    { "OFF",         NULL, NULL, "the picture as it is drawn" },
    { "CRT CURVED",  "crt/crt-geom.slangp", NULL, "crt-geom: a curved tube with scanlines and a shadow mask" },
    { "CRT LOTTES",  "crt/crt-lottes.slangp", NULL, "crt-lottes: bloom, a shadow mask, slight curve" },
    { "CRT ROYALE",  "crt/crt-royale.slangp", NULL, "crt-royale: the most detailed tube (heavy: wants a real GPU)" },
    { "CRT GUEST",   "crt/crt-guest-advanced.slangp", NULL, "crt-guest-advanced: a sharp arcade-monitor look" },
    { "CRT LIGHT",   "crt/zfast-crt.slangp", NULL, "zfast-crt: a light CRT for integrated GPUs and handhelds" },
    { "SCAN LIGHT",  "scanlines/scanline.slangp", "SCANLINE_SINE_COMP_A=0.0;SCANLINE_SINE_COMP_B=0.08", "light scanlines" },
    { "SCAN STRONG", "scanlines/scanline.slangp", "SCANLINE_SINE_COMP_A=0.0;SCANLINE_SINE_COMP_B=0.30", "strong scanlines" },
    { "SCAN OSSC",   "scanlines/ossc.slangp", "ASPECTx=30;ASPECTy=17", "scanlines as the OSSC line doubler draws them" },
    { "SCAN DARK",   "scanlines/scanline.slangp", "SCANLINE_SINE_COMP_A=0.05;SCANLINE_SINE_COMP_B=0.60", "dark, heavy scanlines" },
    { "XBRZ",        "edge-smoothing/xbrz/xbrz-freescale.slangp", NULL, "xBRZ: smooth pixel edges" },
    { "SCALEFX",     "edge-smoothing/scalefx/scalefx.slangp", NULL, "ScaleFX: smooth pixel edges, truer to the pixel art" },
};
#define SHADER_BUILTIN_COUNT ((int)(sizeof SHADER_FILTERS / sizeof SHADER_FILTERS[0]))

/* ---------------------------------------------------------------- every filter (shader_list.c) */
typedef struct {
    int category;                /* an index for filter_category_name; the list is in category order */
    char name[64];               /* capitals, as FILTER shows it (long ones are cut short on screen) */
    char path[1024];             /* the preset: under the shaders folder, or a full path (custom); "" for OFF */
    const char *params;          /* a favourite's parameters, or NULL */
    int heavy;                   /* the catalogue says it wants a real GPU */
    int custom;                  /* the player's own (path is a full path) */
} FilterEntry;
void filters_load(void);                     /* reads the catalogue and the player's folder, once (each call after: nothing) */
int filter_count(void);                      /* every filter; 0 is OFF, 1 - 11 the favourites */
const FilterEntry *filter_get(int i);        /* NULL past the end */
int filter_categories(void);
const char *filter_category_name(int k);
int filter_category_first(int k);            /* the first filter of category K (they are contiguous) */
int filter_category_size(int k);
int filter_find(const char *key);            /* by a3mx.cfg's value (a favourite's name, a path, custom:...); -1 */
int filter_find_loose(const char *s);        /* --filter: as filter_find, or a favourite by letters and digits only (crt-guest), or a name */
void filter_key(int i, char *out, size_t n); /* what a3mx.cfg keeps for filter I */
const char *filters_shaders_dir(void);       /* where the presets are (after filters_load) */
const char *filters_custom_dir(void);        /* the player's folder for their own ("" if there is none) */
int filters_catalogued(void);                /* how many came from the catalogue */

#ifndef SHADER_TABLE_ONLY
extern int rt_filter;                        /* settings.c: the filter chosen (filter_get's index; 0 OFF) */
extern const char *shader_lib_arg, *shader_dir_arg;   /* main.c: --librashader FILE, --shaders DIR */
/* ge_gl.c, when a frame is ready to show: draws TEX's top-left SRC_W x SRC_H texels (row 0 at the top)
 * through the chosen filter into the window, letterboxed to 480:272, and returns 1; 0 if there is no
 * filter to draw with (OFF, the feature off, no library, no shaders, a preset that failed): the caller
 * then draws the picture itself. */
int shader_gl_present(unsigned tex, int tex_w, int tex_h, int src_w, int src_h, int win_w, int win_h);
void shader_gl_init(void *(*getproc)(const char *), const char *work);
int shader_gl_read(uint8_t *rgba, int max_w, int max_h, int *w, int *h);   /* --dump-filtered: this frame as filtered */
extern int rt_filter_preview;                /* the FILTER screen's preview: the filter shown instead of the setting (-1: none) */
const char *shader_describe(void);           /* last_run.txt: the filter in use, or why there is none */
#endif
