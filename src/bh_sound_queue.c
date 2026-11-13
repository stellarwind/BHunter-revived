#include "bh_sound_queue.h"
#include "bh_fp.h"
#include "bh_round.h"
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
static int vector_ok(const BHVec3 *v)
{
    return !v || (_finite(v->x) && _finite(v->y) && _finite(v->z));
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
    BHSoundResource *r;
    if (!sound_queue || !sound_queue->allocator.allocate)
        return NULL;
    if (sound_queue->free_resources)
    {
        r = sound_queue->free_resources;
        sound_queue->free_resources = r->free_next;
        return r;
    }
    if (!reserve(sound_queue, (void ***)&sound_queue->resources, &sound_queue->resource_capacity,
                 sound_queue->resource_count))
        return NULL;
    r = (BHSoundResource *)sound_queue->allocator.allocate(sizeof(*r), sound_queue->allocator.user);
    if (!r)
    {
        fail(sound_queue, BH_SOUND_MEMORY);
        return NULL;
    }
    /* Defined zero bytes for fresh, otherwise unspecified original payload.
       Recycled records retain payload exactly as the original freelist does. */
    memset(r, 0, sizeof(*r));
    r->index = (unsigned short)sound_queue->resource_count;
    sound_queue->resources[sound_queue->resource_count++] = r;
    return r;
}
BHSoundVoice *bh_sound_voice_allocate(BHSoundQueue *sound_queue)
{
    BHSoundVoice *v;
    if (!sound_queue || !sound_queue->allocator.allocate)
        return NULL;
    if (sound_queue->free_voices)
    {
        v = sound_queue->free_voices;
        sound_queue->free_voices = v->next;
        v->generation = (unsigned short)(v->generation + 1);
        return v;
    }
    if (!reserve(sound_queue, (void ***)&sound_queue->voices, &sound_queue->voice_capacity, sound_queue->voice_count))
        return NULL;
    v = (BHSoundVoice *)sound_queue->allocator.allocate(sizeof(*v), sound_queue->allocator.user);
    if (!v)
    {
        fail(sound_queue, BH_SOUND_MEMORY);
        return NULL;
    }
    memset(v, 0, sizeof(*v));
    v->index = (unsigned short)sound_queue->voice_count;
    v->generation = 0;
    sound_queue->voices[sound_queue->voice_count++] = v;
    return v;
}
int bh_sound_queue_dispose(BHSoundQueue *sound_queue)
{
    unsigned long i, j;
    if (!sound_queue || !sound_queue->allocator.release)
        return 0;
    for (i = 0; i < sound_queue->resource_count; ++i)
    {
        BHSoundResource *r = sound_queue->resources[i];
        for (j = 0; j < r->buffer_count && j < 3; ++j)
            if (r->buffers[j])
            {
                bh_audio_stop(r->buffers[j]);
                bh_audio_buffer_release(r->buffers[j]);
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
static void voice_parameters(BHSoundVoice *v, float level, float pitch, const BHVec3 *position, const BHVec3 *velocity)
{
    double db;
    v->positioned = position != NULL;
    if (position)
        v->position = *position;
    v->moving = velocity != NULL;
    if (velocity)
        v->velocity = *velocity;
    db = level == 0 ? -10000.0 : log((double)level) * (-1000.0) / log(.5);
    if (db <= -10000)
        db = -10000;
    bh_round_float(&v->volume, (double)v->resource->base_volume + db);
    bh_round_float(&v->frequency, (double)v->resource->base_frequency * pitch);
}
unsigned long bh_sound_request(BHSoundQueue *sound_queue, unsigned long id, float level, float pitch,
                               const BHVec3 *position, const BHVec3 *velocity)
{
    BHSoundResource *r;
    BHSoundVoice *v, *old;
    unsigned long slot = BH_SOUND_INVALID, i, worst = 0;
    float priority, score, new_priority;
    if (id == BH_SOUND_INVALID)
        return BH_SOUND_INVALID;
    if (!request_arguments(sound_queue, level, pitch, position, velocity) || id >= sound_queue->resource_count)
    {
        fail(sound_queue, BH_SOUND_ARGUMENT);
        return BH_SOUND_INVALID;
    }
    r = sound_queue->resources[id];
    if (!r->streaming)
    {
        if (r->buffer_count < 1 || r->buffer_count > 3 || r->voice_limit < 1 || r->voice_limit > 3)
        {
            fail(sound_queue, BH_SOUND_TOPOLOGY);
            return BH_SOUND_INVALID;
        }
        for (i = 0; i < r->buffer_count; ++i)
            if (!r->voices[i])
            {
                slot = i;
                break;
            }
        if (slot == BH_SOUND_INVALID)
        {
            if (!r->no_more)
            {
                if (!sound_queue->device || r->buffer_count >= 3)
                {
                    fail(sound_queue, BH_SOUND_TOPOLOGY);
                    return BH_SOUND_INVALID;
                }
                sound_queue->last_hresult =
                    bh_audio_duplicate(sound_queue->device, r->buffers[0], r->buffers + r->buffer_count);
                if (sound_queue->last_hresult == 0)
                {
                    ++r->buffer_count;
                    if (r->buffer_count == r->voice_limit)
                        r->no_more = 1;
                    slot = r->buffer_count - 1;
                }
                else
                {
                    r->buffers[r->buffer_count] = NULL;
                    r->no_more = 1;
                }
            }
            if (slot == BH_SOUND_INVALID)
            {
                if (!bh_sound_priority(sound_queue, r->voices[0]->positioned, &r->voices[0]->position, r, 0, &priority))
                    return BH_SOUND_INVALID;
                for (i = 1; i < r->buffer_count; ++i)
                {
                    if (!bh_sound_priority(sound_queue, r->voices[i]->positioned, &r->voices[i]->position, r, i,
                                           &score))
                        return BH_SOUND_INVALID;
                    if (priority < score)
                    {
                        priority = score;
                        worst = i;
                    }
                }
                if (!bh_sound_priority(sound_queue, position != NULL, position, r, -1, &new_priority))
                    return BH_SOUND_INVALID;
                if (priority < new_priority)
                {
                    fail(sound_queue, BH_SOUND_PRIORITY);
                    return BH_SOUND_INVALID;
                }
                slot = worst;
            }
        }
        old = r->voices[slot];
        if (old)
        {
            old->generation = (unsigned short)(old->generation + 1);
            old->next = sound_queue->free_voices;
            sound_queue->free_voices = old;
        }
        v = bh_sound_voice_allocate(sound_queue);
        if (!v)
            return BH_SOUND_INVALID;
        v->resource = r;
        voice_parameters(v, level, pitch, position, velocity);
        v->pending = 1;
        r->voices[slot] = v;
        r->phase = 0;
    }
    else
    {
        v = bh_sound_voice_allocate(sound_queue);
        if (!v)
            return BH_SOUND_INVALID;
        v->resource = r;
        voice_parameters(v, level, pitch, position, velocity);
        v->next = r->stream_head;
        if (v->next)
            v->next->previous = v;
        v->previous = NULL;
        r->stream_head = v;
    }
    sound_queue->error = BH_SOUND_OK;
    return (unsigned long)v->index + ((unsigned long)v->generation << 16);
}
BHSoundVoice *bh_sound_voice(BHSoundQueue *sound_queue, unsigned long handle)
{
    BHSoundVoice *v;
    long signed_value;
    if (!sound_queue || handle == BH_SOUND_INVALID || (handle & 0xffff) >= sound_queue->voice_count)
        return NULL;
    v = sound_queue->voices[handle & 0xffff];
    memcpy(&signed_value, &handle, 4);
    if ((long)v->generation != signed_value / 65536)
        return NULL;
    return v;
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
    BHSoundVoice *v;
    BHSoundResource *r;
    unsigned long i;
    int backend_ok = 1;
    v = bh_sound_voice(sound_queue, handle);
    if (!v)
    {
        fail(sound_queue, BH_SOUND_STALE);
        return 0;
    }
    r = v->resource;
    if (!r)
        return fail(sound_queue, BH_SOUND_TOPOLOGY);
    if (!r->streaming)
    {
        for (i = 0; i < r->buffer_count && i < 3; ++i)
            if (r->voices[i] == v)
                break;
        if (i >= r->buffer_count || i >= 3)
            return fail(sound_queue, BH_SOUND_TOPOLOGY);
        r->voices[i] = NULL;
        r->playing[i] = 0;
        sound_queue->last_hresult = bh_audio_stop(r->buffers[i]);
        backend_ok = sound_queue->last_hresult == 0;
    }
    else
    {
        if (v->previous)
            v->previous->next = v->next;
        else
            r->stream_head = v->next;
        if (v->next)
            v->next->previous = v->previous;
        /* A loop's native buffers outlive its requests. Retire the last
           request synchronously: a throttled tick (or no active listener)
           must not leave a warning/projectile/menu loop playing forever. */
        if (!r->stream_head)
            for (i = 0; i < r->buffer_count && i < 3; ++i)
                if (r->playing[i])
                {
                    sound_queue->last_hresult = bh_audio_stop(r->buffers[i]);
                    if (sound_queue->last_hresult != 0)
                        backend_ok = 0;
                    r->playing[i] = 0;
                }
        /* Shared resources must reselect surviving requests on the next tick. */
        r->phase = 0;
    }
    v->generation = (unsigned short)(v->generation + 1);
    v->next = sound_queue->free_voices;
    sound_queue->free_voices = v;
    sound_queue->error = backend_ok ? BH_SOUND_OK : BH_SOUND_BACKEND;
    return backend_ok;
}

int bh_sound_stop_all(BHSoundQueue *sound_queue)
{
    BHSoundResource *r;
    BHSoundVoice *v;
    unsigned long i, j;
    int backend_ok = 1;
    if (!sound_queue || (sound_queue->resource_count && !sound_queue->resources) ||
        (sound_queue->voice_count && !sound_queue->voices))
        return fail(sound_queue, BH_SOUND_ARGUMENT);
    for (i = 0; i < sound_queue->resource_count; ++i)
    {
        r = sound_queue->resources[i];
        for (j = 0; j < r->buffer_count && j < 3; ++j)
            if (r->buffers[j])
            {
                sound_queue->last_hresult = bh_audio_stop(r->buffers[j]);
                if (sound_queue->last_hresult != 0)
                    backend_ok = 0;
            }
        r->stream_head = NULL;
        memset(r->voices, 0, sizeof(r->voices));
        memset(r->playing, 0, sizeof(r->playing));
        r->phase = 0;
    }
    sound_queue->free_voices = NULL;
    for (i = 0; i < sound_queue->voice_count; ++i)
    {
        v = sound_queue->voices[i];
        ++v->generation;
        v->resource = NULL;
        v->pending = 0;
        v->previous = NULL;
        v->next = sound_queue->free_voices;
        sound_queue->free_voices = v;
    }
    sound_queue->error = backend_ok ? BH_SOUND_OK : BH_SOUND_BACKEND;
    return backend_ok;
}
