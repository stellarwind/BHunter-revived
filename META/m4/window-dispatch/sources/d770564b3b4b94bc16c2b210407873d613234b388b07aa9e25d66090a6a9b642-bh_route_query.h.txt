#ifndef BH_ROUTE_QUERY_H
#define BH_ROUTE_QUERY_H
#include "bh_navigation.h"

/* 0040a8c0: inclusive AABB ownership, with neighbor-slot priority before a
   ring scan starting at hint (or first). Owned child placements are skipped.
   Signed margin is preserved: positive expands, negative shrinks. All inputs
   and ring members must be accessible/live; malformed ring, foreign hint or
   nonfinite point/margin returns NULL. No nearest-room fallback is invented. */
BHWorldGeometry *__cdecl bh_world_locate(BHWorldGeometry *first,unsigned long scene_count,
                                        BHWorldGeometry *hint,const BHVec3 *point,float margin);
/* 00433f70/00434040: scratch aperture center and signed axis normal. Valid
   direction codes are 0/1/2/4/5/6. Invalid direction leaves output unchanged.
   Center uses scratch, which may differ from overlap after runtime clipping. */
int __cdecl bh_portal_center(const BHPortal *portal,BHVec3 *out);
int __cdecl bh_portal_normal(const BHPortal *portal,BHVec3 *out);

typedef struct BHRoutePlan {
    BHWorldGeometry *next;
    long exit_slot;
    BHVec3 goal,portal_center,projected,anchor;
    float parameter_rate;
} BHRoutePlan;
/* 00429480: current and the located lookup_goal must be junctions in the
   source graph. projection_goal is the caller-selected waypoint, separately
   supplied from the original shared waypoint-array/current-index globals.
   Preserve the normal-line projection and 4.666... / distance-sum rate,
   including infinity for zero length under the established masked PC53 profile.
   No synthetic AI/entity callbacks or guessed waypoint globals are required.
   Invalid/unreachable route rejects with output unchanged; topology and routing
   metadata must remain live and immutable during the query. */
int __cdecl bh_route_plan(const BHNavigation *navigation,BHWorldGeometry *first,
                          unsigned long scene_count,BHWorldGeometry *current,
                          const BHVec3 *lookup_goal,const BHVec3 *projection_goal,BHRoutePlan *out);
#endif
