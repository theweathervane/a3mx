/* GPU renderer for the PSP GE (OpenGL 3.3 core), drawing at S x the PSP's resolution.
 *
 * Every frame-buffer address the game draws to gets an off-screen render target (FBO) of
 * 512*S x 272*S. When the game later uses one of those addresses as a texture (it composes each frame
 * at 0x04088000 and then stretches that onto the visible buffer), the upscaled FBO is sampled directly,
 * so detail survives the whole chain. Textures from guest memory are decoded once to RGBA8 and cached
 * by a hash of their bytes and palette.
 *
 * Coordinates: guest pixels with y down. FBO row r holds guest row r (GL's bottom-up origin is used as
 * guest row 0), so readback and texture sampling need no flips; only the window presentation flips.
 */
#include "rt.h"
#include "ge.h"
#include "ge_backend.h"
#include "ge_texcache.h"
#include "gl_funcs.h"
#include "shader.h"
#include "perf.h"
#include <string.h>
#include <stdlib.h>

#define GL_DEFINE(ret, name, args) PFN_##name p_##name;
GL_FUNCS(GL_DEFINE)
GL_FUNCS_OPTIONAL(GL_DEFINE)
#undef GL_DEFINE

int gl_load(void *(*getproc)(const char *)) {
    int ok = 1;
#define GL_LOAD(ret, name, args) p_##name = (PFN_##name)getproc(#name); if (!p_##name) { rt_warn("GL: missing %s", #name); ok = 0; }
    GL_FUNCS(GL_LOAD)
#undef GL_LOAD
#define GL_LOAD_OPTIONAL(ret, name, args) p_##name = (PFN_##name)getproc(#name);
    GL_FUNCS_OPTIONAL(GL_LOAD_OPTIONAL)
#undef GL_LOAD_OPTIONAL
    return ok;
}

#define LW 512          /* logical render-target size in guest pixels */
#define LH 272

static int S = 4;
static int ready;
static GLuint prog, vao, vbo, prog_present;
static struct {
    GLint fbsize, tex_on, texfunc, rgba, dbl, env, atest, aref, amask, afunc, clear, texsize, sampler;
} U;
static struct { GLint tex, src_size, out_size, sharp, uv_max; } UP;

/* ---------------------------------------------------------------- shaders */
static const char *vs_src =
    "#version 330 core\n"
    "layout(location=0) in vec2 a_pos; layout(location=1) in vec2 a_uv; layout(location=2) in vec4 a_col;\n"
    "uniform vec2 u_fbsize;\n"
    "out vec2 v_uv; out vec4 v_col; flat out vec4 v_fcol;\n"
    "void main() {\n"
    "  gl_Position = vec4(a_pos.x / u_fbsize.x * 2.0 - 1.0, a_pos.y / u_fbsize.y * 2.0 - 1.0, 0.0, 1.0);\n"
    "  v_uv = a_uv; v_col = a_col; v_fcol = a_col;\n"
    "}\n";

static const char *fs_src =
    "#version 330 core\n"
    "in vec2 v_uv; in vec4 v_col; flat in vec4 v_fcol;\n"
    "uniform sampler2D u_tex; uniform vec2 u_texsize;\n"
    "uniform int u_tex_on, u_texfunc, u_rgba, u_dbl, u_atest, u_afunc, u_clear;\n"
    "uniform float u_aref, u_amask; uniform vec4 u_env;\n"
    "out vec4 o;\n"
    "void main() {\n"
    "  vec4 c = v_col;\n"
    "  if (u_clear != 0) { o = c; return; }\n"
    "  if (u_tex_on != 0) {\n"
    "    vec4 t = texture(u_tex, v_uv / u_texsize);\n"
    "    vec4 r;\n"
    "    if (u_texfunc == 0) { r.rgb = t.rgb * c.rgb; r.a = u_rgba != 0 ? t.a * c.a : c.a; }\n"
    "    else if (u_texfunc == 1) { r.rgb = mix(c.rgb, t.rgb, t.a); r.a = c.a; }\n"
    "    else if (u_texfunc == 2) { r.rgb = mix(c.rgb, u_env.rgb, t.rgb); r.a = u_rgba != 0 ? t.a * c.a : c.a; }\n"
    "    else if (u_texfunc == 3) { r.rgb = t.rgb; r.a = u_rgba != 0 ? t.a : c.a; }\n"
    "    else { r.rgb = t.rgb + c.rgb; r.a = u_rgba != 0 ? t.a * c.a : c.a; }\n"
    "    if (u_dbl != 0 && u_texfunc != 3) r.rgb *= 2.0;\n"
    "    c = clamp(r, 0.0, 1.0);\n"
    "  }\n"
    "  if (u_atest != 0) {\n"
    "    float a = floor(c.a * 255.0 + 0.5);\n"
    "    float x = float(int(a) & int(u_amask)), ref = float(int(u_aref) & int(u_amask));\n"
    "    bool pass = u_afunc == 1 || (u_afunc == 2 && x == ref) || (u_afunc == 3 && x != ref) || (u_afunc == 4 && x < ref) ||\n"
    "                (u_afunc == 5 && x <= ref) || (u_afunc == 6 && x > ref) || (u_afunc == 7 && x >= ref);\n"
    "    if (!pass) discard;\n"
    "  }\n"
    "  o = c;\n"
    "}\n";

/* Presentation: draws a frame into the window. "sharp" mode does sharp-bilinear (integer pre-scale
 * then linear), which keeps pixels crisp at any window size without uneven pixel widths. */
static const char *vs_present =
    "#version 330 core\n"
    "layout(location=0) in vec2 a_pos; layout(location=1) in vec2 a_uv;\n"
    "out vec2 v_uv;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }\n";
static const char *fs_present =
    "#version 330 core\n"
    "in vec2 v_uv; uniform sampler2D u_tex; uniform vec2 u_src_size, u_out_size, u_uv_max; uniform int u_sharp;\n"
    "out vec4 o;\n"
    "void main() {\n"
    "  vec2 uv = v_uv;\n"
    "  if (u_sharp != 0) {\n"
    "    vec2 texel = uv * u_src_size;\n"
    "    vec2 scale = max(floor(u_out_size / u_src_size), vec2(1.0));\n"
    "    vec2 base = floor(texel), f = fract(texel) - 0.5;\n"
    "    vec2 range = 0.5 - 0.5 / scale;\n"
    "    f = (f - clamp(f, -range, range)) * scale + 0.5;\n"
    "    uv = (base + f) / u_src_size;\n"
    "  }\n"
    "  o = vec4(texture(u_tex, uv * u_uv_max).rgb, 1.0);\n"
    "}\n";

static GLuint compile(GLenum type, const char *src) {
    GLuint s = p_glCreateShader(type);
    p_glShaderSource(s, 1, &src, NULL);
    p_glCompileShader(s);
    GLint ok = 0; p_glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]; p_glGetShaderInfoLog(s, sizeof log, NULL, log); rt_logl(RT_LOG_ERROR, "GL shader error: %s", log); return 0; }
    return s;
}
static GLuint link(const char *vs, const char *fs) {
    GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return 0;
    GLuint p = p_glCreateProgram();
    p_glAttachShader(p, v); p_glAttachShader(p, f);
    p_glLinkProgram(p);
    GLint ok = 0; p_glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; p_glGetProgramInfoLog(p, sizeof log, NULL, log); rt_logl(RT_LOG_ERROR, "GL link error: %s", log); return 0; }
    return p;
}

/* ---------------------------------------------------------------- render targets */
typedef struct { uint32_t addr; GLuint fbo, tex; int used; } Target;
static Target targets[16];
static int ntargets;
static Target *cur_target;

static Target *target_for(uint32_t addr, int create) {
    addr &= 0x0FFFFFFF;
    for (int i = 0; i < ntargets; i++) if (targets[i].addr == addr) return &targets[i];
    if (!create || ntargets >= 16) return NULL;
    Target *t = &targets[ntargets++];
    t->addr = addr;
    p_glGenTextures(1, &t->tex);
    p_glBindTexture(GL_TEXTURE_2D, t->tex);
    p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, LW * S, LH * S, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    p_glGenFramebuffers(1, &t->fbo);
    p_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->tex, 0);
    if (p_glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) rt_warn("GL: render target %08X incomplete", addr);
    p_glDisable(GL_SCISSOR_TEST);
    p_glColorMask(1, 1, 1, 1);
    p_glClearColor(0, 0, 0, 0);
    p_glClear(GL_COLOR_BUFFER_BIT);
    rt_log("GL: render target %08X (%dx%d)", addr, LW * S, LH * S);
    return t;
}

/* Is guest address a inside a render target? Returns it and the texel offset. */
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

/* ---------------------------------------------------------------- texture cache (ge_texcache.c) */
static uint64_t frame_no;
static void gpu_timer_begin(void), gpu_timer_end(void);   /* the frame's GPU timer (below) */
static void flush_batch(void);                             /* draws the open batch (below) */
static uintptr_t gl_tex_create(void) { GLuint t = 0; p_glGenTextures(1, &t); return t; }
static void gl_tex_upload(uintptr_t t, const uint8_t *rgba, int w, int h, int y0, int y1, int whole) {
    p_glBindTexture(GL_TEXTURE_2D, (GLuint)t);
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (whole) {
        p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    } else {
        p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y0, w, y1 - y0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
}
static void gl_tex_destroy(uintptr_t t) { GLuint n = (GLuint)t; p_glDeleteTextures(1, &n); }
static const GeTexOps gl_tex_ops = { gl_tex_create, gl_tex_upload, gl_tex_destroy, flush_batch };

static void ge_gl_list_start(void) { ge_texcache_memory_changed(); }
static void ge_gl_memory_written(void) { ge_texcache_memory_changed(); }

static GeTexEntry *texture_from_memory(const GeTexRef *tx, int v0, int v1) {   /* timed as the frame's texture part (perf.h) */
    uint64_t t0 = perf_now_us();
    GeTexEntry *e = ge_texcache_lookup(tx, v0, v1);
    perf_add(PERF_TEXTURE, perf_now_us() - t0);
    return e;
}

/* ---------------------------------------------------------------- blending (GeDrawState's, ge_state.c) */
static GLenum gl_factor(int f) {
    static const GLenum gl[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
                                 GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR };
    return f >= 0 && f <= GE_BF_CONSTANT ? gl[f] : GL_ONE;
}

/* ---------------------------------------------------------------- drawing
 *
 * Draws are batched: each one's triangles are added to the batch while everything OpenGL needs for
 * them (render target, scissor, texture and its sampling, the shader's settings, blending, colour
 * mask) is the same as the batch's, and the batch is drawn with one call when that changes. Triangles
 * in one call are drawn in order, so the picture is the same as drawing each one alone. A batch is
 * also drawn before anything that would change what it reads or writes: a texture upload or delete,
 * presenting, reading a frame back, and the frame's end. With only one batch open, a draw into a
 * target always follows every earlier draw into any target, so render-to-texture sees them all. */
typedef struct { float x, y, u, v; uint8_t c[4]; } GlVtx;
#define VMAX (65536 * 3)
static GlVtx vtmp[VMAX];                /* one draw's triangles, while it is set up */
static GlVtx vbuf[VMAX];                /* the open batch */
static int vbn;                         /* vertices in it */

typedef struct {                        /* everything a batch shares (zeroed before filling, so memcmp works) */
    Target *t;
    int sx, sy, sw, sh;                 /* scissor, in render-target pixels */
    int clear, tex_on;
    GLuint tex; int linear, wrap_s, wrap_t; float tw, th;   /* wrap -1: a render target's (left as made) */
    int texfunc, rgba, dbl; uint32_t env;
    int atest, afunc, aref, amask;
    int blend, blend_eq, blend_src, blend_dst; uint32_t blend_const;
    int write_r, write_g, write_b, write_a;
} DrawState;
static DrawState batch;
static int batch_draws;                 /* the game's draws in the open batch */
static int frame_draws, frame_calls;

static void apply_state(const DrawState *d) {
    if (d->clear || !d->blend) p_glDisable(GL_BLEND);
    else {
        static const GLenum eqs[5] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX };
        p_glEnable(GL_BLEND);
        p_glBlendEquation(eqs[d->blend_eq >= 0 && d->blend_eq < 5 ? d->blend_eq : 0]);
        p_glBlendFuncSeparate(gl_factor(d->blend_src), gl_factor(d->blend_dst), GL_ZERO, GL_ONE);   /* frame-buffer alpha is not blended */
        if (d->blend_src == GE_BF_CONSTANT || d->blend_dst == GE_BF_CONSTANT) {
            uint32_t k = d->blend_const;
            p_glBlendColor((float)(k & 255) / 255.f, (float)((k >> 8) & 255) / 255.f, (float)((k >> 16) & 255) / 255.f, 1.f);
        }
    }
    p_glColorMask((GLboolean)d->write_r, (GLboolean)d->write_g, (GLboolean)d->write_b, (GLboolean)d->write_a);
}

static void flush_batch(void) {
    if (!vbn) return;
    const DrawState *d = &batch;
    p_glBindFramebuffer(GL_FRAMEBUFFER, d->t->fbo);
    p_glViewport(0, 0, LW * S, LH * S);
    p_glEnable(GL_SCISSOR_TEST);
    p_glScissor(d->sx, d->sy, d->sw, d->sh);
    p_glUseProgram(prog);
    p_glUniform2f(U.fbsize, (float)LW, (float)LH);
    p_glUniform1i(U.clear, d->clear);
    p_glUniform1i(U.tex_on, d->tex_on);
    if (d->tex_on) {
        p_glActiveTexture(GL_TEXTURE0);
        p_glBindTexture(GL_TEXTURE_2D, d->tex);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, d->linear ? GL_LINEAR : GL_NEAREST);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, d->linear ? GL_LINEAR : GL_NEAREST);
        if (d->wrap_s >= 0) {
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, d->wrap_s ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, d->wrap_t ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        }
        p_glUniform2f(U.texsize, d->tw, d->th);
        p_glUniform1i(U.texfunc, d->texfunc);
        p_glUniform1i(U.rgba, d->rgba);
        p_glUniform1i(U.dbl, d->dbl);
        uint32_t env = d->env;
        p_glUniform4f(U.env, (float)(env & 255) / 255.f, (float)((env >> 8) & 255) / 255.f, (float)((env >> 16) & 255) / 255.f, 1.f);
    }
    p_glUniform1i(U.atest, d->atest);
    if (d->atest) {
        p_glUniform1i(U.afunc, d->afunc);
        p_glUniform1f(U.aref, (float)d->aref);
        p_glUniform1f(U.amask, (float)d->amask);
    }
    apply_state(d);
    p_glBindVertexArray(vao);
    p_glBindBuffer(GL_ARRAY_BUFFER, vbo);
    p_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vbn * sizeof(GlVtx)), vbuf, GL_STREAM_DRAW);
    p_glDrawArrays(GL_TRIANGLES, 0, vbn);
    frame_draws += batch_draws; frame_calls++;
    vbn = 0; batch_draws = 0;
}

static void ge_gl_draw(const GeDrawState *s, const GeVtx *v, int m) {
    if (!ready) return;
    gpu_timer_begin();
    Target *t = target_for(s->target, 1);
    if (!t) return;
    if (!m) return;
    memcpy(vtmp, v, (size_t)m * sizeof(GlVtx));   /* (GeVtx and GlVtx: the same layout) */

    DrawState d;
    memset(&d, 0, sizeof d);
    d.t = t;
    d.sx = s->sx1 * S; d.sy = s->sy1 * S; d.sw = (s->sx2 - s->sx1 + 1) * S; d.sh = (s->sy2 - s->sy1 + 1) * S;
    d.clear = s->clear;
    d.tex_on = s->tex_on;
    if (d.tex_on) {
        int ox = 0, oy = 0;
        Target *src = target_containing(s->tex.addr, &ox, &oy);
        d.linear = s->linear;
        if (src && src != t) {
            /* render-to-texture: sample the upscaled target; texel (u,v) is target pixel (ox+u, oy+v) */
            d.tex = src->tex; d.wrap_s = d.wrap_t = -1;
            for (int i = 0; i < m; i++) { vtmp[i].u += (float)ox; vtmp[i].v += (float)oy; }
            d.tw = (float)LW; d.th = (float)LH;
        } else {
            /* the rows this draw reads: its v range, a texel either side for filtering; all of it when
               the coordinates wrap or the range is unusual */
            float vmin = vtmp[0].v, vmax = vtmp[0].v;
            for (int i = 1; i < m; i++) { if (vtmp[i].v < vmin) vmin = vtmp[i].v; if (vtmp[i].v > vmax) vmax = vtmp[i].v; }
            int th = s->tex.h, v0 = 0, v1 = th;
            if (vmin >= 0.f && vmax <= (float)th) { v0 = (int)vmin - 1; v1 = (int)vmax + 2; if (v0 < 0) v0 = 0; if (v1 > th) v1 = th; }
            GeTexEntry *e = texture_from_memory(&s->tex, v0, v1);   /* (flushes the open batch first if it uploads) */
            if (!e) return;
            d.tex = (GLuint)e->tex;
            d.wrap_s = s->clamp_s; d.wrap_t = s->clamp_t;
            d.tw = (float)e->w; d.th = (float)e->h;
        }
        d.texfunc = s->texfunc; d.rgba = s->tex_alpha; d.dbl = s->doubled;
        d.env = s->env;
    }
    d.atest = s->atest;
    if (d.atest) { d.afunc = s->afunc; d.aref = s->aref; d.amask = s->amask; }
    d.blend = s->blend;
    if (d.blend) { d.blend_eq = s->blend_eq; d.blend_src = s->blend_src; d.blend_dst = s->blend_dst; d.blend_const = s->blend_const; }
    d.write_r = s->write_r; d.write_g = s->write_g; d.write_b = s->write_b; d.write_a = s->write_a;

    if (vbn && (vbn + m > VMAX || memcmp(&d, &batch, sizeof d))) flush_batch();
    if (!vbn) batch = d;
    memcpy(&vbuf[vbn], vtmp, (size_t)m * sizeof(GlVtx));
    vbn += m;
    batch_draws++;
    cur_target = t;
    t->used = 1;
}

/* ---------------------------------------------------------------- presentation */
static GLuint upload_tex;
static int upload_w, upload_h;

static void draw_to_window(GLuint tex, float src_w, float src_h, float uv_w, float uv_h, int win_w, int win_h, int sharp) {
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    p_glDisable(GL_SCISSOR_TEST);
    p_glDisable(GL_BLEND);
    p_glColorMask(1, 1, 1, 1);
    p_glViewport(0, 0, win_w, win_h);
    p_glClearColor(0, 0, 0, 1);
    p_glClear(GL_COLOR_BUFFER_BIT);
    /* letterbox to the PSP's 480:272 */
    float sx = (float)win_w / 480.f, sy = (float)win_h / 272.f, sc = sx < sy ? sx : sy;
    float w = 480.f * sc, h = 272.f * sc;
    float x0 = ((float)win_w - w) / 2.f, y0 = ((float)win_h - h) / 2.f;
    float l = x0 / (float)win_w * 2.f - 1.f, r = (x0 + w) / (float)win_w * 2.f - 1.f;
    float b = y0 / (float)win_h * 2.f - 1.f, t = (y0 + h) / (float)win_h * 2.f - 1.f;
    /* guest row 0 is texture row 0, which must appear at the top */
    GlVtx q[6] = {
        { l, t, 0, 0, {0} }, { r, t, 1, 0, {0} }, { l, b, 0, 1, {0} },
        { r, t, 1, 0, {0} }, { r, b, 1, 1, {0} }, { l, b, 0, 1, {0} } };
    p_glUseProgram(prog_present);
    p_glActiveTexture(GL_TEXTURE0);
    p_glBindTexture(GL_TEXTURE_2D, tex);
    p_glUniform1i(UP.tex, 0);
    p_glUniform2f(UP.src_size, src_w, src_h);
    p_glUniform2f(UP.out_size, w, h);
    p_glUniform2f(UP.uv_max, uv_w, uv_h);
    p_glUniform1i(UP.sharp, sharp);
    p_glBindVertexArray(vao);
    p_glBindBuffer(GL_ARRAY_BUFFER, vbo);
    p_glBufferData(GL_ARRAY_BUFFER, sizeof q, q, GL_STREAM_DRAW);
    p_glDrawArrays(GL_TRIANGLES, 0, 6);
}

static int ge_gl_present(uint32_t fb_addr, int win_w, int win_h) {
    if (!ready) return 0;
    flush_batch();
    Target *t = target_for(fb_addr, 0);
    if (!t || !t->used) return 0;
    p_glBindTexture(GL_TEXTURE_2D, t->tex);
    /* window under half the render's size: mip-map so the downscale is clean. Between half and full
       size, bilinear is enough, and that's every frame on a monitor whose height isn't a multiple of
       272 (1080 with scale 4), where making mip-maps of the whole target each frame would cost most */
    if (win_h * 2 <= 272 * S) {
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
        p_glGenerateMipmap(GL_TEXTURE_2D);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* SCREEN SETTING > FILTER (shader_gl.c): through the chosen shader if there is one to draw with */
    if (shader_gl_present(t->tex, LW * S, LH * S, 480 * S, 272 * S, win_w, win_h)) ge_texcache_forget_last();
    else draw_to_window(t->tex, 480.f * S, 272.f * S, 480.f / LW, 272.f / LH, win_w, win_h, 1);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gpu_timer_end();
    return 1;
}

static void ge_gl_present_rgba(const uint8_t *rgba, int w, int h, int win_w, int win_h) {
    if (!ready) return;
    flush_batch();
    if (!upload_tex || upload_w != w || upload_h != h) {
        if (!upload_tex) p_glGenTextures(1, &upload_tex);
        p_glBindTexture(GL_TEXTURE_2D, upload_tex);
        p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        upload_w = w; upload_h = h;
    }
    p_glBindTexture(GL_TEXTURE_2D, upload_tex);
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    if (shader_gl_present(upload_tex, w, h, w, h, win_w, win_h)) ge_texcache_forget_last();   /* the software renderer's frames take the FILTER too */
    else draw_to_window(upload_tex, (float)w, (float)h, 1.f, 1.f, win_w, win_h, 1);
    gpu_timer_end();
}

static int ge_gl_read(uint32_t fb_addr, uint8_t *rgba, int *w, int *h) {
    if (!ready) return 0;
    flush_batch();
    Target *t = target_for(fb_addr, 0);
    if (!t || !t->used) return 0;
    *w = 480 * S; *h = 272 * S;
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
    p_glPixelStorei(GL_PACK_ALIGNMENT, 4);
    p_glReadPixels(0, 0, *w, *h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    for (int i = 0; i < *w * *h; i++) rgba[i * 4 + 3] = 255;
    return 1;
}

static int ge_gl_scale(void) { return S; }
static const char *ge_gl_gpu_name(void) { const char *r = ready ? (const char *)p_glGetString(GL_RENDERER) : NULL; return r ? r : ""; }

/* the GPU's time for each frame: a ring of timer queries, read back a few frames later so the CPU
 * never waits for them. A query starts at the frame's first draw and ends when the frame has been
 * drawn into the window, before the swap: so it no longer counts the wait for vsync (until 0.35.2 it
 * ran from one frame's end to the next, which made it the whole frame, idle included). */
#define NQUERY 4
static GLuint queries[NQUERY];
static int query_on, query_used, query_pending[NQUERY];
static int timer_ok(void) { return p_glGenQueries && p_glBeginQuery && p_glEndQuery && p_glGetQueryObjectiv && p_glGetQueryObjectui64v; }
static void gpu_timer_begin(void) {
    if (query_on || query_used || !timer_ok()) return;
    if (!queries[0]) p_glGenQueries(NQUERY, queries);
    int k = (int)(frame_no % NQUERY);
    query_pending[k] = 0;                           /* never read back: reuse it */
    p_glBeginQuery(GL_TIME_ELAPSED, queries[k]);
    query_on = query_used = 1;
}
static void gpu_timer_end(void) {
    if (!query_on) return;
    p_glEndQuery(GL_TIME_ELAPSED);
    query_pending[frame_no % NQUERY] = 1;
    query_on = 0;
}
static void gpu_timer_frame(void) {
    gpu_timer_end();
    query_used = 0;
    if (!timer_ok() || !queries[0]) return;
    int r = (int)((frame_no + 1) % NQUERY);         /* the oldest: three frames ago */
    if (query_pending[r]) {
        GLint avail = 0; p_glGetQueryObjectiv(queries[r], GL_QUERY_RESULT_AVAILABLE, &avail);
        if (avail) { uint64_t ns = 0; p_glGetQueryObjectui64v(queries[r], GL_QUERY_RESULT, &ns); perf_gpu_time(ns); }
        query_pending[r] = 0;
    }
}

static void ge_gl_frame_end(void) {
    flush_batch();
    perf_count_draws(frame_draws, frame_calls);
    frame_draws = frame_calls = 0;
    gpu_timer_frame();
    frame_no++;
    ge_texcache_frame_end();                       /* (frees textures unused for 10 seconds) */
}

/* ---------------------------------------------------------------- init */
/* Auto scale: the smallest that covers the monitor's height (1080 lines: 4, 1088 rows), but no more
 * than 4. Every render target is 512*S x 272*S, and the game draws each frame through two of them, so
 * the GPU's work grows with S squared: 8 on a 4K monitor is 16 times scale 2's. Past 4 the picture
 * gains little. --scale asks for any size from 1 to 8. */
#define AUTO_SCALE_MAX 4
static const char *scale_reason = "auto";
static const char *ge_gl_scale_reason(void) { return scale_reason; }
static void ge_gl_finish(void) { if (ready) flush_batch(); if (ready && p_glFinish) p_glFinish(); }   /* host_sdl.c (through ge_backend): FRAME TIMING SMOOTH */
static int software_gl(const char *r) {
    return r && (strstr(r, "llvmpipe") || strstr(r, "softpipe") || strstr(r, "SwiftShader") || strstr(r, "GDI Generic") ||
                 strstr(r, "Microsoft Basic Render"));
}
static int ge_gl_init(void *(*getproc)(const char *), int scale, int display_h) {
    if (!gl_load(getproc)) return 0;
    { extern const char *rt_work_dir; shader_gl_init(getproc, rt_work_dir); }
    if (scale > 0) { S = scale > 8 ? 8 : scale; scale_reason = "--scale"; }
    else if (software_gl((const char *)p_glGetString(GL_RENDERER))) { S = 1; scale_reason = "auto, software OpenGL"; }
    else {
        S = display_h > 0 ? (display_h + 271) / 272 : AUTO_SCALE_MAX;
        if (S > AUTO_SCALE_MAX) S = AUTO_SCALE_MAX;
        if (S < 1) S = 1;
        scale_reason = display_h > 0 ? "auto" : "auto, monitor unknown";
    }
    prog = link(vs_src, fs_src);
    prog_present = link(vs_present, fs_present);
    if (!prog || !prog_present) return 0;
#define UNI(f, name) U.f = p_glGetUniformLocation(prog, name)
    UNI(fbsize, "u_fbsize"); UNI(tex_on, "u_tex_on"); UNI(texfunc, "u_texfunc"); UNI(rgba, "u_rgba"); UNI(dbl, "u_dbl");
    UNI(env, "u_env"); UNI(atest, "u_atest"); UNI(aref, "u_aref"); UNI(amask, "u_amask"); UNI(afunc, "u_afunc");
    UNI(clear, "u_clear"); UNI(texsize, "u_texsize"); UNI(sampler, "u_tex");
#undef UNI
    UP.tex = p_glGetUniformLocation(prog_present, "u_tex");
    UP.src_size = p_glGetUniformLocation(prog_present, "u_src_size");
    UP.out_size = p_glGetUniformLocation(prog_present, "u_out_size");
    UP.uv_max = p_glGetUniformLocation(prog_present, "u_uv_max");
    UP.sharp = p_glGetUniformLocation(prog_present, "u_sharp");
    p_glUseProgram(prog);
    p_glUniform1i(U.sampler, 0);
    p_glGenVertexArrays(1, &vao);
    p_glBindVertexArray(vao);
    p_glGenBuffers(1, &vbo);
    p_glBindBuffer(GL_ARRAY_BUFFER, vbo);
    p_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlVtx), (void *)0);
    p_glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GlVtx), (void *)(2 * sizeof(float)));
    p_glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GlVtx), (void *)(4 * sizeof(float)));
    p_glEnableVertexAttribArray(0); p_glEnableVertexAttribArray(1); p_glEnableVertexAttribArray(2);
    p_glDisable(GL_DEPTH_TEST);
    p_glDisable(GL_CULL_FACE);
    ge_texcache_init(&gl_tex_ops);
    ready = 1;
    rt_log("GL: %s / %s, internal resolution %dx%d (x%d, %s)", (const char *)p_glGetString(GL_VERSION),
           (const char *)p_glGetString(GL_RENDERER), 480 * S, 272 * S, S, scale_reason);
    return 1;
}

/* ---------------------------------------------------------------- the backend (ge_backend.h) */
static int gl_backend_init(const GeBackendInit *in) { return ge_gl_init(in->gl_getproc, in->scale, in->display_h); }
const GeBackend ge_backend_gl = {
    "OpenGL",
    gl_backend_init,
    ge_gl_draw, ge_gl_list_start, ge_gl_memory_written,
    ge_gl_present, ge_gl_present_rgba, ge_gl_frame_end, ge_gl_finish, ge_gl_read,
    ge_gl_scale, ge_gl_gpu_name, ge_gl_scale_reason,
};
