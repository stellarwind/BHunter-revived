#ifndef BH_ACTOR_BASIS_H
#define BH_ACTOR_BASIS_H
#include "bh_math.h"
typedef struct BHActorBasisSample
{
    BHVec3 direction, torque, point;
} BHActorBasisSample;
typedef struct BHActorBasisGroup
{
    BHVec3 direction, torque, unit_direction, unit_torque;
} BHActorBasisGroup;
typedef struct BHActorBasisIndices
{
    long count;
    long sample[15];
} BHActorBasisIndices;
typedef struct BHActorBasisScale
{
    float direction, torque;
} BHActorBasisScale;
/* Borrowed equivalents of original sample and two group-array global families.
   refcount preserves original32-bit increment/wrap and zero-to-one init gate.
   Arrays must be disjoint. Existing entries/counts are deliberately retained;
   group definitions reference absolute samples0..13 even when appending. */
typedef struct BHActorBasisContext
{
    unsigned long refcount, sample_count, a_count, b_count;
    BHActorBasisSample *samples;
    unsigned long sample_capacity;
    BHActorBasisGroup *groups_a, *groups_b;
    BHActorBasisIndices *indices_a, *indices_b;
    BHActorBasisScale *scales_a, *scales_b;
    unsigned long a_capacity, b_capacity;
} BHActorBasisContext;
/* Exact original static group definitions0048f898 and0048f900, each ending-1. */
extern const long bh_actor_basis_definition_a[25];
extern const long bh_actor_basis_definition_b[22];
/* Complete00413420: append seven groups to each family with exact per-add
   binary32 stores and retained PC53 reciprocals for final unit-vector products.
   Zero sums intentionally produceInf reciprocal andNaN normalized components
   under the masked-exception profile; no zero-vector fallback is invented.
   Source capacity/binding guards reject before mutation. */
int __cdecl bh_actor_basis_build_groups(BHActorBasisContext *);
/* Complete00413f80: increment reference count on every call, initialize only
   when old count was0. Appends14 samples then invokes actual group builder.
   Original normalize_to leaves sample0.direction untouched for zero input;
   owners initialize array storage before first use if they want BSS semantics.
   Source first-call capacity/binding guards reject before increment/mutation.
   Caller PC53/nearest/all-exceptions-masked profile required. */
int __cdecl bh_actor_basis_acquire(BHActorBasisContext *);
#endif
