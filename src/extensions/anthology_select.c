/* ALPHA SELECT and S-ISM SELECT on the character select screen (docs/ANTHOLOGY_EXTENSION.md, 5: the
 * maintainer's design of 2026-11-15, asked for 2026-11-24, his notes on the first try the same day).
 * EX. SETTING > EX. ANTHOLOGY switches each on; both need the Anthology disc imported (anthology.c), like
 * T.HAWK WALKBACK.
 *
 * The game's flow, each player's cursor (src/core/select/cursor.c): SELECT PLAYER (state 0) > MODE
 * SELECT (state 1, the play styles NORMAL / MAZI / SAIKYO / CLASSIC) > ISM SELECT (state 2) > SPEED
 * (state 3). With these options:
 *
 *   SELECT PLAYER > ALPHA SELECT > MODE SELECT (ALPHA3 only) > ISM SELECT (or MANUAL / AUTO) > SPEED
 *
 * - ALPHA SELECT is the game's own MODE step (state 1) run once more before MODE SELECT: the game's
 *   panel slides in and out as it does, with its tape and the player's name strip, the tape reading
 *   ALPHA SELECT (picture 144, MODE SELECT, swapped for ours: the same tape, ALPHA spelt in its letters,
 *   SELECT its own) and the panel's tiles left out (still run, out of sight) for the list: ALPHA, ALPHA2,
 *   ALPHA2G, ALPHA2GC (the Classical version, for its seven characters, shown as ALPHA2G in blue) and
 *   ALPHA3, in the game's own font (font 1, green), with the game's arrow at the one chosen. The list
 *   goes where the panel goes. Only the versions the character has in Hyper are offered (Alpha: the
 *   first 13, Ryu to Dan; Alpha 2 and 2 Gold: the first 18, to Gen); a character with ALPHA3 alone, or
 *   an entry, skips it. The arrow starts on ALPHA3. ALPHA3 chosen, the panel goes out and comes back
 *   for MODE SELECT (when the mode has it).
 * - After ALPHA - ALPHA2GC, GUARD SELECT the same way in the game's ISM step (state 2, the ISM panel,
 *   its tape (282, ISM SELECT) reading GUARD SELECT): MANUAL / AUTO; the ISM is A (standing for the
 *   older versions' single gauge until they play as themselves), MODE SELECT NORMAL, then SPEED.
 * - MODE SELECT, with S-ISM SELECT on, becomes a carousel (0x0889507C, wrapped) of the styles the mode
 *   offers and the four S-ISMs (green, blue, pink, red; pink only for Hyper's eleven SF2 fighters, its
 *   mask 0x13218C13), drawn with the game's own tiles: the panel's sprite draws each tile as picture
 *   139 (the frame with MODE) and its name (140 - 143) from the select screen's list (slot 8); those
 *   draws (0x0880E150) are taken over and the tiles drawn again where the carousel puts them, the one
 *   chosen in front at full size, the ones before and after behind it at 80 % and fainter. An S-ISM
 *   tile is the frame in its colour's palette with a name of ours, S-ISM, made from the game's own
 *   letters (S of SAIKYO, I and M of MAZI). It starts on NORMAL; in the order NORMAL, MAZI, SAIKYO,
 *   CLASSIC, S-ISM green, blue, pink, red no two tiles of a colour meet, with any of them left out.
 * - ISM SELECT (0x08894C74, wrapped): none after an S-ISM: its ISM is decided as Hyper decides it (zero9
 *   0x2891A0: green and red A-ISM, blue and pink X-ISM) through the game's own "ISM decided" path
 *   (0x08895564), as for an entry.
 *
 * Survival and Training have their own select (below): a TYPE row above MODE, MODE's values going on
 * to the S-ISMs, MANUAL / AUTO in the ISM row.
 *
 * Offline only, and not in Dramatic, Variable or World Tour (the maintainer: the modes where two
 * fighters share a super gauge wait for a rule for mixed gauges). Each player's choice is kept
 * (anthology_choice) and logged; the fight itself is still Alpha 3: the versions' moves, sprites and
 * rules are the next steps (E2, E3). */
#include "rt.h"
#include "ext_features.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void f_089E0CF4(Cpu *c);   /* 2D library: a0 = texture list {descriptors, count} */
void f_089E1618(Cpu *c);   /* 2D library: draw a0 = {xy, colours, uv, n}, a1 = n, a2 = flags, f12 = depth */
void f_08896AA4(Cpu *c);   /* cursor.c: (fighter) its pad's newly pressed buttons */
void f_088951DC(Cpu *c);   /* cursor.c: the style chosen */
void f_08895564(Cpu *c);   /* cursor.c: an ISM decided for the player (fighter, other) */
void f_088A4F1C(Cpu *c);   /* cursor.c: the styles the mode offers (fighter): a bit per style */
void f_0887C4CC(Cpu *c);   /* sound effect (0, id, volume) */
int anthology_hawk_available(void);

int rt_alpha_select = 0, rt_sism_select = 0;   /* EX. SETTING > EX. ANTHOLOGY (settings.c keeps them; off by default: experimental) */
int rt_sism_presentation = 0;                  /* EX. ANTHOLOGY > S-ISM PRESENTATION: the S-ISMs' wipes, S-ISM D's pink (i_ism_display.c, below) */
int anthology_select_available(void) { return FEATURE(ANTHOLOGY) && anthology_hawk_available(); }

#define GAME        0x08E75910u
#define PAGES       0x08B0C334u
#define FIGHTER(k)  (0x092E3EA0u + 0x498u * (uint32_t)(k))
#define TEXT_TEXTURES 0x08A05080u
#define G_MODE      114
#define G_NET       146
#define F_STATE     4
#define F_STEP      5
#define F_CHOSEN    78
#define F_SLOT      337
#define F_CHARACTER 338
#define F_ISM       386
#define F_DONE      424
#define F_STYLE     438
#define F_LAST_ISM  450
#define F_CURSOR_ON 824
#define CHOSE_ISM   2
#define CHOSE_STYLE 4
#define DONE_ISM    2
#define DONE_STYLE  4
#define BUTTONS_CHOOSE 0x3F0u
#define DIR_DOWN    4
#define DIR_UP      8
#define SND_MOVE    0x38
#define SND_CHOOSE  0x39
#define OUR_PAGE    0x3D                    /* a page index nothing uses on this screen (selectscreen.c has 0x3E) */
#define LAYER       22.0f

/* ---------------------------------------------------------------- versions and tiles */
enum { V_ALPHA3, V_ALPHA, V_ALPHA2, V_ALPHA2G, V_ALPHA2GC, V_COUNT };
static const char *const version_name[V_COUNT] = { "ALPHA3", "ALPHA", "ALPHA2", "ALPHA2G", "ALPHA2GC" };   /* (the log's) */
static const char *const version_shown[V_COUNT] = { "ALPHA3", "ALPHA", "ALPHA2", "ALPHA2G", "ALPHA2G" };  /* ALPHA2GC: ALPHA2G in blue (the maintainer) */
static const int listed[V_COUNT] = { V_ALPHA, V_ALPHA2, V_ALPHA2G, V_ALPHA2GC, V_ALPHA3 };   /* top to bottom */
enum { T_NORMAL, T_MAZI, T_SAIKYO, T_CLASSIC, T_SISM_G, T_SISM_B, T_SISM_P, T_SISM_R, T_COUNT };   /* carousel order */
static const char *const tile_log[T_COUNT] = { "NORMAL", "MAZI", "SAIKYO", "CLASSICAL", "S-ISM GREEN", "S-ISM BLUE", "S-ISM PINK", "S-ISM RED" };
static const int8_t sism_ism[4] = { 0, -1, -1, 0 };      /* green A, blue X, pink X, red A (zero9 0x2891A0) */

static int base_character(int ch) { return ch == 0x12 ? 0x04 : ch == 0x13 ? 0x11 : ch == 0x14 ? 0x06 : ch; }   /* the X-ISM ones */
static int has_version(int ch, int v) {
    int b = base_character(ch);
    switch (v) {
    case V_ALPHA3: return 1;
    case V_ALPHA: return b >= 0 && b <= 0x0C;                         /* Hyper's 0x30 - 0x3C */
    case V_ALPHA2: case V_ALPHA2G: return b >= 0 && b <= 0x11;        /* 0x40 - 0x51, 0x60 - 0x71 */
    case V_ALPHA2GC: return b == 0x00 || b == 0x01 || b == 0x04 || b == 0x0A || b == 0x0B || b == 0x0F || b == 0x10;   /* 0x75 - 0x7C */
    }
    return 0;
}
static int sf2_fighter(int ch) { int b = base_character(ch); return b >= 0 && b < 32 && ((0x13218C13u >> b) & 1); }

/* ---------------------------------------------------------------- each player's choice */
/* P_VERSION: ALPHA SELECT in the MODE step; P_VCHOSEN: chosen, the panel going; P_BACK: ALPHA3 chosen,
   the panel going out and coming back as MODE SELECT; P_GUARD: GUARD SELECT in the ISM step; P_GCHOSEN:
   chosen */
enum { P_NONE, P_VERSION, P_VCHOSEN, P_BACK, P_GUARD, P_GCHOSEN, P_DONE };
#define DS_SIZE     0x60
typedef struct {
    int phase, cursor, version;         /* ALPHA SELECT */
    int carousel, tiles[T_COUNT], ntiles, at;   /* MODE SELECT's carousel: on, its tiles, the one in front */
    int sism;                           /* 0, or 1 - 4: green, blue, pink, red */
    int guard;                          /* 0 MANUAL, 1 AUTO */
    int style_skip;                     /* the mode has no MODE SELECT (its MODE step is ALPHA SELECT's only) */
    uint32_t back, back_in;             /* P_BACK: the frame it began; then the frame MODE SELECT came in */
    uint32_t picked;                    /* the frame its character was picked (the latest select wins) */
    uint32_t seen;                      /* the frame one of our steps last ran for it */
    int arrow_dx, arrow_seen;           /* the game's arrow, from the list's centre (seen in the MODE panel) */
    int guard_ax, guard_ay;             /* where it goes for GUARD SELECT */
    int mode_cx, mode_cx_ok;            /* the list's centre in the MODE panel this frame */
} Player;
static Player pl[2];
int anthology_choice(int slot, int *version, int *sism, int *guard) {   /* (the fight, later) */
    if (slot < 0 || slot > 1) return 0;
    *version = pl[slot].version; *sism = pl[slot].sism; *guard = pl[slot].guard;
    return 1;
}

static uint32_t frame_now(void) { return (uint32_t)rt_vblank_count(); }   /* (the PSP's vertical blanks: a frame each) */
static int screen_ok(void) {
    int8_t mode = (int8_t)RB(GAME + G_MODE);
    return anthology_select_available() && RB(GAME + G_NET) == 0 && mode != 3 && mode != 8 && mode != 15;
}
static int slot_of(uint32_t me) { return (int8_t)RB(me + F_SLOT) == 1 ? 1 : 0; }
static uint32_t other_of(uint32_t me) { return me == FIGHTER(0) ? FIGHTER(1) : FIGHTER(0); }

static uint32_t call3(Cpu *c, void (*f)(Cpu *), uint32_t a0, uint32_t a1, uint32_t a2) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = a0; c->r[5] = a1; c->r[6] = a2; f(c);
    uint32_t v = c->r[2];
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    return v;
}
static uint32_t pressed(Cpu *c, uint32_t me) { return call3(c, f_08896AA4, me, 0, 0) & 0xFFFF; }
static void sound(Cpu *c, int id) { call3(c, f_0887C4CC, 0, (uint32_t)id, 0xC0); }

/* 0x08894644: a player's character step starts again, nothing of ours chosen */
int anthology_select_character(Cpu *c, uint32_t me, int after) {
    (void)c;
    if (!after) { memset(&pl[slot_of(me)], 0, sizeof pl[0]); pl[slot_of(me)].picked = frame_now(); }
    return 0;
}

/* text through the game's queue (0x0880A6CC: text, font, x, y, colour), in the text pass's 384 x 224 */
void f_0880A6CC(Cpu *c);
void f_08894E5C(Cpu *c);   /* cursor.c: the ISM chosen (fighter, other) */
static uint32_t gtext(const char *s);
static void queue_text(Cpu *c, const char *s, int font, int x, int y, int colour) {
    uint32_t t = gtext(s);
    if (!t) return;
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = t; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
#define TC_GREEN 4                              /* the game's text colours: white to green */
#define TC_BLUE  6                              /* the V-ISM's blue (X-ISM's red is 1, pink 7) */
#define OPT_FONT 1                              /* the game's 8 x 16 font, as Hyper's version lists */
#define OPT_Y    68                             /* the five rows' first (text units) */
#define OPT_DY   21
#define F_TIMER     82
#define F_ENTRY_BOX 127
#define F_DONE_AT_RESET 78                  /* +424 as 0x08896DE8 left it (bit 2: no style to choose)*/
#define F_FINAL_BISON_C 439
#define BACK_OUT    16                          /* ALPHA3: the MODE panel's way out, */
#define BACK_GAP    8                           /* out of sight, */
#define BACK_IN     12                          /* and in again (the game's own moves, below) */

/* the lists: ALPHA SELECT's (the versions the character has) and GUARD SELECT's */
static int version_list(int ch, int cursor, const char **items, int *colours, int *at) {
    int n = 0;
    for (int i = 0; i < V_COUNT; i++) {
        if (!has_version(ch, listed[i])) continue;
        if (i == cursor) *at = n;
        items[n] = version_shown[listed[i]]; colours[n++] = listed[i] == V_ALPHA2GC ? TC_BLUE : TC_GREEN;
    }
    return n;
}
static int guard_list(const Player *p, const char **items, int *colours, int *at) {
    int colour = TC_GREEN;                      /* (the maintainer: green whatever the version, ALPHA2G's blue too) */
    items[0] = "MANUAL"; items[1] = "AUTO"; colours[0] = colours[1] = colour; *at = p->guard;
    return 2;
}
static int row_y(int n, int i) { return OPT_Y + (5 - n) * OPT_DY / 2 + i * OPT_DY; }
/* a list centred at text point cx (drawn while the panel it stands in is) */
static void draw_list(Cpu *c, int cx, const char *const *items, const int *colours, int n) {
    for (int i = 0; i < n; i++) {
        int w = 8 * (int)strlen(items[i]);
        if (cx - w / 2 > -48 && cx - w / 2 < 384) queue_text(c, items[i], OPT_FONT, cx - w / 2, row_y(n, i), colours[i]);
    }
}

/* 0x08894990, the character shown a moment: once done (state 1 next), ALPHA SELECT if it's offered */
int anthology_select_character_wait(Cpu *c, uint32_t me, int after) {
    (void)c;
    if (!after || !screen_ok()) return 0;
    Player *p = &pl[slot_of(me)];
    if (p->phase != P_NONE || RB(me + F_STATE) != 1) return 0;
    int ch = (int8_t)RB(me + F_CHARACTER), n = 0;
    for (int v = V_ALPHA; v < V_COUNT; v++) n += has_version(ch, v);
    p->version = V_ALPHA3;
    if (!rt_alpha_select || n == 0 || RB(me + F_ENTRY_BOX) != 0 || RB(me + F_FINAL_BISON_C) != 0) { p->phase = P_DONE; return 0; }
    p->phase = P_VERSION; p->cursor = 4;                   /* ALPHA3: choosing at once plays as before */
    p->style_skip = (RH(me + F_DONE) & DONE_STYLE) != 0;   /* MODE SELECT left out: ALPHA SELECT still has the step */
    if (p->style_skip) {
        WH(me + F_DONE, RH(me + F_DONE) & ~(uint32_t)DONE_STYLE);
        WB(me + F_DONE_AT_RESET, RB(me + F_DONE_AT_RESET) & ~(uint32_t)DONE_STYLE);   /* (the MODE panel shows by this copy) */
    }
    return 0;
}

/* 0x0889507C, MODE SELECT's choice: ALPHA SELECT first; then the carousel (the game's panel drawn by
   anthology_select_tile) */
static void carousel_start(Cpu *c, uint32_t me, Player *p) {
    uint32_t offered = call3(c, f_088A4F1C, me, 0, 0) & 0xF;
    int ch = (int8_t)RB(me + F_CHARACTER), entry = RB(me + F_ENTRY_BOX) != 0;   /* an entry: the styles only, no S-ISM */
    p->ntiles = 0;
    for (int t = 0; t < T_COUNT; t++) {
        int ok = t <= T_CLASSIC ? (int)((offered >> t) & 1) : !entry && (offered & 1) && (t != T_SISM_P || sf2_fighter(ch));
        if (ok) p->tiles[p->ntiles++] = t;
    }
    p->at = 0;                                             /* NORMAL (or the first style offered) */
    p->carousel = 1;
}
int anthology_select_style_choose(Cpu *c, uint32_t me, int after) {
    if (after || !screen_ok()) return 0;
    Player *p = &pl[slot_of(me)];
    if (p->phase == P_VERSION) {
        int ch = (int8_t)RB(me + F_CHARACTER), s = slot_of(me);
        p->seen = frame_now();
        WB(me + F_STYLE, 0);
        uint32_t b = pressed(c, me);
        int step = (b & DIR_DOWN) ? 1 : (b & DIR_UP) ? -1 : 0;
        if (step) {
            do p->cursor = (p->cursor + step + V_COUNT) % V_COUNT; while (!has_version(ch, listed[p->cursor]));
            sound(c, SND_MOVE);
        }
        if (b & BUTTONS_CHOOSE) {
            p->version = listed[p->cursor];
            rt_log("anthology: player %d (character %#x) chose %s", s + 1, ch, version_name[p->version]);
            if (p->version == V_ALPHA3 && !p->style_skip) {   /* MODE SELECT next: the panel out and back, the step kept */
                p->phase = P_BACK; p->back = frame_now(); p->back_in = 0;
                sound(c, SND_CHOOSE);
                return 1;
            }
            p->phase = P_VCHOSEN;
            call3(c, f_088951DC, me, other_of(me), 0);    /* the game's "style chosen": its panel goes as it does */
        }
        return 1;
    }
    if (p->phase == P_BACK) {
        p->seen = frame_now();
        if (frame_now() - p->back < BACK_OUT + BACK_GAP) return 1;
        p->phase = P_DONE; p->back_in = frame_now();
        WB(me + F_STYLE, 0);
    }
    if (!rt_sism_select) return 0;
    if (!p->carousel) carousel_start(c, me, p);
    if (p->ntiles <= 1) { p->carousel = 0; return 0; }
    p->seen = frame_now();
    uint32_t b = pressed(c, me);
    int step = (b & DIR_DOWN) ? 1 : (b & DIR_UP) ? -1 : 0;
    if (step) { p->at = (p->at + step + p->ntiles) % p->ntiles; sound(c, SND_MOVE); }
    int t = p->tiles[p->at];
    /* an S-ISM shown as its colour's style (the game lights the chosen style's tile), played as NORMAL:
       the game's "style chosen" sees NORMAL, then the colour's again while it flashes, NORMAL once the
       ISM step comes (anthology_select_style_wait) */
    WB(me + F_STYLE, t <= T_CLASSIC ? (uint32_t)t : (uint32_t)(t - T_SISM_G));
    if (!(b & BUTTONS_CHOOSE)) return 1;
    p->sism = t >= T_SISM_G ? t - T_SISM_G + 1 : 0;
    rt_log("anthology: player %d chose %s", slot_of(me) + 1, tile_log[t]);
    if (p->sism) WB(me + F_STYLE, 0);
    call3(c, f_088951DC, me, other_of(me), 0);            /* the game's "style chosen" */
    if (p->sism) WB(me + F_STYLE, (uint32_t)(p->sism - 1));
    return 1;
}
/* 0x08895248, the style shown a moment, then state 2: after ALPHA SELECT, GUARD SELECT (an older
   version), or for ALPHA3 when the mode has no MODE SELECT, the ISM */
int anthology_select_style_wait(Cpu *c, uint32_t me, int after) {
    (void)c;
    if (!screen_ok()) return 0;
    Player *p = &pl[slot_of(me)];
    if (!after) { if (p->carousel || p->phase == P_VCHOSEN) p->seen = frame_now(); return 0; }
    if (p->sism && RB(me + F_STATE) != 1) WB(me + F_STYLE, 0);   /* the S-ISM plays as NORMAL */
    if (p->phase != P_VCHOSEN || RB(me + F_STATE) != 2) return 0;
    if (p->version != V_ALPHA3) { p->phase = P_GUARD; p->guard = 0; return 0; }
    p->phase = P_DONE;
    return 0;
}

/* 0x08894C74, state 2's first step: no ISM step after an S-ISM */
int anthology_select_ism_enter(Cpu *c, uint32_t me, int after) {
    if (after || !screen_ok()) return 0;
    Player *p = &pl[slot_of(me)];
    if ((RH(me + F_DONE) & DONE_ISM) || !p->sism) return 0;
    int ism = sism_ism[p->sism - 1];
    WB(me + F_ISM, (uint32_t)ism); WB(me + F_LAST_ISM, (uint32_t)ism);
    WB(me + F_CHOSEN, RB(me + F_CHOSEN) | CHOSE_ISM);
    WH(me + F_DONE, RH(me + F_DONE) | DONE_ISM);
    call3(c, f_08895564, me, other_of(me), 0);
    return 0;                                              /* the game moves on to SPEED */
}
/* 0x08894D40, the ISM's choice: GUARD SELECT (MANUAL / AUTO) for the older versions; the ISM is A */
int anthology_select_ism_choose(Cpu *c, uint32_t me, int after) {
    if (after || !screen_ok()) return 0;
    Player *p = &pl[slot_of(me)];
    if (p->phase != P_GUARD) return 0;
    p->seen = frame_now();
    WB(me + F_ISM, 0);
    uint32_t b = pressed(c, me);
    if (b & (DIR_DOWN | DIR_UP)) { p->guard ^= 1; sound(c, SND_MOVE); }
    if (b & BUTTONS_CHOOSE) {
        rt_log("anthology: player %d chose %s %s", slot_of(me) + 1, version_name[p->version], p->guard ? "AUTO" : "MANUAL");
        p->phase = P_GCHOSEN;
        call3(c, f_08894E5C, me, other_of(me), 0);        /* the game's "ISM chosen": A; its panel goes as it does */
    }
    return 1;
}

/* ================================================================ Survival and Training
 *
 * Their select (src/core/select/battle_select.c) is a box of rows for each fighter: the character
 * (state 1), MODE (state 2, the style), ISM (state 3), SPEED ... With ALPHA SELECT and S-ISM SELECT:
 *
 * - TYPE, a new row above MODE: ALPHA, ALPHA2, ALPHA2G, ALPHA2GC (in blue) or ALPHA3, chosen first in
 *   state 2 (its entry, 0x088A34A8, wrapped; up / down, then an attack button), its value blinking while
 *   it is chosen. A version but ALPHA3 skips MODE (NORMAL).
 * - MODE (0x088A35E4, wrapped) also goes round S-ISM G, S-ISM B, S-ISM P (the SF2 fighters) and S-ISM
 *   R after the styles offered; the row's text is swapped while the box is drawn (0x088A4D30).
 * - ISM: after an S-ISM none (decided, as the character select: green / red A, blue / pink X); after
 *   ALPHA - ALPHA2GC, MANUAL / AUTO in the ISM row (0x088A3914 wrapped; the row's text swapped while the
 *   box is drawn, 0x088A60F8).
 * Training (mode 2), and Survival (mode 7) but its two-against-one and team fights (layout 3, BATTLE +
 * 79 = 4 or 5: the shared-gauge kinds the maintainer left out). */
#define BATTLE        0x092E1228u
#define B_LAYOUT      56
#define B_SURV_KIND   79
#define F_PAD_SLOT    775
#define F_PLAYER_PAD  820
#define F_FINAL_BISON 439
#define F_TIMER       82
#define F_ENTRY_BOX_B 127                       /* on an entry's box, 1 - 6 (0 otherwise) */
#define G_PRESENT     53
#define BOX_PLACES      0x08A2B4C4u
#define BOX_HEADINGS    0x08A2BA48u
#define BOX_STYLE_NAMES 0x08A2BA34u
#define TEXT_COLOUR_NOW 0x08B02716u           /* u16: the screen's text colour (low byte) */
void f_088A4EB4(Cpu *c);   /* battle_select.c: (fighter) its box is on the right */
#define f_088A4EB4_ptr f_088A4EB4
void f_088A507C(Cpu *c);   /* battle_select.c: (fighter) the newly pressed buttons of the pad driving it */
void f_088A4E90(Cpu *c);   /* the choose sound */
void f_088A4E30(Cpu *c);   /* (fighter) the style's flags */
void f_088A3AA0(Cpu *c);   /* (fighter, other, buttons, list) an ISM chosen */
void f_0880A6CC(Cpu *c);   /* queues text (a0 text, a1, a2 x, a3 y, t0 colour) */

typedef struct {
    int phase, cursor, version, sism, guard;
    int tiles[T_COUNT], ntiles, at, rolling;   /* MODE's values with the S-ISMs */
    uint32_t ism_shown;                        /* the frame MANUAL / AUTO was last being chosen */
    uint32_t picked;                           /* the frame its character was picked (the latest select wins) */
} Boxed;
static Boxed bx[4];
/* the Survival / Training boxes' highlights of ours: TYPE's while it is chosen, the ISM row's longer end */
static struct { int on; float x, y, w, h; } bar[4][3];
static int bmode_ok(void) {
    int8_t mode = (int8_t)RB(GAME + G_MODE);
    if (!anthology_select_available() || RB(GAME + G_NET) != 0) return 0;
    if (mode == 2) return 1;
    return mode == 7 && (int16_t)RH(BATTLE + B_LAYOUT) != 3 && (int8_t)RB(BATTLE + B_SURV_KIND) != 4 && (int8_t)RB(BATTLE + B_SURV_KIND) != 5;
}
static Boxed *box_of(uint32_t me) {
    if (me < FIGHTER(0) || (me - FIGHTER(0)) % 0x498u || (me - FIGHTER(0)) / 0x498u > 3) return NULL;
    return &bx[(me - FIGHTER(0)) / 0x498u];
}
/* strings of ours the text queue can point at */
static uint32_t pool;
static uint32_t gtext(const char *s) {
    static const char *key[16]; static uint32_t at[16]; static int n; static uint32_t used;
    for (int i = 0; i < n; i++) if (key[i] == s) return at[i];
    if (!pool && !(pool = rt_alloc(0x200, 1, 16, "anthology select text"))) return 0;
    size_t len = strlen(s) + 1;
    if (n == 16 || used + len > 0x200) return 0;
    memcpy(HOST(pool + used), s, len); key[n] = s; at[n] = pool + used; n++; used += (uint32_t)((len + 3) & ~3u);
    return at[n - 1];
}
/* the S-ISMs as this menu names them (the maintainer, 2026-11-25: Hyper's own names; the carousel's tiles say S-ISM):
   green C, blue B, pink D, red A */
static const char *const sism_text[4] = { "S-ISM C", "S-ISM B", "S-ISM D", "S-ISM A" };
/* 0x088A2870: a fighter's character row again: nothing of ours chosen */
int anthology_bselect_character(Cpu *c, uint32_t me, int after) {
    (void)c; Boxed *b = box_of(me);
    if (!after && b) { memset(b, 0, sizeof *b); b->picked = frame_now(); }
    return 0;
}
/* the style row's own skip (0x088A34A8): its pad drives it, it is its turn */
static int style_row_runs(uint32_t me) {
    if ((uint32_t)(int32_t)(int8_t)RB(me + F_PAD_SLOT) != RW(me + F_PLAYER_PAD) && !((int8_t)RB(GAME + G_PRESENT) & (1 << ((int8_t)RB(me + F_SLOT) & 31)))) return 0;
    return (int8_t)RB(me + F_FINAL_BISON) == 0;
}
/* 0x088A34A8, state 2's entry: TYPE first (when the character has more than ALPHA3) */
int anthology_bselect_style_enter(Cpu *c, uint32_t me, int after) {
    Boxed *b = box_of(me);
    if (after || !b || !bmode_ok() || !rt_alpha_select || b->phase == P_DONE) return 0;
    int ch = (int8_t)RB(me + F_CHARACTER);
    if (b->phase == P_NONE) {
        int n = 0;
        for (int v = V_ALPHA; v < V_COUNT; v++) n += has_version(ch, v);
        b->version = V_ALPHA3;
        /* an entry: ALPHA3, its TYPE not chosen (the maintainer, 2026-11-25) */
        if (n == 0 || !style_row_runs(me) || (int8_t)RB(me + F_ENTRY_BOX_B) != 0) { b->phase = P_DONE; return 0; }
        b->phase = P_VERSION; b->cursor = 4;
    }
    uint32_t p = call3(c, f_088A507C, me, 0, 0) & 0xFFFF;
    int step = (p & DIR_DOWN) ? 1 : (p & DIR_UP) ? -1 : 0;
    if (step) {
        do b->cursor = (b->cursor + step + V_COUNT) % V_COUNT; while (!has_version(ch, listed[b->cursor]));
        sound(c, SND_MOVE);
    }
    b->version = listed[b->cursor];
    if (!(p & BUTTONS_CHOOSE)) return 1;
    b->phase = P_DONE;
    call3(c, f_088A4E90, 0, 0, 0);
    if (b->version != V_ALPHA3) { WB(me + F_STYLE, 0); WH(me + F_DONE, RH(me + F_DONE) | 2); }   /* MODE skipped (NORMAL), as with MODE SELECT off */
    rt_log("anthology: fighter %d (character %#x) TYPE %s", (int)(b - bx) + 1, ch, version_name[b->version]);
    return 0;
}
/* 0x088A35E4, MODE's choice: the styles offered, then the S-ISMs */
int anthology_bselect_style_choose(Cpu *c, uint32_t me, int after) {
    Boxed *b = box_of(me);
    if (after || !b || !bmode_ok() || !rt_sism_select || b->version != V_ALPHA3) return 0;
    if ((int8_t)RB(me + F_ENTRY_BOX_B) != 0) return 0;            /* an entry: the styles only, no S-ISM */
    int ch = (int8_t)RB(me + F_CHARACTER);
    if (!b->rolling) {
        uint32_t offered = call3(c, f_088A4F1C, me, 0, 0) & 0xF;
        b->ntiles = 0;
        for (int t = 0; t < T_COUNT; t++) {
            int ok = t <= T_CLASSIC ? (int)((offered >> t) & 1) : (offered & 1) && (t != T_SISM_P || sf2_fighter(ch));
            if (ok) b->tiles[b->ntiles++] = t;
        }
        b->at = 0; b->rolling = 1;
    }
    uint32_t p = call3(c, f_088A507C, me, 0, 0) & 0xFFFF;
    int step = (p & DIR_DOWN) ? 1 : (p & DIR_UP) ? -1 : 0;
    if (step) { b->at = (b->at + step + b->ntiles) % b->ntiles; sound(c, SND_MOVE); }
    int t = b->tiles[b->at];
    WB(me + F_STYLE, t <= T_CLASSIC ? (uint32_t)t : 0);
    b->sism = t >= T_SISM_G ? t - T_SISM_G + 1 : 0;
    if (!(p & BUTTONS_CHOOSE)) return 1;
    WB(me + F_STEP, RB(me + F_STEP) + 1); WH(me + F_TIMER, 8);   /* as the game's own choice */
    WB(me + F_CHOSEN, RB(me + F_CHOSEN) | 4);
    call3(c, f_088A4E90, 0, 0, 0);
    call3(c, f_088A4E30, me, 0, 0);
    rt_log("anthology: fighter %d MODE %s", (int)(b - bx) + 1, tile_log[t]);
    return 1;
}
/* 0x088A382C, state 3's entry: an S-ISM's ISM decided */
int anthology_bselect_ism_enter(Cpu *c, uint32_t me, int after) {
    (void)c; Boxed *b = box_of(me);
    if (after || !b || !bmode_ok() || !b->sism || (RH(me + F_DONE) & 4)) return 0;
    int ism = sism_ism[b->sism - 1];
    WB(me + F_ISM, (uint32_t)ism); WB(me + F_LAST_ISM, (uint32_t)ism);
    WH(me + F_DONE, RH(me + F_DONE) | 4);                  /* the game goes straight to the wait */
    return 0;
}
/* 0x088A3914, the ISM's choice: MANUAL / AUTO for the older versions */
int anthology_bselect_ism_choose(Cpu *c, uint32_t me, int after) {
    Boxed *b = box_of(me);
    if (after || !b || !bmode_ok() || b->version == V_ALPHA3) return 0;
    uint32_t p = call3(c, f_088A507C, me, 0, 0) & 0xFFFF;
    if (p & (DIR_DOWN | DIR_UP)) { b->guard ^= 1; sound(c, SND_MOVE); }
    WB(me + F_ISM, 0);
    b->ism_shown = frame_now();
    if (p & BUTTONS_CHOOSE) {
        rt_log("anthology: fighter %d ISM %s", (int)(b - bx) + 1, b->guard ? "AUTO" : "MANUAL");
        uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
        c->r[4] = me; c->r[5] = 0; c->r[6] = p & BUTTONS_CHOOSE; c->r[7] = c->r[6]; f_088A3AA0(c);   /* (other, list unused there) */
        memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    }
    return 1;
}
static void box_text(Cpu *c, uint32_t s, int x, int y, int colour);
#define BOX_ADV 12                               /* the box font's letters, text units apart */
/* 0x088A60F8, a fighter's box drawn: MANUAL / AUTO in the ISM row (the Anthology's); an entry's ISM as
   I-ISM in yellow (Extra ISM Presentation, the I-ISM's: with or without the Anthology, the maintainer
   2026-11-27); the row's name and colour pointers swapped meanwhile. While MANUAL / AUTO is chosen the
   row's highlight is lengthened (ours, under the text) to cover MANUAL's L. Its flag is FEAT_ANY
   ("ism_presentation_"): each part checks its own switch, and with neither it does nothing. */
#define BOX_COLOURS 0x08A2BA78u
#define TC_YELLOW   3                           /* the text colour nearest I-ISM's yellow */
static uint32_t kept_ism_name, kept_ism_slot, kept_colour_at; static uint8_t kept_colour;
int ism_presentation_bselect_box(Cpu *c, uint32_t me, int after) {
    if (after) {
        if (kept_ism_name) { WW(kept_ism_slot, kept_ism_name); kept_ism_name = 0; }
        if (kept_colour_at) { WB(kept_colour_at, kept_colour); kept_colour_at = 0; }
        return 0;
    }
    kept_ism_name = 0; kept_colour_at = 0;
    int iism = FEATURE(I_ISM_DISPLAY) && (int8_t)RB(me + F_ENTRY_BOX_B) != 0;
    Boxed *b = FEATURE(ANTHOLOGY) && bmode_ok() ? box_of(me) : NULL;
    if (!b && !iism) return 0;
    int ism = (int8_t)RB(me + F_ISM);
    if (ism < -1 || ism > 1) return 0;
    if (!b) {                                   /* the I-ISM label alone */
        uint32_t slot = BOX_HEADINGS + 8 + 4u * (uint32_t)(ism + 1), s = gtext("IISM");
        kept_colour_at = BOX_COLOURS + (uint32_t)(ism + 1); kept_colour = (uint8_t)RB(kept_colour_at); WB(kept_colour_at, TC_YELLOW);
        if (s) { kept_ism_slot = slot; kept_ism_name = RW(slot); WW(slot, s); }
        return 0;
    }
    uint32_t slot = BOX_HEADINGS + 8 + 4u * (uint32_t)(ism + 1);
    int side = (int)(call3(c, f_088A4EB4_ptr, me, 0, 0) & 0xFF);
    uint32_t row = BOX_PLACES + (uint32_t)side * 36u + 4u;               /* the ISM row: heading, then its value */
    int ix = (int16_t)RH(row + 4), iy = (int16_t)RH(row + 6);
    uint32_t mode_at = BOX_PLACES + (uint32_t)((((int8_t)RB(me + F_SLOT)) * 2 + 18) * 4);
    int vx = (int16_t)RH(mode_at + 4);                                 /* where TYPE's and MODE's values start */
    const char *name = NULL;
    if (b->version != V_ALPHA3 && b->phase == P_DONE && ism == 0) {
        /* MANUAL / AUTO: drawn by us where the other values start (the game's name blanked meanwhile) */
        name = " ";
        box_text(c, gtext(b->guard ? "AUTO" : "MANUAL"), vx, iy, RB(BOX_COLOURS + 1));
    } else if (FEATURE(I_ISM_DISPLAY) && (int8_t)RB(me + F_ENTRY_BOX_B) != 0) {
        name = "IISM";
        kept_colour_at = BOX_COLOURS + (uint32_t)(ism + 1); kept_colour = (uint8_t)RB(kept_colour_at); WB(kept_colour_at, TC_YELLOW);
    }
    uint32_t s = name ? gtext(name) : 0;
    if (s) { kept_ism_slot = slot; kept_ism_name = RW(slot); WW(slot, s); }
    if (b->version != V_ALPHA3 && frame_now() - b->ism_shown <= 1) {   /* the highlight under MANUAL / AUTO: ours both sides of the game's */
        int fi = (int)(b - bx);
        bar[fi][1].on = 1; bar[fi][1].x = (float)(vx - 2); bar[fi][1].y = (float)(iy + 2); bar[fi][1].w = (float)(ix - vx + 2); bar[fi][1].h = 10.0f;
        bar[fi][2].on = 1; bar[fi][2].x = (float)(ix + 50); bar[fi][2].y = (float)(iy + 2); bar[fi][2].w = (float)(vx + 6 * BOX_ADV - ix - 50 + 2); bar[fi][2].h = 10.0f;
    }
    return 0;
}
/* 0x088A4D30 (fighter, colour), the MODE line, drawn here: MODE in the screen's colour, its value in the
   style's (NORMAL green, MAZI blue, SAIKYO pink, CLASSIC red, as the carousel; an S-ISM's in its own);
   and TYPE above it with a blank line between, always, ALPHA3 unless chosen otherwise, its value one
   word (ALPHA and its number set close) and highlighted while chosen */
static void box_text(Cpu *c, uint32_t s, int x, int y, int colour) {
    if (!s) return;
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = s; c->r[5] = 3; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour; f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static const int style_colour[4] = { TC_GREEN, TC_BLUE, 7, 1 };   /* NORMAL green, MAZI blue, SAIKYO pink, CLASSIC red */
int anthology_bselect_style_line(Cpu *c, uint32_t me, int after) {
    Boxed *b = box_of(me);
    if (after || !b || !bmode_ok()) return 0;
    int slot = (int8_t)RB(me + F_SLOT);
    uint32_t at = BOX_PLACES + (uint32_t)((slot * 2 + 18) * 4);
    int x = (int16_t)RH(at), y = (int16_t)RH(at + 2), x2 = (int16_t)RH(at + 4), y2 = (int16_t)RH(at + 6);
    int heading = RH(TEXT_COLOUR_NOW) & 0xFF;
    int shown = (int16_t)RH(BATTLE + B_LAYOUT) != 3 && ((int8_t)RB(GAME + G_PRESENT) & (1 << (slot & 31)));
    if (shown && rt_alpha_select) {                        /* TYPE */
        const char *v = version_shown[b->version];
        int colour = b->version == V_ALPHA2GC ? TC_BLUE : TC_GREEN;
        box_text(c, gtext("TYPE"), x, y - 24, heading);
        box_text(c, gtext("ALPHA"), x2, y2 - 24, colour);
        if (v[5]) box_text(c, gtext(v == version_shown[V_ALPHA2] ? "2" : v == version_shown[V_ALPHA3] ? "3" : "2G"), x2 + 5 * BOX_ADV, y2 - 24, colour);
        int fi = (int)(b - bx);
        if (b->phase == P_VERSION) {
            bar[fi][0].on = 1; bar[fi][0].x = (float)(x2 - 2); bar[fi][0].y = (float)(y2 - 24 + 2);
            bar[fi][0].w = (float)(BOX_ADV * (int)strlen(v) - 2); bar[fi][0].h = 10.0f;
        }
    }
    if (!shown || (call3(c, f_088A4F1C, me, 0, 0) & 0xFF) == 1) return 1;   /* the game's own reasons for no MODE line */
    int st = (int8_t)RB(me + F_STYLE);
    uint32_t value = b->sism ? gtext(sism_text[b->sism - 1]) : (st >= 0 && st <= 4 ? RW(BOX_STYLE_NAMES + 4u * (uint32_t)st) : 0);
    int colour = b->sism ? style_colour[b->sism - 1] : st >= 0 && st < 4 ? style_colour[st] : heading;
    box_text(c, 0x089FF4DCu, x, y, heading);              /* "MODE" (the game's own) */
    box_text(c, value, x2, y2, colour);
    return 1;
}

/* ---------------------------------------------------------------- the arrow and the tapes
 * One 64 x 128 page of ours: the white arrow (the game's select arrow's look: white, shaded to grey), the
 * two tapes, ALPHA SELECT and GUARD SELECT, and a white block (the ISM row's longer highlight). The tapes
 * as the game's SELECT "ISM" one: a grey tape seen through (its vertex alpha), the words in dark upright
 * letters one under another, reading down (S, E, L, E, C, T), going down 2 pixels a frame (as the game's
 * MODE SELECT tape, measured); beside it the second strip, the player and the character (1P, RYU) in the
 * game's small font, grey and blue. The letters are the game's own font 1 (its glyphs as exsetting.c reads
 * them: colour k's at rows 144 k + 32 to + 80 of the font's pages, 32 glyphs of 8 x 16 a row from '!'),
 * every other row (8 high, as the tape's). */
#define TEX_W    64
#define TEX_H    128
#define BLOCK_U  0                              /* a white block: 8 x 8 */
#define BLOCK_V  16
#define FONT_LIST   0x08A05080u
static uint16_t px[TEX_H][TEX_W];
static uint32_t tex, desc, list, prim;

static uint16_t rgb(int r, int g, int b) { return (uint16_t)(0x8000 | (r & 31) | (g & 31) << 5 | (b & 31) << 10); }
static int build(void) {
    if (tex) return 1;
    if (!RW(FONT_LIST) || RW(FONT_LIST + 4) < 8) return 0;   /* the font isn't loaded yet */
    uint32_t m = rt_alloc(TEX_W * TEX_H * 2 + 0x100, 1, 256, "anthology select");
    if (!m) return 0;
    memset(px, 0, sizeof px);
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) px[BLOCK_V + y][BLOCK_U + x] = rgb(31, 31, 31);
    for (int y = 0; y < TEX_H; y++) for (int x = 0; x < TEX_W; x++) WH(m + (uint32_t)((y * TEX_W + x) * 2), px[y][x]);
    tex = m;
    desc = m + TEX_W * TEX_H * 2; list = desc + 0x48; prim = list + 0x18;
    memcpy(HOST(desc), HOST(RW(PAGES + 0x14)), 0x44);   /* like page 0's (16-bit 5551) */
    WW(desc, OUR_PAGE); WW(desc + 4, 0); WW(desc + 0x18, 0x01000000); WW(desc + 0x24, TEX_W); WW(desc + 0x28, TEX_H);
    uint32_t d0 = RW(TEXT_TEXTURES);
    WW(list + 8, RW(d0)); WW(list + 12, RW(d0 + 4)); WW(list + 16, OUR_PAGE);
    WW(list, list + 8); WW(list + 4, 1);
    return 1;
}
static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float ffrom(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
static float sx, sy, ox, oy;
/* a quad of our page, its top left at text point (x, y) (384 x 224), w x h text units (the texels'
   size unless given), mirrored across for player 2, its vertex colour (A << 24 | R << 16 | G << 8 | B) */
static void quad_c(Cpu *c, int u, int v, int w, int h, float x, float y, float dw, float dh, int flip, uint32_t colour, float layer) {
    float tx0 = x * sx + ox, ty0 = y * sy + oy;
    int x0 = (int)(tx0 + 0.5f), y0 = (int)(ty0 + 0.5f), x1 = x0 + (int)(dw * sx + 0.5f), y1 = y0 + (int)(dh * sy + 0.5f);
    int ua = flip ? u + w : u, ub = flip ? u : u + w;
    uint32_t xy = prim + 0x10, col = xy + 0x10, uv = col + 0x10;
    WH(xy + 0, x0); WH(xy + 2, y0); WH(xy + 4, x1); WH(xy + 6, y0); WH(xy + 8, x1); WH(xy + 0xA, y1); WH(xy + 0xC, x0); WH(xy + 0xE, y1);
    for (int k = 0; k < 4; k++) WW(col + 4u * (uint32_t)k, colour);
    WH(uv + 0, ua); WH(uv + 2, v); WH(uv + 4, ub); WH(uv + 6, v); WH(uv + 8, ub); WH(uv + 0xA, v + h); WH(uv + 0xC, ua); WH(uv + 0xE, v + h);
    WW(prim, xy); WW(prim + 4, col); WW(prim + 8, uv); WW(prim + 0xC, 4);
    c->r[4] = prim; c->r[5] = 4; c->r[6] = 0x80000060u; c->f[12] = fbits(layer);
    c->r[31] = 0x0880B0E8; f_089E1618(c);
}
#define BAR_COLOUR 0xFF00006Bu                 /* the game's row highlight (0, 0, 107) */
#define BAR_LAYER  12.0f                       /* under the text */
static void labels_check(void);
void anthology_select_draw(Cpu *c) {
    labels_check();
    int any_bar = 0;
    for (int f = 0; f < 4; f++) any_bar |= bar[f][0].on | bar[f][1].on | bar[f][2].on;
    if (!any_bar || !anthology_select_available()) return;
    if (!build()) return;
    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint32_t e = PAGES + 0x18u * OUR_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(e + 4u * (uint32_t)i);
    WW(e, tex); WW(e + 4, tex); WW(e + 8, 0); WW(e + 0xC, 0); WW(e + 0x10, 0x80000000u); WW(e + 0x14, desc);
    c->r[4] = list; c->r[31] = 0x0880B0E8; f_089E0CF4(c);
    sx = ffrom(RW(0x08E759E8)); sy = ffrom(RW(0x08E759EC)); ox = ffrom(RW(0x08E75A04)); oy = ffrom(RW(0x08E75A0C));
    for (int f = 0; f < 4; f++) for (int k = 0; k < 3; k++) if (bar[f][k].on) {
        quad_c(c, BLOCK_U + 2, BLOCK_V + 2, 4, 4, bar[f][k].x, bar[f][k].y, bar[f][k].w, bar[f][k].h, 0, BAR_COLOUR, BAR_LAYER);
        bar[f][k].on = 0;                       /* asked for again each frame */
    }
    c->r[4] = TEXT_TEXTURES; c->r[31] = 0x0880B0E8; f_089E0CF4(c);
    for (int i = 0; i < 6; i++) WW(e + 4u * (uint32_t)i, keep_page[i]);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
}

/* ---------------------------------------------------------------- the carousel, from the game's tiles
 * The MODE panel's sprite (its pictures from the select screen's list, slot 8) draws each tile with two
 * calls of 0x0880E150: the frame (picture 139) then the name (140 + style), in an order that follows
 * the style chosen; the draw state's +0 / +2 each piece's place, +48 - +51 its colour (alpha in the
 * last), and the sprite's +12 / +13 the palette's kind and base (set for each piece: each call still
 * runs, out of sight, as the next one's base follows from it). Each frame the eight calls are kept by
 * style and the last one draws the three tiles of the carousel, each from its style's two calls with a
 * new place, scaled about its centre (sprite_grid_scale, sprite_grid.c) and fainter behind. An S-ISM tile is drawn from its colour's
 * style, with its name picture swapped for ours. */
#define SLOT_LIST   (0x08E75AD8u + 4u * 8u)       /* the select screen's picture list */
#define TILE_SETS8  (0x092D9D40u + 4u * 8u)
#define PIC_FRAME   139
#define PIC_NAME    140                            /* NORMAL, MAZI, SAIKYO, CLASSIC */
extern float sprite_grid_scale, sprite_grid_ax, sprite_grid_ay;
extern int sprite_grid_scale_all;
void f_0880E150(Cpu *c);
static uint8_t kept[2][9][DS_SIZE];              /* by side and style: its frame's and its name's draw states (8: the last frame's) */
static uint8_t kept_sp[2][9][2];                 /* and the sprite's +12 / +13 (the palette's kind and base) */
#define PIC_ARROW   119                            /* the select arrow, at the chosen style's row */
static uint32_t kept_frame[2];
static int calls[2];
static int in_tile;                               /* our own calls pass straight through */

/* our S-ISM name pictures, one for each S-ISM: "S-ISM" and its letter (the maintainer, 2026-11-26: the
   letter on the tile too, for colour-blind players), and their tiles */
static uint32_t sism_block, sism_pics, sism_tiles;
#define SISM_BIAS 0xC0000000u
static int sism_state;                            /* 0 not made, 1 made, -1 can't */
static uint8_t name_px[4][16][64];                /* the four names' pixels (colour indices) */

static void unpack16(uint32_t set, uint32_t n, uint8_t out[16][16]) {   /* a 16 x 16 tile (0x088D2B64's stream) */
    uint8_t page[16 * 32 + 64]; memset(page, 0, sizeof page);
    uint32_t src = set + RW(set + n * 4u), dst = 0, written = 0, flags = 0; int bits = 0;
    static const uint32_t STEP = 0x08A3E680u, BACK = 0x08A3E600u;
    do {
        uint32_t token = RB(src++);
        if (bits == 0) { flags = token; token = RB(src++); bits = 8; }
        if (!(flags & 1)) { if (dst < sizeof page) page[dst] = (uint8_t)token; dst += RB(STEP + (dst & 7)); written++; }
        else {
            uint32_t h = token >> 4, k = token & 15;
            if (h == 0) { uint8_t v = (uint8_t)RB(src++); k++; written += k; while (k--) { if (dst < sizeof page) page[dst] = v; dst += RB(STEP + (dst & 7)); } }
            else {
                uint32_t from = dst - RB(BACK + (dst & 7) * 16 + h); written += k;
                while (k--) { if (dst < sizeof page && from < sizeof page) page[dst] = page[from]; dst += RB(STEP + (dst & 7)); from += RB(STEP + (from & 7)); }
            }
        }
        flags >>= 1; bits--;
    } while (written < 128);
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) out[y][x] = (uint8_t)((page[y * 32 + x / 2] >> (4 * (x & 1))) & 15);
}
static int name_pixels(uint32_t lst, uint32_t set, int k) {   /* picture 140 + k: 4 x 1 cells of 16 */
    uint32_t pic = lst + 12u * (uint32_t)(PIC_NAME + k), cells = lst + RW(pic);
    if (RB(pic + 4) != 4 || RB(pic + 5) != 1) return 0;
    memset(name_px[k], 0, sizeof name_px[k]);
    for (int i = 0; i < 4; i++) {
        if (RB(cells) == 0xFF) { cells++; continue; }
        uint32_t tile = RB(cells) << 8 | RB(cells + 1); uint8_t fl = (uint8_t)RB(cells + 2); cells += 3;
        uint8_t t[16][16]; unpack16(set, tile, t);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
            int xx = (fl & 0x40) ? 15 - x : x, yy = (fl & 0x80) ? 15 - y : y;
            name_px[k][y][i * 16 + x] = t[yy][xx];
        }
    }
    return 1;
}
/* the nth letter of a name (columns between empty ones): its first column and width */
static int letter(int k, int nth, int *x0) {
    int n = -1, in = 0, start = 0;
    for (int x = 0; x <= 64; x++) {
        int any = 0;
        for (int y = 0; x < 64 && y < 16; y++) if (name_px[k][y][x]) { any = 1; break; }
        if (any && !in) { in = 1; start = x; n++; }
        if (!any && in) { in = 0; if (n == nth) { *x0 = start; return x - start; } }
    }
    return 0;
}
/* B and D aren't in NORMAL, MAZI, SAIKYO or CLASSIC: drawn in the font's own pens (by row: f at the top
   to b, the 9 of its shadow on the right and along the foot), D as the O with its left side squared off,
   B as the R's top over a second bowl */
static const char *const tile_glyph[2][16] = {
    { "D", "fffffff9..", "ffffffff9.", "ffd999dff9", "ee9....ee9", "ee9....ee9", "dd9....dd9", "dd9....dd9",
      "cc9....cc9", "cc9....cc9", "bb9....bb9", "bb9...bbb9", "bbbbbbbb9a", "bbbbbbb9a.", "9999999a.." },
    { "B", "fffffff9..", "ffffffff9.", "ff9999dff9", "ee9....ee9", "ee9...cee9", "dddddddc9.", "ddddddddc9",
      "cc9999dcc9", "cc9....cc9", "bb9....bb9", "bb9...abb9", "bbbbbbbbb9", "bbbbbbbb9a", "99999999a." },
};
#define SISM_LETTER_GAP 4                         /* between S-ISM and its letter */
static void make_sism(void) {
    uint32_t lst = RW(SLOT_LIST), set = RW(TILE_SETS8);
    sism_state = -1;
    if (!lst || !set) return;
    for (int k = 0; k < 4; k++) if (!name_pixels(lst, set, k)) return;
    uint8_t out[16][64]; memset(out, 0, sizeof out);
    struct { int k, nth; } parts[4] = { { 2, 0 }, { 1, 3 }, { 2, 0 }, { 1, 0 } };   /* S (SAIKYO), I (MAZI), S, M (MAZI) */
    int xs[4], ws[4], total = 0, hy_w = 6;
    for (int i = 0; i < 4; i++) { ws[i] = letter(parts[i].k, parts[i].nth, &xs[i]); if (!ws[i]) return; total += ws[i] + 1; }
    total += hy_w + 1 - 1;
    /* each S-ISM's letter: by colour (green C, blue B, pink D, red A); C is CLASSIC's first, A MAZI's second */
    static const char sism_letter[4] = { 'C', 'B', 'D', 'A' };
    int cx, cw = letter(3, 0, &cx), ax, aw = letter(1, 1, &ax), lw = 10;
    if (!cw || !aw) return;
    if (cw > 10) cw = 10;                          /* (CLASSIC's C touches its L: the C is 10 across) */
    if (cw > lw) lw = cw;
    if (aw > lw) lw = aw;
    int x = (64 - (total + SISM_LETTER_GAP + lw)) / 2;
    if (x < 0) x = 0;
    for (int i = 0; i < 4; i++) {
        if (i == 1) {                              /* the hyphen: the S's middle stroke, rows 6 - 9 */
            int sx0 = xs[0] + ws[0] / 2;
            for (int y = 6; y <= 9; y++) for (int d = 0; d < hy_w; d++) out[y][x + d] = name_px[2][y][sx0] ? name_px[2][y][sx0] : name_px[2][y][sx0 - 1];
            x += hy_w + 1;
        }
        for (int y = 0; y < 16; y++) for (int d = 0; d < ws[i]; d++) out[y][x + d] = name_px[parts[i].k][y][xs[i] + d];
        x += ws[i] + 1;
    }
    x += SISM_LETTER_GAP - 1;
    static uint8_t outs[4][16][64];
    for (int k = 0; k < 4; k++) {
        memcpy(outs[k], out, sizeof out);
        char ch = sism_letter[k];
        if (ch == 'C' || ch == 'A') {
            int from = ch == 'C' ? cx : ax, w = ch == 'C' ? cw : aw, src = ch == 'C' ? 3 : 1;
            for (int y = 0; y < 16; y++) for (int d = 0; d < w && x + d < 64; d++) outs[k][y][x + d] = name_px[src][y][from + d];
            continue;
        }
        for (int g = 0; g < 2; g++) if (tile_glyph[g][0][0] == ch)
            for (int r = 0; r < 14; r++) for (int d = 0; tile_glyph[g][1 + r][d] && x + d < 64; d++) {
                char v = tile_glyph[g][1 + r][d];
                if (v != '.') outs[k][1 + r][x + d] = (uint8_t)(v <= '9' ? v - '0' : v - 'a' + 10);
            }
    }
    /* our block: four pictures (12 bytes), their cells (4 x 3 bytes each), four tiles (128 bytes) each */
    sism_block = rt_alloc(4 * 12 + 4 * 12 + 16 * 128 + 16, 1, 16, "anthology s-ism tile");
    if (!sism_block) return;
    sism_pics = sism_block; uint32_t cells = sism_block + 48; sism_tiles = cells + 48;
    for (int k = 0; k < 4; k++) {
        uint32_t src = lst + 12u * (uint32_t)(PIC_NAME + k), c0 = lst + RW(src);
        while (RB(c0) == 0xFF) c0++;
        uint8_t fl = (uint8_t)(RB(c0 + 2) & 0x3F);   /* that name's palette and blend, unflipped */
        uint32_t p = sism_pics + 12u * (uint32_t)k, cl = cells + 12u * (uint32_t)k;
        WW(p, SISM_BIAS + (cl - sism_block)); WB(p + 4, 4); WB(p + 5, 1); WH(p + 6, RH(src + 6)); WH(p + 8, RH(src + 8)); WH(p + 10, 0);
        for (int i = 0; i < 4; i++) { WB(cl + 3u * (uint32_t)i, 0); WB(cl + 3u * (uint32_t)i + 1, (uint32_t)(k * 4 + i)); WB(cl + 3u * (uint32_t)i + 2, fl); }
    }
    for (int k = 0; k < 4; k++) for (int i = 0; i < 4; i++) for (int y = 0; y < 16; y++) for (int xx = 0; xx < 16; xx += 2)
        WB(sism_tiles + (uint32_t)((k * 4 + i) * 128 + y * 8 + xx / 2), (uint32_t)(outs[k][y][i * 16 + xx] | outs[k][y][i * 16 + xx + 1] << 4));
    sism_state = 1;
    rt_log("anthology: the S-ISM tile's name made from the game's letters");
}
/* 0x0880EE34 for our S-ISM names (anthology.c asks): their tiles copied into the cache's page */
int anthology_select_fill_labels(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page);
int anthology_select_fill_tapes(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page);
int anthology_select_fill_small(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page) {
    extern void f_089E155C(Cpu *c); extern void f_089E1234(Cpu *c);
    if (anthology_select_fill_labels(c, ds, pic, base, page)) return 1;
    if (anthology_select_fill_tapes(c, ds, pic, base, page)) return 1;
    if (sism_state != 1 || pic < sism_pics || pic >= sism_pics + 48u) return 0;
    uint32_t cell = base + RW(pic);
    uint32_t pxs = call3(c, f_089E155C, RW(page + 8), 0, 0), to = pxs;
    WW(ds + 64, 0);
    for (int n = 0; n < 4; n++) {
        uint32_t id = RB(cell + 1); cell += 3;
        uint32_t place = RW(0x08A042ECu + (((to - pxs) >> 7) & 0xFF) * 4u);
        for (uint32_t y = 0; y < 16; y++) memcpy(HOST(pxs + place + y * 32u), HOST(sism_tiles + id * 128u + y * 8u), 8);
        to += 128; WW(ds + 64, RW(ds + 64) + 1);
    }
    call3(c, f_089E1234, RW(page + 8), RB(page + 13), 0);
    return 1;
}

static void draw_tile(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t lst, int side, int t, int y, float scale, int alpha) {
    int k = t <= T_CLASSIC ? t : t - T_SISM_G;     /* the style whose tile (and colour) it is */
    int i = k * 2;                                 /* its frame's draw state, then its name's */
    int fx = (int16_t)(kept[side][i][0] | kept[side][i][1] << 8), fy = (int16_t)(kept[side][i][2] | kept[side][i][3] << 8);
    for (int part = 0; part < 2; part++) {
        uint8_t *st = kept[side][i + part];
        int dx = (int16_t)(st[0] | st[1] << 8) - fx, dy = (int16_t)(st[2] | st[3] << 8) - fy;
        memcpy(HOST(ds), st, DS_SIZE);
        uint8_t keep12 = (uint8_t)RB(sprite + 12), keep13 = (uint8_t)RB(sprite + 13);
        WB(sprite + 12, kept_sp[side][i + part][0]); WB(sprite + 13, kept_sp[side][i + part][1]);
        WH(ds + 0, (uint32_t)(fx + dx)); WH(ds + 2, (uint32_t)(y + dy));
        WB(ds + 51, (uint32_t)alpha); WB(ds + 52, (uint32_t)alpha);
        uint32_t pic = lst + 12u * (uint32_t)(part == 0 ? PIC_FRAME : PIC_NAME + k);
        uint32_t base = lst;
        if (part == 1 && t >= T_SISM_G) {
            if (sism_state == 0) make_sism();
            if (sism_state != 1) continue;
            pic = sism_pics + 12u * (uint32_t)k; base = sism_block - SISM_BIAS;
        }
        sprite_grid_scale = scale; sprite_grid_ax = (float)fx * sx + ox; sprite_grid_ay = (float)y * sy + oy; sprite_grid_scale_all = 1;
        uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
        c->r[4] = ds; c->r[5] = sprite; c->r[6] = pic; c->r[7] = base; c->r[8] = 0;
        in_tile = 1; f_0880E150(c); in_tile = 0;
        memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
        sprite_grid_scale = 0.0f; sprite_grid_scale_all = 0;
        WB(sprite + 12, keep12); WB(sprite + 13, keep13);
    }
}
static int carousel_mid(int side) {               /* between MAZI's row and SAIKYO's */
    return (int16_t)(kept[side][2][2] | kept[side][2][3] << 8) / 2 + (int16_t)(kept[side][4][2] | kept[side][4][3] << 8) / 2;   /* (frames: MAZI 2, SAIKYO 4) */
}

/* ---------------------------------------------------------------- ALPHA SELECT and GUARD SELECT in the game's panels
 * The MODE panel (its tiles 139 - 143, drawn by the panel's sprite) and the ISM panel (93 - 101) slide
 * in and out as the game moves them; while one of them stands for ALPHA SELECT or GUARD SELECT its
 * pictures still run (out of sight: the next one's palette base follows from each) and our list is
 * drawn where the panel is, its centre a fixed way from the panel's MAZI tile (141) or its first ISM
 * picture (93; player 2's 102); the game's arrow (119; player 2's 120) goes to the list's row, and for
 * GUARD SELECT, whose panel has none, it is drawn by the same sprite in place of the ISM panel's up arrow. The tapes (144 MODE SELECT, 282 ISM SELECT, a column of 16 pixels across) are
 * swapped for ours, the same tape with the word spelt in the tape's own letters (14 x 5 pixels each:
 * SELECT, D and L are the game's, A, P, H, G, U and R drawn as they are). ISM SELECT's tape repeats
 * every 64 pixels, ours (GUARD SELECT, as long as MODE SELECT's) every 80: it is drawn a quarter
 * further down the screen for each of its copies, so its copies meet and it scrolls a little faster. */
#define PIC_MODE_REF 141                        /* MAZI's name: never the one chosen (F_STYLE stays NORMAL) */
#define PIC_ISM_REF  93                         /* (player 2's: 102) */
#define PIC_ISM_LO   93                         /* the ISM panels' pictures: player 1's 93 - 101, player 2's 101 - 108 */
#define PIC_ISM_HI   108
#define PIC_ISM_ARROWS 96                       /* the ISM panel's up arrow (player 2's 105): the same sprite as MODE's arrow */
#define PIC_ARROW_P2 120                        /* player 2's arrow (and name strip, 245 / 281) */
#define PIC_TAPE_MODE 144                       /* MODE SELECT: 1 x 5 cells of 16 */
#define PIC_TAPE_ISM  282                       /* ISM SELECT: 1 x 4 */
#define MODE_REF_CX  4                          /* the list's centre from the panel's reference picture */
#define ISM_REF_CX   12
#define TAPE_ROWS    80
static uint32_t tape_block, tape_pics, tape_tiles;
static int tape_state;                          /* 0 not made, 1 made, -1 can't */
static int fill_cells(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page, uint32_t tiles);
/* the tape's letters drawn ('#' the letter: the tape's pen 0, its holes) */
static const char *const tape_glyph[][6] = {
    { "A", ".############.", "#####....#####", "##############", "#####....#####", "#####....#####" },
    { "P", "#############.", "######...#####", "#############.", "######........", "######........" },
    { "H", "#####....#####", "#####....#####", "##############", "#####....#####", "#####....#####" },
    { "G", ".#############", "######........", "######..######", "######...#####", ".#############" },
    { "U", "#####....#####", "#####....#####", "#####....#####", "#####....#####", ".############." },
    { "R", "#############.", "######...#####", "#############.", "######..#####.", "######...#####" },
};
static void make_tapes(void) {
    tape_state = -1;
    uint32_t lst = RW(SLOT_LIST), set = RW(TILE_SETS8);
    if (!lst || !set) return;
    uint32_t pic = lst + 12u * PIC_TAPE_MODE, cl = lst + RW(pic);
    if (RB(pic + 4) != 1 || RB(pic + 5) != 5) { rt_warn("anthology: the MODE SELECT tape isn't as expected"); return; }
    static uint8_t src[TAPE_ROWS][16], out[2][TAPE_ROWS][16];
    uint8_t fl = 0;
    for (int i = 0; i < 5; i++) {
        if (RB(cl) == 0xFF) return;
        uint32_t tile = RB(cl) << 8 | RB(cl + 1); fl = (uint8_t)RB(cl + 2); cl += 3;
        uint8_t t[16][16]; unpack16(set, tile, t);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) src[i * 16 + y][x] = t[(fl & 0x80) ? 15 - y : y][(fl & 0x40) ? 15 - x : x];
    }
    /* MODE SELECT: rows 1 - 5 M, 7 - 12 O, 14 - 18 D, 20 - 24 E, 31 - 65 SELECT (L at 43 - 47), the rest tape */
    int pen = src[0][0];                        /* the tape */
    if (src[14][1] != 0 || src[43][1] != 0 || src[31][1] != 0 || pen == 0) { rt_warn("anthology: the MODE SELECT tape's letters aren't where expected"); return; }
    static const char *const words[2] = { "ALPHL", "GUARD" };   /* L and D the game's ('L' row 43, 'D' row 14) */
    static const char *const word2[2] = { "ALPHA", "GUARD" };
    for (int w = 0; w < 2; w++) {
        for (int y = 0; y < TAPE_ROWS; y++) for (int x = 0; x < 16; x++) out[w][y][x] = (uint8_t)pen;
        for (int k = 0; k < 5; k++) {
            int y0 = 1 + 6 * k;
            char ch = word2[w][k];
            if (ch == 'L' || ch == 'D') {
                int from = ch == 'L' ? 43 : 14;
                for (int y = 0; y < 5; y++) memcpy(out[w][y0 + y], src[from + y], 16);
                continue;
            }
            for (size_t g = 0; g < sizeof tape_glyph / sizeof tape_glyph[0]; g++) if (tape_glyph[g][0][0] == ch)
                for (int y = 0; y < 5; y++) for (int x = 0; x < 14; x++) if (tape_glyph[g][1 + y][x] == '#') out[w][y0 + y][1 + x] = 0;
        }
        for (int y = 31; y <= 65; y++) memcpy(out[w][y + 5], src[y], 16);   /* SELECT, 5 rows down */
    }
    (void)words;
    tape_block = rt_alloc(2 * 12 + 2 * 5 * 3 + 2 + 2 * 5 * 128 + 16, 1, 16, "anthology tapes");
    if (!tape_block) return;
    tape_pics = tape_block; uint32_t cells = tape_block + 24; tape_tiles = (cells + 30 + 15) & ~15u;
    for (int w = 0; w < 2; w++) {
        uint32_t p = tape_pics + 12u * (uint32_t)w, c0 = cells + 15u * (uint32_t)w;
        WW(p, SISM_BIAS + 0x200000u + (c0 - tape_block)); WB(p + 4, 1); WB(p + 5, 5);
        WH(p + 6, RH((w == 0 ? pic : lst + 12u * PIC_TAPE_ISM) + 6)); WH(p + 8, w == 0 ? RH(pic + 8) : TAPE_ROWS); WH(p + 10, 0);   /* GUARD's origin at its foot, as ISM SELECT's */
        for (int i = 0; i < 5; i++) {
            int id = w * 5 + i;
            WB(c0 + 3u * (uint32_t)i, 0); WB(c0 + 3u * (uint32_t)i + 1, (uint32_t)id); WB(c0 + 3u * (uint32_t)i + 2, fl & 0x3F);
            for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x += 2)
                WB(tape_tiles + (uint32_t)(id * 128 + y * 8 + x / 2), (uint32_t)(out[w][i * 16 + y][x] | out[w][i * 16 + y][x + 1] << 4));
        }
    }
    tape_state = 1;
    rt_log("anthology: the ALPHA SELECT and GUARD SELECT tapes made from the game's tape");
}
int anthology_select_fill_tapes(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page) {
    if (tape_state != 1 || pic < tape_pics || pic >= tape_pics + 24u) return 0;
    return fill_cells(c, ds, pic, base, page, tape_tiles);
}
/* a call of the game's own, with another picture (and list) or place */
static void draw_as(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t base) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = ds; c->r[5] = sprite; c->r[6] = pic; c->r[7] = base; c->r[8] = 0;
    in_tile = 1; f_0880E150(c); in_tile = 0;
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
static void out_of_sight(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst) {
    uint32_t x = RH(ds + 0);
    WH(ds + 0, (uint32_t)-2000);
    draw_as(c, ds, sprite, pic, lst);
    WH(ds + 0, x);
}
/* ALPHA3 chosen: the MODE panel's pieces moved out and in again as the game moves them (its MODE panel
   going out after a choice and coming in, frame by frame, towards the player's edge: the tiles and the
   arrow, the tape, the name strip), the list going with them */
static const int8_t out_tiles[BACK_OUT] = { -1, -3, -5, -8, -12, -17, -23, -29, -36, -44, -53, -63, -73, -84, -96, -109 };
static const int8_t out_tape[BACK_OUT]  = { -1, -2, -3, -4, -6, -8, -10, -13, -16, -20, -24, -28, -32, -37, -43, -50 };
static const int8_t out_strip[BACK_OUT] = { -1, -2, -3, -5, -8, -10, -14, -17, -22, -26, -31, -37, -43, -50, -57, -64 };
static const int8_t in_tiles[BACK_IN]   = { -88, -76, -64, -54, -44, -36, -28, -20, -14, -8, -4, 0 };
static const int8_t in_tape[BACK_IN]    = { -40, -35, -30, -25, -20, -16, -12, -8, -5, -2, 0, 0 };
static const int8_t in_strip[BACK_IN]   = { -56, -49, -42, -35, -29, -24, -18, -14, -9, -6, -2, 0 };
static const int8_t in_arrow[BACK_IN]   = { -92, -80, -68, -58, -48, -40, -32, -24, -18, -12, -8, 0 };
static int back_offset(const Player *p, int n) {
    int kind = (n >= PIC_FRAME && n <= PIC_NAME + 3) ? 0 : (n == PIC_ARROW || n == PIC_ARROW_P2) ? 3 : n == PIC_TAPE_MODE ? 1
             : (n == 244 || n == 280 || n == 245 || n == 281) ? 2 : -1;
    if (kind < 0) return 0;
    if (p->phase == P_BACK) {
        uint32_t t = frame_now() - p->back;
        if (t >= BACK_OUT) return -400;
        return kind == 1 ? out_tape[t] : kind == 2 ? out_strip[t] : out_tiles[t];
    }
    if (p->phase == P_DONE && p->back_in) {
        uint32_t t = frame_now() - p->back_in;
        if (t >= BACK_IN) return 0;
        return kind == 0 ? in_tiles[t] : kind == 1 ? in_tape[t] : kind == 2 ? in_strip[t] : in_arrow[t];
    }
    return 0;
}
static int steps_panel(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst) {
    int n = (int)((pic - lst) / 12u), x = (int16_t)RH(ds + 0), y = (int16_t)RH(ds + 2);
    int side = x >= 192 ? 1 : 0;
    Player *p = &pl[side];
    int in_mode = p->phase >= P_VERSION && p->phase <= P_GCHOSEN;   /* the MODE panel stands for ALPHA SELECT */
    int in_ism = p->phase == P_GUARD || p->phase == P_GCHOSEN;        /* the ISM panel for GUARD SELECT */
    if (!in_mode) return 0;
    uint32_t me = FIGHTER(side);
    const char *items[V_COUNT]; int colours[V_COUNT], at = 0;
    if (n == PIC_TAPE_MODE || (n == PIC_TAPE_ISM && in_ism)) {
        if (tape_state == 0) make_tapes();
        if (tape_state != 1) return 0;
        if (n == PIC_TAPE_MODE) { draw_as(c, ds, sprite, tape_pics, tape_block - SISM_BIAS - 0x200000u); return 1; }
        int top = y - 64, top2 = top + (top >= 0 ? top : top - 3) / 4;   /* 64 apart become 80 apart */
        WH(ds + 2, (uint32_t)(top2 + TAPE_ROWS));
        draw_as(c, ds, sprite, tape_pics + 12u, tape_block - SISM_BIAS - 0x200000u);
        WH(ds + 2, (uint32_t)y);
        return 1;
    }
    if (n >= PIC_FRAME && n <= PIC_NAME + 3) {
        out_of_sight(c, ds, sprite, pic, lst);
        if (n == PIC_MODE_REF) {
            int cx = x + (side ? -MODE_REF_CX : MODE_REF_CX), nv;
            p->mode_cx = cx; p->mode_cx_ok = 1;
            nv = version_list((int8_t)RB(me + F_CHARACTER), p->cursor, items, colours, &at);
            draw_list(c, cx, items, colours, nv);
        }
        return 1;
    }
    if (n == (side ? PIC_ARROW_P2 : PIC_ARROW)) {   /* drawn at the list's row */
        if (p->mode_cx_ok) { p->arrow_dx = x - p->mode_cx; p->arrow_seen = 1; }
        if (p->phase != P_VERSION && p->phase != P_VCHOSEN && p->phase != P_BACK) { out_of_sight(c, ds, sprite, pic, lst); return 1; }
        int nv = version_list((int8_t)RB(me + F_CHARACTER), p->cursor, items, colours, &at);
        WH(ds + 2, (uint32_t)(row_y(nv, at) + 8));
        draw_as(c, ds, sprite, pic, lst);
        WH(ds + 2, (uint32_t)y);
        return 1;
    }
    if (in_ism && n >= PIC_ISM_LO && n <= PIC_ISM_HI) {
        out_of_sight(c, ds, sprite, pic, lst);
        if (n == (side ? PIC_ISM_REF + 9 : PIC_ISM_REF)) {
            int cx = x + (side ? -ISM_REF_CX : ISM_REF_CX), ng = guard_list(p, items, colours, &at);
            draw_list(c, cx, items, colours, ng);
            p->guard_ax = cx + p->arrow_dx; p->guard_ay = row_y(ng, at) + 8;
        }
        if (n == (side ? PIC_ISM_ARROWS + 9 : PIC_ISM_ARROWS) && p->arrow_seen) {   /* the arrow, with this sprite's own colours */
            WH(ds + 0, (uint32_t)p->guard_ax); WH(ds + 2, (uint32_t)p->guard_ay);
            draw_as(c, ds, sprite, lst + 12u * (uint32_t)(side ? PIC_ARROW_P2 : PIC_ARROW), lst);
            WH(ds + 0, (uint32_t)x); WH(ds + 2, (uint32_t)y);
        }
        return 1;
    }
    return 0;
}
/* 0x0880E150 (anthology.c asks): 1 when a call is the MODE panel's and the carousel takes it */
static int steps_panel(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst);
static int vs_label(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst);
int anthology_select_tile(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst) {
    if (in_tile || !sprite || lst != RW(SLOT_LIST)) return 0;
    if (anthology_select_available() && vs_label(c, ds, sprite, pic, lst)) return 1;
    if (!screen_ok()) return 0;
    if (pic >= lst && (pic - lst) % 12u == 0) {
        int x = (int16_t)RH(ds + 0), side = x >= 192 ? 1 : 0, off = back_offset(&pl[side], (int)((pic - lst) / 12u));
        if (off) WH(ds + 0, (uint32_t)(x + (side ? -off : off)));   /* (player 2's side: towards the right) */
        if (steps_panel(c, ds, sprite, pic, lst)) return 1;
    }
    if (!rt_sism_select) return 0;
    int n = (int)((pic - lst) / 12u);
    if (pic < lst || (pic - lst) % 12u || ((n < PIC_FRAME || n > PIC_NAME + 3) && n != PIC_ARROW && n != PIC_ARROW_P2)) return 0;
    int side = (int16_t)RH(ds + 0) >= 192 ? 1 : 0;
    Player *p = &pl[side];
    if (!p->carousel || frame_now() - p->seen > 2) return 0;
    if (n == PIC_ARROW || n == PIC_ARROW_P2) {     /* the arrow: at the carousel's front tile */
        if (!kept_frame[side]) return 0;
        uint32_t y = RH(ds + 2);
        WH(ds + 2, (uint32_t)carousel_mid(side));
        uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
        in_tile = 1; f_0880E150(c); in_tile = 0;
        memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
        WH(ds + 2, y);
        return 1;
    }
    uint32_t now = frame_now();
    if (kept_frame[side] != now) { kept_frame[side] = now; calls[side] = 0; }
    {   /* kept by style: a frame's call into slot 0 for now, then with its name's (140 + style) into the style's pair */
        int at = n == PIC_FRAME ? 8 : (n - PIC_NAME) * 2 + 1;
        memcpy(kept[side][at], HOST(ds), DS_SIZE);
        kept_sp[side][at][0] = (uint8_t)RB(sprite + 12); kept_sp[side][at][1] = (uint8_t)RB(sprite + 13);
        if (n != PIC_FRAME) { memcpy(kept[side][at - 1], kept[side][8], DS_SIZE); memcpy(kept_sp[side][at - 1], kept_sp[side][8], 2); }
    }
    calls[side]++;
    {   /* the game's own call still runs (its sprite's palette base for the next one depends on it), out of sight */
        uint32_t x = RH(ds + 0), keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
        WH(ds + 0, (uint32_t)-2000);
        in_tile = 1; f_0880E150(c); in_tile = 0;
        memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
        WH(ds + 0, x);
    }
    if (calls[side] != 8) return 1;                /* kept; the last one draws them all */
    uint8_t keep_ds[DS_SIZE]; memcpy(keep_ds, HOST(ds), DS_SIZE);
    sx = ffrom(RW(0x08E759E8)); sy = ffrom(RW(0x08E759EC)); ox = ffrom(RW(0x08E75A04)); oy = ffrom(RW(0x08E75A0C));
    int nt = p->ntiles, before = p->tiles[(p->at + nt - 1) % nt], after = p->tiles[(p->at + 1) % nt], at = p->tiles[p->at];
    int mid = carousel_mid(side);
    if (nt > 2) draw_tile(c, ds, sprite, lst, side, before, mid - 26, 0.8f, 0x60);
    draw_tile(c, ds, sprite, lst, side, after, mid + 26, 0.8f, 0x60);
    draw_tile(c, ds, sprite, lst, side, at, mid, 1.0f, 0xC0);
    memcpy(HOST(ds), keep_ds, DS_SIZE);
    return 1;
}

/* ---------------------------------------------------------------- the VS screen's labels
 * Under each name the VS screen shows the ISM (0x08898F5C): a picture of slot 8's list, 521 - 525 (X-ISM,
 * the unused Z-ISM, V-ISM, A-ISM, I-ISM), 7 x 1 cells of 16, in a bold font whose rows run from pen 9
 * (top) to 15 (bottom), coloured by the player's label palette (the sprite's +13: 12, 13). Hyper Street
 * Fighter Alpha shows the version there instead (ALPHA, ALPHA2, ALPHA2G; the maintainer's screenshots)
 * and S-ISM for the S-ISMs; so does this, in the same font: A, the dash, I, S and M are the game's own
 * (cut out of A-ISM), L, P, H, 2 and G are drawn in its strokes (10-pixel stems, 2-row bars, its rounded
 * corners) with the pens of their rows. The palette's pens 9 - 15 are set to the label's colour (a
 * gradient as Hyper's, the same for both players: LABEL_RGB): ALPHA, ALPHA2, ALPHA2G green, ALPHA2G for a Classical version
 * blue; S-ISM green, blue, pink or red. The game's colours come back once the screen is gone. */
#define PIC_LABEL0  521                         /* X-ISM (521) ... I-ISM (525) */
#define PIC_A_ISM   524
#define PALETTES    0x092DF980u                 /* 16 colours (u16) a palette */
#define PALETTES_DIRTY 0x092E0980u
#define LABEL_W     192                         /* our labels' pixels across (12 cells) */
#define LABEL_FIT   154                         /* the widest that stays clear of the VS bar */
enum { L_ALPHA, L_ALPHA2, L_ALPHA2G, L_SISM, L_COUNT };
static uint32_t lab_block, lab_pics, lab_tiles;
static int lab_state;                           /* 0 not made, 1 made, -1 can't */
static int lab_w[L_COUNT];                      /* each label's width */
static uint32_t lab_seen;                       /* the frame a label of ours was last drawn */
static struct { int on; uint8_t pal; uint16_t colour[16]; } lab_kept[2];

/* a picture's pixels (cells of 16, one row of them) into rows of colour indices */
static int picture_pixels(uint32_t lst, uint32_t set, int n, uint8_t out[16][112]) {
    uint32_t pic = lst + 12u * (uint32_t)n, cells = lst + RW(pic);
    int cols = (int)RB(pic + 4);
    if (cols > 7 || RB(pic + 5) != 1) return 0;
    memset(out, 0, 16 * 112);
    for (int i = 0; i < cols; i++) {
        if (RB(cells) == 0xFF) { cells++; continue; }
        uint32_t tile = RB(cells) << 8 | RB(cells + 1); uint8_t fl = (uint8_t)RB(cells + 2); cells += 3;
        uint8_t t[16][16]; unpack16(set, tile, t);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++)
            out[y][i * 16 + x] = t[(fl & 0x80) ? 15 - y : y][(fl & 0x40) ? 15 - x : x];
    }
    return 1;
}
/* the drawn letters, 14 rows (the font's 1 - 14), '#' lit */
static const char *const drawn_glyph[][15] = {
    { "L", "##########..............", "##########..............", "##########..............", "##########..............",
      "##########..............", "##########..............", "##########..............", "##########..............",
      "##########..............", "##########..............", "##########..............", "##########..............",
      "########################", "########################" },
    { "P", "....##################....", "..######################..", "##########......##########", "##########......##########",
      "##########......##########", "##########......##########", "##########......##########", "##########......##########",
      "##########################", "########################..", "##########................", "##########................",
      "##########................", "##########................" },
    { "H", "##########.......##########", "##########.......##########", "##########.......##########", "##########.......##########",
      "##########.......##########", "##########.......##########", "##########.......##########", "##########.......##########",
      "###########################", "###########################", "##########.......##########", "##########.......##########",
      "##########.......##########", "##########.......##########" },
    { "2", "....###################....", "..#######################..", ".................##########", ".................##########",
      ".................##########", ".................##########", "...######################..", ".#######################...",
      "##########.................", "##########.................", "##########.................", "##########.................",
      "###########################", "###########################" },
    { "G", "....###################....", "..#######################..", "##########.......##########", "##########.......##########",
      "##########.................", "##########.................", "##########....#############", "##########....#############",
      "##########.......##########", "##########.......##########", "##########.......##########", "##########.......##########",
      "..#######################..", "....###################...." },
};
static void make_labels(void) {
    lab_state = -1;
    uint32_t lst = RW(SLOT_LIST), set = RW(TILE_SETS8);
    if (!lst || !set) return;
    static uint8_t src[16][112];
    if (!picture_pixels(lst, set, PIC_A_ISM, src)) return;
    /* A-ISM's letters: runs of lit columns, in order A, -, I, S, M */
    int xs[8], ws[8], n = 0, in = 0;
    for (int x = 0; x <= 112 && n < 8; x++) {
        int any = 0;
        for (int y = 0; x < 112 && y < 16; y++) if (src[y][x]) { any = 1; break; }
        if (any && !in) { in = 1; xs[n] = x; }
        if (!any && in) { in = 0; ws[n] = x - xs[n]; n++; }
    }
    if (n != 5) { rt_warn("anthology: A-ISM isn't as expected (%d letters)", n); return; }
    uint8_t pen[16];                              /* the font's pen by row: the I's */
    for (int y = 0; y < 16; y++) pen[y] = src[y][xs[2] + ws[2] / 2];
    static const char *const words[L_COUNT] = { "ALPHA", "ALPHA2", "ALPHA2G", "S-ISM" };
    static uint8_t out[L_COUNT][16][LABEL_W + 32];   /* (wider while it is made: condensed to fit below) */
    memset(out, 0, sizeof out);
    for (int w = 0; w < L_COUNT; w++) {
        int x = 0;
        for (const char *p = words[w]; *p; p++) {
            int k = *p == 'A' ? 0 : *p == '-' ? 1 : *p == 'I' ? 2 : *p == 'S' ? 3 : *p == 'M' ? 4 : -1;
            if (k >= 0) {
                for (int y = 0; y < 16; y++) for (int d = 0; d < ws[k] && x + d < LABEL_W + 32; d++) out[w][y][x + d] = src[y][xs[k] + d];
                x += ws[k] + 2;
                continue;
            }
            for (size_t g = 0; g < sizeof drawn_glyph / sizeof drawn_glyph[0]; g++) if (drawn_glyph[g][0][0] == *p) {
                int gw = (int)strlen(drawn_glyph[g][1]);
                for (int r = 0; r < 14; r++) for (int d = 0; d < gw && x + d < LABEL_W + 32; d++)
                    if (drawn_glyph[g][1 + r][d] == '#') out[w][1 + r][x + d] = pen[1 + r];
                x += gw + (strlen(words[w]) > 5 ? 1 : 2);
            }
        }
        lab_w[w] = x - (strlen(words[w]) > 5 ? 1 : 2);
        /* too wide to stay clear of the VS bar (it starts 160 units in; the label 2 from the edge): narrowed by
           leaving out columns that repeat the one before (inside the stems and bars), spread along the word */
        int over = lab_w[w] - LABEL_FIT;
        if (over > 0) {
            int same[LABEL_W + 32], ns = 0;
            for (int x2 = 1; x2 < lab_w[w]; x2++) {
                int eq = 1;
                for (int y = 0; y < 16 && eq; y++) eq = out[w][y][x2] == out[w][y][x2 - 1];
                int lit = 0;
                for (int y = 0; y < 16; y++) lit |= out[w][y][x2];
                if (eq && lit) same[ns++] = x2;
            }
            if (over > ns) over = ns;
            uint8_t drop[LABEL_W + 32]; memset(drop, 0, sizeof drop);
            for (int k = 0; k < over; k++) drop[same[(k * ns + ns / 2) / over]] = 1;
            int to = 0;
            for (int x2 = 0; x2 < LABEL_W + 32; x2++) {
                if (x2 < lab_w[w] && drop[x2]) continue;
                for (int y = 0; y < 16; y++) out[w][y][to] = x2 < lab_w[w] ? out[w][y][x2] : 0;
                to++;
            }
            for (; to < LABEL_W + 32; to++) for (int y = 0; y < 16; y++) out[w][y][to] = 0;
            lab_w[w] -= over;
        }
    }
    uint32_t pic0 = lst + 12u * PIC_A_ISM, c0 = lst + RW(pic0);
    while (RB(c0) == 0xFF) c0++;
    uint8_t fl = (uint8_t)(RB(c0 + 2) & 0x3F);
    int cells = LABEL_W / 16;
    lab_block = rt_alloc((uint32_t)(L_COUNT * 12 + L_COUNT * cells * 3 + 4 + L_COUNT * cells * 128), 1, 16, "anthology vs labels");
    if (!lab_block) return;
    lab_pics = lab_block; uint32_t cl = lab_block + L_COUNT * 12; lab_tiles = (cl + (uint32_t)(L_COUNT * cells * 3) + 3) & ~3u;
    for (int w = 0; w < L_COUNT; w++) {
        uint32_t p = lab_pics + 12u * (uint32_t)w, c = cl + (uint32_t)(w * cells * 3);
        WW(p, SISM_BIAS + 0x100000u + (c - lab_block)); WB(p + 4, (uint32_t)cells); WB(p + 5, 1); WH(p + 6, 0); WH(p + 8, 0); WH(p + 10, 0);
        for (int i = 0; i < cells; i++) {
            int id = w * cells + i;
            WB(c + 3u * (uint32_t)i, (uint32_t)id >> 8); WB(c + 3u * (uint32_t)i + 1, (uint32_t)id & 0xFF); WB(c + 3u * (uint32_t)i + 2, fl);
            for (int y = 0; y < 16; y++) for (int xx = 0; xx < 16; xx += 2)
                WB(lab_tiles + (uint32_t)(id * 128 + y * 8 + xx / 2), (uint32_t)(out[w][y][i * 16 + xx] | out[w][y][i * 16 + xx + 1] << 4));
        }
    }
    lab_state = 1;
    rt_log("anthology: the VS labels made (ALPHA %d, ALPHA2 %d, ALPHA2G %d, S-ISM %d pixels)", lab_w[0], lab_w[1], lab_w[2], lab_w[3]);
}
static int fill_cells(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page, uint32_t tiles) {
    extern void f_089E155C(Cpu *c); extern void f_089E1234(Cpu *c);
    uint32_t cell = base + RW(pic);
    int n = (int)(RB(pic + 4) * RB(pic + 5));
    uint32_t pxs = call3(c, f_089E155C, RW(page + 8), 0, 0), to = pxs;
    WW(ds + 64, 0);
    for (int k = 0; k < n; k++) {
        uint32_t id = RB(cell) << 8 | RB(cell + 1); cell += 3;
        uint32_t place = RW(0x08A042ECu + (((to - pxs) >> 7) & 0xFF) * 4u);
        for (uint32_t y = 0; y < 16; y++) memcpy(HOST(pxs + place + y * 32u), HOST(tiles + id * 128u + y * 8u), 8);
        to += 128;
        uint32_t fill = RW(ds + 64) + 1; WW(ds + 64, fill);
        if (fill >= 16 && k != n - 1 && page && RW(page)) {
            call3(c, f_089E1234, RW(page + 8), RB(page + 13), 0);
            page = RW(page);
            pxs = to = call3(c, f_089E155C, RW(page + 8), 0, 0);
            WW(ds + 64, 0);
        }
    }
    call3(c, f_089E1234, RW(page + 8), RB(page + 13), 0);
    return 1;
}
int anthology_select_fill_labels(Cpu *c, uint32_t ds, uint32_t pic, uint32_t base, uint32_t page) {
    if (lab_state != 1 || pic < lab_pics || pic >= lab_pics + 12u * L_COUNT) return 0;
    return fill_cells(c, ds, pic, base, page, lab_tiles);
}
/* Our labels' colours, the same for both players (the game's label palettes differ by side, 12 green,
   13 blue, so turning their hue gave player 1 and player 2 different shades): pens 9 (the top rows) to 15
   (the foot), as Hyper's S-ISM labels (the maintainer's screenshot, 2026-11-26): orange-red to red, light
   to darker green, light to deeper blue; pink as red's, in S-ISM D's pink. */
enum { LC_GREEN, LC_BLUE, LC_PINK, LC_RED };
static const uint8_t LABEL_RGB[4][7][3] = {
    { { 0, 205, 0 }, { 0, 190, 0 }, { 2, 184, 0 }, { 2, 172, 0 }, { 2, 166, 2 }, { 1, 152, 1 }, { 0, 140, 0 } },
    { { 6, 122, 255 }, { 0, 106, 253 }, { 2, 96, 255 }, { 4, 84, 255 }, { 2, 70, 255 }, { 2, 58, 255 }, { 2, 48, 240 } },
    { { 255, 120, 215 }, { 250, 98, 205 }, { 246, 78, 196 }, { 238, 58, 186 }, { 228, 40, 176 }, { 212, 26, 162 }, { 196, 20, 150 } },
    { { 253, 102, 4 }, { 255, 84, 3 }, { 255, 64, 1 }, { 255, 33, 3 }, { 253, 14, 3 }, { 236, 0, 0 }, { 220, 4, 7 } },
};
static void palette_label(uint32_t pal, int lc) {
    for (uint32_t i = 0; i < 7; i++) {
        const uint8_t *q = LABEL_RGB[lc][i];
        WH(PALETTES + pal * 32u + (9u + i) * 2u, (uint32_t)(0x8000 | q[0] >> 3 | (q[1] >> 3) << 5 | (q[2] >> 3) << 10));
    }
    WW(PALETTES_DIRTY, 1);
}
/* the player's label: which (L_*, -1 none) and its colour (LC_*) */
static int label_for(uint32_t fighter, int *lc) {
    if (fighter < FIGHTER(0) || (fighter - FIGHTER(0)) % 0x498u || (fighter - FIGHTER(0)) / 0x498u > 3) return -1;
    int version = V_ALPHA3, sism = 0;
    Boxed *b = &bx[(fighter - FIGHTER(0)) / 0x498u];
    int slot = (int8_t)RB(fighter + F_SLOT);
    int from_box = slot < 0 || slot > 1 || b->picked > pl[slot].picked;   /* the select seen last */
    if (from_box) { version = b->version; sism = b->sism; }
    else if (!((int8_t)RB(GAME + G_PRESENT) & (1 << slot))) return -1;   /* the computer's now (a versus match's
                                                                             loser): its own label, as in the fight */
    else { version = pl[slot].version; sism = pl[slot].sism; }
    if (sism) { *lc = sism - 1; return L_SISM; }         /* (S-ISMs green, blue, pink, red: LC_* in that order) */
    *lc = version == V_ALPHA2GC ? LC_BLUE : LC_GREEN;
    return version == V_ALPHA ? L_ALPHA : version == V_ALPHA2 ? L_ALPHA2 : version == V_ALPHA2G || version == V_ALPHA2GC ? L_ALPHA2G : -1;
}
static int vs_label(Cpu *c, uint32_t ds, uint32_t sprite, uint32_t pic, uint32_t lst) {
    int n = (int)((pic - lst) / 12u);
    if (n < PIC_LABEL0 || n > PIC_LABEL0 + 4 || (pic - lst) % 12u) return 0;
    int lc; int w = label_for(RW(sprite + 92), &lc);
    if (w < 0) return 0;
    if (lab_state == 0) make_labels();
    if (lab_state != 1) return 0;
    uint32_t pal = RB(sprite + 13), side = pal & 1;
    if (!lab_kept[side].on || lab_kept[side].pal != pal) {   /* the game's colours kept, to give back */
        lab_kept[side].on = 1; lab_kept[side].pal = (uint8_t)pal;
        for (int i = 0; i < 16; i++) lab_kept[side].colour[i] = (uint16_t)RH(PALETTES + pal * 32u + (uint32_t)i * 2u);
        palette_label(pal, lc);
    }
    lab_seen = frame_now();
    /* in place of the 112-pixel label, centred where it was, kept on the screen once it is in place (its
       slide in and out as the game's) */
    int x = (int16_t)RH(ds + 0);
    int nx = x + 56 - lab_w[w] / 2;
    if (side == 0 && x >= 32 && nx < 2) nx = 2;
    if (side == 1 && x <= 240 && nx + lab_w[w] > 382) nx = 382 - lab_w[w];
    WH(ds + 0, (uint32_t)nx);
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = ds; c->r[5] = sprite; c->r[6] = lab_pics + 12u * (uint32_t)w; c->r[7] = lab_block - SISM_BIAS - 0x100000u; c->r[8] = 0;   /* (the offsets carry 0x100000 more than the S-ISM names': their own cache keys) */
    in_tile = 1; f_0880E150(c); in_tile = 0;
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    WH(ds + 0, (uint32_t)x);
    return 1;
}
/* once the VS screen is gone, the label palettes back as they were */
static void labels_check(void) {
    if (frame_now() - lab_seen <= 2) return;
    for (int s = 0; s < 2; s++) if (lab_kept[s].on) {
        for (int i = 0; i < 16; i++) WH(PALETTES + lab_kept[s].pal * 32u + (uint32_t)i * 2u, lab_kept[s].colour[i]);
        WW(PALETTES_DIRTY, 1);
        lab_kept[s].on = 0;
    }
}

/* ================================================================ The choices, for the fight
 * Each fighter's version, S-ISM and guard as its player chose them (the gameplay's rules read these):
 * in Survival and Training by the fighter's own box, elsewhere by its side's select. A fighter nobody
 * chose for (the computer, or with ALPHA SELECT off) is ALPHA3 with no S-ISM: the game as it is. */
typedef struct { int version, sism, guard; } AnthologyChoice;     /* sism 0 none, 1 - 4 green, blue, pink, red */
AnthologyChoice anthology_fighter_choice(uint32_t me) {
    AnthologyChoice r = { V_ALPHA3, 0, 0 };
    int8_t mode = (int8_t)RB(GAME + G_MODE);
    if (mode == 2 || mode == 7) {
        Boxed *b = box_of(me);
        if (b && b->phase == P_DONE) { r.version = b->version; r.sism = b->sism; r.guard = b->guard; }
        return r;
    }
    if (me != FIGHTER(0) && me != FIGHTER(1)) return r;
    if (!((int8_t)RB(GAME + G_PRESENT) & (1 << slot_of(me)))) return r;   /* the computer's now (a versus match's loser): its own */
    const Player *p = &pl[slot_of(me)];
    if (p->phase == P_DONE || p->phase == P_GCHOSEN) { r.version = p->version; r.sism = p->sism; r.guard = p->guard; }
    return r;
}

/* ================================================================ The S-ISM gauge
 * The maintainer (2026-11-25): the S-ISMs have one bar; S-ISM D (pink), Champion Edition, has none.
 * Alpha 3 already has such a bar: an entry's (the HUD's gauge kind 3, 0x0889192C: one bar, A-ISM's
 * three levels inside it, unmarked), which the Hyper S-ISM A (red, the arcade bosses' "Shadaloo" mode)
 * is. So S-ISM A and C (A-ISM's rules) get kind 3; S-ISM B plays as X-ISM, whose gauge is one bar
 * already; S-ISM D's gauge is kept empty, so it has no supers (its bar not yet hidden). 0x0888DA7C runs
 * the two gauges each frame (o + 12 and o + 32: the fighter, then its kind and the ISM's kind). */
/* The HUD's colours by its gauge kind (0x0888DB10 loads them when it sets the kind up: the gauge's
   into palette 0xF / 0x11, the swoosh behind the portrait and the guard bar's into 0x10 / 0x12, from
   PALS +64: row 0x08A26D3A[kind] past 0x440). An S-ISM's swoosh and guard bar take its colours, as in
   Hyper: A red (X-ISM's kind 0), B blue (V-ISM's 2), C green (A-ISM's 1), D red (X-ISM's, as the PS2
   game; with Extra ISM Presentation the S-ISM D text's purple-pink). The S-ISMs' gauge itself is drawn
   by anthology_hud_bottom, so the kind is only these colours to the HUD. */
#define S_PINK_D 3
static const int8_t SWOOSH_KIND[5] = { 0, 1, 2, 0, 0 };
void f_0880AEA8(Cpu *c);   /* (a0, colours, a2, palette, count): a palette loaded */
#define HUD_PALS     0x092D9E00u
#define SWOOSH_ROWS  0x08A26D3Au
#define PALETTE_RAM  0x092DF980u
#define PALETTE_DIRTY 0x092E0980u
static void swoosh_palette(Cpu *c, int kind, uint32_t side) {
    uint32_t at = RW(HUD_PALS + 64) + (((uint32_t)RB(SWOOSH_ROWS + (uint32_t)kind) << 5) + 0x440u) / 4u * 4u;
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = 0; c->r[5] = at; c->r[6] = 0; c->r[7] = side ? 0x12 : 0x10; c->r[8] = 1; f_0880AEA8(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
/* the swoosh's colours turned the S-ISM D text's purple-pink (the hue of TC 7), a little darker than the
   red they were so the pink health bar stands out against it (the maintainer, 2026-11-26). Run each frame
   on the palette as it stands: a red is one whose green and blue are alike; the pinks made (blue well
   over green) aren't touched again. */
#define SWOOSH_DARK_NUM 7                        /* the pinks at 7/10 of the red's lightness */
#define SWOOSH_DARK_DEN 10
static void swoosh_pink(uint32_t side) {
    uint32_t row = PALETTE_RAM + 32u * (side ? 0x12u : 0x10u);
    int changed = 0;
    for (uint32_t i = 1; i < 16; i++) {
        uint16_t v = (uint16_t)RH(row + 2 * i);
        int r = v & 31, g = v >> 5 & 31, b = v >> 10 & 31;
        int mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
        if (mx == 0 || (mx - mn) * 100 < mx * 15) continue;       /* greys stay */
        if (r != mx || g == mx) continue;                          /* only the reds */
        if ((g > b ? g - b : b - g) * 4 > mx - mn) continue;       /* (a pink already) */
        int nr = mx, nb = mn + (mx - mn) * 3 / 4, ng = mn;       /* purple-pink: blue at three quarters of red */
        nr = nr * SWOOSH_DARK_NUM / SWOOSH_DARK_DEN; ng = ng * SWOOSH_DARK_NUM / SWOOSH_DARK_DEN; nb = nb * SWOOSH_DARK_NUM / SWOOSH_DARK_DEN;
        uint16_t out = (uint16_t)((v & 0x8000) | nr | ng << 5 | nb << 10);
        if (out != v) { WH(row + 2 * i, out); changed = 1; }
    }
    if (changed) WW(PALETTE_DIRTY, 1);
}
/* S-ISM D's health bar with Extra ISM Presentation: its gradient (the fill, element 0x1B4 / 0x7F4: each
   corner's red, green, blue at +36 / +42 / +48) from yellow to a bright pink where the other ISMs have
   their own colour (the corners that aren't yellow), and its dark back (0x274 / 0x874) a dark pink. The
   maintainer, 2026-11-26. (A later EX. SETTING, HEALTH GRADIENT, will put solid yellow over this.) */
/* Paused, the game halves every corner (F0E000 yellow to 787000, the back 007800 to black), so the yellow
   end is told by its shape (red and green alike, no blue), and the pinks take the corner's own brightness
   against the game's full one (the fill's ends F0, the back's 78). */
static int corner_max(uint32_t e, uint32_t k) {
    int r = RB(e + 36 + k), g = RB(e + 42 + k), b = RB(e + 48 + k);
    return r > g ? (r > b ? r : b) : (g > b ? g : b);
}
static void corner_set(uint32_t e, uint32_t k, int r, int g, int b, int now, int full) {
    if (now > full) now = full;
    WB(e + 36 + k, (uint32_t)(r * now / full)); WB(e + 42 + k, (uint32_t)(g * now / full)); WB(e + 48 + k, (uint32_t)(b * now / full));
}
static void health_pink(uint32_t o, uint32_t side) {
    uint32_t fill = o + (side ? 0x7F4u : 0x1B4u), back = o + (side ? 0x874u : 0x274u);
    for (uint32_t k = 0; k < 4; k++) {
        int r = RB(fill + 36 + k), g = RB(fill + 42 + k), b = RB(fill + 48 + k);
        if (r >= 0x20 && g * 4 >= r * 3 && r * 4 >= g * 3 && b * 4 < r) continue;   /* the yellow end */
        corner_set(fill, k, 0xF0, 0x40, 0xC0, corner_max(fill, k), 0xF0);
    }
    for (uint32_t k = 0; k < 4; k++) corner_set(back, k, 0x78, 0x00, 0x60, corner_max(back, k), 0x78);
}
/* a side whose swoosh was turned pink: once it isn't S-ISM D's any more (the computer took the side over
   after a versus match, a new fight), its colours are loaded again for its kind, or the pink stays (the
   maintainer saw a V-ISM computer with S-ISM D's pink swoosh, 2026-11-26) */
static uint8_t swoosh_pinked[2];
int anthology_hud_gauges(Cpu *c, uint32_t o, int after) {
    if (!anthology_select_available()) return 0;
    for (uint32_t k = 0; k < 2; k++) {
        uint32_t part = o + 12 + 20 * k, f = RW(part);
        if (f != FIGHTER(0) && f != FIGHTER(1) && f != FIGHTER(2) && f != FIGHTER(3)) return 0;   /* not the gauges */
        int s = anthology_fighter_choice(f).sism;
        if (!after && swoosh_pinked[k] && !(s == S_PINK_D && rt_sism_presentation)) {
            swoosh_palette(c, (int8_t)RB(part + 4), k);
            swoosh_pinked[k] = 0;
        }
        if (!s) continue;
        if (after) { if (s == S_PINK_D && rt_sism_presentation) health_pink(o, k); continue; }
        /* TODO(S-ISM D, no supers): off until the super meter's code is decompiled (the maintainer,
           2026-11-27). It was a write of 0 to the fighter's meter (+366) every frame, forcing the game's
           state rather than changing its rule; do it in the decompiled meter code. Until then S-ISM D
           keeps its supers. */
        int want = SWOOSH_KIND[s];
        if ((int8_t)RB(part + 4) != want) {                        /* (the HUD set up again: its kind by the ISM) */
            WB(part + 4, (uint32_t)want);
            swoosh_palette(c, want, k);
        }
        if (s == S_PINK_D && rt_sism_presentation) { swoosh_pink(k); swoosh_pinked[k] = 1; }
    }
    return 0;
}

/* ---------------------------------------------------------------- Hyper's S-ISM gauge
 * The maintainer (2026-11-26): an S-ISM's gauge looks as Hyper's: the Shadaloo emblem on a swoosh and
 * one long bar in the S-ISM's colour (S-ISM A red, B blue, C green), CHARGE OK and SUPER over it when
 * full; S-ISM D has none. Its pictures come from the user's Anthology disc (tools/anthology_hud.py:
 * assets/us/anthology/hud/sism_gauge.png, 512 x 512: a band of 168 rows for each of red, green, blue,
 * in layers of 24: the gauge empty, the bar's fill, then CHARGE OK and SUPER in the five colours Hyper
 * pulses through when it is full, and at x 256 the bar in those five; the gauge's top left at row 4 of
 * each layer, its bar at x 32, 144 wide: the gauge's 0 - 144, a pixel each).
 * 0x0880DC88 draws the bottom of the HUD, each side's gauge from its elements (src/core/object/hud.c,
 * part_bottom); for a side with an S-ISM, ours instead (or nothing, D), the other side's as the game
 * draws it. Placed as Hyper places player 1's (its x 0 - 192 at the screen's left, its top 16 above the
 * 4:3 picture's foot), player 2's mirrored. Without the file, the game's gauge (an entry's) stays. */
#include "png_decode.h"
extern char rt_assets_root[1024];
void f_0880E038(Cpu *c);   /* hud.c: an element drawn (a0) */
void f_08810B74(Cpu *c);   /* hud.c: an element drawn as a rectangle (a0, a1 alpha) */
#define G_TEX_W   512
#define G_TEX_H   512
#define G_BAND    168
#define G_COL2    256
#define G_CHARGE_W 32                           /* CHARGE OK at x 0 - 31 of the full layer, SUPER at 65 - 144 */
#define G_SUPER_X 65
#define G_SUPER_W 80
#define G_STEPS   5
#define G_STEP_FRAMES 3                         /* (the full gauge's pulse: 5 rows there and back, 24 frames, from footage) */
#define G_LAYER   24
#define G_TOP     4
#define G_BAR_X   32
#define G_W       192
#define G_FOOT    232                            /* the 4:3 picture's foot, in the 2D library's rows */
#define G_RAISE   7                              /* (measured: the gauge's foot as the game's own gauges')*/
#define G_REST_Y  224                            /* the game's gauge elements' y (0x474 / 0xAB4) once in place: at a
                                                    match's start they slide up from 272, 2 a frame; ours follow */
#define HUD_DEPTHS 0x08AE03B8u
#define HUD_TEX_STATE 0x08B23334u
#define HUD_PAL_SELECTED 0x08ACCD80u
static uint32_t g_tex, g_desc, g_list, g_prim;
static int g_state;                              /* 0 not tried, 1 loaded, -1 none */
static int gauge_load(void) {
    if (g_state) return g_state > 0;
    g_state = -1;
    if (!rt_assets_root[0]) return 0;
    char path[1200], err[160]; PngImage img;
    snprintf(path, sizeof path, "%s/anthology/hud/sism_gauge.png", rt_assets_root);
    if (png_decode_file(path, &img, err, sizeof err) != 0) { rt_log("anthology: no S-ISM gauge (%s): the game's own", err); return 0; }
    if (img.w > G_TEX_W || img.h > G_TEX_H || img.h < 3 * G_BAND) { rt_log("anthology: %s is %d x %d: not used", path, img.w, img.h); png_free(&img); return 0; }
    uint32_t m = rt_alloc(G_TEX_W * G_TEX_H * 2 + 0x100, 1, 256, "anthology S-ISM gauge");
    if (!m) { png_free(&img); return 0; }
    for (int y = 0; y < G_TEX_H; y++) for (int x = 0; x < G_TEX_W; x++) {
        uint16_t v = 0;
        if (x < img.w && y < img.h) {
            const uint8_t *q = img.rgba + 4 * (y * img.w + x);
            if (q[3] >= 128) v = (uint16_t)(0x8000 | q[0] >> 3 | (q[1] >> 3) << 5 | (q[2] >> 3) << 10);
        }
        WH(m + (uint32_t)((y * G_TEX_W + x) * 2), v);
    }
    png_free(&img);
    g_tex = m; g_desc = m + G_TEX_W * G_TEX_H * 2; g_list = g_desc + 0x48; g_prim = g_list + 0x18;
    memcpy(HOST(g_desc), HOST(RW(PAGES + 0x14)), 0x44);
    WW(g_desc, OUR_PAGE); WW(g_desc + 4, 0); WW(g_desc + 0x18, 0x01000000); WW(g_desc + 0x24, G_TEX_W); WW(g_desc + 0x28, G_TEX_H);
    uint32_t d0 = RW(TEXT_TEXTURES);
    WW(g_list + 8, RW(d0)); WW(g_list + 12, RW(d0 + 4)); WW(g_list + 16, OUR_PAGE);
    WW(g_list, g_list + 8); WW(g_list + 4, 1);
    g_state = 1;
    rt_log("anthology: Hyper's S-ISM gauge loaded");
    return 1;
}
/* texels (u, v, w x h) at the 4:3 picture's (x, y), mirrored across for player 2 */
static void g_quad(Cpu *c, int u, int v, int w, int h, int x, int y, int flip, float z) {
    float fx0 = (float)x * sx + ox, fy0 = (float)y * sy + oy, fx1 = (float)(x + w) * sx + ox, fy1 = (float)(y + h) * sy + oy;
    int x0 = (int)(fx0 + 0.5f), x1 = (int)(fx1 + 0.5f), y0 = (int)(fy0 + 0.5f), y1 = (int)(fy1 + 0.5f);
    int ua = flip ? u + w : u, ub = flip ? u : u + w;
    uint32_t xy = g_prim + 0x10, col = xy + 0x10, uv = col + 0x10;
    WH(xy + 0, x0); WH(xy + 2, y0); WH(xy + 4, x1); WH(xy + 6, y0); WH(xy + 8, x1); WH(xy + 0xA, y1); WH(xy + 0xC, x0); WH(xy + 0xE, y1);
    for (int k = 0; k < 4; k++) WW(col + 4u * (uint32_t)k, 0xFFFFFFFFu);
    WH(uv + 0, ua); WH(uv + 2, v); WH(uv + 4, ub); WH(uv + 6, v); WH(uv + 8, ub); WH(uv + 0xA, v + h); WH(uv + 0xC, ua); WH(uv + 0xE, v + h);
    WW(g_prim, xy); WW(g_prim + 4, col); WW(g_prim + 8, uv); WW(g_prim + 0xC, 4);
    c->r[4] = g_prim; c->r[5] = 4; c->r[6] = 0x80000020u; c->f[12] = fbits(z);
    c->r[31] = 0x0880DC88; f_089E1618(c);
}
static void gauge_draw(Cpu *c, uint32_t f, int side, int sism, float z, int rise) {
    static const int band_of[5] = { -1, 1, 2, -1, 0 };   /* by S-ISM (green C, blue B, pink D, red A): bands red 0, green 1, blue 2 */
    int band = band_of[sism];
    if (band < 0) return;
    int level = (int16_t)RH(f + 366);
    if (level < 0) level = 0;
    if (level > 144) level = 144;
    int v0 = band * G_BAND, top = G_FOOT - 16 - G_TOP - G_RAISE + rise;
    int flip = side != 0, x = flip ? 384 - G_W : 0;
    g_quad(c, 0, v0, G_W, G_LAYER, x, top, flip, z);                                   /* empty */
    if (level >= 144) {                                                                /* full: the bar, CHARGE OK and SUPER pulsing */
        int t = (int)((frame_now() / G_STEP_FRAMES) % (2 * G_STEPS - 2));
        int step = t < G_STEPS ? t : 2 * G_STEPS - 2 - t;
        g_quad(c, G_COL2 + G_BAR_X, v0 + step * G_LAYER, 144, G_LAYER, flip ? 384 - G_BAR_X - 144 : G_BAR_X, top, flip, z);
        int vt = v0 + (2 + step) * G_LAYER;
        if (!flip) g_quad(c, 0, vt, G_W, G_LAYER, x, top, 0, z);
        else {                                         /* player 2: the words not mirrored, at the mirrored places */
            g_quad(c, 0, vt, G_CHARGE_W, G_LAYER, 384 - G_CHARGE_W, top, 0, z);
            g_quad(c, G_SUPER_X, vt, G_SUPER_W, G_LAYER, 384 - G_SUPER_X - G_SUPER_W, top, 0, z);
        }
    } else if (level > 0)                                                              /* the fill, from the bar's start */
        g_quad(c, G_BAR_X, v0 + G_LAYER, level, G_LAYER, flip ? 384 - G_BAR_X - level : G_BAR_X, top, flip, z);
}
static void element(Cpu *c, uint32_t e) { call3(c, f_0880E038, e, 0, 0); }
static void rect(Cpu *c, uint32_t e) { call3(c, f_08810B74, e, 0xFF, 0); }
int anthology_hud_bottom(Cpu *c, uint32_t o, int after) {
    if (after || !anthology_select_available()) return 0;
    int s0 = anthology_fighter_choice(RW(o + 12)).sism, s1 = anthology_fighter_choice(RW(o + 32)).sism;
    if (!s0 && !s1) return 0;
    if (!gauge_load()) return 0;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x40;
    if (!s0) {                                                     /* player 1's side as hud.c's part_bottom */
        element(c, o + 0x434); element(c, o + 0x474); element(c, o + 0x5F4);
        if ((int8_t)RB(o + 16) == 0) rect(c, o + 0x3F4);
        if ((int8_t)RB(o + 16) == 1) { rect(c, o + 0x5B4); rect(c, o + 0x574); rect(c, o + 0x534); }
        else { element(c, o + 0x5B4); element(c, o + 0x574); element(c, o + 0x534); }
        element(c, o + 0x4B4); element(c, o + 0x4F4); element(c, o + 0x134); element(c, o + 0x174);
    }
    if (!s1) {
        element(c, o + 0xA74); element(c, o + 0xAB4); element(c, o + 0xC34);
        if ((int8_t)RB(o + 36) == 0) rect(c, o + 0xA34);
        if ((int8_t)RB(o + 36) == 1) { rect(c, o + 0xBF4); rect(c, o + 0xBB4); rect(c, o + 0xB74); }
        else { element(c, o + 0xBF4); element(c, o + 0xBB4); element(c, o + 0xB74); }
        element(c, o + 0xB34); element(c, o + 0xAF4); element(c, o + 0x774); element(c, o + 0x7B4);
    }
    uint32_t keep_r[32], keep_f12 = c->f[12]; memcpy(keep_r, c->r, sizeof keep_r);
    uint8_t keep_tex[20], keep_pal = (uint8_t)RB(HUD_PAL_SELECTED);
    memcpy(keep_tex, HOST(HUD_TEX_STATE), sizeof keep_tex);
    uint32_t e = PAGES + 0x18u * OUR_PAGE, keep_page[6];
    for (int i = 0; i < 6; i++) keep_page[i] = RW(e + 4u * (uint32_t)i);
    WW(e, g_tex); WW(e + 4, g_tex); WW(e + 8, 0); WW(e + 0xC, 0); WW(e + 0x10, 0x80000000u); WW(e + 0x14, g_desc);
    c->r[4] = g_list; c->r[31] = 0x0880DC88; f_089E0CF4(c);
    sx = ffrom(RW(0x08E759E8)); sy = ffrom(RW(0x08E759EC)); ox = ffrom(RW(0x08E75A04)); oy = ffrom(RW(0x08E75A0C));
    if (s0) gauge_draw(c, RW(o + 12), 0, s0, (float)RH(HUD_DEPTHS + 2u * RB(o + 0x474 + 55)), (int16_t)RH(o + 0x474 + 10) - G_REST_Y);
    if (s1) gauge_draw(c, RW(o + 32), 1, s1, (float)RH(HUD_DEPTHS + 2u * RB(o + 0xAB4 + 55)), (int16_t)RH(o + 0xAB4 + 10) - G_REST_Y);
    for (int i = 0; i < 6; i++) WW(e + 4u * (uint32_t)i, keep_page[i]);
    memcpy(HOST(HUD_TEX_STATE), keep_tex, sizeof keep_tex);
    WB(HUD_PAL_SELECTED, keep_pal);
    memcpy(c->r, keep_r, sizeof keep_r); c->f[12] = keep_f12;
    c->r[29] += 0x40; c->r[31] = ra;
    return 1;
}
