/* Number <-> text helpers from the game's C library, used by its printf-style formatting
 * (0x089F370C formats floats with them).
 */
#include "../decomp.h"

#define DIGIT_CHARS 0x08AD84E0u   /* "0123456789abcdef..." in .data */

/* Reverses a string in place; returns it. */
static gaddr reverse_string(gaddr s) {
    COVERAGE(089F3604);
    int8_t *p = guest_ptr(s);
    int32_t len = 0;
    while (p[len] != 0) len++;
    for (int32_t i = 0, j = len - 1; i < j; i++, j--) {
        int8_t t = p[i]; p[i] = p[j]; p[j] = t;
    }
    return s;
}

/* Allegrex divide-unsigned semantics (no trap; x / 0 gives all ones, remainder x). */
static uint32_t udiv(uint32_t x, uint32_t d) { return d ? x / d : 0xFFFFFFFFu; }
static uint32_t umod(uint32_t x, uint32_t d) { return d ? x % d : x; }

/* Writes value in base (2..16) to buf; returns buf.
 * Quirks kept: only base 10 negates a negative value, but a '-' is added for any negative value,
 * so in other bases it is the two's complement digits with a minus sign. */
static gaddr int_to_string(int32_t value, gaddr buf, uint32_t base) {
    int negative = value < 0;
    uint32_t v = (negative && base == 10) ? (uint32_t)-(int64_t)value : (uint32_t)value;
    const int8_t *digits = guest_ptr(DIGIT_CHARS);
    int8_t *out = guest_ptr(buf);
    uint32_t n = 0;
    do {
        out[n++] = digits[umod(v, base)];
        v = udiv(v, base);
    } while (v != 0);
    if (negative) out[n++] = '-';
    out[n] = 0;
    return reverse_string(buf);
}

/* Rounds a string of decimal digits up at its last digit, carrying left: "1299" + "5"-or-more at the
 * end becomes "1300". Returns 0 when the carry runs off the front (all nines), 1 otherwise.
 * Quirks kept: the carry stops at index 0 without looking further, and for len == 1 the carry lands
 * on buf[-1] (the caller's sign or padding byte), as on the PSP. */
static int32_t round_digits(gaddr buf, int32_t len) {
    int8_t *d = guest_ptr(buf);
    if (len <= 0) return 1;
    int32_t i = len - 1;
    if (d[i] < '5') return 1;
    int32_t j;
    for (;;) {
        d[i] = '0';
        j = i - 1;
        if (j <= 0 || d[j] != '9') break;
        i = j;
    }
    if (d[j] == '9') return 0;
    d[j] = (int8_t)(d[j] + 1);
    return 1;
}

/* char *strrev(char *s) */
SFA_FUNC(089F3604, game_strrev) { RETURN(reverse_string(ARG(0))); }

/* char *itoa(int value, char *buf, int base) */
SFA_FUNC(089F3670, game_itoa) { RETURN(int_to_string((int32_t)ARG(0), ARG(1), ARG(2))); }

/* int round_digits(char *digits, int len): used by the float formatter */
SFA_FUNC(089F35A0, game_round_digits) { RETURN(round_digits(ARG(0), (int32_t)ARG(1))); }
