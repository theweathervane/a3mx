/* The 2D library's texture lists and palettes (docs/PSP_PROGRAM_MAP.md, "The 2D library").
 *
 * A texture list is {descriptors, count}; a descriptor is 12 bytes: +0 a word (its own address
 * unless set), +4 flags, +8 a word that the flags say how to read (0x800000: the value itself,
 * otherwise where it is). 0x089E0CF4 makes a list the one in use, 0x089E0EA4 picks a descriptor
 * from it into TEX_CUR.
 */
#include "../decomp.h"

#define TEX_LIST   0x08B23334u   /* {descriptors, count}: the list in use */
#define TEX_CUR    0x08B2333Cu   /* the descriptor picked (12 bytes) */
#define TEX_PICKED 0x08ACCD80u   /* u8: its number */
#define PAL_STATE  0x08B23348u   /* +4 u16 the palette's kind, +8 / +10 u16 the last upload's start and count */
#define PAL_BASE   0x09525E08u   /* u32: where the palette entries live */
#define FLAG_CODES 0x08ACCD88u   /* 17 {code, number} pairs */

#define D_DIRECT   0x800000u     /* +8 is the value */
#define D_OWN_WORD 0x40000000u   /* +0 was given */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_089F3430(Cpu *c);   /* memcpy */

/* (list): the list in use, and its descriptor 0 picked. */
SFA_FUNC(089E0CF4, tex_use_list) {
    gaddr list = ARG(0);
    c->r[29] -= 0x20;
    CALL3(f_089F3430, TEX_LIST, list, 8);
    CALL3(f_089F3430, TEX_CUR, *W(TEX_LIST), 12);
    c->r[29] += 0x20;
    *B(TEX_PICKED) = 0;
    RETURN(0);
}

/* (n): descriptor n of the list in use picked, unless it already is. */
SFA_FUNC(089E0EA4, tex_pick) {
    uint32_t n = ARG(0);
    if (*B(TEX_PICKED) != n) {
        *B(TEX_PICKED) = (uint8_t)n;
        c->r[29] -= 0x20;
        CALL3(f_089F3430, TEX_CUR, *W(TEX_LIST) + n * 12u, 12);
        c->r[29] += 0x20;
    }
    RETURN(0);
}

/* (descriptor, word, value, flags): a descriptor set up; +0 is its own address unless given. */
SFA_FUNC(089E1204, tex_descriptor_set) {
    gaddr d = ARG(0);
    uint32_t flags = ARG(3);
    *W(d + 4) = flags;
    *W(d) = d;
    if (flags & D_DIRECT)   *W(d + 8) = ARG(2);
    if (flags & D_OWN_WORD) *W(d) = ARG(1);
}

/* A descriptor's value: +8 itself with D_DIRECT or D_OWN_WORD, otherwise the word +8 points at. */
SFA_FUNC(089E0778, tex_descriptor_value) {
    gaddr d = ARG(0);
    uint32_t flags = *W(d + 4), v = *W(d + 8);
    RETURN((flags & D_DIRECT) || (flags & D_OWN_WORD) ? v : *W(v));
}

/* The picked descriptor's value: +8 itself with D_DIRECT, otherwise the word +8 points at. */
SFA_FUNC(089E1438, tex_current_value) {
    uint32_t v = *W(TEX_CUR + 8);
    RETURN((*W(TEX_CUR + 4) & D_DIRECT) ? v : *W(v));
}

/* (code): a code's number from FLAG_CODES, by its bits 4 - 19, with its low four bits kept; a code
   not in the table is answered as it is. */
SFA_FUNC(089E055C, flag_code_number) {
    uint32_t code = ARG(0), key = code & 0xFFFF0u;
    for (int i = 0; i < 17; i++) {
        gaddr e = FLAG_CODES + (uint32_t)i * 8u;
        if (*W(e) == key) { RETURN(*W(e + 4) | (code & 0xF)); return; }
    }
    RETURN(code);
}

/* (record, a, b, c, d): a 24-byte record: +0 a, +4 b << 16, +16 c, +20 d, and by b's low byte
   +8 / +12 = 1 / 0 (0), 1 / 1 (1), 3 / 2 (2); left as they were for 3 and over. */
SFA_FUNC(089E0DE0, record_24_set) {
    gaddr r = ARG(0);
    uint32_t b = ARG(2);
    *W(r) = ARG(1);
    switch (b & 0xFF) {
    case 0: *W(r + 8) = 1; *W(r + 12) = 0; break;
    case 1: *W(r + 8) = 1; *W(r + 12) = 1; break;
    case 2: *W(r + 8) = 3; *W(r + 12) = 2; break;
    default: break;
    }
    *W(r + 4) = b << 16;
    *W(r + 16) = ARG(3);
    *W(r + 20) = ARG(4);
}

/* (start, count, entries): count palette entries copied in at start (2 bytes an entry for the kinds
   0 - 2, 4 for kind 6, none for the others). */
SFA_FUNC(089E1254, palette_upload) {
    uint32_t start = ARG(0), count = ARG(1);
    gaddr src = ARG(2);
    *H(PAL_STATE + 8) = (uint16_t)start;
    *H(PAL_STATE + 10) = (uint16_t)count;
    uint16_t kind = *H(PAL_STATE + 4);
    int32_t size = kind == 6 ? 4 : kind <= 2 ? 2 : 0;
    int64_t bytes = (int64_t)(int32_t)count * size;
    c->lo = (uint32_t)bytes; c->hi = (uint32_t)((uint64_t)bytes >> 32);
    c->r[29] -= 0x20;
    CALL3(f_089F3430, *W(PAL_BASE) + start * 2u, src, (uint32_t)bytes);
    c->r[29] += 0x20;
}
