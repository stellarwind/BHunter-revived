#include "bh_sound_queue.h"
#include "../Math/bh_fp.h"
#include "../Math/bh_round.h"
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static_assert(sizeof(BHSoundVoice) == 0x48, "BHSoundVoice size");
static_assert(offsetof(BHSoundVoice, generation) == 0x3e, "BHSoundVoice.generation offset");
static_assert(offsetof(BHSoundVoice, pending) == 0x46, "BHSoundVoice.pending offset");
static_assert(sizeof(BHSoundResource) == 0xac, "BHSoundResource size");
static_assert(offsetof(BHSoundResource, buffer_count) == 0x4c, "BHSoundResource.buffer_count offset");
static_assert(offsetof(BHSoundResource, voices) == 0x9c, "BHSoundResource.voices offset");
static void *allocate(unsigned long size, void *user)
{
    return malloc(size);
}
static void release(void *pointer, void *user)
{
    free(pointer);
}
static int fail(BHSoundQueue *sound_queue, BHSoundQueueError error)
{
    if (sound_queue)
        sound_queue->error = error;
    return 0;
}
static int vector_ok(const BHVec3 *vector)
{
    return !vector || (_finite(vector->x) && _finite(vector->y) && _finite(vector->z));
}
int bh_sound_queue_init(BHSoundQueue *sound_queue, BHAudioDevice *device, BHAudioBuffer *primary,
                        const BHSceneAllocator *allocator)
{
    BHSoundQueue next;
    if (!sound_queue || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(&next, 0, sizeof(next));
    if (allocator)
        next.allocator = *allocator;
    else
    {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    next.device = device;
    next.primary = primary;
    next.voice_limit_policy = 2;
    if (device)
        bh_audio_retain(device);
    if (primary)
        bh_audio_buffer_retain(primary);
    *sound_queue = next;
    return 1;
}
static int reserve(BHSoundQueue *sound_queue, void ***table, unsigned long *capacity, unsigned long count)
{
    unsigned long next;
    void **memory;
    if (count < *capacity)
        return 1;
    if (count >= 65535)
        return fail(sound_queue, BH_SOUND_LIMIT);
    next = *capacity ? (*capacity * 5) / 4 : 16;
    if (next > 65535)
        next = 65535;
    memory = (void **)sound_queue->allocator.allocate(next * sizeof(void *), sound_queue->allocator.user);
    if (!memory)
        return fail(sound_queue, BH_SOUND_MEMORY);
    if (count)
        memcpy(memory, *table, count * sizeof(void *));
    if (*table)
        sound_queue->allocator.release(*table, sound_queue->allocator.user);
    *table = memory;
    *capacity = next;
    return 1;
}
BHSoundResource *bh_sound_resource_allocate(BHSoundQueue *sound_queue)
{
    BHSoundResource *resource;
    if (!sound_queue || !sound_queue->allocator.allocate)
        return NULL;
    if (sound_queue->free_resources)
    {
        resource = sound_queue->free_resources;
        sound_queue->free_resources = resource->free_next;
        return resource;
    }
    if (!reserve(sound_queue, (void ***)&sound_queue->resources, &sound_queue->resource_capacity,
                 sound_queue->resource_count))
        return NULL;
    resource = (BHSoundResource *)sound_queue->allocator.allocate(sizeof(*resource), sound_queue->allocator.user);
    if (!resource)
    {
        fail(sound_queue, BH_SOUND_MEMORY);
        return NULL;
    }
    /* Defined zero bytes for fresh, otherwise unspecified original payload.
       Recycled records retain payload exactly as the original freelist does. */
    memset(resource, 0, sizeof(*resource));
    resource->index = (unsigned short)sound_queue->resource_count;
    sound_queue->resources[sound_queue->resource_count++] = resource;
    return resource;
}
BHSoundVoice *bh_sound_voice_allocate(BHSoundQueue *sound_queue)
{
    BHSoundVoice *voice;
    if (!sound_queue || !sound_queue->allocator.allocate)
        return NULL;
    if (sound_queue->free_voices)
    {
        voice = sound_queue->free_voices;
        sound_queue->free_voices = voice->next;
        voice->generation = (unsigned short)(voice->generation + 1);
        return voice;
    }
    if (!reserve(sound_queue, (void ***)&sound_queue->voices, &sound_queue->voice_capacity, sound_queue->voice_count))
        return NULL;
    voice = (BHSoundVoice *)sound_queue->allocator.allocate(sizeof(*voice), sound_queue->allocator.user);
    if (!voice)
    {
        fail(sound_queue, BH_SOUND_MEMORY);
        return NULL;
    }
    memset(voice, 0, sizeof(*voice));
    voice->index = (unsigned short)sound_queue->voice_count;
    voice->generation = 0;
    sound_queue->voices[sound_queue->voice_count++] = voice;
    return voice;
}
int bh_sound_queue_dispose(BHSoundQueue *sound_queue)
{
    unsigned long i, j;
    if (!sound_queue || !sound_queue->allocator.release)
        return 0;
    for (i = 0; i < sound_queue->resource_count; ++i)
    {
        BHSoundResource *resource = sound_queue->resources[i];
        for (j = 0; j < resource->buffer_count && j < 3; ++j)
            if (resource->buffers[j])
            {
                bh_audio_stop(resource->buffers[j]);
                bh_audio_buffer_release(resource->buffers[j]);
            }
    }
    for (i = 0; i < sound_queue->resource_count; ++i)
        sound_queue->allocator.release(sound_queue->resources[i], sound_queue->allocator.user);
    if (sound_queue->resources)
        sound_queue->allocator.release(sound_queue->resources, sound_queue->allocator.user);
    for (i = 0; i < sound_queue->voice_count; ++i)
        sound_queue->allocator.release(sound_queue->voices[i], sound_queue->allocator.user);
    if (sound_queue->voices)
        sound_queue->allocator.release(sound_queue->voices, sound_queue->allocator.user);
    if (sound_queue->primary)
        bh_audio_buffer_release(sound_queue->primary);
    if (sound_queue->device)
        bh_audio_release(sound_queue->device);
    memset(sound_queue, 0, sizeof(*sound_queue));
    return 1;
}
unsigned long bh_sound_register_wave(BHSoundQueue *sound_queue, const BHWave *wave, unsigned short streaming,
                                     unsigned short limit, float base_volume)
{
    BHAudioBuffer *buffer = NULL;
    BHSoundResource *resource;
    unsigned long frequency;
    unsigned long i;
    if (sound_queue && !limit)
        limit = sound_queue->voice_limit_policy;
    if (!sound_queue || !sound_queue->device || !wave || !wave->samples || !wave->sample_bytes ||
        wave->format_tag != 1 || limit < 1 || limit > 3 || !_finite(base_volume))
    {
        fail(sound_queue, BH_SOUND_ARGUMENT);
        return BH_SOUND_INVALID;
    }
    sound_queue->last_hresult = bh_audio_create(sound_queue->device, wave->samples, wave->sample_bytes, wave->channels,
                                                wave->bits, wave->sample_rate, &buffer);
    if (sound_queue->last_hresult)
    {
        fail(sound_queue, BH_SOUND_BACKEND);
        return BH_SOUND_INVALID;
    }
    resource = bh_sound_resource_allocate(sound_queue);
    if (!resource)
    {
        bh_audio_buffer_release(buffer);
        return BH_SOUND_INVALID;
    }
    resource->buffers[0] = buffer;
    resource->buffers[1] = resource->buffers[2] = NULL;
    resource->buffer_count = 1;
    resource->no_more = limit == 1;
    resource->streaming = streaming;
    resource->voice_limit = limit;
    resource->sample_bytes = wave->sample_bytes;
    resource->base_volume = base_volume;
    if (bh_audio_frequency(buffer, &frequency) != 0)
        frequency = 44100;
    bh_round_float(&resource->base_frequency, (double)frequency);
    resource->stream_head = NULL;
    resource->phase = 0;
    for (i = 0; i < 3; ++i)
    {
        resource->voices[i] = NULL;
        resource->playing[i] = 0;
        resource->cached_volume[i] = resource->cached_frequency[i] = resource->cached_pan[i] = -100000;
    }
    sound_queue->error = BH_SOUND_OK;
    return resource->index;
}
int bh_sound_priority(BHSoundQueue *sound_queue, int positioned, const BHVec3 *position, BHSoundResource *resource,
                      long slot, float *out)
{
    float distance = 9e37f;
    double x, y, z, total;
    unsigned long play, write;
    if (!sound_queue || !resource || !out || !vector_ok(position) || (positioned && !position) ||
        !bh_math_environment_supported())
        return fail(sound_queue, BH_SOUND_ARGUMENT);
    if (!positioned)
        distance = .01f;
    else if (sound_queue->listener_active != 0)
    {
        if (!vector_ok(&sound_queue->listener))
            return fail(sound_queue, BH_SOUND_NUMERICAL);
        x = (double)position->x - sound_queue->listener.x;
        y = (double)position->y - sound_queue->listener.y;
        z = (double)position->z - sound_queue->listener.z;
        total = (z * z + y * y) + x * x;
        if (total < distance)
            bh_round_float(&distance, total);
    }
    if (slot >= 0)
    {
        if ((unsigned long)slot >= resource->buffer_count || slot >= 3 || !resource->buffers[slot] ||
            !resource->sample_bytes)
            return fail(sound_queue, BH_SOUND_ARGUMENT);
        sound_queue->last_hresult = bh_audio_position(resource->buffers[slot], &play, &write);
        if (sound_queue->last_hresult != 0)
            return fail(sound_queue, BH_SOUND_BACKEND);
        if (play >= resource->sample_bytes)
            return fail(sound_queue, BH_SOUND_BACKEND);
        bh_round_float(&distance, ((double)resource->sample_bytes * distance) / (resource->sample_bytes - play));
    }
    *out = distance;
    return 1;
}
static int request_arguments(BHSoundQueue *sound_queue, float level, float pitch, const BHVec3 *position,
                             const BHVec3 *velocity)
{
    return sound_queue && _finite(level) && level >= 0 && _finite(pitch) && vector_ok(position) &&
           vector_ok(velocity) && bh_math_environment_supported();
}
static void voice_parameters(BHSoundVoice *voice, float level, float pitch, const BHVec3 *position, const BHVec3 *velocity)
{
    double db;
    voice->positioned = position != NULL;
    if (position)
        voice->position = *position;
    voice->moving = velocity != NULL;
    if (velocity)
        voice->velocity = *velocity;
    db = level == 0 ? -10000.0 : log((double)level) * (-1000.0) / log(.5);
    if (db <= -10000)
        db = -10000;
    bh_round_float(&voice->volume, (double)voice->resource->base_volume + db);
    bh_round_float(&voice->frequency, (double)voice->resource->base_frequency * pitch);
}
unsigned long bh_sound_request(BHSoundQueue *sound_queue, unsigned long id, float level, float pitch,
                               const BHVec3 *position, const BHVec3 *velocity)
{
    BHSoundResource *resource;
    BHSoundVoice *voice, *old;
    unsigned long slot = BH_SOUND_INVALID, i, worst = 0;
    float priority, score, new_priority;
    if (id == BH_SOUND_INVALID)
        return BH_SOUND_INVALID;
    if (!request_arguments(sound_queue, level, pitch, position, velocity) || id >= sound_queue->resource_count)
    {
        fail(sound_queue, BH_SOUND_ARGUMENT);
        return BH_SOUND_INVALID;
    }
    resource = sound_queue->resources[id];
    if (!resource->streaming)
    {
        if (resource->buffer_count < 1 || resource->buffer_count > 3 || resource->voice_limit < 1 || resource->voice_limit > 3)
        {
            fail(sound_queue, BH_SOUND_TOPOLOGY);
            return BH_SOUND_INVALID;
        }
        for (i = 0; i < resource->buffer_count; ++i)
            if (!resource->voices[i])
            {
                slot = i;
                break;
            }
        if (slot == BH_SOUND_INVALID)
        {
            if (!resource->no_more)
            {
                if (!sound_queue->device || resource->buffer_count >= 3)
                {
                    fail(sound_queue, BH_SOUND_TOPOLOGY);
                    return BH_SOUND_INVALID;
                }
                sound_queue->last_hresult =
                    bh_audio_duplicate(sound_queue->device, resource->buffers[0], resource->buffers + resource->buffer_count);
                if (sound_queue->last_hresult == 0)
                {
                    ++resource->buffer_count;
                    if (resource->buffer_count == resource->voice_limit)
                        resource->no_more = 1;
                    slot = resource->buffer_count - 1;
                }
                else
                {
                    resource->buffers[resource->buffer_count] = NULL;
                    resource->no_more = 1;
                }
            }
            if (slot == BH_SOUND_INVALID)
            {
                if (!bh_sound_priority(sound_queue, resource->voices[0]->positioned, &resource->voices[0]->position, resource, 0, &priority))
                    return BH_SOUND_INVALID;
                for (i = 1; i < resource->buffer_count; ++i)
                {
                    if (!bh_sound_priority(sound_queue, resource->voices[i]->positioned, &resource->voices[i]->position, resource, i,
                                           &score))
                        return BH_SOUND_INVALID;
                    if (priority < score)
                    {
                        priority = score;
                        worst = i;
                    }
                }
                if (!bh_sound_priority(sound_queue, position != NULL, position, resource, -1, &new_priority))
                    return BH_SOUND_INVALID;
                if (priority < new_priority)
                {
                    fail(sound_queue, BH_SOUND_PRIORITY);
                    return BH_SOUND_INVALID;
                }
                slot = worst;
            }
        }
        old = resource->voices[slot];
        if (old)
        {
            old->generation = (unsigned short)(old->generation + 1);
            old->next = sound_queue->free_voices;
            sound_queue->free_voices = old;
        }
        voice = bh_sound_voice_allocate(sound_queue);
        if (!voice)
            return BH_SOUND_INVALID;
        voice->resource = resource;
        voice_parameters(voice, level, pitch, position, velocity);
        voice->pending = 1;
        resource->voices[slot] = voice;
        resource->phase = 0;
    }
    else
    {
        voice = bh_sound_voice_allocate(sound_queue);
        if (!voice)
            return BH_SOUND_INVALID;
        voice->resource = resource;
        voice_parameters(voice, level, pitch, position, velocity);
        voice->next = resource->stream_head;
        if (voice->next)
            voice->next->previous = voice;
        voice->previous = NULL;
        resource->stream_head = voice;
    }
    sound_queue->error = BH_SOUND_OK;
    return (unsigned long)voice->index + ((unsigned long)voice->generation << 16);
}
BHSoundVoice *bh_sound_voice(BHSoundQueue *sound_queue, unsigned long handle)
{
    BHSoundVoice *voice;
    long signed_value;
    if (!sound_queue || handle == BH_SOUND_INVALID || (handle & 0xffff) >= sound_queue->voice_count)
        return NULL;
    voice = sound_queue->voices[handle & 0xffff];
    memcpy(&signed_value, &handle, 4);
    if ((long)voice->generation != signed_value / 65536)
        return NULL;
    return voice;
}
int bh_sound_update(BHSoundQueue *sound_queue, unsigned long handle, float level, float pitch, const BHVec3 *position,
                    const BHVec3 *velocity)
{
    BHSoundVoice *v;
    if (!request_arguments(sound_queue, level, pitch, position, velocity))
    {
        fail(sound_queue, BH_SOUND_ARGUMENT);
        return -1;
    }
    v = bh_sound_voice(sound_queue, handle);
    if (!v)
    {
        fail(sound_queue, BH_SOUND_STALE);
        return 0;
    }
    voice_parameters(v, level, pitch, position, velocity);
    sound_queue->error = BH_SOUND_OK;
    return 1;
}
int bh_sound_stop(BHSoundQueue *sound_queue, unsigned long handle)
{
    BHSoundVoice *voice;
    BHSoundResource *resource;
    unsigned long i;
    int backend_ok = 1;
    voice = bh_sound_voice(sound_queue, handle);
    if (!voice)
    {
        fail(sound_queue, BH_SOUND_STALE);
        return 0;
    }
    resource = voice->resource;
    if (!resource)
        return fail(sound_queue, BH_SOUND_TOPOLOGY);
    if (!resource->streaming)
    {
        for (i = 0; i < resource->buffer_count && i < 3; ++i)
            if (resource->voices[i] == voice)
                break;
        if (i >= resource->buffer_count || i >= 3)
            return fail(sound_queue, BH_SOUND_TOPOLOGY);
        resource->voices[i] = NULL;
        resource->playing[i] = 0;
        sound_queue->last_hresult = bh_audio_stop(resource->buffers[i]);
        backend_ok = sound_queue->last_hresult == 0;
    }
    else
    {
        if (voice->previous)
            voice->previous->next = voice->next;
        else
            resource->stream_head = voice->next;
        if (voice->next)
            voice->next->previous = voice->previous;
        /* A loop's native buffers outlive its requests. Retire the last
           request synchronously: a throttled tick (or no active listener)
           must not leave a warning/projectile/menu loop playing forever. */
        if (!resource->stream_head)
            for (i = 0; i < resource->buffer_count && i < 3; ++i)
                if (resource->playing[i])
                {
                    sound_queue->last_hresult = bh_audio_stop(resource->buffers[i]);
                    if (sound_queue->last_hresult != 0)
                        backend_ok = 0;
                    resource->playing[i] = 0;
                }
        /* Shared resources must reselect surviving requests on the next tick. */
        resource->phase = 0;
    }
    voice->generation = (unsigned short)(voice->generation + 1);
    voice->next = sound_queue->free_voices;
    sound_queue->free_voices = voice;
    sound_queue->error = backend_ok ? BH_SOUND_OK : BH_SOUND_BACKEND;
    return backend_ok;
}

int bh_sound_stop_all(BHSoundQueue *sound_queue)
{
    BHSoundResource *resource;
    BHSoundVoice *voice;
    unsigned long i, j;
    int backend_ok = 1;
    if (!sound_queue || (sound_queue->resource_count && !sound_queue->resources) ||
        (sound_queue->voice_count && !sound_queue->voices))
        return fail(sound_queue, BH_SOUND_ARGUMENT);
    for (i = 0; i < sound_queue->resource_count; ++i)
    {
        resource = sound_queue->resources[i];
        for (j = 0; j < resource->buffer_count && j < 3; ++j)
            if (resource->buffers[j])
            {
                sound_queue->last_hresult = bh_audio_stop(resource->buffers[j]);
                if (sound_queue->last_hresult != 0)
                    backend_ok = 0;
            }
        resource->stream_head = NULL;
        memset(resource->voices, 0, sizeof(resource->voices));
        memset(resource->playing, 0, sizeof(resource->playing));
        resource->phase = 0;
    }
    sound_queue->free_voices = NULL;
    for (i = 0; i < sound_queue->voice_count; ++i)
    {
        voice = sound_queue->voices[i];
        ++voice->generation;
        voice->resource = NULL;
        voice->pending = 0;
        voice->previous = NULL;
        voice->next = sound_queue->free_voices;
        sound_queue->free_voices = voice;
    }
    sound_queue->error = backend_ok ? BH_SOUND_OK : BH_SOUND_BACKEND;
    return backend_ok;
}
