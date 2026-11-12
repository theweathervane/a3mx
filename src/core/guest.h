/* Guest (PSP) memory as seen from decompiled code.
 *
 * Game data lives in one host block that mirrors the PSP's address space (platform/cpu.h). A guest
 * address is a uint32_t (gaddr); guest_ptr() turns it into a host pointer. The PSP and every
 * supported host are little-endian, so structures laid out as the game lays them out can be read
 * through such a pointer directly; use the GUEST_STRUCT size checks so a layout mistake fails to
 * compile instead of desynchronising a match.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "cpu.h"

typedef uint32_t gaddr;   /* an address in PSP memory */

static inline void *guest_ptr(gaddr a) { return (void *)HOST(a); }
static inline gaddr guest_addr(const void *p) { return 0x08000000u + (uint32_t)((const uint8_t *)p - HOST(0x08000000u)); }

/* static_assert that a struct mirroring game memory has the size the game uses */
#define GUEST_STRUCT_SIZE(type, size) _Static_assert(sizeof(type) == (size), #type " must be " #size " bytes, as in the game")

/* Calls a translated (not yet decompiled) game function with up to five arguments; returns v0.
   For decompiled code that still depends on functions nobody has rewritten. */
typedef void (*GuestFunc)(Cpu *c);
static inline uint32_t guest_call(Cpu *c, GuestFunc f, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    c->r[4] = a0; c->r[5] = a1; c->r[6] = a2; c->r[7] = a3; c->r[8] = a4;
    f(c);
    return c->r[2];
}
#define CALL0(f)                 guest_call(c, f, 0, 0, 0, 0, 0)
#define CALL1(f, a)              guest_call(c, f, (a), 0, 0, 0, 0)
#define CALL2(f, a, b)           guest_call(c, f, (a), (b), 0, 0, 0)
#define CALL3(f, a, b, d)        guest_call(c, f, (a), (b), (d), 0, 0)
#define CALL4(f, a, b, d, e)     guest_call(c, f, (a), (b), (d), (e), 0)
#define CALL5(f, a, b, d, e, g)  guest_call(c, f, (a), (b), (d), (e), (g))
