// Decode an ATRAC3 / ATRAC3+ RIFF file to 16-bit stereo PCM as it is played, using the vendored FFmpeg-derived
// decoder (third_party/at3_standalone). Offsets follow the PSP's rules (see hle_atrac.c).
#include "atrac_decode.h"
#include "at3_decoders.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

extern "C" int atrac_parse(const uint8_t *f, uint32_t size, AtracTrack *t) {
    memset(t, 0, sizeof *t);
    t->loop_start = t->loop_end = -1;
    if (size < 72 || memcmp(f, "RIFF", 4) || memcmp(f + 8, "WAVE", 4)) return -1;
    uint32_t off = 12, data_size = 0;
    int adjust = 0, have_loop = 0; uint32_t ls = 0, le = 0;
    while (off + 8 <= size) {
        uint32_t id = rd32(f + off), n = rd32(f + off + 4);
        off += 8;
        if (!memcmp(f + off - 8, "fmt ", 4)) {
            uint16_t tag = rd16(f + off);
            t->codec = tag == 0xFFFE ? 0x1000 : tag == 0x0270 ? 0x1001 : 0;
            t->channels = rd16(f + off + 2);
            t->block_align = rd16(f + off + 12);
            t->extra_off = off + 16; t->extra_size = n > 16 ? n - 16 : 0;
        } else if (!memcmp(f + off - 8, "fact", 4)) {
            t->total_samples = (int)rd32(f + off);
            if (n >= 8) t->first_offset = (int)rd32(f + off + 4);
            if (n >= 12) adjust = t->first_offset - (int)rd32(f + off + 8);
        } else if (!memcmp(f + off - 8, "smpl", 4)) {
            if (n >= 36 + 24 && rd32(f + off + 28) > 0) { have_loop = 1; ls = rd32(f + off + 36 + 8); le = rd32(f + off + 36 + 12); }
        } else if (!memcmp(f + off - 8, "data", 4)) {
            t->data_off = off; data_size = n; break;
        }
        (void)id;
        off += n + (n & 1);
    }
    if (!t->codec || !t->data_off || !t->block_align) return -1;
    if (t->data_off + data_size > size) data_size = size - t->data_off;
    t->data_size = data_size;
    t->skip = t->first_offset + (t->codec == 0x1000 ? 0x170 : 0x45);       /* decoded samples before logical 0 */
    if (t->total_samples <= 0) t->total_samples = (int)(data_size / t->block_align) * (t->codec == 0x1000 ? 2048 : 1024) - t->skip;
    if (have_loop) { t->loop_start = (int)ls + adjust - t->first_offset; t->loop_end = (int)le + adjust - t->first_offset; }
    return 0;
}

/* Decoded as it is played (0.179.1), frame after frame in order through one decoder, so the samples are
   the ones a decode of the whole file gives, and kept, so a loop back reads them again. Until 0.179.0
   the whole file was decoded when the game handed it over: 15 - 65 ms on the game's thread for a
   fight's music, a hitch as the fight started. */
struct AtracStream {
    uint8_t *file;                  /* a copy of what the game handed over (it may reuse its buffer) */
    AtracTrack t;
    int ch, frames, per, next;      /* next: the next frame to decode */
    long cap, pos;                  /* samples the buffer holds at most; decoded so far */
    int16_t *pcm;                   /* stereo s16 */
    ATRAC3PContext *p3; ATRAC3Context *a3;
};

extern "C" AtracStream *atrac_stream_open(const uint8_t *f, uint32_t size, const AtracTrack *t) {
    AtracStream *s = (AtracStream *)calloc(1, sizeof *s);
    if (!s) return nullptr;
    s->t = *t;
    s->ch = t->channels ? t->channels : 2;
    int block = t->block_align;
    s->frames = (int)(t->data_size / (uint32_t)block);
    s->per = t->codec == 0x1000 ? 2048 : 1024;
    s->cap = (long)s->frames * s->per;
    s->file = (uint8_t *)malloc(size ? size : 1);
    s->pcm = (int16_t *)calloc((size_t)s->cap + s->per, 4);
    if (!s->file || !s->pcm) { free(s->file); free(s->pcm); free(s); return nullptr; }
    memcpy(s->file, f, size);
    if (t->codec == 0x1000) s->p3 = atrac3p_alloc(s->ch, &block);
    else s->a3 = atrac3_alloc(s->ch, &block, s->file + t->extra_off, (int)t->extra_size);
    return s;
}

/* decodes until sample `upto` (exclusive) is there or the file ends; returns the samples decoded */
extern "C" long atrac_stream_decode(AtracStream *s, long upto) {
    float l[2048], r[2048]; float *planes[2] = { l, r };
    const AtracTrack *t = &s->t;
    while (s->pos < upto && s->next < s->frames) {
        int n = 0;
        const uint8_t *src = s->file + t->data_off + (uint32_t)s->next * (uint32_t)t->block_align;
        int ok = s->p3 ? atrac3p_decode_frame(s->p3, planes, &n, src, t->block_align) : atrac3_decode_frame(s->a3, planes, &n, src, t->block_align);
        if (ok < 0) n = s->per, memset(l, 0, sizeof l), memset(r, 0, sizeof r);
        for (int k = 0; k < n && s->pos < s->cap; k++, s->pos++) {
            float a = l[k], b = s->ch > 1 ? r[k] : l[k];
            int x = (int)lrintf(a * 32767.f), y = (int)lrintf(b * 32767.f);
            s->pcm[2 * s->pos] = (int16_t)(x < -32768 ? -32768 : x > 32767 ? 32767 : x);
            s->pcm[2 * s->pos + 1] = (int16_t)(y < -32768 ? -32768 : y > 32767 ? 32767 : y);
        }
        s->next++;
    }
    return s->pos;
}

extern "C" const int16_t *atrac_stream_pcm(const AtracStream *s) { return s->pcm; }

extern "C" void atrac_stream_close(AtracStream *s) {
    if (!s) return;
    if (s->p3) atrac3p_free(s->p3);
    if (s->a3) atrac3_free(s->a3);
    free(s->file); free(s->pcm); free(s);
}
