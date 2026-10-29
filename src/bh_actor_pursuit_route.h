#ifndef BH_ACTOR_PURSUIT_ROUTE_H
#define BH_ACTOR_PURSUIT_ROUTE_H
#include "bh_actor_route.h"
#include "bh_navigation.h"
#include "bh_messages.h"
typedef struct BHActorPursuitRouteContext
{
    BHActorRouteContext *route;
    const BHNavigation *navigation;
    BHEntity *player, *actor_head;
    unsigned long actor_limit;
    BHEntity **actor_head_owner; /* live004dfb28 for00421610 */
    /* Source node index -> original city ID(+11c), distinct from graph index. */
    const long *city_ids;
    const char *const *city_names;
    unsigned long city_count;
    BHMessageState *messages;
    long clock, last_report;
} BHActorPursuitRouteContext;
/* Whole00438ff0: borrowed graph/world/target/query/message owners. */
int __cdecl bh_actor_pursuit_route(BHActorPursuitRouteContext *, BHEntity *);
/* Internal 0043950c target-control subpath; caller already checked world/target
   eligibility. This is part of 00438ff0, not another original function. */
int __cdecl bh_actor_pursuit_target_controls(BHActorPursuitRouteContext *, BHEntity *);
#endif
