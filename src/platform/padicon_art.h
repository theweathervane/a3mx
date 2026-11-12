/* The player bubbles drawn on the MODE MENU's bar (src/extensions/padicons.c): a disc in the player's
 * colour with its number, as the game's own 1P / 2P / 3P boxes are coloured (4 is ours). The launcher's
 * Controllers tab draws the same pixels (Pad icon, and Test), so what it shows is what the game shows.
 *
 * Shared by the game (C) and the launcher (C++): plain data and one function, no SDL, no game memory. */
#pragma once
#include <stdint.h>
#include <string.h>

#define PADICON_SIZE 16                        /* a bubble, with its outline and shadow: 16 x 16 */
#define PADICON_SET  (1u << 24)                /* a pixel that is drawn (black is PADICON_SET alone) */
#define PADICON_LIGHT 0xF7F7E7u                /* the outline round it, like the boxes' frame */

/* the numbers, 6 x 5, in the chunky style of the game's 1P / 2P / 3P */
static const char *const padicon_digits[4][5] = {
    { ".###..", "..##..", "..##..", "..##..", ".####." },
    { "######", "....##", "######", "##....", "######" },
    { "######", "....##", ".#####", "....##", "######" },
    { "##..##", "##..##", "######", "....##", "....##" },
};
/* per player: number outline, bubble, bubble rim, number (RGB) */
static const uint32_t padicon_colours[4][4] = {
    { 0x520000, 0x842110, 0xAD7C29, 0xF7F7E7 },   /* 1 red, as the game's 1P box */
    { 0x002163, 0x104284, 0x1F7384, 0xE7F7F7 },   /* 2 blue */
    { 0x082108, 0x214219, 0x5B7319, 0xF7F7E7 },   /* 3 green */
    { 0x4A3100, 0x9C7B10, 0xD6B529, 0xF7F7E7 },   /* 4 yellow (ours) */
};

static int padicon_near(const uint32_t p[PADICON_SIZE][PADICON_SIZE], int y, int x, int diagonals) {
    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
        if (!(dx || dy) || (!diagonals && dx && dy)) continue;
        int yy = y + dy, xx = x + dx;
        if (yy >= 0 && yy < PADICON_SIZE && xx >= 0 && xx < PADICON_SIZE && p[yy][xx]) return 1;
    }
    return 0;
}

/* Player K's bubble (0 - 3): each pixel 0 (clear) or PADICON_SET | RGB. A disc 13 across at 1, 1 with
 * its rim, the number in its outline (columns 5 - 10), then a light outline round it all and a black
 * shadow down and to the right. */
static void padicon_bubble(int k, uint32_t out[PADICON_SIZE][PADICON_SIZE]) {
    uint32_t p[PADICON_SIZE][PADICON_SIZE], digit[PADICON_SIZE][PADICON_SIZE];
    memset(p, 0, sizeof p); memset(digit, 0, sizeof digit);
    for (int y = 0; y < 13; y++) for (int x = 0; x < 13; x++) {
        double dx = x - 6.0, dy = y - 6.0, d = dx * dx + dy * dy;
        if (d <= 6.6 * 6.6) p[y + 1][x + 1] = (d >= 5.4 * 5.4 ? padicon_colours[k][2] : padicon_colours[k][1]) | PADICON_SET;
    }
    for (int y = 0; y < 5; y++) for (int x = 0; x < 6; x++) digit[y + 1][x + 1] = padicon_digits[k][y][x] == '#';
    for (int y = 0; y < PADICON_SIZE; y++) for (int x = 0; x < PADICON_SIZE; x++) {   /* the number, at 4 + its place */
        int yy = 4 + y, xx = 4 + x;
        if (yy >= PADICON_SIZE || xx >= PADICON_SIZE) continue;
        if (digit[y][x]) p[yy][xx] = padicon_colours[k][3] | PADICON_SET;
        else if (padicon_near((const uint32_t (*)[PADICON_SIZE])digit, y, x, 1)) p[yy][xx] = padicon_colours[k][0] | PADICON_SET;
    }
    memcpy(out, p, sizeof p);
    for (int y = 0; y < PADICON_SIZE; y++) for (int x = 0; x < PADICON_SIZE; x++)
        if (!p[y][x] && padicon_near((const uint32_t (*)[PADICON_SIZE])p, y, x, 0)) out[y][x] = PADICON_LIGHT | PADICON_SET;
    for (int y = PADICON_SIZE - 1; y >= 1; y--) for (int x = PADICON_SIZE - 1; x >= 1; x--)
        if (!out[y][x] && out[y - 1][x - 1] && out[y - 1][x - 1] != PADICON_SET) out[y][x] = PADICON_SET;
}
