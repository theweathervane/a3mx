/* The GPU renderer's backend: what the list walker (ge_soft.c, hle_ge.c), the frame loop (main.c) and
 * the window (host_sdl.c) ask of whichever graphics API draws the game.
 *
 * One implementation so far: OpenGL 3.3 (ge_gl.c, ge_backend_gl). The interface is the GE's work in
 * terms any API can do: triangles of GeVtx with a scissor, a texture cache that is told when guest
 * memory may have changed, showing a frame in the window, and reading one back. A backend reads the
 * rest of the draw's state (blending, alpha test, the texture and its palette, the target frame buffer)
 * through ge.h's accessors (ge_regs, ge_tex_addr, ge_decode_texture, ge_clut_hash, ge_fb_base, ...),
 * which belong to the list walker, not to any API. So an SDL3 GPU backend (Vulkan / D3D12 / Metal) or
 * a librashader runtime for it slots in beside ge_backend_gl without touching the callers.
 *
 * Every entry is safe to call before init or after a failed init (it does nothing, or returns 0 / "").
 * The game never sees the backend: it changes what is drawn, never what the game does. */
#pragma once
#include <stdint.h>
#include "ge.h"

typedef struct GeBackendInit {
    void *(*gl_getproc)(const char *);  /* OpenGL backends: the context's function loader */
    int scale;                          /* internal resolution, 1-8 x 480x272; 0 for auto (from display_h) */
    int display_h;                      /* the monitor's height in pixels, 0 if unknown */
} GeBackendInit;

typedef struct GeBackend {
    const char *name;                   /* "OpenGL": the log, perf and last_run.txt ("GPU (OpenGL)") */

    /* 0 if the API is not available. Then the software renderer draws, and frames are shown by SDL. */
    int  (*init)(const GeBackendInit *in);

    /* ---- drawing (ge_soft.c) ----
     * type: GE primitive (3 triangles, 4 strip, 5 fan, 6 sprites given as corner pairs); flat: flat
     * shading (one colour per primitive); sx1..sy2: the scissor in guest pixels. */
    void (*draw)(int type, const GeVtx *v, int n, int flat, int sx1, int sy1, int sx2, int sy2);
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

extern const GeBackend ge_backend_gl;  /* ge_gl.c */
extern const GeBackend *ge_backend;    /* the one in use (ge_backend.c): ge_backend_gl */

/* ge_backend->init with these arguments */
int ge_backend_init(void *(*gl_getproc)(const char *), int scale, int display_h);
