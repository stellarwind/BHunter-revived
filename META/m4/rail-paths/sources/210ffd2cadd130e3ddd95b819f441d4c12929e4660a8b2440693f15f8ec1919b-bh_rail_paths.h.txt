#ifndef BH_RAIL_PATHS_H
#define BH_RAIL_PATHS_H
#include "bh_math.h"
#include "bh_scene.h"
typedef struct BHRailSegment BHRailSegment;
struct BHRailSegment {
    BHVec3 start,end;
    float rise,length;
    BHRailSegment *next,*previous;
    long visited;
};
typedef struct BHRailPaths {
    BHRailSegment *segments;
    long count,chain_count;
    BHSceneAllocator allocator;
    unsigned long additions,dropped,placements,failures;
} BHRailPaths;
/* Source ownership adapters. One stable200*44-byte allocation; init a fresh
   context and dispose before rebinding. Segment pointers expire on disposal.
   Link may reorder records and repairs their actual links as the original does.
   Caller profile: x87 PC53/nearest/masked. CRT sin/cos cover finite moderate
   yaw inputs; extreme FSIN/FCOS/PC24 equivalence remains unvalidated. */
int __cdecl bh_rail_paths_init(BHRailPaths *,const BHSceneAllocator *);
void __cdecl bh_rail_paths_dispose(BHRailPaths *);
/* Whole004458a0: squared distance<.25, including unordered as original C0.
   X/Z differences multiply their binary32 spills; Y remains unspilled. */
int __cdecl bh_rail_near(const BHVec3 *,const BHVec3 *);
/* Whole00445910. Rotation uses rotation->x only. Coordinate float stores
   precede world translation. Full capacity is an original no-op (return1).
   Source rejects invalid owner/count, nonfinite or extreme yaw and aliasing
   input vectors into mutable segment storage before mutation. */
int __cdecl bh_rail_add(BHRailPaths *,const BHVec3 *start,const BHVec3 *end,
    const BHVec3 *position,const BHVec3 *rotation);
/* Whole00445a00: case-insensitive RC_ prefix, ASCII digit0..9 and literal
   segment geometry. Other names are successful no-ops. */
int __cdecl bh_rail_placement(BHRailPaths *,const char *name,
    const BHVec3 *position,const BHVec3 *rotation);
/* Whole00445e70: first-match endpoint chains, reversal, swap/link repair,
   optional loop closure, then rise and length for every registered segment.
   Source validates all existing links point into this owner before mutation. */
int __cdecl bh_rail_link(BHRailPaths *);
#endif
