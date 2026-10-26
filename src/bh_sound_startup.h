#ifndef BH_SOUND_STARTUP_H
#define BH_SOUND_STARTUP_H
#include "bh_sound_queue.h"
typedef struct BHSoundStartupStats {
    unsigned long create_calls,sleep_calls,cooperative_calls,primary_calls,play_calls;
    HRESULT create_result,cooperative_result,primary_result,play_result;
} BHSoundStartupStats;
/* Whole0047b500. Requires a fresh initialized actual queue with no resources,
   voices or native interfaces and an application HWND. Create uses the native
   default device, up to10 attempts,100ms between failures. Success owns one
   device reference and any primary reference directly in the queue. Native
   primary Play(0,0,LOOPING) is part of this owner. No samples are uploaded.
   Creation failure returns0. Cooperative/primary/Play errors retain original
   success return1 once the device exists; diagnostics preserve actual results.
   Dispose through actual bh_sound_queue_dispose. No COM interfaces are mocked. */
int __cdecl bh_sound_startup(BHSoundQueue *,HWND,BHSoundStartupStats *);
#endif
