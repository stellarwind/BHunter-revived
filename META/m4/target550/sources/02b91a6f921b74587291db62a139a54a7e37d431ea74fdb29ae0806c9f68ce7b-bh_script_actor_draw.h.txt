#ifndef BH_SCRIPT_ACTOR_DRAW_H
#define BH_SCRIPT_ACTOR_DRAW_H
#include "bh_script_actor_route.h"
#include "bh_script_particles.h"
typedef struct BHScriptActorDrawContext {
 BHDrawQueue *queue;BHScriptParticleContext *particles;
 unsigned long smoke_resource;unsigned short tick;BHActorRouteError error;
 unsigned long glowing_model,glow_first,glow_second;float animation_time;
} BHScriptActorDrawContext;
/* Whole00436cd0. Actual queue/particle owners and loaded resource handles
   remain borrowed. Shared CRT rand draw order is observable. No native draw. */
int __cdecl bh_script_actor_draw(BHScriptActorDrawContext *,BHEntity *);
int __cdecl bh_script_actor_vehicle_draw(BHScriptActorDrawContext *,BHEntity *); /*004366d0*/
#endif
