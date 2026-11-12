/* The boot screen: the save messages hidden (0.60.0: the version, which used to be drawn where they
 * were, is in the top right corner on every boot screen: src/platform/bootscreen.c).
 *
 * Before the MODE MENU the game auto-loads the system data on a screen of its own (0x0888BFBC,
 * decompiled in src/core/task/title.c, which runs bootmsg_load_screen around it: SFA_FUNC_WRAPPED;
 * a0 the game state, +2 its step):
 *   1  "Auto-loading SYSTEM DATA... Please do not turn off the PSP..." set up
 *   3  60 frames, then the load (0x08816BBC)      4  "Auto-load completed." (sysdata.c cuts it short)
 *   6  the fade out, the message still drawn
 *   (2 waits for a button and 5 asks a question when the load failed: both are shown as they are)
 *
 * The messages are pictures, not text: each is a compressed bitmap of its lines (memory_u.pac's
 * data 0x2000000), shown by two objects of kind 0x88 that step 1 makes. On the message steps those
 * objects are switched off, and the project's version (A3MX_VERSION_SCREEN: "V0.46.1", or a
 * nightly's "V0.46.1-NIGHTLY.20261106") is drawn where the message was: centred, white, in the
 * game's small font (font 0, as the copyright under it and EX. SETTING's version).
 *
 * The drawing is done by bootmsg_objects, around the game's first object list (0x08837690,
 * src/core/object/lists.c, wrapped), which runs every frame: during the load (step 3) the screen
 * itself isn't called for about 12 frames at a time, but the frames are still drawn. It does
 * nothing anywhere else: only while the boot screen is on a message step, and only if that screen
 * ran in the last second.
 *
 * Until 0.46.1 the message steps were held off screen instead (rt_hold_display), so the screen took
 * no time; now it shows for about a second, with the version. */
#include "rt.h"
#include "version.h"
#include <string.h>
#include <stdio.h>

#define MSG_OBJS   0x09513EB0u              /* objects (08822EAC), 32 x 0xC0, downwards: +0 on, +2 kind */
#define MSG_OBJ_SZ 0xC0u
#define MSG_KIND   0x88u                    /* the boot screen's message pictures (step 1 makes two) */

static int in_message;                      /* the boot screen is on a message step */
static uint64_t seen_at;                    /* when the boot screen last ran (rt_vblank_count) */
uint64_t rt_vblank_count(void);

static int message_only(uint32_t step) { return step == 1 || step == 3 || step == 4 || step == 6; }

static void messages_off(void) {
    for (uint32_t k = 0; k < 32; k++) {
        uint32_t o = MSG_OBJS - MSG_OBJ_SZ * k;
        if (RB(o) && RB(o + 2) == MSG_KIND) WB(o, 0);
    }
}

int bootmsg_load_screen(Cpu *c, uint32_t g, int after) {
    (void)c; (void)after;
    in_message = message_only(RH(g + 2));     /* before and after: step 6 ends on step 0 */
    seen_at = rt_vblank_count();
    if (in_message) messages_off();           /* (after step 1: the objects it just made) */
    return 0;
}

int bootmsg_objects(Cpu *c, uint32_t a0, int after) {
    (void)a0;
    if (!in_message || rt_vblank_count() - seen_at > 60) return 0;
    if (!after) { messages_off(); return 0; }
    return 0;                                 /* (the version is drawn in the top right corner: src/platform/bootscreen.c) */
}
