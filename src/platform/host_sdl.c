/* SDL3 host: a resizable window (letterboxed 480x272, sharp pixels), keyboard and gamepads, audio.
 *
 * Keys (player 1):  arrows = d-pad    Z = cross   X = circle   A = square   S = triangle
 *                   Q = L   W = R      Enter = start   Backspace = select
 *                   F11 or Alt+Enter = full screen   F2 = back to the MODE MENU (no save)
 *                   quit: the window's close button, Alt+F4 (Windows), Cmd+Q (macOS), Ctrl+Q (Linux)
 * Gamepads: the first pad joins player 1 (alongside the keyboard), the next ones players 2-4.
 *   south = cross, east = circle, west = square, north = triangle, shoulders = L/R,
 *   d-pad or left stick = directions, start/back = start/select.
 *
 * That is the automatic arrangement, used when there is no configs/controllers.cfg. With one (written
 * by the launcher's Controllers tab: controls.h) each player has the keys and the controller it names,
 * and nothing else: "bring your own device" (byod_*, below).
 */
#ifdef A3MX_SDL
#include "host.h"
#include <SDL3/SDL.h>
void rt_log(const char *fmt, ...);
#include <string.h>
#include "controls.h"

static SDL_Window *win;
static SDL_GLContext glctx;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_AudioDeviceID adev;
static SDL_AudioStream *astream[8];
static SDL_Gamepad *pads_sdl[4];
static int headless_mode, quit_req, fullscreen, menu_req;

static void open_audio(void) {
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 44100 };
    adev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (adev) {
        for (int i = 0; i < 8; i++) { astream[i] = SDL_CreateAudioStream(&spec, &spec); SDL_BindAudioStream(adev, astream[i]); }
    }
}

static void byod_load(void);
int host_init(int headless, int want_gl) {
    headless_mode = headless;
    if (headless) return 1;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 0; }
    byod_load();                               /* configs/controllers.cfg, if the launcher wrote one */
    /* default window: the largest multiple of 480x272 that fits comfortably on the monitor */
    int dh = host_display_height(), mult = dh ? (dh * 3 / 4) / 272 : 2;
    if (mult < 1) mult = 1;
    const char *title = "A3MX";
    /* full screen by default (the desktop's own mode: no mode change); --windowed for a window. F11 /
       Alt+Enter switch either way. The window size above is what a window gets. */
    fullscreen = host_start_fullscreen;
    SDL_WindowFlags full = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    if (want_gl) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);   /* macOS gives 3.2+ core contexts only so */
#endif
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        win = SDL_CreateWindow(title, 480 * mult, 272 * mult, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | full);
        if (win) glctx = SDL_GL_CreateContext(win);
        if (glctx) {
            SDL_GL_MakeCurrent(win, glctx);
            /* VSYNC (SCREEN SETTING vsync, default ON): the swap waits for the monitor, so no tearing. The
               runtime still paces the game to 59.94 Hz itself, so a 120 / 144 Hz monitor does not
               speed it up; adaptive vsync (-1) first, so a late frame tears instead of waiting a
               whole refresh */
            extern int rt_vsync;
            host_set_vsync(rt_vsync);
            rt_log("window: %s", fullscreen ? "full screen (F11 or Alt+Enter: a window)" : "a window (F11 or Alt+Enter: full screen)");
            open_audio();
            return 1;
        }
        SDL_Log("OpenGL 3.3 not available (%s): using the software renderer", SDL_GetError());
        if (win) SDL_DestroyWindow(win);
        win = NULL;
    }
    win = SDL_CreateWindow(title, 480 * mult, 272 * mult, SDL_WINDOW_RESIZABLE | full);
    if (!win) return 0;
    ren = SDL_CreateRenderer(win, NULL);
    if (!ren) return 0;
    { extern int rt_vsync; host_set_vsync(rt_vsync); }
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 480, 272);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    rt_log("window: %s", fullscreen ? "full screen (F11 or Alt+Enter: a window)" : "a window (F11 or Alt+Enter: full screen)");
    open_audio();
    return 1;
}

static void *gl_proc(const char *name) { return (void *)SDL_GL_GetProcAddress(name); }
HostGetProc host_gl_getproc(void) { return glctx ? gl_proc : NULL; }
/* Test aid: A3MX_TEST_VBLANK_HZ=60 makes every swap wait for a simulated vblank at that rate (from a
   fixed start), and host_refresh_hz report it, so FRAME TIMING SMOOTH can be measured on a machine whose
   swap doesn't wait (a headless Xvfb). A3MX_TEST_VBLANK_HZ=0 reports a rate but doesn't wait (the
   fallback). The swap honours the swap interval as a driver does (no sooner than that many vblanks after
   the last swap); A3MX_TEST_SWAP_INTERVAL_MAX=1 makes it ignore anything above 1, as some drivers do.
   A3MX_TEST_SWAP_FROM_CALL=1 counts the interval from the swap instead (n vblanks after it is asked
   for: the full-screen case pacing.c's missed-refresh fallback is for, 0.48.2). Not for players. */
static double test_vblank_hz = -1;
static int test_vblank_wait, test_interval = 1, test_interval_max = 64, test_from_call;
static void test_vblank_init(void) {
    if (test_vblank_hz != -1) return;
    const char *e = SDL_getenv("A3MX_TEST_VBLANK_HZ");
    test_vblank_hz = e ? SDL_atof(e) : 0;
    test_vblank_wait = test_vblank_hz > 0;
    if (e && test_vblank_hz <= 0) test_vblank_hz = 60;
    const char *m = SDL_getenv("A3MX_TEST_SWAP_INTERVAL_MAX");
    if (m && SDL_atoi(m) > 0) test_interval_max = SDL_atoi(m);
    const char *fc = SDL_getenv("A3MX_TEST_SWAP_FROM_CALL"); test_from_call = fc && SDL_atoi(fc) > 0;
    if (e) rt_log("test: a simulated %.2f Hz monitor (%s%s)", test_vblank_hz, test_vblank_wait ? "the swap waits for it" : "the swap doesn't wait",
                  m ? ", swap intervals above that ignored" : "");
    if (!e) test_vblank_hz = 0;
}
static uint64_t test_submitted;               /* when the swap was asked for: the frame goes out at the next vblank after it */
static void test_vblank_submit(void) { test_submitted = SDL_GetTicksNS(); }
static void test_vblank_wait_now(void) {
    test_vblank_init();
    extern int rt_vsync;
    if (!test_vblank_wait || !rt_vsync) return;
    static uint64_t t0, last_k; uint64_t now = test_submitted ? test_submitted : SDL_GetTicksNS();
    if (!t0) t0 = now;
    double period = 1e9 / test_vblank_hz;
    uint64_t k = (uint64_t)((double)(now - t0) / period) + 1;
    int n = test_interval < test_interval_max ? test_interval : test_interval_max;
    if (test_from_call) k += (uint64_t)(n - 1);                                   /* n vblanks from the swap */
    else if (last_k && n > 1 && k < last_k + (uint64_t)n) k = last_k + (uint64_t)n;   /* no sooner than n vblanks after the last swap */
    last_k = k;
    uint64_t until = t0 + (uint64_t)((double)k * period);
    while (SDL_GetTicksNS() < until) {}
}
/* Windows: some drivers' OpenGL swap doesn't wait for the refresh when the desktop compositor (DWM)
   shows the window, in a window and often in borderless full screen too (seen on an RTX 4070 laptop:
   "the swap doesn't wait for the monitor"). Then pacing.c asks for a wait on the compositor instead:
   DwmFlush, which returns when the compositor has shown its next frame, once a refresh. Only after the
   swap was seen not to wait, so a swap that does wait is never waited on twice. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HRESULT (WINAPI *DwmFlushFn)(void);
static DwmFlushFn dwm_flush;
static int dwm_tried;
#endif
static int compositor_wait;
int host_compositor_wait(int on) {
#ifdef _WIN32
    if (on && !dwm_tried) {
        dwm_tried = 1;
        HMODULE m = LoadLibraryA("dwmapi.dll");
        if (m) dwm_flush = (DwmFlushFn)(void *)GetProcAddress(m, "DwmFlush");
    }
    compositor_wait = on && dwm_flush && glctx;
#else
    compositor_wait = 0; (void)on;
#endif
    return compositor_wait;
}

void host_gl_swap(void) {
    if (!glctx) return;
    test_vblank_submit();
    SDL_GL_SwapWindow(win);
    /* FRAME TIMING SMOOTH: wait until the frame is really out, so the swap comes back at the refresh
       (pacing.c times the next frame from it) and the driver can't queue frames ahead (input lag) */
    { int pacing_swap_paced(void); if (pacing_swap_paced()) { void ge_gl_finish(void); ge_gl_finish(); } }
#ifdef _WIN32
    if (compositor_wait && dwm_flush) dwm_flush();
#endif
    test_vblank_wait_now();
}
double host_refresh_hz(void) {
    test_vblank_init();
    if (test_vblank_hz > 0) return test_vblank_hz;
    SDL_DisplayID d = win ? SDL_GetDisplayForWindow(win) : SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *m = d ? SDL_GetCurrentDisplayMode(d) : NULL;
    return m ? (double)m->refresh_rate : 0;
}
void host_set_swap_interval(int n) {
    extern int rt_vsync;
    if (!rt_vsync) n = 0;
    test_interval = n > 0 ? n : 1;
    if (glctx) {
        if (n == 0 || !SDL_GL_SetSwapInterval(-n)) SDL_GL_SetSwapInterval(n);
        int si = 0; SDL_GL_GetSwapInterval(&si);
        rt_log("GL: swap interval %d", si);
    } else if (ren && !SDL_SetRenderVSync(ren, n) && n > 1) {
        /* SDL's renderer may only offer 1: pacing.c's timing still lands each frame on the n-th refresh */
        rt_log("vsync: the renderer can't wait for %d refreshes (%s): waiting for the next one", n, SDL_GetError());
        SDL_SetRenderVSync(ren, 1);
    }
}
void host_audio_ratio(double r) {
    for (int i = 0; i < 8; i++) if (astream[i]) SDL_SetAudioStreamFrequencyRatio(astream[i], (float)r);
}
/* VSYNC: at start, and when SCREEN SETTING > VSYNC changes (the swap interval can change at any time) */
/* Whether the display can wait for the monitor at all: found out the first time VSYNC is set (asked
   for once even when the setting is OFF, then put back). 0 when the driver turns the request down;
   SCREEN SETTING then greys VSYNC out (and FRAME TIMING with it) and the saved value is just not used. */
static int vsync_ok = -1;
int host_vsync_available(void) { return vsync_ok != 0; }
void host_set_vsync(int on) {
    test_interval = 1;
    if (vsync_ok < 0 && (glctx || ren)) {
        vsync_ok = glctx ? (SDL_GL_SetSwapInterval(-1) || SDL_GL_SetSwapInterval(1)) : SDL_SetRenderVSync(ren, 1);
        if (!vsync_ok) rt_log("vsync: this display can't wait for the monitor (%s): VSYNC is greyed out in SCREEN SETTING", SDL_GetError());
    }
    if (glctx) {
        if (!on || !SDL_GL_SetSwapInterval(-1)) SDL_GL_SetSwapInterval(on ? 1 : 0);
        int si = 0; SDL_GL_GetSwapInterval(&si);
        rt_log("GL: vsync %s (swap interval %d), monitor %.2f Hz", on ? "on" : "off", si, host_refresh_hz());
    } else if (ren) {
        SDL_SetRenderVSync(ren, on ? 1 : 0);
        rt_log("vsync %s", on ? "on" : "off");
    }
}
void host_window_size(int *w, int *h) { *w = 480; *h = 272; if (win) SDL_GetWindowSizeInPixels(win, w, h); }
int host_display_height(void) {
    const SDL_DisplayMode *m = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    return m ? (int)(m->h * m->pixel_density) : 0;
}

/* Without OpenGL: SDL's own renderer. Sharp bilinear, as ge_gl's presentation: the frame is first
   scaled up by a whole number with nearest (every pixel the same size), then that is scaled the rest
   of the way to the letterboxed window with linear filtering. Nearest straight to a size that isn't a
   whole multiple of 480 x 272 (1080 / 272 = 3.97) makes some rows one pixel taller than the rest,
   which shows as doubled lines. */
static SDL_Texture *sharp;
static int sharp_k;
void host_present(const uint8_t *rgba, int w, int h) {
    if (headless_mode || !ren) return;   /* the OpenGL path presents through ge_gl */
    test_vblank_submit();
    SDL_UpdateTexture(tex, NULL, rgba, w * 4);
    int ow = 480, oh = 272;
    SDL_GetCurrentRenderOutputSize(ren, &ow, &oh);
    float sc = (float)ow / 480.f < (float)oh / 272.f ? (float)ow / 480.f : (float)oh / 272.f;
    SDL_FRect dst = { ((float)ow - 480.f * sc) / 2.f, ((float)oh - 272.f * sc) / 2.f, 480.f * sc, 272.f * sc };
    int k = (int)sc < 1 ? 1 : (int)sc;
    SDL_Texture *src = tex;
    if ((float)k != sc) {                    /* not a whole multiple: through the pre-scaled copy */
        if (!sharp || sharp_k != k) {
            if (sharp) SDL_DestroyTexture(sharp);
            sharp = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 480 * k, 272 * k);
            if (sharp) SDL_SetTextureScaleMode(sharp, SDL_SCALEMODE_LINEAR);
            sharp_k = k;
        }
        if (sharp && SDL_SetRenderTarget(ren, sharp)) {
            SDL_RenderTexture(ren, tex, NULL, NULL);
            SDL_SetRenderTarget(ren, NULL);
            src = sharp;
        }
    }
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    SDL_RenderTexture(ren, src, NULL, &dst);
    SDL_RenderPresent(ren);
    test_vblank_wait_now();
    (void)h;
}

/* Which player a gamepad is: the controller's own player number when it has one (the XInput slot,
 * the lit LED), so pad 1 is player 1 whatever order Windows lists the pads in; otherwise the first
 * free slot. A pad already in another slot is moved to its own. */
/* Pads are ordered by the player number each controller reports (its XInput slot / lit LED), then by
   when it was connected, with no gaps: a first pad that reports player 2 (slot 1 taken by a device that
   is not a gamepad, say) is still pad 1. */
static int pad_rank[4], pads_seen;
static void order_pads(void) {
    for (int i = 0; i < 4; i++) for (int j = i + 1; j < 4; j++) {
        int ri = pads_sdl[i] ? pad_rank[i] : 1 << 30, rj = pads_sdl[j] ? pad_rank[j] : 1 << 30;
        if (rj < ri) { SDL_Gamepad *g = pads_sdl[i]; pads_sdl[i] = pads_sdl[j]; pads_sdl[j] = g; int r = pad_rank[i]; pad_rank[i] = pad_rank[j]; pad_rank[j] = r; }
    }
    for (int i = 0; i < 4 && pads_sdl[i]; i++) rt_log("gamepad: pad %d is %s", i + 1, SDL_GetGamepadName(pads_sdl[i]) ? SDL_GetGamepadName(pads_sdl[i]) : "?");
}
static void open_pad(SDL_JoystickID id) {
    SDL_Gamepad *g = SDL_OpenGamepad(id);
    if (!g) return;
    int want = SDL_GetGamepadPlayerIndex(g);
    rt_log("gamepad: %s (id %u), player index %d", SDL_GetGamepadName(g) ? SDL_GetGamepadName(g) : "?", (unsigned)id, want);
    for (int i = 0; i < 4; i++) if (!pads_sdl[i]) { pads_sdl[i] = g; pad_rank[i] = (want >= 0 ? want : 8) * 100 + pads_seen++; order_pads(); return; }
    SDL_CloseGamepad(g);
}

/* ---- bring your own device: configs/controllers.cfg (controls.h) ----
 * Each player's keys are scancodes; its controller is found among the open ones by GUID (and which of
 * several the same), again whenever one is plugged in or out. A controller no player names does
 * nothing... unless it can be given to a player that isn't set at all (byod_assign). The left stick always counts as the d-pad for the player whose controller it is, as in the
 * automatic arrangement, and is passed on as the analogue stick. */
extern char rt_config_dir[1024];              /* main.c: the player's configs/ ("" with --save-dir: no file is read) */
static int byod;                              /* controllers.cfg was read */
static Controls ctl;
static SDL_Scancode byod_key[CONTROLS_PLAYERS][CB_COUNT];
static struct { int kind, code, dir; } byod_pad[CONTROLS_PLAYERS][CB_COUNT];   /* kind 1 a button, 2 an axis (dir +1 / -1) */
static SDL_Gamepad *byod_dev[CONTROLS_PLAYERS];     /* each player's named controller, when plugged in */
static SDL_Gamepad *byod_auto[CONTROLS_PLAYERS];    /* a controller no player names, given to a player waiting for one */
static int byod_buttons[CONTROLS_PLAYERS];          /* the player's controller buttons that are set */

static void byod_assign(void) {               /* which open controller is each player's */
    for (int k = 0; k < CONTROLS_PLAYERS; k++) {
        SDL_Gamepad *was = byod_dev[k];
        byod_dev[k] = NULL;
        if (!controls_uses_pad(&ctl.p[k]) || !ctl.p[k].pad_guid[0]) continue;
        int seen = 0;
        for (int i = 0; i < 4 && !byod_dev[k]; i++) {
            if (!pads_sdl[i]) continue;
            char guid[40];
            SDL_GUIDToString(SDL_GetGamepadGUIDForID(SDL_GetGamepadID(pads_sdl[i])), guid, sizeof guid);
            if (strcmp(guid, ctl.p[k].pad_guid)) continue;
            if (seen++ == ctl.p[k].pad_index) byod_dev[k] = pads_sdl[i];
        }
        if (byod_dev[k] != was)
            rt_log("controls: player %d's controller %s%s", k + 1, byod_dev[k] ? "is " : "is not plugged in: ",
                   byod_dev[k] ? (SDL_GetGamepadName(byod_dev[k]) ? SDL_GetGamepadName(byod_dev[k]) : "?") : ctl.p[k].pad_name);
    }
    /* Controllers no player names go, in the order they were plugged in, to the automatic players,
       lowest first: with player 1 on the keyboard and player 3 on a named controller, the next
       controller plugged in is player 2. They use the usual layout (pad_bits). A player keeps the
       controller it was given while it stays plugged in. */
    for (int k = 0; k < CONTROLS_PLAYERS; k++) {
        int still = 0;
        for (int i = 0; i < 4; i++) if (pads_sdl[i] && pads_sdl[i] == byod_auto[k]) still = 1;
        for (int j = 0; j < CONTROLS_PLAYERS; j++) if (byod_dev[j] && byod_dev[j] == byod_auto[k]) still = 0;   /* now named by someone */
        if (!still || !controls_takes_free_pad(&ctl.p[k])) byod_auto[k] = NULL;
    }
    for (int i = 0; i < 4; i++) {
        SDL_Gamepad *g = pads_sdl[i];
        if (!g) continue;
        int taken = 0;
        for (int k = 0; k < CONTROLS_PLAYERS; k++) if (byod_dev[k] == g || byod_auto[k] == g) taken = 1;
        if (taken) continue;
        for (int k = 0; k < CONTROLS_PLAYERS; k++)
            if (controls_takes_free_pad(&ctl.p[k]) && !byod_auto[k]) {
                byod_auto[k] = g;
                rt_log("controls: %s is player %d (%s)", SDL_GetGamepadName(g) ? SDL_GetGamepadName(g) : "a controller", k + 1,
                       ctl.p[k].type == CT_AUTO ? "automatic" : "the first controller no player names");
                break;
            }
    }
}
static void byod_load(void) {
    char path[1200];
    if (!rt_config_dir[0]) return;
    snprintf(path, sizeof path, "%s/controllers.cfg", rt_config_dir);
    if (!controls_load(&ctl, path)) {             /* none yet: player 1 the keyboard and the first controller, the rest automatic */
        controls_defaults(&ctl);
        rt_log("controls: no %s: player 1 Keyboard/Controller (the usual keys and the first controller), players 2 to 4 automatic (the next controllers)", path);
    }
    byod = 1;
    for (int k = 0; k < CONTROLS_PLAYERS; k++) {
        int keys = 0, buttons = 0;
        for (int b = 0; b < CB_COUNT; b++) {
            /* keys: the player's own (Keyboard, Keyboard/Controller), or the usual ones for automatic player 1 */
            const char *kn = controls_uses_keys(&ctl.p[k]) ? ctl.p[k].key[b] : controls_auto_keys(&ctl, k) ? CONTROL_DEFAULT_KEYS[b] : "";
            byod_key[k][b] = kn[0] ? SDL_GetScancodeFromName(kn) : SDL_SCANCODE_UNKNOWN;
            if (kn[0] && byod_key[k][b] == SDL_SCANCODE_UNKNOWN) rt_log("controls: player %d, %s: no key is called \"%s\"", k + 1, CONTROL_BUTTONS[b].label, kn);
            if (byod_key[k][b] != SDL_SCANCODE_UNKNOWN) keys++;
            const char *s = controls_uses_pad(&ctl.p[k]) ? ctl.p[k].pad[b] : "";
            byod_pad[k][b].kind = 0;
            if (!strncmp(s, "b:", 2)) {
                SDL_GamepadButton btn = SDL_GetGamepadButtonFromString(s + 2);
                if (btn != SDL_GAMEPAD_BUTTON_INVALID) { byod_pad[k][b].kind = 1; byod_pad[k][b].code = (int)btn; }
            } else if (!strncmp(s, "a:", 2)) {
                char name[32]; snprintf(name, sizeof name, "%s", s + 2);
                size_t n = strlen(name);
                int dir = n && name[n - 1] == '-' ? -1 : 1;
                if (n && (name[n - 1] == '+' || name[n - 1] == '-')) name[n - 1] = 0;
                SDL_GamepadAxis ax = SDL_GetGamepadAxisFromString(name);
                if (ax != SDL_GAMEPAD_AXIS_INVALID) { byod_pad[k][b].kind = 2; byod_pad[k][b].code = (int)ax; byod_pad[k][b].dir = dir; }
            }
            if (s[0] && !byod_pad[k][b].kind) rt_log("controls: player %d, %s: \"%s\" is not a controller button", k + 1, CONTROL_BUTTONS[b].label, s);
            if (byod_pad[k][b].kind) buttons++;
        }
        rt_log("controls: player %d: %s, %d keys%s%s", k + 1, CONTROL_TYPE_LABELS[ctl.p[k].type], keys,
               controls_uses_pad(&ctl.p[k]) && ctl.p[k].pad_guid[0] ? ", controller " : "", controls_uses_pad(&ctl.p[k]) ? ctl.p[k].pad_name : "");
        byod_buttons[k] = buttons;
    }
    rt_log("controls: %s", path);
}
static uint32_t pad_bits(SDL_Gamepad *g, uint8_t stick[2]);
static uint32_t byod_bits(int k, const bool *keys, uint8_t stick[2]) {
    uint32_t b = 0;
    for (int i = 0; i < CB_COUNT; i++)
        if (byod_key[k][i] != SDL_SCANCODE_UNKNOWN && keys[byod_key[k][i]]) {
            if (byod_key[k][i] == SDL_SCANCODE_RETURN && (SDL_GetModState() & SDL_KMOD_ALT)) continue;   /* Alt+Enter is full screen */
            b |= CONTROL_BUTTONS[i].bit;
        }
    /* its named controller with its buttons; a free one (none named) with its buttons too for a
       Controller or Keyboard/Controller player that has some set, else with the usual layout */
    SDL_Gamepad *g = byod_dev[k] ? byod_dev[k] : controls_uses_pad(&ctl.p[k]) && byod_buttons[k] ? byod_auto[k] : NULL;
    if (!g) return byod_auto[k] ? b | pad_bits(byod_auto[k], stick) : b;
    for (int i = 0; i < CB_COUNT; i++) {
        if (byod_pad[k][i].kind == 1) { if (SDL_GetGamepadButton(g, (SDL_GamepadButton)byod_pad[k][i].code)) b |= CONTROL_BUTTONS[i].bit; }
        else if (byod_pad[k][i].kind == 2) { if (SDL_GetGamepadAxis(g, (SDL_GamepadAxis)byod_pad[k][i].code) * byod_pad[k][i].dir > 16000) b |= CONTROL_BUTTONS[i].bit; }
    }
    int lx = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX), ly = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY);
    stick[0] = (uint8_t)((lx + 32768) >> 8); stick[1] = (uint8_t)((ly + 32768) >> 8);
    if (lx < -16000) b |= PSP_LEFT; else if (lx > 16000) b |= PSP_RIGHT;
    if (ly < -16000) b |= PSP_UP; else if (ly > 16000) b |= PSP_DOWN;
    return b;
}

static uint32_t pad_bits(SDL_Gamepad *g, uint8_t stick[2]) {
    uint32_t b = 0;
    if (!g) return 0;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_SOUTH)) b |= PSP_CROSS;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_EAST)) b |= PSP_CIRCLE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_WEST)) b |= PSP_SQUARE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_NORTH)) b |= PSP_TRIANGLE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) b |= PSP_L;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) b |= PSP_R;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_START)) b |= PSP_START;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_BACK)) b |= PSP_SELECT;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_UP)) b |= PSP_UP;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) b |= PSP_DOWN;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) b |= PSP_LEFT;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) b |= PSP_RIGHT;
    int lx = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX), ly = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY);
    stick[0] = (uint8_t)((lx + 32768) >> 8); stick[1] = (uint8_t)((ly + 32768) >> 8);
    if (lx < -16000) b |= PSP_LEFT; else if (lx > 16000) b |= PSP_RIGHT;
    if (ly < -16000) b |= PSP_UP; else if (ly > 16000) b |= PSP_DOWN;
    if (SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16000) b |= PAD_L2;
    if (SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 16000) b |= PAD_R2;
    return b;
}

void host_poll(uint32_t pads[4], uint8_t sticks[4][2]) {
    if (headless_mode) return;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT: quit_req = 1; break;
        /* the window really entered or left full screen (also when it starts full screen), or moved to
         * another display: the swap may wait differently now, so pacing.c measures again once it settles */
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN: case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN: case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            if (e.type != SDL_EVENT_WINDOW_DISPLAY_CHANGED) fullscreen = e.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN;
            { void pacing_window_changed(void); pacing_window_changed(); }
            break;
        case SDL_EVENT_GAMEPAD_ADDED: open_pad(e.gdevice.which); if (byod) byod_assign(); break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            for (int i = 0; i < 4; i++) if (pads_sdl[i] && SDL_GetGamepadID(pads_sdl[i]) == e.gdevice.which) { SDL_CloseGamepad(pads_sdl[i]); pads_sdl[i] = NULL; }
            order_pads();
            if (byod) byod_assign();
            break;
        case SDL_EVENT_KEY_DOWN:
#if !defined(_WIN32) && !defined(__APPLE__)
            /* Linux: Ctrl+Q, as most programs there. Windows' Alt+F4, macOS's Cmd+Q and the window's
             * close button arrive as SDL_EVENT_QUIT. Esc doesn't quit: it was too easy to press. */
            if (e.key.scancode == SDL_SCANCODE_Q && (e.key.mod & SDL_KMOD_CTRL)) quit_req = 1;
#endif
            if (e.key.scancode == SDL_SCANCODE_F2 && !e.key.repeat) menu_req = 1;   /* keyboard only: no pad has it */
            if (e.key.scancode == SDL_SCANCODE_F11 || (e.key.scancode == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_ALT))) {
                fullscreen = !fullscreen; SDL_SetWindowFullscreen(win, fullscreen);
                { void pacing_window_changed(void); pacing_window_changed(); }   /* the swap may wait now, or not */
            }
            break;
        default: break;
        }
    }
    const bool *k = SDL_GetKeyboardState(NULL);
    if (byod) {                               /* each player's own keys and controller (controllers.cfg) */
        for (int i = 0; i < 4; i++) { sticks[i][0] = sticks[i][1] = 128; pads[i] = byod_bits(i, k, sticks[i]); }
        return;
    }
    uint32_t kb = 0;
    if (k[SDL_SCANCODE_UP]) kb |= PSP_UP;
    if (k[SDL_SCANCODE_DOWN]) kb |= PSP_DOWN;
    if (k[SDL_SCANCODE_LEFT]) kb |= PSP_LEFT;
    if (k[SDL_SCANCODE_RIGHT]) kb |= PSP_RIGHT;
    if (k[SDL_SCANCODE_Z]) kb |= PSP_CROSS;
    if (k[SDL_SCANCODE_X]) kb |= PSP_CIRCLE;
    if (k[SDL_SCANCODE_A]) kb |= PSP_SQUARE;
    if (k[SDL_SCANCODE_S]) kb |= PSP_TRIANGLE;
    if (k[SDL_SCANCODE_Q]) kb |= PSP_L;
    if (k[SDL_SCANCODE_W]) kb |= PSP_R;
    if (k[SDL_SCANCODE_E]) kb |= PAD_L2;
    if (k[SDL_SCANCODE_R]) kb |= PAD_R2;
    if (k[SDL_SCANCODE_RETURN] && !(SDL_GetModState() & SDL_KMOD_ALT)) kb |= PSP_START;
    if (k[SDL_SCANCODE_BACKSPACE]) kb |= PSP_SELECT;
    for (int i = 0; i < 4; i++) { sticks[i][0] = sticks[i][1] = 128; pads[i] = pad_bits(pads_sdl[i], sticks[i]); }
    pads[0] |= kb;
}

int host_quit_requested(void) { return quit_req; }
int host_menu_requested(void) { int r = menu_req; menu_req = 0; return r; }   /* F2, once per press */
/* Pad i is player i+1; player 1 also has the keyboard, so a single gamepad does not make player 2 human.
   With controllers.cfg: the player's controller is plugged in, the player has keys of its own, or an
   unnamed controller was given to it (byod_assign). This is what the MODE MENU's bubbles show. */
int host_pad_connected(int i) {
    if (i < 0 || i >= 4) return 0;
    if (!byod) return pads_sdl[i] != NULL;
    if (byod_dev[i] || byod_auto[i]) return 1;
    for (int b = 0; b < CB_COUNT; b++) if (byod_key[i][b] != SDL_SCANCODE_UNKNOWN) return 1;
    return 0;
}

#define AUDIO_MAX_QUEUED  (44100 * 4 / 10)  /* 100 ms of 16-bit stereo: more is late */
#define AUDIO_KEEP_QUEUED (44100 * 4 / 20)  /* 50 ms: what is left after a skip */
void host_audio(int ch, const int16_t *s, int frames, int stereo, int lvol, int rvol) {
    if (headless_mode || !adev || ch < 0 || ch >= 8 || !astream[ch]) return;
    /* Too much queued means the sound is late: frames ran faster than real time (a load held off screen
       at full speed) and the queue filled. In real time the game makes sound exactly as fast as it
       plays, so a full queue never drains, and every sound after it would stay that late (it was
       232 ms after the boot's loads until 0.36.2). So the oldest, stale sound is skipped, down to
       AUDIO_KEEP_QUEUED, and the new kept: the skip is about as long as the time the frames ran ahead. */
    int queued = SDL_GetAudioStreamQueued(astream[ch]);
    if (queued > AUDIO_MAX_QUEUED) {
        static uint64_t cleared[8], logged_at;
        static uint8_t junk[8192];
        int skip = (queued - AUDIO_KEEP_QUEUED) & ~3;
        cleared[ch] += (uint64_t)skip / 4;
        while (skip > 0) {                     /* (stream calls are thread-safe: the device pulls from the other end) */
            int got = SDL_GetAudioStreamData(astream[ch], junk, skip < (int)sizeof junk ? skip : (int)sizeof junk);
            if (got <= 0) break;
            skip -= got;
        }
        uint64_t now = SDL_GetTicks();
        if (now - logged_at >= 1000) {         /* at most once a second */
            logged_at = now;
            rt_log("audio: channel %d was running late, %.0f ms of queued sound skipped so far", ch, (double)cleared[ch] * 1000.0 / 44100.0);
        }
    }
    static int16_t buf[8192 * 2];
    if (frames > 8192) frames = 8192;
    for (int i = 0; i < frames; i++) {
        int l = stereo ? s[2 * i] : s[i], r = stereo ? s[2 * i + 1] : s[i];
        buf[2 * i] = (int16_t)(l * lvol / 0x8000); buf[2 * i + 1] = (int16_t)(r * rvol / 0x8000);
    }
    SDL_PutAudioStreamData(astream[ch], buf, frames * 4);
}

void host_shutdown(void) { if (!headless_mode) SDL_Quit(); }
#endif
