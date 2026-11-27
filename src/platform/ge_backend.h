/* The GPU renderer's backend: what the list walker (ge_soft.c, hle_ge.c), the frame loop (main.c) and
 * the window (host_sdl.c) ask of whichever graphics API draws the game.
 *
 * Two implementations: OpenGL 3.3 (ge_gl.c, ge_backend_gl, the default) and SDL3's GPU API (ge_sdlgpu.c,
 * ge_backend_sdlgpu: Vulkan, Direct3D 12 or Metal, 0.174.0). The interface is the GE's work in
 * terms any API can do: a list of triangles (GeVtx) with an explicit draw state (GeDrawState: target,
 * scissor, texture and how it is sampled and combined, alpha test, blending, colour mask; 0.174.0), a
 * texture cache that is told when guest memory may have changed, showing a frame in the window, and
 * reading one back. ge_state.c turns the GE's registers and primitives into those (ge_draw): backends
 * never read the GE's registers, and their texture caches are the same code (ge_texcache.c).
 *
 * Every entry is safe to call before init or after a failed init (it does nothing, or returns 0 / "").
 * The game never sees the backend: it changes what is drawn, never what the game does. */
#pragma once
#include <stdint.h>
#include "ge.h"

/* ---- one draw's state (ge_state.c fills it from the GE's registers) ---- */
enum { GE_TEXFUNC_MODULATE, GE_TEXFUNC_DECAL, GE_TEXFUNC_BLEND, GE_TEXFUNC_REPLACE, GE_TEXFUNC_ADD };
enum { GE_CMP_NEVER, GE_CMP_ALWAYS, GE_CMP_EQUAL, GE_CMP_NOTEQUAL, GE_CMP_LESS, GE_CMP_LEQUAL, GE_CMP_GREATER, GE_CMP_GEQUAL };
enum { GE_BLEND_ADD, GE_BLEND_SUBTRACT, GE_BLEND_REVERSE_SUBTRACT, GE_BLEND_MIN, GE_BLEND_MAX };
enum { GE_BF_ZERO, GE_BF_ONE, GE_BF_SRC_COLOR, GE_BF_ONE_MINUS_SRC_COLOR, GE_BF_DST_COLOR, GE_BF_ONE_MINUS_DST_COLOR,
       GE_BF_SRC_ALPHA, GE_BF_ONE_MINUS_SRC_ALPHA, GE_BF_DST_ALPHA, GE_BF_ONE_MINUS_DST_ALPHA, GE_BF_CONSTANT };

typedef struct GeTexRef {               /* the texture a draw samples: guest memory, or a frame buffer drawn earlier */
    uint32_t addr;                      /* guest address; the backend checks whether it is one of its render targets */
    uint32_t bytes;                     /* guest bytes the texture covers */
    int w, h, stride;                   /* texels; stride: texels a row in memory */
    uint32_t fmt, size, mode;           /* how the bytes are read (format, size and swizzle words: the cache's key) */
    uint64_t clut_hash;                 /* the palette, hashed (indexed formats) */
    void (*decode_rows)(uint8_t *rgba, int y0, int y1);   /* rows y0 .. y1 - 1 as RGBA8, tex.w a row */
} GeTexRef;

typedef struct GeDrawState {
    uint32_t target;                    /* the frame buffer drawn into (guest address) */
    int sx1, sy1, sx2, sy2;             /* scissor, guest pixels, inclusive */
    int clear;                          /* a clear: colour only, no texture, test or blending */
    int tex_on;
    GeTexRef tex;
    int linear;                         /* bilinear (else nearest) */
    int clamp_s, clamp_t;               /* clamp to the edge (else repeat) */
    int texfunc, tex_alpha, doubled;    /* GE_TEXFUNC_*; the texture's alpha used; colour doubled */
    uint32_t env;                       /* the BLEND texfunc's colour, 0xBBGGRR */
    int atest, afunc, aref, amask;      /* alpha test: GE_CMP_*, reference and mask 0 - 255 */
    int blend, blend_eq, blend_src, blend_dst;   /* GE_BLEND_*, GE_BF_* */
    uint32_t blend_const;               /* GE_BF_CONSTANT's colour, 0xBBGGRR */
    int write_r, write_g, write_b, write_a;      /* colour mask */
} GeDrawState;

/* ge_soft.c, each primitive: TYPE (3 triangles, 4 strip, 5 fan, 6 sprites given as corner pairs), FLAT
   shading, the scissor SX1 .. SY2 (guest pixels): into triangles and a GeDrawState, to the backend */
void ge_draw(int type, const GeVtx *v, int n, int flat, int sx1, int sy1, int sx2, int sy2);

typedef struct GeBackendInit {
    void *(*gl_getproc)(const char *);  /* OpenGL: the context's function loader */
    void *window;                       /* SDL GPU: the SDL_Window (made without OpenGL) */
    const char *gpu_driver;             /* SDL GPU: "vulkan", "direct3d12" or "metal" */
    int scale;                          /* internal resolution, 1-8 x 480x272; 0 for auto (from display_h) */
    int display_h;                      /* the monitor's height in pixels, 0 if unknown */
} GeBackendInit;

typedef struct GeBackend {
    const char *name;                   /* "OpenGL": the log, perf and last_run.txt ("GPU (OpenGL)") */

    /* 0 if the API is not available. Then the software renderer draws, and frames are shown by SDL. */
    int  (*init)(const GeBackendInit *in);

    /* ---- drawing (ge_state.c's ge_draw) ----
     * N vertices, N / 3 triangles in order (flat shading already applied), with draw state S. N may be 0
     * (a primitive that draws nothing, or an empty scissor): the target is still made then, as the GE's
     * frame buffer is in use from the first draw into it. */
    void (*draw)(const GeDrawState *s, const GeVtx *tri, int n);
    void (*list_start)(void);           /* a display list (or a continuation) starts: guest memory may have changed */
    void (*memory_written)(void);       /* a block transfer inside a list wrote guest memory */

    /* ---- display (main.c, host_sdl.c) ---- */
    int  (*present)(uint32_t fb_addr, int win_w, int win_h);   /* draw the displayed frame to the window; 0 if not GPU-rendered */
    void (*present_rgba)(const uint8_t *rgba, int w, int h, int win_w, int win_h);  /* software frames, sharp-bilinear */
    void (*frame_end)(void);            /* after each frame is shown (or not): batches out, caches aged, GPU timer read */
    void (*finish)(void);               /* wait until the GPU has drawn everything (FRAME TIMING SMOOTH) */
    int  (*read)(uint32_t fb_addr, uint8_t *rgba, int *w, int *h);  /* full-resolution readback (480*S x 272*S RGBA8, row 0 at the top) */

    /* ---- what was chosen, for the log and perf ---- */
    int  (*scale)(void);
    const char *(*gpu_name)(void);      /* the GPU and driver */
    const char *(*scale_reason)(void);  /* how the scale was chosen */
} GeBackend;

extern const GeBackend ge_backend_gl;      /* ge_gl.c */
extern const GeBackend ge_backend_sdlgpu;  /* ge_sdlgpu.c */
extern const GeBackend *ge_backend;        /* the one in use (ge_backend.c): ge_backend_gl unless main.c chose another */

/* ge_backend->init with these arguments (OpenGL) */
int ge_backend_init(void *(*gl_getproc)(const char *), int scale, int display_h);
/* ge_backend_sdlgpu made the one in use and started on WINDOW with DRIVER; 0 (and OpenGL the one in use
   again) if it can't be had */
int ge_backend_init_sdlgpu(void *window, const char *driver, int scale, int display_h);
const char *ge_sdlgpu_api(void);           /* the API SDL chose ("vulkan", "direct3d12", "metal"), or "" */
