/* The launcher's GPU choice: which graphics cards this PC has, which looks fastest, and how to make
 * the game run on the one chosen.
 *
 * OpenGL has no way to pick a card by itself, so each system is asked differently:
 *   Windows  DXGI lists the cards in Windows' own order, high-performance first. For OpenGL Windows
 *            only chooses between its power-saving and its high-performance card, so those two can be
 *            chosen (others are listed but greyed: the monitor they drive decides). The choice is the
 *            per-app setting Windows' Settings > Display > Graphics writes
 *            (HKCU\Software\Microsoft\DirectX\UserGpuPreferences, "GpuPreference=1;" or "=2;"),
 *            for the game's exe, before it starts. A setting the player made there is left alone on
 *            Automatic.
 *   Linux    /sys/class/drm lists the cards. The choice is environment variables for the game: Mesa's
 *            DRI_PRIME=pci-<slot>, or NVIDIA's PRIME render offload. The card the desktop draws on
 *            needs none. Variables already set when the launcher started are left alone on Automatic.
 *   macOS    not listed: macOS chooses (Automatic only).
 * When nothing is found, or only one card, the choice is Automatic and nothing is changed. */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Gpu {
    std::string id;          /* kept in launcher.ini: "vvvv:dddd" plus the slot (Linux) or the adapter's index (Windows) */
    std::string name;        /* e.g. "NVIDIA GeForce RTX 4070 Laptop GPU" */
    unsigned vendor = 0, device = 0;
    uint64_t vram = 0;       /* dedicated video memory in bytes, 0 if unknown */
    bool integrated = false;
    bool choosable = true;   /* the game can be made to run on it */
    std::string why_not;     /* when not choosable: why, for the tooltip */
    int win_pref = 0;        /* Windows: the GpuPreference value that selects it (1 power saving, 2 high performance) */
    std::string slot;        /* Linux: PCI slot, e.g. 0000:01:00.0 */
    std::string driver;      /* Linux: kernel driver, e.g. nvidia, amdgpu, i915 */
    bool boot = false;       /* Linux: boot_vga, the card the desktop draws on */
};

/* The cards, fastest first (empty when none were found). SYSFS: a stand-in for /sys (tests); PCI_IDS: a
 * stand-in for the pci.ids name list (tests). */
std::vector<Gpu> gpu_list(const std::string &sysfs = "", const std::string &pci_ids = "");

/* Before the game starts: make it run on the card CHOICE names ("" or "auto": the fastest when there
 * are two or more). EXE is the game's full path (Windows). OWNED lists, '|'-separated, the exes whose
 * Windows setting the launcher wrote (kept in launcher.ini so a player's own setting is never
 * overwritten by Automatic). Returns a line for the console saying what was done, or "". */
std::string gpu_apply(const std::vector<Gpu> &list, const std::string &choice, const std::string &exe, std::string &owned);

/* The card CHOICE names in LIST, or NULL (Automatic, or no longer present). */
const Gpu *gpu_find(const std::vector<Gpu> &list, const std::string &choice);

/* Does the game's OpenGL renderer string (GL_RENDERER, in work/last_run.txt) look like card G? Only the
 * maker can be told apart. -1 unknown, 0 no, 1 yes. */
int gpu_matches(const Gpu &g, const std::string &gl_renderer);
