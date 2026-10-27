#ifndef BH_CAMERA_CURVE_H
#define BH_CAMERA_CURVE_H
#include "bh_math.h"
/* Complete 004341a0. For 0<t<1, smoothstep blends the two linearly
   interpolated endpoint pairs. Outside that interval, extrapolate the first
   pair at t<=0 and the second pair at t>=1. This is not a cubic Bezier curve.
   Retains original binary32 intermediate stores and PC53 grouping; moderate
   finite inputs required. Output may equal an entire input vector; partial
   overlapping vectors are unsupported. No globals or original code needed. */
void __cdecl bh_camera_curve(float t, BHVec3 *out, const BHVec3 *first, const BHVec3 *first_tangent,
                             const BHVec3 *last_tangent, const BHVec3 *last);
#endif
