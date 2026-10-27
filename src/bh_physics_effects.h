#ifndef BH_PHYSICS_EFFECTS_H
#define BH_PHYSICS_EFFECTS_H
#include "bh_contact_effects.h"
#include "bh_sound_queue.h"
#include "bh_network_transport.h"
#include "bh_sweep.h"

/* Count-zero binding for the effects block inside whole 004124d0. Borrow all
   real owners; no sound tick, provider setup, or callbacks are substituted. */
typedef struct BHPhysicsEffects
{
    BHContactEffects *effects;
    BHSoundQueue *sound;
    BHNetworkTransport *transport;
    unsigned long impact_sound, quad_sound, rays_sound;
    long peer_count;
    unsigned short sender;
    unsigned long sound_requests, spark_attempts, sparks_created, packets;
    unsigned long last_voice, last_packet_size;
    unsigned char last_packet[20];
    HRESULT last_send;
    int error;
} BHPhysicsEffects;
/* flags2 precedes4. Normal level is the original stored binary32 L; append
   contact position using the current world's verified index, then send type6
   before spark allocation. current_world=NULL suppresses only the packet.
   Source diagnostics do not suppress subsequent original side effects. */
int __cdecl bh_physics_effects_run(BHPhysicsEffects *, const BHSweepHit *, float level,
                                   const BHWorldGeometry *current_world, unsigned short world_index);
#endif
