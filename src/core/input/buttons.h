/* PSP controller buttons, as the game and the OS report them (SceCtrlData.buttons). */
#pragma once
#include <stdint.h>

enum {
    BTN_SELECT   = 0x000001,
    BTN_START    = 0x000008,
    BTN_UP       = 0x000010,
    BTN_RIGHT    = 0x000020,
    BTN_DOWN     = 0x000040,
    BTN_LEFT     = 0x000080,
    BTN_L        = 0x000100,
    BTN_R        = 0x000200,
    BTN_TRIANGLE = 0x001000,
    BTN_CIRCLE   = 0x002000,
    BTN_CROSS    = 0x004000,
    BTN_SQUARE   = 0x008000,
    BTN_HOME     = 0x010000,
    BTN_HOLD     = 0x020000,   /* the HOLD switch */
};

/* One controller sample (sceCtrlReadBufferPositive). */
typedef struct {
    uint32_t timestamp;
    uint32_t buttons;
    uint8_t  lx, ly;           /* analogue stick, 0-255, 128 centre */
    uint8_t  reserved[6];
} SceCtrlData;
