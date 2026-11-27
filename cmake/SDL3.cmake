# SDL3, linked statically: the game's window, sound and input, and the launcher's window.
# If tools/sdl3.py has built the pinned release once into the tools folder (.tools/sdl3/: the pipelines
# keep it in their cache), that is used. Otherwise it is downloaded at configure time (pinned by hash)
# and compiled inside this build folder, as it always has been.
#
# Either way it carries the project's patch, cmake/patches/sdl-gpu-native-handles.patch (0.176.0: the
# GPU backend's native device, queue, command buffers and textures as properties, for FILTER on Vulkan /
# Direct3D 12 / Metal; SDL issue #10876), applied by tools/apply_patch.py. Without Python (the launcher-only
# preset) SDL3 is built as it comes: the launcher doesn't use it, and the game then shows FILTER as
# OPENGL ONLY on those renderers.
set(A3MX_SDL3_PATCH "${CMAKE_SOURCE_DIR}/cmake/patches/sdl-gpu-native-handles.patch")
file(SHA256 "${A3MX_SDL3_PATCH}" A3MX_SDL3_PATCH_SHA256)
if(NOT TARGET SDL3::SDL3-static)
  if(DEFINED ENV{A3MX_TOOLS_DIR} AND NOT "$ENV{A3MX_TOOLS_DIR}" STREQUAL "")
    set(A3MX_SDL3_PREFIX "$ENV{A3MX_TOOLS_DIR}/sdl3")
  else()
    set(A3MX_SDL3_PREFIX "${CMAKE_SOURCE_DIR}/.tools/sdl3")
  endif()
  set(A3MX_SDL3_STAMP "")
  if(EXISTS "${A3MX_SDL3_PREFIX}/a3mx-built.txt")
    file(READ "${A3MX_SDL3_PREFIX}/a3mx-built.txt" A3MX_SDL3_STAMP)
  endif()
  if(A3MX_SDL3_STAMP MATCHES "${A3MX_SDL3_PATCH_SHA256}")   # (one built before the patch, or with another: not used)
    find_package(SDL3 3.4.16 EXACT CONFIG QUIET COMPONENTS SDL3-static PATHS "${A3MX_SDL3_PREFIX}" NO_DEFAULT_PATH)
    if(TARGET SDL3::SDL3-static)
      message(STATUS "a3mx: SDL3 ${SDL3_VERSION} as built once in ${A3MX_SDL3_PREFIX} (tools/sdl3.py)")
    endif()
  endif()
endif()
if(NOT TARGET SDL3::SDL3-static)
  include(FetchContent)
  find_package(Python3 3.9 COMPONENTS Interpreter QUIET)
  set(A3MX_SDL3_PATCH_COMMAND "")
  if(Python3_FOUND)
    set(A3MX_SDL3_PATCH_COMMAND PATCH_COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/apply_patch.py" "${A3MX_SDL3_PATCH}" <SOURCE_DIR>)
  else()
    message(STATUS "a3mx: SDL3 without its patch (no Python): FILTER is OPENGL ONLY on the SDL GPU renderer")
  endif()
  FetchContent_Declare(SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    ${A3MX_SDL3_PATCH_COMMAND})
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(SDL3)
endif()
