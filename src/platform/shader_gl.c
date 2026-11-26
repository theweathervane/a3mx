/* SCREEN SETTING > FILTER through librashader's OpenGL runtime (shader.h says what, and what it needs).
 *
 * The game's picture is drawn by ge_gl.c into a texture, row 0 at the top, at the renderer's scale. A
 * CRT or scanline shader wants the picture at the game's own size (480 x 272: its scanlines are the
 * source's rows), the right way up for OpenGL. So each frame:
 *   1. the picture is copied into a 480 x 272 texture, turned over on the way (a blit);
 *   2. librashader draws that through the preset into a texture the size of the letterboxed picture
 *      in the window;
 *   3. that is copied into the window.
 * The chain is made when the filter changes, not every frame; a preset that fails to load is reported
 * once and the picture is shown plain until the filter changes again.
 *
 * Only compiled into builds with a window (A3MX_SDL) or headless OpenGL (A3MX_EGL). */
#include "rt.h"
#include "gl_funcs.h"
#include "shader.h"
#include "ext_features.h"
#include <string.h>
#include <sys/stat.h>

int rt_filter_cli = -1;                      /* main.c: --filter, for this run only (never written to a3mx.cfg) */
static char described[200] = "OFF";
static int presented;                        /* shader_gl_present was asked at least once (there is an OpenGL window) */
int rt_filter_preview = -1;                  /* filterscreen.c: the filter the FILTER screen's preview shows (-1: the setting) */
static int filter_now(void) {
    int f = rt_filter_preview >= 0 ? rt_filter_preview : rt_filter_cli >= 0 ? rt_filter_cli : rt_filter;
    return f >= 0 && f < filter_count() ? f : 0;
}
static const char *fname(int f) { const FilterEntry *e = filter_get(f); return e ? e->name : "OFF"; }
const char *shader_describe(void) {
    if (!presented && FEATURE(SHADERS) && filter_now() > 0)
        snprintf(described, sizeof described, "%s: not shown (this run had no OpenGL window)", fname(filter_now()));
    return described;
}

#if defined(A3MX_SDL) || defined(A3MX_EGL)
/* librashader_ld.h looks for the library by its bare name only. Ours is usually not on the system's
 * search path (it sits beside the game or in .tools/), so the loader's own call is pointed at
 * lib_open(), which tries those places first and the bare name last. */
static char lib_tried[4][1100]; static int lib_tried_n;
static char lib_found[1100];
#ifdef _WIN32
#include <windows.h>
static HMODULE lib_open_w(const wchar_t *bare) {
    for (int i = 0; i < lib_tried_n; i++) {
        wchar_t w[1100];
        if (!MultiByteToWideChar(CP_UTF8, 0, lib_tried[i], -1, w, 1100)) continue;
        HMODULE h = LoadLibraryW(w);
        if (h) { snprintf(lib_found, sizeof lib_found, "%s", lib_tried[i]); return h; }
    }
    HMODULE h = (LoadLibraryW)(bare);
    if (h) snprintf(lib_found, sizeof lib_found, "librashader.dll (found by Windows)");
    return h;
}
#define LoadLibraryW(x) lib_open_w(x)
#define LIB_NAME "librashader.dll"
#else
#include <dlfcn.h>
static void *lib_open(const char *bare, int flags) {
    for (int i = 0; i < lib_tried_n; i++) {
        void *h = (dlopen)(lib_tried[i], flags);
        if (h) { snprintf(lib_found, sizeof lib_found, "%s", lib_tried[i]); return h; }
    }
    void *h = (dlopen)(bare, flags);
    if (h) snprintf(lib_found, sizeof lib_found, "%s (found by the system)", bare);
    return h;
}
#define dlopen(a, b) lib_open(a, b)
#ifdef __APPLE__
#define LIB_NAME "librashader.dylib"
#else
#define LIB_NAME "librashader.so"
#endif
#endif
#define LIBRA_RUNTIME_OPENGL
#include "librashader_ld.h"
#ifdef _WIN32
#undef LoadLibraryW
#else
#undef dlopen
#endif

static libra_instance_t lib;
static int lib_ok, tried_lib;
static void *(*gl_getproc)(const char *);
static const char *shaders_dir = "";
static int shaders_ok;
static libra_gl_filter_chain_t chain;
static int chain_filter = -1;                /* the filter `chain` was made for (-1: none), or that failed */
static int chain_failed;
static GLuint in_tex, in_fbo, src_fbo, out_tex, out_fbo;
static int out_w, out_h;
static size_t frame_no;
static unsigned long frame_seen, drawn_at = ~0ul;   /* shader_gl_present calls; the one that last drew through a filter */

static int is_file(const char *p) { struct stat s; return stat(p, &s) == 0 && (s.st_mode & S_IFMT) == S_IFREG; }
static int is_dir(const char *p) { struct stat s; return stat(p, &s) == 0 && (s.st_mode & S_IFMT) == S_IFDIR; }
static const void *loader(const char *name) { return gl_getproc ? gl_getproc(name) : NULL; }

static void report(libra_error_t err, const char *doing) {
    char *msg = NULL;
    if (err && lib.error_write(err, &msg) == 0 && msg) { rt_warn("filter: %s: %s", doing, msg); lib.error_free_string(&msg); }
    else rt_warn("filter: %s failed", doing);
    if (err) lib.error_free(&err);
}

void shader_gl_init(void *(*getproc)(const char *), const char *work) {
    gl_getproc = getproc;
    if (!FEATURE(SHADERS)) { snprintf(described, sizeof described, "OFF (the shaders feature is off)"); return; }
    /* where the library may be */
    char beside[1100]; snprintf(beside, sizeof beside, "%s", work ? work : "work");
    size_t k = strlen(beside);
    while (k && (beside[k - 1] == '/' || beside[k - 1] == '\\')) beside[--k] = 0;
    while (k && beside[k - 1] != '/' && beside[k - 1] != '\\') beside[--k] = 0;      /* the folder holding work/, with its slash ("" for a bare name) */
    const char *env = getenv("A3MX_LIBRASHADER");
    lib_tried_n = 0;
    if (shader_lib_arg) snprintf(lib_tried[lib_tried_n++], sizeof lib_tried[0], "%s", shader_lib_arg);
    if (env && *env) snprintf(lib_tried[lib_tried_n++], sizeof lib_tried[0], "%s", env);
    snprintf(lib_tried[lib_tried_n++], sizeof lib_tried[0], "%s" LIB_NAME, beside);
    snprintf(lib_tried[lib_tried_n++], sizeof lib_tried[0], "%s.tools/librashader/" LIB_NAME, beside);
    /* where the shaders may be (shader_list.c) */
    shaders_dir = filters_shaders_dir();
    shaders_ok = is_dir(shaders_dir);
}

static int load_lib(void) {
    if (tried_lib) return lib_ok;
    tried_lib = 1;
    lib = librashader_load_instance();
    lib_ok = lib.instance_loaded;
    if (lib_ok) rt_log("filter: librashader %s (ABI %d, API %d)", lib_found, (int)lib.instance_abi_version(), (int)lib.instance_api_version());
    else {
        rt_warn("filter: librashader was not found, so FILTER does nothing. Looked for it at:");
        for (int i = 0; i < lib_tried_n; i++) rt_warn("filter:   %s", lib_tried[i]);
        rt_warn("filter:   and " LIB_NAME " wherever the system looks for libraries (docs/DISPLAY.md, FILTER)");
    }
    return lib_ok;
}

static void drop_chain(void) {
    if (chain) { libra_error_t e = lib.gl_filter_chain_free(&chain); if (e) lib.error_free(&e); chain = NULL; }
}

/* the chain for filter F: made once, kept until the filter changes */
static int want_chain(int f) {
    if (chain_filter == f) return !chain_failed;
    drop_chain();
    chain_filter = f; chain_failed = 1; frame_no = 0;
    const FilterEntry *sf = filter_get(f);
    if (!sf) return 0;
    if (!load_lib()) { snprintf(described, sizeof described, "%s: not shown (librashader was not found)", sf->name); return 0; }
    if (!shaders_ok && !sf->custom) {
        rt_warn("filter: the shaders are not in %s, so FILTER does nothing (python tools/shaders.py fetches them)", shaders_dir);
        snprintf(described, sizeof described, "%s: not shown (no shaders in %s)", sf->name, shaders_dir);
        return 0;
    }
    char path[2200];
    if (sf->custom) snprintf(path, sizeof path, "%s", sf->path); else snprintf(path, sizeof path, "%s/%s", shaders_dir, sf->path);
    if (!is_file(path)) { rt_warn("filter: %s is missing", path); snprintf(described, sizeof described, "%s: not shown (%s is missing)", sf->name, sf->path); return 0; }
    libra_shader_preset_t preset = NULL;
    libra_error_t err = lib.preset_create(path, &preset);
    if (err || !preset) { report(err, path); snprintf(described, sizeof described, "%s: not shown (its preset could not be read)", sf->name); return 0; }
    struct filter_chain_gl_opt_t opt;
    memset(&opt, 0, sizeof opt);
    opt.version = LIBRASHADER_CURRENT_VERSION; opt.glsl_version = 330; opt.use_dsa = false; opt.force_no_mipmaps = false; opt.disable_cache = true;
    err = lib.gl_filter_chain_create(&preset, loader, &opt, &chain);
    if (err || !chain) { report(err, sf->path); chain = NULL; snprintf(described, sizeof described, "%s: not shown (its shaders did not compile)", sf->name); return 0; }
    /* our parameters, "NAME=value;NAME=value" */
    for (const char *p = sf->params; p && *p;) {
        char name[64]; float v = 0; int used = 0;
        if (sscanf(p, "%63[^=]=%f%n", name, &v, &used) != 2) break;
        libra_error_t e = lib.gl_filter_chain_set_param(&chain, name, v);
        if (e) report(e, name);
        p += used; if (*p == ';') p++;
    }
    chain_failed = 0;
    rt_log("filter: %s (%s)", sf->name, sf->path);
    snprintf(described, sizeof described, "%s (%s)", sf->name, sf->path);
    return 1;
}

static void make_tex(GLuint *tex, GLuint *fbo, int w, int h) {
    if (!*tex) p_glGenTextures(1, tex);
    p_glBindTexture(GL_TEXTURE_2D, *tex);
    p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    if (!*fbo) p_glGenFramebuffers(1, fbo);
    p_glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
}

int shader_gl_present(unsigned tex, int tex_w, int tex_h, int src_w, int src_h, int win_w, int win_h) {
    (void)tex_w; (void)tex_h;
    int f = filter_now();
    presented = 1; frame_seen++;
    if (!FEATURE(SHADERS) || f <= 0) {
        if (chain_filter > 0) { drop_chain(); chain_filter = -1; }
        if (FEATURE(SHADERS)) snprintf(described, sizeof described, "OFF");
        return 0;
    }
    if (win_w <= 0 || win_h <= 0 || !want_chain(f)) return 0;
    /* the letterboxed picture in the window, as ge_gl.c places it */
    float sx = (float)win_w / 480.f, sy = (float)win_h / 272.f, sc = sx < sy ? sx : sy;
    int w = (int)(480.f * sc + 0.5f), h = (int)(272.f * sc + 0.5f);
    int x0 = (win_w - w) / 2, y0 = (win_h - h) / 2;
    if (w <= 0 || h <= 0) return 0;

    p_glDisable(GL_SCISSOR_TEST); p_glDisable(GL_BLEND); p_glColorMask(1, 1, 1, 1);
    /* 1. the picture at the game's own size, turned over (row 0 at the bottom, as OpenGL has it) */
    if (!in_tex) make_tex(&in_tex, &in_fbo, 480, 272);
    if (!src_fbo) p_glGenFramebuffers(1, &src_fbo);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
    p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, in_fbo);
    p_glBlitFramebuffer(0, src_h, src_w, 0, 0, 0, 480, 272, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    /* 2. through the preset, into a texture the size it will have in the window */
    if (!out_tex || out_w != w || out_h != h) { make_tex(&out_tex, &out_fbo, w, h); out_w = w; out_h = h; }
    struct libra_image_gl_t src = { in_tex, GL_RGBA8, 480, 272 }, dst = { out_tex, GL_RGBA8, (uint32_t)w, (uint32_t)h };
    struct libra_viewport_t vp = { 0, 0, (uint32_t)w, (uint32_t)h };
    libra_error_t err = lib.gl_filter_chain_frame(&chain, frame_no++, src, dst, &vp, NULL, NULL);
    if (err) {                               /* once: the picture is plain until the filter changes */
        report(err, filter_get(f)->path);
        chain_failed = 1; drop_chain();
        snprintf(described, sizeof described, "%s: not shown (it failed while drawing)", fname(f));
        return 0;
    }
    /* 3. into the window */
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    p_glDisable(GL_SCISSOR_TEST);
    p_glViewport(0, 0, win_w, win_h);
    p_glClearColor(0, 0, 0, 1);
    p_glClear(GL_COLOR_BUFFER_BIT);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, out_fbo);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    p_glBlitFramebuffer(0, 0, w, h, x0, y0, x0 + w, y0 + h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawn_at = frame_seen;
    return 1;
}

/* the FILTER screen: whether a filter can be shown at all (the window, the library, the shaders) */
int shader_gl_ready(void) { return FEATURE(SHADERS) && gl_getproc && load_lib() && shaders_ok; }

/* --dump-filtered (main.c): the last frame as the filter drew it (the letterboxed picture, top row
   first), if it drew this frame; 0 otherwise (the dump is then the game's own picture) */
int shader_gl_read(uint8_t *rgba, int max_w, int max_h, int *w, int *h) {
    if (!out_fbo || drawn_at != frame_seen || out_w > max_w || out_h > max_h) return 0;
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, out_fbo);
    p_glPixelStorei(GL_PACK_ALIGNMENT, 4);
    p_glReadPixels(0, 0, out_w, out_h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    for (int y = 0; y < out_h / 2; y++)                     /* (OpenGL's row 0 is the bottom) */
        for (int x = 0; x < out_w * 4; x++) {
            uint8_t *a = rgba + (size_t)y * out_w * 4 + x, *b = rgba + (size_t)(out_h - 1 - y) * out_w * 4 + x, t = *a;
            *a = *b; *b = t;
        }
    for (int i = 0; i < out_w * out_h; i++) rgba[i * 4 + 3] = 255;
    *w = out_w; *h = out_h;
    return 1;
}

/* --filter-test LIST OUT (tools/shaders.py --catalogue): each preset LIST names (one a line, under the
   shaders folder) loaded, built and drawn for a few frames into a 1904 x 1080 picture, a line to OUT
   for each as it is done: "path<TAB>ok<TAB>build ms<TAB>frame ms", or "path<TAB>fail<TAB>why"; a
   line "*<TAB>why" if nothing can be tested. The game itself does not start. Returns the exit code. */
#include "perf.h"
void rt_log_flush(void);
int shader_gl_test(const char *list_path, const char *out_path) {
    FILE *in = fopen(list_path, "r"), *out = fopen(out_path, "w");
    if (!out) { rt_warn("--filter-test: cannot write %s", out_path); if (in) fclose(in); return 2; }
    if (!in) { fprintf(out, "*\tcannot read %s\n", list_path); fclose(out); return 2; }
    if (!gl_getproc) { fprintf(out, "*\tno OpenGL window\n"); fclose(out); fclose(in); return 2; }
    if (!load_lib()) { fprintf(out, "*\tthe game did not find librashader (--librashader FILE)\n"); fclose(out); fclose(in); return 2; }
    const int W = 1904, H = 1080;
    if (!in_tex) make_tex(&in_tex, &in_fbo, 480, 272);
    make_tex(&out_tex, &out_fbo, W, H); out_w = W; out_h = H;
    {   /* a picture to draw: bands of colour */
        static uint8_t px[480 * 272 * 4];
        for (int y = 0; y < 272; y++) for (int x = 0; x < 480; x++) {
            uint8_t *p = px + (y * 480 + x) * 4; p[0] = (uint8_t)(x * 255 / 479); p[1] = (uint8_t)(y * 255 / 271); p[2] = (uint8_t)((x ^ y) & 0xFF); p[3] = 255;
        }
        p_glBindTexture(GL_TEXTURE_2D, in_tex);
        p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 480, 272, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
    char line[1200]; int n = 0, ok = 0;
    while (fgets(line, sizeof line, in)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0]) continue;
        char path[2400]; snprintf(path, sizeof path, "%s/%s", shaders_dir, line);
        rt_log("filter-test: %s", line); rt_log_flush();
        double t0 = (double)perf_now_us();
        libra_shader_preset_t preset = NULL;
        libra_error_t err = lib.preset_create(path, &preset);
        if (err || !preset) { if (err) lib.error_free(&err); fprintf(out, "%s\tfail\tnot read\n", line); fflush(out); n++; continue; }
        struct filter_chain_gl_opt_t opt; memset(&opt, 0, sizeof opt);
        opt.version = LIBRASHADER_CURRENT_VERSION; opt.glsl_version = 330; opt.disable_cache = true;
        libra_gl_filter_chain_t ch = NULL;
        err = lib.gl_filter_chain_create(&preset, loader, &opt, &ch);
        if (err || !ch) { if (err) lib.error_free(&err); fprintf(out, "%s\tfail\tnot built\n", line); fflush(out); n++; continue; }
        p_glFinish();
        double t1 = (double)perf_now_us(), t2 = t1;
        struct libra_image_gl_t src = { in_tex, GL_RGBA8, 480, 272 }, dst = { out_tex, GL_RGBA8, (uint32_t)W, (uint32_t)H };
        struct libra_viewport_t vp = { 0, 0, (uint32_t)W, (uint32_t)H };
        int good = 1;
        for (int k = 0; k < 8 && good; k++) {
            if (k == 3) { p_glFinish(); t2 = (double)perf_now_us(); }   /* (the first frames make what they need) */
            err = lib.gl_filter_chain_frame(&ch, (size_t)k, src, dst, &vp, NULL, NULL);
            if (err) { lib.error_free(&err); good = 0; }
        }
        p_glFinish();
        double t3 = (double)perf_now_us();
        if (good) { fprintf(out, "%s\tok\t%.0f\t%.2f\n", line, (t1 - t0) / 1000.0, (t3 - t2) / 5000.0); ok++; }
        else fprintf(out, "%s\tfail\tfailed while drawing\n", line);
        fflush(out);
        libra_error_t e = lib.gl_filter_chain_free(&ch); if (e) lib.error_free(&e);
        n++;
    }
    fclose(in); fclose(out);
    rt_log("filter-test: %d presets, %d drawn", n, ok);
    return 0;
}
#else   /* no OpenGL in this build (headless tests): nothing to draw with */
void shader_gl_init(void *(*getproc)(const char *), const char *work) { (void)getproc; (void)work; }
int shader_gl_ready(void) { return 0; }
int shader_gl_read(uint8_t *rgba, int max_w, int max_h, int *w, int *h) { (void)rgba; (void)max_w; (void)max_h; (void)w; (void)h; return 0; }
int shader_gl_test(const char *list_path, const char *out_path) {
    FILE *out = fopen(out_path, "w");
    if (out) { fprintf(out, "*\tthis build has no OpenGL\n"); fclose(out); }
    (void)list_path; return 2;
}
int shader_gl_present(unsigned tex, int tex_w, int tex_h, int src_w, int src_h, int win_w, int win_h) {
    (void)tex; (void)tex_w; (void)tex_h; (void)src_w; (void)src_h; (void)win_w; (void)win_h; return 0;
}
#endif
