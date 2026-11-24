/* The text pass's mask layer: a 512 x 224 picture of 16-bit texels at *0x092E22E4 (0x08F58140 in the
 * attract mode), cleared to opaque black (0x8000) each frame by the screen's setup and drawn over the
 * screen as the extra text layer (0x0880F160, object/screen.c). Sprite drawer 14 (0x0880D468) and
 * drawer 13 (0x0880D3B0, still translated) cut holes in it with 0x08811FF4: the attract mode's name
 * rows and fighters' outlines on black show the coloured panels behind them through the holes.
 *
 * 0x08811FF4 (a0 draw state, a1 sprite, a2 picture, a3 its list): each 16 x 16 cell of the picture
 * (3 bytes: a mask number, big-endian, and its flips in bits 7 / 6; 0xFF alone is an empty cell) has a
 * 1-bit mask of 32 bytes (two rows a word, top bit first) in the table 0x092D9D40[sprite + 86]; every
 * set bit clears its texel to 0 (transparent). The texel is the cell's place on the 4:3 screen, rows
 * 1 - 16 below its top (the original's own offset), 1024 bytes a row; a cell is skipped unless its x is
 * in -16 - 383 and its y in -16 - 223, and a texel before the picture's start is never written. The
 * draw state's +32 / +36 / +40 / +68 / +72 are its working values (the first column's x, the steps
 * across and down, the rows and columns left), as in the original.
 *
 * WIDE NATIVE: cells are taken from x -64 to 447, and a texel left of the 4:3 picture (x -48 - -1) goes
 * to the end of its row (texels 464 - 511, unused by the 4:3 picture), where object/screen.c draws it in
 * the left margin; the rest of the row (384 - 447) is drawn in the right margin already. Only the
 * mask layer changes; the fighters' state is the same with WIDE NATIVE on and off.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */

#include "../guest_mem.h"

#define MASK_LAYER   0x092E22E4u   /* u32: the layer's first texel */
#define MASK_TABLES  0x092D9D40u   /* u32[]: the masks by the sprite's picture list (+86) */
#define PICTURE_LISTS 0x08E75AD8u
#define ROW_BYTES    1024
#define ROW_TEXELS   512
enum { DS_X = 0, DS_Y = 2, DS_COL_X = 32, DS_STEP_X = 36, DS_STEP_Y = 40, DS_FLIP_X = 56, DS_FLIP_Y = 60,
       DS_ROWS_LEFT = 68, DS_COLS_LEFT = 72, DS_POS_X = 80, DS_POS_Y = 82 };
enum { SP_FRAME = 28, SP_LIST = 86, SP_PIECES = 156 };
enum { PIC_CELLS = 0, PIC_COLS = 4, PIC_ROWS = 5, PIC_ORIGIN_X = 6, PIC_ORIGIN_Y = 8, PIC_MORE = 10 };

static int cell_taken(int32_t x) {
    return rt_widescreen ? (uint32_t)(x + 64) < 512u : (uint32_t)(x + 16) < 0x190u;
}

/* one cell's mask at x, y (16 x 16; flipped across / up and down as fx / fy say) */
static void mask_cell(gaddr masks, uint32_t number, int32_t x, int32_t y, int fx, int fy) {
    gaddr layer = *W(MASK_LAYER);
    gaddr bits = masks + number * 32u;
    int32_t first = (fy ? y + 15 : y) * ROW_BYTES, step = fy ? -ROW_BYTES : ROW_BYTES;
    uint32_t bit = 0x80000000u;
    gaddr row = layer + (uint32_t)(x * 2) + (uint32_t)first;   /* the original's address arithmetic */
    for (int r = 0; r < 16; r++) {
        row += (uint32_t)step;                                     /* rows 1 - 16 below the top */
        for (int i = 0; i < 16; i++, bit >>= 1) {
            if (!(*W(bits) & bit)) continue;
            int32_t col = fx ? 15 - i : i;
            gaddr a = row + (uint32_t)(col * 2);
            if (rt_widescreen) {
                int32_t tx = x + col;
                if (tx < -48 || tx >= 448) continue;
                if (tx < 0) a += ROW_TEXELS * 2;                   /* left margin: the row's end */
            }
            if (a >= layer) *H(a) = 0;
        }
        if (bit == 0) { bits += 4; bit = 0x80000000u; }
    }
}

static void mask_picture(gaddr ds, gaddr sp, gaddr pic, gaddr list) {
    int32_t cols = *B(pic + PIC_COLS), rows = *B(pic + PIC_ROWS);
    if (cols * rows == 0) return;
    gaddr cell = list + *W(pic + PIC_CELLS);
    uint32_t flip_x = *W(ds + DS_FLIP_X), flip_y = *W(ds + DS_FLIP_Y);
    int32_t x = S16(ds + DS_X), y = S16(ds + DS_Y);
    int32_t ox = S16(pic + PIC_ORIGIN_X), oy = S16(pic + PIC_ORIGIN_Y);
    *W(ds + DS_COL_X) = (uint32_t)(flip_x ? x + ox - (cols * 16 + 16) : x - ox);
    int32_t top = flip_y ? y + oy - (rows * 16 + 16) : y - oy;
    *W(ds + DS_STEP_Y) = 16;
    *W(ds + DS_STEP_X) = flip_x ? (uint32_t)-16 : 16u;
    if (flip_y) { *W(ds + DS_STEP_Y) = (uint32_t)-16; top += rows * 16; }
    gaddr masks = *W(MASK_TABLES + (uint32_t)*H(sp + SP_LIST) * 4u);
    *W(ds + DS_ROWS_LEFT) = (uint32_t)rows;
    do {
        int32_t cx = (int32_t)*W(ds + DS_COL_X);
        if (flip_x) cx += cols * 16;
        *W(ds + DS_COLS_LEFT) = (uint32_t)cols;
        int row_shown = (uint32_t)(top + 16) < 0xF0u;
        do {
            if (*B(cell) == 0xFF) cell += 1;
            else {
                uint32_t number = (uint32_t)*B(cell) << 8 | *B(cell + 1);
                uint8_t attr = *B(cell + 2);
                cell += 3;
                if (cell_taken(cx) && row_shown)
                    mask_cell(masks, number, cx, top, (int)(((attr >> 6) & 1) ^ flip_x), (int)((attr >> 7) ^ flip_y));
            }
            cx += (int32_t)*W(ds + DS_STEP_X);
            *W(ds + DS_COLS_LEFT) -= 1;
        } while (*W(ds + DS_COLS_LEFT) != 0);
        top += (int32_t)*W(ds + DS_STEP_Y);
        *W(ds + DS_ROWS_LEFT) -= 1;
    } while (*W(ds + DS_ROWS_LEFT) != 0);
}

/* drawer 14: the pieces at the sprite's +156 (6 bytes: x, y, picture; bit 15 last, bit 14 not drawn),
   last first, each at the sprite's place plus its x, y; a picture and the ones it goes on into */
static void mask_pieces(gaddr ds, gaddr sp) {
    gaddr list = *W(PICTURE_LISTS + (uint32_t)*H(sp + SP_LIST) * 4u);
    uint32_t frame = *H(*W(sp + SP_FRAME) + 4);
    if (frame & 0x8000) return;
    gaddr e = *W(sp + SP_PIECES) + frame * 6u;
    uint32_t n = 1;
    while (!(*H(e + 4) & 0x8000)) { e += 6; n++; }
    for (; n != 0; n--, e -= 6) {
        uint32_t p = *H(e + 4);
        if (p & 0x4000) continue;
        *H(ds + DS_X) = (uint16_t)(S16(ds + DS_POS_X) + S16(e));
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(e + 2));
        gaddr pic = list + (p & 0x7FFF) * 12u;
        while (S16(pic + PIC_MORE) != 0) { mask_picture(ds, sp, pic, list); pic += 12; }
        mask_picture(ds, sp, pic, list);
    }
}

SFA_FUNC(08811FF4, mask_layer_picture) { c->r[29] -= 0x20; mask_picture(ARG(0), ARG(1), ARG(2), ARG(3)); c->r[29] += 0x20; }
SFA_FUNC(0880D468, sprite_drawer_mask_pieces) { c->r[29] -= 0x30; mask_pieces(ARG(0), ARG(1)); c->r[29] += 0x30; }
