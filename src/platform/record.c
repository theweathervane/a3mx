/* Recording a session and playing it back (--record FILE, --replay FILE; the launcher's Record Output
 * and Play Output).
 *
 * The game runs in virtual time (kernel.c): it moves on frame by frame, and real time only paces it.
 * So a session is exactly what the game was given, frame by frame, and how it started:
 *   - each pad's buttons as host_poll gave them (the analogue sticks are already directions there),
 *     which pads were plugged in (the extensions ask, to decide who is human), and F2;
 *   - copies of a3mx.cfg and a3mx.dat as they were at the start (options, unlocks, button configs);
 *   - the features, --p2 / --p3 and FRAME TIMING in use;
 *   - the date: the game asks for it (sceKernelLibcTime, gettimeofday), so recording and replay both
 *     give the recorded start plus the virtual time since (rt_time_base).
 * Played back from those, the game follows the same path. To show that it does, the recording keeps
 * a hash of the program's memory every CHECK_EVERY frames, and the replay compares its own: the first
 * frame that differs is where the replay and the recording parted (or the bug being chased).
 *
 * The file is text, one item per line:
 *   a3mx-recording 1
 *   version v0.95.0 / disc <boot SHA-1> / region us / features ... / assets on|off
 *   p2 N / p3 N (-1 auto, 0 cpu, 1 human) / frame_timing N (-1: the setting) / time <seconds>
 *   file a3mx.cfg <hex> (or -: there was none) / file a3mx.dat <hex>
 *   pads F B0 B1 B2 B3     the buttons from frame F on (hex), written when they change
 *   plugged F MASK         the pads plugged in from frame F on (bit k: pad k + 1)
 *   menu F                 F2 at frame F
 *   check F HASH           the program's memory at frame F
 *   end F                  the last frame
 */
#include "rt.h"
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir_one(p) _mkdir(p)
#else
#include <sys/stat.h>
#define mkdir_one(p) mkdir(p, 0755)
#endif

#define CHECK_EVERY 60
#define FORMAT 1

int64_t rt_time_base;                /* hle_sys.c: the date at frame 0 while recording or replaying (0: the clock) */
int rt_pads_replay = -1;             /* host_*.c: the pads plugged in, from the replay (-1: ask the host) */

/* ---------------------------------------------------------------- recording */
static FILE *rec;
static uint32_t rec_pads[4];
static unsigned rec_plugged = ~0u;
static int rec_started;

static void put_file(FILE *o, const char *name, const char *path) {
    FILE *f = path ? fopen(path, "rb") : NULL;
    fprintf(o, "file %s ", name);
    if (!f) { fputs("-\n", o); return; }
    int ch, n = 0;
    while ((ch = fgetc(f)) != EOF) { fprintf(o, "%02x", ch); n++; }
    if (n == 0) fputc('-', o);
    fputc('\n', o);
    fclose(f);
}

int record_open(const char *path, const char *version, const char *disc, const char *region, const char *features,
                int assets, const int human[2], int frame_timing, const char *cfg_path, const char *dat_path) {
    rec = fopen(path, "w");
    if (!rec) { rt_warn("record: can't write %s (%s): this session is not recorded", path, strerror(errno)); return 0; }
    rt_time_base = (int64_t)time(NULL);
    fprintf(rec, "# a3mx: a recorded session. Play it back with a3mx --replay FILE, or the launcher's Play Output.\n");
    fprintf(rec, "a3mx-recording %d\nversion %s\ndisc %s\nregion %s\nfeatures %s\nassets %s\n", FORMAT, version,
            disc[0] ? disc : "-", region[0] ? region : "-", features, assets ? "on" : "off");
    fprintf(rec, "p2 %d\np3 %d\nframe_timing %d\ntime %lld\n", human[0], human[1], frame_timing, (long long)rt_time_base);
    put_file(rec, "a3mx.cfg", cfg_path);
    put_file(rec, "a3mx.dat", dat_path);
    fflush(rec);
    rt_log("record: recording this session to %s", path);
    return 1;
}
int record_active(void) { return rec != NULL; }

void record_frame(uint64_t f, const uint32_t pads[4], unsigned plugged, int menu) {
    if (!rec) return;
    if (!rec_started || memcmp(pads, rec_pads, sizeof rec_pads)) {
        fprintf(rec, "pads %llu %x %x %x %x\n", (unsigned long long)f, pads[0], pads[1], pads[2], pads[3]);
        memcpy(rec_pads, pads, sizeof rec_pads);
    }
    if (!rec_started || plugged != rec_plugged) { fprintf(rec, "plugged %llu %x\n", (unsigned long long)f, plugged); rec_plugged = plugged; }
    if (menu) fprintf(rec, "menu %llu\n", (unsigned long long)f);
    rec_started = 1;
}
void record_check(uint64_t f, uint64_t state) {
    if (!rec || f == 0 || f % CHECK_EVERY) return;
    fprintf(rec, "check %llu %016llx\n", (unsigned long long)f, (unsigned long long)state);
    fflush(rec);                     /* a crash keeps what came before */
}
void record_close(uint64_t frames) {
    if (!rec) return;
    fprintf(rec, "end %llu\n", (unsigned long long)frames);
    fclose(rec); rec = NULL;
    rt_log("record: %llu frames recorded", (unsigned long long)frames);
}

/* ---------------------------------------------------------------- replay */
typedef struct { uint64_t f; uint32_t v[4]; } Ev;
static Ev *ev_pads, *ev_plugged, *ev_checks;     /* checks: v[0] / v[1] the hash's low / high half */
static uint64_t *ev_menu;
static int n_pads, n_plugged, n_checks, n_menu, cap_pads, cap_plugged, cap_checks, cap_menu;
static int i_pads, i_plugged, i_checks, i_menu;
static uint64_t replay_end;
static int replaying, checks_matched, checks_seen;
static uint64_t first_bad = 0, last_good = 0;
static char hdr_version[64], hdr_disc[64], hdr_region[16], hdr_features[256];
static int hdr_assets = 1, hdr_human[2] = { -1, -1 }, hdr_timing = -1;
static char *file_hex[2];            /* a3mx.cfg, a3mx.dat */

static void push(Ev **a, int *n, int *cap, Ev e) {
    if (*n == *cap) { *cap = *cap ? *cap * 2 : 1024; *a = realloc(*a, (size_t)*cap * sizeof **a); if (!*a) rt_fatal("replay: out of memory"); }
    (*a)[(*n)++] = e;
}

/* Reads a recording; 0 (and why, in err) if it isn't one this version can play */
int replay_load(const char *path, char *err, size_t errn) {
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(err, errn, "can't open %s (%s)", path, strerror(errno)); return 0; }
    fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
    char *text = malloc((size_t)size + 1);
    if (!text || fread(text, 1, (size_t)size, f) != (size_t)size) { fclose(f); free(text); snprintf(err, errn, "can't read %s", path); return 0; }
    fclose(f); text[size] = 0;
    int format = 0;
    for (char *line = text, *next; line && *line; line = next) {
        next = strchr(line, '\n');
        if (next) *next++ = 0;
        size_t l = strlen(line); if (l && line[l - 1] == '\r') line[--l] = 0;
        if (!l || line[0] == '#') continue;
        char key[24] = { 0 }; int k = 0;
        while (line[k] && line[k] != ' ' && k < 23) { key[k] = line[k]; k++; }
        const char *v = line[k] ? line + k + 1 : "";
        unsigned long long fr; unsigned a, b, c, d;
        if (!strcmp(key, "a3mx-recording")) format = atoi(v);
        else if (!strcmp(key, "version")) snprintf(hdr_version, sizeof hdr_version, "%s", v);
        else if (!strcmp(key, "disc")) snprintf(hdr_disc, sizeof hdr_disc, "%s", v);
        else if (!strcmp(key, "region")) snprintf(hdr_region, sizeof hdr_region, "%s", v);
        else if (!strcmp(key, "features")) snprintf(hdr_features, sizeof hdr_features, "%s", v);
        else if (!strcmp(key, "assets")) hdr_assets = strcmp(v, "off") != 0;
        else if (!strcmp(key, "p2")) hdr_human[0] = atoi(v);
        else if (!strcmp(key, "p3")) hdr_human[1] = atoi(v);
        else if (!strcmp(key, "frame_timing")) hdr_timing = atoi(v);
        else if (!strcmp(key, "time")) rt_time_base = atoll(v);
        else if (!strcmp(key, "file")) {
            int which = !strncmp(v, "a3mx.cfg ", 9) ? 0 : !strncmp(v, "a3mx.dat ", 9) ? 1 : -1;
            if (which >= 0 && strcmp(v + 9, "-")) { free(file_hex[which]); file_hex[which] = strdup(v + 9); }
        }
        else if (!strcmp(key, "pads") && sscanf(v, "%llu %x %x %x %x", &fr, &a, &b, &c, &d) == 5) {
            Ev e = { fr, { a, b, c, d } }; push(&ev_pads, &n_pads, &cap_pads, e);
        }
        else if (!strcmp(key, "plugged") && sscanf(v, "%llu %x", &fr, &a) == 2) {
            Ev e = { fr, { a, 0, 0, 0 } }; push(&ev_plugged, &n_plugged, &cap_plugged, e);
        }
        else if (!strcmp(key, "menu") && sscanf(v, "%llu", &fr) == 1) {
            if (n_menu == cap_menu) { cap_menu = cap_menu ? cap_menu * 2 : 64; ev_menu = realloc(ev_menu, (size_t)cap_menu * sizeof *ev_menu); if (!ev_menu) rt_fatal("replay: out of memory"); }
            ev_menu[n_menu++] = fr;
        }
        else if (!strcmp(key, "check")) {
            unsigned long long h;
            if (sscanf(v, "%llu %llx", &fr, &h) == 2) { Ev e = { fr, { (uint32_t)h, (uint32_t)(h >> 32), 0, 0 } }; push(&ev_checks, &n_checks, &cap_checks, e); }
        }
        else if (!strcmp(key, "end")) replay_end = strtoull(v, NULL, 10);
    }
    free(text);
    if (format != FORMAT) { snprintf(err, errn, "%s is not an a3mx recording (or one from a later version)", path); return 0; }
    if (!replay_end) {
        /* no end line: the recorded game stopped without closing (a crash); play what there is */
        uint64_t last = 0;
        if (n_pads) last = ev_pads[n_pads - 1].f;
        if (n_checks && ev_checks[n_checks - 1].f > last) last = ev_checks[n_checks - 1].f;
        replay_end = last + 1;
    }
    /* the pads plugged in from the start: the extensions can ask before the first frame's input */
    rt_pads_replay = n_plugged ? (int)ev_plugged[0].v[0] : 1;
    replaying = 1;
    return 1;
}
int replay_active(void) { return replaying; }
uint64_t replay_frames(void) { return replay_end; }
const char *replay_version(void) { return hdr_version; }
const char *replay_disc(void) { return hdr_disc; }
const char *replay_region(void) { return hdr_region; }
const char *replay_features(void) { return hdr_features; }
int replay_assets(void) { return hdr_assets; }
int replay_human(int k) { return hdr_human[k]; }
int replay_frame_timing(void) { return hdr_timing; }

static int unhex(int ch) { return ch <= '9' ? ch - '0' : (ch | 0x20) - 'a' + 10; }
/* The recorded a3mx.cfg and a3mx.dat, into dir (made if needed); a file the recording had none of is
   not written, and one already there is removed, so the game starts as the recording did */
int replay_write_files(const char *dir) {
    mkdir_one(dir);
    static const char *const names[2] = { "a3mx.cfg", "a3mx.dat" };
    for (int i = 0; i < 2; i++) {
        char p[1200]; snprintf(p, sizeof p, "%s/%s", dir, names[i]);
        if (!file_hex[i]) { remove(p); continue; }
        FILE *o = fopen(p, "wb");
        if (!o) { rt_warn("replay: can't write %s", p); return 0; }
        for (const char *h = file_hex[i]; h[0] && h[1]; h += 2) fputc(unhex(h[0]) << 4 | unhex(h[1]), o);
        fclose(o);
    }
    return 1;
}

/* This frame's input, as recorded: the pads' buttons, the pads plugged in (rt_pads_replay), F2 */
void replay_frame(uint64_t f, uint32_t pads[4], int *menu) {
    static uint32_t cur[4];
    while (i_pads < n_pads && ev_pads[i_pads].f <= f) { memcpy(cur, ev_pads[i_pads].v, sizeof cur); i_pads++; }
    while (i_plugged < n_plugged && ev_plugged[i_plugged].f <= f) { rt_pads_replay = (int)ev_plugged[i_plugged].v[0]; i_plugged++; }
    if (rt_pads_replay < 0) rt_pads_replay = 1;
    memcpy(pads, cur, sizeof cur);
    *menu = 0;
    while (i_menu < n_menu && ev_menu[i_menu] <= f) { if (ev_menu[i_menu] == f) *menu = 1; i_menu++; }
}

void replay_check(uint64_t f, uint64_t state) {
    if (!replaying || f == 0 || f % CHECK_EVERY) return;
    while (i_checks < n_checks && ev_checks[i_checks].f < f) i_checks++;
    if (i_checks >= n_checks || ev_checks[i_checks].f != f) return;
    uint64_t want = (uint64_t)ev_checks[i_checks].v[1] << 32 | ev_checks[i_checks].v[0];
    checks_seen++;
    if (want == state) { checks_matched++; if (!first_bad) last_good = f; return; }
    if (!first_bad) {
        first_bad = f;
        rt_warn("replay: the game no longer matches the recording at frame %llu (it still did at frame %llu): the replay has "
                "gone its own way from somewhere in between", (unsigned long long)f, (unsigned long long)last_good);
    }
}
void replay_report(void) {
    if (!replaying) return;
    if (!checks_seen) rt_log("replay: no checkpoints reached");
    else if (!first_bad) rt_log("replay: matched the recording at all %d checkpoints", checks_seen);
    else rt_warn("replay: matched the recording up to frame %llu, differed from frame %llu (%d of %d checkpoints matched)",
                 (unsigned long long)last_good, (unsigned long long)first_bad, checks_matched, checks_seen);
}
