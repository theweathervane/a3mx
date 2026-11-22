/* Cheats and Unlocks' unlock rows (extensions/cheats_unlocks.c; EX. SETTING page 1).
 *
 * One bit of the system data (Arcade cleared) unlocks four things; with some of their rows OFF and the
 * bit there only because another row forces it, rt_unlock_blocked has a bit (1 << UNLOCK_*) for each one
 * the decompiled code must not allow: cursor.c and battle_select.c (SELECT's Shin Akuma, Final M. Bison,
 * EX Balrog), mode_menu.c (FINAL BATTLE's L + R). 0 without the extensions (extensions_off.c). */
#pragma once

enum { UNLOCK_SHIN_AKUMA, UNLOCK_FINAL_BISON, UNLOCK_EX_BALROG, UNLOCK_SHIN_BATTLE, UNLOCK_VARIANTS,
       UNLOCK_EDIT_POINTS, UNLOCK_ISM_PLUS, UNLOCK_COUNT };

extern int rt_unlock_blocked;
#define UNLOCK_BLOCKED(u) ((rt_unlock_blocked >> (u)) & 1)
