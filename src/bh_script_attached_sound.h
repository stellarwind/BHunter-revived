#ifndef BH_SCRIPT_ATTACHED_SOUND_H
#define BH_SCRIPT_ATTACHED_SOUND_H
#include "bh_script_lifetime.h"
typedef struct BHScriptAttachedSoundState {
    BHEntity *actor;
    long script_loaded;
    unsigned long handle,resource;
    long level,target;
} BHScriptAttachedSoundState;
/* Source binding adapter, not recovery of the VM's constructor branch.
   Persistent lifetime context, actual actor table, pool and sound queue are
   borrowed. The actor table supplies the existing verified one-level owner
   and actor+104 activity contract. Installs actual00443f90 query cleanup.
   Writes only the verified actor/loaded/handle/resource/counter/target fields
   and the two existing private timer/lifetime bindings, retaining other bytes. */
int __cdecl bh_script_attached_sound_bind(BHScriptLifetimeContext *,BHEntity *,
                                         const BHScriptAttachedSoundState *);
int __cdecl bh_script_attached_sound_read(BHEntity *,BHScriptAttachedSoundState *);
/* Whole004436f0. Signed counter moves one toward target BEFORE sampling
   activity. Rising1 requests literal binary32 1/15; other changed nonzero
   levels update with binary64 1/15 rounded to binary32. Falling0 stops and
   invalidates. Equal counters do not update position. Requests/updates use
   actual actor position and zero velocity. Invalid handle plus active owner
   sets target15; any other handle plus inactive owner sets target0. Stale
   handles remain distinct from -1. Failed request is retained as -1 without
   fabricated success; next target comes from the normal activity branch.
   Typical VM counter domain is0..15; signed wrap is preserved outside it.
   Tick never releases the entity. No playback or sound-tick call occurs. */
void __cdecl bh_script_attached_sound_tick(BHEntity *);
#endif
