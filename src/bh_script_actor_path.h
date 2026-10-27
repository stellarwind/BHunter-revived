#ifndef BH_SCRIPT_ACTOR_PATH_H
#define BH_SCRIPT_ACTOR_PATH_H
#include "bh_math.h"
/* Complete004363f0. Count-zero linear ABI bridge invokes the existing whole
   00436670 bh_vec3_interpolate owner, accepting six arguments and ignoring
   the final controls. Borrowed vectors must remain
   valid through the call. Output may alias an input: x is stored before y
   and z, matching the original. PC53/nearest/masked profile; PC24 unresolved. */
void __cdecl bh_script_actor_curve(float t, BHVec3 *out, const BHVec3 *start, const BHVec3 *first, const BHVec3 *second,
                                   const BHVec3 *end);
void __cdecl bh_script_actor_linear(float t, BHVec3 *out, const BHVec3 *start, const BHVec3 *end, const BHVec3 *unused1,
                                    const BHVec3 *unused2);
#endif
