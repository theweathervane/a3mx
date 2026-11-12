## Layout
Below define the project structure

| Path | What |
|---|---|
| `src/core/` | the decompiled game: hand-written C replacing translated functions ([docs/DECOMPILING.md](docs/DECOMPILING.md)) |
| `src/platform/` | the PC side: memory, threads, files, graphics, sound, input, the PSP's OS functions |
| `src/launcher/` | the launcher: ISO, options, Run / Build and the console (SDL3 + Dear ImGui; no game code) |
| `src/extensions/` | the extensions from the static recomp (EX. SETTING, PLAYER CONFIG, join-in, widescreen, saves, ...) |
| `tools/` | `extract_assets.py`, `pac.py` (archives), `progress.py`, `show_func.py`; `recomp/` the program analysis and translator; `data/hooks.txt` |
| `tests/` | `parity.py` and its scenarios |
| `assets/` | extracted assets (git-ignored; [assets/README.md](assets/README.md)), except `extensions/`: new art, such as WIDE FORCED's stage edges ([stage extensions](docs/STAGE_EXTENSIONS.md)); the layout's rules: [assets/LAYOUT.md](assets/LAYOUT.md) |
| `third_party/` | ATRAC3/ATRAC3+ decoder from PPSSPP / FFmpeg (LGPL 2.1+), libkirk, Dear ImGui (MIT, the launcher): [ACKNOWLEDGMENTS.md](../ACKNOWLEDGMENTS.md) |
| `docs/` | [building](docs/BUILDING.md), [reporting problems](docs/REPORTING_ISSUES.md), [architecture](docs/ARCHITECTURE.md), [decompiling](docs/DECOMPILING.md), [archives](docs/PAC_FORMAT.md), [loading from assets](docs/ASSET_LOADING.md), [stages](docs/STAGES.md), [stage extensions](docs/STAGE_EXTENSIONS.md), [widescreen](docs/WIDESCREEN.md), [display and frame timing](docs/DISPLAY.md), [roadmap](docs/ROADMAP.md), [program map](docs/PSP_PROGRAM_MAP.md), [known issues](docs/KNOWN_ISSUES.md), [other versions](docs/VERSION_DIFFERENCES.md) |
