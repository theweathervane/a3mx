/* Archives from assets/: see asset_pac.h. */
#include "asset_pac.h"
#include "png_decode.h"
#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ---------------------------------------------------------------- a small JSON reader
   Enough for manifest.json: objects, arrays, strings (with the usual escapes; \u to UTF-8),
   numbers, true / false / null. */
typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } JType;
typedef struct JVal JVal;
struct JVal {
    JType type;
    double num;
    char *str;            /* J_STR: the string; also the key when this is an object member */
    char *key;
    JVal *child, *next;   /* J_ARR / J_OBJ: first element / member, then siblings */
};

typedef struct { const char *p, *end; int depth; } JParser;

static void jfree(JVal *v) {
    while (v) {
        JVal *n = v->next;
        jfree(v->child); free(v->str); free(v->key); free(v);
        v = n;
    }
}

static void jskip(JParser *j) {
    while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r')) j->p++;
}

static char *jstring(JParser *j) {
    if (j->p >= j->end || *j->p != '"') return NULL;
    j->p++;
    size_t cap = 32, n = 0;
    char *s = malloc(cap);
    if (!s) return NULL;
    while (j->p < j->end && *j->p != '"') {
        char ch = *j->p++;
        unsigned cp = (unsigned char)ch;
        if (ch == '\\') {
            if (j->p >= j->end) break;
            char e = *j->p++;
            switch (e) {
            case 'n': cp = '\n'; break; case 't': cp = '\t'; break; case 'r': cp = '\r'; break;
            case 'b': cp = '\b'; break; case 'f': cp = '\f'; break;
            case 'u': {
                if (j->end - j->p < 4) { free(s); return NULL; }
                char hex[5] = { j->p[0], j->p[1], j->p[2], j->p[3], 0 };
                cp = (unsigned)strtoul(hex, NULL, 16); j->p += 4;
                break;
            }
            default: cp = (unsigned char)e; break;
            }
        }
        if (n + 4 >= cap) { cap *= 2; char *t = realloc(s, cap); if (!t) { free(s); return NULL; } s = t; }
        if (cp < 0x80) s[n++] = (char)cp;
        else if (cp < 0x800) { s[n++] = (char)(0xC0 | (cp >> 6)); s[n++] = (char)(0x80 | (cp & 0x3F)); }
        else { s[n++] = (char)(0xE0 | (cp >> 12)); s[n++] = (char)(0x80 | ((cp >> 6) & 0x3F)); s[n++] = (char)(0x80 | (cp & 0x3F)); }
    }
    if (j->p >= j->end) { free(s); return NULL; }
    j->p++;
    s[n] = 0;
    return s;
}

static JVal *jvalue(JParser *j) {
    jskip(j);
    if (j->p >= j->end || ++j->depth > 64) return NULL;
    JVal *v = calloc(1, sizeof *v);
    if (!v) return NULL;
    char ch = *j->p;
    if (ch == '{' || ch == '[') {
        char close = ch == '{' ? '}' : ']';
        v->type = ch == '{' ? J_OBJ : J_ARR;
        j->p++;
        JVal **tail = &v->child;
        jskip(j);
        if (j->p < j->end && *j->p == close) { j->p++; j->depth--; return v; }
        for (;;) {
            char *key = NULL;
            if (v->type == J_OBJ) {
                jskip(j);
                if (!(key = jstring(j))) goto fail;
                jskip(j);
                if (j->p >= j->end || *j->p != ':') { free(key); goto fail; }
                j->p++;
            }
            JVal *m = jvalue(j);
            if (!m) { free(key); goto fail; }
            m->key = key;
            *tail = m; tail = &m->next;
            jskip(j);
            if (j->p < j->end && *j->p == ',') { j->p++; continue; }
            if (j->p < j->end && *j->p == close) { j->p++; break; }
            goto fail;
        }
    } else if (ch == '"') {
        v->type = J_STR;
        if (!(v->str = jstring(j))) goto fail;
    } else if (!strncmp(j->p, "true", 4) || !strncmp(j->p, "false", 5)) {
        v->type = J_BOOL; v->num = *j->p == 't';
        j->p += *j->p == 't' ? 4 : 5;
    } else if (!strncmp(j->p, "null", 4)) {
        v->type = J_NULL; j->p += 4;
    } else {
        char *e;
        v->type = J_NUM; v->num = strtod(j->p, &e);
        if (e == j->p) goto fail;
        j->p = e;
    }
    j->depth--;
    return v;
fail:
    jfree(v);
    return NULL;
}

static JVal *jmember(const JVal *obj, const char *key) {
    if (!obj || obj->type != J_OBJ) return NULL;
    for (JVal *m = obj->child; m; m = m->next) if (m->key && !strcmp(m->key, key)) return m;
    return NULL;
}

/* ---------------------------------------------------------------- files */
static uint8_t *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t *b = NULL;
    size_t cap = 0, n = 0;
    for (;;) {
        if (n == cap) {
            cap = cap ? cap * 2 : 1 << 16;
            uint8_t *t = realloc(b, cap);
            if (!t) { free(b); fclose(f); return NULL; }
            b = t;
        }
        size_t got = fread(b + n, 1, cap - n, f);
        n += got;
        if (got == 0) break;
    }
    fclose(f);
    *size = n;
    if (!b) b = malloc(1);
    return b;
}

static JVal *json_load(const char *path) {
    size_t n;
    char *text = (char *)read_file(path, &n);
    if (!text) return NULL;
    JParser j = { text, text + n, 0 };
    if (n >= 3 && (uint8_t)text[0] == 0xEF && (uint8_t)text[1] == 0xBB && (uint8_t)text[2] == 0xBF) j.p += 3;   /* BOM */
    JVal *v = jvalue(&j);
    free(text);
    return v;
}

int json_file_string(const char *path, const char *key, char *out, size_t outn) {
    JVal *root = json_load(path);
    JVal *m = jmember(root, key);
    int ok = m && m->type == J_STR;
    if (ok) snprintf(out, outn, "%s", m->str);
    jfree(root);
    return ok ? 0 : -1;
}

int json_file_number(const char *path, const char *key, double *out) {
    JVal *root = json_load(path);
    JVal *m = jmember(root, key);
    int ok = m && m->type == J_NUM;
    if (ok) *out = m->num;
    jfree(root);
    return ok ? 0 : -1;
}

/* ---------------------------------------------------------------- where an archive's folder is */
/* index.json is read once per assets folder and kept (it is small: one line per archive). */
static char  index_root[1024];
static JVal *index_tree;
static int   index_loaded;

int asset_index_folder(const char *root, const char *stem, char *out, size_t outn) {
    if (!index_loaded || strcmp(index_root, root)) {
        if (index_tree) jfree(index_tree);
        char path[1100];
        snprintf(path, sizeof path, "%s/index.json", root);
        index_tree = json_load(path);
        snprintf(index_root, sizeof index_root, "%s", root);
        index_loaded = 1;
    }
    if (!index_tree) {                         /* the flat layout from before 0.37.0 */
        snprintf(out, outn, "%s/pac/%s", root, stem);
        return -1;
    }
    JVal *archives = jmember(index_tree, "archives");
    if (!archives || archives->type != J_OBJ) return 1;
    for (JVal *m = archives->child; m; m = m->next) {
        if (!m->key || m->type != J_STR) continue;
        const char *a = m->key, *b = stem;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) a++, b++;
        if (*a || *b) continue;
        snprintf(out, outn, "%s/%s", root, m->str);
        return 0;
    }
    return 1;
}

static uint32_t align_sector(uint32_t n) { return (n + PAC_SECTOR - 1) & ~(PAC_SECTOR - 1); }
static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

static int fail(char *err, size_t errn, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    if (err && errn) vsnprintf(err, errn, fmt, ap);
    va_end(ap);
    return -1;
}


/* ---------------------------------------------------------------- PNG back to entries */
/* 8-bit channel -> n-bit, rounding: exact on the values tools/pac.py widens to (the PSP's own: high
   bits repeated in the low ones), and the nearest colour for anything painted in between */
static unsigned down(unsigned v, unsigned max) { return (v * max + 127) / 255; }
/* The PSP's 16-bit colours keep red in the low bits (ABGR1555, BGR565, ABGR4444 from the top bit);
   pf as in the PowerVR header: 0, 1, 2. */
static uint16_t pack16(const uint8_t *px, int pf) {
    unsigned r = px[0], g = px[1], b = px[2], a = px[3];
    switch (pf) {
    case 0:  return (uint16_t)((a >= 128 ? 0x8000 : 0) | down(b, 31) << 10 | down(g, 31) << 5 | down(r, 31));
    case 1:  return (uint16_t)(down(b, 31) << 11 | down(g, 63) << 5 | down(r, 31));
    default: return (uint16_t)(down(a, 15) << 12 | down(b, 15) << 8 | down(g, 15) << 4 | down(r, 15));
    }
}

/* the colours an indexed texture's PNG was written in (tools/pac.py texture_to_png): ABGR1555 widened,
   index 0 transparent */
static void palette_rgba(const uint16_t *pal, int npal, uint8_t (*rgba)[4]) {
    for (int i = 0; i < npal; i++) {
        unsigned v = pal[i];
        rgba[i][0] = (uint8_t)((v & 31) << 3 | (v & 31) >> 2);
        rgba[i][1] = (uint8_t)((v >> 5 & 31) << 3 | (v >> 5 & 31) >> 2);
        rgba[i][2] = (uint8_t)((v >> 10 & 31) << 3 | (v >> 10 & 31) >> 2);
        rgba[i][3] = i == 0 ? 0 : (v & 0x8000) ? 255 : 0;
    }
}

/* the index of the colour nearest px: transparent pixels to index 0 (as drawn), others to the nearest
   opaque colour (any colour if the palette has no opaque one) */
static int nearest(const uint8_t *px, const uint8_t (*rgba)[4], int npal) {
    if (px[3] < 128) return 0;
    int best = -1; long bestd = 0;
    for (int pass = 0; pass < 2 && best < 0; pass++)
        for (int i = 1; i < npal; i++) {
            if (pass == 0 && rgba[i][3] < 128) continue;
            long dr = px[0] - rgba[i][0], dg = px[1] - rgba[i][1], db = px[2] - rgba[i][2];
            long d = 2 * dr * dr + 4 * dg * dg + 3 * db * db;
            if (best < 0 || d < bestd) { best = i; bestd = d; }
        }
    return best < 0 ? 0 : best;
}

int asset_png_to_entry(uint32_t type, const uint8_t *raw, size_t rawn, const char *png_path,
                       const uint16_t *pal, int npal, uint8_t **out, char *err, size_t errn) {
    *out = NULL;
    uint32_t kind = type & 0xFFFF0000u;
    PngImage img;
    char why[256];
    if (png_decode_file(png_path, &img, why, sizeof why)) return fail(err, errn, "%s", why);
    uint8_t *e = malloc(rawn ? rawn : 1);
    if (!e) { png_free(&img); return fail(err, errn, "out of memory"); }
    memcpy(e, raw, rawn);

    if (kind == 0x04000000u) {                     /* palette: 16 colours a row, ABGR1555 */
        size_t n = rawn / 2;
        int want_h = (int)((n + 15) / 16);
        if (img.w != 16 || img.h != want_h) { free(e); png_free(&img); return fail(err, errn, "the PNG is %dx%d; this palette's is 16x%d", img.w, img.h, want_h); }
        for (size_t i = 0; i < n; i++) {
            uint16_t c = pack16(img.rgba + i * 4, 0);
            e[2 * i] = (uint8_t)c; e[2 * i + 1] = (uint8_t)(c >> 8);
        }
    } else {                                       /* texture: PowerVR header, then linear pixels */
        const uint8_t *p = NULL;
        for (size_t k = 0; k + 16 <= rawn && k < 32; k++) if (!memcmp(raw + k, "PVRT", 4)) { p = raw + k; break; }
        if (!p) { free(e); png_free(&img); return fail(err, errn, "the raw file has no PVRT header"); }
        int pf = p[8], df = p[9], w = p[12] | p[13] << 8, h = p[14] | p[15] << 8;
        size_t off = (size_t)(p - raw) + 16;
        size_t need = df == 1 ? (size_t)w * h * 2 : df == 5 ? ((size_t)w * h + 1) / 2 : df == 7 ? (size_t)w * h : 0;
        if (!need) { free(e); png_free(&img); return fail(err, errn, "texture data format %d is not one the PNG can go back to", df); }
        if (off + need > rawn) { free(e); png_free(&img); return fail(err, errn, "the raw file is shorter than its header says"); }
        if (img.w != w || img.h != h) { free(e); png_free(&img); return fail(err, errn, "the PNG is %dx%d; the texture is %dx%d", img.w, img.h, w, h); }
        uint8_t *d = e + off;
        if (df == 5) memset(d, 0, need);
        int ncol = df == 5 ? 16 : 256;
        uint8_t prgba[256][4];
        int same_palette = 0;                      /* a palette PNG still in the texture's own palette: its indices are the indices */
        if (pal && npal >= ncol && df != 1) {
            palette_rgba(pal, ncol, prgba);
            if (img.color_type == 3 && img.npalette >= ncol) {
                same_palette = 1;
                for (int i = 1; i < ncol && same_palette; i++)
                    same_palette = !memcmp(img.palette[i], prgba[i], 3) && (img.palette[i][3] >= 128) == (prgba[i][3] >= 128);
            }
        }
        for (int i = 0; i < w * h; i++) {
            const uint8_t *px = img.rgba + (size_t)i * 4;
            if (df == 1) {
                uint16_t c = pack16(px, pf);
                d[2 * i] = (uint8_t)c; d[2 * i + 1] = (uint8_t)(c >> 8);
                continue;
            }
            int index;
            if (same_palette && img.index[i] < ncol) index = img.index[i];
            else if (pal && npal >= ncol) index = nearest(px, (const uint8_t (*)[4])prgba, ncol);
            else {
                /* no palette known: the PNG holds the indices as grey levels (i * 17 for 4-bit, i for 8-bit) */
                if (px[0] != px[1] || px[1] != px[2]) {
                    free(e); png_free(&img);
                    return fail(err, errn, "pixel %d,%d is not grey: this indexed texture's palette is not known, so it is painted in the grey levels of its PNG (its colour indices)", i % w, i / w);
                }
                index = df == 7 ? px[0] : (px[0] + 8) / 17;
            }
            if (df == 7) d[i] = (uint8_t)index;
            else d[i >> 1] |= (uint8_t)((index & 15) << ((i & 1) * 4));
        }
    }
    png_free(&img);
    *out = e;
    return 0;
}

int asset_pac_png_always;
static int parse_palette(const JVal *pc, uint16_t *pal, int max);

static long long mtime_of(const char *path) {
    struct stat s;
    return stat(path, &s) == 0 ? (long long)s.st_mtime : -1;
}

static void note(PacImage *img, const char *fmt, ...) {
    size_t used = strlen(img->notes);
    if (used + 4 >= sizeof img->notes) return;
    if (used) { strcpy(img->notes + used, "; "); used += 2; }
    va_list ap; va_start(ap, fmt);
    vsnprintf(img->notes + used, sizeof img->notes - used, fmt, ap);
    va_end(ap);
}

/* ---------------------------------------------------------------- fighter sprites from their PNGs
   docs/SPRITE_FORMAT.md. A fighter archive's folder may hold sprites/ (tools/sprites.py extract):
   pictures/NNNN.png, one indexed PNG a picture (index = palette x 16 + colour, 0 transparent), and
   metadata.json (the six colours' palettes as hex ABGR1555). A picture PNG edited after the pictures
   were written ("pictures_written" in metadata.json, else its own time) is compared with the picture
   the archive holds; if it differs, its cells are made again: an empty cell is 0xFF, an unchanged one
   keeps its tiles, a changed one gets four new tiles appended to the tile set (entry 0x00200000), and
   the picture's new cells are appended to the picture list (entry 0x00100000) with its offset moved
   there. Everything else keeps its bytes. The colours in metadata.json replace the palettes entry
   (0x00400000) where they differ. */
#define SPR_PICTURES 0x00100000u
#define SPR_TILES    0x00200000u
#define SPR_PALETTES 0x00400000u
#define SPR_CELL     32
#define SPR_EMPTY    0xFF
#define SPR_BLEND    0x20u
#define SPR_FLIP_X   0x40u
#define SPR_FLIP_Y   0x80u

/* Tile t of a set, unpacked (0x088D2B64): 128 bytes, row by row (8 bytes a row of 16 pixels, the left
   one in the low nibble). In that order a copy token h:l takes l bytes from h back, as the game's
   tables at 0x08A3E600 / 0x08A3E680 work out for a tile in a page. 0 on success. */
static int tile_unpack(const uint8_t *set, size_t n, uint32_t t, uint8_t out[160]) {
    if ((size_t)t * 4 + 4 > n) return -1;
    size_t s = rd32(set + 4 * (size_t)t);
    unsigned k = 0, flags = 0, bits = 0;
    while (k < 128) {
        if (s >= n) return -1;
        unsigned tok = set[s++];
        if (!bits) { flags = tok; if (s >= n) return -1; tok = set[s++]; bits = 8; }
        if (!(flags & 1)) out[k++] = (uint8_t)tok;
        else {
            unsigned h = tok >> 4, l = tok & 15;
            if (!h) { if (s >= n) return -1; uint8_t v = set[s++]; for (unsigned i = 0; i <= l; i++) out[k++] = v; }
            else { if (h > k) return -1; for (unsigned i = 0; i < l; i++, k++) out[k] = out[k - h]; }
        }
        flags >>= 1; bits--;
    }
    return 0;
}

/* 128 tile bytes packed for tile_unpack: runs (3 - 16 bytes), copies (2 - 15 bytes, 1 - 15 back) and
   literals, never writing past the tile. Answers the length (at most 144). */
static size_t tile_pack(const uint8_t b[128], uint8_t out[160]) {
    size_t o = 0, flag_at = 0;
    unsigned k = 0, bits = 8;
    while (k < 128) {
        if (bits == 8) { flag_at = o++; out[flag_at] = 0; bits = 0; }
        unsigned run = 1;
        while (k + run < 128 && run < 16 && b[k + run] == b[k]) run++;
        unsigned best = 0, back = 0;
        for (unsigned d = 1; d <= 15 && d <= k; d++) {
            unsigned len = 0;
            while (len < 15 && k + len < 128 && b[k + len] == b[k + len - d]) len++;
            if (len > best) { best = len; back = d; }
        }
        if (best >= 2 && best >= run) { out[flag_at] |= (uint8_t)(1u << bits); out[o++] = (uint8_t)(back << 4 | best); k += best; }
        else if (run >= 3) { out[flag_at] |= (uint8_t)(1u << bits); out[o++] = (uint8_t)(run - 1); out[o++] = b[k]; k += run; }
        else out[o++] = b[k++];
        bits++;
    }
    return o;
}

typedef struct {
    const uint8_t *pics, *tiles;
    size_t picsn, tilesn;
    uint32_t count;           /* pictures */
} SpriteSet;

/* Picture n's header: its cells' offset, columns and rows. 0 when it is in the list. */
static int spr_picture(const SpriteSet *s, uint32_t n, uint32_t *off, unsigned *cols, unsigned *rows) {
    if (n >= s->count || (size_t)n * 12 + 12 > s->picsn) return -1;
    *off = rd32(s->pics + 12 * (size_t)n); *cols = s->pics[12 * (size_t)n + 4]; *rows = s->pics[12 * (size_t)n + 5];
    return 0;
}

/* A cell (tile number, flags) drawn into a picture's indices at column cx, row cy. 0 on success. */
static int spr_draw_cell(const SpriteSet *s, unsigned cell, unsigned flags, uint8_t *img, int w, int cx, int cy) {
    for (unsigned q = 0; q < 4; q++) {            /* top left, bottom left, top right, bottom right */
        uint8_t t[160];
        if (tile_unpack(s->tiles, s->tilesn, cell * 4 + q, t)) return -1;
        int qx = (q >> 1) * 16, qy = (q & 1) * 16;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned c = (t[y * 8 + x / 2] >> (4 * (x & 1))) & 15;
                int px = qx + x, py = qy + y;
                if (flags & SPR_FLIP_X) px = SPR_CELL - 1 - px;
                if (flags & SPR_FLIP_Y) py = SPR_CELL - 1 - py;
                img[(size_t)(cy * SPR_CELL + py) * w + cx * SPR_CELL + px] = c ? (uint8_t)(((flags & 0x1F) * 16 + c) & 0xFF) : 0;
            }
    }
    return 0;
}

int asset_sprite_picture(const uint8_t *pics, size_t picsn, const uint8_t *tiles, size_t tilesn, uint32_t n,
                         uint8_t **out, int *w, int *h) {
    SpriteSet s = { pics, tiles, picsn, tilesn, picsn >= 4 ? rd32(pics) / 12 : 0 };
    uint32_t off; unsigned cols, rows;
    *out = NULL;
    if (spr_picture(&s, n, &off, &cols, &rows)) return -1;
    *w = (int)cols * SPR_CELL; *h = (int)rows * SPR_CELL;
    uint8_t *img = calloc(1, (size_t)*w * *h + 1);
    if (!img) return -1;
    size_t p = off;
    for (unsigned i = 0; i < cols * rows; i++) {
        if (p >= picsn) { free(img); return -1; }
        if (pics[p] == SPR_EMPTY) { p++; continue; }
        if (p + 3 > picsn) { free(img); return -1; }
        unsigned cell = (unsigned)pics[p] << 8 | pics[p + 1], flags = pics[p + 2];
        p += 3;
        if (spr_draw_cell(&s, cell, flags, img, *w, (int)(i % cols), (int)(i / cols))) { free(img); return -1; }
    }
    *out = img;
    return 0;
}

typedef struct { uint8_t *b; size_t n, cap; } Buf;
static int buf_add(Buf *b, const void *d, size_t n) {
    if (b->n + n > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (b->n + n > cap) cap *= 2;
        uint8_t *t = realloc(b->b, cap);
        if (!t) return -1;
        b->b = t; b->cap = cap;
    }
    memcpy(b->b + b->n, d, n); b->n += n;
    return 0;
}

/* RGBA to an index of palette p (colours 1 - 15 of colour choice 0, the PNGs' viewing palette): the
   nearest; 0 below half alpha. */
static uint8_t spr_rgba_index(const uint8_t *px, const uint8_t *palettes, size_t paln, unsigned p) {
    if (px[3] < 128) return 0;
    if (!palettes) return (uint8_t)(p * 16 + 1);
    int best = 1; long bestd = -1;
    for (int c = 1; c < 16; c++) {
        size_t at = (size_t)p * 32 + (size_t)c * 2;
        if (at + 2 > paln) break;
        unsigned v = palettes[at] | palettes[at + 1] << 8;
        int r = (v & 31) << 3 | (v & 31) >> 2, g = (v >> 5 & 31) << 3 | (v >> 5 & 31) >> 2, b = (v >> 10 & 31) << 3 | (v >> 10 & 31) >> 2;
        long d = (long)(r - px[0]) * (r - px[0]) + (long)(g - px[1]) * (g - px[1]) + (long)(b - px[2]) * (b - px[2]);
        if (bestd < 0 || d < bestd) { bestd = d; best = c; }
    }
    return (uint8_t)(p * 16 + best);
}

/* The palettes entry from metadata.json's "colours" (colours x palettes x 16 hex strings) into out
   (paln bytes). 0 when it fills the entry exactly. */
static int spr_colours(const JVal *colours, uint8_t *out, size_t paln) {
    size_t at = 0;
    if (!colours || colours->type != J_ARR) return -1;
    for (const JVal *k = colours->child; k; k = k->next) {
        if (k->type != J_ARR) return -1;
        for (const JVal *p = k->child; p; p = p->next) {
            if (p->type != J_ARR) return -1;
            for (const JVal *c = p->child; c; c = c->next) {
                if (c->type != J_STR || at + 2 > paln) return -1;
                char *e; unsigned long v = strtoul(c->str, &e, 16);
                if (*e || v > 0xFFFF) return -1;
                out[at++] = (uint8_t)v; out[at++] = (uint8_t)(v >> 8);
            }
        }
    }
    return at == paln ? 0 : -1;
}

/* The sprites of one fighter archive's folder (above). On entry pics / tiles / pals hold the raw files;
   any of them that changes is replaced (the old buffer freed). Notes say which pictures were taken from
   their PNGs and which could not be, and why. */
static void sprites_from_pngs(const char *folder, uint8_t **pics, size_t *picsn, uint8_t **tiles, size_t *tilesn,
                              uint8_t *pals, size_t paln, int *pals_changed, PacImage *out) {
    char path[1200], why[256];
    snprintf(path, sizeof path, "%s/sprites/metadata.json", folder);
    long long tmeta = mtime_of(path);
    if (tmeta < 0) return;
    JVal *meta = json_load(path);
    if (!meta) { note(out, "sprites/metadata.json not used: not valid JSON"); return; }
    JVal *pw = jmember(meta, "pictures_written");
    long long since = pw && pw->type == J_NUM ? (long long)pw->num : tmeta;

    /* the colours */
    if (pals && paln) {
        uint8_t *c = malloc(paln);
        if (c && spr_colours(jmember(meta, "colours"), c, paln) == 0) {
            if (memcmp(c, pals, paln)) { memcpy(pals, c, paln); *pals_changed = 1; note(out, "palettes from sprites/metadata.json"); }
        } else if (c) note(out, "sprites/metadata.json: its colours do not fill the palettes entry (%zu bytes); not used", paln);
        free(c);
    }
    jfree(meta);

    SpriteSet s = { *pics, *tiles, *picsn, *tilesn, *picsn >= 4 ? rd32(*pics) / 12 : 0 };
    uint32_t ntiles = *tilesn >= 4 ? rd32(*tiles) / 4 : 0;
    uint32_t base_tiles = (ntiles + 3) & ~3u;     /* new cells start on a cell (4 tiles) */
    Buf newpics = { 0 }, newtiles = { 0 }, extra = { 0 };   /* extra: the new tiles' streams */
    uint32_t added = 0;                           /* tiles after base_tiles */
    uint32_t *new_offs = NULL; size_t new_offs_cap = 0;
    int changed = 0, refused = 0;
    if (buf_add(&newpics, *pics, *picsn)) goto done;

    for (uint32_t n = 0; n < s.count; n++) {
        uint32_t off; unsigned cols, rows;
        if (spr_picture(&s, n, &off, &cols, &rows) || !cols || !rows) continue;
        snprintf(path, sizeof path, "%s/sprites/pictures/%04u.png", folder, n);
        long long tp = mtime_of(path);
        if (tp < 0 || (!(tp > since) && !(asset_pac_png_always && tp >= 0))) continue;
        PngImage png;
        if (png_decode_file(path, &png, why, sizeof why)) { if (refused++ < 4) note(out, "sprite %04u.png not used: %s", n, why); continue; }
        int w, h; uint8_t *old;
        if (asset_sprite_picture(*pics, *picsn, *tiles, *tilesn, n, &old, &w, &h)) { png_free(&png); if (refused++ < 4) note(out, "sprite %04u: the archive's picture cannot be read", n); continue; }
        if (png.w != w || png.h != h) {
            if (refused++ < 4) note(out, "sprite %04u.png not used: it is %dx%d; the picture is %dx%d", n, png.w, png.h, w, h);
            png_free(&png); free(old); continue;
        }
        /* the old cells, for their palettes and blend bits */
        unsigned ncells = cols * rows;
        unsigned *ocell = calloc(ncells + 1, sizeof *ocell), *oflag = calloc(ncells + 1, sizeof *oflag);
        uint8_t *idx = malloc((size_t)w * h + 1);
        if (!ocell || !oflag || !idx) { free(ocell); free(oflag); free(idx); png_free(&png); free(old); goto done; }
        size_t p = off;
        for (unsigned i = 0; i < ncells; i++) {
            if (s.pics[p] == SPR_EMPTY) { ocell[i] = 0xFFFFFFFFu; p++; }
            else { ocell[i] = (unsigned)s.pics[p] << 8 | s.pics[p + 1]; oflag[i] = s.pics[p + 2]; p += 3; }
        }
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                size_t i = (size_t)y * w + x;
                if (png.index) idx[i] = (png.index[i] & 15) ? png.index[i] : 0;   /* colour 0 of any palette: transparent */
                else {
                    unsigned c = (unsigned)(y / SPR_CELL) * cols + (unsigned)(x / SPR_CELL);
                    idx[i] = spr_rgba_index(png.rgba + 4 * i, pals, paln / 6 ? paln / 6 : paln, ocell[c] == 0xFFFFFFFFu ? 0 : oflag[c] & 0x1F);
                }
            }
        png_free(&png);
        if (!memcmp(idx, old, (size_t)w * h)) { free(ocell); free(oflag); free(idx); free(old); continue; }

        /* the cells again */
        unsigned npal = paln / 6 / 32 ? (unsigned)(paln / 6 / 32) : 32;
        Buf cells = { 0 };
        int bad = 0;
        for (unsigned i = 0; i < ncells && !bad; i++) {
            int cx = (int)(i % cols) * SPR_CELL, cy = (int)(i / cols) * SPR_CELL;
            int pal = -1, same = 1;
            for (int y = 0; y < SPR_CELL; y++)
                for (int x = 0; x < SPR_CELL; x++) {
                    uint8_t v = idx[(size_t)(cy + y) * w + cx + x];
                    if (v != old[(size_t)(cy + y) * w + cx + x]) same = 0;
                    if (!v) continue;
                    if (pal < 0) pal = v >> 4;
                    else if (pal != v >> 4 && !bad) { bad = 1; snprintf(why, sizeof why, "cell %u, %u mixes palettes %d and %d (a cell has one)", i % cols, i / cols, pal, v >> 4); }
                }
            if (bad) break;
            if (pal < 0) { uint8_t e = SPR_EMPTY; buf_add(&cells, &e, 1); continue; }
            if ((unsigned)pal >= npal) { bad = 1; snprintf(why, sizeof why, "cell %u, %u uses palette %d; this fighter has %u", i % cols, i / cols, pal, npal); break; }
            if (same && ocell[i] != 0xFFFFFFFFu) { uint8_t c3[3] = { (uint8_t)(ocell[i] >> 8), (uint8_t)ocell[i], (uint8_t)oflag[i] }; buf_add(&cells, c3, 3); continue; }
            uint32_t cell = (base_tiles + added) / 4;
            if (cell >= 0xFF00u) { bad = 1; snprintf(why, sizeof why, "more than 65,280 cells in the tile set"); break; }
            for (unsigned q = 0; q < 4; q++) {
                uint8_t t[128], packed[160], back[160];
                int qx = (int)(q >> 1) * 16, qy = (int)(q & 1) * 16;
                for (int y = 0; y < 16; y++)
                    for (int x = 0; x < 16; x += 2) {
                        uint8_t a = idx[(size_t)(cy + qy + y) * w + cx + qx + x] & 15, b = idx[(size_t)(cy + qy + y) * w + cx + qx + x + 1] & 15;
                        t[y * 8 + x / 2] = (uint8_t)(a | b << 4);
                    }
                size_t len = tile_pack(t, packed);
                /* check it unpacks to the same bytes */
                uint8_t one[4 + 160]; wr32(one, 4); memcpy(one + 4, packed, len);
                if (tile_unpack(one, 4 + len, 0, back) || memcmp(back, t, 128)) { bad = 1; snprintf(why, sizeof why, "a tile did not pack (cell %u, %u)", i % cols, i / cols); break; }
                if (added >= new_offs_cap) {
                    size_t cap = new_offs_cap ? new_offs_cap * 2 : 256;
                    uint32_t *nt = realloc(new_offs, cap * sizeof *nt);
                    if (!nt) { bad = 1; snprintf(why, sizeof why, "out of memory"); break; }
                    new_offs = nt; new_offs_cap = cap;
                }
                new_offs[added++] = (uint32_t)extra.n;
                buf_add(&extra, packed, len);
            }
            if (bad) break;
            uint8_t c3[3] = { (uint8_t)(cell >> 8), (uint8_t)cell, (uint8_t)((unsigned)pal | (ocell[i] != 0xFFFFFFFFu ? oflag[i] & SPR_BLEND : 0)) };
            buf_add(&cells, c3, 3);
        }
        if (bad) {
            if (refused++ < 4) note(out, "sprite %04u.png not used: %s", n, why);
            /* its tiles were appended already: they stay, unused */
        } else {
            wr32(newpics.b + 12 * (size_t)n, (uint32_t)newpics.n);
            buf_add(&newpics, cells.b, cells.n);
            if (changed++ < 8) note(out, "sprite %04u from its PNG", n);
        }
        free(cells.b); free(ocell); free(oflag); free(idx); free(old);
    }
    if (changed > 8) note(out, "%d sprites from their PNGs in all", changed);
    if (refused > 4) note(out, "%d sprite PNGs not used in all", refused);
    if (!changed) goto done;

    /* the tile set: offsets (old ones moved by the longer table), the old streams, the new ones */
    {
        uint32_t total = base_tiles + added;
        size_t table = 4 * (size_t)total, old_table = 4 * (size_t)ntiles;
        newtiles.b = calloc(1, table + (*tilesn - old_table) + extra.n + 1);
        if (!newtiles.b) goto done;
        for (uint32_t t = 0; t < total; t++) {
            uint32_t v;
            if (t < ntiles) v = rd32(*tiles + 4 * (size_t)t) + (uint32_t)(table - old_table);
            else if (t < base_tiles) v = rd32(*tiles) + (uint32_t)(table - old_table);   /* padding to a whole cell: tile 0's stream */
            else v = (uint32_t)(table + (*tilesn - old_table) + new_offs[t - base_tiles]);
            wr32(newtiles.b + 4 * (size_t)t, v);
        }
        memcpy(newtiles.b + table, *tiles + old_table, *tilesn - old_table);
        memcpy(newtiles.b + table + (*tilesn - old_table), extra.b, extra.n);
        newtiles.n = table + (*tilesn - old_table) + extra.n;
    }
    free(*pics); *pics = newpics.b; *picsn = newpics.n; newpics.b = NULL;
    free(*tiles); *tiles = newtiles.b; *tilesn = newtiles.n; newtiles.b = NULL;
done:
    free(newpics.b); free(newtiles.b); free(extra.b); free(new_offs);
}

/* ---------------------------------------------------------------- building */
void pac_image_free(PacImage *img) { free(img->data); img->data = NULL; img->size = 0; }

/* An entry's type word: "0x10000203" as tools/pac.py writes it, or a plain number. */
static int entry_type(const JVal *v, uint32_t *type) {
    if (!v) return -1;
    if (v->type == J_NUM) { *type = (uint32_t)v->num; return 0; }
    if (v->type != J_STR || !v->str[0]) return -1;
    char *e;
    unsigned long t = strtoul(v->str, &e, 0);
    if (*e) return -1;
    *type = (uint32_t)t;
    return 0;
}

int asset_pac_build(const char *folder, PacImage *out, char *err, size_t errn) {
    out->data = NULL; out->size = 0; out->notes[0] = 0;
    char path[1024];
    snprintf(path, sizeof path, "%s/manifest.json", folder);
    JVal *root = json_load(path);
    if (!root) return fail(err, errn, "%s: missing or not valid JSON", path);
    JVal *entries = jmember(root, "entries");
    if (!entries || entries->type != J_ARR) { jfree(root); return fail(err, errn, "%s: no \"entries\" list", path); }

    uint32_t count = 0;
    for (JVal *e = entries->child; e; e = e->next) count++;
    if (count > PAC_MAX_ENTRIES) { jfree(root); return fail(err, errn, "%s: %u entries, at most %u fit the header", path, count, PAC_MAX_ENTRIES); }

    size_t cap = PAC_SECTOR, size = PAC_SECTOR;
    uint8_t *img = calloc(1, cap);
    if (!img) { jfree(root); return fail(err, errn, "out of memory"); }
    uint32_t i = 0, data_size = 0;

    /* a fighter archive with sprites/: its picture list, tile set and palettes from the PNGs first */
    uint8_t *made[PAC_MAX_ENTRIES] = { 0 };
    size_t made_n[PAC_MAX_ENTRIES] = { 0 };
    {
        int at[3] = { -1, -1, -1 };
        static const uint32_t kinds[3] = { SPR_PICTURES, SPR_TILES, SPR_PALETTES };
        uint32_t k = 0;
        for (JVal *e = entries->child; e; e = e->next, k++) {
            uint32_t type;
            if (entry_type(jmember(e, "type"), &type) != 0) continue;
            for (int q = 0; q < 3; q++) if (type == kinds[q] && at[q] < 0) at[q] = (int)k;
        }
        snprintf(path, sizeof path, "%s/sprites/metadata.json", folder);
        if (at[0] >= 0 && at[1] >= 0 && mtime_of(path) >= 0) {
            uint8_t *raw3[3] = { 0 }; size_t n3[3] = { 0 };
            for (int q = 0; q < 3; q++) {
                if (at[q] < 0) continue;
                k = 0;
                for (JVal *e = entries->child; e; e = e->next, k++) if ((int)k == at[q]) {
                    JVal *raw = jmember(e, "raw");
                    if (raw && raw->type == J_STR) { snprintf(path, sizeof path, "%s/%s", folder, raw->str); raw3[q] = read_file(path, &n3[q]); }
                }
            }
            if (raw3[0] && raw3[1]) {
                uint8_t *p0 = raw3[0], *p1 = raw3[1];
                int pals_changed = 0;
                sprites_from_pngs(folder, &raw3[0], &n3[0], &raw3[1], &n3[1], raw3[2], n3[2], &pals_changed, out);
                if (raw3[0] != p0) { made[at[0]] = raw3[0]; made_n[at[0]] = n3[0]; raw3[0] = NULL; }
                if (raw3[1] != p1) { made[at[1]] = raw3[1]; made_n[at[1]] = n3[1]; raw3[1] = NULL; }
                if (pals_changed) { made[at[2]] = raw3[2]; made_n[at[2]] = n3[2]; raw3[2] = NULL; }
            }
            for (int q = 0; q < 3; q++) free(raw3[q]);
        }
    }
#define FREE_MADE() do { for (uint32_t q_ = 0; q_ < count; q_++) free(made[q_]); } while (0)

    for (JVal *e = entries->child; e; e = e->next, i++) {
        uint32_t type;
        JVal *raw = jmember(e, "raw"), *pad = jmember(e, "padding");
        if (entry_type(jmember(e, "type"), &type) != 0) { FREE_MADE(); free(img); jfree(root); return fail(err, errn, "%s: entry %u: no valid \"type\"", path, i); }
        if (!raw || raw->type != J_STR) { FREE_MADE(); free(img); jfree(root); return fail(err, errn, "%s: entry %u: no \"raw\" file", path, i); }

        size_t dn, pn = 0;
        snprintf(path, sizeof path, "%s/%s", folder, raw->str);
        uint8_t *d = made[i];
        if (d) { dn = made_n[i]; made[i] = NULL; }
        else d = read_file(path, &dn);
        if (!d) { FREE_MADE(); free(img); jfree(root); return fail(err, errn, "%s: not found", path); }
        JVal *png = jmember(e, "png");
        uint32_t kind = type & 0xFFFF0000u;
        if (png && png->type == J_STR && png->str[0] && (kind == 0x10000000u || kind == 0x00800000u || kind == 0x04000000u)) {
            char png_path[1024], why[256];
            snprintf(png_path, sizeof png_path, "%s/%s", folder, png->str);
            long long tp = mtime_of(png_path), tr = mtime_of(path);
            if (tp > tr || (asset_pac_png_always && tp >= 0)) {                         /* edited after the raw file: the PNG wins */
                uint8_t *conv;
                uint16_t pal[256];
                int npal = parse_palette(jmember(e, "palette_colours"), pal, 256);   /* hex, 4 digits a colour */
                if (asset_png_to_entry(type, d, dn, png_path, npal ? pal : NULL, npal, &conv, why, sizeof why) == 0) {
                    if (memcmp(conv, d, dn)) note(out, "entry %u from %s", i, png->str);
                    free(d); d = conv;
                } else note(out, "entry %u: %s not used: %s", i, png->str, why);
            }
        }
        if (dn > 0x7FFFFFFFu - PAC_SECTOR) { FREE_MADE(); free(d); free(img); jfree(root); return fail(err, errn, "%s: too large", path); }
        uint8_t *p = NULL;
        if (pad && pad->type == J_STR && pad->str[0]) {
            snprintf(path, sizeof path, "%s/%s", folder, pad->str);
            p = read_file(path, &pn);   /* a missing .pad: zeros, as for any changed entry */
        }
        uint32_t aligned = align_sector((uint32_t)dn);
        if (size + aligned > cap) {
            while (size + aligned > cap) cap *= 2;
            uint8_t *t = realloc(img, cap);
            if (!t) { FREE_MADE(); free(d); free(p); free(img); jfree(root); return fail(err, errn, "out of memory"); }
            img = t;
        }
        memcpy(img + size, d, dn);
        /* the stale bytes after the entry, only where they still fill its last sector exactly (an
           unchanged entry); otherwise zeros: tools/pac.py build */
        if (p && dn + pn == aligned) memcpy(img + size + dn, p, pn);
        else memset(img + size + dn, 0, aligned - dn);
        size += aligned;
        data_size += aligned;
        wr32(img + 8 + 8 * i, type);
        wr32(img + 12 + 8 * i, (uint32_t)dn);
        free(d); free(p);
    }
#undef FREE_MADE
    wr32(img, count);
    wr32(img + 4, data_size);
    jfree(root);
    out->data = img;
    out->size = size;
    return 0;
}

static int parse_palette(const JVal *pc, uint16_t *pal, int max) {
    int n = 0;
    if (!pc || pc->type != J_STR) return 0;
    for (const char *q = pc->str; n < max && q[0] && q[1] && q[2] && q[3]; q += 4) {
        char hex[5] = { q[0], q[1], q[2], q[3], 0 };
        pal[n++] = (uint16_t)strtoul(hex, NULL, 16);
    }
    return n;
}

int asset_manifest_palette(const char *folder, int index, uint16_t *pal, int max) {
    char path[1024];
    snprintf(path, sizeof path, "%s/manifest.json", folder);
    JVal *root = json_load(path), *entries = jmember(root, "entries");
    int n = 0, i = 0;
    if (entries && entries->type == J_ARR)
        for (JVal *e = entries->child; e; e = e->next, i++) if (i == index) { n = parse_palette(jmember(e, "palette_colours"), pal, max); break; }
    jfree(root);
    return n;
}

/* ---------------------------------------------------------------- checking against the disc */
/* Kinds the loaders copy into a buffer that already exists (a table entry): growing one can write
   past it. Fighter data goes on the fighter heap and sound / voice into a buffer sized for it;
   textures into the staging buffer (checked separately). */
static int fixed_buffer_kind(uint32_t kind) {
    switch (kind) {
    case 0x00000000u: case 0x00010000u: case 0x00020000u: case 0x01000000u: case 0x02000000u:
    case 0x04000000u: case 0x08000000u: case 0x80000000u: return 1;
    default: return 0;
    }
}

static int header_ok(const uint8_t *b, size_t n, uint32_t *count) {
    if (n < PAC_SECTOR) return 0;
    uint32_t c = rd32(b), ds = rd32(b + 4);
    if (8 + 8 * (uint64_t)c > PAC_SECTOR || (uint64_t)PAC_SECTOR + ds != n) return 0;
    uint64_t off = PAC_SECTOR;
    for (uint32_t i = 0; i < c; i++) off += align_sector(rd32(b + 12 + 8 * i));
    if (off != n) return 0;
    *count = c;
    return 1;
}

typedef struct { char *p; size_t left; int lines; } Report;
static void say(Report *r, const char *fmt, ...) {
    if (r->left < 2) return;
    if (r->lines++) { *r->p++ = ';'; *r->p++ = ' '; *r->p = 0; r->left -= 2; }
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(r->p, r->left, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= r->left) n = (int)r->left - 1;
    r->p += n; r->left -= (size_t)n;
}

int asset_pac_check(const PacImage *img, const uint8_t *disc, size_t disc_size, char *report, size_t reportn) {
    Report r = { report, reportn, 0 };
    if (reportn) report[0] = 0;
    uint32_t count, dcount = 0;
    if (!header_ok(img->data, img->size, &count)) { say(&r, "the built archive's header does not match its contents"); return -1; }
    int have_disc = disc && header_ok(disc, disc_size, &dcount);
    int result = 0, shown = 0;
    uint32_t off = PAC_SECTOR, doff = PAC_SECTOR;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t type = rd32(img->data + 8 + 8 * i), size = rd32(img->data + 12 + 8 * i), kind = type & 0xFFFF0000u;
        int in_disc = have_disc && i < dcount;
        uint32_t dtype = in_disc ? rd32(disc + 8 + 8 * i) : 0, dsize = in_disc ? rd32(disc + 12 + 8 * i) : 0;
        if ((kind == 0x10000000u || kind == 0x00800000u) && size > PAC_TEXTURE_STAGING) {
            say(&r, "entry %u: a %u-byte texture does not fit the loaders' %u-byte staging buffer", i, size, PAC_TEXTURE_STAGING);
            return -1;
        }
        const char *what = NULL;
        if (!have_disc) what = NULL;
        else if (!in_disc) what = "added";
        else if (type != dtype) what = "type changed";
        else if (size != dsize || memcmp(img->data + off, disc + doff, size) != 0) what = "changed";
        if (what) {
            if (shown++ < 8) {
                if (in_disc && size != dsize) say(&r, "entry %u (%08X) %s, %u -> %u bytes", i, type, what, dsize, size);
                else say(&r, "entry %u (%08X) %s", i, type, what);
            }
            if (fixed_buffer_kind(kind) && (!in_disc || size > dsize)) {
                say(&r, "WARNING: entry %u has grown and the game copies this kind into an existing buffer, which it may overrun", i);
                result = 1;
            }
        }
        off += align_sector(size);
        if (in_disc) doff += align_sector(dsize);
    }
    if (shown > 8) say(&r, "and %d more", shown - 8);
    if (have_disc && dcount > count) say(&r, "%u entries removed", dcount - count);
    return result;
}
