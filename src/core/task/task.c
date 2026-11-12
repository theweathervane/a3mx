/* The task system: the main thread, the scheduler pass, and what a task does to wait, end or restart
 * (task.h has the picture). The thread calls are the PSP's: a task and the main thread hand the CPU
 * to each other with wake-up / sleep, so exactly one of them runs at a time, in the same order as on
 * the PSP.
 */
#include "../decomp.h"
#include "../psp_os.h"
#include "task.h"

static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline gaddr current_task_addr(void) { return *W(CURRENT_TASK); }

#define TASK_THREAD_NAME   0x089FD0D8u   /* the name every task's thread gets */
#define TASK_PRIORITY      0x6B
#define TASK_STACK         0x20000u
#define TASK_THREAD_ATTR   0x200000u     /* PSP_THREAD_ATTR_... as the game passes it */

/* functions not decompiled yet */
void f_08804CFC(Cpu *c);   /* game set-up (memory, sound, graphics, settings) */
void f_08805318(Cpu *c);   /* set-up */
void f_088DBF88(Cpu *c);   /* set-up (network side) */
void f_0880509C(Cpu *c);   /* the per-frame input records (hooked: hooks.c) */
void f_08816E84(Cpu *c);   /* per frame, before the tasks */
void f_088E1AF8(Cpu *c);   /* before the scheduler pass */
void f_088E1ABC(Cpu *c);   /* after the scheduler pass */
void f_08804E68(Cpu *c);   /* end of the frame's game work */
void f_089E48F8(Cpu *c);   /* graphics: finish the display list */
void f_089E4924(Cpu *c);   /* graphics: sync */
void f_089E48D8(Cpu *c);   /* graphics */
void f_089E48B4(Cpu *c);   /* graphics: swap */
void f_089E3E78(Cpu *c);   /* graphics: wait for the vertical blank */
void f_089E47C4(Cpu *c);   /* graphics: start the next display list */
void f_089E4544(Cpu *c);   /* graphics */
void f_088059E8(Cpu *c);   /* per frame, after the frame (sound) */
void f_088051BC(Cpu *c);   /* controller read (decompiled: input/pad.c) */
void f_08809A4C(Cpu *c);   /* per frame */
void f_08807380(Cpu *c);   /* start-up */
void f_08804A08(Cpu *c);   /* start-up: the network module */
void f_089E3F00(Cpu *c);   /* start-up (graphics) */
void f_0880A6CC(Cpu *c);   /* draw text (string, x, y, ...) */
void f_0880B6CC(Cpu *c);   /* text drawn: flush */
void f_088054F0(Cpu *c);
void f_088054C8(Cpu *c);
void f_08805374(Cpu *c);
void f_08804A80(Cpu *c);
void rt_call_indirect(Cpu *c, uint32_t addr);

/* ---------------------------------------------------------------- what a task does */

/* Queues `entry` to start in task slot `slot` on the next scheduler pass. */
static void task_start(uint32_t slot, gaddr entry) {
    Task *t = task_slot(slot);
    t->entry = entry;
    t->state = TASK_START;
}

/* The running task gives the frame back and sleeps until `frames` more scheduler passes have been
   (1: the next frame). Every task's loop goes through here. */
static void task_wait_frames(Cpu *c, uint32_t frames) {
    current_task()->wait = (uint16_t)frames;
    current_task()->state = TASK_WAITING;
    os_wakeup_thread(c, (int32_t)*W(MAIN_THREAD));
    os_sleep_thread(c);
}

/* The running task ends: its slot is free, the main thread goes on. */
static void task_exit(Cpu *c) {
    current_task()->state = TASK_FREE;
    os_wakeup_thread(c, (int32_t)*W(MAIN_THREAD));
    os_exit_delete_thread(c, 0);
}

/* Stops the task in `slot`, unless it is the running one or the slot is free. */
static void task_kill(Cpu *c, uint32_t slot) {
    gaddr t = task_slot_addr(slot);
    if (t == current_task_addr()) return;
    Task *task = guest_ptr(t);
    if (task->state == TASK_FREE) return;
    int32_t thread = task->thread;
    task->state = TASK_FREE;
    os_terminate_delete_thread(c, thread);
}

/* The running task starts again at `entry` (a new thread, from the next pass); this one ends. */
static void task_restart(Cpu *c, gaddr entry) {
    current_task()->state = TASK_START;
    current_task()->entry = entry;
    os_wakeup_thread(c, (int32_t)*W(MAIN_THREAD));
    os_exit_delete_thread(c, 0);
}

/* ---------------------------------------------------------------- the scheduler pass */

/* Lets a task run: wakes its thread, then sleeps until it gives the frame back. (A wake-up left over
   from the task is dropped, so the next sleep waits for the next one.) */
static void run_task(Cpu *c, int32_t thread) {
    os_wakeup_thread(c, thread);
    os_sleep_thread(c);
    os_cancel_wakeup_thread(c, 0);
}

/* One frame of every task, slot by slot. The slot being run is kept in CURRENT_TASK, which is where a
   task finds its own record. */
static void scheduler_pass(Cpu *c) {
    const gaddr end = TASKS + 12u * TASK_COUNT;
    *W(CURRENT_TASK) = TASKS;
    gaddr t = current_task_addr();
    for (;;) {
        Task *task = guest_ptr(t);
        uint16_t state = task->state;
        if (state == TASK_START) {                          /* a new thread at its entry */
            task->state = TASK_RUNNING;
            int32_t thread = os_create_thread(c, TASK_THREAD_NAME, current_task()->entry, TASK_PRIORITY,
                                              TASK_STACK, TASK_THREAD_ATTR, 0);
            current_task()->thread = thread;
            os_start_thread(c, current_task()->thread, 0, 0);
            os_sleep_thread(c);
            os_cancel_wakeup_thread(c, 0);
        } else if (state == TASK_WAITING) {                 /* one frame less to wait */
            uint16_t left = (uint16_t)(task->wait - 1);
            task->wait = left;
            if (left == 0) {
                current_task()->state = TASK_RUNNING;
                run_task(c, current_task()->thread);
            }
        } else if (state == TASK_RESUME) {
            task->state = TASK_RUNNING;
            run_task(c, current_task()->thread);
        }
        /* TASK_FREE and TASK_RUNNING (and anything else): nothing this frame */
        gaddr next = (state >= 3 && state != TASK_RESUME ? t : current_task_addr()) + 12;
        *W(CURRENT_TASK) = next;
        if (next >= end) return;
        t = current_task_addr();
    }
}

/* ---------------------------------------------------------------- the main thread */

#define FRAME_PACING      0x08A02DE4u   /* u32: non-zero: the frame is shown only when it took under 2 vertical blanks */
#define VCOUNT_BEFORE     0x08A02DECu   /* u32: the vertical-blank count at the previous frame's end */
#define VCOUNT_LAST       0x08A02DF0u   /* u32: ... and at this one's */
#define FRAME_TICK        0x08E75931u   /* u8: GAME + 0x21, counts every loop */

/* Start-up done, the two first tasks queued (the screens, the loader), then one loop a frame, for
   ever. While the game exits (GAME_EXITING) no task runs, but input and sound go on. */
static void main_thread(Cpu *c) {
    CALL0(f_08804CFC);
    CALL0(f_08805318);
    CALL0(f_088DBF88);
    CALL2(f_088054C8, 0, 0x088056A8);                      /* task 0: the screens */
    CALL2(f_088054C8, 1, 0x0880775C);                      /* task 1: the loader */
    *H(FRAME_COUNT) = 0;
    for (;;) {
        CALL0(f_0880509C);
        CALL0(f_08816E84);
        if (*W(GAME_EXITING) == 0) {
            CALL0(f_088E1AF8);
            CALL0(f_08805374);
            CALL0(f_088E1ABC);
            *H(FRAME_COUNT) = (uint16_t)(*H(FRAME_COUNT) + 1);
            CALL0(f_08804E68);
            os_dcache_writeback_all(c);
            CALL0(f_089E48F8);
            if (*W(FRAME_PACING) != 0) {
                *W(VCOUNT_BEFORE) = *W(VCOUNT_LAST);
                uint32_t vcount = os_display_get_vcount(c);
                int32_t took = (int32_t)(vcount - *W(VCOUNT_BEFORE));
                *W(VCOUNT_LAST) = vcount;
                if (took < 2) {
                    CALL0(f_089E4924); CALL0(f_089E48D8); CALL0(f_089E48B4); CALL0(f_08804A80); CALL0(f_089E47C4);
                }
            } else {
                CALL0(f_089E4924); CALL0(f_089E48D8); CALL0(f_089E3E78); CALL0(f_089E48B4); CALL0(f_08804A80); CALL0(f_089E47C4);
            }
            CALL0(f_089E4544);
        }
        CALL0(f_088059E8);
        CALL0(f_088051BC);
        CALL0(f_08809A4C);
        *B(FRAME_TICK) = (uint8_t)(*B(FRAME_TICK) + 1);
    }
}

#define MAIN_THREAD_NAME   0x089FD0C8u
#define EXIT_CALLBACK_NAME 0x089FD0C0u
#define EXIT_CALLBACK      0x08804A88u   /* (not decompiled yet: only runs when the game is closed) */

/* Start-up on the first thread: the exit callback, then the main thread; this one then only sleeps,
   running callbacks. */
static void game_start(Cpu *c) {
    CALL0(f_08807380);
    CALL0(f_08804A08);
    int32_t callback = os_create_callback(c, EXIT_CALLBACK_NAME, EXIT_CALLBACK, 0);
    os_register_exit_callback(c, callback);
    int32_t thread = os_create_thread(c, MAIN_THREAD_NAME, 0x08804B84, 0x6F, 0x10000, 0, 0);
    *W(MAIN_THREAD) = (uint32_t)thread;
    os_start_thread(c, thread, 0, 0);
    CALL0(f_089E3F00);
    for (;;) os_delay_thread_cb(c, 10000);
}

/* ---------------------------------------------------------------- task 0's first entry: the screens */

#define SCREEN         0x08E75910u   /* u16: GAME + 0: the screen task 0 is on */
#define SCREEN_STEP    0x08E75912u   /* u16: GAME + 2: that screen's own state */
#define SCREENS_ON     0x08E75B98u   /* u8 */
#define SCREEN_TABLE   0x08A02EB0u   /* void (*[])(void): one function a screen, called every frame */
#define LOADING_TEXT   0x089FD0F0u

/* Every frame: the current screen's function, a text on the first five screens, then the frame. (A
   mode replaces this task's entry with task_restart.) The screen function gets its own address in
   a0, as the PSP code leaves it. */
static void screens_task(Cpu *c) {
    *H(SCREEN) = 0;
    *H(SCREEN_STEP) = 0;
    *B(SCREENS_ON) = 1;
    for (;;) {
        gaddr fn = W(SCREEN_TABLE)[*H(SCREEN)];
        c->r[4] = fn;
        rt_call_indirect(c, fn);
        if (*H(SCREEN) < 5) {
            CALL5(f_0880A6CC, LOADING_TEXT, 2, 0x50, 0x68, 4);
            CALL0(f_0880B6CC);
        }
        CALL1(f_088054F0, 1);
    }
}

/* ---------------------------------------------------------------- adapters
   Each reserves the original's stack frame, so the game code it calls runs at the same stack
   addresses as on the PSP. */
#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
/* void task_start(int slot, void (*entry)(void)) */
SFA_FUNC(088054C8, game_task_start) { task_start(ARG(0), ARG(1)); }
/* void task_wait_frames(u16 frames) */
/* quitkey.c: F2, back to the MODE MENU, takes over here */
SFA_FUNC_WRAPPED(088054F0, game_task_wait_frames, quitkey_task_wait) { FRAME(0x20, task_wait_frames(c, ARG(0))); }
/* void task_exit(void) */
SFA_FUNC(08805530, game_task_exit) { FRAME(0x20, task_exit(c)); }
/* void task_kill(int slot) */
SFA_FUNC(08805564, game_task_kill) { FRAME(0x20, task_kill(c, ARG(0))); }
/* void task_restart(void (*entry)(void)) */
SFA_FUNC(08805614, game_task_restart) { FRAME(0x20, task_restart(c, ARG(0))); }
/* void scheduler_pass(void) */
SFA_FUNC(08805374, game_scheduler_pass) { FRAME(0x40, scheduler_pass(c)); }
/* the main thread's entry */
SFA_FUNC(08804B84, game_main_thread) { FRAME(0x20, main_thread(c)); }
/* start-up (module_start's thread) */
SFA_FUNC(08804AF8, game_start_threads) { FRAME(0x30, game_start(c)); }
/* task 0's first entry */
SFA_FUNC(088056A8, game_screens_task) { FRAME(0x30, screens_task(c)); }
/* void set_frame_pacing(int on) */
SFA_FUNC(08804A74, game_set_frame_pacing) { *W(FRAME_PACING) = ARG(0); }
/* an empty function the main loop calls every frame (a stub left in the PSP build) */
SFA_FUNC(08804A80, game_frame_stub) { }
