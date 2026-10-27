#ifndef BH_PROJECTILE_CREATE_H
#define BH_PROJECTILE_CREATE_H
#include "bh_ballistic_projectile.h"
typedef struct BHProjectileDrawContext
{
    BHDrawQueue *queue;
    unsigned long ballistic_flare, ballistic_trail, homing_flare, homing_trail, pulse_image;
    unsigned long attempted, queued, random_calls;
    int error;
} BHProjectileDrawContext;
typedef struct BHProjectileCreateContext
{
    BHProjectileLifetime *lifetime;
    BHBallisticProjectileContext *ballistic;
    BHHomingProjectileContext *homing;
    BHProjectileDrawContext *draw;
    unsigned long *next_identity;
    unsigned long loop_sound, launch_sound;
    int error;
} BHProjectileCreateContext;
/* Whole004408e0/004409d0. Explicit original pool/sound/renderer/global owners
   must be initialized and outlive each created entity. Returned entity is
   source convenience; original owner returns void. World and source remain
   borrowed. Homing target must have a real synchronous event3 query.
   Original unused/recycled words remain unchanged, including ballistic12c
   and homing100..108 when no target is supplied. No original code executed. */
BHEntity *__cdecl bh_projectile_create_ballistic(BHProjectileCreateContext *, BHWorldGeometry *, BHEntity *source,
                                                 const BHVec3 *, const BHVec3 *, float lifetime);
BHEntity *__cdecl bh_projectile_create_homing(BHProjectileCreateContext *, BHWorldGeometry *, BHEntity *source,
                                              const BHVec3 *, const BHVec3 *, float lifetime, BHEntity *target);
/* Whole visual producers0043e620/0043e710/00441ce0. Actual queue owners,
   image identities and PC53 profile; no original backend invocation.
   Return0 queue-full (other producers still attempted),-1 binding error. */
int __cdecl bh_projectile_ballistic_draw(BHProjectileDrawContext *, const BHEntity *);
int __cdecl bh_projectile_homing_draw(BHProjectileDrawContext *, const BHEntity *);
int __cdecl bh_projectile_pulse_draw(BHProjectileDrawContext *, const BHEntity *);
#endif
