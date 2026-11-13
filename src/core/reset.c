/* Clearing things before a screen or a fight starts. */
#include "decomp.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

static void clear(gaddr a, uint32_t n) { for (uint32_t i = 0; i < n; i++) *B(a + i) = 0; }

void f_089F3470(Cpu *c);   /* memset */
void f_088E3FE0(Cpu *c);   /* net_step_clear_25 (network/state.c) */

SFA_FUNC(08822D04, battle_clear) { clear(BATTLE, 0xB0); }
SFA_FUNC(08822C4C, fighters_3_4_clear) { for (uint32_t i = 0; i < 0x498u; i++) { *B(FIGHTER_3 + i) = 0; *B(FIGHTER_4 + i) = 0; } }
SFA_FUNC(08822B60, ticker_clear) { clear(0x092E51C0u, 0xDB4); }      /* the object runners.c runs, and what follows it */
SFA_FUNC(08822BB0, clear_0950FFA0) { clear(0x0950FFA0u, 4); }

/* The 0xC0-byte object at 0x092E5100 cleared (+8 = 8), with GAME + 38, + 39 and + 40. */
SFA_FUNC(08822A64, clear_092E5100) {
    clear(0x092E5100u, 0xC0);
    *B(0x092E5100u + 8) = 8;
    *B(GAME + 40) = 0; *B(GAME + 38) = 0; *B(GAME + 39) = 0;
}

SFA_FUNC(088081B0, clear_08E76068) {
    gaddr p = 0x08E76068u;
    *H(p + 8) = 0; *H(p + 10) = 0;
    *W(p + 12) = 0; *W(p + 20) = 0; *W(p + 16) = 0; *W(p + 24) = 0;
}

/* Sixteen 0x24-byte entries at 0x08E86E20 marked unused, with two flags. */
SFA_FUNC(0880829C, clear_08E86E20) {
    for (int i = 0; i < 16; i++) *W(0x08E86E20u + (uint32_t)i * 0x24u) = 0;
    *B(0x08E76084u) = 0;
    *B(0x08E87060u) = 0;
}

/* 0x2800 bytes at 0x092DCA40 set to 0xFF. */
SFA_FUNC(0880A39C, fill_092DCA40) {
    c->r[29] -= 0x20;
    CALL3(f_089F3470, 0x092DCA40u, 0xFF, 0x2800);
    c->r[29] += 0x20;
}

/* Three queues emptied (every entry marked unused), each ending with the network step's flag cleared:
   the text queue (96 of 16 bytes at 0x092DF240), 4 of 0x14 at 0x092DF930, 10 of 0x18 at 0x092DF840. */
static void queue_clear(Cpu *c, gaddr first, int count, uint32_t size) {
    for (int i = 0; i < count; i++) *B(first + (uint32_t)i * size) = 0;
    c->r[29] -= 0x10;
    f_088E3FE0(c);
    c->r[29] += 0x10;
}
SFA_FUNC(0880A87C, text_queue_clear) { queue_clear(c, 0x092DF240u, 96, 0x10); }
SFA_FUNC(0880A8B8, queue_4_clear)    { queue_clear(c, 0x092DF930u, 4, 0x14); }
SFA_FUNC(0880A8F4, queue_10_clear)   { queue_clear(c, 0x092DF840u, 10, 0x18); }

/* A record's counter (+6) on by one and its work fields cleared, with byte 15 of a second record. */
SFA_FUNC(08824AA0, record_next_08824AA0) {
    gaddr r = ARG(0);
    *H(r + 10) = 0;
    *H(r + 6) = (uint16_t)(*H(r + 6) + 1);
    *H(r + 8) = 0;
    *B(r + 72) = 0; *B(r + 43) = 0; *B(r + 125) = 0;
    *B(ARG(1) + 15) = 0;
}

/* The pair of words the table 0x08A0E244 holds for the first argument's +56, into the two outs. */
SFA_FUNC(08826148, pair_by_layout) {
    gaddr entry = 0x08A0E244u + (uint32_t)(int16_t)*H(ARG(0) + 56) * 8u;
    *W(ARG(1)) = W(entry)[0];
    *W(ARG(2)) = W(entry)[1];
}

/* The stage's three bytes from 0x08A09310 into +122 of the three layers at 0x092E8800, 0x092E88A0 and
   0x092E8940. */
SFA_FUNC(0881D488, stage_set_layers_122) {
    gaddr t = 0x08A09310u + (uint32_t)(int16_t)*H(BATTLE) * 3u;
    *B(0x092E8800u + 122) = *B(t);
    *B(0x092E88A0u + 122) = *B(t + 1);
    *B(0x092E8940u + 122) = *B(t + 2);
}

#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
void f_08822C4C(Cpu *c);   /* fighters_3_4_clear, above */

SFA_FUNC(08822C00, fighters_clear) {
    for (uint32_t i = 0; i < 0x498u; i++) { *B(FIGHTER_1 + i) = 0; *B(FIGHTER_2 + i) = 0; }
    c->r[29] -= 0x10;
    f_08822C4C(c);
    c->r[29] += 0x10;
}

/* The fade back to full: every slot's level (0x80 bytes at 0x092E11A0), the level for everything else,
   and 0x08E75C44. */
SFA_FUNC(0882E0D0, fade_reset) {
    for (uint32_t i = 0; i < 0x80u; i++) *B(0x092E11A0u + i) = 0xFF;
    *B(0x092E1220u) = 0xFF;
    *H(0x08E75C44u) = 0xFF;
}

/* The stage's colour number (BATTLE + 102) from the byte table 0x08A0F1C4. */
SFA_FUNC(0882E110, stage_set_colour_number) {
    *H(BATTLE + 102) = (uint16_t)(int8_t)*B(0x08A0F1C4u + ARG(0));
}

/* The s16 at 0x09524A68 held to 0 - 255. */
SFA_FUNC(08836E90, clamp_09524A68) {
    int16_t v = (int16_t)*H(0x09524A68u);
    if (v < 0) *H(0x09524A68u) = 0;
    else if (v >= 256) *H(0x09524A68u) = 0xFF;
}

/* The same s16 changed by the amount given, and held to 0 - 255. */
void f_08836E90(Cpu *c);
SFA_FUNC(08836E60, add_to_09524A68) {
    *H(0x09524A68u) = (uint16_t)((int16_t)*H(0x09524A68u) + (int16_t)ARG(0));
    c->r[29] -= 0x10;
    f_08836E90(c);
    c->r[29] += 0x10;
}
