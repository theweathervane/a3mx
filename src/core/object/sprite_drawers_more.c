/* More of the sprite drawers (0x08A04B48[sprite +148]; sprite_drawers.c has 0, 1 / 2, 3, 6 and 11,
 * mask_layer.c 13 and 14), and a fighter's extra picture. Each draws through 0x0880E150
 * (sprite_cache.c), after copying the sprite's +170 / +171 to the draw state's +86 / +87.
 *
 *   drawer 4  (0x0880C7D8): a row of pictures 16 pixels apart, to the right (or left, facing +11)
 *   drawer 5  (0x0880C930): pieces of 8 bytes, each with its own palette, drawn last first
 *   drawer 7  (0x0880CC08): as drawer 0, the cells' own palettes, from the texture list 0x08E75A18[+86]
 *   drawers 8 and 9 (0x0880CCEC): as drawers 1 and 2, the same way
 *   drawer 10 (0x0880CE74): as drawer 3 (the picture number at +84), the same way
 *   drawer 12 (0x0880D0F4): as drawer 6 (pieces from +156), each placed by 0x0880D214 (scaled and turned),
 *                           one picture each (no chain)
 *
 * None of them has anything of WIDE NATIVE's: what they draw is cut where the original cuts it.
 */
#include "../decomp.h"

#include "../guest_mem.h"

void f_0880E150(Cpu *c);   /* sprite_cache.c: a picture as a grid of cells (draw state, sprite, picture, list, own palettes) */
void f_089E0CF4(Cpu *c);   /* 2D library: a texture list in use (a0) */
void f_0880D214(Cpu *c);   /* sprite.c: a part's offset scaled and turned, onto the draw state's place */
void f_0880E6E8(Cpu *c);   /* sprite.c: a sprite's anchor */

#define DRAW          0x08AD9320u   /* the draw state */
#define PICTURE_LISTS 0x08E75AD8u   /* u32[]: picture lists by the sprite's +86 */
#define TEXTURE_LISTS 0x08E75A18u   /* u32[]: texture lists by the sprite's +86 */
#define PIECE_TABLES  0x092E3E98u   /* u32[2]: piece tables for drawers 1 / 2 and 8 / 9 */

enum { SP_FACING = 11, SP_FRAME = 28, SP_PICTURE_84 = 84, SP_LIST = 86, SP_DRAWER = 148, SP_PIECES = 156,
       SP_EFFECT = 170, SP_SCALED = 171 };
enum { DS_X = 0, DS_Y = 2, DS_EFFECT = 86, DS_SCALED = 87, DS_POS_X = 80, DS_POS_Y = 82 };
enum { LAST = 0x8000, SKIP = 0x4000, PIC_MORE = 10, PIC_SIZE = 12 };

static uint32_t sprite_frame(gaddr sp) { return *H(*W(sp + SP_FRAME) + 4); }
static gaddr sprite_list(gaddr sp) { return *W(PICTURE_LISTS + (uint32_t)*H(sp + SP_LIST) * 4u); }
static void copy_effects(gaddr ds, gaddr sp) {
    *B(ds + DS_EFFECT) = *B(sp + SP_EFFECT);
    *B(ds + DS_SCALED) = *B(sp + SP_SCALED);
}
static void use_textures(Cpu *c, gaddr sp) { CALL1(f_089E0CF4, *W(TEXTURE_LISTS + (uint32_t)*H(sp + SP_LIST) * 4u)); }

static void draw(Cpu *c, gaddr ds, gaddr sp, gaddr pic, gaddr list, uint32_t own) {
    CALL5(f_0880E150, ds, sp, pic, list, own);
}
/* a picture and the ones it goes on into (+10 nonzero) */
static void draw_chain(Cpu *c, gaddr ds, gaddr sp, gaddr pic, gaddr list, uint32_t own) {
    while (S16(pic + PIC_MORE) != 0) { draw(c, ds, sp, pic, list, own); pic += PIC_SIZE; }
    draw(c, ds, sp, pic, list, own);
}
/* the last of a set of pieces `size` bytes apart (+4 bit 15), and how many there are */
static gaddr last_piece(gaddr e, uint32_t size, int32_t *count) {
    int32_t n = 0;
    while (!(*H(e + 4) & LAST)) { e += size; n++; }
    *count = n + 1;
    return e;
}

/* ---- drawer 4: u16 entries from +156 at the frame (bit 15 the last), each a picture of the list,
   drawn while on screen (x -31 - 415; the last -15 - 399), the next 16 to the right (left if facing) */
static void draw_row(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    gaddr e = *W(sp + SP_PIECES) + sprite_frame(sp) * 2u;
    copy_effects(ds, sp);
    int32_t x = S16(ds + DS_X);
    for (;;) {
        uint16_t v = *H(e);
        int last = (v & LAST) != 0;
        gaddr pic = list + (v & 0x7FFF) * (uint32_t)PIC_SIZE;
        if (last ? x >= -15 && x < 400 : x >= -31 && x < 416) {
            draw(c, ds, sp, pic, list, 0);
            x = S16(ds + DS_X);
        }
        *H(ds + DS_X) = (uint16_t)(x + (*B(sp + SP_FACING) ? -16 : 16));
        if (last) return;
        x = S16(ds + DS_X);
        e += 2;
    }
}
SFA_FUNC(0880C7D8, sprite_drawer_row) { draw_row(c, ARG(0), ARG(1)); }

/* ---- drawer 5: 8-byte pieces from +156 at the frame (+0 x, +2 y, +4 picture: bit 15 the last, bit 14
   not drawn; +6 the palette's kind, +7 the palette), drawn last first at the sprite's place plus x, y.
   Each goes through a stand-in sprite on the guest stack: +12 / +13 from the piece, +86 and +164
   (32-pixel cells) the sprite's */
#define STAND_IN_FRAME 0x100
#define STAND_IN       0x20
static void draw_palette_pieces(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint16_t set = *H(sp + SP_LIST);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    int32_t n;
    gaddr e = last_piece(*W(sp + SP_PIECES) + frame * 8u, 8, &n);
    c->r[29] -= STAND_IN_FRAME;
    gaddr me = c->r[29] + STAND_IN;
    *B(ds + DS_EFFECT) = *B(sp + SP_EFFECT);
    *H(me + SP_LIST) = set;
    *W(me + 164) = *W(sp + 164);
    *B(ds + DS_SCALED) = *B(sp + SP_SCALED);
    for (; n != 0; n--, e -= 8) {
        uint16_t v = *H(e + 4);
        if (v & SKIP) continue;
        *H(ds + DS_X) = (uint16_t)(S16(ds + DS_POS_X) + S16(e));
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(e + 2));
        *B(me + 12) = *B(e + 6);
        *B(me + 13) = *B(e + 7);
        draw_chain(c, ds, me, list + (v & 0x7FFF) * (uint32_t)PIC_SIZE, list, 0);
    }
    c->r[29] += STAND_IN_FRAME;
}
SFA_FUNC(0880C930, sprite_drawer_palette_pieces) { draw_palette_pieces(c, ARG(0), ARG(1)); }

/* ---- drawer 7: the frame's picture (and those it goes on into), the cells' own palettes */
static void draw_one_own(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    use_textures(c, sp);
    copy_effects(ds, sp);
    draw_chain(c, ds, sp, list + frame * PIC_SIZE, list, 1);
}
SFA_FUNC(0880CC08, sprite_drawer_one_own) { draw_one_own(c, ARG(0), ARG(1)); }

/* ---- drawers 8 / 9: 6-byte pieces (+0 x, +2 y, +4 picture) from 0x092E3E98[drawer - 8] at the frame,
   drawn last first at the sprite's place plus x, y, the cells' own palettes */
static void draw_pieces_own(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    use_textures(c, sp);
    gaddr table = *W(PIECE_TABLES + *W(sp + SP_DRAWER) * 4u - 32u);
    int32_t n;
    gaddr e = last_piece(table + frame * 6u, 6, &n);
    copy_effects(ds, sp);
    for (; n != 0; n--, e -= 6) {
        uint16_t v = *H(e + 4);
        if (v & SKIP) continue;
        *H(ds + DS_X) = (uint16_t)(S16(ds + DS_POS_X) + S16(e));
        *H(ds + DS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(e + 2));
        draw_chain(c, ds, sp, list + (v & 0x7FFF) * (uint32_t)PIC_SIZE, list, 1);
    }
}
SFA_FUNC(0880CCEC, sprite_drawer_pieces_own) { draw_pieces_own(c, ARG(0), ARG(1)); }

/* ---- drawer 10: the picture numbered at +84 (bit 15: none), the cells' own palettes */
static void draw_numbered_own(Cpu *c, gaddr ds, gaddr sp) {
    if (*H(sp + SP_PICTURE_84) & 0x8000) return;
    use_textures(c, sp);
    gaddr list = sprite_list(sp);
    gaddr pic = list + *H(sp + SP_PICTURE_84) * (uint32_t)PIC_SIZE;
    copy_effects(ds, sp);
    draw_chain(c, ds, sp, pic, list, 1);
}
SFA_FUNC(0880CE74, sprite_drawer_numbered_own) { draw_numbered_own(c, ARG(0), ARG(1)); }

/* ---- drawer 12: 6-byte pieces from +156 at the frame, drawn last first, each placed by 0x0880D214
   (the piece's offset scaled and turned by the sprite), one picture each */
static void draw_turned_pieces(Cpu *c, gaddr ds, gaddr sp) {
    gaddr list = sprite_list(sp);
    uint32_t frame = sprite_frame(sp);
    if (frame & 0x8000) return;
    int32_t n;
    gaddr e = last_piece(*W(sp + SP_PIECES) + frame * 6u, 6, &n);
    for (; n != 0; n--, e -= 6) {
        uint16_t v = *H(e + 4);
        if (v & SKIP) continue;
        gaddr pic = list + (v & 0x7FFF) * (uint32_t)PIC_SIZE;
        CALL4(f_0880D214, ds, sp, pic, e);
        copy_effects(ds, sp);
        draw(c, ds, sp, pic, list, 0);
    }
}
SFA_FUNC(0880D0F4, sprite_drawer_turned_pieces) { draw_turned_pieces(c, ARG(0), ARG(1)); }

/* ---- 0x0880D68C: a fighter's extra picture. The fighter's animation frame (+28, its +8) picks a
   12-byte record in the character's table (+416, its +16); a nonzero +6 there picks a pair in the
   table at +416's +32 (s16 entry of 0x08A03D60, s16 x offset). The entry's +4 is a picture of list 5
   (bit 15: none), drawn at the fighter's place (+18, 240 - +132, anchored) moved by the offset (back,
   facing), flipped by +11, colour 0xC0000000, unblended, depth +24; through a stand-in sprite on the
   guest stack: palette kind 1, palette 0x20, list 5, 32-pixel cells. Nothing while +0 or +1 is clear
   or +264's low byte is set. */
#define EXTRA_ENTRIES 0x08A03D60u   /* 8 bytes each, +4 the picture */
#define EXTRA_FRAME   0xF0
static void fighter_extra(Cpu *c, gaddr f) {
    gaddr ds = DRAW;
    if (*B(f) == 0 || *B(f + 1) == 0 || (S16(f + 264) & 0xFF) != 0) return;
    gaddr rec = *W(*W(f + 416) + 16) + *H(*W(f + SP_FRAME) + 8) * (uint32_t)PIC_SIZE;
    if (*B(rec + 6) == 0) return;
    *H(ds + DS_POS_X) = *H(f + 18);
    *H(ds + DS_POS_Y) = (uint16_t)(0xF0 - S16(f + 132));
    CALL2(f_0880E6E8, ds, f);
    *H(ds + DS_X) = *H(ds + DS_POS_X);
    *H(ds + DS_Y) = *H(ds + DS_POS_Y);
    gaddr pair = *W(*W(f + 416) + 32) + (((uint32_t)*B(rec + 6) * 2u) & 0xFFFF) * 2u;
    gaddr entry = EXTRA_ENTRIES + (uint32_t)(int32_t)S16(pair) * 8u;
    int32_t x = S16(ds + DS_X), dx = S16(pair + 2);
    *H(ds + DS_X) = (uint16_t)(*B(f + SP_FACING) ? x - dx : x + dx);
    uint16_t v = *H(entry + 4);
    gaddr list = *W(PICTURE_LISTS + 5 * 4);
    if (v & 0x8000) return;
    *W(ds + 56) = *B(f + SP_FACING) & 1;
    *W(ds + 60) = *B(f + SP_FACING) >> 1;
    *W(ds + 48) = 0xC0000000u;
    *B(ds + 85) = 0;
    *W(ds + 44) = *H(f + 24);
    c->r[29] -= EXTRA_FRAME;
    gaddr me = c->r[29] + STAND_IN;
    *B(me + 12) = 1;
    *B(me + 13) = 0x20;
    *H(me + SP_LIST) = 5;
    *W(me + 164) = 1;
    copy_effects(ds, f);
    draw(c, ds, me, list + v * (uint32_t)PIC_SIZE, list, 0);
    c->r[29] += EXTRA_FRAME;
}
SFA_FUNC(0880D68C, fighter_extra_picture) { fighter_extra(c, ARG(0)); }
