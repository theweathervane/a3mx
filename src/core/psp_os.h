/* PSP OS functions as decompiled code calls them: typed wrappers over the runtime's HLE functions
 * (src/platform/hle_*.c, kernel.c), which take their arguments in the Cpu registers like the game's
 * own calls did. Add a wrapper here the first time decompiled code needs an OS function.
 */
#pragma once
#include "cpu.h"

void hle_sceKernelCpuSuspendIntr(Cpu *c);
void hle_sceKernelCpuResumeIntr(Cpu *c);

/* Interrupts off; returns the previous state, for os_resume_interrupts. */
static inline uint32_t os_suspend_interrupts(Cpu *c) { hle_sceKernelCpuSuspendIntr(c); return c->r[2]; }
static inline void os_resume_interrupts(Cpu *c, uint32_t state) { c->r[4] = state; hle_sceKernelCpuResumeIntr(c); }

void hle_sceCtrlReadBufferPositive(Cpu *c);
void hle_sceKernelPowerTick(Cpu *c);

/* Reads `count` controller samples into guest memory at `data` (SceCtrlData each). */
static inline int32_t os_ctrl_read_buffer_positive(Cpu *c, uint32_t data, int32_t count) {
    c->r[4] = data; c->r[5] = (uint32_t)count; hle_sceCtrlReadBufferPositive(c); return (int32_t)c->r[2];
}
/* Tells the power manager the user did something (keeps the screen from dimming). */
static inline void os_power_tick(Cpu *c, int32_t type) { c->r[4] = (uint32_t)type; hle_sceKernelPowerTick(c); }

void hle_sceIoOpen(Cpu *c);
void hle_sceIoRead(Cpu *c);
void hle_sceIoClose(Cpu *c);
void hle_sceIoLseek(Cpu *c);
void hle_sceUmdCheckMedium(Cpu *c);
void hle_sceUmdActivate(Cpu *c);
void hle_sceUmdWaitDriveStat(Cpu *c);
void hle_sceKernelDelayThread(Cpu *c);

/* Files: descriptors are small non-negative ints; negative results are PSP error codes. */
static inline int32_t os_io_open(Cpu *c, uint32_t path, uint32_t flags, uint32_t mode) {
    c->r[4] = path; c->r[5] = flags; c->r[6] = mode; hle_sceIoOpen(c); return (int32_t)c->r[2];
}
static inline int32_t os_io_read(Cpu *c, int32_t fd, uint32_t buf, uint32_t size) {
    c->r[4] = (uint32_t)fd; c->r[5] = buf; c->r[6] = size; hle_sceIoRead(c); return (int32_t)c->r[2];
}
static inline int32_t os_io_close(Cpu *c, int32_t fd) { c->r[4] = (uint32_t)fd; hle_sceIoClose(c); return (int32_t)c->r[2]; }
/* sceIoLseek(fd, 64-bit offset, whence): the offset goes in a2 / a3 (o32 64-bit argument pairs). */
static inline int64_t os_io_lseek(Cpu *c, int32_t fd, int64_t offset, int32_t whence) {
    c->r[4] = (uint32_t)fd; c->r[6] = (uint32_t)offset; c->r[7] = (uint32_t)((uint64_t)offset >> 32); c->r[8] = (uint32_t)whence;
    hle_sceIoLseek(c);
    return (int64_t)((uint64_t)c->r[3] << 32 | c->r[2]);
}
static inline int32_t os_umd_check_medium(Cpu *c) { hle_sceUmdCheckMedium(c); return (int32_t)c->r[2]; }
static inline int32_t os_umd_activate(Cpu *c, int32_t mode, uint32_t drive) { c->r[4] = (uint32_t)mode; c->r[5] = drive; hle_sceUmdActivate(c); return (int32_t)c->r[2]; }
static inline int32_t os_umd_wait_drive_stat(Cpu *c, int32_t stat) { c->r[4] = (uint32_t)stat; hle_sceUmdWaitDriveStat(c); return (int32_t)c->r[2]; }
/* Sleeps this game thread; other threads run meanwhile. */
static inline void os_delay_thread(Cpu *c, uint32_t microseconds) { c->r[4] = microseconds; hle_sceKernelDelayThread(c); }

void hle_sceIoReadAsync(Cpu *c);
void hle_sceIoPollAsync(Cpu *c);
void hle_sceIoGetstat(Cpu *c);

#define PSP_ERR_ASYNC_BUSY 0x80020329u   /* an asynchronous operation is still in progress */

/* Starts reading size bytes in the background; 0, PSP_ERR_ASYNC_BUSY or an error. */
static inline int32_t os_io_read_async(Cpu *c, int32_t fd, uint32_t buf, uint32_t size) {
    c->r[4] = (uint32_t)fd; c->r[5] = buf; c->r[6] = size; hle_sceIoReadAsync(c); return (int32_t)c->r[2];
}
/* 0 when the background operation is done (its 64-bit result written to `result`), 1 while it
   runs, or an error. */
static inline int32_t os_io_poll_async(Cpu *c, int32_t fd, uint32_t result) {
    c->r[4] = (uint32_t)fd; c->r[5] = result; hle_sceIoPollAsync(c); return (int32_t)c->r[2];
}
/* File information (SceIoStat) into `stat`; 0 or an error. */
static inline int32_t os_io_getstat(Cpu *c, uint32_t path, uint32_t stat) {
    c->r[4] = path; c->r[5] = stat; hle_sceIoGetstat(c); return (int32_t)c->r[2];
}

/* ---------------------------------------------------------------- threads (the task system, task/task.c) */
void hle_sceKernelCreateThread(Cpu *c);
void hle_sceKernelStartThread(Cpu *c);
void hle_sceKernelSleepThread(Cpu *c);
void hle_sceKernelWakeupThread(Cpu *c);
void hle_sceKernelCancelWakeupThread(Cpu *c);
void hle_sceKernelExitDeleteThread(Cpu *c);
void hle_sceKernelTerminateDeleteThread(Cpu *c);
void hle_sceKernelDelayThreadCB(Cpu *c);
void hle_sceKernelCreateCallback(Cpu *c);
void hle_sceKernelRegisterExitCallback(Cpu *c);
void hle_sceKernelDcacheWritebackAll(Cpu *c);
void hle_sceDisplayGetVcount(Cpu *c);

/* A new thread (dormant until started): its id, or an error. */
static inline int32_t os_create_thread(Cpu *c, uint32_t name, uint32_t entry, int32_t priority, uint32_t stack_size, uint32_t attr, uint32_t options) {
    c->r[4] = name; c->r[5] = entry; c->r[6] = (uint32_t)priority; c->r[7] = stack_size; c->r[8] = attr; c->r[9] = options;
    hle_sceKernelCreateThread(c); return (int32_t)c->r[2];
}
static inline int32_t os_start_thread(Cpu *c, int32_t thread, uint32_t arg_size, uint32_t argp) {
    c->r[4] = (uint32_t)thread; c->r[5] = arg_size; c->r[6] = argp; hle_sceKernelStartThread(c); return (int32_t)c->r[2];
}
/* Sleeps until another thread wakes this one (a wake-up sent earlier counts). */
static inline int32_t os_sleep_thread(Cpu *c) { hle_sceKernelSleepThread(c); return (int32_t)c->r[2]; }
static inline int32_t os_wakeup_thread(Cpu *c, int32_t thread) { c->r[4] = (uint32_t)thread; hle_sceKernelWakeupThread(c); return (int32_t)c->r[2]; }
/* Drops the wake-ups sent to a thread (0: this one) that it has not slept through yet. */
static inline int32_t os_cancel_wakeup_thread(Cpu *c, int32_t thread) { c->r[4] = (uint32_t)thread; hle_sceKernelCancelWakeupThread(c); return (int32_t)c->r[2]; }
/* Ends this thread and deletes it: does not return. */
static inline void os_exit_delete_thread(Cpu *c, int32_t status) { c->r[4] = (uint32_t)status; hle_sceKernelExitDeleteThread(c); }
static inline int32_t os_terminate_delete_thread(Cpu *c, int32_t thread) { c->r[4] = (uint32_t)thread; hle_sceKernelTerminateDeleteThread(c); return (int32_t)c->r[2]; }
/* As os_delay_thread, running callbacks (exit, power) meanwhile. */
static inline int32_t os_delay_thread_cb(Cpu *c, uint32_t microseconds) { c->r[4] = microseconds; hle_sceKernelDelayThreadCB(c); return (int32_t)c->r[2]; }
static inline int32_t os_create_callback(Cpu *c, uint32_t name, uint32_t func, uint32_t arg) {
    c->r[4] = name; c->r[5] = func; c->r[6] = arg; hle_sceKernelCreateCallback(c); return (int32_t)c->r[2];
}
static inline int32_t os_register_exit_callback(Cpu *c, int32_t callback) { c->r[4] = (uint32_t)callback; hle_sceKernelRegisterExitCallback(c); return (int32_t)c->r[2]; }
static inline void os_dcache_writeback_all(Cpu *c) { hle_sceKernelDcacheWritebackAll(c); }
/* Vertical blanks since the display started. */
static inline uint32_t os_display_get_vcount(Cpu *c) { hle_sceDisplayGetVcount(c); return c->r[2]; }
