/* Cheats and Unlocks (the cheats_unlocks feature): EX. SETTING's first two pages (exsetting.c) and what
 * they do, once a frame (sysdata_frame). After the Dreamcast recomp's cheats (runtime/cheats.c), made the
 * same for each of the three fighters.
 *
 * Page 1/3, for each fighter slot (1P the first fighter, 2P the second, 3P the third, a player's or the
 * computer's, as 1P - 3P PRIORITY count them):
 *   HEALTH  NORMAL, INFINITE (back to full each frame), HALF (never above half the round's full health),
 *           NONE (never above 1: the next hit knocks out, by the game's own damage)
 *   GUARD   NORMAL, INFINITE (no guard damage), HALF (at least half the gauge's damage), NONE (one short
 *           of a crush: the next guarded hit crushes)
 *   SUPER   NORMAL, INFINITE (full), LV1 / LV2 / LV3 (held at that level: 48 / 96 / 144), NONE (empty)
 * They act while a round is on (GAME + 2 = 2, + 6 = 3) in every mode but the network battles (mode 1,
 * and NETWORK's own, 12), and never on a knocked-out fighter (health below 0).
 *
 * Page 2/3, in World Tour (mode 3), for the entry each side plays as (0x09358528 + 0x354 n while +0 is
 * 1): WORLD TOUR EXP MAX holds the total EXP (+8) at 9,999,999, the level (+4, level - 1) at 32 and the
 * five STATUS values (+0x26 - +0x2E: ATTACK, SC ATTACK, DEFENSE, GAUGE, STAMINA, s16) at their limits
 * (+0x30 - +0x38: 120, 120, 120, 120, 140 for Ryu); X-, A- and V-ISM EXP MAX hold that ISM's EXP (+0xC /
 * +0x10 / +0x14) at 9,999,999 and its level (+5 / +6 / +7) at 32 (MASTER). The game raised levels only
 * one at a time after a win, so EXP alone left them at 1. Kept by saving the World Tour data. And UNLOCK
 * ALL, the unlocks (sysdata.c, on by default as the unlocks feature was before 0.127.0).
 *
 * Fighters (0x092E3EA0 + 0x498 k): +0 in play, +0x70 health (s16), +0x72 the round's full health,
 * +0x16E the super gauge (0 - 144), +0x22C the guard gauge's size (60, less after each crush), +0x22D
 * its damage (a crush when it reaches the size). Measured on the PSP program: the Dreamcast's offsets
 * + 0x10 (health) and + 0x20 (the rest).
 *
 * The settings are kept in a3mx.cfg's [cheats and unlocks] (settings.c).
 */
#include "rt.h"
#include <stdio.h>
#include <string.h>

int rt_cheat_health[3], rt_cheat_guard[3], rt_cheat_super[3];
int rt_cheat_wt[4];                         /* WORLD TOUR EXP, X-ISM, A-ISM, V-ISM: NORMAL (0), MAX (1) */
int rt_unlock_all = 1;

const char *const cheat_health_names[] = { "NORMAL", "INFINITE", "HALF", "NONE", NULL };
const char *const cheat_guard_names[] = { "NORMAL", "INFINITE", "HALF", "NONE", NULL };
const char *const cheat_super_names[] = { "NORMAL", "INFINITE", "LV1", "LV2", "LV3", "NONE", NULL };
const char *const cheat_wt_names[] = { "NORMAL", "MAX", NULL };
enum { HP_NORMAL, HP_INFINITE, HP_HALF, HP_NONE };
enum { SU_NORMAL, SU_INFINITE, SU_LV1, SU_LV2, SU_LV3, SU_NONE };

void cheats_unlocks_default_fighters(void) {
    for (int k = 0; k < 3; k++) rt_cheat_health[k] = rt_cheat_guard[k] = rt_cheat_super[k] = 0;
}
void cheats_unlocks_default_world_tour(void) {
    for (int i = 0; i < 4; i++) rt_cheat_wt[i] = 0;
    rt_unlock_all = 1;
}

#define GAME          0x08E75910u
#define G_MODE        0x72
#define MODE_NETWORK_BATTLE 1
#define MODE_WORLD_TOUR     3
#define MODE_NETWORK        12
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
    case HP_INFINITE: if (dmg) WB(f + F_GUARD_DAMAGE, 0); break;
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

void cheats_unlocks_frame(void) {
    int mode = (int8_t)RB(GAME + G_MODE);
    if (mode == MODE_NETWORK_BATTLE || mode == MODE_NETWORK) return;
    if ((int16_t)RH(GAME + 2) == 2 && (int16_t)RH(GAME + 6) == 3)
        for (int k = 0; k < 3; k++) if (RB(FIGHTER(k)) == 1) fighter(FIGHTER(k), k);
    if (mode == MODE_WORLD_TOUR)
        for (uint32_t n = 0; n < 2; n++) {
            uint32_t e = ENTRIES + ENTRY_SIZE * n;
            if (RB(e) != 1) continue;
            if (rt_cheat_wt[0]) {
                if (RW(e + E_EXP) != EXP_MOST) WW(e + E_EXP, EXP_MOST);
                if (RB(e + E_LEVEL) != LEVEL_MOST) WB(e + E_LEVEL, LEVEL_MOST);
                for (uint32_t s = 0; s < 5; s++)
                    if ((int16_t)RH(e + E_STATUS_MAX + 2 * s) > 0 && RH(e + E_STATUS + 2 * s) != RH(e + E_STATUS_MAX + 2 * s))
                        WH(e + E_STATUS + 2 * s, RH(e + E_STATUS_MAX + 2 * s));
            }
            for (uint32_t i = 0; i < 3; i++) if (rt_cheat_wt[1 + i]) {
                if (RW(e + E_ISM_EXP + 4 * i) != EXP_MOST) WW(e + E_ISM_EXP + 4 * i, EXP_MOST);
                if (RB(e + E_LEVEL + 1 + i) != LEVEL_MOST) WB(e + E_LEVEL + 1 + i, LEVEL_MOST);
            }
        }
}
