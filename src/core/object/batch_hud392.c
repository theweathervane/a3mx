/* The fight's status display laid out: every piece of it (the bars, the names, the win marks,
   the gauges and their levels, for both sides) gets its place, layer and picture. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define F1     0x092E3EA0u
#define F2     0x092E4338u
#define F3     0x092E47D0u

void f_0880AEA8(Cpu *c); void f_0888F0EC(Cpu *c); void f_0888FF38(Cpu *c); void f_0888D8BC(Cpu *c);

/* The display's pieces are 0x40 bytes each from +0x34 of the display: +0 whether it is a picture
   (1) or a box (0), +2 and +54 its palette and a flag, +6 and +10 its place, +14 and +26 a box's
   size (+12 and +24 its start), +36.. its colours, +56 its layer, +60 its picture. */
#define PIECE(o, n) ((o) + 0x34u + 0x40u * (uint32_t)(n))

static void place(gaddr p, uint8_t kind, uint8_t layer, uint32_t x, uint32_t y) {
    *B(p) = kind; *B(p + 56) = layer; *H(p + 6) = (uint16_t)x; *H(p + 10) = (uint16_t)y;
}
static void box(gaddr p, uint8_t layer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    place(p, 0, layer, x, y);
    *H(p + 12) = 0; *H(p + 24) = 0; *H(p + 14) = (uint16_t)w; *H(p + 26) = (uint16_t)h;
}
static void pal(gaddr p, uint8_t palette) { *B(p + 2) = palette; *B(p + 54) = 0x10; }

/* A box's colours from a 15-bit colour: each part as a byte, the same at all four corners. */
static void tint(gaddr p, uint32_t colour) {
    uint8_t r = (uint8_t)((colour & 0x7C00) >> 8), g = (uint8_t)((colour & 0x3E0) >> 3);
    uint8_t b = (uint8_t)((colour & 0x1F) << 2);
    *B(p + 48) = r; *B(p + 42) = g; *B(p + 36) = b;
    *B(p + 39) = b; *B(p + 38) = b; *B(p + 37) = b;
    *B(p + 45) = g; *B(p + 44) = g; *B(p + 43) = g;
    *B(p + 51) = r; *B(p + 50) = r; *B(p + 49) = r;
}

/* A win mark's picture from how the round was won (0 for none: the empty mark 0xF3): the kind
   in the low four bits through the table at 0x08A26D1C, 10 further for bit 6 and 20 for bit
   5. */
static void win_mark(gaddr p, int32_t how) {
    if (how == 0) { *H(p + 60) = 0xF3; return; }
    *H(p + 60) = *H(0x08A26D1Cu + 2 * ((uint32_t)how & 0xF));
    if (how & 0x40) *H(p + 60) = (uint16_t)(*H(p + 60) + 0xA);
    if (how & 0x20) *H(p + 60) = (uint16_t)(*H(p + 60) + 0x14);
}

/* One digit of the level in a fighter's gauge record (+784, when it is in use): the picture
   0x16E plus the level and one, either its tens or what is left over. */
static void level_digit(Cpu *c, gaddr p, gaddr fighter, int tens) {
    gaddr gauge = *W(fighter + 784);
    if (gauge == 0 || S8(gauge) == 0) return;
    *H(p + 60) = 0x16E;
    DIV(c, *B(*W(fighter + 784) + 4) + 1u, 10);
    *H(p + 60) = (uint16_t)(*H(p + 60) + (tens ? c->lo : c->hi));
}

/* The fighter's state for the display's tables: 0x10 further in its extra state (+309), 0x18
   with +308, 8 when its gauge record is in use. */
static uint32_t state_offset(gaddr fighter) {
    if (S8(fighter + 309) != 0) return 0x10;
    if (S8(fighter + 308) != 0) return 0x18;
    gaddr gauge = *W(fighter + 784);
    return (gauge != 0 && S8(gauge) != 0) ? 8 : 0;
}

/* The style a side shows (0, 1 or 2 from the sign of the fighter's +386: below 0, 0, above),
   kept at `at`. */
static void style_from(gaddr at, int32_t v) {
    *B(at) = v == 0 ? 1 : v > 0 ? 2 : 0;
}

/* A sheet for one side: the table's entry for the style picks which of the sheets (0x20 bytes
   each, the second set 0x440 further on) is loaded as `number`. */
static void load_sheet(Cpu *c, uint32_t entry, int second, uint32_t number, uint32_t back) {
    uint32_t off = entry << 5;
    if (second) {
        off += 0x440;
        off += (uint32_t)((int32_t)off >> 2) >> 30;
        off = (uint32_t)((int32_t)off >> 2) << 2;
    }
    ARG(1) = *W(0x092D9E40u) + off;
    ARG(0) = 0; ARG(2) = 0; ARG(3) = number; c->r[8] = 1;
    c->r[31] = back; f_0880AEA8(c);
}

/* (o): the status display's step that lays everything out. The count at +2 goes up. The two
   fighters shown are chosen by the kind of fight (the battle's +56): the first two, the first
   and third, or the third and second; in kind 3 the battle's own choice (+136) with the first or
   second. Each side's style (+16 and +36) comes from its fighter's +386, 0 when its +371 is 2,
   and 3 (the old one kept at +17 and +37) when its gauge record is in use; the four sheets for
   the two styles are loaded, and the display's three routines run. Then every piece is placed,
   those that hang from the top 0x28 or 0x10 or 0x1D less the display's drop (+4, set to 0x30 when
   the battle's +2 is 0), those at the bottom that much more: the frame and the two names, the
   two life bars with their colours, the timer, each side's win marks (not in game mode 2; the
   second and third only for fights of that many rounds), its gauge with its level, its style's
   own pieces, and its portrait or its mark. Last, in game mode 6 (or 0x10 with the battle's
   +159 at 2, or with the game's +137 set and the link's +904 at 2) the win marks not yet earned
   are hidden. A 0x70 frame keeping s0..s8. */
SFA_FUNC(0888DB10, status_display_layout) {
    gaddr o = ARG(0);
    gaddr a = ARG(3), b = c->r[8];
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x70;
    gaddr sp = c->r[29];
    *W(sp + 60) = sv[0];
    c->r[16] = o;
    *B(o + 2) = (uint8_t)(*B(o + 2) + 1);
    *W(sp + 32) = o + 0x74;
    int32_t kind = S16(BATTLE + 56);
    *W(sp + 44) = GAME;
    *W(sp + 64) = sv[1]; *W(sp + 68) = sv[2]; *W(sp + 72) = sv[3]; *W(sp + 76) = sv[4];
    *W(sp + 80) = sv[5]; *W(sp + 84) = sv[6]; *W(sp + 88) = sv[7]; *W(sp + 92) = sv[8];
    c->r[23] = 2; c->r[21] = 1; c->r[18] = 4; c->r[20] = 0xC0; c->r[22] = 0x28;
    c->r[30] = o + 0xB4;
    c->r[17] = 0x10; c->r[19] = 5;
    *W(sp + 96) = sv[9];
    *W(sp + 36) = 0x08A26CA8u;
    int32_t v;
    if (kind >= 0 && kind < 4) {
        if (kind == 0) { a = F1; b = F2; }
        else if (kind == 1) { a = F1; b = F3; }
        else if (kind == 2) { a = F3; b = F2; }
        else {
            a = *W(BATTLE + 136);
            if (S8(a + 337) != 2) { a = F1; b = F3; }
            else b = S8(F1 + 373) == 0 ? F1 : F2;
        }
        v = S8(a + 386);
    } else {
        if (kind >= 4) a = 0x092E0000u;
        v = S8(a + 386);
    }
    style_from(o + 16, v);
    style_from(o + 36, S8(b + 386));
    if (S8(a + 371) == 2) *B(o + 16) = 0;
    int32_t style1 = S8(o + 16);
    if (S8(b + 371) == 2) *B(o + 36) = 0;
    if (S16(BATTLE + 2) == 0) *H(o + 4) = 0x30;
    gaddr gauge = *W(a + 784);
    if (gauge != 0 && S8(gauge) != 0) {
        *B(o + 17) = (uint8_t)style1;
        *B(o + 16) = 3;
        style1 = S8(o + 16);
    }
    gauge = *W(b + 784);
    if (gauge != 0 && S8(gauge) != 0) {
        *B(o + 37) = *B(o + 36);
        *B(o + 36) = 3;
    }
    *W(sp + 56) = 2;
    *W(sp + 52) = 0x10;
    *W(sp + 48) = 5;
    c->r[19] = 0x08A26D36u;
    *W(sp + 40) = BATTLE;
    c->r[17] = 0x092D9E00u;
    load_sheet(c, *B(0x08A26D36u + (uint32_t)style1), 0, 0xF, 0x0888DD4Cu);
    c->r[23] = 0x08A26D3Au;
    load_sheet(c, *B(0x08A26D3Au + (uint32_t)S8(o + 16)), 1, 0x10, 0x0888DD98u);
    load_sheet(c, *B(0x08A26D36u + (uint32_t)S8(o + 36)), 0, 0x11, 0x0888DDC4u);
    load_sheet(c, *B(0x08A26D3Au + (uint32_t)S8(o + 36)), 1, 0x12, 0x0888DE08u);
    ARG(0) = o; c->r[31] = 0x0888DE10u; f_0888F0EC(c);
    ARG(0) = o; c->r[31] = 0x0888DE18u; f_0888FF38(c);
    ARG(0) = o; c->r[31] = 0x0888DE20u; f_0888D8BC(c);

    gaddr p1 = *W(o + 12), p2 = *W(o + 32);
    uint32_t drop = (uint32_t)S16(o + 4);
    uint32_t top = 0x28 - drop, mark_y = 0x10 - drop, name_y = 0x1D - drop;
    uint32_t low = drop + 0xE0, mid = drop + 0xD0, bar_y = drop + 0x1D;
    int32_t game_mode = S8(GAME + 114);
    int32_t rounds = S8(BATTLE + 8);
    c->r[31] = GAME;
    c->r[19] = 5; c->r[17] = 0x10; c->r[23] = 2;

    /* The frame and the two names. */
    place(PIECE(o, 0), 1, 4, 0xC0, top);
    *H(PIECE(o, 0) + 60) = 0xBE;
    place(PIECE(o, 1), 1, 4, 0xBA, name_y);
    place(PIECE(o, 2), 1, 4, 0xC6, name_y);
    if (S8(GAME + 47) == 3) {
        *H(PIECE(o, 1) + 6) = 0xC0;
        *H(PIECE(o, 1) + 60) = 0xCA;
        *H(PIECE(o, 2) + 60) = 0x8000;
    }
    /* The timer's box, the first side's life bar and what lies under it. */
    box(PIECE(o, 3), 4, 0xBE, mid, 4, 6);
    box(PIECE(o, 9), 4, 0x20, low, 0x90, 8);
    tint(PIECE(o, 9), *H(0x08A26CA8u + 2 * (uint32_t)S8(o + 16)));
    box(PIECE(o, 6), 4, 0xB0, low, 0, 8);
    box(PIECE(o, 7), 4, 0xB0, low, 0, 8);
    place(PIECE(o, 8), 1, 4, 0xC0, top);
    *B(PIECE(o, 8) + 2) = 0x10;
    *H(PIECE(o, 8) + 60) = 0x179;
    *B(PIECE(o, 8) + 54) = 0x10;
    place(PIECE(o, 10), 1, 4, 0xC0, top);
    *B(PIECE(o, 10) + 54) = 0x10;
    place(PIECE(o, 11), 1, 4, 0xC0, top);

    /* The first side's win marks. */
    if (game_mode != 2) {
        gaddr m = PIECE(o, 12);
        place(m, 1, 4, 0xA8, mark_y);
        int32_t how = S8(p1 + 378);
        win_mark(m, how);
        if (how != 0 && S8(p2 + 309) != 0 && *H(m + 60) == 0xCC) *H(m + 60) = 0xF3;
        m = PIECE(o, 13);
        place(m, 1, 4, 0x98, mark_y);
        if (rounds >= 2 && S8(p2 + 309) == 0) win_mark(m, S8(p1 + 379));
        m = PIECE(o, 14);
        place(m, 1, 4, 0x88, mark_y);
        if (rounds >= 3 && S8(p2 + 309) == 0) win_mark(m, S8(p1 + 380));
    }
    box(PIECE(o, 15), 5, 0xC0, bar_y, 0, 6);

    /* The first side's gauge. */
    if (S8(p1 + 313) == 0) {
        if (S8(p1 + 309) == 0 && S8(p1 + 308) == 0) {
            gaddr d = PIECE(o, 4);
            place(d, 0, 5, 0x52, mid);
            pal(d, 0x10);
            level_digit(c, d, p1, 0);
            d = PIECE(o, 5);
            place(d, 0, 5, 0x4C, mid);
            pal(d, 0x10);
            level_digit(c, d, p1, 1);
        }
        gaddr g = PIECE(o, 16);
        place(g, 0, 5, 0xC0, low);
        pal(g, 0x10);
        if (S8(p1 + 371) == 2) *H(g + 60) = 0x1D8;
        else {
            uint32_t n = ((uint32_t)S8(o + 16) << 1) + state_offset(p1);
            *H(g + 60) = *H(0x08A26CCCu + 2 * n);
        }
        g = PIECE(o, 17);
        place(g, 0, 5, 0xC0, low);
        *B(g + 2) = 0xF;
        *B(g + 54) = 0x10;
        *H(g + 60) = *H(0x08A26D0Cu + 4 * (uint32_t)S8(o + 16));
        if (S8(o + 16) >= 2) *H(g + 60) = (uint16_t)(*H(g + 60) + (uint32_t)S16(p1 + 366));
        place(PIECE(o, 18), 0, 5, 0x68, low);
        g = PIECE(o, 19);
        *B(g) = 0; *B(g + 56) = 5; *H(g + 6) = 0xC0;
        *B(g + 2) = 0xF;
        if (S8(o + 16) != 1) *B(g + 54) = 0x10;
        if (S8(o + 16) != 0) *H(g + 6) = (uint16_t)((uint32_t)S16(g + 6) - 0xB0);
        *H(g + 10) = (uint16_t)low;
        int32_t style = S8(o + 16);
        if (style == 1) {
            box(PIECE(o, 20), 5, 0xC0, bar_y, 0, 3);
            box(PIECE(o, 21), 5, 0xC0, bar_y, 0, 5);
            box(PIECE(o, 22), 5, 0xC0, bar_y, 0, 7);
        } else if (style == 2) {
            static const uint16_t xs[3] = { 6, 0x10, 0x20 };
            for (int i = 0; i < 3; i++) {
                place(PIECE(o, 20 + i), 0, 5, xs[i], low);
                pal(PIECE(o, 20 + i), 0xF);
            }
            g = PIECE(o, 23);
            place(g, 0, 5, 0xB1, drop + 0xE6);
            pal(g, 0xF);
            *H(g + 60) = 0x324;
        } else if (style == 3) {
            g = PIECE(o, 20);
            place(g, 0, 5, 0xB1, drop + 0xE6);
            pal(g, 0xF);
            *H(g + 60) = 0x34B;
        }
    }
    /* The first side's portrait, or its mark. */
    if (S16(BATTLE + 56) == 0 && S8(p1 + 313) == 0 && (*W(p1 + 792) & 0x00820010u) == 0) {
        gaddr g = PIECE(o, 24);
        place(g, 1, 4, 0xC0, top);
        pal(g, 0x10);
        *H(g + 60) = (uint16_t)((uint32_t)S8(p1 + 556) + 0x110);
        g = PIECE(o, 26);
        place(g, 1, 4, 0xC0, top);
        pal(g, 0xF);
        *H(g + 60) = (uint16_t)((uint32_t)S8(p1 + 556) + 0x59);
        g = PIECE(o, 27);
        place(g, 1, 4, 0xAA - (uint32_t)S8(p1 + 556), top);
        *H(g + 60) = 0xBB;
        place(PIECE(o, 28), 1, 4, 0xC0, top);
    } else {
        gaddr g = PIECE(o, 25);
        place(g, 1, 4, 0xC0, top);
        pal(g, 0x10);
        *H(g + 60) = 0x16C;
    }

    /* The second side's life bar and what lies under it. */
    box(PIECE(o, 33), 4, 0xD0, low, 0x90, 8);
    tint(PIECE(o, 33), *H(0x08A26CA8u + 2 * (uint32_t)S8(o + 36)));
    box(PIECE(o, 31), 4, 0xD0, low, 0, 8);
    box(PIECE(o, 32), 4, 0xD0, low, 0, 8);
    place(PIECE(o, 34), 1, 4, 0xC0, top);
    pal(PIECE(o, 34), 0x12);
    *H(PIECE(o, 34) + 60) = 0x17A;
    place(PIECE(o, 35), 1, 4, 0xC0, top);
    *B(PIECE(o, 35) + 54) = 0x10;
    place(PIECE(o, 36), 1, 4, 0xC0, top);
    c->r[30] = 6;

    /* The second side's win marks. */
    if (game_mode != 2) {
        gaddr m = PIECE(o, 37);
        place(m, 1, 4, 0xD8, mark_y);
        int32_t how = S8(p2 + 378);
        win_mark(m, how);
        if (how != 0 && S8(p1 + 309) != 0 && *H(m + 60) == 0xCC) *H(m + 60) = 0xF3;
        m = PIECE(o, 38);
        place(m, 1, 4, 0xE8, mark_y);
        if (rounds >= 2 && S8(p1 + 309) == 0) win_mark(m, S8(p2 + 379));
        m = PIECE(o, 39);
        place(m, 1, 4, 0xF8, mark_y);
        if (rounds >= 3 && S8(p1 + 309) == 0) win_mark(m, S8(p2 + 380));
    }

    /* The second side's gauge. */
    if (S8(p2 + 313) == 0) {
        if (S8(p2 + 309) == 0 && S8(p2 + 308) == 0) {
            gaddr d = PIECE(o, 29);
            place(d, 0, 5, 0x17A, mid);
            pal(d, 0x12);
            level_digit(c, d, p2, 0);
            d = PIECE(o, 30);
            place(d, 0, 5, 0x174, mid);
            pal(d, 0x12);
            level_digit(c, d, p2, 1);
        }
        box(PIECE(o, 40), 5, 0xC0, bar_y, 0, 6);
        gaddr g = PIECE(o, 41);
        place(g, 0, 5, 0xC0, low);
        pal(g, 0x12);
        if (S8(p2 + 371) == 2) *H(g + 60) = 0x1DF;
        else {
            uint32_t n = ((uint32_t)S8(o + 36) << 1) + 1 + state_offset(p2);
            *H(g + 60) = *H(0x08A26CCCu + 2 * n);
        }
        g = PIECE(o, 42);
        place(g, 0, 5, 0xC0, low);
        *B(g + 2) = 0x11;
        *B(g + 54) = 0x10;
        *H(g + 60) = *H(0x08A26D0Cu + 4 * (uint32_t)S8(o + 36) + 2);
        if (S8(o + 36) >= 2) *H(g + 60) = (uint16_t)(*H(g + 60) + (uint32_t)S16(p2 + 366));
        place(PIECE(o, 43), 0, 5, 0x118, low);
        g = PIECE(o, 44);
        *B(g) = 0; *B(g + 56) = 5; *H(g + 6) = 0xC0;
        *B(g + 2) = 0x11;
        if (S8(o + 36) != 1) *B(g + 54) = 0x10;
        if (S8(o + 36) != 0) *H(g + 6) = (uint16_t)((uint32_t)S16(g + 6) + 0xB0);
        *H(g + 10) = (uint16_t)low;
        int32_t style = S8(o + 36);
        if (style == 1) {
            box(PIECE(o, 45), 5, 0xCD, bar_y, 0, 3);
            box(PIECE(o, 46), 5, 0xFD, bar_y, 0, 5);
            box(PIECE(o, 47), 5, 0x12D, bar_y, 0, 7);
        } else if (style == 2) {
            static const uint16_t xs[3] = { 0x166, 0x170, 0x180 };
            for (int i = 0; i < 3; i++) {
                place(PIECE(o, 45 + i), 0, 5, xs[i], low);
                pal(PIECE(o, 45 + i), 0x11);
            }
            g = PIECE(o, 48);
            place(g, 0, 5, 0xD0, drop + 0xE6);
            pal(g, 0x11);
            *H(g + 60) = 0x324;
        } else if (style == 3) {
            g = PIECE(o, 45);
            place(g, 0, 5, 0xD0, drop + 0xE6);
            pal(g, 0x11);
            *H(g + 60) = 0x34B;
        }
    }
    /* The second side's portrait, or its mark. */
    if (S16(BATTLE + 56) == 0 && S8(p2 + 313) == 0 && (*W(p2 + 792) & 0x00820010u) == 0) {
        gaddr g = PIECE(o, 49);
        place(g, 1, 4, 0xC0, top);
        pal(g, 0x12);
        *H(g + 60) = (uint16_t)((uint32_t)S8(p2 + 556) + 0xB2);
        g = PIECE(o, 51);
        place(g, 1, 4, 0xC0, top);
        pal(g, 0x11);
        *H(g + 60) = (uint16_t)S8(p2 + 556);
        g = PIECE(o, 52);
        place(g, 1, 4, (uint32_t)S8(p2 + 556) + 0xD6, top);
        *H(g + 60) = 0x5D;
        place(PIECE(o, 53), 1, 4, 0xC0, top);
    } else {
        gaddr g = PIECE(o, 50);
        place(g, 1, 4, 0xC0, top);
        pal(g, 0x12);
        *H(g + 60) = 0x10E;
    }

    /* The win marks not yet earned are hidden where the fight is for a set number of wins. */
    game_mode = S8(GAME + 114);
    int hide = game_mode == 6 || (game_mode == 0x10 && S8(BATTLE + 159) == 2);
    if (!hide) hide = S8(GAME + 137) != 0 && S8(0x09517A78u + 904) == 2;
    if (hide) {
        if (S8(0x09517A50u) < 1) *H(PIECE(o, 13) + 60) = 0x8000;
        if (S8(0x09517A50u) < 2) *H(PIECE(o, 14) + 60) = 0x8000;
        if (S8(0x09517A28u) < 1) *H(PIECE(o, 38) + 60) = 0x8000;
        if (S8(0x09517A28u) < 2) *H(PIECE(o, 39) + 60) = 0x8000;
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9];
    c->r[29] += 0x70;
}
