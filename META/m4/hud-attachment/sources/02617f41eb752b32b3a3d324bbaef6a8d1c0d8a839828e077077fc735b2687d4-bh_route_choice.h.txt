#ifndef BH_ROUTE_CHOICE_H
#define BH_ROUTE_CHOICE_H
#include "bh_entities.h"
#include "bh_world_geometry.h"
/* Complete 004344f0 over the existing real source world/portal owners. Original
   registry00497230[room+f8].portal_count (+18 in its372-byte descriptor) is
   room->model->portal_count; original room+204/stride3c is each portal.neighbor.
   Neighbor +114 is its existing query_flags. No weapon slots are involved.
   Actor signed class+fc and RNG state+1e8 remain in the real BHEntity payload.
   First eligible preferred-neighbor slot wins, even if it equals excluded.
   Otherwise eligible slots exclude that index; rank derives from exactly one
   original32-bit RNG transition. No eligible slot returns excluded unchanged
   and consumes no RNG. Caller retains all world/model/neighbor/actor lifetimes.
   Returns0 for missing pointers or inconsistent/>6 source portal counts before
   mutation,1 after publishing selected slot. Output must not alias inputs. */
int __cdecl bh_route_choose_exit(BHEntity *actor,const BHWorldGeometry *room,
    const BHWorldGeometry *preferred,long excluded,long *selected);
#endif
