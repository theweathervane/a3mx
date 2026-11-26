/* OPTION MODE gets a sixth button, EX. SETTING, between SCREEN SETTING and SAVE/LOAD.
 *
 * The option menu (0x088B6164, context 0x08E75912: +0 state, +1 cursor, +5 submenu shown, +6 in a
 * submenu) has five buttons: screen objects (kind 0xC, types 0x0F-0x13) whose picture is a 192x32
 * part of a texture page (+0x3C page, +0x40/+0x42 u, +0x4C/+0x4E v; page n is entry 0x30 + n of the
 * page table at 0x08B0C334). Each frame 0x088BA82C makes the one under the cursor bright and a bit
 * bigger and the others dim; the cursor moves with 0x088B7EC0 (a2 = last item).
 *
 * Here: the cursor runs 0-5 (item 3 is EX. SETTING); a sixth button object is a copy of EXIT's, drawing
 * a picture made at run time (SCREEN SETTING's background mirrored, lettered by tiles.c) on a texture
 * page of our own, and the other buttons get pictures made the same way; all six are laid out in the height the five had, slightly shorter each. Choosing a
 * button below EX. SETTING passes the game its own item number. EX. SETTING opens a screen of ours
 * with the project's own options (settings.c); Circle or EXIT goes back and saves them.
 */
#include "rt.h"
#include "version.h"
#include "ext_features.h"
#include <string.h>
#include <stdio.h>

/* The game's OPTION MODE functions as this run has them (src/core/task/option_mode.c wraps them with
   the extensions below; decomp.h): hand-written, or the original under --reference */
void inner_f_088B6164(Cpu *c); void inner_f_088B7EC0(Cpu *c); void inner_f_088BA8F4(Cpu *c);
void inner_f_088B70F8(Cpu *c); void inner_f_088B7AE8(Cpu *c); void inner_f_088B9790(Cpu *c);
void inner_f_088BB268(Cpu *c); void inner_f_088B8FC4(Cpu *c); void inner_f_088B90E4(Cpu *c);
void inner_f_088B9208(Cpu *c);

void f_0880A6CC(Cpu *c);                    /* text: a0 string, a1 font, a2 x, a3 y, t0 colour */
void rt_hold_display(int frames);           /* main.c: keep the last frame on screen */
void rt_hold_display_set(int frames);
void rt_hold_display_paced(int frames);    /* main.c: the same, in real time (a sound is playing) */
void f_0887C4CC(Cpu *c);                    /* sound effect: a0 0, a1 id, a2 0xC0 */

#define OPT        0x08E75912u              /* +0 state, +1 cursor, +5 submenu shown, +6 in a submenu */
#define SLOTS      0x08E75BA0u              /* per-slot input, 0x2C each; +0xC new presses (game bits) */
#define OBJS       0x092E2B88u              /* screen objects, 32 x 0x94 */
#define OBJ_N      32
#define OBJ_SZ     0x94u
#define OBJ_COUNT  0x095176F9u              /* free objects (the allocator counts down) */
#define PAGES      0x08B0C334u              /* texture pages, 0x18 each: +0 address, +4 copy, +0x14 description */
#define PAGE_BASE  0x30                     /* an object's page n is entry 0x30 + n */
#define OUR_PAGE   4
#define OUR_PAGE2  5
#define B_CONFIRM  0x8080u
#define B_BACK     0x100u
#define N_ITEMS    6
#define EX_ITEM    3
#define PLAYER_ITEM 1                       /* BUTTON CONFIG becomes PLAYER CONFIG */
#define SL_ITEM    3                        /* the game's number for SAVE/LOAD */

static uint32_t mem;                        /* our block: two page textures (256 x 256 x 2), their descriptions, strings */
#define TEX       (mem)
#define TEX2      (mem + 0x20000u)
#define DESC      (mem + 0x40000u)
#define DESC2     (mem + 0x40080u)
#define LIST      (mem + 0x40100u)          /* our texture list for the buttons' layer: 8 + 6 x 12 bytes */
#define STR(i)    (mem + 0x40180u + 0x20u * (uint32_t)(i))   /* our strings, 32 bytes each */
enum { S_TITLE, S_P1, S_P2, S_P3, S_COOP, S_EXIT, S_ON, S_OFF, S_PLAY, S_COM, S_PLAYER, S_P1C, S_P2C, S_P3C, S_P4C, S_TAUNT,
       S_COMMAND, S_TYPE, S_RAPID, S_DEFAULT, S_EXIT2, S_VERSION, S_COUNT };
static const char *const strings[S_COUNT] = { "EX. SETTING", "1P PRIORITY", "2P PRIORITY", "3P PRIORITY", "VIRTUAL PSPS", "EXIT", "ON", "OFF", "PLAYER", "COM",
    "PLAYER CONFIG", "P1 CONFIG", "P2 CONFIG", "P3 CONFIG", "P4 CONFIG", "TAUNT", "COMMAND", "TYPE", "RAPID", "DEFAULT", "EXIT", A3MX_VERSION_SCREEN };
static int clone = -1, item_v, sub_from, built;
static int starting;                        /* the option menu just started, not laid out yet (f_088B5FB8) */
static long seen = -1;
static int16_t x_even, x_odd;

static uint32_t obj(int k) { return OBJS + OBJ_SZ * (uint32_t)k; }
static int button(int k) {                  /* the game's item number of a menu button, or -1 */
    uint32_t o = obj(k);
    if (!RB(o + 0x5A) || RB(o + 0x5B) != 0xC) return -1;
    int t = RB(o + 0x5C) - 0x0F;
    return t >= 0 && t < 5 ? t : -1;
}
static uint32_t pressed(void) { return RW(SLOTS + 0xC) | RW(SLOTS + 0x2C + 0xC); }
static void sound(Cpu *c, int id) {
    uint32_t keep[8]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = 0; c->r[5] = (uint32_t)id; c->r[6] = 0xC0; f_0887C4CC(c);
    memcpy(&c->r[4], keep, sizeof keep);
}
static void text(Cpu *c, uint32_t s, int font, int x, int y, int colour) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = s; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}

/* ---------------------------------------------------------------- the button pictures */
/* Every OPTION MODE tile is drawn afresh in one font (tiles.c): the game's own tiles' backgrounds
 * with their letters wiped, lettered in the MODE MENU font in yellow with a purple outline. Two
 * texture pages of ours: page 4 (EX. SETTING, PLAYER CONFIG, P1 - P4 CONFIG, SAVE/LOAD; the 2 of
 * L2 / R2 at the bottom) and page 5 (GAME OPTION, SCREEN SETTING, EXIT). */
int tiles_draw(uint32_t tex, int dst, int page_entry, int v0, int mirror, const char *text);
enum { ROW_EX, ROW_PLAYER, ROW_P1, ROW_SL = ROW_P1 + 4 };   /* rows of page 4 (32 high) */
enum { ROW_GO, ROW_SCR, ROW_EXIT };                         /* rows of page 5 */
static int tiles_ok, two_ready, arrows_ready;
static void build_picture(void) {
    static const struct { int page, row, src, v0, mirror; const char *text; } t[] = {
        { 4, ROW_EX, 2, 64, 1, "EX. SETTING" },     { 4, ROW_PLAYER, 2, 32, 0, "PLAYER CONFIG" },
        { 4, ROW_P1, 2, 0, 0, "P1 CONFIG" },        { 4, ROW_P1 + 1, 2, 32, 0, "P2 CONFIG" },
        { 4, ROW_P1 + 2, 2, 64, 0, "P3 CONFIG" },   { 4, ROW_P1 + 3, 3, 128, 0, "P4 CONFIG" },
        { 4, ROW_SL, 3, 128, 0, "SAVE/LOAD" },
        { 5, ROW_GO, 2, 0, 0, "GAME OPTION" },      { 5, ROW_SCR, 2, 64, 0, "SCREEN SETTING" },
        { 5, ROW_EXIT, 2, 96, 0, "EXIT" },
    };
    memset(HOST(TEX), 0, 256 * 256 * 2); memset(HOST(TEX2), 0, 256 * 256 * 2);
    two_ready = 0; arrows_ready = 0;                          /* (the 2 of L2 / R2 and the arrows are on page 4 too) */
    tiles_ok = 1;
    for (size_t i = 0; i < sizeof t / sizeof *t; i++)
        tiles_ok &= tiles_draw(t[i].page == 4 ? TEX : TEX2, t[i].row, PAGE_BASE + t[i].src, t[i].v0, t[i].mirror, t[i].text);
    if (!tiles_ok) rt_log("exsetting: tiles not made, the game's pictures stay");
    built = 1;
}

/* our texture pages: entries 0x30 + 4 and 0x30 + 5, described like page 3 */
static void one_page_on(int n, uint32_t tex, uint32_t desc) {
    uint32_t e = PAGES + 0x18u * (uint32_t)(PAGE_BASE + n), e3 = PAGES + 0x18u * (PAGE_BASE + 3);
    if (RW(e) == tex || RW(e) != 0) return;                    /* ours already, or in use by the game */
    memcpy(HOST(desc), HOST(RW(e3 + 0x14)), 0x44);
    WW(desc, (uint32_t)(PAGE_BASE + n));
    WW(e, tex); WW(e + 4, tex); WW(e + 8, RW(e3 + 8)); WW(e + 0xC, RW(e3 + 0xC)); WW(e + 0x10, RW(e3 + 0x10)); WW(e + 0x14, desc);
}
/* An object's page is an index into the texture list of its layer (+0x36; the lists are at
 * 0x08E75A18 + 4 x layer: entries of 12 bytes, +8 the page table entry). The buttons' layer 0x12 has
 * five (0x30 - 0x34); while the menu is up it gets a copy of ours with a sixth, page 5 = entry 0x35. */
#define LAYER_LISTS 0x08E75A18u
#define BTN_LAYER   0x12u
static uint32_t game_list;
static void list_on(void) {
    uint32_t slot = LAYER_LISTS + 4 * BTN_LAYER, l = RW(slot);
    if (l == LIST || !l || RW(l + 4) != OUR_PAGE2 || RW(RW(l) + 12 * OUR_PAGE + 8) != PAGE_BASE + OUR_PAGE) return;   /* not the list we know */
    game_list = l;
    memcpy(HOST(LIST + 8), HOST(RW(l)), 12 * OUR_PAGE2);
    WW(LIST + 8 + 12 * OUR_PAGE2, RW(RW(l))); WW(LIST + 8 + 12 * OUR_PAGE2 + 4, RW(RW(l) + 4)); WW(LIST + 8 + 12 * OUR_PAGE2 + 8, PAGE_BASE + OUR_PAGE2);
    WW(LIST, LIST + 8); WW(LIST + 4, OUR_PAGE2 + 1);
    WW(slot, LIST);
}
static void page_on(void) { one_page_on(OUR_PAGE, TEX, DESC); one_page_on(OUR_PAGE2, TEX2, DESC2); list_on(); }
static int page2_ours(void) { return RW(PAGES + 0x18u * (PAGE_BASE + OUR_PAGE2)) == TEX2 && RW(LAYER_LISTS + 4 * BTN_LAYER) == LIST; }
static void page_off(void) {
    uint32_t e = PAGES + 0x18u * (PAGE_BASE + OUR_PAGE), e2 = PAGES + 0x18u * (PAGE_BASE + OUR_PAGE2);
    if (mem && RW(e) == TEX) memset(HOST(e), 0, 0x18);
    if (mem && RW(e2) == TEX2) memset(HOST(e2), 0, 0x18);
    if (mem && RW(LAYER_LISTS + 4 * BTN_LAYER) == LIST) { WW(LAYER_LISTS + 4 * BTN_LAYER, game_list); }
}

/* the sixth button: a copy of EXIT's object, showing our page */
static void clone_on(void) {
    if (clone >= 0 && RB(obj(clone) + 0x5A) && RB(obj(clone) + 0x5B) == 0xC) return;
    clone = -1;
    int exit_k = -1, free_k = -1;
    for (int k = 0; k < OBJ_N; k++) {
        int b = button(k);
        if (b == 0) x_even = (int16_t)RH(obj(k) + 6);
        if (b == 1) x_odd = (int16_t)RH(obj(k) + 6);
        if (b == 4) exit_k = k;
    }
    if (exit_k < 0) return;
    for (int k = 0; k < OBJ_N; k++) if (!RB(obj(k) + 0x5A)) { free_k = k; break; }
    if (free_k < 0) return;
    memcpy(HOST(obj(free_k)), HOST(obj(exit_k)), OBJ_SZ);
    uint32_t o = obj(free_k);
    WW(o + 0x3C, OUR_PAGE); WH(o + 0x40, 0); WH(o + 0x42, 192); WH(o + 0x4C, 0); WH(o + 0x4E, 32);
    WB(OBJ_COUNT, RB(OBJ_COUNT) - 1);
    clone = free_k;
}
static void clone_off(void) {
    if (clone >= 0 && RB(obj(clone) + 0x5A)) { memset(HOST(obj(clone)), 0, OBJ_SZ); WB(OBJ_COUNT, RB(OBJ_COUNT) + 1); }
    clone = -1;
}

/* ---------------------------------------------------------------- our screens */
/* EX. SETTING and P1 - P4 CONFIG are laid out like GAME OPTION (0x088B88F4): names in font 1, a row
 * cursor in the option context's +7, colour 0xB on its row and 0xA elsewhere, and GAME OPTION's green
 * bar (object kind 0xC, type 4; once a frame 0x088BA8F4) under it. PLAYER CONFIG reuses the menu's
 * buttons as five tiles. */
extern int rt_join_in[3], rt_virtual_psps, rt_l2[4], rt_r2[4];
extern const uint8_t pcfg_default[20];
uint8_t *pcfg_block(int p);
void settings_save(void);
void f_08823338(Cpu *c);                    /* new screen object: a0 kind, a1 type; v0 = object */
void f_0880A7B0(Cpu *c);                    /* button symbol a0 (font 5) at a1, a2 */
void f_0880A7F4(Cpu *c);                    /* SELECT (font 6) at a1, a2 */
enum { M_MENU, M_EX, M_PLAYERS, M_PEDIT } ;
static int mode, row, bar = -1, bar_y = -1, tile, editing, two[2] = { -1, -1 };
static int new_obj(Cpu *c, int kind, int type) {           /* a screen object of the game's; its index or -1 */
    uint32_t keep[2] = { c->r[4], c->r[5] }, ra = c->r[31];
    c->r[4] = (uint32_t)kind; c->r[5] = (uint32_t)type; f_08823338(c);
    uint32_t o = c->r[2];
    c->r[4] = keep[0]; c->r[5] = keep[1]; c->r[31] = ra;
    return o ? (int)((o - OBJS) / OBJ_SZ) : -1;
}
static void free_obj(int *k) {
    if (*k >= 0 && RB(obj(*k) + 0x5A)) { memset(HOST(obj(*k)), 0, OBJ_SZ); WB(OBJ_COUNT, RB(OBJ_COUNT) + 1); }
    *k = -1;
}
static void bar_on(Cpu *c) { if (bar < 0) bar = new_obj(c, 0xC, 4); }
static void bar_off(void) { free_obj(&bar); }
/* The "2" of L2 and R2, after the L and R symbols: SELECT's S (font 6's picture, 0x09739DC0, at
 * 0,162: white letters 3 wide in a dark outline, 14 high with it) turned round, on our page at
 * TWO_U, TWO_V, shown by an object of ours (kind 0xC, type 4 like the bar, handled in 0x088BA8F4) */
#define FONT6_TEX 0x09739DC0u
#define TWO_U 0
#define TWO_V 224
#define TWO_W 5
#define TWO_H 14
/* L and R are 8 wide (with their outline) and the other symbols are centred on x + 8 at 0x68:
   L2 (8 + 5 - the shared outline = 12 wide) starts at 0x6A, its 2 at 0x6A + 7 */
#define TWO_SYM_X 0x6A
#define TWO_X (TWO_SYM_X + 7)
static void two_build(void) {
    uint32_t t = FONT6_TEX;
    if ((RH(t + (161 * 256) * 2) >> 15) || !(RH(t + (162 * 256) * 2) >> 15) || RH(t + (164 * 256 + 1) * 2) != 0xFFFF) {
        rt_log("exsetting: SELECT not where expected, no 2 for L2 / R2"); return;
    }
    for (int y = 0; y < TWO_H; y++) for (int x = 0; x < TWO_W; x++) {
        uint16_t p = (uint16_t)RH(t + (uint32_t)(((162 + y) * 256 + (TWO_W - 1 - x)) * 2));
        if (x == 0 && !(RH(t + (uint32_t)(((162 + y) * 256 + TWO_W - 2) * 2)) >> 15)) p = 0;   /* E's outline */
        WH(TEX + (uint32_t)(((TWO_V + y) * 256 + TWO_U + x) * 2), p);
    }
    two_ready = 1;
}
static void two_on(Cpu *c) {
    if (!two_ready) two_build();
    if (!two_ready) return;
    for (int i = 0; i < 2; i++) if (two[i] < 0) two[i] = new_obj(c, 0xC, 4);
}
static void two_off(void) { free_obj(&two[0]); free_obj(&two[1]); }
/* An object's +6 x is in the text's units; its +0xA y (top, downwards) and +0x1A height are in halves */
static void two_frame(uint32_t o, int text_y) {           /* place a 2 for a row drawn at text_y */
    WW(o + 0x3C, OUR_PAGE); WH(o + 0x40, TWO_U); WH(o + 0x42, TWO_U + TWO_W); WH(o + 0x4C, TWO_V); WH(o + 0x4E, TWO_V + TWO_H);
    WH(o + 6, TWO_X); WH(o + 0xA, (uint32_t)(2 * (text_y + 2)));
    WH(o + 0xC, 0); WH(o + 0x18, 0); WH(o + 0xE, TWO_W); WH(o + 0x1A, 2 * TWO_H);
    static const uint8_t col[] = { 0x24, 0x25, 0x26, 0x27, 0x2A, 0x2B, 0x2C, 0x2D, 0x30, 0x31, 0x32, 0x33 };
    for (size_t i = 0; i < sizeof col; i++) WB(o + col[i], 0xFF);
    WB(o + 0x36, 0x12); WB(o + 0x37, 7); WB(o + 1, 1);          /* drawn like the buttons: textured */
    WB(o + 0x58, 1); WB(o + 0x59, 1); WB(o + 0x8A, 1);
    WB(o + 0x88, 0xFF); WB(o + 0x38, 0); WB(o + 0x5D, 1);
}
static void title(Cpu *c, int s) { text(c, STR(s), 2, 0xC0 - 8 * (int)strlen((char *)HOST(STR(s))), 5, 0xB); }
static void symbol(Cpu *c, int sel, int id, int x, int y) {
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = (uint32_t)id; c->r[5] = (uint32_t)x; c->r[6] = (uint32_t)y;
    if (sel) f_0880A7F4(c); else f_0880A7B0(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}

/* EX. SETTING: the project's options, on pages (0.127.0), each with its own banner in place of
 * "EX. SETTING" (0.165.0):
 *   EX. GAME      BOOT TO MODE MENU (from the next start), MODE MENU SHORTCUT, SHARED ENTRY POOL, 1P - 3P
 *                 PRIORITY and VIRTUAL PSPS
 *   EX. DISPLAY   MODE MENU ICONS, ISM PRESENTATION, SCALED ENDINGS, STAGE EXTENSIONS
 * The extensions on these two were the launcher's checkboxes before 0.165.0: they are kept in a3mx.cfg
 * and switched at once (ext_features.h: features_setting_changed). With Cheats and Unlocks (part of
 * Extra menus: cheats_unlocks.c), four more:
 *   EX. UNLOCKS   SHIN AKUMA, FINAL M.BISON, EX BALROG, SHIN AKUMA BATTLE, BATTLE VARIANTS, EDIT MODE
 *                 POINTS, ISM-PLUS (ON forced, OFF as the save earned them)
 *   EX. FIGHTERS  1P and 2P HEALTH, GUARD and SUPER; then 3P's, and 4P's greyed out (there is no fourth
 *                 fighter: their values can't be changed)
 *   EX. EXP       WORLD TOUR EXP and its X-, A- and V-ISM EXP; the same four for EDIT MODE
 * Each page ends with DEFAULT (that page's rows back to their
 * defaults) and EXIT. As on SCORE RANKING, an arrow on each side and the page number (font 2, colour 9,
 * above the version) show there are more pages: L / R turn the page, and so do left / right on DEFAULT
 * and EXIT (on a row with a value they change it). With one page it is 1/1, with no arrows. Under the
 * rows, the row's description in the small font, as SAVE LOAD's messages. Rows 14 apart from y 0x1C
 * (eleven fit above the description). */
#include "unlocks.h"
extern int rt_cheat_health[3], rt_cheat_guard[3], rt_cheat_super[3], rt_cheat_wt[4], rt_cheat_edit[4], rt_unlock[UNLOCK_COUNT];   /* cheats_unlocks.c */
extern const char *const cheat_health_names[], *const cheat_guard_names[], *const cheat_super_names[], *const cheat_wt_names[];
void cheats_unlocks_default_unlocks(void);
void cheats_unlocks_default_fighters(int from, int to);
void cheats_unlocks_default_entries(void);
static const char *const v_priority[] = { "COM", "PLAYER", NULL };
static const char *const v_off_on[] = { "OFF", "ON", NULL };
static int no_fighter;              /* 4P's rows: shown, not used */

/* Strings drawn by the text queue must stay in guest memory until the text pass: each host string gets a
   slot of its own, once, in a pool of ours (the same string, the same slot, every frame) */
#define POOL_SIZE 0x2000u
static uint32_t pool, pool_used;
static const char *pool_key[256];
static uint32_t pool_at[256];
static int pool_n;
static uint32_t gstr(const char *s) {
    for (int i = 0; i < pool_n; i++) if (pool_key[i] == s) return pool_at[i];
    size_t n = strlen(s) + 1;
    if (!pool) { pool = rt_alloc(POOL_SIZE, 1, 16, "ex. setting text"); pool_used = 0; }
    if (!pool || pool_n == 256 || pool_used + n > POOL_SIZE) return STR(S_EXIT);   /* (never: the strings are few) */
    uint32_t a = pool + pool_used; pool_used += (uint32_t)((n + 3) & ~3u);
    memcpy(HOST(a), s, n);
    pool_key[pool_n] = s; pool_at[pool_n++] = a;
    return a;
}

static uint16_t rgb15(int r, int g, int b) { return (uint16_t)(0x8000u | (uint32_t)(r >> 3) | (uint32_t)(g >> 3) << 5 | (uint32_t)(b >> 3) << 10); }

typedef struct {
    const char *label;
    int *value;                     /* NULL: DEFAULT or EXIT */
    const char *const *names;
    const char *what[2];            /* the description, two lines of at most 32: clear of the page number */
    int greyed;                     /* shown in grey, its value can't be changed (4P) */
} Row;
#define R_DEFAULT_ROW(w) { "DEFAULT", NULL, NULL, { "THIS PAGE BACK TO ITS DEFAULTS.", w }, 0 }
#define R_EXIT_ROW { "EXIT", NULL, NULL, { "SAVE AND BACK TO OPTION MODE.", "" }, 0 }
#define EARNED_ARCADE "OFF - EARNED BY BEATING ARCADE."
static const Row page_unlocks[] = {
    { "SHIN AKUMA", &rt_unlock[UNLOCK_SHIN_AKUMA], v_off_on, { "SELECT ON AKUMA - SHIN AKUMA.", EARNED_ARCADE }, 0 },
    { "FINAL M.BISON", &rt_unlock[UNLOCK_FINAL_BISON], v_off_on, { "SELECT ON M.BISON - FINAL BISON.", EARNED_ARCADE }, 0 },
    { "EX BALROG", &rt_unlock[UNLOCK_EX_BALROG], v_off_on, { "SELECT ON BALROG - EX BALROG.", EARNED_ARCADE }, 0 },
    { "SHIN AKUMA BATTLE", &rt_unlock[UNLOCK_SHIN_BATTLE], v_off_on, { "FIGHT SHIN AKUMA IN FINAL BATTLE", "HOLD L AND R. OFF - BEAT ARCADE." }, 0 },
    { "BATTLE VARIANTS", &rt_unlock[UNLOCK_VARIANTS], v_off_on, { "L AND R - DRAMATIC VARIABLE AND", "R.DRAMATIC. OFF - KUMITE 100." }, 0 },
    { "EDIT MODE POINTS", &rt_unlock[UNLOCK_EDIT_POINTS], v_off_on, { "EDIT MODE HAS 9999 POINTS.", "OFF - EARNED BY CLEARING MODES." }, 0 },
    { "ISM-PLUS", &rt_unlock[UNLOCK_ISM_PLUS], v_off_on, { "ALL 26 ISM-PLUSES IN EDIT MODE.", "OFF - EARNED BY CLEARING MODES." }, 0 },
    R_DEFAULT_ROW("ALL ON."), R_EXIT_ROW,
};
#define FIGHTER_ROWS(n, k, who) \
    { n "P HEALTH", &rt_cheat_health[k], cheat_health_names, { "THE " n "P FIGHTER HEALTH" who, "NONE - ONE HIT KNOCKS OUT." }, 0 }, \
    { n "P GUARD", &rt_cheat_guard[k], cheat_guard_names, { "THE " n "P FIGHTER GUARD GAUGE.", "NONE - THE NEXT GUARD CRUSHES." }, 0 }, \
    { n "P SUPER", &rt_cheat_super[k], cheat_super_names, { "THE " n "P FIGHTER SUPER GAUGE.", "LV1 - LV3 - HELD AT THAT LEVEL." }, 0 }
static const Row page_fighters12[] = {
    FIGHTER_ROWS("1", 0, "."), FIGHTER_ROWS("2", 1, " - CPU TOO."),
    R_DEFAULT_ROW("ALL NORMAL."), R_EXIT_ROW,
};
static const Row page_fighters34[] = {
    FIGHTER_ROWS("3", 2, "."),
    { "4P HEALTH", &no_fighter, cheat_health_names, { "NO 4TH FIGHTER YET.", "" }, 1 },
    { "4P GUARD", &no_fighter, cheat_guard_names, { "NO 4TH FIGHTER YET.", "" }, 1 },
    { "4P SUPER", &no_fighter, cheat_super_names, { "NO 4TH FIGHTER YET.", "" }, 1 },
    R_DEFAULT_ROW("ALL NORMAL."), R_EXIT_ROW,
};
static const Row page_entries[] = {
    { "WORLD TOUR EXP", &rt_cheat_wt[0], cheat_wt_names, { "WORLD TOUR EXP. MAX - FULL AND", "LV 32 WITH EVERY STATUS AT MOST." }, 0 },
    { "WORLD TOUR X-ISM EXP", &rt_cheat_wt[1], cheat_wt_names, { "WORLD TOUR X-ISM EXP.", "MAX - FULL AND LV 32 - MASTER." }, 0 },
    { "WORLD TOUR A-ISM EXP", &rt_cheat_wt[2], cheat_wt_names, { "WORLD TOUR A-ISM EXP.", "MAX - FULL AND LV 32 - MASTER." }, 0 },
    { "WORLD TOUR V-ISM EXP", &rt_cheat_wt[3], cheat_wt_names, { "WORLD TOUR V-ISM EXP.", "MAX - FULL AND LV 32 - MASTER." }, 0 },
    { "EDIT MODE EXP", &rt_cheat_edit[0], cheat_wt_names, { "THE CHARACTER EDIT MODE MAKES.", "MAX - FULL EXP AND LV 32." }, 0 },
    { "EDIT MODE X-ISM EXP", &rt_cheat_edit[1], cheat_wt_names, { "EDIT MODE X-ISM EXP.", "MAX - FULL AND LV 32 - MASTER." }, 0 },
    { "EDIT MODE A-ISM EXP", &rt_cheat_edit[2], cheat_wt_names, { "EDIT MODE A-ISM EXP.", "MAX - FULL AND LV 32 - MASTER." }, 0 },
    { "EDIT MODE V-ISM EXP", &rt_cheat_edit[3], cheat_wt_names, { "EDIT MODE V-ISM EXP.", "MAX - FULL AND LV 32 - MASTER." }, 0 },
    R_DEFAULT_ROW("ALL NORMAL."), R_EXIT_ROW,
};
#define FEAT_ROW(label, f, w1, w2) { label, &rt_feature_setting[f], v_off_on, { w1, w2 }, 0 }
static const Row page_game[] = {
    FEAT_ROW("BOOT TO MODE MENU", FEAT_BOOTMENU, "START ON THE MODE MENU - NO", "TITLE. FROM THE NEXT START."),
    FEAT_ROW("MODE MENU SHORTCUT", FEAT_QUITKEY, "F2 OR SELECT AND START - BACK", "TO THE MODE MENU. NO SAVING."),
    FEAT_ROW("SHARED ENTRY POOL", FEAT_I_ISM_SELECT, "ENTRY CHARACTERS CAN BE PICKED", "FOR THE OTHER SIDE TOO."),
    { "1P PRIORITY", &rt_join_in[0], v_priority, { "PLAYER - PAD 1 PLAYS A FIGHTER", "NOBODY STARTED. COM - THE CPU." }, 0 },
    { "2P PRIORITY", &rt_join_in[1], v_priority, { "PLAYER - PAD 2 PLAYS A FIGHTER", "NOBODY STARTED. COM - THE CPU." }, 0 },
    { "3P PRIORITY", &rt_join_in[2], v_priority, { "PLAYER - PAD 3 PLAYS A FIGHTER", "NOBODY STARTED. COM - THE CPU." }, 0 },
    { "VIRTUAL PSPS", &rt_virtual_psps, v_off_on, { "NETWORK FINDS PADS 2 AND 3 OR", "THE CPU AS OTHER PSPS HERE." }, 0 },
    R_DEFAULT_ROW("BOOT OFF - COM - THE REST ON."), R_EXIT_ROW,
};
static const Row page_display[] = {
    FEAT_ROW("MODE MENU ICONS", FEAT_MODE_MENU_ICONS, "PAD BUBBLES AND FRAME TIMING", "BADGE ON THE MODE MENU BAR."),
    FEAT_ROW("ISM PRESENTATION", FEAT_I_ISM_DISPLAY, "I-ISM WIPE INTO THE FIGHT AND", "I-ISM RANKING IN THE ATTRACT."),
    FEAT_ROW("SCALED ENDINGS", FEAT_SCALED_ENDINGS, "ENDINGS FILL THE WIDE SCREEN.", "WITH WIDE NATIVE."),
    FEAT_ROW("STAGE EXTENSIONS", FEAT_STAGE_EXT, "NEW ART FOR THE STAGE EDGES IN", "WIDE NATIVE. EXPERIMENTAL."),
    R_DEFAULT_ROW("ICONS AND ENDINGS ON."), R_EXIT_ROW,
};
/* EX. GAME and EX. DISPLAY always; the cheats' pages after them, with Cheats and Unlocks */
enum { PG_GAME, PG_DISPLAY, PG_UNLOCKS, PG_FIGHTERS12, PG_FIGHTERS34, PG_ENTRIES, N_PAGES };
#define PAGE(t, r, x) { t, r, (int)(sizeof r / sizeof *r), x }
static const struct { const char *title; const Row *rows; int n, value_x; } pages[N_PAGES] = {
    PAGE("EX. GAME", page_game, 0xD8), PAGE("EX. DISPLAY", page_display, 0xD8), PAGE("EX. UNLOCKS", page_unlocks, 0xD8),
    PAGE("EX. FIGHTERS", page_fighters12, 0xA0), PAGE("EX. FIGHTERS", page_fighters34, 0xA0), PAGE("EX. EXP", page_entries, 0xE0),
};
static int page;                    /* PG_* */
static int first_page(void) { return PG_GAME; }
static int page_count(void) { return FEATURE(CHEATS_UNLOCKS) ? N_PAGES : PG_DISPLAY + 1; }
/* a row of EX. GAME / EX. DISPLAY: the extension it switches, or -1 */
static int row_feature(const Row *r) {
    return r->value >= &rt_feature_setting[0] && r->value < &rt_feature_setting[FEAT_COUNT] ? (int)(r->value - rt_feature_setting) : -1;
}
static void page_features_default(int pg) {
    for (int i = 0; i < pages[pg].n; i++) {
        int f = row_feature(&pages[pg].rows[i]);
        if (f >= 0) { rt_feature_setting[f] = FEATURES[f].on_by_default; features_setting_changed(f); }
    }
}
static int count_names(const char *const *n) { int k = 0; while (n[k]) k++; return k; }
static int ex_row_y(int r) { return 0x1C + 14 * r; }
static void page_defaults(int pg) {
    if (pg == PG_UNLOCKS) cheats_unlocks_default_unlocks();
    else if (pg == PG_FIGHTERS12) cheats_unlocks_default_fighters(0, 1);
    else if (pg == PG_FIGHTERS34) cheats_unlocks_default_fighters(2, 2);
    else if (pg == PG_ENTRIES) cheats_unlocks_default_entries();
    else if (pg == PG_DISPLAY) page_features_default(pg);
    else { page_features_default(pg); rt_join_in[0] = rt_join_in[1] = rt_join_in[2] = 0; rt_virtual_psps = 1; }
}

/* The side arrows: drawn into our page at ARROW_U / ARROW_V (left, then right), shown by two objects of
   ours like the 2 of L2 (kind 0xC, type 4), beside the first row. Our own drawing, in SCORE RANKING's
   colours: a white edge round a purple fill, lighter towards the point. */
#define ARROW_U 8
#define ARROW_V 224
#define ARROW_W 7
#define ARROW_H 13
#define ARROW_LX 4                  /* at the 4:3 picture's sides (the user's mock-up) */
#define ARROW_RX (384 - 4 - ARROW_W)
static int arrow[2] = { -1, -1 };
static void arrows_build(void) {
    for (int side = 0; side < 2; side++)
        for (int y = 0; y < ARROW_H; y++)
            for (int x = 0; x < ARROW_W; x++) {
                int d = y <= ARROW_H / 2 ? y : ARROW_H - 1 - y;       /* rows from the nearer end: 0 - 6 */
                int from_point = side ? ARROW_W - 1 - x : x;           /* 0 at the point */
                int inside = from_point >= ARROW_W - 1 - d;            /* the triangle: wider towards the middle */
                int edge = inside && (from_point == ARROW_W - 1 - d || from_point == ARROW_W - 1 || d == 0);
                uint16_t p = 0;
                if (edge) p = rgb15(0xFF, 0xFF, 0xFF);
                else if (inside) { int t = from_point * 255 / (ARROW_W - 1); p = rgb15(0x60 + t * 0x60 / 255, 0x90 - t * 0x70 / 255, 0xFF); }
                WH(TEX + (uint32_t)(((ARROW_V + y) * 256 + ARROW_U + side * 8 + x) * 2), p);
            }
    arrows_ready = 1;
}
static void arrows_on(Cpu *c) {
    if (!arrows_ready) arrows_build();
    for (int i = 0; i < 2; i++) if (arrow[i] < 0) arrow[i] = new_obj(c, 0xC, 4);
}
static void arrows_off(void) { free_obj(&arrow[0]); free_obj(&arrow[1]); }
static void arrow_frame(uint32_t o, int side) {
    two_frame(o, 0);                                          /* drawn like the 2: then ours */
    int u = ARROW_U + side * 8;
    WH(o + 0x40, (uint32_t)u); WH(o + 0x42, (uint32_t)(u + ARROW_W)); WH(o + 0x4C, ARROW_V); WH(o + 0x4E, ARROW_V + ARROW_H);
    WH(o + 6, side ? ARROW_RX : ARROW_LX); WH(o + 0xA, (uint32_t)(2 * (ex_row_y(0) + 1)));
    WH(o + 0xE, ARROW_W); WH(o + 0x1A, 2 * ARROW_H);
    WB(o + 0x5D, mode == M_EX && page_count() > 1);
}

static void grey_font_on(void);
static void ex_open(Cpu *c) {
    mode = M_EX; page = first_page(); row = 0; bar_y = 239 - ex_row_y(0);
    WB(OPT + 5, EX_ITEM + 1); WB(OPT + 7, 0); bar_on(c); if (page_count() > 1) arrows_on(c); grey_font_on(); sound(c, 0x39);
}
static void ex_close(Cpu *c, int snd) { mode = M_MENU; WB(OPT + 5, 0); WB(OPT + 7, 0); bar_off(); arrows_off(); bar_y = -1; settings_save(); sound(c, snd); }
#define VERSION_Y 0xD4
#define VERSION_X (0x178 - 8 * (int)(sizeof A3MX_VERSION_SCREEN - 1))
#define PAGE_Y    0xB6
#define WHAT_Y    0xBC
/* Greyed rows (4P): two colours of font 1 of our own, 14 (unselected) and 15 (selected), added to the
 * game's font for good the first time EX. SETTING opens (grey_font_on). The font's glyphs are in eight
 * texture pages (the font list 0x08A05080: descriptors, count), colour k's font 1 at rows 144 k + 32 to
 * + 80 of them all (32 glyphs of 8 x 16 a row, from '!'; descriptor = row / 256). Colours 14 and 15 fall
 * at rows 2048 and 2192: a ninth page, ours, given page table entry 0x3F2 (next to the font's own 0x3F0
 * and 0x3F1; never used by the game in any screen seen) and a ninth descriptor in a copy of the list.
 * Their glyphs are made from colour 10's (the cyan of the unselected rows, navy outline): 14 with the
 * outline dark and the inside one solid grey; 15 with colour 9's dark red outline (grey on red: the
 * user's pick) and the inside one solid lighter grey. No gradients. */
#define GREYED      14
#define GREYED_ON   15
#define FONT_LIST   0x08A05080u
#define FONT1_Y     32
#define FONT1_H     48
#define FONT_SET_H  144
#define GREY_PAGE   0x3F2
static uint32_t grey_mem;                   /* our page (256 x 256 x 2), its description, the descriptors */
#define GREY_TEX    (grey_mem)
#define GREY_DESC   (grey_mem + 0x20000u)
#define GREY_LIST   (grey_mem + 0x20080u)
static uint32_t font1_texel(int colour, int x, int y) {
    int v = FONT_SET_H * colour + FONT1_Y + y;
    uint32_t list = RW(FONT_LIST), page = RW(list + 12u * (uint32_t)(v >> 8) + 8);
    return RW(PAGES + 0x18u * page) + (uint32_t)(((v & 255) * 256 + x) * 2);
}
static int bright(uint16_t p) { int r = p & 31, g = (p >> 5) & 31, b = (p >> 10) & 31; return (r > g ? (r > b ? r : b) : (g > b ? g : b)) >= 18; }
static void grey_font_on(void) {
    uint32_t list = RW(FONT_LIST), n = RW(FONT_LIST + 4), e = PAGES + 0x18u * GREY_PAGE, e8 = PAGES + 0x18u * 0x8F;
    if (!list || (grey_mem && list == GREY_LIST && RW(e) == GREY_TEX)) return;   /* not loaded yet, or ours already */
    if (n != 8 || RW(e) != 0 || !RW(e8)) { rt_log("exsetting: the font isn't as expected, no grey colours"); return; }
    if (!grey_mem && !(grey_mem = rt_alloc(0x20080u + 12u * 9u, 1, 256, "grey font"))) return;
    memset(HOST(GREY_TEX), 0, 0x20000);
    memcpy(HOST(GREY_DESC), HOST(RW(e8 + 0x14)), 0x44); WW(GREY_DESC, GREY_PAGE);
    for (int k = 0; k < 5; k++) WW(e + 4u * (uint32_t)k, k == 0 ? GREY_TEX : RW(e8 + 4u * (uint32_t)k));   /* as the font's own pages */
    WW(e + 0x14, GREY_DESC);
    memcpy(HOST(GREY_LIST), HOST(list), 12u * 8u);
    memcpy(HOST(GREY_LIST + 96), HOST(list), 12); WW(GREY_LIST + 96 + 8, GREY_PAGE);
    uint16_t red = rgb15(0x78, 0x20, 0x00);                         /* colour 9's outline, as it is in the font */
    for (int y = 0; y < FONT1_H; y++) for (int x = 0; x < 256; x++) {
        uint16_t b = (uint16_t)RH(font1_texel(9, x, y));
        if ((b >> 15) && !bright(b)) { red = b; y = FONT1_H; break; }
    }
    const uint16_t dark = rgb15(0x20, 0x20, 0x20), grey = rgb15(0x88, 0x88, 0x88), light = rgb15(0xD0, 0xD0, 0xD0);
    for (int y = 0; y < FONT1_H; y++) for (int x = 0; x < 256; x++) {               /* both in colour 10's glyph shapes */
        uint16_t a = (uint16_t)RH(font1_texel(10, x, y));
        WH(GREY_TEX + (uint32_t)(((FONT_SET_H * GREYED + FONT1_Y + y - 2048) * 256 + x) * 2), !(a >> 15) ? a : bright(a) ? grey : dark);
        WH(GREY_TEX + (uint32_t)(((FONT_SET_H * GREYED_ON + FONT1_Y + y - 2048) * 256 + x) * 2), !(a >> 15) ? a : bright(a) ? light : red);
    }
    WW(FONT_LIST, GREY_LIST); WW(FONT_LIST + 4, 9);
    rt_log("exsetting: font colours 14 and 15 (greyed rows) added on page %X", GREY_PAGE);
}
#define B_L       0x40u
#define B_R       0x200u
static void turn_page(Cpu *c, int step) {
    int n = page_count(), at = page - first_page();
    const int from_end = pages[page].n - row;                  /* DEFAULT and EXIT stay DEFAULT and EXIT */
    page = first_page() + (at + step + n) % n;
    row = from_end <= 2 ? pages[page].n - from_end : row < pages[page].n - 2 ? row : pages[page].n - 3;
    sound(c, 0x38);
}
static void ex_screen(Cpu *c) {
    uint32_t p = pressed();
    if (page < first_page() || page >= first_page() + page_count()) page = first_page();
    const Row *rows = pages[page].rows; int n = pages[page].n;
    const Row *r = &rows[row];
    int step = (p & 1) ? 1 : (p & 2) ? -1 : 0;                /* right / left */
    int more = page_count() > 1;
    if (p & 8) { row = (row + n - 1) % n; sound(c, 0x38); }
    else if (p & 4) { row = (row + 1) % n; sound(c, 0x38); }
    else if (more && (p & (B_L | B_R))) turn_page(c, (p & B_R) ? 1 : -1);
    else if ((step || (p & B_CONFIRM)) && r->greyed) sound(c, 0x2F);   /* not there yet: nothing to change */
    else if (step && r->value) { int k = count_names(r->names); *r->value = (*r->value + step + k) % k; features_setting_changed(row_feature(r)); sound(c, 0x38); }
    else if (step && more) turn_page(c, step);
    else if ((p & B_CONFIRM) && r->value) { int k = count_names(r->names); *r->value = (*r->value + 1) % k; features_setting_changed(row_feature(r)); sound(c, 0x39); }
    else if ((p & B_CONFIRM) && row == n - 2) { page_defaults(page); sound(c, 0x39); }   /* DEFAULT: this page */
    else if ((p & B_CONFIRM) && row == n - 1) { ex_close(c, 0x39); return; }   /* EXIT chosen: a confirm */
    else if (p & B_BACK) { ex_close(c, 0x2F); return; }
    rows = pages[page].rows; n = pages[page].n; r = &rows[row];
    WB(OPT + 7, (uint32_t)row); bar_y = 239 - ex_row_y(row);
    {   /* the page's own banner (EX. GAME, EX. DISPLAY, ...) where "EX. SETTING" was */
        const char *t = pages[page].title;
        text(c, gstr(t), 2, 0xC0 - 8 * (int)strlen(t), 5, 0xB);
    }
    for (int i = 0; i < n; i++) {
        int y = ex_row_y(i), colour = rows[i].greyed ? (i == row ? GREYED_ON : GREYED) : i == row ? 0xB : 0xA;
        text(c, gstr(rows[i].label), 1, 0x30, y, colour);
        if (rows[i].value) {
            int k = count_names(rows[i].names), v = *rows[i].value;
            text(c, gstr(rows[i].names[v >= 0 && v < k ? v : 0]), 1, pages[page].value_x, y, rows[i].greyed ? (i == row ? GREYED_ON : GREYED) : 0xB);   /* values: always yellow, as GAME OPTION */
        }
    }
    for (int k = 0; k < 2; k++) if (r->what[k][0]) text(c, gstr(r->what[k]), 0, 0x30, WHAT_Y + 10 * k, 9);
    {                                                         /* the page number, 1/1 too (the user, 0.127.0) */
        static const char *const numbers[N_PAGES][N_PAGES] = { { "1/1" }, { "1/2", "2/2" }, { "1/3", "2/3", "3/3" },
            { "1/4", "2/4", "3/4", "4/4" }, { "1/5", "2/5", "3/5", "4/5", "5/5" }, { "1/6", "2/6", "3/6", "4/6", "5/6", "6/6" } };
        text(c, gstr(numbers[page_count() - 1][page - first_page()]), 2, 0x178 - 48, PAGE_Y, 9);
    }
    text(c, STR(S_VERSION), 0, VERSION_X, VERSION_Y, 0xA);   /* The project's version, bottom right in the small font: just shown */
}

/* VIRTUAL PSPS OFF: the network is a stub (hle_net.c), and going into NETWORK with it crashes the game.
 * So, once a frame while the MODE MENU runs (padicons_mode_menu, before 0x0888C5B0), its NETWORK tile is
 * greyed and can't be chosen: the cursor can still land on it (bright, as any tile under the cursor; dim
 * otherwise), but Cross / START there only gives the back sound. The tile is a picture in texture page
 * 0x30 (176 x 31 at y 192, tiles.c's source list): its texels are turned to their lightness, kept as they
 * were, and put back when VIRTUAL PSPS is on again. A page loaded afresh is greyed again. */
#define NET_PAGE   0x30
#define NET_V      192
#define NET_W      176
#define NET_H      31
#define MODE_NETWORK 12
#define MENU_PADS  0x08E75BA0u            /* slot 0 +12, slot 1 +56: new presses (the MODE MENU reads these two) */
static uint16_t net_kept[NET_H][NET_W];
static uint32_t net_kept_at;            /* the page's texels the copy is of (0: none) */
static uint32_t net_texel(uint32_t t, int x, int y) { return t + (uint32_t)(((NET_V + y) * 256 + x) * 2); }
static int grey15(uint16_t p) { int r = p & 31, g = (p >> 5) & 31, b = (p >> 10) & 31; return (r * 77 + g * 150 + b * 29) >> 8; }
void exsetting_mode_menu(Cpu *c, uint32_t g) {
    uint32_t e = PAGES + 0x18u * NET_PAGE, t = RW(e + 4), t0 = RW(e);
    if (!t) return;
    if (rt_virtual_psps) {
        if (net_kept_at == t) {
            for (int y = 0; y < NET_H; y++) for (int x = 0; x < NET_W; x++) {
                WH(net_texel(t, x, y), net_kept[y][x]); if (t0 && t0 != t) WH(net_texel(t0, x, y), net_kept[y][x]);
            }
            net_kept_at = 0;
        }
        return;
    }
    int fresh = net_kept_at != t;
    for (int y = 0; y < NET_H && !fresh; y++) for (int x = 0; x < NET_W && !fresh; x++) {   /* any colour: loaded afresh */
        uint16_t p = (uint16_t)RH(net_texel(t, x, y));
        fresh = (p & 31) != ((p >> 5) & 31) || ((p >> 5) & 31) != ((p >> 10) & 31);
    }
    if (fresh) {
        for (int y = 0; y < NET_H; y++) for (int x = 0; x < NET_W; x++) {
            uint16_t p = (uint16_t)RH(net_texel(t, x, y)); int l = grey15(p);
            net_kept[y][x] = p;
            uint16_t q = (uint16_t)((p & 0x8000) | l | l << 5 | l << 10);
            WH(net_texel(t, x, y), q); if (t0 && t0 != t) WH(net_texel(t0, x, y), q);
        }
        net_kept_at = t;
    }
    if ((int8_t)RB(g + 0x72) == MODE_NETWORK) {
        uint32_t m = RW(MENU_PADS + 12) | RW(MENU_PADS + 56);
        if (m & B_CONFIRM) {
            WW(MENU_PADS + 12, RW(MENU_PADS + 12) & ~B_CONFIRM); WW(MENU_PADS + 56, RW(MENU_PADS + 56) & ~B_CONFIRM);
            sound(c, 0x2F);
        }
    }
}

/* PLAYER CONFIG: P1 - P4 CONFIG and EXIT on the menu's buttons */
static void players_open(Cpu *c, int at) { mode = M_PLAYERS; tile = at; WB(OPT + 5, 0); sound(c, 0x39); }
static void players_screen(Cpu *c);
static void pedit_open(Cpu *c, int p);

static void players_screen(Cpu *c) {
    uint32_t p = pressed();
    if (p & 8) { tile = (tile + 4) % 5; sound(c, 0x38); }
    else if (p & 4) { tile = (tile + 1) % 5; sound(c, 0x38); }
    else if ((p & B_CONFIRM) && tile < 4) { pedit_open(c, tile); return; }
    else if (p & B_CONFIRM) { mode = M_MENU; sound(c, 0x39); return; }         /* EXIT */
    else if (p & B_BACK) { mode = M_MENU; sound(c, 0x2F); return; }
    title(c, S_PLAYER);
}

/* P1 - P4 CONFIG: the game's BUTTON CONFIG rows, with L2 and R2, for any player */
enum { E_SQ, E_TR, E_L, E_CR, E_CI, E_R, E_SEL, E_L2, E_R2, E_CMD, E_TYPE, E_RAPID, E_DEFAULT, E_EXIT, N_EROWS };
#define N_ATTACKS 0x37                      /* 0x088B7D08 limit */
#define ATTACK_NAMES 0x08A37B44u
static const uint32_t table_cmd = 0x08A37A5Cu, table_type = 0x08A37A68u, table_rapid = 0x08A37A70u;
static int erow_y(int r) { return 0x18 + 14 * r; }
/* The attack pictures of the game's BUTTON CONFIG (L / M / H with a fist or a foot, "Taunt"): objects
 * of kind 0x93, types 0x24 - 0x2A (0x08822EAC makes one; once a frame 0x088BB268, which shows the
 * value of its button's row in P1's or P2's config). Ours, one per button row, show the value of
 * the player being edited: animation 0x08A38010[value] for values 0 - 14; longer ones are names. */
void f_08822EAC(Cpu *c);                    /* new object: a0 kind, a1 type; v0 = object */
void f_088231C8(Cpu *c);                    /* free an object */
void f_0882196C(Cpu *c);                    /* object a0: animation a1 */
void f_088215C4(Cpu *c);                    /* object a0: step and draw */
#define ICON_ANIMS 0x08A38010u
#define N_ICONS    (E_R2 + 1)
static uint32_t icon[N_ICONS];
static int icon_v[N_ICONS];
static uint32_t call2(Cpu *c, void (*f)(Cpu *), uint32_t a0, uint32_t a1) {
    uint32_t keep[8], ra = c->r[31], v0 = c->r[2]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = a0; c->r[5] = a1; f(c);
    uint32_t r = c->r[2];
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra; c->r[2] = v0;
    return r;
}
static int icon_ok(int r) { return icon[r] && RB(icon[r]) && RB(icon[r] + 2) == 0x93; }
static void icons_on(Cpu *c, int from) {
    for (int r = from; r < N_ICONS; r++) {
        if (!icon_ok(r)) icon[r] = call2(c, f_08822EAC, 0x93, (uint32_t)(0x24 + (r < 7 ? r : 6)));
        icon_v[r] = -1;
        if (icon[r]) WB(icon[r] + 1, 0);
    }
}
static void icons_off(Cpu *c) {
    for (int r = 0; r < N_ICONS; r++) { if (c && icon_ok(r)) call2(c, f_088231C8, icon[r], 0); icon[r] = 0; }
}
static int is_icon(uint32_t o) { for (int r = 0; r < N_ICONS; r++) if (icon[r] && icon[r] == o) return 1; return 0; }
/* one button row's value: a picture (1) or not (0, the name is drawn as text) */
static int icon_show(Cpu *c, int r, int v, int y) {
    if (!icon_ok(r)) return 0;
    uint32_t o = icon[r];
    if (v < 0 || v >= 15) { WB(o + 1, 0); return 0; }
    if (icon_v[r] != v) { call2(c, f_0882196C, o, RW(ICON_ANIMS + 4u * (uint32_t)v)); icon_v[r] = v; }
    WH(o + 0x12, 0xB0); WH(o + 0x16, (uint32_t)(240 - y)); WB(o + 1, 1);
    return 1;
}
/* their once-a-frame: just drawn where pedit_screen put them */
int exsetting_attack_picture(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    uint32_t o = c->r[4];
    if (!is_icon(o)) { inner_f_088BB268(c); return 1; }
    if (mode != M_PEDIT) WB(o + 1, 0);
    if (RB(o + 1)) call2(c, f_088215C4, o, 0);
    return 1;
}
static void pedit_open(Cpu *c, int p) {
    mode = M_PEDIT; editing = p; row = 0; WB(OPT + 5, EX_ITEM + 1); WB(OPT + 7, 0);
    bar_y = 239 - erow_y(0); bar_on(c); two_on(c); icons_on(c, 0); sound(c, 0x39);
}
static int *attack_of(int r, uint8_t *cfg, int *tmp) {
    if (r <= E_SEL) { *tmp = cfg[2 * r] | cfg[2 * r + 1] << 8; return tmp; }
    return r == E_L2 ? &rt_l2[editing] : &rt_r2[editing];
}
static void pedit_screen(Cpu *c) {
    uint8_t *cfg = pcfg_block(editing);
    uint32_t p = pressed();
    int step = (p & 1) ? 1 : (p & 2) ? -1 : 0;               /* right / left */
    if (p & 8) { row = (row + N_EROWS - 1) % N_EROWS; sound(c, 0x38); }
    else if (p & 4) { row = (row + 1) % N_EROWS; sound(c, 0x38); }
    else if ((p & B_CONFIRM) && row == E_DEFAULT) { memcpy(cfg, pcfg_default, 20); rt_l2[editing] = 6; rt_r2[editing] = 7; sound(c, 0x39); }
    else if (((p & B_CONFIRM) && row == E_EXIT) || (p & B_BACK)) {
        mode = M_PLAYERS; WB(OPT + 5, 0); bar_off(); two_off(); icons_off(c); bar_y = -1; settings_save(); sound(c, (p & B_BACK) ? 0x2F : 0x39); return;
    } else if (step || ((p & B_CONFIRM) && row < E_DEFAULT)) {
        if (!step) step = 1;
        if (row <= E_R2) {
            int tmp, *v = attack_of(row, cfg, &tmp);
            *v = (*v + step + N_ATTACKS) % N_ATTACKS;
            if (row <= E_SEL) { cfg[2 * row] = (uint8_t)*v; cfg[2 * row + 1] = (uint8_t)(*v >> 8); }
        } else if (row == E_CMD) cfg[0x10] = (uint8_t)((cfg[0x10] + step + 3) % 3);
        else if (row == E_TYPE) cfg[0x11] ^= 1;
        else if (row == E_RAPID) cfg[0x12] ^= 1;
        sound(c, 0x38);
    }
    WB(OPT + 7, (uint32_t)row); bar_y = 239 - erow_y(row);
    title(c, S_P1C + editing);
    static const signed char sym[9] = { 2, 1, 8, 3, 0, 9, -1, 8, 9 };   /* button symbols (font 5); -1 SELECT */
    for (int r = 0; r < N_EROWS; r++) {
        int y = erow_y(r), colour = r == row ? 0xB : 0xA;
        if (r <= E_R2) {
            int x = (sym[r] == 8 || sym[r] == 9) ? 0x6C : 0x68;
            if (r == E_L2 || r == E_R2) x = TWO_SYM_X;
            if (sym[r] < 0) symbol(c, 1, 0, x, y); else symbol(c, 0, sym[r], x, y);
            int tmp, v = *attack_of(r, cfg, &tmp);
            if (!icon_show(c, r, v, y)) text(c, v == 14 ? STR(S_TAUNT) : RW(ATTACK_NAMES + 4u * (uint32_t)v), 1, 0xB0, y, 0xB);   /* values: always yellow */
        } else {
            text(c, STR(S_COMMAND + (r - E_CMD)), 1, 0x68, y, colour);
            if (r == E_CMD) text(c, RW(table_cmd + 4u * (cfg[0x10] % 3)), 1, 0xB0, y, 0xB);
            if (r == E_TYPE) text(c, RW(table_type + 4u * (cfg[0x11] & 1)), 1, 0xB0, y, 0xB);
            if (r == E_RAPID) text(c, RW(table_rapid + 4u * (cfg[0x12] & 1)), 1, 0xB0, y, 0xB);
        }
    }
}

static int ensure_mem(void) {
    if (mem) return 1;
    mem = rt_alloc(0x40180 + 0x20 * S_COUNT, 1, 256, "ex. setting");
    if (!mem) return 0;
    memset(HOST(TEX), 0, 256 * 256 * 2);
    for (int i = 0; i < S_COUNT; i++) strcpy((char *)HOST(STR(i)), strings[i]);
    return 1;
}

/* ---------------------------------------------------------------- hooks */
/* the option menu, once a frame */
static void option_frame(Cpu *c);
/* Opening or closing a submenu (the game's, or ours), the frames in between draw neither screen
   properly: the menu with every button dimmed or with nothing but its background (opening), or the
   submenu without its bar (closing; SAVE LOAD also with its cursor back on SAVE). The screen is held
   from the change until the next screen is drawn, paced so that the confirm / back sound plays on. */
#define SWITCH_HOLD 4
int exsetting_option_menu(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    int before = mode * 2 + !!RB(OPT + 6);
    option_frame(c);
    if (mode * 2 + !!RB(OPT + 6) != before) rt_hold_display_paced(SWITCH_HOLD);
    return 1;
}
static void option_frame(Cpu *c) {
    if (!ensure_mem()) { inner_f_088B6164(c); return; }
    if (seen < 0 && !sub_from) WB(OPT + 1, 0);               /* OPTION MODE just chosen: start on GAME OPTION */
    seen = (long)rt_vblank_count();
    int have_buttons = 0; for (int k = 0; k < OBJ_N; k++) if (button(k) >= 0) have_buttons = 1;
    if (have_buttons) { if (!built) build_picture(); page_on(); clone_on(); }
    if (!RB(OPT + 6) && sub_from && mode == M_MENU) { WB(OPT + 1, sub_from); sub_from = 0; }   /* a submenu ended (LOAD ends SAVE LOAD itself) */
    if (mode == M_EX) { ex_screen(c); return; }
    if (mode == M_PLAYERS) { players_screen(c); return; }
    if (mode == M_PEDIT) { pedit_screen(c); return; }
    if (!RB(OPT + 6)) {                                       /* on the menu: the cursor is our item */
        item_v = RB(OPT + 1);
        uint32_t p = pressed();
        if ((p & B_CONFIRM) && item_v == EX_ITEM) { ex_open(c); return; }
        if ((p & B_CONFIRM) && item_v == PLAYER_ITEM) { players_open(c, 0); return; }
        if ((p & B_CONFIRM) && item_v > EX_ITEM) { if (item_v < N_ITEMS - 1) sub_from = item_v; WB(OPT + 1, item_v - 1); }   /* the game's item (EXIT leaves: nothing to come back to) */
        inner_f_088B6164(c);
        if (!RB(OPT + 6) && sub_from) { WB(OPT + 1, sub_from); sub_from = 0; }
        return;
    }
    inner_f_088B6164(c);                                       /* a submenu of the game's */
    if (RB(OPT + 1) == 2) { void widescreen_bar(void); widescreen_bar(); }   /* SCREEN SETTING */
    if (!RB(OPT + 6) && sub_from) { WB(OPT + 1, sub_from); sub_from = 0; }
}

/* menu cursor: one more item when called from the option menu */
int exsetting_menu_cursor(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    if ((c->r[31] == 0x088B61FCu || c->r[31] == 0x088B6210u) && c->r[6] == 4) c->r[6] = N_ITEMS - 1;
    if (c->r[31] == 0x088B7144u || c->r[31] == 0x088B7158u) { c->r[2] = c->r[5]; return 1; }   /* SAVE LOAD: moved already (f_088B70F8) */
    inner_f_088B7EC0(c);
    return 1;
}

/* a menu button, once a frame: six buttons on the menu, five tiles on PLAYER CONFIG */
static void exsetting_lay_out(Cpu *c, uint32_t o);
#define LAYOUT 0x08A37C60u                  /* per item: size, page, u0, v0, u1, v1 (16 bytes) */
static void picture(uint32_t o, int page, int v0) { WW(o + 0x3C, (uint32_t)page); WH(o + 0x40, 0); WH(o + 0x42, 192); WH(o + 0x4C, (uint32_t)v0); WH(o + 0x4E, (uint32_t)(v0 + 32)); }
int exsetting_option_button(Cpu *c, uint32_t o, int after) {
    if (!after) return 0;
    exsetting_lay_out(c, o);
    return 0;
}
static void exsetting_lay_out(Cpu *c, uint32_t o) {
    int k = (int)((o - OBJS) / OBJ_SZ), b = button(k);
    if (b < 0) return;
    if (starting && built) { starting = 0; rt_hold_display_set(3); }
    int item, on, y, h, slot;
    if (mode == M_PLAYERS) {                                  /* tiles: P1-P3 on items 0-2, P4 on ours, EXIT */
        item = k == clone ? 3 : b == 3 ? -1 : b;
        WB(o + 0x5D, item >= 0);
        if (item < 0) return;
        if (item < 4) picture(o, OUR_PAGE, 32 * (ROW_P1 + item));
        else if (tiles_ok && page2_ours()) picture(o, OUR_PAGE2, 32 * ROW_EXIT);
        else picture(o, (int)RW(LAYOUT + 16 * 4 + 4), (int)RH(LAYOUT + 16 * 4 + 0xA));
        on = tile == item; slot = item; y = 80 + 64 * item; h = 64;
    } else {
        item = k == clone ? EX_ITEM : b < EX_ITEM ? b : b + 1;
        if (k == clone) picture(o, OUR_PAGE, 32 * ROW_EX);
        else if (b == PLAYER_ITEM) picture(o, OUR_PAGE, 32 * ROW_PLAYER);
        else if (tiles_ok && b == SL_ITEM) picture(o, OUR_PAGE, 32 * ROW_SL);
        else if (tiles_ok && page2_ours() && (b == 0 || b == 2 || b == 4)) picture(o, OUR_PAGE2, 32 * (b == 0 ? ROW_GO : b == 2 ? ROW_SCR : ROW_EXIT));
        else picture(o, (int)RW(LAYOUT + 16u * (uint32_t)b + 4), (int)RH(LAYOUT + 16u * (uint32_t)b + 0xA));
        on = mode == M_MENU && RB(OPT + 1) == item && !RB(OPT + 6);
        if (mode != M_MENU) WB(o + 0x5D, 0);
        slot = item; y = 80 + 53 * item; h = 53;              /* five rows of 64 become six of 53 */
    }
    WH(o + 0xA, (uint32_t)y);
    WH(o + 6, (uint16_t)((slot & 1) ? x_odd : x_even));
    WB(o + 0x38, on ? 0 : 1); WB(o + 0x88, on ? 0xFF : 0x80);
    WH(o + 0xC, on ? (uint16_t)-4 : 0); WH(o + 0xE, on ? 0xC8 : 0xC0);
    WH(o + 0x18, on ? (uint16_t)(h == 64 ? -4 : -3) : 0); WH(o + 0x1A, (uint32_t)(on ? h + h / 8 : h));
    static const uint8_t col[] = { 0x24, 0x25, 0x26, 0x27, 0x2A, 0x2B, 0x2C, 0x2D, 0x30, 0x31, 0x32, 0x33 };   /* corner colours */
    for (size_t i = 0; i < sizeof col; i++) WB(o + col[i], on ? 0xFF : 0x80);
}

/* The option menu's first frames are drawn with its five buttons as the game made them, before they
 * are laid out here: they are kept off screen (the previous, black frame stays up) from the menu's
 * start until a few frames after the buttons' first lay-out (the drawing is that much behind). */

int exsetting_option_start(Cpu *c, uint32_t a0, int after) { if (after) { starting = 1; rt_hold_display_paced(30); } return 0; }   /* paced: the MODE MENU's confirm sound is still playing */

/* once a frame: tidy up when the option menu is gone */
void exsetting_frame(void) {
    if (!mem || seen < 0) return;
    /* Circle on the menu moves OPTION MODE on to its next state (+0 = 2: the fade out), so the menu's
       frame (above) stops running while its buttons still fade out on screen: they keep our pictures,
       the sixth button and the lay-out until they are gone, so the menu fades out as it looked (EXIT
       takes its buttons away first, so it has nothing to keep) */
    if (RB(OPT) == 2 && mode == M_MENU) {
        int have_buttons = 0; for (int k = 0; k < OBJ_N; k++) if (button(k) >= 0) have_buttons = 1;
        if (have_buttons) { seen = (long)rt_vblank_count(); return; }
    }
    /* (not sub_from: a load or save runs frames of its own inside a submenu, without the menu) */
    if ((long)rt_vblank_count() - seen > 2) { clone_off(); page_off(); bar_off(); two_off(); arrows_off(); icons_off(NULL); mode = M_MENU; seen = -1; }
}

/* GAME OPTION's green bar, once a frame: it shows only on GAME OPTION (+5 == 1) and goes away when
 * no submenu is open (+6 == 0); ours is shown on our screens */
int exsetting_green_bar(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    uint32_t o = c->r[4];
    for (int i = 0; i < 2; i++) if (arrow[i] >= 0 && o == obj(arrow[i])) { arrow_frame(o, i); return 1; }
    for (int i = 0; i < 2; i++) if (two[i] >= 0 && o == obj(two[i])) {
        if (mode == M_PEDIT) two_frame(o, erow_y(E_L2 + i)); else WB(o + 0x5D, 0);
        return 1;
    }
    if (!((mode == M_EX || mode == M_PEDIT) && bar >= 0 && o == obj(bar))) { inner_f_088BA8F4(c); return 1; }
    uint32_t k5 = RB(OPT + 5), k6 = RB(OPT + 6);
    WB(OPT + 5, 1); WB(OPT + 6, 1);
    inner_f_088BA8F4(c);
    WB(OPT + 5, k5); WB(OPT + 6, k6);
    if (bar_y >= 0) WH(o + 0xA, (uint32_t)bar_y);
    return 1;
}

/* ---------------------------------------------------------------- SAVE LOAD: IMPORT and EXPORT */
/* The game's SAVE LOAD submenu (logic 0x088B70F8, drawing 0x088B7AE8; row in the option context's +7:
 * SAVE, LOAD, DELETE, AUTO SAVE, EXIT) gets two more rows before EXIT:
 *   IMPORT  reads the PSP saves (system data and World Tour) in <work>/import (savestore.c), keeps
 *           them in a3mx.dat / a3mx.cfg (the previous ones are copied to .bak first) and gives
 *           them to the game at once
 *   EXPORT  writes the saves to <work>/export as a PSP keeps them (encrypted, with PARAM.SFO), to
 *           copy to a PSP's or PPSSPP's PSP/SAVEDATA
 * The game's rows 0-3 work as before; ours are handled here, and the whole list is drawn here. */
void f_0880A39C(Cpu *c);                    /* leaving an option submenu */
void f_088B73B8(Cpu *c);                    /* SAVE LOAD's LOAD row: on confirm, loads the system data and puts it in effect */
int store_import(const char *dir);
int store_export_dir(const char *dir);
void store_path(char *out, size_t n, const char *name);
enum { SL_SAVE, SL_LOAD, SL_DELETE, SL_AUTO, SL_IMPORT, SL_EXPORT, SL_EXIT, SL_ROWS };
#define SL_TASK       0x09523C60u           /* save / load task: +4 message, +5 result (sysdata.c) */
#define MSG_SAVE_DONE 0x0E                  /* "Save completed." */
#define MSG_LOAD_DONE 0x11                  /* "Load completed." */
/* After the game's save or load returns, the frame it returns into is only half drawn (black on
   screen), and the next has the bar where the game's own table puts it: hold until the menu is whole */
#define SL_AFTER      4
static int sl_y(int r) { return 0x26 + 0x14 * r; }       /* up a little: room for the status line under EXIT */
static char sl_msg[2][48];
static long sl_msg_until;
static uint32_t sl_strings;                 /* our strings in guest memory: IMPORT, EXPORT, the message */
static void sl_say(const char *a, const char *b) {
    snprintf(sl_msg[0], sizeof sl_msg[0], "%s", a); snprintf(sl_msg[1], sizeof sl_msg[1], "%s", b ? b : "");
    sl_msg_until = (long)rt_vblank_count() + 300;
}
static void backup(const char *name) {
    char from[1100], to[1200]; store_path(from, sizeof from, name); snprintf(to, sizeof to, "%s.bak", from);
    FILE *f = fopen(from, "rb"); if (!f) return;
    FILE *g = fopen(to, "wb"); char buf[4096]; size_t n;
    if (g) { while ((n = fread(buf, 1, sizeof buf, f)) > 0) fwrite(buf, 1, n, g); fclose(g); }
    fclose(f);
}
static void sl_leave(Cpu *c, int snd) {
    sound(c, snd);
    uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
    f_0880A39C(c);
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    WB(OPT + 7, 0); WB(OPT + 6, 0);
}
/* the game's own LOAD (0x088B73B8, as if chosen), off screen: it reads the save (savestore.c) and puts
   the settings in effect (with its confirm sound) */
int store_delete_sys(void);
static void sl_game_load(Cpu *c) {
    uint32_t keep[8], ra = c->r[31], k0 = RW(SLOTS + 0xC); memcpy(keep, &c->r[4], sizeof keep);
    WW(SLOTS + 0xC, k0 | B_CONFIRM);
    rt_hold_display_paced(600);
    WB(OPT + 7, SL_LOAD); f_088B73B8(c);
    rt_hold_display_set(SL_AFTER);
    WW(SLOTS + 0xC, k0); memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
}
int exsetting_save_load(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;               /* (leaving it is held by exsetting_option_menu, SWITCH_HOLD) */
    /* up / down over all seven rows, by the game's own cursor mover (its press-and-hold repeat too:
       moving rows by hand left its repeat timer stale, so a held press moved twice) */
    int start = (int8_t)RB(OPT + 7);
    {
        uint32_t keep[8], ra = c->r[31]; memcpy(keep, &c->r[4], sizeof keep);
        for (int k = 0; k < 2; k++) {
            c->r[4] = SLOTS + 0x2Cu * (uint32_t)k; c->r[5] = (uint32_t)(int8_t)RB(OPT + 7); c->r[6] = SL_ROWS - 1; c->r[31] = 0;
            inner_f_088B7EC0(c); WB(OPT + 7, c->r[2]);
        }
        memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra;
    }
    int r = (int8_t)RB(OPT + 7);
    uint32_t p = pressed();
    if (r != start) return 1;
    if ((r == SL_SAVE || r == SL_LOAD) && (p & B_CONFIRM)) {  /* SAVE / LOAD: no question, no screens, a line at the bottom */
        rt_hold_display_paced(600);                           /* (they run frames of their own; paced: the confirm sound plays out) */
        WB(SL_TASK + 4, 0);
        inner_f_088B70F8(c);
        rt_hold_display_set(SL_AFTER);
        int ok = RB(SL_TASK + 5) == 0 && RB(SL_TASK + 4) == (r == SL_SAVE ? MSG_SAVE_DONE : MSG_LOAD_DONE);
        if (ok) sl_say(r == SL_SAVE ? "SYSTEM DATA SAVED" : "SYSTEM DATA LOADED", NULL);
        else rt_hold_display_set(0);                          /* something went wrong: the game's own message shows */
        return 1;
    }
    if (r == SL_DELETE && (p & B_CONFIRM)) {                  /* DELETE: the system data back to the defaults (World Tour kept) */
        backup("a3mx.dat"); backup("a3mx.cfg");
        if (store_delete_sys()) { sl_game_load(c); WB(OPT + 7, SL_DELETE); sl_say("SYSTEM DATA DELETED", NULL); }
        else { sl_say("DELETE FAILED", NULL); sound(c, 0x2F); }
        return 1;
    }
    if (r < SL_IMPORT) { inner_f_088B70F8(c); return 1; }        /* the game's own rows */
    if (p & B_BACK) { sl_leave(c, 0x2F); return 1; }
    if (!(p & B_CONFIRM)) return 1;
    char dir[1100];
    if (r == SL_EXIT) { sl_leave(c, 0x39); return 1; }
    if (r == SL_IMPORT) {
        store_path(dir, sizeof dir, "import");
        backup("a3mx.dat"); backup("a3mx.cfg");
        int n = store_import(dir);
        if (n) {
            /* the game takes it in by its own LOAD (0x088B73B8, as if chosen): the load reads the new
               save (savestore.c) and puts the settings in effect */
            sl_game_load(c);
            WB(OPT + 7, SL_IMPORT);
            sl_say(n == 2 ? "IMPORTED SYSTEM AND WORLD TOUR" : "IMPORTED 1 SAVE", "OLD SAVES KEPT AS .BAK");
        }
        else { sl_say("NO PSP SAVE FOUND IN", "THE IMPORT FOLDER"); sound(c, 0x2F); }
    } else {
        store_path(dir, sizeof dir, "export");
        int n = store_export_dir(dir);
        if (n) { sl_say(n == 2 ? "EXPORTED SYSTEM AND WORLD TOUR" : "EXPORTED 1 SAVE", "TO THE EXPORT FOLDER"); sound(c, 0x39); }
        else { sl_say("EXPORT FAILED", NULL); sound(c, 0x2F); }
    }

    return 1;
}
int exsetting_save_load_text(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    if (!sl_strings) {
        sl_strings = rt_alloc(0x100, 1, 16, "save load");
        if (!sl_strings) { inner_f_088B7AE8(c); return 1; }
        strcpy((char *)HOST(sl_strings), "IMPORT"); strcpy((char *)HOST(sl_strings + 0x10), "EXPORT");
    }
    static const uint32_t game_str[SL_ROWS] = { 0x089FF914u, 0x089FF91Cu, 0x089FF924u, 0x089FF92Cu, 0, 0, 0x089FF8ACu };
    int r = (int8_t)RB(OPT + 7);
    text(c, 0x089FF938u, 2, 0x78, 5, 0xB);                   /* SAVE LOAD */
    for (int i = 0; i < SL_ROWS; i++) {
        uint32_t s = game_str[i] ? game_str[i] : sl_strings + (i == SL_IMPORT ? 0 : 0x10);
        text(c, s, 1, 0x80, sl_y(i), i == r ? 0xB : 0xA);
    }
    text(c, RW(0x08A37AACu + 4u * (uint32_t)((int8_t)RB(0x08A058A0u + 0x4F) & 1)), 1, 0xE0, sl_y(SL_AUTO), 0xB);   /* AUTO SAVE ON / OFF */
    if ((long)rt_vblank_count() < sl_msg_until)
        for (int k = 0; k < 2; k++) if (sl_msg[k][0]) {
            strcpy((char *)HOST(sl_strings + 0x20 + 0x30u * (uint32_t)k), sl_msg[k]);
            text(c, sl_strings + 0x20 + 0x30u * (uint32_t)k, 0, 0xC0 - 4 * (int)strlen(sl_msg[k]), 0xC6 + 10 * k, 9);
        }
    return 1;
}

/* SAVE LOAD's bar (object kind 0xC, type 0x14), once a frame: on our row (its y is 240 - the row's) */
int exsetting_save_load_bar(Cpu *c, uint32_t o, int after) {
    if (after && RB(OPT + 6) && RB(OPT + 5) == SL_ITEM + 1 && mode == M_MENU) WH(o + 0xA, (uint32_t)(240 - sl_y((int8_t)RB(OPT + 7))));
    return 0;
}

/* ---------------------------------------------------------------- the pause menu's BUTTON CONFIG (M4)
 * The pause menu's BUTTON CONFIG (a task: 0x088B8ED0 opens it, 0x088BA028 once a frame) edits the
 * config of the player who paused (context 0x08B02E24: +0 - +2 each player's row 0 - 11, +4 that
 * player): input 0x088B8FC4 / 0x088B90E4 / 0x088B9208 for players 1 - 3 (up / down 0x088B9CEC, left /
 * right 0x088B93BC, Circle on DEFAULT 0x088B96B0, on EXIT or Cross: leave), drawing 0x088B9790, the
 * attack pictures objects of kind 0 (types player x 8 + 1 + row, made by 0x08822EE4, placed once) and
 * the green bar an object of kind 3 type 5 (once a frame 0x08888CC0, rows of 16 from the table at
 * 0x08A2620C).
 *
 * Here it gets the L2 and R2 rows of P1 - P4 CONFIG: the same fourteen rows, 14 apart. Our row per
 * player (pex) maps to the game's (L2 / R2 to COMMAND's, which has no picture); up / down, and left
 * / right on L2 / R2, are ours; the rest is the game's. The 2 of L2 / R2 and their pictures are the
 * ones P1 - P4 CONFIG uses (our page 4; kind 0x93 objects). Leaving saves (a3mx.cfg). */
void f_088B9CEC(Cpu *c);                    /* up / down with repeat: a0 pad, a1 row, a2 last row; v0 row */
void f_088B9B30(Cpu *c);                    /* left / right with repeat: a0 pad, a1 value, a2 last value; v0 value */
#define PCTX      0x08B02E24u
#define PICONS    0x09514AB0u               /* the pool of the attack pictures: 16 x 0xC0, downwards */
static const uint32_t pause_cmd = 0x08A37C44u, pause_type = 0x08A37C50u, pause_rapid = 0x08A37C58u;
static int pex[3];
static long pause_seen = -100, pause_opened = -100;
static int pause_closing;                   /* left, still closing (pause_end, pause_frame) */
void f_08822EE4(Cpu *c);                    /* new object of the pause's pool: a0 kind, a1 type; v0 = object */
void selectscreen_two(int x, int y);
#define PAUSE_ANIMS 0x08A3CC78u             /* the pause's attack pictures, by value */
static uint32_t picon[2];
static int picon_v[2];
static int pause_live(void) { return (long)rt_vblank_count() - pause_seen <= 2; }
static int game_row(int ex) { return ex <= E_SEL ? ex : ex <= E_R2 ? 7 : ex - 2; }
static uint32_t call3(Cpu *c, void (*f)(Cpu *), uint32_t a0, uint32_t a1, uint32_t a2) {
    uint32_t keep[8], ra = c->r[31], v0 = c->r[2]; memcpy(keep, &c->r[4], sizeof keep);
    c->r[4] = a0; c->r[5] = a1; c->r[6] = a2; f(c);
    uint32_t r = c->r[2];
    memcpy(&c->r[4], keep, sizeof keep); c->r[31] = ra; c->r[2] = v0;
    return r;
}
/* L2's and R2's pictures are made when the menu opens, with the game's own (kind 0 type 0x18, just
 * drawn): an object takes a few frames to show its picture, so made later they came in late */
static void pause_open(Cpu *c) {
    pex[0] = pex[1] = pex[2] = 0; pause_closing = 0;
    ensure_mem();
    for (int i = 0; i < 2; i++) {
        picon[i] = call2(c, f_08822EE4, 0, 0x18); picon_v[i] = -1;
        if (picon[i]) WW(picon[i] + 0x58, PCTX);
    }
    pause_seen = pause_opened = (long)rt_vblank_count();
}
int exsetting_pause_open(Cpu *c, uint32_t a0, int after) { if (after) { uint32_t v0 = c->r[2]; pause_open(c); c->r[2] = v0; } return 0; }
static void pause_begin(Cpu *c) {
    if (!pause_live() && !picon[0]) pause_open(c);
    pause_closing = 0;
    pause_seen = (long)rt_vblank_count();
}
/* Leaving: the menu is still drawn while it closes, so our rows stay up until the game's pictures
 * go (pause_frame), and L2's and R2's go with them. */

static void pause_end(Cpu *c) {
    (void)c;
    settings_save();
    pause_closing = 1;
}
static int game_pictures(void) {
    for (int k = 0; k < 16; k++) {
        uint32_t o = PICONS - 0xC0u * (uint32_t)k;
        if (RB(o) && RB(o + 2) == 0 && RB(o + 3) >= 1 && RB(o + 3) < 0x18) return 1;
    }
    return 0;
}
void pause_frame(void) {                    /* once a frame, after the 2D objects have run (selectscreen.c) */
    if (!pause_closing || game_pictures()) return;
    for (int i = 0; i < 2; i++) {
        uint32_t o = picon[i];
        if (o && RB(o) && RB(o + 2) == 0 && RB(o + 3) == 0x18) WH(o + 0x12, (uint16_t)-200);   /* (the game frees its kind 0 objects itself) */
        picon[i] = 0;
    }
    pause_closing = 0; pause_seen = -100;
}
static void pause_input(Cpu *c, int pl, void (*orig)(Cpu *)) {
    uint32_t task = c->r[6], pad = SLOTS + 0x2Cu * (uint32_t)pl;
    pause_begin(c);
    uint32_t now = RW(pad + 0xC) | RW(pad + 4);
    int ex = pex[pl];
    if (now & 0xC) {                                          /* up / down: our rows */
        ex = (int)call3(c, f_088B9CEC, pad, (uint32_t)ex, N_EROWS - 1);
        pex[pl] = ex; WB(PCTX + (uint32_t)pl, (uint32_t)game_row(ex));
        return;
    }
    WB(PCTX + (uint32_t)pl, (uint32_t)game_row(ex));
    if ((ex == E_L2 || ex == E_R2) && (now & 3)) {            /* left / right on L2 / R2 */
        int *v = ex == E_L2 ? &rt_l2[pl] : &rt_r2[pl];
        *v = (int)call3(c, f_088B9B30, pad, (uint32_t)*v, N_ATTACKS - 1);
        return;
    }
    int dflt = (RW(pad + 0xC) & 0x80) && ex == E_DEFAULT;
    uint32_t before = RB(task);
    orig(c);
    if (dflt) { rt_l2[pl] = 6; rt_r2[pl] = 7; }
    if (RB(task) != before) pause_end(c);                      /* leaving */
}
int exsetting_pause_input_1(Cpu *c, uint32_t a0, int after) { if (!after) pause_input(c, 0, inner_f_088B8FC4); return !after; }
int exsetting_pause_input_2(Cpu *c, uint32_t a0, int after) { if (!after) pause_input(c, 1, inner_f_088B90E4); return !after; }
int exsetting_pause_input_3(Cpu *c, uint32_t a0, int after) { if (!after) pause_input(c, 2, inner_f_088B9208); return !after; }

/* the drawing: fourteen rows */
int exsetting_pause_text(Cpu *c, uint32_t a0, int after) {
    if (after) return 0;
    if (picon[0] && (pause_closing || (!pause_live() && (long)rt_vblank_count() - pause_opened <= 30)))
        pause_seen = (long)rt_vblank_count();                 /* opened with no input yet, or closing */
    if (!pause_live() || !mem) { inner_f_088B9790(c); return 1; }
    uint32_t cfgs = c->r[4];
    int pl = (int8_t)RB(PCTX + 4);
    if (pl < 0 || pl > 2) { inner_f_088B9790(c); return 1; }
    uint8_t *cfg = HOST(cfgs + 20u * (uint32_t)pl);
    int cur = pex[pl];
    static const signed char sym[9] = { 2, 1, 8, 3, 0, 9, -1, 8, 9 };
    for (int r = 0; r < N_EROWS; r++) {
        int y = erow_y(r), colour = r == cur ? 0xB : 0xA;
        if (r <= E_R2) {
            int x = (sym[r] == 8 || sym[r] == 9) ? 0x6C : 0x68;
            if (r == E_L2 || r == E_R2) x = TWO_SYM_X;
            if (sym[r] < 0) symbol(c, 1, 0, x, y); else symbol(c, 0, sym[r], x, y);
            int v = r <= E_SEL ? (cfg[2 * r] | cfg[2 * r + 1] << 8) : r == E_L2 ? rt_l2[pl] : rt_r2[pl];
            if (r <= E_SEL) {                                 /* the game's picture, moved to our row */
                for (int k = 0; k < 16; k++) {
                    uint32_t o = PICONS - 0xC0u * (uint32_t)k;
                    if (RB(o) && RB(o + 2) == 0 && RB(o + 3) == (uint32_t)(pl * 8 + 1 + r)) WH(o + 0x16, (uint32_t)(240 - y));
                }
                if (v >= 15) text(c, RW(ATTACK_NAMES + 4u * (uint32_t)v), 1, 0xB0, y, 0xB);
            } else {
                selectscreen_two(TWO_X, y + 2);
                uint32_t o = picon[r - E_L2];
                int pic = o && RB(o) && RB(o + 2) == 0 && RB(o + 3) == 0x18 && v < 15;
                if (pic && picon_v[r - E_L2] != v) {            /* (again until its own start has run: +4) */
                    call2(c, f_0882196C, o, RW(PAUSE_ANIMS + 4u * (uint32_t)v));
                    if (RB(o + 4)) picon_v[r - E_L2] = v;
                }
                if (o && RB(o) && RB(o + 2) == 0) { WH(o + 0x12, pic ? 0xB0 : (uint16_t)-200); WH(o + 0x16, (uint32_t)(240 - y)); }
                if (!pic) text(c, RW(ATTACK_NAMES + 4u * (uint32_t)v), 1, 0xB0, y, 0xB);
            }
        } else {
            text(c, STR(S_COMMAND + (r - E_CMD)), 1, 0x68, y, colour);
            if (r == E_CMD) text(c, RW(pause_cmd + 4u * (cfg[0x10] % 3)), 1, 0xB0, y, 0xB);
            if (r == E_TYPE) text(c, RW(pause_type + 4u * (cfg[0x11] & 1)), 1, 0xB0, y, 0xB);
            if (r == E_RAPID) text(c, RW(pause_rapid + 4u * (cfg[0x12] & 1)), 1, 0xB0, y, 0xB);
        }
    }
    return 1;
}
/* the green bar on our row */
void hook_f_08888CC0(Cpu *c) {
    uint32_t o = c->r[4];
    orig_f_08888CC0(c);
    if (!pause_live() || RW(o + 0x7C) != PCTX) return;
    int pl = (int16_t)RH(o + 0x66);
    if (pl >= 0 && pl <= 2) WH(o + 0xA, (uint32_t)(240 - erow_y(pex[pl])));
}
