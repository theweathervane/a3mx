# SDL3, linked statically: the game's window, sound and input, and the launcher's window.
# If tools/sdl3.py has built the pinned release once into the tools folder (.tools/sdl3/: the pipelines
# keep it in their cache), that is used. Otherwise it is downloaded at configure time (pinned by hash)
# and compiled inside this build folder, as it always has been.
if(NOT TARGET SDL3::SDL3-static)
  if(DEFINED ENV{A3MX_TOOLS_DIR} AND NOT "$ENV{A3MX_TOOLS_DIR}" STREQUAL "")
    set(A3MX_SDL3_PREFIX "$ENV{A3MX_TOOLS_DIR}/sdl3")
  else()
    set(A3MX_SDL3_PREFIX "${CMAKE_SOURCE_DIR}/.tools/sdl3")
  endif()
  if(EXISTS "${A3MX_SDL3_PREFIX}/a3mx-built.txt")
    find_package(SDL3 3.4.16 EXACT CONFIG QUIET COMPONENTS SDL3-static PATHS "${A3MX_SDL3_PREFIX}" NO_DEFAULT_PATH)
    if(TARGET SDL3::SDL3-static)
      message(STATUS "a3mx: SDL3 ${SDL3_VERSION} as built once in ${A3MX_SDL3_PREFIX} (tools/sdl3.py)")
    endif()
  endif()
endif()
if(NOT TARGET SDL3::SDL3-static)
  include(FetchContent)
  FetchContent_Declare(SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(SDL3)
endif()
