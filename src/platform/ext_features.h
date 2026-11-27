/* The extensions, as features that can be switched on and off one by one (0.60.0).
 *
 * Every hook and wrapper in src/extensions/ belongs to one feature. With all of them off the game is
 * the PSP program alone, exactly as --no-extensions (which is the same thing: no feature on).
 *
 * Where each one is switched (Feature.where):
 *   WHERE_LAUNCHER     the launcher's Extensions tab: Enhanced saves, Extra menus, Local multiplayer
 *                      (and "PSP original": none at all)
 *   WHERE_EX_GAME      in the game, OPTION MODE > EX. SETTING's EX. GAME page, kept in a3mx.cfg
 *   WHERE_EX_DISPLAY   the same, EX. DISPLAY page
 *   WHERE_EX_ANTHOLOGY the same, EX. ANTHOLOGY page (its first row: the whole Anthology extension)
 *   WHERE_BUILT_IN     no switch of its own: on with the extensions, and what it does is chosen by a
 *                      setting the game already has (SCREEN SIZE's WIDE NATIVE, FILTER) or by EX.
 *                      SETTING's own pages (the cheats and unlocks)
 * So every player runs the same build and the same launcher line; what differs is in a3mx.cfg.
 *
 *   --features LIST     the features that are on, by name, comma-separated; "all", "none", or
 *                       "default" (every feature that is on by default) can stand in the list, and
 *                       -name takes one out: --features default,-mode_menu_icons.
 *                       "settings": every feature that isn't the launcher's starts at its default and
 *                       then follows a3mx.cfg (EX. SETTING); one named in the list stays as named for
 *                       this run (tests). No --features at all is "default,settings"; the launcher
 *                       passes its three and "settings".
 *   --no-extensions     none
 *   --custom-assets     as before 0.60.0: adds stage_extensions
 *
 * A feature that needs another is switched off with it, and the log says so (features_set). The log's
 * first lines and logs/last_run.txt (features=) say which were on.
 *
 * This header is shared with the launcher (C++): the table is all it needs. The game's own functions
 * are in ext_features.c. */
#pragma once

/* The launcher's first, then EX. GAME's and EX. DISPLAY's in their pages' order, then the built-in ones. */
enum {
    /* the launcher */
    FEAT_SAVES,          /* savestore.c, sysdata.c (saves), bootmsg.c */
    FEAT_MENUS,          /* exsetting.c, screensetting.c, playercfg.c, tiles.c */
    FEAT_MULTIPLAYER,    /* hooks.c, netplay.c, vpeer.c, selectscreen.c */
    /* EX. GAME */
    FEAT_BOOTMENU,       /* bootmenu.c */
    FEAT_QUITKEY,        /* quitkey.c */
    FEAT_I_ISM_SELECT,   /* i_ism_select.c */
    /* EX. DISPLAY */
    FEAT_MODE_MENU_ICONS,/* padicons.c */
    FEAT_I_ISM_DISPLAY,  /* i_ism_display.c */
    FEAT_SCALED_ENDINGS, /* scaled_endings.c */
    FEAT_STAGE_EXT,      /* stagestrips.c */
    /* built in */
    FEAT_WIDESCREEN,     /* widescreen.c: WIDE NATIVE (rt_widescreen), chosen in SCREEN SETTING > SCREEN SIZE */
    FEAT_SHADERS,        /* platform: shader.c, chosen in SCREEN SETTING > FILTER */
    FEAT_CHEATS_UNLOCKS, /* cheats_unlocks.c, EX. SETTING's UNLOCKS, FIGHTERS and EXP pages (exsetting.c), sysdata.c */
    FEAT_ANTHOLOGY,      /* anthology.c: imports from Hyper Street Fighter Alpha (the Anthology disc); EX. ANTHOLOGY's ANTHOLOGY row */
    FEAT_COUNT,
    FEAT_ANY = FEAT_COUNT   /* on while any feature is: the plumbing several features share */
};

enum { WHERE_LAUNCHER, WHERE_EX_GAME, WHERE_EX_DISPLAY, WHERE_EX_ANTHOLOGY, WHERE_BUILT_IN, WHERE_COUNT };

typedef struct {
    const char *name;        /* --features, last_run.txt */
    const char *title;       /* the launcher's checkbox, the docs */
    const char *what;        /* its tooltip */
    int on_by_default;
    int needs;               /* a feature this one can't work without, or -1 */
    int where;               /* WHERE_*: where it is switched */
    const char *key;         /* a3mx.cfg's [ex setting] key (EX. GAME / EX. DISPLAY), or NULL */
} Feature;

static const Feature FEATURES[FEAT_COUNT] = {
    /* ---- the launcher ---- */
    { "saves", "Enhanced saves",
      "The project's own save system: your saves and settings in a3mx.dat / a3mx.cfg instead of a PSP\n"
      "memory stick, AUTO SAVE always on, and none of the PSP's save messages and questions.\n"
      "Off: the game saves as a PSP does (saves/ms0) and asks its own questions.", 1, -1, WHERE_LAUNCHER, 0 },
    { "menus", "Extra menus",
      "Every menu the project has added to or changed since the PSP game: OPTION MODE > EX. SETTING\n"
      "(with its cheats and unlocks, the MODE MENU shortcut and Boot to the MODE MENU), PLAYER CONFIG\n"
      "(P1 - P4, with L2 / R2), VSYNC, FRAME TIMING and FILTER in SCREEN SETTING, IMPORT / EXPORT of PSP\n"
      "saves, BUTTON CONFIG for each player from the pause menu. Needs Enhanced saves.", 1, FEAT_SAVES, WHERE_LAUNCHER, 0 },
    { "local_multiplayer", "Local multiplayer",
      "Two and three players on one PC: join-in on the select screens, the network modes played\n"
      "locally (EX. SETTING > VIRTUAL PSPS), Variable Battle tagging for two players.\n"
      "Off: one player, and NETWORK looks for other PSPs as the original does.", 1, -1, WHERE_LAUNCHER, 0 },
    /* ---- EX. GAME ---- */
    { "boot_to_menu", "Boot to the MODE MENU",
      "The game opens straight on the MODE MENU, with your settings and save loaded: the boot screens and\n"
      "the load run off screen at full speed, with no title, logos, story, attract demos or PRESS START.\n"
      "If the save can't be loaded, the game's own question is shown first. Takes effect at the next start.\n"
      "Off: the title and attract mode first, as on the PSP.", 0, FEAT_MENUS, WHERE_EX_GAME, "boot_to_mode_menu" },
    { "quit_key", "Mode Menu Shortcut",
      "F2 on the keyboard, or player 1 holding SELECT and pressing START, goes straight back to the\n"
      "MODE MENU without saving.", 1, FEAT_MENUS, WHERE_EX_GAME, "mode_menu_shortcut" },
    { "i_ism_select", "Shared Entry Pool",
      "Entry characters (I-ISM) can be picked for the other side too, where a mode allows it: in Free\n"
      "Battle, Survival, Training and the like, a second player, or player 1 choosing for the computer,\n"
      "can land on the entries' row.\n"
      "Off: only the side whose player is present can pick an entry, as on the PSP.", 1, -1, WHERE_EX_GAME, "shared_entry_pool" },
    /* ---- EX. DISPLAY ---- */
    { "mode_menu_icons", "MODE MENU icons",
      "The numbered bubble for each player's controller and the FRAME TIMING badge (SMOOTH / ORIGINAL)\n"
      "on the MODE MENU's bar.", 1, -1, WHERE_EX_DISPLAY, "mode_menu_icons" },
    { "i_ism_display", "Extra ISM Presentation",
      "The I-ISM's own wipe into the fight for entry characters (a yellow I, a gold logo), and the\n"
      "I-ISM score ranking in the attract mode. (The S-ISMs' wipes are EX. ANTHOLOGY's S-ISM\n"
      "PRESENTATION, whatever this is.)\n"
      "Off: an entry gets its ISM's wipe, as on the PSP, and no I-ISM ranking.", 0, -1, WHERE_EX_DISPLAY, "ism_presentation" },
    { "scaled_endings", "Scaled endings (WIDE NATIVE)",
      "The endings fill the 16:9 screen: a scene that has more of its picture shows it, the others are\n"
      "drawn 1.25 times larger, and the profile pictures move out to the screen's edges.\n"
      "Off: the endings keep black sides, their scenes the 4:3 picture. Needs Widescreen.", 1, FEAT_WIDESCREEN, WHERE_EX_DISPLAY, "scaled_endings" },
    { "stage_extensions", "Stage Extensions",
      "The project's own new art for the stage edges that WIDE NATIVE leaves black (assets/extensions/).\n"
      "Still placeholders: experimental. Needs Widescreen.", 0, FEAT_WIDESCREEN, WHERE_EX_DISPLAY, "stage_extensions" },
    /* ---- built in ---- */
    { "widescreen", "Widescreen (WIDE NATIVE)",
      "SCREEN SETTING > SCREEN SIZE gets a third value, WIDE NATIVE: 16:9 with a 4:3 play area\n"
      "(after Rickow's widescreen patch, now the project's own code). Chosen there.", 1, -1, WHERE_BUILT_IN, 0 },
    { "shaders", "Shaders (FILTER)",
      "SCREEN SETTING > FILTER: CRT, scanline and pixel-smoothing shaders (librashader). Chosen there;\n"
      "FILTER OFF shows the picture as it is drawn.", 1, -1, WHERE_BUILT_IN, 0 },
    { "cheats_unlocks", "Cheats and Unlocks",
      "Part of Extra menus: EX. SETTING's UNLOCKS, FIGHTERS and EXP pages (the unlocks one by one, never\n"
      "written into your save; each fighter's HEALTH, GUARD and SUPER; World Tour / EDIT MODE EXP).\n"
      "Not in network battles.", 1, FEAT_MENUS, WHERE_BUILT_IN, 0 },
    { "anthology", "Hyper Street Fighter Alpha imports",
      "Extra content from Hyper Street Fighter Alpha on your Street Fighter Alpha Anthology (PS2) disc,\n"
      "imported by the launcher when that disc is chosen on the Paths tab (Show optional images), and\n"
      "switched on and off on EX. SETTING's EX. ANTHOLOGY page (its rows greyed until it is imported).\n"
      "Experimental: off by default.", 0, -1, WHERE_EX_ANTHOLOGY, "anthology" },
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
const char *features_spec(void);                        /* the --features that starts the same way (--record) */
/* EX. GAME / EX. DISPLAY: each feature's setting as a3mx.cfg keeps it and the page shows it (0 / 1;
 * the launcher's and the built-in ones unused) */
extern int rt_feature_setting[FEAT_COUNT];
/* settings.c, once a3mx.cfg is read: the features that follow it take their settings (not one named in
 * --features, nor any without "settings" in it) */
void features_from_settings(void);
/* exsetting.c: F's row was changed. On at once (Boot to the MODE MENU: at the next start) */
void features_setting_changed(int f);
/* decomp.h's SFA_FUNC_WRAPPED and the generated hooks: the flag an extension's function goes by, from
 * its name (its file's name is the prefix: padicons_mode_menu) or its hook's address */
const unsigned char *feature_flag(const char *ext_name);
const unsigned char *feature_flag_hook(unsigned addr);
#ifdef __cplusplus
}
#endif
#endif
