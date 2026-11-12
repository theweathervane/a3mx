/* Reading PNG files: enough for assets edited in any image editor (src/platform/asset_pac.c).
 * Standalone (its own inflate, no runtime dependencies) so tests can link it on its own. */
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int w, h;
    uint8_t *rgba;           /* w * h * 4, straight alpha */
    uint8_t *index;          /* colour type 3 (palette): w * h palette indices, else NULL */
    int color_type;          /* as in the file: 0 grey, 2 RGB, 3 palette, 4 grey + alpha, 6 RGBA */
    int npalette;            /* colour type 3: the file's palette (RGBA, alpha from tRNS) */
    uint8_t palette[256][4];
} PngImage;

/* Decodes a PNG (any colour type; bit depths 1-16; not interlaced). 0 on success; otherwise -1 and a
   reason in err. Free with png_free. */
int png_decode(const uint8_t *data, size_t size, PngImage *out, char *err, size_t errn);
int png_decode_file(const char *path, PngImage *out, char *err, size_t errn);
void png_free(PngImage *img);

/* zlib stream -> bytes (exposed for tests). out grows as needed; 0 on success. */
int zlib_inflate(const uint8_t *src, size_t n, uint8_t **out, size_t *outn);
