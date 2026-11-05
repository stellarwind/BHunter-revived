#include "bh_sound_load.h"
#include "bh_vfile.h"
#include "bh_riff.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static unsigned long fail(BHSoundLoad *c, BHSoundLoadError error)
{
    if (c)
    {
        c->error = error;
        ++c->failures;
    }
    return BH_SOUND_INVALID;
}
static unsigned long u32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}
static unsigned short u16(const unsigned char *p)
{
    return (unsigned short)(p[0] | ((unsigned short)p[1] << 8));
}
static void *allocate(BHSoundLoad *c, unsigned long bytes)
{
    ++c->allocations;
    return c->file_allocator.allocate ? c->file_allocator.allocate(bytes, c->file_allocator.user) : malloc(bytes);
}
static void release(BHSoundLoad *c, void *p)
{
    if (!p)
        return;
    if (c->file_allocator.release)
        c->file_allocator.release(p, c->file_allocator.user);
    else
        free(p);
}
unsigned long __cdecl bh_sound_load(BHSoundLoad *c, const char *name, float range, float level,
                                    unsigned short streaming, float denominator)
{
    BHSoundQueue *sound_queue;
    BHVFile *file;
    unsigned char *bytes;
    long length;
    const unsigned char *format, *samples;
    unsigned long sample_bytes, format_bytes, i, frequency;
    BHAudioBuffer *buffer = NULL;
    DWORD first_size;
    BHSoundResource *resource;
    double volume;
    int random;
    if (!c)
        return BH_SOUND_INVALID;
    c->error = BH_SOUND_LOAD_OK;
    if (!(sound_queue = c->queue) || !name || strlen(name) >= 64 || !sound_queue->device ||
        sound_queue->voice_limit_policy < 1 || sound_queue->voice_limit_policy > 3 ||
        (!!c->file_allocator.allocate != !!c->file_allocator.release))
        return fail(c, BH_SOUND_LOAD_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(c, BH_SOUND_LOAD_NUMERICAL);
    file = bh_vfile_open(name, "rb");
    if (!file)
        return fail(c, BH_SOUND_LOAD_IO);
    ++c->files;
    if (bh_vfile_seek(file, 0, SEEK_END) || (length = bh_vfile_tell(file)) <= 0 || length > 64L * 1024L * 1024L ||
        bh_vfile_seek(file, 0, SEEK_SET))
    {
        bh_vfile_close(file);
        return fail(c, BH_SOUND_LOAD_IO);
    }
    c->file_bytes = (unsigned long)length;
    bytes = (unsigned char *)allocate(c, (unsigned long)length);
    if (!bytes)
    {
        bh_vfile_close(file);
        return fail(c, BH_SOUND_LOAD_MEMORY);
    }
    if (bh_vfile_read(bytes, (unsigned long)length, 1, file) != 1)
    {
        bh_vfile_close(file);
        release(c, bytes);
        return fail(c, BH_SOUND_LOAD_IO);
    }
    bh_vfile_close(file);
    if (!bh_riff_scan(bytes, (unsigned long)length, &format, &samples, &sample_bytes) || !format || !samples ||
        !sample_bytes)
    {
        release(c, bytes);
        return fail(c, BH_SOUND_LOAD_FORMAT);
    }
    format_bytes = u32(format - 4);
    if (format_bytes < 16 || u16(format) != 1)
    {
        release(c, bytes);
        return fail(c, BH_SOUND_LOAD_FORMAT);
    }
    ++c->native_calls;
    c->create_result = bh_audio_create(sound_queue->device, samples, sample_bytes, u16(format + 2), u16(format + 14),
                                       u32(format + 4), &buffer);
    release(c, bytes);
    if (c->create_result)
        return fail(c, BH_SOUND_LOAD_NATIVE);
    first_size = c->first_bytes = sample_bytes;
    c->second_bytes = 0;
    c->lock_result = c->unlock_result = 0;
    resource = bh_sound_resource_allocate(sound_queue);
    if (!resource)
    {
        bh_audio_buffer_release(buffer);
        ++c->releases;
        return fail(c, BH_SOUND_LOAD_MEMORY);
    }
    strcpy(resource->name, name);
    ++c->native_calls;
    c->frequency_result = bh_audio_frequency(buffer, &frequency);
    if (c->frequency_result == 0)
        bh_round_float(&resource->base_frequency, (double)frequency);
    else
        resource->base_frequency = 22050.0f;
    resource->no_more = resource->voice_limit == 1;
    resource->buffers[0] = buffer;
    resource->streaming = streaming;
    resource->buffer_count = 1;
    resource->sample_bytes = first_size;
    resource->voice_limit = sound_queue->voice_limit_policy;
    resource->stream_head = NULL;
    volume = level > 0.0f ? (log((double)level) * -1000.0) / log(0.5) : -10000.0;
    if (!(volume > -10000.0))
        volume = -10000.0;
    bh_round_float(&resource->time_scale, 1.0 / (double)denominator);
    bh_round_float(&resource->base_volume, volume);
    random = rand();
    resource->audible_range = range;
    bh_round_float(&resource->phase, ((double)random * resource->time_scale) * (1.0 / 32768.0));
    for (i = 0; i < resource->voice_limit; ++i)
    {
        resource->playing[i] = 0;
        resource->cached_volume[i] = resource->cached_frequency[i] = resource->cached_pan[i] = -100000;
        resource->voices[i] = NULL;
    }
    ++c->loads;
    return resource->index;
}
