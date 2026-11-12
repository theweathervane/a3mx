/* Frame timing: when each frame reaches the screen (SCREEN SETTING > FRAME TIMING: AUTO, SMOOTH, ORIGINAL).
 *
 * The game always counts one PSP vblank (1/59.94 s of its own time) per frame; nothing here changes
 * that, so the game plays the same frame for frame. What changes is how the PC's wall clock lines up
 * with it:
 *
 *   ORIGINAL  the kernel waits until exactly 1/59.94 s after the last frame (kernel.c,
 *             host_wait_until). On a 60 Hz monitor 0.06 refreshes a second are left over, so a frame
 *             is shown twice about every 16.7 s. VRR, or a 59.94 Hz mode, shows it perfectly.
 *   SMOOTH    60.00 Hz (0.1 % faster than the PSP), so a 60 / 120 / 240 Hz monitor shows each frame
 *             for the same number of refreshes and none longer than another. Sound is played that much
 *             faster too (host_audio_ratio, measured from the frames shown) so it keeps up without skips.
 *
 * Where it can, the monitor sets the pace. With VSYNC on and a monitor at about a multiple of 60 Hz
 * (60, 120, 180 ... 480; within 0.6 %):
 *   - at one refresh a frame (60 Hz) it is "swap-paced": the kernel squeezes each frame to end a little
 *     early (pacing_time_scale) and the swap then waits for the refresh, so the game runs exactly as fast
 *     as the monitor refreshes;
 *   - at two or more (120 Hz ...) it is "clocked" (0.50.1): a timer at exactly that many of the
 *     monitor's refreshes (its reported rate), swap interval 1, so the swap puts each frame on the next
 *     refresh; nothing is measured from the swap. A swap interval above 1 is never used: on a 120 Hz
 *     NVIDIA laptop in full screen it made frames miss their refresh, from the QSound logo for up to
 *     13 s (0.46.1 - 0.50.0).
 * Otherwise a timer does: at 59.94 Hz for ORIGINAL, at 60.00 Hz for SMOOTH
 * (anchored to a 60.00 Hz grid each frame, rt_pace_anchor).
 *   - SMOOTH is swap-paced only on a monitor at a multiple of 60.00 (not 59.94), else the 60.00 timer.
 *   - ORIGINAL is swap-paced only on a monitor at a multiple of 59.94, else the 59.94 timer.
 *   - AUTO, the default, chooses one of the two, and from then on is exactly that one: SMOOTH where the
 *     monitor can set the pace at a multiple of 60.00 Hz, ORIGINAL anywhere else (VSYNC off, 75 / 144 /
 *     165 Hz, a 59.94 Hz mode, an unknown rate, a swap that doesn't wait: most often VRR, where 59.94 is
 *     shown perfectly).
 *
 * The speed never changes on its own (0.60.0). An explicit SMOOTH or ORIGINAL is always that speed:
 * how the frames are timed may fall back (the monitor, the clock, a timer), never which of 60.00 and
 * 59.94 Hz they are timed at. AUTO chooses only at a decision point (pacing_decide): the first MODE MENU,
 * and again on coming back to the MODE MENU or leaving SCREEN SETTING, where it keeps its choice unless
 * what it chose from has changed (VSYNC, the monitor's rate, the swap found not to wait). Until the
 * first MODE MENU, AUTO is ORIGINAL's timer. Before, SMOOTH's badge followed the measured rate of the
 * monitor, and a 60 Hz monitor measuring about 59.97 Hz showed SMOOTH and ORIGINAL in turn every few
 * seconds.
 * What the player gets (pacing_given: SMOOTH or ORIGINAL, as set or as AUTO chose) is shown on the MODE
 * MENU (padicons.c).
 *
 * Held frames (nothing presented) are timed as ORIGINAL. A swap that turns out not to wait for the
 * monitor (frames coming faster than it refreshes) is first given the desktop compositor's wait
 * (Windows: DwmFlush, host_sdl.c); if frames still come too fast, SMOOTH is off until the window
 * changes between full screen and a window.
 *
 * Nothing is measured for the first SETTLE frames after the start, or after the window enters or leaves
 * full screen or moves to another display (host_sdl.c: SDL's window events, not only F11 / Alt+Enter):
 * Windows takes a moment to hand a new full-screen window from the desktop compositor to the screen,
 * and swaps that come back early meanwhile used to lock in the wrong pacing for the whole run (fixed
 * only by Alt+Enter twice). */
#include "rt.h"
#include "host.h"
#include "perf.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

extern int rt_vsync, rt_frame_timing;
int rt_frame_timing_cli = -1;                /* main.c: --frame-timing original|smooth|auto (-1: the setting) */
void rt_pace_anchor(int64_t us_ago);          /* kernel.c: the frame just shown went out us_ago: the next is timed from then */

#define PSP_HZ        (60000.0 / 1001.0)
#define PSP_FRAME_US  (1001000.0 / 60.0)
#define MARGIN_MIN    2000.0                 /* how long before the refresh the frame is ready (drawing it */
#define MARGIN_MAX    8000.0                 /* into the window and the swap): grows after a missed refresh */
#define TOLERANCE     0.006                  /* how far from 59.94 Hz (or a multiple) the monitor may be */
#define WINDOW        120                    /* frames between checks of the monitor's real rate */
#define MAX_INTERVAL  8                      /* refreshes per frame at most: 480 Hz */
#define SETTLE        120                    /* frames shown before anything is measured (2 s at 60 Hz) */

static int swap_paced, interval = 1, broken, last_presented, checked_at = -1000, clean, compositor;
static int bad_windows;                      /* checks in a row in which too many frames missed their refresh */
static const char *broken_why = "the swap doesn't wait for the monitor";
#define MISSED_BAD    (WINDOW / 20)          /* missed refreshes in a check that make it a bad one (more than 6 of 120) */
static int one_refresh;                      /* the swap waits for the next refresh only, not `interval` of them */
static int settle = SETTLE;                  /* frames still to show before measuring (start, window changes) */
int host_compositor_wait(int on);           /* host_sdl.c: wait on the desktop compositor after each swap (Windows); 1 if it can */
static double monitor_hz, frame_us_monitor, ratio = 1.0, margin = 4000.0;
static uint64_t last_out_us;                 /* when the last frame went out: the refresh it caught (not when the swap returned, if late) */
static uint64_t last_back_us;                /* when the last swap came back: intervals are measured between these */
static double samples[WINDOW]; static int sum_n, missed_n, fast_n;   /* one-refresh intervals: their median is the monitor's real period */
static char why[128] = "timer, 59.94 Hz";
static int timer60;                          /* the timer runs at 60.00 Hz (SMOOTH where it can't be swap-paced) */
static double grid_us;                       /* timer60 / clocked: when the next frame is due on the grid (fractions kept: no drift) */
static int clocked;                          /* 0.50.1: a monitor at 2 or more refreshes a frame (120 Hz ...): a timer at exactly
                                                that many of its refreshes, swap interval 1, nothing measured from the swap */
static double clock_us, clock_hz;            /* clocked: the frame's period, and the monitor rate it was worked out from */
static int cur_mode = -1;                    /* what was chosen last: 0 a timer, 1 swap-paced, 2 clocked (-1: choose again) */
#define SMOOTH_US     (1e6 / 60.0)
#define SMOOTH_RATIO  (PSP_FRAME_US / SMOOTH_US)

/* FRAME TIMING: 0 ORIGINAL, 1 SMOOTH, 2 AUTO */
static int timing_wanted(void) { int v = rt_frame_timing_cli >= 0 ? rt_frame_timing_cli : rt_frame_timing; return v >= 0 && v <= 2 ? v : 2; }
static int auto_family = -1;                 /* what AUTO chose: 1 SMOOTH, 0 ORIGINAL (-1: not yet, ORIGINAL's timer meanwhile) */
static int auto_no_swap;                     /* AUTO's SMOOTH found the swap not waiting: ORIGINAL for the rest of the run */
static int decide_pending; static const char *decide_where = "";
/* the speed frames are timed at: 1 SMOOTH (60.00 Hz), 0 ORIGINAL (59.94 Hz), -1 AUTO before its first choice */
static int family(void) { int w = timing_wanted(); return w == 2 ? auto_family : w; }
/* padicons.c (the MODE MENU opening), screensetting.c (SCREEN SETTING closing): AUTO may choose now */
void pacing_decide(const char *where) { decide_pending = 1; decide_where = where ? where : ""; }
static const char *timing_name(int v) { return v == 0 ? "ORIGINAL" : v == 1 ? "SMOOTH" : "AUTO"; }

/* ", each frame shown four times": for the log and last_run.txt */
static const char *shown_times(int n) {
    static const char *const words[] = { "", "", "twice", "three times", "four times", "five times", "six times", "seven times", "eight times" };
    static char s[40];
    if (n < 2) return "";
    if (n <= MAX_INTERVAL) snprintf(s, sizeof s, ", each frame shown %s", words[n]); else snprintf(s, sizeof s, ", each frame shown %d times", n);
    return s;
}

/* How early the frame may be ready at most: 0.45 of a frame (as at 60 and 120 Hz). When the swap
   waits only for the next refresh (a driver that doesn't honour a swap interval above 1, or the
   desktop compositor's wait), less than a refresh (0.9 of one), so a frame ready that early still goes
   out at the n-th refresh rather than sooner: 3.75 ms at 240 Hz. */
static double margin_cap(void) {
    double refresh = frame_us_monitor > 0 ? frame_us_monitor : PSP_FRAME_US, cap = fmin(MARGIN_MAX, refresh * interval * 0.45);
    if (one_refresh || compositor) cap = fmin(cap, refresh * 0.9);
    return fmax(MARGIN_MIN, cap);
}

/* Which pacing to use: 0 a timer (timer60 says which), else the swap interval (refreshes per frame) */
static int timer_because(int at60, const char *reason) {
    timer60 = at60;
    snprintf(why, sizeof why, "timer, %s Hz (%s)", at60 ? "60.00" : "59.94", reason);
    return 0;
}
static int monitor_interval(void) {
    int want = family(), at60 = want == 1;
    timer60 = 0;
    if (want < 0) return timer_because(0, "AUTO chooses on the MODE MENU");
    if (!rt_vsync) return timer_because(at60, "VSYNC is off");
    if (!host_vsync_available()) return timer_because(at60, "the display can't wait for the monitor");
    if (broken) return timer_because(at60, broken_why);
    if (monitor_hz <= 0) return timer_because(at60, "the monitor's rate is unknown");
    int n = (int)floor(monitor_hz / PSP_HZ + 0.5);
    if (n < 1 || n > MAX_INTERVAL || fabs(monitor_hz / n - PSP_HZ) / PSP_HZ > TOLERANCE) {
        char r[80]; snprintf(r, sizeof r, "a %.2f Hz monitor isn't a multiple of 60 Hz", monitor_hz);
        return timer_because(at60, r);
    }
    int ntsc = monitor_hz / n < (PSP_HZ + 60.0) / 2;   /* a multiple of 59.94 rather than of 60.00 */
    if (want == 1 && ntsc) { char r[80]; snprintf(r, sizeof r, "SMOOTH on a %.2f Hz monitor, a multiple of 59.94", monitor_hz); return timer_because(1, r); }
    if (want == 0 && !ntsc) { char r[80]; snprintf(r, sizeof r, "ORIGINAL on a %.2f Hz monitor", monitor_hz); return timer_because(0, r); }
    if (n >= 2) { snprintf(why, sizeof why, "the monitor's rate, %.2f Hz%s, timed by the clock", monitor_hz, shown_times(n)); return n; }
    snprintf(why, sizeof why, "the monitor, %.2f Hz%s", monitor_hz, compositor ? ", through the desktop compositor" : "");
    return n;
}

/* AUTO's choice: SMOOTH only where the monitor can set the pace at a multiple of 60.00 Hz */
static int auto_choose(void) {
    if (auto_no_swap || !rt_vsync || !host_vsync_available() || monitor_hz <= 0) return 0;
    int n = (int)floor(monitor_hz / PSP_HZ + 0.5);
    if (n < 1 || n > MAX_INTERVAL || fabs(monitor_hz / n - PSP_HZ) / PSP_HZ > TOLERANCE) return 0;
    return monitor_hz / n >= (PSP_HZ + 60.0) / 2;
}
static int cmp_d(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
static void set_ratio(double r) {
    if (r < 0.99) r = 0.99;
    if (r > 1.01) r = 1.01;
    if (fabs(r - ratio) < 0.0002) return;
    ratio = r;
    host_audio_ratio(r);
}
static void stop_smooth(void) { swap_paced = 0; clocked = 0; host_set_swap_interval(1); set_ratio(timer60 ? SMOOTH_RATIO : 1.0); last_out_us = 0; grid_us = 0; }

/* once a frame, after it was shown (presented) or not (held, headless) */
void pacing_frame(int presented, int frame) {
    uint64_t now = perf_now_us();
    static int seen_vsync = -1, seen_timing = -1, seen_timer60 = -1;
    if (rt_vsync != seen_vsync || timing_wanted() != seen_timing) {   /* changed in SCREEN SETTING: decide again now */
        seen_vsync = rt_vsync; seen_timing = timing_wanted();
        if (swap_paced) { swap_paced = 0; set_ratio(1.0); last_out_us = 0; }   /* (host_set_vsync has just set the swap interval) */
        clocked = 0; cur_mode = -1; grid_us = 0;
        checked_at = -1000;
    }
    if (presented && decide_pending) {             /* a decision point: AUTO chooses, or keeps what it chose */
        decide_pending = 0;
        monitor_hz = host_refresh_hz();
        if (timing_wanted() == 2) {
            int f = auto_choose();
            if (f != auto_family) {
                auto_family = f; cur_mode = -1; checked_at = -1000;
                rt_log("pacing: AUTO is %s (%s, a %.2f Hz monitor, VSYNC %s%s)", f ? "SMOOTH" : "ORIGINAL", decide_where, monitor_hz,
                       rt_vsync ? "on" : "off", auto_no_swap ? ", the swap doesn't wait" : "");
            }
        }
    }
    if (presented && frame - checked_at >= 60) {   /* the monitor or the settings can change at any time */
        checked_at = frame;
        monitor_hz = host_refresh_hz();
        int n = monitor_interval();
        /* 0.50.1: only a monitor at one refresh a frame (60 Hz) is swap-paced. At 2 or more (120, 180,
           240 Hz ...) a swap interval above 1 made frames miss their refresh on a 120 Hz NVIDIA laptop in
           full screen (0.46.1 - 0.50.0: hitches and broken-up sound from the QSound logo for up to 13 s,
           until 0.48.2's fallback gave up), and swaps that don't block (a driver queueing frames) can't be
           measured. So the clock times each frame at exactly n of the monitor's refreshes, as ORIGINAL's
           timer does at 59.94 (clean on that laptop), and the swap (interval 1) puts it on the next one. */
        int mode = n == 1 ? 1 : n >= 2 ? 2 : 0;
        if (mode != cur_mode || (mode == 2 && (n != interval || fabs(monitor_hz - clock_hz) > 0.005)) || (mode == 0 && timer60 != seen_timer60)) {
            seen_timer60 = mode == 0 ? timer60 : -1;
            cur_mode = mode;
            swap_paced = mode == 1; clocked = mode == 2; grid_us = 0;
            if (mode == 1) {
                interval = 1; frame_us_monitor = 1e6 / monitor_hz; one_refresh = 0;
                margin = fmin(margin, margin_cap());
                host_set_swap_interval(1);
                set_ratio(PSP_FRAME_US / frame_us_monitor);
            } else if (mode == 2) {
                interval = n; clock_hz = monitor_hz; clock_us = n * 1e6 / monitor_hz;
                host_set_swap_interval(1); host_compositor_wait(0); compositor = 0;
                set_ratio(PSP_FRAME_US / clock_us);
            } else stop_smooth();
            sum_n = missed_n = fast_n = 0; last_out_us = 0;
            rt_log("pacing: %s", why);
        }
    }
    if (!presented) { last_presented = 0; last_out_us = 0; grid_us = 0; return; }   /* (a held frame: not a missed refresh) */
    last_presented = 1;
    if (!swap_paced) {
        if (timer60 || clocked) {            /* SMOOTH's timer, or clocked: each frame due a period after the last, on a grid */
            double t = (double)now;
            if (grid_us <= 0 || t > grid_us + 50000 || grid_us > t + 50000) grid_us = t;   /* (started, or far off: start again) */
            rt_pace_anchor((int64_t)(t - grid_us));
            grid_us += clocked ? clock_us : SMOOTH_US;
        }
        return;
    }
    if (settle > 0) { settle--; last_out_us = 0; return; }   /* the window is still settling: measure nothing */
    double refresh = frame_us_monitor, period = refresh * interval, reported = 1e6 / monitor_hz * interval;
    /* when this frame went out: the swap came back at the refresh, unless it came back late (a missed
       refresh, or a load that wasn't shown): then it went out at a refresh on the grid of the last one,
       and the next frame is timed from that, so it catches up and the game doesn't fall behind. With
       several refreshes per frame a late frame goes out at the next refresh, not the next frame's:
       a quarter of a frame late, but no more than half a refresh, is still on time (1 and 2 as before) */
    uint64_t out = now;
    if (last_out_us) {
        /* measured between swaps coming back, not from where a late frame was put back on the grid: a swap
           that always comes back a little after the refresh would otherwise make every frame look late */
        double d = (double)(now - last_back_us);
        if (d < reported * 0.9) fast_n++;                                    /* sooner than the monitor refreshes */
        else if (d < period + fmin(period * 0.25, refresh * 0.5)) { if (sum_n < WINDOW) samples[sum_n] = d; sum_n++; }   /* one frame: measures the real rate */
        else if (d < period * 8) {                                           /* missed refresh(es): back onto the grid */
            int k = (int)floor((double)(now - last_out_us) / refresh + 0.25);
            out = last_out_us + (uint64_t)(refresh * k);
            if (out > now) out = now;
            missed_n++;
            margin = fmin(margin_cap(), margin + 500.0); clean = 0;
        }
    }
    if (++clean >= 600 && margin > MARGIN_MIN) { margin = fmax(MARGIN_MIN, margin - 250.0); clean = 0; }
    last_out_us = out; last_back_us = now;
    rt_pace_anchor((int64_t)(now - out));
    if (sum_n + missed_n + fast_n >= WINDOW) {
        if (fast_n > WINDOW / 4 && interval > 1 && !one_refresh && margin > frame_us_monitor * 0.9) {
            /* frames ready early went out early: the swap waits for the next refresh, not `interval` of
               them. Ready less than a refresh early, the next refresh is the right one */
            one_refresh = 1; margin = margin_cap();
            snprintf(why, sizeof why, "the monitor, %.2f Hz%s, the swap waiting one refresh", monitor_hz, shown_times(interval));
            rt_log("pacing: the swap doesn't wait for %d refreshes of the %.2f Hz monitor: each frame is now ready %.1f ms before its refresh", interval, monitor_hz, margin / 1000.0);
            sum_n = missed_n = fast_n = 0; last_out_us = 0;
            return;
        }
        if (fast_n > WINDOW / 4 && !compositor && host_compositor_wait(1)) {   /* the swap doesn't wait: try the compositor's wait */
            compositor = 1; margin = fmin(margin, margin_cap());
            rt_log("pacing: the swap doesn't wait for the %.2f Hz monitor: waiting for the desktop compositor (DwmFlush) after it instead", monitor_hz);
            snprintf(why, sizeof why, "the monitor, %.2f Hz, through the desktop compositor%s", monitor_hz, shown_times(interval));
            sum_n = missed_n = fast_n = 0; last_out_us = 0;
            return;
        }
        if (fast_n > WINDOW / 4) {                                           /* frames came faster than the monitor refreshes */
            broken = 1; broken_why = "the swap doesn't wait for the monitor: VRR, or the driver"; host_compositor_wait(0);
            if (timing_wanted() == 2) auto_no_swap = 1;   /* AUTO is ORIGINAL from the next decision point (not before) */
            monitor_interval();               /* a timer now: 59.94 Hz, or 60.00 for SMOOTH */
            stop_smooth(); seen_timer60 = timer60;
            rt_warn("pacing: frames went out faster than a %.2f Hz monitor refreshes: the swap doesn't wait for it (VRR, or the driver), so %s until the window changes", monitor_hz, why);
            return;
        }
        if (sum_n >= WINDOW / 2) {                                           /* the monitor's real rate (not quite what it reports) */
            int n = sum_n < WINDOW ? sum_n : WINDOW;
            qsort(samples, (size_t)n, sizeof *samples, cmp_d);
            double med = samples[n / 2];                                     /* (a swap that came back late doesn't move it) */
            if (fabs(med - reported) < reported * 0.01) {                    /* (anything further off isn't the monitor) */
                frame_us_monitor = med / interval;
                set_ratio(PSP_FRAME_US / med);
            }
        }
        if (missed_n > WINDOW / 20) rt_log("pacing: %d of the last %d frames missed a refresh (now ready %.1f ms before it)", missed_n, WINDOW, margin / 1000.0);
        /* Frames that keep missing their refresh make the game run slow and the sound break up (0.48.2:
           seen in full screen on NVIDIA, now and then, from the QSound logo on). One cause: in full screen
           the driver may count a swap interval of 2 from the swap rather than from the last frame shown,
           so every frame waits a refresh too many. First: interval 1, each frame ready in the last refresh
           before its own (as for a driver that ignores the interval). Still missing: the timer, until the
           window changes. Two bad checks in a row each time, so a load or two doesn't count. */
        bad_windows = missed_n > MISSED_BAD ? bad_windows + 1 : 0;
        if (bad_windows >= 2) {
            bad_windows = 0;
            if (interval > 1 && !one_refresh) {
                one_refresh = 1; host_set_swap_interval(1); margin = margin_cap();
                snprintf(why, sizeof why, "the monitor, %.2f Hz%s, the swap waiting one refresh", monitor_hz, shown_times(interval));
                rt_log("pacing: frames kept missing the %.2f Hz monitor's refresh: the swap now waits one refresh, each frame ready %.1f ms before it", monitor_hz, margin / 1000.0);
            } else {
                broken = 1; broken_why = "frames kept missing the monitor's refresh"; host_compositor_wait(0); compositor = 0;
                monitor_interval();           /* a timer now: 59.94 Hz, or 60.00 for SMOOTH */
                stop_smooth(); seen_timer60 = timer60;
                rt_warn("pacing: %d of the last %d frames missed the %.2f Hz monitor's refresh: %s until the window changes (full screen / a window)", missed_n, WINDOW, monitor_hz, why);
            }
            sum_n = missed_n = fast_n = 0; last_out_us = 0;
            return;
        }
        sum_n = missed_n = fast_n = 0;
    }
}

/* host_sdl.c: full screen <-> a window: the swap may wait now where it didn't (or not), so find out again */
void pacing_window_changed(void) {
    broken = 0; compositor = 0; one_refresh = 0; bad_windows = 0; host_compositor_wait(0);
    if (swap_paced || clocked) stop_smooth();
    cur_mode = -1;
    checked_at = -1000; sum_n = missed_n = fast_n = 0;
    settle = SETTLE; margin = 4000.0; clean = 0;
}

/* kernel.c: how fast the wall clock runs against the game's until the next vblank, so the frame is
   ready `margin` before the monitor's refresh and the swap waits for it (1: as the game's, ORIGINAL) */
double pacing_time_scale(void) {
    if (!last_presented) return 1.0;
    if (!swap_paced) return clocked ? clock_us / PSP_FRAME_US : timer60 ? SMOOTH_US / PSP_FRAME_US : 1.0;
    if (settle > 0) return 1.0;              /* settling: no frame is anchored to a refresh, so time it as ORIGINAL. Squeezed
                                                from a stale anchor the game didn't wait for the clock at all (0.46.1 - 0.48.1) */
    double gap = frame_us_monitor * interval - fmin(margin, margin_cap());
    return gap < PSP_FRAME_US ? gap / PSP_FRAME_US : 1.0;
}

const char *pacing_describe(void) { return why; }
int pacing_setting(void) { return timing_wanted(); }
const char *pacing_setting_name(void) { return timing_name(timing_wanted()); }
/* what the player gets: 1 SMOOTH (60.00 Hz), 0 ORIGINAL (59.94 Hz): the MODE MENU, last_run.txt */
int pacing_given(void) { return family() == 1; }   /* as set, or as AUTO chose (ORIGINAL before it has): never the measured rate */
int pacing_swap_paced(void) { return swap_paced; }
double pacing_audio_ratio(void) { return ratio; }
