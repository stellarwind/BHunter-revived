#ifndef BH_SCRIPT_LIFETIME_H
#define BH_SCRIPT_LIFETIME_H
#include "bh_script_timer.h"
#include "bh_sound_queue.h"
typedef struct BHScriptLifetimeContext
{
    BHScriptTimerContext timers;
    BHSoundQueue *sounds;
    unsigned long mission_sound[2];
} BHScriptLifetimeContext;
typedef struct BHScriptLifetimeState
{
    BHEntity *actor;
    long script_loaded;
    unsigned long sound, mission_resource;
    float countdown, mission_countdown;
    BHVec3 delta;
} BHScriptLifetimeState;
/* Count-zero explicit state/context binding. Context, pool, sound queue and
   actor outlive callbacks; timer context is also bound for actual00442eb0.
   A loaded flag is tested only by callbacks whose original bodies test it. */
int __cdecl bh_script_lifetime_bind(BHScriptLifetimeContext *, BHEntity *, const BHScriptLifetimeState *,
                                    BHEntityCallback);
int __cdecl bh_script_lifetime_read(BHEntity *, BHScriptLifetimeState *);
/* Source-private timer/lifetime pointers only; preserves cue time/resource/step. */
int __cdecl bh_script_lifetime_bind_context(BHScriptLifetimeContext *, BHEntity *);
void __cdecl bh_script_unloaded_query(BHEntity *, void *);            /*00442ec0*/
void __cdecl bh_script_sound_expiry(BHEntity *);                      /*00443350*/
void __cdecl bh_script_motion_query(BHEntity *, void *);              /*00443e80*/
void __cdecl bh_script_sensor_query(BHEntity *, void *);              /*00443f00*/
void __cdecl bh_script_sound_query(BHEntity *, void *);               /*00443f50*/
void __cdecl bh_script_sound_actor_query(BHEntity *, void *);         /*00443f90*/
void __cdecl bh_mission_sound_tick(BHEntity *);                       /*00437a30*/
BHEntity *__cdecl bh_mission_sound_first(BHScriptLifetimeContext *);  /*00437a80*/
BHEntity *__cdecl bh_mission_sound_second(BHScriptLifetimeContext *); /*00437ab0*/
#endif
