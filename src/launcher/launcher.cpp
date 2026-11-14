/* a3mx-launcher: the one window over tools/build.py and the game (0.60.0: it also does what
 * create-launcher did, so there is no second program and no hand-off between two windows).
 *
 *   left         the project's picture, then the tabs; under them the grey bug (a new GitLab issue,
 *                filled in) and About. The colours follow the system's light or dark setting
 *   Paths        your ISO (empty: the one in imports/psp/), checked by its program's checksum: a flag
 *                for the US, Japanese or European disc, a red mark otherwise (only the US disc builds);
 *                the folders everything lives in (the checkout, work/, the build tools, your a3mx
 *                folder: saves, configs, logs); and a button that makes a shortcut, if you want one
 *   Dependencies what the build needs, each with a green tick when it is here (deps.cpp, from
 *                tools/data/pins.json). Anything missing is listed with where it comes from, its
 *                SHA-256 and where it goes, and is downloaded only after you agree
 *   Controllers  bring your own device: for each of four players, keyboard keys and a controller with
 *                its buttons, and Test (configs/controllers.cfg, read by the game when it starts)
 *   Extensions   every extension as a feature of its own (src/platform/ext_features.h): all off is the
 *                PSP game as it was. Widescreen and Stage Extensions are here
 *   Build/Run    preset, extract again, tests; full screen, log level, renderer, resolution, GPU, extra
 *                options; Run (build if needed, then play) or Build; Stop ends a build
 *   bottom       the console, on every tab: build.py's and the game's output as it comes, whichever
 *                tab started it; also written to logs/launcher.log. Then the status and the version
 *
 * From a source checkout (tools/build.py found above the launcher, or in the current folder) the
 * buttons run build.cmd / build.sh with the chosen options, so the launcher and the command line
 * always build the same way. Without one (a launcher copied next to a game), only Run is offered: it
 * starts a3mx beside the launcher.
 *
 * Holds no game code. SDL3 for the window, file dialogs and controllers, Dear ImGui for the controls;
 * process.cpp starts and stops the child, disc_check.cpp identifies the ISO, deps.cpp reads the pinned
 * downloads, shortcut.cpp makes a shortcut, icons.cpp draws the flags and the bug, game_options.h
 * lists the game's options.
 *
 * Settings: launcher/launcher.ini in your a3mx folder (src/platform/userdir.h: %APPDATA%\a3mx and its
 * equivalents; beside the checkout if that can't be written). One from before 0.60.0
 * (saves/launcher.ini in the checkout) is read once.
 *
 * Options (for tests and screenshots): --settings FILE (instead of launcher.ini), --user-dir DIR,
 * --autostart (press the button once), --screenshot FILE.bmp --frames N (save the window after N
 * frames and quit), --root DIR (the source checkout), --theme light|dark, --tab paths|dependencies|
 * controllers|extensions|run, --show options|about (open that window, for screenshots), --gpus (print
 * the graphics cards found, fastest first, and quit; A3MX_TEST_SYSFS / A3MX_TEST_PCI_IDS stand in for
 * /sys and pci.ids in tests).
 */
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#ifndef _WIN32
#include <csignal>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <climits>
#endif

#include "disc_check.h"
#include "game_options.h"
#include "gpu.h"
#include "deps.h"
#include "shortcut.h"
#include "logo_png.h"                 /* generated: res/launcher_logo.png's bytes (CMakeLists.txt) */
extern "C" {
#include "png_decode.h"               /* src/platform: the project's own PNG reader */
}
#include "userdir.h"                  /* src/platform: your a3mx folder */
#include "controls.h"                 /* src/platform: configs/controllers.cfg */
#include "padicon_art.h"              /* src/platform: the MODE MENU's player bubbles */
#define FEATURES_TABLE_ONLY
#include "ext_features.h"             /* src/platform: the extensions, one flag each */

/* how a program is closed on this system (the game has no quit key of its own: host_sdl.c) */
#if defined(_WIN32)
#define QUIT_KEYS "Alt+F4"
#elif defined(__APPLE__)
#define QUIT_KEYS "Cmd+Q"
#else
#define QUIT_KEYS "Ctrl+Q"
#endif
#include "icons.h"
#include "process.h"

#ifndef A3MX_VERSION
#define A3MX_VERSION "v?"
#endif

/* the project on GitLab (.recomp.json has the same addresses) */
static const char *const REPO_URL = "https://gitlab.com/theweathervane-group/a3mx";
static const char *const NEW_ISSUE = "/-/work_items/new?type=Issue&initialCreationContext=list-route";   /* as .recomp.json's link */

/* ================================================================ files and settings */
static bool is_file(const std::string &p) {
    SDL_PathInfo info;
    return !p.empty() && SDL_GetPathInfo(p.c_str(), &info) && info.type == SDL_PATHTYPE_FILE;
}
static bool is_dir(const std::string &p) {
    SDL_PathInfo info;
    return !p.empty() && SDL_GetPathInfo(p.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY;
}
/* size and modification time, to notice a file being replaced */
static std::string stamp(const std::string &p) {
    SDL_PathInfo info;
    if (p.empty() || !SDL_GetPathInfo(p.c_str(), &info)) return std::string();
    return std::to_string((long long)info.size) + ":" + std::to_string((long long)info.modify_time);
}
static std::string join(const std::string &dir, const std::string &name) {
    if (dir.empty()) return name;
    char last = dir.back();
    return (last == '/' || last == '\\') ? dir + name : dir + "/" + name;
}
static std::string parent(const std::string &p) {
    std::string s = p;
    while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
    size_t k = s.find_last_of("/\\");
    return k == std::string::npos ? std::string() : s.substr(0, k + 1);
}
static std::string base_name(const std::string &p) {
    size_t k = p.find_last_of("/\\");
    return k == std::string::npos ? p : p.substr(k + 1);
}

/* macOS: an app opened from a downloaded (quarantined) folder is run by Gatekeeper from a read-only copy
 * under /private/var/folders/.../AppTranslocation/, away from the folder it was unzipped into, so the
 * checkout above it can't be found. Security.framework says where the original is (looked up at run
 * time: the call is not in the public headers, though every macOS since 10.12 has it). */
static bool g_translocated = false;                   /* still translocated after asking: see no_folder_hint */
static std::string untranslocated(const std::string &path) {
#ifdef __APPLE__
    if (path.find("/AppTranslocation/") == std::string::npos) return path;
    g_translocated = true;
    size_t app = path.find(".app/");
    std::string bundle = app == std::string::npos ? path : path.substr(0, app + 4), rest = app == std::string::npos ? "" : path.substr(app + 4);
    typedef CFURLRef (*Original)(CFURLRef, CFErrorRef *);
    void *sec = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY);
    Original original = sec ? (Original)dlsym(sec, "SecTranslocateCreateOriginalPathForURL") : nullptr;
    if (!original) return path;
    std::string out = path;
    CFURLRef in = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)bundle.c_str(), (CFIndex)bundle.size(), true);
    CFURLRef was = in ? original(in, nullptr) : nullptr;
    char buf[PATH_MAX];
    if (was && CFURLGetFileSystemRepresentation(was, true, (UInt8 *)buf, sizeof buf)) { out = std::string(buf) + rest; g_translocated = false; }
    if (was) CFRelease(was);
    if (in) CFRelease(in);
    return out;
#else
    return path;
#endif
}
/* why there is no checkout to build in, and what to do about it */
static std::string no_folder_hint() {
    if (g_translocated)
        return "macOS opened the launcher from a temporary copy (App Translocation), away from the folder you unzipped. "
               "In Terminal, run: xattr -dr com.apple.quarantine <that folder>, then open the launcher again.";
    return "The launcher can't find its a3mx folder (tools/build.py). Keep it in the folder you unzipped the "
           "a3mx-<version>-<system>.zip download into, and open it from there.";
}
static std::string read_text(const std::string &p) {
    size_t n = 0;
    void *d = SDL_LoadFile(p.c_str(), &n);
    if (!d) return std::string();
    std::string s((const char *)d, n);
    SDL_free(d);
    return s;
}
/* "key": "value" in a small JSON file (work/disc.json) */
static std::string json_string(const std::string &json, const char *key) {
    std::string k = std::string("\"") + key + "\"";
    size_t at = json.find(k);
    if (at == std::string::npos) return std::string();
    at = json.find(':', at + k.size());
    if (at == std::string::npos) return std::string();
    at = json.find('"', at);
    if (at == std::string::npos) return std::string();
    std::string v;
    for (size_t i = at + 1; i < json.size() && json[i] != '"'; i++) {
        if (json[i] == '\\' && i + 1 < json.size()) i++;
        v += json[i];
    }
    return v;
}
static std::string url_encode(const std::string &s) {
    static const char *hex = "0123456789ABCDEF";
    std::string o;
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c;
        else { o += '%'; o += hex[c >> 4]; o += hex[c & 15]; }
    }
    return o;
}
/* a folder as a file:// URL, for the file manager */
static std::string file_url(const std::string &dir) {
    std::string p = dir;
    for (char &c : p) if (c == '\\') c = '/';
    std::string o = "file://";
    if (!p.empty() && p[0] != '/') o += "/";          /* C:/... */
    for (unsigned char c : p) {
        if (c == ' ' || c == '#' || c == '%' || c == '?') { char b[4]; snprintf(b, sizeof b, "%%%02X", c); o += b; }
        else o += (char)c;
    }
    return o;
}

static const char *const LOG_LEVELS[] = { "OFF", "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE" };
static const char *const LOG_ARGS[]   = { "off", "error", "warn", "info", "debug", "verbose" };
static const char *const PRESETS[]    = { "Release", "Debug" };
static const char *const PRESET_ARGS[] = { "release", "debug" };
/* The renderers the game's --renderer takes. Only OpenGL and the software one exist today: the others
 * are listed so the choice and the scripts are in place, and say plainly that OpenGL is used meanwhile. */
struct RendererChoice { const char *label, *arg; bool written; };
static const RendererChoice RENDERERS[] = {
    { "OpenGL", "opengl", true },
    { "Software", "soft", true },
    { "Vulkan (not available yet: uses OpenGL)", "vulkan", false },
#if defined(_WIN32)
    { "DirectX 12 (not available yet: uses OpenGL)", "d3d12", false },
#elif defined(__APPLE__)
    { "Metal / MoltenVK (not available yet: uses OpenGL)", "metal", false },
#endif
};
static const int N_RENDERERS = (int)(sizeof RENDERERS / sizeof RENDERERS[0]);
enum { T_PATHS, T_DEPS, T_CONTROLLERS, T_EXTENSIONS, T_RUN, T_COUNT };
static const char *const TAB_NAMES[T_COUNT] = { "Paths", "Dependencies", "Controllers", "Extensions", "Build / Run" };
static const char *const TAB_ARGS[T_COUNT] = { "paths", "dependencies", "controllers", "extensions", "run" };
static const char *const SCALES[]     = { "Auto", "1x (480x272)", "2x (960x544)", "3x (1440x816)", "4x (1920x1088)",
                                          "5x (2400x1360)", "6x (2880x1632)", "7x (3360x1904)", "8x (3840x2176)" };
enum { THEME_AUTO, THEME_LIGHT, THEME_DARK };   /* always the system's; --theme forces one for screenshots */

struct Settings {
    std::string iso;
    int preset = 0;
    bool extract = false, tests = false, run = true;
    bool fullscreen = true;
    bool feature[FEAT_COUNT];       /* the Extensions tab: --features (ext_features.h) */
    int log_level = 0;              /* OFF */
    int renderer = 0;               /* an index into RENDERERS (saved by its --renderer name) */
    int scale = 0;                  /* 0: auto, else --scale N */
    std::string gpu = "auto";       /* a Gpu id (gpu.h), or auto: the fastest */
    std::string gpu_owned;          /* Windows: the exes whose GPU setting the launcher wrote, '|'-separated */
    std::string extra;
    std::string tools_dir;          /* where the build tools are downloaded ("": .tools/ in the checkout) */
    std::string saves_dir, configs_dir, logs_dir;   /* the Paths tab: each "" for its folder in your a3mx folder */
    std::string consent;            /* deps_signature() of the downloads the player agreed to ("": not asked yet) */
    int tab = T_RUN;
    int console_h = 180;
    int win_w = 1000, win_h = 740;
    Settings() { for (int f = 0; f < FEAT_COUNT; f++) feature[f] = FEATURES[f].on_by_default != 0; }
};

static void settings_load(Settings &s, const std::string &path) {
    std::string t = read_text(path);
    size_t at = 0;
    while (at < t.size()) {
        size_t e = t.find('\n', at);
        if (e == std::string::npos) e = t.size();
        std::string line = t.substr(at, e - at);
        at = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t eq = line.find('=');
        if (eq == std::string::npos || line[0] == '#') continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        int n = atoi(v.c_str());
        if (k == "iso") s.iso = v;
        else if (k == "preset") s.preset = n >= 0 && n < 2 ? n : 0;
        else if (k == "tests") s.tests = n != 0;
        else if (k == "run") s.run = n != 0;
        else if (k == "fullscreen") s.fullscreen = n != 0;
        else if (k == "log_level") s.log_level = n >= 0 && n < 6 ? n : 0;
        else if (k == "renderer") {
            s.renderer = 0;
            for (int i = 0; i < N_RENDERERS; i++) if (v == RENDERERS[i].arg) s.renderer = i;
            if (v == "1") s.renderer = 1;                           /* before 0.60.0: 0 GPU, 1 software */
        }
        else if (k == "scale") s.scale = n >= 0 && n <= 8 ? n : 0;
        else if (k == "gpu") s.gpu = v.empty() ? "auto" : v;
        else if (k == "gpu_owned") s.gpu_owned = v;
        else if (k == "extra") s.extra = v;
        else if (k == "tools_dir") s.tools_dir = v;
        else if (k == "saves_dir") s.saves_dir = v;
        else if (k == "configs_dir") s.configs_dir = v;
        else if (k == "logs_dir") s.logs_dir = v;
        else if (k == "consent") s.consent = v;
        else if (k == "tab") s.tab = n >= 0 && n < T_COUNT ? n : T_RUN;
        else if (k == "console_h" && n >= 60) s.console_h = n;
        else if (k == "win_w" && n >= 400) s.win_w = n;
        else if (k == "win_h" && n >= 200) s.win_h = n;
        else if (k.compare(0, 8, "feature.") == 0) { for (int f = 0; f < FEAT_COUNT; f++) if (k.substr(8) == FEATURES[f].name) s.feature[f] = n != 0; }
        /* before 0.60.0: Extensions (all or none), the widescreen patch, Custom Assets */
        else if (k == "extensions" && n == 0) { for (int f = 0; f < FEAT_COUNT; f++) s.feature[f] = false; }
        else if (k == "widescreen" && n == 0) s.feature[FEAT_WIDESCREEN] = false;
        else if (k == "custom_assets" && n != 0) s.feature[FEAT_STAGE_EXT] = true;
    }
}
static void settings_save(const Settings &s, const std::string &path) {
    if (path.empty()) return;
    std::string t = "# a3mx-launcher settings\n";
    t += "iso=" + s.iso + "\n";
    t += "preset=" + std::to_string(s.preset) + "\n";
    t += "tests=" + std::to_string(s.tests) + "\n";
    t += "run=" + std::to_string(s.run) + "\n";
    t += "fullscreen=" + std::to_string(s.fullscreen) + "\n";
    for (int f = 0; f < FEAT_COUNT; f++) t += std::string("feature.") + FEATURES[f].name + "=" + std::to_string(s.feature[f]) + "\n";
    t += "log_level=" + std::to_string(s.log_level) + "\n";
    t += std::string("renderer=") + RENDERERS[s.renderer].arg + "\n";
    t += "scale=" + std::to_string(s.scale) + "\n";
    t += "gpu=" + s.gpu + "\n";
    t += "gpu_owned=" + s.gpu_owned + "\n";
    t += "extra=" + s.extra + "\n";
    t += "tools_dir=" + s.tools_dir + "\n";
    t += "saves_dir=" + s.saves_dir + "\n";
    t += "configs_dir=" + s.configs_dir + "\n";
    t += "logs_dir=" + s.logs_dir + "\n";
    t += "consent=" + s.consent + "\n";
    t += "tab=" + std::to_string(s.tab) + "\n";
    t += "console_h=" + std::to_string(s.console_h) + "\n";
    t += "win_w=" + std::to_string(s.win_w) + "\n";
    t += "win_h=" + std::to_string(s.win_h) + "\n";
    std::string dir = parent(path);
    if (!dir.empty()) SDL_CreateDirectory(dir.c_str());
    SDL_SaveFile(path.c_str(), t.data(), t.size());
}

/* ================================================================ the console */
enum Kind { K_TEXT, K_STEP, K_CMD, K_WARN, K_ERROR, K_DEBUG, K_NOTE, K_GOOD };
struct Line { std::string text; Kind kind; };

struct Console {
    std::deque<Line> lines;
    std::string partial;            /* the line being written: shown, and rewritten after a lone \r */
    bool in_escape = false, cr = false;
    bool scroll_to_end = false;

    static Kind classify(const std::string &s) {
        if (s.compare(0, 3, "==>") == 0) return K_STEP;
        if (s.compare(0, 2, "> ") == 0) return K_CMD;
        if (s.find("FAILED") != std::string::npos || s.find("FATAL") != std::string::npos ||
            s.find("ERROR: ") != std::string::npos || s.find(" error:") != std::string::npos ||
            s.compare(0, 6, "error:") == 0 || s.find("Error:") != std::string::npos) return K_ERROR;
        if (s.find("WARN: ") != std::string::npos || s.find("warning:") != std::string::npos) return K_WARN;
        if (s.find("DEBUG: ") != std::string::npos) return K_DEBUG;
        return K_TEXT;
    }
    FILE *file = NULL;              /* logs/launcher.log: everything the console shows, as it comes */
    void push(const std::string &s, Kind k) {
        if (file) { fputs(s.c_str(), file); fputc('\n', file); fflush(file); }
        lines.push_back({ s, k });
        while (lines.size() > 50000) lines.pop_front();
        scroll_to_end = true;
    }
    void note(const std::string &s, Kind k = K_NOTE) { flush(); push(s, k); }
    void feed(const std::string &data) {
        for (char ch : data) {
            if (in_escape) { if ((ch >= '@' && ch <= '~') && ch != '[') in_escape = false; continue; }
            if (ch == '\x1b') { in_escape = true; continue; }        /* colour codes */
            if (cr) { cr = false; if (ch != '\n') partial.clear(); }  /* \r alone: the line starts again (progress bars) */
            if (ch == '\r') { cr = true; continue; }
            if (ch == '\n') { push(partial, classify(partial)); partial.clear(); continue; }
            if (ch == '\t') { partial += "    "; continue; }
            partial += ch;
            scroll_to_end = true;
        }
    }
    void flush() { if (!partial.empty()) { push(partial, classify(partial)); partial.clear(); } cr = false; }
    std::string all() const {
        std::string s;
        for (const Line &l : lines) { s += l.text; s += '\n'; }
        return s + partial;
    }
    /* the last lines, at most MAX bytes (for the bug report) */
    std::string tail(size_t max) const {
        std::string s = partial;
        for (size_t i = lines.size(); i-- > 0;) {
            const std::string &t = lines[i].text;
            if (s.size() + t.size() + 1 > max) break;
            s = t + "\n" + s;
        }
        return s;
    }
};

/* ================================================================ the theme */
static bool system_dark() { return SDL_GetSystemTheme() != SDL_SYSTEM_THEME_LIGHT; }   /* unknown: dark */

static void apply_theme(bool dark, float scale) {
    ImGuiStyle &style = ImGui::GetStyle();
    float font_dpi = style.FontScaleDpi;
    style = ImGuiStyle();
    if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();
    style.WindowPadding = ImVec2(14, 12);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 7);
    style.FrameRounding = 4; style.GrabRounding = 4; style.ChildRounding = 4; style.PopupRounding = 4; style.WindowRounding = 6;
    style.FrameBorderSize = 1;
    ImVec4 *c = style.Colors;
    c[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.55f, 0.20f, 1);
    if (dark) {
        c[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.13f, 1);
        c[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.17f, 1);
        c[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.25f, 1);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.27f, 0.32f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.31f, 0.37f, 1);
        c[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.35f, 0.6f);
        c[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.26f, 1);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.33f, 1);
    } else {
        c[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.97f, 1);
        c[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 1);
        c[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.93f, 0.94f, 0.97f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.89f, 0.90f, 0.95f, 1);
        c[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.75f, 0.7f);
        c[ImGuiCol_Header] = ImVec4(0.86f, 0.87f, 0.91f, 1);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.80f, 0.82f, 0.88f, 1);
        c[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.42f, 0.10f, 1);
    }
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = font_dpi;
    style.FontSizeBase = 16.0f;
}

/* ================================================================ the launcher */
static bool bug_dark = true;        /* the theme, for the bug's colours (drawn from a plain function) */

struct App {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Settings set;
    std::string settings_path;
    std::string root;               /* the source checkout (has tools/build.py), or "" */
    std::string base;               /* the launcher's own folder */
    Console console;
    Child child;
    bool game_running = false;      /* the build is over and the game is playing: Stop is off */
    Uint64 started_at = 0;
    std::string status = "Ready";
    Kind status_kind = K_TEXT;
    std::string work_line;          /* what work/ holds */
    std::string work_region;        /* its region (disc.json), "" if not extracted */
    Uint64 work_checked = 0;
    bool quit_asked = false, quit = false;
    Uint64 closing_since = 0;       /* "Close both": when the game was asked to close */
    int want_tab = -1;              /* --tab, or a button that sends the player to another tab */
    std::string user_root;          /* your a3mx folder (userdir.h): saves/, configs/, logs/, launcher/ */
    int user_kind = 0;              /* 1 the system's folder, 2 beside the game (the system's can't be written), 0 neither */
    SDL_Texture *logo = NULL; int logo_w = 0, logo_h = 0;
    SDL_Texture *bubble[4] = {};    /* the MODE MENU's player bubbles, the game's own pixels (padicon_art.h) */
    /* Dependencies */
    std::vector<Dep> deps; std::string deps_err; bool deps_checked = false;
    enum { RUN_GAME, RUN_TOOLS } running = RUN_GAME;   /* what the child is: a build / the game, or the tools download */
    bool start_after_tools = false; /* Run was pressed with tools missing: carry on when they are here */
    bool consent_modal = false;     /* the one-time "these will be downloaded" question is up */
    bool lr_modal = false;          /* the "build librashader" question is up */
    std::string cargo_path;         /* Rust's cargo, if it is on this PC (find_cargo) */
    int msvc = -1;                  /* Windows: Microsoft's C++ build tools found (1), not found (0); elsewhere 1 */
    /* Controllers */
    Controls ctl; bool ctl_file = false;      /* configs/controllers.cfg exists (else the game's automatic arrangement) */
    int ctl_player = 0;
    int cap_player = -1, cap_button = -1, cap_column = 0;   /* waiting for a key (column 0) or a controller button (1) */
    bool ctl_test = false;
    struct Pad { SDL_Gamepad *g; SDL_JoystickID id; std::string guid, name; int index; };
    std::vector<Pad> pads;
    enum { F_TOOLS, F_SAVES, F_CONFIGS, F_LOGS, F_COUNT };     /* the folders the Paths tab lets the player choose */
    std::vector<char> folder_buf[F_COUNT];
    int folder_target = F_TOOLS;
    std::string *folder_setting(int f) { return f == F_TOOLS ? &set.tools_dir : f == F_SAVES ? &set.saves_dir : f == F_CONFIGS ? &set.configs_dir : &set.logs_dir; }
    void folders_to_bufs() { for (int f = 0; f < F_COUNT; f++) { folder_buf[f].assign(1024, 0); snprintf(folder_buf[f].data(), folder_buf[f].size(), "%s", folder_setting(f)->c_str()); } }
    void folder_changed(int f) {                              /* a new place: look there from now on */
        settings_save(set, settings_path);
        if (f == F_TOOLS) deps_checked = false;
        if (f == F_CONFIGS) { ctl_file = controls_load(&ctl, controls_path().c_str()) != 0; if (!ctl_file) controls_defaults(&ctl); }
        if (f == F_LOGS) open_log();
    }
    void open_log() {                                         /* logs/launcher.log: this session's (the last one's is launcher.log.1) */
        if (console.file) { fclose(console.file); console.file = NULL; }
        SDL_CreateDirectory(user_dir("logs").c_str());
        std::string lp = join(user_dir("logs"), "launcher.log");
        SDL_RenamePath(lp.c_str(), (lp + ".1").c_str());
        console.file = fopen(lp.c_str(), "w");
        if (console.file) { std::string all = console.all(); fputs(all.c_str(), console.file); fflush(console.file); }
    }
    std::mutex dialog_mutex;
    std::string dialog_result, dialog_error, folder_result;
    bool dialog_done = false, folder_done = false;
    ImFont *mono = NULL;
    float scale = 1;
    bool dark = true;
    int theme = THEME_AUTO;         /* the system's light / dark setting, followed as it changes */
    std::string show_at_start;      /* --show options|about */
    std::vector<Gpu> gpus;          /* this PC's graphics cards, fastest first (gpu.cpp); empty: Automatic only */
    bool gpu_checked = true;        /* the last run's GPU compared with the choice (false while a run is due) */

    /* the disc check */
    DiscCheck disc;
    std::string disc_path;          /* the ISO being checked: the field, or the one found in imports/psp/ */
    std::string disc_stamp;         /* its size and time when checked */
    bool disc_from_imports = false;
    Uint64 disc_polled = 0;
    std::string imports_note;       /* when the field is empty and imports/psp/ has nothing usable */
    std::string imports_key = "?", imports_pick;   /* imports/psp/'s files when last looked at, and the one picked */

    std::vector<char> iso_buf = std::vector<char>(4096, 0);
    std::vector<char> extra_buf = std::vector<char>(1024, 0);

    std::string game_exe() const {
#ifdef _WIN32
        return join(base, "a3mx.exe");
#else
        return join(base, "a3mx");
#endif
    }
    std::string home_dir() const { return root.empty() ? base : root; }      /* the checkout, or the launcher's own folder */
    std::string work_dir() const { return join(home_dir(), "work"); }
    /* saves, configs, logs, launcher: in your a3mx folder, unless the Paths tab names another (not the
       launcher's own, which is where that choice is kept) */
    std::string user_dir(const char *sub) const {
        const std::string *o = !strcmp(sub, "saves") ? &set.saves_dir : !strcmp(sub, "configs") ? &set.configs_dir : !strcmp(sub, "logs") ? &set.logs_dir : NULL;
        return o && !o->empty() ? *o : join(user_root, sub);
    }
    std::string tools_dir() const { return set.tools_dir.empty() ? join(home_dir(), ".tools") : set.tools_dir; }
    std::string controls_path() const { return join(user_dir("configs"), "controllers.cfg"); }
    /* --features: the ticked ones by name, "none" if none (then the game is the PSP program alone) */
    std::string features_arg() const {
        std::string l;
        for (int f = 0; f < FEAT_COUNT; f++) {
            bool on = set.feature[f] && (FEATURES[f].needs < 0 || set.feature[FEATURES[f].needs]);
            if (on) l += (l.empty() ? "" : ",") + std::string(FEATURES[f].name);
        }
        return l.empty() ? "none" : l;
    }
    /* the game the button starts: beside the launcher, or the one build.py builds for the preset */
    std::string run_exe() const {
        if (root.empty()) return game_exe();
        std::string dir = set.preset == 1 ? "build-debug" : "build";
#ifdef _WIN32
        return join(join(root, dir), "a3mx.exe");
#else
        return join(join(root, dir), "a3mx");
#endif
    }
    /* the GPU combo's text for a card, and for Automatic */
    std::string gpu_label(const Gpu &g) const {
        std::string s = g.name;
        if (g.integrated) s += " (integrated)";
        return s;
    }
    std::string gpu_auto_label() const {
        if (gpus.empty()) return "Automatic";
        if (gpus.size() == 1) return "Automatic (" + gpus[0].name + ")";
        return "Automatic (fastest: " + gpus[0].name + ")";
    }

    void set_theme_now() {
        bool d = theme == THEME_AUTO ? system_dark() : theme == THEME_DARK;
        if (d != dark || ImGui::GetStyle().FontSizeBase != 16.0f) { dark = d; apply_theme(dark, scale); }
    }

    void check_work() {
        std::string j = read_text(join(work_dir(), "disc.json"));
        if (j.empty()) {
            work_region.clear();
            work_line = root.empty() ? "" : "Not extracted yet: the first Run or Build extracts your disc into work/ (a few minutes).";
            return;
        }
        work_region = json_string(j, "region");
        work_line = "In work/: " + json_string(j, "title") + " (" + json_string(j, "serial") + " v" + json_string(j, "version") + "), from " + json_string(j, "iso");
    }

    /* which ISO to check: the field's, or (empty) the US one in imports/psp/, as build.py would pick */
    void choose_disc() {
        std::string want = set.iso;
        bool imports = false;
        imports_note.clear();
        if (want.empty() && !root.empty()) {
            std::string dir = join(root, "imports/psp");
            int n = 0;
            char **found = is_dir(dir) ? SDL_GlobDirectory(dir.c_str(), "*.iso", SDL_GLOB_CASEINSENSITIVE, &n) : NULL;
            std::vector<std::string> isos;
            std::string key;
            for (int i = 0; found && i < n; i++) { isos.push_back(join(dir, found[i])); key += isos.back() + "=" + stamp(isos.back()) + ";"; }
            if (found) SDL_free(found);
            if (key != imports_key) {                 /* the folder changed: pick again (reads each program) */
                imports_key = key;
                imports_pick.clear();
                for (const std::string &p : isos) if (DiscCheck::check_now(p).verdict == DISC_OK) { imports_pick = p; break; }
                if (imports_pick.empty() && !isos.empty()) imports_pick = isos[0];   /* none is the US disc: check the first, to say why */
            }
            want = imports_pick;
            imports = !want.empty();
            if (want.empty()) imports_note = "No ISO in imports/psp/: choose yours with Browse...";
        }
        std::string st = stamp(want);
        if (want != disc_path || st != disc_stamp) {
            disc_path = want; disc_stamp = st; disc_from_imports = imports;
            disc.start(want);
        }
    }

    /* builds allowed: a checked US disc, or nothing to check but work/ already holds the US disc */
    bool disc_allows_build(DiscResult &r) {
        r = disc.result();
        if (r.verdict == DISC_OK) return true;
        if (r.verdict == DISC_EMPTY && set.iso.empty() && work_region == "us") return true;
        return false;
    }

    /* the command the button runs */
    void command(std::vector<std::string> &argv, std::string &cmdline, std::string &shown) {
        std::vector<std::string> game;
        game.push_back(set.fullscreen ? "--fullscreen" : "--windowed");
        game.push_back("--features"); game.push_back(features_arg());
        game.push_back("--user-dir"); game.push_back(user_root);          /* the same folder the launcher uses */
        if (!set.saves_dir.empty()) { game.push_back("--dir-saves"); game.push_back(set.saves_dir); }
        if (!set.configs_dir.empty()) { game.push_back("--dir-configs"); game.push_back(set.configs_dir); }
        if (!set.logs_dir.empty()) { game.push_back("--dir-logs"); game.push_back(set.logs_dir); }
        if (!set.tools_dir.empty()) {         /* FILTER's shaders and library, when the tools aren't in .tools beside the game */
            game.push_back("--shaders"); game.push_back(join(set.tools_dir, "slang-shaders"));
            std::string lib = join(join(set.tools_dir, "librashader"),
#if defined(_WIN32)
                                   "librashader.dll");
#elif defined(__APPLE__)
                                   "librashader.dylib");
#else
                                   "librashader.so");
#endif
            if (is_file(lib)) { game.push_back("--librashader"); game.push_back(lib); }
        }
        game.push_back("--log-level"); game.push_back(LOG_ARGS[set.log_level]);
        if (set.renderer != 0) { game.push_back("--renderer"); game.push_back(RENDERERS[set.renderer].arg); }
        if (set.renderer != 1 && set.scale > 0) { game.push_back("--scale"); game.push_back(std::to_string(set.scale)); }
        for (const std::string &a : split_args(set.extra)) game.push_back(a);

        std::vector<std::string> args;
        if (!root.empty()) {
            args.push_back("--preset"); args.push_back(PRESET_ARGS[set.preset]);
            if (!set.iso.empty()) { args.push_back("--iso"); args.push_back(set.iso); }
            if (set.extract) args.push_back("--extract");
            /* Widescreen off and the patch not extracted yet: don't download it. Once it is in work/ the
               feature alone decides (the game's --features), so ticking it never extracts the disc again */
            if (!set.feature[FEAT_WIDESCREEN] && !is_file(join(work_dir(), "boot.widescreen.json")) && !is_file(join(work_dir(), "boot.elf")))
                args.push_back("--no-widescreen");
            if (set.tests) args.push_back("--test");
            if (set.run) {
                args.push_back("--run");
                args.push_back("--");
                for (const std::string &a : game) args.push_back(a);
            }
            script_command(args, argv, cmdline, shown);
            return;
        }
        args.push_back("--work"); args.push_back(work_dir());
        for (const std::string &a : game) args.push_back(a);
#ifdef _WIN32
        cmdline = win_quote(game_exe());
        for (const std::string &a : args) cmdline += " " + win_quote(a);
#else
        argv.push_back(game_exe());
#endif
        shown = game_exe();
        for (const std::string &a : args) {
            argv.push_back(a);
            shown += " " + (a.find(' ') != std::string::npos ? "\"" + a + "\"" : a);
        }
    }
    /* build.cmd / build.sh with ARGS */
    void script_command(const std::vector<std::string> &args, std::vector<std::string> &argv, std::string &cmdline, std::string &shown) {
#ifdef _WIN32
        std::string inner = win_quote(join(root, "build.cmd"));
        for (const std::string &a : args) inner += " " + win_quote(a);
        cmdline = "cmd.exe /d /s /c \"" + inner + "\"";     /* /s: cmd drops exactly the outer quotes */
        shown = "build.cmd";
#else
        argv.push_back("sh"); argv.push_back(join(root, "build.sh"));
        shown = "./build.sh";
#endif
        for (const std::string &a : args) {
            argv.push_back(a);
            shown += " " + (a.find(' ') != std::string::npos ? "\"" + a + "\"" : a);
        }
    }

    /* ---- Dependencies: what the build needs, and which of it is here */
    void check_deps(bool say) {
        if (root.empty()) { deps.clear(); deps_err = no_folder_hint(); deps_checked = true; return; }
        deps = deps_list(root, set.tools_dir, deps_err);
        deps_checked = true;
        if (!say) return;
        if (!deps_err.empty()) { console.note("Dependencies: " + deps_err, K_ERROR); return; }
        int here = 0, missing = 0, later = 0;
        for (const Dep &d : deps) { if (d.here) here++; else if (d.optional || (!d.system && !d.when.empty())) later++; else missing++; }
        console.note("");
        console.note("==> Check build dependencies", K_STEP);
        for (const Dep &d : deps)
            console.note(std::string(d.here ? "  [x] " : "  [ ] ") + d.name + (d.version.empty() ? "" : " " + d.version) + ": " +
                         (d.system ? d.detail : d.here ? "in " + d.dest : d.when.empty() ? "to download into " + d.dest : d.when),
                         d.here || d.optional ? K_TEXT : (d.system || d.when.empty()) ? K_WARN : K_TEXT);
        console.note(missing ? std::to_string(missing) + " missing, " + std::to_string(here) + " here" + (later ? ", " + std::to_string(later) + " downloaded later by the build" : "")
                             : "Everything the build needs now is here" + std::string(later ? " (" + std::to_string(later) + " more downloaded by the build when first needed)" : ""),
                     missing ? K_WARN : K_GOOD);
    }
    /* something the build would download (now or when it first needs it) that isn't here: the optional
       extras (FILTER's) are asked for on their own buttons */
    bool downloads_pending() const {
        for (const Dep &d : deps) if (!d.system && !d.here && !d.optional) return true;
        return false;
    }
    /* agreed to the list as it is now, or nothing to agree to */
    bool consent_given() const { return !downloads_pending() || set.consent == deps_signature(deps); }
    /* the player agreed to the list as shown: download the build tools now (tools/build.py --tools-only,
       which checks each file's SHA-256 before unpacking it) */
    void download_tools() {
        if (child.busy() || root.empty()) return;
        set.consent = deps_signature(deps);
        settings_save(set, settings_path);
        std::vector<std::string> argv, args; std::string cmdline, shown, err;
        args.push_back("--tools-only");
        script_command(args, argv, cmdline, shown);
        console.note("");
        console.note("Agreed: downloading the build tools into " + tools_dir() + " (each is checked against its SHA-256 before it is unpacked).", K_NOTE);
        console.note("> " + shown, K_CMD);
        set_child_env();
        if (!child.start(argv, cmdline, root, err)) { console.note("Could not start: " + err, K_ERROR); status = "Could not start"; status_kind = K_ERROR; start_after_tools = false; return; }
        started_at = SDL_GetTicks();
        running = RUN_TOOLS; game_running = false;
        status = "Downloading the build tools..."; status_kind = K_NOTE;
    }
    /* the FILTER setting's shaders: only when the player presses the button (tools/shaders.py checks each preset's SHA-256) */
    void download_shaders() {
        if (child.busy() || root.empty()) return;
        std::vector<std::string> argv, args; std::string cmdline, shown, err;
        args.push_back("--shaders");
        script_command(args, argv, cmdline, shown);
        console.note("");
        console.note("Downloading the FILTER setting's shaders into " + join(tools_dir(), "slang-shaders") + " (checked against their SHA-256).", K_NOTE);
        console.note("> " + shown, K_CMD);
        set_child_env();
        if (!child.start(argv, cmdline, root, err)) { console.note("Could not start: " + err, K_ERROR); status = "Could not start"; status_kind = K_ERROR; return; }
        started_at = SDL_GetTicks();
        running = RUN_TOOLS; game_running = false; start_after_tools = false;
        status = "Downloading the shaders..."; status_kind = K_NOTE;
    }
    /* ---- librashader, FILTER's library: built from its pinned source with Rust (tools/librashader.py) */
    static std::string find_cargo() {
        const char *exe =
#ifdef _WIN32
            "cargo.exe";
#else
            "cargo";
#endif
        const char *path = SDL_getenv("PATH");
        std::string p = path ? path : "";
#ifdef _WIN32
        const char sep = ';';
#else
        const char sep = ':';
#endif
        for (size_t at = 0; at <= p.size();) {
            size_t e = p.find(sep, at);
            if (e == std::string::npos) e = p.size();
            if (e > at && is_file(join(p.substr(at, e - at), exe))) return join(p.substr(at, e - at), exe);
            at = e + 1;
        }
        const char *ch = SDL_getenv("CARGO_HOME");
        std::string home = ch && *ch ? ch : std::string();
        if (home.empty()) {
#ifdef _WIN32
            const char *u = SDL_getenv("USERPROFILE");
#else
            const char *u = SDL_getenv("HOME");
#endif
            if (u) home = join(u, ".cargo");
        }
        std::string c = home.empty() ? std::string() : join(join(home, "bin"), exe);
        return !c.empty() && is_file(c) ? c : std::string();
    }
    /* Windows: Microsoft's C++ build tools (Rust links with them): Visual Studio <year>\<edition>\VC\Tools\MSVC */
    static int find_msvc() {
#ifdef _WIN32
        const char *bases[] = { SDL_getenv("ProgramFiles(x86)"), SDL_getenv("ProgramFiles") };
        for (const char *b : bases) {
            if (!b) continue;
            std::string vs = join(b, "Microsoft Visual Studio");
            int n = 0; char **years = SDL_GlobDirectory(vs.c_str(), "*", 0, &n);
            for (int i = 0; years && i < n; i++) {
                std::string y = join(vs, years[i]);
                int m = 0; char **eds = SDL_GlobDirectory(y.c_str(), "*", 0, &m);
                for (int j = 0; eds && j < m; j++)
                    if (is_dir(join(join(y, eds[j]), "VC/Tools/MSVC"))) { SDL_free(eds); SDL_free(years); return 1; }
                SDL_free(eds);
            }
            SDL_free(years);
        }
        return 0;
#else
        return 1;
#endif
    }
    void check_rust() { cargo_path = find_cargo(); msvc = find_msvc(); }
    void build_librashader(bool install_rust) {
        if (child.busy() || root.empty()) return;
        std::vector<std::string> argv, args; std::string cmdline, shown, err;
        args.push_back("--librashader");
        if (install_rust) args.push_back("--install-rust");
        script_command(args, argv, cmdline, shown);
        console.note("");
        console.note(std::string(install_rust ? "Installing Rust, then building" : "Building") + " librashader into " + join(tools_dir(), "librashader") +
                     " (ten minutes or more the first time).", K_NOTE);
        console.note("> " + shown, K_CMD);
        set_child_env();
        if (!child.start(argv, cmdline, root, err)) { console.note("Could not start: " + err, K_ERROR); status = "Could not start"; status_kind = K_ERROR; return; }
        started_at = SDL_GetTicks();
        running = RUN_TOOLS; game_running = false; start_after_tools = false;
        status = "Building librashader..."; status_kind = K_NOTE;
    }
    void set_child_env() {
        if (set.tools_dir.empty()) SDL_unsetenv_unsafe("A3MX_TOOLS_DIR");
        else SDL_setenv_unsafe("A3MX_TOOLS_DIR", set.tools_dir.c_str(), 1);     /* tools/build.py */
        SDL_setenv_unsafe("A3MX_USER_DIR", user_root.c_str(), 1);                 /* the game, however it is started */
    }

    void start() {
        if (child.busy()) return;
        DiscResult r;
        if (!root.empty() && !disc_allows_build(r)) {
            console.note(r.message.empty() ? "Choose your ISO first (Browse...)." : "Not building: " + r.message, K_ERROR);
            status = "The disc image is not one that builds"; status_kind = K_ERROR;
            return;
        }
        if (root.empty() && !is_file(game_exe())) {
            console.note("No source checkout (tools/build.py) and no " + game_exe() + " beside the launcher. " + no_folder_hint(), K_ERROR);
            status = "Nothing to run"; status_kind = K_ERROR;
            return;
        }
        if (!root.empty()) {
            check_deps(false);
            bool sys_missing = false;
            for (const Dep &d : deps) if (d.system && !d.here && !d.optional) sys_missing = true;
            if (sys_missing) {
                console.note("Not building: something the build needs is not on this system. See Dependencies.", K_WARN);
                status = "See Dependencies"; status_kind = K_WARN;
                want_tab = T_DEPS; start_after_tools = false;
                return;
            }
            if (!deps.empty() && !consent_given()) {
                /* nothing is downloaded before the player has seen the list and agreed, once (the modal) */
                consent_modal = true; start_after_tools = true;
                status = "Waiting for your agreement to the downloads"; status_kind = K_NOTE;
                return;
            }
            if (deps_any_to_download(deps) && !start_after_tools) {
                /* agreed before (the list hasn't changed since): fetch what's missing, then carry on */
                start_after_tools = true;
                download_tools();
                return;
            }
        }
        start_after_tools = false;
        std::vector<std::string> argv; std::string cmdline, shown, err;
        command(argv, cmdline, shown);
        console.note("");
        if (!RENDERERS[set.renderer].written)
            console.note(std::string("Renderer: ") + RENDERERS[set.renderer].arg + " is not available yet, so the game will use OpenGL (its log says so).", K_WARN);
        console.note("Features: " + features_arg(), K_NOTE);
        set_child_env();
        running = RUN_GAME;
        if ((root.empty() || set.run) && set.renderer != 1) {
            if (set.gpu != "auto" && !gpu_find(gpus, set.gpu))
                console.note("GPU: the one chosen before isn't here now; using Automatic.", K_WARN);
            std::string g = gpu_apply(gpus, set.gpu, run_exe(), set.gpu_owned);
            if (!g.empty()) console.note(g, K_NOTE);
            gpu_checked = false;
        }
        console.note("> " + shown, K_CMD);
        if (!child.start(argv, cmdline, home_dir(), err)) {
            console.note("Could not start: " + err, K_ERROR);
            status = "Could not start"; status_kind = K_ERROR;
            return;
        }
        started_at = SDL_GetTicks();
        game_running = root.empty();         /* without a checkout the child is the game itself */
        ctl_test = false; cap_player = -1;
        status = game_running ? "Game running: close it (" QUIT_KEYS ") to come back here" : !set.run ? "Building..." : "Building, then running...";
        status_kind = K_NOTE;
        set.extract = false;                 /* one go, not a setting */
        settings_save(set, settings_path);
    }

    /* "Close both": ask the game to close the way its own close button would (it saves), and quit
     * once it has; if it hasn't within 4 s, end it (closing_tick). */
    void close_both() {
        int n = child.close_game(run_exe());
        console.note(n ? "Closing the game..." : "Closing the game (no window of it found: ending it)...", K_NOTE);
        status = "Closing the game..."; status_kind = K_NOTE;
        closing_since = SDL_GetTicks();
        if (!n) closing_since -= 4000;     /* nothing to ask: go straight to ending it */
    }
    void closing_tick() {
        if (!closing_since) return;
        if (!child.busy()) { closing_since = 0; quit = true; return; }
        if (SDL_GetTicks() - closing_since < 4000) return;
        int n = child.end_game(run_exe());
        if (n) console.note("The game was still running outside the launcher's process group: ended it.", K_WARN);
        closing_since = 0;
        quit = true;                       /* main() waits a moment more for the pipe to close */
    }

    /* the child's output and the disc check; true if there's something new to draw */
    bool tick() {
        closing_tick();
        std::string got;
        bool was = child.busy();
        std::string status_before = status;
        size_t before = console.lines.size();
        child.poll(got);
        if (!got.empty()) console.feed(got);
        /* build.py's "==> Run" step: the build is over and the game starts */
        for (size_t i = before; i < console.lines.size() && !game_running && was && running == RUN_GAME; i++)
            if (console.lines[i].text.compare(0, 7, "==> Run") == 0) {
                game_running = true;
                status = "Game running: close it (" QUIT_KEYS ") to come back here"; status_kind = K_NOTE;
            }
        if (was && !child.busy()) {
            console.flush();
            Uint64 s = (SDL_GetTicks() - started_at + 500) / 1000;
            std::string took = s >= 60 ? std::to_string(s / 60) + " min " + std::to_string(s % 60) + " s" : std::to_string(s) + " s";
            if (child.exit_code == 0) { status = "Finished in " + took; status_kind = K_GOOD; }
            else { status = "Stopped with exit code " + std::to_string(child.exit_code) + " after " + took; status_kind = K_ERROR; }
            console.note(status, status_kind);
            game_running = false;
            check_work();
            if (running == RUN_TOOLS) {              /* the tools download: look again, and carry on with the Run that was waiting */
                running = RUN_GAME;
                check_deps(true);
                check_rust();
                if (child.exit_code == 0 && start_after_tools && !deps_any_to_download(deps)) { want_tab = T_RUN; start(); }
                else start_after_tools = false;
            } else check_gpu_used();
        }
        Uint64 now = SDL_GetTicks();
        if (now - work_checked > 2000) { work_checked = now; if (!child.busy()) check_work(); }
        bool changed = !got.empty() || was != child.busy() || status != status_before || closing_since || ctl_test || cap_player >= 0;
        if (now - disc_polled > 1500) { disc_polled = now; choose_disc(); }   /* not a reason to redraw behind the game */
        std::lock_guard<std::mutex> lock(dialog_mutex);
        if (dialog_done) {
            changed = true;
            dialog_done = false;
            if (!dialog_result.empty()) {
                set.iso = dialog_result;
                snprintf(iso_buf.data(), iso_buf.size(), "%s", set.iso.c_str());
                disc_polled = 0;                 /* check it now */
                settings_save(set, settings_path);
            }
            if (!dialog_error.empty()) { console.note("The file dialog could not open (" + dialog_error + "): type the path instead.", K_WARN); dialog_error.clear(); }
        }
        if (folder_done) {                       /* the tools folder, from Browse */
            changed = true; folder_done = false;
            if (!folder_result.empty()) {
                *folder_setting(folder_target) = folder_result;
                folders_to_bufs();
                folder_changed(folder_target);
            }
        }
        return changed;
    }

    static void SDLCALL on_dialog(void *userdata, const char *const *files, int filter) {
        App *app = (App *)userdata;
        std::lock_guard<std::mutex> lock(app->dialog_mutex);
        app->dialog_done = true;
        app->dialog_result = (files && files[0]) ? files[0] : "";
        if (!files) app->dialog_error = SDL_GetError();     /* the console is the main thread's: tick() reports it */
    }
    void browse() {
        static const SDL_DialogFileFilter filters[] = { { "PSP disc image (*.iso)", "iso" }, { "All files", "*" } };
        std::string where = set.iso.empty() ? (root.empty() ? base : join(root, "imports/psp/")) : parent(set.iso);
        SDL_ShowOpenFileDialog(on_dialog, this, window, filters, 2, where.empty() ? NULL : where.c_str(), false);
    }

    /* a new issue on GitLab, filled in with what the launcher knows. The address is .recomp.json's
       "Report a problem" link; the report is also put on the clipboard, in case the page opens empty */
    void report_bug() {
        const char *os = SDL_GetPlatform();
        std::string how = std::string("Launcher: preset ") + PRESET_ARGS[set.preset] + ", features " + features_arg() +
                          (set.fullscreen ? ", full screen" : ", windowed") + (ctl_file ? ", controllers.cfg" : ", controllers automatic") +
                          ", logging " + LOG_ARGS[set.log_level] + ", renderer " + RENDERERS[set.renderer].arg + (set.renderer == 1 ? std::string() : set.scale ? ", resolution " + std::to_string(set.scale) + "x" : std::string(", resolution auto")) +
                          (gpus.size() < 2 ? std::string() : ", GPU " + (gpu_find(gpus, set.gpu) ? gpu_find(gpus, set.gpu)->name : std::string("automatic"))) +
                          (set.extra.empty() ? "" : ", options: " + set.extra);
        DiscResult r = disc.result();
        /* GitLab's new-issue page takes a title and a description in its address: the report, filled in */
        std::string body = "## What happened\n\n(what you did, what you expected, what you saw instead)\n\n## Details\n\n";
        body += std::string("- Version: ") + A3MX_VERSION + "\n- System: " + (os ? os : "") + "\n- " + how + "\n";
        if (!r.serial.empty()) body += "- Disc: " + r.serial + (r.exact ? " (the known dump)" : "") + "\n";
        /* the renderer the game actually used and its frame times, from the last run (logs/last_run.txt) */
        std::string last = last_run_summary();
        if (!last.empty()) body += "\n## The last run\n\n```\n" + last + "\n```\n";
        body += "\n## Log\n\nPlease attach `logs/a3mx.log` (the launcher's **Log folder** button opens the folder).\n";
        std::string base_url = std::string(REPO_URL) + NEW_ISSUE + "&issue[title]=" + url_encode("Bug: ") + "&issue[description]=";
        std::string log = console.tail(2500);
        /* very long addresses are refused: the end of the console only, then the file by hand */
        while (!log.empty() && base_url.size() + url_encode(body).size() + url_encode(log).size() > 7000) {
            size_t nl = log.find('\n');
            log = nl == std::string::npos ? std::string() : log.substr(nl + 1);
        }
        if (!log.empty()) body += "\nThe end of the launcher's console:\n\n```\n" + log + "\n```\n";
        std::string url = base_url + url_encode(body);
        SDL_SetClipboardText(body.c_str());
        if (!SDL_OpenURL(url.c_str())) console.note(std::string("Could not open the browser: ") + SDL_GetError() + ". The address: " + url, K_WARN);
        else console.note("Opened a new issue on GitLab in your browser. If it isn't filled in, paste: the report is on the clipboard. Please attach logs/a3mx.log (Log folder, bottom right).", K_NOTE);
    }
    /* one value from logs/last_run.txt */
    std::string last_run(const char *k) const {
        std::string t = "\n" + read_text(join(user_dir("logs"), "last_run.txt"));
        std::string key = std::string("\n") + k + "=";
        size_t at = t.find(key);
        if (at == std::string::npos) return std::string();
        at += key.size();
        size_t e = t.find('\n', at);
        std::string v = t.substr(at, e == std::string::npos ? std::string::npos : e - at);
        if (!v.empty() && v.back() == '\r') v.pop_back();
        return v;
    }
    /* after a run: did the game draw on the card chosen? (only its maker can be told from GL_RENDERER) */
    void check_gpu_used() {
        if (gpu_checked || set.renderer == 1 || gpus.size() < 2) return;
        gpu_checked = true;
        const Gpu *want = gpu_find(gpus, set.gpu);
        if (!want) want = &gpus[0];
        if (!want->choosable) return;
        std::string used = last_run("gpu");
        if (gpu_matches(*want, used) == 0)
            console.note("GPU: the game ran on " + used + ", not " + want->name + ". Another setting may be choosing for it "
                         "(the graphics driver's control panel, or Windows' Settings > Display > Graphics).", K_WARN);
    }
    /* logs/last_run.txt (written by the game when it quits) as a few readable lines */
    std::string last_run_summary() const {
        std::string t = read_text(join(user_dir("logs"), "last_run.txt")), out;
        auto get = [&](const char *k) {
            std::string key = std::string("\n") + k + "=";
            size_t at = ("\n" + t).find(key);
            if (at == std::string::npos) return std::string();
            at += key.size() - 1;
            size_t e = t.find('\n', at);
            std::string v = t.substr(at, e == std::string::npos ? std::string::npos : e - at);
            if (!v.empty() && v.back() == '\r') v.pop_back();
            return v;
        };
        if (get("renderer").empty()) return "";
        out = "Renderer: " + get("renderer");
        if (!get("features").empty()) out = "Features: " + get("features") + "\n" + out;
        if (!get("gpu").empty()) out += " on " + get("gpu");
        if (get("renderer").rfind("GPU", 0) == 0) out += ", scale " + get("scale") + " (" + get("scale_from") + ")";
        out += "\nFrame time (the last run, " + get("frames") + " frames, " + get("version") + "): busy " + get("busy_ms") +
               " ms (game " + get("game_ms") + ", render " + get("render_ms") + " of which textures " + get("texture_ms") +
               ", present " + get("present_ms") + "), p99 " + get("p99_ms") + " ms, max " + get("max_ms") + " ms, " +
               get("slow_frames") + " frames over 16.7 ms";
        if (!get("gpu_ms").empty()) out += ", GPU " + get("gpu_ms") + " ms";
        if (!get("hitches").empty()) {           /* 0.35.3 on: what reached the screen */
            std::string hz = get("refresh_hz");
            out += "\nOn screen: vsync " + get("vsync") + (get("frame_timing").empty() ? std::string() : ", frame timing " + get("frame_timing") + (get("frame_timing_given").empty() ? std::string() : ", given " + get("frame_timing_given")) + " (" + get("pacing") + ")") + (hz.empty() || hz == "0.00" ? std::string() : ", monitor " + hz + " Hz") +
                   ", waiting for vsync " + get("swap_ms") + " ms, a new frame every p99 " + get("shown_p99_ms") + " ms (max " +
                   get("shown_max_ms") + " ms), " + get("hitches") + " frames shown twice or more";
        }
        return out;
    }
    void open_url(const std::string &u) {
        if (!SDL_OpenURL(u.c_str())) console.note(std::string("Could not open ") + u + ": " + SDL_GetError(), K_WARN);
    }

    ImVec4 colour(Kind k) const {
        if (dark) switch (k) {
        case K_STEP:  return ImVec4(0.45f, 0.75f, 1.00f, 1);
        case K_CMD:   return ImVec4(0.62f, 0.62f, 0.68f, 1);
        case K_WARN:  return ImVec4(0.98f, 0.80f, 0.35f, 1);
        case K_ERROR: return ImVec4(1.00f, 0.45f, 0.42f, 1);
        case K_DEBUG: return ImVec4(0.60f, 0.66f, 0.60f, 1);
        case K_NOTE:  return ImVec4(0.80f, 0.74f, 0.98f, 1);
        case K_GOOD:  return ImVec4(0.50f, 0.86f, 0.55f, 1);
        default:      return ImVec4(0.88f, 0.88f, 0.88f, 1);
        }
        switch (k) {
        case K_STEP:  return ImVec4(0.08f, 0.36f, 0.72f, 1);
        case K_CMD:   return ImVec4(0.42f, 0.42f, 0.48f, 1);
        case K_WARN:  return ImVec4(0.62f, 0.40f, 0.00f, 1);
        case K_ERROR: return ImVec4(0.78f, 0.12f, 0.10f, 1);
        case K_DEBUG: return ImVec4(0.32f, 0.44f, 0.32f, 1);
        case K_NOTE:  return ImVec4(0.42f, 0.26f, 0.72f, 1);
        case K_GOOD:  return ImVec4(0.08f, 0.50f, 0.18f, 1);
        default:      return ImVec4(0.10f, 0.10f, 0.12f, 1);
        }
    }

    /* the GPU row: Automatic (the fastest), or one of this PC's cards */
    void gpu_row(float em) {
        label("GPU", em * 5.5f);
        ImGui::SetNextItemWidth(-em);
        const Gpu *cur = gpu_find(gpus, set.gpu);
        std::string preview = cur ? gpu_label(*cur) : gpus.size() < 2 ? gpu_auto_label() : std::string("Automatic (fastest)");
        ImGui::BeginDisabled(set.renderer == 1 || gpus.size() < 2);
        if (ImGui::BeginCombo("##gpu", preview.c_str())) {
            if (ImGui::Selectable(gpu_auto_label().c_str(), !cur)) set.gpu = "auto";
            for (const Gpu &g : gpus) {
                ImGui::BeginDisabled(!g.choosable);
                if (ImGui::Selectable(gpu_label(g).c_str(), cur == &g)) set.gpu = g.id;
                ImGui::EndDisabled();
                if (!g.choosable) ImGui::SetItemTooltip("%s", g.why_not.c_str());
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        std::string tip = "The graphics card the game draws on. Automatic (the default): the fastest one found.";
        if (gpus.empty()) tip += "\nNo list of cards here, so the system chooses.";
        else if (gpus.size() == 1) tip += "\nThis PC has one, so there's nothing to choose.";
#ifdef _WIN32
        else tip += "\nSaved as the game's setting in Windows' Settings > Display > Graphics when it starts.";
#else
        else tip += "\nPassed to the game as DRI_PRIME, or NVIDIA's PRIME offload variables.";
#endif
        if (gpus.size() >= 2 && !cur) tip += "\nThe fastest here: " + gpus[0].name;
        std::string used = last_run("gpu");
        if (!used.empty()) tip += "\nThe last run drew on: " + used;
        ImGui::SetItemTooltip("%s", tip.c_str());
    }
    void label(const char *s, float w) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s);
        ImGui::SameLine(w);
    }
    /* text that looks and works like a link */
    bool link(const char *text) {
        ImVec4 c = dark ? ImVec4(0.45f, 0.70f, 1.0f, 1) : ImVec4(0.10f, 0.35f, 0.80f, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, c);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        bool hovered = ImGui::IsItemHovered();
        if (hovered) {
            ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(a.x, b.y), b, ImGui::GetColorU32(c));
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        return hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    }

    /* a square button with a picture instead of text (the bug, the copy icon) */
    bool icon_button(const char *id, float size, void (*draw)(ImDrawList *, ImVec2, float, ImU32), bool framed = true) {
        ImGuiStyle &st = ImGui::GetStyle();
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
        bool hov = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 q(p.x + size, p.y + size);
        if (framed || hov) {
            dl->AddRectFilled(p, q, ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : hov ? ImGuiCol_ButtonHovered : ImGuiCol_Button), st.FrameRounding);
        }
        draw(dl, p, size, ImGui::GetColorU32(ImGuiCol_Text));
        if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        return clicked;
    }

    /* ---- the Image section: the ISO and its check */
    void image_section(bool busy) {
        const float em = ImGui::GetFontSize(), lw = em * 7.0f, h = ImGui::GetFrameHeight();
        const bool repo = !root.empty();
        ImGui::SeparatorText("Disc image");
        ImGui::BeginDisabled(busy);
        label("Your ISO", lw);
        float browse_w = em * 6.0f, fw = flag_width(h);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_w - fw - ImGui::GetStyle().ItemSpacing.x * 2);
        std::string hint = disc_from_imports ? "empty: " + base_name(disc_path) + " in imports/psp/" : repo ? "empty: the ISO in imports/psp/" : "not used: the game reads work/";
        if (ImGui::InputTextWithHint("##iso", hint.c_str(), iso_buf.data(), iso_buf.size())) { set.iso = iso_buf.data(); disc_polled = 0; }
        if (ImGui::IsItemDeactivatedAfterEdit()) settings_save(set, settings_path);
        ImGui::SameLine();

        /* the flag, or the mark, as high as the field */
        DiscResult r = disc.result();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(fw, h));
        ImDrawList *dl = ImGui::GetWindowDrawList();
        bool from_work = r.verdict == DISC_EMPTY && set.iso.empty() && !work_region.empty();
        Region shown = r.region;
        if (from_work) shown = work_region == "us" ? REGION_US : work_region == "jp" ? REGION_JP : REGION_NONE;
        if (r.verdict == DISC_CHECKING) draw_wait_mark(dl, p, h, (float)SDL_GetTicks() / 1000.0f, ImGui::GetColorU32(ImGuiCol_TextDisabled));
        else if (r.verdict == DISC_OK || r.verdict == DISC_UNSUPPORTED || (from_work && shown != REGION_NONE)) draw_flag(dl, p, h, shown);
        else if (r.verdict == DISC_BAD) draw_bad_mark(dl, p, h);
        if (ImGui::IsItemHovered() && !r.message.empty()) ImGui::SetTooltip("%s", r.message.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Browse...", ImVec2(-1, 0))) browse();
        ImGui::EndDisabled();

        /* the line underneath: the check, then what work/ holds */
        ImGui::SetCursorPosX(lw);
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        if (!repo) ImGui::TextColored(colour(K_NOTE), "No source checkout found: Run starts %s.", game_exe().c_str());
        else if (r.verdict == DISC_CHECKING) ImGui::TextColored(colour(K_CMD), "Checking %s...", base_name(r.path).c_str());
        else if (r.verdict == DISC_BAD || r.verdict == DISC_UNSUPPORTED) {
            ImGui::TextColored(colour(K_ERROR), "%s Run and Build are off until you choose the US disc (ULUS-10062).", r.message.c_str());
        } else if (r.verdict == DISC_OK) {
            std::string extra = r.whole_done ? (r.exact ? " The image is the known dump." : " A different dump of the same program: fine.")
                                             : disc.busy() ? " Checking the whole image... " + std::to_string((int)(disc.whole_progress() * 100)) + "%" : "";
            ImGui::TextColored(colour(K_GOOD), "%s%s", r.message.c_str(), extra.c_str());
        } else if (!imports_note.empty() && work_region.empty()) ImGui::TextColored(colour(K_ERROR), "%s", imports_note.c_str());
        else if (!imports_note.empty() && work_region != "us") ImGui::TextColored(colour(K_ERROR), "%s", imports_note.c_str());
        if (repo && !work_line.empty() && r.verdict != DISC_BAD && r.verdict != DISC_UNSUPPORTED) {
            ImGui::SetCursorPosX(lw);
            ImGui::TextColored(colour(work_region.empty() ? K_NOTE : K_CMD), "%s", work_line.c_str());
        }
        ImGui::PopTextWrapPos();
    }

    void options_window() {
        const float em = ImGui::GetFontSize();
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x * 0.94f, vp->WorkSize.y * 0.88f), ImGuiCond_Appearing);
        if (!ImGui::BeginPopupModal("The game's options", NULL, ImGuiWindowFlags_NoSavedSettings)) return;
        ImGui::TextWrapped("Type any of these into Game > Options (separated by spaces), or copy one and paste it there. "
                           "Those marked * are already set by the launcher's own controls.");
        ImGui::Spacing();
        ImGui::BeginChild("list", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - em * 0.3f), ImGuiChildFlags_None);
        auto table = [&](const char *title, const GameOption *opts, int n) {
            ImGui::SeparatorText(title);
            if (!ImGui::BeginTable(title, 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX)) return;
            ImGui::TableSetupColumn("option", ImGuiTableColumnFlags_WidthFixed, em * 15.5f);
            ImGui::TableSetupColumn("what", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("copy", ImGuiTableColumnFlags_WidthFixed, em * 1.8f);
            for (int i = 0; i < n; i++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (mono) ImGui::PushFont(mono, 14.0f);
                ImGui::TextWrapped("%s%s", opts[i].syntax, opts[i].launcher_sets ? " *" : "");
                if (mono) ImGui::PopFont();
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", opts[i].what);
                ImGui::TableNextColumn();
                ImGui::PushID(opts[i].option);
                if (icon_button("copy", ImGui::GetTextLineHeight() + 6, draw_copy, false)) { SDL_SetClipboardText(opts[i].option); console.note(std::string("Copied: ") + opts[i].option, K_NOTE); }
                ImGui::SetItemTooltip("Copies \"%s\"", opts[i].option);
                ImGui::PopID();
            }
            ImGui::EndTable();
        };
        table("For playing", PLAY_OPTIONS, (int)(sizeof PLAY_OPTIONS / sizeof PLAY_OPTIONS[0]));
        ImGui::Spacing();
        table("For developers and tests", DEV_OPTIONS, (int)(sizeof DEV_OPTIONS / sizeof DEV_OPTIONS[0]));
        ImGui::EndChild();
        if (link("All of them, with details: docs/BUILDING.md")) open_url(std::string(REPO_URL) + "/-/blob/main/docs/BUILDING.md#command-line-the-game");
        ImGui::SameLine(ImGui::GetWindowWidth() - em * 6.5f);
        if (ImGui::Button("Close", ImVec2(em * 5.5f, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    void about_window() {
        const float em = ImGui::GetFontSize();
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        /* a fixed width, and the height fitted to the contents (0): fitting both made the separators and the
           right-aligned Close widen the window each frame, so it grew and shrank while open */
        ImGui::SetNextWindowSize(ImVec2(std::min(em * 32, vp->WorkSize.x - em * 2), 0), ImGuiCond_Always);
        if (!ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) return;
        ImGui::TextUnformatted("A3MX");
        ImGui::TextDisabled("Launcher %s", A3MX_VERSION);
        ImGui::Spacing();
        ImGui::TextWrapped("A decompilation of the PSP game, built from your own disc. No game data is included.");
        ImGui::SeparatorText("Project");
        if (link("The project's page")) open_url(REPO_URL);
        if (link("Report a bug")) report_bug();
        if (link("Request a feature")) open_url(std::string(REPO_URL) + NEW_ISSUE + "&issue[title]=" + url_encode("Feature request: ") + "&issue[description]=" + url_encode(std::string("(what you would like, and why)\n\nVersion: ") + A3MX_VERSION + "\n"));
        if (link("Something else (a question, an idea, a problem building)")) open_url(std::string(REPO_URL) + "/-/issues");
        if (link("How to report a problem, and what to include")) open_url(std::string(REPO_URL) + "/-/blob/main/docs/REPORTING_ISSUES.md");
        ImGui::SeparatorText("Licences");
        ImGui::PushTextWrapPos(0);                                   /* wrap at the window's (fixed) edge */
        ImGui::TextDisabled("GPL-3.0-or-later. The launcher uses SDL3 (zlib) and Dear ImGui (MIT).");
        ImGui::TextDisabled("Your files: %s", user_root.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - em * 5.5f);
        if (ImGui::Button("Close", ImVec2(em * 5.5f, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }


    /* the orange of Run and of the buttons that download (pop with ImGui::PopStyleColor(4)) */
    static void orange_button_push() {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.36f, 0.12f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.45f, 0.18f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.30f, 0.08f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    }

    /* ---- the one-time question before the first downloads: what, from where, checked how, to where.
       Agreed once, it isn't asked again until the pinned list changes (deps_signature). */
    void consent_window() {
        const float em = ImGui::GetFontSize();
        const char *title = "Download the build dependencies";
        if (consent_modal) { ImGui::OpenPopup(title); consent_modal = false; }
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(std::min(vp->WorkSize.x - em * 4, em * 48), 0), ImGuiCond_Appearing);
        if (!ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_NoSavedSettings)) return;
        bool to_download = deps_any_to_download(deps);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(to_download ? "Building needs these, and they aren't on this PC yet:" : "The build downloads these when it first needs them:");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        /* the list: names, versions, folders; the addresses and checksums beneath, scrolled if long */
        ImGui::BeginChild("list", ImVec2(0, std::min(em * 15.0f, vp->WorkSize.y * 0.4f)), ImGuiChildFlags_Borders);
        for (const Dep &d : deps) {
            if (d.system || d.here || d.optional) continue;
            ImGui::TextColored(colour(K_STEP), "%s %s%s", d.name.c_str(), d.version.c_str(), d.when.empty() ? "" : "  (later, when the build needs it)");
            if (mono) ImGui::PushFont(mono, 13.0f);
            ImGui::TextDisabled("  from     %s", d.url.c_str());
            ImGui::TextDisabled("  SHA-256  %s%s", d.sha256.c_str(), d.sha_of.empty() ? "" : ("  (of " + d.sha_of + ")").c_str());
            ImGui::TextDisabled("  into     %s", d.dest.c_str());
            if (mono) ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::Spacing();
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted("Each comes from the address shown, at the version this project pins, and is checked against its SHA-256 "
                               "before it is unpacked (deleted if it doesn't match). Nothing is installed on your system.");
        ImGui::TextDisabled("Tools folder: %s", tools_dir().c_str());
        ImGui::TextDisabled("You'll be asked again only if this list changes. The same list is in docs/DEPENDENCIES.md.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        orange_button_push();
        if (ImGui::Button(to_download ? "I agree: download them" : "I agree", ImVec2(em * 12, 0))) {
            set.consent = deps_signature(deps); settings_save(set, settings_path);
            console.note("Agreed: the build may download what Dependencies lists.", K_NOTE);
            if (to_download) download_tools();                       /* (start_after_tools: Run carries on after) */
            else if (start_after_tools) { start_after_tools = false; start(); }
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(em * 6, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            if (start_after_tools) { console.note("Not building: the downloads weren't agreed to.", K_WARN); status = "Cancelled"; status_kind = K_WARN; }
            start_after_tools = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    /* ---- building librashader: what it takes (Rust, and on Windows Microsoft's C++ build tools) */
    void librashader_window() {
        const float em = ImGui::GetFontSize();
        const char *title = "Build librashader (for FILTER)";
        if (lr_modal) { ImGui::OpenPopup(title); lr_modal = false; }
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(std::min(vp->WorkSize.x - em * 4, em * 44), 0), ImGuiCond_Appearing);
        if (!ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_NoSavedSettings)) return;
        const bool rust = !cargo_path.empty();
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted("SCREEN SETTING > FILTER draws its shaders with librashader. It is built here, once, from its source at "
                               "the commit this project pins, and kept in the tools folder (--clean leaves it).");
        ImGui::Spacing();
        std::string ver;
        for (const Dep &d : deps) if (d.id == "librashader") ver = d.version;
        ImGui::Bullet(); ImGui::TextWrapped("Source: SnowflakePowered/librashader, %s, checked against the SHA-256s in pins.json", ver.empty() ? "the pinned commit" : ver.c_str());
        ImGui::Bullet(); ImGui::TextWrapped("Its Rust packages, from crates.io, each checked against the checksum its Cargo.lock gives");
        ImGui::Bullet(); ImGui::TextWrapped("Into: %s", join(tools_dir(), "librashader").c_str());
        ImGui::Spacing();
        if (rust) ImGui::TextColored(colour(K_GOOD), "Rust: found (%s)", cargo_path.c_str());
        else {
            ImGui::TextColored(colour(K_WARN), "Rust is not on this PC.");
            ImGui::TextUnformatted("\"Install Rust and build\" runs Rust's own installer (rustup, from rustup.rs), which installs Rust for "
                                   "your user only (in .cargo and .rustup in your home folder) and adds it to your PATH. "
                                   "It can be removed later with: rustup self uninstall");
        }
#ifdef _WIN32
        if (msvc == 0) {
            ImGui::Spacing();
            ImGui::TextColored(colour(K_WARN), "Microsoft's C++ build tools were not found. Rust needs them to build on Windows.");
            ImGui::TextUnformatted("Install \"Build Tools for Visual Studio\" and tick \"Desktop development with C++\", then come back here.");
            if (link("visualstudio.microsoft.com/visual-cpp-build-tools")) open_url("https://visualstudio.microsoft.com/visual-cpp-build-tools/");
        }
#endif
        ImGui::TextDisabled("It takes ten minutes or more, and uses about 1.5 GB while it builds. The game runs without it: FILTER then does nothing.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::BeginDisabled(child.busy() || msvc == 0);
        orange_button_push();
        if (ImGui::Button(rust ? "Build it" : "Install Rust and build", ImVec2(em * 13, 0))) { build_librashader(!rust); ImGui::CloseCurrentPopup(); }
        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(em * 6, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    /* ================================================================ the left side: picture, tabs, bug, About */
    void sidebar(float h) {
        const float em = ImGui::GetFontSize();
        ImGuiStyle &st = ImGui::GetStyle();
        float w = ImGui::GetContentRegionAvail().x;
        if (logo && logo_w > 0) {                          /* the project's picture, scaled to the side's width */
            float lh = w * (float)logo_h / (float)logo_w;
            ImGui::Image((ImTextureID)(intptr_t)logo, ImVec2(w, lh));
        } else { ImGui::TextUnformatted("A3MX"); }
        /* the version, under the picture, against its right edge. A nightly's is longer than the side is
           wide (v0.60.0-nightly.20261112+c97f96a): then on two lines, the commit under the rest */
        {
            std::string v = A3MX_VERSION, rest;
            size_t plus = v.find('+');
            if (plus != std::string::npos && ImGui::CalcTextSize(v.c_str()).x > w - em * 0.3f) { rest = v.substr(plus); v.erase(plus); }
            float x0 = ImGui::GetCursorPosX();
            for (const std::string &line : { v, rest }) {
                if (line.empty()) continue;
                ImGui::SetCursorPosX(x0 + std::max(0.0f, w - ImGui::CalcTextSize(line.c_str()).x - em * 0.3f));
                ImGui::TextDisabled("%s", line.c_str());
                ImGui::SetItemTooltip("A3MX %s", A3MX_VERSION);
            }
        }
        /* the bug and About, under it */
        const float bh = ImGui::GetFrameHeight();
        bug_dark = dark;
        if (icon_button("bug", bh, [](ImDrawList *dl, ImVec2 p, float s, ImU32) {
                ImU32 body = bug_dark ? IM_COL32(205, 205, 210, 255) : IM_COL32(150, 150, 158, 255);
                ImU32 shade = bug_dark ? IM_COL32(140, 140, 148, 255) : IM_COL32(95, 95, 104, 255);
                draw_bug(dl, ImVec2(p.x + 2, p.y + 2), s - 4, body, shade); })) report_bug();
        ImGui::SetItemTooltip("Report a problem: opens a new issue on GitLab, filled in with the version, your system,\n"
                              "these settings and the end of the console. Attach logs/a3mx.log there.");
        ImGui::SameLine();
        if (ImGui::Button("About", ImVec2(w - bh - st.ItemSpacing.x, bh))) ImGui::OpenPopup("About");
        about_window();
        if (show_at_start == "about") { ImGui::OpenPopup("About"); show_at_start.clear(); }
        ImGui::Dummy(ImVec2(0, em * 0.7f));                            /* a little air before the tabs */
        const float row_h = em * 1.9f, bar_w = 3 * scale, text_x = em * 0.9f;
        for (int t = 0; t < T_COUNT; t++) {
            bool sel = set.tab == t;
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::PushID(t);
            if (ImGui::Selectable("##tab", sel, 0, ImVec2(w, row_h))) { set.tab = t; settings_save(set, settings_path); }
            ImGui::PopID();
            ImDrawList *dl = ImGui::GetWindowDrawList();
            if (sel) dl->AddRectFilled(p, ImVec2(p.x + bar_w, p.y + row_h), ImGui::GetColorU32(ImGuiCol_CheckMark));
            /* the name, drawn here so it is clear of the bar and exactly in the middle of the row's height */
            dl->AddText(ImVec2(p.x + text_x, p.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f), ImGui::GetColorU32(ImGuiCol_Text), TAB_NAMES[t]);
            /* a mark on the tab that wants the player's attention */
            bool mark = (t == T_DEPS && deps_checked && !root.empty() && (deps_any_missing(deps) || !consent_given()));
            if (mark) dl->AddCircleFilled(ImVec2(p.x + w - em * 0.6f, p.y + row_h * 0.5f), em * 0.22f, ImGui::GetColorU32(colour(K_WARN)));
        }
        (void)h;
    }

    void open_folder(const std::string &dir) {
        if (!is_dir(dir)) console.note("There is no " + dir + " yet.", K_WARN);
        else open_url(file_url(dir));
    }
    /* a row of the Paths tab that is only shown: what, where, Open */
    void path_row(const char *what, const std::string &where, const char *tip) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(what);
        if (tip) ImGui::SetItemTooltip("%s", tip);
        ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
        ImGui::TextColored(colour(is_dir(where) ? K_TEXT : K_CMD), "%s", where.c_str());
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::PushID(what);
        if (ImGui::Button("Open", ImVec2(-1, 0))) open_folder(where);
        ImGui::PopID();
    }
    /* a row the player can choose: the folder typed or browsed for (empty: the default, shown as the hint), Open */
    void folder_row(const char *what, int f, const std::string &def, const char *tip, bool busy) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(what);
        if (tip) ImGui::SetItemTooltip("%s", tip);
        ImGui::PushID(what);
        ImGui::BeginDisabled(busy);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##dir", def.c_str(), folder_buf[f].data(), folder_buf[f].size())) *folder_setting(f) = folder_buf[f].data();
        if (ImGui::IsItemDeactivatedAfterEdit()) folder_changed(f);
        if (tip) ImGui::SetItemTooltip("%s\n\nEmpty: %s", tip, def.c_str());
        ImGui::TableNextColumn();
        if (ImGui::Button("Browse...", ImVec2(-1, 0))) { folder_target = f; browse_folder(folder_setting(f)->empty() ? def : *folder_setting(f)); }
        ImGui::EndDisabled();
        ImGui::TableNextColumn();
        if (ImGui::Button("Open", ImVec2(-1, 0))) open_folder(folder_setting(f)->empty() ? def : *folder_setting(f));
        ImGui::PopID();
    }

    /* ================================================================ Paths */
    void paths_tab(bool busy) {
        const float em = ImGui::GetFontSize();
        image_section(busy);
        ImGui::Spacing();
        ImGui::SeparatorText("Folders");
        if (ImGui::BeginTable("folders", 4, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("what", ImGuiTableColumnFlags_WidthFixed, em * 6.5f);
            ImGui::TableSetupColumn("where", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("browse", ImGuiTableColumnFlags_WidthFixed, em * 5.5f);
            ImGui::TableSetupColumn("open", ImGuiTableColumnFlags_WidthFixed, em * 3.8f);
            folder_row("Saves", F_SAVES, join(user_root, "saves"), "a3mx.dat, and import/ and export/ for PSP saves.", busy);
            folder_row("Configs", F_CONFIGS, join(user_root, "configs"), "a3mx.cfg (every setting, by name: it can be edited by hand) and controllers.cfg.", busy);
            folder_row("Logs", F_LOGS, join(user_root, "logs"), "a3mx.log and last_run.txt from the last run of the game, and launcher.log:\nthis console, with every build and download.", busy);
            if (!root.empty())
                folder_row("Build tools", F_TOOLS, join(root, ".tools"),
                           "Where the Dependencies tab downloads the compiler, CMake and Ninja (about 250 MB on Windows).\n"
                           "Nothing is installed on the system either way.\n"
                           "(Windows: if Python has to be downloaded too, it always goes into .tools\\python.)", busy);
            if (!root.empty()) path_row("Source", root, "The checkout: the code, tools/, imports/psp/ for your ISO, and the build folders.");
            path_row("Disc files", work_dir(), "work/: your disc, extracted. Made again from the ISO whenever it's needed, so it can be deleted.\nIt is always beside the source: the build looks for it there.");
            path_row("Launcher", join(user_root, "launcher"), "launcher.ini: what is chosen in this window, these folders included.\nIt stays here, so the launcher can always find it.");
            ImGui::EndTable();
        }
        ImGui::BeginDisabled(busy);
        if (ImGui::Button("Defaults")) {                       /* every path as it is with nothing chosen */
            set.iso.clear(); iso_buf[0] = 0; disc_polled = 0;
            for (int f = 0; f < F_COUNT; f++) folder_setting(f)->clear();
            folders_to_bufs();
            for (int f = 0; f < F_COUNT; f++) folder_changed(f);
            choose_disc();
            console.note("Paths: back to the defaults (the ISO in imports/psp/, saves, configs and logs in your a3mx folder, tools in .tools).", K_NOTE);
        }
        ImGui::SetItemTooltip("Every path back to its default. Nothing is moved or deleted: files in a folder you chose stay there.");
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        if (user_kind == 1) ImGui::TextDisabled("Kept in launcher.ini. Changing a folder doesn't move what is in the old one.");
        else if (user_kind == 2) ImGui::TextColored(colour(K_WARN), "Your user folder couldn't be written, so saves, configs and logs default to beside the game.");
        else ImGui::TextColored(colour(K_ERROR), "Neither your user folder nor this one can be written: settings and saves will not be kept.");
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::SeparatorText("Shortcut");
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextDisabled("Nothing makes a shortcut by itself. If you want one to this launcher:");
        ImGui::PopTextWrapPos();
        if (ImGui::Button("Create a Desktop shortcut")) make_shortcut(SHORTCUT_DESKTOP);
        if (shortcut_menu_available()) {
            ImGui::SameLine();
#ifdef _WIN32
            if (ImGui::Button("Create a Start menu shortcut")) make_shortcut(SHORTCUT_MENU);
#else
            if (ImGui::Button("Add to the applications menu")) make_shortcut(SHORTCUT_MENU);
#endif
        }
    }
    void make_shortcut(int where) {
        std::string made, err, exe = join(base, launcher_name());
        std::string icon = root.empty() ? std::string() : join(root, "res/launcher_512.png");
        if (shortcut_create(where, exe, home_dir(), is_file(icon) ? icon : std::string(), made, err)) console.note("Shortcut made: " + made, K_GOOD);
        else console.note("The shortcut couldn't be made: " + err, K_ERROR);
    }
    static const char *launcher_name() {
#ifdef _WIN32
        return "a3mx-launcher.exe";
#else
        return "a3mx-launcher";
#endif
    }
    static void SDLCALL on_folder(void *userdata, const char *const *files, int) {
        App *app = (App *)userdata;
        std::lock_guard<std::mutex> lock(app->dialog_mutex);
        app->folder_done = true;
        app->folder_result = (files && files[0]) ? files[0] : "";
    }
    void browse_folder(const std::string &from) { SDL_ShowOpenFolderDialog(on_folder, this, window, from.c_str(), false); }

    /* ================================================================ Dependencies */
    void tick_mark(bool ok, bool later) {
        const float h = ImGui::GetFrameHeight(), em = ImGui::GetFontSize();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(em * 1.3f, h));
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 c(p.x + em * 0.6f, p.y + h * 0.5f);
        float r = em * 0.32f;
        if (ok) {                                             /* a green tick */
            ImU32 g = ImGui::GetColorU32(colour(K_GOOD));
            dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.25f, c.y + r * 0.75f), g, 2.2f * scale);
            dl->AddLine(ImVec2(c.x - r * 0.25f, c.y + r * 0.75f), ImVec2(c.x + r, c.y - r * 0.8f), g, 2.2f * scale);
        } else if (later) dl->AddCircle(c, r * 0.8f, ImGui::GetColorU32(ImGuiCol_TextDisabled), 0, 1.5f * scale);   /* not needed yet */
        else {                                                /* missing: an amber cross */
            ImU32 a = ImGui::GetColorU32(colour(K_WARN));
            dl->AddLine(ImVec2(c.x - r * 0.8f, c.y - r * 0.8f), ImVec2(c.x + r * 0.8f, c.y + r * 0.8f), a, 2.0f * scale);
            dl->AddLine(ImVec2(c.x - r * 0.8f, c.y + r * 0.8f), ImVec2(c.x + r * 0.8f, c.y - r * 0.8f), a, 2.0f * scale);
        }
    }
    void deps_tab(bool busy) {
        const float em = ImGui::GetFontSize();
        if (!deps_checked) check_deps(false);
        ImGui::BeginDisabled(busy);
        if (ImGui::Button("Check build dependencies")) { check_deps(true); check_rust(); }
        bool need_ok = !root.empty() && deps_err.empty() && (deps_any_to_download(deps) || !consent_given());
        if (need_ok) {
            ImGui::SameLine();
            orange_button_push();
            if (ImGui::Button(deps_any_to_download(deps) ? "Download what's missing..." : "Agree to the downloads...")) consent_modal = true;
            ImGui::PopStyleColor(4);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled(need_ok ? "nothing is downloaded until you agree (once)" : "what the build needs on this system, and whether it's here");
        ImGui::Spacing();
        if (!deps_err.empty()) { ImGui::PushTextWrapPos(0); ImGui::TextColored(colour(root.empty() ? K_NOTE : K_ERROR), "%s", deps_err.c_str()); ImGui::PopTextWrapPos(); return; }

        if (ImGui::BeginTable("deps", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX)) {
            ImGui::TableSetupColumn("ok", ImGuiTableColumnFlags_WidthFixed, em * 1.5f);
            ImGui::TableSetupColumn("Package", ImGuiTableColumnFlags_WidthFixed, em * 14.5f);
            ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, em * 11.5f);
            ImGui::TableSetupColumn("Where", ImGuiTableColumnFlags_WidthStretch);
            for (const Dep &d : deps) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); tick_mark(d.here, d.optional || (!d.system && !d.when.empty()));
                ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(d.name.c_str());
                ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding(); ImGui::TextDisabled("%s", d.version.c_str());
                ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                if (d.system) ImGui::TextColored(colour(d.here ? K_CMD : d.optional ? K_CMD : K_WARN), "%s", d.detail.c_str());
                else if (d.here) ImGui::TextColored(colour(K_CMD), "%s", d.dest.c_str());
                else if (d.when.empty()) ImGui::TextColored(colour(K_WARN), "missing: to download");
                else ImGui::TextDisabled("%s", d.when.c_str());
            }
            ImGui::EndTable();
        }
        {   /* FILTER's two extras: asked for here, never fetched by a build */
            bool have_shaders = true, have_lib = true;
            for (const Dep &d : deps) { if (d.id == "slang-shaders") have_shaders = d.here; if (d.id == "librashader") have_lib = d.here; }
            if (!have_shaders) {
                ImGui::BeginDisabled(busy);
                if (ImGui::Button("Download the FILTER shaders")) download_shaders();
                ImGui::EndDisabled();
                ImGui::SetItemTooltip("libretro's slang-shaders at the commit this project pins, about 85 MB, into the tools folder.\nPressing this is your agreement to that one download. The game runs without them: FILTER then does nothing.");
                ImGui::SameLine();
            }
            if (!have_lib) {
                ImGui::BeginDisabled(busy);
                if (ImGui::Button("Build librashader...")) { check_rust(); lr_modal = true; }
                ImGui::EndDisabled();
                ImGui::SetItemTooltip("FILTER's library, built here from its pinned source with Rust (about ten minutes the first time).\n"
                                      "Kept by --clean, so it is built once.");
                ImGui::SameLine();
            }
            ImGui::TextDisabled(have_shaders && have_lib ? "FILTER has its shaders and its library." : !have_lib ? "FILTER also needs librashader: built here from its source, once." : "Optional: for SCREEN SETTING > FILTER.");
        }
        ImGui::Spacing();

        bool sys_missing = false, to_download = deps_any_to_download(deps), later = false;
        for (const Dep &d : deps) { if (d.system && !d.here && !d.optional) sys_missing = true; if (!d.system && !d.here && !d.optional && !d.when.empty()) later = true; }
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        if (sys_missing) {
            ImGui::TextColored(colour(K_WARN), "Something the build needs isn't on this system, and this project doesn't install software on it.");
#if defined(__APPLE__)
            ImGui::TextUnformatted("Install Apple's command line tools (xcode-select --install in Terminal), then Check again.");
#elif !defined(_WIN32)
            ImGui::TextUnformatted("Install the packages marked above with your package manager (./build.sh in a terminal lists the exact\n"
                                   "command for Debian / Ubuntu, Fedora and Arch), then Check again.");
#endif
        }
        if (!to_download && !later && !sys_missing) ImGui::TextColored(colour(K_GOOD), "Everything is here. Nothing will be downloaded.");
        else if (to_download || later || !consent_given()) {
            /* what will be downloaded, before anything is: the list, where from, the checksum, where to */
            ImGui::SeparatorText(to_download ? "These will be downloaded" : consent_given() ? "Downloaded later, by the build" : "The build will download these when it first needs them");
            ImGui::TextUnformatted("Each file comes from the address shown, at the version this project pins, and is checked against its "
                                   "SHA-256 before it is unpacked (and deleted if it doesn't match). Nothing is installed on your system: "
                                   "the build tools go into the tools folder, the rest into this project's own folders.");
            ImGui::Spacing();
            if (mono) ImGui::PushFont(mono, 13.0f);
            for (const Dep &d : deps) {
                if (d.system || d.here) continue;
                ImGui::TextColored(colour(K_STEP), "%s %s%s", d.name.c_str(), d.version.c_str(), d.optional ? "  (only if you ask)" : d.when.empty() ? "" : "  (later)");
                ImGui::Text("    from     %s", d.url.c_str());
                ImGui::Text("    SHA-256  %s%s", d.sha256.c_str(), d.sha_of.empty() ? "" : ("  (of " + d.sha_of + ")").c_str());
                ImGui::Text("    into     %s", d.dest.c_str());
                if (!d.when.empty()) ImGui::TextDisabled("    when     %s", d.when.c_str());
            }
            if (mono) ImGui::PopFont();
            ImGui::Spacing();
            ImGui::Text("Tools folder:     %s", tools_dir().c_str());
            ImGui::Text("Settings folder:  %s", user_root.c_str());
            ImGui::TextDisabled("Both can be seen, and the tools folder changed, on the Paths tab. The same list is in docs/DEPENDENCIES.md.");
            ImGui::Spacing();
            if (consent_given()) ImGui::TextColored(colour(K_GOOD), "You agreed to these. They are downloaded when the build first needs them.");
            else ImGui::TextDisabled("Nothing is downloaded until you agree: \"%s\" at the top, or Run.", deps_any_to_download(deps) ? "Download what's missing..." : "Agree to the downloads...");
        }
        ImGui::PopTextWrapPos();
    }

    /* ================================================================ Controllers (bring your own device) */
    void pads_refresh() {                                 /* the controllers plugged in now, each with its GUID and which of its kind */
        for (Pad &p : pads) if (p.g) SDL_CloseGamepad(p.g);
        pads.clear();
        int n = 0;
        SDL_JoystickID *ids = SDL_GetGamepads(&n);
        for (int i = 0; ids && i < n; i++) {
            Pad p; p.id = ids[i]; p.g = SDL_OpenGamepad(ids[i]);
            if (!p.g) continue;
            char guid[40]; SDL_GUIDToString(SDL_GetGamepadGUIDForID(ids[i]), guid, sizeof guid);
            p.guid = guid;
            const char *nm = SDL_GetGamepadName(p.g);
            p.name = nm ? nm : "Controller";
            p.index = 0;
            for (const Pad &q : pads) if (q.guid == p.guid) p.index++;
            pads.push_back(p);
        }
        SDL_free(ids);
    }
    /* a player's bubble, at a whole multiple of its 16 pixels (nearest) nearest SIZE, so it stays crisp */
    void bubble_image(int k, float size, float alpha) {
        int n = (int)(size / PADICON_SIZE + 0.5f);
        size = (float)(PADICON_SIZE * (n < 1 ? 1 : n));
        if (k < 0 || k > 3 || !bubble[k]) { ImGui::Dummy(ImVec2(size, size)); return; }
        ImGui::ImageWithBg((ImTextureID)(intptr_t)bubble[k], ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, alpha));
    }
    /* Which bubbles the MODE MENU would show with the controllers plugged in now: as the game decides
       (host_sdl.c, host_pad_connected): a player with keys, or whose controller is plugged in, and then
       controllers no player names, in order, for the players waiting for one (automatic, or a controller
       type with none chosen). Without controllers.cfg: ctl's defaults (controls_defaults). */
    void menu_bubbles(bool shown[CONTROLS_PLAYERS]) const {
        for (int k = 0; k < CONTROLS_PLAYERS; k++) shown[k] = false;
        std::vector<bool> claimed(pads.size(), false);     /* (no file: ctl holds the defaults, as the game uses) */
        for (int k = 0; k < CONTROLS_PLAYERS; k++) {
            const ControlsPlayer &p = ctl.p[k];
            if (controls_auto_keys(&ctl, k)) shown[k] = true;
            if (controls_uses_keys(&p)) for (int b = 0; b < CB_COUNT; b++) if (p.key[b][0]) shown[k] = true;
            for (size_t i = 0; i < pads.size(); i++)
                if (controls_uses_pad(&p) && p.pad_guid[0] && pads[i].guid == p.pad_guid && pads[i].index == p.pad_index) { claimed[i] = true; shown[k] = true; }
        }
        std::vector<bool> got(CONTROLS_PLAYERS, false);
        for (size_t i = 0; i < pads.size(); i++) {         /* the rest go to the players waiting for one, lowest first */
            if (claimed[i]) continue;
            for (int k = 0; k < CONTROLS_PLAYERS; k++)
                if (controls_takes_free_pad(&ctl.p[k]) && !got[k]) { got[k] = true; shown[k] = true; break; }
        }
    }
    const Pad *player_pad(int k) const {
        for (const Pad &p : pads) if (p.guid == ctl.p[k].pad_guid && p.index == ctl.p[k].pad_index) return &p;
        return NULL;
    }
    void controls_changed() {                             /* any change writes the file: from then on the game goes by it */
        SDL_CreateDirectory(user_dir("configs").c_str());
        if (controls_save(&ctl, controls_path().c_str())) { if (!ctl_file) console.note("Controllers: saved to " + controls_path() + ". The game now goes by it.", K_NOTE); ctl_file = true; }
        else console.note("Controllers: could not write " + controls_path(), K_ERROR);
    }
    /* a key or a controller button arrived while a cell was waiting for one (main()'s event loop) */
    bool capture_event(const SDL_Event &e) {
        if (cap_player < 0) return false;
        ControlsPlayer &p = ctl.p[cap_player];
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if (e.key.scancode == SDL_SCANCODE_ESCAPE) { cap_player = -1; return true; }     /* Esc: leave it as it was */
            if (cap_column != 0) return false;
            const char *nm = SDL_GetScancodeName(e.key.scancode);
            if (!nm || !*nm) return true;
            snprintf(p.key[cap_button], sizeof p.key[cap_button], "%s", nm);
        } else if (cap_column == 1 && e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            const Pad *mine = player_pad(cap_player);
            if (!mine || e.gbutton.which != mine->id) return false;
            const char *nm = SDL_GetGamepadStringForButton((SDL_GamepadButton)e.gbutton.button);
            if (!nm) return true;
            snprintf(p.pad[cap_button], sizeof p.pad[cap_button], "b:%s", nm);
        } else if (cap_column == 1 && e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            const Pad *mine = player_pad(cap_player);
            if (!mine || e.gaxis.which != mine->id || (e.gaxis.value > -24000 && e.gaxis.value < 24000)) return false;
            const char *nm = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)e.gaxis.axis);
            if (!nm) return true;
            snprintf(p.pad[cap_button], sizeof p.pad[cap_button], "a:%s%c", nm, e.gaxis.value > 0 ? '+' : '-');
        } else return false;
        cap_player = -1;
        controls_changed();
        return true;
    }
    /* is this player's button down right now, by its keys or its controller (Test) */
    int button_down(int k, int b) const {
        int down = 0;
        const ControlsPlayer &p = ctl.p[k];
        const char *key = controls_uses_keys(&p) ? p.key[b] : controls_auto_keys(&ctl, k) ? CONTROL_DEFAULT_KEYS[b] : "";
        if (key[0]) {
            SDL_Scancode sc = SDL_GetScancodeFromName(key);
            int n = 0; const bool *keys = SDL_GetKeyboardState(&n);
            if (sc != SDL_SCANCODE_UNKNOWN && (int)sc < n && keys[sc]) down |= 1;
        }
        const Pad *pad = controls_uses_pad(&p) ? player_pad(k) : NULL;
        if (pad && p.pad[b][0]) {
            const char *sp = p.pad[b];
            if (!strncmp(sp, "b:", 2)) {
                SDL_GamepadButton btn = SDL_GetGamepadButtonFromString(sp + 2);
                if (btn != SDL_GAMEPAD_BUTTON_INVALID && SDL_GetGamepadButton(pad->g, btn)) down |= 2;
            } else if (!strncmp(sp, "a:", 2)) {
                std::string nm = sp + 2; int dir = 1;
                if (!nm.empty() && (nm.back() == '+' || nm.back() == '-')) { dir = nm.back() == '-' ? -1 : 1; nm.pop_back(); }
                SDL_GamepadAxis ax = SDL_GetGamepadAxisFromString(nm.c_str());
                if (ax != SDL_GAMEPAD_AXIS_INVALID && SDL_GetGamepadAxis(pad->g, ax) * dir > 16000) down |= 2;
            }
        }
        if (pad && b <= CB_RIGHT) {                         /* the left stick is always the d-pad too (as in the game) */
            int lx = SDL_GetGamepadAxis(pad->g, SDL_GAMEPAD_AXIS_LEFTX), ly = SDL_GetGamepadAxis(pad->g, SDL_GAMEPAD_AXIS_LEFTY);
            if ((b == CB_LEFT && lx < -16000) || (b == CB_RIGHT && lx > 16000) || (b == CB_UP && ly < -16000) || (b == CB_DOWN && ly > 16000)) down |= 2;
        }
        return down;
    }
    static std::string pad_binding_label(const char *s) {
        if (!s[0]) return "(not set)";
        if (!strncmp(s, "b:", 2)) return std::string(s + 2);
        if (!strncmp(s, "a:", 2)) return std::string(s + 2);
        return s;
    }
    void controllers_tab(bool busy) {
        const float em = ImGui::GetFontSize();
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextDisabled("Choose how each player plays. Automatic (the default) is what the game always did: controllers go to the "
                            "automatic players in the order they're plugged in, and player 1 also has the keyboard. Read when the game starts.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::BeginDisabled(busy && game_running);
        for (int k = 0; k < CONTROLS_PLAYERS; k++) {           /* which player is shown */
            if (k) ImGui::SameLine();
            char lbl[48]; snprintf(lbl, sizeof lbl, "Player %d", k + 1);
            if (ImGui::RadioButton(lbl, ctl_player == k)) { ctl_player = k; cap_player = -1; }
        }
        ControlsPlayer &p = ctl.p[ctl_player];
        const Pad *mine = player_pad(ctl_player);
        const float lw = em * 7.0f;

        /* the control type: what plays this player */
        static const int TYPE_ORDER[CT_COUNT] = { CT_KEYBOARD, CT_CONTROLLER, CT_BOTH, CT_NONE, CT_AUTO };   /* as listed */
        label("Control type", lw);
        ImGui::SetNextItemWidth(em * 14.0f);
        if (ImGui::BeginCombo("##type", CONTROL_TYPE_LABELS[p.type])) {
            for (int t : TYPE_ORDER)
                if (ImGui::Selectable(CONTROL_TYPE_LABELS[t], p.type == t) && p.type != t) {
                    p.type = t;
                    bool any = false;
                    for (int b = 0; b < CB_COUNT; b++) if (p.key[b][0]) any = true;
                    if (controls_uses_keys(&p) && !any && ctl_player == 0)        /* player 1's first keys: the usual ones */
                        for (int b = 0; b < CB_COUNT; b++) snprintf(p.key[b], sizeof p.key[b], "%s", CONTROL_DEFAULT_KEYS[b]);
                    cap_player = -1;
                    controls_changed();
                }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Keyboard: the keys below.  Controller: the controller and buttons below.\n"
                              "Keyboard/Controller: both (player 1's default).  None: this player can't play.\n"
                              "Automatic: a controller no player names, in the order they're plugged in (player 1 also has the usual keys).");
        ImGui::SameLine();
        ImGui::BeginDisabled(p.type == CT_NONE);
        if (ImGui::Button(ctl_test ? "Stop test" : "Test")) { ctl_test = !ctl_test; cap_player = -1; }
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Press this player's keys and controller buttons: each button below lights up, with the player's\n"
                              "pad icon, while it is down. Nothing is sent to the game.");

        /* the player's controller (Controller, Keyboard/Controller): one of those plugged in (one that isn't any more keeps its name) */
        label("Controller", lw);
        ImGui::SetNextItemWidth(em * 22.0f);
        ImGui::BeginDisabled(!controls_uses_pad(&p));
        std::string preview = p.type == CT_AUTO ? "Automatic" : !controls_uses_pad(&p) ? "(not used)" :
                              !p.pad_guid[0] ? "Any (the first free one)" : mine ? mine->name : std::string(p.pad_name) + " (not plugged in)";
        if (ImGui::BeginCombo("##pad", preview.c_str())) {
            if (ImGui::Selectable("Any (the first free one)", !p.pad_guid[0]) && p.pad_guid[0]) {   /* no controller named: it takes one no player names */
                p.pad_guid[0] = 0; p.pad_name[0] = 0; p.pad_index = 0;
                controls_changed();
            }
            ImGui::SetItemTooltip("The first controller plugged in that no player has chosen, with the buttons below.\n"
                                  "With none plugged in this player has only its keys.");
            for (const Pad &d : pads) {
                int owner = -1;
                for (int k = 0; k < CONTROLS_PLAYERS; k++)
                    if (k != ctl_player && controls_uses_pad(&ctl.p[k]) && d.guid == ctl.p[k].pad_guid && d.index == ctl.p[k].pad_index) owner = k;
                std::string item = d.name + (d.index ? " #" + std::to_string(d.index + 1) : "") + (owner >= 0 ? "  (player " + std::to_string(owner + 1) + "'s)" : "");
                ImGui::PushID((int)d.id);
                if (ImGui::Selectable(item.c_str(), mine == &d)) {
                    if (owner >= 0) { ctl.p[owner].pad_guid[0] = 0; ctl.p[owner].pad_name[0] = 0; }      /* one controller, one player */
                    snprintf(p.pad_guid, sizeof p.pad_guid, "%s", d.guid.c_str());
                    snprintf(p.pad_name, sizeof p.pad_name, "%s", d.name.c_str());
                    p.pad_index = d.index;
                    bool any = false;
                    for (int b = 0; b < CB_COUNT; b++) if (p.pad[b][0]) any = true;
                    if (!any) for (int b = 0; b < CB_COUNT; b++) snprintf(p.pad[b], sizeof p.pad[b], "%s", CONTROL_DEFAULT_PAD[b]);   /* a start: the usual layout */
                    controls_changed();
                }
                ImGui::PopID();
            }
            if (pads.empty()) ImGui::TextDisabled("No controller is plugged in");
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();

        /* the pad icon: the bubble the MODE MENU shows for this player (every type but None), and which it would show now */
        label("Pad icon", lw);
        bool shown_now[CONTROLS_PLAYERS];
        menu_bubbles(shown_now);
        if (p.type == CT_NONE) ImGui::Dummy(ImVec2(PADICON_SIZE * 2 * scale, PADICON_SIZE * 2 * scale));
        else bubble_image(ctl_player, PADICON_SIZE * 2 * scale, 1.0f);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        switch (p.type) {
        case CT_NONE: ImGui::TextDisabled("None: no icon, and this player can't play"); break;
        case CT_AUTO: ImGui::TextDisabled(ctl_player == 0 && controls_auto_keys(&ctl, 0) ? "On the MODE MENU: always (it has the usual keys)"
                                                                                          : "On the MODE MENU while a controller is given to it"); break;
        case CT_CONTROLLER: ImGui::TextDisabled("On the MODE MENU while its controller is plugged in"); break;
        default: ImGui::TextDisabled("On the MODE MENU: always (this player has keys)"); break;
        }
        ImGui::SameLine(0, em * 1.5f);
        ImGui::TextDisabled("Now:");
        for (int k = 0; k < CONTROLS_PLAYERS; k++) {
            ImGui::SameLine(0, em * 0.25f);
            bubble_image(k, PADICON_SIZE * scale, shown_now[k] ? 1.0f : 0.2f);
            ImGui::SetItemTooltip(shown_now[k] ? "Player %d's icon would show on the MODE MENU now" : "Player %d's icon would not show now", k + 1);
        }

        /* the buttons: a key and a controller button for each, where the type uses them */
        const bool use_keys = controls_uses_keys(&p), use_pad = controls_uses_pad(&p);
        if (ImGui::BeginTable("map", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
                              ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, em * 7.0f);
            ImGui::TableSetupColumn("Keyboard", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int b = 0; b < CB_COUNT; b++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                int down = ctl_test ? button_down(ctl_player, b) : 0;
                ImGui::AlignTextToFramePadding();
                if (down) {                                       /* by a key or a button: the player's icon, as the game shows it */
                    ImGui::TextColored(colour(K_GOOD), "%s", CONTROL_BUTTONS[b].label);
                    ImGui::SameLine(0, em * 0.4f);
                    bubble_image(ctl_player, PADICON_SIZE * scale, 1.0f);
                } else ImGui::TextUnformatted(CONTROL_BUTTONS[b].label);
                for (int col = 0; col < 2; col++) {
                    ImGui::TableNextColumn();
                    ImGui::PushID(b * 2 + col);
                    bool used = col == 0 ? use_keys : use_pad;
                    bool waiting = cap_player == ctl_player && cap_button == b && cap_column == col;
                    const char *cur = col == 0 ? p.key[b] : p.pad[b];
                    std::string text = waiting ? (col == 0 ? "press a key... (Esc: cancel)" : "press a button... (Esc: cancel)")
                                     : !used ? (p.type == CT_AUTO ? (col == 0 && controls_auto_keys(&ctl, ctl_player) ? CONTROL_DEFAULT_KEYS[b] : col == 1 ? pad_binding_label(CONTROL_DEFAULT_PAD[b]) : std::string("-"))
                                                                  : std::string("-"))
                                     : col == 0 ? (cur[0] ? std::string(cur) : std::string("(not set)")) : pad_binding_label(cur);
                    ImGui::BeginDisabled(ctl_test || !used || (col == 1 && !mine));
                    if (waiting) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    if ((!cur[0] || !used) && !waiting) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    if (ImGui::Button(text.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - em * 2.0f, 0))) { cap_player = ctl_player; cap_button = b; cap_column = col; }
                    if ((!cur[0] || !used) && !waiting) ImGui::PopStyleColor();
                    if (waiting) ImGui::PopStyleColor();
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x") && cur[0]) { if (col == 0) p.key[b][0] = 0; else p.pad[b][0] = 0; controls_changed(); }
                    ImGui::SetItemTooltip("Unset");
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::BeginDisabled(!use_keys);
        if (ImGui::Button("Default keys")) { for (int b = 0; b < CB_COUNT; b++) snprintf(p.key[b], sizeof p.key[b], "%s", CONTROL_DEFAULT_KEYS[b]); controls_changed(); }
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Arrows, Z X A S, Q W, E R, Enter, Backspace. Two players can't share a key: give the second their own.");
        ImGui::SameLine();
        ImGui::BeginDisabled(!use_pad || !p.pad_guid[0]);
        if (ImGui::Button("Default buttons")) { for (int b = 0; b < CB_COUNT; b++) snprintf(p.pad[b], sizeof p.pad[b], "%s", CONTROL_DEFAULT_PAD[b]); controls_changed(); }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
    }

    /* ================================================================ Extensions */
    void extensions_tab(bool busy) {
        const float em = ImGui::GetFontSize();
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextDisabled("What this project adds to the PSP game, each on its own switch. With all of them off the game plays as the original does: "
                            "its own menus and saves, one player.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::BeginDisabled(busy);
        bool changed = false;
        for (int f = 0; f < FEAT_COUNT; f++) {
            int need = FEATURES[f].needs;
            bool blocked = need >= 0 && !set.feature[need];
            ImGui::BeginDisabled(blocked);
            bool v = set.feature[f] && !blocked;
            if (ImGui::Checkbox(FEATURES[f].title, &v)) { set.feature[f] = v; changed = true; }
            ImGui::EndDisabled();
            std::string tip = FEATURES[f].what;
            if (blocked) tip += std::string("\n\nOff, because it needs \"") + FEATURES[need].title + "\".";
            if (f == FEAT_SHADERS) tip += "\n\nNeeds the shaders and the librashader library: see Dependencies.";
            ImGui::SetItemTooltip("%s", tip.c_str());
            ImGui::SameLine(em * 17.0f);
            ImGui::TextDisabled("%s%s", FEATURES[f].name, FEATURES[f].on_by_default ? "" : "  (off by default: experimental)");
        }
        ImGui::Spacing();
        if (ImGui::Button("Defaults")) { for (int f = 0; f < FEAT_COUNT; f++) set.feature[f] = FEATURES[f].on_by_default != 0; changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("All off (the PSP game)")) { for (int f = 0; f < FEAT_COUNT; f++) set.feature[f] = false; changed = true; }
        ImGui::EndDisabled();
        if (changed) settings_save(set, settings_path);
        ImGui::Spacing();
        ImGui::Text("The game will start with:  --features %s", features_arg().c_str());
        std::string last = last_run("features");
        if (!last.empty()) ImGui::TextDisabled("The last run had:  %s", last.c_str());
    }

    /* ================================================================ Build / Run */
    void run_tab(bool busy) {
        const float em = ImGui::GetFontSize();
        const bool repo = !root.empty();
        if (ImGui::BeginTable("options", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::SeparatorText("Build");
            ImGui::BeginDisabled(busy || !repo);
            label("Preset", em * 5.5f);
            ImGui::SetNextItemWidth(-em);
            ImGui::Combo("##preset", &set.preset, PRESETS, 2);
            ImGui::SetItemTooltip("--preset release: build/   --preset debug: build-debug/ (debug info, unoptimised)");
            ImGui::Checkbox("Extract the disc again", &set.extract);
            ImGui::SetItemTooltip("--extract: for this Run or Build only. A different ISO is extracted again by itself.");
            ImGui::Checkbox("Run the tests after building", &set.tests);
            ImGui::SetItemTooltip("--test: ctest (the parity tests take a long time; docs/TESTS.md says what each is for)");
            bool run = repo ? set.run : true;
            if (ImGui::Checkbox("Run after building", &run) && repo) set.run = run;
            ImGui::SetItemTooltip("Checked: Run builds what changed, then starts the game (--run).\nUnchecked: the button only builds.");
            ImGui::EndDisabled();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - em);
            ImGui::TextDisabled("From a terminal, build.cmd / build.sh do the same (docs/BUILDING.md).");
            ImGui::PopTextWrapPos();

            ImGui::TableNextColumn();
            ImGui::SeparatorText("Game");
            ImGui::BeginDisabled(busy || (repo && !set.run));
            ImGui::Checkbox("Full screen", &set.fullscreen);
            ImGui::SetItemTooltip("Checked (the default): --fullscreen. Unchecked: --windowed. F11 or Alt+Enter switch while playing.");
            label("Logging", em * 5.5f);
            ImGui::SetNextItemWidth(-em);
            ImGui::Combo("##log", &set.log_level, LOG_LEVELS, 6);
            ImGui::SetItemTooltip("--log-level: what the game writes to the console below and to logs/a3mx.log.\n"
                                  "OFF writes nothing but a fatal error; each level adds to the one above it.\n"
                                  "Reporting a problem? Choose INFO (or DEBUG) and make it happen again.\n"
                                  "VERBOSE also logs every OS call (slow).");
            label("Renderer", em * 5.5f);
            ImGui::SetNextItemWidth(-em);
            if (ImGui::BeginCombo("##renderer", RENDERERS[set.renderer].label)) {
                for (int i = 0; i < N_RENDERERS; i++) if (ImGui::Selectable(RENDERERS[i].label, set.renderer == i)) set.renderer = i;
                ImGui::EndCombo();
            }
            ImGui::SetItemTooltip("--renderer. OpenGL: the GPU renderer. Software: the reference renderer, drawn on the CPU.\n"
                                  "Vulkan, DirectX 12 (Windows) and Metal / MoltenVK (macOS) are listed for when they are written:\n"
                                  "choosing one today runs OpenGL, and the game's log says so.");
            label("Resolution", em * 5.5f);
            ImGui::SetNextItemWidth(-em);
            ImGui::BeginDisabled(set.renderer == 1);
            ImGui::Combo("##scale", &set.scale, SCALES, 9);
            ImGui::EndDisabled();
            ImGui::SetItemTooltip("The GPU renderer's internal resolution (--scale N). Auto: enough to cover your monitor,\n"
                                  "at most 4x. Fights running slowly? Try 2x or 1x: the GPU's work grows with the square.");
            gpu_row(em);
            label("Options", em * 5.5f);
            ImGui::SetNextItemWidth(-em);
            if (ImGui::InputTextWithHint("##extra", "more game options, e.g. --p2 human", extra_buf.data(), extra_buf.size()))
                set.extra = extra_buf.data();
            ImGui::EndDisabled();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + em * 5.5f - ImGui::GetStyle().CellPadding.x);
            if (link("See option details") || show_at_start == "options") { ImGui::OpenPopup("The game's options"); if (show_at_start == "options") show_at_start.clear(); }
            options_window();
            ImGui::EndTable();
        }
        ImGui::Spacing();

        /* ---- the button */
        DiscResult dr;
        bool disc_ok = !repo || disc_allows_build(dr);
        const char *what = (repo && !set.run) ? "Build" : "Run";
        float bw = em * 7.5f, sw = em * 5.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - bw - sw - ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(busy || !disc_ok);
        orange_button_push();
        if (ImGui::Button(what, ImVec2(bw, 0)) || (!busy && disc_ok && cap_player < 0 && ImGui::IsKeyPressed(ImGuiKey_F5, false))) start();
        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();
        if (!disc_ok) ImGui::SetItemTooltip("Choose the US disc (ULUS-10062) on the Paths tab first");
        else ImGui::SetItemTooltip("F5");
        ImGui::SameLine();
        /* Stop is for a build or a download: the game is closed from its own window, even full screen */
        ImGui::BeginDisabled(!busy || game_running);
        if (ImGui::Button("Stop", ImVec2(sw, 0))) { child.stop(); console.note("Stopping...", K_WARN); }
        ImGui::EndDisabled();
        if (game_running) ImGui::SetItemTooltip("The game is running: close it with " QUIT_KEYS " or its window's close button");
        else ImGui::SetItemTooltip("Stops the build, and everything it started");
        if (!disc_ok) { ImGui::SameLine(ImGui::GetStyle().WindowPadding.x); ImGui::TextColored(colour(K_WARN), "Choose your disc image on the Paths tab."); }
    }

    /* ================================================================ the window */
    void frame() {
        set_theme_now();
        ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("launcher", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const float em = ImGui::GetFontSize();
        ImGuiStyle &st = ImGui::GetStyle();
        const bool busy = child.busy();
        if (want_tab >= 0) { set.tab = want_tab; want_tab = -1; }
        if (show_at_start == "consent") { consent_modal = true; show_at_start.clear(); }          /* --show, for screenshots */
        if (show_at_start == "librashader") { lr_modal = true; show_at_start.clear(); }

        /* three bands: the side and the tab's page; the console, on every tab; the status line */
        const float footer = ImGui::GetFrameHeightWithSpacing();
        const float head = ImGui::GetFrameHeightWithSpacing();          /* the console's title row */
        float avail = ImGui::GetContentRegionAvail().y;
        float con_h = (float)set.console_h * scale;
        float min_top = em * 16.0f, min_con = em * 4.0f;
        if (con_h > avail - footer - head - min_top) con_h = avail - footer - head - min_top;
        if (con_h < min_con) con_h = min_con;
        float top_h = avail - footer - head - con_h - st.ItemSpacing.y * 2 - 6 * scale;
        float side_w = em * 11.5f;

        ImGui::BeginChild("side", ImVec2(side_w, top_h), ImGuiChildFlags_None, ImGuiWindowFlags_None);   /* scrolls if the window is short */
        sidebar(top_h);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("page", ImVec2(0, top_h), ImGuiChildFlags_Borders);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(TAB_NAMES[set.tab]);
        ImGui::PopStyleColor();
        ImGui::Separator();
        switch (set.tab) {
        case T_PATHS: paths_tab(busy); break;
        case T_DEPS: deps_tab(busy); break;
        case T_CONTROLLERS: controllers_tab(busy); break;
        case T_EXTENSIONS: extensions_tab(busy); break;
        default: run_tab(busy); break;
        }
        ImGui::EndChild();

        /* ---- the bar between the page and the console: drag it to give the console more or less room */
        ImGui::InvisibleButton("split", ImVec2(-1, 6 * scale));
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (ImGui::IsItemActive()) set.console_h = (int)((con_h - ImGui::GetIO().MouseDelta.y) / scale);
        if (set.console_h < 60) set.console_h = 60;
        {
            ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(a.x, (a.y + b.y) * 0.5f), ImVec2(b.x, (a.y + b.y) * 0.5f),
                                                ImGui::GetColorU32(ImGui::IsItemActive() || ImGui::IsItemHovered() ? ImGuiCol_CheckMark : ImGuiCol_Border));
        }

        /* ---- the console: anchored here on every tab, showing whatever was started from any of them */
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Console");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, dark ? ImVec4(0.06f, 0.06f, 0.07f, 1) : ImVec4(1, 1, 1, 1));
        ImGui::BeginChild("log", ImVec2(0, -footer), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        if (mono) ImGui::PushFont(mono, 14.0f);
        int n = (int)console.lines.size() + (console.partial.empty() ? 0 : 1);
        ImGuiListClipper clip;
        clip.Begin(n);
        while (clip.Step())
            for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
                bool last = i == (int)console.lines.size();
                const std::string &t = last ? console.partial : console.lines[(size_t)i].text;
                ImGui::PushStyleColor(ImGuiCol_Text, colour(last ? Console::classify(t) : console.lines[(size_t)i].kind));
                ImGui::TextUnformatted(t.c_str(), t.c_str() + t.size());
                ImGui::PopStyleColor();
            }
        /* New lines bring the view to the end if it was there (within a line) when they came; scrolled
         * back, it stays put and the lines still come in. The clipper leaves the cursor on the last line
         * drawn, not the last line, so scroll by value; it is clamped to the new end next frame. */
        if (console.scroll_to_end && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeightWithSpacing() * 1.5f)
            ImGui::SetScrollY(1e9f);
        console.scroll_to_end = false;
        if (mono) ImGui::PopFont();
        ImGui::EndChild();
        ImGui::PopStyleColor();

        /* ---- status, and the version in the bottom right corner */
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - st.WindowPadding.y - ImGui::GetFrameHeight());   /* the last row, clear of the edge */
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(colour(status_kind), "%s", status.c_str());
        /* the console's three buttons, in the bottom right corner, all the same kind so they line up */
        float bw3 = ImGui::CalcTextSize("Log folderCopyClear").x + st.FramePadding.x * 6 + st.ItemSpacing.x * 2;
        ImGui::SameLine(ImGui::GetWindowWidth() - st.WindowPadding.x - bw3);
        if (ImGui::Button("Log folder")) open_folder(user_dir("logs"));
        ImGui::SetItemTooltip("Opens your logs folder: a3mx.log (the game's: attach it to a bug report),\nlast_run.txt, and launcher.log (this console).");
        ImGui::SameLine();
        if (ImGui::Button("Copy")) SDL_SetClipboardText(console.all().c_str());
        ImGui::SetItemTooltip("Copy the whole console");
        ImGui::SameLine();
        if (ImGui::Button("Clear")) { console.lines.clear(); console.partial.clear(); }
        ImGui::SetItemTooltip("Clears the view. launcher.log in the logs folder keeps everything.");

        /* closing while something runs */
        if (quit_asked) { ImGui::OpenPopup("Still running"); quit_asked = false; }
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Still running", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(game_running ? "The game is still running.\nClose it too, and the launcher?"
                                                : "A build or a download is still running.\nStop it and close the launcher?");
            ImGui::Spacing();
            if (ImGui::Button(game_running ? "Close both" : "Stop and close", ImVec2(em * 8, 0))) {
                if (game_running) close_both(); else { child.stop(); quit = true; }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(em * 6, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        consent_window();
        librashader_window();
        ImGui::End();
    }
};

/* a font from the system if there is one, else ImGui's own */
static ImFont *system_font(const char *const *paths, float size) {
    for (int i = 0; paths[i]; i++)
        if (is_file(paths[i])) {
            ImFont *f = ImGui::GetIO().Fonts->AddFontFromFileTTF(paths[i], size);
            if (f) return f;
        }
    return NULL;
}
static const char *const UI_FONTS[] = {
    "C:/Windows/Fonts/segoeui.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf", "/Library/Fonts/Arial.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf", "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", NULL };
static const char *const MONO_FONTS[] = {
    "C:/Windows/Fonts/consola.ttf", "/System/Library/Fonts/Menlo.ttc", "/System/Library/Fonts/Monaco.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", "/usr/share/fonts/TTF/DejaVuSansMono.ttf", "/usr/share/fonts/dejavu-sans-mono-fonts/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", NULL };

static std::string find_root(const std::string &base) {
    std::string d = base;
    for (int i = 0; i < 5 && !d.empty(); i++) {          /* build-launcher/ is one level below the checkout */
        if (is_file(join(d, "tools/build.py"))) return d;
        d = parent(d);
    }
    char *cwd = SDL_GetCurrentDirectory();
    std::string c = cwd ? cwd : "";
    SDL_free(cwd);
    if (!c.empty() && is_file(join(c, "tools/build.py"))) return c;
    return std::string();
}

int main(int argc, char **argv) {
    std::string screenshot, settings_arg, root_arg, theme_arg, show_arg, tab_arg, user_arg;
    long frames = 0;
    bool autostart = false, list_gpus = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) screenshot = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--settings") && i + 1 < argc) settings_arg = argv[++i];
        else if (!strcmp(argv[i], "--root") && i + 1 < argc) root_arg = argv[++i];
        else if (!strcmp(argv[i], "--theme") && i + 1 < argc) theme_arg = argv[++i];
        else if (!strcmp(argv[i], "--show") && i + 1 < argc) show_arg = argv[++i];
        else if (!strcmp(argv[i], "--tab") && i + 1 < argc) tab_arg = argv[++i];
        else if (!strcmp(argv[i], "--user-dir") && i + 1 < argc) user_arg = argv[++i];
        else if (!strcmp(argv[i], "--autostart")) autostart = true;
        else if (!strcmp(argv[i], "--gpus")) list_gpus = true;
    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    SDL_setenv_unsafe("PYTHONUNBUFFERED", "1", 1);      /* build.py's output as it is printed, not in blocks */
    SDL_setenv_unsafe("PYTHONIOENCODING", "utf-8", 1);
    SDL_setenv_unsafe("A3MX_PROGRESS", "1", 1);         /* extract_assets.py's progress bars, redrawn in place */

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    App app;
    const char *b = SDL_GetBasePath();
    app.base = untranslocated(b ? b : "");
    app.root = root_arg.empty() ? find_root(app.base) : root_arg;
    /* Your a3mx folder (src/platform/userdir.h): the system's per-user folder, or beside the game if
     * that can't be written. The game is started with the same one (--user-dir). The launcher's own
     * settings are launcher/launcher.ini in it; one from before 0.60.0 (saves/launcher.ini in the
     * checkout) is read once, when there is none yet. */
    {
        char root_buf[1100];
        std::string beside = app.home_dir();
        app.user_kind = userdir_choose(root_buf, sizeof root_buf, user_arg.empty() ? NULL : user_arg.c_str(), beside.c_str());
        app.user_root = root_buf;
    }
    app.settings_path = !settings_arg.empty() ? settings_arg : join(app.user_dir("launcher"), "launcher.ini");
    bool old_settings = false;
    if (settings_arg.empty() && !is_file(app.settings_path)) {
        std::string old = join(join(app.home_dir(), "saves"), "launcher.ini");
        if (is_file(old)) { settings_load(app.set, old); old_settings = true; }
    }
    settings_load(app.set, app.settings_path);
    app.gpus = gpu_list(getenv("A3MX_TEST_SYSFS") ? getenv("A3MX_TEST_SYSFS") : "", getenv("A3MX_TEST_PCI_IDS") ? getenv("A3MX_TEST_PCI_IDS") : "");
    if (list_gpus) {            /* --gpus: the list, fastest first, then quit */
        for (const Gpu &g : app.gpus)
            printf("%s\t%s%s%s\n", g.id.c_str(), g.name.c_str(), g.integrated ? " (integrated)" : "", g.choosable ? "" : " (can't be chosen)");
        if (app.gpus.empty()) printf("(none found: Automatic only)\n");
        return 0;
    }
    if (theme_arg == "light") app.theme = THEME_LIGHT;
    else if (theme_arg == "dark") app.theme = THEME_DARK;
    app.show_at_start = show_arg;
    snprintf(app.iso_buf.data(), app.iso_buf.size(), "%s", app.set.iso.c_str());
    snprintf(app.extra_buf.data(), app.extra_buf.size(), "%s", app.set.extra.c_str());
    app.folders_to_bufs();
    for (int t = 0; t < T_COUNT; t++) if (tab_arg == TAB_ARGS[t]) app.set.tab = t;
    if (old_settings) settings_save(app.set, app.settings_path);
    /* the console, kept: logs/launcher.log holds this session's (the last one's is launcher.log.1) */
    if (screenshot.empty()) app.open_log();
    /* configs/controllers.cfg, if there is one; else what a first change would start from */
    app.ctl_file = controls_load(&app.ctl, app.controls_path().c_str()) != 0;
    if (!app.ctl_file) controls_defaults(&app.ctl);

    float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (scale <= 0) scale = 1;
    app.scale = scale;
    app.window = SDL_CreateWindow("A3MX - Launcher", (int)(app.set.win_w * scale), (int)(app.set.win_h * scale),
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!app.window) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }
    SDL_SetWindowMinimumSize(app.window, (int)(820 * scale), (int)(560 * scale));
    app.renderer = SDL_CreateRenderer(app.window, NULL);
    if (!app.renderer) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }
    SDL_SetRenderVSync(app.renderer, 1);
    SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(app.window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;                       /* our own settings file instead */
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetStyle().FontScaleDpi = scale;
    app.dark = !(app.theme == THEME_AUTO ? system_dark() : app.theme == THEME_DARK);   /* differs: applied on the first frame */
    app.set_theme_now();
    if (!system_font(UI_FONTS, 16.0f)) io.Fonts->AddFontDefault();
    app.mono = system_font(MONO_FONTS, 14.0f);
    ImGui_ImplSDL3_InitForSDLRenderer(app.window, app.renderer);
    ImGui_ImplSDLRenderer3_Init(app.renderer);

    {   /* the picture at the top of the side: res/launcher_logo.png (docs/images/a3mx.png, made smaller), built in */
        PngImage img; char perr[96];
        if (png_decode(LOGO_PNG, sizeof LOGO_PNG, &img, perr, sizeof perr) == 0) {
            SDL_Surface *surf = SDL_CreateSurfaceFrom(img.w, img.h, SDL_PIXELFORMAT_RGBA32, img.rgba, img.w * 4);
            if (surf) {
                app.logo = SDL_CreateTextureFromSurface(app.renderer, surf);
                if (app.logo) { SDL_SetTextureScaleMode(app.logo, SDL_SCALEMODE_LINEAR); app.logo_w = img.w; app.logo_h = img.h; }
                SDL_DestroySurface(surf);
            }
            png_free(&img);
        }
    }
    for (int k = 0; k < 4; k++) {         /* the player bubbles, from the same pixels as the game's */
        uint32_t px[PADICON_SIZE][PADICON_SIZE], rgba[PADICON_SIZE * PADICON_SIZE];
        padicon_bubble(k, px);
        for (int y = 0; y < PADICON_SIZE; y++) for (int x = 0; x < PADICON_SIZE; x++) {
            uint32_t v = px[y][x];
            unsigned char *o = (unsigned char *)&rgba[y * PADICON_SIZE + x];
            o[0] = (unsigned char)(v >> 16); o[1] = (unsigned char)(v >> 8); o[2] = (unsigned char)v; o[3] = v ? 255 : 0;
        }
        SDL_Surface *surf = SDL_CreateSurfaceFrom(PADICON_SIZE, PADICON_SIZE, SDL_PIXELFORMAT_RGBA32, rgba, PADICON_SIZE * 4);
        if (surf) {
            app.bubble[k] = SDL_CreateTextureFromSurface(app.renderer, surf);
            if (app.bubble[k]) SDL_SetTextureScaleMode(app.bubble[k], SDL_SCALEMODE_NEAREST);
            SDL_DestroySurface(surf);
        }
    }
    app.pads_refresh();
    app.check_rust();
    app.check_work();
    app.choose_disc();
    app.check_deps(false);
    if (app.root.empty()) app.console.note("No source checkout found above " + app.base + ": Build is off; Run starts the game beside the launcher. " + no_folder_hint(), K_WARN);
    else app.console.note("Source checkout: " + app.root, K_NOTE);
    app.console.note(std::string(app.user_kind == 1 ? "Your a3mx folder: " : app.user_kind == 2 ? "Your user folder can't be written: saves, configs and logs are beside the game, in " : "No folder to keep settings in could be written: ") + app.user_root,
                     app.user_kind == 1 ? K_NOTE : K_WARN);
    if (old_settings) app.console.note("Settings from before 0.60.0 (saves/launcher.ini) were taken over.", K_NOTE);

    long frame = 0;
    while (!app.quit) {
        SDL_Event e;
        /* idle: wake for input, or every 50 ms for the child's output and the disc check */
        /* While the game runs the launcher is behind it: wake every 100 ms for its output, and draw only
         * when there is something new (output, input, a change of state). With Logging OFF the game
         * writes nothing, so nothing is redrawn. Otherwise 16 ms while a build runs, 50 ms idle. */
        bool behind = app.game_running && screenshot.empty();
        int wait_ms = behind ? 100 : app.child.busy() || app.disc.busy() || !screenshot.empty() || autostart || app.ctl_test || app.cap_player >= 0 ? 16 : 50;
        bool had_events = false, had_input = false;
        if (SDL_WaitEventTimeout(&e, wait_ms)) {
            had_events = true;
            do {
                if (e.type == SDL_EVENT_GAMEPAD_ADDED || e.type == SDL_EVENT_GAMEPAD_REMOVED) { app.pads_refresh(); had_input = true; }
                if (app.capture_event(e)) { had_input = true; continue; }     /* a key or button for the Controllers tab: not ImGui's */
                ImGui_ImplSDL3_ProcessEvent(&e);
                switch (e.type) {           /* the person using the launcher, not the game covering it */
                case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: case SDL_EVENT_TEXT_INPUT: case SDL_EVENT_MOUSE_MOTION:
                case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP: case SDL_EVENT_MOUSE_WHEEL:
                case SDL_EVENT_WINDOW_FOCUS_GAINED: case SDL_EVENT_WINDOW_RESIZED: case SDL_EVENT_WINDOW_RESTORED:
                case SDL_EVENT_DROP_FILE: had_input = true; break;
                default: break;
                }
#if !defined(_WIN32) && !defined(__APPLE__)
                /* Linux: Ctrl+Q closes, as most programs there do (Windows' Alt+F4 and macOS's Cmd+Q
                 * arrive as SDL_EVENT_QUIT by themselves) */
                if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q && (e.key.mod & SDL_KMOD_CTRL) && !ImGui::GetIO().WantTextInput) {
                    if (app.child.busy()) app.quit_asked = true; else app.quit = true;
                }
#endif
                if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && e.window.windowID == SDL_GetWindowID(app.window))) {
                    if (app.child.busy()) app.quit_asked = true; else app.quit = true;
                }
                /* SDL_EVENT_SYSTEM_THEME_CHANGED needs nothing: set_theme_now() reads the system's each frame */
            } while (SDL_PollEvent(&e));
        }
        bool changed = app.tick();
        if (SDL_GetWindowFlags(app.window) & SDL_WINDOW_MINIMIZED) { SDL_Delay(30); continue; }
        /* While the game runs the launcher stays out of its way: drawing (presenting a frame) while the
         * game is full screen can make Windows take the game off its direct path to the screen for a
         * moment, a hitch. So: drawn after input to the launcher, after a change of state, and for
         * new output only when the launcher has the focus, at most every 250 ms (0.51.0: no Auto-refresh).
         * The output is read all the time anyway (process.cpp's reader thread). Otherwise a couple of
         * frames after anything new, so ImGui settles (hover, popups, the scroll's clamp). */
        static int settle = 0;
        static Uint64 drawn_at = 0;
        bool focused = (SDL_GetWindowFlags(app.window) & SDL_WINDOW_INPUT_FOCUS) != 0;
        if (behind) {
            bool output_due = changed && focused && SDL_GetTicks() - drawn_at >= 250;
            if (had_input || output_due || app.quit_asked || app.closing_since) settle = 3;
            if (settle == 0) continue;
        } else if (had_events || changed || app.quit_asked) settle = 3;
        if (settle > 0) settle--;
        drawn_at = SDL_GetTicks();
        if (autostart && frame == 2) app.start();

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        app.frame();
        ImGui::Render();
        SDL_SetRenderScale(app.renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImVec4 bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        SDL_SetRenderDrawColorFloat(app.renderer, bg.x, bg.y, bg.z, 1);
        SDL_RenderClear(app.renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), app.renderer);
        if (!screenshot.empty() && frame >= frames) {
            SDL_Surface *s = SDL_RenderReadPixels(app.renderer, NULL);
            if (s) { SDL_SaveBMP(s, screenshot.c_str()); SDL_DestroySurface(s); }
            app.child.stop();
            app.quit = true;
        }
        SDL_RenderPresent(app.renderer);
        frame++;
    }
    /* stop whatever is still running, then keep the window's size for next time */
    if (app.child.busy()) {
        app.child.stop();
        for (int i = 0; i < 100 && app.child.busy(); i++) { std::string got; app.child.poll(got); SDL_Delay(30); }
        if (app.child.busy() && app.game_running) app.child.end_game(app.run_exe());   /* outside the job (process.h) */
    }
    if (screenshot.empty()) {
        int w, h;
        SDL_GetWindowSize(app.window, &w, &h);
        app.set.win_w = (int)(w / scale);
        app.set.win_h = (int)(h / scale);
        settings_save(app.set, app.settings_path);
    }
    if (app.console.file) fclose(app.console.file);
    for (App::Pad &pd : app.pads) if (pd.g) SDL_CloseGamepad(pd.g);
    if (app.logo) SDL_DestroyTexture(app.logo);
    for (SDL_Texture *t : app.bubble) if (t) SDL_DestroyTexture(t);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 0;
}
