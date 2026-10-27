#ifndef BH_SEGMENT_OWNER_H
#define BH_SEGMENT_OWNER_H
#include "bh_collision_cache.h"
#include "bh_segment_query.h"
/* Lazy whole0044d9b0 owner. Zero initialize before use, dispose before BSP
   replacement. Owns actual collision cache/face flags and traversal frames;
   borrows decoded immutable BSP and explicit world/local transform pair.
   Mutable face flags are cache.face_flags, shared by its prepared query.
   NULL bsp is the original no-op, leaves result and owner unchanged.
   Return1 hit,0 miss/no-op,-1 source validation/allocation failure.
   Complete preparation calls admitted0044b130, transforms/traversal call real
   admitted0044d530. No collision metadata or transform inverse is guessed. */
typedef struct BHSegmentOwner
{
    BHCollisionCache cache;
    BHGeometry geometry;
    BHSegmentQuery query;
} BHSegmentOwner;
int __cdecl bh_segment_owner_query(BHSegmentOwner *, const BHBsp *, const BHVec3 *start, const BHVec3 *end,
                                   const BHMat34 *to_world, const BHMat34 *to_local, BHSegmentHit *result);
int __cdecl bh_segment_owner_dispose(BHSegmentOwner *);
#endif
