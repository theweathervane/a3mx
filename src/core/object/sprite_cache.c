/* A sprite's picture drawn from the picture cache (docs/SPRITE_FORMAT.md once it exists; for now
 * docs/PSP_PROGRAM_MAP.md, 0x0880E150).
 *
 * A picture (12 bytes in the lists 0x08E75AD8 / 0x092D9D40 / 0x092D9E00) is +0 where its cells start
 * (from a base the caller gives), +4 columns, +5 rows, +6 / +8 its origin. A cell is one byte 0xFF when
 * empty, otherwise three: the tile number (big-endian u16) and a flags byte (bits 0 - 4 the palette,
 * bits 7 / 6 flips, bit 5 blends).
 *
 * The tiles a cell names are not drawn from where they are loaded: they are compressed, so the first
 * time a picture is drawn it is unpacked into pages of the picture cache, and drawn from there while it
 * stays wanted. A page (one of 512 16-byte entries at 0x092E6000) is a 64 x 64 4-bit texture: +0 the
 * picture's next page, +4 the picture's data (its +0), +8 the texture, +12 the sprite's +86 (which tile
 * set), +13 the palette, +14 in use, +15 frames left before it may be freed (0x0880EBF8 counts them
 * down). The first page of each cached picture is also listed at 0x092E8000 (0x0880EB84 searches that).
 *
 * Tile sets: 0x092D9D40[set] is a table of u32 offsets from the table itself, one per 16 x 16 tile.
 * A 16-pixel cell is tile n; a 32-pixel cell (sprite +164 set) is the four tiles at entries 4n - 4n + 3.
 * A tile is LZ-compressed into 128 bytes of 4-bit pixels (0x088D2B64).
 */
#include "../decomp.h"

#include "../guest_mem.h"

void f_0880EB84(Cpu *c);   /* cache_find (pool.c): the cached picture for (draw state, picture, sprite) or 0 */
void f_0880EBF8(Cpu *c);   /* game_sprite_timers (frame.c): frees cache pages no longer wanted */
void f_088054F0(Cpu *c);   /* the frame wait (task.c) */
void f_08810D84(Cpu *c);   /* sprite_grid.c: 16-pixel cells from the cache (draw state, picture, base, page) */
void f_088112A8(Cpu *c);   /* sprite_grid.c: 32-pixel cells from the cache */
void f_088117D0(Cpu *c);   /* sprite_grid.c: 16-pixel cells with their own palette and texel cell */
void f_08811BCC(Cpu *c);   /* sprite_grid.c: 32-pixel cells, the same */
void f_089E155C(Cpu *c);   /* 2D library: a texture's pixels to write (a0 texture), answers their address */
void f_089E1234(Cpu *c);   /* 2D library: a texture written (a0 texture, a1 palette) */

#define CACHE_PAGES   0x092E6000u   /* 512 pages of 16 bytes */
#define CACHE_FIRSTS  0x092E8000u   /* pointers to each cached picture's first page */
#define PAGE_COUNT    512
#define PAGE_BYTES    2048          /* 64 x 64 at 4 bits */
#define TILE_BYTES    128           /* 16 x 16 at 4 bits */
#define TILE_SETS     0x092D9D40u   /* u32[]: a tile set by the sprite's +86 */
#define TILE_PLACES   0x08A042ECu   /* u32[16]: where the i-th tile of a page goes in it (swizzled) */
#define EMPTY         0xFF

/* draw state (0x08AD9320) fields this file uses */
#define DS_PALETTE    84            /* u8 */
#define DS_PAGE_FILL  64            /* u32: cells (16-pixel) or big cells (32-pixel) in the current page */
#define DS_PART       68            /* u32: which quarter of a 32-pixel cell is being unpacked */

/* sprite fields */
#define SP_PALETTE_KIND 12          /* s8: where the palette comes from (below) */
#define SP_PALETTE      13          /* u8 */
#define SP_TILE_SET     86          /* u16: which tile set (low byte kept in the page) */
#define SP_BIG_CELLS    164         /* u32: nonzero = 32-pixel cells */

/* ---- 0x088D2B64: one tile unpacked ----
 *
 * The stream is tokens, eight to a flags byte (bit 0 first). Flag 0: one literal byte. Flag 1: a byte
 * hl: h = 0 repeats the next byte l + 1 times; otherwise l bytes are copied from earlier output, h (1 -
 * 15) picking how far back from a table by where the write is in its row (0x08A3E600 + (dst & 7) * 16:
 * h 1 - 7 reach back along this row, h 8 - 15 into the row above). Rows are 8 bytes (16 pixels) and the
 * write steps by 0x08A3E680[dst & 7]: 1, and 25 after a row's last byte, so rows land 32 bytes apart,
 * as in a page. Stops once 128 bytes are written (a token may write past that). */
#define LZ_STEP 0x08A3E680u         /* u8[8] */
#define LZ_BACK 0x08A3E600u         /* u8[8][16] */

static uint32_t unpack_tile(gaddr src, gaddr dst) {
    const uint8_t *step = B(LZ_STEP), *back = B(LZ_BACK);
    uint32_t written = 0, flags = 0;
    int32_t bits = 0;
    do {
        uint8_t token = *B(src++);
        if (bits == 0) { flags = token; token = *B(src++); bits = 8; }
        if (!(flags & 1)) {                           /* a literal */
            *B(dst) = token; dst += step[dst & 7];
            written++;
        } else {
            uint32_t h = token >> 4, n = token & 0xF;
            if (h == 0) {                             /* a run of the next byte */
                uint8_t v = *B(src++);
                n += 1; written += n;
                while (n--) { *B(dst) = v; dst += step[dst & 7]; }
            } else {                                  /* a copy from earlier output */
                gaddr from = dst - back[(dst & 7) * 16 + h];
                written += n;
                while (n--) { *B(dst) = *B(from); dst += step[dst & 7]; from += step[from & 7]; }
            }
        }
        flags = (flags >> 1) & 0xFF;
        bits--;
    } while (written < TILE_BYTES);
    return (uint32_t)bits;
}

SFA_FUNC(088D2B64, sprite_unpack_tile) { RETURN(unpack_tile(ARG(0), ARG(1))); }

/* ---- 0x0880E9E0: pages for a picture ----
 *
 * Counts the picture's non-empty cells from `cells` (the caller passes the base, as the original does)
 * and takes enough free pages for them, chained by +0. The first is listed in the first free slot of
 * 0x092E8000 and wanted for 8 frames. With too few free pages it frees the unwanted ones (0x0880EBF8)
 * and goes on looking, keeping what it already has, up to seven passes; then it waits for ever.
 * The count looks at one cell more than the picture has (columns x rows + 1), as the original does. */
static gaddr take_pages(Cpu *c, gaddr ds, gaddr pic, gaddr cells, gaddr sprite) {
    int32_t n = *B(pic + 4) * *B(pic + 5), used = 0;
    for (;; n--) {
        if (*B(cells) != EMPTY) { cells += 2; used++; }
        cells++;
        if (n == 0) break;
    }
    int32_t bytes = used << (*W(sprite + SP_BIG_CELLS) ? 9 : 7);
    int32_t left = bytes >> 11;                       /* pages, rounded up as the original rounds */
    if ((bytes >= 0 ? bytes & 0x7FF : -((-bytes) & 0x7FF)) != 0) left++;

    gaddr first = 0, last = 0;
    for (int passes = 7;;) {
        gaddr p = CACHE_PAGES;
        for (int i = PAGE_COUNT; i != 0; i--) {
            if (*B(p + 14) == 0) {
                *W(p + 4) = *W(pic);
                *B(p + 12) = (uint8_t)*H(sprite + SP_TILE_SET);
                *B(p + 13) = *B(ds + DS_PALETTE);
                *B(p + 14) = 1;
                if (last == 0) {
                    gaddr slot = CACHE_FIRSTS;
                    while (*W(slot) != 0) slot += 4;
                    *W(slot) = p;
                    first = last = p;
                    *B(p + 15) = 8;
                } else {
                    *W(last) = p;
                    last = p;
                }
                left--;
            }
            if (left == 0) return first;
            p += 16;
        }
        if (--passes == 0) for (;;) CALL1(f_088054F0, 1);
        CALL0(f_0880EBF8);
    }
}

SFA_FUNC(0880E9E0, sprite_cache_take) { RETURN(take_pages(c, ARG(0), ARG(1), ARG(2), ARG(3))); }

/* Moving on to the picture's next page once one is full: not for its last cell, nor without a next
   page. Answers the new page's pixels, or 0 to keep writing where it was. */
static gaddr next_page(Cpu *c, gaddr *page, int32_t cells_left) {
    if (cells_left == 1 || *page == 0 || *W(*page) == 0) return 0;
    CALL2(f_089E1234, *W(*page + 8), *B(*page + 13));
    *page = *W(*page);
    return CALL1(f_089E155C, *W(*page + 8));
}

/* ---- 0x0880EE34: a picture's 16-pixel cells unpacked into its pages, 16 to a page ---- */
static void fill_small(Cpu *c, gaddr ds, gaddr pic, gaddr base, gaddr page) {
    gaddr cell = base + *W(pic);
    int32_t n = *B(pic + 4) * *B(pic + 5);
    gaddr pixels = CALL1(f_089E155C, *W(page + 8)), to = pixels;
    gaddr set = *W(TILE_SETS + *B(page + 12) * 4u);
    *W(ds + DS_PAGE_FILL) = 0;
    do {
        if (*B(cell) == EMPTY) { cell += 1; n--; continue; }
        uint32_t tile = (uint32_t)*B(cell) << 8 | *B(cell + 1);
        gaddr place = *W(TILE_PLACES + (((to - pixels) >> 7) & 0xFF) * 4u);
        unpack_tile(set + *W(set + tile * 4), pixels + place);
        cell += 3;
        to += TILE_BYTES;
        int32_t fill = (int32_t)++*W(ds + DS_PAGE_FILL);
        if (fill >= 16) {
            gaddr np = next_page(c, &page, n);
            if (np) { pixels = to = np; *W(ds + DS_PAGE_FILL) = 0; }
        }
        n--;
    } while (n != 0);
    CALL2(f_089E1234, *W(page + 8), *B(page + 13));
}

SFA_FUNC_WRAPPED(0880EE34, sprite_cache_fill_small, anthology_fill_small) { fill_small(c, ARG(0), ARG(1), ARG(2), ARG(3)); }

/* ---- 0x0880EFB8: a picture's 32-pixel cells unpacked into its pages, 4 to a page ---- */
static void fill_big(Cpu *c, gaddr ds, gaddr pic, gaddr base, gaddr page) {
    gaddr cell = base + *W(pic);
    int32_t n = *B(pic + 4) * *B(pic + 5);
    gaddr pixels = CALL1(f_089E155C, *W(page + 8)), to = pixels;
    gaddr set = *W(TILE_SETS + *B(page + 12) * 4u);
    *W(ds + DS_PAGE_FILL) = 0;
    do {
        if (*B(cell) == EMPTY) { cell += 1; n--; continue; }
        gaddr quarters = set + (((uint32_t)*B(cell) << 8 | *B(cell + 1)) << 4);
        cell += 3;
        *W(ds + DS_PART) = 0;
        int32_t k = 0;
        do {
            gaddr place = *W(TILE_PLACES + (((to - pixels) >> 7) & 0xFF) * 4u);
            unpack_tile(set + *W(quarters + (uint32_t)k * 4u), pixels + place);
            to += TILE_BYTES;
            k = (int32_t)*W(ds + DS_PART) + 1;
            *W(ds + DS_PART) = (uint32_t)k;
        } while (k < 4);
        int32_t fill = (int32_t)++*W(ds + DS_PAGE_FILL);
        if (fill >= 4) {
            gaddr np = next_page(c, &page, n);
            if (np) { pixels = to = np; *W(ds + DS_PAGE_FILL) = 0; }
        }
        n--;
    } while (n != 0);
    CALL2(f_089E1234, *W(page + 8), *B(page + 13));
}

/* (anthology.c fills its own pictures' cells itself) */
SFA_FUNC_WRAPPED(0880EFB8, sprite_cache_fill_big, anthology_fill_big) { fill_big(c, ARG(0), ARG(1), ARG(2), ARG(3)); }

/* ---- 0x0880E150: a sprite's picture drawn as a grid of cells ----
 *
 * (draw state, sprite, picture, base, own): nothing for a picture of no cells. With `own` set the
 * cells carry their own palette and texel cell (0x088117D0, or 0x08811BCC for a sprite with 32-pixel
 * cells). Otherwise the draw state's palette is set by the sprite's +12 and the picture drawn from the
 * cache, cached first if it isn't:
 *   +12 = 0    the first non-empty cell's palette
 *   +12 = 1    the sprite's +13
 *   +12 >= 2   left as it is
 *   +12 = -1   the first non-empty cell's palette plus the sprite's +13
 *   +12 = -2   the sprite's +13
 *   +12 < -2   left as it is */
static uint8_t first_cell_palette(gaddr cells) {
    while (*B(cells) == EMPTY) cells++;
    return *B(cells + 2) & 0x1F;
}

static void draw_picture(Cpu *c, gaddr ds, gaddr sprite, gaddr pic, gaddr base, uint32_t own) {
    if (*B(pic + 4) * *B(pic + 5) == 0) return;
    if (own & 0xFF) {
        if (sprite != 0 && *W(sprite + SP_BIG_CELLS) != 0) CALL3(f_08811BCC, ds, pic, base);
        else CALL3(f_088117D0, ds, pic, base);
        return;
    }
    int8_t kind = S8(sprite + SP_PALETTE_KIND);
    if (kind == 0) *B(ds + DS_PALETTE) = first_cell_palette(base + *W(pic));
    else if (kind == 1 || kind == -2) *B(ds + DS_PALETTE) = *B(sprite + SP_PALETTE);
    else if (kind == -1) *B(ds + DS_PALETTE) = (uint8_t)(first_cell_palette(base + *W(pic)) + *B(sprite + SP_PALETTE));

    int big = *W(sprite + SP_BIG_CELLS) != 0;
    gaddr page = CALL3(f_0880EB84, ds, pic, sprite);
    if (page == 0) {
        page = CALL4(f_0880E9E0, ds, pic, base, sprite);
        CALL4(big ? f_0880EFB8 : f_0880EE34, ds, pic, base, page);
    }
    CALL4(big ? f_088112A8 : f_08810D84, ds, pic, base, page);
}

/* (anthology.c can give T. Hawk's walk-back pictures from Hyper Street Fighter Alpha instead) */
SFA_FUNC_WRAPPED(0880E150, sprite_draw_picture, anthology_draw_picture) { draw_picture(c, ARG(0), ARG(1), ARG(2), ARG(3), c->r[8]); }
