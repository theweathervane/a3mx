/* The extensions, as features that can be switched on and off one by one (0.60.0).
 *
 * Every hook and wrapper in src/extensions/ belongs to one feature. With all of them off the game is
 * the PSP program alone, exactly as --no-extensions (which is the same thing: no feature on). The
 * launcher's Extensions tab ticks them and starts the game with --features a,b,c; the log's first
 * lines and logs/last_run.txt (features=) say which were on.
 *
 *   --features LIST     the features that are on, by name, comma-separated; "all", "none", or
 *                       "default" (every feature that is on by default) can stand in the list, and
 *                       -name takes one out: --features default,-mode_menu_icons
 *   --no-extensions     none
 *   --custom-assets     as before 0.60.0: adds stage_extensions
 *
 * A feature that needs another is switched off with it, and the log says so (features_set).
 *
 * This header is shared with the launcher (C++): the table is all it needs. The game's own functions
 * are in ext_features.c. */
#pragma once

/* In the launcher's order: its Extensions tab shows them under three headings (FEATURE_SECTIONS). */
enum {
    /* Core */
    FEAT_SAVES,          /* savestore.c, sysdata.c (saves), bootmsg.c */
    FEAT_BOOTMENU,       /* bootmenu.c */
    FEAT_MODE_MENU_ICONS,/* padicons.c */
    FEAT_QUITKEY,        /* quitkey.c */
    FEAT_MENUS,          /* exsetting.c, screensetting.c, playercfg.c, tiles.c */
    /* Display */
    FEAT_WIDESCREEN,     /* widescreen.c: WIDE NATIVE (rt_widescreen) */
    FEAT_I_ISM_DISPLAY,  /* i_ism_display.c */
    FEAT_SCALED_ENDINGS, /* scaled_endings.c */
    FEAT_STAGE_EXT,      /* stagestrips.c */
    FEAT_SHADERS,        /* platform: shader.c (the FILTER setting) */
    /* Extra */
    FEAT_CHEATS_UNLOCKS, /* cheats_unlocks.c, EX. SETTING's pages 1 and 2 (exsetting.c), sysdata.c (UNLOCK ALL) */
    FEAT_MULTIPLAYER,    /* hooks.c, netplay.c, vpeer.c, selectscreen.c */
    FEAT_I_ISM_SELECT,   /* i_ism_select.c */
    FEAT_COUNT,
    FEAT_ANY = FEAT_COUNT   /* on while any feature is: the plumbing several features share */
};

enum { SECTION_CORE, SECTION_DISPLAY, SECTION_EXTRA, SECTION_COUNT };
static const char *const FEATURE_SECTIONS[SECTION_COUNT] = { "Core", "Display", "Extra" };

typedef struct {
    const char *name;        /* --features, last_run.txt */
    const char *title;       /* the launcher's checkbox */
    const char *what;        /* its tooltip */
    int on_by_default;
    int needs;               /* a feature this one can't work without, or -1 */
    int section;             /* the launcher's heading it goes under (SECTION_*) */
} Feature;

static const Feature FEATURES[FEAT_COUNT] = {
    /* ---- Core ---- */
    { "saves", "Silent saves",
      "Saves kept in a3mx.dat / a3mx.cfg instead of a PSP memory stick, with no save messages or\n"
      "questions, and AUTO SAVE always on.\n"
      "Off: the game saves as a PSP does (saves/ms0) and asks its own questions.", 1, -1, SECTION_CORE },
    { "boot_to_menu", "Boot to the MODE MENU",
      "The game opens straight on the MODE MENU, with your settings and save loaded: the boot screens and\n"
      "the load run off screen at full speed, with no title, logos, story, attract demos or PRESS START.\n"
      "If the save can't be loaded, the game's own question is shown first.\n"
      "Off: the title and attract mode first, as on the PSP.", 1, -1, SECTION_CORE },
    { "mode_menu_icons", "MODE MENU icons",
      "The numbered bubble for each player's controller and the FRAME TIMING badge (SMOOTH / ORIGINAL)\n"
      "on the MODE MENU's bar.", 1, -1, SECTION_CORE },
    { "quit_key", "F2 / SELECT + START: back to the MODE MENU",
      "F2 on the keyboard, or player 1 holding SELECT and pressing START, goes straight back to the\n"
      "MODE MENU without saving.", 1, -1, SECTION_CORE },
    { "menus", "Extra menus and settings",
      "OPTION MODE > EX. SETTING and PLAYER CONFIG (P1 - P4, with L2 / R2), VSYNC, FRAME TIMING and\n"
      "FILTER in SCREEN SETTING, IMPORT / EXPORT of PSP saves, BUTTON CONFIG for each player from the\n"
      "pause menu. Needs Silent saves.", 1, FEAT_SAVES, SECTION_CORE },
    /* ---- Display ---- */
    { "widescreen", "Widescreen (WIDE NATIVE)",
      "SCREEN SETTING > SCREEN SIZE gets a third value, WIDE NATIVE: 16:9 with a 4:3 play area\n"
      "(after Rickow's widescreen patch, now the project's own code). Experimental.\n"
      "Off: the game's own NORMAL and WIDE only.", 1, -1, SECTION_DISPLAY },
    { "i_ism_display", "I-ISM ranking and wipe",
      "What the game left out on screen for entry characters (I-ISM): the I-ISM score ranking in the\n"
      "attract mode, and their own wipe into the fight (a yellow I and a gold logo, where they had\n"
      "their ISM's).\n"
      "Off: no I-ISM ranking, and an entry gets its ISM's wipe, as on the PSP.", 0, -1, SECTION_DISPLAY },
    { "scaled_endings", "Scaled endings (WIDE NATIVE)",
      "The endings fill the 16:9 screen: a scene that has more of its picture shows it, the others are\n"
      "drawn 1.25 times larger, and the profile pictures move out to the screen's edges.\n"
      "Off: the endings keep black sides, their scenes the 4:3 picture. Needs Widescreen.", 1, FEAT_WIDESCREEN, SECTION_DISPLAY },
    { "stage_extensions", "Stage Extensions",
      "The project's own new art for the stage edges that WIDE NATIVE leaves black (assets/extensions/).\n"
      "Still placeholders: experimental. Needs Widescreen.", 0, FEAT_WIDESCREEN, SECTION_DISPLAY },
    { "shaders", "Shaders (FILTER)",
      "SCREEN SETTING > FILTER: CRT, scanline and pixel-smoothing shaders (librashader).\n"
      "Off: the picture is shown as it is drawn, and FILTER can't be chosen.", 1, -1, SECTION_DISPLAY },
    /* ---- Extra ---- */
    { "cheats_unlocks", "Cheats and Unlocks",
      "Two more pages in OPTION MODE > EX. SETTING: each fighter's HEALTH, GUARD and SUPER (infinite,\n"
      "half, none, a fixed level), World Tour EXP at its most, and UNLOCK ALL (every mode, fighter and\n"
      "ISM-Plus from the start, on unless you turn it off there). Not in network battles.\n"
      "Off: EX. SETTING has only its own page, nothing is forced, and modes and fighters are unlocked by\n"
      "playing, as on the PSP (what your save has already unlocked stays).", 1, -1, SECTION_EXTRA },
    { "local_multiplayer", "Local multiplayer",
      "Two and three players on one PC: join-in on the select screens, the network modes played\n"
      "locally (EX. SETTING > VIRTUAL PSPS), Variable Battle tagging for two players.\n"
      "Off: one player, and NETWORK looks for other PSPs as the original does.", 1, -1, SECTION_EXTRA },
    { "i_ism_select", "I-ISM for both sides",
      "Entry characters (I-ISM) can be picked for the other side too, where a mode allows it: in Free\n"
      "Battle, Survival, Training and the like, a second player, or player 1 choosing for the computer,\n"
      "can land on the entries' row.\n"
      "Off: only the side whose player is present can pick an entry, as on the PSP.", 1, -1, SECTION_EXTRA },
};

#ifndef FEATURES_TABLE_ONLY
#ifdef __cplusplus
extern "C" {
#endif
extern unsigned char rt_feature_on[FEAT_COUNT + 1];     /* what is on in this run ([FEAT_ANY]: any of them) */
#define FEATURE(f) (rt_feature_on[FEAT_##f])
/* main.c, once, before the game starts: the list as --features gives it (NULL: the defaults). Returns
 * 0 if a name in it isn't a feature (the name in *bad). */
int features_set(const char *list, const char **bad);
void features_none(void);                               /* --no-extensions */
void features_add(int f);                               /* --custom-assets */
const char *features_describe(void);                    /* "saves,menus,..." or "none": the log, last_run.txt */
/* decomp.h's SFA_FUNC_WRAPPED and the generated hooks: the flag an extension's function goes by, from
 * its name (its file's name is the prefix: padicons_mode_menu) or its hook's address */
const unsigned char *feature_flag(const char *ext_name);
const unsigned char *feature_flag_hook(unsigned addr);
#ifdef __cplusplus
}
#endif
#endif
