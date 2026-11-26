/* The extensions as features (ext_features.h): which are on in this run, and which flag each hook and
 * wrapper goes by. */
#include "ext_features.h"
#include <stdio.h>
#include <string.h>
void rt_log(const char *fmt, ...);
extern int rt_extensions;                    /* main.c: any feature on (0: the PSP program alone) */

unsigned char rt_feature_on[FEAT_COUNT + 1];
int rt_feature_setting[FEAT_COUNT];
static unsigned char named[FEAT_COUNT];      /* named in --features: stays so for this run, whatever a3mx.cfg says */
static int follow_settings;                  /* "settings" was in the list */
static char described[256] = "none", spec[320] = "none";
static int in_game(int f) { return FEATURES[f].where == WHERE_EX_GAME || FEATURES[f].where == WHERE_EX_DISPLAY; }

static void finish(void) {
    /* a feature whose need is off goes off with it (twice: a need can itself have just gone) */
    for (int pass = 0; pass < 2; pass++)
        for (int f = 0; f < FEAT_COUNT; f++)
            if (rt_feature_on[f] && FEATURES[f].needs >= 0 && !rt_feature_on[FEATURES[f].needs]) {
                rt_feature_on[f] = 0;
                rt_log("features: %s is off, as it needs %s", FEATURES[f].name, FEATURES[FEATURES[f].needs].name);
            }
#ifdef A3MX_NO_EXTENSIONS
    memset(rt_feature_on, 0, sizeof rt_feature_on);          /* a build without src/extensions */
#endif
    int any = 0; size_t n = 0;
    described[0] = 0;
    for (int f = 0; f < FEAT_COUNT; f++)
        if (rt_feature_on[f]) {
            n += (size_t)snprintf(described + n, sizeof described - n, "%s%s", any ? "," : "", FEATURES[f].name);
            any = 1;
        }
    if (!any) snprintf(described, sizeof described, "none");
    /* the same run again (--record): what isn't left to a3mx.cfg by name, and "settings" (a replay has
       this run's a3mx.cfg) */
    n = 0; spec[0] = 0;
    for (int f = 0; f < FEAT_COUNT && any; f++) {
        if (follow_settings && in_game(f) && !named[f]) continue;          /* a3mx.cfg's to say */
        /* an off one is written out only where "settings" would otherwise turn it on */
        if (!rt_feature_on[f] && !(follow_settings && FEATURES[f].where != WHERE_LAUNCHER)) continue;
        n += (size_t)snprintf(spec + n, sizeof spec - n, "%s%s%s", n ? "," : "", rt_feature_on[f] ? "" : "-", FEATURES[f].name);
    }
    if (any && follow_settings) n += (size_t)snprintf(spec + n, sizeof spec - n, "%ssettings", n ? "," : "");
    if (!any) snprintf(spec, sizeof spec, "none");
    rt_feature_on[FEAT_ANY] = (unsigned char)any;
    rt_extensions = any;
}

static int by_name(const char *s, size_t n) {
    for (int f = 0; f < FEAT_COUNT; f++) if (strlen(FEATURES[f].name) == n && !strncmp(FEATURES[f].name, s, n)) return f;
    return -1;
}

int features_set(const char *list, const char **bad) {
    static char bad_name[40];
    memset(rt_feature_on, 0, sizeof rt_feature_on);
    memset(named, 0, sizeof named);
    follow_settings = 0;
    for (int f = 0; f < FEAT_COUNT; f++) rt_feature_setting[f] = FEATURES[f].on_by_default;   /* until a3mx.cfg is read */
    if (!list) list = "default,settings";
    for (const char *p = list; *p;) {
        const char *e = strchr(p, ',');
        size_t n = e ? (size_t)(e - p) : strlen(p);
        int off = n && *p == '-';
        const char *s = p + off; size_t sn = n - (size_t)off;
        if (sn == 3 && !strncmp(s, "all", 3)) { for (int f = 0; f < FEAT_COUNT; f++) rt_feature_on[f] = (unsigned char)!off; }
        else if (sn == 4 && !strncmp(s, "none", 4)) memset(rt_feature_on, 0, sizeof rt_feature_on);
        else if (sn == 7 && !strncmp(s, "default", 7)) { for (int f = 0; f < FEAT_COUNT; f++) rt_feature_on[f] = (unsigned char)FEATURES[f].on_by_default; }
        else if (sn == 8 && !strncmp(s, "settings", 8)) {         /* the rest at their defaults, then as a3mx.cfg has them */
            follow_settings = !off;
            for (int f = 0; f < FEAT_COUNT; f++) if (FEATURES[f].where != WHERE_LAUNCHER && !named[f]) rt_feature_on[f] = (unsigned char)(!off && FEATURES[f].on_by_default);
        }
        else if (sn) {
            int f = by_name(s, sn);
            if (f < 0) { snprintf(bad_name, sizeof bad_name, "%.*s", (int)(sn < 39 ? sn : 39), s); if (bad) *bad = bad_name; return 0; }
            rt_feature_on[f] = (unsigned char)!off;
            named[f] = 1;
        }
        p = e ? e + 1 : p + n;
    }
    finish();
    return 1;
}
void features_none(void) { memset(rt_feature_on, 0, sizeof rt_feature_on); follow_settings = 0; finish(); }

void features_from_settings(void) {
    if (!follow_settings || !rt_feature_on[FEAT_ANY]) return;
    for (int f = 0; f < FEAT_COUNT; f++)
        if (in_game(f) && !named[f]) rt_feature_on[f] = (unsigned char)!!rt_feature_setting[f];
    finish();
    rt_log("features: with EX. SETTING's (a3mx.cfg): %s", described);
}
void features_setting_changed(int f) {
    if (f < 0 || f >= FEAT_COUNT || !in_game(f) || !rt_feature_on[FEAT_ANY]) return;
    if (f == FEAT_BOOTMENU) return;                          /* it acts at boot only: the next start */
    rt_feature_on[f] = (unsigned char)!!rt_feature_setting[f];
    finish();
    rt_log("features: %s %s from EX. SETTING", FEATURES[f].name, rt_feature_on[f] ? "on" : "off");
}
void features_add(int f) { if (f >= 0 && f < FEAT_COUNT && rt_feature_on[FEAT_ANY]) { rt_feature_on[f] = 1; named[f] = 1; finish(); } }
const char *features_describe(void) { return described; }
const char *features_spec(void) { return spec; }

/* An extension's function is named after its file: exsetting_option_menu is exsetting.c's. */
static const struct { const char *prefix; int feature; } BY_PREFIX[] = {
    { "bootmsg_", FEAT_SAVES }, { "sysdata_", FEAT_SAVES },
    { "exsetting_", FEAT_MENUS }, { "screensetting_", FEAT_MENUS },
    { "hooks_", FEAT_MULTIPLAYER }, { "selectscreen_", FEAT_MULTIPLAYER },
    { "padicons_mode_menu", FEAT_ANY },      /* also tells pacing.c the MODE MENU has opened; its drawing goes by FEAT_MODE_MENU_ICONS */
    { "padicons_", FEAT_MODE_MENU_ICONS },
    { "quitkey_task_wait", FEAT_ANY },      /* F2 (quit_key) and the FILTER preview (shaders): it asks each */
    { "pausefilter_", FEAT_MENUS },
    { "quitkey_", FEAT_QUITKEY },
    { "bootmenu_", FEAT_BOOTMENU },
    { "widescreen_", FEAT_WIDESCREEN },
    { "stagestrips_", FEAT_STAGE_EXT },
    { "i_ism_display_", FEAT_I_ISM_DISPLAY },
    { "i_ism_select_", FEAT_I_ISM_SELECT },
    { "cheats_unlocks_", FEAT_CHEATS_UNLOCKS },
    { "scaled_endings_", FEAT_SCALED_ENDINGS },
};
static const unsigned char always_on = 1;     /* platform code wrapping the game's functions: bootscreen.c */
const unsigned char *feature_flag(const char *ext_name) {
    if (!strncmp(ext_name, "bootscreen_", 11)) return &always_on;
    for (size_t i = 0; i < sizeof BY_PREFIX / sizeof *BY_PREFIX; i++)
        if (!strncmp(ext_name, BY_PREFIX[i].prefix, strlen(BY_PREFIX[i].prefix))) return &rt_feature_on[BY_PREFIX[i].feature];
    rt_log("features: %s belongs to no feature (src/platform/ext_features.c): on with any", ext_name);
    return &rt_feature_on[FEAT_ANY];
}

/* tools/data/hooks.txt, by address */
static const struct { unsigned addr; int feature; } BY_HOOK[] = {
    { 0x0880509C, FEAT_MULTIPLAYER }, { 0x088DD5D0, FEAT_MULTIPLAYER }, { 0x0883465C, FEAT_MULTIPLAYER },
    { 0x0882E380, FEAT_MULTIPLAYER }, { 0x08833DDC, FEAT_MULTIPLAYER }, { 0x088A8404, FEAT_MULTIPLAYER },
    { 0x08818BA4, FEAT_SAVES }, { 0x08817C10, FEAT_SAVES }, { 0x088335B8, FEAT_SAVES },
    { 0x08888CC0, FEAT_MENUS }, { 0x08864DFC, FEAT_MENUS }, { 0x08867794, FEAT_MENUS },
    { 0x0882AE20, FEAT_CHEATS_UNLOCKS }, { 0x0883B800, FEAT_CHEATS_UNLOCKS },
    /* the two text passes draw for three features (JOIN / WAIT, the MODE MENU bar, the pause menu's
       pictures): selectscreen.c's hooks ask each */
    { 0x0880B098, FEAT_ANY }, { 0x08812520, FEAT_ANY },
};
const unsigned char *feature_flag_hook(unsigned addr) {
    for (size_t i = 0; i < sizeof BY_HOOK / sizeof *BY_HOOK; i++) if (BY_HOOK[i].addr == addr) return &rt_feature_on[BY_HOOK[i].feature];
    rt_log("features: the hook at %08X belongs to no feature (src/platform/ext_features.c): on with any", addr);
    return &rt_feature_on[FEAT_ANY];
}
