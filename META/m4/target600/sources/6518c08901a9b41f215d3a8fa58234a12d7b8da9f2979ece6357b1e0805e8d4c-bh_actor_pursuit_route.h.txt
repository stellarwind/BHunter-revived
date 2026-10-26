#ifndef BH_ACTOR_PURSUIT_ROUTE_H
#define BH_ACTOR_PURSUIT_ROUTE_H
#include "bh_actor_route.h"
#include "bh_navigation.h"
#include "bh_messages.h"
typedef struct BHActorPursuitRouteContext {
 BHActorRouteContext *route;const BHNavigation *navigation;
 BHEntity *player,*actor_head;unsigned long actor_limit;
 /* Source node index -> original city ID(+11c), distinct from graph index. */
 const long *city_ids;const char *const *city_names;unsigned long city_count;
 BHMessageState *messages;long clock,last_report;
} BHActorPursuitRouteContext;
/* Whole00438ff0: borrowed graph/world/target/query/message owners. */
int __cdecl bh_actor_pursuit_route(BHActorPursuitRouteContext *,BHEntity *);
#endif
