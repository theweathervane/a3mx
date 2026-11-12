/* IoFileMgrForUser, StdioForUser and sceUmdUser: the disc is the extracted ISO folder, ms0: is saves/.
 *
 * Archives from assets/ (docs/ASSET_LOADING.md): when rt_assets_root is set and a .pac opened from
 * the disc's data folder has an unpacked folder there (the one index.json names for it, with its
 * manifest.json; assets/LAYOUT.md), the descriptor is
 * served from an image of the archive built from that folder instead of the disc file. The game's
 * loaders make the same calls either way, so their timing is unchanged. */
#include "rt.h"
#include "asset_pac.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

char rt_disc_root[1024];
char rt_save_root[1024];
char rt_assets_root[1024];             /* assets/<region>, or empty: everything from the disc */

/* f: a host file; or mem: an archive built from assets/ (f then NULL), read at mem_pos */
typedef struct { FILE *f; char path[512]; int64_t async_result; int async_pending;
                 uint8_t *mem; size_t mem_size, mem_pos; } Fd;
static int fd_open(const Fd *d) { return d->f || d->mem; }
#define MAX_FD 64
static Fd fds[MAX_FD];

/* PSP path (disc0:/PSP_GAME/...) to host path. Returns 0 if the device is unknown. */
static int host_path(const char *psp, char *out, size_t n) {
    const char *colon = strchr(psp, ':');
    if (!colon) return 0;
    size_t dl = (size_t)(colon - psp);
    char dev[16] = {0};
    if (dl >= sizeof dev) return 0;
    for (size_t i = 0; i < dl; i++) dev[i] = (char)tolower((unsigned char)psp[i]);
    const char *root;
    if (!strncmp(dev, "disc", 4) || !strncmp(dev, "umd", 3)) root = rt_disc_root;
    else if (!strncmp(dev, "ms", 2) || !strncmp(dev, "fatms", 5)) root = rt_save_root;
    else return 0;
    const char *rest = colon + 1;
    while (*rest == '/' || *rest == '\\') rest++;
    snprintf(out, n, "%s/%s", root, rest);
    return 1;
}

static FILE *open_ci(const char *path, const char *mode) {
    FILE *f = fopen(path, mode);
    if (f) return f;
    /* the disc is case-insensitive; try lower and upper case of the last component */
    char tmp[1024]; strncpy(tmp, path, sizeof tmp - 1); tmp[sizeof tmp - 1] = 0;
    char *slash = strrchr(tmp, '/');
    char *p = slash ? slash + 1 : tmp;
    for (char *q = p; *q; q++) *q = (char)tolower((unsigned char)*q);
    if ((f = fopen(tmp, mode))) return f;
    for (char *q = p; *q; q++) *q = (char)toupper((unsigned char)*q);
    return fopen(tmp, mode);
}

/* ---------------------------------------------------------------- archives from assets/ */
/* The archive name if `psp` is a .pac in the disc's data folder (disc0:/PSP_GAME/USRDIR/<r>/data/). */
static int data_archive(const char *psp, char *stem, size_t n) {
    const char *colon = strchr(psp, ':');
    if (!colon || (strncmp(psp, "disc", 4) && strncmp(psp, "umd", 3))) return 0;
    const char *name = strrchr(psp, '/');
    if (!name || name - psp < 5 || strncmp(name - 5, "/data", 5)) return 0;
    name++;
    size_t len = strlen(name);
    if (len < 5 || len - 4 >= n) return 0;
    const char *ext = name + len - 4;
    if (ext[0] != '.' || tolower((unsigned char)ext[1]) != 'p' || tolower((unsigned char)ext[2]) != 'a' || tolower((unsigned char)ext[3]) != 'c') return 0;
    for (size_t i = 0; i < len - 4; i++) stem[i] = (char)tolower((unsigned char)name[i]);
    stem[len - 4] = 0;
    return 1;
}

static unsigned asset_opens, asset_fallbacks;
static int old_layout_said;   /* for the end-of-run report */

/* What was last said about each modded archive, so the log does not repeat it on every load. */
static struct { char stem[48]; unsigned hash; } said[64];
static int said_before(const char *stem, const char *text) {
    unsigned h = 2166136261u;
    for (const char *p = text; *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
    int free_slot = -1;
    for (int i = 0; i < 64; i++) {
        if (!said[i].stem[0]) { if (free_slot < 0) free_slot = i; continue; }
        if (!strcmp(said[i].stem, stem)) { if (said[i].hash == h) return 1; said[i].hash = h; return 0; }
    }
    if (free_slot >= 0) { snprintf(said[free_slot].stem, sizeof said[0].stem, "%s", stem); said[free_slot].hash = h; }
    return 0;
}

static uint8_t *read_whole(const char *path, size_t *n) {
    FILE *f = open_ci(path, "rb");
    if (!f) return NULL;
    fseeko(f, 0, SEEK_END); int64_t len = ftello(f); fseeko(f, 0, SEEK_SET);
    uint8_t *b = len > 0 ? malloc((size_t)len) : NULL;
    if (b && fread(b, 1, (size_t)len, f) != (size_t)len) { free(b); b = NULL; }
    fclose(f);
    *n = b ? (size_t)len : 0;
    return b;
}

/* The archive `psp` built from assets/, or 0 to read it from the disc (no folder for it, or the
   folder cannot be used: the reason is logged). */
static int asset_archive(const char *psp, const char *disc_path, PacImage *img) {
    char stem[48], folder[1200], manifest[1300];
    if (!rt_assets_root[0] || !data_archive(psp, stem, sizeof stem)) return 0;
    int where = asset_index_folder(rt_assets_root, stem, folder, sizeof folder);
    if (where > 0) return 0;                   /* the index has no folder for it */
    if (where < 0 && !old_layout_said) {
        old_layout_said = 1;
        rt_log("assets: no index.json in %s: using the layout from before 0.37.0 (pac/<archive>/); "
               "run tools/extract_assets.py again to move to the new one (your edits are carried over)", rt_assets_root);
    }
    snprintf(manifest, sizeof manifest, "%s/manifest.json", folder);
    struct stat s;
    if (stat(manifest, &s) != 0) return 0;
    char err[512];
    if (asset_pac_build(folder, img, err, sizeof err) != 0) {
        asset_fallbacks++;
        if (!said_before(stem, err)) rt_log("assets: %s.pac from the disc: %s", stem, err);
        return 0;
    }
    size_t dn = 0;
    uint8_t *disc = read_whole(disc_path, &dn);
    char report[2048];
    int ok = asset_pac_check(img, disc, dn, report, sizeof report);
    free(disc);
    if (ok < 0) {
        asset_fallbacks++;
        if (!said_before(stem, report)) rt_log("assets: %s.pac from the disc: %s (%s)", stem, report, folder);
        pac_image_free(img);
        return 0;
    }
    if (img->notes[0]) {                       /* entries from edited PNGs, or PNGs that could not be used */
        size_t used = strlen(report);
        snprintf(report + used, sizeof report - used, "%s%s", used ? "; " : "", img->notes);
    }
    if (report[0] && !said_before(stem, report)) rt_log("assets: %s.pac from %s: %s", stem, folder, report);
    return 1;
}

void rt_io_report(void) {
    if (rt_assets_root[0]) rt_log("assets: %u archive opens served from %s, %u from the disc instead", asset_opens, rt_assets_root, asset_fallbacks);
}

void rt_io_init(void) {
    if (rt_assets_root[0]) rt_log("assets: archives with a folder in %s (index.json) are loaded from it", rt_assets_root);
}

void hle_sceIoOpen(Cpu *c) {
    const char *p = rt_str(ARG(0)); uint32_t flags = ARG(1);
    char hp[1024];
    if (!p || !host_path(p, hp, sizeof hp)) { TRACE("sceIoOpen %s: unknown device", p ? p : "(null)"); RET(ERR_FILE_NOT_FOUND); }
    if (!(flags & 0x0002)) {                    /* read-only: an archive from assets/? */
        PacImage img;
        if (asset_archive(p, hp, &img)) {
            for (int i = 3; i < MAX_FD; i++) if (!fd_open(&fds[i])) {
                Fd *d = &fds[i];
                asset_opens++;
                d->f = NULL; d->mem = img.data; d->mem_size = img.size; d->mem_pos = 0; d->async_pending = 0;
                strncpy(d->path, p, sizeof d->path - 1);
                TRACE("sceIoOpen %s -> fd %d (from assets)", p, i);
                RET(i);
            }
            pac_image_free(&img); RET(0x80020320);
        }
    }
    const char *mode = "rb";
    if (flags & 0x0002) mode = (flags & 0x0200) ? ((flags & 0x0400) ? "w+b" : "r+b") : "r+b";
    if ((flags & 0x0002) && (flags & 0x0400)) mode = "w+b";
    if ((flags & 0x0002) && (flags & 0x0100)) mode = "a+b";
    FILE *f = open_ci(hp, mode);
    if (!f && (flags & 0x0200)) f = open_ci(hp, "w+b");
    if (!f) { TRACE("sceIoOpen %s -> not found", p); RET(ERR_FILE_NOT_FOUND); }
    for (int i = 3; i < MAX_FD; i++) if (!fd_open(&fds[i])) {
        fds[i].f = f; fds[i].mem = NULL; strncpy(fds[i].path, p, sizeof fds[i].path - 1); fds[i].async_pending = 0;
        TRACE("sceIoOpen %s -> fd %d", p, i);
        RET(i);
    }
    fclose(f); RET(0x80020320);
}
void hle_sceIoClose(Cpu *c) {
    uint32_t fd = ARG(0);
    if (fd >= MAX_FD || !fd_open(&fds[fd])) RET(ERR_BAD_FD);
    if (fds[fd].f) fclose(fds[fd].f);
    free(fds[fd].mem);
    fds[fd].f = NULL; fds[fd].mem = NULL; RET(0);
}
static int64_t do_read(uint32_t fd, uint32_t buf, uint32_t size) {
    if (fd >= MAX_FD || !fd_open(&fds[fd])) return (int32_t)ERR_BAD_FD;
    Fd *d = &fds[fd];
    if (d->mem) {
        size_t n = d->mem_pos < d->mem_size ? d->mem_size - d->mem_pos : 0;
        if (n > size) n = size;
        memcpy(HOST(buf), d->mem + d->mem_pos, n);
        d->mem_pos += n;
        return (int64_t)n;
    }
    int64_t got = (int64_t)fread(HOST(buf), 1, size, d->f);
    { extern uint64_t rt_io_read_bytes; if (got > 0) rt_io_read_bytes += (uint64_t)got; }   /* perf.c: loads during a hitch */
    return got;
}
/* --io-faults (tests): every Nth call of a kind fails as a real disc can, so the game's retry paths
   run. Deterministic: the same calls fail in every run. */
int rt_io_fault_busy, rt_io_fault_error, rt_io_fault_poll, rt_io_fault_read;
static unsigned io_async_calls, io_poll_calls, io_read_calls;
#define ERR_ASYNC_BUSY 0x80020329u
#define ERR_IO_FAULT   0x80010005u
void hle_sceIoRead(Cpu *c) {
    io_read_calls++;
    if (rt_io_fault_read && io_read_calls % (unsigned)rt_io_fault_read == 0) RET(ERR_IO_FAULT);
    RET((uint32_t)do_read(ARG(0), ARG(1), ARG(2)));
}
void hle_sceIoReadAsync(Cpu *c) {
    uint32_t fd = ARG(0);
    if (fd >= MAX_FD || !fd_open(&fds[fd])) RET(ERR_BAD_FD);
    io_async_calls++;
    if (rt_io_fault_busy && io_async_calls % (unsigned)rt_io_fault_busy < 2) RET(ERR_ASYNC_BUSY);   /* two in a row: a retry can find it still busy */
    if (rt_io_fault_error && io_async_calls % (unsigned)rt_io_fault_error == 0) RET(ERR_IO_FAULT);
    fds[fd].async_result = do_read(fd, ARG(1), ARG(2)); fds[fd].async_pending = 1;
    RET(0);
}
void hle_sceIoPollAsync(Cpu *c) {
    uint32_t fd = ARG(0), res = ARG(1);
    if (fd >= MAX_FD || !fd_open(&fds[fd])) RET(ERR_BAD_FD);
    if (!fds[fd].async_pending) RET(1);          /* nothing pending */
    io_poll_calls++;
    if (rt_io_fault_poll && io_poll_calls % (unsigned)rt_io_fault_poll == 0) RET(1);   /* not finished yet */
    if (res) { WW(res, (uint32_t)fds[fd].async_result); WW(res + 4, (uint32_t)((uint64_t)fds[fd].async_result >> 32)); }
    fds[fd].async_pending = 0;
    RET(0);
}
void hle_sceIoWrite(Cpu *c) {
    uint32_t fd = ARG(0), buf = ARG(1), size = ARG(2);
    if (fd == 1 || fd == 2) { rt_log("[game] %.*s", (int)size, (char *)HOST(buf)); RET(size); }
    if (fd >= MAX_FD || !fds[fd].f) RET(ERR_BAD_FD);
    RET(fwrite(HOST(buf), 1, size, fds[fd].f));
}
void hle_sceIoLseek(Cpu *c) {
    uint32_t fd = ARG(0); int64_t off = (int64_t)ARG64(2); int whence = (int)ARG(4);
    if (fd >= MAX_FD || !fd_open(&fds[fd])) RET64((int64_t)(int32_t)ERR_BAD_FD);
    if (fds[fd].mem) {
        Fd *d = &fds[fd];
        int64_t pos = (whence == 1 ? (int64_t)d->mem_pos : whence == 2 ? (int64_t)d->mem_size : 0) + off;
        if (pos < 0) RET64((int64_t)(int32_t)0x80010016);   /* EINVAL, as a seek before the start */
        d->mem_pos = (size_t)pos;
        RET64(pos);
    }
    fseeko(fds[fd].f, off, whence == 1 ? SEEK_CUR : whence == 2 ? SEEK_END : SEEK_SET);
    RET64(ftello(fds[fd].f));
}
void hle_sceIoGetstat(Cpu *c) {
    const char *p = rt_str(ARG(0)); uint32_t st = ARG(1);
    char hp[1024]; struct stat s;
    if (!p || !host_path(p, hp, sizeof hp) || stat(hp, &s) != 0) RET(ERR_FILE_NOT_FOUND);
    {   /* an archive from assets/ has the size of its image */
        PacImage img;
        if (asset_archive(p, hp, &img)) { s.st_size = (off_t)img.size; pac_image_free(&img); }
    }
    memset(HOST(st), 0, 0x58);
    int dir = (s.st_mode & S_IFDIR) != 0;
    WW(st + 0, dir ? 0x11FF : 0x21FF);           /* mode */
    WW(st + 4, dir ? 0x10 : 0x20);               /* attr */
    WW(st + 8, (uint32_t)s.st_size); WW(st + 12, (uint32_t)((uint64_t)s.st_size >> 32));
    RET(0);
}
void hle_sceIoRename(Cpu *c) {
    char a[1024], b[1024];
    if (!host_path(rt_str(ARG(0)), a, sizeof a) || !host_path(rt_str(ARG(1)), b, sizeof b)) RET(ERR_FILE_NOT_FOUND);
    RET(rename(a, b) == 0 ? 0 : ERR_FILE_NOT_FOUND);
}
/* Devices: only the memory stick (fatms0:/ms0:) is asked anything; it is always inserted, never full. */
void hle_sceIoDevctl(Cpu *c) {
    const char *dev = rt_str(ARG(0)); uint32_t cmd = ARG(1), argp = ARG(2), argl = ARG(3), outp = ARG(4);
    TRACE("sceIoDevctl %s cmd=%08X", dev ? dev : "", cmd);
    switch (cmd) {
    case 0x02415821:                               /* register insert/eject callback: fire "inserted" now */
        if (argp && argl >= 4) rt_notify_callback(RW(argp), 1);
        RET(0);
    case 0x02415822: RET(0);                       /* unregister it */
    case 0x02415823: RET(0);                       /* set FAT state */
    case 0x02425823: if (outp) WW(outp, 1); RET(0);   /* FAT enabled / stick inserted */
    case 0x02425824: if (outp) WW(outp, 0); RET(0);   /* not write-protected */
    case 0x02425818:                               /* capacity: pointer to {max, free clusters, max sectors, sector size, count} */
        if (argp && argl >= 4) {
            uint32_t d = RW(argp);
            uint32_t clusters = (uint32_t)((uint64_t)1 << 30) / 0x8000u;   /* 1 GB free in 32 KB clusters */
            WW(d, clusters); WW(d + 4, clusters); WW(d + 8, clusters); WW(d + 12, 0x200); WW(d + 16, 0x40);
        }
        RET(0);
    default: RET(0);
    }
}

void hle_sceKernelStdin(Cpu *c) { RET(0); }
void hle_sceKernelStdout(Cpu *c) { RET(1); }
void hle_sceKernelStderr(Cpu *c) { RET(2); }

void hle_sceUmdCheckMedium(Cpu *c) { RET(1); }
void hle_sceUmdActivate(Cpu *c) { RET(0); }
void hle_sceUmdWaitDriveStat(Cpu *c) { RET(0); }
void hle_sceUmdGetDriveStat(Cpu *c) { RET(0x32); }   /* present | ready | readable */
