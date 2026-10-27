#ifndef BH_SOUND_QUEUE_H
#define BH_SOUND_QUEUE_H
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include "bh_wave.h"
#include "bh_math.h"

typedef struct BHSoundResource BHSoundResource;
typedef struct BHSoundVoice BHSoundVoice;
struct BHSoundVoice {
    BHVec3 position,velocity,cached;
    float volume,frequency;
    float distance_squared;
    BHSoundVoice *next,*previous;
    BHSoundResource *resource;
    unsigned short index,generation,listener_index,positioned,moving,pending;
};
struct BHSoundResource {
    char name[64];
    LPDIRECTSOUNDBUFFER buffers[3];
    unsigned short buffer_count,no_more,streaming,index,voice_limit,playing[3];
    unsigned long sample_bytes;
    long cached_volume[3],cached_frequency[3],cached_pan[3];
    float base_frequency,base_volume,phase,time_scale;
    float audible_range;
    BHSoundVoice *stream_head,*voices[3];
    BHSoundResource *free_next;
};
typedef enum BHSoundQueueError {
    BH_SOUND_OK,BH_SOUND_ARGUMENT,BH_SOUND_MEMORY,BH_SOUND_LIMIT,BH_SOUND_BACKEND,
    BH_SOUND_STALE,BH_SOUND_PRIORITY,BH_SOUND_NUMERICAL,BH_SOUND_TOPOLOGY
} BHSoundQueueError;
typedef struct BHSoundQueue {
    BHSoundResource **resources,*free_resources;
    BHSoundVoice **voices,*free_voices;
    unsigned long resource_count,resource_capacity,voice_count,voice_capacity;
    LPDIRECTSOUND device;
    LPDIRECTSOUNDBUFFER primary;
    BHSceneAllocator allocator;
    BHVec3 listener,listener_velocity,listener_right;
    long listener_active;
    unsigned short voice_limit_policy;
    BHSoundQueueError error;
    HRESULT last_hresult;
} BHSoundQueue;
#define BH_SOUND_INVALID 0xffffffffUL
/* Fresh explicit owner, AddRefs supplied native interfaces. NULL device permits
   pool-only operation but cannot create/duplicate buffers. Disposal implements
   0047b5f0 Stop/Release/free for all owned arrays/buffers/interfaces. No Play. */
int __cdecl bh_sound_queue_init(BHSoundQueue *queue,LPDIRECTSOUND device,
    LPDIRECTSOUNDBUFFER primary,const BHSceneAllocator *allocator);
int __cdecl bh_sound_queue_dispose(BHSoundQueue *queue);
BHSoundResource *__cdecl bh_sound_resource_allocate(BHSoundQueue *queue); /* 0047b330 */
BHSoundVoice *__cdecl bh_sound_voice_allocate(BHSoundQueue *queue); /* 0047b410 */
/* Source adapter from decoded PCM to real native buffer/registered resource.
   Does not recover full 0047b950's option/name parser. limit is 1..3, streaming
   is the original distinct list mode; volume is caller's stored base dB.
   A zero limit uses the configured voice-limit policy (default 2).
   Clip upload never plays. Caller retains BHWave ownership after return. */
unsigned long __cdecl bh_sound_register_wave(BHSoundQueue *queue,const BHWave *wave,
    unsigned short streaming,unsigned short limit,float base_volume);
/* 0047bc60: one explicit listener, original .01 or 9e37 baseline; optional
   native buffer progress weighting. Return 1 / 0 for success / invalid/backend. */
int __cdecl bh_sound_priority(BHSoundQueue *queue,int positioned,const BHVec3 *position,
    BHSoundResource *resource,long slot,float *out);
/* 0047bd40, integer resource ID. Queues source voice state, not playback.
   Generations wrap at 16 bits; original signed handle validation is retained,
   including rejection of negative packed generation handles. */
unsigned long __cdecl bh_sound_request(BHSoundQueue *queue,unsigned long resource,
    float level,float pitch,const BHVec3 *position,const BHVec3 *velocity);
int __cdecl bh_sound_update(BHSoundQueue *queue,unsigned long handle,float level,float pitch,
    const BHVec3 *position,const BHVec3 *velocity); /* 0047c050: 1 updated, 0 stale, -1 invalid */
int __cdecl bh_sound_stop(BHSoundQueue *queue,unsigned long handle); /* 0047c140 */
/* Checked source view uses the same signed-generation validation as originals. */
BHSoundVoice *__cdecl bh_sound_voice(BHSoundQueue *queue,unsigned long handle);
/* 0047b710: original single listener (index 0). Nonzero enabled stores the
   exact signed flag, Y(angles.x) -> X(angles.y) -> Z(angles.z) right vector,
   then position and velocity. Disabled retains all three vectors and accepts
   NULL unused inputs. Finite moderate enabled inputs require PC53; invalid
   bindings/index/numerics leave listener fields unchanged. No playback. */
int __cdecl bh_sound_listener(BHSoundQueue *queue,unsigned long index,long enabled,
    const BHVec3 *position,const BHVec3 *angles,const BHVec3 *velocity);
/* 0047b920: request 0 -> 2, 1..3 unchanged, all other signed values -> 3.
   Explicit context replaces original global policy; existing resources retain
   their captured limits. New source PCM registrations can request limit 0. */
int __cdecl bh_sound_set_voice_limit(BHSoundQueue *queue,long requested);
typedef struct BHSoundTickStats {
    unsigned long clock_calls,status_calls,stop_calls,play_calls,position_calls;
    unsigned long volume_calls,pan_calls,frequency_calls,duplicate_calls,recycled;
    HRESULT last_error;
    /* First rejected frequency for diagnostics; original cache still advances. */
    unsigned long frequency_failures,failed_resource,failed_slot;
    long failed_frequency;
} BHSoundTickStats;
/* Complete 0047c200. Resource audible_range and time_scale are explicit loaded
   metadata; phase is decremented by dt, and processing occurs only below zero.
   master_db binds the original software master attenuation, not device volume.
   PC53 finite moderate state, <=3 slots and stable real native buffers/voices.
   Returns 1 / 0 for clean execution / invalid state or native error. Native
   errors are reported while original cache/playing/pending mutation continues;
   this is not rollback. Optional statistics describe actual API calls.
   No active listener freezes all phases/voices. Persistent loop requests retain
   their lifetime until explicit stop; muted one-shots retain original behavior.
   Nonpositioned displacement scratch starts at zero (original unspecified),
   then retains the preceding positioned scratch within this call. */
int __cdecl bh_sound_tick(BHSoundQueue *queue,float dt,float master_db,BHSoundTickStats *stats);
#endif
