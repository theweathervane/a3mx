/* Small steps (0.144.0): the 4 functions of 9 - 60 instructions made ready by the batch before and 4 of
   61 - 100. Pictures by type and slot, CPU levels for a fight, the network scan's results and a record
   added to a network list, a character's colours made to differ, a CPU's opponent swapped, and a hold
   counter with its text. Offsets stay numbers until something shows what they are. What the original
   leaves in a0 - t1 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define NET_SEMA  0x08A3EB88u
#define SLOTS     0x08A058A0u   /* pictures by slot: 20 bytes a slot */
#define PICTURES  0x08A3CC78u

void f_088D0C08(Cpu *c);   /* picture_by_slot */
void f_088215C4(Cpu *c);   /* object_draw */
void f_088B9F50(Cpu *c);   /* game_option_remove_kind */
void f_0883680C(Cpu *c);   /* cpu_levels_by_layout */
void f_088366D8(Cpu *c);   /* cpu_levels_versus */
void f_088DC18C(Cpu *c);   /* heap_alloc */
void f_088DC2C4(Cpu *c);   /* unmark_under_lock */
void f_088E066C(Cpu *c);   /* list_append */
void f_089F3470(Cpu *c);   /* game_memset */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088D5E44(Cpu *c);   /* game_network_battle */
void f_088A1ED8(Cpu *c);   /* game_bselect_next_colours */
void f_088347B4(Cpu *c);   /* fighter_set_412 */
void f_0880A6CC(Cpu *c);   /* game_text */

/* ---------------------------------------------------------------- pictures by slot */

/* (object): its type (+3) names a slot: 1 - 7 slot 0, 9 - 15 slot 1, 17 - 23 slot 2 (picture_by_slot
   with the owner's byte for it); 24 / 25 just drawn. Then, unless 0x0950FF4A is 3, kind 0 taken off the
   options. */
SFA_FUNC(088D0D64, picture_by_type_slot) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int32_t t = *B(o + 3);
    gaddr own = *W(o + 88);
    int slot = -1;
    if (t > 0 && t < 8) slot = 0;
    else if (t >= 9 && t < 16) slot = 1;
    else if (t >= 17 && t < 24) slot = 2;
    if (slot >= 0) {
        c->r[4] = o; c->r[5] = (uint32_t)slot; c->r[6] = (uint32_t)slot * 8 + 1;
        c->r[7] = (uint32_t)S8(own + slot) & 0xFF;
        f_088D0C08(c);
    } else if (t == 0x18 || t == 0x19) {
        c->r[4] = o; f_088215C4(c);
    }
    if (*B(0x0950FF4Au) != 3) { c->r[4] = 0; f_088B9F50(c); }
    c->r[29] += 0x20;
}

/* (object): a picture object's start: running, +148 7, +86 0x18, +164 1, layer, place and picture by
   its type from 0x08A3CB40 (8 bytes a type: picture, x, y, layer); types 0 / 8 / 16 / 24 / 25 that
   picture, else the slot's (types 1 - 7, 9 - 15, 17 - 23 from 0x08A058A0) when under 15. */
SFA_FUNC(088D09A8, picture_object_start) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 7; *H(o + 86) = 0x18; *W(o + 164) = 1;
    *B(o + 13) = 0; *B(o + 12) = 0; *H(o + 38) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    int32_t t = *B(o + 3);
    *H(o + 24) = 7; *B(o + 168) = 1; *B(o + 72) = 1;
    gaddr e = 0x08A3CB40u + (uint32_t)t * 8;
    *B(o + 9) = *B(e + 6);
    *H(o + 18) = (uint16_t)S16(e + 2);
    *H(o + 22) = (uint16_t)S16(e + 4);
    int32_t pic = -1;
    if (t == 0 || t == 8 || t == 16 || t == 24 || t == 25) pic = *B(e);
    else {
        int slot = -1;
        if (t > 0 && t < 8) slot = 0;
        else if (t >= 9 && t < 16) slot = 1;
        else if (t >= 17 && t < 24) slot = 2;
        if (slot >= 0) {
            int16_t v = S16(SLOTS + (uint32_t)slot * 20 + (uint32_t)(t - (slot * 8 + 1)) * 2);
            if (v < 15) pic = v;
        }
    }
    if (pic >= 0) { c->r[4] = o; c->r[5] = *W(PICTURES + (uint32_t)pic * 4); f_0882196C(c); }
    *H(o + 82) = 0;
    *B(o + 1) = 0; *B(o + 5) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (): CPU levels for a fight: 0x09524A6A = 0x280 with GAME +137, else 0x780; by layout when BATTLE
   +56 is set, the versus way when GAME +57 is 3, else 0x10 for each human side. */
SFA_FUNC(08836624, cpu_levels_for_fight) {
    c->r[29] -= 0x10;
    *H(0x09524A6Au) = S8(GAME + 137) != 0 ? 0x280 : 0x780;
    if (S16(BATTLE + 56) != 0) f_0883680C(c);
    else if (S8(GAME + 57) == 3) f_088366D8(c);
    else {
        if (S8(FIGHTER0 + 373) == 0) *B(FIGHTER0 + 375) = 0x10;
        if (S8(FIGHTER1 + 373) == 0) *B(FIGHTER1 + 375) = 0x10;
    }
    c->r[29] += 0x10;
}

/* (fighter, character): for a CPU (+373), the character it will be (0x25 counts as 0x15) checked
   against the opponent's (+92 +338): kept as is for 0x21 against 0x15 and 0x23 against 0x1E, and in
   other cases; Arcade-like modes (GAME +114 0) against 0x28 with a layout give 0x21, against 0x29 with
   BATTLE +66 = 1 give 0. Else, outside a network battle (or one with GAME +146), 0x15 (or any with a
   layout) becomes 0xA. */
SFA_FUNC(0889844C, cpu_character_swap) {
    gaddr f = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    uint32_t ch = ARG(1) == 0x25 ? 0x15 : ARG(1);
    c->r[16] = ch;
    uint32_t out = ch;
    if (S8(f + 373) != 0) {
        int32_t oc = S8(*W(f + 92) + 338);
        c->r[17] = GAME;
        if (oc == 0x21 && ch == 0x15) goto done;
        if (oc == 0x23 && ch == 0x1E) goto done;
        if (S8(GAME + 114) == 0) {
            if (oc == 0x28 && S16(BATTLE + 56) != 0) { out = 0x21; goto done; }
            if (oc == 0x29 && S8(BATTLE + 66) == 1) { out = 0; goto done; }
        }
        f_088D5E44(c);
        if (c->r[2] != 0 && S8(GAME + 146) == 0) goto done;
        if (ch == 0x15 || S16(BATTLE + 56) != 0) out = 0xA;
    }
done:
    RETURN(out);
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* Same character for colours: equal, or 0x15 and 0x25 either way. */
static int same_character(int32_t x, int32_t y) {
    return x == y || (x == 0x15 && y == 0x25) || (x == 0x25 && y == 0x15);
}

/* (a, b, d): the three fighters' colours picked again (a / b, b / d, a / d) while two of the same
   character share a colour (+376), at most 15 more times. */
SFA_FUNC(088A1BA4, colours_differ) {
    gaddr a = ARG(0), b = ARG(1), d = ARG(2);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21];
    c->r[29] -= 0x30;
    c->r[21] = 0; c->r[20] = 0x15; c->r[19] = 0x25; c->r[18] = a; c->r[17] = b; c->r[16] = d;
    int32_t n = 0;
    for (;;) {
        c->r[4] = a; c->r[5] = b; f_088A1ED8(c);
        c->r[4] = b; c->r[5] = d; f_088A1ED8(c);
        c->r[4] = a; c->r[5] = d; f_088A1ED8(c);
        if (!(n < 15)) break;
        n = (n + 1) & 0xFFFF;
        c->r[21] = (uint32_t)n;
        int32_t ca = S8(a + 338), cb = S8(b + 338), cd = S8(d + 338);
        if (same_character(ca, cb) && S8(a + 376) == S8(b + 376)) continue;
        if (same_character(cb, cd) && S8(b + 376) == S8(d + 376)) continue;
        if (same_character(ca, cd) && S8(a + 376) == S8(d + 376)) continue;
        break;
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21;
}

/* (game, _, fighter): a hold counter: off (+412 cleared) while the game's +40 / +43 / +72; else +350
   counts frames, +346 ticks of 0x7F, +348 rounds of those; at the end +340 on, the counter cleared and
   the fighter's side bit off the game's +42; between, fighter_set_412 (9). Its text (0x08A0F8C8 by
   +348) at 0x78 or 0x150 by side. */
SFA_FUNC(08834150, hold_counter) {
    gaddr g = ARG(0), f = ARG(2), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x30;
    c->r[17] = g; c->r[16] = f;
    if (S8(g + 40) != 0 || S8(g + 43) != 0 || S8(g + 72) != 0) *B(f + 412) = 0;
    else {
        int step = 0;
        uint16_t v = *H(f + 350);
        *H(f + 350) = (uint16_t)(v - 1);
        if (v != 0) step = 1;
        else {
            *H(f + 350) = 1;
            uint16_t t = (uint16_t)(*H(f + 346) - 1);
            *H(f + 346) = t;
            if (t == 0) {
                *H(f + 346) = 0x7F;
                uint16_t r = *H(f + 348);
                *H(f + 348) = (uint16_t)(r - 1);
                if (r != 0) step = 1;
                else {
                    *B(f + 340) = (uint8_t)(S8(f + 340) + 1);
                    *B(f + 413) = 0; *B(f + 361) = 0;
                    *H(f + 348) = 0; *H(f + 346) = 0;
                    *B(f + 343) = 0; *B(f + 342) = 0; *B(f + 341) = 0;
                    *B(g + 42) = (uint8_t)(S8(g + 42) & (0xFF - (1 << (S8(f + 337) & 31))));
                    *B(f + 412) = 0;
                }
            }
        }
        if (step) { c->r[4] = f; c->r[5] = 9; f_088347B4(c); }
        if (S8(g + 72) == 0) {
            c->r[4] = *W(0x08A0F8C8u + *H(f + 348) * 4);
            c->r[5] = 1; c->r[6] = S8(f + 337) != 0 ? 0x150 : 0x78; c->r[7] = 0; c->r[8] = 4;
            f_0880A6CC(c);
        }
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---------------------------------------------------------------- the network */

/* (record): +20 = 5; with +16 bit 3: cleared, +20 = 6, the old results (+184) let go and the scan's
   results read into a new block (+184; length on the stack); 1 then, or a failed read's result. With
   +16 bit 0: cleared and +4 returned. Else 0. */
SFA_FUNC(088DF5B4, net_scan_results) {
    gaddr r = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    gaddr len = c->r[29] + 16;
    c->r[16] = r;
    *W(len) = 0;
    *W(r + 20) = 5;
    uint32_t res = 0;
    if (*W(r + 16) & 8) {
        *W(r + 16) &= ~8u;
        *W(r + 20) = 6;
        c->r[17] = len;
        if (*W(r + 184) != 0) { c->r[4] = *W(r + 184); f_088DC2C4(c); *W(r + 184) = 0; }
        int32_t e = os_net_adhocctl_get_scan_info(c, len, 0);
        if (e < 0) { RETURN((uint32_t)e); goto out; }
        res = 1;
        if ((int32_t)*W(len) > 0) {
            c->r[4] = *W(len); f_088DC18C(c);
            gaddr buf = c->r[2];
            *W(r + 184) = buf;
            if (buf != 0) {
                e = os_net_adhocctl_get_scan_info(c, len, buf);
                if (e < 0) { RETURN((uint32_t)e); goto out; }
                *W(len) = 0;
            }
        }
    }
    if (*W(r + 16) & 1) { *W(r + 16) &= ~1u; res = *W(r + 4); }
    RETURN(res);
out:
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (record, from, kind, value): a 16-byte item (+12 = *from, +4 value, +8 kind) made and added to the
   record's list at +0x10 under the network lock; list_append's result, or 0x81001000 without memory. */
SFA_FUNC(088E0CC8, net_record_add) {
    gaddr rec = ARG(0), from = ARG(1);
    uint32_t kind = ARG(2) & 0xFF, value = ARG(3);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    c->r[29] -= 0x30;
    gaddr list = rec + 0x10;
    c->r[18] = list; c->r[16] = value; c->r[19] = from; c->r[20] = kind;
    c->r[4] = 0x10; f_088DC18C(c);
    gaddr item = c->r[2];
    c->r[17] = item;
    if (item == 0) RETURN(0x81001000u);
    else {
        c->r[4] = item; c->r[5] = 0; c->r[6] = 0x10; f_089F3470(c);
        *W(item + 12) = *W(from);
        *W(item + 4) = value;
        *B(item + 8) = (uint8_t)kind;
        c->r[16] = 0x08A40000u;
        if (*W(NET_SEMA) != 0) os_wait_sema(c, (int32_t)*W(NET_SEMA), 1, 0);
        c->r[4] = list; c->r[5] = item; f_088E066C(c);
        uint32_t r = c->r[2];
        if (*W(NET_SEMA) != 0) os_signal_sema(c, (int32_t)*W(NET_SEMA), 1);
        RETURN(r);
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
}
