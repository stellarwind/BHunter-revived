#ifndef BH_ACTOR_VEHICLE_LIFETIME_H
#define BH_ACTOR_VEHICLE_LIFETIME_H
#include "bh_actor_route.h"
typedef struct BHActorVehicleLifetimeContext
{
    BHEntityPool *pool;
    BHActorRouteContext *route;
    BHEntity *head;
    unsigned long count;
} BHActorVehicleLifetimeContext;
/* Whole004397e0; source-owned global ec/f0 list plus existing real room f4/f8
   list. Broadcast17 follows sound stop and precedes both list unlink operations.
   Bindings/lists remain borrowed during synchronous callbacks. */
int __cdecl bh_actor_vehicle_release(BHActorVehicleLifetimeContext *, BHEntity *);
#endif
