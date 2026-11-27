# Architecture

```
 your ISO ──extract_assets.py──► work/  (disc files, BOOT.BIN, analysis, imports)  +  assets/<region>/
                                   │
                    cmake --build: tools/recomp/psp_recomp.py
                                   │   every function not in src/core ──► build/generated/*.c (translated)
                                   │   every SFA_FUNC in src/core      ──► orig_f_XXXXXXXX (kept for checks)
                                   ▼
          ┌──────────────────────────────────────────────────────────────────────┐
          │                         a3mx (native)                                │
          │  src/core/        decompiled game code (hand-written C)              │
          │  build/generated/ the rest, translated; calls into src/core freely   │
          │  src/extensions/  the recomp's extensions (hooks on game functions)  │
          │        │ calls 181 PSP OS functions                                  │
          │  src/platform/    HLE: threads · files · save data · display ·       │
          │                   controller · SAS audio · ATRAC3+ · GE · net        │
          │                   host: SDL3 window / input / audio, OpenGL          │
          └──────────────────────────────────────────────────────────────────────┘
```

## 1. Extraction (`tools/extract_assets.py`)

Finds your disc in `imports/psp/` by the checksum of its program, checks it against the known discs (US
ULUS-10062 v1.00, JP ULJM-05082 v1.01), extracts every file into `work/iso/`, fetches the widescreen
patch and records it as an overlay, lists the program's OS imports and analyses it
(`tools/recomp/psp_analyze.py`: every function, switch table and code pointer, found through the
program's complete relocations). Assets in standard formats are copied to `assets/<region>/`.

## 2. Translation and decompiled code (`tools/recomp/`, `src/core/`)

`psp_recomp.py` turns each function of the PSP program into `void f_XXXXXXXX(Cpu *c)`: registers in a
`Cpu` struct, memory as one host block mirroring the PSP's address space, `jal` as a direct C call,
switch tables as C `switch`, calls through pointers through a generated address-to-function table.
It runs as part of the build.

Before translating, it scans `src/core/` for `SFA_FUNC(ADDR, name)`. Those functions are translated
as `orig_f_ADDR` instead, and `src/core/decomp.h` defines `f_ADDR` to run the hand-written code. All
callers, translated or decompiled, direct or through pointer tables, reach the new code with no other
change. `--reference` / `--reference-only` switch decompiled functions back to their originals at run
time, which is what the parity tests compare against.

Unchanged generated files keep their timestamps, so decompiling one function recompiles one generated
file plus your source.

As the decomp grows, `build/generated/` shrinks. The goal is for it to be empty: then the PSP program
is no longer needed at build time, only for its assets.

## 3. Platform (`src/platform/`)

Carried over from the static recomp:

- **Memory**: the program image is loaded at 0x08804000, as on the PSP.
- **Threads**: each game thread is a fiber on a small priority scheduler, like the PSP's single-core
  kernel. Vblank at 59.94 Hz drives frame pacing.
- **Files**: `disc0:` / `umd0:` read from `work/iso/`; saves go to `a3mx.dat` / `a3mx.cfg`.
- **Sound**: ATRAC3+ music through the vendored decoder; the SAS voice synthesiser (with reverb) in C.
- **Graphics**: the game draws 2D only. Software renderer as reference; a GPU renderer up to 8×.
  The GPU renderer sits behind a backend interface, `GeBackend` (`ge_backend.h`). `ge_state.c`
  turns each GE primitive into triangles of `GeVtx` and an explicit `GeDrawState` (target, scissor,
  texture and its sampling, texture function, alpha test, blending, colour mask), so no backend
  reads the GE's registers; the list walker (`ge_soft.c`, `hle_ge.c`) tells it when guest memory may
  have changed; the frame loop (`main.c`) and the window (`host_sdl.c`) ask it to show a frame, read
  one back and wait for the GPU. The texture cache (`ge_texcache.c`: hashes, and band-by-band checks
  of the big pages) is shared code; each backend only makes, fills and frees textures. Two backends:
  `ge_gl.c` (OpenGL 3.3, the default) and `ge_sdlgpu.c` (SDL3's GPU API: Vulkan, Direct3D 12,
  Metal; 0.174.0). The latter's shaders are HLSL (and MSL for Metal) in `gpu_shaders/`, compiled
  once into `gpu_shaders.h` by `tools/gpu_shaders.py` (DXIL and SPIR-V with Microsoft's dxc, pinned
  in `pins.json`): builds need no shader compiler. FILTER (librashader) runs on both: `shader_gl.c`
  for OpenGL, `shader_sdlgpu.c` (and `shader_sdlgpu_mtl.m`) for Vulkan / Direct3D 12 / Metal (0.176.0).
  SDL_GPU doesn't hand out the API's own device, command buffers or textures, which librashader's
  runtimes draw with; the project's patch to SDL (`cmake/patches/sdl-gpu-native-handles.patch`, applied
  by `tools/apply_patch.py` when SDL is built; DEPENDENCIES.md) adds them as device properties, until SDL
  has its own way (libsdl-org/SDL#10876). The FILTER pass records into SDL's command buffer outside its
  passes, puts every texture back in the layout or state SDL left it in, and submits that command
  buffer before SDL's own drawing goes on.
- **Test hooks**: `--headless --fast --renderer soft --input ... --hash-log FILE --save-dir DIR`.

These are the hardware abstraction layer: game code reaches them only through the PSP OS functions
(`hle_*`) today. As subsystems are decompiled (input, sound, graphics submission), they get direct,
typed interfaces here instead.

## 4. Extensions (`src/extensions/`)

The static recomp's extensions, and the project's own since: EX. SETTING (its pages, and the Cheats and
Unlocks pages: cheats_unlocks.c), PLAYER CONFIG and L2 / R2, join-in and JOIN / WAIT on select screens,
network modes on one PC (VIRTUAL PSPS), MODE MENU icons, silent saves and IMPORT / EXPORT, WIDE NATIVE,
F2 back to the MODE MENU, the I-ISM extras, scaled endings. Each belongs to one feature that the
launcher's Extensions tab switches (`src/platform/ext_features.h`). Each hooks game functions listed in
`tools/data/hooks.txt`: the original is translated as `orig_f_XXXXXXXX`, the extension's replacement is
`hook_f_XXXXXXXX`, and the generated `f_XXXXXXXX` runs the hook, or the original when the extensions are
off (`--no-extensions`, or a build with `-DA3MX_EXTENSIONS=OFF`, which leaves `src/extensions/` out and
ignores `hooks.txt`: [BUILDING.md](BUILDING.md#without-the-extensions)).
When the hooked function is decompiled, the extension becomes a direct call or setting in the
decompiled code ([ROADMAP.md](ROADMAP.md), phase 4).

## 5. Parity tests (`tests/`)

`tests/parity.py` runs each scenario twice, with the originals and with the decompiled code. Both runs
are headless and deterministic (`--fast`, software renderer, fresh saves), and it compares a hash of
the program's whole memory image and of the frame buffer on every frame. The first difference fails the
test. `ctest --test-dir build` runs all scenarios. For more details see [TESTS.md](TESTS.md).
