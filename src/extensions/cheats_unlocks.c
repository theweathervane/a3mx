/* Cheats and Unlocks (the cheats_unlocks feature): EX. SETTING's first four pages (exsetting.c) and what
 * they do, once a frame (sysdata_frame). The fighters' cheats after the Dreamcast recomp's (runtime/
 * cheats.c), made the same for each fighter slot.
 *
 * Page 1/5, the unlocks, each ON (forced on) or OFF (as your save earned it). What the system data
 * keeps (0x08A058A0: +0x50 bits 0 - 2, +0x52 the clears, +0x200 the ISM-Pluses):
 *   +0x52 bit 0x0001  Arcade cleared (0x089DE194): SELECT held when choosing Akuma gives Shin Akuma, the
 *                     second Balrog the EX one (0x25 -> 0x15), M. Bison Final M. Bison (cursor.c,
 *                     battle_select.c), and L + R on FINAL BATTLE fights Shin Akuma (mode_menu.c). One
 *                     bit for four rows: SHIN AKUMA, FINAL M.BISON, EX BALROG and SHIN AKUMA BATTLE. When
 *                     a row is OFF that the save hasn't earned but another row forces the bit, the
 *                     game's code asks rt_unlock_blocked (cursor.c, battle_select.c, mode_menu.c).
 *   +0x52 bit 0x4000  VS 100 KUMITE with 100 wins (0x0882AE20): L + R on DRAMATIC, VARIABLE and
 *                     R.DRAMATIC BATTLE gives their other versions. Row BATTLE VARIANTS.
 *   the other bits    (0x0882AE20) Variable 0x2, Dramatic 0x4, R.Dramatic 0x8, Survival's kinds 1 - 6
 *                     0x10 / 0x20 / 0x40 / 0x100 / 0x80 / 0x200, World Tour 0x400 / 0x800, Kumite's 81
 *                     and 91 wins 0x1000 / 0x2000, Final Battle against Shin Akuma 0x8000; and +0x50 bits
 *                     0 - 2 (the variants of Dramatic, Variable and R.Dramatic cleared). EDIT MODE's points
 *                     are added up from them (0x088D43BC: 0x08A3E8B8's 30 - 250 a bit, 9999 with all
 *                     three of +0x50). Row EDIT MODE POINTS.
 *   +0x200            the 26 ISM-Pluses (0x0883B800 adds an entry's). Row ISM-PLUS.
 * Forced bits are never saved: what the game earns is followed (the three functions that set them are
 * wrapped: the forced bits are taken off while they run), and the options piece is put back to the
 * earned bits wherever it is saved (savestore.c, hle_sys.c). So turning a row OFF goes back to the save.
 *
 * Pages 2/5 and 3/5, for each fighter slot (1P the first fighter, 2P the second, 3P the third, a
 * player's or the computer's, as 1P - 3P PRIORITY count them; 4P is shown, greyed out: there is none):
 *   HEALTH  NORMAL, INFINITE (back to full each frame), HALF (never above half the round's full health),
 *           NONE (never above 1: the next hit knocks out, by the game's own damage)
 *   GUARD   NORMAL, INFINITE (no guard damage), HALF (at least half the gauge's damage), NONE (one short
 *           of a crush: the next guarded hit crushes), OFF (0.166.0: no guard gauge at all, as before
 *           Alpha 3: no guard damage, so never a crush, as INFINITE; and the fighter's guard gauge and
 *           its GUARD label aren't drawn, 1P's and 2P's sides of the HUD: hud_guard_off, object/hud.c)
 *   SUPER   NORMAL, INFINITE (full), LV1 / LV2 / LV3 (held at that level: 48 / 96 / 144), NONE (empty)
 * They act while a round is on (GAME + 2 = 2, + 6 = 3) in every mode but the network battles (mode 1,
 * and NETWORK's own, 12), and never on a knocked-out fighter (health below 0).
 *
 * Page 4/5, the entry each side plays as or edits (0x09358528 + 0x354 n): in World Tour (mode 3, while
 * +0 is 1) WORLD TOUR EXP MAX holds the total EXP (+8) at 9,999,999, the level (+4, level - 1) at 32
 * and the five STATUS values (+0x26 - +0x2E: ATTACK, SC ATTACK, DEFENSE, GAUGE, STAMINA, s16) at their
 * limits (+0x30 - +0x38); X-, A- and V-ISM EXP MAX hold that ISM's EXP (+0xC / +0x10 / +0x14) at
 * 9,999,999 and its level (+5 / +6 / +7) at 32 (MASTER). In EDIT MODE (mode 14) the EDIT MODE rows do
 * the same for the character being made, but not its STATUS, which EDIT MODE sets with its points.
 * Kept by saving the World Tour data, or the edited character.
 *
 * Fighters (0x092E3EA0 + 0x498 k): +0 in play, +0x70 health (s16), +0x72 the round's full health,
 * +0x16E the super gauge (0 - 144), +0x22C the guard gauge's size (60, less after each crush), +0x22D
 * its damage (a crush when it reaches the size): the Dreamcast's offsets + 0x10 (health) and + 0x20.
 *
 * The settings are kept in a3mx.cfg's [cheats and unlocks] (settings.c).
 */
#include "rt.h"
#include "ext_features.h"
#include "unlocks.h"
#include <stdio.h>
#include <string.h>

/* EX. DISPLAY > HEALTH GRADIENT OFF (rt_health_gradient, settings.c): the health bars' fills in one solid
   colour (object/hud.c). The one switch for it: any other change to the health bars' colours checks this
   first, and OFF wins. */
extern int rt_health_gradient;
int hud_health_solid(void) { return FEATURE(MENUS) && !rt_health_gradient; }

int rt_cheat_health[3], rt_cheat_guard[3], rt_cheat_super[3];
int rt_cheat_wt[4];                         /* WORLD TOUR EXP, X-ISM, A-ISM, V-ISM: NORMAL (0), MAX (1) */
int rt_cheat_edit[4];                       /* EDIT MODE EXP, X-ISM, A-ISM, V-ISM */
int rt_unlock[UNLOCK_COUNT] = { 1, 1, 1, 1, 1, 1, 1 };
int rt_unlock_blocked;                      /* bits by UNLOCK_*: the game's code doesn't allow these (see above) */

const char *const cheat_health_names[] = { "NORMAL", "INFINITE", "HALF", "NONE", NULL };
const char *const cheat_guard_names[] = { "NORMAL", "INFINITE", "HALF", "NONE", "OFF", NULL };
const char *const cheat_super_names[] = { "NORMAL", "INFINITE", "LV1", "LV2", "LV3", "NONE", NULL };
const char *const cheat_wt_names[] = { "NORMAL", "MAX", NULL };
enum { HP_NORMAL, HP_INFINITE, HP_HALF, HP_NONE, GU_OFF };   /* (GU_OFF: GUARD only) */
enum { SU_NORMAL, SU_INFINITE, SU_LV1, SU_LV2, SU_LV3, SU_NONE };

void cheats_unlocks_default_unlocks(void) { for (int i = 0; i < UNLOCK_COUNT; i++) rt_unlock[i] = 1; }
void cheats_unlocks_default_fighters(int from, int to) {
    for (int k = from; k <= to && k < 3; k++) rt_cheat_health[k] = rt_cheat_guard[k] = rt_cheat_super[k] = 0;
}
void cheats_unlocks_default_entries(void) { for (int i = 0; i < 4; i++) rt_cheat_wt[i] = rt_cheat_edit[i] = 0; }

#define GAME          0x08E75910u
#define G_MODE        0x72
#define MODE_NETWORK_BATTLE 1
#define MODE_WORLD_TOUR     3
#define MODE_NETWORK        12
#define MODE_EDIT           14
#define FIGHTER(k)    (0x092E3EA0u + 0x498u * (uint32_t)(k))
#define F_HEALTH      0x70
#define F_FULL        0x72
#define F_SUPER       0x16E
#define F_GUARD_SIZE  0x22C
#define F_GUARD_DAMAGE 0x22D
#define SUPER_LEVEL   48
#define SUPER_FULL    144
#define ENTRIES       0x09358528u
#define ENTRY_SIZE    0x354u
#define E_EXP         8
#define E_ISM_EXP     0xC
#define EXP_MOST      9999999u
#define E_LEVEL       4         /* level - 1: the total, then X-, A-, V-ISM's */
#define E_STATUS      0x26      /* five s16: ATTACK, SC ATTACK, DEFENSE, GAUGE, STAMINA */
#define E_STATUS_MAX  0x30      /* the same five's limits */
#define LEVEL_MOST    31        /* Lv 32: MASTER */

/* ---------------------------------------------------------------- the unlocks */
#define SYS_OPTS      0x08A058A0u
#define O_VARIANTS    0x50      /* bits 0 - 2 */
#define O_CLEARS      0x52
#define O_ISM_PLUS    0x200
#define ARCADE_BIT    0x0001u
#define KUMITE_BIT    0x4000u
#define EDIT_BITS     0xBFFEu   /* the rest of +0x52 */
#define VARIANT_BITS  0x07u
#define ALL_ISM_PLUS  0x0FFFFFFFu   /* bits 0 - 27 (the mode rewards use up to 27) */

typedef struct { uint32_t v50, v52, ism; } Bits;
static Bits earned, forced, written;
static int have_earned, forcing;

static Bits now_bits(void) { Bits b = { RB(SYS_OPTS + O_VARIANTS) & VARIANT_BITS, RH(SYS_OPTS + O_CLEARS), RW(SYS_OPTS + O_ISM_PLUS) }; return b; }
static void put_bits(Bits b) {
    WB(SYS_OPTS + O_VARIANTS, (RB(SYS_OPTS + O_VARIANTS) & ~VARIANT_BITS) | (b.v50 & VARIANT_BITS));
    WH(SYS_OPTS + O_CLEARS, (uint16_t)b.v52);
    WW(SYS_OPTS + O_ISM_PLUS, b.ism);
}
static Bits wanted(void) {
    Bits b = { 0, 0, 0 };
    if (rt_unlock[UNLOCK_SHIN_AKUMA] || rt_unlock[UNLOCK_FINAL_BISON] || rt_unlock[UNLOCK_EX_BALROG] || rt_unlock[UNLOCK_SHIN_BATTLE]) b.v52 |= ARCADE_BIT;
    if (rt_unlock[UNLOCK_VARIANTS]) b.v52 |= KUMITE_BIT;
    if (rt_unlock[UNLOCK_EDIT_POINTS]) { b.v52 |= EDIT_BITS; b.v50 = VARIANT_BITS; }
    if (rt_unlock[UNLOCK_ISM_PLUS]) b.ism = ALL_ISM_PLUS;
    return b;
}
static Bits merged(void) { Bits b = { earned.v50 | forced.v50, earned.v52 | forced.v52, earned.ism | forced.ism }; return b; }
/* bits set outside the wrapped functions since our last write: earned */
static void follow(void) {
    if (!forcing) return;
    Bits m = now_bits();
    earned.v50 |= m.v50 & ~written.v50; earned.v52 |= m.v52 & ~written.v52; earned.ism |= m.ism & ~written.ism;
    earned.v50 &= m.v50 | ~written.v50; earned.v52 &= m.v52 | ~written.v52; earned.ism &= m.ism | ~written.ism;   /* (cleared by the game: gone) */
}
static void strip(void) {                   /* memory back to the earned bits (keeping any the game set since) */
    if (!forcing) return;
    follow();
    Bits m = now_bits(), e = { m.v50 & ~(forced.v50 & ~earned.v50), m.v52 & ~(forced.v52 & ~earned.v52), m.ism & ~(forced.ism & ~earned.ism) };
    put_bits(e);
    forcing = 0;
}
static void force(void) {
    forced = wanted();
    written = merged();
    put_bits(written);
    forcing = 1;
    int arcade_earned = (earned.v52 & ARCADE_BIT) != 0;
    rt_unlock_blocked = 0;
    if ((forced.v52 & ARCADE_BIT) && !arcade_earned)
        for (int u = UNLOCK_SHIN_AKUMA; u <= UNLOCK_SHIN_BATTLE; u++) if (!rt_unlock[u]) rt_unlock_blocked |= 1 << u;
}
static void unlocks_frame(int ready) {
    if (!ready) {                           /* a load under way (sysdata.c): take ours off, follow the load */
        strip();
        earned = now_bits(); have_earned = 1; rt_unlock_blocked = 0;
        return;
    }
    if (!have_earned) { earned = now_bits(); have_earned = 1; }
    follow();
    force();
}
/* The functions that earn them: run with the forced bits off, so what they set is seen */
static void around(Cpu *c, void (*f)(Cpu *)) {
    int was = forcing;
    strip();
    f(c);
    if (was) { earned = now_bits(); force(); }
}
void hook_f_0882AE20(Cpu *c) { around(c, orig_f_0882AE20); }      /* a mode cleared: +0x52, +0x50 */
void hook_f_0883B800(Cpu *c) { around(c, orig_f_0883B800); }      /* an entry's ISM-Pluses */
int cheats_unlocks_next_round(Cpu *c, uint32_t a0, int after) {     /* 0x089DE194: Arcade cleared (bit 0x0001) */
    (void)c; (void)a0;
    static int was;
    if (!after) { was = forcing; strip(); return 0; }
    if (was) { earned = now_bits(); force(); }
    return 0;
}
/* An options piece about to be saved (host memory, as 0x08A058A0): the earned bits, not ours */
void cheats_unlocks_unforce(uint8_t *o) {
    if (!forcing) return;
    follow();
    o[O_VARIANTS] = (uint8_t)(o[O_VARIANTS] & ~(forced.v50 & ~earned.v50));
    uint16_t v = (uint16_t)(o[O_CLEARS] | o[O_CLEARS + 1] << 8);
    v = (uint16_t)(v & ~(forced.v52 & ~earned.v52));
    o[O_CLEARS] = (uint8_t)v; o[O_CLEARS + 1] = (uint8_t)(v >> 8);
    uint32_t w = (uint32_t)o[O_ISM_PLUS] | (uint32_t)o[O_ISM_PLUS + 1] << 8 | (uint32_t)o[O_ISM_PLUS + 2] << 16 | (uint32_t)o[O_ISM_PLUS + 3] << 24;
    w &= ~(forced.ism & ~earned.ism);
    for (int i = 0; i < 4; i++) o[O_ISM_PLUS + i] = (uint8_t)(w >> (8 * i));
}

/* ---------------------------------------------------------------- the fighters */
static void fighter(uint32_t f, int k) {
    int16_t full = (int16_t)RH(f + F_FULL), hp = (int16_t)RH(f + F_HEALTH);
    if (hp < 0) return;                                     /* knocked out: left alone */
    switch (rt_cheat_health[k]) {
    case HP_INFINITE: if (hp < full) WH(f + F_HEALTH, (uint16_t)full); break;
    case HP_HALF: if (hp > full / 2) WH(f + F_HEALTH, (uint16_t)(full / 2)); break;
    case HP_NONE: if (hp > 1) WH(f + F_HEALTH, 1); break;
    }
    int size = RB(f + F_GUARD_SIZE), dmg = RB(f + F_GUARD_DAMAGE);
    switch (rt_cheat_guard[k]) {
    case HP_INFINITE: case GU_OFF: if (dmg) WB(f + F_GUARD_DAMAGE, 0); break;   /* (OFF: never crushes) */
    case HP_HALF: if (size > 1 && dmg < size / 2) WB(f + F_GUARD_DAMAGE, (uint8_t)(size / 2)); break;
    case HP_NONE: if (size > 1 && dmg < size - 1) WB(f + F_GUARD_DAMAGE, (uint8_t)(size - 1)); break;
    }
    int want = -1;
    switch (rt_cheat_super[k]) {
    case SU_INFINITE: want = SUPER_FULL; break;
    case SU_LV1: case SU_LV2: case SU_LV3: want = SUPER_LEVEL * (rt_cheat_super[k] - SU_LV1 + 1); break;
    case SU_NONE: want = 0; break;
    }
    if (want >= 0 && (int16_t)RH(f + F_SUPER) != want) WH(f + F_SUPER, (uint16_t)want);
}

/* ---------------------------------------------------------------- World Tour and EDIT MODE */
static void entry_max(uint32_t e, const int rows[4], int status) {
    if (rows[0]) {
        if (RW(e + E_EXP) != EXP_MOST) WW(e + E_EXP, EXP_MOST);
        if (RB(e + E_LEVEL) != LEVEL_MOST) WB(e + E_LEVEL, LEVEL_MOST);
        if (status)
            for (uint32_t s = 0; s < 5; s++)
                if ((int16_t)RH(e + E_STATUS_MAX + 2 * s) > 0 && RH(e + E_STATUS + 2 * s) != RH(e + E_STATUS_MAX + 2 * s))
                    WH(e + E_STATUS + 2 * s, RH(e + E_STATUS_MAX + 2 * s));
    }
    for (uint32_t i = 0; i < 3; i++) if (rows[1 + i]) {
        if (RW(e + E_ISM_EXP + 4 * i) != EXP_MOST) WW(e + E_ISM_EXP + 4 * i, EXP_MOST);
        if (RB(e + E_LEVEL + 1 + i) != LEVEL_MOST) WB(e + E_LEVEL + 1 + i, LEVEL_MOST);
    }
}

/* object/hud.c: whether side SIDE's guard gauge is off (GUARD OFF on that fighter slot, where the cheats
   act: not in the network battles) */
int hud_guard_off(int side) {
    if (side < 0 || side > 2 || !FEATURE(CHEATS_UNLOCKS) || rt_cheat_guard[side] != GU_OFF) return 0;
    int mode = (int8_t)RB(GAME + G_MODE);
    return mode != MODE_NETWORK_BATTLE && mode != MODE_NETWORK;
}
void cheats_unlocks_frame(int ready) {
    unlocks_frame(ready);
    int mode = (int8_t)RB(GAME + G_MODE);
    if (mode == MODE_NETWORK_BATTLE || mode == MODE_NETWORK) return;
    if ((int16_t)RH(GAME + 2) == 2 && (int16_t)RH(GAME + 6) == 3)
        for (int k = 0; k < 3; k++) if (RB(FIGHTER(k)) == 1) fighter(FIGHTER(k), k);
    for (uint32_t n = 0; n < 2; n++) {
        uint32_t e = ENTRIES + ENTRY_SIZE * n;
        if (mode == MODE_WORLD_TOUR && RB(e) == 1) entry_max(e, rt_cheat_wt, 1);
        if (mode == MODE_EDIT) entry_max(e, rt_cheat_edit, 0);
    }
}
