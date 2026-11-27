/* GE (PSP GPU) interfaces shared by the list walker, the renderer and the display. */
#pragma once
#include <stdint.h>
#include <stddef.h>
void     ge_soft_cmd(uint32_t pc, uint32_t op, uint32_t arg);   /* every command goes through here */
uint32_t ge_soft_reladdr(uint32_t arg);                          /* BASE/offset-relative address */
void     ge_fb_to_rgba(uint32_t addr, int stride, int fmt, int w, int h, uint8_t *out);
int      png_write(const char *path, const uint8_t *rgba, int w, int h);
extern uint64_t ge_prims_drawn, ge_pixels_drawn;

/* ---- shared GE state for the GPU renderer (ge_soft.c owns the registers and vertex decoding) ---- */
typedef struct { float x, y, u, v; uint8_t r, g, b, a; } GeVtx;
extern int ge_use_gl;                               /* 1: primitives go to ge_backend->draw instead of the software rasteriser */
extern int ge_no_draw;                              /* 1: walk display lists but draw nothing (--renderer none / lazy) */
const uint32_t *ge_regs(void);                      /* last value of every GE register */
uint32_t ge_tex_addr(void);
int      ge_tex_w(void), ge_tex_h(void), ge_tex_stride(void);
uint32_t ge_tex_bytes(void);                        /* bytes of guest memory the current texture covers */
void     ge_decode_texture(uint8_t *rgba);          /* whole current texture (tex_w x tex_h) as RGBA8 */
void     ge_decode_texture_rows(uint8_t *rgba, int y0, int y1);   /* rows y0 .. y1 - 1 of it */
uint64_t ge_clut_hash(void);
uint32_t ge_fb_base(void);
int      ge_fb_stride(void), ge_fb_fmt(void);
uint64_t ge_hash(const void *p, size_t n);

/* ---- GPU renderer: ge_backend.h (the interface), ge_gl.c (OpenGL) ---- */
