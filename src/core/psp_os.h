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

void hle_sceKernelCreateSema(Cpu *c);

/* A semaphore named `name` (a guest string): its id, or a negative error. */
static inline int32_t os_create_sema(Cpu *c, uint32_t name, uint32_t attr, int32_t initial, int32_t max, uint32_t options) {
    c->r[4] = name; c->r[5] = attr; c->r[6] = (uint32_t)initial; c->r[7] = (uint32_t)max; c->r[8] = options;
    hle_sceKernelCreateSema(c);
    return (int32_t)c->r[2];
}

void hle_sceNetGetLocalEtherAddr(Cpu *c);
void hle_sceUtilityGetSystemParamString(Cpu *c);
void hle_sceNetAdhocctlScan(Cpu *c);

/* This PSP's MAC address (6 bytes) into `buf`. */
static inline int32_t os_net_get_local_ether_addr(Cpu *c, uint32_t buf) { c->r[4] = buf; hle_sceNetGetLocalEtherAddr(c); return (int32_t)c->r[2]; }
/* A system parameter's string (1: the nickname) into `buf`, at most `len` bytes. */
static inline int32_t os_utility_get_system_param_string(Cpu *c, int32_t id, uint32_t buf, int32_t len) {
    c->r[4] = (uint32_t)id; c->r[5] = buf; c->r[6] = (uint32_t)len; hle_sceUtilityGetSystemParamString(c); return (int32_t)c->r[2];
}
/* Starts a scan for ad-hoc groups. */
static inline int32_t os_net_adhocctl_scan(Cpu *c) { hle_sceNetAdhocctlScan(c); return (int32_t)c->r[2]; }

void hle_sceNetAdhocctlConnect(Cpu *c);
void hle_sceNetAdhocMatchingStop(Cpu *c);
void hle_sceNetAdhocMatchingDelete(Cpu *c);
void hle_sceNetAdhocMatchingTerm(Cpu *c);

/* Joins (or makes) the ad-hoc group named by the guest string `name`. */
static inline int32_t os_net_adhocctl_connect(Cpu *c, uint32_t name) { c->r[4] = name; hle_sceNetAdhocctlConnect(c); return (int32_t)c->r[2]; }
/* Stops / deletes the matching context `id`; ends the matching library. */
static inline int32_t os_net_adhoc_matching_stop(Cpu *c, int32_t id) { c->r[4] = (uint32_t)id; hle_sceNetAdhocMatchingStop(c); return (int32_t)c->r[2]; }
static inline int32_t os_net_adhoc_matching_delete(Cpu *c, int32_t id) { c->r[4] = (uint32_t)id; hle_sceNetAdhocMatchingDelete(c); return (int32_t)c->r[2]; }
static inline int32_t os_net_adhoc_matching_term(Cpu *c) { hle_sceNetAdhocMatchingTerm(c); return (int32_t)c->r[2]; }
void hle_sceNetAdhocMatchingInit(Cpu *c);
void hle_sceNetAdhocMatchingCreate(Cpu *c);
void hle_sceNetAdhocMatchingStart(Cpu *c);
static inline int32_t os_net_adhoc_matching_init(Cpu *c, uint32_t pool) { c->r[4] = pool; hle_sceNetAdhocMatchingInit(c); return (int32_t)c->r[2]; }
/* The ninth argument, the handler, goes in the guest stack's first word (sp + 0) as the PSP passes it. */
static inline int32_t os_net_adhoc_matching_create(Cpu *c, int32_t mode, int32_t max_peers, uint32_t port, int32_t buf_size,
                                                   uint32_t hello_us, uint32_t ping_us, int32_t retries, uint32_t msg_us, uint32_t handler) {
    c->r[4] = (uint32_t)mode; c->r[5] = (uint32_t)max_peers; c->r[6] = port; c->r[7] = (uint32_t)buf_size;
    c->r[8] = hello_us; c->r[9] = ping_us; c->r[10] = (uint32_t)retries; c->r[11] = msg_us;
    WW(c->r[29], handler);
    hle_sceNetAdhocMatchingCreate(c); return (int32_t)c->r[2];
}
static inline int32_t os_net_adhoc_matching_start(Cpu *c, int32_t id, int32_t stack, int32_t prio, int32_t stack2, int32_t prio2, int32_t opt_len, uint32_t opt) {
    c->r[4] = (uint32_t)id; c->r[5] = (uint32_t)stack; c->r[6] = (uint32_t)prio; c->r[7] = (uint32_t)stack2;
    c->r[8] = (uint32_t)prio2; c->r[9] = (uint32_t)opt_len; c->r[10] = opt;
    hle_sceNetAdhocMatchingStart(c); return (int32_t)c->r[2];
}

void hle_sceKernelWaitSema(Cpu *c);
void hle_sceKernelSignalSema(Cpu *c);

/* Waits for `count` of semaphore `sema` (timeout: a guest pointer, 0 for none). */
static inline int32_t os_wait_sema(Cpu *c, int32_t sema, int32_t count, uint32_t timeout) {
    c->r[4] = (uint32_t)sema; c->r[5] = (uint32_t)count; c->r[6] = timeout; hle_sceKernelWaitSema(c); return (int32_t)c->r[2];
}
static inline int32_t os_signal_sema(Cpu *c, int32_t sema, int32_t count) {
    c->r[4] = (uint32_t)sema; c->r[5] = (uint32_t)count; hle_sceKernelSignalSema(c); return (int32_t)c->r[2];
}

void hle_sceNetAdhocctlDisconnect(Cpu *c);
void hle_sceNetAdhocctlGetParameter(Cpu *c);

/* Leaves the ad-hoc group. */
static inline int32_t os_net_adhocctl_disconnect(Cpu *c) { hle_sceNetAdhocctlDisconnect(c); return (int32_t)c->r[2]; }
/* The ad-hoc group's parameters (channel, group name, BSSID, nickname) into `buf`. */
static inline int32_t os_net_adhocctl_get_parameter(Cpu *c, uint32_t buf) { c->r[4] = buf; hle_sceNetAdhocctlGetParameter(c); return (int32_t)c->r[2]; }
void hle_sceNetAdhocctlGetState(Cpu *c);
/* The ad hoc control state into the guest word at `out`. */
static inline int32_t os_net_adhocctl_get_state(Cpu *c, uint32_t out) { c->r[4] = out; hle_sceNetAdhocctlGetState(c); return (int32_t)c->r[2]; }
void hle_sceNetAdhocctlDelHandler(Cpu *c);
void hle_sceNetAdhocctlTerm(Cpu *c);
void hle_sceNetAdhocTerm(Cpu *c);
void hle_sceNetTerm(Cpu *c);
static inline int32_t os_net_adhocctl_del_handler(Cpu *c, int32_t id) { c->r[4] = (uint32_t)id; hle_sceNetAdhocctlDelHandler(c); return (int32_t)c->r[2]; }
static inline int32_t os_net_adhocctl_term(Cpu *c) { hle_sceNetAdhocctlTerm(c); return (int32_t)c->r[2]; }
static inline int32_t os_net_adhoc_term(Cpu *c) { hle_sceNetAdhocTerm(c); return (int32_t)c->r[2]; }
static inline int32_t os_net_term(Cpu *c) { hle_sceNetTerm(c); return (int32_t)c->r[2]; }
void hle_sceNetAdhocctlGetScanInfo(Cpu *c);
/* The scan results: *len bytes into buf (buf 0 asks only for the length). */
static inline int32_t os_net_adhocctl_get_scan_info(Cpu *c, uint32_t len, uint32_t buf) { c->r[4] = len; c->r[5] = buf; hle_sceNetAdhocctlGetScanInfo(c); return (int32_t)c->r[2]; }

void hle_sceNetFreeThreadinfo(Cpu *c);
void hle_sceKernelWaitThreadEnd(Cpu *c);
void hle_sceKernelTerminateThread(Cpu *c);
void hle_sceKernelDeleteThread(Cpu *c);
void hle_sceNetAdhocMatchingSelectTarget(Cpu *c);

static inline int32_t os_net_free_threadinfo(Cpu *c, int32_t thread) { c->r[4] = (uint32_t)thread; hle_sceNetFreeThreadinfo(c); return (int32_t)c->r[2]; }
/* Waits for a thread to end (timeout: a guest pointer, 0 for none). */
static inline int32_t os_wait_thread_end(Cpu *c, int32_t thread, uint32_t timeout) { c->r[4] = (uint32_t)thread; c->r[5] = timeout; hle_sceKernelWaitThreadEnd(c); return (int32_t)c->r[2]; }
static inline int32_t os_terminate_thread(Cpu *c, int32_t thread) { c->r[4] = (uint32_t)thread; hle_sceKernelTerminateThread(c); return (int32_t)c->r[2]; }
static inline int32_t os_delete_thread(Cpu *c, int32_t thread) { c->r[4] = (uint32_t)thread; hle_sceKernelDeleteThread(c); return (int32_t)c->r[2]; }
/* Asks the matching context `id` to pair with the PSP at `mac`, sending `len` bytes of `data`. */
static inline int32_t os_net_adhoc_matching_select_target(Cpu *c, int32_t id, uint32_t mac, int32_t len, uint32_t data) {
    c->r[4] = (uint32_t)id; c->r[5] = mac; c->r[6] = (uint32_t)len; c->r[7] = data; hle_sceNetAdhocMatchingSelectTarget(c); return (int32_t)c->r[2];
}

void hle_sceKernelGetThreadId(Cpu *c);
/* The calling thread's id. */
static inline int32_t os_get_thread_id(Cpu *c) { hle_sceKernelGetThreadId(c); return (int32_t)c->r[2]; }
void hle_sceKernelExitThread(Cpu *c);
static inline void os_exit_thread(Cpu *c, int32_t status) { c->r[4] = (uint32_t)status; hle_sceKernelExitThread(c); }
