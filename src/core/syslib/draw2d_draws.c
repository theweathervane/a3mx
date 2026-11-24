/* The 2D library's draws (docs/PSP_PROGRAM_MAP.md, "The 2D library"): each builds a 28- or 12-byte
 * record and its vertices in the frame's arena (0x089E40C0) and queues it by depth (0x089E44EC), to be
 * sent in depth order when the frame ends.
 *
 * Vertices go from the game's 4:3 picture to the library's: x + 48, y - 8. Colours go from the game's
 * ABGR to the library's ARGB (red and blue swapped).
 *
 * A textured draw's record (28 bytes): +0 vertices (16 bytes each: +0 u, +2 v, +4 colour, +8 x, +10 y),
 * +4 the palette's entries (0x09525E08's base + 2 × 16 × the texture's palette number), +8 the
 * texture's pixels, +12 the caller's blend (or flags), +16 the count, +20 / +22 the texture's size,
 * +24 the pixel kind's group (below), +25, +26 the caller's effect.
 */
#include "../decomp.h"

#include "../guest_mem.h"

void f_089E40C0(Cpu *c);   /* the frame's arena: n bytes (a0), answers where */
void f_089E44EC(Cpu *c);   /* a record queued: a0 depth, a1 record, a2 its kind (0 flat, 1 textured, 2 quads) */

#define TEXTURES   0x08B0C334u   /* 24 bytes each: +0 pixels, +20 its info */
#define TEX_CUR    0x08B2333Cu   /* the descriptor picked (draw2d.c): +8 the texture's number, -1 none */
#define PAL_BASE   0x09525E08u   /* u32: where the palette entries live */
#define UNTEXTURED 0x80000000u   /* 0x089E1618's flags: colours only */

/* info: +4 its palette's number, +24 the pixel kind (bits 24 - 31) and more (16 - 23), +36 / +40 its size */
enum { INFO_PALETTE = 4, INFO_KIND = 24, INFO_W = 36, INFO_H = 40 };

static uint32_t argb(uint32_t abgr) {
    return (abgr & 0xFF000000u) | (abgr & 0xFF0000u) >> 16 | (abgr & 0xFF00u) | (abgr & 0xFFu) << 16;
}
static gaddr arena(Cpu *c, uint32_t bytes) { return CALL1(f_089E40C0, bytes); }

/* The group the texture's pixel kind falls in: 0 for kinds 5 - 6, 1 for 7 - 8, otherwise 2, or 3 when
   bit 16 of the info's +24 is set. */
static uint8_t kind_group(uint32_t info24) {
    int32_t kind = (int32_t)(info24 >> 24 & 0xFF);
    if (kind == 5 || kind == 6) return 0;
    if (kind == 7 || kind == 8) return 1;
    return (info24 >> 16 & 1) ? 3 : 2;
}

/* The texture's part of a textured record. */
static void textured(gaddr rec, gaddr tex) {
    gaddr info = *W(tex + 20);
    uint32_t palette = (*W(info + INFO_PALETTE) << 4) & 0xFFFF;
    *W(rec + 4) = *W(PAL_BASE) + palette * 2u;
    *W(rec + 8) = *W(tex);
    *H(rec + 20) = (uint16_t)*W(info + INFO_W);
    *H(rec + 22) = (uint16_t)*W(info + INFO_H);
    *B(rec + 24) = kind_group(*W(info + INFO_KIND));
}

/* ---- 0x089E1950: textured vertices (a0 vertices of 16 bytes: +0 x, +2 y, +4 the depth (first
   vertex's low byte), +6 u, +8 v, +12 colour; a1 count; a2 texture; a3 blend; t0 effect), queued as
   kind 1 */
SFA_FUNC(089E1950, draw2d_textured) {
    gaddr in = ARG(0);
    uint32_t n = ARG(1);
    gaddr tex = TEXTURES + ARG(2) * 24u;
    gaddr rec = arena(c, 0x1C), out = arena(c, n << 4);
    *W(rec) = out;
    for (uint32_t i = 0; i < n; i++) {
        gaddr s = in + i * 16u, d = out + i * 16u;
        *H(d + 8) = (uint16_t)(S16(s) + 0x30);
        *H(d + 10) = (uint16_t)(S16(s + 2) - 8);
        *W(d + 4) = argb(*W(s + 12));
        *H(d) = *H(s + 6);
        *H(d + 2) = *H(s + 8);
    }
    textured(rec, tex);
    *W(rec + 12) = ARG(3);
    *W(rec + 16) = n;
    *B(rec + 25) = 0;
    *B(rec + 26) = (uint8_t)c->r[8];
    RETURN(CALL3(f_089E44EC, *H(in + 4) & 0xFF, rec, 1));
}

/* ---- 0x089E10D8: flat vertices (a0 vertices of 12 bytes: +0 x, +2 y, +4 the depth (first vertex's low
   byte), +8 colour; a1 count; a2 the record's +8), as a 12-byte record {vertices (12 bytes: +0 colour,
   +4 x, +6 y), count, a2}, queued as kind 0 */
SFA_FUNC(089E10D8, draw2d_flat) {
    gaddr in = ARG(0);
    uint32_t n = ARG(1), extra = ARG(2);
    gaddr rec = arena(c, 0xC), out = arena(c, n * 12u);
    *W(rec) = out;
    for (uint32_t i = 0; i < n; i++) {
        gaddr s = in + i * 12u, d = out + i * 12u;
        *H(d + 4) = (uint16_t)(S16(s) + 0x30);
        *H(d + 6) = (uint16_t)(S16(s + 2) - 8);
        *W(d) = argb(*W(s + 8));
    }
    *W(rec + 4) = n;
    *W(rec + 8) = extra;
    RETURN(CALL3(f_089E44EC, *H(in + 4) & 0xFF, rec, 0));
}

/* ---- 0x089E1618: a quad list (a0 {+0 places (x, y), +4 colours, +8 texel places (u, v), +12 count};
   a2 flags; f12 the depth, cut to an integer and its low byte kept), queued as kind 2. Textured from
   the descriptor picked (nothing at all while it has no texture, -1), unless the flags have bit 31:
   then colours only, the texture's fields 0. */
SFA_FUNC(089E1618, draw2d_quads) {
    gaddr list = ARG(0);
    uint32_t flags = ARG(2);
    float depth = FS(c, 12);
    gaddr pos = *W(list), colour = *W(list + 4), uv = *W(list + 8);
    if (!(flags & UNTEXTURED)) {
        uint32_t texture = *W(TEX_CUR + 8);
        if (texture == 0xFFFFFFFFu) return;
        uint32_t z = (uint32_t)CVT_TRUNC(c, depth) & 0xFF;
        gaddr tex = TEXTURES + texture * 24u;
        gaddr rec = arena(c, 0x1C), out = arena(c, *W(list + 12) << 4);
        *W(rec) = out;
        for (uint32_t i = 0; i < *W(list + 12); i++, pos += 4, colour += 4, uv += 4, out += 16) {
            *H(out + 8) = (uint16_t)(S16(pos) + 0x30);
            *H(out + 10) = (uint16_t)(S16(pos + 2) - 8);
            *W(out + 4) = argb(*W(colour));
            *H(out) = *H(uv);
            *H(out + 2) = *H(uv + 2);
        }
        textured(rec, tex);
        *W(rec + 12) = flags;
        *W(rec + 16) = *W(list + 12);
        *B(rec + 25) = 0;
        *B(rec + 26) = 0;
        RETURN(CALL3(f_089E44EC, z, rec, 2));
        return;
    }
    gaddr rec = arena(c, 0x1C), out = arena(c, *W(list + 12) << 4);
    uint32_t z = (uint32_t)CVT_TRUNC(c, depth) & 0xFF;
    *W(rec) = out;
    for (uint32_t i = 0; i < *W(list + 12); i++, pos += 4, colour += 4, out += 16) {
        *H(out + 8) = (uint16_t)(S16(pos) + 0x30);
        *H(out + 10) = (uint16_t)(S16(pos + 2) - 8);
        *W(out + 4) = argb(*W(colour));
        *H(out) = 0;
        *H(out + 2) = 0;
    }
    *W(rec + 4) = 0;
    *W(rec + 8) = 0;
    *W(rec + 12) = flags;
    *H(rec + 20) = 0;
    *W(rec + 16) = *W(list + 12);
    *H(rec + 22) = 0;
    *B(rec + 24) = 0; *B(rec + 25) = 0; *B(rec + 26) = 0;
    RETURN(CALL3(f_089E44EC, z, rec, 2));
}
