#ifndef BH_SCRIPT_ACTOR_ROUTE_H
#define BH_SCRIPT_ACTOR_ROUTE_H
#include "bh_actor_route.h"
#include "bh_script_actor_path.h"
typedef void (__cdecl *BHScriptActorPath)(float,BHVec3 *,const BHVec3 *,const BHVec3 *,const BHVec3 *,const BHVec3 *);
/* Complete00436fa0: stop countdown sound, move room ownership, select linked
   aperture and rebuild path. Existing room bindings/world models/portals and
   actor lists must remain live. Reject malformed topology before mutation.
   The selected callback is a real recovered interpolation owner. */
int __cdecl bh_script_actor_route_advance(BHActorRouteContext *,BHEntity *);
BHScriptActorPath __cdecl bh_script_actor_route_path(const BHEntity *);
#endif
