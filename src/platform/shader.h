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
 * The filters are a fixed list: the picture's own look (OFF), five CRTs, four kinds of scanline and two
 * pixel smoothers. Each is one preset from slang-shaders, some with parameters of ours. The names are
 * what SCREEN SETTING shows and what a3mx.cfg holds ([screen setting] filter=). */
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
#define SHADER_FILTER_COUNT ((int)(sizeof SHADER_FILTERS / sizeof SHADER_FILTERS[0]))

#ifndef SHADER_TABLE_ONLY
extern int rt_filter;                        /* settings.c: the filter chosen (an index above; 0 OFF) */
extern const char *shader_lib_arg, *shader_dir_arg;   /* main.c: --librashader FILE, --shaders DIR */
/* ge_gl.c, when a frame is ready to show: draws TEX's top-left SRC_W x SRC_H texels (row 0 at the top)
 * through the chosen filter into the window, letterboxed to 480:272, and returns 1; 0 if there is no
 * filter to draw with (OFF, the feature off, no library, no shaders, a preset that failed): the caller
 * then draws the picture itself. */
int shader_gl_present(unsigned tex, int tex_w, int tex_h, int src_w, int src_h, int win_w, int win_h);
void shader_gl_init(void *(*getproc)(const char *), const char *work);
const char *shader_describe(void);           /* last_run.txt: the filter in use, or why there is none */
#endif
