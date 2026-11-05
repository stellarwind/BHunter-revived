#ifndef BH_SOUND_STARTUP_H
#define BH_SOUND_STARTUP_H
#include "bh_sound_queue.h"
typedef struct BHSoundStartupStats
{
    unsigned long create_calls, sleep_calls, cooperative_calls, primary_calls, play_calls;
    long create_result, cooperative_result, primary_result, play_result;
} BHSoundStartupStats;
/* Fresh initialized queue. Owns an SDL mixer reference; no window dependency.
   An unavailable output is retried by the platform poll without discarding
   resources or queued voices. Dispose through bh_sound_queue_dispose. */
int __cdecl bh_sound_startup(BHSoundQueue *, BHSoundStartupStats *);
#endif
