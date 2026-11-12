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

/* ---------------------------------------------------------------- texture cache
 *
 * Textures from guest memory are decoded to RGBA8 once and kept as GL textures. The catch is knowing
 * when the guest memory under one has changed: the game can rewrite it between any two display lists,
 * and it sends most sprites as a list of their own, so a check runs for nearly every draw.
 *
 * Small textures (up to 32 KB: glyphs, palettes' worth of tiles) are keyed by a hash of their bytes,
 * palette and format, as before; hashing them is cheap.
 *
 * Large ones (the 128 KB sprite and stage pages in VRAM) are keyed by where they are and how they are
 * read (address, size, format, swizzle, stride, palette), and keep a copy of the bytes they were decoded
 * from. A draw compares only the 8-row bands its texture coordinates reach (one texel of margin, for
 * linear filtering) against that copy, and decodes and re-uploads only the bands that differ. The
 * result is the same as hashing the whole texture before every draw, for every pixel drawn: a band the
 * draw doesn't read can't change what it draws, and is checked by the next draw that reads it. */
typedef struct { uint64_t key; GLuint tex; int w, h; uint64_t last_used; } TexEntry;
#define TEX_CACHE 1024
static TexEntry tcache[TEX_CACHE];
static uint64_t frame_no;
static void gpu_timer_begin(void), gpu_timer_end(void);   /* the frame's GPU timer (below) */
static uint8_t decode_buf[1024 * 1024 * 4];
static struct { int valid; uint32_t addr, bytes, fmt, clutfmt, size, mode; uint64_t clut_hash; TexEntry *e; } last_tex;

#define SMALL_TEX 0x8000u
#define BIG_CACHE 64
#define BIG_SHADOW_MAX (48u << 20)      /* bytes of copies kept at most */
typedef struct {
    TexEntry te;                        /* te.tex, w, h, last_used as for a small texture */
    uint32_t addr, bytes, fmt, size, mode, stride;
    uint64_t clut;
    uint8_t *shadow;                    /* the guest bytes the texture was decoded from */
    uint64_t *band_checked;             /* list number each band was last compared in */
    int nbands;
    uint32_t band_bytes;
} BigTex;
static BigTex big[BIG_CACHE];
static uint32_t big_shadow_total;
static uint64_t list_no = 1;            /* bumped whenever guest memory may have changed */

void ge_gl_list_start(void) { last_tex.valid = 0; list_no++; }
void ge_gl_memory_written(void) { last_tex.valid = 0; list_no++; }

static void big_free(BigTex *b) {
    if (b->te.tex) p_glDeleteTextures(1, &b->te.tex);
    big_shadow_total -= b->shadow ? b->bytes : 0;
    free(b->shadow); free(b->band_checked);
    memset(b, 0, sizeof *b);
}

static void upload_rows(TexEntry *e, int y0, int y1, int whole) {
    ge_decode_texture_rows(decode_buf, y0, y1);
    p_glBindTexture(GL_TEXTURE_2D, e->tex);
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (whole) {
        p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, e->w, e->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, decode_buf);
        p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    } else {
        p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y0, e->w, y1 - y0, GL_RGBA, GL_UNSIGNED_BYTE, decode_buf);
    }
}

/* a large texture: rows v0 .. v1 - 1 are the ones this draw reads */
static TexEntry *big_lookup(const uint32_t *R, uint32_t addr, uint32_t bytes, uint64_t ch, int w, int h, int v0, int v1) {
    uint32_t fmt = R[0xC3], size = R[0xB8], mode = R[0xC2] & 1, stride = (uint32_t)ge_tex_stride();
    BigTex *b = NULL;
    for (int i = 0; i < BIG_CACHE && !b; i++) {
        BigTex *c = &big[i];
        if (c->shadow && c->addr == addr && c->bytes == bytes && c->fmt == fmt && c->size == size && c->mode == mode &&
            c->stride == stride && c->clut == ch) b = c;
    }
    const uint8_t *guest = (const uint8_t *)HOST(addr);
    if (!b) {
        for (;;) {                                  /* a free slot within the copies' budget, dropping the least recently used */
            BigTex *lru = NULL, *empty = NULL;
            for (int i = 0; i < BIG_CACHE; i++) {
                if (!big[i].shadow) { if (!empty) empty = &big[i]; }
                else if (!lru || big[i].te.last_used < lru->te.last_used) lru = &big[i];
            }
            if (empty && big_shadow_total + bytes <= BIG_SHADOW_MAX) { b = empty; break; }
            if (!lru) { b = empty; break; }
            big_free(lru);
        }
        if (!b) return NULL;
        b->addr = addr; b->bytes = bytes; b->fmt = fmt; b->size = size; b->mode = mode; b->stride = stride; b->clut = ch;
        b->shadow = (uint8_t *)malloc(bytes);
        b->band_bytes = bytes / (uint32_t)h * 8u;                     /* 8 rows: also one row of swizzled blocks */
        b->nbands = (h + 7) / 8;
        b->band_checked = (uint64_t *)calloc((size_t)b->nbands, sizeof(uint64_t));
        if (!b->shadow || !b->band_checked) { free(b->shadow); free(b->band_checked); memset(b, 0, sizeof *b); return NULL; }
        big_shadow_total += bytes;
        memcpy(b->shadow, guest, bytes);
        for (int i = 0; i < b->nbands; i++) b->band_checked[i] = list_no;
        p_glGenTextures(1, &b->te.tex);
        b->te.w = w; b->te.h = h;
        upload_rows(&b->te, 0, h, 1);
        perf_count_texture(1, bytes);
    } else {
        int b0 = v0 / 8, b1 = (v1 + 7) / 8;
        if (b0 < 0) b0 = 0;
        if (b1 > b->nbands) b1 = b->nbands;
        uint32_t compared = 0;
        int run = -1;                                                 /* first band of a run that changed */
        for (int i = b0; i <= b1; i++) {
            int changed = 0;
            if (i < b1 && b->band_checked[i] != list_no) {
                uint32_t off = (uint32_t)i * b->band_bytes, n = b->band_bytes;
                if (off + n > bytes) n = bytes - off;
                compared += n;
                b->band_checked[i] = list_no;
                if (memcmp(b->shadow + off, guest + off, n)) { memcpy(b->shadow + off, guest + off, n); changed = 1; }
            }
            if (changed && run < 0) run = i;
            if (!changed && run >= 0) {
                int y1 = i * 8 > h ? h : i * 8;
                upload_rows(&b->te, run * 8, y1, 0);
                perf_count_texture(1, 0);
                run = -1;
            }
        }
        perf_count_texture(0, compared);
    }
    b->te.last_used = frame_no;
    return &b->te;
}

static TexEntry *texture_lookup(const uint32_t *R, int v0, int v1);
static TexEntry *texture_from_memory(const uint32_t *R, int v0, int v1) {   /* timed as the frame's texture part (perf.h) */
    uint64_t t0 = perf_now_us();
    TexEntry *e = texture_lookup(R, v0, v1);
    perf_add(PERF_TEXTURE, perf_now_us() - t0);
    return e;
}
static TexEntry *texture_lookup(const uint32_t *R, int v0, int v1) {
    uint32_t addr = ge_tex_addr(), bytes = ge_tex_bytes();
    uint64_t ch = ge_clut_hash();
    int w = ge_tex_w(), h = ge_tex_h();
    if (w > 1024 || h > 1024 || bytes > 0x200000) return NULL;
    if (bytes > SMALL_TEX && h >= 8) return big_lookup(R, addr, bytes, ch, w, h, v0, v1);
    if (last_tex.valid && last_tex.addr == addr && last_tex.bytes == bytes && last_tex.fmt == R[0xC3] && last_tex.size == R[0xB8] &&
        last_tex.mode == R[0xC2] && last_tex.clut_hash == ch) { last_tex.e->last_used = frame_no; return last_tex.e; }
    uint64_t key = ge_hash(HOST(addr), bytes) ^ (ch * 31) ^ ((uint64_t)R[0xC3] << 40) ^ ((uint64_t)R[0xB8] << 48) ^
                   ((uint64_t)(R[0xC2] & 1) << 63) ^ ((uint64_t)addr << 8) ^ ((uint64_t)ge_tex_stride() << 20);
    TexEntry *e = NULL, *victim = &tcache[0];
    for (int i = 0; i < TEX_CACHE; i++) {
        if (tcache[i].tex && tcache[i].key == key) { e = &tcache[i]; break; }
        if (!tcache[i].tex) { victim = &tcache[i]; }
        else if (victim->tex && tcache[i].last_used < victim->last_used) victim = &tcache[i];
    }
    perf_count_texture(!e, bytes);
    if (!e) {
        e = victim;
        if (!e->tex) p_glGenTextures(1, &e->tex);
        e->w = w; e->h = h;
        upload_rows(e, 0, h, 1);
        e->key = key;
    }
    e->last_used = frame_no;
    last_tex.valid = 1; last_tex.addr = addr; last_tex.bytes = bytes; last_tex.fmt = R[0xC3]; last_tex.size = R[0xB8];
    last_tex.mode = R[0xC2]; last_tex.clut_hash = ch; last_tex.e = e;
    return e;
}

/* ---------------------------------------------------------------- blending */
static GLenum blend_factor(int f, int src_side, uint32_t fixed, int *use_const, float cc[3]) {
    switch (f) {
    case 0: return src_side ? GL_DST_COLOR : GL_SRC_COLOR;
    case 1: return src_side ? GL_ONE_MINUS_DST_COLOR : GL_ONE_MINUS_SRC_COLOR;
    case 2: case 6: return GL_SRC_ALPHA;
    case 3: case 7: return GL_ONE_MINUS_SRC_ALPHA;
    case 4: case 8: return GL_DST_ALPHA;
    case 5: case 9: return GL_ONE_MINUS_DST_ALPHA;
    default:
        if ((fixed & 0xFFFFFF) == 0xFFFFFF) return GL_ONE;
        if ((fixed & 0xFFFFFF) == 0) return GL_ZERO;
        *use_const = 1;
        cc[0] = (float)(fixed & 255) / 255.f; cc[1] = (float)((fixed >> 8) & 255) / 255.f; cc[2] = (float)((fixed >> 16) & 255) / 255.f;
        return GL_CONSTANT_COLOR;
    }
}

static void apply_state(const uint32_t *R, int clear) {
    if (clear || !(R[0x21] & 1)) p_glDisable(GL_BLEND);
    else {
        uint32_t bm = R[0xDF];
        int uc = 0; float cc[3] = { 0, 0, 0 };
        GLenum sf = blend_factor((int)(bm & 15), 1, R[0xE0], &uc, cc);
        GLenum df = blend_factor((int)((bm >> 4) & 15), 0, R[0xE1], &uc, cc);
        static const GLenum eqs[6] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX, GL_FUNC_ADD };
        p_glEnable(GL_BLEND);
        uint32_t eq = (bm >> 8) & 15;
        p_glBlendEquation(eqs[eq < 6 ? eq : 0]);
        p_glBlendFuncSeparate(sf, df, GL_ZERO, GL_ONE);          /* frame-buffer alpha is not blended */
        if (uc) p_glBlendColor(cc[0], cc[1], cc[2], 1.f);
    }
    if (clear) {
        uint32_t cm = R[0xD3];
        GLboolean c = (cm & 0x100) != 0, a = (cm & 0x200) != 0;
        p_glColorMask(c, c, c, a);
    } else {
        uint32_t m = R[0xE8];
        p_glColorMask((m & 0xFF) != 0xFF, ((m >> 8) & 0xFF) != 0xFF, ((m >> 16) & 0xFF) != 0xFF, 0);
    }
}

/* ---------------------------------------------------------------- drawing */
typedef struct { float x, y, u, v; uint8_t c[4]; } GlVtx;
static GlVtx vbuf[65536 * 3];

void ge_gl_draw(int type, const GeVtx *v, int n, int flat, int sx1, int sy1, int sx2, int sy2) {
    if (!ready) return;
    gpu_timer_begin();
    const uint32_t *R = ge_regs();
    Target *t = target_for(ge_fb_base(), 1);
    if (!t) return;
    int clear = (R[0xD3] & 1) != 0;

    /* triangles in guest pixels */
    int m = 0;
#define PUT(src) do { const GeVtx *s_ = (src); GlVtx *d_ = &vbuf[m++]; d_->x = s_->x; d_->y = s_->y; d_->u = s_->u; d_->v = s_->v; \
        d_->c[0] = s_->r; d_->c[1] = s_->g; d_->c[2] = s_->b; d_->c[3] = s_->a; } while (0)
    switch (type) {
    case 3: for (int i = 0; i + 2 < n; i += 3) { PUT(&v[i]); PUT(&v[i + 1]); PUT(&v[i + 2]); } break;
    case 4: for (int i = 0; i + 2 < n; i++) { PUT(&v[i]); PUT(&v[i + 1]); PUT(&v[i + 2]); } break;
    case 5: for (int i = 1; i + 1 < n; i++) { PUT(&v[0]); PUT(&v[i]); PUT(&v[i + 1]); } break;
    case 6:
        for (int i = 0; i + 1 < n; i += 2) {
            const GeVtx *a = &v[i], *b = &v[i + 1];
            GeVtx q[4] = { *b, *b, *b, *b };                /* sprites take the second vertex's colour */
            q[0].x = a->x; q[0].y = a->y; q[0].u = a->u; q[0].v = a->v;
            q[1].x = b->x; q[1].y = a->y; q[1].u = b->u; q[1].v = a->v;
            q[2].x = a->x; q[2].y = b->y; q[2].u = a->u; q[2].v = b->v;
            q[3].x = b->x; q[3].y = b->y; q[3].u = b->u; q[3].v = b->v;
            PUT(&q[0]); PUT(&q[1]); PUT(&q[2]); PUT(&q[1]); PUT(&q[3]); PUT(&q[2]);
        }
        flat = 1;
        break;
    default: return;
    }
#undef PUT
    if (!m) return;

    p_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    p_glViewport(0, 0, LW * S, LH * S);
    p_glEnable(GL_SCISSOR_TEST);
    if (sx2 < sx1 || sy2 < sy1) return;
    p_glScissor(sx1 * S, sy1 * S, (sx2 - sx1 + 1) * S, (sy2 - sy1 + 1) * S);
    p_glUseProgram(prog);
    p_glUniform2f(U.fbsize, (float)LW, (float)LH);
    p_glUniform1i(U.clear, clear);
    int tex_on = !clear && (R[0x1E] & 1);
    p_glUniform1i(U.tex_on, tex_on);
    if (tex_on) {
        int ox = 0, oy = 0;
        Target *src = target_containing(ge_tex_addr(), &ox, &oy);
        int linear = (R[0xC6] >> 8) & 1;
        p_glActiveTexture(GL_TEXTURE0);
        if (src && src != t) {
            /* render-to-texture: sample the upscaled target; texel (u,v) is target pixel (ox+u, oy+v) */
            p_glBindTexture(GL_TEXTURE_2D, src->tex);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
            for (int i = 0; i < m; i++) { vbuf[i].u += (float)ox; vbuf[i].v += (float)oy; }
            p_glUniform2f(U.texsize, (float)LW, (float)LH);
        } else {
            /* the rows this draw reads: its v range, a texel either side for filtering; all of it when
               the coordinates wrap or the range is unusual */
            float vmin = vbuf[0].v, vmax = vbuf[0].v;
            for (int i = 1; i < m; i++) { if (vbuf[i].v < vmin) vmin = vbuf[i].v; if (vbuf[i].v > vmax) vmax = vbuf[i].v; }
            int th = ge_tex_h(), v0 = 0, v1 = th;
            if (vmin >= 0.f && vmax <= (float)th) { v0 = (int)vmin - 1; v1 = (int)vmax + 2; if (v0 < 0) v0 = 0; if (v1 > th) v1 = th; }
            TexEntry *e = texture_from_memory(R, v0, v1);
            if (!e) return;
            p_glBindTexture(GL_TEXTURE_2D, e->tex);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (R[0xC7] & 1) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (R[0xC7] & 0x100) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            p_glUniform2f(U.texsize, (float)e->w, (float)e->h);
        }
        uint32_t tf = R[0xC9];
        p_glUniform1i(U.texfunc, (int)(tf & 7));
        p_glUniform1i(U.rgba, (int)((tf >> 8) & 1));
        p_glUniform1i(U.dbl, (int)((tf >> 16) & 1));
        uint32_t env = R[0xCA];
        p_glUniform4f(U.env, (float)(env & 255) / 255.f, (float)((env >> 8) & 255) / 255.f, (float)((env >> 16) & 255) / 255.f, 1.f);
    }
    int at = !clear && (R[0x22] & 1);
    p_glUniform1i(U.atest, at);
    if (at) {
        uint32_t a = R[0xDB];
        p_glUniform1i(U.afunc, (int)(a & 7));
        p_glUniform1f(U.aref, (float)((a >> 8) & 255));
        p_glUniform1f(U.amask, (float)((a >> 16) & 255));
    }
    apply_state(R, clear);
    if (flat) {   /* flat shading: every corner gets the triangle's last vertex colour */
        for (int i = 0; i + 2 < m; i += 3) {
            memcpy(vbuf[i].c, vbuf[i + 2].c, 4); memcpy(vbuf[i + 1].c, vbuf[i + 2].c, 4);
        }
    }
    p_glBindVertexArray(vao);
    p_glBindBuffer(GL_ARRAY_BUFFER, vbo);
    p_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(m * sizeof(GlVtx)), vbuf, GL_STREAM_DRAW);
    p_glDrawArrays(GL_TRIANGLES, 0, m);
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

int ge_gl_present(uint32_t fb_addr, int win_w, int win_h) {
    if (!ready) return 0;
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
    if (shader_gl_present(t->tex, LW * S, LH * S, 480 * S, 272 * S, win_w, win_h)) last_tex.valid = 0;
    else draw_to_window(t->tex, 480.f * S, 272.f * S, 480.f / LW, 272.f / LH, win_w, win_h, 1);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gpu_timer_end();
    return 1;
}

void ge_gl_present_rgba(const uint8_t *rgba, int w, int h, int win_w, int win_h) {
    if (!ready) return;
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
    if (shader_gl_present(upload_tex, w, h, w, h, win_w, win_h)) last_tex.valid = 0;   /* the software renderer's frames take the FILTER too */
    else draw_to_window(upload_tex, (float)w, (float)h, 1.f, 1.f, win_w, win_h, 1);
    gpu_timer_end();
}

int ge_gl_read(uint32_t fb_addr, uint8_t *rgba, int *w, int *h) {
    if (!ready) return 0;
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

int ge_gl_scale(void) { return S; }
const char *ge_gl_gpu_name(void) { const char *r = ready ? (const char *)p_glGetString(GL_RENDERER) : NULL; return r ? r : ""; }

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

void ge_gl_frame_end(void) {
    gpu_timer_frame();
    frame_no++;
    /* free textures unused for 10 seconds */
    if (frame_no % 600 == 0)
        for (int i = 0; i < TEX_CACHE; i++)
            if (tcache[i].tex && frame_no - tcache[i].last_used > 600) { p_glDeleteTextures(1, &tcache[i].tex); tcache[i].tex = 0; tcache[i].key = 0; }
    if (frame_no % 600 == 0)
        for (int i = 0; i < BIG_CACHE; i++)
            if (big[i].shadow && frame_no - big[i].te.last_used > 600) big_free(&big[i]);
}

/* ---------------------------------------------------------------- init */
/* Auto scale: the smallest that covers the monitor's height (1080 lines: 4, 1088 rows), but no more
 * than 4. Every render target is 512*S x 272*S, and the game draws each frame through two of them, so
 * the GPU's work grows with S squared: 8 on a 4K monitor is 16 times scale 2's. Past 4 the picture
 * gains little. --scale asks for any size from 1 to 8. */
#define AUTO_SCALE_MAX 4
static const char *scale_reason = "auto";
const char *ge_gl_scale_reason(void) { return scale_reason; }
void ge_gl_finish(void) { if (ready && p_glFinish) p_glFinish(); }   /* host_sdl.c: FRAME TIMING SMOOTH */
static int software_gl(const char *r) {
    return r && (strstr(r, "llvmpipe") || strstr(r, "softpipe") || strstr(r, "SwiftShader") || strstr(r, "GDI Generic") ||
                 strstr(r, "Microsoft Basic Render"));
}
int ge_gl_init(void *(*getproc)(const char *), int scale, int display_h) {
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
    ready = 1;
    rt_log("GL: %s / %s, internal resolution %dx%d (x%d, %s)", (const char *)p_glGetString(GL_VERSION),
           (const char *)p_glGetString(GL_RENDERER), 480 * S, 272 * S, S, scale_reason);
    return 1;
}
