/* The game's own string and memory routines (from the compiler's C library, linked into BOOT.BIN).
 *
 * They are called from everywhere, so they are among the first functions decompiled. Each one keeps
 * the original's exact behaviour, including the cases a host C library leaves undefined:
 *   - copies run forwards one byte at a time, so an overlapping copy gives what the PSP gave;
 *   - strcmp compares bytes as unsigned and returns their difference, not just the sign.
 */
#include "../decomp.h"

static gaddr copy_bytes(gaddr dst, gaddr src, uint32_t count) {
    uint8_t *to = guest_ptr(dst);
    const uint8_t *from = guest_ptr(src);
    for (uint32_t i = 0; i < count; i++) to[i] = from[i];
    return dst;
}

static gaddr fill_bytes(gaddr dst, uint8_t value, uint32_t count) {
    uint8_t *to = guest_ptr(dst);
    for (uint32_t i = 0; i < count; i++) to[i] = value;
    return dst;
}

static int32_t compare_strings(gaddr a, gaddr b) {
    const uint8_t *s = guest_ptr(a), *t = guest_ptr(b);
    while (*s != 0 && *s == *t) { s++; t++; }
    return (int32_t)*s - (int32_t)*t;
}

static gaddr copy_string(gaddr dst, gaddr src) {
    uint8_t *to = guest_ptr(dst);
    const uint8_t *from = guest_ptr(src);
    do { *to++ = *from; } while (*from++ != 0);
    return dst;
}

static uint32_t string_length(gaddr s) {
    const uint8_t *p = guest_ptr(s);
    uint32_t n = 0;
    while (p[n] != 0) n++;
    return n;
}

/* void *memcpy(void *dst, const void *src, size_t n) */
SFA_FUNC(089F3430, game_memcpy) { RETURN(copy_bytes(ARG(0), ARG(1), ARG(2))); }

/* void *memset(void *dst, int value, size_t n) */
SFA_FUNC(089F3470, game_memset) { RETURN(fill_bytes(ARG(0), (uint8_t)ARG(1), ARG(2))); }

/* int strcmp(const char *a, const char *b) */
SFA_FUNC(089F34A4, game_strcmp) { RETURN(compare_strings(ARG(0), ARG(1))); }

/* char *strcpy(char *dst, const char *src) */
SFA_FUNC(089F34D8, game_strcpy) { RETURN(copy_string(ARG(0), ARG(1))); }

/* size_t strlen(const char *s) */
SFA_FUNC(089F351C, game_strlen) { RETURN(string_length(ARG(0))); }

/* ---------------------------------------------------------------- more of string.h */

static int32_t compare_bytes(gaddr a, gaddr b, uint32_t count) {
    const uint8_t *s = guest_ptr(a), *t = guest_ptr(b);
    for (uint32_t i = 0; i < count; i++)
        if (s[i] != t[i]) return (int32_t)s[i] - (int32_t)t[i];
    return 0;
}

/* Copies up to count bytes, stopping after the terminator, then pads with zeros to count bytes. */
static gaddr copy_string_n(gaddr dst, gaddr src, uint32_t count) {
    uint8_t *to = guest_ptr(dst);
    const uint8_t *from = guest_ptr(src);
    while (count != 0) {
        count--;
        if ((*to++ = *from++) == 0) break;
    }
    while (count != 0) { *to++ = 0; count--; }
    return dst;
}

/* int memcmp(const void *a, const void *b, size_t n) */
SFA_FUNC(089F33E4, game_memcmp) { RETURN(compare_bytes(ARG(0), ARG(1), ARG(2))); }

/* char *strncpy(char *dst, const char *src, size_t n) */
SFA_FUNC(089F3540, game_strncpy) { RETURN(copy_string_n(ARG(0), ARG(1), ARG(2))); }
