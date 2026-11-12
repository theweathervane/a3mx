# Decompiling a function

The game builds from two kinds of code at once. Every function you have not decompiled is translated
from the PSP program at build time (`build/generated/`, never committed); every function written in
`src/core/` with `SFA_FUNC` replaces its translation. So the game always runs, and the decomp grows one
function at a time, each one proven identical before it goes in.

## The loop

1. **Pick a function.** Leaves first (functions that call nothing, or only decompiled functions), then
   up the call tree. `python tools/progress.py --by-region` shows what is done. Good first targets:
   the C library (0x089E6000 onwards), small getters and setters, maths helpers.
2. **Read the translation.** `python tools/show_func.py XXXXXXXX` prints it with its callers and
   callees (`--callers` adds the callers' code). It comes from `build/generated/code_*.c`.
   Registers are `c->r[n]` (`4`-`7` arguments, `2` the result, `29` the stack pointer); `MRW` / `MWW`
   are 32-bit loads and stores, `MRH` / `MRB` 16 and 8-bit. The analysis (`work/analysis.json`) has
   the function's range and why it was found (`jal`, `dataptr` = called through a pointer table).
3. **Write it in `src/core/`**, in the file for its subsystem (create one if needed: the build picks
   up new files):
   ```c
   #include "../decomp.h"

   /* What it does, in a sentence; anything surprising about how. */
   static uint32_t string_length(gaddr s) {
       const uint8_t *p = guest_ptr(s);
       uint32_t n = 0;
       while (p[n] != 0) n++;
       return n;
   }

   /* size_t strlen(const char *s) */
   SFA_FUNC(089F351C, game_strlen) { RETURN(string_length(ARG(0))); }
   ```
   The `SFA_FUNC` body is only an adapter: arguments in with `ARG(n)`, result out with `RETURN(v)`.
   The implementation is ordinary C with real types and names. PSP OS calls go through the typed
   wrappers in `src/core/psp_os.h` (add one when you need a new OS function).
   If other decompiled code calls your implementation directly (not through `f_XXXXXXXX`), start it
   with `COVERAGE(XXXXXXXX);` so the test report sees those calls.
4. **Build and test:**
   ```
   cmake --build build          # re-translates without the function, compiles one generated file
   ctest --test-dir build       # difftest + every parity scenario, originals against your code
   python tools/progress.py --list   # which decompiled functions the tests actually ran
   ```
   **A function is merged only when it is verified:** a parity scenario runs it, or it has a difftest
   case. If neither does, add a scenario that reaches it, or a difftest generator (for functions that
   only touch their arguments), or keep it for later.
5. **Name what you learnt.** Structs, fields, enums and function names go in the headers of
   `src/core/`; addresses and findings in [PSP_PROGRAM_MAP.md](PSP_PROGRAM_MAP.md).
6. **Update the progress bars.** `python tools/progress.py --update-docs` rewrites the overall bar, the totals,
   percentages and bars in the README and the roadmap. Commit the result with your functions.

## Rules

- **Exact behaviour, not intended behaviour.** Keep the original's quirks: byte-by-byte forward copies
  (overlap), unsigned versus signed compares, integer overflow, float rounding order, the order of
  memory writes other code can see. Fixing a Capcom bug is a feature for `src/extensions/`, behind a
  setting, after parity.
- **Same results in memory.** Anything the game can read later must end up as the original left it.
  Temporaries the original kept on its own stack frame do not matter.
- **No register talk outside the adapter.** No `c->r[...]` in implementations; no `goto` mirroring
  branches where a loop or `switch` says it.
- **Calling functions not decompiled yet:** `CALL1(f_0880ABCD, x)` ... `CALL5(...)` (`guest.h`) set
  the arguments, call the translation and return v0. Declare the function (`void f_0880ABCD(Cpu *c);`)
  with a comment saying what it does, as far as is known.
- **Stack frames:** if the original passes the address of one of its locals to another function (or
  others might depend on where the stack is), reserve its frame on the guest stack for the whole call
  (`c->r[29] -= SIZE` ... `+= SIZE`) so callees see the same addresses; `pac.c` does this.
- **Game memory as structs.** Read objects through structs laid over guest memory (`guest_ptr`), with
  `GUEST_STRUCT_SIZE(type, size)` so a wrong layout fails to compile.
- **One function, one `SFA_FUNC`.** The translator stops if an address is decompiled twice, or is both
  decompiled and hooked (`tools/data/hooks.txt`): move the hook into `src/extensions/` on top of the
  decompiled function, with `SFA_FUNC_WRAPPED(ADDR, name, ext)` and
  `int ext(Cpu *c, uint32_t a0, int after)` in `src/extensions/`. It runs before (`after` 0) and after
  (`after` 1) whichever version runs, so the parity tests' reference run sees it too; returning nonzero
  before takes the function over (it and the second call are skipped). Examples: `padicons.c`,
  `sysdata.c` and `hooks.c` on the MODE MENU, and F2 in `quitkey.c`, which takes over the frame wait.
  An extension that takes over and still needs the function (once or more, with its own changes
  around each call) calls `inner_f_ADDR`: the function as the run has it, hand-written or (under
  `--reference`) the original, without the extension. Never `orig_f_ADDR` from an extension once the
  function is decompiled: only the reference run would match it. Example: the pause in `hooks.c`.
  Don't call an extension from inside a decompiled function: the reference run would miss it.
- **Assets keep their layout.** When a function shows what an archive or an entry is for (the screen
  it draws, the character it belongs to), give it its name in `tools/asset_layout.py`, the one map from
  the disc's archives to folders under `assets/`, with the evidence in a comment. A new decoder writes
  into the archive's own folder (a fighter's sprites into `characters/<nn>_<name>/fighter/sprites/`),
  never into a new top-level folder, and no path goes into C code: the game finds folders through
  `assets/<region>/index.json`. The rules: [assets/LAYOUT.md](../assets/LAYOUT.md).

## When parity fails

`tests/parity.py` prints the first frame where memory or the picture differ. Then:

- **Which function?** Run the decomp build with some functions back on their originals:
  `--reference-only 089F3430,089F3470`. When the run matches the reference again, the culprit is in
  that list. (`--reference` turns all of them back.)
- **What changed?** Dump screens around the frame from both runs (`--dump DIR --dump-from N
  --dump-every 1`), or snapshot memory (`--snap FRAME:FILE`) and compare.
- **Logs** of each run are in `build/parity/<scenario>/<reference|decomp>/run.log`.

## The difftest

`src/core/difftest.c` runs listed functions and their originals side by side on thousands of random
inputs (`--difftest N`, the `difftest` test), comparing the result and the scratch memory around the
arguments. It suits string, memory and number routines; it is how functions no scenario reaches, and
corner cases no scenario hits, get checked. Adding a function: a generator that fills the scratch area
and sets the arguments, and a `CASE(...)` line.

## Scenarios

`tests/scenarios/*.txt` drive the game with scripted buttons (see `tests/parity.py`). A function only
counts as checked if a scenario runs it, so add a scenario that reaches the mode or move your function
belongs to (World Tour, Dramatic Battle, a character's specials ...). Keep each under a few minutes.
Besides `input` (buttons by frame), a scenario can script the analogue stick with `stick` (`FRAME=LX,LY`,
0-255, 128 centre) and pass extra options with `args`. Error paths that need a misbehaving disc are
reached with `args --io-faults busy=N,error=N,poll=N,read=N` (`disc_faults.txt`): every Nth call of each
kind fails, deterministically, so the reference and decompiled runs see the same failures.

## Japanese version

The JP program is almost the same code at shifted addresses. The plan: once the US decomp settles a
subsystem, map its functions onto the JP program (by matching instruction patterns) and build the JP
version from the same `src/core/` with a per-region address table. Until then, write code against the
US addresses only.
