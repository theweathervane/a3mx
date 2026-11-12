/* The extensions, as features that can be switched on and off one by one (0.60.0).
 *
 * Every hook and wrapper in src/extensions/ belongs to one feature. With all of them off the game is
 * the PSP program alone, exactly as --no-extensions (which is the same thing: no feature on). The
 * launcher's Extensions tab ticks them and starts the game with --features a,b,c; the log's first
 * lines and logs/last_run.txt (features=) say which were on.
 *
 *   --features LIST     the features that are on, by name, comma-separated; "all", "none", or
 *                       "default" (every feature that is on by default) can stand in the list, and
 *                       -name takes one out: --features default,-hud
 *   --no-extensions     none
 *   --custom-assets     as before 0.60.0: adds stage_extensions
 *
 * A feature that needs another is switched off with it, and the log says so (features_set).
 *
 * This header is shared with the launcher (C++): the table is all it needs. The game's own functions
 * are in ext_features.c. */
#pragma once

enum {
    FEAT_SAVES,          /* savestore.c, sysdata.c (saves), bootmsg.c */
    FEAT_UNLOCKS,        /* sysdata.c (unlocks) */
    FEAT_MENUS,          /* exsetting.c, screensetting.c, playercfg.c, tiles.c */
    FEAT_MULTIPLAYER,    /* hooks.c, netplay.c, vpeer.c, selectscreen.c */
    FEAT_HUD,            /* padicons.c */
    FEAT_QUITKEY,        /* quitkey.c */
    FEAT_WIDESCREEN,     /* widescreen.c and the translated 16:9 patch */
    FEAT_STAGE_EXT,      /* stagestrips.c */
    FEAT_SHADERS,        /* platform: shader.c (the FILTER setting) */
    FEAT_COUNT,
    FEAT_ANY = FEAT_COUNT   /* on while any feature is: the plumbing several features share */
};

typedef struct {
    const char *name;        /* --features, last_run.txt */
    const char *title;       /* the launcher's checkbox */
    const char *what;        /* its tooltip */
    int on_by_default;
    int needs;               /* a feature this one can't work without, or -1 */
} Feature;

static const Feature FEATURES[FEAT_COUNT] = {
    { "saves", "Silent saves",
      "Saves kept in a3mx.dat / a3mx.cfg instead of a PSP memory stick, with no save messages or\n"
      "questions, and AUTO SAVE always on.\n"
      "Off: the game saves as a PSP does (saves/ms0) and asks its own questions.", 1, -1 },
    { "unlocks", "Everything unlocked",
      "Every mode, fighter and ISM-Plus is there from the start.\n"
      "Off: they are unlocked by playing, as on the PSP (what your save has already unlocked stays).", 1, -1 },
    { "menus", "Extra menus and settings",
      "OPTION MODE > EX. SETTING and PLAYER CONFIG (P1 - P4, with L2 / R2), VSYNC, FRAME TIMING and\n"
      "FILTER in SCREEN SETTING, IMPORT / EXPORT of PSP saves, BUTTON CONFIG for each player from the\n"
      "pause menu. Needs Silent saves.", 1, FEAT_SAVES },
    { "local_multiplayer", "Local multiplayer",
      "Two and three players on one PC: join-in on the select screens, the network modes played\n"
      "locally (couch co-op), Variable Battle tagging for two players.\n"
      "Off: one player, and NETWORK looks for other PSPs as the original does.", 1, -1 },
    { "hud", "MODE MENU bar",
      "The numbered bubble for each player's controller and the FRAME TIMING badge (SMOOTH / ORIGINAL)\n"
      "on the MODE MENU's bar.", 1, -1 },
    { "quit_key", "F2: back to the MODE MENU",
      "F2 on the keyboard goes straight back to the MODE MENU without saving.", 1, -1 },
    { "widescreen", "Widescreen (WIDE FORCED)",
      "SCREEN SETTING > SCREEN SIZE gets a third value, WIDE FORCED: 16:9 with a 4:3 play area, using\n"
      "Rickow's widescreen patch (downloaded when your disc is extracted). Experimental.\n"
      "Off: the game's own NORMAL and WIDE only.", 1, -1 },
    { "stage_extensions", "Stage Extensions",
      "The project's own new art for the stage edges that WIDE FORCED leaves black (assets/extensions/).\n"
      "Still placeholders: experimental. Needs Widescreen.", 0, FEAT_WIDESCREEN },
    { "shaders", "Shaders (FILTER)",
      "SCREEN SETTING > FILTER: CRT, scanline and pixel-smoothing shaders (librashader).\n"
      "Off: the picture is shown as it is drawn, and FILTER can't be chosen.", 1, -1 },
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
