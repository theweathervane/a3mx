/* Every archive on the disc must come back byte for byte from its folder in assets/ (the one
 * index.json names), built by the
 * game's own builder (src/platform/asset_pac.c), and pass its check against the disc with nothing
 * reported. Then a few hand-made folders: a changed entry, a larger one, an added one, a texture too
 * large for the loaders, and a broken manifest.
 *
 * With a third argument, the PNG decoder on the files tests/png_cases.py wrote there: every colour
 * type, bit depth and row filter an image editor may save.
 *
 * asset_pac_test DISC_DATA_DIR ASSETS_DIR [PNG_CASES_DIR]
 * asset_pac_test --png-only PNG_CASES_DIR       (just the decoder: no disc needed, for CI)
 */
#include "../src/platform/asset_pac.h"
#include "../src/platform/png_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <sys/utime.h>
#define mkdir_p(p) _mkdir(p)
#define utime _utime
#define utimbuf _utimbuf
#else
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#define mkdir_p(p) mkdir(p, 0755)
#endif

static uint8_t *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc(len > 0 ? (size_t)len : 1);
    if (b && fread(b, 1, (size_t)len, f) != (size_t)len) { free(b); b = NULL; }
    fclose(f);
    *n = (size_t)len;
    return b;
}
static void spit(const char *path, const void *b, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) { printf("cannot write %s\n", path); exit(1); }
    fwrite(b, 1, n, f); fclose(f);
}

/* the .pac names in a folder, sorted */
static int cmp(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static char **list_ext(const char *dir, const char *ext, int *count) {
    char **names = NULL; int n = 0, cap = 0;
#ifdef _WIN32
    char pattern[1024]; snprintf(pattern, sizeof pattern, "%s\\*%s", dir, ext);
    WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (n == cap) names = realloc(names, sizeof *names * (cap = cap ? cap * 2 : 512));
            names[n++] = _strdup(fd.cFileName);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(dir);
    struct dirent *e;
    while (d && (e = readdir(d))) {
        size_t l = strlen(e->d_name);
        if (e->d_name[0] == '.' || l <= strlen(ext) || strcmp(e->d_name + l - strlen(ext), ext)) continue;
        if (n == cap) names = realloc(names, sizeof *names * (cap = cap ? cap * 2 : 512));
        names[n++] = strdup(e->d_name);
    }
    if (d) closedir(d);
#endif
    if (n) qsort(names, (size_t)n, sizeof *names, cmp);
    *count = n;
    return names;
}

/* An archive's folder in assets/, by its name on the disc ("end1e_u"): where index.json puts it */
static void archive_folder(const char *assets, const char *stem, char *out, size_t n) {
    if (asset_index_folder(assets, stem, out, n) > 0) snprintf(out, n, "%s/(no folder for %s in index.json)", assets, stem);
}

/* Copies an archive's folder (manifest.json and the subfolders by kind tools/pac.py writes), every
   file dated `when`. */
static const char *const KIND_DIRS[] = { "", "textures", "palettes", "text", "sound", "voices", "fighter_data", "data" };
static void copy_folder(const char *src, const char *dst, time_t when);

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); failures++; } } while (0)

/* ---------------------------------------------------------------- PNG edits */
static uint32_t crc_table[256];
static uint32_t crc32_of(uint32_t c, const uint8_t *p, size_t n) {
    if (!crc_table[1]) for (uint32_t i = 0; i < 256; i++) { uint32_t x = i; for (int k = 0; k < 8; k++) x = x & 1 ? 0xEDB88320u ^ (x >> 1) : x >> 1; crc_table[i] = x; }
    c = ~c; while (n--) c = crc_table[(c ^ *p++) & 255] ^ (c >> 8); return ~c;
}
static void be(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static void put_chunk(FILE *f, const char *t, const uint8_t *d, uint32_t n) {
    uint8_t h[8]; be(h, n); memcpy(h + 4, t, 4); fwrite(h, 1, 8, f); if (n) fwrite(d, 1, n, f);
    uint8_t c[4]; be(c, crc32_of(crc32_of(0, (const uint8_t *)t, 4), d, n)); fwrite(c, 1, 4, f);
}
/* an RGBA PNG of one colour, stored (uncompressed) deflate blocks */
static void write_png(const char *path, int w, int h, const uint8_t rgba[4]) {
    size_t row = (size_t)w * 4 + 1, rawn = row * h, zn = rawn + rawn / 65535 * 5 + 16;
    uint8_t *raw = malloc(rawn), *z = malloc(zn), *q = z;
    for (int y = 0; y < h; y++) { raw[y * row] = 0; for (int x = 0; x < w; x++) memcpy(raw + y * row + 1 + x * 4, rgba, 4); }
    *q++ = 0x78; *q++ = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t off = 0; off < rawn; off += 65535) {
        size_t n = rawn - off < 65535 ? rawn - off : 65535;
        *q++ = off + n >= rawn; *q++ = (uint8_t)n; *q++ = (uint8_t)(n >> 8); *q++ = (uint8_t)~n; *q++ = (uint8_t)(~n >> 8);
        memcpy(q, raw + off, n); q += n;
        for (size_t i = 0; i < n; i++) { a = (a + raw[off + i]) % 65521; b = (b + a) % 65521; }
    }
    be(q, b << 16 | a); q += 4;
    FILE *f = fopen(path, "wb");
    static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    uint8_t ihdr[13]; be(ihdr, (uint32_t)w); be(ihdr + 4, (uint32_t)h); ihdr[8] = 8; ihdr[9] = 6; ihdr[10] = ihdr[11] = ihdr[12] = 0;
    fwrite(sig, 1, 8, f); put_chunk(f, "IHDR", ihdr, 13); put_chunk(f, "IDAT", z, (uint32_t)(q - z)); put_chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(z);
}
static void set_mtime(const char *path, time_t t) { struct utimbuf u = { t, t }; utime(path, &u); }
static void copy_folder(const char *src, const char *dst, time_t when) {
    char from[1200], to[1200], sd[1100], dd[1100];
    mkdir_p(dst);
    for (size_t k = 0; k < sizeof KIND_DIRS / sizeof *KIND_DIRS; k++) {
        snprintf(sd, sizeof sd, "%s%s%s", src, KIND_DIRS[k][0] ? "/" : "", KIND_DIRS[k]);
        snprintf(dd, sizeof dd, "%s%s%s", dst, KIND_DIRS[k][0] ? "/" : "", KIND_DIRS[k]);
        int n; char **files = list_ext(sd, "", &n);
        if (k && n) mkdir_p(dd);
        for (int i = 0; i < n; i++) {
            struct stat st;
            snprintf(from, sizeof from, "%s/%s", sd, files[i]); snprintf(to, sizeof to, "%s/%s", dd, files[i]);
            if (stat(from, &st) != 0 || !(st.st_mode & S_IFREG)) { free(files[i]); continue; }
            size_t len; uint8_t *b = slurp(from, &len);
            if (b) { spit(to, b, len); free(b); set_mtime(to, when); }
            free(files[i]);
        }
        free(files);
    }
}

/* the image's entry i: its bytes and size */
static const uint8_t *image_entry(const PacImage *img, uint32_t i, uint32_t *size) {
    uint32_t off = PAC_SECTOR;
    for (uint32_t k = 0; k < i; k++) { uint32_t s; memcpy(&s, img->data + 12 + 8 * k, 4); off += (s + PAC_SECTOR - 1) & ~(PAC_SECTOR - 1); }
    memcpy(size, img->data + 12 + 8 * i, 4);
    return img->data + off;
}

/* end1e_u: entry 1 a 256x256 ABGR1555 texture, entry 2 a 256x256 4-bit indexed one, entry 5 a palette of 1056 colours */
static void png_edits(const char *assets) {
    const char *names[][2] = { { "textures/01_texture_10000C17", "texture" }, { "textures/02_texture_10000C18", "indexed" }, { "palettes/05_palette_04000019", "palette" } };
    char src[1024], dst[] = "asset_pac_png.tmp", path[1200], err[512];
    archive_folder(assets, "end1e_u", src, sizeof src);
    time_t now = time(NULL), old = now - 1000;
    copy_folder(src, dst, old);
    for (int k = 0; k < 3; k++) {
        snprintf(path, sizeof path, "%s/%s.png", dst, names[k][0]);
        FILE *f = fopen(path, "rb"); CHECK(f != NULL, "png edits: no %s (has the archive changed?)", path); if (!f) return; fclose(f);
    }
    PacImage base, img;
    CHECK(asset_pac_build(dst, &base, err, sizeof err) == 0 && !base.notes[0], "png edits: unedited copy: %s %s", err, base.notes);

    /* repaint: the texture red, the indexed texture index 5, the palette green; PNGs newer than their raw files */
    static const uint8_t red[4] = { 255, 0, 0, 255 }, grey5[4] = { 85, 85, 85, 255 }, green[4] = { 0, 255, 0, 255 };
    snprintf(path, sizeof path, "%s/%s.png", dst, names[0][0]); write_png(path, 256, 256, red); set_mtime(path, now);
    /* the indexed texture: index 5 is grey level 85 while its palette is unknown, else palette colour 5 */
    uint16_t ipal[256];
    int npal = asset_manifest_palette(dst, 2, ipal, 256);
    uint8_t paint5[4] = { grey5[0], grey5[1], grey5[2], grey5[3] };
    if (npal >= 16) {
        unsigned v = ipal[5];
        paint5[0] = (uint8_t)((v & 31) << 3 | (v & 31) >> 2); paint5[1] = (uint8_t)((v >> 5 & 31) << 3 | (v >> 5 & 31) >> 2);
        paint5[2] = (uint8_t)((v >> 10 & 31) << 3 | (v >> 10 & 31) >> 2); paint5[3] = 255;
        for (int k = 1; k < 16; k++) CHECK(k == 5 || ipal[k] != ipal[5] || !(ipal[k] & 0x8000), "png edits: palette colour 5 is not unique; pick another");
    }
    snprintf(path, sizeof path, "%s/%s.png", dst, names[1][0]); write_png(path, 256, 256, paint5); set_mtime(path, now);
    snprintf(path, sizeof path, "%s/%s.png", dst, names[2][0]); write_png(path, 16, 66, green); set_mtime(path, now);
    CHECK(asset_pac_build(dst, &img, err, sizeof err) == 0, "png edits: %s", err);
    uint32_t s1, s2, s5, b1;
    const uint8_t *t = image_entry(&img, 1, &s1), *x = image_entry(&img, 2, &s2), *pal = image_entry(&img, 5, &s5), *orig = image_entry(&base, 1, &b1);
    CHECK(s1 == b1 && !memcmp(t, orig, 0x20), "png edits: texture size or header changed");
    int ok = 1; for (uint32_t i = 0x20; i < s1; i += 2) ok &= t[i] == 0x1F && t[i + 1] == 0x80;   /* ABGR1555 red: 0x801F (red in the low bits) */
    CHECK(ok, "png edits: the texture is not red (0x801F)");
    ok = 1; for (uint32_t i = 0x20; i < s2; i++) ok &= x[i] == 0x55;                                /* index 5 in both nibbles */
    CHECK(ok, "png edits: the indexed texture is not index 5");
    ok = 1; for (uint32_t i = 0; i < s5; i += 2) ok &= pal[i] == 0xE0 && pal[i + 1] == 0x83;        /* ABGR1555 green: 0x83E0 */
    CHECK(ok, "png edits: the palette is not green (0x83E0)");
    CHECK(strstr(img.notes, "entry 1 from") && strstr(img.notes, "entry 2 from") && strstr(img.notes, "entry 5 from"), "png edits: notes: %s", img.notes);
    pac_image_free(&img);

    /* a raw file newer than its PNG wins: replacing raw files still works */
    snprintf(path, sizeof path, "%s/%s.pvr", dst, names[0][0]); set_mtime(path, now + 10);
    CHECK(asset_pac_build(dst, &img, err, sizeof err) == 0, "png edits: %s", err);
    t = image_entry(&img, 1, &s1);
    CHECK(!memcmp(t, orig, s1) && !strstr(img.notes, "entry 1"), "png edits: an older PNG was used over its raw file");
    pac_image_free(&img);
    set_mtime(path, old);

    /* PNGs that cannot be used: the raw file stays, the reason is noted */
    static const uint8_t pink[4] = { 255, 0, 128, 255 };
    snprintf(path, sizeof path, "%s/%s.png", dst, names[0][0]); write_png(path, 128, 128, red); set_mtime(path, now);
    snprintf(path, sizeof path, "%s/%s.png", dst, names[1][0]); write_png(path, 256, 256, npal >= 16 ? red : pink); set_mtime(path, now);
    snprintf(path, sizeof path, "%s/%s.png", dst, names[2][0]); spit(path, "not a png", 9); set_mtime(path, now);
    CHECK(asset_pac_build(dst, &img, err, sizeof err) == 0, "png edits: %s", err);
    if (npal < 16) {   /* (with its palette known, any colour is usable: it goes to the nearest) */
        CHECK(img.size == base.size && !memcmp(img.data, base.data, base.size), "png edits: an unusable PNG changed the archive");
        CHECK(strstr(img.notes, "not grey"), "png edits: notes: %s", img.notes);
    }
    CHECK(strstr(img.notes, "128x128") && strstr(img.notes, "not a PNG"), "png edits: notes: %s", img.notes);
    pac_image_free(&img); pac_image_free(&base);
    printf("png edits: texture, indexed texture (%s) and palette from edited PNGs; newer raw file wins; bad PNGs refused\n",
           npal >= 16 ? "in its own colours" : "in grey indices");
}

/* An indexed texture with its palette known, painted in an editor that saves RGBA: each colour goes to
   the nearest palette index (index 0 for transparent). end00_u entry 6, 256 x 256, 4-bit. */
static void indexed_nearest(const char *assets) {
    char src[1024], dst[] = "asset_pac_idx.tmp", path[1200], err[512], png[256] = "";
    archive_folder(assets, "end00_u", src, sizeof src);
    copy_folder(src, dst, time(NULL) - 1000);
    snprintf(path, sizeof path, "%s/textures", dst);
    int n; char **files = list_ext(path, ".png", &n);
    for (int i = 0; i < n; i++) if (!strncmp(files[i], "06_texture_", 11)) snprintf(png, sizeof png, "textures/%s", files[i]);
    uint16_t pal[256];
    int npal = asset_manifest_palette(dst, 6, pal, 256);
    if (npal < 16 || !png[0]) { CHECK(0, "indexed nearest: end00_u entry 6 has no palette (extract again with tools/data/palette_pairs.json)"); return; }
    int k = -1;                                    /* an opaque colour no other index has */
    for (int c = 1; c < 16 && k < 0; c++) {
        int unique = (pal[c] & 0x8000) != 0;
        for (int d = 1; d < 16; d++) if (d != c && pal[d] == pal[c]) unique = 0;
        if (unique) k = c;
    }
    CHECK(k > 0, "indexed nearest: no unique opaque colour in the palette");
    if (k < 0) return;
    unsigned v = pal[k];
    uint8_t col[4] = { (uint8_t)((v & 31) << 3 | (v & 31) >> 2), (uint8_t)((v >> 5 & 31) << 3 | (v >> 5 & 31) >> 2),
                       (uint8_t)((v >> 10 & 31) << 3 | (v >> 10 & 31) >> 2), 255 };
    col[0] = (uint8_t)(col[0] > 250 ? col[0] - 2 : col[0] + 2);   /* a shade off: still nearest to colour k */
    snprintf(path, sizeof path, "%s/%s", dst, png); write_png(path, 256, 256, col); set_mtime(path, time(NULL));
    PacImage img; uint32_t size;
    CHECK(asset_pac_build(dst, &img, err, sizeof err) == 0, "indexed nearest: %s", err);
    const uint8_t *t = image_entry(&img, 6, &size);
    int ok = 1; for (uint32_t i = 0x20; i < size; i++) ok &= t[i] == (uint8_t)(k | k << 4);
    CHECK(ok, "indexed nearest: the texture is not all index %d", k);
    pac_image_free(&img);
    static const uint8_t clear[4] = { 10, 20, 30, 0 };
    write_png(path, 256, 256, clear); set_mtime(path, time(NULL) + 5);
    CHECK(asset_pac_build(dst, &img, err, sizeof err) == 0, "indexed nearest: %s", err);
    t = image_entry(&img, 6, &size);
    ok = 1; for (uint32_t i = 0x20; i < size; i++) ok &= t[i] == 0;
    CHECK(ok, "indexed nearest: transparent pixels are not index 0");
    pac_image_free(&img);
    printf("indexed nearest: colours to the nearest palette index (%d), transparent to 0\n", k);
}

int main(int argc, char **argv) {
    int png_only = argc == 3 && !strcmp(argv[1], "--png-only");
    if (png_only) { argv[3] = argv[2]; argc = 4; }
    if (argc != 3 && argc != 4) { printf("usage: asset_pac_test DISC_DATA_DIR ASSETS_DIR [PNG_CASES_DIR] | --png-only PNG_CASES_DIR\n"); return 2; }
    if (argc == 4) {   /* 0. the PNG decoder */
        int n, good = 0;
        char **cases = list_ext(argv[3], ".png", &n);
        CHECK(n > 0, "no PNG cases in %s (tests/png_cases.py writes them)", argv[3]);
        for (int i = 0; i < n; i++) {
            char path[1024], err[256]; size_t en;
            snprintf(path, sizeof path, "%s/%s", argv[3], cases[i]);
            PngImage img;
            if (png_decode_file(path, &img, err, sizeof err)) { CHECK(0, "%s: %s", cases[i], err); continue; }
            snprintf(path + strlen(path) - 4, 8, ".rgba");
            uint8_t *want = slurp(path, &en);
            int same = want && en == (size_t)img.w * img.h * 4 && !memcmp(want, img.rgba, en);
            if (!same && want) for (size_t k = 0; k < en; k++) if (want[k] != img.rgba[k]) { CHECK(0, "%s: byte %zu is %u, should be %u", cases[i], k, img.rgba[k], want[k]); break; }
            good += same;
            free(want); png_free(&img);
        }
        printf("png: %d of %d cases decoded exactly\n", good, n);
        /* damaged files are refused, not crashed on */
        uint8_t junk[64] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        PngImage img; char err[256];
        CHECK(png_decode(junk, sizeof junk, &img, err, sizeof err) != 0, "a damaged PNG was accepted");
        if (png_only) { printf(failures ? "png decoder: %d failures\n" : "png decoder: all passed\n", failures); return failures ? 1 : 0; }
    }
    const char *data = argv[1], *assets = argv[2];
    int count;
    char **names = list_ext(data, ".pac", &count);
    if (!count) { printf("no .pac archives in %s\n", data); return 1; }

    /* 1. every archive, unedited: identical, and nothing to report */
    int exact = 0;
    for (int i = 0; i < count; i++) {
        char disc_path[1024], folder[1024], err[512], report[1024];
        snprintf(disc_path, sizeof disc_path, "%s/%s", data, names[i]);
        { char stem[64]; snprintf(stem, sizeof stem, "%.*s", (int)(strlen(names[i]) - 4), names[i]); archive_folder(assets, stem, folder, sizeof folder); }
        size_t dn; uint8_t *disc = slurp(disc_path, &dn);
        PacImage img;
        if (asset_pac_build(folder, &img, err, sizeof err) != 0) { CHECK(0, "%s: %s", names[i], err); free(disc); continue; }
        int same = img.size == dn && !memcmp(img.data, disc, dn);
        int checked = asset_pac_check(&img, disc, dn, report, sizeof report);
        CHECK(same, "%s: built %zu bytes, disc %zu, not identical", names[i], img.size, dn);
        CHECK(checked == 0 && !report[0], "%s: check says %d: %s", names[i], checked, report);
        exact += same && checked == 0 && !report[0];
        pac_image_free(&img); free(disc);
    }
    printf("assets: %d of %d archives built byte for byte from their folders\n", exact, count);

    /* 1b. the same, every texture and palette converted back from its PNG: still byte for byte */
    asset_pac_png_always = 1;
    int exact_png = 0;
    for (int i = 0; i < count; i++) {
        char disc_path[1024], folder[1024], err[512];
        snprintf(disc_path, sizeof disc_path, "%s/%s", data, names[i]);
        { char stem[64]; snprintf(stem, sizeof stem, "%.*s", (int)(strlen(names[i]) - 4), names[i]); archive_folder(assets, stem, folder, sizeof folder); }
        size_t dn; uint8_t *disc = slurp(disc_path, &dn);
        PacImage img;
        if (asset_pac_build(folder, &img, err, sizeof err) != 0) { CHECK(0, "%s (PNGs): %s", names[i], err); free(disc); continue; }
        int same = img.size == dn && !memcmp(img.data, disc, dn);
        CHECK(same && !img.notes[0], "%s: from its PNGs, %s; %s", names[i], same ? "identical" : "NOT identical", img.notes);
        exact_png += same && !img.notes[0];
        pac_image_free(&img); free(disc);
    }
    asset_pac_png_always = 0;
    printf("assets: %d of %d archives built byte for byte with every texture and palette taken from its PNG\n", exact_png, count);

    /* 2. hand-made folders, from a copy of a small archive's folder */
    const char *probe = "sign";   /* two small data entries */
    char tmp[1024], err[512], report[1024], path[1100];
    snprintf(tmp, sizeof tmp, "asset_pac_test.tmp");
    mkdir_p(tmp);
    snprintf(path, sizeof path, "%s/%s.pac", data, probe);
    size_t dn; uint8_t *disc = slurp(path, &dn);
    if (!disc) { printf("no %s\n", path); return 1; }
    uint32_t entries = disc[0] | disc[1] << 8 | disc[2] << 16 | (uint32_t)disc[3] << 24;
    /* the manifest is rewritten by hand: one entry per raw file, written here from the disc archive */
    uint32_t off = PAC_SECTOR;
    char manifest[16384]; int m = snprintf(manifest, sizeof manifest, "{\"archive\": \"%s.pac\", \"entries\": [", probe);
    for (uint32_t i = 0; i < entries; i++) {
        uint32_t t, s;
        memcpy(&t, disc + 8 + 8 * i, 4); memcpy(&s, disc + 12 + 8 * i, 4);
        char raw[64]; snprintf(raw, sizeof raw, "e%02u.bin", i);
        snprintf(path, sizeof path, "%s/%s", tmp, raw); spit(path, disc + off, s);
        uint32_t aligned = (s + PAC_SECTOR - 1) & ~(PAC_SECTOR - 1);
        char padname[64] = "";
        if (aligned > s) { snprintf(padname, sizeof padname, "e%02u.pad", i); snprintf(path, sizeof path, "%s/%s", tmp, padname); spit(path, disc + off + s, aligned - s); }
        m += snprintf(manifest + m, sizeof manifest - (size_t)m, "%s{\"type\": \"0x%08X\", \"raw\": \"%s\"%s%s%s}",
                      i ? ", " : "", t, raw, padname[0] ? ", \"padding\": \"" : "", padname, padname[0] ? "\"" : "");
        off += aligned;
    }
    snprintf(manifest + m, sizeof manifest - (size_t)m, "]}");
    snprintf(path, sizeof path, "%s/manifest.json", tmp); spit(path, manifest, strlen(manifest));

    PacImage img;
    CHECK(asset_pac_build(tmp, &img, err, sizeof err) == 0, "rewritten folder: %s", err);
    CHECK(img.size == dn && !memcmp(img.data, disc, dn), "rewritten folder: not identical to the disc");
    pac_image_free(&img);

    /* a changed entry (same size): reported, loadable */
    {
        size_t n; snprintf(path, sizeof path, "%s/e00.bin", tmp); uint8_t *e = slurp(path, &n);
        uint8_t *orig = malloc(n); memcpy(orig, e, n);
        e[n - 1] ^= 0xFF; spit(path, e, n);
        CHECK(asset_pac_build(tmp, &img, err, sizeof err) == 0, "changed entry: %s", err);
        int r = asset_pac_check(&img, disc, dn, report, sizeof report);
        CHECK(r == 0 && strstr(report, "entry 0") && strstr(report, "changed"), "changed entry: %d %s", r, report);
        CHECK(img.data[PAC_SECTOR + n - 1] == e[n - 1], "changed entry: new byte not in the image");
        pac_image_free(&img);
        /* larger by 3 sectors and a bit: the header and everything after it move */
        size_t big = n + 3 * PAC_SECTOR + 100;
        uint8_t *g = calloc(1, big); memcpy(g, orig, n); memset(g + n, 0xA5, big - n); spit(path, g, big);
        CHECK(asset_pac_build(tmp, &img, err, sizeof err) == 0, "larger entry: %s", err);
        r = asset_pac_check(&img, disc, dn, report, sizeof report);
        uint32_t size0; memcpy(&size0, img.data + 12, 4);
        CHECK(size0 == big && r == 1 && strstr(report, "->") && strstr(report, "WARNING"), "larger entry: size %u, %d %s", size0, r, report);
        uint32_t ds; memcpy(&ds, img.data + 4, 4);
        CHECK(PAC_SECTOR + ds == img.size, "larger entry: data size %u for an image of %zu", ds, img.size);
        CHECK(img.size == dn + ((big + PAC_SECTOR - 1) & ~(size_t)(PAC_SECTOR - 1)) - ((n + PAC_SECTOR - 1) & ~(size_t)(PAC_SECTOR - 1)),
              "larger entry: image %zu bytes", img.size);
        pac_image_free(&img);
        spit(path, orig, n); free(orig); free(e); free(g);
    }
    /* a texture too large for the staging buffer: refused */
    {
        size_t big = PAC_TEXTURE_STAGING + 1;
        uint8_t *g = calloc(1, big); memcpy(g, "GBIX", 4);
        snprintf(path, sizeof path, "%s/huge.pvr", tmp); spit(path, g, big); free(g);
        char m2[sizeof manifest + 128]; size_t l = strlen(manifest);
        memcpy(m2, manifest, l - 2);
        snprintf(m2 + l - 2, sizeof m2 - (l - 2), ", {\"type\": \"0x10000000\", \"raw\": \"huge.pvr\"}]}");
        snprintf(path, sizeof path, "%s/manifest.json", tmp); spit(path, m2, strlen(m2));
        CHECK(asset_pac_build(tmp, &img, err, sizeof err) == 0, "huge texture: build: %s", err);
        int r = asset_pac_check(&img, disc, dn, report, sizeof report);
        CHECK(r < 0 && strstr(report, "staging"), "huge texture: %d %s", r, report);
        pac_image_free(&img);
        /* the same as a data entry: added, with the overrun warning */
        snprintf(m2 + l - 2, sizeof m2 - (l - 2), ", {\"type\": 7, \"raw\": \"huge.pvr\"}]}");
        spit(path, m2, strlen(m2));
        CHECK(asset_pac_build(tmp, &img, err, sizeof err) == 0, "added entry: %s", err);
        r = asset_pac_check(&img, disc, dn, report, sizeof report);
        CHECK(r == 1 && strstr(report, "added") && strstr(report, "WARNING"), "added entry: %d %s", r, report);
        pac_image_free(&img);
        /* a broken manifest and a missing raw file */
        spit(path, "{\"entries\": [", 13);
        CHECK(asset_pac_build(tmp, &img, err, sizeof err) != 0 && strstr(err, "JSON"), "broken manifest: %s", err);
        snprintf(m2 + l - 2, sizeof m2 - (l - 2), ", {\"type\": 7, \"raw\": \"nothere.bin\"}]}");
        spit(path, m2, strlen(m2));
        CHECK(asset_pac_build(tmp, &img, err, sizeof err) != 0 && strstr(err, "not found"), "missing file: %s", err);
    }
    free(disc);
    png_edits(assets);
    indexed_nearest(assets);
    printf(failures ? "asset_pac_test: %d failures\n" : "asset_pac_test: all passed\n", failures);
    return failures ? 1 : 0;
}
