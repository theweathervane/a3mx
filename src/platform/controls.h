/* Bring your own device (0.60.0): which keyboard keys and which controller play each of the four
 * players, set outside the game (the launcher's Controllers tab) and read when the game starts.
 *
 *   configs/controllers.cfg in the player's folder (userdir.h):
 *
 *     [player1]
 *     type=auto                 the player's control type: auto, keyboard, controller, both, none
 *     pad=                      the controller: empty (none), or its SDL GUID, then ":" and which of
 *                               several identical ones (0 the first), then " " and its name (not used
 *                               to find it: for people reading the file)
 *     key.up=Up                 keyboard column: SDL scancode names (SDL_GetScancodeName); empty: unset
 *     pad.cross=b:south         controller column: b:<button> or a:<axis>+ / a:<axis>- (SDL's names:
 *                               SDL_GetGamepadStringForButton / ...ForAxis); empty: unset
 *
 *   Control types (each player's own):
 *     Automatic            players 2 to 4's default. Controllers that no player names go, in the order
 *                          they were plugged in, to the players waiting for one (lowest first), with
 *                          the usual layout; automatic player 1 also has the usual keys, unless another
 *                          player has keys. All four automatic is the arrangement the game always had:
 *                          the keyboard and the first controller are player 1, the next players 2 to 4.
 *     Keyboard             only the keys set here
 *     Controller           only the controller named here, with the buttons set here; with none named,
 *                          the first controller no player names (as Automatic), with these buttons
 *     Keyboard/Controller  both. Player 1's default (0.72.1, issue #15): the usual keys and the first
 *                          controller no player names, so the keyboard and a pad share player 1's slot
 *                          (no controller plugged in: just the keys)
 *     None                 nothing: the player can't play
 *   Player 1 on the keyboard, player 3 on a named controller, 2 and 4 automatic: the next controller
 *   plugged in is player 2, then player 4.
 *
 *   In the game a player is "plugged in" (its bubble on the MODE MENU; joining in as a human) while it
 *   can play: it has keys, or its controller is plugged in. In the launcher every type but None shows
 *   the player's bubble.
 *
 * With no file at all (the launcher's Controllers tab never changed) player 1 is Keyboard/Controller
 * with the usual keys and no controller named, players 2 to 4 automatic (controls_defaults; host_sdl.c). Started with --save-dir (the tests) no file is read and the arrangement the game always had is used
 * (every player automatic: the keyboard and the first controller are player 1). A file from before the control types (no type= lines) is read as
 * keyboard / controller / both from what it sets, and automatic for a player with nothing set.
 *
 * Shared by the game (C, host_sdl.c) and the launcher (C++): parsing and writing only, no SDL here.
 * Names are turned into scancodes and buttons by whoever includes it. */
#pragma once
#include <stdio.h>
#include <string.h>

#define CONTROLS_PLAYERS 4
enum { CB_UP, CB_DOWN, CB_LEFT, CB_RIGHT, CB_CROSS, CB_CIRCLE, CB_SQUARE, CB_TRIANGLE,
       CB_L, CB_R, CB_L2, CB_R2, CB_START, CB_SELECT, CB_COUNT };

/* the PSP's buttons in the order above: the file's name, the label, the bit in a pad word (host.h) */
static const struct { const char *id, *label; unsigned bit; } CONTROL_BUTTONS[CB_COUNT] = {
    { "up", "Up", 0x0010u }, { "down", "Down", 0x0040u }, { "left", "Left", 0x0080u }, { "right", "Right", 0x0020u },
    { "cross", "Cross", 0x4000u }, { "circle", "Circle", 0x2000u }, { "square", "Square", 0x8000u }, { "triangle", "Triangle", 0x1000u },
    { "l", "L", 0x0100u }, { "r", "R", 0x0200u }, { "l2", "L2", 0x40000000u }, { "r2", "R2", 0x80000000u },
    { "start", "Start", 0x0008u }, { "select", "Select", 0x0001u },
};
/* player 1's keys in a fresh file (the keys the game always had), and what a controller gets when it
 * is first given to a player (SDL's names, so they fit any pad SDL knows) */
static const char *const CONTROL_DEFAULT_KEYS[CB_COUNT] = { "Up", "Down", "Left", "Right", "Z", "X", "A", "S", "Q", "W", "E", "R", "Return", "Backspace" };
static const char *const CONTROL_DEFAULT_PAD[CB_COUNT] = { "b:dpup", "b:dpdown", "b:dpleft", "b:dpright", "b:a", "b:b", "b:x", "b:y",
                                                           "b:leftshoulder", "b:rightshoulder", "a:lefttrigger+", "a:righttrigger+", "b:start", "b:back" };

enum { CT_AUTO, CT_KEYBOARD, CT_CONTROLLER, CT_BOTH, CT_NONE, CT_COUNT };
static const char *const CONTROL_TYPE_IDS[CT_COUNT] = { "auto", "keyboard", "controller", "both", "none" };
static const char *const CONTROL_TYPE_LABELS[CT_COUNT] = { "Automatic", "Keyboard", "Controller", "Keyboard/Controller", "None" };

typedef struct {
    int type;                    /* CT_*: CT_AUTO unless the player chose another */
    char pad_guid[40];           /* "" none */
    int pad_index;               /* which of several controllers with that GUID */
    char pad_name[96];
    char key[CB_COUNT][32];      /* "" unset */
    char pad[CB_COUNT][32];      /* "" unset */
} ControlsPlayer;
typedef struct { ControlsPlayer p[CONTROLS_PLAYERS]; } Controls;

static inline int controls_uses_keys(const ControlsPlayer *p) { return p->type == CT_KEYBOARD || p->type == CT_BOTH; }
static inline int controls_uses_pad(const ControlsPlayer *p) { return p->type == CT_CONTROLLER || p->type == CT_BOTH; }
/* waits for a controller no player names: automatic, or a controller type with none named */
static inline int controls_takes_free_pad(const ControlsPlayer *p) { return p->type == CT_AUTO || (controls_uses_pad(p) && !p->pad_guid[0]); }
/* player 1 Keyboard/Controller with the usual keys and buttons and no controller named (it takes the
 * first one plugged in that no player names), players 2 to 4 automatic (CT_AUTO is 0) */
static inline void controls_defaults(Controls *c) {
    memset(c, 0, sizeof *c);
    c->p[0].type = CT_BOTH;
    for (int b = 0; b < CB_COUNT; b++) {
        snprintf(c->p[0].key[b], sizeof c->p[0].key[b], "%s", CONTROL_DEFAULT_KEYS[b]);
        snprintf(c->p[0].pad[b], sizeof c->p[0].pad[b], "%s", CONTROL_DEFAULT_PAD[b]);
    }
}
/* automatic player 1's keys: the usual ones, unless a player has keys of its own */
static inline int controls_auto_keys(const Controls *c, int k) {
    if (k != 0 || c->p[0].type != CT_AUTO) return 0;
    for (int j = 0; j < CONTROLS_PLAYERS; j++) if (controls_uses_keys(&c->p[j])) return 0;
    return 1;
}
static inline int controls_button_by_id(const char *id) {
    for (int b = 0; b < CB_COUNT; b++) if (!strcmp(CONTROL_BUTTONS[b].id, id)) return b;
    return -1;
}

/* 1 if the file was read (c is then what it says; anything it leaves out is unset), 0 if there is none */
static inline int controls_load(Controls *c, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    memset(c, 0, sizeof *c);
    char line[512];
    int player = -1, typed[CONTROLS_PLAYERS] = { 0 };
    while (fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' ')) line[--n] = 0;
        if (!line[0] || line[0] == '#') continue;
        if (line[0] == '[') { int k = 0; player = sscanf(line, "[player%d]", &k) == 1 && k >= 1 && k <= CONTROLS_PLAYERS ? k - 1 : -1; continue; }
        char *eq = strchr(line, '=');
        if (!eq || player < 0) continue;
        *eq = 0;
        const char *key = line, *val = eq + 1;
        ControlsPlayer *p = &c->p[player];
        if (!strcmp(key, "type")) {
            for (int t = 0; t < CT_COUNT; t++) if (!strcmp(val, CONTROL_TYPE_IDS[t])) { p->type = t; typed[player] = 1; }
        } else if (!strcmp(key, "pad")) {
            /* GUID:index name */
            const char *sp = strchr(val, ' ');
            char id[64]; snprintf(id, sizeof id, "%.*s", sp ? (int)(sp - val) : (int)strlen(val), val);
            char *colon = strchr(id, ':');
            p->pad_index = 0;
            if (colon) { *colon = 0; sscanf(colon + 1, "%d", &p->pad_index); }
            snprintf(p->pad_guid, sizeof p->pad_guid, "%s", id);
            snprintf(p->pad_name, sizeof p->pad_name, "%s", sp ? sp + 1 : "");
        } else if (!strncmp(key, "key.", 4)) {
            int b = controls_button_by_id(key + 4);
            if (b >= 0) snprintf(p->key[b], sizeof p->key[b], "%s", val);
        } else if (!strncmp(key, "pad.", 4)) {
            int b = controls_button_by_id(key + 4);
            if (b >= 0) snprintf(p->pad[b], sizeof p->pad[b], "%s", val);
        }
    }
    fclose(f);
    for (int k = 0; k < CONTROLS_PLAYERS; k++) {        /* a file from before the types: what it sets says which */
        if (typed[k]) continue;
        ControlsPlayer *p = &c->p[k];
        int keys = 0;
        for (int b = 0; b < CB_COUNT; b++) if (p->key[b][0]) keys = 1;
        p->type = keys && p->pad_guid[0] ? CT_BOTH : keys ? CT_KEYBOARD : p->pad_guid[0] ? CT_CONTROLLER : CT_AUTO;
    }
    return 1;
}

static inline int controls_save(const Controls *c, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "# a3mx: which keys and which controller play each player (the launcher's Controllers tab).\n"
               "# Read when the game starts. key.* are key names, pad.* are b:<button> or a:<axis>+ / a:<axis>-;\n"
               "# empty means not set. type: auto, keyboard, controller, both or none. Every player auto is the\n"
               "# arrangement the game always had (keyboard and first controller: player 1; the next: players 2 to 4).\n");
    for (int k = 0; k < CONTROLS_PLAYERS; k++) {
        const ControlsPlayer *p = &c->p[k];
        fprintf(f, "\n[player%d]\ntype=%s\n", k + 1, CONTROL_TYPE_IDS[p->type >= 0 && p->type < CT_COUNT ? p->type : CT_AUTO]);
        if (p->pad_guid[0]) fprintf(f, "pad=%s:%d %s\n", p->pad_guid, p->pad_index, p->pad_name);
        else fprintf(f, "pad=\n");
        for (int b = 0; b < CB_COUNT; b++) fprintf(f, "key.%s=%s\n", CONTROL_BUTTONS[b].id, p->key[b]);
        for (int b = 0; b < CB_COUNT; b++) fprintf(f, "pad.%s=%s\n", CONTROL_BUTTONS[b].id, p->pad[b]);
    }
    return fclose(f) == 0;
}
