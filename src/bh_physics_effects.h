#ifndef BH_PHYSICS_EFFECTS_H
#define BH_PHYSICS_EFFECTS_H
#include "bh_contact_effects.h"
#include "bh_sound_queue.h"
#include "bh_sweep.h"

/* Count-zero binding for the effects block inside whole 004124d0. Borrow all
   real owners; no sound tick, provider setup, or callbacks are substituted. */
typedef struct BHPhysicsEffects
{
    BHContactEffects *effects;
    BHSoundQueue *sound;
    unsigned long impact_sound, quad_sound, rays_sound;
    unsigned long sound_requests, spark_attempts, sparks_created;
    unsigned long last_voice;
    int error;
} BHPhysicsEffects;
int __cdecl bh_physics_effects_run(BHPhysicsEffects *, const BHSweepHit *, float level);
#endif
