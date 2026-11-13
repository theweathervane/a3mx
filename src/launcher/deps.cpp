#include "deps.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstring>
#include <map>

namespace {
/* ---- a small JSON reader: enough for pins.json (objects, arrays, strings; numbers and words skipped) */
struct J {
    enum { NUL, STR, OBJ, ARR } t = NUL;
    std::string s;
    std::vector<std::pair<std::string, J>> o;
    std::vector<J> a;
    const J *get(const char *k) const { for (auto &kv : o) if (kv.first == k) return &kv.second; return nullptr; }
    std::string str(const char *k) const { const J *v = get(k); return v && v->t == STR ? v->s : std::string(); }
};
struct P {
    const std::string &t; size_t i = 0; bool bad = false;
    explicit P(const std::string &text) : t(text) {}
    void ws() { while (i < t.size() && (t[i] == ' ' || t[i] == '\n' || t[i] == '\r' || t[i] == '\t')) i++; }
    std::string string() {
        std::string out;
        if (i >= t.size() || t[i] != '"') { bad = true; return out; }
        for (i++; i < t.size() && t[i] != '"'; i++) {
            if (t[i] == '\\' && i + 1 < t.size()) {
                char c = t[++i];
                if (c == 'n') out += '\n'; else if (c == 't') out += '\t';
                else if (c == 'u' && i + 4 < t.size()) { unsigned v = (unsigned)strtoul(t.substr(i + 1, 4).c_str(), nullptr, 16); i += 4; out += v < 128 ? (char)v : '?'; }
                else out += c;
            } else out += t[i];
        }
        if (i >= t.size()) bad = true; else i++;
        return out;
    }
    J value() {
        J v; ws();
        if (i >= t.size()) { bad = true; return v; }
        if (t[i] == '"') { v.t = J::STR; v.s = string(); }
        else if (t[i] == '{') {
            v.t = J::OBJ; i++; ws();
            while (!bad && i < t.size() && t[i] != '}') {
                std::string k = string(); ws();
                if (i >= t.size() || t[i] != ':') { bad = true; break; }
                i++;
                v.o.emplace_back(k, value()); ws();
                if (i < t.size() && t[i] == ',') { i++; ws(); }
            }
            if (i < t.size()) i++; else bad = true;
        } else if (t[i] == '[') {
            v.t = J::ARR; i++; ws();
            while (!bad && i < t.size() && t[i] != ']') {
                v.a.push_back(value()); ws();
                if (i < t.size() && t[i] == ',') { i++; ws(); }
            }
            if (i < t.size()) i++; else bad = true;
        } else while (i < t.size() && t[i] != ',' && t[i] != '}' && t[i] != ']') i++;   /* a number, true, false, null */
        return v;
    }
};

std::string join(const std::string &a, const std::string &b) { return a.empty() ? b : (a.back() == '/' || a.back() == '\\') ? a + b : a + "/" + b; }
bool exists(const std::string &p) { SDL_PathInfo i; return SDL_GetPathInfo(p.c_str(), &i); }
std::string read_text(const std::string &p) {
    size_t n = 0; void *d = SDL_LoadFile(p.c_str(), &n);
    if (!d) return std::string();
    std::string s((const char *)d, n); SDL_free(d); return s;
}
#if !defined(_WIN32) && !defined(__APPLE__)
/* a development library, found the way SDL's configure finds it */
bool pkg_config(const char *name) {
    std::string cmd = std::string("pkg-config --exists ") + name + " >/dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}
#endif
#ifndef _WIN32
/* a program on the PATH, as the build would find it */
std::string on_path(const char *name) {
    const char *path = getenv("PATH");
    if (!path) return std::string();
    std::string p = path; size_t at = 0;
    while (at <= p.size()) {
        size_t e = p.find(':', at);
        if (e == std::string::npos) e = p.size();
        std::string f = join(p.substr(at, e - at), name);
        SDL_PathInfo i;
        if (e > at && SDL_GetPathInfo(f.c_str(), &i) && i.type == SDL_PATHTYPE_FILE) return f;
        at = e + 1;
    }
    return std::string();
}
#endif

/* what this system's build wants: tools/downloads.py's WANTED and tools/build.py's PROBES, kept the same */
struct Want { const char *id, *dest, *probe, *when; bool tool; };
const char *const SDL_WHEN = "its source, by CMake, the first time the launcher or the game is built";
const char *const SH_WHEN = "only if you ask (the button below): the shaders SCREEN SETTING > FILTER draws with, about 85 MB";
const char *const WS_WHEN = "when your disc is first extracted, for WIDE FORCED (not with Widescreen off)";
#if defined(_WIN32)
const Want WANTED[] = {
    { "python-embed", "python", "python.exe", "only if this PC has no Python 3.9+ that can check HTTPS certificates (build.cmd decides)", true },
    { "llvm-mingw", "llvm-mingw", "bin/clang.exe", nullptr, true },
    { "cmake", "cmake", "bin/cmake.exe", nullptr, true },
    { "ninja", "ninja", "ninja.exe", nullptr, true },
#elif defined(__APPLE__)
const Want WANTED[] = {
    { "cmake-macos", "cmake-macos", "CMake.app/Contents/bin/cmake", nullptr, true },
    { "ninja-macos", "ninja-macos", "ninja", nullptr, true },
#else
const Want WANTED[] = {
#endif
    { "slang-shaders", "slang-shaders", "crt/crt-geom.slangp", SH_WHEN, true },
    { "sdl3", "build/_deps and build-launcher/_deps", nullptr, SDL_WHEN, false },
    { "widescreen", "work/sfa3-PSP-widescreen", "sfa3_ws_patternpatcher.py", WS_WHEN, false },
};
}

std::vector<Dep> deps_list(const std::string &root, const std::string &tools_dir, std::string &err) {
    std::vector<Dep> out;
    std::string text = read_text(join(root, "tools/data/pins.json"));
    if (text.empty()) { err = "tools/data/pins.json was not found under " + root; return out; }
    P p(text); J doc = p.value();
    const J *dl = doc.get("downloads"), *src = doc.get("sources");
    if (p.bad || doc.t != J::OBJ || !dl || dl->t != J::ARR) { err = "tools/data/pins.json could not be read"; return out; }
    std::map<std::string, Dep> pins;
    for (const J &e : dl->a) {
        Dep d; d.id = e.str("id"); d.name = e.str("name"); d.version = e.str("version"); d.url = e.str("url"); d.sha256 = e.str("sha256");
        pins[d.id] = d;
    }
    if (src && src->t == J::ARR)
        for (const J &e : src->a) {                    /* a repository at a commit, checked file by file */
            Dep d; d.id = e.str("id"); d.name = e.str("name");
            std::string commit = e.str("commit");
            d.version = "commit " + commit.substr(0, 12);
            d.url = e.str("repo") + "/archive/" + commit + ".zip";
            const J *files = e.get("files");
            if (files && files->t == J::OBJ && !files->o.empty()) { d.sha_of = files->o[0].first; d.sha256 = files->o[0].second.s; }
            pins[d.id] = d;
        }
    const std::string tools = tools_dir.empty() ? join(root, ".tools") : tools_dir;
#ifndef _WIN32
    /* Linux and macOS take these from the system (Apple's command line tools; a package manager) */
    static const struct { const char *name, *bin, *how; } SYS[] = {
#ifdef __APPLE__
        { "Clang (Apple's command line tools)", "clang", "xcode-select --install" },
        { "Python 3.9 or later", "python3", "xcode-select --install" },
#else
        { "CMake 3.25 or later", "cmake", "your package manager: cmake" },
        { "Ninja", "ninja", "your package manager: ninja-build (Arch: ninja)" },
        { "Clang", "clang", "your package manager: clang" },
        { "Python 3.9 or later", "python3", "your package manager: python3" },
        { "pkg-config", "pkg-config", "your package manager: pkg-config (Fedora: pkgconf-pkg-config, Arch: pkgconf)" },
#endif
    };
    for (auto &s : SYS) {
        Dep d; d.id = s.bin; d.name = s.name; d.system = true;
        std::string at = on_path(s.bin);
        d.here = !at.empty();
        d.detail = d.here ? at : std::string("not found: ") + s.how;
        d.dest = "(your system)";
        out.push_back(d);
    }
#ifndef __APPLE__
    {   /* SDL3's development libraries: without them SDL's configure stops after the tools are all found
           (issue #2). tools/build.py checks the same before CMake runs; the lines that install them are
           in docs/BUILDING.md and in what build.py prints. */
        const bool pc = !on_path("pkg-config").empty();
        const bool x11 = pc && pkg_config("x11") && pkg_config("xext");
        const bool wl = pc && pkg_config("wayland-client") && pkg_config("wayland-egl") && pkg_config("wayland-cursor") && pkg_config("xkbcommon");
        Dep w; w.id = "window-libs"; w.name = "Window libraries (X11 or Wayland)"; w.system = true; w.dest = "(your system)";
        w.here = x11 || wl;
        w.detail = w.here ? std::string(x11 && wl ? "X11 and Wayland" : x11 ? "X11" : "Wayland")
                 : !pc ? std::string("not checked: install pkg-config first")
                 : std::string("not found: your package manager, e.g. libx11-dev libxext-dev libwayland-dev libxkbcommon-dev (docs/BUILDING.md)");
        out.push_back(w);
        std::string found;
        for (auto &s : { std::make_pair("ALSA", "alsa"), std::make_pair("PulseAudio", "libpulse"), std::make_pair("PipeWire", "libpipewire-0.3") })
            if (pc && pkg_config(s.second)) found += (found.empty() ? "" : ", ") + std::string(s.first);
        Dep a; a.id = "sound-libs"; a.name = "Sound libraries (ALSA or PulseAudio)"; a.system = true; a.dest = "(your system)";
        a.here = !found.empty();
        a.detail = a.here ? found
                 : !pc ? std::string("not checked: install pkg-config first")
                 : std::string("not found: your package manager, e.g. libasound2-dev libpulse-dev (docs/BUILDING.md)");
        out.push_back(a);
    }
#endif
#endif
    for (const Want &w : WANTED) {
        auto it = pins.find(w.id);
        if (it == pins.end()) continue;
        Dep d = it->second;
        size_t cut = d.name.find(" (");                 /* "CMake (Windows)" -> "CMake" */
        if (cut != std::string::npos) d.name.erase(cut);
        d.when = w.when ? w.when : "";
        d.optional = w.when == SH_WHEN;
        if (w.tool) {
            d.dest = join(tools, w.dest);
            d.here = exists(join(d.dest, w.probe));
        } else if (!w.probe) {                          /* SDL: its source in either build folder */
            d.dest = w.dest;
            d.here = exists(join(root, "build/_deps/sdl3-src")) || exists(join(root, "build-launcher/_deps/sdl3-src")) ||
                     exists(join(join(tools, "sdl3"), "a3mx-built.txt"));           /* or built once by tools/sdl3.py */
        } else {
            d.dest = w.dest;
            d.here = exists(join(join(root, w.dest), w.probe));
        }
        out.push_back(d);
    }
    {   /* the shader library: built from its pinned source by tools/librashader.py (needs Rust), or in a
           release's download already; never downloaded built (docs/DISPLAY.md, FILTER) */
#if defined(_WIN32)
        const char *lib = "librashader.dll";
#elif defined(__APPLE__)
        const char *lib = "librashader.dylib";
#else
        const char *lib = "librashader.so";
#endif
        Dep d; d.id = "librashader"; d.name = "librashader (for FILTER)"; d.system = true; d.optional = true;
        std::string a = join(join(tools, "librashader"), lib), b = join(root, lib);
        d.here = exists(a) || exists(b);
        d.dest = join(tools, "librashader");
        auto src = pins.find("librashader");                 /* its source's pin (tools/librashader.py builds from it) */
        if (src != pins.end()) { d.version = src->second.version; d.url = src->second.url; }
        d.detail = d.here ? (exists(a) ? a : b) : std::string("optional: python tools/librashader.py builds it (needs Rust), or put ") + lib + " in " + d.dest;
        out.push_back(d);
    }
    return out;
}

bool deps_any_to_download(const std::vector<Dep> &d) {
    for (const Dep &e : d) if (!e.system && !e.here && e.when.empty()) return true;
    return false;
}
bool deps_any_missing(const std::vector<Dep> &d) {
    for (const Dep &e : d) if (!e.here && !e.optional && (e.system || e.when.empty())) return true;
    return false;
}
std::string deps_signature(const std::vector<Dep> &d) {
    unsigned long long h = 1469598103934665603ull;       /* FNV-1a over what was shown: ids, versions, hashes */
    for (const Dep &e : d) { if (e.system) continue; for (char c : e.id + "|" + e.version + "|" + e.sha256 + ";") { h ^= (unsigned char)c; h *= 1099511628211ull; } }
    char b[24]; snprintf(b, sizeof b, "%016llx", h);
    return b;
}
