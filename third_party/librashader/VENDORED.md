# librashader (headers only)

The C headers of [librashader](https://github.com/SnowflakePowered/librashader), the RetroArch slang
shader runtime, from the tag `librashader-v0.12.1` (commit `07ac790cdb9826f37cf8e1daf5011f03e3c4daa7`):
C ABI version 2, API version 5.

- `include/librashader.h` and `include/librashader_ld.h`, unchanged. Their own notices give their
  licence (MIT): the headers may be used by any project.
- **The library itself is not here and is not built by this project.** `librashader_ld.h` loads
  `librashader.dll` / `librashader.so` / `librashader.dylib` when the game starts, if there is one
  (docs/DISPLAY.md, "FILTER"); the library is MPL-2.0. Without it the game runs as it always has and
  FILTER does nothing.

Used by `src/platform/shader_gl.c`. To update: copy the two headers from a newer tag, change this
file, then `python tools/pins.py --write`.
