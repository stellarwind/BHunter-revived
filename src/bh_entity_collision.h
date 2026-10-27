#ifndef BH_ENTITY_COLLISION_H
#define BH_ENTITY_COLLISION_H
#include "bh_sweep.h"
#include "bh_world_query.h"

/* Borrow ready source sweeps and object lifetimes. Several candidates may share
   a sweep when their source/destination geometry is identical. Actor geometry,
   matrices and world-node per-call movement/flags are explicit owner inputs;
   no original registry or opaque entity fields are invented by this binding. */
typedef struct BHCollisionCandidate
{
    BHWorldHit object;
    BHSweepQuery *sweep;
    BHMat34 world, local;
    BHVec3 movement;     /* per-tick world-node translation; ignored for actors */
    unsigned long flags; /* world query flags; physical actors are unconditional */
} BHCollisionCandidate;
int __cdecl bh_collision_candidate_world(BHCollisionCandidate *out, BHWorldGeometry *world, BHSweepQuery *sweep);
int __cdecl bh_collision_candidate_actor(BHCollisionCandidate *out, BHEntity *actor, BHSweepQuery *sweep,
                                         const BHMat34 *world, const BHMat34 *local, const BHVec3 *movement,
                                         unsigned long flags);
/* 0040b410: first count entries replace original END-terminated mixed array.
   Returns 1 hit, 0 miss, -1 invalid/dependency failure. Initialize out first.
   Multi-candidate hits scale movement/angles, accumulate fractions and swap the
   last hit entry to index 0; single-candidate requests retain input vectors.
   Run allocates nothing. On failure all caller outputs/order are unchanged.
   Sweeps/models and candidates may not be replaced during the call. */
int __cdecl bh_entity_collision_dispatch(BHCollisionCandidate *candidates, unsigned long count, BHMat34 *pose,
                                         BHVec3 *movement, BHVec3 *angles, int segment_fallback, BHSweepHit *out);
#endif
