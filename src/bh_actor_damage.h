#ifndef BH_ACTOR_DAMAGE_H
#define BH_ACTOR_DAMAGE_H
#include "bh_actor_route_lifetime.h"
#include "bh_script_particles.h"
#include "bh_script_debris.h"
#include "bh_target_construct.h"
typedef struct BHActorDamageContext
{
    BHEntityPool *pool;
    BHActorRouteContext *route;
    BHScriptParticleContext *particles;
    BHScriptDebrisContext *fragments;
    BHTargetConstructContext *targets;
    const unsigned long (*loot_rows)[32];
    unsigned long loot_row_count;
    unsigned long smoke_resource, death_sound;
    long mode;
    /* Original4357d0 passes unwritten stack angles to43d7c0. Source requires
    an explicit caller binding rather than reading uninitialized C storage. */
    const BHVec3 *legacy_death_angles;
    BHActorRouteError error;
} BHActorDamageContext;
/* Count-zero source callback/context binding; borrowed until entity release. */
int __cdecl bh_actor_damage_bind(BHActorDamageContext *, BHEntity *);
void __cdecl bh_actor_damage_tick(BHEntity *);          /*004357d0*/
void __cdecl bh_actor_damage_query(BHEntity *, void *); /*00435e80*/
#endif
