/* The GPU renderer through SDL3's GPU API (0.174.0): Vulkan, Direct3D 12 or Metal, beside OpenGL
 * (ge_gl.c). A GeBackend (ge_backend.h): it draws what ge_state.c hands it (triangles and a
 * GeDrawState) and keeps its textures with the shared cache (ge_texcache.c), so it reads the GE as
 * OpenGL does. Chosen by --renderer vulkan / d3d12 / metal (the launcher's Renderer); OpenGL stays the
 * default, and if the API can't be had the game falls back to OpenGL and says why.
 *
 * As ge_gl.c: every frame-buffer address the game draws to gets a render target of 512*S x 272*S, row 0
 * at the top (SDL_GPU's own way up, so nothing is turned over); a draw that samples one of them samples
 * the upscaled target. Draws are batched while their state is the same; batches are recorded into
 * render passes, one per run of batches into the same target, when anything needs the picture as it
 * stands (a texture upload, a new target, presenting, a readback, the frame's end). Texture uploads go
 * in their own copy passes between those, so each draw sees the texture as it was when it was asked for.
 *
 * Shaders: src/platform/gpu_shaders/ (HLSL; Metal's own source), compiled once by tools/gpu_shaders.py
 * into gpu_shaders.h (DXIL, SPIR-V, MSL): no compiler at build time.
 *
 * Not here yet: FILTER (librashader needs the API's own device, which SDL_GPU doesn't hand out: the
 * picture is shown plain and SCREEN SETTING greys FILTER, "OPENGL ONLY"); the GPU-time figure in the
 * perf log (SDL_GPU has no timer queries); mip-mapped presentation below half size. Metal is built
 * from the same code but untested. */
#include "rt.h"
#include "ge.h"
#include "ge_backend.h"
#include "ge_texcache.h"
#include "perf.h"
#include <string.h>
#include <stdlib.h>

#ifdef A3MX_SDL
#include <SDL3/SDL.h>
#include "gpu_shaders.h"

#define LW 512          /* logical render-target size in guest pixels */
#define LH 272

static SDL_GPUDevice *dev;
static SDL_Window *win;
static int S = 4, ready;
static const char *scale_reason = "auto";
static char gpu_name_buf[300];
static SDL_GPUShader *draw_vs, *draw_fs, *present_vs, *present_fs;
static SDL_GPUGraphicsPipeline *present_pipe;
static SDL_GPUTextureFormat swap_fmt = SDL_GPU_TEXTUREFORMAT_INVALID;
static SDL_GPUSampler *samplers[8];          /* linear << 2 | clamp_s << 1 | clamp_t */
static SDL_GPUTexture *dummy_tex;            /* bound when a draw has no texture */
static SDL_GPUBuffer *vbuf, *quad_buf;
static SDL_GPUTransferBuffer *vtb, *dltb;
static uint32_t dltb_size;
static SDL_GPUCommandBuffer *cmd;
static uint64_t frame_no;
static int frame_draws, frame_calls;

typedef struct { float x, y, u, v; uint8_t c[4]; } GVtx;   /* = GeVtx */
typedef struct { float fbsize[2], pad[2]; } VertUniforms;
typedef struct {
    float env[4]; float texsize[2]; float aref, amask;
    int32_t tex_on, texfunc, rgba, dbl;
    int32_t atest, afunc, clear_on, pad1;
} FragUniforms;
typedef struct { float src_size[2], out_size[2], uv_max[2]; int32_t sharp, pad0; } PresentUniforms;

/* ---------------------------------------------------------------- command buffer */
static SDL_GPUCommandBuffer *cb(void) {
    if (!cmd) cmd = SDL_AcquireGPUCommandBuffer(dev);
    return cmd;
}
static void submit(void) { if (cmd) { SDL_SubmitGPUCommandBuffer(cmd); cmd = NULL; } }

/* ---------------------------------------------------------------- render targets */
typedef struct { uint32_t addr; SDL_GPUTexture *tex; int used; } Target;
static Target targets[16];
static int ntargets;
static void flush(void);

static SDL_GPUTexture *make_texture(int w, int h, SDL_GPUTextureUsageFlags usage) {
    SDL_GPUTextureCreateInfo ci;
    memset(&ci, 0, sizeof ci);
    ci.type = SDL_GPU_TEXTURETYPE_2D; ci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM; ci.usage = usage;
    ci.width = (Uint32)w; ci.height = (Uint32)h; ci.layer_count_or_depth = 1; ci.num_levels = 1; ci.sample_count = SDL_GPU_SAMPLECOUNT_1;
    return SDL_CreateGPUTexture(dev, &ci);
}

static Target *target_for(uint32_t addr, int create) {
    addr &= 0x0FFFFFFF;
    for (int i = 0; i < ntargets; i++) if (targets[i].addr == addr) return &targets[i];
    if (!create || ntargets >= 16) return NULL;
    flush();                                               /* (the clear below goes in order) */
    Target *t = &targets[ntargets];
    t->tex = make_texture(LW * S, LH * S, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!t->tex) { rt_warn("SDL GPU: render target %08X: %s", addr, SDL_GetError()); return NULL; }
    t->addr = addr; t->used = 0;
    ntargets++;
    SDL_GPUColorTargetInfo ct;
    memset(&ct, 0, sizeof ct);
    ct.texture = t->tex; ct.load_op = SDL_GPU_LOADOP_CLEAR; ct.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cb(), &ct, 1, NULL);
    if (rp) SDL_EndGPURenderPass(rp);
    rt_log("SDL GPU: render target %08X (%dx%d)", addr, LW * S, LH * S);
    return t;
}
static Target *target_containing(uint32_t a, int *ox, int *oy) {
    a &= 0x0FFFFFFF;
    for (int i = 0; i < ntargets; i++) {
        uint32_t base = targets[i].addr, bytes = LW * LH * 2u;
        if (a >= base && a < base + bytes) {
            uint32_t px = (a - base) / 2u;
            *ox = (int)(px % LW); *oy = (int)(px / LW);
            return &targets[i];
        }
    }
    return NULL;
}

/* ---------------------------------------------------------------- textures from guest memory (ge_texcache.c) */
typedef struct { SDL_GPUTexture *t; int w, h; } Tex;
static uintptr_t tex_create(void) { return (uintptr_t)calloc(1, sizeof(Tex)); }
static void tex_upload(uintptr_t h, const uint8_t *rgba, int w, int hh, int y0, int y1, int whole) {
    Tex *t = (Tex *)h;
    if (!t) return;
    if (whole && (!t->t || t->w != w || t->h != hh)) {
        if (t->t) SDL_ReleaseGPUTexture(dev, t->t);
        t->t = make_texture(w, hh, SDL_GPU_TEXTUREUSAGE_SAMPLER); t->w = w; t->h = hh;
        if (!t->t) return;
    }
    if (!t->t || y1 <= y0) return;
    uint32_t bytes = (uint32_t)w * (uint32_t)(y1 - y0) * 4u;
    SDL_GPUTransferBufferCreateInfo ti = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, bytes, 0 };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(dev, &ti);
    if (!tb) return;
    void *p = SDL_MapGPUTransferBuffer(dev, tb, false);
    if (p) { memcpy(p, rgba, bytes); SDL_UnmapGPUTransferBuffer(dev, tb); }
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb());
    SDL_GPUTextureTransferInfo src = { tb, 0, (Uint32)w, (Uint32)(y1 - y0) };
    SDL_GPUTextureRegion dst;
    memset(&dst, 0, sizeof dst);
    dst.texture = t->t; dst.y = (Uint32)y0; dst.w = (Uint32)w; dst.h = (Uint32)(y1 - y0); dst.d = 1;
    SDL_UploadToGPUTexture(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_ReleaseGPUTransferBuffer(dev, tb);                 /* (freed once the GPU is done with it) */
}
static void tex_destroy(uintptr_t h) {
    Tex *t = (Tex *)h;
    if (!t) return;
    if (t->t) SDL_ReleaseGPUTexture(dev, t->t);
    free(t);
}
static const GeTexOps tex_ops = { tex_create, tex_upload, tex_destroy, flush };

/* ---------------------------------------------------------------- pipelines */
static const SDL_GPUBlendFactor sdl_factor[] = {
    SDL_GPU_BLENDFACTOR_ZERO, SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_SRC_COLOR, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
    SDL_GPU_BLENDFACTOR_DST_COLOR, SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR, SDL_GPU_BLENDFACTOR_SRC_ALPHA,
    SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_GPU_BLENDFACTOR_DST_ALPHA, SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
    SDL_GPU_BLENDFACTOR_CONSTANT_COLOR };
static const SDL_GPUBlendOp sdl_op[] = { SDL_GPU_BLENDOP_ADD, SDL_GPU_BLENDOP_SUBTRACT, SDL_GPU_BLENDOP_REVERSE_SUBTRACT,
                                         SDL_GPU_BLENDOP_MIN, SDL_GPU_BLENDOP_MAX };
/* key: blend 1 bit, op 3, src 4, dst 4, write mask 4 */
static uint32_t pipe_key(int blend, int op, int src, int dst, int wr, int wg, int wb, int wa) {
    if (!blend) op = src = dst = 0;
    return (uint32_t)blend | (uint32_t)op << 1 | (uint32_t)src << 4 | (uint32_t)dst << 8 |
           (uint32_t)(wr | wg << 1 | wb << 2 | wa << 3) << 12;
}
typedef struct { uint32_t key; SDL_GPUGraphicsPipeline *p; } Pipe;
static Pipe pipes[256];
static int npipes;
static SDL_GPUGraphicsPipeline *make_pipeline(SDL_GPUShader *vs, SDL_GPUShader *fs, SDL_GPUTextureFormat fmt, int ncolour_attrs,
                                              uint32_t pitch, const SDL_GPUColorTargetBlendState *bs) {
    SDL_GPUVertexBufferDescription vb = { 0, pitch, SDL_GPU_VERTEXINPUTRATE_VERTEX, 0 };
    SDL_GPUVertexAttribute at[3] = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0 },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 8 },
        { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 16 } };
    SDL_GPUColorTargetDescription ctd;
    memset(&ctd, 0, sizeof ctd);
    ctd.format = fmt; ctd.blend_state = *bs;
    SDL_GPUGraphicsPipelineCreateInfo ci;
    memset(&ci, 0, sizeof ci);
    ci.vertex_shader = vs; ci.fragment_shader = fs;
    ci.vertex_input_state.vertex_buffer_descriptions = &vb; ci.vertex_input_state.num_vertex_buffers = 1;
    ci.vertex_input_state.vertex_attributes = at; ci.vertex_input_state.num_vertex_attributes = (Uint32)(2 + ncolour_attrs);
    ci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    ci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL; ci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    ci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    ci.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    ci.target_info.color_target_descriptions = &ctd; ci.target_info.num_color_targets = 1;
    return SDL_CreateGPUGraphicsPipeline(dev, &ci);
}
static SDL_GPUGraphicsPipeline *pipeline(uint32_t key) {
    for (int i = 0; i < npipes; i++) if (pipes[i].key == key) return pipes[i].p;
    SDL_GPUColorTargetBlendState bs;
    memset(&bs, 0, sizeof bs);
    int blend = key & 1, op = (key >> 1) & 7, src = (key >> 4) & 15, dst = (key >> 8) & 15, m = (key >> 12) & 15;
    bs.enable_blend = blend != 0;
    bs.color_blend_op = sdl_op[op < 5 ? op : 0];
    bs.src_color_blendfactor = sdl_factor[src <= GE_BF_CONSTANT ? src : 1];
    bs.dst_color_blendfactor = sdl_factor[dst <= GE_BF_CONSTANT ? dst : 1];
    bs.alpha_blend_op = SDL_GPU_BLENDOP_ADD;                       /* frame-buffer alpha is not blended */
    bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO; bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    bs.enable_color_write_mask = true;
    bs.color_write_mask = (SDL_GPUColorComponentFlags)((m & 1 ? SDL_GPU_COLORCOMPONENT_R : 0) | (m & 2 ? SDL_GPU_COLORCOMPONENT_G : 0) |
                                                      (m & 4 ? SDL_GPU_COLORCOMPONENT_B : 0) | (m & 8 ? SDL_GPU_COLORCOMPONENT_A : 0));
    SDL_GPUGraphicsPipeline *p = make_pipeline(draw_vs, draw_fs, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, 1, sizeof(GVtx), &bs);
    if (!p) { rt_warn("SDL GPU: pipeline %X: %s", key, SDL_GetError()); return NULL; }
    if (npipes < 256) { pipes[npipes].key = key; pipes[npipes].p = p; npipes++; }
    return p;
}

/* ---------------------------------------------------------------- batches */
typedef struct {                        /* everything a batch shares (zeroed before filling, so memcmp works) */
    Target *t;
    int sx, sy, sw, sh;                 /* scissor, in render-target pixels */
    SDL_GPUTexture *tex; int sampler;
    uint32_t pipe; uint32_t blend_const; int const_on;
    FragUniforms fu;
} Key;
typedef struct { Key k; int first, count, draws; } Batch;
#define VMAX (65536 * 3)
#define BMAX 8192
static GVtx verts[VMAX];
static int nverts;
static Batch batches[BMAX];
static int nbatches;

/* records the batches gathered so far: their vertices into the vertex buffer, then a render pass for
   each run of batches into the same target */
static void flush(void) {
    if (!nbatches) return;
    uint32_t bytes = (uint32_t)nverts * sizeof(GVtx);
    void *p = SDL_MapGPUTransferBuffer(dev, vtb, true);
    if (p) { memcpy(p, verts, bytes); SDL_UnmapGPUTransferBuffer(dev, vtb); }
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb());
    SDL_GPUTransferBufferLocation src = { vtb, 0 };
    SDL_GPUBufferRegion dst = { vbuf, 0, bytes };
    SDL_UploadToGPUBuffer(cp, &src, &dst, true);
    SDL_EndGPUCopyPass(cp);
    VertUniforms vu = { { (float)LW, (float)LH }, { 0, 0 } };
    for (int i = 0; i < nbatches;) {
        Target *t = batches[i].k.t;
        SDL_GPUColorTargetInfo ct;
        memset(&ct, 0, sizeof ct);
        ct.texture = t->tex; ct.load_op = SDL_GPU_LOADOP_LOAD; ct.store_op = SDL_GPU_STOREOP_STORE;
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof vu);
        SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cmd, &ct, 1, NULL);
        if (!rp) { i++; continue; }
        SDL_GPUViewport vp = { 0, 0, (float)(LW * S), (float)(LH * S), 0, 1 };
        SDL_SetGPUViewport(rp, &vp);
        SDL_GPUBufferBinding bb = { vbuf, 0 };
        SDL_BindGPUVertexBuffers(rp, 0, &bb, 1);
        SDL_GPUGraphicsPipeline *bound = NULL;
        for (; i < nbatches && batches[i].k.t == t; i++) {
            const Batch *b = &batches[i];
            SDL_GPUGraphicsPipeline *pl = pipeline(b->k.pipe);
            if (!pl) continue;
            if (pl != bound) { SDL_BindGPUGraphicsPipeline(rp, pl); bound = pl; }
            SDL_Rect sc = { b->k.sx, b->k.sy, b->k.sw, b->k.sh };
            SDL_SetGPUScissor(rp, &sc);
            if (b->k.const_on) {
                uint32_t k = b->k.blend_const;
                SDL_FColor c = { (float)(k & 255) / 255.f, (float)((k >> 8) & 255) / 255.f, (float)((k >> 16) & 255) / 255.f, 1.f };
                SDL_SetGPUBlendConstants(rp, c);
            }
            SDL_GPUTextureSamplerBinding ts = { b->k.tex ? b->k.tex : dummy_tex, samplers[b->k.sampler] };
            SDL_BindGPUFragmentSamplers(rp, 0, &ts, 1);
            SDL_PushGPUFragmentUniformData(cmd, 0, &b->k.fu, sizeof b->k.fu);
            SDL_DrawGPUPrimitives(rp, (Uint32)b->count, 1, (Uint32)b->first, 0);
            frame_draws += b->draws; frame_calls++;
        }
        SDL_EndGPURenderPass(rp);
    }
    nverts = 0; nbatches = 0;
}

static void gpu_draw(const GeDrawState *s, const GeVtx *v, int m) {
    if (!ready) return;
    Target *t = target_for(s->target, 1);
    if (!t) return;
    if (!m) return;
    if (nverts + m > VMAX || nbatches >= BMAX) flush();
    if (m > VMAX) return;
    GVtx *vt = &verts[nverts];
    memcpy(vt, v, (size_t)m * sizeof(GVtx));               /* (GeVtx and GVtx: the same layout) */

    Key k;
    memset(&k, 0, sizeof k);
    k.t = t;
    int x = s->sx1 * S, y = s->sy1 * S, w = (s->sx2 - s->sx1 + 1) * S, h = (s->sy2 - s->sy1 + 1) * S;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LW * S) w = LW * S - x;
    if (y + h > LH * S) h = LH * S - y;
    if (w <= 0 || h <= 0) return;
    k.sx = x; k.sy = y; k.sw = w; k.sh = h;
    k.fu.clear_on = s->clear;
    k.fu.tex_on = s->tex_on;
    int linear = 0, clamp_s = 1, clamp_t = 1;
    if (s->tex_on) {
        int ox = 0, oy = 0;
        Target *src = target_containing(s->tex.addr, &ox, &oy);
        linear = s->linear;
        if (src && src != t) {
            /* render-to-texture: sample the upscaled target; texel (u,v) is target pixel (ox+u, oy+v) */
            k.tex = src->tex;
            for (int i = 0; i < m; i++) { vt[i].u += (float)ox; vt[i].v += (float)oy; }
            k.fu.texsize[0] = (float)LW; k.fu.texsize[1] = (float)LH;
        } else {
            float vmin = vt[0].v, vmax = vt[0].v;
            for (int i = 1; i < m; i++) { if (vt[i].v < vmin) vmin = vt[i].v; if (vt[i].v > vmax) vmax = vt[i].v; }
            int th = s->tex.h, v0 = 0, v1 = th;
            if (vmin >= 0.f && vmax <= (float)th) { v0 = (int)vmin - 1; v1 = (int)vmax + 2; if (v0 < 0) v0 = 0; if (v1 > th) v1 = th; }
            uint64_t t0 = perf_now_us();
            GeTexEntry *e = ge_texcache_lookup(&s->tex, v0, v1);   /* (flushes first if it uploads) */
            perf_add(PERF_TEXTURE, perf_now_us() - t0);
            if (!e || !e->tex || !((Tex *)e->tex)->t) return;
            k.tex = ((Tex *)e->tex)->t;
            clamp_s = s->clamp_s; clamp_t = s->clamp_t;
            k.fu.texsize[0] = (float)e->w; k.fu.texsize[1] = (float)e->h;
        }
        k.fu.texfunc = s->texfunc; k.fu.rgba = s->tex_alpha; k.fu.dbl = s->doubled;
        uint32_t env = s->env;
        k.fu.env[0] = (float)(env & 255) / 255.f; k.fu.env[1] = (float)((env >> 8) & 255) / 255.f;
        k.fu.env[2] = (float)((env >> 16) & 255) / 255.f; k.fu.env[3] = 1.f;
    }
    k.sampler = linear << 2 | clamp_s << 1 | clamp_t;
    k.fu.atest = s->atest;
    if (s->atest) { k.fu.afunc = s->afunc; k.fu.aref = (float)s->aref; k.fu.amask = (float)s->amask; }
    int blend = !s->clear && s->blend;
    k.pipe = pipe_key(blend, s->blend_eq, s->blend_src, s->blend_dst, s->write_r, s->write_g, s->write_b, s->write_a);
    if (blend && (s->blend_src == GE_BF_CONSTANT || s->blend_dst == GE_BF_CONSTANT)) { k.const_on = 1; k.blend_const = s->blend_const; }

    /* a texture lookup may have flushed (the batch list emptied): this draw's vertices to where it now ends */
    if (vt != &verts[nverts]) memmove(&verts[nverts], vt, (size_t)m * sizeof(GVtx));
    vt = &verts[nverts];
    Batch *last = nbatches ? &batches[nbatches - 1] : NULL;
    if (last && last->first + last->count == nverts && !memcmp(&last->k, &k, sizeof k)) { last->count += m; last->draws++; }
    else { Batch *b = &batches[nbatches++]; b->k = k; b->first = nverts; b->count = m; b->draws = 1; }
    nverts += m;
    t->used = 1;
}

/* ---------------------------------------------------------------- presentation */
static SDL_GPUTexture *upload_tex;
static int upload_w, upload_h;
static int vsync_on = 1;

static void draw_to_window(SDL_GPUTexture *tex, float src_w, float src_h, float uv_w, float uv_h, int sharp) {
    SDL_GPUTexture *sw = NULL; Uint32 ww = 0, wh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cb(), win, &sw, &ww, &wh) || !sw) { submit(); return; }   /* (minimised) */
    SDL_GPUColorTargetInfo ct;
    memset(&ct, 0, sizeof ct);
    ct.texture = sw; ct.load_op = SDL_GPU_LOADOP_CLEAR; ct.store_op = SDL_GPU_STOREOP_STORE;
    ct.clear_color.a = 1.f;
    /* letterbox to the PSP's 480:272 */
    float sx = (float)ww / 480.f, sy = (float)wh / 272.f, sc = sx < sy ? sx : sy;
    float w = 480.f * sc, h = 272.f * sc, x0 = ((float)ww - w) / 2.f, y0 = ((float)wh - h) / 2.f;
    PresentUniforms pu = { { src_w, src_h }, { w, h }, { uv_w, uv_h }, sharp, 0 };
    SDL_PushGPUFragmentUniformData(cmd, 0, &pu, sizeof pu);
    SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cmd, &ct, 1, NULL);
    if (rp && present_pipe && w >= 1.f && h >= 1.f) {
        SDL_GPUViewport vp = { x0, y0, w, h, 0, 1 };
        SDL_SetGPUViewport(rp, &vp);
        SDL_BindGPUGraphicsPipeline(rp, present_pipe);
        SDL_GPUBufferBinding bb = { quad_buf, 0 };
        SDL_BindGPUVertexBuffers(rp, 0, &bb, 1);
        SDL_GPUTextureSamplerBinding ts = { tex, samplers[1 << 2 | 3] };   /* linear, clamped */
        SDL_BindGPUFragmentSamplers(rp, 0, &ts, 1);
        SDL_DrawGPUPrimitives(rp, 6, 1, 0, 0);
    }
    if (rp) SDL_EndGPURenderPass(rp);
    submit();
}

static int gpu_present(uint32_t fb_addr, int win_w, int win_h) {
    (void)win_w; (void)win_h;
    if (!ready) return 0;
    flush();
    Target *t = target_for(fb_addr, 0);
    if (!t || !t->used) return 0;
    draw_to_window(t->tex, 480.f * S, 272.f * S, 480.f / LW, 272.f / LH, 1);
    return 1;
}

static void gpu_present_rgba(const uint8_t *rgba, int w, int h, int win_w, int win_h) {
    (void)win_w; (void)win_h;
    if (!ready) return;
    flush();
    if (!upload_tex || upload_w != w || upload_h != h) {
        if (upload_tex) SDL_ReleaseGPUTexture(dev, upload_tex);
        upload_tex = make_texture(w, h, SDL_GPU_TEXTUREUSAGE_SAMPLER); upload_w = w; upload_h = h;
        if (!upload_tex) return;
    }
    Tex tmp = { upload_tex, w, h };
    tex_upload((uintptr_t)&tmp, rgba, w, h, 0, h, 0);
    draw_to_window(upload_tex, (float)w, (float)h, 1.f, 1.f, 1);
}

static int gpu_read(uint32_t fb_addr, uint8_t *rgba, int *w, int *h) {
    if (!ready) return 0;
    flush();
    Target *t = target_for(fb_addr, 0);
    if (!t || !t->used) return 0;
    *w = 480 * S; *h = 272 * S;
    uint32_t bytes = (uint32_t)*w * (uint32_t)*h * 4u;
    if (!dltb || dltb_size < bytes) {
        if (dltb) SDL_ReleaseGPUTransferBuffer(dev, dltb);
        SDL_GPUTransferBufferCreateInfo ti = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, bytes, 0 };
        dltb = SDL_CreateGPUTransferBuffer(dev, &ti); dltb_size = bytes;
        if (!dltb) return 0;
    }
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb());
    SDL_GPUTextureRegion src;
    memset(&src, 0, sizeof src);
    src.texture = t->tex; src.w = (Uint32)*w; src.h = (Uint32)*h; src.d = 1;
    SDL_GPUTextureTransferInfo dst = { dltb, 0, (Uint32)*w, (Uint32)*h };
    SDL_DownloadFromGPUTexture(cp, &src, &dst);
    SDL_EndGPUCopyPass(cp);
    SDL_GPUFence *f = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    cmd = NULL;
    if (!f) return 0;
    SDL_WaitForGPUFences(dev, true, &f, 1);
    SDL_ReleaseGPUFence(dev, f);
    const uint8_t *p = (const uint8_t *)SDL_MapGPUTransferBuffer(dev, dltb, false);
    if (!p) return 0;
    memcpy(rgba, p, bytes);
    SDL_UnmapGPUTransferBuffer(dev, dltb);
    for (int i = 0; i < *w * *h; i++) rgba[i * 4 + 3] = 255;
    return 1;
}

static void gpu_frame_end(void) {
    flush();
    submit();                                            /* (a frame not shown: held, or headless) */
    perf_count_draws(frame_draws, frame_calls);
    frame_draws = frame_calls = 0;
    frame_no++;
    ge_texcache_frame_end();
}
static void gpu_finish(void) { if (!ready) return; flush(); submit(); SDL_WaitForGPUIdle(dev); }
static void gpu_list_start(void) { ge_texcache_memory_changed(); }
static void gpu_memory_written(void) { ge_texcache_memory_changed(); }
static int gpu_scale(void) { return S; }
static const char *gpu_name(void) { return gpu_name_buf; }
static const char *gpu_scale_reason(void) { return scale_reason; }

/* VSYNC (host_sdl.c: SCREEN SETTING > VSYNC): the swapchain's present mode */
static void gpu_vsync(int on) {
    vsync_on = on;
    if (!dev || !win) return;
    SDL_GPUPresentMode m = SDL_GPU_PRESENTMODE_VSYNC;
    if (!on) m = SDL_WindowSupportsGPUPresentMode(dev, win, SDL_GPU_PRESENTMODE_IMMEDIATE) ? SDL_GPU_PRESENTMODE_IMMEDIATE :
                 SDL_WindowSupportsGPUPresentMode(dev, win, SDL_GPU_PRESENTMODE_MAILBOX) ? SDL_GPU_PRESENTMODE_MAILBOX : SDL_GPU_PRESENTMODE_VSYNC;
    SDL_SetGPUSwapchainParameters(dev, win, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, m);
    rt_log("SDL GPU: vsync %s", on ? "on" : "off");
}

/* ---------------------------------------------------------------- init */
#define AUTO_SCALE_MAX 4
static SDL_GPUShader *shader(const unsigned char *code, size_t n, const char *msl, SDL_GPUShaderFormat fmt, SDL_GPUShaderStage stage,
                             int samplers_n, int uniforms_n, const char *entry) {
    SDL_GPUShaderCreateInfo ci;
    memset(&ci, 0, sizeof ci);
    if (fmt == SDL_GPU_SHADERFORMAT_MSL) { ci.code = (const Uint8 *)msl; ci.code_size = strlen(msl); }
    else { ci.code = code; ci.code_size = n; }
    ci.entrypoint = entry; ci.format = fmt; ci.stage = stage;
    ci.num_samplers = (Uint32)samplers_n; ci.num_uniform_buffers = (Uint32)uniforms_n;
    SDL_GPUShader *s = SDL_CreateGPUShader(dev, &ci);
    if (!s) rt_warn("SDL GPU: shader %s: %s", entry, SDL_GetError());
    return s;
}
extern void (*host_gpu_vsync)(int on);
void shader_set_unavailable(const char *why);
static int gpu_init(const GeBackendInit *in) {
    win = (SDL_Window *)in->window;
    if (!win) return 0;
    SDL_GPUShaderFormat want = SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | (GPU_SHADERS_SPIRV ? SDL_GPU_SHADERFORMAT_SPIRV : 0);
    const char *drv = in->gpu_driver;
    if (drv && !strcmp(drv, "vulkan") && !GPU_SHADERS_SPIRV) { rt_warn("SDL GPU: this build has no Vulkan shaders (tools/gpu_shaders.py without a SPIR-V compiler)"); return 0; }
    dev = SDL_CreateGPUDevice(want, false, drv);
    if (!dev) { rt_warn("SDL GPU: no %s device: %s", drv ? drv : "GPU", SDL_GetError()); return 0; }
    if (!SDL_ClaimWindowForGPUDevice(dev, win)) { rt_warn("SDL GPU: the window: %s", SDL_GetError()); SDL_DestroyGPUDevice(dev); dev = NULL; return 0; }
    SDL_GPUShaderFormat have = SDL_GetGPUShaderFormats(dev);
    SDL_GPUShaderFormat f = (have & SDL_GPU_SHADERFORMAT_DXIL) ? SDL_GPU_SHADERFORMAT_DXIL : (have & SDL_GPU_SHADERFORMAT_SPIRV) && GPU_SHADERS_SPIRV ?
                            SDL_GPU_SHADERFORMAT_SPIRV : (have & SDL_GPU_SHADERFORMAT_MSL) ? SDL_GPU_SHADERFORMAT_MSL : SDL_GPU_SHADERFORMAT_INVALID;
    if (f == SDL_GPU_SHADERFORMAT_INVALID) { rt_warn("SDL GPU: no shader format this build has (device takes %X)", (unsigned)have); goto fail; }
#define BLOB(n) (f == SDL_GPU_SHADERFORMAT_DXIL ? n##_dxil : n##_spirv), (f == SDL_GPU_SHADERFORMAT_DXIL ? sizeof n##_dxil : sizeof n##_spirv)
    draw_vs = shader(BLOB(ge_draw_vs), ge_draw_msl, f, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, "vs_main");
    draw_fs = shader(BLOB(ge_draw_fs), ge_draw_msl, f, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, "fs_main");
    present_vs = shader(BLOB(ge_present_vs), ge_present_msl, f, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, "vs_main");
    present_fs = shader(BLOB(ge_present_fs), ge_present_msl, f, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, "fs_main");
#undef BLOB
    if (!draw_vs || !draw_fs || !present_vs || !present_fs) goto fail;
    swap_fmt = SDL_GetGPUSwapchainTextureFormat(dev, win);
    {
        SDL_GPUColorTargetBlendState bs;
        memset(&bs, 0, sizeof bs);
        present_pipe = make_pipeline(present_vs, present_fs, swap_fmt, 0, 16, &bs);
        if (!present_pipe) { rt_warn("SDL GPU: the present pipeline: %s", SDL_GetError()); goto fail; }
    }
    for (int i = 0; i < 8; i++) {
        SDL_GPUSamplerCreateInfo si;
        memset(&si, 0, sizeof si);
        si.min_filter = si.mag_filter = (i & 4) ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        si.address_mode_u = (i & 2) ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        si.address_mode_v = (i & 1) ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplers[i] = SDL_CreateGPUSampler(dev, &si);
        if (!samplers[i]) goto fail;
    }
    {
        SDL_GPUBufferCreateInfo bi = { SDL_GPU_BUFFERUSAGE_VERTEX, (Uint32)sizeof verts, 0 };
        vbuf = SDL_CreateGPUBuffer(dev, &bi);
        SDL_GPUTransferBufferCreateInfo ti = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, (Uint32)sizeof verts, 0 };
        vtb = SDL_CreateGPUTransferBuffer(dev, &ti);
        static const float quad[6][4] = { { -1, 1, 0, 0 }, { 1, 1, 1, 0 }, { -1, -1, 0, 1 }, { 1, 1, 1, 0 }, { 1, -1, 1, 1 }, { -1, -1, 0, 1 } };
        SDL_GPUBufferCreateInfo qi = { SDL_GPU_BUFFERUSAGE_VERTEX, (Uint32)sizeof quad, 0 };
        quad_buf = SDL_CreateGPUBuffer(dev, &qi);
        dummy_tex = make_texture(1, 1, SDL_GPU_TEXTUREUSAGE_SAMPLER);
        if (!vbuf || !vtb || !quad_buf || !dummy_tex) { rt_warn("SDL GPU: buffers: %s", SDL_GetError()); goto fail; }
        SDL_GPUTransferBufferCreateInfo qt = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, (Uint32)sizeof quad + 4, 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(dev, &qt);
        uint8_t *p = tb ? (uint8_t *)SDL_MapGPUTransferBuffer(dev, tb, false) : NULL;
        if (!p) goto fail;
        memcpy(p, quad, sizeof quad); memset(p + sizeof quad, 0, 4);
        SDL_UnmapGPUTransferBuffer(dev, tb);
        SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb());
        SDL_GPUTransferBufferLocation src = { tb, 0 };
        SDL_GPUBufferRegion dst = { quad_buf, 0, (Uint32)sizeof quad };
        SDL_UploadToGPUBuffer(cp, &src, &dst, false);
        SDL_GPUTransferBufferLocation src2 = { tb, (Uint32)sizeof quad };
        SDL_GPUTextureTransferInfo tsrc = { tb, (Uint32)sizeof quad, 1, 1 };
        SDL_GPUTextureRegion tdst; memset(&tdst, 0, sizeof tdst); tdst.texture = dummy_tex; tdst.w = 1; tdst.h = 1; tdst.d = 1;
        (void)src2;
        SDL_UploadToGPUTexture(cp, &tsrc, &tdst, false);
        SDL_EndGPUCopyPass(cp);
        submit();
        SDL_ReleaseGPUTransferBuffer(dev, tb);
    }
    if (in->scale > 0) { S = in->scale > 8 ? 8 : in->scale; scale_reason = "--scale"; }
    else {
        S = in->display_h > 0 ? (in->display_h + 271) / 272 : AUTO_SCALE_MAX;
        if (S > AUTO_SCALE_MAX) S = AUTO_SCALE_MAX;
        if (S < 1) S = 1;
        scale_reason = in->display_h > 0 ? "auto" : "auto, monitor unknown";
    }
    {
        SDL_PropertiesID pr = SDL_GetGPUDeviceProperties(dev);
        const char *nm = SDL_GetStringProperty(pr, SDL_PROP_GPU_DEVICE_NAME_STRING, "");
        const char *dn = SDL_GetStringProperty(pr, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "");
        const char *dv = SDL_GetStringProperty(pr, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, "");
        snprintf(gpu_name_buf, sizeof gpu_name_buf, "%s%s%s%s%s", nm, *dn ? " / " : "", dn, *dv ? " " : "", dv);
    }
    ge_texcache_init(&tex_ops);
    host_gpu_vsync = gpu_vsync;
    { extern int rt_vsync; gpu_vsync(rt_vsync); }
    shader_set_unavailable("OPENGL ONLY");
    ready = 1;
    rt_log("SDL GPU: %s (%s), %s shaders, internal resolution %dx%d (x%d, %s)", SDL_GetGPUDeviceDriver(dev), gpu_name_buf,
           f == SDL_GPU_SHADERFORMAT_DXIL ? "DXIL" : f == SDL_GPU_SHADERFORMAT_SPIRV ? "SPIR-V" : "MSL", 480 * S, 272 * S, S, scale_reason);
    rt_log("SDL GPU: FILTER needs the OpenGL renderer: the picture is shown plain");
    return 1;
fail:
    SDL_ReleaseWindowFromGPUDevice(dev, win);
    SDL_DestroyGPUDevice(dev); dev = NULL;
    return 0;
}

const GeBackend ge_backend_sdlgpu = {
    "SDL GPU",
    gpu_init,
    gpu_draw, gpu_list_start, gpu_memory_written,
    gpu_present, gpu_present_rgba, gpu_frame_end, gpu_finish, gpu_read,
    gpu_scale, gpu_name, gpu_scale_reason,
};
const char *ge_sdlgpu_api(void) { return dev ? SDL_GetGPUDeviceDriver(dev) : ""; }

#else   /* no SDL (headless test builds): never available */
static int none_init(const GeBackendInit *in) { (void)in; return 0; }
static void none_draw(const GeDrawState *s, const GeVtx *v, int n) { (void)s; (void)v; (void)n; }
static void none_void(void) {}
static int none_present(uint32_t a, int w, int h) { (void)a; (void)w; (void)h; return 0; }
static void none_rgba(const uint8_t *p, int w, int h, int ww, int wh) { (void)p; (void)w; (void)h; (void)ww; (void)wh; }
static int none_read(uint32_t a, uint8_t *p, int *w, int *h) { (void)a; (void)p; (void)w; (void)h; return 0; }
static int none_scale(void) { return 1; }
static const char *none_str(void) { return ""; }
const GeBackend ge_backend_sdlgpu = { "SDL GPU", none_init, none_draw, none_void, none_void, none_present, none_rgba,
                                      none_void, none_void, none_read, none_scale, none_str, none_str };
const char *ge_sdlgpu_api(void) { return ""; }
#endif
