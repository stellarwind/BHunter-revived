#include "bh_sound_queue.h"
#include "../Math/bh_fp.h"
#include "../Math/bh_round.h"
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <string.h>

static_assert(offsetof(BHSoundVoice, distance_squared) == 0x2c, "BHSoundVoice.distance_squared offset");
static_assert(offsetof(BHSoundVoice, listener_index) == 0x40, "BHSoundVoice.listener_index offset");
static_assert(offsetof(BHSoundResource, audible_range) == 0x94, "BHSoundResource.audible_range offset");
static int native_result(BHSoundQueue *sound_queue, BHSoundTickStats *sound_tick_stats, long hr)
{
    sound_queue->last_hresult = hr;
    if (hr != 0)
    {
        sound_tick_stats->last_error = hr;
        sound_queue->error = BH_SOUND_BACKEND;
        return 0;
    }
    return 1;
}
static long truncate_low(double x)
{
    __int64 converted = (__int64)x;
    unsigned long low = (unsigned long)converted;
    long result;
    memcpy(&result, &low, 4);
    return result;
}
static long clamp(long value, long low, long high)
{
    return value < low ? low : value > high ? high : value;
}
static void select_listener(BHSoundQueue *sound_queue, BHSoundVoice *voice, BHVec3 *scratch)
{
    double x, y, z, distance = 0;
    voice->distance_squared = 9e37f;
    if (voice->positioned)
    {
        x = (double)voice->position.x - sound_queue->listener.x;
        y = (double)voice->position.y - sound_queue->listener.y;
        z = (double)voice->position.z - sound_queue->listener.z;
        bh_round_float(&scratch->x, x);
        bh_round_float(&scratch->y, y);
        bh_round_float(&scratch->z, z);
        distance = (x * scratch->x + y * scratch->y) + z * scratch->z;
    }
    if (distance < voice->distance_squared)
    {
        bh_round_float(&voice->distance_squared, distance);
        voice->cached = *scratch;
        voice->listener_index = 0;
    }
}
static void spatial(BHSoundQueue *sound_queue, BHSoundResource *resource, BHSoundVoice *voice, float *attenuation, double *pan,
                    float *factor)
{
    double length, dot, n;
    float distance;
    *attenuation = 0;
    *pan = 0;
    *factor = 1;
    if (!voice->positioned)
        return;
    length = sqrt((double)voice->distance_squared);
    bh_round_float(&distance, length);
    if (length > resource->audible_range)
        bh_round_float(attenuation, -(((double)distance - resource->audible_range) / resource->audible_range) * 1000.0);
    if (distance == 0)
        return;
    if (resource->streaming)
        dot = ((double)voice->cached.y * sound_queue->listener_right.y +
               (double)voice->cached.z * sound_queue->listener_right.z) +
              (double)voice->cached.x * sound_queue->listener_right.x;
    else
        dot = ((double)voice->cached.x * sound_queue->listener_right.x +
               (double)voice->cached.y * sound_queue->listener_right.y) +
              (double)voice->cached.z * sound_queue->listener_right.z;
    n = dot / distance;
    *pan = (n * n) * (n > 0 ? 10000.0 : -10000.0);
    if (voice->moving)
    {
        if (resource->streaming)
            dot = (((double)voice->velocity.y - sound_queue->listener_velocity.y) * voice->cached.y +
                   ((double)voice->velocity.z - sound_queue->listener_velocity.z) * voice->cached.z) +
                  ((double)voice->velocity.x - sound_queue->listener_velocity.x) * voice->cached.x;
        else
            dot = (((double)voice->velocity.x - sound_queue->listener_velocity.x) * voice->cached.x +
                   ((double)voice->velocity.y - sound_queue->listener_velocity.y) * voice->cached.y) +
                  ((double)voice->velocity.z - sound_queue->listener_velocity.z) * voice->cached.z;
        bh_round_float(factor, 1.0 - (-(dot / distance)) * (-1.0 / 330.0));
    }
}
static void stop(BHSoundQueue *sound_queue, BHSoundResource *resource, unsigned long slot, BHSoundTickStats *sound_tick_stats)
{
    ++sound_tick_stats->stop_calls;
    native_result(sound_queue, sound_tick_stats, bh_audio_stop(resource->buffers[slot]));
    resource->playing[slot] = 0;
}
static void parameters(BHSoundQueue *sound_queue, BHSoundResource *resource, BHSoundVoice *voice, unsigned long slot, float master,
                       BHSoundTickStats *sound_tick_stats)
{
    float attenuation, factor;
    double pan_value;
    long volume, pan, frequency;
    long hr;
    spatial(sound_queue, resource, voice, &attenuation, &pan_value, &factor);
    volume = clamp(truncate_low(((double)voice->volume + attenuation) + master), -10000, 0);
    if (resource->streaming && volume < -3500)
    {
        if (resource->playing[slot])
            stop(sound_queue, resource, slot, sound_tick_stats);
        return;
    }
    pan = clamp(truncate_low(pan_value), -10000, 10000);
    frequency = truncate_low((double)voice->frequency * factor);
    if (volume < -3500)
    {
        if (resource->playing[slot])
            stop(sound_queue, resource, slot, sound_tick_stats);
        voice->pending = 0;
        return;
    }
    if (resource->cached_volume[slot] != volume)
    {
        ++sound_tick_stats->volume_calls;
        native_result(sound_queue, sound_tick_stats, bh_audio_volume(resource->buffers[slot], volume));
        resource->cached_volume[slot] = volume;
    }
    if (resource->cached_pan[slot] != pan)
    {
        ++sound_tick_stats->pan_calls;
        native_result(sound_queue, sound_tick_stats, bh_audio_pan(resource->buffers[slot], pan));
        resource->cached_pan[slot] = pan;
    }
    if (resource->cached_frequency[slot] != frequency)
    {
        ++sound_tick_stats->frequency_calls;
        hr = bh_audio_pitch(resource->buffers[slot], (unsigned long)frequency);
        native_result(sound_queue, sound_tick_stats, hr);
        resource->cached_frequency[slot] = frequency;
    }
    if (resource->streaming ? !resource->playing[slot] : voice->pending != 0)
    {
        if (!resource->streaming)
            voice->pending = 0;
        ++sound_tick_stats->position_calls;
        native_result(sound_queue, sound_tick_stats, bh_audio_seek(resource->buffers[slot], 0));
        ++sound_tick_stats->play_calls;
        native_result(sound_queue, sound_tick_stats, bh_audio_play(resource->buffers[slot], 0, 0, resource->streaming ? 1 : 0));
        resource->playing[slot] = 1;
    }
}
int bh_sound_tick(BHSoundQueue *sound_queue, float dt, float master, BHSoundTickStats *out)
{
    BHSoundTickStats stats;
    BHVec3 scratch = {0, 0, 0};
    BHSoundResource *resource;
    BHSoundVoice *voice, *best[4], *swap;
    unsigned long i, j, k, n, walk;
    unsigned long status;
    double phase;
    memset(&stats, 0, sizeof(stats));
    if (!sound_queue || !_finite(dt) || !_finite(master) || !bh_math_environment_supported())
        return 0;

    if ((sound_queue->resource_count && !sound_queue->resources) || (sound_queue->voice_count && !sound_queue->voices))
    {
        sound_queue->error = BH_SOUND_TOPOLOGY;
        return 0;
    }
    sound_queue->error = BH_SOUND_OK;
    if (!sound_queue->listener_active)
    {
        if (out)
            *out = stats;
        return 1;
    }
    for (i = 0; i < sound_queue->resource_count; ++i)
    {
        resource = sound_queue->resources[i];
        if (!resource || resource->buffer_count < 1 || resource->buffer_count > 3 || resource->voice_limit < 1 || resource->voice_limit > 3)
        {
            sound_queue->error = BH_SOUND_TOPOLOGY;
            break;
        }
        for (j = 0; j < resource->buffer_count; ++j)
            if (!resource->buffers[j])
                break;
        if (j != resource->buffer_count)
        {
            sound_queue->error = BH_SOUND_TOPOLOGY;
            break;
        }
        if (!_finite(resource->phase) || !_finite(resource->time_scale) || !_finite(resource->audible_range))
        {
            sound_queue->error = BH_SOUND_NUMERICAL;
            break;
        }
        phase = (double)resource->phase - dt;
        bh_round_float(&resource->phase, phase);
        if (!(phase < 0))
            continue;
        phase += resource->time_scale;
        bh_round_float(&resource->phase, phase);
        if (phase < 0)
            resource->phase = 0;
        if (resource->streaming)
        {
            n = 0;
            for (voice = resource->stream_head; voice && n < resource->voice_limit; voice = voice->next)
                ++n;
            while (!resource->no_more && resource->buffer_count < n)
            {
                ++stats.duplicate_calls;
                if (!sound_queue->device)
                {
                    sound_queue->error = BH_SOUND_TOPOLOGY;
                    break;
                }
                sound_queue->last_hresult =
                    bh_audio_duplicate(sound_queue->device, resource->buffers[0], resource->buffers + resource->buffer_count);
                if (sound_queue->last_hresult == 0)
                {
                    ++resource->buffer_count;
                    if (resource->buffer_count == resource->voice_limit)
                        resource->no_more = 1;
                }
                else
                {
                    resource->buffers[resource->buffer_count] = NULL;
                    resource->no_more = 1;
                }
            }
            if (n > resource->buffer_count)
                n = resource->buffer_count;
            memset(best, 0, sizeof(best));
            walk = 0;
            for (voice = resource->stream_head; voice; voice = voice->next)
            {
                if (++walk > sound_queue->voice_count)
                {
                    sound_queue->error = BH_SOUND_TOPOLOGY;
                    break;
                }
                select_listener(sound_queue, voice, &scratch);
                best[n] = voice;
                for (k = n; k && (!best[k - 1] || best[k]->distance_squared < best[k - 1]->distance_squared); --k)
                {
                    swap = best[k - 1];
                    best[k - 1] = best[k];
                    best[k] = swap;
                }
            }
            if (walk > sound_queue->voice_count)
                break;
            for (j = 0; j < n; ++j)
                parameters(sound_queue, resource, best[j], j, master, &stats);
            for (j = n; j < resource->buffer_count; ++j)
                if (resource->playing[j])
                    stop(sound_queue, resource, j, &stats);
        }
        else
        {
            for (j = 0; j < resource->buffer_count; ++j)
            {
                voice = resource->voices[j];
                if (resource->playing[j] && voice && !voice->pending)
                {
                    status = 0;
                    ++stats.status_calls;
                    native_result(sound_queue, &stats, bh_audio_status(resource->buffers[j], &status));
                    if (!(status & 1))
                    {
                        resource->playing[j] = 0;
                        ++voice->generation;
                        voice->next = sound_queue->free_voices;
                        sound_queue->free_voices = voice;
                        resource->voices[j] = NULL;
                        ++stats.recycled;
                    }
                }
            }
            for (j = 0; j < resource->buffer_count; ++j)
            {
                voice = resource->voices[j];
                if (!voice)
                    continue;
                if (voice->pending && resource->playing[j])
                    stop(sound_queue, resource, j, &stats);
                select_listener(sound_queue, voice, &scratch);
                parameters(sound_queue, resource, voice, j, master, &stats);
            }
        }
    }
    if (out)
        *out = stats;
    return sound_queue->error == BH_SOUND_OK;
}
