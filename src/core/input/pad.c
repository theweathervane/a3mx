/* Reading the controller, once a frame.
 *
 * The PSP has one pad. The game reads it here, keeps a raw copy, and folds the analogue stick into the
 * d-pad: past ±64 from centre counts as a direction, unless the opposite direction is already held,
 * and not at all while HOME or the HOLD switch is on. Any direction added that way also tells the power
 * manager the player is active. The result then goes through 0x088E3660 (not decompiled yet) and is
 * what the rest of the game reads as this frame's buttons.
 *
 * In this port the extra players' pads are fed in later, per player slot (src/extensions/hooks.c on
 * 0x0880509C), so this function stays the PSP's single-pad read.
 */
#include "../decomp.h"
#include "../psp_os.h"
#include "buttons.h"

GUEST_STRUCT_SIZE(SceCtrlData, 16);

#define PAD_SAMPLE     0x08AD8FD0u   /* SceCtrlData read this frame */
#define PAD_BUTTONS    0x08AD8FE0u   /* the buttons the game uses: d-pad with the stick folded in */
#define PAD_RAW_COPY   0x08E75900u   /* the buttons as read, before the stick is folded in */
#define STICK_DEADZONE 64

void f_088E3660(Cpu *c);             /* buttons -> buttons (not decompiled yet) */

static uint32_t filter_buttons(Cpu *c, uint32_t buttons) {
    c->r[4] = buttons;
    f_088E3660(c);
    return c->r[2];
}

static uint32_t read_pad(Cpu *c) {
    os_ctrl_read_buffer_positive(c, PAD_SAMPLE, 1);
    const SceCtrlData *sample = guest_ptr(PAD_SAMPLE);
    uint32_t *buttons = guest_ptr(PAD_BUTTONS);
    uint32_t raw = sample->buttons;
    *buttons = raw;
    *(uint32_t *)guest_ptr(PAD_RAW_COPY) = raw;

    int up = 0, down = 0, left = 0, right = 0;
    if (!(raw & (BTN_HOME | BTN_HOLD))) {
        int x = sample->lx - 128, y = sample->ly - 128;
        up = y < -STICK_DEADZONE;
        down = y > STICK_DEADZONE;
        left = x < -STICK_DEADZONE;
        right = x > STICK_DEADZONE;
    }
    int moved = 0;
    if (up && !(*buttons & BTN_DOWN))     { *buttons |= BTN_UP;    moved = 1; }
    if (down && !(*buttons & BTN_UP))     { *buttons |= BTN_DOWN;  moved = 1; }
    if (left && !(*buttons & BTN_RIGHT))  { *buttons |= BTN_LEFT;  moved = 1; }
    if (right && !(*buttons & BTN_LEFT))  { *buttons |= BTN_RIGHT; moved = 1; }
    if (moved) os_power_tick(c, 0);

    *buttons = filter_buttons(c, *buttons);
    return *buttons;
}

/* u32 pad_read(void): once a frame */
SFA_FUNC(088051BC, game_pad_read) { RETURN(read_pad(c)); }
