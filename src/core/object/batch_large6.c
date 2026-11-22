/* More steps (0.125.0): six characters' set-ups (their routine tables copied in, their lists unpacked,
   their shadows and companion objects made), a seventh character's lists, a score put into the ranking
   tables, a fighter's name panel counted down, and a screen that resets the fight on its signal. Offsets
   stay numbers until something shows what they are. Arguments of callees, and what the original leaves
   in a0 - a3 at a call, are set as the original sets them; s0 - s8 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define P1      0x092E3EA0u
#define P2      0x092E4338u
#define P3      0x092E47D0u
#define P4      0x092E4C68u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_08896190(Cpu *c);   /* screen_start_reset */
void f_08892020(Cpu *c);   /* name_panel_texts */
void f_08822F54(Cpu *c);   /* object_make_42 */
void f_0886EB48(Cpu *c);   /* fighter_unpack_lists */
void f_08988FAC(Cpu *c);   /* fighter_unpack_lists_fixed */
void f_088D5E44(Cpu *c);   /* game_network_battle */
void f_088212F4(Cpu *c);   /* to_bcd */
void f_0886E638(Cpu *c);   /* unpack_offsets */

/* ---------------------------------------------------------------- screens and panels */

/* (screen): on its signal (+20's low byte 0xFE) it starts with the fight reset. */
SFA_FUNC(08896160, screen_reset_on_fe) {
    gaddr r = ARG(0);
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S16(r + 20) & 0xFF;
    c->r[6] = 0xFE;
    if (c->r[5] == 0xFE) f_08896190(c);
    c->r[29] += 0x20;
}

/* (fighter): +347 counts down; at 0 +341 goes on; then its name panel, shown on odd frames (GAME + 32). */
SFA_FUNC(08891FA0, name_panel_count) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    uint32_t v = *B(f + 347);
    *B(f + 347) = (uint8_t)(v - 1);
    if (v == 0) *B(f + 341) = (uint8_t)(S8(f + 341) + 1);
    c->r[6] = v; c->r[7] = v - 1;
    c->r[5] = *B(GAME + 32) & 1;
    f_08892020(c);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- characters' set-ups */

/* +171 cleared, +12 = -1, the 39 words of its routine table copied from `from` (at +416) */
static void copy_routines(gaddr f, gaddr from) {
    *B(f + 171) = 0;
    gaddr to = *W(f + 416);
    *B(f + 12) = 0xFF;
    for (uint32_t i = 0; i < 0x27; i++) *W(to + i * 4) = *W(from + i * 4);
}
static void unpack(Cpu *c, gaddr f, uint32_t kind, gaddr offs) {
    c->r[7] = 0;
    c->r[4] = f; c->r[5] = kind; c->r[6] = offs;
    f_0886EB48(c);
}
/* a 42-pool object of the character's kind (type 0), its +88 the fighter and the fighter's +40 it */
static gaddr make_companion(Cpu *c, gaddr f, uint32_t kind) {
    c->r[4] = kind; c->r[5] = 0;
    f_08822F54(c);
    return c->r[2];
}

/* (fighter): character 0x088ED71C's set-up: routines 0x08A438FC, lists by 0x08A43998, +828 = 0, its
   shadow (kind 0), +86 its side, +164 = 1. */
SFA_FUNC(088ED71C, character_setup_ed71c) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    copy_routines(f, 0x08A438FCu);
    unpack(c, f, 0, 0x08A43998u);
    *W(f + 828) = 0;
    gaddr p = make_companion(c, f, 0);
    if (p != 0) { *W(p + 88) = f; *W(f + 40) = p; }
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): kind 0xC's: routines 0x08A6BC60, lists by 0x08A6BCFC, its kind-0xC companion, +86 / +164,
   its routine table's +176 from 0x08A6A780 + 28. */
SFA_FUNC(08929B8C, character_setup_29b8c) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    copy_routines(f, 0x08A6BC60u);
    unpack(c, f, 0xC, 0x08A6BCFCu);
    gaddr p = make_companion(c, f, 0xC);
    if (p != 0) { *W(p + 88) = f; *W(f + 40) = p; }
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    *W(*W(f + 416) + 176) = *W(0x08A6A780u + 28);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): kind 6's: routines 0x08A581B0, lists by 0x08A5824C, +86 / +164, its kind-6 companion
   (the fighter's facing from it; its +86 / +164 from the fighter). */
SFA_FUNC(08908F68, character_setup_08f68) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    copy_routines(f, 0x08A581B0u);
    unpack(c, f, 6, 0x08A5824Cu);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    gaddr p = make_companion(c, f, 6);
    if (p != 0) {
        *W(f + 40) = p;
        *B(f + 11) = *B(p + 11);
        *W(p + 88) = f;
        *H(p + 86) = *H(f + 86);
        *W(p + 164) = *W(f + 164);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): kind 7's: routines 0x08A5ABA0, lists by 0x08A5AC3C, +164 = 1, its kind-7 companion in use
   (type the fighter's character, its +86 the side, +164 from the fighter). */
SFA_FUNC(0890DEC4, character_setup_0dec4) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[17] = 1;
    copy_routines(f, 0x08A5ABA0u);
    unpack(c, f, 7, 0x08A5AC3Cu);
    *W(f + 164) = 1;
    gaddr p = make_companion(c, f, 7);
    if (p != 0) {
        *B(p) = 1;
        *B(p + 1) = 0;
        *B(p + 2) = 7;
        *B(p + 3) = 0;
        *W(p + 88) = f;
        *W(f + 40) = p;
        *B(p + 3) = (uint8_t)S8(f + 338);
        *H(p + 86) = (uint16_t)(int32_t)S8(f + 337);
        *W(p + 164) = *W(f + 164);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): kind 0x1F's: routines 0x08A98C7C, lists by 0x08A98D18, +164 = 1; +600 the fighter of
   character 0x1E on its own side of a team layout (BATTLE + 56: 1 P3 / P4 by side bit 0, 2 the same
   unless side 1, 3 P1 / P2 unless side 2), else 0 (always 0 in Variable Battle, Survival kind 5, Free
   Battle kind 3 and mode 0x13); its routine table's +176 from 0x08A96F98 + 72. */
SFA_FUNC(0899A430, character_setup_9a430) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[17] = GAME;
    copy_routines(f, 0x08A98C7Cu);
    unpack(c, f, 0x1F, 0x08A98D18u);
    *W(f + 164) = 1;
    int8_t m = S8(GAME + 114);
    c->r[17] = *W(f + 416);
    int none = m == 15 || (m == 7 && S8(BATTLE + 79) == 5) || (m == 0x10 && S8(BATTLE + 159) == 3) || m == 0x13;
    if (none) *W(f + 600) = 0;
    else {
        int16_t lay = S16(BATTLE + 56);
        int side = S8(f + 337);
        gaddr p = 0;
        int write = 1;
        if (lay < 0 || lay >= 4) write = 0;
        else if (lay == 0) p = 0;
        else if (lay == 1) p = side == 0 ? 0 : (side & 1) ? P3 : P4;
        else if (lay == 2) p = side == 1 ? 0 : (side & 1) ? P3 : P4;
        else p = side == 2 ? 0 : (side & 1) ? P1 : P2;
        if (write) *W(f + 600) = (p != 0 && S8(p + 338) == 0x1E) ? p : 0;
    }
    *W(c->r[17] + 176) = *W(0x08A96F98u + 72);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): kind 0x1C's: routines 0x08A943D4, the fixed lists, +86 / +164; its record at +828: +1 = 1,
   +7 set in a network battle whose 0x09517E2A is 0x12 for the computer; +0 = 1 unless +7; companions
   0x13 (kept at +40) and, unless +7, 0x14 (at +44), each with the fighter's +12 - +14, +86, +164. */
SFA_FUNC(08982ECC, character_setup_82ecc) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[18] = 1;
    gaddr rec = f + 0x33C;
    c->r[17] = rec;
    copy_routines(f, 0x08A943D4u);
    c->r[7] = 0;
    c->r[4] = f; c->r[5] = 0x1C;
    f_08988FAC(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    *B(rec + 1) = 1;
    *B(rec + 7) = 0;
    f_088D5E44(c);
    if (c->r[2] != 0 && S16(0x09517A78u + 946) == 0x12 && S8(f + 373) != 0) *B(rec + 7) = 1;
    *B(rec) = S8(rec + 7) == 0 ? 1 : 0;
    static const uint32_t KINDS[2] = { 0x13, 0x14 };
    for (int i = 0; i < 2; i++) {
        gaddr p = make_companion(c, f, KINDS[i]);
        if (p == 0) break;
        *W(p + 88) = f;
        *B(p + 12) = (uint8_t)S8(f + 12);
        *B(p + 13) = *B(f + 13);
        *B(p + 14) = *B(f + 14);
        *H(p + 86) = *H(f + 86);
        *W(p + 164) = *W(f + 164);
        *W(f + (i == 0 ? 40 : 44)) = p;
        if (S8(rec + 7) != 0) break;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (fighter): a character's lists unpacked as 0x0886EB48 does, with the entries 1 - 3, 4 (5 for ISM -1),
   7 - 0xD, 0xF, 0x16, 6, 0xE, 0x12 - 0x15 and 0x15 twice into +0x84. */
SFA_FUNC(0896A6C4, fighter_unpack_lists_6a6c4) {
    uint32_t s[9];
    for (int i = 0; i < 8; i++) s[i] = c->r[16 + i];
    s[8] = c->r[30];
    c->r[29] -= 0x40;
    gaddr f = ARG(0), t = *W(f + 416);
    c->r[20] = f; c->r[16] = t;
#define ONE(off, n) do { c->r[4] = t + (off); c->r[5] = *W(t + 196); c->r[6] = *W(t + 192); c->r[7] = (n); \
        f_0886E638(c); } while (0)
    c->r[17] = t + 0xC8;  ONE(0xC8, 1);
    c->r[18] = t + 0x308; ONE(0x308, 2);
    c->r[19] = t + 0x548; ONE(0x548, 3);
    uint32_t n4 = S8(f + 386) == -1 ? 5 : 4;
    c->r[20] = t + 0x788; c->r[22] = t + 0x9F0; c->r[23] = t + 0x9FC;
    ONE(0x788, n4);
    c->r[21] = t + 0x9D4; ONE(0x9D4, 7);
    ONE(0x9D8, 8); ONE(0x9DC, 9); ONE(0x9E0, 0xA); ONE(0x9E4, 0xB); ONE(0x9E8, 0xC); ONE(0x9EC, 0xD);
    ONE(0x9F0, 0xF); ONE(0x9FC, 0x16);
    ONE(0x10, 6); ONE(0x18, 0xE); ONE(0x20, 0x12); ONE(0x28, 0x13); ONE(0x2C, 0x14); ONE(0x80, 0x15);
    c->r[30] = t + 0x84; ONE(0x84, 0x15); ONE(0x84, 0x15);
#undef ONE
    for (int k = 0; k < 3; k++) *W(t + 2504 + 4 * k) = c->r[21];
    *W(t + 20) = t + 0x9C8;
    *W(t) = c->r[17]; *W(t + 4) = c->r[18]; *W(t + 8) = c->r[19]; *W(t + 12) = c->r[20];
    *W(t + 28) = c->r[22]; *W(t + 36) = c->r[23];
    c->r[29] += 0x40;
    for (int i = 0; i < 8; i++) c->r[16 + i] = s[i];
    c->r[30] = s[8];
}

/* ---------------------------------------------------------------- the ranking */

static int same_character(int32_t a, int32_t b) {
    return a == b || (a == 4 && b == 0x12) || (a == 0x12 && b == 4) || (a == 6 && b == 0x14) || (a == 0x14 && b == 6);
}

/* (fighter): its score (+452, in BCD) into the ranking table after its ISM's (+450: 0x08A05AA8 + 0x2F8 *
   (ISM + 1); the entries' table 6 with an entry in use at +784), 38 records of 0x14: a record of the same
   character (Chun-Li and Sodom count with their X versions) already scoring more keeps it out; otherwise
   it goes in above the lower scores as "CAP" with its character, colour (+449), ISM and +451 (BCD) at
   +12; +460 its place + 2 (+15 too), and the other player's +460 (P1 or P2) moved down past it, or 0x27
   for the same character and ISM. */
SFA_FUNC(08892D6C, ranking_insert) {
    gaddr f = ARG(0);
    uint32_t s[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    c->r[16] = f;
    const gaddr tables = 0x08A05AA8u;
    gaddr table = tables + 0x2F8u * (uint32_t)(S8(f + 450) + 1);
    int32_t ch = S8(f + 448);
    gaddr entry = *W(f + 784);
    c->r[17] = P1;
    int16_t place = 0x25;
    uint32_t score = *W(f + 452);
    c->r[21] = 0x25;
    if (entry != 0 && S8(entry) != 0) table = tables + 0x11D0;
    c->r[19] = table;
    gaddr at = table + 0x2E4;
    for (;;) {
        if (same_character(ch, S8(at + 8))) break;
        int16_t was = place;
        place = (int16_t)(place - 1);
        at -= 0x14;
        if (was == 0) { place = 0x26; break; }
    }
    c->r[18] = (uint32_t)(int32_t)place; c->r[20] = at;
    c->r[4] = score;
    f_088212F4(c);
    uint32_t bcd = c->r[2];
    int32_t next;
    if ((int32_t)bcd < (int32_t)*W(at)) { place = 0x26; at = table + 0x2F8; next = 0x25; }
    else next = place - 1;
    place = (int16_t)next;
    uint32_t r4;
    if (place != 0x25) {
        if (place >= 0) {
            gaddr up = at - 0x14;
            for (;;) {
                uint32_t v = *W(up);
                if ((int32_t)bcd < (int32_t)v) break;
                *W(at) = v;
                for (int i = 4; i <= 7; i++) *B(at + i) = *B(up + i);
                *B(at + 8) = (uint8_t)S8(up + 8);
                *B(at + 15) = (uint8_t)S8(up + 15);
                *B(at + 9) = (uint8_t)S8(up + 9);
                *B(at + 10) = (uint8_t)S8(up + 10);
                *H(at + 12) = (uint16_t)S16(up + 12);
                int16_t was = place;
                at = up;
                place = (int16_t)(place - 1);
                up = at - 0x14;
                if (was == 0) break;
            }
        }
        *W(at) = bcd;
        *B(at + 4) = 0x43; *B(at + 5) = 0x41; *B(at + 6) = 0x50;
        *B(at + 8) = (uint8_t)S8(f + 448);
        *B(at + 9) = (uint8_t)S8(f + 449);
        *B(at + 10) = (uint8_t)S8(f + 450);
        c->r[18] = (uint32_t)(int32_t)place; c->r[20] = at;
        c->r[4] = (uint32_t)(int32_t)S8(f + 451);
        f_088212F4(c);
        *H(at + 12) = (uint16_t)(int32_t)(int8_t)c->r[2];
    }
    r4 = (uint32_t)(place + 2);
    *B(f + 460) = (uint8_t)r4;
    if (place != 0x25) {
        *B(at + 15) = (uint8_t)r4;
        gaddr other = S8(f + 337) != 0 ? P1 : P2;
        c->r[17] = other;
        int32_t o460 = S8(other + 460);
        if (o460 != 0 && !(o460 < (int16_t)r4) && S8(f + 450) == S8(other + 450)) {
            if (S8(f + 448) != S8(other + 448)) *B(other + 460) = (uint8_t)(o460 + 1);
            else *B(other + 460) = 0x27;
        }
    }
    c->r[18] = (uint32_t)(int32_t)place; c->r[20] = at;
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4]; c->r[21] = s[5];
}
