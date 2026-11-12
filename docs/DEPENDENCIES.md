# Pinned dependencies

Everything the build downloads and every source folder copied into this repository, each pinned to
one exact file or commit. **This page is written from
[tools/data/pins.json](../tools/data/pins.json) by `python tools/pins.py --write`; edit that, not this.**
CI fails if the two disagree, or if any file that uses a pin has a different one (`tools/pins.py --check`).
`--download` fetches each download and checks its SHA-256: run it after changing one.

To check a download yourself: `certutil -hashfile FILE SHA256` (Windows), `sha256sum FILE` (Linux),
`shasum -a 256 FILE` (macOS).

## Downloads

Each is checked against its SHA-256 before it is used; a mismatch stops the build.

| What | Version | Used by | SHA-256 |
|---|---|---|---|
| [LLVM-MinGW (Windows build tools: Clang, LLD, the MinGW headers)](https://github.com/mstorsjo/llvm-mingw/releases/download/20251104/llvm-mingw-20251104-ucrt-x86_64.zip) | 20251104 (ucrt, x86_64) | tools/build.py, on Windows, into .tools/llvm-mingw/ | `171b298f46fa4b5653dc7b01cc5e709caca645991be8e1b0724466c4a282a1ac` |
| [CMake (Windows)](https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-windows-x86_64.zip) | 4.1.2 | tools/build.py, on Windows, into .tools/cmake/ | `82730741149681597510a4bc7095da1460b472dac24eaf9c23ad7440f47753ab` |
| [Ninja (Windows)](https://github.com/ninja-build/ninja/releases/download/v1.13.1/ninja-win.zip) | 1.13.1 | tools/build.py, on Windows, into .tools/ninja/ | `26a40fa8595694dec2fad4911e62d29e10525d2133c9a4230b66397774ae25bf` |
| [CMake (macOS)](https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-macos-universal.tar.gz) | 4.1.2 (universal) | tools/build.py, on macOS, into .tools/cmake-macos/ | `3be85f5b999e327b1ac7d804cbc9acd767059e9f603c42ec2765f6ab68fbd367` |
| [Ninja (macOS)](https://github.com/ninja-build/ninja/releases/download/v1.13.1/ninja-mac.zip) | 1.13.1 | tools/build.py, on macOS, into .tools/ninja-macos/ | `da7797794153629aca5570ef7c813342d0be214ba84632af886856e8f0063dd9` |
| [Python, embeddable package (Windows)](https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip) | 3.12.10 (amd64) | build.cmd, on Windows with no usable Python of your own, into .tools/python/ (checked with certutil) | `4acbed6dd1c744b0376e3b1cf57ce906f9dc9e95e68824584c8099a63025a3c3` |
| [SDL3 (window, sound, input; source, built with the game and the launcher)](https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz) | 3.4.16 | cmake/SDL3.cmake (CMake's FetchContent URL_HASH), every system | `7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68` |

## Downloaded source

Pinned to a commit; the files used are checked against their SHA-256 before anything runs.

| What | Commit | File | SHA-256 | Used by |
|---|---|---|---|---|
| [SFA3MAX 16:9 widescreen patch (Rickow, MIT)](https://github.com/Rickow/Street-Fighter-Alpha-3-MAX-PSP-widescreen) | [`d7fc89b5579d`](https://github.com/Rickow/Street-Fighter-Alpha-3-MAX-PSP-widescreen/tree/d7fc89b5579d06a55149c862193891ef3da4958c) | `sfa3_ws_patternpatcher.py` | `d64b2c826b422a13bfde9bc13deecc303d903c769c6b60d7a79d98ead0c164fb` | tools/extract_assets.py, which downloads that commit and runs the patcher only if its SHA-256 matches (else WIDE FORCED is off) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `crt/crt-geom.slangp` | `094d5133a3b6fbb4c2e6cba65141e5b850517aa3201bea7bdb41e4899087b60b` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `crt/crt-guest-advanced.slangp` | `eb597473e15639e9dec4f11a18d4a789f5f901e9368816d4e5114b09b51c2d19` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `crt/crt-lottes.slangp` | `51bdf93aa6e3dd8605bf1c2b19af1f511672b53f3551d8de1041e02a7bbf9610` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `crt/crt-royale.slangp` | `724e0196cf83da1a229bc896e970d639b4cbeb9c41f341e1e5764780fecede96` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `crt/zfast-crt.slangp` | `fa3a7f37344f351572e692bc317a21f89f30628c08c09adc1091e4900f1f8e11` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `edge-smoothing/scalefx/scalefx.slangp` | `016739a084bd0f88f364d7a5994ce53bd166e29859e2b98b1af47e900674fe3f` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `edge-smoothing/xbrz/xbrz-freescale.slangp` | `545f35fc2edf17756a3b14d75c7beb9cb91c9a5a143daa825d5fa5625c505341` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `scanlines/ossc.slangp` | `d115f7b47635c946079b3f2018fce4e2ef6c291df7faa98d7d24b1eecc8d39de` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [libretro slang-shaders (the FILTER setting's shaders; each shader under its own licence)](https://github.com/libretro/slang-shaders) | [`2345ee91602a`](https://github.com/libretro/slang-shaders/tree/2345ee91602a7404d3a1a6a5df6108d7305fdad4) | `scanlines/scanline.slangp` | `33768f5bbce0b53ada4fc73dc5ff13e6dfba6990b4d8fe27e87ece50024a2623` | tools/shaders.py, which downloads that commit into .tools/slang-shaders/ when asked and keeps it only if the presets SCREEN SETTING > FILTER uses match their SHA-256 (src/platform/shader.h) |
| [librashader (the FILTER setting's shader library, MPL-2.0; built from source, not downloaded built)](https://github.com/SnowflakePowered/librashader) | [`07ac790cdb98`](https://github.com/SnowflakePowered/librashader/tree/07ac790cdb9826f37cf8e1daf5011f03e3c4daa7) | `Cargo.lock` | `8c694b51d7b079f5136fd06b24b8e7712463d958219bf101b2cd95198228b3f9` | tools/librashader.py, which builds the library from that commit with Rust when asked (the pipelines do, once, and keep it); the same tag as third_party/librashader/'s headers |
| [librashader (the FILTER setting's shader library, MPL-2.0; built from source, not downloaded built)](https://github.com/SnowflakePowered/librashader) | [`07ac790cdb98`](https://github.com/SnowflakePowered/librashader/tree/07ac790cdb9826f37cf8e1daf5011f03e3c4daa7) | `include/librashader.h` | `5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62` | tools/librashader.py, which builds the library from that commit with Rust when asked (the pipelines do, once, and keep it); the same tag as third_party/librashader/'s headers |
| [librashader (the FILTER setting's shader library, MPL-2.0; built from source, not downloaded built)](https://github.com/SnowflakePowered/librashader) | [`07ac790cdb98`](https://github.com/SnowflakePowered/librashader/tree/07ac790cdb9826f37cf8e1daf5011f03e3c4daa7) | `include/librashader_ld.h` | `bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33` | tools/librashader.py, which builds the library from that commit with Rust when asked (the pipelines do, once, and keep it); the same tag as third_party/librashader/'s headers |

## Vendored source (copied into this repository)

Each folder says where it came from in its own `VENDORED` file. Its SHA-256 here covers every file in it
(each path and file SHA-256, in order), so a change to vendored code has to be deliberate: update the
folder, then `tools/pins.py --write`.

| What | Folder | Upstream | Folder SHA-256 |
|---|---|---|---|
| Dear ImGui (the launcher's interface) | `third_party/imgui/` | https://github.com/ocornut/imgui v1.93.0, commit c1aaac65e896a45f81f47a4d68f9959e0f86c859 | `4405bba7a6419a1a90a3fc1d83ac70ed52ed899ba2d2e6f1cc78cca984198db9` |
| ATRAC3 / ATRAC3+ decoder (the music) | `third_party/at3_standalone/` | https://github.com/hrydgard/ppsspp ext/at3_standalone, commit 53a71276186f63cf1605823099be728eef982a59 | `71b1cb7a4fde31d1f1aa297248267bff5f52dd5debf1b8bb2d75aa0c049f44ad` |
| libkirk (PSP save encryption, for IMPORT / EXPORT) | `third_party/libkirk/` | https://github.com/hrydgard/ppsspp ext/libkirk, commit aa9dfa85f6d69479527980e038971ebf6d2682e5 | `b616ea11c4567f8dced454392fcc1bb3bbb7890ec553acdbd7c8143f4612cc99` |
| librashader C headers (MIT; the library is not included) | `third_party/librashader/` | https://github.com/SnowflakePowered/librashader at librashader-v0.12.1 (07ac790cdb9826f37cf8e1daf5011f03e3c4daa7) | `d1ea1e2f3d4fdb943abab5704b8770eafedd78bec3003eca855fff018371a85f` |

## Nothing compiled is committed

The repository holds no program built in advance (create-launcher.exe was the last, removed in 0.60.0).
The launcher that a release download carries is built by the release pipeline from the tagged source
([BUILDING.md](BUILDING.md)); it is not signed, so Windows SmartScreen and macOS Gatekeeper may warn the
first time it is opened.

## Not pinned here

- **Linux and macOS build tools** (CMake, Ninja, Clang, Python, SDL's system libraries): your
  system's packages, from its package manager.
- **Your disc** is checked by its own checksums (`tools/extract_assets.py`, [BUILDING.md](BUILDING.md)).
