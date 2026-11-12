/* Frame timing: where each frame's time goes on the PC (not in the game: nothing here reads or
 * writes guest memory, so it can't change what the game does).
 *
 * A frame runs from one vblank to the next. Its wall time is split into:
 *   game     the game's code (decompiled and translated) and the OS calls it makes
 *   render   walking the display lists and issuing the draws (ge_run): CPU time, including
 *   texture    looking textures up, and decoding and uploading the new ones (part of render)
 *   present  drawing the frame into the window
 *   swap     handing it to the monitor: waits for vsync (and for the GPU to catch up)
 *   idle     sleeping to keep 59.94 frames a second
 *   gpu      the GPU's time for the frame, from its first draw to the end of presenting (GL timer
 *            queries; not every driver has them). It includes gaps while the CPU is still issuing
 *            draws, but not the wait for vsync
 * "Busy" is the work: everything but idle and swap. A frame is slow when busy is over 16.7 ms: the PC
 * couldn't keep up. A hitch is what the player sees: the time from one frame reaching the screen to the
 * next was over 1.5 frames (at 60 Hz with vsync, a frame shown twice). Loads held off screen
 * (rt_hold_display) count as one long interval.
 *
 * Every 600 frames (10 s) one INFO line sums them up; the end of the run writes a summary to the
 * log and to <work>/last_run.txt (which the launcher's bug report reads). --perf-log FILE writes
 * one CSV row per frame. */
#pragma once
#include <stdint.h>

enum { PERF_RENDER, PERF_TEXTURE, PERF_PRESENT, PERF_IDLE, PERF_SWAP, PERF_N };
uint64_t perf_now_us(void);
void perf_add(int what, uint64_t us);                /* time spent in one part of the frame */
void perf_count_texture(int uploaded, uint32_t bytes_hashed);
void perf_gpu_time(uint64_t ns);                    /* a finished frame's GPU time (arrives a few frames late) */
void perf_frame(uint64_t frame);                    /* a vblank: closes the frame before it */
void perf_shown(void);                              /* a frame has just reached the screen (after the swap) */
void perf_set_renderer(const char *renderer, const char *gpu, int scale, const char *scale_from);
void perf_open_csv(const char *path);
void perf_report(const char *work_dir);              /* at exit */
