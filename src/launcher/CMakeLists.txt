# a3mx-launcher: the one window over the build and the game (launcher.cpp): paths, dependencies,
# controllers, extensions, Build / Run, with tools/build.py's and the game's output in a console underneath.
# SDL3 for the window, Dear ImGui (third_party/imgui, MIT) for the controls. No game code.
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
add_library(imgui STATIC
  "${IMGUI_DIR}/imgui.cpp" "${IMGUI_DIR}/imgui_draw.cpp" "${IMGUI_DIR}/imgui_tables.cpp" "${IMGUI_DIR}/imgui_widgets.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp" "${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp")
target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)
target_compile_options(imgui PRIVATE -w)

# the picture at the top of the launcher's side: res/launcher_logo.png (docs/images/a3mx.png at 360
# pixels wide; the original is not touched) as a byte array, so the launcher is still one file
set(LOGO_PNG "${CMAKE_SOURCE_DIR}/res/launcher_logo.png")
file(READ "${LOGO_PNG}" LOGO_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," LOGO_BYTES "${LOGO_HEX}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/logo_png.h"
     CONTENT "/* generated from res/launcher_logo.png (src/launcher/CMakeLists.txt) */\nstatic const unsigned char LOGO_PNG[] = {${LOGO_BYTES}};\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${LOGO_PNG}")

add_executable(a3mx-launcher WIN32 launcher.cpp process.cpp disc_check.cpp icons.cpp gpu.cpp deps.cpp shortcut.cpp
               "${CMAKE_SOURCE_DIR}/src/platform/png_decode.c")   # WIN32: no console window of its own on Windows
target_include_directories(a3mx-launcher PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated" "${CMAKE_SOURCE_DIR}/src/platform")
target_link_libraries(a3mx-launcher PRIVATE imgui SDL3::SDL3-static)
set_target_properties(a3mx-launcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")   # beside a3mx
target_compile_definitions(a3mx-launcher PRIVATE "A3MX_VERSION=\"v${A3MX_VER}\"")
target_compile_options(a3mx-launcher PRIVATE -Wall -Wextra -Wno-unused-parameter)
if(APPLE)
  target_link_libraries(a3mx-launcher PRIVATE "-framework CoreFoundation")       # launcher.cpp: untranslocated()
endif()
if(WIN32)
  target_sources(a3mx-launcher PRIVATE "${CMAKE_SOURCE_DIR}/res/launcher.rc")   # its own icon (res/launcher.ico)
  target_link_libraries(a3mx-launcher PRIVATE ole32 uuid shell32)                 # shortcut.cpp: a .lnk through IShellLink
  if(MINGW)
    target_link_options(a3mx-launcher PRIVATE -static)
  endif()
endif()
