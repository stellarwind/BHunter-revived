#ifndef BH_SOUND_LOAD_H
#define BH_SOUND_LOAD_H
#include "bh_sound_queue.h"
typedef enum BHSoundLoadError
{
    BH_SOUND_LOAD_OK,
    BH_SOUND_LOAD_ARGUMENT,
    BH_SOUND_LOAD_IO,
    BH_SOUND_LOAD_MEMORY,
    BH_SOUND_LOAD_FORMAT,
    BH_SOUND_LOAD_NATIVE,
    BH_SOUND_LOAD_NUMERICAL
} BHSoundLoadError;
typedef struct BHSoundLoad
{
    BHSoundQueue *queue;
    BHSceneAllocator file_allocator; /* optional pair; NULL pair uses real CRT */
    unsigned long files, allocations, native_calls, releases, loads, failures;
    unsigned long file_bytes, first_bytes, second_bytes;
    HRESULT create_result, lock_result, unlock_result, frequency_result;
    BHSoundLoadError error;
} BHSoundLoad;
/* Original five-argument resource loader and publication policy. Reads a
   bounded VFILE RIFF and uploads mono/stereo PCM8/PCM16 into the SDL backend.
   Keeps scheduling metadata, random phase, cache reset and voice-limit policy.
   Unsupported codecs fail without publishing a resource. No playback. */
unsigned long __cdecl bh_sound_load(BHSoundLoad *, const char *name, float audible_range, float level,
                                    unsigned short streaming, float interval_denominator);
#endif
