/* Written by tools/ending_stages.py: do not edit. The endings' scenes cut from a stage (scaled_endings_stages.c). */
#pragma once
#include <stdint.h>

typedef struct { uint8_t layer, tile; } StageSource;          /* a stage layer: its number (map at +0x2000 * n) and tile size */
typedef struct {
    uint8_t ending, layer, row0, row1, col0, col1;   /* the ending (endNN_u.pac), its layer, the band's rows and art columns */
    uint32_t crc;                                     /* crc32 of the band's map words, its first block of each tile, in order */
    const char *stage;                                /* the stage's archive */
    int8_t dx, dy;                                    /* the stage map's 16-pixel column / row for the ending's (0, 0) */
    uint8_t nsources; StageSource sources[3];         /* drawn in this order, the first at the back */
    uint8_t nclut; struct { uint8_t slot, entry; uint16_t offset; } cluts[4];   /* the 4-bit textures' colours */
} EndingStageBand;

static const EndingStageBand ENDING_STAGE_BANDS[] = {
    { 0x00, 1, 4, 12, 28, 51, 0xDE1A8EEDu, "st00.pac", 0, 0, 2, { { 2, 32 }, { 1, 16 } }, 2, { { 0x05, 0, 0x6000 }, { 0x06, 0, 0x6080 } } },
    { 0x05, 1, 0, 15, 28, 51, 0xDED5E8A1u, "st05.pac", 0, 0, 3, { { 0, 16 }, { 2, 32 }, { 1, 16 } }, 0, { { 0, 0, 0 } } },
    { 0x07, 1, 0, 15, 16, 63, 0xE62A9B40u, "st07.pac", 0, 0, 2, { { 0, 16 }, { 1, 16 } }, 0, { { 0, 0, 0 } } },
    { 0x0B, 1, 20, 28, 28, 51, 0x0F5EE152u, "st00.pac", 0, -16, 2, { { 2, 32 }, { 1, 16 } }, 2, { { 0x05, 0, 0x6000 }, { 0x06, 0, 0x6080 } } },
    { 0x0D, 1, 20, 28, 28, 51, 0x2C9054B6u, "st00.pac", 0, -16, 2, { { 2, 32 }, { 1, 16 } }, 2, { { 0x05, 0, 0x6000 }, { 0x06, 0, 0x6080 } } },
    { 0x27, 1, 0, 15, 16, 63, 0x561BF637u, "st07.pac", 0, 0, 2, { { 0, 16 }, { 1, 16 } }, 0, { { 0, 0, 0 } } },
    { 0x28, 1, 0, 15, 28, 51, 0x54B0F1C6u, "st21.pac", 0, 0, 3, { { 0, 16 }, { 2, 32 }, { 1, 16 } }, 0, { { 0, 0, 0 } } },
};
