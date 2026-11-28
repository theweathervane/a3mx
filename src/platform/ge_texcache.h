/* The GPU renderers' texture cache (0.174.0): which of the guest's textures each backend already has,
 * and which rows of one changed since. Shared by OpenGL (ge_gl.c) and SDL3 GPU (ge_sdlgpu.c), so both
 * decide alike what to decode and upload; each supplies how to make, fill and free a texture.
 *
 * Small textures (up to 32 KB: glyphs, palettes' worth of tiles) are keyed by a hash of their bytes,
 * palette and format. Large ones (the 128 KB sprite and stage pages in VRAM) are keyed by where they
 * are and how they are read, and keep a copy of the bytes they were decoded from: a draw compares only
 * the 8-row bands its texture coordinates reach against that copy, and decodes and re-uploads only the
 * bands that differ. (Moved from ge_gl.c unchanged: OpenGL draws exactly what it drew before.) */
#pragma once
#include <stdint.h>
#include "ge_backend.h"

typedef struct GeTexOps {
    uintptr_t (*create)(void);                           /* a new texture object (0: failed) */
    void (*upload)(uintptr_t t, const uint8_t *rgba, int w, int h, int y0, int y1, int whole);   /* rows y0 .. y1 - 1; whole: (re)define it w x h */
    void (*destroy)(uintptr_t t);
    void (*before_change)(void);                         /* about to upload into or free a texture: draw what is pending first */
} GeTexOps;

typedef struct GeTexEntry { uint64_t key; uintptr_t tex; int w, h; uint64_t last_used; } GeTexEntry;

void ge_texcache_init(const GeTexOps *ops);
/* the texture TX for a draw reading rows v0 .. v1 - 1, uploaded or brought up to date; NULL if too big */
GeTexEntry *ge_texcache_lookup(const GeTexRef *tx, int v0, int v1);
void ge_texcache_memory_changed(void);                   /* a display list starts, or a transfer wrote memory */
void ge_texcache_forget_last(void);                     /* the last texture is looked up again next time (after FILTER drew) */
void ge_texcache_frame_end(void);                        /* ages the cache: textures unused for 10 seconds are freed, a few a frame */
