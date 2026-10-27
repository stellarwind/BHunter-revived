#ifndef BH_SWEEP_H
#define BH_SWEEP_H
#include "bh_collision_query.h"
#include "bh_segment_query.h"

typedef struct BHSweepHit
{
    float fraction;
    BHCollisionHit surface;
    BHVec3 motion;
    int surface_present;
} BHSweepHit;
typedef struct BHSweepQuery
{
    BHGeometry *a, *b;
    BHCollisionQuery collision;
    BHSegmentQuery segment;
    BHBspError error;
    int initialized, busy;
} BHSweepQuery;
/* Borrow immutable topology and live flags. NULL model pointers retain original
   non-colliding-model paths (including destination-only segment fallback).
   Dispose this workspace before either geometry. Never reinitialize live owners. */
int __cdecl bh_sweep_init(BHSweepQuery *query, BHGeometry *a, BHGeometry *b);
int __cdecl bh_sweep_dispose(BHSweepQuery *query);
/* 0044da20: translate then rotate local X/Y/Z; endpoint collision followed by
   exactly three bisections and optional origin-segment fallback. b_world/local
   are explicit original transform pairs. On hit commits last clear pose and
   fraction; on miss commits full motion and fraction=1. Other surface/motion
   fields change only when actual mesh contact is produced; surface_present
   makes the original fallback-without-new-contact case explicit. Invalid input
   returns -1, preserving matrix/output. Initialize out before its first call.
   No allocation during run. Not an
   analytic continuous time-of-impact solver; PC53 finite, rigid-transform scope. */
int __cdecl bh_sweep_run(BHSweepQuery *query, BHMat34 *pose, const BHVec3 *movement, const BHVec3 *angles,
                         const BHMat34 *b_world, const BHMat34 *b_local, int segment_fallback, BHSweepHit *out);
#endif
