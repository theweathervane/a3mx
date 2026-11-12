/* Boot: load the relocated program, start module_start on a root thread, run the scheduler.
 *
 * a3mx [--work DIR] [--frames N] [--fast] [--trace]
 *   --work DIR   folder written by tools/prepare.py (default: work)
 *   --frames N   quit after N frames (for tests)
 *   --fast       do not wait for the wall clock
 *   --trace      log every OS call
 *   --headless   no window or sound (tests)
 *   --fullscreen / --windowed   open full screen (the default) or in a window (F11 switches)
 *   --renderer gl|soft   GPU renderer (default) or the software reference renderer
 *   --scale N    GPU internal resolution, N x 480x272 (1-8; default: covers the monitor)
 *   --p2 / --p3 human|cpu|auto   who controls fighters 2 and 3 (auto: human if gamepad 2/3 is connected)
 *   --pads LIST                  the gamepads (1-4) shown as plugged in on the MODE MENU's pad icons (tests)
 *   --watch LO-HI  log loads/stores to that range by game address (build with -DA3MX_WATCH=ON)
 *   --watch-from N start watching at frame N
 *   --watch-pc PC  only report accesses made by the instruction at PC (repeatable, up to 16)
 *   --dump-from N   no screens saved before frame N
 *   --snap F:FILE  save user RAM (0x08800000-0x0A000000) at frame F
 *   --poke F[-G]:ADDR=VAL[/SIZE]  force a byte (or /2, /4) every frame from frame F (to frame G)
 *   --net-hub PORT   be node 0 of a local multi-instance network (PORT 0: any)
 *   --net-join PORT N  be node N (1-3) of that network (tests)
 *   --renderer opengl|soft   the GPU renderer (the default) or the software one. vulkan, d3d12 and metal are
 *                        accepted but not written yet: OpenGL is used and the log says so
 *   --renderer none|lazy  draw nothing / only the frames being dumped (tests)
 *   --log FILE   write the log there instead of work/a3mx.log
 *   --log-level off|error|warn|info|debug|verbose   how much to log (default info; verbose = --trace)
 *   --frame-timing auto|smooth|original   overrides SCREEN SETTING > FRAME TIMING for this run (pacing.c)
 *   --record-audio DIR   write each PSP audio channel to DIR/chN.wav
 *   --input S    scripted buttons, "FRAME=BUTTONS;..." e.g. "200=cross;206=" (names: cross circle square
 *                triangle start select up down left right l r, joined with +; "p2:" before them for pad 2)
 *   --dump DIR   save the displayed frame as DIR/frame_NNNNN.png every --dump-every N frames (default 60)
 *   --user-dir DIR       the player's folder, with saves/, configs/, logs/ and launcher/ in it (default: the
 *                        system's per-user application folder, e.g. %APPDATA%\a3mx; beside the work folder if
 *                        that can't be written: userdir.h). Saves and settings from earlier versions (saves/
 *                        beside the work folder, or the work folder itself) are moved there once
 *   --dir-saves DIR, --dir-configs DIR, --dir-logs DIR   one of those folders somewhere else (the launcher's
 *                        Paths tab); the others stay in the player's folder
 *   --filter NAME        SCREEN SETTING > FILTER for this run: off, crt-curved, crt-lottes, crt-royale, crt-guest,
 *                        crt-light, scan-light, scan-strong, scan-ossc, scan-dark, xbrz, scalefx (shader.h)
 *   --librashader FILE, --shaders DIR   the shader library and libretro's slang-shaders, if they are not where
 *                        the game looks (beside it, and .tools/ beside the work folder: shader.h)
 *   --save-dir DIR       keep saves and settings together in DIR, and the log and last_run.txt in the work
 *                        folder, as before 0.60.0 (tests start from an empty DIR)
 *   --hash-log FILE      write one line per frame: frame, hash of the program's memory, hash of the frame buffer
 *                        (tests/parity.py compares two runs with it)
 *   --stick "F=LX,LY;.." scripted analogue stick by frame (tests)
 *   --assets DIR         load each archive that DIR/index.json gives an unpacked folder from it (default:
 *                        assets/<region> beside the work folder, when it exists; docs/ASSET_LOADING.md)
 *   --features LIST      which extensions are on, by name (ext_features.h): saves, unlocks, menus, local_multiplayer, hud,
 *                        quit_key, widescreen, stage_extensions, shaders; also all, none, default, and -name to
 *                        take one out ("default,-hud"). The launcher's Extensions tab sets it
 *   --no-extensions      none of them: the PSP program alone
 *   --custom-assets      as before 0.60.0: also turns stage_extensions on (the project's own stage edge art)
 *   --no-assets          load everything from the disc
 *   --palette-census FILE   record which palette colours each indexed texture is drawn with (tools/pair_palettes.py)
 *   --io-faults busy=N,error=N,poll=N,read=N  every Nth async read busy (two in a row) / failing, poll not done, read failing (tests)
 *   --save-fails         every save the game makes fails, as on a full or missing memory stick (tests: the save
 *                        error questions)
 *   --save-questions     a failed auto-save asks the game's own questions instead of going on silently (tests)
 *   --difftest [N]       test decompiled functions against their originals on N random inputs each, then quit
 *   --decomp-report FILE how many times each decompiled function ran (tests/parity.py)
 *   --reference          run the translated original of every decompiled function (src/core) instead
 *   --reference-only A,B just these decompiled functions run as the original (hex addresses)
 */
#include "rt.h"
#include <string.h>
#include <sys/stat.h>
#include "ge.h"
#include "host.h"
#include "net_link.h"
#include "version.h"
#include "perf.h"
#include "userdir.h"
#include "ext_features.h"
#include "shader.h"

#ifdef _WIN32
/* Laptops with two GPUs: these exports ask the NVIDIA (Optimus) and AMD (PowerXpress) drivers to run
 * the game on the fast GPU rather than the power-saving one. */
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

int rt_trace;
int rt_log_level = RT_LOG_INFO;
int host_start_fullscreen = 1;
static FILE *log_file;

static int log_dirty;                       /* lines logged since the last rt_log_flush */
static void log_line(int level, const char *fmt, va_list ap) {
    static const char *const prefix[] = { "", "ERROR: ", "WARN: ", "", "DEBUG: ", "" };
    char buf[2048];
    vsnprintf(buf, sizeof buf, fmt, ap);
    size_t n = strlen(buf);
    while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
    double t = rt_now_us() / 1e6;
    /* buffered, and written out once a frame (rt_log_flush, from the frame's end): a line costs a
     * copy, not a write to the pipe or the disk, so logging doesn't slow the game */
    fprintf(stdout, "[%7.3f] %s%s\n", t, prefix[level], buf);
    if (log_file) fprintf(log_file, "[%7.3f] %s%s\n", t, prefix[level], buf);
    log_dirty = 1;
}
/* once a frame, and before the program ends: what was logged since, out to the launcher and the file */
void rt_log_flush(void) {
    if (!log_dirty) return;
    log_dirty = 0;
    fflush(stdout);
    if (log_file) fflush(log_file);
}
void rt_log(const char *fmt, ...) {
    if (rt_log_level < RT_LOG_INFO) return;
    va_list ap; va_start(ap, fmt); log_line(RT_LOG_INFO, fmt, ap); va_end(ap);
}
void rt_logl(int level, const char *fmt, ...) {
    if (level < RT_LOG_ERROR || level > RT_LOG_VERBOSE || rt_log_level < level) return;
    va_list ap; va_start(ap, fmt); log_line(level, fmt, ap); va_end(ap);
}
static void log_always(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_line(RT_LOG_OFF, fmt, ap); va_end(ap);
}
static int parse_log_level(const char *s) {
    static const char *const names[] = { "off", "error", "warn", "info", "debug", "verbose" };
    for (int i = 0; i < 6; i++) if (!strcmp(s, names[i])) return i;
    return -1;
}
void rt_fatal(const char *fmt, ...) {
    va_list ap; char buf[2048];
    va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    log_always("FATAL: %s", buf);   /* whatever the level: why the game stopped */
    rt_report();
    rt_log_flush();
    exit(1);
}

/* ---------------------------------------------------------------- translated-code services */
extern const FuncEntry rt_func_table[];
extern const unsigned rt_func_count;
extern const StubEntry rt_stub_table[];
extern const unsigned rt_stub_count;

static GuestFn lookup(uint32_t addr) {
    unsigned lo = 0, hi = rt_func_count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (rt_func_table[mid].addr < addr) lo = mid + 1; else hi = mid;
    }
    if (lo < rt_func_count && rt_func_table[lo].addr == addr) return rt_func_table[lo].fn;
    for (unsigned i = 0; i < rt_stub_count; i++) if (rt_stub_table[i].addr == addr) return rt_stub_table[i].fn;
    return NULL;
}
void rt_call_indirect(Cpu *c, uint32_t addr) {
    GuestFn f = lookup(addr & 0x0FFFFFFF ? addr : addr);
    if (!f) rt_fatal("call to unknown address %08X (ra=%08X)", addr, c->r[31]);
    f(c);
}
void rt_jump_indirect(Cpu *c, uint32_t addr) { rt_call_indirect(c, addr); }
void rt_bad_jump(Cpu *c, uint32_t addr, uint32_t from) { (void)c; rt_fatal("switch at %08X jumped to unknown case %08X", from, addr); }
void rt_syscall(Cpu *c, uint32_t code, uint32_t pc) { (void)c; rt_fatal("syscall %X at %08X", code, pc); }
void rt_break(Cpu *c, uint32_t pc) { (void)c; rt_fatal("break at %08X", pc); }
void rt_unsupported(Cpu *c, uint32_t pc) { (void)c; rt_fatal("unsupported instruction at %08X", pc); }

/* ---------------------------------------------------------------- memory watch (A3MX_WATCH builds) */
uint32_t rt_watch_lo = 1, rt_watch_hi = 0;      /* empty range */
static long watch_from;
static uint32_t watch_pc[16]; static int nwatch_pc;
typedef struct { uint32_t pc, addr; int write; uint64_t n; uint32_t last; } WatchHit;
static WatchHit whits[4096];
static int nwhits;
void rt_watch_hit(uint32_t addr, uint32_t pc, uint32_t value, int size, int write) {
    (void)size;
    if ((long)rt_vblank_count() < watch_from) return;
    if (nwatch_pc) { int ok = 0; for (int i = 0; i < nwatch_pc; i++) if (watch_pc[i] == pc) ok = 1; if (!ok) return; }
    for (int i = 0; i < nwhits; i++)
        if (whits[i].pc == pc && whits[i].addr == (addr & 0x0FFFFFFF) && whits[i].write == write) { whits[i].n++; whits[i].last = value; return; }
    if (nwhits < 4096) {
        WatchHit *h = &whits[nwhits++]; h->pc = pc; h->addr = addr & 0x0FFFFFFF; h->write = write; h->n = 1; h->last = value;
        rt_log("watch: %s %08X at pc %08X value %X", write ? "write" : "read ", h->addr, pc, value);
    }
}
static void watch_report(void) {
    for (int i = 0; i < nwhits; i++)
        rt_log("watch summary: %s %08X pc %08X x%llu last %X", whits[i].write ? "W" : "R", whits[i].addr, whits[i].pc,
               (unsigned long long)whits[i].n, whits[i].last);
}

/* ---------------------------------------------------------------- snapshots and pokes (reverse-engineering aids) */
static struct { long frame; char path[600]; } snaps[16];
static int nsnaps;
static struct { long frame, until; uint32_t addr; uint32_t value; int size; } pokes[1024];
static int npokes;
static void snaps_and_pokes(long f) {
    for (int i = 0; i < nsnaps; i++) if (snaps[i].frame == f) {
        FILE *o = fopen(snaps[i].path, "wb");
        if (o) { fwrite(HOST(USER_LO), 1, USER_HI - USER_LO, o); fclose(o); rt_log("snapshot of user RAM -> %s", snaps[i].path); }
    }
    for (int i = 0; i < npokes; i++) if (pokes[i].frame <= f && pokes[i].frame >= 0 && (pokes[i].until < 0 || f <= pokes[i].until)) {
        if (pokes[i].size == 1) WB(pokes[i].addr, pokes[i].value); else if (pokes[i].size == 2) WH(pokes[i].addr, pokes[i].value); else WW(pokes[i].addr, pokes[i].value);
    }
}

/* ---------------------------------------------------------------- per-frame hook and report */
uint64_t rt_ge_lists(void);
uint64_t rt_ge_cmds(void);
uint64_t rt_ge_take_frame_lists(void);
uint32_t rt_fb_addr(void);
uint32_t rt_fb_width(void);
uint32_t rt_fb_format(void);
static const char *dump_dir;
static int headless;
static int lazy_draw;                       /* --renderer lazy: rasterise only the frames being dumped */
/* ---- linked instances (local multiplayer through the game's network modes) ---- */
static int net_hub_port = -1, net_join_port, net_node;
static uint32_t net_pad;                         /* background instance: our player's buttons, from the hub */
static long auto_accept_at = -1, auto_request_at = -1;
extern int rt_net_established, rt_net_requested, rt_net_parent;
extern void (*rt_net_on_hello)(int node);
void rt_net_init(void);
void netplay_frame(void);
void sysdata_frame(void);
void rt_net_frame(void);
extern void (*rt_net_on_pad)(const uint8_t *p, size_t n);
extern void (*rt_net_on_request)(void);
static void on_pad(const uint8_t *p, size_t n) { if (n >= 4) memcpy(&net_pad, p, 4); }
/* Lobby automation. The instance that accepts a battle request becomes 1P, so background instances
 * send the requests (to node 0) and the player's own instance accepts them. */
static void on_request(void) { if (net_node == 0) auto_accept_at = (long)rt_vblank_count() + 20; }
static void on_hello(int node) { if (net_node > 0 && node == 0 && auto_request_at < 0 && !rt_net_requested) auto_request_at = (long)rt_vblank_count() + 60 + 90 * (net_node - 1); }
static void on_denied(void) { if (net_node > 0) auto_request_at = (long)rt_vblank_count() + 90 + 30 * net_node; }
extern void (*rt_net_on_denied)(void);
static int renderer_gl_ready, want_soft, render_scale;
/* scripted input: "FRAME=BUTTONS;FRAME=BUTTONS..." e.g. "200=cross;206=;300=start;306=" */
extern uint32_t rt_pad_buttons[4];
static struct { long frame; int player; uint32_t buttons; } script[512];
static int nscript, script_pos;
static uint32_t button_bits(const char *s) {
    static const struct { const char *n; uint32_t b; } names[] = {
        {"select",0x1},{"start",0x8},{"up",0x10},{"right",0x20},{"down",0x40},{"left",0x80},{"l",0x100},{"r",0x200},
        {"triangle",0x1000},{"circle",0x2000},{"cross",0x4000},{"square",0x8000},{"l2",0x40000000u},{"r2",0x80000000u},{"menukey",0x20000000u} };   /* menukey: F2 (tests) */
    uint32_t b = 0; char tok[32]; int n = 0;
    for (;; s++) {
        if (*s && *s != '+' && *s != ',') { if (n < 31) tok[n++] = *s; continue; }
        tok[n] = 0;
        for (unsigned i = 0; i < sizeof names / sizeof names[0]; i++) if (!strcmp(tok, names[i].n)) b |= names[i].b;
        n = 0;
        if (!*s) break;
    }
    return b;
}
static void parse_script(const char *s) {
    while (*s && nscript < 512) {
        char item[128]; int n = 0;
        while (*s && *s != ';' && n < 127) item[n++] = *s++;
        item[n] = 0; if (*s == ';') s++;
        char *eq = strchr(item, '='); if (!eq) continue;
        *eq = 0;
        const char *v = eq + 1; int pl = 0;
        if (v[0] == 'p' && v[1] >= '1' && v[1] <= '4' && v[2] == ':') { pl = v[1] - '1'; v += 3; }
        script[nscript].frame = atol(item); script[nscript].player = pl; script[nscript].buttons = button_bits(v); nscript++;
    }
}
static long dump_every = 60, dump_from = 0;
static FILE *hash_log;                      /* --hash-log */
static const char *decomp_report;            /* --decomp-report */
static int difftest_n;                      /* --difftest [N] */
/* --stick "FRAME=LX,LY;...": scripted analogue stick (0-255, 128 centre) from that frame on (tests) */
static struct { long frame; uint8_t lx, ly; } stick_script[256];
static int nstick, stick_pos;
static void parse_stick(const char *s) {
    while (*s && nstick < (int)(sizeof stick_script / sizeof stick_script[0])) {
        long f; int x, y, n = 0;
        if (sscanf(s, "%ld=%d,%d%n", &f, &x, &y, &n) != 3) rt_fatal("--stick: FRAME=LX,LY;...");
        stick_script[nstick].frame = f; stick_script[nstick].lx = (uint8_t)x; stick_script[nstick].ly = (uint8_t)y; nstick++;
        s += n; if (*s == ';') s++;
    }
}
static const char *save_dir;                /* --save-dir: saves and settings (tests start from none); else saves/ beside --work */
static uint32_t image_end;                  /* end of the program image (text, data, bss) */
/* A fast 64-bit hash for comparing runs: not cryptographic, only has to see any difference. */
static uint64_t hash_mem(uint32_t addr, uint32_t len) {
    const uint8_t *p = HOST(addr);
    uint64_t h = 0x9E3779B97F4A7C15ull ^ len;
    uint32_t i = 0;
    for (; i + 8 <= len; i += 8) {
        uint64_t w; memcpy(&w, p + i, 8);
        h ^= w * 0xBF58476D1CE4E5B9ull; h = (h << 27 | h >> 37) * 0x94D049BB133111EBull;
    }
    for (; i < len; i++) { h ^= p[i]; h *= 0x100000001B3ull; }
    return h ^ (h >> 31);
}
static void hash_frame(uint64_t f) {
    uint64_t state = hash_mem(0x08804000u, image_end - 0x08804000u), fb = 0;
    uint32_t fba = rt_fb_addr();
    if (fba) fb = hash_mem(fba, rt_fb_width() * 272u * (rt_fb_format() == 3 ? 4u : 2u));
    fprintf(hash_log, "%llu %016llx %016llx\n", (unsigned long long)f, (unsigned long long)state, (unsigned long long)fb);
}
/* Screens the runtime answers by itself (sysdata.c) are kept off screen: while held, the window keeps
 * showing the last frame presented. */
static long display_hold_until = -1;
/* A held display keeps the last frame on screen. Held frames normally run at full speed (a load or a
 * save no one needs to watch), but sound made during them comes out faster than it plays, and the
 * audio queue drops what doesn't fit (250 ms, host_sdl.c): a sound started just before is cut short.
 * rt_hold_display_paced keeps those frames in real time instead, for holds where a sound is playing
 * (OPTION MODE's SAVE / LOAD rows). It lasts until the hold ends: holds asked for during it (the save
 * task's own, sysdata.c) stay paced. */
static int hold_paced;
void rt_hold_display(int frames) { long u = (long)rt_vblank_count() + frames; if (u > display_hold_until) display_hold_until = u; }
void rt_hold_display_paced(int frames) { rt_hold_display(frames); hold_paced = 1; }
void rt_hold_display_set(int frames) { display_hold_until = (long)rt_vblank_count() + frames; }   /* also shortens a hold (paced or not) */
int rt_display_held(void) { return (long)rt_vblank_count() < display_hold_until; }
int rt_display_held_fast(void) {             /* kernel.c: held, and not paced: run at full speed */
    if (!rt_display_held()) { hold_paced = 0; return 0; }
    return !hold_paced;
}
static void lazy_update(uint64_t f) { if (lazy_draw) ge_no_draw = !(dump_dir && (long)f + 3 >= dump_from && ((f + 1) % (uint64_t)dump_every == 0 || (f + 2) % (uint64_t)dump_every == 0 || (f + 3) % (uint64_t)dump_every == 0)); }
void quitkey_request(void);                     /* quitkey.c */
/* The extensions carried over from the static recomp (src/extensions/: hooks, EX. SETTING, saves in
 * a3mx.dat / .cfg, WIDE FORCED, local players in the network modes, F2, ...). --no-extensions turns
 * them all off for a run, so the game plays as the PSP program alone: its own menus and saves (on the
 * emulated memory stick, <save dir>/ms0), one pad. A build configured with -DA3MX_EXTENSIONS=OFF
 * leaves them out altogether (extensions_off.c stands in for the names the platform uses). */
#ifdef A3MX_NO_EXTENSIONS
int rt_extensions = 0;
static int rt_extensions_built(void) { return 0; }
#else
int rt_extensions = 1;
static int rt_extensions_built(void) { return 1; }
#endif
void rt_display_vblank(void) {
    uint64_t n = rt_ge_take_frame_lists(), f = rt_vblank_count();
    perf_frame(f);
    rt_log_flush();
    snaps_and_pokes((long)f);
    static uint32_t scripted[4];
    while (script_pos < nscript && script[script_pos].frame <= (long)f) { scripted[script[script_pos].player] = script[script_pos].buttons; script_pos++; }
    while (stick_pos < nstick && stick_script[stick_pos].frame <= (long)f) {
        extern uint8_t rt_pad_lx, rt_pad_ly;
        rt_pad_lx = stick_script[stick_pos].lx; rt_pad_ly = stick_script[stick_pos].ly; stick_pos++;
    }
    {
        uint32_t pads[4] = { 0 }; uint8_t sticks[4][2];
        host_poll(pads, sticks);
        uint32_t pcfg_pad_to_psp(int player, uint32_t raw);   /* playercfg.c: L2 / R2 by the player's config */
        static int menu_prev;
        int menu_now = (scripted[0] & 0x20000000u) != 0;
        if (FEATURE(QUITKEY) && (host_menu_requested() || (menu_now && !menu_prev))) quitkey_request();
        menu_prev = menu_now;
        for (int i = 0; i < 4; i++) {
            uint32_t raw = pads[i] | (scripted[i] & ~0x20000000u);
            rt_pad_buttons[i] = FEATURE(MENUS) ? pcfg_pad_to_psp(i, raw) : raw & ~0xC0000000u;   /* no L2 / R2 on a PSP */
        }
        if (host_quit_requested()) rt_quit = 1;
    }
    rt_net_frame();
    if (FEATURE(MULTIPLAYER)) netplay_frame();
    if (rt_extensions) sysdata_frame();          /* (each of its parts by its own feature) */
    if (net_link_active()) {
        static int est_seen;
        if (net_link_self() == 0) {                     /* hub: send each background instance its player's pad */
            for (int k = 1; k < 4; k++) if (net_link_connected(k)) net_link_send(k, NL_PAD, &rt_pad_buttons[k], 4);
        } else {
            rt_pad_buttons[0] |= net_pad;
        }
        /* press Cross every 40 frames until the request is sent / the connection is made */
        if (auto_request_at >= 0) {
            long d = (long)f - auto_request_at;
            if (rt_net_requested || d > 1200) auto_request_at = -1;
            else if (d >= 0 && d % 40 < 6) rt_pad_buttons[0] |= 0x4000;
        }
        if (auto_accept_at >= 0) {
            long d = (long)f - auto_accept_at;
            if (rt_net_established > est_seen || d > 1200) { auto_accept_at = -1; est_seen = rt_net_established; }
            else if (d >= 0 && d % 40 < 6) rt_pad_buttons[0] |= 0x4000;
        }
        /* Dramatic Battle host: once every linked instance has joined, press Cross to start */
        if (net_link_self() == 0 && rt_net_parent) {
            static long full_since = -1; static int started;
            int want = 0; for (int k = 1; k < 4; k++) want += net_link_connected(k);
            if (!started && want && rt_net_established >= want && auto_accept_at < 0) {
                if (full_since < 0) full_since = (long)f;
                long d = (long)f - full_since;
                if (d >= 60 && d < 66) rt_pad_buttons[0] |= 0x4000;
                if (d >= 66) started = 1;
            } else if (!started) full_since = -1;
        }
    }
    int have_gl = renderer_gl_ready;
    int held = (long)f < display_hold_until;          /* the last frame stays on screen */
    uint64_t present_t0 = perf_now_us();
    if (!headless && !held) {
        static uint8_t rgba[480 * 272 * 4];
        int ww, wh;
        host_window_size(&ww, &wh);
        uint64_t swap_t0 = 0;
        if (have_gl && ge_use_gl && ge_gl_present(rt_fb_addr(), ww, wh)) {
            swap_t0 = perf_now_us(); host_gl_swap();
        } else {
            if (rt_fb_addr()) ge_fb_to_rgba(rt_fb_addr(), (int)rt_fb_width(), (int)rt_fb_format(), 480, 272, rgba);
            if (have_gl) { ge_gl_present_rgba(rgba, 480, 272, ww, wh); swap_t0 = perf_now_us(); host_gl_swap(); }
            else { swap_t0 = perf_now_us(); host_present(rgba, 480, 272); }   /* (SDL's renderer: draws and swaps) */
        }
        uint64_t shown = perf_now_us();
        perf_add(PERF_SWAP, shown - swap_t0);
        present_t0 += shown - swap_t0;                /* present is the drawing only */
        perf_shown();
    }
    { void pacing_frame(int presented, int frame); pacing_frame(!headless && !held, (int)f); }
    if (have_gl) ge_gl_frame_end();
    perf_add(PERF_PRESENT, perf_now_us() - present_t0);
    if (f <= 5 || f % 60 == 0)
        rt_log("frame %llu: %llu display lists, framebuffer %08X, %llu prims so far", (unsigned long long)f,
               (unsigned long long)n, rt_fb_addr(), (unsigned long long)ge_prims_drawn);
    if (dump_dir && !held && rt_fb_addr() && (long)f >= dump_from && f % (uint64_t)dump_every == 0) {
        static uint8_t *big;
        char p[1200];
        int w = 480, h = 272;
        if (!big) big = (uint8_t *)malloc((size_t)480 * 8 * 272 * 8 * 4);
        snprintf(p, sizeof p, "%s/frame_%05llu.png", dump_dir, (unsigned long long)f);
        if (!(ge_use_gl && ge_gl_read(rt_fb_addr(), big, &w, &h)))
            ge_fb_to_rgba(rt_fb_addr(), (int)rt_fb_width(), (int)rt_fb_format(), 480, 272, big);
        png_write(p, big, w, h);
    }
    if (hash_log) hash_frame(f);
    lazy_update(f);
}
void rt_ge_census(void);
void rt_net_report(void);
void rt_io_report(void);
void ge_census_write(void);
void rt_report(void) {
    ge_census_write();
    rt_net_report();
    rt_io_report();
    watch_report();
    rt_ge_census();
    rt_log("report: %llu frames, %llu display lists, %llu GE commands",
           (unsigned long long)rt_vblank_count(), (unsigned long long)rt_ge_lists(), (unsigned long long)rt_ge_cmds());
}

static long file_size(const char *p) { struct stat s; return stat(p, &s) == 0 ? (long)s.st_size : -1; }

/* the folder that holds DIR, with its trailing slash ("" for a bare name): work -> "", C:/x/work -> C:/x/ */
static void parent_of(const char *dir, char *out, size_t n) {
    snprintf(out, n, "%s", dir);
    size_t k = strlen(out);
    while (k && (out[k - 1] == '/' || out[k - 1] == '\\')) out[--k] = 0;   /* trailing slashes */
    while (k && out[k - 1] != '/' && out[k - 1] != '\\') out[--k] = 0;     /* the last name */
}

/* Earlier versions kept the saves and settings in the work folder (before 0.44.0) or in saves/ beside
 * it (to 0.55.0). Move them into the player's folder, once: the settings to CONFIGS, the rest to SAVES,
 * and only what isn't there already, so nothing there is ever replaced. (tools/build.py's SAVE_NAMES is
 * the same list.) Across drives a rename fails: the file is then copied and the old one left where it
 * was, with a line in the log. */
static int copy_file(const char *from, const char *to) {
    FILE *i = fopen(from, "rb"); if (!i) return 0;
    FILE *o = fopen(to, "wb"); if (!o) { fclose(i); return 0; }
    char buf[65536]; size_t n; int ok = 1;
    while ((n = fread(buf, 1, sizeof buf, i)) > 0) if (fwrite(buf, 1, n, o) != n) { ok = 0; break; }
    if (ferror(i)) ok = 0;
    fclose(i);
    if (fclose(o) != 0) ok = 0;
    if (!ok) remove(to);
    return ok;
}
static void move_saves_from(const char *from_dir, const char *saves, const char *configs) {
    static const char *names[] = { "a3mx.cfg", "a3mx.dat", "a3mx.cfg.bak", "a3mx.dat.bak",
                                   "sfa3max.cfg", "sfa3max.dat", "ms0", "import", "export" };
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        char from[1200], to[1200];
        struct stat st;
        const char *dest = strstr(names[i], ".cfg") ? configs : saves;
        snprintf(from, sizeof from, "%s/%s", from_dir, names[i]);
        snprintf(to, sizeof to, "%s/%s", dest, names[i]);
        if (!strcmp(from, to) || stat(from, &st) != 0) continue;
        int is_dir = (st.st_mode & S_IFMT) == S_IFDIR;
        if (stat(to, &st) == 0) { rt_log("saves: %s is in both %s and %s; using the one in %s", names[i], from_dir, dest, dest); continue; }
        if (rename(from, to) == 0) rt_log("saves: moved %s to %s", from, to);
        else if (!is_dir && copy_file(from, to)) rt_log("saves: copied %s to %s (the old one is still there and no longer used)", from, to);
        else rt_warn("saves: could not move %s to %s; move it by hand", from, to);
    }
}
const char *rt_work_dir = "work";            /* --work (shader_gl.c looks beside it for the shader library and the shaders) */
char rt_config_dir[1024];                   /* where a3mx.cfg is kept (savestore.c's store_path); "": with the saves (--save-dir) */

#ifdef A3MX_SDL
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#endif
int rt_custom_assets;                        /* --custom-assets: assets/extensions/ (the launcher's Custom Assets, off by default) */

int main(int argc, char **argv) {
    const char *work = "work", *log_path = NULL, *assets = NULL, *user_dir = NULL, *features_arg = NULL, *renderer_asked = NULL, *dir_saves = NULL, *dir_configs = NULL, *dir_logs = NULL, *filter_arg = NULL;
    int no_assets = 0;
    setvbuf(stdout, NULL, _IOFBF, 1 << 16);   /* written once a frame (rt_log_flush), not line by line */
    atexit(rt_log_flush);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--work") && i + 1 < argc) work = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) rt_max_frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--fast")) rt_realtime = 0;
        else if (!strcmp(argv[i], "--trace")) rt_trace = 1;
        else if (!strcmp(argv[i], "--log-level") && i + 1 < argc) {
            rt_log_level = parse_log_level(argv[++i]);
            if (rt_log_level < 0) { rt_log_level = RT_LOG_INFO; rt_fatal("--log-level: off, error, warn, info, debug or verbose"); }
        }
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) dump_dir = argv[++i];
        else if (!strcmp(argv[i], "--dump-every") && i + 1 < argc) dump_every = atol(argv[++i]);
        else if (!strcmp(argv[i], "--dump-from") && i + 1 < argc) dump_from = atol(argv[++i]);
        else if (!strcmp(argv[i], "--input") && i + 1 < argc) parse_script(argv[++i]);
        else if (!strcmp(argv[i], "--headless")) headless = 1;
        else if (!strcmp(argv[i], "--fullscreen")) host_start_fullscreen = 1;
        else if (!strcmp(argv[i], "--windowed")) host_start_fullscreen = 0;
        else if (!strcmp(argv[i], "--no-extensions")) features_arg = "none";
        else if (!strcmp(argv[i], "--features") && i + 1 < argc) features_arg = argv[++i];
        else if (!strcmp(argv[i], "--save-dir") && i + 1 < argc) save_dir = argv[++i];
        else if (!strcmp(argv[i], "--save-fails")) { extern int store_save_fails; store_save_fails = 1; }
        else if (!strcmp(argv[i], "--save-questions")) { extern int sysdata_save_questions; sysdata_save_questions = 1; }
        else if (!strcmp(argv[i], "--decomp-report") && i + 1 < argc) decomp_report = argv[++i];
        else if (!strcmp(argv[i], "--stick") && i + 1 < argc) parse_stick(argv[++i]);
        else if (!strcmp(argv[i], "--io-faults") && i + 1 < argc) {   /* busy=N,error=N,poll=N,read=N */
            extern int rt_io_fault_busy, rt_io_fault_error, rt_io_fault_poll, rt_io_fault_read;
            const char *s = argv[++i];
            while (*s) {
                char key[16]; int n = 0, used = 0;
                if (sscanf(s, "%15[a-z]=%d%n", key, &n, &used) != 2) rt_fatal("--io-faults: busy=N,error=N,poll=N,read=N");
                if (!strcmp(key, "busy")) rt_io_fault_busy = n; else if (!strcmp(key, "error")) rt_io_fault_error = n;
                else if (!strcmp(key, "poll")) rt_io_fault_poll = n; else if (!strcmp(key, "read")) rt_io_fault_read = n;
                else rt_fatal("--io-faults: unknown %s", key);
                s += used; if (*s == ',') s++;
            }
        }
        else if (!strcmp(argv[i], "--difftest")) difftest_n = (i + 1 < argc && argv[i + 1][0] != '-') ? atoi(argv[++i]) : 20000;
        else if (!strcmp(argv[i], "--hash-log") && i + 1 < argc) { hash_log = fopen(argv[++i], "w"); if (!hash_log) rt_fatal("cannot write %s", argv[i]); }
        else if (!strcmp(argv[i], "--reference")) { void decomp_set_reference(void); decomp_set_reference(); }
        else if (!strcmp(argv[i], "--reference-only") && i + 1 < argc) { int decomp_set_reference_only(const char *); if (!decomp_set_reference_only(argv[++i])) rt_fatal("--reference-only: hex addresses, comma-separated"); }
        else if (!strcmp(argv[i], "--net-hub") && i + 1 < argc) net_hub_port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--net-join") && i + 2 < argc) { net_join_port = atoi(argv[++i]); net_node = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--log") && i + 1 < argc) log_path = argv[++i];
        else if (!strcmp(argv[i], "--user-dir") && i + 1 < argc) user_dir = argv[++i];
        else if (!strcmp(argv[i], "--librashader") && i + 1 < argc) { extern const char *shader_lib_arg; shader_lib_arg = argv[++i]; }
        else if (!strcmp(argv[i], "--shaders") && i + 1 < argc) { extern const char *shader_dir_arg; shader_dir_arg = argv[++i]; }
        else if (!strcmp(argv[i], "--filter") && i + 1 < argc) filter_arg = argv[++i];
        else if (!strcmp(argv[i], "--dir-saves") && i + 1 < argc) dir_saves = argv[++i];
        else if (!strcmp(argv[i], "--dir-configs") && i + 1 < argc) dir_configs = argv[++i];
        else if (!strcmp(argv[i], "--dir-logs") && i + 1 < argc) dir_logs = argv[++i];
        else if (!strcmp(argv[i], "--assets") && i + 1 < argc) assets = argv[++i];
        else if (!strcmp(argv[i], "--custom-assets")) rt_custom_assets = 1;       /* before 0.60.0: now the stage_extensions feature */
        else if (!strcmp(argv[i], "--no-assets")) no_assets = 1;
        else if (!strcmp(argv[i], "--palette-census") && i + 1 < argc) { extern const char *ge_census_path; ge_census_path = argv[++i]; }
        else if (!strcmp(argv[i], "--renderer") && i + 1 < argc) {
            const char *r = argv[++i];
            /* vulkan, d3d12 and metal are taken, so the launcher's list and scripts can name them, but
               none of them is written yet (the plan: one SDL3 GPU backend for all three). OpenGL is
               used instead and the log says so: never a silent change of renderer. */
            if (!strcmp(r, "vulkan") || !strcmp(r, "d3d12") || !strcmp(r, "metal")) renderer_asked = r;
            else if (strcmp(r, "soft") && strcmp(r, "none") && strcmp(r, "lazy") && strcmp(r, "opengl") && strcmp(r, "gl"))
                rt_fatal("--renderer: opengl, soft, vulkan, d3d12 or metal (tests: none, lazy), not %s", r);
            want_soft = !strcmp(r, "soft") || !strcmp(r, "none") || !strcmp(r, "lazy");
            ge_no_draw = !strcmp(r, "none");
            if (!strcmp(r, "lazy")) { lazy_draw = 1; ge_no_draw = 1; }
        }
        else if (!strcmp(argv[i], "--scale") && i + 1 < argc) render_scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--perf-log") && i + 1 < argc) perf_open_csv(argv[++i]);
        else if (!strcmp(argv[i], "--frame-timing") && i + 1 < argc) { extern int rt_frame_timing_cli; const char *v = argv[++i]; rt_frame_timing_cli = !strcmp(v, "original") ? 0 : !strcmp(v, "smooth") ? 1 : !strcmp(v, "auto") ? 2 : -1; }
        else if ((!strcmp(argv[i], "--p2") || !strcmp(argv[i], "--p3")) && i + 1 < argc) {
            extern int rt_human[3]; int k = argv[i][3] - '1'; const char *v = argv[++i];
            rt_human[k] = !strcmp(v, "human") ? 1 : !strcmp(v, "cpu") ? 0 : -1;
        }
        else if (!strcmp(argv[i], "--pads") && i + 1 < argc) {       /* e.g. 124: gamepads taken as plugged in for the pad icons */
            extern int rt_pads_forced; rt_pads_forced = 0;
            for (const char *p = argv[++i]; *p; p++) if (*p >= '1' && *p <= '4') rt_pads_forced |= 1 << (*p - '1');
        }
        else if (!strcmp(argv[i], "--watch") && i + 1 < argc) {      /* LO-HI (hex), A3MX_WATCH builds only */
            const char *w = argv[++i]; rt_watch_lo = (uint32_t)strtoul(w, NULL, 16) & 0x0FFFFFFF;
            const char *d = strchr(w, '-'); rt_watch_hi = d ? (uint32_t)strtoul(d + 1, NULL, 16) & 0x0FFFFFFF : rt_watch_lo + 4;
        }
        else if (!strcmp(argv[i], "--watch-from") && i + 1 < argc) watch_from = atol(argv[++i]);
        else if (!strcmp(argv[i], "--snap") && i + 1 < argc && nsnaps < 16) {        /* FRAME:FILE */
            char *a = argv[++i], *colon = strchr(a, ':');
            if (colon) { snaps[nsnaps].frame = atol(a); snprintf(snaps[nsnaps].path, sizeof snaps[0].path, "%s", colon + 1); nsnaps++; }
        }
        else if (!strcmp(argv[i], "--poke") && i + 1 < argc && npokes < 1024) {      /* FRAME[-END]:ADDR=VALUE[/SIZE], held every frame from FRAME (to END) */
            char *a = argv[++i]; char *c1 = strchr(a, ':'), *eq = strchr(a, '='), *sl = strchr(a, '/');
            if (c1 && eq) { char *dash = strchr(a, '-'); pokes[npokes].frame = atol(a); pokes[npokes].until = dash && dash < c1 ? atol(dash + 1) : -1; pokes[npokes].addr = (uint32_t)strtoul(c1 + 1, NULL, 16);
                pokes[npokes].value = (uint32_t)strtoul(eq + 1, NULL, 16); pokes[npokes].size = sl ? atoi(sl + 1) : 1; npokes++; }
        }
        else if (!strcmp(argv[i], "--watch-pc") && i + 1 < argc && nwatch_pc < 16) watch_pc[nwatch_pc++] = (uint32_t)strtoul(argv[++i], NULL, 16);
        else if (!strcmp(argv[i], "--record-audio") && i + 1 < argc) { extern const char *rt_record_audio_dir; rt_record_audio_dir = argv[++i]; }
    }
    if (rt_trace && rt_log_level < RT_LOG_VERBOSE) rt_log_level = RT_LOG_VERBOSE;   /* --trace is verbose */
    if (rt_log_level == RT_LOG_VERBOSE) rt_trace = 1;
    char path[1200];
    /* The player's folder (userdir.h): saves/, configs/, logs/ and launcher/ in the system's per-user
     * application folder, or beside the work folder if that can't be written. --save-dir DIR keeps
     * saves and settings together in DIR and the log in work/, as before 0.60.0 (the tests). */
    static char user_root[1100], saves_default[1200], logs_dir[1200], beside[1100];
    int user_kind = -1;                       /* 1 the system's folder, 2 beside the game, 0 neither could be written */
    parent_of(work, beside, sizeof beside);
    if (!beside[0]) snprintf(beside, sizeof beside, ".");
    if (!save_dir) {
        user_kind = userdir_choose(user_root, sizeof user_root, user_dir, beside);
        snprintf(saves_default, sizeof saves_default, "%s/saves", user_root);
        snprintf(rt_config_dir, sizeof rt_config_dir, "%s/configs", user_root);
        snprintf(logs_dir, sizeof logs_dir, "%s/logs", user_root);
        /* one folder somewhere else (the launcher's Paths tab): made if it isn't there */
        if (dir_saves) { snprintf(saves_default, sizeof saves_default, "%s", dir_saves); userdir_make(saves_default); }
        if (dir_configs) { snprintf(rt_config_dir, sizeof rt_config_dir, "%s", dir_configs); userdir_make(rt_config_dir); }
        if (dir_logs) { snprintf(logs_dir, sizeof logs_dir, "%s", dir_logs); userdir_make(logs_dir); }
    } else snprintf(logs_dir, sizeof logs_dir, "%s", work);
    if (log_path) snprintf(path, sizeof path, "%s", log_path);
    else snprintf(path, sizeof path, "%s/a3mx.log", logs_dir);
    log_file = fopen(path, "w");
    snprintf(rt_disc_root, sizeof rt_disc_root, "%s/iso", work);
    if (!save_dir) {
        save_dir = saves_default;
        if (dir_saves || dir_configs || dir_logs) rt_log("folders: saves %s, configs %s, logs %s", save_dir, rt_config_dir, logs_dir);
        if (user_kind == 1) rt_log("user folder: %s (saves, configs, logs, launcher)", user_root);
        else if (user_kind == 2) rt_warn("user folder: the system's folder can't be written, so saves, configs and logs are kept beside the game, in %s", user_root);
        else rt_warn("user folder: neither the system's folder nor %s can be written: saves and settings will not be kept", user_root);
        char old_saves[1200];
        snprintf(old_saves, sizeof old_saves, "%s/saves", beside);
        move_saves_from(old_saves, save_dir, rt_config_dir);     /* 0.44.0 - 0.55.0: saves/ beside the work folder */
        move_saves_from(work, save_dir, rt_config_dir);          /* before 0.44.0: the work folder */
    }
    snprintf(rt_save_root, sizeof rt_save_root, "%s/ms0", save_dir);
    if (!no_assets) {
        extern char rt_assets_root[1024];
        if (assets) snprintf(rt_assets_root, sizeof rt_assets_root, "%s", assets);
        else {   /* assets/<region> beside the work folder: the region from work/disc.json */
            int json_file_string(const char *path, const char *key, char *out, size_t outn);
            char region[16] = "us", parent[1024];
            snprintf(path, sizeof path, "%s/disc.json", work);
            json_file_string(path, "region", region, sizeof region);
            parent_of(work, parent, sizeof parent);
            snprintf(rt_assets_root, sizeof rt_assets_root, "%sassets/%s", parent, region);
            /* no folder (index.json since the 0.37.0 layout, pac/ before it): the disc only */
            char index[1100], pac[1100];
            snprintf(index, sizeof index, "%s/index.json", rt_assets_root);
            snprintf(pac, sizeof pac, "%s/pac", rt_assets_root);
            if (file_size(index) < 0 && file_size(pac) < 0) rt_assets_root[0] = 0;
        }
    }
    {   /* saves and settings from releases before 0.55.0 were sfa3max.cfg / sfa3max.dat: take them over, once */
        static const char *names[][2] = { { "sfa3max.cfg", "a3mx.cfg" }, { "sfa3max.dat", "a3mx.dat" } };
        for (int k = 0; k < 2; k++) {
            char from[1200], to[1200];
            const char *dir = k == 0 && rt_config_dir[0] ? rt_config_dir : save_dir;   /* the .cfg: with the settings */
            snprintf(from, sizeof from, "%s/%s", dir, names[k][0]);
            snprintf(to, sizeof to, "%s/%s", dir, names[k][1]);
            if (file_size(from) >= 0 && file_size(to) < 0 && rename(from, to) == 0) rt_log("renamed %s to %s", from, to);
        }
    }
    rt_log("A3MX %s", A3MX_VERSION);   /* the first thing a bug report's log says */
    if (renderer_asked) rt_warn("renderer: %s is not available in this version (only OpenGL and the software renderer are): using OpenGL", renderer_asked);
    {   /* which features are on (ext_features.h): --features, --no-extensions, or the defaults */
        const char *bad = NULL;
        if (!features_set(features_arg, &bad)) rt_fatal("--features: %s is not a feature (saves, unlocks, menus, local_multiplayer, hud, quit_key, widescreen, stage_extensions, shaders; all, none, default)", bad);
        if (rt_custom_assets) features_add(FEAT_STAGE_EXT);
    }
    rt_work_dir = work;
    if (rt_extensions) { void settings_load(void); rt_log("features: %s", features_describe()); settings_load(); }
    if (filter_arg) {                         /* --filter NAME: this run only (the setting is SCREEN SETTING > FILTER) */
        extern int rt_filter_cli; int found = -1;
        for (int k = 0; k < SHADER_FILTER_COUNT; k++) {               /* letters and digits only, any case: crt-curved is CRT CURVED */
            char a[24], b[24]; const char *src[2] = { filter_arg, SHADER_FILTERS[k].name }; char *dst[2] = { a, b };
            for (int w = 0; w < 2; w++) {
                size_t n = 0;
                for (const char *p = src[w]; *p && n < sizeof a - 1; p++) {
                    char ch = *p >= 'a' && *p <= 'z' ? (char)(*p - 32) : *p;
                    if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) dst[w][n++] = ch;
                }
                dst[w][n] = 0;
            }
            if (!strcmp(a, b)) found = k;
        }
        if (found < 0) rt_fatal("--filter: off, crt-curved, crt-lottes, crt-royale, crt-guest, crt-light, scan-light, scan-strong, scan-ossc, scan-dark, xbrz or scalefx, not %s", filter_arg);
        rt_filter_cli = found;
    }
    else rt_log("extensions: off (%s): the PSP program alone", rt_extensions_built() ? "no feature is on (--no-extensions)" : "built without them");

    if (!rt_mem_init()) rt_fatal("could not reserve guest memory");
    snprintf(path, sizeof path, "%s/analysis.image.bin", work);
    long n = file_size(path);
    FILE *f = fopen(path, "rb");
    if (!f || n <= 0) rt_fatal("%s not found: run tools/prepare.py first", path);
    if (fread(HOST(0x08804000), 1, (size_t)n, f) != (size_t)n) rt_fatal("short read on %s", path);
    fclose(f);
    rt_log("loaded program image: %ld bytes at 08804000", n);
    image_end = 0x08804000u + (uint32_t)n;
    { unsigned decomp_function_count(void); rt_log("decompiled functions: %u (src/core)", decomp_function_count()); }
    rt_reserve(USER_LO, (0x08804000u + (uint32_t)n + 0xFFFF) & ~0xFFFFu, "program");

    /* In a window the software renderer's frames are still shown through OpenGL when it is there
       (ge_gl_present_rgba: sharp bilinear, so rows and columns stay even at any window size, and the
       same VSYNC / FRAME TIMING as the GPU renderer); only the drawing is done in software. */
    if (!host_init(headless, !want_soft || !headless)) rt_fatal("could not open the window (SDL)");
    if (!want_soft) {
        HostGetProc gp = headless ? host_headless_gl() : host_gl_getproc();
        /* --scale N, or 0 for auto (ge_gl_init: enough to cover the monitor, at most 4) */
        if (gp && ge_gl_init(gp, render_scale, host_display_height())) { renderer_gl_ready = 1; ge_use_gl = 1; }
        else rt_log("GPU renderer unavailable: using the software renderer");
    } else if (!headless) {
        HostGetProc gp = host_gl_getproc();
        if (gp && ge_gl_init(gp, 1, host_display_height())) { renderer_gl_ready = 1; ge_use_gl = 0; rt_log("software renderer, shown through OpenGL"); }
    }
    if (ge_use_gl) perf_set_renderer("GPU (OpenGL)", ge_gl_gpu_name(), ge_gl_scale(), ge_gl_scale_reason());
    else perf_set_renderer(want_soft ? "software" : "software (GPU unavailable)", "", 1, "");
    if (difftest_n > 0) { int decomp_difftest(int); int failed = decomp_difftest(difftest_n); fflush(stdout); return failed ? 1 : 0; }
    rt_kernel_init(); rt_io_init(); rt_ge_init();
    rt_net_init();
    if (FEATURE(MULTIPLAYER)) { void netplay_init(void); netplay_init(); }
    rt_net_on_pad = on_pad; rt_net_on_request = on_request; rt_net_on_hello = on_hello; rt_net_on_denied = on_denied;
    if (net_hub_port >= 0 && !net_link_hub(net_hub_port)) rt_fatal("could not start the local network hub");
    if (net_join_port && !net_link_join(net_join_port, net_node)) rt_fatal("could not join the local network hub");

    /* module_start(argsize, argp) runs on a root thread, as the PSP's loader does */
    const char *bootpath = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN";
    uint32_t argp = rt_alloc(256, 0, 16, "args");
    strcpy((char *)HOST(argp), bootpath);
    uint32_t root = rt_thread_create("root", 0x08804000, 0x20, 0x4000, 0x80000000u);
    rt_thread_start(root, (uint32_t)strlen(bootpath) + 1, argp);

    rt_run();
    if (rt_extensions) { void store_shutdown(void); store_shutdown(); }   /* keep the settings as they are now */
    rt_report();
    perf_report(logs_dir);
    if (hash_log) fclose(hash_log);
    if (decomp_report) { int decomp_write_report(const char *); if (!decomp_write_report(decomp_report)) rt_warn("cannot write %s", decomp_report); }
    host_shutdown();
    if (log_file) fclose(log_file);
    return 0;
}
