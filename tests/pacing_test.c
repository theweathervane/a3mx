/* FRAME TIMING's rules (src/platform/pacing.c), against a simulated monitor: no disc, no window.
 *
 *   - SMOOTH and ORIGINAL are always what the player gets, whatever the monitor measures at;
 *   - AUTO chooses only at a decision point (the MODE MENU opening, SCREEN SETTING closing), is
 *     ORIGINAL before the first one, and keeps its choice between them.
 *
 * The case that failed before 0.60.0: SMOOTH on a monitor that reports 60.00 Hz and really runs at
 * about 59.97 Hz, half way between SMOOTH and ORIGINAL, where the MODE MENU's badge (pacing_given)
 * followed the measured rate and showed each in turn. */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

int rt_vsync = 1, rt_frame_timing = 2;
extern int rt_frame_timing_cli;
void pacing_frame(int presented, int frame);
void pacing_decide(const char *where);
void pacing_window_changed(void);
int pacing_given(void);
const char *pacing_describe(void);

/* ---- the host, simulated ---- */
static uint64_t now_us = 1000000;
static double reported_hz = 60.0;            /* what the system says the monitor runs at */
static int vsync_can = 1, verbose;
uint64_t perf_now_us(void) { return now_us; }
double host_refresh_hz(void) { return reported_hz; }
int host_vsync_available(void) { return vsync_can; }
void host_set_swap_interval(int n) { (void)n; }
void host_audio_ratio(double r) { (void)r; }
int host_compositor_wait(int on) { (void)on; return 0; }
void rt_pace_anchor(int64_t us_ago) { (void)us_ago; }
void rt_log(const char *fmt, ...) { if (!verbose) return; va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); putchar('\n'); }
void rt_logl(int level, const char *fmt, ...) { (void)level; if (!verbose) return; va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); putchar('\n'); }

static int frame_no, failures;
/* n frames shown, each `us` after the last (with a little jitter either way, as real swaps have) */
static int run(int n, double us, int *flips) {
    int first = -1, last = -1;
    static uint32_t seed = 12345;
    for (int i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        double jitter = ((int)(seed >> 16 & 0xFF) - 128) * 0.4;      /* +- 51 us */
        now_us += (uint64_t)(us + jitter);
        pacing_frame(1, frame_no++);
        int g = pacing_given();
        if (first < 0) first = g;
        if (last >= 0 && g != last && flips) (*flips)++;
        last = g;
    }
    return last;
}
static void expect(int got, int want, const char *what) {
    if (got == want) { printf("ok    %s\n", what); return; }
    printf("FAIL  %s: got %s, want %s (%s)\n", what, got ? "SMOOTH" : "ORIGINAL", want ? "SMOOTH" : "ORIGINAL", pacing_describe());
    failures++;
}
static void expect_flips(int flips, const char *what) {
    if (!flips) { printf("ok    %s\n", what); return; }
    printf("FAIL  %s: the speed changed %d times\n", what, flips); failures++;
}

int main(int argc, char **argv) {
    verbose = argc > 1 && !strcmp(argv[1], "-v");
    static const double real_hz[] = { 60.00, 59.97, 59.94, 59.95, 59.985 };   /* what the monitor really runs at */
    int flips;

    /* explicit SMOOTH / ORIGINAL: always that, on every monitor, with and without VSYNC */
    for (int setting = 0; setting <= 1; setting++)
        for (size_t k = 0; k < sizeof real_hz / sizeof *real_hz; k++)
            for (int vs = 1; vs >= 0; vs--) {
                char what[160];
                rt_frame_timing = setting; rt_vsync = vs; reported_hz = 60.0; pacing_window_changed();
                flips = 0;
                int g = run(1500, 1e6 / real_hz[k], &flips);
                snprintf(what, sizeof what, "%s stays %s: monitor says 60.00, runs at %.3f Hz, VSYNC %s", setting ? "SMOOTH" : "ORIGINAL",
                         setting ? "SMOOTH" : "ORIGINAL", real_hz[k], vs ? "on" : "off");
                expect(g, setting, what); expect_flips(flips, what);
            }
    /* ... and on monitors where the pace is a timer's (144 Hz) or the clock's (120 Hz), or the swap doesn't wait (VRR) */
    static const double others[][2] = { { 144.0, 144.0 }, { 120.0, 120.0 }, { 119.88, 119.88 }, { 60.0, 500.0 } };
    for (int setting = 0; setting <= 1; setting++)
        for (size_t k = 0; k < sizeof others / sizeof *others; k++) {
            char what[160];
            rt_frame_timing = setting; rt_vsync = 1; reported_hz = others[k][0]; pacing_window_changed();
            flips = 0;
            int g = run(1500, 1e6 / others[k][1] * (others[k][1] > 100 && others[k][1] < 200 ? 2 : 1), &flips);
            snprintf(what, sizeof what, "%s stays %s: monitor says %.2f, frames every %.2f ms", setting ? "SMOOTH" : "ORIGINAL",
                     setting ? "SMOOTH" : "ORIGINAL", others[k][0], 1000.0 / others[k][1]);
            expect(g, setting, what); expect_flips(flips, what);
        }

    /* AUTO: ORIGINAL until the first decision point, then its choice, kept */
    rt_frame_timing = 2; rt_vsync = 1; reported_hz = 60.0; pacing_window_changed();
    flips = 0;
    expect(run(600, 1e6 / 59.97, &flips), 0, "AUTO is ORIGINAL before the first MODE MENU");
    expect_flips(flips, "AUTO doesn't choose before the first MODE MENU");
    pacing_decide("the MODE MENU");
    flips = 0;
    expect(run(1, 1e6 / 59.97, NULL), 1, "AUTO chooses SMOOTH on the MODE MENU (a 60.00 Hz monitor, VSYNC on)");
    expect(run(3000, 1e6 / 59.97, &flips), 1, "AUTO keeps SMOOTH on a monitor measuring 59.97 Hz");
    expect_flips(flips, "AUTO doesn't change between decision points");
    /* the monitor changes while playing (another display): nothing until the next decision point */
    reported_hz = 144.0; pacing_window_changed(); flips = 0;
    expect(run(600, 1e6 / 60.0, &flips), 1, "AUTO keeps its choice when the monitor changes mid-game");
    expect_flips(flips, "AUTO doesn't change when the monitor changes mid-game");
    pacing_decide("the MODE MENU");
    expect(run(2, 1e6 / 60.0, NULL), 0, "AUTO chooses ORIGINAL at the next MODE MENU (a 144 Hz monitor)");
    /* the same monitor again: the same choice at every later decision point */
    flips = 0;
    for (int i = 0; i < 5; i++) { pacing_decide("the MODE MENU"); run(300, 1e6 / 60.0, &flips); }
    expect_flips(flips, "AUTO keeps its choice across MODE MENU visits while nothing has changed");
    /* VSYNC off in SCREEN SETTING: applies on leaving it */
    reported_hz = 60.0; pacing_window_changed(); pacing_decide("the MODE MENU"); run(300, 1e6 / 60.0, NULL);
    expect(pacing_given(), 1, "AUTO is SMOOTH again on a 60.00 Hz monitor");
    rt_vsync = 0; flips = 0;
    expect(run(300, 1e6 / 60.0, &flips), 1, "AUTO keeps its choice while SCREEN SETTING is open (VSYNC switched off)");
    pacing_decide("leaving SCREEN SETTING");
    expect(run(2, 1e6 / 60.0, NULL), 0, "AUTO chooses ORIGINAL on leaving SCREEN SETTING with VSYNC off");
    /* explicit choices made in SCREEN SETTING apply at once and hold */
    rt_frame_timing = 1; flips = 0;
    run(1, 1e6 / 60.0, NULL);
    expect(run(600, 1e6 / 59.97, &flips), 1, "SMOOTH chosen in SCREEN SETTING is SMOOTH (VSYNC off)");
    expect_flips(flips, "SMOOTH chosen in SCREEN SETTING holds");

    printf(failures ? "\n%d FAILED\n" : "\nall passed\n", failures);
    return failures ? 1 : 0;
}
