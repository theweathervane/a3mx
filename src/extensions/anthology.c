/* Imports from Hyper Street Fighter Alpha (Street Fighter Alpha Anthology, PS2): the first, a proof of
 * the way (docs/HYPER_ALPHA.md, "T. Hawk's walk back"):
 *
 * T.HAWK WALKBACK (OPTION MODE > EX. SETTING > EX. ANTHOLOGY). The PSP never had T. Hawk's walk-back art: his
 * walk back (pl23's animation 3 of set 1) shows pictures 51 down to 40, and those are copies of the walk
 * forward's 28 - 39 played in reverse. Hyper Street Fighter Alpha has twelve pictures of its own for it.
 * tools/anthology_hawk.py (the launcher does it when the Anthology disc is chosen on the Paths tab)
 * takes them from the user's own disc into assets/<region>/anthology/hawk_walkback/pictures/0040.png -
 * 0051.png: 160 x 160, 8-bit indexed (index = colour of palette 0, 0 transparent), the sprite's origin
 * at 80, 141, exactly as the PSP's pictures 40 - 51 (5 x 5 cells of 32).
 *
 * In the game nothing in the archive is changed. When a fighter sprite with 32-pixel cells is drawn
 * (0x0880E150, sprite_cache.c) from T. Hawk's picture list (known by a CRC of its first 52 pictures)
 * and the picture is one of 40 - 51, it is drawn from a picture of ours instead: the same columns,
 * rows and origin, cells of our own whose 32 x 32 pixels are ours (0x0880EFB8, the cache fill, copies
 * them where it would unpack the archive's tiles). Our pictures' cell offsets have bit 31 set (the
 * list is given as our block less 0x80000000), so the picture cache, which goes by the offset, never
 * mixes them up with the archive's; turning the setting off or on shows at the next frame.
 * Without the files the option is greyed (OFF) and nothing changes. */
#include "rt.h"
#include "ext_features.h"
#include "png_decode.h"
#include <stdio.h>
#include <string.h>
#include <stdio.h>

void f_089E155C(Cpu *c);   /* 2D library: a texture's pixels to write (a0 texture), answers their address */
void f_089E1234(Cpu *c);   /* 2D library: a texture written (a0 texture, a1 palette) */
extern char rt_assets_root[1024];

int rt_hawk_walkback = 0;                  /* EX. SETTING: T.HAWK WALKBACK (settings.c keeps it; off by default: experimental) */

#define FIRST       40                     /* pictures 40 - 51 */
#define COUNT       12
#define PICS_KNOWN  52                     /* the CRC covers the list's first 52 pictures (12 bytes each) */
#define HAWK_CRC    0x2AEBAD8Fu            /* pl23_u.pac (US) */
#define COLS        5
#define ROWS        5
#define CELL        32
#define SIZE        (COLS * CELL)          /* 160 */
#define TILE_BYTES  128
#define TILE_PLACES 0x08A042ECu            /* u32[16]: where the i-th tile of a cache page goes in it */
#define DS_PAGE_FILL 64                    /* draw state: big cells in the current page */
#define DS_PART     68                     /* draw state: the quarter being written */
#define SP_BIG_CELLS 164
#define BIAS        0x80000000u

static int state;                          /* 0 not looked yet, 1 loaded, -1 no files */
static uint32_t block;                     /* our pictures (12 x 12 bytes), their cells, their tiles */
static uint32_t pics, tiles;               /* within block */
static uint8_t pixels[COUNT][SIZE * SIZE]; /* the PNGs, colour indices */

static uint32_t crc32_of(const uint8_t *p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    while (n--) { c ^= *p++; for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1))); }
    return ~c;
}

static int load_pngs(void) {
    if (!rt_assets_root[0]) return 0;                      /* --no-assets, the tests */
    for (int k = 0; k < COUNT; k++) {
        char path[1200], err[160];
        snprintf(path, sizeof path, "%s/anthology/hawk_walkback/pictures/%04d.png", rt_assets_root, FIRST + k);
        PngImage img;
        if (png_decode_file(path, &img, err, sizeof err) != 0) {
            if (k == 0) rt_log("anthology: no T. Hawk walk back (%s): T.HAWK WALKBACK greyed", path);
            else rt_warn("anthology: %s: %s", path, err);
            return 0;
        }
        int ok = img.w == SIZE && img.h == SIZE && img.index;
        if (ok) for (int i = 0; i < SIZE * SIZE; i++) pixels[k][i] = img.index[i] & 15;
        png_free(&img);
        if (!ok) { rt_warn("anthology: %s: not a %d x %d indexed PNG", path, SIZE, SIZE); return 0; }
    }
    return 1;
}

/* Our block: the pictures, then for each its cells (0xFF, or the cell number big-endian and the
   flags), then four 16 x 16 tiles for each cell (top left, bottom left, top right, bottom right), as
   unpacked tiles are: 8 bytes a row, the left pixel in the low nibble. */
static int build(void) {
    int cells = 0;
    for (int k = 0; k < COUNT; k++)
        for (int i = 0; i < COLS * ROWS; i++) {
            int cx = (i % COLS) * CELL, cy = (i / COLS) * CELL, any = 0;
            for (int y = 0; y < CELL && !any; y++) for (int x = 0; x < CELL; x++) if (pixels[k][(cy + y) * SIZE + cx + x]) { any = 1; break; }
            cells += any;
        }
    uint32_t pic_bytes = 12 * COUNT, cell_bytes = (uint32_t)(COUNT * COLS * ROWS * 3 + 16) & ~3u;
    uint32_t size = pic_bytes + cell_bytes + (uint32_t)cells * 4u * TILE_BYTES;
    if (!(block = rt_alloc(size, 1, 16, "anthology: T. Hawk walk back"))) return 0;
    pics = block; tiles = block + pic_bytes + cell_bytes;
    uint32_t at = block + pic_bytes; int id = 0;
    for (int k = 0; k < COUNT; k++) {
        uint32_t p = pics + 12u * (uint32_t)k;
        WW(p, BIAS + (at - block)); WB(p + 4, COLS); WB(p + 5, ROWS); WH(p + 6, SIZE / 2); WH(p + 8, 141); WH(p + 10, 0);
        for (int i = 0; i < COLS * ROWS; i++) {
            int cx = (i % COLS) * CELL, cy = (i / COLS) * CELL, any = 0;
            for (int y = 0; y < CELL && !any; y++) for (int x = 0; x < CELL; x++) if (pixels[k][(cy + y) * SIZE + cx + x]) { any = 1; break; }
            if (!any) { WB(at++, 0xFF); continue; }
            WB(at, (uint32_t)id >> 8); WB(at + 1, (uint32_t)id & 0xFF); WB(at + 2, 0x20);   /* as the archive's cells: palette 0, 0x20 */
            at += 3;
            for (int q = 0; q < 4; q++) {                  /* TL, BL, TR, BR */
                uint32_t t = tiles + ((uint32_t)id * 4u + (uint32_t)q) * TILE_BYTES;
                int qx = cx + (q >= 2 ? 16 : 0), qy = cy + (q & 1 ? 16 : 0);
                for (int y = 0; y < 16; y++)
                    for (int x = 0; x < 16; x += 2)
                        WB(t + (uint32_t)(y * 8 + x / 2), (uint32_t)(pixels[k][(qy + y) * SIZE + qx + x] | pixels[k][(qy + y) * SIZE + qx + x + 1] << 4));
            }
            id++;
        }
    }
    rt_log("anthology: T. Hawk walk back loaded (%d pictures, %d cells)", COUNT, cells);
    return 1;
}

int anthology_hawk_available(void) {
    if (state == 0) state = load_pngs() && build() ? 1 : -1;
    return state == 1;
}

/* Whether the disc has been imported, for EX. SETTING's rows: the twelve files are there. Nothing is
   read or decoded and nothing is taken from the game's memory (that waits for the extension to be used
   with ANTHOLOGY on: anthology_hawk_available). Answered once, as anthology_hawk_available is. */
int anthology_files_present(void) {
    static int present;                    /* 0 not looked yet, 1 there, -1 not */
    if (state) return state == 1;          /* already loaded (or tried and failed) */
    if (present == 0) {
        present = rt_assets_root[0] ? 1 : -1;
        for (int k = 0; k < COUNT && present == 1; k++) {
            char path[1200];
            snprintf(path, sizeof path, "%s/anthology/hawk_walkback/pictures/%04d.png", rt_assets_root, FIRST + k);
            FILE *f = fopen(path, "rb");
            if (f) fclose(f); else present = -1;
        }
    }
    return present == 1;
}

/* ---- 0x0880E150 (a0 draw state, a1 sprite, a2 picture, a3 list): ours for T. Hawk's 40 - 51 ---- */
int anthology_draw_picture(Cpu *c, uint32_t ds, int after) {
    (void)ds;
    uint32_t sprite = c->r[5], pic = c->r[6], list = c->r[7];
    int anthology_select_tile(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst);   /* anthology_select.c: the MODE carousel */
    if (!after && anthology_select_tile(c, ds, sprite, pic, list)) return 1;
    if (after || !rt_hawk_walkback || !list || !sprite || pic < list + 12u * FIRST || pic >= list + 12u * (FIRST + COUNT)) return 0;
    if ((pic - list) % 12u || !RW(sprite + SP_BIG_CELLS) || !anthology_hawk_available()) return 0;
    if (crc32_of(HOST(list), 12u * PICS_KNOWN) != HAWK_CRC) return 0;
    c->r[6] = pics + (pic - list - 12u * FIRST);
    c->r[7] = block - BIAS;
    return 0;                                              /* the game draws it, from ours */
}

/* ---- 0x0880EE34 (a0 draw state, a1 picture, a2 list, a3 first page): 16-pixel cells, for the S-ISM name of
   anthology_select.c ---- */
int anthology_fill_small(Cpu *c, uint32_t ds, int after) {
    int anthology_select_fill_small(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page);
    return !after && anthology_select_fill_small(c, ds, c->r[5], c->r[6], c->r[7]);
}

/* ---- 0x0880EFB8 (a0 draw state, a1 picture, a2 list, a3 first page): our cells into the cache ---- */
static uint32_t call1(Cpu *c, void (*f)(Cpu *), uint32_t a0, uint32_t a1) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = a0; c->r[5] = a1; f(c);
    uint32_t v = c->r[2];
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    return v;
}
int anthology_fill_big(Cpu *c, uint32_t ds, int after) {
    uint32_t pic = c->r[5], base = c->r[6], page = c->r[7];
    if (after || state != 1 || pic < pics || pic >= pics + 12u * COUNT) return 0;
    uint32_t cell = base + RW(pic);
    int32_t n = (int32_t)(RB(pic + 4) * RB(pic + 5));
    uint32_t px = call1(c, f_089E155C, RW(page + 8), 0), to = px;
    WW(ds + DS_PAGE_FILL, 0);
    while (n != 0) {
        if (RB(cell) == 0xFF) { cell++; n--; continue; }
        uint32_t id = RB(cell) << 8 | RB(cell + 1);
        cell += 3;
        for (uint32_t q = 0; q < 4; q++) {
            uint32_t place = RW(TILE_PLACES + (((to - px) >> 7) & 0xFF) * 4u);
            for (uint32_t y = 0; y < 16; y++) memcpy(HOST(px + place + y * 32u), HOST(tiles + (id * 4u + q) * TILE_BYTES + y * 8u), 8);
            to += TILE_BYTES;
            WW(ds + DS_PART, q + 1);
        }
        uint32_t fill = RW(ds + DS_PAGE_FILL) + 1; WW(ds + DS_PAGE_FILL, fill);
        if (fill >= 4 && n != 1 && page && RW(page)) {     /* on to the picture's next page */
            call1(c, f_089E1234, RW(page + 8), RB(page + 13));
            page = RW(page);
            px = to = call1(c, f_089E155C, RW(page + 8), 0);
            WW(ds + DS_PAGE_FILL, 0);
        }
        n--;
    }
    call1(c, f_089E1234, RW(page + 8), RB(page + 13));
    return 1;                                              /* done: not the archive's tiles */
}
