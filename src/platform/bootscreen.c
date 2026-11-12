/* The boot screens' title (0.60.0), around the game's own (0x0888BCC8, src/core/task/title.c):
 *
 *   - the version (A3MX_VERSION_SCREEN: "V0.60.0"), in the top right corner, over every boot screen
 *     (the system data load, PRESS START): always, extensions or not, so a screenshot or a report
 *     always says which build it was. It ends where the title does (x 360), in the game's small font
 *     (font 0, colour 9, as EX. SETTING's version);
 *   - with any extension on, A3MX in the large font in place of "STREET FIGHTER ALPHA3 / MAX", so the
 *     game as it was (--no-extensions, the launcher's "All off") is told apart at a glance.
 *
 * This is platform code, not an extension: its wrapper's flag is always on (ext_features.c,
 * "bootscreen_"). A build without src/extensions (-DA3MX_EXTENSIONS=OFF) names no extension at all,
 * so there the title is the game's and no version is drawn. */
#include "rt.h"
#include "version.h"
#include <string.h>
#include <stdio.h>

void f_0880A6CC(Cpu *c);                    /* text: a0 string, a1 font, a2 x, a3 y, t0 colour */
void f_0888BB24(Cpu *c);                    /* the copyright (a0 1) */
extern int rt_extensions;                   /* main.c: any extension on */

#define FONT_SMALL   0
#define FONT_LARGE   2
#define RED          9                      /* the small font's colour 9: as the version has always been drawn */
#define GREEN        4                      /* the title's */
#define TITLE_RIGHT  360                    /* the title's right edge: 0x18 + 21 letters of 16 */
#define VERSION_Y    6

static uint32_t ver, name;                  /* in guest memory, for the game's text routine */

/* the game's text routine, with every register as it was afterwards (this runs inside the game's code) */
static void text(Cpu *c, uint32_t s, int font, int x, int y, int colour) {
    uint32_t keep[32]; memcpy(keep, c->r, sizeof keep);
    c->r[4] = s; c->r[5] = (uint32_t)font; c->r[6] = (uint32_t)x; c->r[7] = (uint32_t)y; c->r[8] = (uint32_t)colour;
    f_0880A6CC(c);
    memcpy(c->r, keep, sizeof keep);
}
static int strings(void) {
    if (!ver) {
        ver = rt_alloc(64, 1, 16, "boot version");
        name = rt_alloc(16, 1, 16, "boot title");
        if (!ver || !name) return 0;
        snprintf((char *)HOST(ver), 64, "%s", A3MX_VERSION_SCREEN);
        snprintf((char *)HOST(name), 16, "A3MX");
    }
    return 1;
}
static void version(Cpu *c) {
    int len = (int)strlen((const char *)HOST(ver));
    text(c, ver, FONT_SMALL, TITLE_RIGHT - 8 * len, VERSION_Y, RED);   /* 8 units a letter, right-aligned */
}

int bootscreen_title(Cpu *c, uint32_t a0, int after) {
    (void)a0;
    if (!strings()) return 0;
    if (after) { version(c); return 0; }
    if (!rt_extensions) return 0;            /* the game's own name, then the version (after) */
    /* A3MX, 4 letters of 16 centred on 192, on the first line's row; then the copyright, as the game does */
    text(c, name, FONT_LARGE, 192 - 32, 0x20, GREEN);
    uint32_t keep[32]; memcpy(keep, c->r, sizeof keep);
    c->r[4] = 1; f_0888BB24(c);
    memcpy(c->r, keep, sizeof keep);
    version(c);                              /* (taking over skips the after call) */
    return 1;
}
