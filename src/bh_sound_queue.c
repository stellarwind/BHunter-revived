#include "bh_sound_queue.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef char voice_size[sizeof(BHSoundVoice) == 0x48 ? 1 : -1];
typedef char voice_generation[offsetof(BHSoundVoice, generation) == 0x3e ? 1 : -1];
typedef char voice_flags[offsetof(BHSoundVoice, pending) == 0x46 ? 1 : -1];
typedef char resource_size[sizeof(BHSoundResource) == 0xac ? 1 : -1];
typedef char resource_count[offsetof(BHSoundResource, buffer_count) == 0x4c ? 1 : -1];
typedef char resource_voices[offsetof(BHSoundResource, voices) == 0x9c ? 1 : -1];
static void *__cdecl allocate(unsigned long size, void *user)
{
    return malloc(size);
}
static void __cdecl release(void *pointer, void *user)
{
    free(pointer);
}
static int fail(BHSoundQueue *q, BHSoundQueueError error)
{
    if (q)
        q->error = error;
    return 0;
}
static int vector_ok(const BHVec3 *v)
{
    return !v || (_finite(v->x) && _finite(v->y) && _finite(v->z));
}
int __cdecl bh_sound_queue_init(BHSoundQueue *q, LPDIRECTSOUND device, LPDIRECTSOUNDBUFFER primary,
                                const BHSceneAllocator *allocator)
{
    BHSoundQueue next;
    if (!q || (allocator && (!allocator->allocate || !allocator->release)))
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
        IDirectSound_AddRef(device);
    if (primary)
        IDirectSoundBuffer_AddRef(primary);
    *q = next;
    return 1;
}
static int reserve(BHSoundQueue *q, void ***table, unsigned long *capacity, unsigned long count)
{
    unsigned long next;
    void **memory;
    if (count < *capacity)
        return 1;
    if (count >= 65535)
        return fail(q, BH_SOUND_LIMIT);
    next = *capacity ? (*capacity * 5) / 4 : 16;
    if (next > 65535)
        next = 65535;
    memory = (void **)q->allocator.allocate(next * sizeof(void *), q->allocator.user);
    if (!memory)
        return fail(q, BH_SOUND_MEMORY);
    if (count)
        memcpy(memory, *table, count * sizeof(void *));
    if (*table)
        q->allocator.release(*table, q->allocator.user);
    *table = memory;
    *capacity = next;
    return 1;
}
BHSoundResource *__cdecl bh_sound_resource_allocate(BHSoundQueue *q)
{
    BHSoundResource *r;
    if (!q || !q->allocator.allocate)
        return NULL;
    if (q->free_resources)
    {
        r = q->free_resources;
        q->free_resources = r->free_next;
        return r;
    }
    if (!reserve(q, (void ***)&q->resources, &q->resource_capacity, q->resource_count))
        return NULL;
    r = (BHSoundResource *)q->allocator.allocate(sizeof(*r), q->allocator.user);
    if (!r)
    {
        fail(q, BH_SOUND_MEMORY);
        return NULL;
    }
    /* Defined zero bytes for fresh, otherwise unspecified original payload.
       Recycled records retain payload exactly as the original freelist does. */
    memset(r, 0, sizeof(*r));
    r->index = (unsigned short)q->resource_count;
    q->resources[q->resource_count++] = r;
    return r;
}
BHSoundVoice *__cdecl bh_sound_voice_allocate(BHSoundQueue *q)
{
    BHSoundVoice *v;
    if (!q || !q->allocator.allocate)
        return NULL;
    if (q->free_voices)
    {
        v = q->free_voices;
        q->free_voices = v->next;
        v->generation = (unsigned short)(v->generation + 1);
        return v;
    }
    if (!reserve(q, (void ***)&q->voices, &q->voice_capacity, q->voice_count))
        return NULL;
    v = (BHSoundVoice *)q->allocator.allocate(sizeof(*v), q->allocator.user);
    if (!v)
    {
        fail(q, BH_SOUND_MEMORY);
        return NULL;
    }
    memset(v, 0, sizeof(*v));
    v->index = (unsigned short)q->voice_count;
    v->generation = 0;
    q->voices[q->voice_count++] = v;
    return v;
}
int __cdecl bh_sound_queue_dispose(BHSoundQueue *q)
{
    unsigned long i, j;
    if (!q || !q->allocator.release)
        return 0;
    for (i = 0; i < q->resource_count; ++i)
    {
        BHSoundResource *r = q->resources[i];
        for (j = 0; j < r->buffer_count && j < 3; ++j)
            if (r->buffers[j])
            {
                IDirectSoundBuffer_Stop(r->buffers[j]);
                IDirectSoundBuffer_Release(r->buffers[j]);
            }
    }
    for (i = 0; i < q->resource_count; ++i)
        q->allocator.release(q->resources[i], q->allocator.user);
    if (q->resources)
        q->allocator.release(q->resources, q->allocator.user);
    for (i = 0; i < q->voice_count; ++i)
        q->allocator.release(q->voices[i], q->allocator.user);
    if (q->voices)
        q->allocator.release(q->voices, q->allocator.user);
    if (q->primary)
        IDirectSoundBuffer_Release(q->primary);
    if (q->device)
        IDirectSound_Release(q->device);
    memset(q, 0, sizeof(*q));
    return 1;
}
unsigned long __cdecl bh_sound_register_wave(BHSoundQueue *q, const BHWave *wave, unsigned short streaming,
                                             unsigned short limit, float base_volume)
{
    WAVEFORMATEX format;
    DSBUFFERDESC description;
    LPDIRECTSOUNDBUFFER buffer = NULL;
    BHSoundResource *resource;
    void *first, *second;
    DWORD first_size, second_size, frequency;
    unsigned long i;
    if (q && !limit)
        limit = q->voice_limit_policy;
    if (!q || !q->device || !wave || !wave->samples || !wave->sample_bytes || wave->format_tag != 1 || limit < 1 ||
        limit > 3 || !_finite(base_volume))
    {
        fail(q, BH_SOUND_ARGUMENT);
        return BH_SOUND_INVALID;
    }
    memset(&format, 0, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = wave->channels;
    format.nSamplesPerSec = wave->sample_rate;
    format.nAvgBytesPerSec = wave->byte_rate;
    format.nBlockAlign = wave->block_align;
    format.wBitsPerSample = wave->bits;
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
    description.dwBufferBytes = wave->sample_bytes;
    description.lpwfxFormat = &format;
    q->last_hresult = IDirectSound_CreateSoundBuffer(q->device, &description, &buffer, NULL);
    if (q->last_hresult != DS_OK)
    {
        fail(q, BH_SOUND_BACKEND);
        return BH_SOUND_INVALID;
    }
    q->last_hresult =
        IDirectSoundBuffer_Lock(buffer, 0, wave->sample_bytes, &first, &first_size, &second, &second_size, 0);
    if (q->last_hresult != DS_OK)
    {
        IDirectSoundBuffer_Release(buffer);
        fail(q, BH_SOUND_BACKEND);
        return BH_SOUND_INVALID;
    }
    memcpy(first, wave->samples, first_size);
    if (second_size)
        memcpy(second, wave->samples + first_size, second_size);
    q->last_hresult = IDirectSoundBuffer_Unlock(buffer, first, first_size, second, second_size);
    if (q->last_hresult != DS_OK)
    {
        IDirectSoundBuffer_Release(buffer);
        fail(q, BH_SOUND_BACKEND);
        return BH_SOUND_INVALID;
    }
    resource = bh_sound_resource_allocate(q);
    if (!resource)
    {
        IDirectSoundBuffer_Release(buffer);
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
    if (IDirectSoundBuffer_GetFrequency(buffer, &frequency) != DS_OK)
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
    q->error = BH_SOUND_OK;
    return resource->index;
}
int __cdecl bh_sound_priority(BHSoundQueue *q, int positioned, const BHVec3 *position, BHSoundResource *resource,
                              long slot, float *out)
{
    float distance = 9e37f;
    double x, y, z, total;
    DWORD play, write;
    if (!q || !resource || !out || !vector_ok(position) || (positioned && !position) ||
        !bh_math_environment_supported())
        return fail(q, BH_SOUND_ARGUMENT);
    if (!positioned)
        distance = .01f;
    else if (q->listener_active != 0)
    {
        if (!vector_ok(&q->listener))
            return fail(q, BH_SOUND_NUMERICAL);
        x = (double)position->x - q->listener.x;
        y = (double)position->y - q->listener.y;
        z = (double)position->z - q->listener.z;
        total = (z * z + y * y) + x * x;
        if (total < distance)
            bh_round_float(&distance, total);
    }
    if (slot >= 0)
    {
        if ((unsigned long)slot >= resource->buffer_count || slot >= 3 || !resource->buffers[slot] ||
            !resource->sample_bytes)
            return fail(q, BH_SOUND_ARGUMENT);
        q->last_hresult = IDirectSoundBuffer_GetCurrentPosition(resource->buffers[slot], &play, &write);
        if (q->last_hresult != DS_OK)
            return fail(q, BH_SOUND_BACKEND);
        if (play >= resource->sample_bytes)
            return fail(q, BH_SOUND_BACKEND);
        bh_round_float(&distance, ((double)resource->sample_bytes * distance) / (resource->sample_bytes - play));
    }
    *out = distance;
    return 1;
}
static int request_arguments(BHSoundQueue *q, float level, float pitch, const BHVec3 *position, const BHVec3 *velocity)
{
    return q && _finite(level) && level >= 0 && _finite(pitch) && vector_ok(position) && vector_ok(velocity) &&
           bh_math_environment_supported();
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
unsigned long __cdecl bh_sound_request(BHSoundQueue *q, unsigned long id, float level, float pitch,
                                       const BHVec3 *position, const BHVec3 *velocity)
{
    BHSoundResource *r;
    BHSoundVoice *v, *old;
    unsigned long slot = BH_SOUND_INVALID, i, worst = 0;
    float priority, score, new_priority;
    if (id == BH_SOUND_INVALID)
        return BH_SOUND_INVALID;
    if (!request_arguments(q, level, pitch, position, velocity) || id >= q->resource_count)
    {
        fail(q, BH_SOUND_ARGUMENT);
        return BH_SOUND_INVALID;
    }
    r = q->resources[id];
    if (!r->streaming)
    {
        if (r->buffer_count < 1 || r->buffer_count > 3 || r->voice_limit < 1 || r->voice_limit > 3)
        {
            fail(q, BH_SOUND_TOPOLOGY);
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
                if (!q->device || r->buffer_count >= 3)
                {
                    fail(q, BH_SOUND_TOPOLOGY);
                    return BH_SOUND_INVALID;
                }
                q->last_hresult =
                    IDirectSound_DuplicateSoundBuffer(q->device, r->buffers[0], r->buffers + r->buffer_count);
                if (q->last_hresult == DS_OK)
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
                if (!bh_sound_priority(q, r->voices[0]->positioned, &r->voices[0]->position, r, 0, &priority))
                    return BH_SOUND_INVALID;
                for (i = 1; i < r->buffer_count; ++i)
                {
                    if (!bh_sound_priority(q, r->voices[i]->positioned, &r->voices[i]->position, r, i, &score))
                        return BH_SOUND_INVALID;
                    if (priority < score)
                    {
                        priority = score;
                        worst = i;
                    }
                }
                if (!bh_sound_priority(q, position != NULL, position, r, -1, &new_priority))
                    return BH_SOUND_INVALID;
                if (priority < new_priority)
                {
                    fail(q, BH_SOUND_PRIORITY);
                    return BH_SOUND_INVALID;
                }
                slot = worst;
            }
        }
        old = r->voices[slot];
        if (old)
        {
            old->generation = (unsigned short)(old->generation + 1);
            old->next = q->free_voices;
            q->free_voices = old;
        }
        v = bh_sound_voice_allocate(q);
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
        v = bh_sound_voice_allocate(q);
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
    q->error = BH_SOUND_OK;
    return (unsigned long)v->index + ((unsigned long)v->generation << 16);
}
BHSoundVoice *__cdecl bh_sound_voice(BHSoundQueue *q, unsigned long handle)
{
    BHSoundVoice *v;
    long signed_value;
    if (!q || handle == BH_SOUND_INVALID || (handle & 0xffff) >= q->voice_count)
        return NULL;
    v = q->voices[handle & 0xffff];
    memcpy(&signed_value, &handle, 4);
    if ((long)v->generation != signed_value / 65536)
        return NULL;
    return v;
}
int __cdecl bh_sound_update(BHSoundQueue *q, unsigned long handle, float level, float pitch, const BHVec3 *position,
                            const BHVec3 *velocity)
{
    BHSoundVoice *v;
    if (!request_arguments(q, level, pitch, position, velocity))
    {
        fail(q, BH_SOUND_ARGUMENT);
        return -1;
    }
    v = bh_sound_voice(q, handle);
    if (!v)
    {
        fail(q, BH_SOUND_STALE);
        return 0;
    }
    voice_parameters(v, level, pitch, position, velocity);
    q->error = BH_SOUND_OK;
    return 1;
}
int __cdecl bh_sound_stop(BHSoundQueue *q, unsigned long handle)
{
    BHSoundVoice *v;
    BHSoundResource *r;
    unsigned long i;
    v = bh_sound_voice(q, handle);
    if (!v)
    {
        fail(q, BH_SOUND_STALE);
        return 0;
    }
    r = v->resource;
    if (!r)
        return fail(q, BH_SOUND_TOPOLOGY);
    if (!r->streaming)
    {
        for (i = 0; i < r->buffer_count && i < 3; ++i)
            if (r->voices[i] == v)
                break;
        if (i >= r->buffer_count || i >= 3)
            return fail(q, BH_SOUND_TOPOLOGY);
        r->voices[i] = NULL;
        r->playing[i] = 0;
        q->last_hresult = IDirectSoundBuffer_Stop(r->buffers[i]);
    }
    else
    {
        if (v->previous)
            v->previous->next = v->next;
        else
            r->stream_head = v->next;
        if (v->next)
            v->next->previous = v->previous;
    }
    v->generation = (unsigned short)(v->generation + 1);
    v->next = q->free_voices;
    q->free_voices = v;
    q->error = BH_SOUND_OK;
    return 1;
}
