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
/* Complete0047b950, FIVE original args after explicit source context. Owns
   VFILE/read buffer, real native buffer upload and real resource publication.
   Native format is forwarded without a PCM decoder: bounded PCM16-byte fmt
   uses an equivalent zero-extension; other formats retain checked cbSize data.
   Name length<64, file<=64MiB, safe fmt/data spans and policy1..3 are guards.
   Original Unlock errors do not suppress publication. sample_bytes records
   FIRST lock span, no_more uses OLD voice_limit before current policy update,
   and only current policy cache slots reset. No Play, cache lookup or parser.
   PC53 caller; original PC24 equivalence remains unvalidated. */
unsigned long __cdecl bh_sound_load(BHSoundLoad *, const char *name, float audible_range, float level,
                                    unsigned short streaming, float interval_denominator);
#endif
