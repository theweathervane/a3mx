/* I-ISM display (the i_ism_display feature): what the game left out on screen for the entry characters
 * (EDIT / ENTRY, shown as I-ISM): their wipe into the fight and their ranking in the attract mode.
 * Picking an entry for the other side is i_ism_select.c (the i_ism_select feature).
 *
 * 1. The wipe into the fight. Between the VS screen and the fight three columns roll like a slot
 *    machine and settle into the ISM's letter, X, A or V, in its colour, and the STREET FIGHTER ALPHA3
 *    logo slides in with that ISM's gradient (task/object: screen_steps.c's logo_start_step). An entry
 *    plays as its ISM underneath (an A-ISM entry got A's wipe), so I-ISM had none of its own. Now an
 *    entry gets an I: the same rolling columns (A's animation) with I shapes in yellow, and a gold logo.
 *    The I was measured from Dan's WIDE NATIVE mock-up: a serifed I across the 4:3 picture (its serifs
 *    from x 0 to 384, their points on the 4:3 edges as X's are, 32 high, 8 in from the top and the bottom as X's are; its stem 160 - 224, as wide as the
 *    VS screen's middle bar behind it (the maintainer, 2026-11-27)), the
 *    yellow around it; the margins of WIDE NATIVE are filled by shapes.c's rule as for the other
 *    letters. NORMAL and WIDE draw the same shapes.
 *
 * The same wipe plays after a win (the single object's type 0x0E instead of 0x0D, the same start), and
 * gets the I there too. Not in World Tour, where an entry's ISM is the one chosen to level up: there
 * it keeps that ISM's wipe, as the game has it.
 *
 * The wipe is for one fighter, as the game picks it (logo_ism in screen_steps.c): fighter 3 when BATTLE
 * + 56 is 3, fighter 2 when BATTLE + 48 is set, else fighter 1. It plays an entry when its player (+337,
 * 0 or 1) has one in use (0x09358528 + 0x354 n: the entry a player plays as), as the VS screen's ISM
 * label decides (0x08898F5C shows I-ISM, label 0x147, from the same record).
 */
#include "rt.h"
#include "ext_features.h"
#include <string.h>

void f_0880E434(Cpu *c);
void shapes_draw(Cpu *c, uint32_t place, const uint16_t *quads, uint32_t rgb, uint32_t alpha);   /* object/shapes.c */

#define BATTLE        0x092E1228u
#define FIGHTER(k)    (0x092E3EA0u + 0x498u * (uint32_t)(k))
#define ENTRIES       0x09358528u       /* the entry each player plays as (+0 in use) */
#define ENTRY_SIZE    0x354u
#define SINGLE        0x092E5100u       /* the logo's object: +0 in use, +3 its type */
#define LOGO_TYPE     0x0D       /* into the fight; 0x0E after a win */
#define WIN_LOGO_TYPE 0x0E
#define PALETTES      0x092DF980u       /* 16 colours (u16) a palette */
#define PALETTES_DIRTY 0x092E0980u
#define LOGO_PALETTE  0x1Eu

/* ---- the colours ---- */

/* The wipe's yellow (0xRRGGBB, as the game's own green 0x00D000), from the mock-ups: #F3E40C */
#define I_YELLOW      0xF3E40Cu

/* The logo's gold, for A's green palette entry by entry. X's, A's and V's gradients run from nearly
   black (STREET's fill: #210000, #002000, #000031) to the wipe's own colour in the middle (the H of
   ALPHA), so the gold one runs from a dark brown #311800 through #C87B01 (Dan's mock-up) to the wipe's
   yellow #F3E40C (5 - 15). 4 is the letters' outline, a bright contrasting hue as theirs are (orange,
   yellow-green, cyan): magenta #B53184, from the World Tour map's NEXT STAGE bar; 2 and 3 are its
   corner pixels, in the outline's own hue as theirs are (X's are a darker orange, V's a darker
   cyan): purple #943184 and pink #C64273, Dan's picks from the same bar; 1 is the yellow; 0 clear. */
static const uint8_t GOLD[16][3] = {
    { 0x00, 0x00, 0x00 }, { 0xF3, 0xE4, 0x0C }, { 0x94, 0x31, 0x84 }, { 0xC6, 0x42, 0x73 },
    { 0xB5, 0x31, 0x84 }, { 0x31, 0x18, 0x00 }, { 0x4A, 0x28, 0x00 }, { 0x63, 0x39, 0x00 },
    { 0x7C, 0x4A, 0x00 }, { 0x96, 0x5A, 0x01 }, { 0xAF, 0x6A, 0x01 }, { 0xC8, 0x7B, 0x01 },
    { 0xD3, 0x95, 0x04 }, { 0xDE, 0xB0, 0x06 }, { 0xE8, 0xCA, 0x09 }, { 0xF3, 0xE4, 0x0C },
};

/* ---- the I ---- */

/* Quads (4 corners: top left, top right, bottom right, bottom left) of each of the three columns, 128
   wide and 256 high like the other letters' (object/shapes.c), in yellow; what they leave open is the
   I. At rest a column's y 16 is the screen's top: like X-ISM's X, the I leaves a band of 8 at the top
   and the bottom (24 and 232), its serifs 32 high (to 56, from 200) and its stem between. */
#define BAND_TOP    0, 0, 128, 0, 128, 24, 0, 24
#define BAND_BOTTOM 0, 232, 128, 232, 128, 256, 0, 256
#define MIDDLE      0, 56, 128, 56, 128, 200, 0, 200
static const uint16_t I_LEFT[] = {
    BAND_TOP, 0, 24, 0, 24, 64, 56, 0, 56, MIDDLE, 0, 200, 64, 200, 0, 232, 0, 232, BAND_BOTTOM, 0xFFFF };
static const uint16_t I_MIDDLE[] = {
    BAND_TOP, 0, 56, 32, 56, 32, 200, 0, 200, 96, 56, 128, 56, 128, 200, 96, 200, BAND_BOTTOM, 0xFFFF };
static const uint16_t I_RIGHT[] = {
    BAND_TOP, 128, 24, 128, 24, 128, 56, 64, 56, MIDDLE, 64, 200, 128, 200, 128, 232, 128, 232, BAND_BOTTOM, 0xFFFF };
static const uint16_t *const I_SHAPES[3] = { I_LEFT, I_MIDDLE, I_RIGHT };

/* ---- which fighter ---- */

#define GAME_MODE     (0x08E75910u + 0x72)
#define MODE_WORLD_TOUR 3
static uint32_t wipe_fighter(void) {
    return (int16_t)RH(BATTLE + 56) == 3 ? FIGHTER(2) : (int8_t)RB(BATTLE + 48) != 0 ? FIGHTER(1) : FIGHTER(0);
}
static int wipe_is_entry(void) {
    if ((int8_t)RB(GAME_MODE) == MODE_WORLD_TOUR) return 0;     /* there the ISM is chosen to level up */
    uint32_t f = wipe_fighter();
    int player = (int8_t)RB(f + 337);
    if ((int8_t)RB(f + 438) == 3) return 0;      /* CLASSIC: X-ISM's red wipe and logo, as any CLASSIC (the maintainer, 2026-11-25) */
    return player >= 0 && player < 2 && RB(ENTRIES + ENTRY_SIZE * (uint32_t)player) != 0;
}

static uint16_t rgb555(const uint8_t c[3]) {
    return (uint16_t)(0x8000u | (uint32_t)(c[0] >> 3) | (uint32_t)(c[1] >> 3) << 5 | (uint32_t)(c[2] >> 3) << 10);
}

/* ---- the S-ISMs (with the anthology's S-ISM SELECT) ----
 * The maintainer's scheme (2026-11-25): an S-ISM's wipe has a giant S, open like the X, A, V and I
 * (no outline), in another ISM's colours, and that ISM's logo: S-ISM A (red) X-ISM's, S-ISM B (blue)
 * V-ISM's, S-ISM C (green) A-ISM's, S-ISM D (pink) X-ISM's turned pink, its logo a pink gradient.
 * Two transitions, two switches (the maintainer, 2026-11-27): the I-ISM's (an entry's I and gold logo)
 * with Extra ISM Presentation (FEAT_I_ISM_DISPLAY, EX. DISPLAY), the S-ISMs' (not in the PSP game) with
 * EX. ANTHOLOGY's S-ISM PRESENTATION (rt_sism_presentation, with ANTHOLOGY on and an S-ISM chosen),
 * neither depending on the other. Without S-ISM PRESENTATION an S-ISM gets the wipe of the ISM it plays
 * as, as the game draws it. The wrappers below are on FEAT_ANY ("ism_presentation_") and check each
 * part's own switch. */
typedef struct { int version, sism, guard; } AnthologyChoice;
AnthologyChoice anthology_fighter_choice(uint32_t me);         /* anthology_select.c */
int anthology_select_available(void);
extern int rt_sism_presentation;                                /* anthology_select.c */
enum { S_NONE, S_GREEN, S_BLUE, S_PINK, S_RED };
static const int8_t SISM_SCHEME[5] = { 0, 0, 1, -1, -1 };       /* the ISM whose wipe it borrows: C A, B V, D X, A X */
#define S_PINK_RGB 0xE749BEu                                    /* (as I_YELLOW: 0xRRGGBB) the hot pink of the logo's H (the maintainer's sample, 2026-11-26) */
static const uint8_t PINK[16][3] = {
    { 0x00, 0x00, 0x00 }, { 0xE0, 0x48, 0xB8 }, { 0x84, 0x31, 0x94 }, { 0xA5, 0x42, 0xB5 },
    { 0xF8, 0xC8, 0xF0 }, { 0x29, 0x00, 0x21 }, { 0x42, 0x08, 0x35 }, { 0x5A, 0x10, 0x4A },
    { 0x73, 0x18, 0x5E }, { 0x8C, 0x20, 0x73 }, { 0xA5, 0x29, 0x88 }, { 0xBD, 0x31, 0x9C },
    { 0xCE, 0x3A, 0xA8 }, { 0xD6, 0x40, 0xB0 }, { 0xDE, 0x44, 0xB4 }, { 0xE0, 0x48, 0xB8 },
};
static int wipe_sism(void) {    /* the S-ISM transition's switch: S-ISM PRESENTATION, ANTHOLOGY on (and its files), an S-ISM chosen */
    if (!rt_sism_presentation || !FEATURE(ANTHOLOGY) || (int8_t)RB(GAME_MODE) == MODE_WORLD_TOUR || !anthology_select_available()) return S_NONE;
    int s = anthology_fighter_choice(wipe_fighter()).sism;
    return s >= S_GREEN && s <= S_RED ? s : S_NONE;
}

/* The S: three columns of quads, 128 x 256 each like the letters', what they leave open the S (the
   maintainer's pick, 2026-11-28): a stroke 48 wide along a line through these points, so its two bends
   are rounded, its diagonal as steep as the Japanese version's Z-ISM wipe (measured from its 4:3
   picture: 7 pixels across for every 6 rows of the 272-row screen, 7 for every 5 of the game's 224
   rows), and its open ends cut at the diagonal's slope mirrored, to points on the 4:3 picture's
   corners, as the I's serifs reach the sides: the top bar's right end from x 384 on its top row
   (row 8) slanting left 7 per 5 rows, the bottom bar's left end from x 0 on its bottom row (215)
   slanting right likewise. Rows are the column's from y 16 (the screen's top row).
   Sampled every row at its middle, each row's open part one quad (lengthened down while the next row's
   is the same), so the diagonal's edges step 7 pixels per 5 rows, whole pixels (0.175.1's middle,
   sampled every 2 rows, stepped 9 or 10 pixels at a time). */
#define S_TOP 32                            /* the bars' middles: the S's y 48 and 208 */
#define S_BOT 192
#define S_HALF 24
#define S_RUN 7
#define S_RISE 5
#define S_X0 (192 - (S_BOT - S_TOP) * S_RUN / S_RISE / 2)
static const int16_t S_LINE[][2] = {
    { 384, S_TOP }, { S_X0, S_TOP }, { S_X0 + (S_BOT - S_TOP) * S_RUN / S_RISE, S_BOT }, { 0, S_BOT } };
#define S_QUADS 4096
static uint16_t s_cols[3][S_QUADS];
static int s_made;
static int in_s(int x, int row) {
    if (row < S_TOP - S_HALF || row >= S_BOT + S_HALF) return 0;
    if (row < S_TOP + S_HALF && x >= 384 - (row - (S_TOP - S_HALF)) * S_RUN / S_RISE) return 0;       /* the top bar's end */
    if (row >= S_BOT - S_HALF && x < ((S_BOT + S_HALF - 1) - row) * S_RUN / S_RISE) return 0;          /* the bottom bar's */
    float px = (float)x + 0.5f, py = (float)row + 0.5f;
    for (size_t i = 0; i + 1 < sizeof S_LINE / sizeof S_LINE[0]; i++) {
        float ax = S_LINE[i][0], ay = S_LINE[i][1], bx = S_LINE[i + 1][0], by = S_LINE[i + 1][1];
        float dx = bx - ax, dy = by - ay;
        float t = (px - ax) * dx;
        t = t + (py - ay) * dy;
        float l = dx * dx;
        l = l + dy * dy;
        t = t / l; if (t < 0) t = 0; if (t > 1) t = 1;
        float ex = ax + t * dx - px, ey = ay + t * dy - py;
        float d = ex * ex;
        d = d + ey * ey;
        if (d <= (float)(S_HALF * S_HALF)) return 1;
    }
    return 0;
}
static void make_s(void) {
    for (int col = 0; col < 3; col++) {
        uint16_t *q = s_cols[col]; int n = 0, prev_to = 0;
        for (int y0 = 0; y0 < 256; y0++) {
            int runs[16][2], nr = 0, x = 0;
            while (x < 128 && nr < 16) {
                while (x < 128 && in_s(col * 128 + x, y0 - 16)) x++;
                if (x >= 128) break;
                int a = x;
                while (x < 128 && !in_s(col * 128 + x, y0 - 16)) x++;
                runs[nr][0] = a; runs[nr][1] = x; nr++;
            }
            for (int r = 0; r < nr; r++) {
                int merged = 0;
                for (int k = 0; k < prev_to; k += 8)     /* the same run in the row above: lengthened */
                    if (q[k] == runs[r][0] && q[k + 2] == runs[r][1] && q[k + 5] == y0) { q[k + 5] = q[k + 7] = (uint16_t)(y0 + 1); merged = 1; break; }
                if (merged || n + 8 >= S_QUADS - 9) continue;
                uint16_t v[8] = { (uint16_t)runs[r][0], (uint16_t)y0, (uint16_t)runs[r][1], (uint16_t)y0,
                                  (uint16_t)runs[r][1], (uint16_t)(y0 + 1), (uint16_t)runs[r][0], (uint16_t)(y0 + 1) };
                memcpy(q + n, v, sizeof v); n += 8;
            }
            prev_to = n;
        }
        if (col != 1) {                     /* WIDE NATIVE: an empty quad on the 4:3 edge, the column's height, so
                                               shapes.c fills the margin beside it in the colour even where the
                                               S's ends meet the edge */
            uint16_t e = col == 0 ? 0 : 128;
            uint16_t v[8] = { e, 0, e, 0, e, 256, e, 256 };
            memcpy(q + n, v, sizeof v); n += 8;
        }
        q[n] = 0xFFFF;
    }
    s_made = 1;
}

/* The logo starts (screen_steps.c, 0x08885A44). The S-ISM transition (S-ISM PRESENTATION): the wipe that of
   the ISM the S-ISM borrows (the fighter's ISM changed during the call, put back after it), S-ISM D's
   logo pink. The I-ISM transition (Extra ISM Presentation): an entry's palette gold, after the game has
   loaded the ISM's. Neither: nothing. */
static int8_t kept_ism; static uint32_t kept_at;
int ism_presentation_logo_start(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    if (after && kept_at) { WB(kept_at, (uint8_t)kept_ism); kept_at = 0; }
    int s = wipe_sism();
    if (!after) {
        kept_at = 0;
        if (s == S_NONE) return 0;
        uint32_t f = wipe_fighter();
        kept_at = f + 386; kept_ism = (int8_t)RB(kept_at);
        WB(kept_at, (uint8_t)SISM_SCHEME[s]);
        return 0;
    }
    if (s != S_NONE) {
        if (s == S_PINK) {
            for (uint32_t i = 1; i < 16; i++) WH(PALETTES + LOGO_PALETTE * 32u + i * 2u, rgb555(PINK[i]));
            WW(PALETTES_DIRTY, 1);
        }
        rt_log("anthology: an S-ISM's wipe (%d): the S", s);
        return 0;
    }
    if (!FEATURE(I_ISM_DISPLAY) || !wipe_is_entry()) return 0;
    for (uint32_t i = 1; i < 16; i++) WH(PALETTES + LOGO_PALETTE * 32u + i * 2u, rgb555(GOLD[i]));
    WW(PALETTES_DIRTY, 1);
    rt_log("i_ism_display: an entry's wipe: the I and the gold logo");
    return 0;
}

/* A fading shape of the logo's wipe (batch_large.c, 0x0880DA78): while an S-ISM's is drawn (S-ISM PRESENTATION)
   the borrowed ISM's shapes are drawn as the S, while an entry's is (Extra ISM Presentation) A's shapes
   (3 - 5) as the I, by ism_presentation_shape. */
static int drawing_entry_wipe, drawing_sism;
int ism_presentation_fading_shape(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    int logo = !after && RB(SINGLE) && (RB(SINGLE + 3) == LOGO_TYPE || RB(SINGLE + 3) == WIN_LOGO_TYPE);
    drawing_sism = logo ? wipe_sism() : S_NONE;
    drawing_entry_wipe = logo && drawing_sism == S_NONE && FEATURE(I_ISM_DISPLAY) && wipe_is_entry();
    return 0;
}

/* A shape drawn (shapes.c, 0x0880E434: a0 its place, a1 the shape, a2 the alpha). */
int ism_presentation_shape(Cpu *c, uint32_t a0, int after) {
    uint32_t shape = c->r[5], alpha = c->r[6];
    if (!after && drawing_sism != S_NONE) {
        uint32_t base = (uint32_t)(SISM_SCHEME[drawing_sism] + 1) * 3;   /* the borrowed ISM's shapes: X 0, A 3, V 6 */
        if (shape < base || shape > base + 2) return 0;
        if (!s_made) make_s();
        uint32_t rgb = drawing_sism == S_PINK ? S_PINK_RGB : RW(0x08A04B88u + 4u * (shape / 3u)) & 0xFFFFFFu;
        c->r[29] -= 0x40;
        shapes_draw(c, a0, s_cols[shape - base], rgb, alpha);
        c->r[29] += 0x40;
        return 1;
    }
    if (after || !drawing_entry_wipe || shape < 3 || shape > 5) return 0;
    c->r[29] -= 0x40;
    shapes_draw(c, a0, I_SHAPES[shape - 3], I_YELLOW, alpha);
    c->r[29] += 0x40;
    return 1;
}

/* ---- 2. The I-ISM ranking ---- */

/* The system data keeps the entries' records (object/ranking.c): table 6 their scores, 7 their VS
 * wins, which the attract mode never showed. Its cycle (0x09358400: 0 - 2, X-, A- and V-ISM's scores,
 * title.c) now goes 0, 1, 2, 6: after V-ISM's, the I-ISM scores. Table 6's records say kind 2, so the
 * game dresses it as V-ISM's; while it is shown:
 *   - its title, "V-ISM SCORE RANKING" repeated across the band (pieces 0x08A38208, the letter picture
 *     0x2C), has picture 0x2B instead, the unused Z: its cell's texels are replaced, where the picture
 *     cache has put them (0x088112A8), by an I copied out of the "-I" cell of "-ISM" (picture 0x2E),
 *     right-aligned where X's X ends;
 *   - the screen's palettes (the five V-ISM ones 0x088BC164 loads) are turned gold: each colour's hue
 *     moved to I-ISM's yellow, its lightness and saturation kept;
 *   - the rows' plates behind the faces (translucent boxes, object/batch_large2.c's 0x088BDBB8: red,
 *     green or blue by the ISM) are yellow.
 * The pieces are put back when the next ranking starts. */
#define CYCLE         0x09358400u
#define RANK_TABLES   0x08A05AA8u
#define RANK_TABLE_SZ 0x2F8u
#define I_TABLE       (RANK_TABLES + 6 * RANK_TABLE_SZ)
#define SCREEN_TABLE  0x09525DE8u
#define TITLE_PIECES  0x08A38208u
#define TITLE_PIECE_N 0x60
#define PIC_V         0x2C
#define PIC_Z         0x2B
#define PIC_DASH_ISM  0x2E
#define PICTURE_LISTS 0x08E75AD8u
#define RANK_LIST     22
#define CACHE_PAGES   0x08B0C334u   /* a cached page's texels: *(this + its flags * 24) */
#define I_HUE         52.0f         /* degrees: the wipe's yellow */

static int ranking_i;               /* the I-ISM table is on screen */
static uint32_t patched[TITLE_PIECE_N];
static int npatched;
static uint8_t i_glyph[32][32];     /* 4-bit indices */
static int have_glyph;

/* A colour's hue moved to I_HUE (lightness and saturation kept); only hues between lo and hi degrees
   (0 - 360: all). Answers whether it changed. */
static int hue_to_gold_in(uint32_t a, float lo, float hi) {
    uint16_t v = (uint16_t)RH(a);
    float r = (float)(v & 31) / 31.0f, g = (float)((v >> 5) & 31) / 31.0f, b = (float)((v >> 10) & 31) / 31.0f;
    float mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    if (mx <= 0.0f || (mx - mn) / mx < 0.15f) return 0;       /* greys and black stay */
    float d = mx - mn, hue = mx == r ? 60.0f * ((g - b) / d) : mx == g ? 60.0f * ((b - r) / d + 2) : 60.0f * ((r - g) / d + 4);
    if (hue < 0) hue += 360.0f;
    if (hue < lo || hue > hi) return 0;
    float sat = (mx - mn) / mx, val = mx;
    float h = I_HUE / 60.0f; int i = (int)h; float f = h - (float)i;
    float p = val * (1 - sat), q = val * (1 - sat * f), t = val * (1 - sat * (1 - f));
    float rr, gg, bb;
    switch (i) { case 0: rr = val; gg = t; bb = p; break; default: rr = q; gg = val; bb = p; break; }
    uint16_t out = (uint16_t)((v & 0x8000) | (uint32_t)(rr * 31.0f + 0.5f) | (uint32_t)(gg * 31.0f + 0.5f) << 5 |
                              (uint32_t)(bb * 31.0f + 0.5f) << 10);
    if (out == v) return 0;
    WH(a, out);
    return 1;
}
static void hue_to_gold(uint32_t a) { hue_to_gold_in(a, 0.0f, 360.0f); }

static void restore_title(void) {
    for (int k = 0; k < npatched; k++) WH(patched[k], (RH(patched[k]) & ~0x3FFFu) | PIC_V);
    npatched = 0;
}

int i_ism_display_ranking_start(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    if (!after) { restore_title(); ranking_i = 0; return 0; }
    if (RW(SCREEN_TABLE) != I_TABLE) return 0;
    ranking_i = 1;
    for (uint32_t k = 0; k < TITLE_PIECE_N; k++) {
        uint32_t a = TITLE_PIECES + k * 6 + 4;
        if ((RH(a) & 0x3FFF) == PIC_V) { WH(a, (RH(a) & ~0x3FFFu) | PIC_Z); patched[npatched++] = a; }
    }
    static const uint32_t slots[] = { 0x100, 0x120, 0x140, 0x160, 0x180, 0x280 };
    for (unsigned k = 0; k < sizeof slots / sizeof *slots; k++)
        for (uint32_t i = 1; i < 16; i++) hue_to_gold(PALETTES + slots[k] + i * 2);
    WW(PALETTES_DIRTY, 1);
    rt_log("i_ism_display: the I-ISM ranking");
    return 0;
}

/* A cached page's cell 0, 32 x 32 at 4 bits, 32 bytes a row (object/sprite_grid.c) */
static uint32_t page_texels(uint32_t page) { return RW(CACHE_PAGES + RW(page + 8) * 24u); }
static uint8_t texel(uint32_t buf, int x, int y) { uint8_t b = (uint8_t)RB(buf + (uint32_t)(y * 32 + x / 2)); return x & 1 ? b >> 4 : b & 15; }
static void set_texel(uint32_t buf, int x, int y, uint8_t v) {
    uint32_t a = buf + (uint32_t)(y * 32 + x / 2); uint8_t b = (uint8_t)RB(a);
    WB(a, x & 1 ? (uint8_t)((b & 0x0F) | v << 4) : (uint8_t)((b & 0xF0) | v));
}

/* 0x088112A8 (a0 draw state, a1 the picture, a2 its list, a3 its first cached page) */
int i_ism_display_cached32(Cpu *c, uint32_t a0, int after) {
    (void)a0;
    if (after || !ranking_i) return 0;
    uint32_t pic = c->r[5], base = c->r[6], page = c->r[7];
    if (!page || base != RW(PICTURE_LISTS + RANK_LIST * 4) || (pic - base) % 12) return 0;
    uint32_t n = (pic - base) / 12, buf = page_texels(page);
    if (n == PIC_DASH_ISM && !have_glyph) {
        /* "-I" then the start of "S": the I is the run of columns 8 - 17; moved to 20 - 29 */
        memset(i_glyph, 0, sizeof i_glyph);
        for (int y = 0; y < 32; y++) for (int x = 8; x < 18; x++) i_glyph[y][x + 12] = texel(buf, x, y);
        have_glyph = 1;
    } else if (n == PIC_Z && have_glyph) {
        for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) set_texel(buf, x, y, i_glyph[y][x]);
    }
    return 0;
}

/* The attract cycle (title.c, 0x0888B8D4): after V-ISM's scores, the I-ISM ones (table 6). */
int i_ism_display_attract_cycle(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    static int8_t before;
    if (!after) { before = (int8_t)RB(CYCLE); return 0; }
    if (before == 2) WB(CYCLE, 6);
    return 0;
}

/* A row's plate (batch_large2.c, 0x088BDBB8; the object, +92 its ISM 1 - 3, +36 / +42 / +48 its
   colour's red, green and blue at each corner): yellow on the I-ISM ranking, where V's blue was. */
#define PLATE_R  0xE0
#define PLATE_G  0xD0
#define PLATE_B  0x20
int i_ism_display_name_box(Cpu *c, uint32_t o, int after) {
    (void)c;
    if (!after || !ranking_i || RB(o + 92) == 0) return 0;
    for (uint32_t k = 0; k < 4; k++) { WB(o + 36 + k, PLATE_R); WB(o + 42 + k, PLATE_G); WB(o + 48 + k, PLATE_B); }
    return 0;
}
