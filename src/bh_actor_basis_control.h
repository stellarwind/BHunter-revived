#ifndef BH_ACTOR_BASIS_CONTROL_H
#define BH_ACTOR_BASIS_CONTROL_H
#include "bh_actor_basis.h"
#include "bh_physics_response.h"

/* Whole00413990/00413c80. Borrow actual initialized basis arrays; no copies or
   invented global tables. Outputs must be distinct and disjoint from inputs.
   Pair search preserves original order, strict improvement, positive-weight
   gates and 1000000 cap. A searches torque and returns1; B searches direction and returns-1;
   no pair returns0 with all outputs zero. PC53/nearest, moderate finite vectors
   and finite nonnegative scales; parallel pairs are skipped. */
int __cdecl bh_actor_basis_a_pair(const BHActorBasisContext *, const BHVec3 *, int *, int *, float *, float *);
int __cdecl bh_actor_basis_b_pair(const BHActorBasisContext *, const BHVec3 *, int *, int *, float *, float *);
/* Whole004146f0 over the SAME authoritative physics component used by the
   real step owner. accumulated[15] is caller-owned original player+2bc array;
   inertia is original+78. Group count includes its leading count word, so
   count1 visits no samples. Original repeated indices apply repeatedly.
   Valid sample indices0..14, nonzero finite mass/inertia, disjoint storage.
   Source guards reject invalid binding/index before any mutation. */
int __cdecl bh_actor_basis_apply_group(const BHActorBasisContext *, BHPhysicsState *, float mass, const BHVec3 *inertia,
                                       float accumulated[15], const BHActorBasisIndices *, float);
#endif
