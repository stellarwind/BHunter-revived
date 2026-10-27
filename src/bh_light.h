#ifndef BH_LIGHT_H
#define BH_LIGHT_H
#include "bh_math.h"
/* Original 0x5c light record. Fields at00/08 are retained opaque metadata;
   this module borrows the list and does not claim its construction/lifetime. */
typedef struct BHLight BHLight;
struct BHLight
{
    unsigned long opaque00;
    BHLight *next;
    unsigned long opaque08, type, color;
    float intensity, range, inner_cos, outer_cos, effective_intensity;
    BHVec3 position, direction, local_position, local_direction;
    unsigned long prepared_type;
};
typedef struct BHLightContext
{
    BHLight *lights;
    BHMat34 world_to_model;
    const BHVec3 *reference_override;
    BHVec3 local_reference;
} BHLightContext;
/* Complete00450980 over explicit bindings for original globals. The original
   uses the untransformed override for distances and separately transforms it
   for later evaluation, including when the list is empty. Types1/unknown and
   inactive caches remain unchanged. Caller provides a live, acyclic borrowed
   list and PC53/nearest/masked FP environment. Source preflight rejects null
   required pointers, cycles and nonfinite consumed input without mutation.
   No light allocation, shading evaluator or renderer dispatch is claimed. */
int __cdecl bh_light_prepare(BHLightContext *, const BHVec3 *reference, float radius);
#endif
