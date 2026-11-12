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
- **Graphics**: the game draws 2D only. Software renderer as reference; OpenGL 3.3 renderer up to 8×.
- **Test hooks**: `--headless --fast --renderer soft --input ... --hash-log FILE --save-dir DIR`.

These are the hardware abstraction layer: game code reaches them only through the PSP OS functions
(`hle_*`) today. As subsystems are decompiled (input, sound, graphics submission), they get direct,
typed interfaces here instead.

## 4. Extensions (`src/extensions/`)

The static recomp's extensions, unchanged: EX. SETTING, PLAYER CONFIG and L2 / R2, join-in and JOIN /
WAIT on select screens, network modes on one PC (virtual players), pad icons, silent saves and
IMPORT / EXPORT, WIDE FORCED, F2 back to the MODE MENU. Each hooks game functions listed in
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
