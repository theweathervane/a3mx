/* Frame timing (perf.h). */
#include "perf.h"
#include "rt.h"
#include "version.h"
#include "host.h"
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

uint64_t perf_now_us(void) {
#ifdef _WIN32
    static LARGE_INTEGER f; LARGE_INTEGER t;
    if (!f.QuadPart) QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    return (uint64_t)(t.QuadPart / f.QuadPart) * 1000000u + (uint64_t)(t.QuadPart % f.QuadPart) * 1000000u / (uint64_t)f.QuadPart;
#else
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
#endif
}

#define BUDGET_US 16683u               /* one frame at 59.94 Hz */
#define HIST_STEP 250u                 /* busy-time histogram: 0.25 ms buckets up to 100 ms */
#define HIST_N 400

typedef struct {
    uint64_t frames, slow, busy, busy_max, part[PERF_N], game, gpu_ns, gpu_frames, uploads, hashed;
    uint64_t shown, hitches, gap_max;          /* frames that reached the screen; intervals over 1.5 frames; the longest */
    uint32_t hist[HIST_N + 1], gaps[HIST_N + 1];   /* busy times; intervals between frames on screen */
} Sums;
static Sums win, all;                  /* the last 600 frames, and the whole run */
static uint64_t part[PERF_N], frame_start, frame_uploads, frame_hashed, frame_gpu_ns, frame_no;
static uint64_t last_shown, frame_gap;      /* when the last frame reached the screen; this frame's interval (0: none) */
static int started;
static FILE *csv;
static char renderer[48] = "software", gpu_name[128] = "", scale_from[24] = "";
static int scale_used;

/* Hitches, each with what was going on: when, which frame, how long the frame before stayed on screen,
 * the game's screen (GAME + 0, + 2: the fight's), whether the display was being held (a load's frames
 * kept off screen) and how much was read from the disc since the frame before (a load). The
 * MAX_HITCHES longest go to last_run.txt as hitch_N lines, in time order, so a hitch in a fight can be
 * told from one at a load. */
uint64_t rt_io_read_bytes;              /* hle_io.c: bytes read from the disc so far */
#define MAX_HITCHES 64
typedef struct { double t; uint64_t frame; double ms; int screen, fight_screen, held; uint64_t read_kb; } Hitch;
static Hitch hitches[MAX_HITCHES];
static int n_hitches;
static uint64_t run_start, io_at_last_frame;
int rt_display_held(void);

void perf_add(int what, uint64_t us) { if (what >= 0 && what < PERF_N) part[what] += us; }
void perf_count_texture(int uploaded, uint32_t bytes_hashed) { frame_uploads += (uint64_t)uploaded; frame_hashed += bytes_hashed; }
void perf_gpu_time(uint64_t ns) { frame_gpu_ns = ns; }
void perf_shown(void) {
    uint64_t now = perf_now_us();
    if (last_shown) frame_gap = now - last_shown;
    last_shown = now;
}
void perf_set_renderer(const char *r, const char *g, int scale, const char *from) {
    snprintf(renderer, sizeof renderer, "%s", r ? r : "");
    snprintf(gpu_name, sizeof gpu_name, "%s", g ? g : "");
    snprintf(scale_from, sizeof scale_from, "%s", from ? from : "");
    scale_used = scale;
}
void perf_open_csv(const char *path) {
    csv = fopen(path, "w");
    if (!csv) { rt_warn("cannot write %s", path); return; }
    fprintf(csv, "frame,total_us,game_us,render_us,texture_us,present_us,idle_us,gpu_us,uploads,bytes_hashed,swap_us,shown_interval_us\n");
}

static void sums_add(Sums *s, uint64_t busy, uint64_t game) {
    s->frames++;
    s->busy += busy; if (busy > s->busy_max) s->busy_max = busy;
    if (busy > BUDGET_US) s->slow++;
    for (int i = 0; i < PERF_N; i++) s->part[i] += part[i];
    s->game += game;
    if (frame_gpu_ns) { s->gpu_ns += frame_gpu_ns; s->gpu_frames++; }
    s->uploads += frame_uploads; s->hashed += frame_hashed;
    uint64_t b = busy / HIST_STEP; s->hist[b > HIST_N ? HIST_N : b]++;
    if (frame_gap) {
        s->shown++;
        if (frame_gap > BUDGET_US * 3 / 2) {
            s->hitches++;
            int slot = -1;                       /* the next free one, else the shortest if this is longer */
            if (s == &all) {
                if (n_hitches < MAX_HITCHES) slot = n_hitches++;
                else {
                    int k = 0;
                    for (int i = 1; i < MAX_HITCHES; i++) if (hitches[i].ms < hitches[k].ms) k = i;
                    if (hitches[k].ms < (double)frame_gap / 1000.0) slot = k;
                }
            }
            if (slot >= 0) {
                Hitch *h = &hitches[slot];
                h->t = (double)(perf_now_us() - run_start) / 1e6; h->frame = frame_no; h->ms = (double)frame_gap / 1000.0;
                h->screen = *HOST(0x08E75910u); h->fight_screen = *HOST(0x08E75912u);   /* GAME + 0, + 2 (bytes) */
                h->held = rt_display_held();
                h->read_kb = (rt_io_read_bytes - io_at_last_frame) / 1024;
            }
        }
        if (frame_gap > s->gap_max) s->gap_max = frame_gap;
        uint64_t g = frame_gap / HIST_STEP; s->gaps[g > HIST_N ? HIST_N : g]++;
    }
}
static double hist_percentile_ms(const uint32_t *h, uint64_t count, double p) {
    uint64_t want = (uint64_t)((double)count * p), n = 0;
    for (int i = 0; i <= HIST_N; i++) { n += h[i]; if (n > want) return (double)(i + 1) * HIST_STEP / 1000.0; }
    return 0;
}
static double percentile_ms(const Sums *s, double p) { return hist_percentile_ms(s->hist, s->frames, p); }
static double avg_ms(uint64_t us, uint64_t n) { return n ? (double)us / (double)n / 1000.0 : 0; }

static void summary(const Sums *s, char *out, size_t n) {
    uint64_t f = s->frames ? s->frames : 1;
    int k = snprintf(out, n, "busy %.2f ms (game %.2f, render %.2f of which textures %.2f, present %.2f), max %.1f, p99 %.1f, %llu of %llu slow; "
                     "swap %.2f; on screen every p99 %.1f, max %.1f ms, %llu hitches",
                     avg_ms(s->busy, f), avg_ms(s->game, f), avg_ms(s->part[PERF_RENDER], f), avg_ms(s->part[PERF_TEXTURE], f),
                     avg_ms(s->part[PERF_PRESENT], f), (double)s->busy_max / 1000.0, percentile_ms(s, 0.99),
                     (unsigned long long)s->slow, (unsigned long long)s->frames, avg_ms(s->part[PERF_SWAP], f),
                     hist_percentile_ms(s->gaps, s->shown, 0.99), (double)s->gap_max / 1000.0, (unsigned long long)s->hitches);
    if (k > 0 && (size_t)k < n && s->gpu_frames)
        k += snprintf(out + k, n - (size_t)k, ", gpu %.2f ms", (double)s->gpu_ns / (double)s->gpu_frames / 1e6);
    if (k > 0 && (size_t)k < n)
        snprintf(out + k, n - (size_t)k, ", %.1f texture uploads and %.0f KB hashed a frame", (double)s->uploads / (double)f,
                 (double)s->hashed / (double)f / 1024.0);
}

void perf_frame(uint64_t f) {
    uint64_t now = perf_now_us();
    if (started) {
        uint64_t total = now - frame_start;
        uint64_t waits = part[PERF_IDLE] + part[PERF_SWAP];
        uint64_t idle = waits < total ? waits : total, busy = total - idle;   /* (idle here: sleeping and waiting for vsync) */
        uint64_t other = part[PERF_RENDER] + part[PERF_PRESENT];
        uint64_t game = busy > other ? busy - other : 0;
        if (csv) fprintf(csv, "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n", (unsigned long long)frame_no, (unsigned long long)total,
                         (unsigned long long)game, (unsigned long long)part[PERF_RENDER], (unsigned long long)part[PERF_TEXTURE],
                         (unsigned long long)part[PERF_PRESENT], (unsigned long long)part[PERF_IDLE], (unsigned long long)(frame_gpu_ns / 1000),
                         (unsigned long long)frame_uploads, (unsigned long long)frame_hashed, (unsigned long long)part[PERF_SWAP],
                         (unsigned long long)frame_gap);
        sums_add(&win, busy, game);
        sums_add(&all, busy, game);
        if (win.frames == 600) {
            char line[400]; summary(&win, line, sizeof line);
            rt_log("frame time (frames %llu-%llu): %s", (unsigned long long)(f - 600), (unsigned long long)(f - 1), line);
            memset(&win, 0, sizeof win);
        }
    }
    if (!run_start) run_start = now;
    if (started) io_at_last_frame = rt_io_read_bytes;    /* (after sums_add: a hitch sees this frame's reads) */
    started = 1; frame_start = now; frame_no = f;
    memset(part, 0, sizeof part);
    frame_uploads = frame_hashed = frame_gpu_ns = frame_gap = 0;
}

void perf_report(const char *work) {      /* (the folder last_run.txt goes to: the player's logs/, or work/ with --save-dir) */
    if (csv) { fclose(csv); csv = NULL; }
    char line[400]; summary(&all, line, sizeof line);
    rt_log("frame time (whole run, %s x%d%s%s): %s", renderer, scale_used, scale_from[0] ? ", " : "", scale_from, line);
    if (!work) return;
    char path[1200];
    snprintf(path, sizeof path, "%s/last_run.txt", work);
    FILE *o = fopen(path, "w");
    if (!o) return;
    uint64_t f = all.frames ? all.frames : 1;
    fprintf(o, "# the last run of the game: its renderer and frame times (the launcher's bug report reads this)\n");
    { const char *features_describe(void), *shader_describe(void); fprintf(o, "features=%s\nfilter=%s\n", features_describe(), shader_describe()); }
    fprintf(o, "version=%s\nrenderer=%s\ngpu=%s\nscale=%d\nscale_from=%s\nframes=%llu\n", A3MX_VERSION, renderer, gpu_name, scale_used,
            scale_from, (unsigned long long)all.frames);
    fprintf(o, "busy_ms=%.2f\ngame_ms=%.2f\nrender_ms=%.2f\ntexture_ms=%.2f\npresent_ms=%.2f\np99_ms=%.1f\nmax_ms=%.1f\nslow_frames=%llu\n",
            avg_ms(all.busy, f), avg_ms(all.game, f), avg_ms(all.part[PERF_RENDER], f), avg_ms(all.part[PERF_TEXTURE], f),
            avg_ms(all.part[PERF_PRESENT], f), percentile_ms(&all, 0.99), (double)all.busy_max / 1000.0, (unsigned long long)all.slow);
    if (all.gpu_frames) fprintf(o, "gpu_ms=%.2f\n", (double)all.gpu_ns / (double)all.gpu_frames / 1e6);
    extern int rt_vsync;
    double hz = host_refresh_hz();
    const char *pacing_describe(void); const char *pacing_setting_name(void); int pacing_given(void); double pacing_audio_ratio(void);
    fprintf(o, "frame_timing=%s\nframe_timing_given=%s\npacing=%s\naudio_ratio=%.4f\n", pacing_setting_name(), pacing_given() ? "SMOOTH" : "ORIGINAL", pacing_describe(), pacing_audio_ratio());
    fprintf(o, "vsync=%s\nrefresh_hz=%.2f\nswap_ms=%.2f\nshown_p99_ms=%.1f\nshown_max_ms=%.1f\nhitches=%llu\n", rt_vsync ? "ON" : "OFF", hz,
            avg_ms(all.part[PERF_SWAP], f), hist_percentile_ms(all.gaps, all.shown, 0.99), (double)all.gap_max / 1000.0,
            (unsigned long long)all.hitches);
    /* each hitch: seconds into the run, frame, ms on screen, the game's screen and fight screen,
     * whether the display was held (a load kept off screen), KB read from the disc since the frame before */
    fprintf(o, "# hitch_N=seconds,frame,ms_on_screen,screen,fight_screen,held,disc_kb (the %d longest, in time order)\n", MAX_HITCHES);
    for (int i = 1; i < n_hitches; i++)                 /* (slots fill out of order once full: sort by time) */
        for (int j = i; j > 0 && hitches[j - 1].t > hitches[j].t; j--) { Hitch x = hitches[j]; hitches[j] = hitches[j - 1]; hitches[j - 1] = x; }
    for (int i = 0; i < n_hitches; i++)
        fprintf(o, "hitch_%d=%.2f,%llu,%.1f,%d,%d,%d,%llu\n", i + 1, hitches[i].t, (unsigned long long)hitches[i].frame, hitches[i].ms,
                hitches[i].screen, hitches[i].fight_screen, hitches[i].held, (unsigned long long)hitches[i].read_kb);
    fclose(o);
}
