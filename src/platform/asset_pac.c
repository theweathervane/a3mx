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
    for (JVal *e = entries->child; e; e = e->next, i++) {
        uint32_t type;
        JVal *raw = jmember(e, "raw"), *pad = jmember(e, "padding");
        if (entry_type(jmember(e, "type"), &type) != 0) { free(img); jfree(root); return fail(err, errn, "%s: entry %u: no valid \"type\"", path, i); }
        if (!raw || raw->type != J_STR) { free(img); jfree(root); return fail(err, errn, "%s: entry %u: no \"raw\" file", path, i); }

        size_t dn, pn = 0;
        snprintf(path, sizeof path, "%s/%s", folder, raw->str);
        uint8_t *d = read_file(path, &dn);
        if (!d) { free(img); jfree(root); return fail(err, errn, "%s: not found", path); }
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
        if (dn > 0x7FFFFFFFu - PAC_SECTOR) { free(d); free(img); jfree(root); return fail(err, errn, "%s: too large", path); }
        uint8_t *p = NULL;
        if (pad && pad->type == J_STR && pad->str[0]) {
            snprintf(path, sizeof path, "%s/%s", folder, pad->str);
            p = read_file(path, &pn);   /* a missing .pad: zeros, as for any changed entry */
        }
        uint32_t aligned = align_sector((uint32_t)dn);
        if (size + aligned > cap) {
            while (size + aligned > cap) cap *= 2;
            uint8_t *t = realloc(img, cap);
            if (!t) { free(d); free(p); free(img); jfree(root); return fail(err, errn, "out of memory"); }
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
