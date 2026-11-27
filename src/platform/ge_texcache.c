/* The GPU renderers' texture cache: ge_texcache.h says what and why. */
#include "rt.h"
#include "ge.h"
#include "ge_texcache.h"
#include "perf.h"
#include <string.h>
#include <stdlib.h>

static GeTexOps ops;
static uint64_t frame_no;
static uint8_t decode_buf[1024 * 1024 * 4];

#define TEX_CACHE 1024
static GeTexEntry tcache[TEX_CACHE];
static struct { int valid; uint32_t addr, bytes, fmt, clutfmt, size, mode; uint64_t clut_hash; GeTexEntry *e; } last_tex;

#define SMALL_TEX 0x8000u
#define BIG_CACHE 64
#define BIG_SHADOW_MAX (48u << 20)      /* bytes of copies kept at most */
typedef struct {
    GeTexEntry te;                      /* te.tex, w, h, last_used as for a small texture */
    uint32_t addr, bytes, fmt, size, mode, stride;
    uint64_t clut;
    uint8_t *shadow;                    /* the guest bytes the texture was decoded from */
    uint64_t *band_checked;             /* list number each band was last compared in */
    int nbands;
    uint32_t band_bytes;
} BigTex;
static BigTex big[BIG_CACHE];
static uint32_t big_shadow_total;
static uint64_t list_no = 1;            /* bumped whenever guest memory may have changed */

void ge_texcache_init(const GeTexOps *o) { ops = *o; }
void ge_texcache_memory_changed(void) { last_tex.valid = 0; list_no++; }
void ge_texcache_forget_last(void) { last_tex.valid = 0; }

static void big_free(BigTex *b) {
    ops.before_change();                /* the pending draws may still read it */
    if (b->te.tex) ops.destroy(b->te.tex);
    big_shadow_total -= b->shadow ? b->bytes : 0;
    free(b->shadow); free(b->band_checked);
    memset(b, 0, sizeof *b);
}

static void upload_rows(const GeTexRef *tx, GeTexEntry *e, int y0, int y1, int whole) {
    ops.before_change();                /* the pending draws must see the texture as it was */
    tx->decode_rows(decode_buf, y0, y1);
    ops.upload(e->tex, decode_buf, e->w, e->h, y0, y1, whole);
}

/* a large texture: rows v0 .. v1 - 1 are the ones this draw reads */
static GeTexEntry *big_lookup(const GeTexRef *tx, uint32_t addr, uint32_t bytes, uint64_t ch, int w, int h, int v0, int v1) {
    uint32_t fmt = tx->fmt, size = tx->size, mode = tx->mode & 1, stride = (uint32_t)tx->stride;
    BigTex *b = NULL;
    for (int i = 0; i < BIG_CACHE && !b; i++) {
        BigTex *c = &big[i];
        if (c->shadow && c->addr == addr && c->bytes == bytes && c->fmt == fmt && c->size == size && c->mode == mode &&
            c->stride == stride && c->clut == ch) b = c;
    }
    const uint8_t *guest = (const uint8_t *)HOST(addr);
    if (!b) {
        for (;;) {                                  /* a free slot within the copies' budget, dropping the least recently used */
            BigTex *lru = NULL, *empty = NULL;
            for (int i = 0; i < BIG_CACHE; i++) {
                if (!big[i].shadow) { if (!empty) empty = &big[i]; }
                else if (!lru || big[i].te.last_used < lru->te.last_used) lru = &big[i];
            }
            if (empty && big_shadow_total + bytes <= BIG_SHADOW_MAX) { b = empty; break; }
            if (!lru) { b = empty; break; }
            big_free(lru);
        }
        if (!b) return NULL;
        b->addr = addr; b->bytes = bytes; b->fmt = fmt; b->size = size; b->mode = mode; b->stride = stride; b->clut = ch;
        b->shadow = (uint8_t *)malloc(bytes);
        b->band_bytes = bytes / (uint32_t)h * 8u;                     /* 8 rows: also one row of swizzled blocks */
        b->nbands = (h + 7) / 8;
        b->band_checked = (uint64_t *)calloc((size_t)b->nbands, sizeof(uint64_t));
        if (!b->shadow || !b->band_checked) { free(b->shadow); free(b->band_checked); memset(b, 0, sizeof *b); return NULL; }
        big_shadow_total += bytes;
        memcpy(b->shadow, guest, bytes);
        for (int i = 0; i < b->nbands; i++) b->band_checked[i] = list_no;
        b->te.tex = ops.create();
        b->te.w = w; b->te.h = h;
        upload_rows(tx, &b->te, 0, h, 1);
        perf_count_texture(1, bytes);
    } else {
        int b0 = v0 / 8, b1 = (v1 + 7) / 8;
        if (b0 < 0) b0 = 0;
        if (b1 > b->nbands) b1 = b->nbands;
        uint32_t compared = 0;
        int run = -1;                                                 /* first band of a run that changed */
        for (int i = b0; i <= b1; i++) {
            int changed = 0;
            if (i < b1 && b->band_checked[i] != list_no) {
                uint32_t off = (uint32_t)i * b->band_bytes, n = b->band_bytes;
                if (off + n > bytes) n = bytes - off;
                compared += n;
                b->band_checked[i] = list_no;
                if (memcmp(b->shadow + off, guest + off, n)) { memcpy(b->shadow + off, guest + off, n); changed = 1; }
            }
            if (changed && run < 0) run = i;
            if (!changed && run >= 0) {
                int y1 = i * 8 > h ? h : i * 8;
                upload_rows(tx, &b->te, run * 8, y1, 0);
                perf_count_texture(1, 0);
                run = -1;
            }
        }
        perf_count_texture(0, compared);
    }
    b->te.last_used = frame_no;
    return &b->te;
}

GeTexEntry *ge_texcache_lookup(const GeTexRef *tx, int v0, int v1) {
    uint32_t addr = tx->addr, bytes = tx->bytes;
    uint64_t ch = tx->clut_hash;
    int w = tx->w, h = tx->h;
    if (w > 1024 || h > 1024 || bytes > 0x200000) return NULL;
    if (bytes > SMALL_TEX && h >= 8) return big_lookup(tx, addr, bytes, ch, w, h, v0, v1);
    if (last_tex.valid && last_tex.addr == addr && last_tex.bytes == bytes && last_tex.fmt == tx->fmt && last_tex.size == tx->size &&
        last_tex.mode == tx->mode && last_tex.clut_hash == ch) { last_tex.e->last_used = frame_no; return last_tex.e; }
    uint64_t key = ge_hash(HOST(addr), bytes) ^ (ch * 31) ^ ((uint64_t)tx->fmt << 40) ^ ((uint64_t)tx->size << 48) ^
                   ((uint64_t)(tx->mode & 1) << 63) ^ ((uint64_t)addr << 8) ^ ((uint64_t)tx->stride << 20);
    GeTexEntry *e = NULL, *victim = &tcache[0];
    for (int i = 0; i < TEX_CACHE; i++) {
        if (tcache[i].tex && tcache[i].key == key) { e = &tcache[i]; break; }
        if (!tcache[i].tex) { victim = &tcache[i]; }
        else if (victim->tex && tcache[i].last_used < victim->last_used) victim = &tcache[i];
    }
    perf_count_texture(!e, bytes);
    if (!e) {
        e = victim;
        if (!e->tex) e->tex = ops.create();
        e->w = w; e->h = h;
        upload_rows(tx, e, 0, h, 1);
        e->key = key;
    }
    e->last_used = frame_no;
    last_tex.valid = 1; last_tex.addr = addr; last_tex.bytes = bytes; last_tex.fmt = tx->fmt; last_tex.size = tx->size;
    last_tex.mode = tx->mode; last_tex.clut_hash = ch; last_tex.e = e;
    return e;
}

void ge_texcache_frame_end(void) {
    frame_no++;
    /* free textures unused for 10 seconds */
    if (frame_no % 600 == 0)
        for (int i = 0; i < TEX_CACHE; i++)
            if (tcache[i].tex && frame_no - tcache[i].last_used > 600) { ops.destroy(tcache[i].tex); tcache[i].tex = 0; tcache[i].key = 0; }
    if (frame_no % 600 == 0)
        for (int i = 0; i < BIG_CACHE; i++)
            if (big[i].shadow && frame_no - big[i].te.last_used > 600) big_free(&big[i]);
}
