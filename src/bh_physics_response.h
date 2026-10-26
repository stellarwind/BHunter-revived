#ifndef BH_PHYSICS_RESPONSE_H
#define BH_PHYSICS_RESPONSE_H
#include "bh_entity_collision.h"

typedef struct BHPhysicsState {
    BHMat34 pose,last_clear;
    BHVec3 velocity,angular_velocity,acceleration,angular_acceleration,euler;
} BHPhysicsState;
/* Original event 2 packet at +0/+4/+10/+1c/+20, not direct-damage event 0. */
typedef struct BHPhysicsContactPacket {
    long event;
    BHVec3 point,normal;
    float impulse,severity;
} BHPhysicsContactPacket;
typedef struct BHPhysicsResponse {
    BHPhysicsContactPacket first,second;
    int emit_first,emit_second;
    BHVec3 jittered_motion;
    float speed,slide_fraction,audio_level;
    unsigned long material_flags;
} BHPhysicsResponse;
/* Complete 0044d480 over real source query state. NULL model path misses. */
int __cdecl bh_physics_overlap(BHSweepQuery *query,const BHMat34 *pose,
    const BHMat34 *world,const BHMat34 *local,BHCollisionHit *out);
/* Extracted arithmetic from 004124d0, NOT admission of its full owner.
   These helpers do not install effects, queue audio/network packets or invoke
   actor response callbacks. The complete orchestration is supplied separately
   by bh_physics_step, with explicit registry and shared-state ownership.
   PC53, finite moderate inputs, dt>0; initialize state/output before use.
   Success is 1, invalid numerical input is 0 and leaves caller output alone. */
int __cdecl bh_physics_prepare(BHPhysicsState *state,float dt,float radius,
    BHVec3 *movement,BHVec3 *angles,float *broad_margin);
int __cdecl bh_physics_slide(const BHVec3 *movement,const BHSweepHit *contact,
    float coefficient,BHVec3 *slide);
/* Consumes exactly three calls to the real shared VC5 CRT rand, including
   special surfaces. Records event packets; future owner must deliver first
   then second at the reviewed point in the effects/callback lifecycle. */
int __cdecl bh_physics_contact_response(const BHSweepHit *contact,float slide_fraction,
    float dt,float first_mass,float second_mass,BHVec3 *acceleration,BHPhysicsResponse *out);
int __cdecl bh_physics_finish(BHPhysicsState *state,const BHMat34 *previous,
    float dt,int no_collision);
#endif
