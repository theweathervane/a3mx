/* Reading PNG files: see png_decode.h. Inflate after RFC 1951 (stored, fixed and dynamic Huffman
 * blocks), canonical codes decoded a bit at a time (the textures are small; simplicity wins). */
#include "png_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ---------------------------------------------------------------- inflate */
typedef struct {
    const uint8_t *src; size_t n, pos;   /* input, byte position */
    uint32_t bits; int nbits;            /* bit buffer, LSB first */
    uint8_t *out; size_t outn, cap;
    int error;
} Inflate;

typedef struct { uint16_t count[16]; uint16_t symbol[288]; } Huffman;

static int getbit(Inflate *s) {
    if (!s->nbits) {
        if (s->pos >= s->n) { s->error = 1; return 0; }
        s->bits = s->src[s->pos++]; s->nbits = 8;
    }
    int b = s->bits & 1; s->bits >>= 1; s->nbits--;
    return b;
}
static uint32_t getbits(Inflate *s, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) v |= (uint32_t)getbit(s) << i;
    return v;
}
static int put(Inflate *s, uint8_t b) {
    if (s->outn == s->cap) {
        size_t cap = s->cap ? s->cap * 2 : 1 << 16;
        uint8_t *t = realloc(s->out, cap);
        if (!t) { s->error = 1; return -1; }
        s->out = t; s->cap = cap;
    }
    s->out[s->outn++] = b;
    return 0;
}
/* Canonical code from code lengths; -1 if over-subscribed. */
static int build(Huffman *h, const uint8_t *lengths, int n) {
    uint16_t offs[16];
    memset(h->count, 0, sizeof h->count);
    for (int i = 0; i < n; i++) h->count[lengths[i]]++;
    h->count[0] = 0;
    int left = 1;
    for (int len = 1; len < 16; len++) { left <<= 1; left -= h->count[len]; if (left < 0) return -1; }
    offs[1] = 0;
    for (int len = 1; len < 15; len++) offs[len + 1] = (uint16_t)(offs[len] + h->count[len]);
    for (int i = 0; i < n; i++) if (lengths[i]) h->symbol[offs[lengths[i]]++] = (uint16_t)i;
    return 0;
}
static int decode(Inflate *s, const Huffman *h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 16; len++) {
        code |= getbit(s);
        int count = h->count[len];
        if (code - count < first) return h->symbol[index + (code - first)];
        index += count; first += count; first <<= 1; code <<= 1;
        if (s->error) return -1;
    }
    s->error = 1;
    return -1;
}

static const uint16_t LEN_BASE[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
static const uint8_t  LEN_EXTRA[29] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
static const uint16_t DIST_BASE[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
static const uint8_t  DIST_EXTRA[30] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

static int codes(Inflate *s, const Huffman *lit, const Huffman *dist) {
    for (;;) {
        int sym = decode(s, lit);
        if (s->error || sym < 0) return -1;
        if (sym < 256) { if (put(s, (uint8_t)sym)) return -1; continue; }
        if (sym == 256) return 0;
        sym -= 257;
        if (sym >= 29) return -1;
        size_t len = LEN_BASE[sym] + getbits(s, LEN_EXTRA[sym]);
        int ds = decode(s, dist);
        if (s->error || ds < 0 || ds >= 30) return -1;
        size_t d = DIST_BASE[ds] + getbits(s, DIST_EXTRA[ds]);
        if (s->error || d > s->outn) return -1;
        for (size_t i = 0; i < len; i++) if (put(s, s->out[s->outn - d])) return -1;
    }
}

static int block_stored(Inflate *s) {
    s->bits = 0; s->nbits = 0;                    /* to the byte boundary */
    if (s->pos + 4 > s->n) return -1;
    unsigned len = s->src[s->pos] | s->src[s->pos + 1] << 8, nlen = s->src[s->pos + 2] | s->src[s->pos + 3] << 8;
    s->pos += 4;
    if ((len ^ 0xFFFF) != nlen || s->pos + len > s->n) return -1;
    for (unsigned i = 0; i < len; i++) if (put(s, s->src[s->pos + i])) return -1;
    s->pos += len;
    return 0;
}

static int block_fixed(Inflate *s) {
    static Huffman lit, dist; static int ready;
    if (!ready) {
        uint8_t l[288];
        int i = 0;
        for (; i < 144; i++) l[i] = 8;
        for (; i < 256; i++) l[i] = 9;
        for (; i < 280; i++) l[i] = 7;
        for (; i < 288; i++) l[i] = 8;
        build(&lit, l, 288);
        for (i = 0; i < 30; i++) l[i] = 5;
        build(&dist, l, 30);
        ready = 1;
    }
    return codes(s, &lit, &dist);
}

static int block_dynamic(Inflate *s) {
    static const uint8_t ORDER[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
    int nlen = (int)getbits(s, 5) + 257, ndist = (int)getbits(s, 5) + 1, ncode = (int)getbits(s, 4) + 4;
    if (nlen > 286 || ndist > 30) return -1;
    uint8_t lengths[320] = { 0 };
    for (int i = 0; i < ncode; i++) lengths[ORDER[i]] = (uint8_t)getbits(s, 3);
    Huffman lencode, lit, dist;
    if (build(&lencode, lengths, 19)) return -1;
    int i = 0;
    while (i < nlen + ndist) {
        int sym = decode(s, &lencode);
        if (s->error || sym < 0) return -1;
        if (sym < 16) { lengths[i++] = (uint8_t)sym; continue; }
        int len = 0, rep;
        if (sym == 16) { if (!i) return -1; len = lengths[i - 1]; rep = 3 + (int)getbits(s, 2); }
        else if (sym == 17) rep = 3 + (int)getbits(s, 3);
        else rep = 11 + (int)getbits(s, 7);
        if (i + rep > nlen + ndist) return -1;
        while (rep--) lengths[i++] = (uint8_t)len;
    }
    if (!lengths[256]) return -1;
    if (build(&lit, lengths, nlen) || build(&dist, lengths + nlen, ndist)) return -1;
    return codes(s, &lit, &dist);
}

int zlib_inflate(const uint8_t *src, size_t n, uint8_t **out, size_t *outn) {
    *out = NULL; *outn = 0;
    if (n < 2 || (src[0] & 0x0F) != 8 || ((src[0] << 8) | src[1]) % 31 != 0 || (src[1] & 0x20)) return -1;
    Inflate s = { src, n, 2, 0, 0, NULL, 0, 0, 0 };
    int last;
    do {
        last = getbit(&s);
        int type = (int)getbits(&s, 2), r;
        if (s.error) break;
        if (type == 0) r = block_stored(&s);
        else if (type == 1) r = block_fixed(&s);
        else if (type == 2) r = block_dynamic(&s);
        else r = -1;
        if (r || s.error) { free(s.out); return -1; }
    } while (!last);
    if (s.error) { free(s.out); return -1; }
    *out = s.out; *outn = s.outn;
    return 0;
}

/* ---------------------------------------------------------------- PNG */
static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static int fail(char *err, size_t errn, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    if (err && errn) vsnprintf(err, errn, fmt, ap);
    va_end(ap);
    return -1;
}

void png_free(PngImage *img) { free(img->rgba); free(img->index); img->rgba = img->index = NULL; }

static int paeth(int a, int b, int c) {
    int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

int png_decode(const uint8_t *d, size_t n, PngImage *out, char *err, size_t errn) {
    static const uint8_t SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    memset(out, 0, sizeof *out);
    if (n < 8 || memcmp(d, SIG, 8)) return fail(err, errn, "not a PNG file");
    uint32_t w = 0, h = 0;
    int depth = 0, ctype = -1, interlace = 0;
    uint8_t plte[256][4];
    int nplte = 0;
    uint8_t trns_grey[2] = { 0 }, trns_rgb[6] = { 0 }; int has_trns = 0;
    for (int i = 0; i < 256; i++) { plte[i][0] = plte[i][1] = plte[i][2] = 0; plte[i][3] = 255; }
    uint8_t *idat = NULL; size_t idatn = 0;
    size_t pos = 8;
    int seen_end = 0;
    while (pos + 12 <= n) {
        uint32_t len = be32(d + pos);
        const uint8_t *type = d + pos + 4, *body = d + pos + 8;
        if (len > n - pos - 12) { free(idat); return fail(err, errn, "PNG chunk runs past the end of the file"); }
        if (!memcmp(type, "IHDR", 4) && len >= 13) {
            w = be32(body); h = be32(body + 4); depth = body[8]; ctype = body[9]; interlace = body[12];
        } else if (!memcmp(type, "PLTE", 4)) {
            nplte = (int)(len / 3 > 256 ? 256 : len / 3);
            for (int i = 0; i < nplte; i++) { plte[i][0] = body[3 * i]; plte[i][1] = body[3 * i + 1]; plte[i][2] = body[3 * i + 2]; }
        } else if (!memcmp(type, "tRNS", 4)) {
            has_trns = 1;
            if (ctype == 3) for (uint32_t i = 0; i < len && i < 256; i++) plte[i][3] = body[i];
            else if (ctype == 0 && len >= 2) memcpy(trns_grey, body, 2);
            else if (ctype == 2 && len >= 6) memcpy(trns_rgb, body, 6);
        } else if (!memcmp(type, "IDAT", 4)) {
            uint8_t *t = realloc(idat, idatn + len + 1);
            if (!t) { free(idat); return fail(err, errn, "out of memory"); }
            idat = t; memcpy(idat + idatn, body, len); idatn += len;
        } else if (!memcmp(type, "IEND", 4)) { seen_end = 1; break; }
        pos += 12 + len;
    }
    (void)seen_end;
    if (ctype < 0 || !w || !h) { free(idat); return fail(err, errn, "PNG without a header"); }
    if (w > 16384 || h > 16384) { free(idat); return fail(err, errn, "PNG too large (%ux%u)", w, h); }
    if (interlace) { free(idat); return fail(err, errn, "interlaced PNG: save it without interlacing"); }
    int channels = ctype == 0 ? 1 : ctype == 2 ? 3 : ctype == 3 ? 1 : ctype == 4 ? 2 : ctype == 6 ? 4 : 0;
    int depth_ok = depth == 8 || depth == 16 || ((ctype == 0 || ctype == 3) && (depth == 1 || depth == 2 || depth == 4));
    if (!channels || !depth_ok || (ctype == 3 && depth == 16)) { free(idat); return fail(err, errn, "PNG colour type %d at %d bits is not valid", ctype, depth); }
    if (ctype == 3 && !nplte) { free(idat); return fail(err, errn, "palette PNG without a palette"); }
    uint8_t *raw; size_t rawn;
    if (!idat || zlib_inflate(idat, idatn, &raw, &rawn)) { free(idat); return fail(err, errn, "PNG image data is damaged"); }
    free(idat);

    size_t bpp = (size_t)(channels * depth + 7) / 8;              /* bytes per pixel, for the filters */
    size_t stride = ((size_t)w * channels * depth + 7) / 8;
    if (rawn < (stride + 1) * h) { free(raw); return fail(err, errn, "PNG image data is short"); }
    uint8_t *prev = calloc(1, stride), *cur;
    for (uint32_t y = 0; y < h; y++) {
        uint8_t ft = raw[y * (stride + 1)];
        cur = raw + y * (stride + 1) + 1;
        for (size_t x = 0; x < stride; x++) {
            int a = x >= bpp ? cur[x - bpp] : 0, b = prev[x], c = x >= bpp ? prev[x - bpp] : 0;
            switch (ft) {
            case 0: break;
            case 1: cur[x] = (uint8_t)(cur[x] + a); break;
            case 2: cur[x] = (uint8_t)(cur[x] + b); break;
            case 3: cur[x] = (uint8_t)(cur[x] + ((a + b) >> 1)); break;
            case 4: cur[x] = (uint8_t)(cur[x] + paeth(a, b, c)); break;
            default: free(prev); free(raw); return fail(err, errn, "PNG row filter %d is not valid", ft);
            }
        }
        memcpy(prev, cur, stride);
    }
    free(prev);

    out->w = (int)w; out->h = (int)h; out->color_type = ctype;
    out->npalette = ctype == 3 ? nplte : 0;
    if (ctype == 3) memcpy(out->palette, plte, sizeof out->palette);
    out->rgba = malloc((size_t)w * h * 4);
    if (ctype == 3) out->index = malloc((size_t)w * h);
    if (!out->rgba || (ctype == 3 && !out->index)) { free(raw); png_free(out); return fail(err, errn, "out of memory"); }
    for (uint32_t y = 0; y < h; y++) {
        const uint8_t *row = raw + y * (stride + 1) + 1;
        for (uint32_t x = 0; x < w; x++) {
            uint8_t *px = out->rgba + ((size_t)y * w + x) * 4;
            unsigned v[4];       /* the channels, as read (16-bit samples kept whole for tRNS) */
            for (int ch = 0; ch < channels; ch++) {
                if (depth == 16) v[ch] = (unsigned)row[(x * channels + ch) * 2] << 8 | row[(x * channels + ch) * 2 + 1];
                else if (depth == 8) v[ch] = row[x * channels + ch];
                else { size_t bit = (size_t)x * depth; v[ch] = (row[bit >> 3] >> (8 - depth - (bit & 7))) & ((1u << depth) - 1); }
            }
            unsigned to8 = depth == 16 ? 8 : 0;
            switch (ctype) {
            case 3: {
                unsigned i = v[0];
                out->index[(size_t)y * w + x] = (uint8_t)i;
                memcpy(px, plte[i], 4);
                break;
            }
            case 0: case 4: {
                unsigned g = depth < 8 ? v[0] * 255 / ((1u << depth) - 1) : v[0] >> to8;
                px[0] = px[1] = px[2] = (uint8_t)g;
                if (ctype == 4) px[3] = (uint8_t)(v[1] >> to8);
                else px[3] = has_trns && v[0] == (unsigned)(trns_grey[0] << 8 | trns_grey[1]) ? 0 : 255;
                break;
            }
            case 2: case 6:
                for (int ch = 0; ch < 3; ch++) px[ch] = (uint8_t)(v[ch] >> to8);
                if (ctype == 6) px[3] = (uint8_t)(v[3] >> to8);
                else px[3] = has_trns && v[0] == (unsigned)(trns_rgb[0] << 8 | trns_rgb[1]) && v[1] == (unsigned)(trns_rgb[2] << 8 | trns_rgb[3])
                             && v[2] == (unsigned)(trns_rgb[4] << 8 | trns_rgb[5]) ? 0 : 255;
                break;
            }
        }
    }
    free(raw);
    return 0;
}

int png_decode_file(const char *path, PngImage *out, char *err, size_t errn) {
    memset(out, 0, sizeof *out);
    FILE *f = fopen(path, "rb");
    if (!f) return fail(err, errn, "%s: not found", path);
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = n > 0 ? malloc((size_t)n) : NULL;
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(b); return fail(err, errn, "%s: cannot read", path); }
    fclose(f);
    int r = png_decode(b, (size_t)n, out, err, errn);
    free(b);
    return r;
}
