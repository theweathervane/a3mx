/* Reading and writing the game's own memory by its PSP address.
 *
 *     *B(a), *H(a), *W(a)   the byte, halfword or word at a (read or write)
 *     S8(a), S16(a)         the byte or halfword at a, sign-extended
 *
 * Decompiled code keeps offsets as numbers until something shows what they are, so it reads and
 * writes through these. They are static inline: a file that does not use one gets no warning for it.
 */
#pragma once
#include "guest.h"

static inline uint8_t  *B(gaddr a)  { return guest_ptr(a); }
static inline uint16_t *H(gaddr a)  { return guest_ptr(a); }
static inline uint32_t *W(gaddr a)  { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline int16_t  S16(gaddr a) { return (int16_t)*H(a); }
