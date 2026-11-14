/* Small functions not yet tied to a subsystem. */
#include "decomp.h"

#define GAME         0x08E75910u
#define G_MODE       114
#define CAMERA_LAYER 0x092E88A0u     /* the fighters' layer */
#define RING         0x092D9EF0u     /* 64 entries of four u16 */
#define RING_NEXT    0x092D9EEDu     /* u8: the entry the next call fills */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

/* Four values into the next entry of a 64-entry ring. */
SFA_FUNC(08809A08, ring_put) {
    uint16_t *e = guest_ptr(RING + *B(RING_NEXT) * 8u);
    e[0] = (uint16_t)ARG(0); e[1] = (uint16_t)ARG(1); e[2] = (uint16_t)ARG(2); e[3] = (uint16_t)ARG(3);
    *B(RING_NEXT) = (uint8_t)((*B(RING_NEXT) + 1) & 0x3F);
}

/* A 0xAC-byte record cleared for use, with its kind (+8) set. */
SFA_FUNC(088232B0, record_reset) {
    gaddr r = ARG(0);
    W(r)[0] = 0; W(r)[1] = 0; W(r)[2] = 0; W(r)[3] = 0;
    *B(r + 72) = 0;
    *H(r + 38) = 0;
    for (int i = 0; i < 5; i++) W(r + 40)[i] = 0;
    *B(r + 8) = (uint8_t)ARG(1);
    *B(r + 138) = 0; *B(r + 170) = 0; *B(r + 171) = 0;
}

/* The mode is neither Dramatic Battle (8) nor Variable Battle (15). */
SFA_FUNC(088939AC, mode_is_not_team) {
    int8_t mode = (int8_t)*B(GAME + G_MODE);
    RETURN(mode != 8 && mode != 15);
}

/* The low byte of GAME + 16 has reached 7: the object's +17 steps on and 0x08B02800 is set to -0x180. */
SFA_FUNC(088ABA0C, wait_game_16) {
    if ((*H(GAME + 16) & 0xFF) < 7) return;
    *B(ARG(0) + 17) = (uint8_t)(*B(ARG(0) + 17) + 1);
    *H(0x08B02800u) = 0xFE80;
}

/* Three of an object's switches (+78, +79, +80) become bits 0 to 2 of the fighters' layer + 123. */
SFA_FUNC(088AC4D8, layer_flags_from_object) {
    gaddr o = ARG(0);
    *B(CAMERA_LAYER + 123) = (uint8_t)((*B(o + 78) != 0) | (*B(o + 79) != 0) << 1 | (*B(o + 80) != 0) << 2);
}

void f_0880A6CC(Cpu *c);   /* text(string, font, x, y, colour) (text/queue.c) */

/* Three lines of text at the bottom of a screen: 0x089FEFE0 at (120, 186) when asked (0) and GAME + 33
   has bit 6, then 0x089FEFF4 at (68, 206) and 0x089FF014 at (112, 214). */
SFA_FUNC(0888BB24, bottom_text) {
    int8_t skip_first = (int8_t)ARG(0);
    c->r[29] -= 0x30;
    if (skip_first == 0 && (*B(GAME + 33) & 0x40) != 0) CALL5(f_0880A6CC, 0x089FEFE0u, 0, 0x78, 0xBA, 2);
    CALL5(f_0880A6CC, 0x089FEFF4u, 0, 0x44, 0xCE, 4);
    CALL5(f_0880A6CC, 0x089FF014u, 0, 0x70, 0xD6, 4);
    c->r[29] += 0x30;
}

/* A sequence that sets the low byte of GAME + 16 from a table, each entry held for a number of frames:
   0x08B02840 counts the frames left and 0x08B02844 is the entry. An entry lasts seven times the s16 at
   0x08A2C1A8[entry] less the s8 at 0x08A2C20A[entry]; the value is the s16 at 0x08A2C536[entry]. */
#define SEQ_LEFT  0x08B02840u
#define SEQ_ENTRY 0x08B02844u
SFA_FUNC(088A96B8, sequence_game_16) {
    uint32_t left = *W(SEQ_LEFT) - 1, entry = *W(SEQ_ENTRY);
    *W(SEQ_LEFT) = left;
    if (left == 0) {
        entry++;
        *W(SEQ_ENTRY) = entry;
        int32_t n = (int16_t)*H(0x08A2C1A8u + entry * 2u);
        *W(SEQ_LEFT) = (uint32_t)(n * 7 - (int8_t)*B(0x08A2C20Au + entry));
    }
    *H(GAME + 16) = (uint16_t)((*H(GAME + 16) & 0xFF00) | *H(0x08A2C536u + entry * 2u));
}

/* Seeds the game's random numbers (a Mersenne Twister: 624 words at 0x08B01564, then its place in them):
   word 0 is the seed and each next is 1812433253 * (last ^ (last >> 30)) + its number. GAME + 64 keeps
   the low half of the last word, which is also the answer. */
#define RANDOM_STATE 0x08B01564u
SFA_FUNC(08820B7C, random_seed) {
    uint32_t *mt = guest_ptr(RANDOM_STATE), v = ARG(0);
    mt[0] = v;
    for (uint32_t i = 1; i < 624; i++) {
        v = (v ^ (v >> 30)) * 0x6C078965u + i;
        mt[i] = v;
    }
    *H(RANDOM_STATE + 2496) = 0;
    *H(GAME + 64) = (uint16_t)v;
    RETURN(v);
}

/* The save question's cursor (0x09523C69) shown by an object: hidden at 0, at picture 0x91 for 1 and
   0x81 for -1 (both at +6 = 0xA0); other values leave it. */
SFA_FUNC(08819020, save_cursor_object) {
    gaddr o = ARG(0);
    int8_t cursor = (int8_t)*B(0x09523C60u + 9);
    if (cursor == 0) { *B(o + 93) = 0; return; }
    if (cursor != 1 && cursor != -1) return;
    *B(o + 93) = 1;
    *H(o + 6) = 0xA0;
    *H(o + 10) = cursor == 1 ? 0x91 : 0x81;
}

/* ---- 0.70.0 ---- */

void f_088D7EA0(Cpu *c);
void f_08808424(Cpu *c);
void f_089F3470(Cpu *c);   /* memset */
void f_089F3540(Cpu *c);   /* strncpy */
#define QUEUES      0x092DA0F0u   /* three rows of eight words, then five buffer pointers at +160 */
#define QUEUE_BYTES 0x092DA1B0u
#define QUEUE_BUFFERS 0x092DA240u /* 0x800 bytes each */

/* The ring and the queues beside it emptied, and the five buffers' addresses set. */
SFA_FUNC(0880A028, queues_reset) {
    for (uint32_t i = 0; i < 0x8C; i++) *B(QUEUE_BYTES + i) = 0;
    for (uint32_t i = 0; i < 8; i++) { *W(QUEUES + i * 4) = 0; *W(QUEUES + 64 + i * 4) = 0; *W(QUEUES + 32 + i * 4) = 0; }
    *B(RING_NEXT) = 0;
    *B(RING_NEXT - 1) = 0;
    for (uint32_t i = 0; i < 5; i++) *W(QUEUES + 160 + i * 4) = QUEUE_BUFFERS + i * 0x800u;
}

/* The current record of 216 bytes (number at 0x08E76084, records from 0x08E76088) has +202 to +208
   cleared and 0x08808424 run with 5, with the first two buffers lent to 0x08A055DC (+52, +56) and
   0x088D7EA0 set to 1 meanwhile. */
SFA_FUNC(088156FC, record_clear_and_run_5) {
    gaddr saved = c->r[16], lend = 0x08A055DCu;
    c->r[29] -= 0x20;
    c->r[4] = 1; f_088D7EA0(c);
    gaddr record = 0x08E76088u + *B(0x08E76084u) * 216u;
    *B(record + 202) = 0; *B(record + 203) = 0; *H(record + 208) = 0;
    c->r[16] = lend;
    *W(lend + 52) = QUEUE_BUFFERS; *W(lend + 56) = QUEUE_BUFFERS + 0x800u;
    *B(record + 204) = 0;
    c->r[4] = 5; f_08808424(c);
    *W(lend + 52) = 0; *W(lend + 56) = 0;
    c->r[4] = 0; f_088D7EA0(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (record, name): the eight characters at +0xB0 set from the name, or from the default at 0x08A3EB78. */
SFA_FUNC(088DF288, record_set_name) {
    gaddr to = ARG(0) + 0xB0, name = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = name; c->r[17] = to;
    c->r[4] = to; c->r[5] = 0; c->r[6] = 8; f_089F3470(c);
    c->r[4] = to; c->r[5] = name != 0 ? name : 0x08A3EB78u; c->r[6] = 8; f_089F3540(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* 0x08808424 run with 6 and 7, with 0x088D7EA0 set to 1 meanwhile. */
SFA_FUNC(08814B4C, run_6_and_7) {
    c->r[29] -= 0x20;
    c->r[4] = 1; f_088D7EA0(c);
    c->r[4] = 6; f_08808424(c);
    c->r[4] = 7; f_08808424(c);
    c->r[4] = 0; f_088D7EA0(c);
    c->r[29] += 0x20;
}

void f_088DF288(Cpu *c);   /* record_set_name */

/* The 0xC4-byte record at 0x09523740 cleared, words 0, 2 and 3 set to -1, and the default name. */
SFA_FUNC(088DEB10, record_23740_reset) {
    gaddr r = 0x09523740u, saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = r;
    c->r[4] = r; c->r[5] = 0; c->r[6] = 0xC4; f_089F3470(c);
    *W(r) = 0xFFFFFFFFu; *W(r + 8) = 0xFFFFFFFFu; *W(r + 12) = 0xFFFFFFFFu;
    c->r[4] = r; c->r[5] = 0; f_088DF288(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The sixteen 216-byte records from 0x08E76088 emptied (word 0 = -1) and the 0x28 bytes at 0x08E87908
   cleared with +4 = -1. */
SFA_FUNC(088081D4, records_reset) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    static const uint16_t words[] = { 164, 184, 156, 152, 148, 144, 180, 140, 136, 192 };
    c->r[29] -= 0x30;
    c->r[16] = 0; c->r[17] = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < 16; i++) {
        gaddr r = 0x08E76088u + i * 216u;
        c->r[18] = r; c->r[19] = i;
        c->r[4] = r + 4; c->r[5] = 0; c->r[6] = 0x80; f_089F3470(c);
        *W(r) = 0xFFFFFFFFu;
        *H(r + 210) = 0;
        for (size_t k = 0; k < sizeof words / sizeof *words; k++) *W(r + words[k]) = 0;
        *H(r + 212) = 0; *H(r + 214) = 0;
    }
    c->r[16] = 0x08E87908u; c->r[18] = 0x08E76088u + 16 * 216u; c->r[19] = 16;
    c->r[4] = 0x08E87908u; c->r[5] = 0; c->r[6] = 0x28; f_089F3470(c);
    *B(0x08E87908u + 4) = 0xFF;
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

void f_088081B0(Cpu *c);
void f_0880829C(Cpu *c);

/* 0x088081B0, the records reset (0x088081D4), then 0x0880829C. */
SFA_FUNC(088073E8, records_start) {
    c->r[29] -= 0x10;
    f_088081B0(c);
    f_088081D4(c);
    f_0880829C(c);
    c->r[29] += 0x10;
}
