# Vendored: Dear ImGui (the launcher's interface)

- Source: https://github.com/ocornut/imgui, tag **v1.93.0** (commit
  c1aaac65e896a45f81f47a4d68f9959e0f86c859), files copied unchanged.
- Files: the core (`imgui*.cpp`, `imgui*.h`, `imconfig.h`, `imstb_*.h`) and two backends,
  `backends/imgui_impl_sdl3.*` (window and input) and `backends/imgui_impl_sdlrenderer3.*` (drawing
  through SDL's own renderer, so no OpenGL / Vulkan / Metal code is needed). The demo, examples and
  other backends are left out.
- Licence: **MIT** (`LICENSE.txt`), compatible with this project's GPL-3.0-or-later.
- Used only by `src/launcher/` (the `a3mx-launcher` program), never by the game.
- To update: copy the same files from a newer tag and change the tag and commit above.
- Credited, with the project's other sources, in [ACKNOWLEDGMENTS.md](../../ACKNOWLEDGMENTS.md).
