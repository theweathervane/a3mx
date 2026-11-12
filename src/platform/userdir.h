/* Where the player's own files live (0.60.0): the system's per-user application folder, not the
 * checkout or the download.
 *
 *   Windows  %APPDATA%\a3mx                          (C:\Users\<you>\AppData\Roaming\a3mx)
 *   macOS    ~/Library/Application Support/a3mx
 *   Linux    $XDG_DATA_HOME/a3mx, else ~/.local/share/a3mx
 *
 *   saves/     a3mx.dat and its .bak, ms0/ (the memory stick of --no-extensions), import/, export/
 *   configs/   a3mx.cfg and its .bak
 *   logs/      a3mx.log and last_run.txt of the last run, launcher.log (builds and downloads)
 *   launcher/  launcher.ini
 *
 * If that folder can't be made or written (a locked-down account, a read-only profile), the same four
 * folders are used beside the game instead (the folder that holds work/ and imports/), and the log
 * says so. A3MX_USER_DIR, or --user-dir DIR, names another folder (a portable copy on a stick, tests).
 *
 * Shared by the game (C) and the launcher (C++): everything here is static, with no SDL. */
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define USERDIR_MKDIR(p) _mkdir(p)
#else
#define USERDIR_MKDIR(p) mkdir(p, 0755)
#endif

#define USERDIR_APP "a3mx"
static const char *const userdir_subs[] = { "saves", "configs", "logs", "launcher" };

/* the system's folder for this user, or 0 if the environment doesn't say */
static int userdir_system(char *out, size_t n) {
    const char *e = getenv("A3MX_USER_DIR");
    if (e && *e) { snprintf(out, n, "%s", e); return 1; }
#if defined(_WIN32)
    e = getenv("APPDATA");
    if (!e || !*e) return 0;
    snprintf(out, n, "%s/" USERDIR_APP, e);
#elif defined(__APPLE__)
    e = getenv("HOME");
    if (!e || !*e) return 0;
    snprintf(out, n, "%s/Library/Application Support/" USERDIR_APP, e);
#else
    e = getenv("XDG_DATA_HOME");
    if (e && *e == '/') snprintf(out, n, "%s/" USERDIR_APP, e);
    else {
        e = getenv("HOME");
        if (!e || !*e) return 0;
        snprintf(out, n, "%s/.local/share/" USERDIR_APP, e);
    }
#endif
    for (char *p = out; *p; p++) if (*p == '\\') *p = '/';
    return 1;
}

/* make DIR and every folder above it that is missing */
static void userdir_make(const char *dir) {
    char tmp[1200]; snprintf(tmp, sizeof tmp, "%s", dir);
    for (char *p = tmp + 1; *p; p++)
        if (*p == '/' || *p == '\\') { char keep = *p; *p = 0; if (p[-1] != ':') USERDIR_MKDIR(tmp); *p = keep; }
    USERDIR_MKDIR(tmp);
}

/* ROOT with its four folders, each proved writable by writing a file into it */
static int userdir_usable(const char *root) {
    for (size_t i = 0; i < sizeof userdir_subs / sizeof *userdir_subs; i++) {
        char p[1300];
        snprintf(p, sizeof p, "%s/%s", root, userdir_subs[i]);
        userdir_make(p);
        snprintf(p, sizeof p, "%s/%s/.write_test", root, userdir_subs[i]);
        FILE *f = fopen(p, "w");
        if (!f) return 0;
        int ok = fputs("a3mx", f) >= 0;
        if (fclose(f) != 0) ok = 0;
        remove(p);
        if (!ok) return 0;
    }
    return 1;
}

/* The folder to use: WANTED if given (--user-dir), else the system's; BESIDE (the folder holding work/)
 * if that can't be written. Returns 1 for the first, 2 for the fallback, 0 if neither can be written
 * (out is then BESIDE all the same, so the paths stay sensible and the failures are reported where they
 * happen). */
static int userdir_choose(char *out, size_t n, const char *wanted, const char *beside) {
    char sys[1100] = "";
    if (wanted && *wanted) snprintf(sys, sizeof sys, "%s", wanted);
    else userdir_system(sys, sizeof sys);
    size_t k = strlen(sys);
    while (k > 1 && (sys[k - 1] == '/' || sys[k - 1] == '\\')) sys[--k] = 0;
    if (sys[0] && userdir_usable(sys)) { snprintf(out, n, "%s", sys); return 1; }
    snprintf(out, n, "%s", beside && *beside ? beside : ".");
    k = strlen(out);
    while (k > 1 && (out[k - 1] == '/' || out[k - 1] == '\\')) out[--k] = 0;
    return userdir_usable(out) ? 2 : 0;
}
