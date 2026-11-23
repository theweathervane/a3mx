/* Written by tools/ending_stages.py: do not edit. The endings' scenes cut from a stage (scaled_endings_stages.c). */
#pragma once
#include <stdint.h>

typedef struct { uint8_t layer, tile; int8_t dx, dy; } StageSource;   /* a stage layer (map at +0x2000 * n), its tile size, and its 16-pixel column / row for the ending's (0, 0) */
typedef struct {
    uint8_t ending, layer, row0, row1, col0, col1;   /* the ending (endNN_u.pac), its layer, the band's rows and art columns */
    uint32_t crc;                                     /* crc32 of the band's map words, its first block of each tile, in order */
    const char *stage;                                /* the stage's archive */
    uint8_t nsources; StageSource sources[4];         /* drawn in this order, the first at the back */
    uint8_t nclut; struct { uint8_t slot, entry; uint16_t offset; } cluts[12];   /* the 4-bit textures' colours */
    uint8_t wide; int8_t shift; uint8_t floor_row;    /* SCENE_VIEW: drawn as it is, not scaled; its view moved right (pixels); from this row down the strips take the stage's floor band */
} EndingStageBand;

static const EndingStageBand ENDING_STAGE_BANDS[] = {
    { 0x00, 1, 4, 12, 28, 51, 0xDE1A8EEDu, "st00.pac", 2, { { 2, 32, 0, 0 }, { 1, 16, 0, 0 } }, 2, { { 0x05, 0, 0x6000 }, { 0x06, 0, 0x6080 } }, 0, 0, 0 },
    { 0x05, 1, 0, 15, 28, 51, 0xDED5E8A1u, "st05.pac", 3, { { 0, 16, 0, 0 }, { 2, 32, 0, 0 }, { 1, 16, 0, 0 } }, 0, { { 0, 0, 0 } }, 0, 0, 0 },
    { 0x07, 1, 0, 15, 16, 63, 0xE62A9B40u, "st07.pac", 2, { { 0, 16, 0, 0 }, { 1, 16, 0, 0 } }, 0, { { 0, 0, 0 } }, 0, 0, 0 },
    { 0x0B, 1, 20, 28, 28, 51, 0x0F5EE152u, "st00.pac", 2, { { 2, 32, 0, -16 }, { 1, 16, 0, -16 } }, 2, { { 0x05, 0, 0x6000 }, { 0x06, 0, 0x6080 } }, 0, 0, 0 },
    { 0x0D, 1, 20, 28, 28, 51, 0x2C9054B6u, "st00.pac", 2, { { 2, 32, 0, -16 }, { 1, 16, 0, -16 } }, 2, { { 0x05, 0, 0x6000 }, { 0x06, 0, 0x6080 } }, 0, 0, 0 },
    { 0x16, 1, 0, 14, 28, 51, 0xEE2FC11Eu, "st16.pac", 3, { { 0, 16, -27, 0 }, { 2, 32, 0, 0 }, { 1, 16, 0, 0 } }, 4, { { 0x07, 0, 0x6000 }, { 0x08, 0, 0x60C0 }, { 0x09, 0, 0x6180 }, { 0x0A, 0, 0x6240 } }, 0, 0, 0 },
    { 0x18, 1, 0, 15, 17, 51, 0xC8A3E5D9u, "st18.pac", 4, { { 0, 32, 0, 0 }, { 2, 32, 0, 0 }, { 2, 16, 0, 0 }, { 1, 16, 0, 0 } }, 9, { { 0x03, 0, 0x6600 }, { 0x04, 0, 0x61C0 }, { 0x05, 0, 0x61E0 }, { 0x06, 0, 0x6200 }, { 0x07, 0, 0x6220 }, { 0x08, 0, 0x6680 }, { 0x09, 0, 0x66E0 }, { 0x0A, 0, 0x6740 }, { 0x0B, 0, 0x67A0 } }, 1, 16, 13 },
    { 0x27, 1, 0, 15, 16, 63, 0x561BF637u, "st07.pac", 2, { { 0, 16, 0, 0 }, { 1, 16, 0, 0 } }, 0, { { 0, 0, 0 } }, 0, 0, 0 },
    { 0x28, 1, 0, 15, 28, 51, 0x54B0F1C6u, "st21.pac", 2, { { 2, 32, 0, 0 }, { 1, 16, 0, 0 } }, 0, { { 0, 0, 0 } }, 0, 0, 0 },
};
