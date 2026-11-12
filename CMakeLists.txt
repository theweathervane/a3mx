# A3MX decompilation. Clang (LLVM-MinGW on Windows), CMake 3.25+ and Ninja.
#
#   python tools/build.py                       all of it: extract your disc, configure, build (docs/BUILDING.md)
# or by hand:
#   python tools/extract_assets.py              extract and verify your disc into work/
#   cmake --preset release                      configure (CMakePresets.json; downloads SDL3 once)
#   cmake --build --preset release              translate what is not decompiled yet, and compile
#   ctest --preset release                      parity tests: decompiled code against the originals
#   build/a3mx --work work                   play
#
# Without the disc (CI): cmake --preset ci compiles every hand-written file and runs the tests that
# need no game data (A3MX_DISC_FREE).
#
# The program is built from two kinds of code:
#   src/core/          hand-written C, one game function at a time (SFA_FUNC, src/core/decomp.h)
#   build/generated/   everything not decompiled yet, translated from the PSP program by
#                      tools/recomp/psp_recomp.py at build time
# plus src/platform/ (the PSP's OS, graphics, sound and input on a PC) and src/extensions/
# (the extensions carried over from the static recomp).
cmake_minimum_required(VERSION 3.20)
# macOS: run on macOS 11 and later (the first for Apple silicon), not only the macOS that built it, and
# native on Apple silicon even when CMake itself runs under Rosetta (an Intel Homebrew on an M-series
# Mac), so nothing ever asks for Rosetta. Both before project(); either can still be set by hand.
# (/usr/bin/sw_vers exists only on macOS: APPLE isn't known yet here.)
if(EXISTS "/usr/bin/sw_vers")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "The oldest macOS the programs run on")
  if(NOT CMAKE_OSX_ARCHITECTURES)
    execute_process(COMMAND /usr/sbin/sysctl -n hw.optional.arm64 OUTPUT_VARIABLE A3MX_ARM64_MAC
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(A3MX_ARM64_MAC STREQUAL "1")
      set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Apple silicon: arm64, whatever CMake runs as")
    endif()
  endif()
endif()
project(a3mx C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(A3MX_WORK_DIR "${CMAKE_SOURCE_DIR}/work" CACHE PATH "The folder tools/extract_assets.py wrote")
set(A3MX_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/us" CACHE PATH "The unpacked assets tools/extract_assets.py wrote (tests)")
option(A3MX_SDL "Window, input and sound through SDL3 (downloaded automatically)" ON)
option(A3MX_EGL "Headless OpenGL through EGL, for tests (Linux)" OFF)
option(A3MX_WATCH "Debug: memory watchpoints (--watch); slower" OFF)
option(A3MX_TESTS "Parity tests (ctest)" ON)
option(A3MX_EXTENSIONS "The extensions carried over from the static recomp (src/extensions/). OFF: the PSP program alone (also --no-extensions at run time)" ON)
option(A3MX_DISC_FREE "No disc: compile the hand-written code (not linked into a game) and run the tests that need no game data (CI)" OFF)

if(MSVC)
  message(FATAL_ERROR "Build with Clang (LLVM-MinGW on Windows): python tools/build.py, or cmake --preset release (docs/BUILDING.md)")
endif()

# The version (VERSION: shown on EX. SETTING and in the launcher's corner); changing the file re-runs CMake
# MAJOR.MINOR.PATCH; a nightly build's VERSION also carries -nightly.<date>+<commit> (written by the
# workflow, and into the nightly's download, so a player's own build of it says so too).
file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" A3MX_VER LIMIT_COUNT 1)
string(STRIP "${A3MX_VER}" A3MX_VER)
# Shown as v<VERSION> everywhere, as the tags are (v0.34.2), except on EX. SETTING, whose font has
# capitals, digits and . / - only: V<VERSION> in capitals there, without a nightly's +commit part
string(REGEX REPLACE "\\+.*$" "" A3MX_VER_SCREEN "${A3MX_VER}")
string(TOUPPER "${A3MX_VER_SCREEN}" A3MX_VER_SCREEN)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/VERSION")

# The launcher (src/launcher/): pick your ISO and the options, Run or Build, the log underneath. It holds
# no game code, so it needs neither the disc nor Python to build: cmake --preset launcher.
option(A3MX_LAUNCHER "Build the launcher (a3mx-launcher) beside the game" ON)
option(A3MX_LAUNCHER_ONLY "Build only the launcher: no disc, no Python (preset launcher)" OFF)
if(A3MX_LAUNCHER_ONLY)
  include("${CMAKE_SOURCE_DIR}/cmake/SDL3.cmake")
  add_subdirectory(src/launcher)
  return()
endif()

find_package(Python3 3.9 COMPONENTS Interpreter REQUIRED)
if(NOT A3MX_DISC_FREE)
  foreach(f boot.elf analysis.json imports.json analysis.image.bin)
    if(NOT EXISTS "${A3MX_WORK_DIR}/${f}")
      message(FATAL_ERROR "${A3MX_WORK_DIR}/${f} is missing. Put your ISO in imports/psp/ and run: python tools/build.py (or python tools/extract_assets.py)")
    endif()
  endforeach()
endif()

# ---------------------------------------------------------------- translation of the rest
set(GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/core/*.c" "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
if(A3MX_DISC_FREE)
  # no program to translate: a funcs.h declaring just the names the hand-written code uses
  execute_process(COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_SOURCE_DIR}/tools/recomp/stub_funcs.py" "${CMAKE_SOURCE_DIR}/src" "${GEN_DIR}"
                  RESULT_VARIABLE stub_rc)
  if(NOT stub_rc EQUAL 0)
    message(FATAL_ERROR "tools/recomp/stub_funcs.py failed")
  endif()
  set(GEN_SRC "")
else()
set(GEN_FILES 40)
set(GEN_SRC "${GEN_DIR}/dispatch.c" "${GEN_DIR}/decomp_table.c")
math(EXPR GEN_LAST "${GEN_FILES} - 1")
foreach(i RANGE ${GEN_LAST})
  string(LENGTH "${i}" len)
  if(len EQUAL 1)
    set(n "00${i}")
  elseif(len EQUAL 2)
    set(n "0${i}")
  else()
    set(n "${i}")
  endif()
  list(APPEND GEN_SRC "${GEN_DIR}/code_${n}.c")
endforeach()

set(GEN_NO_HOOKS "")
if(NOT A3MX_EXTENSIONS)
  set(GEN_NO_HOOKS --no-hooks)      # no hooks to call: every hooked function is translated as itself
endif()
file(GLOB TOOL_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tools/recomp/*.py")
set(WS_JSON "")
if(EXISTS "${A3MX_WORK_DIR}/boot.widescreen.json")
  set(WS_JSON "${A3MX_WORK_DIR}/boot.widescreen.json")
endif()
# Unchanged outputs keep their timestamps (psp_recomp.py writes only what changed), so decompiling
# one function recompiles one generated file.
add_custom_command(
  OUTPUT ${GEN_SRC} "${GEN_DIR}/funcs.h"
  COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_SOURCE_DIR}/tools/recomp/psp_recomp.py"
          "${A3MX_WORK_DIR}/boot.elf" "${A3MX_WORK_DIR}/analysis.json" "${A3MX_WORK_DIR}/imports.json" "${GEN_DIR}"
          --core "${CMAKE_SOURCE_DIR}/src/core" --files ${GEN_FILES} ${GEN_NO_HOOKS}
  DEPENDS ${TOOL_SRC} "${CMAKE_SOURCE_DIR}/tools/data/hooks.txt" "${A3MX_WORK_DIR}/analysis.json" ${WS_JSON} ${CORE_SRC}
  COMMENT "Translating the functions not decompiled yet"
  VERBATIM)
endif()

# ---------------------------------------------------------------- third-party code
# ATRAC3/ATRAC3+ decoder (FFmpeg-derived, LGPL 2.1+), vendored from PPSSPP
file(GLOB AT3_SRC "${CMAKE_SOURCE_DIR}/third_party/at3_standalone/*.cpp")
add_library(at3_standalone STATIC ${AT3_SRC})
target_include_directories(at3_standalone PUBLIC "${CMAKE_SOURCE_DIR}/third_party/at3_standalone"
                                                 "${CMAKE_SOURCE_DIR}/third_party/at3_standalone/shim")
# KIRK (PSP crypto engine, GPL 3.0+), vendored from PPSSPP: PSP save data IMPORT / EXPORT
file(GLOB KIRK_SRC "${CMAKE_SOURCE_DIR}/third_party/libkirk/*.c")
add_library(libkirk STATIC ${KIRK_SRC})
target_include_directories(libkirk PUBLIC "${CMAKE_SOURCE_DIR}/third_party/libkirk")
foreach(t at3_standalone libkirk)
  target_compile_options(${t} PRIVATE -w)
endforeach()

# ---------------------------------------------------------------- the game
file(GLOB PLATFORM_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/platform/*.c" "${CMAKE_SOURCE_DIR}/src/platform/*.cpp")
file(GLOB EXT_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/extensions/*.c" "${CMAKE_SOURCE_DIR}/src/extensions/*.cpp")
if(NOT A3MX_EXTENSIONS)
  set(EXT_SRC "")                   # src/platform/extensions_off.c stands in for the names the platform uses
endif()

if(A3MX_DISC_FREE)
  set(GAME a3mx_code)          # every hand-written file, compiled; nothing to link it into without the disc
  add_library(${GAME} OBJECT ${CORE_SRC} ${PLATFORM_SRC} ${EXT_SRC})
else()
  set(GAME a3mx)
  add_executable(${GAME} ${CORE_SRC} ${PLATFORM_SRC} ${EXT_SRC} ${GEN_SRC})
endif()
target_include_directories(${GAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/src/platform" "${CMAKE_SOURCE_DIR}/src/extensions" "${CMAKE_SOURCE_DIR}/src/core" "${GEN_DIR}"
  "${CMAKE_SOURCE_DIR}/third_party/librashader/include")   # headers only: the library is loaded at run time if it is there (shader_gl.c)
target_link_libraries(${GAME} PRIVATE at3_standalone)
source_group("core (decompiled)" FILES ${CORE_SRC})
source_group("platform" FILES ${PLATFORM_SRC})
source_group("extensions" FILES ${EXT_SRC})
source_group("generated (translated)" FILES ${GEN_SRC})

if(NOT A3MX_EXTENSIONS)
  target_compile_definitions(${GAME} PRIVATE A3MX_NO_EXTENSIONS)
  message(STATUS "a3mx: built without the extensions (A3MX_EXTENSIONS=OFF)")
else()
  target_link_libraries(${GAME} PRIVATE libkirk)   # psp_savecrypt.c (EX. SETTING's IMPORT / EXPORT)
endif()
if(A3MX_EGL)
  target_compile_definitions(${GAME} PRIVATE A3MX_EGL)
  target_link_libraries(${GAME} PRIVATE EGL)
endif()
if(A3MX_WATCH)
  target_compile_definitions(${GAME} PRIVATE A3MX_WATCH)
endif()

if(A3MX_SDL)
  include("${CMAKE_SOURCE_DIR}/cmake/SDL3.cmake")
  target_link_libraries(${GAME} PRIVATE SDL3::SDL3-static)
  target_compile_definitions(${GAME} PRIVATE A3MX_SDL)
endif()

if(A3MX_SDL AND A3MX_LAUNCHER)
  add_subdirectory(src/launcher)
endif()

if(WIN32)
  target_link_libraries(${GAME} PRIVATE ws2_32)
  if(MINGW)
    target_link_options(${GAME} PRIVATE -static)   # no MinGW runtime DLLs next to the exe
  endif()
  target_sources(${GAME} PRIVATE "${CMAKE_SOURCE_DIR}/res/a3mx.rc")   # exe and window icon
endif()

target_compile_definitions(${GAME} PRIVATE "A3MX_VERSION=\"v${A3MX_VER}\"" "A3MX_VERSION_SCREEN=\"V${A3MX_VER_SCREEN}\"")

target_compile_options(${GAME} PRIVATE -Wall -Wno-unused-label -Wno-unused-parameter -Wno-unused-variable
                       $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>)
if(APPLE)
  # ucontext (fiber.c) is deprecated on macOS but still the portable way to switch stacks there
  target_compile_options(${GAME} PRIVATE -Wno-deprecated-declarations)
endif()
if(NOT WIN32 AND NOT A3MX_DISC_FREE)
  target_link_libraries(${GAME} PRIVATE m)
endif()

# ---------------------------------------------------------------- tests
if(A3MX_TESTS)
  enable_testing()
  # the PNG reader on files written the way image editors do (no disc needed)
  add_executable(asset_pac_test tests/asset_pac_test.c src/platform/asset_pac.c src/platform/png_decode.c)
  # FRAME TIMING's rules against simulated monitors (no disc, no window): SMOOTH and ORIGINAL never
  # change on their own, AUTO chooses only on the MODE MENU and on leaving SCREEN SETTING
  add_executable(pacing_test tests/pacing_test.c src/platform/pacing.c)
  target_include_directories(pacing_test PRIVATE "${CMAKE_SOURCE_DIR}/src/platform" "${GEN_DIR}")
  add_dependencies(pacing_test ${GAME})          # rt.h includes the generated funcs.h
  if(NOT WIN32)
    target_link_libraries(pacing_test PRIVATE m)
  endif()
  add_test(NAME pacing_rules COMMAND pacing_test)
  set_tests_properties(pacing_rules PROPERTIES TIMEOUT 60 LABELS platform)
  add_test(NAME png_cases COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/png_cases.py" "${CMAKE_BINARY_DIR}/png_cases")
  set_tests_properties(png_cases PROPERTIES FIXTURES_SETUP png_cases LABELS assets)
  if(A3MX_DISC_FREE)
    add_test(NAME png_decoder COMMAND asset_pac_test --png-only "${CMAKE_BINARY_DIR}/png_cases" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_tests_properties(png_decoder PROPERTIES TIMEOUT 120 LABELS assets FIXTURES_REQUIRED png_cases)
  else()
    add_test(NAME difftest COMMAND a3mx --work "${A3MX_WORK_DIR}" --headless --log "${CMAKE_BINARY_DIR}/difftest.log" --difftest 20000)
    set_tests_properties(difftest PROPERTIES TIMEOUT 300 LABELS difftest)
    # the archives built from assets/ (src/platform/asset_pac.c) against the disc's
    add_test(NAME asset_pac COMMAND asset_pac_test "${A3MX_WORK_DIR}/iso/PSP_GAME/USRDIR/usa/data" "${A3MX_ASSETS_DIR}" "${CMAKE_BINARY_DIR}/png_cases"
             WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_tests_properties(asset_pac PROPERTIES TIMEOUT 300 LABELS assets FIXTURES_REQUIRED png_cases)
    add_test(NAME assets_in_game COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/assets_test.py"
             --exe $<TARGET_FILE:a3mx> --work "${A3MX_WORK_DIR}" --assets "${A3MX_ASSETS_DIR}" --out "${CMAKE_BINARY_DIR}/assets_test")
    set_tests_properties(assets_in_game PROPERTIES TIMEOUT 900 LABELS assets)
    add_test(NAME stage_strips COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/stage_strips_test.py"
             --exe $<TARGET_FILE:a3mx> --work "${A3MX_WORK_DIR}" --assets "${A3MX_ASSETS_DIR}" --out "${CMAKE_BINARY_DIR}/stage_strips_test")
    set_tests_properties(stage_strips PROPERTIES TIMEOUT 600 LABELS assets)
    add_test(NAME pac_roundtrip COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/pac_roundtrip.py" --work "${A3MX_WORK_DIR}")
    set_tests_properties(pac_roundtrip PROPERTIES TIMEOUT 300 LABELS assets)
    file(GLOB SCENARIOS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/scenarios/*.txt")
    foreach(s ${SCENARIOS})
      get_filename_component(name "${s}" NAME_WE)
      add_test(NAME parity_${name}
               COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/parity.py"
                       --exe $<TARGET_FILE:a3mx> --work "${A3MX_WORK_DIR}" --scenario "${s}"
                       --out "${CMAKE_BINARY_DIR}/parity")
      set_tests_properties(parity_${name} PROPERTIES TIMEOUT 1500 LABELS parity)
    endforeach()
  endif()
endif()
