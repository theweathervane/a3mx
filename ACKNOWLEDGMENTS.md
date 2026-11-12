# Acknowledgments

This project stands on other people's work: projects that inspired it, people working on the same
game in other ways, and the code and tools it is built with. Thank you to all of them.

## Inspiration, and others working on Street Fighter Alpha 3

- [cps2recomp](https://github.com/tmcleroy/cps2recomp) by tmcleroy: A toolkit for statically recompiling Capcom CP System II arcade games to native code, with a launcher; on macOS it ships as a native app (`CPS-2 Recomp.app`, Apple silicon and Intel), which is how this project's macOS launcher is packaged too.
- [Street Fighter Alpha 3 Recomp](https://github.com/omegakatana92/Street-Fighter-Alpha-3-Recomp) by omegakatana92: A native PC port of the 1998 **arcade** game (CP System II), its 68000 and Z80 programs statically recompiled to C++ with cps2recomp; you supply your own ROM set. GPL-3.0.
- [Street Fighter Alpha 3 Recomp](https://github.com/TechnicallyComputers/Street-Fighter-Alpha-3-Recomp) by TechnicallyComputers: A static recompilation of the **PlayStation** version, built with PSXrecomp. Its [issues](https://github.com/TechnicallyComputers/Street-Fighter-Alpha-3-Recomp/issues/) collect ideas for the game, among them the `.recomp.json` project file this repository also carries.
- [Street Fighter Alpha 3 MAX PSP widescreen](https://github.com/Rickow/Street-Fighter-Alpha-3-MAX-PSP-widescreen) by Rickow: A patcher that gives *Street Fighter Alpha 3 MAX* native 16:9 rendering (EU, US and JP, made for PPSSPP). MIT. **WIDE FORCED started as this patch, and is still built on it:** the build downloads it at a pinned commit (SHA-256 checked), runs its patcher on your disc's program and keeps the changes as a switchable overlay. Nothing of it is kept in this repository; the download is dropped once the decomp does everything it does natively (0.49.0's stage tile drawers are the first) ([WIDESCREEN.md](docs/WIDESCREEN.md)).

## Code in this repository from other projects

Each folder's `VENDORED.md` says exactly where it came from (the commit) and its licence; the code
keeps its own notices.

- [ATRAC3 / ATRAC3+ decoder](third_party/at3_standalone/VENDORED.md): From [PPSSPP](https://github.com/hrydgard/ppsspp) (Henrik Rydgård and contributors), from FFmpeg's atrac3 / atrac3plus decoders (LGPL 2.1 or later) — used for the music
- [libkirk](third_party/libkirk/VENDORED.md): From PPSSPP; "KIRK-ENGINE" by Draan and the PSP scene (GPL 3.0 or later) — used for PSP save encryption (IMPORT / EXPORT)
- PSP save data and PARAM.SFO code (`src/extensions/psp_savecrypt.c`): Ported from PPSSPP (`Core/HLE/sceChnnlsv.cpp`, `Core/Dialog/SavedataParam.cpp`, `Core/ELF/ParamSFO.cpp`) (GPL 2.0 or later) — used for IMPORT / EXPORT
- SAS sound mixing: Written in C following PPSSPP (this project's licence) — used for sound effects and voices
- [Dear ImGui](third_party/imgui/VENDORED.md): From [Omar Cornut and contributors](https://github.com/ocornut/imgui) (MIT) — used for the launcher's interface

PPSSPP is also what much of the PSP knowledge here was checked against.

## Tools and libraries the build downloads

All pinned and checked by SHA-256 before use: the list, with versions and hashes, is
[docs/DEPENDENCIES.md](docs/DEPENDENCIES.md).

- [SDL3](https://github.com/libsdl-org/SDL) (Sam Lantinga and contributors, zlib licence): the window, sound and input
- [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw) (Martin Storsjö; LLVM, Clang and mingw-w64): the Windows compiler
- [CMake](https://cmake.org) (Kitware) and [Ninja](https://ninja-build.org): the build
- [Python](https://www.python.org) (the Python Software Foundation): the build's tools

## The game

*Street Fighter Alpha 3 MAX* (*Street Fighter Zero 3 Double Upper* in Japan) is © Capcom. This project
isn't affiliated with or endorsed by Capcom, and no game data is in it: you build it from your own
disc ([Legal](README.md#legal)).

---

Missing someone, or got something wrong? Open an issue or a merge request: credit should be right.