#ifndef BH_SEGMENT_QUERY_H
#define BH_SEGMENT_QUERY_H
#include "bh_geometry.h"

typedef struct BHSegmentHit {
    float fraction;
    BHVec3 point, normal;
    long face;
    unsigned long flags;
} BHSegmentHit;

typedef struct BHSegmentQuery {
    BHGeometry *geometry;
    BHSceneAllocator allocator;
    void *frames;
    unsigned long capacity;
    BHBspError error;
    int busy;
} BHSegmentQuery;

/* Fresh owner; borrows immutable decoded geometry and its live face flags.
   This supplies the collision metadata already owned by bh_geometry_init and
   bh_bsp_decode, rather than the original lazy allocation in 0044b130. */
int __cdecl bh_segment_query_init(BHSegmentQuery *query, BHGeometry *geometry);
int __cdecl bh_segment_query_dispose(BHSegmentQuery *query);
/* 0044d530 over source BSP data, with the transform setup of 0044d9b0.
   Return 1 for first traversal-order contact, 0 for miss, -1 for invalid input
   or traversal limits. Out is replaced only on contact. This is not a closest-
   polygon search: visit near child, front/back face lists, then far child.
   Flag bit 1 disables a face; back-list normals are reversed. Contact point
   includes original +0.05*world_normal offset; fraction is absolute plane ratio.
   Transforms are explicit original world/local pairs; no inverse is guessed.
   Query allocates nothing. Topology must stay live; bounded work rejects cycles
   or excessive repeated DAG traversal. Coplanar original arithmetic is retained
   under masked PC53, including nonfinite intermediate results. */
int __cdecl bh_segment_query_run(BHSegmentQuery *query, const BHVec3 *start, const BHVec3 *end,
                                 const BHMat34 *to_world, const BHMat34 *to_local,
                                 BHSegmentHit *out);
#endif
