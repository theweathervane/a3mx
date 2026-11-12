/* The game's tasks: the backbone everything else runs on (docs/PSP_PROGRAM_MAP.md).
 *
 * The game is a handful of tasks, each a PSP thread, run in turn once a frame by the main thread:
 *
 *   main thread (0x08804B84)        every frame: input, then one scheduler pass, then the frame is
 *                                   finished (graphics) and it waits for the next vertical blank
 *   scheduler pass (0x08805374)     walks the 8 task slots: starts a new task's thread, counts down a
 *                                   waiting one, resumes one ready to go on; each time it sleeps until
 *                                   the task gives the frame back
 *   a task                          runs until it waits for a frame (0x088054F0), which wakes the main
 *                                   thread and sleeps; or ends (0x08805530), or restarts itself at a
 *                                   new entry (0x08805614)
 *
 * Task 0 runs the game's screens (0x088056A8: the screen table, then a mode replaces it); task 1 is
 * the background loader (0x0880775C, loader/async.c).
 */
#pragma once
#include "../guest.h"

/* A task slot (8 of them at 0x08AD8FE8). */
typedef enum {
    TASK_FREE     = 0,   /* nothing to run */
    TASK_START    = 1,   /* start at `entry` on the next pass (a new thread) */
    TASK_WAITING  = 2,   /* waiting `wait` more frames */
    TASK_RUNNING  = 3,   /* has had its turn this frame, or is having it */
    TASK_RESUME   = 4,   /* go on next pass without counting frames */
} TaskState;

typedef struct {
    uint16_t state;      /* TaskState */
    uint16_t wait;       /* frames left while TASK_WAITING */
    gaddr    entry;      /* where its thread starts */
    int32_t  thread;     /* its thread, while it has one */
} Task;
GUEST_STRUCT_SIZE(Task, 12);

#define TASKS          0x08AD8FE8u   /* Task[8] */
#define TASK_COUNT     8
#define CURRENT_TASK   0x08E75C40u   /* Task *: the one the pass is on (the running one, inside a task) */
#define MAIN_THREAD    0x08E75904u   /* the main thread's id: tasks wake it when they give the frame back */
#define FRAME_COUNT    0x08E75908u   /* u16: scheduler passes run */
#define GAME_EXITING   0x08A02DE8u   /* u32: set by the exit callback: the main thread stops running tasks */

static inline Task *task_slot(uint32_t i) { return (Task *)guest_ptr(TASKS) + i; }
static inline gaddr task_slot_addr(uint32_t i) { return TASKS + 12u * i; }
static inline Task *current_task(void) { return guest_ptr(*(gaddr *)guest_ptr(CURRENT_TASK)); }
