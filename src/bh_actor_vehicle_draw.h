#ifndef BH_ACTOR_VEHICLE_DRAW_H
#define BH_ACTOR_VEHICLE_DRAW_H
#include "bh_actor_route.h"
#include "bh_draw_queue.h"
typedef struct BHActorVehicleDrawContext {
 BHDrawQueue *queue;BHSoundQueue *sound;
 unsigned long damage_resource,glowing_model,glow_first,glow_second;
 unsigned short tick;long animation_clock;BHActorRouteError error;
} BHActorVehicleDrawContext;
/* Whole00434920; borrowed queue/sound/resource bindings. Does not draw natively
   or play audio: actual restored queue owners retain those operations. */
int __cdecl bh_actor_vehicle_draw(BHActorVehicleDrawContext *,BHEntity *);
#endif
