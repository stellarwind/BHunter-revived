#ifndef BH_WORLD_QUERY_H
#define BH_WORLD_QUERY_H
#include "bh_entities.h"
#include "bh_route_query.h"

typedef enum BHWorldHitKind {
    BH_WORLD_HIT_END,
    BH_WORLD_HIT_GEOMETRY,
    BH_WORLD_HIT_ACTOR
} BHWorldHitKind;
typedef struct BHWorldHit {
    BHWorldHitKind kind;
    union {
        BHWorldGeometry *geometry;
        BHEntity *actor;
    } object;
} BHWorldHit;
typedef enum BHWorldQueryError {
    BH_QUERY_OK,
    BH_QUERY_ARGUMENT,
    BH_QUERY_MEMORY,
    BH_QUERY_TOPOLOGY,
    BH_QUERY_CAPACITY,
    BH_QUERY_BUSY
} BHWorldQueryError;
typedef struct BHWorldQuery {
    BHWorldGeometry *first;
    unsigned long scene_count;
    BHEntity *const *actors;
    unsigned long actor_count;
    BHWorldHit *hits;
    unsigned long hit_count, capacity;
    BHSceneAllocator allocator;
    BHWorldQueryError error;
    void *members, *visited, *frames; /* private owning workspace */
    BHWorldHit *work;
    int busy;
} BHWorldQuery;

/* Fresh owner; dispose before reinitializing. Borrow the stable scene ring and
   actor-list storage/lifetimes. query_flags are explicit applied model/script
   flags; collision_radius is the verified common entity +c0 field, initialized
   by each real actor constructor (pool recycling does not supply that default).
   Up to 65535 members of each kind. Init failure leaves caller output unchanged. */
int __cdecl bh_world_query_init(BHWorldQuery *query, BHWorldGeometry *first,
                                unsigned long scene_count, BHEntity *const *actors,
                                unsigned long actor_count, const BHSceneAllocator *allocator);
int __cdecl bh_world_query_dispose(BHWorldQuery *query);
/* 0040b300 with owned 0040abe0 DFS and 0040af20 actor broad phase.
   Select previous if it contains the point; otherwise try exact then expanded
   owner lookup, finally retaining previous even outside bounds. If no owner,
   still collect actors. Static output is room, eligible following children
   (query_flags bit 2, inclusive AABB), then portal DFS in local slot order using
   original aperture extents. Actors append in caller array order using STRICT
   axis overlap, excluding self. END terminator always follows successful hits.
   This is candidate collection, not narrow-phase collision or damage delivery.
   Per-query visitation replaces temporary original +fc writes and is cleared.
   Negative margin/radius retains original signed behavior. Queries allocate
   nothing, replace prior results/owner only on success, and reject topology
   mutation, foreign links, nonfinite inputs or overflow of owned capacity. */
int __cdecl bh_world_query_run(BHWorldQuery *query, BHEntity *self, BHWorldGeometry *previous,
                               const BHVec3 *point, float margin, BHWorldGeometry **owner);
#endif
