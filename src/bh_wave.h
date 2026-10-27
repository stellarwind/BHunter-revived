#ifndef BH_WAVE_H
#define BH_WAVE_H
#include "bh_scene.h"

typedef enum BHWaveError
{
    BH_WAVE_OK,
    BH_WAVE_ARGUMENT,
    BH_WAVE_TRUNCATED,
    BH_WAVE_FORMAT,
    BH_WAVE_UNSUPPORTED,
    BH_WAVE_LIMIT,
    BH_WAVE_MEMORY,
    BH_WAVE_IO
} BHWaveError;
/* Borrowed view; bytes must outlive the view. Scan validates the complete RIFF
   extent and padding, retains the first fmt/data chunks, skips unknown chunks.
   It does not validate a codec. The original scanner accepts fmt >=14 bytes. */
typedef struct BHWaveView
{
    const unsigned char *format, *samples;
    unsigned long format_size, sample_bytes, riff_bytes;
    BHWaveError error;
} BHWaveView;
typedef struct BHWave
{
    unsigned char *samples;
    unsigned long sample_bytes, frames, sample_rate, byte_rate;
    unsigned short format_tag, channels, block_align, bits;
    BHWaveError error;
    BHSceneAllocator allocator;
} BHWave;
int __cdecl bh_wave_scan(BHWaveView *view, const void *bytes, unsigned long size);
/* Initialize fresh owner; successful PCM decode replaces old data. Failure
   retains old samples/metadata. Input must not overlap owned samples.
   Supported PCM: 1/2 channels, 8/16 bits; no resampling or amplitude changes.
   Max input 64 MiB, max sample rate 192000. Empty PCM data is supported. */
int __cdecl bh_wave_init(BHWave *wave, const BHSceneAllocator *allocator);
void __cdecl bh_wave_dispose(BHWave *wave);
int __cdecl bh_wave_decode(BHWave *wave, const void *bytes, unsigned long size);
/* Borrowed VFILE: read remaining payload; success at end, failure attempts
   cursor rollback. Caller owns handle/cache lifetime and serialized access. */
int __cdecl bh_wave_read(BHWave *wave, BHVFile *file);
#endif
