/* SCREEN SETTING > FILTER on the SDL3 GPU renderer (ge_sdlgpu.c, 0.176.0): librashader's Vulkan and
 * Direct3D 12 runtimes (shader_sdlgpu_mtl.m: Metal) drawing into SDL_GPU's own work, as shader_gl.c does
 * for OpenGL. SDL_GPU doesn't hand out its device, command buffers or textures; the project's patch to
 * SDL (cmake/patches/sdl-gpu-native-handles.patch) adds them as device properties. Where any piece is
 * missing (an SDL without the patch, a librashader without the API's runtime, Direct3D 12 without the
 * DXC libraries beside the game), FILTER stays greyed on this renderer with the reason, as before.
 *
 * Each frame with a filter (ge_sdlgpu.c's present):
 *   1. the target's picture blitted into a 480 x 272 texture (the game's own size: scanlines follow
 *      its rows), row 0 at the top, as librashader wants;
 *   2. into the command buffer, natively: the output texture to what librashader wants it in (Vulkan:
 *      COLOR_ATTACHMENT_OPTIMAL; Direct3D 12: RENDER_TARGET, and the input PIXEL_SHADER_RESOURCE), the
 *      chain's frame, and back to the layout / state SDL keeps it in outside passes;
 *   3. that command buffer submitted: what librashader set (Direct3D 12's descriptor heaps, Vulkan's
 *      bindings) never meets SDL's later commands, which go in a new command buffer;
 *   4. ge_sdlgpu.c draws the output to the swapchain.
 * The chain is made when the filter changes, as in shader_gl.c, and failures are reported the same way
 * (the FILTER screen greys the filter with the reason). */
#include "rt.h"
#include "shader.h"
#include "ext_features.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef A3MX_SDL
#include "vk_min/vulkan/vulkan.h"       /* (librashader.h's <vulkan/vulkan.h>: the same file, by the include path CMake gives) */
#ifdef _WIN32
#define COBJMACROS
#include <windows.h>
#include <d3d12.h>
#endif
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

/* librashader, through librashader_ld.h with the runtimes this renderer may use; the library file is
   the one shader_gl.c found (shader_lib_path) */
static const char *lib_path;
#ifdef _WIN32
static HMODULE lib_open_w(const wchar_t *bare) {
    wchar_t w[1100];
    if (lib_path && MultiByteToWideChar(CP_UTF8, 0, lib_path, -1, w, 1100)) { HMODULE h = (LoadLibraryW)(w); if (h) return h; }
    return (LoadLibraryW)(bare);
}
#define LoadLibraryW(x) lib_open_w(x)
#else
#include <dlfcn.h>
static void *lib_open(const char *bare, int flags) {
    void *h = lib_path ? (dlopen)(lib_path, flags) : NULL;
    return h ? h : (dlopen)(bare, flags);
}
#define dlopen(a, b) lib_open(a, b)
#endif
#define LIBRA_RUNTIME_VULKAN
#ifdef _WIN32
#define LIBRA_RUNTIME_D3D12
#endif
#include "librashader_ld.h"
#ifdef _WIN32
#undef LoadLibraryW
#else
#undef dlopen
#endif

/* shader_gl.c's, shared by both renderers */
int shader_lib_load(void);
const char *shader_lib_path(void);
int shader_filter_now(void);
void shader_set_failed(int f, const char *why);
void shader_set_described(const char *s);
void shader_mark_presented(void);

#define P_NATIVE_CB "SDL.gpu.device.native_command_buffer_function"
#define P_NATIVE_TEX "SDL.gpu.device.native_texture_function"
typedef void *(SDLCALL *NativeCbFn)(SDL_GPUCommandBuffer *);
typedef void *(SDLCALL *NativeTexFn)(SDL_GPUTexture *, Uint32 *);

enum { API_NONE, API_VULKAN, API_D3D12, API_METAL };
#ifdef __APPLE__
const char *shader_mtl_init(void *mtl_queue, const char *path);   /* shader_sdlgpu_mtl.m */
const char *shader_mtl_make(const char *path, const char *params);
const char *shader_mtl_frame(void *cmd, void *in, void *out, int w, int h, size_t frame);
void shader_mtl_drop(void);
static int mtl_chain;
#endif
static int api;
static libra_instance_t lib;
static SDL_GPUDevice *dev;
static NativeCbFn native_cb;
static NativeTexFn native_tex;
static VkInstance vk_instance; static VkPhysicalDevice vk_phys; static VkDevice vk_device; static VkQueue vk_queue;
static PFN_vkGetInstanceProcAddr vk_gipa;
static PFN_vkCmdPipelineBarrier vk_barrier;
#ifdef _WIN32
static ID3D12Device *d3d_device;
static libra_d3d12_filter_chain_t d3d_chain;
#endif
static libra_vk_filter_chain_t vk_chain;
static int chain_filter = -1, chain_failed;
static size_t frame_no;
static SDL_GPUTexture *in_tex, *out_tex;
static int out_w, out_h;
static char why[160];
int shader_sdlgpu_drawn;                       /* the last frame went through a filter (--dump-filtered) */

static int is_file(const char *p) { struct stat s; return stat(p, &s) == 0 && (s.st_mode & S_IFMT) == S_IFREG; }

static void report(libra_error_t err, const char *doing) {
    char *msg = NULL;
    if (err && lib.error_write(err, &msg) == 0 && msg) { rt_warn("filter (SDL GPU): %s: %s", doing, msg); lib.error_free_string(&msg); }
    else rt_warn("filter (SDL GPU): %s failed", doing);
    if (err) lib.error_free(&err);
}

/* NULL when FILTER can be drawn on this device, else why not (also for the log) */
const char *shader_sdlgpu_init(SDL_GPUDevice *device, const char *driver) {
    dev = device;
    SDL_PropertiesID pr = SDL_GetGPUDeviceProperties(dev);
    native_cb = (NativeCbFn)SDL_GetPointerProperty(pr, P_NATIVE_CB, NULL);
    native_tex = (NativeTexFn)SDL_GetPointerProperty(pr, P_NATIVE_TEX, NULL);
    if (!native_cb || !native_tex) return "this SDL hasn't the project's patch (cmake/patches/sdl-gpu-native-handles.patch)";
    if (!shader_lib_load()) return "librashader was not found";
    lib_path = shader_lib_path();
    lib = librashader_load_instance();
    if (!lib.instance_loaded) return "librashader did not load";
    if (driver && !strcmp(driver, "vulkan")) {
        vk_instance = (VkInstance)SDL_GetPointerProperty(pr, "SDL.gpu.device.vulkan.instance", NULL);
        vk_phys = (VkPhysicalDevice)SDL_GetPointerProperty(pr, "SDL.gpu.device.vulkan.physical_device", NULL);
        vk_device = (VkDevice)SDL_GetPointerProperty(pr, "SDL.gpu.device.vulkan.device", NULL);
        vk_queue = (VkQueue)SDL_GetPointerProperty(pr, "SDL.gpu.device.vulkan.queue", NULL);
        vk_gipa = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
        if (!vk_instance || !vk_device || !vk_gipa) return "SDL gave no Vulkan device";
        PFN_vkGetDeviceProcAddr gdpa = (PFN_vkGetDeviceProcAddr)vk_gipa(vk_instance, "vkGetDeviceProcAddr");
        vk_barrier = gdpa ? (PFN_vkCmdPipelineBarrier)gdpa(vk_device, "vkCmdPipelineBarrier") : NULL;
        if (!vk_barrier) return "no vkCmdPipelineBarrier";
        if ((void *)lib.vk_filter_chain_create == (void *)__librashader__noop_vk_filter_chain_create)
            return "this librashader has no Vulkan runtime (tools/librashader.py builds one with it)";
        api = API_VULKAN;
        return NULL;
    }
#ifdef __APPLE__
    if (driver && !strcmp(driver, "metal")) {
        void *q = SDL_GetPointerProperty(pr, "SDL.gpu.device.metal.command_queue", NULL);
        const char *no = shader_mtl_init(q, lib_path);
        if (no) return no;
        api = API_METAL;
        return NULL;
    }
#endif
#ifdef _WIN32
    if (driver && !strcmp(driver, "direct3d12")) {
        d3d_device = (ID3D12Device *)SDL_GetPointerProperty(pr, "SDL.gpu.device.d3d12.device", NULL);
        if (!d3d_device) return "SDL gave no Direct3D 12 device";
        if ((void *)lib.d3d12_filter_chain_create == (void *)__librashader__noop_d3d12_filter_chain_create)
            return "this librashader has no Direct3D 12 runtime (tools/librashader.py builds one with it)";
        HMODULE dxc = LoadLibraryA("dxcompiler.dll"), dxil = LoadLibraryA("dxil.dll");
        if (!dxc || !dxil) return "dxcompiler.dll and dxil.dll are not beside the game (tools/build.py puts them there)";
        api = API_D3D12;
        return NULL;
    }
#endif
    return "no librashader runtime for this API here";
}

static void drop_chain(void) {
#ifdef __APPLE__
    if (mtl_chain) { SDL_WaitForGPUIdle(dev); shader_mtl_drop(); mtl_chain = 0; }
#endif
    if (vk_chain) { SDL_WaitForGPUIdle(dev); libra_error_t e = lib.vk_filter_chain_free(&vk_chain); if (e) lib.error_free(&e); vk_chain = NULL; }
#ifdef _WIN32
    if (d3d_chain) { SDL_WaitForGPUIdle(dev); libra_error_t e = lib.d3d12_filter_chain_free(&d3d_chain); if (e) lib.error_free(&e); d3d_chain = NULL; }
#endif
}

/* the chain for filter F, made once */
static int want_chain(int f) {
    if (chain_filter == f) return !chain_failed;
    drop_chain();
    chain_filter = f; chain_failed = 1; frame_no = 0;
    const FilterEntry *sf = filter_get(f);
    if (!sf) return 0;
    char path[2200], d[300];
    if (sf->custom) snprintf(path, sizeof path, "%s", sf->path); else snprintf(path, sizeof path, "%s/%s", filters_shaders_dir(), sf->path);
    if (!is_file(path)) { snprintf(d, sizeof d, "%s: not shown (%s is missing)", sf->name, sf->path); shader_set_described(d); return 0; }
#ifdef __APPLE__
    if (api == API_METAL) {
        const char *no = shader_mtl_make(path, sf->params);
        if (no) {
            rt_warn("filter (SDL GPU): %s: %s", sf->path, no);
            snprintf(d, sizeof d, "%s: not shown (%s)", sf->name, no); shader_set_described(d);
            shader_set_failed(f, strstr(no, "preset") ? "ITS PRESET COULD NOT BE READ" : "ITS SHADERS DID NOT COMPILE");
            return 0;
        }
        mtl_chain = 1; chain_failed = 0;
        rt_log("filter (SDL GPU): %s (%s)", sf->name, sf->path);
        snprintf(d, sizeof d, "%s (%s)", sf->name, sf->path); shader_set_described(d);
        return 1;
    }
#endif
    libra_shader_preset_t preset = NULL;
    libra_error_t err = lib.preset_create(path, &preset);
    if (err || !preset) { report(err, path); snprintf(d, sizeof d, "%s: not shown (its preset could not be read)", sf->name); shader_set_described(d); shader_set_failed(f, "ITS PRESET COULD NOT BE READ"); return 0; }
    if (api == API_VULKAN) {
        struct filter_chain_vk_opt_t opt; memset(&opt, 0, sizeof opt);
        opt.version = LIBRASHADER_CURRENT_VERSION; opt.frames_in_flight = 3; opt.disable_cache = true;
        struct libra_device_vk_t vk = { vk_phys, vk_instance, vk_device, vk_queue, vk_gipa };
        err = lib.vk_filter_chain_create(&preset, vk, &opt, &vk_chain);
        if (err || !vk_chain) { report(err, sf->path); vk_chain = NULL; goto failed; }
        for (const char *p = sf->params; p && *p;) {
            char name[64]; float v = 0; int used = 0;
            if (sscanf(p, "%63[^=]=%f%n", name, &v, &used) != 2) break;
            libra_error_t e = lib.vk_filter_chain_set_param(&vk_chain, name, v);
            if (e) report(e, name);
            p += used; if (*p == ';') p++;
        }
    }
#ifdef _WIN32
    else if (api == API_D3D12) {
        struct filter_chain_d3d12_opt_t opt; memset(&opt, 0, sizeof opt);
        opt.version = LIBRASHADER_CURRENT_VERSION; opt.frames_in_flight = 3; opt.disable_cache = true;
        err = lib.d3d12_filter_chain_create(&preset, d3d_device, &opt, &d3d_chain);
        if (err || !d3d_chain) { report(err, sf->path); d3d_chain = NULL; goto failed; }
        for (const char *p = sf->params; p && *p;) {
            char name[64]; float v = 0; int used = 0;
            if (sscanf(p, "%63[^=]=%f%n", name, &v, &used) != 2) break;
            libra_error_t e = lib.d3d12_filter_chain_set_param(&d3d_chain, name, v);
            if (e) report(e, name);
            p += used; if (*p == ';') p++;
        }
    }
#endif
    else return 0;
    chain_failed = 0;
    rt_log("filter (SDL GPU): %s (%s)", sf->name, sf->path);
    snprintf(d, sizeof d, "%s (%s)", sf->name, sf->path); shader_set_described(d);
    return 1;
failed:
    snprintf(d, sizeof d, "%s: not shown (its shaders did not compile)", sf->name); shader_set_described(d);
    shader_set_failed(f, "ITS SHADERS DID NOT COMPILE");
    return 0;
}

static SDL_GPUTexture *texture(int w, int h) {
    SDL_GPUTextureCreateInfo ci; memset(&ci, 0, sizeof ci);
    ci.type = SDL_GPU_TEXTURETYPE_2D; ci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    ci.width = (Uint32)w; ci.height = (Uint32)h; ci.layer_count_or_depth = 1; ci.num_levels = 1; ci.sample_count = SDL_GPU_SAMPLECOUNT_1;
    return SDL_CreateGPUTexture(dev, &ci);
}

static void vk_transition(VkCommandBuffer cb, VkImage img, VkImageLayout from, VkImageLayout to, VkAccessFlags src_access, VkAccessFlags dst_access,
                          VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
    VkImageMemoryBarrier b; memset(&b, 0, sizeof b);
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; b.srcAccessMask = src_access; b.dstAccessMask = dst_access;
    b.oldLayout = from; b.newLayout = to; b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.image = img;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; b.subresourceRange.levelCount = 1; b.subresourceRange.layerCount = 1;
    vk_barrier(cb, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &b);
}
#ifdef _WIN32
static void d3d_transition(ID3D12GraphicsCommandList *cl, ID3D12Resource *r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    if (from == to) return;
    D3D12_RESOURCE_BARRIER b; memset(&b, 0, sizeof b);
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = r; b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = from; b.Transition.StateAfter = to;
    ID3D12GraphicsCommandList_ResourceBarrier(cl, 1, &b);
}
#endif

/* ge_sdlgpu.c, presenting: the picture SRC (SRC_W x SRC_H of its top left) through the filter in use into
   a texture W x H (the letterboxed picture in the window), recorded into CMD, which is then submitted
   (*cmd is NULL after). That texture, or NULL: no filter to draw with (the picture is shown plain). */
SDL_GPUTexture *shader_sdlgpu_present(SDL_GPUCommandBuffer **cmd, SDL_GPUTexture *src, int src_w, int src_h, int w, int h) {
    shader_sdlgpu_drawn = 0;
    if (!api || !dev) return NULL;
    int f = shader_filter_now();
    shader_mark_presented();
    if (!FEATURE(SHADERS) || f <= 0) {
        if (chain_filter > 0) { drop_chain(); chain_filter = -1; }
        if (FEATURE(SHADERS)) shader_set_described("OFF");
        return NULL;
    }
    if (w <= 0 || h <= 0 || !want_chain(f)) return NULL;
    if (!in_tex && !(in_tex = texture(480, 272))) return NULL;
    if (!out_tex || out_w != w || out_h != h) {
        if (out_tex) { SDL_WaitForGPUIdle(dev); SDL_ReleaseGPUTexture(dev, out_tex); }
        out_tex = texture(w, h); out_w = w; out_h = h;
        if (!out_tex) return NULL;
    }
    /* 1. the picture at the game's own size */
    SDL_GPUBlitInfo bi; memset(&bi, 0, sizeof bi);
    bi.source.texture = src; bi.source.w = (Uint32)src_w; bi.source.h = (Uint32)src_h;
    bi.destination.texture = in_tex; bi.destination.w = 480; bi.destination.h = 272;
    bi.load_op = SDL_GPU_LOADOP_DONT_CARE; bi.filter = SDL_GPU_FILTER_LINEAR;
    SDL_BlitGPUTexture(*cmd, &bi);
    /* 2. the chain, natively, in the same command buffer */
    Uint32 in_state = 0, out_state = 0;
    void *ncb = native_cb(*cmd), *nin = native_tex(in_tex, &in_state), *nout = native_tex(out_tex, &out_state);
    libra_error_t err = NULL;
    struct libra_viewport_t vp = { 0, 0, (uint32_t)w, (uint32_t)h };
    if (!ncb || !nin || !nout) err = NULL, nin = NULL;
    else if (api == API_VULKAN) {
        VkCommandBuffer cb = (VkCommandBuffer)ncb;
        vk_transition(cb, (VkImage)nout, (VkImageLayout)out_state, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        if ((VkImageLayout)in_state != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            vk_transition(cb, (VkImage)nin, (VkImageLayout)in_state, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        struct libra_image_vk_t li = { (VkImage)nin, VK_FORMAT_R8G8B8A8_UNORM, 480, 272 }, lo = { (VkImage)nout, VK_FORMAT_R8G8B8A8_UNORM, (uint32_t)w, (uint32_t)h };
        err = lib.vk_filter_chain_frame(&vk_chain, cb, frame_no++, li, lo, &vp, NULL, NULL);
        if ((VkImageLayout)in_state != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            vk_transition(cb, (VkImage)nin, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (VkImageLayout)in_state, VK_ACCESS_SHADER_READ_BIT,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        vk_transition(cb, (VkImage)nout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, (VkImageLayout)out_state, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
#ifdef _WIN32
    else if (api == API_D3D12) {
        ID3D12GraphicsCommandList *cl = (ID3D12GraphicsCommandList *)ncb;
        ID3D12Resource *ri = (ID3D12Resource *)nin, *ro = (ID3D12Resource *)nout;
        d3d_transition(cl, ri, (D3D12_RESOURCE_STATES)in_state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        d3d_transition(cl, ro, (D3D12_RESOURCE_STATES)out_state, D3D12_RESOURCE_STATE_RENDER_TARGET);
        struct libra_image_d3d12_t li, lo; memset(&li, 0, sizeof li); memset(&lo, 0, sizeof lo);
        li.image_type = LIBRA_D3D12_IMAGE_TYPE_RESOURCE; li.handle.resource = ri;
        lo.image_type = LIBRA_D3D12_IMAGE_TYPE_RESOURCE; lo.handle.resource = ro;
        err = lib.d3d12_filter_chain_frame(&d3d_chain, cl, frame_no++, li, lo, &vp, NULL, NULL);
        d3d_transition(cl, ri, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, (D3D12_RESOURCE_STATES)in_state);
        d3d_transition(cl, ro, D3D12_RESOURCE_STATE_RENDER_TARGET, (D3D12_RESOURCE_STATES)out_state);
    }
#endif
#ifdef __APPLE__
    const char *mtl_err = NULL;
    if (api == API_METAL && ncb && nin && nout) mtl_err = shader_mtl_frame(ncb, nin, nout, w, h, frame_no++);
#endif
    /* 3. submitted: SDL's next commands start in a command buffer of their own */
    SDL_SubmitGPUCommandBuffer(*cmd);
    *cmd = NULL;
    if (!nin) return NULL;
#ifdef __APPLE__
    if (mtl_err) {
        rt_warn("filter (SDL GPU): %s: %s", filter_get(f)->path, mtl_err);
        chain_failed = 1; drop_chain();
        char d[300]; snprintf(d, sizeof d, "%s: not shown (it failed while drawing)", filter_get(f)->name); shader_set_described(d);
        shader_set_failed(f, "IT FAILED WHILE DRAWING");
        return NULL;
    }
#endif
    if (err) {                               /* once: the picture is plain until the filter changes */
        report(err, filter_get(f)->path);
        chain_failed = 1; drop_chain();
        char d[300]; snprintf(d, sizeof d, "%s: not shown (it failed while drawing)", filter_get(f)->name); shader_set_described(d);
        shader_set_failed(f, "IT FAILED WHILE DRAWING");
        return NULL;
    }
    shader_sdlgpu_drawn = 1;
    return out_tex;
}
SDL_GPUTexture *shader_sdlgpu_output(int *w, int *h) { *w = out_w; *h = out_h; return shader_sdlgpu_drawn ? out_tex : NULL; }
const char *shader_sdlgpu_why(void) { return why; }
#endif
typedef int shader_sdlgpu_unused;       /* (a build without SDL: nothing above) */
