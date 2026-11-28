/* ATRAC3/ATRAC3+ decoding (C interface to the vendored C++ decoder). */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int codec;            /* 0x1000 ATRAC3+, 0x1001 ATRAC3 */
    int channels, block_align;
    uint32_t data_off, data_size, extra_off, extra_size;
    int total_samples;    /* logical samples (fact chunk) */
    int first_offset;     /* fact chunk's first-sample offset */
    int skip;             /* decoded samples before logical sample 0 */
    int loop_start, loop_end;   /* logical samples, -1 if none */
} AtracTrack;
int      atrac_parse(const uint8_t *file, uint32_t size, AtracTrack *t);
/* decoding as the samples are wanted (atrac_decode.cpp): open copies the file */
typedef struct AtracStream AtracStream;
AtracStream   *atrac_stream_open(const uint8_t *file, uint32_t size, const AtracTrack *t);
long           atrac_stream_decode(AtracStream *s, long upto);   /* samples decoded (up to `upto`, unless the file ends first) */
const int16_t *atrac_stream_pcm(const AtracStream *s);           /* stereo s16, as far as decoded */
void           atrac_stream_close(AtracStream *s);
#ifdef __cplusplus
}
#endif
