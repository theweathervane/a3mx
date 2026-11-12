/* The launcher's GPU choice (see gpu.h). */
#include "gpu.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_6.h>
#endif

static std::string lower(std::string s) { for (char &c : s) c = (char)tolower((unsigned char)c); return s; }

#if !defined(_WIN32) && !defined(__APPLE__)
static const char *vendor_name(unsigned v) {
    switch (v) {
    case 0x10de: return "NVIDIA";
    case 0x1002: case 0x1022: return "AMD";
    case 0x8086: return "Intel";
    case 0x5143: return "Qualcomm";
    case 0x106b: return "Apple";
    case 0x1414: return "Microsoft";
    case 0x15ad: return "VMware";
    case 0x1af4: return "virtio";
    case 0x1234: case 0x1b36: return "QEMU";
    }
    return NULL;
}
#endif

#ifndef __APPLE__
/* Intel's own cards (Arc) rather than the graphics inside the processor: by device number */
static bool intel_discrete(unsigned d) { return (d >= 0x5690 && d <= 0x56ff) || (d >= 0xe200 && d <= 0xe2ff); }
#endif

const Gpu *gpu_find(const std::vector<Gpu> &list, const std::string &choice) {
    if (choice.empty() || choice == "auto") return NULL;
    for (const Gpu &g : list) if (g.id == choice) return &g;
    return NULL;
}

int gpu_matches(const Gpu &g, const std::string &gl_renderer) {
    if (gl_renderer.empty()) return -1;
    std::string r = lower(gl_renderer);
    static const struct { unsigned v; const char *words[3]; } K[] = {
        { 0x10de, { "nvidia", "geforce", "quadro" } },
        { 0x1002, { "amd", "radeon", "ati " } },
        { 0x8086, { "intel", NULL, NULL } },
    };
    bool known = false;
    for (const auto &k : K) {
        bool hit = false;
        for (const char *w : k.words) if (w && r.find(w) != std::string::npos) hit = true;
        if (hit && k.v == (g.vendor == 0x1022 ? 0x1002 : g.vendor)) return 1;
        if (hit) known = true;
    }
    return known ? 0 : -1;       /* a renderer naming another maker: no; naming none (llvmpipe, ...): unknown */
}

#if !defined(_WIN32) && !defined(__APPLE__)
/* fastest first: a card of its own before integrated graphics, then more video memory */
static void rank(std::vector<Gpu> &v) {
    std::stable_sort(v.begin(), v.end(), [](const Gpu &a, const Gpu &b) {
        if (a.integrated != b.integrated) return !a.integrated;
        return a.vram > b.vram;
    });
}
#endif

#ifdef _WIN32
/* ================================================================ Windows: DXGI and the per-app setting */
static std::string utf8(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, NULL, NULL);
    return s;
}
static std::wstring wide(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

std::vector<Gpu> gpu_list(const std::string &, const std::string &) {
    std::vector<Gpu> out;
    HMODULE dxgi = LoadLibraryW(L"dxgi.dll");
    if (!dxgi) return out;
    typedef HRESULT(WINAPI * Create)(REFIID, void **);
    Create create = (Create)(void *)GetProcAddress(dxgi, "CreateDXGIFactory1");
    IDXGIFactory1 *f1 = NULL;
    IDXGIFactory6 *f6 = NULL;
    if (create && SUCCEEDED(create(__uuidof(IDXGIFactory1), (void **)&f1)) && f1)
        f1->QueryInterface(__uuidof(IDXGIFactory6), (void **)&f6);
    if (f6) {   /* Windows 10 1803 and later: Windows' own order */
        LUID fast = {}, saving = {};
        bool have_fast = false, have_saving = false;
        for (int pass = 0; pass < 2; pass++) {
            DXGI_GPU_PREFERENCE pref = pass ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            IDXGIAdapter1 *a = NULL;
            for (UINT i = 0; f6->EnumAdapterByGpuPreference(i, pref, __uuidof(IDXGIAdapter1), (void **)&a) == S_OK; i++) {
                DXGI_ADAPTER_DESC1 d;
                bool ok = SUCCEEDED(a->GetDesc1(&d)) && !(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && d.VendorId != 0x1414;
                a->Release(); a = NULL;
                if (!ok) continue;
                if (pass == 0) {
                    if (!have_fast) { fast = d.AdapterLuid; have_fast = true; }
                    Gpu g;
                    char id[48];
                    snprintf(id, sizeof id, "%04x:%04x:%08x:%u", d.VendorId, d.DeviceId, d.SubSysId, (unsigned)out.size());
                    g.id = id;
                    g.name = utf8(d.Description);
                    g.vendor = d.VendorId; g.device = d.DeviceId;
                    g.vram = d.DedicatedVideoMemory;
                    g.integrated = d.DedicatedVideoMemory < (512ull << 20) && !(d.VendorId == 0x8086 && intel_discrete(d.DeviceId));
                    g.slot = std::to_string(d.AdapterLuid.HighPart) + "." + std::to_string(d.AdapterLuid.LowPart);   /* the LUID, to match the two passes */
                    out.push_back(g);
                } else if (!have_saving) { saving = d.AdapterLuid; have_saving = true; }
            }
        }
        auto luid_str = [](LUID l) { return std::to_string(l.HighPart) + "." + std::to_string(l.LowPart); };
        for (Gpu &g : out) {
            if (have_fast && g.slot == luid_str(fast)) g.win_pref = 2;
            else if (have_saving && g.slot == luid_str(saving)) g.win_pref = 1;
            else { g.choosable = false; g.why_not = "Windows runs OpenGL on its power-saving or its high-performance GPU only.\nTo use this one, connect the monitor to it."; }
        }
        /* the ids end in the position in Windows' order, which can change: key them by the card instead */
        for (Gpu &g : out) g.id = g.id.substr(0, g.id.rfind(':')) + (g.win_pref == 2 ? ":fast" : g.win_pref == 1 ? ":saving" : ":other");
        f6->Release();
    }
    /* (older Windows: no list, so Automatic only, the same as before) */
    if (f1) f1->Release();
    FreeLibrary(dxgi);
    return out;      /* already fastest first: Windows' high-performance order */
}

static const wchar_t *const PREFS_KEY = L"Software\\Microsoft\\DirectX\\UserGpuPreferences";

static bool owned_has(const std::string &owned, const std::string &exe) {
    std::string o = "|" + lower(owned) + "|";
    return o.find("|" + lower(exe) + "|") != std::string::npos;
}

std::string gpu_apply(const std::vector<Gpu> &list, const std::string &choice, const std::string &exe_in, std::string &owned) {
    if (list.size() < 2 || exe_in.empty()) return "";
    std::string exe = exe_in;
    for (char &c : exe) if (c == '/') c = '\\';
    const Gpu *g = gpu_find(list, choice);
    bool automatic = !g;
    if (!g || !g->choosable) g = &list[0];

    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, PREFS_KEY, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &k, NULL) != ERROR_SUCCESS)
        return "GPU: could not open Windows' graphics settings; Windows chooses.";
    std::wstring name = wide(exe);
    wchar_t old[256] = L"";
    DWORD type = 0, size = sizeof old - sizeof(wchar_t);
    bool exists = RegQueryValueExW(k, name.c_str(), NULL, &type, (BYTE *)old, &size) == ERROR_SUCCESS && type == REG_SZ;
    std::string now = exists ? utf8(old) : "";
    std::string line;
    if (automatic && exists && !owned_has(owned, exe)) {
        line = "GPU: Automatic, but Windows' Settings > Display > Graphics already sets one for this game (" + now + "); left as it is.";
    } else {
        std::string want = "GpuPreference=" + std::to_string(g->win_pref) + ";";
        if (now != want) {
            std::wstring w = wide(want);
            if (RegSetValueExW(k, name.c_str(), 0, REG_SZ, (const BYTE *)w.c_str(), (DWORD)((w.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS) {
                RegCloseKey(k);
                return "GPU: could not save the choice in Windows' graphics settings; Windows chooses.";
            }
        }
        if (!owned_has(owned, exe)) owned += (owned.empty() ? "" : "|") + exe;
        line = std::string("GPU: ") + (automatic ? "Automatic, the fastest: " : "") + g->name + " (Windows' " +
               (g->win_pref == 2 ? "high-performance" : "power-saving") + " GPU for " + exe + ")";
    }
    RegCloseKey(k);
    return line;
}

#elif defined(__APPLE__)
/* ================================================================ macOS: macOS chooses */
std::vector<Gpu> gpu_list(const std::string &, const std::string &) { return {}; }
std::string gpu_apply(const std::vector<Gpu> &, const std::string &, const std::string &, std::string &) { return ""; }

#else
/* ================================================================ Linux: /sys/class/drm and the environment */
#include <dirent.h>
#include <unistd.h>

static std::string read_line(const std::string &p) {
    FILE *f = fopen(p.c_str(), "r");
    if (!f) return "";
    char b[256] = "";
    if (!fgets(b, sizeof b, f)) b[0] = 0;
    fclose(f);
    std::string s = b;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}
static unsigned hex(const std::string &s) { return (unsigned)strtoul(s.c_str(), NULL, 16); }

/* pci.ids: "vvvv  Vendor" lines, then "\tdddd  Device" lines under each */
static std::string pci_name(const std::string &file, unsigned vendor, unsigned device) {
    const char *files[] = { file.c_str(), "/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids", "/usr/share/pci.ids", NULL };
    for (int i = file.empty() ? 1 : 0; files[i]; i++) {
        FILE *f = fopen(files[i], "r");
        if (!f) continue;
        char b[512];
        char v4[8], d4[8];
        snprintf(v4, sizeof v4, "%04x", vendor);
        snprintf(d4, sizeof d4, "%04x", device);
        bool in = false;
        std::string vname, dname;
        while (fgets(b, sizeof b, f)) {
            if (b[0] == '#' || b[0] == '\n') continue;
            if (b[0] != '\t') { if (in) break; if (!strncmp(b, v4, 4) && b[4] == ' ') { in = true; vname = b + 6; } continue; }
            if (in && b[1] != '\t' && !strncmp(b + 1, d4, 4)) { dname = b + 7; break; }
        }
        fclose(f);
        auto trim = [](std::string s) { while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back(); return s; };
        if (!dname.empty()) return trim(dname);
    }
    return "";
}

std::vector<Gpu> gpu_list(const std::string &sysfs, const std::string &pci_ids) {
    std::vector<Gpu> out;
    std::string drm = (sysfs.empty() ? std::string("/sys") : sysfs) + "/class/drm";
    DIR *d = opendir(drm.c_str());
    if (!d) return out;
    std::vector<std::string> cards;
    while (dirent *e = readdir(d)) {
        std::string n = e->d_name;
        if (n.compare(0, 4, "card") == 0 && n.size() > 4 && n.find('-') == std::string::npos) cards.push_back(n);   /* card0, not card0-HDMI-A-1 */
    }
    closedir(d);
    std::sort(cards.begin(), cards.end());
    for (const std::string &c : cards) {
        std::string dev = drm + "/" + c + "/device";
        std::string v = read_line(dev + "/vendor"), dv = read_line(dev + "/device");
        if (v.empty() || dv.empty()) continue;          /* simpledrm and the like: no card behind it */
        Gpu g;
        g.vendor = hex(v); g.device = hex(dv);
        g.boot = read_line(dev + "/boot_vga") == "1";
        std::string ue;
        if (FILE *f = fopen((dev + "/uevent").c_str(), "r")) {
            char b[256];
            while (fgets(b, sizeof b, f)) {
                std::string s = b;
                while (!s.empty() && s.back() == '\n') s.pop_back();
                if (s.compare(0, 14, "PCI_SLOT_NAME=") == 0) g.slot = s.substr(14);
                else if (s.compare(0, 7, "DRIVER=") == 0) g.driver = s.substr(7);
            }
            fclose(f);
        }
        if (g.slot.empty()) continue;                   /* not a PCI device: DRI_PRIME couldn't name it */
        std::string vram = read_line(dev + "/mem_info_vram_total");     /* amdgpu */
        g.vram = vram.empty() ? 0 : strtoull(vram.c_str(), NULL, 10);
        /* the name: NVIDIA's driver says it, else pci.ids, else the maker and the numbers */
        std::string model = read_line("/proc/driver/nvidia/gpus/" + g.slot + "/information");
        if (model.compare(0, 6, "Model:") == 0) { g.name = model.substr(6); g.name.erase(0, g.name.find_first_not_of(" \t")); }
        if (g.name.empty()) {
            std::string p = pci_name(pci_ids, g.vendor, g.device);
            size_t lb = p.find('['), rb = p.rfind(']');       /* "AD106M [GeForce RTX 4070 Max-Q / Mobile]": the name in brackets */
            if (lb != std::string::npos && rb != std::string::npos && rb > lb + 1) p = p.substr(lb + 1, rb - lb - 1);
            const char *vn = vendor_name(g.vendor);
            if (!p.empty()) g.name = (vn && lower(p).find(lower(vn)) == std::string::npos ? std::string(vn) + " " : std::string()) + p;
        }
        if (g.name.empty()) {
            char b[64];
            const char *vn = vendor_name(g.vendor);
            snprintf(b, sizeof b, "%s GPU (%04x:%04x)", vn ? vn : "Unknown", g.vendor, g.device);
            g.name = b;
        }
        bool virt = g.vendor == 0x15ad || g.vendor == 0x1af4 || g.vendor == 0x1234 || g.vendor == 0x1b36;
        g.integrated = virt || (g.vendor == 0x8086 && !intel_discrete(g.device)) ||
                       ((g.vendor == 0x1002 || g.vendor == 0x1022) && g.vram > 0 && g.vram < (2ull << 30));
        g.id = std::string(v.c_str() + 2) + ":" + std::string(dv.c_str() + 2) + "@" + g.slot;
        out.push_back(g);
    }
    rank(out);
    return out;
}

static const char *const ENV_VARS[] = { "DRI_PRIME", "__NV_PRIME_RENDER_OFFLOAD", "__GLX_VENDOR_LIBRARY_NAME", "__VK_LAYER_NV_optimus", NULL };

std::string gpu_apply(const std::vector<Gpu> &list, const std::string &choice, const std::string &, std::string &) {
    static int player_set = -1;          /* the player's own variables, from before the launcher touched any */
    if (player_set < 0) {
        player_set = 0;
        for (int i = 0; ENV_VARS[i]; i++) if (getenv(ENV_VARS[i])) player_set = 1;
    }
    if (list.size() < 2) return "";
    const Gpu *g = gpu_find(list, choice);
    bool automatic = !g;
    if (automatic && player_set) return "GPU: Automatic, but DRI_PRIME / __NV_PRIME_RENDER_OFFLOAD are already set; left as they are.";
    if (!g) g = &list[0];
    for (int i = 0; ENV_VARS[i]; i++) unsetenv(ENV_VARS[i]);
    std::string how;
    if (g->boot) how = "the desktop's own GPU";
    else if (g->driver == "nvidia") {
        setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1);
        setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
        setenv("__VK_LAYER_NV_optimus", "NVIDIA_only", 1);
        how = "__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia";
    } else {
        std::string tag = "pci-" + g->slot;
        for (char &c : tag) if (c == ':' || c == '.') c = '_';
        setenv("DRI_PRIME", tag.c_str(), 1);
        how = "DRI_PRIME=" + tag;
    }
    return std::string("GPU: ") + (automatic ? "Automatic, the fastest: " : "") + g->name + " (" + how + ")";
}
#endif
