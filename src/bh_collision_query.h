#ifndef BH_COLLISION_QUERY_H
#define BH_COLLISION_QUERY_H
#include "bh_collision_model.h"

typedef struct BHCollisionHit
{
    BHFaceContact contact;
    BHVec3 normal;
    long a_node, b_node, a_face, b_face;
    unsigned long flags;
} BHCollisionHit;

typedef struct BHCollisionQuery
{
    BHGeometry *a, *b;
    BHSceneAllocator allocator;
    void *workspace, *a_stack;
    long *b_stack;
    unsigned long *suppressed;
    unsigned long a_capacity, b_capacity, work;
    BHBspError error;
    int busy;
} BHCollisionQuery;

/* Borrow both geometries, including immutable node bounds and live face flags.
   Dispose before either geometry changes topology/lifetime. Allocates workspace
   once; run allocates nothing. Rigid orthonormal transforms are caller-owned.
   Return 1 contact, 0 miss, -1 invalid/limit; output changes only on contact.
   Front lists precede back lists; first contact is traversal order, not closest.
   Malformed/cyclic references reject with bounded work and counters restored. */
int __cdecl bh_collision_query_init(BHCollisionQuery *query, BHGeometry *a, BHGeometry *b);
int __cdecl bh_collision_query_dispose(BHCollisionQuery *query);
/* 0044c200: fixed source node against destination node/subtree. */
int __cdecl bh_collision_query_node(BHCollisionQuery *query, long a_node, long b_node, const BHMat34 *a_to_b,
                                    const BHMat34 *b_to_world, BHCollisionHit *out);
/* 0044d2f0: source subtree against destination subtree, ancestor face suppression. */
int __cdecl bh_collision_query_run(BHCollisionQuery *query, long a_node, long b_node, const BHMat34 *a_to_b,
                                   const BHMat34 *b_to_world, BHCollisionHit *out);
#endif
