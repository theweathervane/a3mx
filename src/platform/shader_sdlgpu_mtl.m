/* FILTER on the SDL GPU renderer with Metal (macOS, 0.176.0): librashader's Metal runtime, for
 * shader_sdlgpu.c, which does everything else (when, which filter, the blit in, the failures). Its C API
 * takes Metal's own objects, so this part is Objective-C. Metal has no image layouts or resource states:
 * the textures go to librashader as they are, between SDL's passes. Built from the same code as Vulkan
 * and Direct3D 12 but not yet run on a Mac. */
#if defined(__APPLE__) && defined(A3MX_SDL)
#import <Metal/Metal.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

static const char *lib_path;
static void *lib_open(const char *bare, int flags) {
    void *h = lib_path ? (dlopen)(lib_path, flags) : NULL;
    return h ? h : (dlopen)(bare, flags);
}
#define dlopen(a, b) lib_open(a, b)
#define LIBRA_RUNTIME_METAL
#include "librashader_ld.h"
#undef dlopen

static libra_instance_t lib;
static id<MTLCommandQueue> queue;
static libra_mtl_filter_chain_t chain;
static char msg[300];

static const char *error_text(libra_error_t err, const char *doing) {
    char *m = NULL;
    if (err && lib.error_write(err, &m) == 0 && m) { snprintf(msg, sizeof msg, "%s: %s", doing, m); lib.error_free_string(&m); }
    else snprintf(msg, sizeof msg, "%s failed", doing);
    if (err) lib.error_free(&err);
    return msg;
}

/* NULL when the Metal runtime can be used, else why not */
const char *shader_mtl_init(void *mtl_queue, const char *path) {
    lib_path = path;
    queue = (__bridge id<MTLCommandQueue>)mtl_queue;
    if (!queue) return "SDL gave no Metal command queue";
    lib = librashader_load_instance();
    if (!lib.instance_loaded) return "librashader did not load";
    if ((void *)lib.mtl_filter_chain_create == (void *)__librashader__noop_mtl_filter_chain_create)
        return "this librashader has no Metal runtime (tools/librashader.py builds one with it)";
    return NULL;
}

void shader_mtl_drop(void) {
    if (chain) { libra_error_t e = lib.mtl_filter_chain_free(&chain); if (e) lib.error_free(&e); chain = NULL; }
}

/* the chain for the preset at PATH, with PARAMS ("NAME=value;..."): NULL, or what went wrong */
const char *shader_mtl_make(const char *path, const char *params) {
    shader_mtl_drop();
    libra_shader_preset_t preset = NULL;
    libra_error_t err = lib.preset_create(path, &preset);
    if (err || !preset) return error_text(err, "its preset could not be read");
    struct filter_chain_mtl_opt_t opt; memset(&opt, 0, sizeof opt);
    opt.version = LIBRASHADER_CURRENT_VERSION;
    err = lib.mtl_filter_chain_create(&preset, queue, &opt, &chain);
    if (err || !chain) { chain = NULL; return error_text(err, "its shaders did not compile"); }
    for (const char *p = params; p && *p;) {
        char name[64]; float v = 0; int used = 0;
        if (sscanf(p, "%63[^=]=%f%n", name, &v, &used) != 2) break;
        libra_error_t e = lib.mtl_filter_chain_set_param(&chain, name, v);
        if (e) lib.error_free(&e);
        p += used; if (*p == ';') p++;
    }
    return NULL;
}

/* one frame: IN (480 x 272) through the chain into OUT (W x H), recorded into CMD: NULL, or what went wrong */
const char *shader_mtl_frame(void *cmd, void *in, void *out, int w, int h, size_t frame) {
    if (!chain) return "no chain";
    struct libra_viewport_t vp = { 0, 0, (uint32_t)w, (uint32_t)h };
    libra_error_t err = lib.mtl_filter_chain_frame(&chain, (__bridge id<MTLCommandBuffer>)cmd, frame, (__bridge id<MTLTexture>)in,
                                                   (__bridge id<MTLTexture>)out, &vp, NULL, NULL);
    return err ? error_text(err, "it failed while drawing") : NULL;
}
#else
typedef int shader_sdlgpu_mtl_unused;
#endif
