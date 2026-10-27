#ifndef BH_TARGET_STEP_H
#define BH_TARGET_STEP_H
#include "bh_debris_response.h"
#include "bh_physics_step.h"
#include "bh_script_explosion.h"
#include "bh_script_particles.h"
#include "bh_pickup_draw.h"
typedef struct BHTargetStepState {float countdown;BHVec3 minimum,maximum;long emission,token,loot,counter;} BHTargetStepState;
typedef struct BHTargetStepContext {
 BHDebrisBody *body;BHPhysicsStepContext *physics;BHScriptExplosionContext *explosions;
 BHScriptParticleContext *particles;BHPickupContext *pickups;long *mode; /* original004df250 editor mode, not gameplay mode or pause */
 unsigned long smoke_image;int error;
} BHTargetStepContext;
/* Count-zero binding. Same actual debris body/physics state/owner/sleep word;
   same outside registry as pickup owner. Borrow all owners until retirement.
   Installs actual whole step; query remains real bh_debris_receive. An owner
   receives borrowed event12 through its actual live query; its receiver's
   event12 meaning is NOT restored here. Existing fixed physics roster must be
   disposed/rebound after target retirement before another physics step. */
int __cdecl bh_target_step_bind(BHTargetStepContext *,const BHTargetStepState *);
int __cdecl bh_target_step_read(const BHEntity *,BHTargetStepState *);
int __cdecl bh_target_step_write(BHEntity *,const BHTargetStepState *);
void __cdecl bh_target_step(BHEntity *); /*whole0043cd00*/
#endif
