/* Reading files from the disc: the bottom of the game's loader.
 *
 * The game keeps one open file per load request (a FileHandle at the start of each LoadRequest,
 * pac.c): the descriptor, then the path it was opened with. Paths are built from a bare file name
 * ("st00.pac") as disc0:/PSP_GAME/USRDIR/usa/data/<name>. A flag at 0x08A03788 says whether a file
 * is open.
 *
 * Load requests are queued in a ring of 16 (0x088075F8); the loading thread works through them.
 */
#include "../decomp.h"
#include "../psp_os.h"
#include "loader.h"

#define FILE_OPEN_FLAG   0x08A03788u   /* 1 while a file is open */
#define DATA_PATH_FORMAT 0x08A03740u   /* "disc0:/PSP_GAME/USRDIR/usa/data/%s" */
#define OPEN_PATH        0x08E87888u   /* the path being opened */
#define UMD_DRIVE        0x089FDFC4u   /* "disc0:" */
#define OPEN_FLAGS       0x020F0001u   /* PSP_O_RDONLY plus the disc read-ahead bits the game uses */
#define ERR_UMD_NOT_READY 0x80210001u

void f_089F31AC(Cpu *c);               /* sprintf (C library, not decompiled yet) */

/* Builds disc0:/PSP_GAME/USRDIR/usa/data/<name> into dst. */
static int32_t data_path(Cpu *c, gaddr dst, gaddr name) {
    COVERAGE(08808188);
    CALL3(f_089F31AC, dst, DATA_PATH_FORMAT, name);
    return 0;
}

/* Closes the handle's file if it has one open; returns 1 and the close result if it did. */
static int file_close_r(Cpu *c, gaddr handle, int32_t *result) {
    COVERAGE(08808144);
    FileHandle *h = guest_ptr(handle);
    if (h->fd < 0) return 0;
    *result = os_io_close(c, h->fd);
    h->fd = -1;
    *(uint32_t *)guest_ptr(FILE_OPEN_FLAG) = 0;
    return 1;
}
static void file_close(Cpu *c, gaddr handle) { int32_t ignored; file_close_r(c, handle, &ignored); }

/* Reads size bytes; -1 if the disc has gone. */
static int32_t file_read(Cpu *c, int32_t fd, gaddr buf, uint32_t size) {
    COVERAGE(088080D8);
    if (!os_umd_check_medium(c)) return -1;
    return os_io_read(c, fd, buf, size);
}

/* Opens data/<name> into the handle (closing what it had); the descriptor, or -1. If the disc
   drive was not ready it is woken up, and the caller tries again. */
static int32_t file_open(Cpu *c, gaddr handle, gaddr name) {
    COVERAGE(08807F18);
    FileHandle *h = guest_ptr(handle);
    file_close(c, handle);
    data_path(c, OPEN_PATH, name);
    if (!os_umd_check_medium(c)) return -1;
    int32_t fd = os_io_open(c, OPEN_PATH, OPEN_FLAGS, 0);
    h->fd = fd;
    if (fd < 0) {
        if ((uint32_t)fd == ERR_UMD_NOT_READY) {
            os_umd_activate(c, 1, UMD_DRIVE);
            os_umd_wait_drive_stat(c, 0x20);
        }
        return -1;
    }
    data_path(c, handle + 4, name);
    *(uint32_t *)guest_ptr(FILE_OPEN_FLAG) = 1;
    return h->fd;
}

/* As file_open, without waking the drive (the loader thread's open). */
static int32_t file_open_quiet(Cpu *c, gaddr handle, gaddr name) {
    COVERAGE(08808010);
    FileHandle *h = guest_ptr(handle);
    file_close(c, handle);
    data_path(c, OPEN_PATH, name);
    if (!os_umd_check_medium(c)) return -1;
    int32_t fd = os_io_open(c, OPEN_PATH, OPEN_FLAGS, 0);
    h->fd = fd;
    if (fd < 0) return -1;
    *(uint32_t *)guest_ptr(FILE_OPEN_FLAG) = 1;
    data_path(c, handle + 4, name);
    return h->fd;
}

int32_t loader_file_open(Cpu *c, gaddr handle, gaddr name) { return file_open(c, handle, name); }
int32_t loader_file_open_quiet(Cpu *c, gaddr handle, gaddr name) { return file_open_quiet(c, handle, name); }
void loader_file_close(Cpu *c, gaddr handle) { file_close(c, handle); }
int32_t loader_file_read(Cpu *c, int32_t fd, gaddr buf, uint32_t size) { return file_read(c, fd, buf, size); }

/* ---------------------------------------------------------------- the request ring */
#define REQUEST_NEXT   0x08E76084u   /* u8: the next slot, 0-15 */
#define LOADER_STATE   0x08E76068u

typedef struct { uint32_t busy; char name[32]; } RequestName;
GUEST_STRUCT_SIZE(RequestName, 36);

void f_089F34D8(Cpu *c);               /* strcpy */

/* Queues a load of `name`; `kind` and `dest` go into the request. The ring holds 16: queueing into a
   slot still in use hangs here, as on the PSP (the game never queues that many). */
static int32_t queue_load(Cpu *c, gaddr name, uint8_t kind, uint32_t dest) {
    uint8_t *next = guest_ptr(REQUEST_NEXT);
    RequestName *slot = (RequestName *)guest_ptr(REQUEST_NAMES) + *next;
    ((uint8_t *)guest_ptr(LOADER_STATE))[4] = 0;
    if (slot->busy != 0) for (;;) {}
    slot->busy = 1;
    CALL2(f_089F34D8, guest_addr(slot->name), name);

    LoadRequest *r = loader_request(*next);
    (*next)++;
    r->status = 0;
    r->kind = kind;
    r->dest = dest;
    r->file_pos = 0;
    *next &= 0xF;
    return 0;
}

/* int file_open(FileHandle *, const char *name) */
SFA_FUNC(08807F18, game_file_open) { RETURN(file_open(c, ARG(0), ARG(1))); }
/* int file_open_quiet(FileHandle *, const char *name) */
SFA_FUNC(08808010, game_file_open_quiet) { RETURN(file_open_quiet(c, ARG(0), ARG(1))); }
/* int file_read(int fd, void *buf, size_t size) */
SFA_FUNC(088080D8, game_file_read) { RETURN(file_read(c, (int32_t)ARG(0), ARG(1), ARG(2))); }
/* void file_close(FileHandle *) */
SFA_FUNC(08808144, game_file_close) {
    int32_t result;
    if (file_close_r(c, ARG(0), &result)) RETURN(result);   /* (v0 is left alone when nothing was open) */
}
/* int data_path(char *dst, const char *name) */
SFA_FUNC(08808188, game_data_path) { RETURN(data_path(c, ARG(0), ARG(1))); }
/* int queue_load(const char *name, u8 kind, u32 dest) */
SFA_FUNC(088075F8, game_queue_load) { RETURN(queue_load(c, ARG(0), (uint8_t)ARG(1), ARG(2))); }
