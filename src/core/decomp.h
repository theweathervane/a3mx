/* How a hand-written (decompiled) function takes the place of a translated one.
 *
 *     SFA_FUNC(089F3430, game_memcpy) { ...C that does what the PSP code at 0x089F3430 did... }
 *
 * tools/recomp/psp_recomp.py finds every SFA_FUNC line in src/core, translates the original as
 * orig_f_089F3430 instead of f_089F3430, and this macro supplies f_089F3430: every caller in the
 * translated program (direct calls and the pointer-call table alike) now runs the code here.
 *
 * The body is an adapter between the PSP calling convention and plain C: it reads arguments with
 * ARG(n), calls a typed, readable implementation, and sets the result with RETURN(v). Keep register
 * talk in the adapter only; the implementation should read like ordinary C.
 *
 * The original stays callable for checking: --reference runs every original instead of the
 * hand-written code, and --reference-only ADDR[,ADDR...] just the ones listed (bisecting a parity
 * failure). tests/parity.py compares the two runs frame by frame.
 */
#pragma once
#include "cpu.h"
#include "guest.h"

int decomp_use_original(uint32_t addr);   /* decomp.c: --reference / --reference-only */

/* decomp_calls_ADDR counts calls, so the parity tests can tell which decompiled functions a scenario
   actually ran (--decomp-report). */
#define SFA_FUNC(addr, name)                                                                     \
    static void name(Cpu *c);                                                                  \
    void orig_f_##addr(Cpu *c);                                                                \
    uint64_t decomp_calls_##addr;                                                              \
    void f_##addr(Cpu *c) {                                                                    \
        decomp_calls_##addr++;                                                                 \
        if (decomp_use_original(0x##addr##u)) orig_f_##addr(c); else name(c);                  \
    }                                                                                          \
    static void name(Cpu *c)

/* A decompiled function with an extension around it (src/extensions/): `ext(c, a0, 0)` runs before
   the function and `ext(c, a0, 1)` after it, a0 being the first argument as it came in. If the first
   call returns nonzero, the extension has taken over: neither the function nor the second call runs.
   The extension runs whichever version runs, the hand-written one or the original (--reference), so
   the parity tests compare like with like. Its own changes are features, not decompiled behaviour.
   An extension that takes over and still needs the function itself (perhaps more than once) calls
   inner_f_ADDR: the function as this run has it (hand-written, or the original under --reference),
   without the extension around it. Never orig_f_ADDR, which the reference run alone would match.
   With the extensions off (--no-extensions, or a build without them) only the function runs. */
extern int rt_extensions;   /* platform/main.c: 0 with --no-extensions, or in a build without them */
/* platform/ext_features.c: the flag of the feature an extension belongs to, from its function's name. The
   extension runs only while its feature is on (--features; all of them off is --no-extensions). */
const unsigned char *feature_flag(const char *ext_name);
#ifdef A3MX_NO_EXTENSIONS
/* A build without src/extensions (-DA3MX_EXTENSIONS=OFF): the function alone, the extension not named. */
#define SFA_FUNC_WRAPPED(addr, name, ext)                                                        \
    static void name(Cpu *c);                                                                  \
    void orig_f_##addr(Cpu *c);                                                                \
    uint64_t decomp_calls_##addr;                                                              \
    void inner_f_##addr(Cpu *c) {                                                              \
        if (decomp_use_original(0x##addr##u)) orig_f_##addr(c); else name(c);                  \
    }                                                                                          \
    void f_##addr(Cpu *c) {                                                                    \
        decomp_calls_##addr++;                                                                 \
        inner_f_##addr(c);                                                                     \
    }                                                                                          \
    static void name(Cpu *c)
#else
/* With --no-extensions the extension is not called at all. */
#define SFA_FUNC_WRAPPED(addr, name, ext)                                                        \
    static void name(Cpu *c);                                                                  \
    void orig_f_##addr(Cpu *c);                                                                \
    int ext(Cpu *c, uint32_t a0, int after);                                                   \
    uint64_t decomp_calls_##addr;                                                              \
    void inner_f_##addr(Cpu *c) {                                                              \
        if (decomp_use_original(0x##addr##u)) orig_f_##addr(c); else name(c);                  \
    }                                                                                          \
    void f_##addr(Cpu *c) {                                                                    \
        static const unsigned char *on_;       /* the extension's feature (platform/ext_features.c) */ \
        uint32_t a0 = c->r[4];                                                                 \
        decomp_calls_##addr++;                                                                 \
        if (!on_) on_ = feature_flag(#ext);                                                    \
        if (*on_ && ext(c, a0, 0)) return;                                                     \
        inner_f_##addr(c);                                                                     \
        if (*on_) ext(c, a0, 1);                                                               \
    }                                                                                          \
    static void name(Cpu *c)
#endif

/* Counts a call made directly from other decompiled C (not through f_ADDR), so --decomp-report sees
   it: put COVERAGE(ADDR) first in an implementation that other decompiled code calls. */
#define COVERAGE(addr) do { extern uint64_t decomp_calls_##addr; decomp_calls_##addr++; } while (0)

/* MIPS o32 argument and result registers, for adapters only */
#define ARG(n)      (c->r[4 + (n)])
#define RETURN(v)   (c->r[2] = (uint32_t)(v))
