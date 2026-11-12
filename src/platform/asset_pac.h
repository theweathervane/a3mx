/* Archives from assets/: a .pac rebuilt in memory from its unpacked folder (docs/ASSET_LOADING.md).
 *
 * A folder written by tools/pac.py unpack (or tools/extract_assets.py) holds manifest.json and one
 * raw file per entry, plus a .pad file where the disc archive had stale bytes after an entry. The
 * image is laid out exactly as tools/pac.py build does it, so an unedited folder gives back the disc's
 * archive byte for byte.
 *
 * Standalone (no runtime dependencies) so tests/asset_pac_test.c can link it on its own.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

#define PAC_SECTOR          0x800u
#define PAC_MAX_ENTRIES     254u        /* (0x800 - 8) / 8: the header is one sector */
#define PAC_TEXTURE_STAGING 0x20800u    /* the loaders' texture staging buffer (loader.h TEXTURE_STAGING) */

typedef struct {
    uint8_t *data;
    size_t   size;
    char     notes[768];   /* entries taken from an edited PNG, or PNGs that could not be used, and why */
} PacImage;

/* Builds the archive for `folder` (which holds manifest.json). An entry whose manifest names a PNG
   ("png") newer than its raw file is converted back from the PNG, in the raw file's format: 16-bit
   textures, indexed textures (in the colours of their palette, "palette_colours", when it is known;
   otherwise in the grey levels the PNG was written with: the indices) and palettes. A PNG that cannot be converted (wrong size, colours an indexed texture cannot hold, a
   damaged file) leaves the raw file in use, with the reason in notes. 0 on success; otherwise -1 and a
   reason in err. Free the image with pac_image_free. */
int asset_pac_build(const char *folder, PacImage *out, char *err, size_t errn);
void pac_image_free(PacImage *img);
/* Tests: take every PNG as newer than its raw file. */
extern int asset_pac_png_always;

/* Compares a built image with the disc's archive. Writes what differs (one line, entries by index)
   into report, empty if nothing does. Returns:
     0  safe to load
     1  loadable, with a warning in report: an entry of a kind the game copies into an existing
        buffer has grown, which can overwrite whatever follows that buffer
    -1  refused, reason in report: an entry the loaders cannot take (a texture larger than their
        staging buffer, a broken header). disc may be NULL (no disc archive of that name). */
int asset_pac_check(const PacImage *img, const uint8_t *disc, size_t disc_size, char *report, size_t reportn);

/* Converts a PNG back into the format of `raw` (a texture or palette entry of type `type`): the result
   is the same size as raw. An indexed texture with its palette (pal: npal ABGR1555 colours, the
   manifest's "palette_colours") takes, for each pixel, the index of the nearest colour (index 0 for
   transparent pixels; a palette PNG with that palette unchanged keeps its indices exactly); without
   one, the grey level is the index. 0 on success; otherwise -1 and a reason in err. Free *out. */
int asset_png_to_entry(uint32_t type, const uint8_t *raw, size_t rawn, const char *png_path,
                       const uint16_t *pal, int npal, uint8_t **out, char *err, size_t errn);

/* An entry's "palette_colours" from folder/manifest.json, into pal (up to max); the count, 0 if none. */
int asset_manifest_palette(const char *folder, int index, uint16_t *pal, int max);

/* Where an archive's folder is: the folder index.json (in the assets folder `root`, written by
   tools/extract_assets.py; assets/LAYOUT.md) gives for `stem` (its name on the disc without .pac, e.g.
   "st0c": "stages/0c_dan"), as root/folder in out. Returns 0 when the index names it, 1 when the index
   has no line for it (the archive comes from the disc), -1 when there is no index: out is then
   root/pac/stem, the flat layout from before 0.37.0. The index is read once per root. */
int asset_index_folder(const char *root, const char *stem, char *out, size_t outn);

/* The value of "key" in a flat JSON object file (e.g. work/disc.json's "region"), as a string, into
   out. 0 if found. */
int json_file_string(const char *path, const char *key, char *out, size_t outn);
/* The value of "key" in a flat JSON object file, as a number, into out. 0 if found (and a number). */
int json_file_number(const char *path, const char *key, double *out);
