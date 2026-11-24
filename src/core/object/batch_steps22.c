/* Steps (0.151.0): the last 3 ready functions of 101 - 200 instructions. A fighter's ranking records
   written after the name entry, the fight's name plates made by layout, and a name plate's start.
   Offsets stay numbers until something shows what they are. What the original leaves in a0 - t0 at a
   call is set as the original sets it.
 */
#include "step.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define RANKINGS  0x08A05AA8u   /* 8 tables of 38 entries of 0x14 (0x2F8 a table) */
#define LETTERS   0x08A26DC0u   /* the name entry's letters */

void f_0889229C(Cpu *c);   /* name_entry_step */
void f_08892020(Cpu *c);   /* name_panel_texts */
void f_08892438(Cpu *c);   /* name_replace */
void f_08819144(Cpu *c);   /* screen_vs_backdrop_draw */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_0889CFE8(Cpu *c);   /* record_setup_layer */
void f_088988FC(Cpu *c);   /* fighter_display_side */
void f_0889CEE4(Cpu *c);   /* set_motion_by_mode */
void f_08897E2C(Cpu *c);   /* arrive_then_report_18 */
void f_0882196C(Cpu *c);   /* object_set_animation */

/* The fighter's three name letters (+456 - +458) into a ranking entry's name (+4). */
static void put_name(gaddr f, gaddr entry) {
    for (int k = 0; k < 3; k++) *B(entry + 4 + k) = *B(LETTERS + (uint32_t)(int32_t)S8(f + 456 + k));
}

/* (fighter): after its name entry step, unless the time (+350, then +346) runs on and the name isn't
   finished (+463 = 4; then the panel's texts): GAME +82 set, +341 on, +347 0x3C, +459 0x2D, the name
   replaced, and the name written into its places: the score table (1 + ISM, or 6 for an entry, +784)
   at the better of +460 / +461 (+462), and the versus table (4 + ISM, or 7) at +461, for places under
   38. With BATTLE +79 (the other modes' tables at 0x08A07268, 160 bytes a mode, 16 an entry) only at
   +460 + 1, for places under 10. */
SFA_FUNC(08892A98, record_ranking) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f;
    f_0889229C(c);
    int32_t b79 = S8(BATTLE + 79);
    int32_t place = S8(f + 460);
    int32_t done = S8(f + 463);
    if (b79 == 0) {
        int32_t vs = S8(f + 461);
        if (vs < place) place = vs;
        *B(f + 462) = (uint8_t)place;
        int finish = done == 4;
        if (!finish) {
            uint16_t t = *H(f + 350); *H(f + 350) = (uint16_t)(t - 1);
            if (t == 0) finish = 1;
            else {
                uint16_t u = *H(f + 346); *H(f + 346) = (uint16_t)(u - 1);
                if (u == 0) finish = 1;
            }
        }
        if (!finish) { c->r[4] = f; c->r[5] = 1; f_08892020(c); goto out; }
        *B(GAME + 82) = 1;
        *B(f + 341) = (uint8_t)(S8(f + 341) + 1);
        *B(f + 347) = 0x3C; *B(f + 459) = 0x2D;
        c->r[4] = f + 0x1C8; f_08892438(c);
        int32_t ism = S8(f + 450);
        gaddr entry = *W(f + 784);
        int is_entry = entry != 0 && S8(entry) != 0;
        gaddr tbl = is_entry ? RANKINGS + 0x2F8 * 6 : RANKINGS + (uint32_t)((1 + ism) * 0x2F8);
        int32_t at = S8(f + 460);
        if (at < 38) put_name(f, tbl + (uint32_t)(at * 20) - 20);
        ism = S8(f + 450);
        entry = *W(f + 784);
        is_entry = entry != 0 && S8(entry) != 0;
        tbl = is_entry ? RANKINGS + 0x2F8 * 7 : RANKINGS + (uint32_t)((4 + ism) * 0x2F8);
        at = S8(f + 461);
        if (at < 38) put_name(f, tbl + (uint32_t)(at * 20) - 20);
    } else {
        *B(f + 462) = (uint8_t)(place + 1);
        if (done != 4) { c->r[4] = f; c->r[5] = 1; f_08892020(c); goto out; }
        *B(f + 341) = (uint8_t)(S8(f + 341) + 1);
        *B(f + 347) = 0x3C; *B(f + 459) = 0x2D;
        int32_t mode = S8(BATTLE + 79);
        mode = mode < 7 ? mode - 1 : mode - 4;
        gaddr entry = 0x08A07268u + (uint32_t)(mode * 160) + (uint32_t)(place * 16);
        c->r[17] = entry;
        c->r[4] = f + 0x1C8; f_08892438(c);
        if (S8(f + 460) < 10) put_name(f, entry);
    }
out:
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (game, record): unless the game's +40 or fewer than 10 free in the pool of 32, the fight's name
   plates (kind 0xE) from the pairs at 0x08A27361 (type, fighter; -1 ends; a type with bit 7 only from
   layout 3 on) for the fighters by layout (the record's +136 / +140, or fixed sets by layout, +3 in
   the modes 0xD / 0x13 / 0x11 and specials, +411 bit 5, the game's +57), and one more for fighter 4
   when the first has +411 bit 5. The VS backdrop drawn. */
SFA_FUNC(0889783C, fight_name_plates) {
    gaddr g = ARG(0), rec = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    gaddr s21 = c->r[21], s22 = c->r[22], s23 = c->r[23];
    c->r[29] -= 0x40;
    gaddr arr = c->r[29] + 16;
    if (S8(g + 40) == 0 && !(S8(0x095176F3u) < 10)) {
        *H(g + 10) = (uint16_t)(*H(g + 10) + 1);
        uint32_t col = *B(g + 32) & 3;
        c->r[16] = col;
        *W(arr) = *W(rec + 136);
        *W(arr + 4) = *W(rec + 140);
        uint32_t layout = (uint32_t)(int32_t)S16(rec + 56);
        int32_t m = S8(g + 114);
        uint32_t k = layout & 0xFFFF;
        c->r[18] = 1; c->r[19] = 2; c->r[22] = 0xFFFFFFFFu;
        if (m == 0xD || m == 0x13 || m == 0x11) k = (layout + 3) & 0xFFFF;
        else if (S8(rec + 79) != 0 || S8(g + 137) != 0 || S8(rec + 159) != 0) {
            if (layout == 1 || layout == 2) k = (layout + 3) & 0xFFFF;
        }
        if (k == 1) {
            *W(arr) = FIGHTER0; *W(arr + 8) = FIGHTER1; *W(arr + 4) = FIGHTER2;
            if ((S8(FIGHTER0 + 411) & 0x20) == 0) { *W(arr + 4) = FIGHTER1; *W(arr + 8) = FIGHTER2; }
        }
        if (k == 2) {
            *W(arr + 8) = FIGHTER0; *W(arr) = FIGHTER1; *W(arr + 4) = FIGHTER2;
            if ((S8(FIGHTER1 + 411) & 0x20) == 0) { *W(arr + 4) = FIGHTER0; *W(arr + 8) = FIGHTER2; }
        }
        if (k == 3) {
            *W(arr) = FIGHTER0; *W(arr + 8) = FIGHTER1; *W(arr + 4) = FIGHTER2;
            if ((S8(g + 57) & 1) == 0) { *W(arr) = FIGHTER1; *W(arr + 8) = FIGHTER0; }
        }
        if (k & 4) {
            *W(arr) = FIGHTER0; *W(arr + 4) = FIGHTER2; *W(arr + 8) = FIGHTER3;
            if (k & 1) *W(arr) = FIGHTER1;
            k = 4;
        }
        c->r[17] = k; c->r[23] = (int32_t)k < 3;
        gaddr p = 0x08A27361u;
        for (int32_t t = S8(p); t != -1; p += 2, t = S8(p)) {
            c->r[20] = p;
            if (t < 0 && (int32_t)k < 3) continue;
            c->r[4] = 0xE; c->r[5] = (uint32_t)t & 0x7F; f_08822EAC(c);
            gaddr o = c->r[2];
            *B(o + 10) = (uint8_t)col;
            *B(o + 127) = (uint8_t)k;
            *W(o + 92) = *W(arr + (uint32_t)(int32_t)S8(p + 1) * 4);
        }
        if ((k == 1 || k == 2) && (S8(*W(arr) + 411) & 0x20)) {
            c->r[4] = 0xE; c->r[5] = 0; f_08822EAC(c);
            gaddr o = c->r[2];
            if (o != 0) {
                *B(o) = 1; *B(o + 10) = (uint8_t)col;
                *W(o + 92) = FIGHTER3;
                *B(o + 127) = (uint8_t)k;
            }
        }
    }
    f_08819144(c);
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
    c->r[21] = s21; c->r[22] = s22; c->r[23] = s23;
}

/* The side for a plate's place: (side bit 0, or 2 when bit 1 is clear) of the fighter's +337. */
static uint32_t plate_side(gaddr fighter) {
    int32_t s = S8(fighter + 337);
    return (s & 2) ? (uint32_t)(s & 1) : 2;
}

/* Whether the plate goes by its fighter's side: modes 0xD / 0x13 / 0x11, or a special fight (BATTLE
   +79, GAME +137, BATTLE +159) with the plate's set (+127) at 4. */
static int by_side(gaddr o) {
    int32_t m = S8(GAME + 114);
    if (m == 0xD || m == 0x13 || m == 0x11) return 1;
    if (S8(BATTLE + 79) != 0 || S8(GAME + 137) != 0 || S8(BATTLE + 159) != 0) return S8(o + 127) == 4;
    return 0;
}

/* (plate): a name plate's start: layer 0, +86 8, set up; facing from its fighter's display side (+12
   0xFF when shown flipped, +11 / +13); without +10 bit 1 placed from 0x08A2A136 by side and at the
   height of 0x08A2A13E / 0x08A2A144 (by side in layout 3 or by_side, else 0x100 / 0x28), sliding
   (-0x196900 / 0x19600); with it a motion by mode and the height from 0x08A2A13A. Animation
   0x08A2773C. Then it waits for BATTLE +81 (arrive) or GAME +20 = 3 to go on. */
SFA_FUNC(08898C6C, name_plate_start) {
    gaddr o = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    gaddr s21 = c->r[21], s22 = c->r[22], s23 = c->r[23];
    c->r[29] -= 0x40;
    c->r[16] = o; c->r[18] = BATTLE; c->r[23] = 0;
    if (*B(o + 5) == 0) {
        gaddr fighter = *W(o + 92);
        c->r[17] = fighter;
        *B(o + 5) = 1;
        *W(o + 148) = 0;
        *H(o + 86) = 8;
        c->r[4] = o; c->r[5] = 0; f_0889CFE8(c);
        c->r[4] = fighter; f_088988FC(c);
        uint32_t side = c->r[2];
        c->r[20] = GAME; c->r[22] = 0x08A27730u; c->r[19] = 3; c->r[21] = 0xD;
        *B(o + 12) = side != 0 ? 0xFF : 0;
        *B(o + 11) = (uint8_t)side;
        *B(o + 13) = (uint8_t)side;
        if ((*B(o + 10) & 2) == 0) {
            int32_t h = 0x100, y = 0x28;
            if (S16(BATTLE + 56) == 3) {
                int32_t s = S8(fighter + 337);
                h = S16(0x08A2A13Eu + (uint32_t)s * 2);
                y = S8(0x08A2A144u + (uint32_t)s);
            }
            if (by_side(o)) {
                uint32_t s = plate_side(fighter);
                h = S16(0x08A2A13Eu + s * 2);
                y = S8(0x08A2A144u + s);
            }
            *H(o + 112) = (uint16_t)h;
            *H(o + 114) = (uint16_t)y;
            *W(o + 100) = 0xFFE69700u;
            *W(o + 108) = 0x19600;
            *H(o + 18) = (uint16_t)S16(0x08A2A136u + side * 2);
            *H(o + 22) = (uint16_t)S16(o + 112);
        } else {
            c->r[4] = o; c->r[5] = side + 0x3B; f_0889CEE4(c);
            int32_t y = 0x28;
            if (S16(BATTLE + 56) == 3) y = S8(0x08A2A13Au + (uint32_t)(int32_t)S8(fighter + 337));
            if (by_side(o)) y = S8(0x08A2A13Au + plate_side(fighter));
            *H(o + 22) = (uint16_t)y;
        }
        c->r[4] = o; c->r[5] = *W(0x08A2773Cu); f_0882196C(c);
    } else if (S8(BATTLE + 81) != 0) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        c->r[4] = o; f_08897E2C(c);
    } else if ((S16(GAME + 20) & 0xFF) == 3) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
    c->r[21] = s21; c->r[22] = s22; c->r[23] = s23;
}
